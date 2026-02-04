#include "ota_manager.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <Update.h>
#include <WiFiClientSecure.h>
#include <lvgl.h>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <vector>

#include "config_manager.h"
#include "web_server.h"

namespace {
constexpr const char* kUserAgent = "BroncoControls/OTA";
constexpr const char* kGitHubToken = "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";
constexpr const char* kGitHubApiUrl = "https://api.github.com/repos/js9467/cancontroller/contents/versions";
constexpr const char* kGitHubRawBase = "https://raw.githubusercontent.com/js9467/cancontroller/master/versions/";
const char* kAuthHeader = "Authorization";
const char* kAuthValue = "token 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";
constexpr std::uint32_t kMinIntervalMinutes = 5;
constexpr std::uint32_t kOnlineMinIntervalMinutes = 2;
constexpr std::uint32_t kMaxIntervalMinutes = 24 * 60;

std::uint32_t clampIntervalMinutes(std::uint32_t minutes) {
    return std::max(kMinIntervalMinutes, std::min(kMaxIntervalMinutes, minutes));
}

std::uint32_t onlineIntervalMs(std::uint32_t base_interval_ms) {
    const std::uint32_t online_min_ms = kOnlineMinIntervalMinutes * 60UL * 1000UL;
    return std::min(base_interval_ms, online_min_ms);
}

bool beginsWith(const std::string& value, const char* prefix) {
    return value.rfind(prefix, 0) == 0;
}

std::string sanitizeMd5(const std::string& md5) {
    std::string cleaned;
    cleaned.reserve(md5.size());
    for (char c : md5) {
        if (std::isxdigit(static_cast<unsigned char>(c))) {
            cleaned.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
        }
    }
    return cleaned;
}

std::string resolveUrl(const std::string& base_url, const std::string& candidate) {
    if (candidate.empty()) {
        return "";
    }
    if (beginsWith(candidate, "http://") || beginsWith(candidate, "https://")) {
        return candidate;
    }

    if (!candidate.empty() && candidate.front() == '/') {
        const auto proto_pos = base_url.find("//");
        if (proto_pos == std::string::npos) {
            return candidate;
        }
        const auto host_end = base_url.find('/', proto_pos + 2);
        const std::string origin = (host_end == std::string::npos) ? base_url : base_url.substr(0, host_end);
        return origin + candidate;
    }

    const auto last_slash = base_url.find_last_of('/');
    if (last_slash == std::string::npos) {
        return candidate;
    }
    return base_url.substr(0, last_slash + 1) + candidate;
}

std::string extractHost(const std::string& url) {
    if (url.empty()) {
        return "";
    }
    std::size_t start = 0;
    const std::size_t scheme_pos = url.find("://");
    if (scheme_pos != std::string::npos) {
        start = scheme_pos + 3;
    }
    if (start >= url.size()) {
        return "";
    }
    std::size_t end = url.find('/', start);
    std::string host = (end == std::string::npos) ? url.substr(start) : url.substr(start, end - start);
    if (host.empty()) {
        return "";
    }
    const std::size_t port_pos = host.find(':');
    if (port_pos != std::string::npos) {
        host = host.substr(0, port_pos);
    }
    return host;
}

std::string readJsonString(JsonVariantConst value) {
    if (value.isNull()) {
        return "";
    }
    if (value.is<const char*>()) {
        return std::string(value.as<const char*>());
    }
    if (value.is<String>()) {
        return std::string(value.as<String>().c_str());
    }
    if (value.is<std::string>()) {
        return value.as<std::string>();
    }
    return "";
}

bool beginHttp(HTTPClient& http, WiFiClientSecure& secure_client, const std::string& url) {
    if (beginsWith(url, "https://")) {
        secure_client.setInsecure();
        return http.begin(secure_client, url.c_str());
    }
    return http.begin(url.c_str());
}
}

OTAUpdateManager& OTAUpdateManager::instance() {
    static OTAUpdateManager mgr;
    return mgr;
}

void OTAUpdateManager::begin() {
    const auto& ota_cfg = ConfigManager::instance().getConfig().ota;
    enabled_ = ota_cfg.enabled;
    manifest_url_ = ota_cfg.manifest_url;
    expected_channel_ = ota_cfg.channel.empty() ? "stable" : ota_cfg.channel;
    wifi_ready_ = false;
    pending_manual_check_ = false;
    last_status_ = enabled_ ? "manual-only" : "disabled";

    if (manifest_url_.empty()) {
        enabled_ = false;
        last_status_ = "missing-manifest-url";
        Serial.println("[OTA] Disabled: manifest URL not configured");
    }
    
    Serial.println("[OTA] Initialized in manual-only mode");
}

void OTAUpdateManager::loop(const WifiStatusSnapshot& wifi_status) {
    // Manual-only updates: no automatic polling
    if (!enabled_) {
        return;
    }

    if (!wifi_status.sta_connected) {
        wifi_ready_ = false;
        if (pending_manual_check_ || manual_install_requested_) {
            Serial.printf("[OTA] Manual check blocked: WiFi STA not connected\n");
            pending_manual_check_ = false;
            manual_install_requested_ = false;
            setStatus("waiting-for-wifi");
        }
        return;
    }

    if (!wifi_ready_) {
        wifi_ready_ = true;
        setStatus("wifi-ready");
        Serial.printf("[OTA] WiFi now ready\n");
    }

    // Only process manual check requests
    if (!pending_manual_check_) {
        return;
    }

    Serial.printf("[OTA] Processing manual check/install request\n");
    pending_manual_check_ = false;

    ManifestInfo manifest;
    if (!fetchManifest(manifest)) {
        manual_install_requested_ = false;
        return;
    }

    applyManifest(manifest, manual_install_requested_);
    manual_install_requested_ = false;
}

void OTAUpdateManager::triggerImmediateCheck(bool install_now) {
    Serial.printf("[OTA] triggerImmediateCheck called, install_now=%d, enabled=%d, wifi_ready=%d\n", 
                  install_now, enabled_, wifi_ready_);
    if (!enabled_) {
        setStatus("disabled");
        manual_install_requested_ = false;
        return;
    }
    pending_manual_check_ = true;
    if (install_now) {
        manual_install_requested_ = true;
    }
    if (wifi_ready_) {
        setStatus("manual-check-requested");
    } else {
        setStatus("waiting-for-wifi");
    }
}

void OTAUpdateManager::checkForUpdatesNow() {
    if (!enabled_) {
        setStatus("disabled");
        return;
    }

    if (WiFi.status() != WL_CONNECTED) {
        wifi_ready_ = false;
        setStatus("waiting-for-wifi");
        return;
    }

    wifi_ready_ = true;
    ManifestInfo manifest;
    if (!fetchManifest(manifest)) {
        return;
    }

    if (!expected_channel_.empty() && !manifest.channel.empty() && manifest.channel != expected_channel_) {
        setStatus("manifest-channel-mismatch");
        return;
    }

    if (isNewerVersion(manifest.version)) {
        setStatus(std::string("update-available-") + manifest.version);
    } else {
        setStatus("up-to-date");
    }
}

bool OTAUpdateManager::fetchManifest(ManifestInfo& manifest) {
    if (manifest_url_.empty()) {
        setStatus("manifest-url-empty");
        return false;
    }

    setStatus("checking");
    Serial.println("[OTA] Fetching update manifest...");

    HTTPClient http;
    WiFiClientSecure secure_client;
    if (!beginHttp(http, secure_client, manifest_url_)) {
        setStatus("manifest-begin-failed");
        return false;
    }

    http.setUserAgent(kUserAgent);
    http.setTimeout(30000);  // Increased timeout to 30 seconds
    http.setConnectTimeout(15000);  // 15 second connection timeout
    http.addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    http.addHeader("Pragma", "no-cache");

    Serial.println("[OTA] Sending HTTP GET request...");
    const int http_code = http.GET();
    Serial.printf("[OTA] HTTP response code: %d\n", http_code);
    
    if (http_code != HTTP_CODE_OK) {
        setStatus(std::string("manifest-http-") + std::to_string(http_code));
        http.end();
        return false;
    }

    String payload = http.getString();
    http.end();

    DynamicJsonDocument doc(4096);
    const auto err = deserializeJson(doc, payload);
    if (err) {
        setStatus(std::string("manifest-parse-") + err.c_str());
        return false;
    }

    manifest.version = readJsonString(doc["version"]);
    manifest.channel = readJsonString(doc["channel"]);

    JsonVariantConst firmware_node = doc["firmware"];
    if (firmware_node.isNull()) {
        JsonObjectConst files = doc["files"];
        if (!files.isNull()) {
            firmware_node = files["firmware"];
        }
    }

    manifest.firmware_url = resolveUrl(manifest_url_, readJsonString(firmware_node["url"]));
    manifest.md5 = sanitizeMd5(readJsonString(firmware_node["md5"]));
    manifest.size = static_cast<std::uint32_t>(firmware_node["size"] | 0);

    if (manifest.version.empty() || manifest.firmware_url.empty()) {
        setStatus("manifest-missing-fields");
        return false;
    }

    return true;
}

bool OTAUpdateManager::applyManifest(const ManifestInfo& manifest, bool force_install) {
    if (!expected_channel_.empty() && !manifest.channel.empty() && manifest.channel != expected_channel_) {
        setStatus("manifest-channel-mismatch");
        return false;
    }

    if (!isNewerVersion(manifest.version)) {
        setStatus("up-to-date");
        return true;
    }

    // Manual-only mode: only install if explicitly requested
    if (!force_install) {
        setStatus(std::string("update-available-") + manifest.version);
        return true;
    }

    // User requested install
    if (!downloadAndInstall(manifest)) {
        return false;
    }

    return true;
}

bool OTAUpdateManager::downloadAndInstall(const ManifestInfo& manifest) {
    setStatus(std::string("downloading-") + manifest.version);

    HTTPClient http;
    WiFiClientSecure secure_client;
    if (!beginHttp(http, secure_client, manifest.firmware_url)) {
        setStatus("firmware-begin-failed");
        return false;
    }

    http.setUserAgent(kUserAgent);
    http.setTimeout(60000);

    const int http_code = http.GET();
    if (http_code != HTTP_CODE_OK) {
        setStatus(std::string("firmware-http-") + std::to_string(http_code));
        http.end();
        return false;
    }

    std::uint32_t content_length = static_cast<std::uint32_t>(http.getSize());
    if (content_length == 0) {
        content_length = manifest.size;
    }

    if (content_length == 0) {
        setStatus("firmware-size-unknown");
        http.end();
        return false;
    }

    // Begin OTA update - explicitly specify U_FLASH to update firmware partition
    if (!Update.begin(content_length, U_FLASH)) {
        Serial.println("[OTA] Update.begin() FAILED - cannot allocate flash space");
        Serial.printf("[OTA] Required: %u bytes, U_FLASH=%d\n", content_length, U_FLASH);
        Update.printError(Serial);
        setStatus("update-begin-failed");
        http.end();
        return false;
    }
    
    Serial.printf("[OTA] Update.begin() SUCCESS - allocated %u bytes in flash partition\n", content_length);

    if (!manifest.md5.empty()) {
        if (!Update.setMD5(manifest.md5.c_str())) {
            setStatus("md5-invalid");
            http.end();
            return false;
        }
    }

    // Download in chunks to allow UI updates and show progress
    WiFiClient* stream = http.getStreamPtr();
    size_t written = 0;
    uint8_t buffer[512];  // Smaller buffer = more frequent watchdog feeds
    unsigned long last_update_ms = millis();
    unsigned long last_data_ms = millis();
    const unsigned long kReadTimeout = 30000;  // 30 second timeout for no data
    
    Serial.printf("[OTA] Starting download: %u bytes\n", content_length);
    showOtaScreen(manifest.version);
    updateOtaProgress(0);
    
    while (written < content_length) {
        yield();  // Feed watchdog BEFORE every iteration
        
        size_t available = stream->available();
        unsigned long now = millis();
        
        if (available > 0) {
            last_data_ms = now;  // Reset timeout on data received
            
            size_t to_read = (available > sizeof(buffer)) ? sizeof(buffer) : available;
            size_t read_bytes = stream->readBytes(buffer, to_read);
            
            if (read_bytes > 0) {
                yield();  // Feed watchdog BEFORE write (critical!)
                
                size_t chunk_written = Update.write(buffer, read_bytes);
                if (chunk_written != read_bytes) {
                    Serial.printf("[OTA] Write failed: expected %u, wrote %u\n", read_bytes, chunk_written);
                    Update.abort();
                    http.end();
                    setStatus("firmware-write-failed");
                    return false;
                }
                written += chunk_written;
                
                yield();  // Feed watchdog AFTER write
                
                // Update progress bar
                if (now - last_update_ms >= 500 || written >= content_length) {
                    uint8_t progress = (written * 100) / content_length;
                    Serial.printf("[OTA] Progress: %u%% (%u/%u bytes)\n", progress, written, content_length);
                    updateOtaProgress(progress);
                    last_update_ms = now;
                }
            }
        } else {
            // No data available - check timeout
            if (now - last_data_ms > kReadTimeout) {
                Serial.printf("[OTA] Download timeout - no data for %lu ms\n", now - last_data_ms);
                Update.abort();
                http.end();
                setStatus("firmware-timeout");
                return false;
            }
            
            // Keep feeding watchdog while waiting for data
            delay(10);
            yield();
        }
    }
    
    http.end();
    Serial.printf("[OTA] Download complete: %u bytes\n", written);

    if (written != content_length) {
        Serial.printf("[OTA] Size mismatch: expected %u, got %u\n", content_length, written);
        Update.abort();
        setStatus("firmware-size-mismatch");
        return false;
    }

    // Finalize update - this is critical and can hang
    Serial.println("[OTA] Finalizing update...");
    updateOtaProgress(98);
    
    // Feed watchdog thoroughly before final step
    for (int i = 0; i < 10; i++) {
        yield();
        delay(5);
    }
    
    // Use Update.end(true) to mark new firmware as bootable
    if (!Update.end(true)) {
        Serial.println("[OTA] Update.end() FAILED");
        Update.printError(Serial);
        setStatus("update-end-failed");
        return false;
    }

    Serial.println("[OTA] ✓ Firmware update finalized SUCCESSFULLY");
    Serial.printf("[OTA] ✓ Boot partition will switch on restart\n");
    Serial.printf("[OTA] ✓ Update MD5: %s\n", Update.md5String().c_str());
    Serial.printf("[OTA] Boot partition will switch on restart\n");
    Serial.printf("[OTA] Current MD5: %s\n", Update.md5String().c_str());
    updateOtaProgress(99);
    
    // CRITICAL FIX: Do NOT save config before restart!
    // ConfigManager operations during flash transition can cause hangs
    // The device will auto-detect the new firmware version on boot
    
    setStatus("firmware-restart-pending");
    Serial.println("[OTA] Skipping config save - will auto-detect version on boot");
    updateOtaProgress(100);
    
    // Wait with aggressive watchdog feeding before restart
    Serial.println("[OTA] Restarting in 5 seconds...");
    for (int i = 0; i < 50; i++) {
        delay(100);
        yield();
        if (i % 10 == 0) {
            Serial.printf("[OTA] Restart in %d seconds\n", 5 - (i / 10));
        }
    }
    
    Serial.println("[OTA] **INITIATING RESTART**");
    ESP.restart();
    
    // Should never reach here
    return true;
}

bool OTAUpdateManager::isNewerVersion(const std::string& remote_version) const {
    const std::string current = ConfigManager::instance().getConfig().version;
    return compareVersions(remote_version, current) > 0;
}

int OTAUpdateManager::compareVersions(const std::string& lhs, const std::string& rhs) {
    auto tokenize = [](const std::string& value) {
        std::vector<int> parts;
        std::string token;
        for (char c : value) {
            if (c == '.' || c == '-' || c == '_') {
                if (!token.empty()) {
                    parts.push_back(std::atoi(token.c_str()));
                    token.clear();
                }
                if (c == '-' || c == '_') {
                    break;  // Ignore suffixes like -beta
                }
            } else if (std::isdigit(static_cast<unsigned char>(c))) {
                token.push_back(c);
            }
        }
        if (!token.empty()) {
            parts.push_back(std::atoi(token.c_str()));
        }
        while (parts.size() < 3) {
            parts.push_back(0);
        }
        return parts;
    };

    const std::vector<int> lhs_parts = tokenize(lhs);
    const std::vector<int> rhs_parts = tokenize(rhs);
    const std::size_t count = std::min(lhs_parts.size(), rhs_parts.size());
    for (std::size_t i = 0; i < count; ++i) {
        if (lhs_parts[i] > rhs_parts[i]) {
            return 1;
        }
        if (lhs_parts[i] < rhs_parts[i]) {
            return -1;
        }
    }
    return 0;
}

void OTAUpdateManager::setStatus(const std::string& status) {
    last_status_ = status;
    Serial.printf("[OTA] %s\n", status.c_str());
}

// OTA update screen with progress bar
static lv_obj_t* ota_screen = nullptr;
static lv_obj_t* ota_bar = nullptr;
static lv_obj_t* ota_label = nullptr;
static lv_obj_t* ota_status_label = nullptr;
static lv_obj_t* ota_percent_label = nullptr;

void OTAUpdateManager::showOtaScreen(const std::string& version) {
    if (ota_screen != nullptr) {
        return;  // Already showing
    }

    // Create fullscreen overlay
    ota_screen = lv_obj_create(lv_scr_act());
    lv_obj_set_size(ota_screen, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(ota_screen, lv_color_hex(0x1a1a1a), 0);
    lv_obj_set_style_bg_opa(ota_screen, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(ota_screen, 0, 0);
    lv_obj_center(ota_screen);
    
    // Title
    lv_obj_t* title = lv_label_create(ota_screen);
    lv_label_set_text(title, "Updating Firmware");
    lv_obj_set_style_text_color(title, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, -80);
    
    // Version label
    ota_label = lv_label_create(ota_screen);
    std::string msg = "Version " + version;
    lv_label_set_text(ota_label, msg.c_str());
    lv_obj_set_style_text_color(ota_label, lv_color_hex(0xaaaaaa), 0);
    lv_obj_set_style_text_font(ota_label, &lv_font_montserrat_16, 0);
    lv_obj_align(ota_label, LV_ALIGN_CENTER, 0, -40);
    
    // Status message label
    ota_status_label = lv_label_create(ota_screen);
    lv_label_set_text(ota_status_label, "Connecting to GitHub...");
    lv_obj_set_style_text_color(ota_status_label, lv_color_hex(0x00a8e8), 0);
    lv_obj_set_style_text_font(ota_status_label, &lv_font_montserrat_14, 0);
    lv_obj_align(ota_status_label, LV_ALIGN_CENTER, 0, -5);
    
    // Progress bar
    ota_bar = lv_bar_create(ota_screen);
    lv_obj_set_size(ota_bar, 320, 24);
    lv_obj_align(ota_bar, LV_ALIGN_CENTER, 0, 30);
    lv_obj_set_style_bg_color(ota_bar, lv_color_hex(0x333333), LV_PART_MAIN);
    lv_obj_set_style_bg_color(ota_bar, lv_color_hex(0x00a8e8), LV_PART_INDICATOR);
    lv_bar_set_value(ota_bar, 0, LV_ANIM_OFF);
    lv_bar_set_range(ota_bar, 0, 100);
    
    // Percent label
    ota_percent_label = lv_label_create(ota_screen);
    lv_label_set_text(ota_percent_label, "0%");
    lv_obj_set_style_text_color(ota_percent_label, lv_color_hex(0xaaaaaa), 0);
    lv_obj_align(ota_percent_label, LV_ALIGN_CENTER, 0, 65);
    
    lv_obj_move_foreground(ota_screen);
}

void OTAUpdateManager::updateOtaProgress(uint8_t percent) {
    if (ota_bar == nullptr) {
        return;
    }
    lv_bar_set_value(ota_bar, percent, LV_ANIM_OFF);
    
    if (ota_percent_label) {
        char buf[8];
        snprintf(buf, sizeof(buf), "%d%%", percent);
        lv_label_set_text(ota_percent_label, buf);
    }
    
    // Update progress for web interface
    last_progress_ = percent;
}

void updateOtaStatusMessage(const char* message) {
    if (ota_status_label) {
        lv_label_set_text(ota_status_label, message);
    }
    // Update status message for web interface
    OTAUpdateManager::instance().setStatusMessage(message);
}

void OTAUpdateManager::hideOtaScreen() {
    if (ota_screen != nullptr) {
        lv_obj_del(ota_screen);
        ota_screen = nullptr;
        ota_bar = nullptr;
        ota_label = nullptr;
    }
}

bool OTAUpdateManager::checkGitHubVersions(std::vector<std::string>& versions) {
    Serial.println("[OTA] checkGitHubVersions() called");
    versions.clear();
    
    if (WiFi.status() != WL_CONNECTED) {
        Serial.printf("[OTA] WiFi not connected (status: %d)\n", WiFi.status());
        return false;
    }
    
    Serial.println("[OTA] WiFi connected, checking GitHub for available versions...");
    Serial.printf("[OTA] Requesting: %s\n", kGitHubApiUrl);
    
    HTTPClient http;
    WiFiClientSecure client;
    client.setInsecure();  // For GitHub API
    
    Serial.println("[OTA] Starting HTTP request...");
    if (!http.begin(client, kGitHubApiUrl)) {
        Serial.println("[OTA] http.begin() failed!");
        return false;
    }
    
    http.setUserAgent(kUserAgent);
    http.addHeader("Accept", "application/vnd.github.v3+json");
    // Don't send Authorization header for public repos - causes issues
    // http.addHeader("Authorization", String("token ") + kGitHubToken);
    
    Serial.println("[OTA] Sending GET request...");
    int httpCode = http.GET();
    Serial.printf("[OTA] HTTP response code: %d\n", httpCode);
    
    if (httpCode != HTTP_CODE_OK) {
        Serial.printf("[OTA] GitHub API failed with code: %d\n", httpCode);
        String error = http.errorToString(httpCode);
        Serial.printf("[OTA] Error: %s\n", error.c_str());
        http.end();
        return false;
    }
    
    String payload = http.getString();
    http.end();
    
    Serial.printf("[OTA] Response: %d bytes\n", payload.length());
    
    // Parse JSON array
    DynamicJsonDocument doc(16384);  // Large enough for file list
    auto err = deserializeJson(doc, payload);
    if (err) {
        Serial.printf("[OTA] JSON parse failed: %s\n", err.c_str());
        return false;
    }
    
    if (!doc.is<JsonArray>()) {
        Serial.println("[OTA] Response is not an array");
        return false;
    }
    
    // Extract version numbers from filenames (BIN files only for OTA)
    JsonArray files = doc.as<JsonArray>();
    Serial.printf("[OTA] Found %d items\n", files.size());
    
    for (JsonVariant file : files) {
        const char* name_c = file["name"];
        if (!name_c) continue;
        
        String name = String(name_c);
        
        // Look for bronco_v*.bin files only (OTA updates)
        if (name.startsWith("bronco_v") && name.endsWith(".bin")) {
            // BIN file: bronco_v1.3.84.bin -> 1.3.84
            int start = name.indexOf('v') + 1;
            int end = name.indexOf(".bin");
            if (start > 0 && end > start) {
                String version = name.substring(start, end);
                // Remove _FULL suffix if present
                version.replace("_FULL", "");
                if (version.length() > 0) {
                    versions.push_back(version.c_str());
                    Serial.printf("[OTA] Found: %s\n", version.c_str());
                }
            }
        }
    }
    
    Serial.printf("[OTA] Found %d versions on GitHub\n", versions.size());
    return !versions.empty();
}

bool OTAUpdateManager::installVersionFromGitHub(const std::string& version) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[OTA] WiFi not connected");
        setStatus("wifi-not-connected");
        return false;
    }
    
    Serial.printf("[OTA] Installing version %s from GitHub...\n", version.c_str());
    showOtaScreen(version);
    updateOtaStatusMessage("Connecting to GitHub...");
    delay(100);  // Allow UI to update
    
    // Download firmware.bin directly from GitHub versions folder
    // GitHub stores: versions/bronco_v1.3.84.bin (for OTA updates)
    // Also stores: versions/bronco_v2.0.0_FULL.zip (for major upgrades via USB)
    
    // OTA updates use .bin files directly from the versions folder
    std::string bin_url = std::string(kGitHubRawBase) + "bronco_v" + version + ".bin";
    
    HTTPClient http;
    WiFiClientSecure client;
    client.setInsecure();
    client.setTimeout(15);  // 15 second timeout for SSL handshake
    
    // Download the .bin file
    Serial.printf("[OTA] Downloading: %s\n", bin_url.c_str());
    updateOtaStatusMessage("Requesting firmware...");
    
    if (!http.begin(client, bin_url.c_str())) {
        Serial.println("[OTA] http.begin() failed!");
        setStatus("http-begin-failed");
        updateOtaStatusMessage("Connection failed!");
        delay(3000);
        hideOtaScreen();
        return false;
    }
    
    http.setUserAgent(kUserAgent);
    // Don't send auth header for public repos
    // http.addHeader(kAuthHeader, kAuthValue);
    http.setTimeout(30000);  // 30 second timeout for download
    
    Serial.println("[OTA] Sending GET request...");
    updateOtaStatusMessage("Downloading from GitHub...");
    int httpCode = http.GET();
    Serial.printf("[OTA] HTTP response code: %d\n", httpCode);
    
    if (httpCode != HTTP_CODE_OK) {
        Serial.printf("[OTA] Download failed: %d\n", httpCode);
        if (httpCode == -1) {
            Serial.println("[OTA] HTTP error -1: Connection/SSL handshake failed");
            Serial.println("[OTA] Check WiFi signal strength and GitHub availability");
            updateOtaStatusMessage("Connection failed!");
        } else if (httpCode == 404) {
            Serial.printf("[OTA] File not found: %s\n", bin_url.c_str());
            updateOtaStatusMessage("Version not found!");
        } else {
            Serial.printf("[OTA] HTTP error: %s\n", http.errorToString(httpCode).c_str());
            updateOtaStatusMessage(("Error: " + std::to_string(httpCode)).c_str());
        }
        setStatus(std::string("download-failed-") + std::to_string(httpCode));
        delay(3000);
        http.end();
        hideOtaScreen();
        return false;
    }
    
    int contentLength = http.getSize();
    Serial.printf("[OTA] Firmware size: %d bytes\n", contentLength);
    
    if (contentLength <= 0) {
        Serial.println("[OTA] Invalid content length");
        setStatus("invalid-content-length");
        updateOtaStatusMessage("Invalid firmware size!");
        delay(3000);
        http.end();
        hideOtaScreen();
        return false;
    }
    
    // Begin OTA update - explicitly specify U_FLASH to update firmware partition
    updateOtaStatusMessage("Preparing flash...");
    if (!Update.begin(contentLength, U_FLASH)) {
        Serial.printf("[OTA] Not enough space: %s\n", Update.errorString());
        setStatus("insufficient-space");
        updateOtaStatusMessage("Insufficient space!");
        delay(3000);
        http.end();
        hideOtaScreen();
        return false;
    }
    
    // Download and write firmware
    updateOtaStatusMessage("Installing firmware...");
    WiFiClient* stream = http.getStreamPtr();
    size_t written = 0;
    uint8_t buffer[512];
    
    while (http.connected() && written < contentLength) {
        size_t available = stream->available();
        if (available) {
            size_t to_read = std::min(available, sizeof(buffer));
            size_t read_bytes = stream->readBytes(buffer, to_read);
            
            if (Update.write(buffer, read_bytes) != read_bytes) {
                Serial.println("[OTA] Write failed");
                setStatus("write-failed");
                updateOtaStatusMessage("Write failed!");
                Update.abort();
                delay(3000);
                http.end();
                hideOtaScreen();
                return false;
            }
            
            written += read_bytes;
            uint8_t progress = (written * 100) / contentLength;
            updateOtaProgress(progress);
            
            if (written % 10240 == 0) {  // Log every 10KB
                Serial.printf("[OTA] Progress: %d/%d (%d%%)\n", written, contentLength, progress);
            }
        }
        delay(1);
    }
    
    http.end();
    
    if (written != contentLength) {
        Serial.printf("[OTA] Size mismatch: %d != %d\n", written, contentLength);
        setStatus("size-mismatch");
        updateOtaStatusMessage("Download incomplete!");
        Update.abort();
        delay(3000);
        hideOtaScreen();
        return false;
    }
    
    updateOtaStatusMessage("Verifying firmware...");
    if (!Update.end(true)) {
        Serial.printf("[OTA] Update failed: %s\n", Update.errorString());
        setStatus(std::string("update-failed-") + Update.errorString());
        updateOtaStatusMessage("Verification failed!");
        delay(3000);
        hideOtaScreen();
        return false;
    }
    
    Serial.println("[OTA] ✓ Update successful! Rebooting in 3 seconds...");
    setStatus("update-successful");
    updateOtaProgress(100);
    updateOtaStatusMessage("Success! Rebooting...");
    
    // Give time for UI to update
    for (int i = 3; i > 0; i--) {
        Serial.printf("[OTA] Restarting in %d...\n", i);
        delay(1000);
        yield();
    }
    
    Serial.println("[OTA] **RESTARTING NOW**");
    Serial.flush();
    delay(100);
    
    ESP.restart();
    
    // Should never reach here
    while(1) {
        delay(1000);
    }
    
    return true;
}

void OTAUpdateManager::installVersionFromGitHubAsync(const std::string& version) {
    Serial.println("[OTA] installVersionFromGitHubAsync() called");
    Serial.printf("[OTA] Requested version: %s\n", version.c_str());
    
    // Create a task to run the update in the background
    // This prevents the web server from interfering with the download
    static std::string version_copy;
    version_copy = version;  // Make a copy for the task
    
    Serial.println("[OTA] Creating FreeRTOS task...");
    BaseType_t result = xTaskCreate(
        [](void* param) {
            std::string* ver = static_cast<std::string*>(param);
            Serial.printf("[OTA] Starting async update for version %s\n", ver->c_str());
            
            // Give web server time to send response
            vTaskDelay(pdMS_TO_TICKS(1000));
            
            // Run the actual update
            OTAUpdateManager::instance().installVersionFromGitHub(*ver);
            
            // Task will end here (device reboots on success)
            vTaskDelete(NULL);
        },
        "OTA_Update",
        8192,  // Stack size
        &version_copy,
        1,     // Priority
        NULL
    );
    
    if (result == pdPASS) {
        Serial.println("[OTA] Task created successfully");
    } else {
        Serial.println("[OTA] ERROR: Failed to create task!");
    }
    
    Serial.println("[OTA] installVersionFromGitHubAsync() returning");
}
