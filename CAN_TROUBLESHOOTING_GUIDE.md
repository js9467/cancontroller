# CAN Troubleshooting Guide - Waveshare ESP32-S3-Touch-LCD-7

## Critical Issue: CAN Stops Receiving Frames

The most common issue is: **TWAI driver starts successfully, but receives ZERO frames from the bus.**

This is NOT a compile error—it's a **hardware initialization problem**.

### Root Cause

The Waveshare ESP32-S3 board has a **unique hardware design** where the CAN transceiver (SN65HVD230) is **gated behind a CH422G I2C expander**. 

Before the TWAI peripheral can receive CAN messages:
1. **USB_SEL (bit 5) on the CH422G I2C expander MUST be HIGH**
2. This gate must be set **BEFORE** TWAI driver starts
3. If set too late or not at all: transceiver stays powered down → RX pin receives nothing

---

## Quick Diagnosis: Run These Tests

### Test 1: Verify CH422G Gate is Set

Call this from your code after `CanManager::instance().begin()`:

```cpp
CanManager::instance().verifyTransceiverEnabled();
```

**Expected Output:**
```
[CanManager] Verifying CAN transceiver enable status...
[CanManager]   CH422G[0x38] = 0x2A, USB_SEL (bit 5) = HIGH ✓
```

**If USB_SEL shows LOW ✗:** The CAN transceiver is powered down. Check main.cpp to ensure `expander->digitalWrite(USB_SEL, HIGH)` runs BEFORE `CanManager::instance().begin()`.

### Test 2: Verify RX Pin is Not Stuck

Call this to check if GPIO19 (RX pin) is actually connected to a working transceiver:

```cpp
CanManager::instance().dumpHardwareStatus();
```

**Expected Output (if bus is quiet):**
```
├─ GPIO RX Pin State (RAW GPIO READ - 100 samples over 100ms):
  ones=50, zeros=50, transitions=...
  ✓ Pin transitions detected - good sign!
```

**If you see:**
```
  ones=100, zeros=0, transitions=0
```

This means the RX pin is **stuck HIGH**. Possible causes:
- CH422G gate is not set (USB_SEL = LOW) → transceiver disabled
- Wrong RX pin configured
- Wiring issue (CANH/CANL not connected properly)
- Transceiver hardware failure

### Test 3: Verify Bus Bitrate Compatibility

The current code hardcodes **250 kbps**. Verify your vehicle/adapter is also set to 250 kbps:

```cpp
Serial.printf("[CAN] Configured bitrate: 250000 bps\n");
```

If your vehicle network runs at a different speed (e.g., 500 kbps), update:

```cpp
CanManager::instance().begin(20, 19, 500000);  // 500 kbps
```

---

## Common Issues & Solutions

### Issue #1: "TWAI appears to work but receives 0 frames"

**Symptoms:**
- `CanManager::instance().isReady()` returns `true`
- But `CanManager::instance().receiveMessage()` always returns false

**Diagnosis:**
1. Run `CanManager::instance().dumpHardwareStatus()`
2. Check if USB_SEL is HIGH
3. Check if RX pin transitions are occurring

**Solution:**
- Ensure `expander->digitalWrite(USB_SEL, HIGH)` runs **before** CAN initialization
- Verify CH422G is initialized first via `expander->init()` and `expander->begin()`

### Issue #2: "Arduino-ESP32 core mismatch errors at compile time"

**Symptoms:**
```
error: 'twai_status_info_t' has no member named 'bus_off'
error: 'twai_status_info_t' has no member named 'alerts_triggered'
```

**Solution:**
- Use only **core-compatible** status fields:
  - ✓ `status.state` - TWAI state enum
  - ✓ `status.msgs_to_tx` - TX queue depth
  - ✓ `status.msgs_to_rx` - RX queue depth
  - ✓ `status.tx_error_counter` - TX error count
  - ✓ `status.rx_error_counter` - RX error count
  - ✗ `status.bus_off` - NOT in all versions
  - ✗ `status.alerts_triggered` - NOT in all versions

- Check Arduino-ESP32 core version (should be 3.0.7 or similar)

### Issue #3: "TWAI_TIMING_CONFIG inside a ternary operator breaks"

**Wrong ❌:**
```cpp
const auto t_config = (bitrate == 500000) ? 
    TWAI_TIMING_CONFIG_500KBITS() :  // ❌ Macro expands to initializer list
    TWAI_TIMING_CONFIG_250KBITS();
```

**Right ✓:**
```cpp
twai_timing_config_t t_config;
if (bitrate == 500000) {
    t_config = TWAI_TIMING_CONFIG_500KBITS();
} else {
    t_config = TWAI_TIMING_CONFIG_250KBITS();
}
```

### Issue #4: "CAN stops working after a code change"

**Possible causes:**
- Code is writing to CH422G addresses `0x20-0x27` or `0x30-0x3F` without being careful
- These ranges are reserved for CH422G, and random writes can disable the touch controller
- Never do "route discovery" or blind I2C address scanning on this board

**Solution:**
- Audit any I2C probing code
- Only read/write CH422G at its designated address (0x38 for output gate control)
- Never write to arbitrary I2C addresses to "test" the bus

---

## Hardware Initialization Sequence (Must Happen in This Order)

```cpp
// 1. Initialize I2C (Serial port for debugging)
Serial.begin(115200);

// 2. Create and initialize CH422G I2C expander
ESP_IOExpander* expander = new ESP_IOExpander_CH422G(1, 0x24);
expander->init();
expander->begin();

// 3. Set USB_SEL HIGH to enable CAN transceiver
expander->digitalWrite(USB_SEL, HIGH);  // <- CRITICAL
delay(10);  // Let hardware settle

// 4. NOW safe to initialize TWAI
CanManager::instance().begin();

// 5. Optional: Verify everything worked
CanManager::instance().dumpHardwareStatus();
```

---

## CAN Bus Physical Wiring Checklist

Before trying code fixes, verify the hardware:

- [ ] **CANH ↔ CANH** - Both boards connected to same signal
- [ ] **CANL ↔ CANL** - Both boards connected to same signal  
- [ ] **GND ↔ GND** - Common ground between board and vehicle/adapter
- [ ] **Termination** - This board has on-board termination (120Ω, jumper-selectable)
  - If vehicle network is already terminated, disable this board's termination (open jumper)
  - If vehicle is NOT terminated, leave this board's jumper closed
- [ ] **Bitrate** - Verify USB-CAN adapter and vehicle are set to same rate (250k or 500k)
- [ ] **Power** - Verify 12V power to CAN transceiver (typically through USB_SEL control pin)

---

## Waveshare Demo Code Reference

Waveshare provides working TWAI examples:
- Location: `tools/deploy/ESP32-S3-Touch-LCD-7-Demo/`
- Working demo: `ESP-IDF/06_TWAItransmit/` and `07_TWAIreceive/`
- Key file: `waveshare_twai_port.c` - shows correct initialization sequence

The critical line in their code:
```c
uint8_t write_buf = 0x01;
i2c_master_write_to_device(I2C_MASTER_NUM, 0x38, &write_buf, 1, ...);
```

This writes `0x01` (or 0x20, depending on register interpretation) to CH422G address 0x38 to enable the CAN gate.

---

## Permanent Fix Checklist

✅ **COMPLETED:**

1. ✓ TWAI status fields fixed (only use core-compatible fields)
2. ✓ CH422G gate verification added (`verifyTransceiverEnabled()`)
3. ✓ RX pin state diagnostic added (`dumpHardwareStatus()`)
4. ✓ No unsafe I2C probing code in main project
5. ✓ USB_SEL confirmed HIGH before TWAI init in main.cpp
6. ✓ Ternary operator issue fixed (not using macros in expressions)

---

## Emergency: "CAN Just Stopped Working"

If CAN suddenly stopped receiving frames:

1. **Call diagnostic immediately:**
   ```cpp
   CanManager::instance().dumpHardwareStatus();
   ```

2. **Check the output:**
   - USB_SEL LOW? → Expander lost power or state; reboot or call `expander->digitalWrite(USB_SEL, HIGH)`
   - RX pin stuck HIGH? → Transceiver offline; check power supply to board
   - Bus state is BUS-OFF? → Electrical issue or CAN frame error; may need recovery

3. **Power cycle the board** - resets I2C expander state

4. **If that doesn't work:** Backup to last known good version and rebuild incrementally

---

## Next Steps for Full Resolution

To complete CAN troubleshooting:

1. **Install Arduino-ESP32 3.0.7** (or verify your current core version)
   - Tools → Board Manager → Search "ESP32" → Install version 3.0.7

2. **Select exact board profile:**  
   - Tools → Board → ESP32S3 Dev Module (or "ESP32-S3-Touch-LCD-7" if available)

3. **Rebuild and test:**
   ```cpp
   void setup() {
       Serial.begin(115200);
       CanManager::instance().dumpHardwareStatus();  // See what hardware reports
   }
   ```

4. **If still no frames:**
   - Use Waveshare's raw demo code (tools/deploy/.../07_TWAIreceive/) without modifications
   - If THAT works, incrementally merge back your UI code
   - If THAT doesn't work, hardware issue (wiring, power, CAN adapter)

---

## Reference: Correct CAN Initialization in main.cpp

See [main.cpp](src/main.cpp#L240-L260) for the working sequence:

1. CH422G is created and initialized
2. USB_SEL is set HIGH immediately
3. After panel initialization, USB_SEL is re-confirmed HIGH
4. ONLY THEN is `CanManager::instance().begin()` called

This ensures the CAN transceiver gate is open before TWAI driver starts.
