#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

// Limits that align with the documentation
constexpr std::size_t MAX_PAGES = 20;
constexpr std::size_t MAX_BUTTONS_PER_PAGE = 12;

constexpr const char kOtaManifestUrl[] =
    "https://image-optimizer-still-flower-1282.fly.dev/ota/manifest";

struct CanFrameConfig {
    bool enabled = false;
    std::uint32_t pgn = 0x00FF00;  // Default to proprietary B frame
    std::uint8_t priority = 3;
    std::uint8_t source_address = 0x80;
    std::uint8_t destination_address = 0xFF;  // Broadcast by default
    std::array<std::uint8_t, 8> data{};
    std::uint8_t length = 0;  // Actual number of data bytes to transmit (0-8)
};

struct ButtonConfig {
    std::string id = "button_0";
    std::string label = "Button";
    std::string color = "#FFA500";
    std::string pressed_color = "#FF8800";
    std::string text_color = "";  // Optional button text color (empty = use page/theme default)
    std::string icon = "";  // Reserved for future icon uploads
    std::uint8_t row = 0;
    std::uint8_t col = 0;
    std::uint8_t row_span = 1;
    std::uint8_t col_span = 1;
    bool momentary = false;
    std::uint8_t font_size = 24;
    std::string font_family = "montserrat";  // montserrat or unscii
    std::string font_weight = "400";  // 300, 400, 500, 600, 700, 800
    std::string font_name = "montserrat_16";  // Specific font identifier
    std::string text_align = "center";  // top-left, top-center, top-right, center, bottom-left, bottom-center, bottom-right
    std::uint8_t corner_radius = 12;  // Button corner radius in pixels
    std::uint8_t border_width = 0;  // Button border width in pixels
    std::string border_color = "#FFFFFF";  // Button border color
    CanFrameConfig can;
    CanFrameConfig can_off;  // Optional OFF/release frame (used by some modules that require release frames)
    
    // ═══════════════════════════════════════════════════════════════════════
    // BEHAVIORAL OUTPUT SYSTEM - User-Friendly Approach
    // ═══════════════════════════════════════════════════════════════════════
    
    // Mode Selection: "can" (traditional CAN frames), "output" (single output with behavior), or "scene" (complex multi-output scene)
    std::string mode = "can";  // "can", "output", or "scene"
    
    // SIMPLE OUTPUT MODE: Control a single output with a behavior
    struct OutputBehaviorConfig {
        std::string output_id = "";           // Which output to control (user-defined from output manager)
        std::string action = "on";           // on, off, toggle
        std::string behavior_type = "steady"; // steady, flash, pulse, fade_in, fade_out, strobe, hold_timed, ramp
        std::uint8_t target_value = 100;     // 0-100%
        std::uint16_t period_ms = 500;        // For flash, pulse, strobe
        std::uint8_t duty_cycle = 50;         // For flash (0-100%)
        std::uint16_t fade_time_ms = 1000;    // For fade_in, fade_out, ramp
        std::uint16_t hold_duration_ms = 0;   // For hold_timed (0 = infinite)
        std::uint16_t on_time_ms = 100;       // For strobe
        std::uint16_t off_time_ms = 100;      // For strobe
        bool auto_off = false;                 // Release automatically when done
    } output_behavior;
    
    // SCENE MODE: Activate a predefined scene (created in scene builder)
    std::string scene_id = "";  // ID of scene to activate
    std::string scene_action = "on";  // on, off, toggle
    std::uint16_t scene_duration_ms = 0;  // 0 = indefinite
    bool scene_release_off = false;  // Release to OFF for scenes
    
    // Legacy/Deprecated (kept for backward compatibility)
    std::string infinitybox_function = "";  // DEPRECATED - use mode="output" instead
    std::string behavioral_scene = "";      // DEPRECATED - use mode="scene" and scene_id instead
    std::uint16_t flash_frequency = 500;    // DEPRECATED - use output_behavior.period_ms
    std::uint16_t fade_time = 1000;         // DEPRECATED - use output_behavior.fade_time_ms
    std::uint16_t on_time = 2000;           // DEPRECATED - use output_behavior.hold_duration_ms
};

struct PageConfig {
    std::string id = "page_0";
    std::string name = "Home";
    std::string nav_text = "";          // Optional nav label override
    std::string nav_color = "";             // Active nav button color
    std::string nav_inactive_color = "";    // Inactive nav button color
    std::string nav_text_color = "";        // Optional nav text color override
    std::int16_t nav_button_radius = -1;     // Optional nav button radius (-1 inherits theme)
    std::string bg_color = "";              // Optional per-page background
    std::string text_color = "";            // Optional per-page text color
    std::string button_color = "";          // Optional per-page button fill
    std::string button_pressed_color = "";  // Optional per-page pressed fill
    std::string button_border_color = "";   // Optional per-page button border color
    std::uint8_t button_border_width = 0;    // Optional per-page border width (0 means inherit)
    std::uint8_t button_radius = 0;          // Optional per-page radius (0 means inherit)
    std::uint8_t rows = 2;
    std::uint8_t cols = 2;
    std::vector<ButtonConfig> buttons;
};

struct FontConfig {
    std::string name = "montserrat_16";
    std::string display_name = "Montserrat 16";
    std::uint8_t size = 16;
};

struct WifiCredentials {
    bool enabled = true;
    std::string ssid = "CAN-Control";
    std::string password = "";
};

struct WifiConfig {
    WifiCredentials ap{};
    WifiCredentials sta{};

    WifiConfig() {
        sta.enabled = false;
        sta.ssid.clear();
        sta.password.clear();
    }
};

struct OTAConfig {
    bool enabled = true;
    // bool auto_apply = false;  // Removed - manual-only
    std::string manifest_url = kOtaManifestUrl;
    std::string channel = "stable";
    // std::uint32_t check_interval_minutes = 60;  // Removed - manual-only
};

struct HeaderConfig {
    std::string title = "CAN Control";
    std::string subtitle = "Configuration Interface";
    bool show_logo = true;
    std::string logo_variant = "";  // Empty by default - custom logos only
    std::string logo_base64 = "";  // Custom uploaded logo (base64 encoded image)
    std::string title_font = "montserrat_24";
    std::string subtitle_font = "montserrat_12";
    std::string title_align = "center";  // "left", "center", "right"
    std::string logo_position = "stacked"; // "stacked", "inline-left", "inline-right"
    std::uint16_t logo_target_height = 64;   // Desired on-device logo height in px
    bool logo_preserve_aspect = true;        // Whether uploads should keep original aspect
    std::uint8_t nav_spacing = 12;           // Gap between header and nav (px)
};

struct ImageAssets {
    std::string header_logo = "";      // Header logo (max 48x36, PNG with alpha)
    std::string splash_logo = "";      // Splash screen logo (max 400x300, PNG with alpha)
    std::string background_image = ""; // Background image (800x480, JPG or PNG)
    std::string sleep_logo = "";       // Sleep overlay logo (max 200x150, PNG with alpha)
};

struct DisplayConfig {
    std::uint8_t brightness = 100;  // 0-100 percent
    bool sleep_enabled = false;
    std::uint16_t sleep_timeout_seconds = 60;  // idle timeout before sleep overlay
    std::string sleep_icon_base64 = "";  // Custom sleep image (PNG/JPG base64) - DEPRECATED, use ImageAssets
};

struct ThemeConfig {
    std::string bg_color = "#1A1A1A";
    std::string surface_color = "#2A2A2A";
    std::string page_bg_color = "#0F0F0F";
    std::string accent_color = "#FFA500";
    std::string text_primary = "#FFFFFF";
    std::string text_secondary = "#AAAAAA";
    std::string border_color = "#3A3A3A";
    std::string header_border_color = "#FFA500";
    std::string nav_button_color = "#3A3A3A";
    std::string nav_button_active_color = "#FFA500";
    std::string nav_button_text_color = "#FFFFFF";
    std::uint8_t nav_button_radius = 20;
    std::uint8_t button_radius = 12;
    std::uint8_t border_width = 2;
    std::uint8_t header_border_width = 0;
};

struct CanMessage {
    std::string id = "msg_0";
    std::string name = "Unnamed";
    std::uint32_t pgn = 0x00FF00;
    std::uint8_t priority = 3;
    std::uint8_t source_address = 0x80;
    std::uint8_t destination_address = 0xFF;
    std::array<std::uint8_t, 8> data{};
    std::string description = "";
};

struct DeviceConfig {
    std::string version = "1.0.0";
    WifiConfig wifi{};
    OTAConfig ota{};
    HeaderConfig header{};
    ThemeConfig theme{};
    DisplayConfig display{};
    ImageAssets images{};
    std::vector<PageConfig> pages;
    std::vector<CanMessage> can_library;
    std::vector<FontConfig> available_fonts;  // List of available fonts for UI
};
