#include "web_server.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <AsyncJson.h>
#include <cstddef>

#include "can_manager.h"
#include "config_manager.h"
#include "ipm1_can_system.h"
#include "ota_manager.h"
#include "ui_builder.h"
#include "suspension_page_template.h"
#include "version_auto.h"
#include "web_interface.h"
#include "behavioral_output_integration.h"

namespace {
const IPAddress kApIp(192, 168, 4, 250);
const IPAddress kApGateway(192, 168, 4, 250);
const IPAddress kApMask(255, 255, 255, 0);
constexpr std::size_t kConfigJsonLimit = 2097152;  // 2MB to allow larger configs (base64 assets)
constexpr std::size_t kWifiConnectJsonLimit = 1024;
constexpr std::size_t kImageUploadJsonLimit = 2097152;  // 2MB limit for header/base64 payloads
constexpr std::size_t kImageUploadContentLimit = 2097152;
constexpr std::uint32_t kWifiReconfigureDelayMs = 750;  // Allow HTTP responses to finish before toggling radios

const char* AuthModeToString(wifi_auth_mode_t mode) {
    switch (mode) {
        case WIFI_AUTH_OPEN: return "open";
        case WIFI_AUTH_WEP: return "wep";
        case WIFI_AUTH_WPA_PSK: return "wpa";
        case WIFI_AUTH_WPA2_PSK: return "wpa2";
        case WIFI_AUTH_WPA_WPA2_PSK: return "wpa_wpa2";
        case WIFI_AUTH_WPA2_ENTERPRISE: return "wpa2_enterprise";
        case WIFI_AUTH_WPA3_PSK: return "wpa3";
        case WIFI_AUTH_WPA2_WPA3_PSK: return "wpa2_wpa3";
        case WIFI_AUTH_WAPI_PSK: return "wapi";
        default: return "unknown";
    }
}

bool WifiConfigEquals(const WifiConfig& lhs, const WifiConfig& rhs) {
    const auto creds_equal = [](const WifiCredentials& a, const WifiCredentials& b) {
        return a.enabled == b.enabled && a.ssid == b.ssid && a.password == b.password;
    };
    return creds_equal(lhs.ap, rhs.ap) && creds_equal(lhs.sta, rhs.sta);
}
}

WebServerManager& WebServerManager::instance() {
    static WebServerManager server;
    return server;
}

WebServerManager::WebServerManager()
    : server_(80), can_monitor_ws_("/ws/can") {}

void WebServerManager::begin() {
    static bool dns_configured = false;
    if (!events_registered_) {
        WiFi.onEvent([this](WiFiEvent_t event, WiFiEventInfo_t info) {
            if (event == ARDUINO_EVENT_WIFI_STA_GOT_IP) {
                sta_connected_ = true;
                sta_ip_ = WiFi.localIP();
                sta_ssid_ = WiFi.SSID().c_str();
                // Set DNS servers AFTER getting IP to prevent DHCP from overwriting
                // Only do this once to avoid triggering more events
                if (!dns_configured) {
                    WiFi.config(WiFi.localIP(), WiFi.gatewayIP(), WiFi.subnetMask(), 
                               IPAddress(8, 8, 8, 8), IPAddress(1, 1, 1, 1));
                    dns_configured = true;
                    Serial.println("[WiFi] DNS configured to 8.8.8.8 (primary) and 1.1.1.1 (secondary)");
                }
                Serial.printf("[WebServer] Station connected: %s\n", sta_ip_.toString().c_str());
            } else if (event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED) {
                sta_connected_ = false;
                sta_ip_ = IPAddress(0, 0, 0, 0);
                sta_ssid_.clear();
                Serial.println("[WebServer] Station disconnected");
            }
        });
        events_registered_ = true;
    }

    configureWifi();
    setupRoutes();
    server_.begin();
    Serial.println("[WebServer] HTTP server started on port 80");
}

void WebServerManager::loop() {
    // Process DNS requests for captive portal
    if (dns_active_) {
        dns_server_.processNextRequest();
    }
    
    // Clean up WebSocket connections
    can_monitor_ws_.cleanupClients();

    if (wifi_reconfigure_pending_) {
        const std::uint32_t now = millis();
        if (now - wifi_reconfigure_request_ms_ >= kWifiReconfigureDelayMs) {
            wifi_reconfigure_pending_ = false;
            configureWifi();
        }
    }
}

void WebServerManager::notifyConfigChanged() {
    wifi_reconfigure_pending_ = true;
    wifi_reconfigure_request_ms_ = millis();
}

void WebServerManager::configureWifi() {
    auto& wifi = ConfigManager::instance().getConfig().wifi;
    WiFi.mode(WIFI_MODE_APSTA);

    const bool sta_configured = wifi.sta.enabled && !wifi.sta.ssid.empty();
    if ((!wifi.ap.enabled || ap_suppressed_) && !sta_configured) {
        Serial.println("[WebServer] WARNING: Station credentials missing. Enabling fallback AP.");
        wifi.ap.enabled = true;
        ap_suppressed_ = false;
        if (wifi.ap.ssid.empty()) {
            wifi.ap.ssid = "CAN-Control";
        }
        ConfigManager::instance().save();
    }

    if (ap_suppressed_ && !sta_connected_) {
        Serial.println("[WebServer] STA disconnected. Re-enabling AP for recovery.");
        ap_suppressed_ = false;
    }

    if (wifi.ap.enabled && !ap_suppressed_) {
        const char* password = nullptr;
        if (wifi.ap.password.length() >= 8) {
            password = wifi.ap.password.c_str();
        }
        WiFi.softAPdisconnect(true);
        if (!WiFi.softAPConfig(kApIp, kApGateway, kApMask)) {
            Serial.println("[WebServer] Failed to set AP IP config");
        }
        
        Serial.printf("[WebServer] Starting AP - SSID: %s, Password: %s\n", 
                     wifi.ap.ssid.c_str(), password ? password : "(none - open network)");
        
        if (!WiFi.softAP(wifi.ap.ssid.c_str(), password)) {
            Serial.println("[WebServer] Failed to start access point");
        }
        ap_ip_ = WiFi.softAPIP();
        Serial.printf("[WebServer] AP ready at %s\n", ap_ip_.toString().c_str());
        if (!dns_active_) {
            dns_server_.start(53, "*", kApIp);
            dns_active_ = true;
            Serial.println("[WebServer] Captive portal DNS active");
        }
    } else {
        WiFi.softAPdisconnect(true);
        ap_ip_ = IPAddress(0, 0, 0, 0);
        if (dns_active_) {
            dns_server_.stop();
            dns_active_ = false;
            Serial.println("[WebServer] Captive portal DNS stopped");
        }
    }

    if (wifi.sta.enabled && !wifi.sta.ssid.empty()) {
        Serial.printf("[WebServer] Connecting to %s...\n", wifi.sta.ssid.c_str());
        sta_connected_ = false;
        sta_ssid_.clear();
        WiFi.begin(wifi.sta.ssid.c_str(), wifi.sta.password.c_str());
    } else {
        WiFi.disconnect(true);
        sta_connected_ = false;
        sta_ip_ = IPAddress(0, 0, 0, 0);
        sta_ssid_.clear();
    }
}

void WebServerManager::setupRoutes() {
    // Set up CAN monitoring WebSocket
    can_monitor_ws_.onEvent([](AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type,
                                void* arg, uint8_t* data, size_t len) {
        if (type == WS_EVT_CONNECT) {
            Serial.printf("[WebSocket] CAN monitor client connected: %u from %s\n", 
                         client->id(), client->remoteIP().toString().c_str());
            // Send initial status
            DynamicJsonDocument doc(256);
            doc["type"] = "status";
            doc["message"] = "CAN monitor connected";
            doc["bus_ready"] = CanManager::instance().isReady();
            String msg;
            serializeJson(doc, msg);
            client->text(msg);
        } else if (type == WS_EVT_DISCONNECT) {
            Serial.printf("[WebSocket] CAN monitor client disconnected: %u\n", client->id());
        } else if (type == WS_EVT_ERROR) {
            Serial.printf("[WebSocket] Error from client %u\n", client->id());
        }
    });
    server_.addHandler(&can_monitor_ws_);
    
    // Log ALL incoming requests for debugging
    server_.onNotFound([](AsyncWebServerRequest* request) {
        Serial.printf("[WEB] 404: %s %s\n", request->methodToString(), request->url().c_str());
        request->send(404, "text/plain", "Not Found");
    });
    
    // Captive portal detection endpoints - return wrong content to trigger portal
    // iOS and macOS - expects "Success" but we return wrong content to trigger portal
    server_.on("/hotspot-detect.html", HTTP_GET, [](AsyncWebServerRequest* request) {
        Serial.println("[WEB] GET /hotspot-detect.html");
        String redirectUrl = "http://" + request->host() + "/";
        AsyncWebServerResponse* response = request->beginResponse(200, "text/html", 
            "<!DOCTYPE html><html><head><meta http-equiv='refresh' content='0; url=" + redirectUrl + "'></head><body></body></html>");
        response->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
        response->addHeader("Pragma", "no-cache");
        response->addHeader("Expires", "0");
        request->send(response);
    });
    server_.on("/library/test/success.html", HTTP_GET, [](AsyncWebServerRequest* request) {
        String redirectUrl = "http://" + request->host() + "/";
        AsyncWebServerResponse* response = request->beginResponse(200, "text/html", 
            "<!DOCTYPE html><html><head><meta http-equiv='refresh' content='0; url=" + redirectUrl + "'></head><body></body></html>");
        response->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
        request->send(response);
    });
    // Android - expects 204 No Content, we return different to trigger portal
    server_.on("/generate_204", HTTP_GET, [](AsyncWebServerRequest* request) {
        String redirectUrl = "http://" + request->host() + "/";
        request->redirect(redirectUrl);
    });
    // Windows connectivity tests
    server_.on("/connecttest.txt", HTTP_GET, [](AsyncWebServerRequest* request) {
        String redirectUrl = "http://" + request->host() + "/";
        request->redirect(redirectUrl);
    });
    server_.on("/ncsi.txt", HTTP_GET, [](AsyncWebServerRequest* request) {
        String redirectUrl = "http://" + request->host() + "/";
        request->redirect(redirectUrl);
    });
    server_.on("/redirect", HTTP_GET, [](AsyncWebServerRequest* request) {
        String redirectUrl = "http://" + request->host() + "/";
        request->redirect(redirectUrl);
    });
    // Additional Microsoft connectivity endpoints
    server_.on("/connectivity-check", HTTP_GET, [](AsyncWebServerRequest* request) {
        String redirectUrl = "http://" + request->host() + "/";
        request->redirect(redirectUrl);
    });
    server_.on("/microsoft-connectivity-check", HTTP_GET, [](AsyncWebServerRequest* request) {
        String redirectUrl = "http://" + request->host() + "/";
        request->redirect(redirectUrl);
    });
    
    // Main configuration page
    server_.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
        // Generate HTML with version embedded
        String html = FPSTR(WEB_INTERFACE_HTML);
        html.replace("{{VERSION}}", APP_VERSION);
        AsyncWebServerResponse* response = request->beginResponse(200, "text/html", html.c_str());
        response->addHeader("Cache-Control", "no-cache, no-store, must-revalidate, max-age=0");
        response->addHeader("Pragma", "no-cache");
        response->addHeader("Expires", "0");
        response->addHeader("ETag", String(millis()).c_str());  // Force fresh content
        request->send(response);
    });

    server_.on("/api/status", HTTP_GET, [this](AsyncWebServerRequest* request) {
        DynamicJsonDocument doc(384);
        doc["firmware_version"] = APP_VERSION;
        doc["ap_ip"] = ap_ip_.toString();
        doc["sta_ip"] = sta_ip_.toString();
        doc["sta_connected"] = sta_connected_;

        std::string device_ip;
        if (sta_connected_ && sta_ip_ != IPAddress(0, 0, 0, 0)) {
            device_ip = sta_ip_.toString().c_str();
        } else if (ap_ip_ != IPAddress(0, 0, 0, 0)) {
            device_ip = ap_ip_.toString().c_str();
        }
        doc["device_ip"] = device_ip.c_str();

        std::string connected_network;
        if (sta_connected_) {
            if (!sta_ssid_.empty()) {
                connected_network = sta_ssid_;
            } else {
                const auto& cfg = ConfigManager::instance().getConfig().wifi;
                connected_network = cfg.sta.ssid.empty() ? "Hidden network" : cfg.sta.ssid;
            }
        } else if (ap_ip_ != IPAddress(0, 0, 0, 0)) {
            const auto& cfg = ConfigManager::instance().getConfig().wifi;
            std::string ap_ssid = cfg.ap.ssid.empty() ? "CAN-Control" : cfg.ap.ssid;
            connected_network = "AP: " + ap_ssid;
        }
        doc["connected_network"] = connected_network.c_str();

        doc["uptime_ms"] = millis();
        doc["heap"] = ESP.getFreeHeap();
        String payload;
        serializeJson(doc, payload);
        request->send(200, "application/json", payload);
    });

    // Behavioral output/scene lists for button configuration
    server_.on("/api/behavioral/options", HTTP_GET, [](AsyncWebServerRequest* request) {
        DynamicJsonDocument doc(4096);
        
        // Get available outputs
        JsonArray outputs = doc.createNestedArray("outputs");
        auto outputList = behaviorEngine.getAllOutputs();
        for (const auto& id : outputList) {
            auto* output = behaviorEngine.getOutput(id);
            if (output) {
                JsonObject obj = outputs.createNestedObject();
                obj["id"] = output->id;
                obj["name"] = output->name;
                obj["description"] = output->description;
            }
        }
        
        // Get available scenes  
        JsonArray scenes = doc.createNestedArray("scenes");
        auto sceneList = behaviorEngine.getAllScenes();
        for (const auto& id : sceneList) {
            auto& allScenes = behaviorEngine.getScenes();
            auto it = allScenes.find(id);
            if (it != allScenes.end()) {
                JsonObject obj = scenes.createNestedObject();
                obj["id"] = it->second.id;
                obj["name"] = it->second.name;
                obj["description"] = it->second.description;
            }
        }
        
        // Behavior types for output mode
        JsonArray behaviors = doc.createNestedArray("behavior_types");
        behaviors.add("steady");
        behaviors.add("flash");
        behaviors.add("pulse");
        behaviors.add("fade_in");
        behaviors.add("fade_out");
        behaviors.add("strobe");
        behaviors.add("hold_timed");
        behaviors.add("ramp");
        
        String payload;
        serializeJson(doc, payload);
        request->send(200, "application/json", payload);
    });

    // DEBUG: Direct output test endpoint
    server_.on("/api/test/output1", HTTP_GET, [](AsyncWebServerRequest* request) {
        // Directly activate left_turn_front via behavioral engine
        extern BehaviorEngine behaviorEngine;
        BehavioralOutput::BehaviorConfig cfg;
        cfg.type = BehavioralOutput::BehaviorType::STEADY;
        cfg.targetValue = 255;
        bool success = behaviorEngine.setBehavior("left_turn_front", cfg);
        request->send(200, "text/plain", success ? "Output 1 ON" : "Failed");
    });
    
    server_.on("/api/test/output1/off", HTTP_GET, [](AsyncWebServerRequest* request) {
        extern BehaviorEngine behaviorEngine;
        behaviorEngine.deactivateOutput("left_turn_front");
        request->send(200, "text/plain", "Output 1 OFF");
    });
    
    server_.on("/api/config", HTTP_GET, [](AsyncWebServerRequest* request) {
        std::string json = ConfigManager::instance().toJson();
        String payload(json.c_str());
        request->send(200, "application/json", payload);
    });

    auto* handler = new AsyncCallbackJsonWebHandler("/api/config",
        [this](AsyncWebServerRequest* request, JsonVariant& json) {
            auto& config_mgr = ConfigManager::instance();
            WifiConfig previous_wifi = config_mgr.getConfig().wifi;
            std::string error;
            if (!config_mgr.updateFromJson(json.as<JsonVariantConst>(), error)) {
                DynamicJsonDocument doc(256);
                doc["status"] = "error";
                doc["message"] = error.c_str();
                String payload;
                serializeJson(doc, payload);
                request->send(400, "application/json", payload);
                return;
            }

            const bool wifi_changed = !WifiConfigEquals(previous_wifi, config_mgr.getConfig().wifi);

            if (!config_mgr.save()) {
                request->send(500, "application/json", "{\"status\":\"error\",\"message\":\"Failed to persist\"}");
                return;
            }

            UIBuilder::instance().markDirty();

            if (wifi_changed) {
                request->onDisconnect([this]() {
                    notifyConfigChanged();
                });
            }

            DynamicJsonDocument doc(64);
            doc["status"] = "ok";
            String payload;
            serializeJson(doc, payload);
            request->send(200, "application/json", payload);
        }, kConfigJsonLimit);
    handler->setMaxContentLength(kConfigJsonLimit);  // CRITICAL: Also set max content length to 2MB
    server_.addHandler(handler);

    auto* wifi_handler = new AsyncCallbackJsonWebHandler("/api/wifi/connect",
        [this](AsyncWebServerRequest* request, JsonVariant& json) {
            String ssid = json["ssid"] | "";
            String password = json["password"] | "";
            bool persist = json["persist"] | true;

            if (ssid.isEmpty()) {
                DynamicJsonDocument doc(128);
                doc["status"] = "error";
                doc["message"] = "SSID is required";
                String payload;
                serializeJson(doc, payload);
                request->send(400, "application/json", payload);
                return;
            }

            auto& cfg = ConfigManager::instance().getConfig();
            cfg.wifi.sta.enabled = true;
            cfg.wifi.sta.ssid = ssid.c_str();
            cfg.wifi.sta.password = password.c_str();

            if (persist && !ConfigManager::instance().save()) {
                request->send(500, "application/json", "{\"status\":\"error\",\"message\":\"Failed to persist\"}");
                return;
            }

            request->onDisconnect([this]() {
                notifyConfigChanged();
            });

            DynamicJsonDocument doc(160);
            doc["status"] = "connecting";
            doc["ssid"] = cfg.wifi.sta.ssid.c_str();
            String payload;
            serializeJson(doc, payload);
            request->send(200, "application/json", payload);
        }, kWifiConnectJsonLimit);
    server_.addHandler(wifi_handler);

    // Dedicated image upload endpoints (separate from main config to avoid size limits)
    auto* image_handler = new AsyncCallbackJsonWebHandler("/api/image/upload",
        [this](AsyncWebServerRequest* request, JsonVariant& json) {
            String imageType = json["type"] | "";
            String imageData = json["data"] | "";
            
            if (imageType.isEmpty()) {
                request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Missing type\"}");
                return;
            }
            
            // Allow empty data for clearing images
            Serial.printf("[WebServer] Image upload: type=%s, data_length=%d\n", imageType.c_str(), imageData.length());
            
            auto& cfg = ConfigManager::instance().getConfig();
            
            // Store image in appropriate field
            if (imageType == "header") {
                cfg.images.header_logo = imageData.c_str();
                // Toggle logo display based on whether we have data
                cfg.header.show_logo = !imageData.isEmpty();
                // Clear logo_variant when custom header is uploaded
                if (!imageData.isEmpty()) {
                    cfg.header.logo_variant = "";
                }
            } else if (imageType == "splash") {
                cfg.images.splash_logo = imageData.c_str();
            } else if (imageType == "background") {
                cfg.images.background_image = imageData.c_str();
            } else if (imageType == "sleep") {
                cfg.images.sleep_logo = imageData.c_str();
            } else {
                request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid image type\"}");
                return;
            }
            
            if (!ConfigManager::instance().save()) {
                request->send(500, "application/json", "{\"status\":\"error\",\"message\":\"Failed to save\"}");
                return;
            }
            
            UIBuilder::instance().markDirty();
            
            DynamicJsonDocument doc(64);
            doc["status"] = "ok";
            String payload;
            serializeJson(doc, payload);
            request->send(200, "application/json", payload);
        }, kImageUploadJsonLimit);
    image_handler->setMaxContentLength(kImageUploadContentLimit);
    server_.addHandler(image_handler);

    server_.on("/api/wifi/scan", HTTP_GET, [](AsyncWebServerRequest* request) {
        const int16_t count = WiFi.scanNetworks(/*async=*/false, /*show_hidden=*/true);
        if (count < 0) {
            request->send(500, "application/json", "{\"status\":\"error\",\"message\":\"Scan failed\"}");
            return;
        }

        DynamicJsonDocument doc(4096);
        doc["status"] = "ok";
        doc["count"] = count;
        JsonArray networks = doc.createNestedArray("networks");
        for (int16_t i = 0; i < count; ++i) {
            const String ssid = WiFi.SSID(i);
            JsonObject entry = networks.createNestedObject();
            entry["ssid"] = ssid;
            entry["rssi"] = WiFi.RSSI(i);
            entry["channel"] = WiFi.channel(i);
            entry["bssid"] = WiFi.BSSIDstr(i);
            const wifi_auth_mode_t auth = WiFi.encryptionType(i);
            entry["secure"] = auth != WIFI_AUTH_OPEN;
            entry["auth"] = AuthModeToString(auth);
            entry["hidden"] = ssid.isEmpty();
        }
        WiFi.scanDelete();

        String payload;
        serializeJson(doc, payload);
        request->send(200, "application/json", payload);
    });

    // OTA Update Endpoints
    server_.on("/api/ota/check", HTTP_GET, [](AsyncWebServerRequest* request) {
        DynamicJsonDocument doc(256);
        OTAUpdateManager& ota = OTAUpdateManager::instance();
        ota.checkForUpdatesNow();

        const std::string status = ota.lastStatus();
        bool update_available = false;
        std::string available_version;

        const std::string kUpdatePrefix = "update-available-";
        const std::string kDownloadingPrefix = "downloading-";
        if (status.rfind(kUpdatePrefix, 0) == 0) {
            update_available = true;
            available_version = status.substr(kUpdatePrefix.size());
        } else if (status.rfind(kDownloadingPrefix, 0) == 0) {
            available_version = status.substr(kDownloadingPrefix.size());
        } else if (status == "up-to-date") {
            available_version = APP_VERSION;
        }

        doc["status"] = status.c_str();
        doc["update_available"] = update_available;
        doc["current_version"] = APP_VERSION;
        doc["available_version"] = available_version.c_str();

        String payload;
        serializeJson(doc, payload);
        request->send(200, "application/json", payload);
    });

    server_.on("/api/ota/update", HTTP_POST, [](AsyncWebServerRequest* request) {
        OTAUpdateManager::instance().triggerImmediateCheck(true);
        DynamicJsonDocument doc(128);
        doc["status"] = "ok";
        doc["message"] = "Update triggered";
        String payload;
        serializeJson(doc, payload);
        request->send(200, "application/json", payload);
    });

    // GitHub version listing endpoint
    server_.on("/api/ota/github/versions", HTTP_GET, [](AsyncWebServerRequest* request) {
        Serial.println("[WEB] /api/ota/github/versions endpoint called");
        std::vector<std::string> versions;
        bool success = OTAUpdateManager::instance().checkGitHubVersions(versions);
        Serial.printf("[WEB] checkGitHubVersions returned: %s, found %d versions\n", 
                     success ? "SUCCESS" : "FAILED", versions.size());
        
        DynamicJsonDocument doc(4096);
        doc["status"] = success ? "ok" : "error";
        
        if (success) {
            JsonArray arr = doc.createNestedArray("versions");
            for (const auto& ver : versions) {
                arr.add(ver);
                Serial.printf("[WEB] Adding version: %s\n", ver.c_str());
            }
            doc["count"] = versions.size();
            doc["current"] = APP_VERSION;
        } else {
            doc["message"] = "Failed to fetch GitHub versions";
        }
        
        String payload;
        serializeJson(doc, payload);
        Serial.printf("[WEB] Sending response: %s\n", payload.c_str());
        request->send(success ? 200 : 500, "application/json", payload);
    });

    // OTA status endpoint for polling during update
    server_.on("/api/ota/status", HTTP_GET, [](AsyncWebServerRequest* request) {
        DynamicJsonDocument doc(512);
        OTAUpdateManager& ota = OTAUpdateManager::instance();
        
        doc["status"] = ota.lastStatus().c_str();
        doc["message"] = ota.lastStatusMessage().c_str();
        doc["progress"] = ota.lastProgress();
        doc["version"] = APP_VERSION;
        
        // Determine if update is in progress
        const std::string& status = ota.lastStatus();
        bool in_progress = (status.find("downloading") != std::string::npos ||
                          status.find("installing") != std::string::npos ||
                          status.find("update") != std::string::npos);
        doc["in_progress"] = in_progress;
        
        String payload;
        serializeJson(doc, payload);
        request->send(200, "application/json", payload);
    });

    // GitHub OTA install endpoint
    server_.on("/api/ota/github/install", HTTP_POST, [](AsyncWebServerRequest* request) {}, nullptr,
        [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            Serial.println("[WEB] /api/ota/github/install endpoint called");
            
            DynamicJsonDocument doc(256);
            DeserializationError error = deserializeJson(doc, data, len);
            
            if (error) {
                Serial.println("[WEB] ERROR: Invalid JSON");
                request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
                return;
            }
            
            if (!doc.containsKey("version")) {
                Serial.println("[WEB] ERROR: Missing version parameter");
                request->send(400, "application/json", "{\"error\":\"Missing version parameter\"}");
                return;
            }
            
            std::string version = doc["version"].as<std::string>();
            Serial.printf("[WEB] OTA install requested for version: %s\n", version.c_str());
            
            // Send immediate response
            DynamicJsonDocument response(128);
            response["status"] = "ok";
            response["message"] = "OTA update started";
            response["version"] = version;
            String payload;
            serializeJson(response, payload);
            request->send(200, "application/json", payload);
            
            // Start update in separate task (will reboot on success)
            Serial.println("[WEB] Calling installVersionFromGitHubAsync...");
            OTAUpdateManager::instance().installVersionFromGitHubAsync(version);
            Serial.println("[WEB] installVersionFromGitHubAsync returned");
        });

    // Test CAN frame endpoint
    server_.on("/api/can/send", HTTP_POST, [](AsyncWebServerRequest* request) {}, nullptr,
        [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            DynamicJsonDocument doc(512);
            DeserializationError error = deserializeJson(doc, data, len);
            
            if (error) {
                request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
                return;
            }

            CanFrameConfig frame;
            frame.enabled = true;
            frame.pgn = doc["pgn"] | 0xFF01;
            frame.priority = doc["priority"] | 6;
            frame.source_address = doc["source"] | 0xF9;
            frame.destination_address = doc["destination"] | 0xFF;
            
            JsonArray dataArray = doc["data"];
            if (dataArray.isNull()) {
                request->send(400, "application/json", "{\"error\":\"Missing data array\"}");
                return;
            }
            
            size_t idx = 0;
            for (JsonVariant v : dataArray) {
                if (idx >= frame.data.size()) break;
                frame.data[idx++] = v.as<uint8_t>();
            }
            frame.length = static_cast<uint8_t>(idx);  // Set actual data length

            bool success = CanManager::instance().sendFrame(frame);
            
            DynamicJsonDocument response(256);
            response["success"] = success;
            response["pgn"] = String(frame.pgn, HEX);
            response["bytes"] = idx;
            
            String payload;
            serializeJson(response, payload);
            request->send(success ? 200 : 500, "application/json", payload);
        });

    // Receive CAN messages endpoint
    server_.on("/api/can/receive", HTTP_GET, [](AsyncWebServerRequest* request) {
        uint32_t timeout = 500;
        if (request->hasParam("timeout")) {
            timeout = request->getParam("timeout")->value().toInt();
        }

        std::vector<CanRxMessage> messages = CanManager::instance().receiveAll(timeout);
        
        DynamicJsonDocument doc(4096);
        JsonArray array = doc.createNestedArray("messages");
        
        for (const auto& msg : messages) {
            JsonObject msgObj = array.createNestedObject();
            msgObj["id"] = String(msg.identifier, HEX);
            msgObj["timestamp"] = msg.timestamp;
            
            JsonArray dataArray = msgObj.createNestedArray("data");
            for (uint8_t i = 0; i < msg.length; i++) {
                dataArray.add(msg.data[i]);
            }
        }
        
        doc["count"] = messages.size();
        
        String payload;
        serializeJson(doc, payload);
        request->send(200, "application/json", payload);
    });

    // IPM1 system definition (UI contract)
    server_.on("/api/ipm1/system", HTTP_GET, [](AsyncWebServerRequest* request) {
        String payload = Ipm1CanSystem::instance().getSystemJson();
        request->send(200, "application/json", payload);
    });

    // IPM1 action endpoint (circuit-first)
    auto* ipm1_action = new AsyncCallbackJsonWebHandler("/api/ipm1/action",
        [](AsyncWebServerRequest* request, JsonVariant& json) {
            DynamicJsonDocument response(512);
            JsonObject resp = response.to<JsonObject>();
            String error;
            bool success = Ipm1CanSystem::instance().handleAction(json, error, resp);
            resp["success"] = success;
            if (!success) {
                resp["error"] = error;
            }
            String payload;
            serializeJson(response, payload);
            request->send(success ? 200 : 400, "application/json", payload);
        });
    server_.addHandler(ipm1_action);

    // Suspension template preview (static HTML)
    server_.on("/suspension", HTTP_GET, [](AsyncWebServerRequest* request) {
        AsyncWebServerResponse* response = request->beginResponse_P(200, "text/html", SUSPENSION_PAGE_HTML);
        response->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
        request->send(response);
    });

    // Captive portal - redirect all unknown requests to main page
    server_.onNotFound([](AsyncWebServerRequest* request) {
        // For API calls, return 404
        if (request->url().startsWith("/api/")) {
            request->send(404, "application/json", "{\"error\":\"Not found\"}");
        } else {
            // For all other requests, redirect to captive portal with 302 redirect
            request->redirect("http://192.168.4.250/");
        }
    });
    
    // CAN Monitor page
    server_.on("/can-monitor", HTTP_GET, [](AsyncWebServerRequest* request) {
        const char* html = R"html(<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <title>CAN Bus Monitor - Bronco Controls</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body { 
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
            background: #1a1a1a; 
            color: #e0e0e0; 
            padding: 20px;
        }
        .header {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            padding: 20px;
            border-radius: 12px;
            margin-bottom: 20px;
            box-shadow: 0 4px 6px rgba(0,0,0,0.3);
        }
        .top-nav {
            display: flex;
            gap: 10px;
            margin: 14px 0 0 0;
            flex-wrap: wrap;
        }
        .top-nav button {
            background: rgba(255,255,255,0.15);
            border: 1px solid rgba(255,255,255,0.2);
        }
        .top-nav button.active {
            background: rgba(255,255,255,0.35);
            border-color: rgba(255,255,255,0.5);
        }
        h1 { font-size: 28px; font-weight: 600; }
        .status {
            display: flex;
            gap: 20px;
            margin-bottom: 20px;
            flex-wrap: wrap;
        }
        .status-card {
            background: #2a2a2a;
            padding: 15px 20px;
            border-radius: 8px;
            flex: 1;
            min-width: 150px;
            border-left: 4px solid #667eea;
        }
        .status-label { 
            font-size: 12px; 
            color: #888;
            text-transform: uppercase;
            letter-spacing: 0.5px;
        }
        .status-value { 
            font-size: 24px; 
            font-weight: 600;
            margin-top: 5px;
        }
        .connected { color: #4ade80; }
        .disconnected { color: #f87171; }
        .controls {
            background: #2a2a2a;
            padding: 15px;
            border-radius: 8px;
            margin-bottom: 20px;
            display: flex;
            gap: 10px;
            flex-wrap: wrap;
            align-items: center;
        }
        button {
            background: #667eea;
            color: white;
            border: none;
            padding: 10px 20px;
            border-radius: 6px;
            cursor: pointer;
            font-size: 14px;
            font-weight: 500;
            transition: background 0.2s;
        }
        button:hover { background: #5568d3; }
        button:active { transform: scale(0.98); }
        button.danger { background: #ef4444; }
        button.danger:hover { background: #dc2626; }
        label {
            display: flex;
            align-items: center;
            gap: 8px;
            font-size: 14px;
        }
        input[type="checkbox"] {
            width: 18px;
            height: 18px;
            cursor: pointer;
        }
        .frame-container {
            background: #2a2a2a;
            border-radius: 8px;
            overflow: hidden;
            max-height: 600px;
            overflow-y: auto;
        }
        table {
            width: 100%;
            border-collapse: collapse;
        }
        th {
            background: #1f1f1f;
            padding: 12px;
            text-align: left;
            font-size: 12px;
            font-weight: 600;
            text-transform: uppercase;
            letter-spacing: 0.5px;
            position: sticky;
            top: 0;
            z-index: 10;
        }
        td {
            padding: 10px 12px;
            border-top: 1px solid #333;
            font-family: 'Courier New', monospace;
            font-size: 13px;
        }
        tr:hover { background: #333; }
        .frame-id { color: #fbbf24; }
        .frame-pgn { color: #60a5fa; }
        .frame-data { 
            color: #a3e635;
            word-break: break-all;
        }
        .frame-decode { 
            color: #c084fc; 
            font-size: 11px;
            font-family: sans-serif;
            line-height: 1.4;
        }
        .timestamp { color: #94a3b8; font-size: 11px; }
        .no-frames {
            text-align: center;
            padding: 60px 20px;
            color: #666;
        }
        @media (max-width: 768px) {
            .header { padding: 15px; }
            h1 { font-size: 22px; }
            .status { flex-direction: column; }
            table { font-size: 11px; }
            th, td { padding: 8px; }
        }
    </style>
</head>
<body>
    <div class="header">
        <h1>🚗 CAN Bus Monitor</h1>
        <p style="margin-top:8px; opacity:0.9;">Real-time POWERCELL NGX Frame Analysis</p>
        <div class="top-nav">
            <button onclick="location.href='/'">🏠 Configurator</button>
            <button class="active" onclick="location.href='/can-monitor'">📡 CAN Monitor</button>
            <button onclick="location.href='/behavioral'">🎛️ Behavioral Outputs</button>
        </div>
    </div>
    
    <div class="status">
        <div class="status-card">
            <div class="status-label">WebSocket</div>
            <div class="status-value" id="ws-status">⏳ Connecting...</div>
        </div>
        <div class="status-card">
            <div class="status-label">Frames Received</div>
            <div class="status-value" id="frame-count">0</div>
        </div>
        <div class="status-card">
            <div class="status-label">Frame Rate</div>
            <div class="status-value" id="frame-rate">0 /s</div>
        </div>
    </div>
    
    <div class="controls">
        <button onclick="clearFrames()">🗑️ Clear</button>
        <button onclick="pauseToggle()" id="pause-btn">⏸️ Pause</button>
        <button onclick="location.href='/'" class="danger">🏠 Back to Home</button>
        <label>
            <input type="checkbox" id="auto-scroll" checked>
            Auto-scroll
        </label>
        <label>
            <input type="checkbox" id="decode-frames" checked>
            Decode POWERCELL
        </label>
    </div>
    
    <div class="frame-container" id="frame-container">
        <table>
            <thead>
                <tr>
                    <th style="width:80px">Time</th>
                    <th style="width:120px">ID (Hex)</th>
                    <th style="width:100px">PGN</th>
                    <th style="width:50px">SA</th>
                    <th style="width:50px">DA</th>
                    <th>Data (Hex)</th>
                    <th style="width:250px">Decoded</th>
                </tr>
            </thead>
            <tbody id="frame-table">
                <tr><td colspan="7" class="no-frames">Waiting for CAN frames...</td></tr>
            </tbody>
        </table>
    </div>
    
    <script>
        let ws = null;
        let frameCount = 0;
        let paused = false;
        let lastSecondCount = 0;
        let lastSecondTime = Date.now();
        
        function connect() {
            const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
            const wsUrl = protocol + '//' + window.location.host + '/ws/can';
            
            ws = new WebSocket(wsUrl);
            
            ws.onopen = () => {
                document.getElementById('ws-status').innerHTML = '<span class="connected">✓ Connected</span>';
                console.log('[WS] Connected to CAN monitor');
            };
            
            ws.onmessage = (event) => {
                if (paused) return;
                
                try {
                    const data = JSON.parse(event.data);
                    
                    if (data.type === 'can_frame') {
                        addFrame(data);
                        updateStats();
                    } else if (data.type === 'status') {
                        console.log('[WS] Status:', data.message);
                    }
                } catch (e) {
                    console.error('[WS] Parse error:', e);
                }
            };
            
            ws.onerror = (error) => {
                console.error('[WS] Error:', error);
                document.getElementById('ws-status').innerHTML = '<span class="disconnected">✗ Error</span>';
            };
            
            ws.onclose = () => {
                document.getElementById('ws-status').innerHTML = '<span class="disconnected">✗ Disconnected</span>';
                console.log('[WS] Disconnected, reconnecting in 3s...');
                setTimeout(connect, 3000);
            };
        }
        
        function addFrame(frame) {
            const tbody = document.getElementById('frame-table');
            const noFrames = tbody.querySelector('.no-frames');
            if (noFrames) tbody.innerHTML = '';
            
            const row = tbody.insertRow(0);
            
            // Parse J1939 ID
            const id = parseInt(frame.id, 16);
            const priority = (id >> 26) & 0x7;
            const pgn = (id >> 8) & 0x3FFFF;
            const sa = id & 0xFF;
            const da = (id >> 8) & 0xFF;
            
            const now = new Date();
            const timeStr = now.toLocaleTimeString() + '.' + String(now.getMilliseconds()).padStart(3, '0');
            
            row.innerHTML = `
                <td class="timestamp">${timeStr}</td>
                <td class="frame-id">${frame.id}</td>
                <td class="frame-pgn">0x${pgn.toString(16).toUpperCase()}</td>
                <td>${sa.toString(16).toUpperCase()}</td>
                <td>${da.toString(16).toUpperCase()}</td>
                <td class="frame-data">${frame.data.map(b => b.toString(16).toUpperCase().padStart(2, '0')).join(' ')}</td>
                <td class="frame-decode">${decodeFrame(pgn, frame.data)}</td>
            `;
            
            // Limit table size
            while (tbody.rows.length > 500) {
                tbody.deleteRow(tbody.rows.length - 1);
            }
            
            // Auto-scroll if enabled
            if (document.getElementById('auto-scroll').checked) {
                document.getElementById('frame-container').scrollTop = 0;
            }
            
            frameCount++;
        }
        
        function decodeFrame(pgn, data) {
            if (!document.getElementById('decode-frames').checked) return '-';
            
            // POWERCELL NGX detection (PGN 0xFF01-0xFF0A for addresses 1-10)
            if (pgn >= 0xFF01 && pgn <= 0xFF0A) {
                const addr = pgn & 0x0F;
                const deviceName = addr === 1 ? 'FRONT' : addr === 2 ? 'REAR' : `ADDR${addr}`;
                
                const outputs_1_8 = data[0] || 0;
                const outputs_9_10 = data[1] || 0;
                const softstart = data[2] || 0;
                const pwm = data[3] || 0;
                
                let desc = `POWERCELL ${deviceName}:<br>`;
                desc += `OUT1-8: ${outputs_1_8.toString(2).padStart(8,'0')}<br>`;
                desc += `OUT9-10: ${(outputs_9_10 & 0x03).toString(2).padStart(2,'0')}<br>`;
                desc += `SS: ${softstart.toString(16).toUpperCase().padStart(2,'0')} PWM: ${pwm.toString(16).toUpperCase().padStart(2,'0')}`;
                
                return desc;
            }
            
            return '-';
        }
        
        function updateStats() {
            document.getElementById('frame-count').textContent = frameCount;
            
            const now = Date.now();
            if (now - lastSecondTime >= 1000) {
                const rate = frameCount - lastSecondCount;
                document.getElementById('frame-rate').textContent = rate + ' /s';
                lastSecondCount = frameCount;
                lastSecondTime = now;
            }
        }
        
        function clearFrames() {
            document.getElementById('frame-table').innerHTML = '<tr><td colspan="7" class="no-frames">Cleared - waiting for frames...</td></tr>';
            frameCount = 0;
            lastSecondCount = 0;
            document.getElementById('frame-count').textContent = '0';
            document.getElementById('frame-rate').textContent = '0 /s';
        }
        
        function pauseToggle() {
            paused = !paused;
            const btn = document.getElementById('pause-btn');
            btn.textContent = paused ? '▶️ Resume' : '⏸️ Pause';
        }
        
        // Start connection
        connect();
    </script>
</body>
</html>)html";
        request->send(200, "text/html", html);
    });
}

WifiStatusSnapshot WebServerManager::getStatusSnapshot() const {
    WifiStatusSnapshot snapshot;
    snapshot.ap_ip = ap_ip_;
    snapshot.sta_ip = sta_ip_;
    snapshot.sta_connected = sta_connected_ || (WiFi.status() == WL_CONNECTED);
    if (!sta_ssid_.empty()) {
        snapshot.sta_ssid = sta_ssid_;
    } else if (snapshot.sta_connected) {
        snapshot.sta_ssid = WiFi.SSID().c_str();
    }
    return snapshot;
}

void WebServerManager::disableAP() {
    Serial.println("[WebServer] Disabling Access Point");
    dns_server_.stop();
    dns_active_ = false;
    ap_suppressed_ = true;
    WiFi.softAPdisconnect(true);
    ap_ip_ = IPAddress(0, 0, 0, 0);
}

void WebServerManager::broadcastCanFrame(const CanRxMessage& msg) {
    if (can_monitor_ws_.count() == 0) return;  // No clients connected
    
    DynamicJsonDocument doc(512);
    doc["type"] = "can_frame";
    
    char id_hex[12];
    snprintf(id_hex, sizeof(id_hex), "%08lX", static_cast<unsigned long>(msg.identifier));
    doc["id"] = id_hex;
    doc["timestamp"] = msg.timestamp;
    
    JsonArray dataArray = doc.createNestedArray("data");
    for (uint8_t i = 0; i < msg.length && i < 8; i++) {
        dataArray.add(msg.data[i]);
    }
    
    String payload;
    serializeJson(doc, payload);
    can_monitor_ws_.textAll(payload);
}
