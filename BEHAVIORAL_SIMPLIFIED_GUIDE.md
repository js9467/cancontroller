# Behavioral Output System - Simplified User Guide

## Overview

The new behavioral output system has been redesigned for simplicity and user-friendliness. No more confusing predefined outputs or hardcoded scenes!

## Three Ways to Use Buttons

### 1. **CAN Mode** (Traditional)
Send raw CAN frames - same as before.
```json
{
  "mode": "can",
  "can": { "pgn": 0x00FF00, "data": [1, 2, 3], ... }
}
```

### 2. **Output Mode** (Simple Behaviors)  
Control a single output with a behavior - no coding required!
```json
{
  "mode": "output",
  "output_behavior": {
    "output_id": "left_turn",
    "behavior_type": "flash",
    "target_value": 100,
    "period_ms": 500,
    "duty_cycle": 50
  }
}
```

**Available Behaviors:**
- `steady` - Constant on
- `flash` - On/off blinking
- `pulse` - Smooth fade in/out
- `fade_in` - Gradual brightness increase
- `fade_out` - Gradual brightness decrease
- `strobe` - Fast on/off with custom timing
- `hold_timed` - On for X milliseconds then auto-off
- `ramp` - Gradual value change

### 3. **Scene Mode** (Complex Choreography)
Activate a scene you built with multiple synchronized outputs.
```json
{
  "mode": "scene",
  "scene_id": "police_lights"
}
```

## Creating Outputs

Visit `http://[device-ip]/behavioral` to create outputs:

1. Click **"Create New Output"**
2. Fill in:
   - **ID**: Unique name (e.g., `left_turn`)
   - **Name**: Display name (e.g., "Left Turn Signal")
   - **Cell Address**: POWERCELL device number
   - **Output Number**: Which output on that cell
3. Click **"Save Output"**

Now this output is available for buttons to use!

## Button Configuration Examples

### Example 1: Simple Flash
```json
{
  "label": "Left Turn",
  "mode": "output",
  "output_behavior": {
    "output_id": "left_turn",
    "behavior_type": "flash",
    "target_value": 100,
    "period_ms": 500,
    "duty_cycle": 50
  }
}
```

### Example 2: Timed Hold
```json
{
  "label": "Horn",
  "mode": "output",
  "momentary": true,
  "output_behavior": {
    "output_id": "horn",
    "behavior_type": "hold_timed",
    "target_value": 100,
    "hold_duration_ms": 2000,
    "auto_off": true
  }
}
```

### Example 3: Fade-In Headlights
```json
{
  "label": "Headlights",
  "mode": "output",
  "output_behavior": {
    "output_id": "headlights",
    "behavior_type": "fade_in",
    "target_value": 100,
    "fade_time_ms": 1500
  }
}
```

## Creating Scenes

For complex multi-output patterns (like emergency lights, turn signals with markers, etc.):

1. Visit `/behavioral`
2. Click **"Create New Scene"**
3. Add multiple outputs with different behaviors
4. Save with a unique scene ID
5. Use `mode: "scene"` in your button config

## Migration from Old System

If you have buttons with `behavioral_scene` field, they still work (backward compatibility), but update them to the new format:

**Old:**
```json
{
  "behavioral_scene": "left_turn"
}
```

**New:**
```json
{
  "mode": "scene",
  "scene_id": "left_turn"
}
```

## Next Steps

1. **Build and upload** the new firmware
2. **Visit `/behavioral`** to create your outputs
3. **Configure buttons** using the new `mode` and `output_behavior` fields
4. **Test** - outputs are controlled via the behavioral engine instead of raw CAN

---

**Version 3.2.0** - Simplified behavioral system
