#include "config_manager.h"

#include <Arduino.h>
#include <FS.h>
#include <LittleFS.h>

#include <algorithm>
#include <sstream>
#include <cctype>

#include "version_auto.h"

namespace {
constexpr const char* kConfigPath = "/config.json";

template <typename T>
T clampValue(T value, T min_value, T max_value) {
    return std::min(max_value, std::max(min_value, value));
}

std::string safeString(JsonVariantConst value, const std::string& fallback) {
    if (value.is<const char*>()) {
        return std::string(value.as<const char*>());
    }
    if (value.is<std::string>()) {
        return value.as<std::string>();
    }
    if (value.is<long>()) {
        return std::to_string(value.as<long>());
    }
    if (value.is<unsigned long>()) {
        return std::to_string(value.as<unsigned long>());
    }
    return fallback;
}

std::string sanitizeColor(const std::string& hex) {
    if (hex.size() == 7 && hex[0] == '#') {
        return hex;
    }
    return "#FFA500";
}

bool isValidHexColor(const std::string& hex) {
    if (hex.size() != 7 || hex[0] != '#') {
        return false;
    }
    for (std::size_t i = 1; i < hex.size(); ++i) {
        if (!std::isxdigit(static_cast<unsigned char>(hex[i]))) {
            return false;
        }
    }
    return true;
}

std::string sanitizeColorOptional(const std::string& hex, const std::string& fallback = "") {
    if (hex.empty()) {
        return fallback;
    }
    return isValidHexColor(hex) ? hex : fallback;
}

std::string trimCopy(const std::string& value) {
    const std::size_t start = value.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) {
        return "";
    }
    const std::size_t end = value.find_last_not_of(" \t\r\n");
    return value.substr(start, end - start + 1);
}

std::string fallbackId(const char* prefix, std::size_t index) {
    std::ostringstream oss;
    oss << prefix << '_' << index;
    return oss.str();
}
}

ConfigManager& ConfigManager::instance() {
    static ConfigManager mgr;
    return mgr;
}

bool ConfigManager::begin() {
    if (!LittleFS.begin(true)) {
        Serial.println("[ConfigManager] Failed to mount LittleFS");
        return false;
    }

    if (!LittleFS.exists(kConfigPath)) {
        Serial.println("[ConfigManager] No config file found. Creating defaults.");
        config_ = buildDefaultConfig();
        return save();
    }

    if (!loadFromStorage()) {
        Serial.println("[ConfigManager] Failed to load config. Reverting to defaults.");
        config_ = buildDefaultConfig();
        return save();
    }

    bool needs_save = false;

    // Check if config needs upgrade based on available fonts
    DeviceConfig defaults = buildDefaultConfig();
    if (config_.available_fonts.size() < defaults.available_fonts.size()) {
        Serial.printf("[ConfigManager] Config upgrade needed: %d fonts -> %d fonts\n", 
                     config_.available_fonts.size(), defaults.available_fonts.size());
        // Preserve user settings but update available fonts
        config_.available_fonts = defaults.available_fonts;
        needs_save = true;
    }

    // Always use APP_VERSION as the source of truth
    if (config_.version != APP_VERSION) {
        Serial.printf("[ConfigManager] Syncing version: %s -> %s\n", 
                     config_.version.c_str(), APP_VERSION);
        config_.version = APP_VERSION;
        needs_save = true;
    } else {
        Serial.printf("[ConfigManager] Version: %s\n", APP_VERSION);
    }

    // Always force OTA URL to the managed Fly.io endpoint
    if (config_.ota.manifest_url != kOtaManifestUrl) {
        Serial.println("[ConfigManager] Forcing OTA manifest URL to Fly.io endpoint");
        config_.ota.manifest_url = kOtaManifestUrl;
        needs_save = true;
    }

    if (needs_save) {
        return save();
    }

    return true;
}

int ConfigManager::compareVersions(const std::string& lhs, const std::string& rhs) {
    auto tokenize = [](const std::string& value) {
        std::vector<int> parts;
        std::string token;
        for (char c : value) {
            if (c == '.' || c == '-' || c == '_') {
                if (!token.empty()) {
                    parts.push_back(std::atoi(token.c_str()));
                    token.clear();
                }
                if (c == '-' || c == '_') break;
            } else if (std::isdigit(static_cast<unsigned char>(c))) {
                token.push_back(c);
            }
        }
        if (!token.empty()) {
            parts.push_back(std::atoi(token.c_str()));
        }
        while (parts.size() < 3) parts.push_back(0);
        return parts;
    };

    const std::vector<int> lhs_parts = tokenize(lhs);
    const std::vector<int> rhs_parts = tokenize(rhs);
    for (std::size_t i = 0; i < 3; ++i) {
        if (lhs_parts[i] > rhs_parts[i]) return 1;
        if (lhs_parts[i] < rhs_parts[i]) return -1;
    }
    return 0;
}

bool ConfigManager::save() const {
    std::string json = toJson();
    return writeToStorage(json);
}

void ConfigManager::factoryReset() {
    Serial.println("[ConfigManager] Factory reset - deleting config file");
    if (LittleFS.exists(kConfigPath)) {
        LittleFS.remove(kConfigPath);
    }
    if (LittleFS.exists("/config.tmp")) {
        LittleFS.remove("/config.tmp");
    }
    // Reset to defaults in memory
    config_ = buildDefaultConfig();
}

bool ConfigManager::resetToDefaults() {
    config_ = buildDefaultConfig();
    return save();
}

std::string ConfigManager::toJson() const {
    DynamicJsonDocument doc(524288);  // 512KB for base64 images
    encodeConfig(config_, doc);

    std::string output;
    serializeJson(doc, output);
    return output;
}

bool ConfigManager::updateFromJson(JsonVariantConst json, std::string& error) {
    DeviceConfig incoming = config_;  // Preserve existing fields when JSON omits them
    if (!decodeConfig(json, incoming, error)) {
        return false;
    }

    config_ = std::move(incoming);
    return true;
}

bool ConfigManager::loadFromStorage() {
    File file = LittleFS.open(kConfigPath, FILE_READ);
    if (!file) {
        Serial.println("[ConfigManager] Could not open config file");
        return false;
    }

    DynamicJsonDocument doc(524288);  // 512KB for base64 images
    DeserializationError err = deserializeJson(doc, file);
    file.close();

    if (err) {
        Serial.printf("[ConfigManager] JSON parse error: %s\n", err.c_str());
        return false;
    }

    std::string parse_error;
    if (!decodeConfig(doc.as<JsonVariantConst>(), config_, parse_error)) {
        Serial.printf("[ConfigManager] Decode error: %s\n", parse_error.c_str());
        return false;
    }

    return true;
}

bool ConfigManager::writeToStorage(const std::string& json) const {
    // ATOMIC WRITE: Use temp file + rename to prevent corruption during brownout
    const char* kTempPath = "/config.tmp";
    
    // Write to temp file first
    File file = LittleFS.open(kTempPath, FILE_WRITE);
    if (!file) {
        Serial.println("[ConfigManager] Could not open temp file for writing");
        return false;
    }

    size_t written = file.print(json.c_str());
    file.close();
    
    if (written != json.size()) {
        Serial.println("[ConfigManager] Incomplete write to temp file");
        LittleFS.remove(kTempPath);
        return false;
    }
    
    // Atomic rename (remove old, rename temp)
    if (LittleFS.exists(kConfigPath)) {
        LittleFS.remove(kConfigPath);
    }
    
    if (!LittleFS.rename(kTempPath, kConfigPath)) {
        Serial.println("[ConfigManager] Failed to rename temp to config");
        return false;
    }
    
    return true;
}

DeviceConfig ConfigManager::buildDefaultConfig() const {
    DeviceConfig cfg;
    cfg.version = APP_VERSION;
    cfg.header.title = "CAN Control";
    cfg.header.subtitle = "Configuration Interface";
    cfg.header.show_logo = true;
    cfg.header.logo_variant = "";  // Empty by default - no built-in logo
    cfg.header.title_font = "montserrat_24";
    cfg.header.subtitle_font = "montserrat_12";
    cfg.header.logo_target_height = 64;
    cfg.header.logo_preserve_aspect = true;
    cfg.header.nav_spacing = 12;

    cfg.display.brightness = 100;
    cfg.display.sleep_enabled = false;
    cfg.display.sleep_timeout_seconds = 60;

    // Ensure WiFi AP is always enabled by default
    cfg.wifi.ap.enabled = true;
    cfg.wifi.ap.ssid = "CAN-Control";
    cfg.wifi.ap.password.clear();
    cfg.wifi.sta.enabled = false;

    cfg.ota.enabled = true;
    // cfg.ota.auto_apply = true;  // Removed - manual-only
    cfg.ota.manifest_url = kOtaManifestUrl;
    cfg.ota.channel = "stable";
    // cfg.ota.check_interval_minutes = 60;  // Removed - manual-only

    // Initialize available fonts
    cfg.available_fonts.clear();
    FontConfig font_12; font_12.name = "montserrat_12"; font_12.display_name = "Montserrat 12"; font_12.size = 12;
    cfg.available_fonts.push_back(font_12);
    FontConfig font_14; font_14.name = "montserrat_14"; font_14.display_name = "Montserrat 14"; font_14.size = 14;
    cfg.available_fonts.push_back(font_14);
    FontConfig font_16; font_16.name = "montserrat_16"; font_16.display_name = "Montserrat 16"; font_16.size = 16;
    cfg.available_fonts.push_back(font_16);
    FontConfig font_18; font_18.name = "montserrat_18"; font_18.display_name = "Montserrat 18"; font_18.size = 18;
    cfg.available_fonts.push_back(font_18);
    FontConfig font_20; font_20.name = "montserrat_20"; font_20.display_name = "Montserrat 20"; font_20.size = 20;
    cfg.available_fonts.push_back(font_20);
    FontConfig font_22; font_22.name = "montserrat_22"; font_22.display_name = "Montserrat 22"; font_22.size = 22;
    cfg.available_fonts.push_back(font_22);
    FontConfig font_24; font_24.name = "montserrat_24"; font_24.display_name = "Montserrat 24"; font_24.size = 24;
    cfg.available_fonts.push_back(font_24);
    FontConfig font_26; font_26.name = "montserrat_26"; font_26.display_name = "Montserrat 26"; font_26.size = 26;
    cfg.available_fonts.push_back(font_26);
    FontConfig font_28; font_28.name = "montserrat_28"; font_28.display_name = "Montserrat 28"; font_28.size = 28;
    cfg.available_fonts.push_back(font_28);
    FontConfig font_30; font_30.name = "montserrat_30"; font_30.display_name = "Montserrat 30"; font_30.size = 30;
    cfg.available_fonts.push_back(font_30);
    FontConfig font_32; font_32.name = "montserrat_32"; font_32.display_name = "Montserrat 32"; font_32.size = 32;
    cfg.available_fonts.push_back(font_32);
    FontConfig font_34; font_34.name = "montserrat_34"; font_34.display_name = "Montserrat 34"; font_34.size = 34;
    cfg.available_fonts.push_back(font_34);
    FontConfig font_36; font_36.name = "montserrat_36"; font_36.display_name = "Montserrat 36"; font_36.size = 36;
    cfg.available_fonts.push_back(font_36);
    FontConfig font_38; font_38.name = "montserrat_38"; font_38.display_name = "Montserrat 38"; font_38.size = 38;
    cfg.available_fonts.push_back(font_38);
    FontConfig font_40; font_40.name = "montserrat_40"; font_40.display_name = "Montserrat 40"; font_40.size = 40;
    cfg.available_fonts.push_back(font_40);
    FontConfig font_42; font_42.name = "montserrat_42"; font_42.display_name = "Montserrat 42"; font_42.size = 42;
    cfg.available_fonts.push_back(font_42);
    FontConfig font_44; font_44.name = "montserrat_44"; font_44.display_name = "Montserrat 44"; font_44.size = 44;
    cfg.available_fonts.push_back(font_44);
    FontConfig font_46; font_46.name = "montserrat_46"; font_46.display_name = "Montserrat 46"; font_46.size = 46;
    cfg.available_fonts.push_back(font_46);
    FontConfig font_48; font_48.name = "montserrat_48"; font_48.display_name = "Montserrat 48"; font_48.size = 48;
    cfg.available_fonts.push_back(font_48);
    FontConfig font_dejavu16; font_dejavu16.name = "dejavu_16"; font_dejavu16.display_name = "DejaVu 16 (Persian/Hebrew)"; font_dejavu16.size = 16;
    cfg.available_fonts.push_back(font_dejavu16);
    FontConfig font_simsun16; font_simsun16.name = "simsun_16"; font_simsun16.display_name = "SimSun 16 (CJK)"; font_simsun16.size = 16;
    cfg.available_fonts.push_back(font_simsun16);
    FontConfig font_unscii8; font_unscii8.name = "unscii_8"; font_unscii8.display_name = "UNSCII 8"; font_unscii8.size = 8;
    cfg.available_fonts.push_back(font_unscii8);
    FontConfig font_unscii16; font_unscii16.name = "unscii_16"; font_unscii16.display_name = "UNSCII 16"; font_unscii16.size = 16;
    cfg.available_fonts.push_back(font_unscii16);

    PageConfig home;
    home.id = "home";
    home.name = "Factory Home";
    home.rows = 2;
    home.cols = 2;

    ButtonConfig windows;
    windows.id = "windows";
    windows.label = "Windows";
    windows.color = "#FF8A00";
    windows.row = 0;
    windows.col = 0;

    ButtonConfig locks;
    locks.id = "locks";
    locks.label = "Locks";
    locks.color = "#1ABC9C";
    locks.row = 0;
    locks.col = 1;

    ButtonConfig running;
    running.id = "running";
    running.label = "Running Boards";
    running.color = "#2980B9";
    running.row = 1;
    running.col = 0;

    ButtonConfig aux;
    aux.id = "aux";
    aux.label = "Aux";
    aux.color = "#9B59B6";
    aux.row = 1;
    aux.col = 1;

    home.buttons = {windows, locks, running, aux};
    cfg.pages = {home};

    return cfg;
}

void ConfigManager::encodeConfig(const DeviceConfig& source, DynamicJsonDocument& doc) const {
    doc.clear();
    doc["version"] = source.version.c_str();

    JsonObject header = doc["header"].to<JsonObject>();
    header["title"] = source.header.title.c_str();
    header["subtitle"] = source.header.subtitle.c_str();
    header["show_logo"] = source.header.show_logo;
    header["logo_variant"] = source.header.logo_variant.c_str();
    header["logo_base64"] = source.header.logo_base64.c_str();
    header["title_font"] = source.header.title_font.c_str();
    header["subtitle_font"] = source.header.subtitle_font.c_str();
    header["title_align"] = source.header.title_align.c_str();
    header["logo_position"] = source.header.logo_position.c_str();
    header["logo_target_height"] = source.header.logo_target_height;
    header["logo_preserve_aspect"] = source.header.logo_preserve_aspect;
    header["nav_spacing"] = source.header.nav_spacing;

    JsonObject display = doc["display"].to<JsonObject>();
    display["brightness"] = source.display.brightness;
    display["sleep_enabled"] = source.display.sleep_enabled;
    display["sleep_timeout_seconds"] = source.display.sleep_timeout_seconds;
    display["sleep_icon_base64"] = source.display.sleep_icon_base64.c_str();

    JsonObject images = doc["images"].to<JsonObject>();
    images["header_logo"] = source.images.header_logo.c_str();
    images["splash_logo"] = source.images.splash_logo.c_str();
    images["background_image"] = source.images.background_image.c_str();
    images["sleep_logo"] = source.images.sleep_logo.c_str();

    JsonObject theme = doc["theme"].to<JsonObject>();
    theme["bg_color"] = source.theme.bg_color.c_str();
    theme["surface_color"] = source.theme.surface_color.c_str();
    theme["page_bg_color"] = source.theme.page_bg_color.c_str();
    theme["accent_color"] = source.theme.accent_color.c_str();
    theme["text_primary"] = source.theme.text_primary.c_str();
    theme["text_secondary"] = source.theme.text_secondary.c_str();
    theme["border_color"] = source.theme.border_color.c_str();
    theme["header_border_color"] = source.theme.header_border_color.c_str();
    theme["nav_button_color"] = source.theme.nav_button_color.c_str();
    theme["nav_button_active_color"] = source.theme.nav_button_active_color.c_str();
    theme["nav_button_text_color"] = source.theme.nav_button_text_color.c_str();
    theme["nav_button_radius"] = source.theme.nav_button_radius;
    theme["button_radius"] = source.theme.button_radius;
    theme["border_width"] = source.theme.border_width;
    theme["header_border_width"] = source.theme.header_border_width;

    JsonObject wifi = doc["wifi"].to<JsonObject>();
    JsonObject ap = wifi["ap"].to<JsonObject>();
    ap["enabled"] = source.wifi.ap.enabled;
    ap["ssid"] = source.wifi.ap.ssid.c_str();
    ap["password"] = source.wifi.ap.password.c_str();

    JsonObject sta = wifi["sta"].to<JsonObject>();
    sta["enabled"] = source.wifi.sta.enabled;
    sta["ssid"] = source.wifi.sta.ssid.c_str();
    sta["password"] = source.wifi.sta.password.c_str();

    JsonObject ota = doc["ota"].to<JsonObject>();
    ota["enabled"] = source.ota.enabled;
    // ota["auto_apply"] = source.ota.auto_apply;  // Removed - manual-only
    ota["manifest_url"] = source.ota.manifest_url.c_str();
    ota["channel"] = source.ota.channel.c_str();
    // ota["check_interval_minutes"] = source.ota.check_interval_minutes;  // Removed - manual-only

    JsonArray pages = doc["pages"].to<JsonArray>();
    for (const auto& page : source.pages) {
        JsonObject page_obj = pages.createNestedObject();
        page_obj["id"] = page.id.c_str();
        page_obj["name"] = page.name.c_str();
        page_obj["nav_text"] = page.nav_text.c_str();
        page_obj["nav_color"] = page.nav_color.c_str();
        page_obj["nav_inactive_color"] = page.nav_inactive_color.c_str();
        page_obj["nav_text_color"] = page.nav_text_color.c_str();
        if (page.nav_button_radius >= 0) {
            page_obj["nav_button_radius"] = page.nav_button_radius;
        }
        page_obj["bg_color"] = page.bg_color.c_str();
        page_obj["text_color"] = page.text_color.c_str();
        page_obj["button_color"] = page.button_color.c_str();
        page_obj["button_pressed_color"] = page.button_pressed_color.c_str();
        page_obj["button_border_color"] = page.button_border_color.c_str();
        page_obj["button_border_width"] = page.button_border_width;
        page_obj["button_radius"] = page.button_radius;
        page_obj["rows"] = page.rows;
        page_obj["cols"] = page.cols;
        page_obj["type"] = page.type.c_str();
        page_obj["custom_content"] = page.custom_content.c_str();

        JsonArray buttons = page_obj["buttons"].to<JsonArray>();
        for (const auto& button : page.buttons) {
            JsonObject btn_obj = buttons.createNestedObject();
            btn_obj["id"] = button.id.c_str();
            btn_obj["label"] = button.label.c_str();
            btn_obj["color"] = button.color.c_str();
            btn_obj["pressed_color"] = button.pressed_color.c_str();
            btn_obj["text_color"] = button.text_color.c_str();
            btn_obj["icon"] = button.icon.c_str();
            btn_obj["row"] = button.row;
            btn_obj["col"] = button.col;
            btn_obj["row_span"] = button.row_span;
            btn_obj["col_span"] = button.col_span;
            btn_obj["momentary"] = button.momentary;
            btn_obj["font_size"] = button.font_size;
            btn_obj["font_family"] = button.font_family.c_str();
            btn_obj["font_weight"] = button.font_weight.c_str();
            btn_obj["font_name"] = button.font_name.c_str();
            btn_obj["text_align"] = button.text_align.c_str();
            btn_obj["corner_radius"] = button.corner_radius;
            btn_obj["border_width"] = button.border_width;
            btn_obj["border_color"] = button.border_color.c_str();

            JsonObject can_obj = btn_obj["can"].to<JsonObject>();
            can_obj["enabled"] = button.can.enabled;
            can_obj["pgn"] = button.can.pgn;
            can_obj["priority"] = button.can.priority;
            can_obj["source_address"] = button.can.source_address;
            can_obj["destination_address"] = button.can.destination_address;

            JsonArray data_arr = can_obj["data"].to<JsonArray>();
            for (std::uint8_t i = 0; i < button.can.length; ++i) {
                data_arr.add(button.can.data[i]);
            }

            JsonObject can_off_obj = btn_obj["can_off"].to<JsonObject>();
            can_off_obj["enabled"] = button.can_off.enabled;
            can_off_obj["pgn"] = button.can_off.pgn;
            can_off_obj["priority"] = button.can_off.priority;
            can_off_obj["source_address"] = button.can_off.source_address;
            can_off_obj["destination_address"] = button.can_off.destination_address;

            JsonArray off_data_arr = can_off_obj["data"].to<JsonArray>();
            for (std::uint8_t i = 0; i < button.can_off.length; ++i) {
                off_data_arr.add(button.can_off.data[i]);
            }
            
            // Behavioral output system fields
            btn_obj["mode"] = button.mode.c_str();
            btn_obj["scene_id"] = button.scene_id.c_str();
            btn_obj["scene_action"] = button.scene_action.c_str();
            btn_obj["scene_duration_ms"] = button.scene_duration_ms;
            btn_obj["scene_release_off"] = button.scene_release_off;
            
            JsonObject output_behavior = btn_obj["output_behavior"].to<JsonObject>();
            output_behavior["output_id"] = button.output_behavior.output_id.c_str();
            output_behavior["action"] = button.output_behavior.action.c_str();
            output_behavior["behavior_type"] = button.output_behavior.behavior_type.c_str();
            output_behavior["target_value"] = button.output_behavior.target_value;
            output_behavior["period_ms"] = button.output_behavior.period_ms;
            output_behavior["duty_cycle"] = button.output_behavior.duty_cycle;
            output_behavior["fade_time_ms"] = button.output_behavior.fade_time_ms;
            output_behavior["hold_duration_ms"] = button.output_behavior.hold_duration_ms;
            output_behavior["on_time_ms"] = button.output_behavior.on_time_ms;
            output_behavior["off_time_ms"] = button.output_behavior.off_time_ms;
            output_behavior["auto_off"] = button.output_behavior.auto_off;
            
            // Legacy fields (backward compatibility)
            btn_obj["infinitybox_function"] = button.infinitybox_function.c_str();
            btn_obj["flash_frequency"] = button.flash_frequency;
            btn_obj["fade_time"] = button.fade_time;
            btn_obj["on_time"] = button.on_time;
        }
    }

    JsonArray can_library = doc["can_library"].to<JsonArray>();
    for (const auto& msg : source.can_library) {
        JsonObject msg_obj = can_library.createNestedObject();
        msg_obj["id"] = msg.id.c_str();
        msg_obj["name"] = msg.name.c_str();
        msg_obj["pgn"] = msg.pgn;
        msg_obj["priority"] = msg.priority;
        msg_obj["source_address"] = msg.source_address;
        msg_obj["destination_address"] = msg.destination_address;
        msg_obj["description"] = msg.description.c_str();
        
        JsonArray data_arr = msg_obj["data"].to<JsonArray>();
        for (std::uint8_t byte : msg.data) {
            data_arr.add(byte);
        }
    }

    // Encode available fonts
    JsonArray fonts = doc["available_fonts"].to<JsonArray>();
    for (const auto& font : source.available_fonts) {
        JsonObject font_obj = fonts.createNestedObject();
        font_obj["name"] = font.name.c_str();
        font_obj["display_name"] = font.display_name.c_str();
        font_obj["size"] = font.size;
    }
}

bool ConfigManager::decodeConfig(JsonVariantConst json, DeviceConfig& target, std::string& error) const {
    if (json.isNull()) {
        error = "JSON payload is empty";
        return false;
    }

    target.version = safeString(json["version"], "1.0.0");

    JsonObjectConst header = json["header"];
    if (!header.isNull()) {
        target.header.title = safeString(header["title"], target.header.title);
        target.header.subtitle = safeString(header["subtitle"], target.header.subtitle);
        target.header.show_logo = header["show_logo"] | target.header.show_logo;
        target.header.logo_variant = safeString(header["logo_variant"], target.header.logo_variant);
        target.header.logo_base64 = safeString(header["logo_base64"], target.header.logo_base64);
        target.header.title_font = safeString(header["title_font"], target.header.title_font);
        target.header.subtitle_font = safeString(header["subtitle_font"], target.header.subtitle_font);
        target.header.title_align = safeString(header["title_align"], target.header.title_align);
        target.header.logo_position = safeString(header["logo_position"], target.header.logo_position);
        target.header.logo_target_height = clampValue<std::uint16_t>(header["logo_target_height"] | target.header.logo_target_height, 16u, 128u);
        target.header.logo_preserve_aspect = header["logo_preserve_aspect"] | target.header.logo_preserve_aspect;
        target.header.nav_spacing = clampValue<std::uint8_t>(header["nav_spacing"] | target.header.nav_spacing, 0u, 60u);
    }

    JsonObjectConst display = json["display"];
    if (!display.isNull()) {
        target.display.brightness = clampValue<std::uint8_t>(display["brightness"] | target.display.brightness, 0u, 100u);
        target.display.sleep_enabled = display["sleep_enabled"] | target.display.sleep_enabled;
        target.display.sleep_timeout_seconds = clampValue<std::uint16_t>(display["sleep_timeout_seconds"] | target.display.sleep_timeout_seconds, 5u, 3600u);
        target.display.sleep_icon_base64 = safeString(display["sleep_icon_base64"], target.display.sleep_icon_base64);
    }

    JsonObjectConst images = json["images"];
    if (!images.isNull()) {
        target.images.header_logo = safeString(images["header_logo"], target.images.header_logo);
        target.images.splash_logo = safeString(images["splash_logo"], target.images.splash_logo);
        target.images.background_image = safeString(images["background_image"], target.images.background_image);
        target.images.sleep_logo = safeString(images["sleep_logo"], target.images.sleep_logo);
    }

    JsonObjectConst theme = json["theme"];
    if (!theme.isNull()) {
        target.theme.bg_color = sanitizeColor(safeString(theme["bg_color"], target.theme.bg_color));
        target.theme.surface_color = sanitizeColor(safeString(theme["surface_color"], target.theme.surface_color));
        target.theme.page_bg_color = sanitizeColor(safeString(theme["page_bg_color"], target.theme.page_bg_color));
        target.theme.accent_color = sanitizeColor(safeString(theme["accent_color"], target.theme.accent_color));
        target.theme.text_primary = sanitizeColor(safeString(theme["text_primary"], target.theme.text_primary));
        target.theme.text_secondary = sanitizeColor(safeString(theme["text_secondary"], target.theme.text_secondary));
        target.theme.border_color = sanitizeColor(safeString(theme["border_color"], target.theme.border_color));
        target.theme.header_border_color = sanitizeColor(safeString(theme["header_border_color"], target.theme.header_border_color));
        target.theme.nav_button_color = sanitizeColor(safeString(theme["nav_button_color"], target.theme.nav_button_color));
        target.theme.nav_button_active_color = sanitizeColor(safeString(theme["nav_button_active_color"], target.theme.nav_button_active_color));
        target.theme.nav_button_text_color = sanitizeColor(safeString(theme["nav_button_text_color"], target.theme.nav_button_text_color));
        target.theme.nav_button_radius = clampValue<std::uint8_t>(theme["nav_button_radius"] | target.theme.nav_button_radius, 0u, 50u);
        target.theme.button_radius = clampValue<std::uint8_t>(theme["button_radius"] | target.theme.button_radius, 0u, 50u);
        target.theme.border_width = clampValue<std::uint8_t>(theme["border_width"] | target.theme.border_width, 0u, 10u);
        target.theme.header_border_width = clampValue<std::uint8_t>(theme["header_border_width"] | target.theme.header_border_width, 0u, 10u);
    }

    JsonObjectConst wifi = json["wifi"];
    if (!wifi.isNull()) {
        JsonObjectConst ap = wifi["ap"];
        if (!ap.isNull()) {
            target.wifi.ap.enabled = ap["enabled"] | true;
            target.wifi.ap.ssid = safeString(ap["ssid"], target.wifi.ap.ssid);
            target.wifi.ap.password = safeString(ap["password"], target.wifi.ap.password);
        }

        JsonObjectConst sta = wifi["sta"];
        if (!sta.isNull()) {
            target.wifi.sta.enabled = sta["enabled"] | false;
            target.wifi.sta.ssid = safeString(sta["ssid"], target.wifi.sta.ssid);
            target.wifi.sta.password = safeString(sta["password"], target.wifi.sta.password);
        }
    }

    target.ota.manifest_url = kOtaManifestUrl;  // OTA endpoint is centrally managed
    JsonObjectConst ota = json["ota"];
    if (!ota.isNull()) {
        target.ota.enabled = ota["enabled"] | target.ota.enabled;
        // target.ota.auto_apply = ota["auto_apply"] | target.ota.auto_apply;  // Removed - manual-only
        target.ota.channel = safeString(ota["channel"], target.ota.channel);
        // const std::uint32_t interval = ota["check_interval_minutes"] | target.ota.check_interval_minutes;  // Removed - manual-only
        // target.ota.check_interval_minutes = clampValue<std::uint32_t>(interval, 5u, 1440u);  // Removed - manual-only
    }

    target.pages.clear();
    JsonArrayConst pages = json["pages"].as<JsonArrayConst>();
    if (!pages.isNull()) {
        std::size_t page_index = 0;
        for (JsonObjectConst page_obj : pages) {
            if (page_index >= MAX_PAGES) {
                break;
            }

            PageConfig page;
            page.id = safeString(page_obj["id"], fallbackId("page", page_index));
            std::string raw_name = trimCopy(safeString(page_obj["name"], ""));
            if (raw_name.empty()) {
                raw_name = page.id;
            }
            page.name = raw_name;
            page.nav_text = trimCopy(safeString(page_obj["nav_text"], ""));
            page.nav_color = sanitizeColorOptional(safeString(page_obj["nav_color"], ""));
            page.nav_inactive_color = sanitizeColorOptional(safeString(page_obj["nav_inactive_color"], ""));
            page.nav_text_color = sanitizeColorOptional(safeString(page_obj["nav_text_color"], ""));
            JsonVariantConst nav_radius_variant = page_obj["nav_button_radius"];
            if (!nav_radius_variant.isNull()) {
                int radius_value = nav_radius_variant | -1;
                radius_value = std::max(-1, std::min(50, radius_value));
                page.nav_button_radius = radius_value;
            } else {
                page.nav_button_radius = -1;
            }
            page.bg_color = sanitizeColorOptional(safeString(page_obj["bg_color"], ""));
            page.text_color = sanitizeColorOptional(safeString(page_obj["text_color"], ""));
            page.button_color = sanitizeColorOptional(safeString(page_obj["button_color"], ""));
            page.button_pressed_color = sanitizeColorOptional(safeString(page_obj["button_pressed_color"], ""));
            page.button_border_color = sanitizeColorOptional(safeString(page_obj["button_border_color"], ""));
            page.button_border_width = clampValue<std::uint8_t>(page_obj["button_border_width"] | page.button_border_width, 0u, 10u);
            page.button_radius = clampValue<std::uint8_t>(page_obj["button_radius"] | page.button_radius, 0u, 50u);
            page.rows = clampValue<std::uint8_t>(page_obj["rows"] | 2, 1, 4);
            page.cols = clampValue<std::uint8_t>(page_obj["cols"] | 2, 1, 4);
            page.type = safeString(page_obj["type"], "");
            page.custom_content = safeString(page_obj["custom_content"], "");

            JsonArrayConst buttons = page_obj["buttons"].as<JsonArrayConst>();
            if (!buttons.isNull()) {
                std::size_t button_index = 0;
                for (JsonObjectConst btn_obj : buttons) {
                    if (button_index >= MAX_BUTTONS_PER_PAGE) {
                        break;
                    }

                    ButtonConfig button;
                    button.id = safeString(btn_obj["id"], fallbackId("btn", button_index));
                    button.label = safeString(btn_obj["label"], button.id);
                    button.color = sanitizeColor(safeString(btn_obj["color"], button.color));
                    button.pressed_color = sanitizeColor(safeString(btn_obj["pressed_color"], button.pressed_color));
                    button.text_color = sanitizeColorOptional(safeString(btn_obj["text_color"], ""));
                    button.icon = safeString(btn_obj["icon"], "");
                    button.row = clampValue<std::uint8_t>(btn_obj["row"] | 0, 0, page.rows - 1);
                    button.col = clampValue<std::uint8_t>(btn_obj["col"] | 0, 0, page.cols - 1);
                    button.row_span = clampValue<std::uint8_t>(btn_obj["row_span"] | 1, 1, page.rows - button.row);
                    button.col_span = clampValue<std::uint8_t>(btn_obj["col_span"] | 1, 1, page.cols - button.col);
                    button.momentary = btn_obj["momentary"] | false;
                    button.font_size = clampValue<std::uint8_t>(btn_obj["font_size"] | 24, 8, 72);
                    button.font_family = safeString(btn_obj["font_family"], "montserrat");
                    button.font_weight = safeString(btn_obj["font_weight"], "400");
                    button.font_name = safeString(btn_obj["font_name"], "montserrat_16");
                    button.text_align = safeString(btn_obj["text_align"], "center");
                    button.corner_radius = clampValue<std::uint8_t>(btn_obj["corner_radius"] | 12, 0, 50);
                    button.border_width = clampValue<std::uint8_t>(btn_obj["border_width"] | 0, 0, 10);
                    button.border_color = sanitizeColor(safeString(btn_obj["border_color"], "#FFFFFF"));

                    JsonObjectConst can_obj = btn_obj["can"];
                    if (!can_obj.isNull()) {
                        button.can.enabled = can_obj["enabled"] | false;
                        button.can.pgn = can_obj["pgn"] | button.can.pgn;
                        button.can.priority = clampValue<std::uint8_t>(can_obj["priority"] | button.can.priority, 0u, 7u);
                        button.can.source_address = can_obj["source_address"] | button.can.source_address;
                        button.can.destination_address = can_obj["destination_address"] | button.can.destination_address;

                        JsonArrayConst data_arr = can_obj["data"].as<JsonArrayConst>();
                        if (!data_arr.isNull()) {
                            std::size_t i = 0;
                            for (JsonVariantConst byte_val : data_arr) {
                                if (i >= button.can.data.size()) {
                                    break;
                                }
                                button.can.data[i] = clampValue<std::uint8_t>(byte_val | 0, 0u, 255u);
                                ++i;
                            }
                            button.can.length = static_cast<std::uint8_t>(i);  // Set length based on actual data bytes
                        }
                    }

                    JsonObjectConst can_off_obj = btn_obj["can_off"];
                    if (!can_off_obj.isNull()) {
                        button.can_off.enabled = can_off_obj["enabled"] | false;
                        button.can_off.pgn = can_off_obj["pgn"] | button.can_off.pgn;
                        button.can_off.priority = clampValue<std::uint8_t>(can_off_obj["priority"] | button.can_off.priority, 0u, 7u);
                        button.can_off.source_address = can_off_obj["source_address"] | button.can_off.source_address;
                        button.can_off.destination_address = can_off_obj["destination_address"] | button.can_off.destination_address;

                        JsonArrayConst off_data_arr = can_off_obj["data"].as<JsonArrayConst>();
                        if (!off_data_arr.isNull()) {
                            std::size_t i = 0;
                            for (JsonVariantConst byte_val : off_data_arr) {
                                if (i >= button.can_off.data.size()) {
                                    break;
                                }
                                button.can_off.data[i] = clampValue<std::uint8_t>(byte_val | 0, 0u, 255u);
                                ++i;
                            }
                            button.can_off.length = static_cast<std::uint8_t>(i);  // Set length based on actual data bytes
                        }
                    }
                    
                    // Behavioral output system fields
                    button.mode = safeString(btn_obj["mode"], "can");
                    button.scene_id = safeString(btn_obj["scene_id"], "");
                    button.scene_action = safeString(btn_obj["scene_action"], "on");
                    button.scene_duration_ms = clampValue<std::uint16_t>(btn_obj["scene_duration_ms"] | 0, 0u, 60000u);
                    button.scene_release_off = btn_obj["scene_release_off"] | false;
                    
                    JsonObjectConst output_behavior = btn_obj["output_behavior"];
                    if (!output_behavior.isNull()) {
                        button.output_behavior.output_id = safeString(output_behavior["output_id"], "");
                        button.output_behavior.action = safeString(output_behavior["action"], "on");
                        button.output_behavior.behavior_type = safeString(output_behavior["behavior_type"], "steady");
                        button.output_behavior.target_value = clampValue<std::uint8_t>(output_behavior["target_value"] | 100, 0u, 100u);
                        button.output_behavior.period_ms = clampValue<std::uint16_t>(output_behavior["period_ms"] | 500, 1u, 10000u);
                        button.output_behavior.duty_cycle = clampValue<std::uint8_t>(output_behavior["duty_cycle"] | 50, 0u, 100u);
                        button.output_behavior.fade_time_ms = clampValue<std::uint16_t>(output_behavior["fade_time_ms"] | 1000, 0u, 10000u);
                        button.output_behavior.hold_duration_ms = clampValue<std::uint16_t>(output_behavior["hold_duration_ms"] | 0, 0u, 60000u);
                        button.output_behavior.on_time_ms = clampValue<std::uint16_t>(output_behavior["on_time_ms"] | 100, 1u, 10000u);
                        button.output_behavior.off_time_ms = clampValue<std::uint16_t>(output_behavior["off_time_ms"] | 100, 1u, 10000u);
                        if (output_behavior.containsKey("auto_off")) {
                            button.output_behavior.auto_off = output_behavior["auto_off"] | false;
                        } else {
                            button.output_behavior.auto_off = (button.mode == "output");
                        }
                    }
                    
                    // Legacy fields (backward compatibility)
                    button.infinitybox_function = btn_obj["infinitybox_function"] | "";
                    button.flash_frequency = btn_obj["flash_frequency"] | 500;
                    button.fade_time = btn_obj["fade_time"] | 1000;
                    button.on_time = btn_obj["on_time"] | 2000;

                    page.buttons.push_back(std::move(button));
                    ++button_index;
                }
            }

            target.pages.push_back(std::move(page));
            ++page_index;
        }
    }

    if (target.pages.empty()) {
        target.pages = buildDefaultConfig().pages;
    }

    // Decode CAN library
    target.can_library.clear();
    JsonArrayConst can_library = json["can_library"].as<JsonArrayConst>();
    if (!can_library.isNull()) {
        std::size_t msg_index = 0;
        for (JsonObjectConst msg_obj : can_library) {
            if (msg_index >= 50) {  // Reasonable limit for CAN library
                break;
            }

            CanMessage msg;
            msg.id = safeString(msg_obj["id"], fallbackId("can_msg", msg_index));
            msg.name = safeString(msg_obj["name"], msg.id);
            msg.pgn = msg_obj["pgn"] | 0;
            msg.priority = clampValue<std::uint8_t>(msg_obj["priority"] | 6, 0u, 7u);
            msg.source_address = msg_obj["source_address"] | 0xF9;
            msg.destination_address = msg_obj["destination_address"] | 0xFF;
            msg.description = safeString(msg_obj["description"], "");

            JsonArrayConst data_arr = msg_obj["data"].as<JsonArrayConst>();
            if (!data_arr.isNull()) {
                std::size_t i = 0;
                for (JsonVariantConst byte_val : data_arr) {
                    if (i >= msg.data.size()) {
                        break;
                    }
                    msg.data[i] = clampValue<std::uint8_t>(byte_val | 0, 0u, 255u);
                    ++i;
                }
            }

            target.can_library.push_back(std::move(msg));
            ++msg_index;
        }
    }

    // Decode available fonts
    target.available_fonts.clear();
    JsonArrayConst fonts = json["available_fonts"].as<JsonArrayConst>();
    if (!fonts.isNull()) {
        for (JsonObjectConst font_obj : fonts) {
            FontConfig font;
            font.name = safeString(font_obj["name"], "montserrat_16");
            font.display_name = safeString(font_obj["display_name"], "Montserrat 16");
            font.size = clampValue<std::uint8_t>(font_obj["size"] | 16, 8, 72);
            target.available_fonts.push_back(std::move(font));
        }
    }
    
    // If no fonts defined, use default list
    if (target.available_fonts.empty()) {
        target.available_fonts = buildDefaultConfig().available_fonts;
    }

    return true;
}

// Clock persistence removed
