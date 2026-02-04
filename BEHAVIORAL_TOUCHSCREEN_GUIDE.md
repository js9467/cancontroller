# Behavioral Output - Touchscreen Integration Guide

## Overview

This guide shows you how to add behavioral output controls to the physical touchscreen UI (LVGL interface).

## Integration Methods

### Method 1: Add as a New Page (Recommended)

Add a dedicated "Behavioral" page to the navigation bar:

#### Step 1: Modify `ui_builder.cpp`

Add to the includes section:
```cpp
#include "behavioral_ui_integration.h"
#include "behavioral_output_integration.h"  // For engine reference
```

#### Step 2: Add Navigation Button

In the `buildNavigation()` function, after building the regular pages, add:

```cpp
// Add Behavioral page button
lv_obj_t* behavioral_btn = lv_btn_create(nav_bar_);
lv_obj_set_size(behavioral_btn, 80, 50);
lv_obj_set_style_bg_color(behavioral_btn, lv_color_hex(0x3A3A3A), LV_PART_MAIN);
lv_obj_set_style_bg_color(behavioral_btn, lv_color_hex(0xFFA500), LV_PART_MAIN | LV_STATE_CHECKED);

lv_obj_t* label = lv_label_create(behavioral_btn);
lv_label_set_text(label, "Behaviors");
lv_obj_center(label);

lv_obj_add_event_cb(behavioral_btn, [](lv_event_t* e) {
    UIBuilder::instance().showBehavioralPage();
}, LV_EVENT_CLICKED, nullptr);

nav_buttons_.push_back(behavioral_btn);
```

#### Step 3: Add Page Builder Method

Add to `ui_builder.h`:
```cpp
void showBehavioralPage();
```

Add to `ui_builder.cpp`:
```cpp
void UIBuilder::showBehavioralPage() {
    // Clear current page
    if (page_container_) {
        lv_obj_del(page_container_);
        page_container_ = nullptr;
    }
    
    // Create new page container
    page_container_ = lv_obj_create(base_screen_);
    lv_obj_set_size(page_container_, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_align(page_container_, LV_ALIGN_TOP_MID, 0, 100);  // Below header
    lv_obj_set_style_bg_opa(page_container_, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(page_container_, 0, LV_PART_MAIN);
    
    // Build behavioral controls
    BehavioralUI::buildBehavioralPage(page_container_, &behaviorEngine);
    
    // Update navigation selection
    active_page_ = nav_buttons_.size() - 1;  // Last button
    updateNavSelection();
}
```

---

### Method 2: Add to Existing Button CAN Configuration

You can make **any button** on the touchscreen trigger a behavioral scene by configuring it in the JSON:

#### Example: Button Configuration

```json
{
  "id": "left_turn_btn",
  "label": "Left Turn",
  "color": "#FF9D2E",
  "pressed_color": "#FF8800",
  "row": 0,
  "col": 0,
  "behavioral_scene": "left_turn"  // This triggers the scene!
}
```

#### Implement in `ui_builder.cpp`

In the `actionButtonEvent` handler, add:

```cpp
void UIBuilder::actionButtonEvent(lv_event_t* e) {
    lv_obj_t* btn = lv_event_get_target(e);
    ButtonConfig* cfg = (ButtonConfig*)lv_obj_get_user_data(btn);
    
    if (!cfg) return;
    
    // Check if button has behavioral scene configured
    if (!cfg->behavioral_scene.empty()) {
        behaviorEngine.activateScene(cfg->behavioral_scene.c_str());
        Serial.printf("[UI] Activated behavioral scene: %s\n", 
                     cfg->behavioral_scene.c_str());
        return;
    }
    
    // Existing CAN/Infinitybox logic...
}
```

Add to `config_types.h` in `ButtonConfig` struct:
```cpp
std::string behavioral_scene = "";  // Behavioral scene to activate
```

---

### Method 3: Quick Integration (For Testing)

Add a temporary button to any existing page:

```cpp
// In any page builder function (e.g., buildPage)
lv_obj_t* test_btn = lv_btn_create(page_container_);
lv_obj_set_size(test_btn, 200, 60);
lv_obj_align(test_btn, LV_ALIGN_CENTER, 0, 0);

lv_obj_t* label = lv_label_create(test_btn);
lv_label_set_text(label, "Test Left Turn");
lv_obj_center(label);

lv_obj_add_event_cb(test_btn, [](lv_event_t* e) {
    behaviorEngine.activateScene("left_turn");
    Serial.println("[UI] Left turn activated!");
}, LV_EVENT_CLICKED, nullptr);
```

---

## Accessing BehaviorEngine in UI

The `behaviorEngine` is a global instance declared in `behavioral_output_integration.h`:

```cpp
extern BehavioralOutput::BehaviorEngine behaviorEngine;
```

To use it in `ui_builder.cpp`:
1. Add `#include "behavioral_output_integration.h"`
2. Reference `behaviorEngine` directly

---

## Available Predefined Scenes

These scenes are configured by default:

| Scene ID | Description |
|----------|-------------|
| `left_turn` | Left turn signal (1.5Hz flash) |
| `right_turn` | Right turn signal (1.5Hz flash) |
| `four_way` | Hazard flashers (both signals) |
| `beacon` | Emergency beacon strobe (120 BPM) |

---

## Example: Full Behavioral Page

Here's a complete example of adding a "Behavioral" page to your UI:

### 1. Add to `ui_builder.h`:
```cpp
private:
    void buildBehavioralControlPage();
    lv_obj_t* behavioral_page_btn_ = nullptr;
```

### 2. Add to `ui_builder.cpp`:

```cpp
#include "behavioral_ui_integration.h"
#include "behavioral_output_integration.h"

void UIBuilder::buildNavigation() {
    // ... existing navigation code ...
    
    // Add Behavioral page button at the end
    behavioral_page_btn_ = lv_btn_create(nav_bar_);
    lv_obj_set_size(behavioral_page_btn_, 90, 50);
    
    lv_obj_t* label = lv_label_create(behavioral_page_btn_);
    lv_label_set_text(label, "🎛️ Lights");
    lv_obj_center(label);
    
    lv_obj_add_event_cb(behavioral_page_btn_, navButtonEvent, LV_EVENT_CLICKED,
                       (void*)(config_->pages.size()));  // Page index beyond normal pages
    
    nav_buttons_.push_back(behavioral_page_btn_);
}

void UIBuilder::buildPage(std::size_t index) {
    // Check if it's the behavioral page
    if (index == config_->pages.size()) {
        buildBehavioralControlPage();
        return;
    }
    
    // ... existing page building code ...
}

void UIBuilder::buildBehavioralControlPage() {
    if (page_container_) {
        lv_obj_del(page_container_);
        page_container_ = nullptr;
    }
    
    page_container_ = lv_obj_create(base_screen_);
    lv_obj_set_size(page_container_, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_align(page_container_, LV_ALIGN_TOP_MID, 0, 120);
    lv_obj_set_style_bg_color(page_container_, lv_color_hex(0x0F0F0F), LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(page_container_, LV_SCROLLBAR_MODE_AUTO);
    
    // Use the helper function to build the UI
    BehavioralUI::buildBehavioralPage(page_container_, &behaviorEngine);
}
```

---

## Web UI Access

For advanced configuration, users can access the web interface at:
- **AP Mode**: `http://192.168.4.250/behavioral`
- **Station Mode**: `http://192.168.7.116/behavioral` (or device IP)

The web UI provides:
- Custom output creation
- Behavior configuration (flash, fade, pulse, etc.)
- Pattern editor
- Scene builder
- Live preview

---

## Testing

1. **Build and upload**: `pio run -e waveshare_7in -t upload`
2. **Power on device**: Touchscreen should show new "Behavioral" or "Lights" button
3. **Tap the button**: Navigate to behavioral controls page
4. **Tap a scene**: e.g., "Left Turn" - outputs should activate
5. **Monitor serial**: Check for `[Behavioral UI]` log messages

---

## Troubleshooting

**Button does nothing:**
- Check serial output for error messages
- Verify `initBehavioralOutputSystem()` was called in `main.cpp`
- Confirm predefined scenes were loaded

**Outputs not visible on CAN:**
- Check CanManager is initialized
- Verify POWERCELL device is connected
- Monitor CAN bus traffic

**Scene not found:**
- Check scene IDs match exactly (case-sensitive)
- Verify `setupPredefinedScenes()` was called
- Use web UI to view available scenes

---

## Next Steps

1. **Customize scenes**: Edit `behavioral_output_integration.h` to add more predefined scenes
2. **Add custom outputs**: Define additional POWERCELL outputs for your hardware
3. **Create behaviors**: Use web UI to configure flash patterns, fades, and timings
4. **Build complex scenes**: Coordinate multiple outputs for intricate lighting sequences
