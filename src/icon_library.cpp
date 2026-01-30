#include "icon_library.h"
#include "ui_theme.h"
#include <algorithm>
#include <array>
#include <cctype>
#include <string>

namespace {

using Descriptor = IconLibrary::IconDescriptor;

constexpr std::array<Descriptor, 23> kIconTable = {{
    {IconType::HOME, "home", LV_SYMBOL_HOME},
    {IconType::WINDOWS, "windows", LV_SYMBOL_LIST},
    {IconType::LOCKS, "locks", LV_SYMBOL_CLOSE},
    {IconType::LIGHTS, "lights", LV_SYMBOL_EYE_OPEN},
    {IconType::WIPERS, "wipers", LV_SYMBOL_REFRESH},
    {IconType::CLIMATE, "climate", LV_SYMBOL_SETTINGS},
    {IconType::POWER, "power", LV_SYMBOL_POWER},
    {IconType::SETTINGS, "settings", LV_SYMBOL_SETTINGS},
    {IconType::NAVIGATION, "navigation", LV_SYMBOL_GPS},
    {IconType::AUDIO, "audio", LV_SYMBOL_AUDIO},
    {IconType::CAMERA, "camera", LV_SYMBOL_VIDEO},
    {IconType::WARNING, "warning", LV_SYMBOL_WARNING},
    {IconType::CHECK, "check", LV_SYMBOL_OK},
    {IconType::ARROW_UP, "arrow_up", LV_SYMBOL_UP, "up"},
    {IconType::ARROW_DOWN, "arrow_down", LV_SYMBOL_DOWN, "down"},
    {IconType::ARROW_LEFT, "arrow_left", LV_SYMBOL_LEFT, "left"},
    {IconType::ARROW_RIGHT, "arrow_right", LV_SYMBOL_RIGHT, "right"},
    {IconType::PLUS, "plus", LV_SYMBOL_PLUS, "+"},
    {IconType::MINUS, "minus", LV_SYMBOL_MINUS, "-"},
    {IconType::GEAR, "gear", LV_SYMBOL_SETTINGS},
    {IconType::OFFROAD, "offroad", LV_SYMBOL_GPS},
    {IconType::TOWING, "towing", LV_SYMBOL_DRIVE},
    {IconType::PARKING, "parking", "P"},
}};

} // namespace

lv_obj_t* IconLibrary::create_icon(lv_obj_t* parent, IconType icon_type, lv_color_t base_color) {
    if (icon_type == IconType::NONE) {
        return nullptr;
    }

    const IconDescriptor* info = descriptor(icon_type);
    if (!info || !info->symbol) {
        return nullptr;
    }

    // Create a label for the icon
    lv_obj_t* icon_label = lv_label_create(parent);
    lv_label_set_text(icon_label, info->symbol);
    
    // Get contrasting color for icon
    lv_color_t icon_color = get_contrasting_color(base_color);
    
    // Apply styling with larger font for icons
    lv_obj_set_style_text_color(icon_label, icon_color, 0);
    lv_obj_set_style_text_font(icon_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_opa(icon_label, LV_OPA_80, 0);
    
    return icon_label;
}

IconType IconLibrary::icon_from_string(const std::string& id) {
    const IconDescriptor* info = descriptor(id);
    return info ? info->type : IconType::NONE;
}

std::string IconLibrary::string_from_icon(IconType type) {
    const IconDescriptor* info = descriptor(type);
    return (info && info->id) ? std::string(info->id) : std::string("none");
}

const char* IconLibrary::get_icon_symbol(IconType type) {
    const IconDescriptor* info = descriptor(type);
    return info ? info->symbol : nullptr;
}

const IconLibrary::IconDescriptor* IconLibrary::descriptor(IconType type) {
    return find_descriptor(type);
}

const IconLibrary::IconDescriptor* IconLibrary::descriptor(const std::string& id) {
    if (id.empty()) {
        return nullptr;
    }

    const std::string normalized = normalize_identifier(id);
    return find_descriptor(normalized);
}

const IconLibrary::IconDescriptor* IconLibrary::find_descriptor(IconType type) {
    auto it = std::find_if(kIconTable.begin(), kIconTable.end(), [type](const Descriptor& entry) {
        return entry.type == type;
    });

    return (it != kIconTable.end()) ? &(*it) : nullptr;
}

const IconLibrary::IconDescriptor* IconLibrary::find_descriptor(const std::string& normalized_id) {
    if (normalized_id.empty()) {
        return nullptr;
    }

    auto it = std::find_if(kIconTable.begin(), kIconTable.end(), [&](const Descriptor& entry) {
        if (entry.id && normalized_id == entry.id) {
            return true;
        }

        if (entry.alias1 && normalized_id == entry.alias1) {
            return true;
        }

        if (entry.alias2 && normalized_id == entry.alias2) {
            return true;
        }

        if (entry.alias3 && normalized_id == entry.alias3) {
            return true;
        }

        return false;
    });

    return (it != kIconTable.end()) ? &(*it) : nullptr;
}

std::string IconLibrary::normalize_identifier(const std::string& id) {
    std::string lower = id;
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return lower;
}

lv_color_t IconLibrary::get_contrasting_color(lv_color_t base_color) {
    // Calculate luminance to determine if we need dark or light icon
    uint8_t r = (base_color.full >> 11) & 0x1F;
    uint8_t g = (base_color.full >> 5) & 0x3F;
    uint8_t b = base_color.full & 0x1F;
    
    // Simple luminance approximation
    uint16_t luminance = (r * 299 + g * 587 + b * 114) / 1000;
    
    // Return white for dark backgrounds, black for light backgrounds
    return (luminance < 16) ? lv_color_hex(0xFFFFFF) : lv_color_hex(0x000000);
}
