/**
 * @file ui_theme.h
 * Bronco-themed design system for automotive HMI
 * 
 * Provides consistent colors, typography, spacing, and component styles
 * Dark theme with amber accents inspired by Ford Bronco
 */

#ifndef UI_THEME_H
#define UI_THEME_H

#include <lvgl.h>

class UITheme {
public:
    // ========== COLOR PALETTE ==========
    // Dark, rugged automotive theme
    static const lv_color_t COLOR_BG;           // #1A1A1A - Main background
    static const lv_color_t COLOR_SURFACE;      // #2A2A2A - Cards, panels
    static const lv_color_t COLOR_ACCENT;       // #FFA500 - Amber/Orange accent
    static const lv_color_t COLOR_TEXT_PRIMARY; // #FFFFFF - Main text
    static const lv_color_t COLOR_TEXT_SECONDARY; // #AAAAAA - Secondary text
    static const lv_color_t COLOR_SUCCESS;      // #00FF00 - Success/Active state
    static const lv_color_t COLOR_ERROR;        // #FF0000 - Error/Warning
    static const lv_color_t COLOR_BORDER;       // #3A3A3A - Borders, dividers

    // ========== SPACING ==========
    static const lv_coord_t SPACE_XS = 4;
    static const lv_coord_t SPACE_SM = 8;
    static const lv_coord_t SPACE_MD = 16;
    static const lv_coord_t SPACE_LG = 24;
    static const lv_coord_t SPACE_XL = 32;

    // ========== TYPOGRAPHY ==========
    static const lv_font_t* FONT_TITLE;    // Montserrat 32 for titles
    static const lv_font_t* FONT_HEADING;  // Montserrat 24 for headings
    static const lv_font_t* FONT_BODY;     // Montserrat 16 for body text
    static const lv_font_t* FONT_CAPTION;  // Montserrat 14 for captions

    // ========== COMPONENT SIZES ==========
    static const lv_coord_t TOP_BAR_HEIGHT = 60;
    static const lv_coord_t TILE_BUTTON_WIDTH = 160;
    static const lv_coord_t TILE_BUTTON_HEIGHT = 120;
    static const lv_coord_t TOGGLE_WIDTH = 60;
    static const lv_coord_t TOGGLE_HEIGHT = 30;
    static const lv_coord_t MIN_TOUCH_SIZE = 80;  // Minimum for automotive touch

    // ========== BORDERS & RADIUS ==========
    static const lv_coord_t RADIUS_SM = 4;
    static const lv_coord_t RADIUS_MD = 8;
    static const lv_coord_t RADIUS_LG = 12;
    static const lv_coord_t BORDER_WIDTH = 2;

    // ========== OPACITY ==========
    static const lv_opa_t OPA_FULL = LV_OPA_COVER;
    static const lv_opa_t OPA_HIGH = LV_OPA_80;
    static const lv_opa_t OPA_MED = LV_OPA_50;
    static const lv_opa_t OPA_LOW = LV_OPA_30;
    static const lv_opa_t OPA_NONE = LV_OPA_TRANSP;

    // ========== ANIMATION ==========
    static const uint32_t ANIM_TIME_FAST = 150;
    static const uint32_t ANIM_TIME_NORMAL = 300;
    static const uint32_t ANIM_TIME_SLOW = 500;

    // ========== INITIALIZATION ==========
    /**
     * @brief Initialize the theme
     * Must be called after lv_init() but before creating UI
     */
    static void init();

    // ========== STYLE HELPERS ==========
    /**
     * @brief Apply base screen style (dark background)
     */
    static void apply_screen_style(lv_obj_t* obj);

    /**
     * @brief Apply card/surface style (elevated panel)
     */
    static void apply_card_style(lv_obj_t* obj);

    /**
     * @brief Apply button style (primary action)
     */
    static void apply_button_style(lv_obj_t* obj, bool accent = false);

    /**
     * @brief Apply label style with specified font and color
     */
    static void apply_label_style(lv_obj_t* label, const lv_font_t* font, lv_color_t color);

    /**
     * @brief Apply focus/pressed animation to object
     */
    static void apply_press_anim(lv_obj_t* obj);

private:
    static bool initialized;
    
    // Pre-allocated styles for performance
    static lv_style_t style_screen;
    static lv_style_t style_card;
    static lv_style_t style_button;
    static lv_style_t style_button_accent;
    static lv_style_t style_button_pressed;
};

#endif // UI_THEME_H
