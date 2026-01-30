#pragma once

#include <lvgl.h>
#include <string>

/**
 * @file icon_library.h
 * Built-in icon library using Unicode symbols
 * 
 * Provides a collection of standard automotive icons using Unicode characters
 * without requiring image files. Automatically handles color contrast.
 */

enum class IconType {
    NONE,
    HOME,
    WINDOWS,
    LOCKS,
    LIGHTS,
    WIPERS,
    CLIMATE,
    POWER,
    SETTINGS,
    NAVIGATION,
    AUDIO,
    CAMERA,
    WARNING,
    CHECK,
    ARROW_UP,
    ARROW_DOWN,
    ARROW_LEFT,
    ARROW_RIGHT,
    PLUS,
    MINUS,
    GEAR,
    OFFROAD,
    TOWING,
    PARKING
};

class IconLibrary {
public:
    /**
     * @brief Metadata describing an icon entry.
     */
    struct IconDescriptor {
                constexpr IconDescriptor(IconType icon_type = IconType::NONE,
                                                                 const char* primary_id = nullptr,
                                                                 const char* icon_symbol = nullptr,
                                                                 const char* alt1 = nullptr,
                                                                 const char* alt2 = nullptr,
                                                                 const char* alt3 = nullptr)
            : type(icon_type),
              id(primary_id),
              symbol(icon_symbol),
              alias1(alt1),
              alias2(alt2),
              alias3(alt3) {}

        IconType type;
        const char* id;
        const char* symbol;
        const char* alias1;
        const char* alias2;
        const char* alias3;
    };

    /**
     * @brief Create an icon on a button with automatic color handling
     * @param parent Button object to draw icon on
     * @param icon_type Type of icon to create
     * @param base_color Base color of the button (icon will use contrasting color)
     * @return Created icon label object
     */
    static lv_obj_t* create_icon(lv_obj_t* parent, IconType icon_type, lv_color_t base_color);

    /**
     * @brief Get icon type from string identifier
     * @param id String identifier (e.g., "home", "windows", "locks")
     * @return IconType enum value
     */
    static IconType icon_from_string(const std::string& id);

    /**
     * @brief Get string identifier from icon type
     * @param type IconType enum value
     * @return String identifier
     */
    static std::string string_from_icon(IconType type);

    /**
     * @brief Retrieve immutable descriptor for a given icon type.
     */
    static const IconDescriptor* descriptor(IconType type);

    /**
     * @brief Retrieve immutable descriptor for a string identifier or alias.
     */
    static const IconDescriptor* descriptor(const std::string& id);

private:
    static const IconDescriptor* find_descriptor(IconType type);
    static const IconDescriptor* find_descriptor(const std::string& normalized_id);
    static std::string normalize_identifier(const std::string& id);
    static const char* get_icon_symbol(IconType type);
    static lv_color_t get_contrasting_color(lv_color_t base_color);
};
