#pragma once

/**
 * ╔═══════════════════════════════════════════════════════════════════════════╗
 * ║  BEHAVIORAL OUTPUT - TOUCHSCREEN UI INTEGRATION                           ║
 * ║                                                                           ║
 * ║  Integrates behavioral output system with the LVGL touchscreen interface ║
 * ╚═══════════════════════════════════════════════════════════════════════════╝
 */

#include <lvgl.h>
#include "output_behavior_engine.h"

namespace BehavioralUI {

using namespace BehavioralOutput;

// ═══════════════════════════════════════════════════════════════════════════
// TOUCHSCREEN UI HELPERS
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Create a button that activates a scene when pressed
 * 
 * @param parent Parent LVGL container
 * @param label Button text to display
 * @param scene_id Scene ID to activate
 * @param color Button color (LVGL color)
 * @return LVGL button object
 */
inline lv_obj_t* createSceneButton(lv_obj_t* parent, const char* label, 
                                   const char* scene_id, lv_color_t color) {
    lv_obj_t* btn = lv_btn_create(parent);
    lv_obj_set_size(btn, LV_SIZE_CONTENT, 60);
    lv_obj_set_style_bg_color(btn, color, LV_PART_MAIN);
    lv_obj_set_style_radius(btn, 12, LV_PART_MAIN);
    
    lv_obj_t* btn_label = lv_label_create(btn);
    lv_label_set_text(btn_label, label);
    lv_obj_center(btn_label);
    
    // Store scene ID as user data
    lv_obj_set_user_data(btn, (void*)scene_id);
    
    return btn;
}

/**
 * Create a button that applies a behavior to an output
 * 
 * @param parent Parent LVGL container
 * @param label Button text
 * @param output_id Output channel ID
 * @param behavior Behavior to apply
 * @param color Button color
 * @return LVGL button object
 */
inline lv_obj_t* createBehaviorButton(lv_obj_t* parent, const char* label,
                                      const char* output_id,
                                      const OutputBehavior& behavior,
                                      lv_color_t color) {
    lv_obj_t* btn = lv_btn_create(parent);
    lv_obj_set_size(btn, LV_SIZE_CONTENT, 60);
    lv_obj_set_style_bg_color(btn, color, LV_PART_MAIN);
    lv_obj_set_style_radius(btn, 12, LV_PART_MAIN);
    
    lv_obj_t* btn_label = lv_label_create(btn);
    lv_label_set_text(btn_label, label);
    lv_obj_center(btn_label);
    
    // Store output ID as user data
    lv_obj_set_user_data(btn, (void*)output_id);
    
    return btn;
}

/**
 * Event handler for scene activation buttons
 * 
 * Usage:
 *   lv_obj_add_event_cb(btn, sceneButtonEvent, LV_EVENT_CLICKED, &behaviorEngine);
 */
inline void sceneButtonEvent(lv_event_t* e) {
    BehaviorEngine* engine = (BehaviorEngine*)lv_event_get_user_data(e);
    lv_obj_t* btn = lv_event_get_target(e);
    const char* scene_id = (const char*)lv_obj_get_user_data(btn);
    
    if (engine && scene_id) {
        Serial.printf("[Behavioral UI] Activating scene: %s\n", scene_id);
        engine->activateScene(scene_id);
    }
}

/**
 * Event handler for behavior buttons
 * 
 * Usage:
 *   lv_obj_add_event_cb(btn, behaviorButtonEvent, LV_EVENT_CLICKED, &behaviorEngine);
 */
inline void behaviorButtonEvent(lv_event_t* e) {
    BehaviorEngine* engine = (BehaviorEngine*)lv_event_get_user_data(e);
    lv_obj_t* btn = lv_event_get_target(e);
    const char* output_id = (const char*)lv_obj_get_user_data(btn);
    
    if (engine && output_id) {
        // Toggle the output (stop if active, start if inactive)
        Serial.printf("[Behavioral UI] Toggling output: %s\n", output_id);
        
        // You could also store a specific behavior as user data
        // For now, just stop the output
        OutputBehavior stopBehavior;
        stopBehavior.type = BehaviorType::STEADY;
        stopBehavior.target_value = 0;
        stopBehavior.duration_ms = 0;
        
        engine->setBehavior(output_id, stopBehavior);
    }
}

/**
 * Build a behavioral controls page with common scenes
 * 
 * @param parent Parent container for the page
 * @param engine Pointer to BehaviorEngine instance
 */
inline void buildBehavioralPage(lv_obj_t* parent, BehaviorEngine* engine) {
    // Create a grid layout for buttons
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(parent, 16, LV_PART_MAIN);
    lv_obj_set_style_pad_row(parent, 12, LV_PART_MAIN);
    
    // Title
    lv_obj_t* title = lv_label_create(parent);
    lv_label_set_text(title, "Behavioral Output Control");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFA500), LV_PART_MAIN);
    
    // Turn Signal Section
    lv_obj_t* turn_label = lv_label_create(parent);
    lv_label_set_text(turn_label, "Turn Signals:");
    lv_obj_set_style_text_font(turn_label, &lv_font_montserrat_16, LV_PART_MAIN);
    
    lv_obj_t* left_btn = createSceneButton(parent, "◀️ Left Turn", "left_turn", 
                                           lv_color_hex(0xFF9D2E));
    lv_obj_add_event_cb(left_btn, sceneButtonEvent, LV_EVENT_CLICKED, engine);
    
    lv_obj_t* right_btn = createSceneButton(parent, "▶️ Right Turn", "right_turn",
                                            lv_color_hex(0xFF9D2E));
    lv_obj_add_event_cb(right_btn, sceneButtonEvent, LV_EVENT_CLICKED, engine);
    
    lv_obj_t* hazard_btn = createSceneButton(parent, "⚠️ Hazards (4-Way)", "four_way",
                                             lv_color_hex(0xFFD93D));
    lv_obj_add_event_cb(hazard_btn, sceneButtonEvent, LV_EVENT_CLICKED, engine);
    
    // Emergency Section
    lv_obj_t* emergency_label = lv_label_create(parent);
    lv_label_set_text(emergency_label, "Emergency:");
    lv_obj_set_style_text_font(emergency_label, &lv_font_montserrat_16, LV_PART_MAIN);
    
    lv_obj_t* beacon_btn = createSceneButton(parent, "🚨 Emergency Beacon", "beacon",
                                             lv_color_hex(0xFF6B6B));
    lv_obj_add_event_cb(beacon_btn, sceneButtonEvent, LV_EVENT_CLICKED, engine);
    
    // Stop All
    lv_obj_t* stop_btn = createSceneButton(parent, "⏹️ Stop All Outputs", "",
                                           lv_color_hex(0x444444));
    lv_obj_add_event_cb(stop_btn, [](lv_event_t* e) {
        BehaviorEngine* engine = (BehaviorEngine*)lv_event_get_user_data(e);
        if (engine) {
            Serial.println("[Behavioral UI] Stopping all outputs");
            engine->stopAll();
        }
    }, LV_EVENT_CLICKED, engine);
    
    // Info text
    lv_obj_t* info = lv_label_create(parent);
    lv_label_set_text(info, "Access http://192.168.7.116/behavioral\nfor advanced controls");
    lv_obj_set_style_text_font(info, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_set_style_text_color(info, lv_color_hex(0x888888), LV_PART_MAIN);
    lv_obj_set_style_text_align(info, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
}

} // namespace BehavioralUI
