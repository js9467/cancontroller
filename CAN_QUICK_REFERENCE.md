# CAN Quick Reference - 30 Second Fix Check

## Problem: "CAN stops receiving frames"

### Immediate Diagnosis (copy & paste to setup())

```cpp
// Add this to setup() right after CanManager begins:
void setup() {
    // ... existing setup code ...
    
    CanManager::instance().begin();
    
    // NEW: Run this diagnostic
    delay(100);
    CanManager::instance().dumpHardwareStatus();
}
```

### Check These Three Things

1. **USB_SEL Status** (see output)
   - ✓ `HIGH ✓` = Transceiver is powered → OK
   - ✗ `LOW ✗` = Transceiver is OFF → **FIX NEEDED**

2. **RX Pin State** (see output)
   - ✓ `ones=~50, zeros=~50` = Pin is toggling → Good signal
   - ✗ `ones=100, zeros=0` = Pin stuck HIGH → Transceiver disabled or no connection

3. **Bus State** (see output)
   - ✓ `RUNNING` = Normal operation
   - ✗ `BUS_OFF` = Error recovery in progress

---

## Common Fixes

### If USB_SEL = LOW

**In main.cpp, make sure this runs BEFORE `CanManager::instance().begin()`:**

```cpp
ESP_IOExpander* expander = new ESP_IOExpander_CH422G(I2C_MASTER_NUM, ESP_IO_EXPANDER_I2C_CH422G_ADDRESS_000);
expander->init();
expander->begin();
expander->multiPinMode(TP_RST | LCD_RST | SD_CS | USB_SEL, OUTPUT);

// ← THIS LINE IS CRITICAL ←
expander->digitalWrite(USB_SEL, HIGH);  // Enable CAN transceiver
delay(10);  // Wait for hardware to settle
// ← NOW safe to start CAN ←

CanManager::instance().begin();
```

### If RX Pin is Stuck HIGH

```
Possible causes (in order):
1. USB_SEL is LOW (see above)
2. CANH/CANL not connected to vehicle/adapter
3. Wrong bitrate (try 500k instead of 250k)
4. Termination jumper issue
5. Transceiver hardware failure
```

### If Bus State = BUS_OFF

```cpp
// Try recovery:
twai_initiate_recovery();
delay(100);
CanManager::instance().dumpHardwareStatus();  // Check again
```

---

## Compile Error Quick Fixes

### Error: "twai_status_info_t has no member named 'bus_off'"

**Solution:** Use `status.state` instead
```cpp
// WRONG:
if (status.bus_off) { ... }

// RIGHT:
if (status.state == TWAI_STATE_BUS_OFF) { ... }
```

### Error: "TWAI_TIMING_CONFIG_500KBITS() in ternary operator"

**Solution:** Use if/else instead
```cpp
// WRONG:
twai_timing_config_t t = (rate==500k) ? TWAI_TIMING_CONFIG_500KBITS() : ...

// RIGHT:
twai_timing_config_t t;
if (rate == 500000) {
    t = TWAI_TIMING_CONFIG_500KBITS();
} else {
    t = TWAI_TIMING_CONFIG_250KBITS();
}
```

---

## Serial Monitor Commands

Add to your main loop:

```cpp
void loop() {
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        if (cmd == "can") {
            CanManager::instance().dumpHardwareStatus();
        }
    }
}
```

Then type in Serial Monitor:
- `can` → Full diagnostic report

---

## Hardware Checklist

Before software debugging:

- [ ] CANH wire connected to vehicle CANH
- [ ] CANL wire connected to vehicle CANL  
- [ ] GND wire connected to vehicle GND
- [ ] Bitrate matches vehicle (250k or 500k)
- [ ] Termination jumper correct (see board docs)
- [ ] 12V power to board

---

## Waveshare Demo Code

If your fixes don't work, test with Waveshare's reference:

**Location:** `tools/deploy/ESP32-S3-Touch-LCD-7-Demo/ESP-IDF/07_TWAIreceive/`

**To use:**
1. Copy the working code structure
2. Compare with your code
3. Merge back incrementally

---

## Still Stuck?

1. Check [CAN_TROUBLESHOOTING_GUIDE.md](CAN_TROUBLESHOOTING_GUIDE.md)
2. Check [CAN_DIAGNOSTIC_API.md](CAN_DIAGNOSTIC_API.md)
3. Search in [HARDWARE_CONTRACT.md](HARDWARE_CONTRACT.md)

---

## Latest Changes (This Session)

✅ Enhanced [src/can_manager.cpp](src/can_manager.cpp) with diagnostics
✅ Added [CAN_TROUBLESHOOTING_GUIDE.md](CAN_TROUBLESHOOTING_GUIDE.md)
✅ Added [CAN_DIAGNOSTIC_API.md](CAN_DIAGNOSTIC_API.md)
✅ Added [CAN_FIX_SUMMARY.md](CAN_FIX_SUMMARY.md)
✅ Fixed TWAI struct field compatibility issues
✅ Added transceiver verification
✅ Added RX pin state monitoring

**No errors found in build.**

---

**TL;DR:** Call `CanManager::instance().dumpHardwareStatus()` in setup and read the output. It will tell you exactly what's wrong.
