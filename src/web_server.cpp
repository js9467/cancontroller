#include "web_server.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <AsyncJson.h>
#include <cstddef>

#include "can_manager.h"
#include "config_manager.h"
#include "ota_manager.h"
#include "ui_builder.h"
#include "suspension_page_template.h"
#include "version_auto.h"
#include "web_interface.h"

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
    : server_(80) {}

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
    // Captive portal detection endpoints - return wrong content to trigger portal
    // iOS and macOS - expects "Success" but we return wrong content to trigger portal
    server_.on("/hotspot-detect.html", HTTP_GET, [](AsyncWebServerRequest* request) {
        AsyncWebServerResponse* response = request->beginResponse(200, "text/html", 
            "<!DOCTYPE html><html><head><meta http-equiv='refresh' content='0; url=http://192.168.4.250/'></head><body></body></html>");
        response->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
        response->addHeader("Pragma", "no-cache");
        response->addHeader("Expires", "0");
        request->send(response);
    });
    server_.on("/library/test/success.html", HTTP_GET, [](AsyncWebServerRequest* request) {
        AsyncWebServerResponse* response = request->beginResponse(200, "text/html", 
            "<!DOCTYPE html><html><head><meta http-equiv='refresh' content='0; url=http://192.168.4.250/'></head><body></body></html>");
        response->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
        request->send(response);
    });
    // Android - expects 204 No Content, we return different to trigger portal
    server_.on("/generate_204", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->redirect("http://192.168.4.250/");
    });
    // Windows connectivity tests
    server_.on("/connecttest.txt", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->redirect("http://192.168.4.250/");
    });
    server_.on("/ncsi.txt", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->redirect("http://192.168.4.250/");
    });
    server_.on("/redirect", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->redirect("http://192.168.4.250/");
    });
    // Additional Microsoft connectivity endpoints
    server_.on("/connectivity-check", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->redirect("http://192.168.4.250/");
    });
    server_.on("/microsoft-connectivity-check", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->redirect("http://192.168.4.250/");
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
        std::vector<std::string> versions;
        bool success = OTAUpdateManager::instance().checkGitHubVersions(versions);
        
        DynamicJsonDocument doc(4096);
        doc["status"] = success ? "ok" : "error";
        
        if (success) {
            JsonArray arr = doc.createNestedArray("versions");
            for (const auto& ver : versions) {
                arr.add(ver);
            }
            doc["count"] = versions.size();
            doc["current"] = APP_VERSION;
        } else {
            doc["message"] = "Failed to fetch GitHub versions";
        }
        
        String payload;
        serializeJson(doc, payload);
        request->send(success ? 200 : 500, "application/json", payload);
    });

    // GitHub OTA install endpoint
    server_.on("/api/ota/github/install", HTTP_POST, [](AsyncWebServerRequest* request) {}, nullptr,
        [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            DynamicJsonDocument doc(256);
            DeserializationError error = deserializeJson(doc, data, len);
            
            if (error) {
                request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
                return;
            }
            
            if (!doc.containsKey("version")) {
                request->send(400, "application/json", "{\"error\":\"Missing version parameter\"}");
                return;
            }
            
            std::string version = doc["version"].as<std::string>();
            
            // Send immediate response
            DynamicJsonDocument response(128);
            response["status"] = "ok";
            response["message"] = "OTA update started";
            response["version"] = version;
            String payload;
            serializeJson(response, payload);
            request->send(200, "application/json", payload);
            
            // Start update in background (will reboot on success)
            // We delay slightly to let the HTTP response complete
            delay(500);
            OTAUpdateManager::instance().installVersionFromGitHub(version);
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

    // Infinitybox Output1 ON
    server_.on("/api/infinitybox/output1/on", HTTP_POST, [](AsyncWebServerRequest* request) {
        bool success = CanManager::instance().sendInfinityboxOutput1On();
        DynamicJsonDocument response(256);
        response["success"] = success;
        response["message"] = success ? "Output1 ON sent" : "Failed to send Output1 ON";
        String payload;
        serializeJson(response, payload);
        request->send(success ? 200 : 500, "application/json", payload);
    });

    // Infinitybox Output1 OFF
    server_.on("/api/infinitybox/output1/off", HTTP_POST, [](AsyncWebServerRequest* request) {
        bool success = CanManager::instance().sendInfinityboxOutput1Off();
        DynamicJsonDocument response(256);
        response["success"] = success;
        response["message"] = success ? "Output1 OFF sent" : "Failed to send Output1 OFF";
        String payload;
        serializeJson(response, payload);
        request->send(success ? 200 : 500, "application/json", payload);
    });

    // Infinitybox Output9 ON
    server_.on("/api/infinitybox/output9/on", HTTP_POST, [](AsyncWebServerRequest* request) {
        bool success = CanManager::instance().sendInfinityboxOutput9On();
        DynamicJsonDocument response(256);
        response["success"] = success;
        response["message"] = success ? "Output9 ON sent" : "Failed to send Output9 ON";
        String payload;
        serializeJson(response, payload);
        request->send(success ? 200 : 500, "application/json", payload);
    });

    // Infinitybox Output9 OFF
    server_.on("/api/infinitybox/output9/off", HTTP_POST, [](AsyncWebServerRequest* request) {
        bool success = CanManager::instance().sendInfinityboxOutput9Off();
        DynamicJsonDocument response(256);
        response["success"] = success;
        response["message"] = success ? "Output9 OFF sent" : "Failed to send Output9 OFF";
        String payload;
        serializeJson(response, payload);
        request->send(success ? 200 : 500, "application/json", payload);
    });

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
