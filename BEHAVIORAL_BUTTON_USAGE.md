# Behavioral Scene Button Configuration Guide

## Overview

Touchscreen buttons can now activate behavioral scenes directly! This allows complex lighting behaviors (flashing, pulsing, sequential patterns) to be triggered with a single button press instead of sending raw CAN frames.

## How It Works

When you configure a button with a `behavioral_scene` property, the button will activate that scene instead of sending CAN frames when pressed.

## Configuration Example

In your button configuration JSON (stored in `config.json`), add the `behavioral_scene` field:

```json
{
  "pages": [
    {
      "name": "Lights",
      "buttons": [
        {
          "label": "Left Turn",
          "behavioral_scene": "left_turn"
        },
        {
          "label": "Right Turn",
          "behavioral_scene": "right_turn"
        },
        {
          "label": "4-Way Flashers",
          "behavioral_scene": "four_way"
        },
        {
          "label": "Beacon",
          "behavioral_scene": "beacon"
        }
      ]
    }
  ]
}
```

## Available Predefined Scenes

These scenes are automatically configured when the system starts:

### Turn Signals
- **`left_turn`** - Left turn signal (flashes at 1Hz)
- **`right_turn`** - Right turn signal (flashes at 1Hz)
- **`four_way`** - Hazard/4-way flashers (both sides flash together at 1Hz)

### Beacon
- **`beacon`** - Rotating beacon pattern (slow sweep effect)

## Button Behavior

### Press Event
When you press a button with `behavioral_scene` configured:
1. System checks if `behavioral_scene` field exists
2. If yes → Activates the scene using `behaviorEngine.activateScene(scene_name)`
3. If no → Falls back to sending CAN frames (original behavior)

### Release Event  
Currently, button release does NOT automatically deactivate scenes. Scenes continue running until:
- Another button activates a different scene (scenes can override each other)
- Scene is manually stopped via web interface
- System is reset

### Serial Output
When a button activates a scene, you'll see:
```
[UI] Activating behavioral scene: left_turn
```

## Mixing Behavioral and CAN Buttons

You can have both types of buttons on the same page:

```json
{
  "buttons": [
    {
      "label": "Left Turn",
      "behavioral_scene": "left_turn"  ← Activates behavioral scene
    },
    {
      "label": "Door Lock",
      "frames": [...]  ← Sends CAN frames (traditional)
    }
  ]
}
```

## Creating Custom Scenes

### Via Web Interface
1. Navigate to `http://192.168.4.250/behavioral` (or device IP)
2. Click "Create New Scene"
3. Configure outputs and behaviors
4. Save with a unique scene name
5. Add scene name to button configuration

### Via Code
Edit `behavioral_output_integration.h` and add your scene to `setupPredefinedScenes()`:

```cpp
inline void setupPredefinedScenes() {
    // ... existing scenes ...
    
    // Custom police lights scene
    Scene policeLights;
    policeLights.id = "police_lights";
    policeLights.description = "Police alternating pattern";
    
    SceneOutput leftRed;
    leftRed.outputId = "light_left";
    leftRed.behavior.type = BehaviorType::FLASH;
    leftRed.behavior.targetValue = 100;
    leftRed.behavior.period_ms = 400;  // Fast flash
    leftRed.behavior.dutyCycle = 50;
    policeLights.outputs.push_back(leftRed);
    
    SceneOutput rightBlue;
    rightBlue.outputId = "light_right";
    rightBlue.behavior.type = BehaviorType::FLASH;
    rightBlue.behavior.targetValue = 100;
    rightBlue.behavior.period_ms = 400;
    rightBlue.behavior.dutyCycle = 50;
    rightBlue.behavior.phaseOffset_ms = 200;  // Offset for alternating
    policeLights.outputs.push_back(rightBlue);
    
    behaviorEngine.addScene(policeLights);
}
```

Then in your button config:
```json
{
  "label": "Police",
  "behavioral_scene": "police_lights"
}
```

## Advantages Over CAN Frames

| Feature | CAN Frames | Behavioral Scenes |
|---------|------------|-------------------|
| **Setup** | Need to configure exact bytes | High-level behavior description |
| **Timing** | Manual frame timing | Automatic synchronized timing |
| **Complexity** | Limited to static states | Flash, pulse, fade, patterns, etc. |
| **Multiple Outputs** | One frame per output | Scene controls many outputs |
| **Web Control** | No web interface | Full CRUD via `/behavioral` |

## Troubleshooting

### Button Does Nothing
1. Check serial monitor for activation message
2. Verify scene name matches exactly (case-sensitive)
3. Ensure behavioral system initialized (check boot logs)

### Scene Not Found
Error message: `[Behavioral Output] Scene not found: xxx`
- Scene name typo in button config
- Scene not defined in `setupPredefinedScenes()`
- System failed to initialize (check earlier errors)

### Outputs Not Visible on CAN
- Verify CanManager is running
- Check CAN bus connections
- Monitor CAN frames with analyzer
- Outputs defined with correct cell/output numbers

## Next Steps

1. **Test Predefined Scenes**: Use the 4 built-in scenes to verify functionality
2. **Monitor Serial**: Watch for scene activation messages
3. **Check Web UI**: Visit `/behavioral` to see active scenes
4. **Create Custom Scenes**: Build scenes for your specific lighting needs
5. **Update Button Configs**: Replace CAN frames with behavioral scenes where appropriate

## Related Files

- **Button Handler**: `src/ui_builder.cpp` - `actionButtonEvent()` function
- **Scene Definitions**: `src/behavioral_output_integration.h` - `setupPredefinedScenes()`
- **Button Config**: `src/config_types.h` - `ButtonConfig` struct
- **Web Interface**: `/behavioral` endpoint for scene management

---

**Version 3.2.0** - Behavioral scene button integration deployed
