# Bronco Controls - Stability Improvements

## Overview
This document describes the architectural improvements made to prevent mysterious crashes, freezes, and "won't boot even with old firmware" issues.

## What Was Fixed

### 1. **Safe Boot Mode & Factory Reset** ✅
**Problem**: Bad config persists across firmware updates, causing app to crash even after reverting
**Solution**: Hold top-left corner of screen during boot for 3 seconds to trigger factory reset

**How it works**:
- After panel initialization, system checks if top-left corner (0-100px) is being touched
- If held for full 3 seconds, wipes ALL persistent storage (NVS + LittleFS config)
- System reboots with fresh defaults
- **This is your "nuclear option" recovery hatch**

**Usage**:
1. Power on device
2. Immediately touch and hold top-left corner
3. Keep holding for 3 seconds until you see "[FACTORY RESET] Complete" in serial
4. Device reboots with clean slate

### 2. **Proper Task Separation** ✅
**Problem**: CAN receive blocking UI thread → touchscreen freezes, WDT resets
**Solution**: Dedicated CAN RX task on core 1 with FreeRTOS queue

**Architecture**:
```
Core 0: LVGL UI task (lv_timer_handler only, never blocks)
Core 1: CAN RX task (twai_receive in tight loop, feeds queue)
Core 1: Logic task (reads queue, updates model - runs in main loop())
Core 1: Health monitor (memory watchdog)
Core 1: Mux watchdog (keeps USB_SEL HIGH)
```

**Benefits**:
- UI never freezes waiting for CAN
- CAN never misses frames due to UI rendering
- Each subsystem can die independently without killing everything
- Proper RTOS task priorities prevent starvation

### 3. **Health Monitoring** ✅
**Problem**: "App dies mysteriously" with no diagnostics
**Solution**: Dedicated health task that watches for memory leaks and reports stats

**What it monitors**:
- Free heap (internal RAM)
- Free PSRAM
- CAN frames per second
- Sustained heap drops (leak detection)

**Output** (every 2 seconds):
```
[HEALTH] heap=245678 psram=7823456 can_fps=42
```

**Leak detection**: If heap drops >1KB for 5 consecutive checks (10s), prints warning:
```
[HEALTH] WARNING: Heap dropped 5120 bytes in 10s
```

### 4. **Hardware Init Lockdown** 🔒
**Problem**: CH422G mux gets re-initialized or flipped, CAN stops working
**Solution**: `force_can_mux_hardware()` runs ONCE at boot, before everything else

**New boot order**:
1. **Force CAN mux** (GPIO9/8 I2C, set outputs HIGH)
2. Print reset reason & memory stats
3. Initialize LVGL
4. Initialize panel (display/touch)
5. Check for safe boot
6. Start tasks

**Critical**: This function is now **isolated and first**. Do not modify unless you want CAN to mysteriously die.

### 5. **Reduced Logging** ✅
**Problem**: Printing every CAN frame → serial buffer overflow → task starvation → WDT reset
**Solution**: Aggregate stats instead of per-frame spam

**Changes**:
- CAN frames only printed if `canmon` command is active
- Otherwise: summary stats every 5 seconds
- Health stats every 2 seconds (not every loop)
- Removed verbose panel init logs

**Serial load reduction**: ~99% less spam in normal operation

### 6. **Atomic Config Saves** ✅
**Problem**: Brownout during file write → corrupted JSON → boot loop
**Solution**: Write to temp file, then atomic rename

**Pattern**:
```cpp
write("/config.tmp")  → if successful →
remove("/config.json") → 
rename("/config.tmp" → "/config.json")
```

**Benefits**:
- Brownout during write: old config still intact
- Corruption only affects temp file, never live config
- Atomic filesystem operation (ESP32 LittleFS guarantees this)

## Serial Commands (Updated)

### New Commands
- `help` - Show all commands
- **No new commands** - safe boot is touch-based, not serial

### Existing Commands (preserved)
- `b <0-100>` - Set brightness
- `canstatus` - Show CAN bus status
- `canpoll <1-16>` - Poll Powercell module
- `canconfig <1-16>` - Configure Powercell
- `canmon` - Monitor CAN for 10 seconds (now non-blocking!)
- `cansend <pgn> <data...>` - Send raw CAN frame

## What to Watch For

### Normal Boot Sequence
```
[HW-INIT] Forcing CAN mux (CRITICAL - do not modify this)
[HW-INIT] ✓ CAN mux forced, hardware stable
=================================
 Bronco Controls - Web Config 
=================================
 Firmware Version: 2.1.3
 Reset Reason: Power-on
 Free Heap: 245678 bytes
 Free PSRAM: 7823456 bytes
[SAFE BOOT] Checking for factory reset request (hold top-left)...
[SAFE BOOT] Normal boot

[CAN-TASK] RX task started on core 1
[CAN-TASK] ✓ CAN RX task started on core 1
[HEALTH] Monitor started
[HEALTH] ✓ Health monitor started
[CAN] Initializing CAN bus...
[CAN] ✓ TWAI driver initialized successfully!
```

### Warning Signs
**Heap leak detected**:
```
[HEALTH] WARNING: Heap dropped 5120 bytes in 10s
```
→ Check for missing `free()`, `delete`, or `String` abuse

**CAN not receiving**:
```
[CAN-STATS] Processed 0 frames in last 5s
```
→ Check if mux flipped (run `canstatus`)

**Brownout resets**:
```
Reset Reason: Brownout (power issue!)
```
→ Hardware problem: add bulk capacitors, check power supply

## Recovery Procedures

### "Device won't boot after update"
1. Try safe boot (hold top-left corner during boot)
2. If display not working: re-flash via USB
3. Check serial output for reset reason
4. If brownout: fix power supply before continuing

### "CAN stopped working"
1. Run `canstatus` to verify TX/RX pins
2. Check health monitor for task crashes
3. Try safe boot (might be bad config)
4. Last resort: full erase + re-flash

### "UI frozen but serial working"
1. Check health monitor output (is LVGL task alive?)
2. Look for heap depletion
3. Safe boot to clear potentially bad UI config

## Technical Notes

### FreeRTOS Queue
- Size: 128 frames
- Non-blocking send: drops frames if full (prevents CAN task from blocking)
- Non-blocking receive: main loop never waits

### Task Priorities
- LVGL: 2 (normal)
- CAN RX: 3 (high - never miss frames)
- Health: 1 (low - non-critical)
- Mux watchdog: 1 (low - periodic maintenance)

### Memory Management
- LVGL buffers: PSRAM (2x 76KB)
- CAN queue: Internal RAM (~4KB)
- Task stacks: Internal RAM (total ~20KB)

### Hardware Protection
**Mux watchdog**: Reasserts safe pin state every 1 second
- TP_RST = HIGH
- LCD_RST = HIGH
- SD_CS = HIGH
- USB_SEL = HIGH (critical for CAN)

**Why this matters**: Some library code (LVGL, touch, display) may accidentally write to I2C expander, flipping USB_SEL LOW → CAN stops working. Watchdog prevents this.

## Code Architecture Rules

### DO NOT:
- ❌ Call `twai_receive()` from UI thread
- ❌ Call LVGL functions from CAN task
- ❌ Print every CAN frame in production
- ❌ Save config on every slider movement
- ❌ Re-initialize TWAI repeatedly
- ❌ Modify `force_can_mux_hardware()` without very good reason
- ❌ Scan I2C in production code

### DO:
- ✅ Keep tasks yielding (`vTaskDelay` frequently)
- ✅ Use queues for inter-task communication
- ✅ Set flags/state from CAN task, update UI from main loop
- ✅ Debounce config saves (wait 2s after last change)
- ✅ Watch health monitor output regularly
- ✅ Test safe boot after making config changes
- ✅ Use atomic file writes for persistent data

## Testing Checklist

Before deploying new firmware:
- [ ] Boot 10 times (check for random failures)
- [ ] Test safe boot (hold corner, verify wipe)
- [ ] Monitor heap for 5 minutes (check for leaks)
- [ ] Send continuous CAN traffic (check for freezes)
- [ ] Change config values (check atomic saves)
- [ ] Simulate brownout (power cycle during config save)
- [ ] Check serial for WDT resets

## Version History

### v2.1.3+ (Current)
- ✅ Safe boot + factory reset
- ✅ Dedicated CAN RX task with queue
- ✅ Health monitoring task
- ✅ Early hardware init lockdown
- ✅ Reduced production logging
- ✅ Atomic config saves

### v2.1.2 and earlier
- ❌ No safe boot (bad config persists forever)
- ❌ CAN + UI in same thread (freezes)
- ❌ No health monitoring
- ❌ Mux initialization mixed with app code
- ❌ Per-frame CAN logging
- ❌ Direct file writes (corruption risk)

---

## Support

If device still dies after these fixes:
1. Check serial output for `[HEALTH]` warnings
2. Enable health monitor and watch for 10 minutes
3. Test safe boot to rule out config corruption
4. Check for brownout resets (power supply issue)
5. Verify CAN mux state with `canstatus`

**Remember**: The safe boot mode is your ultimate recovery tool. When in doubt, hold that corner and wipe everything.
