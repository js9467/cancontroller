# Infinitybox IPM1 Control System Implementation

## Overview

This firmware now includes a comprehensive Infinitybox IPM1 control system that implements the complete UI behavioral model from the Front Engine Standard System Assignments (REV1). The system is **fully additive** - all existing functionality is preserved.

## Architecture

### Core Components

1. **infinitybox_control.h/cpp** - Main control system
   - Function registry (27 functions from JSON schema)
   - Device management (7 devices: 2 powercells, 4 inmotions, 1 mastercell)
   - Behavior engines (toggle, momentary, flash, fade, timed, scene, one_shot)
   - Ownership & conflict management
   - Security interlock system

2. **Integration**
   - Connected to existing IPM1 CAN system
   - Behavior engines run from main loop
   - Serial command interface for testing

## Function Database

All 27 functions from the IPM1 JSON schema are loaded at startup:

### Turn Signals
- Left Turn Signal Front (PC Front Output 1) - flash/flash_timed, requires ignition
- Right Turn Signal Front (PC Front Output 2) - flash/flash_timed, requires ignition
- 4-Ways (PC Front Outputs 1+2) - flash
- Left Turn Signal Rear (PC Rear Output 1) - flash/flash_timed
- Right Turn Signal Rear (PC Rear Output 2) - flash/flash_timed

### Powertrain
- Ignition (PC Front Output 3) - toggle
- Starter (PC Front Output 4) - momentary, **blocked when security active**
- Fuel Pump (PC Rear Output 10) - toggle, **blocked when security active**
- Cooling Fan (PC Front Output 10) - toggle/timed

### Lighting - Front
- Headlights (PC Front Output 5) - toggle/scene/fade
- Parking Lights Front (PC Front Output 6) - toggle
- High Beams (PC Front Output 7) - momentary/toggle
- Horn (PC Front Output 9) - momentary

### Lighting - Rear
- Brake Lights (PC Rear Output 3) - toggle
- Interior Lights (PC Rear Output 4) - toggle/fade/timed
- Backup Lights (PC Rear Output 5) - toggle
- Parking Lights Rear (PC Rear Output 6) - toggle

### Window Controls
- Driver Window Up (IM DF relay_1a) - momentary
- Driver Window Down (IM DF relay_1b) - momentary
- Passenger Window Up (IM PF relay_1a) - momentary
- Passenger Window Down (IM PF relay_1b) - momentary

### Door Locks
- Driver Door Lock (IM DF relay_2a) - one_shot
- Driver Door Unlock (IM DF relay_2b) - one_shot

### AUX Outputs (Renameable)
- AUX 03 (IM DF aux_03) - toggle/flash/fade/timed
- AUX 04 (IM DF aux_04) - toggle/flash/fade/timed

## Behavior Engines

### Toggle
- User controls ON/OFF state directly
- Example: Headlights, Parking Lights, Ignition

### Momentary
- Active only while button pressed
- Auto-releases when released
- Example: Horn, Starter, Window Up/Down, High Beams

### Flash
- Continuous flashing at configured rate
- Default: 500ms on, 500ms off
- Example: Turn signals, 4-Ways

### Flash-Timed
- Flash for specific duration then stop
- Duration configured per activation
- Example: Turn signals with auto-cancel

### Fade
- PWM ramp from current level to target over duration
- Default: 1000ms ramp time
- Example: Interior Lights, Headlights

### Timed / One-Shot
- Activate for fixed duration then auto-deactivate
- Default: 500ms pulse
- Example: Door locks, Cooling fan with timer

### Scene
- Multi-function preset (not yet fully implemented)
- Example: "Night Drive" - Headlights ON, Interior 20%, Gauge ON

## Ownership & Conflict Management

Only ONE owner can control a function at a time:
- **NONE** - No active owner
- **MANUAL** - User direct control (toggle/momentary)
- **FLASH_ENGINE** - Flash behavior active
- **TIMER** - Timed behavior active
- **SCENE** - Scene controlling
- **FADE_ENGINE** - Fade behavior active

When a new owner requests control:
1. Check if function is blocked (security/ignition)
2. Check current owner
3. If different owner, cancel previous and take over
4. Execute new behavior

## Security Interlocks

When security is **ACTIVE**, the following functions are **BLOCKED**:
- Starter
- Fuel Pump

Functions with `requires: ["ignition"]` are blocked when ignition is **OFF**:
- Left Turn Signal Front
- Right Turn Signal Front

To enable/disable security:
```
security on
security off
```

## Serial Commands

### Function Control
```bash
# Basic control
ibox headlights on
ibox headlights off

# Use underscores for multi-word function names
ibox left_turn_signal_front flash
ibox interior_lights fade 50

# Ignition control (also sets ignition state)
ignition on
ignition off

# Security control
security on
security off
```

### Status & Information
```bash
# List all functions and their allowed behaviors
iboxlist

# Show active functions and state
iboxstatus

# General help
help
```

### Example Session
```bash
# Turn on ignition (required for turn signals)
ignition on

# Activate left turn signal (flash continuously)
ibox left_turn_signal_front flash

# Fade interior lights to 30%
ibox interior_lights fade 30

# Check status
iboxstatus

# Enable security (will block starter and fuel pump)
security on

# Try to start (will be blocked)
ibox starter on  # FAILS - blocked by security

# Disable security
security off

# Now starter works
ibox starter on  # OK - momentary activation
```

## CAN Integration

The Infinitybox controller integrates with the existing IPM1 CAN system:

1. Function activation → Builds JSON action for IPM1 system
2. IPM1 system → Sends CAN message to appropriate device
3. Feedback → Updates function state (current draw, fault detection)

Currently implemented:
- ✅ Function → CAN command mapping
- ✅ Device address routing
- ✅ Behavior execution
- ⏳ Feedback integration (current draw, faults)
- ⏳ Scene system
- ⏳ UI pages (next phase)

## Next Steps

1. **UI Pages** - Add LVGL UI with navigation categories:
   - Driving (Turn Signals, Hazards, Horn, High Beams)
   - Exterior Lighting (Headlights, Parking, Backup)
   - Interior (Interior Lights, Gauge Illumination)
   - Body (Door Locks, Windows)
   - Powertrain (Ignition, Starter, Fuel Pump, Cooling Fan)
   - AUX / Custom (AUX Inputs, AUX Outputs, Open/Reserved)

2. **Scenes** - Implement scene creation, editing, and activation
   - Example: "Night Drive", "Parking", "Security Armed"

3. **Feedback** - Connect CAN feedback to UI
   - Display current draw for each function
   - Show fault states
   - Indicate actual ON/OFF from CAN

4. **AUX Renaming** - Allow users to rename AUX outputs
   - Store custom names in config
   - Display in UI

## Technical Details

**Firmware Size**: 2,039,509 bytes (31.1% flash, 17.1% RAM)
**Function Count**: 27
**Device Count**: 7
**Behavior Types**: 7
**Build Time**: ~248 seconds

## Testing

The system can be tested entirely via serial commands without hardware connected. All behavior engines run in the main loop and simulate the expected CAN traffic.

```bash
# Test flash engine
ignition on
ibox left_turn_signal_front flash
# Wait 10 seconds, watch serial output for toggle messages
ibox left_turn_signal_front off

# Test fade engine
ibox interior_lights fade 75
# Watch serial output for PWM level updates

# Test security interlock
security on
ibox starter on  # Should fail
security off
ibox starter on  # Should succeed
```

## Design Principles

1. **ADDITIVE** - No existing functionality removed
2. **Source of Truth** - JSON schema defines all functions
3. **Behavior Assignment** - UI assigns behaviors to functions
4. **One Owner** - Only one owner per function at a time
5. **Feedback Driven** - Actual state comes from CAN, not assumptions
6. **Safety First** - Security interlocks cannot be bypassed

---

*Implementation Date: February 2, 2026*
*Based on: Infinitybox IPM1 Front Engine Standard System Assignments (REV1)*
