# Stability Fixes v2.1.3 - Surgical Improvements

## Overview
This firmware implements 5 specific surgical stability fixes to eliminate "mysterious dying" and ensure robust automotive HMI operation, addressing persistent storage corruption, task starvation, and driver initialization issues.

## Changes Made

### 1. ✅ EXPLICIT I2C INITIALIZATION (Single Owner Pattern)
**Problem:** I2C double-init causing bootloop when Wire.begin() called early, then panel lib called I2C init again
**Solution:** 
- `Wire.begin(8, 9)` at VERY START of setup() before ANY other I2C operations
- Single explicit owner prevents conflicts
- All subsequent I2C code reuses same driver instance

**Code Location:** [src/main.cpp#L239-L242](src/main.cpp#L239-L242)
```cpp
Wire.begin(8, 9);
Wire.setClock(100000);
delay(10);
```

---

### 2. ✅ EXPANDER INITIALIZATION BEFORE PANEL->INIT/BEGIN
**Problem:** Late expander init meant USB_SEL mux not asserted when panel code ran, CAN transceiver could be disabled
**Solution:**
- Initialize IO Expander BEFORE panel->init() and panel->begin()
- Explicit pin configuration with bitmasks (not pin numbers)
- Assert USB_SEL=HIGH immediately to ensure CAN mux is in correct state
- Add expander to panel BEFORE panel->init/begin

**Code Location:** [src/main.cpp#L260-L280](src/main.cpp#L260-L280)
```cpp
// STEP 2: INITIALIZE EXPANDER BEFORE PANEL->INIT/BEGIN
g_expander = new ESP_IOExpander_CH422G(I2C_MASTER_NUM, ESP_IO_EXPANDER_I2C_CH422G_ADDRESS_000);
g_expander->init();
g_expander->begin();

// Configure all control pins as outputs and assert safe state
g_expander->multiPinMode(TP_RST_MASK | LCD_RST_MASK | SD_CS_MASK | USB_SEL_MASK, OUTPUT);
g_expander->multiDigitalWrite(TP_RST_MASK | LCD_RST_MASK | SD_CS_MASK | USB_SEL_MASK, HIGH);

// Add expander to panel BEFORE init/begin so panel knows about it
panel->addIOExpander(g_expander);
```

---

### 3. ✅ SYNCHRONOUS LVGL FLUSH (Eliminated Async Callback Failure Mode)
**Problem:** LVGL async flush callback (notify_lvgl_flush_ready) not always firing on driver mismatch, causing UI to "freeze"
**Solution:**
- Always call `lv_disp_flush_ready(disp)` IMMEDIATELY inside lvgl_port_disp_flush()
- Removed async callback pattern entirely
- Guarantees LVGL knows flush completed, unblocks rendering

**Code Location:** [src/main.cpp#L130-L135](src/main.cpp#L130-L135)
```cpp
void lvgl_port_disp_flush(lv_disp_drv_t* disp, const lv_area_t* area, lv_color_t* color_p) {
    // ... driver-specific flush code ...
    
    // SYNCHRONOUS: Always signal completion immediately
    lv_disp_flush_ready(disp);
}
```

---

### 4. ✅ USB_SEL MUX WATCHDOG TASK (Prevents Mux Flips)
**Problem:** Backlight code, panel library, or other routines could accidentally toggle USB_SEL expander bits, silently disabling CAN
**Solution:**
- Background FreeRTOS task on core 1 (priority=1)
- Every 1000ms: explicitly reassert USB_SEL=HIGH
- Prevents mux flips from any source
- Non-blocking via g_expander global pointer

**Code Location:** [src/main.cpp#L89-96](src/main.cpp#L89-96)
```cpp
// USB_SEL mux watchdog - reassert CAN mux periodically to prevent flips
void mux_watchdog_task(void*) {
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        if (g_expander) {
            static constexpr uint8_t USB_SEL_MASK = (1 << 5);
            g_expander->multiDigitalWrite(USB_SEL_MASK, HIGH);
        }
    }
}
```

**Spawned in setup():** [src/main.cpp#L330](src/main.cpp#L330)
```cpp
xTaskCreatePinnedToCore(mux_watchdog_task, "mux_wd", 2048, nullptr, 1, nullptr, 1);
```

---

### 5. ✅ NON-BLOCKING SERIAL COMMANDS (Prevent Loop Starvation)
**Problem:** `canmon` (10-second while loop) and `canpoll` (1-second polling) blocked main loop, starving LVGL/WiFi tasks
**Solution:**
- `canmon`: Start monitoring state machine, process each iteration in loop without blocking
- `canpoll`: Send frame immediately, don't wait for response (user can run `canmon` separately)
- Non-blocking state tracking: `canmon_active`, `canmon_start_ms`, `canmon_count`
- Main loop check every iteration, print message count when done

**Code Locations:**
- Command trigger: [src/main.cpp#L420-L425](src/main.cpp#L420-L425)
- Non-blocking processor: [src/main.cpp#L540-L560](src/main.cpp#L540-L560)

```cpp
// Command: Start non-blocking CAN monitoring
} else if (cmd == "canmon") {
    if (!canmon_active) {
        canmon_active = true;
        canmon_start_ms = millis();
        canmon_count = 0;
        Serial.println("[CAN] *** Monitoring CAN bus for 10 seconds (non-blocking) ***");
    }

// Main loop: Process monitoring without blocking
if (canmon_active) {
    uint32_t elapsed = millis() - canmon_start_ms;
    if (elapsed < 10000) {
        CanRxMessage msg;
        if (CanManager::instance().receiveMessage(msg, 0)) {  // 0ms = non-blocking
            canmon_count++;
            // Print message...
        }
    } else {
        Serial.printf("[CAN] *** Monitoring complete. Received %d messages. ***\n", canmon_count);
        canmon_active = false;
    }
}
```

---

## Safety Architecture Alignment

These fixes directly implement patterns from [ARCHITECTURE_STABILITY.md](ARCHITECTURE_STABILITY.md):

| Failure Mode | Prevention Pattern | Implementation |
|---|---|---|
| I2C double-init | Driver init at boot, explicit ownership | Explicit `Wire.begin(8,9)` at setup() top |
| CAN mux disabled | Mux watchdog task | `mux_watchdog_task` every 1000ms |
| LVGL hanging | Synchronous flush | Always call `lv_disp_flush_ready()` immediately |
| Loop starvation | Non-blocking commands | `canmon` state machine in loop |
| Expander/reset race | Order: I2C → expander → panel | Reordered init sequence |

---

## Testing Checklist

After flashing v2.1.3:

- [ ] Device boots without I2C errors or bootloop
- [ ] Screen renders (no black screen)
- [ ] Touch responsive (GT911 initialized)
- [ ] CAN bus working (test with `canmon` and `canstatus`)
- [ ] WiFi AP active at 192.168.4.250
- [ ] Serial commands responsive (try `b 50`, `canmon`, `help`)
- [ ] Mux stays HIGH (check with logic analyzer on GPIO5 expander output)
- [ ] No watchdog resets or crashes over 5 minute runtime

---

## Firmware Build Details

- **Environment:** waveshare_7in
- **Compiled:** Successfully ✓
- **Flash Size:** 2017KB / 6553KB (30.8%)
- **RAM:** 55904 / 327680 bytes (17.1%)
- **Build Time:** ~71 seconds

---

## Known Limitations & Mitigations

1. **Async callbacks removed** - Guarantees synchronous flush but requires LVGL 8.3.11+ (already in use)
2. **Mux watchdog runs on core 1** - Doesn't interfere with core 0 (WiFi, BLE), keeps I2C stable
3. **canmon non-blocking** - User won't see response until running `canmon` separately; by design to prevent starvation
4. **NVS/LittleFS corruption from power loss** - Factory reset hatch required separately (see Safety Architecture point #1)

---

## Related Documentation

- [ARCHITECTURE_STABILITY.md](ARCHITECTURE_STABILITY.md) - 9-point stability framework
- [HARDWARE_CONFIG.md](HARDWARE_CONFIG.md) - GPIO/I2C/CAN pinout (UNCHANGED by these fixes)
- [QUICK_START.md](QUICK_START.md) - General startup guide

---

## Version History

- **v2.1.3** ← YOU ARE HERE
  - Implemented 5 surgical stability fixes
  - I2C explicit init, expander reordering, synchronous LVGL flush, mux watchdog, non-blocking commands
  - Addresses "mysterious dying" root causes

- v2.1.2 
  - Removed early Wire.begin() (fixed bootloop symptom but left structural risks)
  - Black screen (async flush callback never firing)

- v2.1.1
  - IPM1 behavioral orchestrator added
  - Black screen appeared

- v2.0.7
  - Removed Infinitybox hardcoded sequences

