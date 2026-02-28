/**
 * @file ui_theme.cpp
 * Bronco-themed design system implementation
 */

#include "ui_theme.h"

// ========== COLOR DEFINITIONS ==========
const lv_color_t UITheme::COLOR_BG = lv_color_hex(0x1A1A1A);
const lv_color_t UITheme::COLOR_SURFACE = lv_color_hex(0x2A2A2A);
const lv_color_t UITheme::COLOR_ACCENT = lv_color_hex(0xFFA500);  // Amber
const lv_color_t UITheme::COLOR_TEXT_PRIMARY = lv_color_hex(0xFFFFFF);
const lv_color_t UITheme::COLOR_TEXT_SECONDARY = lv_color_hex(0xAAAAAA);
const lv_color_t UITheme::COLOR_SUCCESS = lv_color_hex(0x00FF00);
const lv_color_t UITheme::COLOR_ERROR = lv_color_hex(0xFF0000);
const lv_color_t UITheme::COLOR_BORDER = lv_color_hex(0x3A3A3A);

// ========== FONT ASSIGNMENTS ==========
const lv_font_t* UITheme::FONT_TITLE = &lv_font_montserrat_32;
const lv_font_t* UITheme::FONT_HEADING = &lv_font_montserrat_24;
const lv_font_t* UITheme::FONT_BODY = &lv_font_montserrat_16;
const lv_font_t* UITheme::FONT_CAPTION = &lv_font_montserrat_12;

// ========== STATIC MEMBERS ==========
bool UITheme::initialized = false;
lv_style_t UITheme::style_screen;
lv_style_t UITheme::style_card;
lv_style_t UITheme::style_button;
lv_style_t UITheme::style_button_accent;
lv_style_t UITheme::style_button_pressed;

void UITheme::init() {
    if (initialized) return;
    
    // ========== SCREEN STYLE ==========
    lv_style_init(&style_screen);
    lv_style_set_bg_color(&style_screen, COLOR_BG);
    lv_style_set_bg_opa(&style_screen, OPA_FULL);
    lv_style_set_pad_all(&style_screen, 0);
    lv_style_set_border_width(&style_screen, 0);

    // ========== CARD STYLE ==========
    lv_style_init(&style_card);
    lv_style_set_bg_color(&style_card, COLOR_SURFACE);
    lv_style_set_bg_opa(&style_card, OPA_FULL);
    lv_style_set_radius(&style_card, RADIUS_MD);
    lv_style_set_border_color(&style_card, COLOR_BORDER);
    lv_style_set_border_width(&style_card, BORDER_WIDTH);
    lv_style_set_pad_all(&style_card, SPACE_MD);

    // ========== BUTTON STYLE ==========
    lv_style_init(&style_button);
    lv_style_set_bg_color(&style_button, COLOR_SURFACE);
    lv_style_set_bg_opa(&style_button, OPA_FULL);
    lv_style_set_radius(&style_button, RADIUS_MD);
    lv_style_set_border_color(&style_button, COLOR_BORDER);
    lv_style_set_border_width(&style_button, BORDER_WIDTH);
    lv_style_set_pad_all(&style_button, SPACE_MD);
    lv_style_set_text_color(&style_button, COLOR_TEXT_PRIMARY);
    lv_style_set_text_font(&style_button, FONT_BODY);

    // ========== ACCENT BUTTON STYLE ==========
    lv_style_init(&style_button_accent);
    lv_style_set_bg_color(&style_button_accent, COLOR_ACCENT);
    lv_style_set_bg_opa(&style_button_accent, OPA_FULL);
    lv_style_set_radius(&style_button_accent, RADIUS_MD);
    lv_style_set_border_width(&style_button_accent, 0);
    lv_style_set_pad_all(&style_button_accent, SPACE_MD);
    lv_style_set_text_color(&style_button_accent, COLOR_BG);
    lv_style_set_text_font(&style_button_accent, FONT_BODY);

    // ========== PRESSED STATE STYLE ==========
    lv_style_init(&style_button_pressed);
    lv_style_set_bg_opa(&style_button_pressed, 200);  // Slightly dimmer
    // Removed transform width/height to prevent button jump

    initialized = true;
}

void UITheme::apply_screen_style(lv_obj_t* obj) {
    if (!initialized) init();
    lv_obj_add_style(obj, &style_screen, 0);
}

void UITheme::apply_card_style(lv_obj_t* obj) {
    if (!initialized) init();
    lv_obj_add_style(obj, &style_card, 0);
}

void UITheme::apply_button_style(lv_obj_t* obj, bool accent) {
    if (!initialized) init();
    
    if (accent) {
        lv_obj_add_style(obj, &style_button_accent, 0);
    } else {
        lv_obj_add_style(obj, &style_button, 0);
    }
    
    // Add pressed state
    lv_obj_add_style(obj, &style_button_pressed, LV_STATE_PRESSED);
}

void UITheme::apply_label_style(lv_obj_t* label, const lv_font_t* font, lv_color_t color) {
    lv_obj_set_style_text_font(label, font, 0);
    lv_obj_set_style_text_color(label, color, 0);
}

void UITheme::apply_press_anim(lv_obj_t* obj) {
    // Enable animations for this object
    lv_obj_set_style_anim_time(obj, ANIM_TIME_FAST, 0);
}
