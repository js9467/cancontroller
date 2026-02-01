# Waveshare ESP32-S3 CAN Hardware Contract

## ⚠️ CRITICAL: Understanding the Hardware Requirement

The **Waveshare ESP32-S3-Touch-LCD-7** board has a unique hardware design where the CAN transceiver is **not directly connected** to the ESP32's TWAI (CAN) peripheral. Instead, it is **gated by an I2C expander**.

### The Problem

Without proper initialization sequence, you will experience:
- TWAI driver reports `ESP_OK` and state `RUNNING`
- Bus appears operational from software perspective
- **But receives ZERO CAN frames** ❌

This happens because:
1. The CAN transceiver path is controlled by a CH422G I2C expander at address `0x38`
2. Bit 5 (`USB_SEL`) must be **HIGH** to enable CAN mode (disables USB)
3. If TWAI starts **before** this bit is set, the transceiver remains disconnected

## ✅ The Solution: Hardware Abstraction Layer

### Files Created

1. **`src/hardware_config.h`** - Centralized hardware constants
   - TWAI pin definitions (TX=GPIO20, RX=GPIO19)
   - I2C bus configuration (SDA=GPIO8, SCL=GPIO9)
   - CH422G expander address and gate values
   - Timing parameters and retry settings

2. **`src/can_hw.h`** / **`src/can_hw.cpp`** - Hardware abstraction layer
   - Enforces the correct initialization sequence
   - Provides simple API: `can_hw_begin()`, `can_hw_end()`
   - Includes self-test and verification functions

### The Mandatory Sequence

```cpp
// CORRECT ✓
can_hw_begin(bitrate, mode, verify)
  ↓
  1. Stop any existing TWAI driver
  2. Write gate value to CH422G (0x38)
  3. Wait for hardware to settle (10ms)
  4. Install TWAI driver
  5. Start TWAI driver
  6. Optional: Verify frames are being received

// INCORRECT ✗ - Will fail silently
twai_driver_install()  // Gate not set yet!
twai_start()           // Transceiver is disconnected
// Result: 0 frames received, bus appears "working"
```

### Code Changes

#### Before (Fragile)
```cpp
// main.cpp - multiple scattered functions
forceBoardToCAN();           // Set gate
initializeCANHardware();     // Another gate attempt
hardwareBringUp_CAN();       // Yet another gate attempt
CanManager::instance().begin(); // Finally start TWAI

// Problem: Easy to change code and bypass gate initialization
```

#### After (Robust)
```cpp
// main.cpp - One clean call
CanManager::instance().begin(250000, true);

// can_manager.cpp - Delegates to hardware layer
bool CanManager::begin(uint32_t bitrate, bool verify) {
    return can_hw_begin(bitrate, CanMode::Normal, verify);
}

// can_hw.cpp - Enforces contract ALWAYS
bool can_hw_begin(...) {
    can_hw_end();              // Clean slate
    can_hw_apply_gate();       // ✓ Gate FIRST
    delay(settle_time);
    twai_install_and_start();  // ✓ TWAI AFTER
    if (verify) {
        can_hw_count_frames(); // ✓ Proof of life
    }
}
```

## 🔒 Rules to Prevent Future Breakage

### 1. **NEVER** call TWAI functions directly
```cpp
// ❌ FORBIDDEN
twai_driver_install()
twai_start()
twai_stop()
twai_driver_uninstall()

// ✅ ALWAYS USE
can_hw_begin()
can_hw_end()
```

### 2. **NEVER** write to CH422G expander outside `can_hw.cpp`
```cpp
// ❌ FORBIDDEN
Wire.beginTransmission(0x38);
Wire.write(gate_value);

// ✅ ONLY IN can_hw.cpp
can_hw_apply_gate()  // Internal implementation
```

### 3. **ALWAYS** initialize CAN after panel initialization
```cpp
panel->init();
panel->begin();  // ← I2C bus is now ready

// THEN initialize CAN (uses the same I2C bus)
CanManager::instance().begin(250000, true);
```

### 4. **ALWAYS** re-apply gate before TWAI restart
```cpp
// Even if recovering from bus-off, use the abstraction
CanManager::instance().stop();
delay(100);
CanManager::instance().begin(250000, true);  // ✓ Gate re-applied
```

## 📊 Verification and Diagnostics

### Self-Test on Boot
```cpp
// Enable verification to ensure CAN is receiving frames
CanManager::instance().begin(250000, true);  // verify=true
```

Output:
```
[CAN_HW] ========================================
[CAN_HW] Starting CAN hardware initialization
[CAN_HW] ========================================
[CAN_HW] Step 1/3: Applying I2C expander gate...
[CAN_HW] Gate write 0x20 to 0x38: OK
[CAN_HW] ✓ Gate applied successfully
[CAN_HW] Step 2/3: Installing TWAI driver...
[CAN_HW] TWAI driver installed
[CAN_HW] TWAI started: 250000 bps, mode=NORMAL, TX=GPIO20, RX=GPIO19
[CAN_HW] ✓ TWAI driver running
[CAN_HW] Step 3/3: Verifying bus activity...
[CAN_HW] Listening for frames (1000ms)...
[CAN_HW] Received 142 frames in 1000ms
[CAN_HW] ✓ Received 142 frames in 1000ms - bus is active
[CAN_HW] ========================================
[CAN_HW] ✓ CAN hardware initialization complete
[CAN_HW] ========================================
```

### Manual Diagnostics
```cpp
// Count frames over 5 seconds to verify bus health
int frame_count = can_hw_count_frames(5000);
Serial.printf("Received %d frames\n", frame_count);

// Get status string
const char* status = can_hw_get_status_string();
Serial.printf("CAN Status: %s\n", status);  // "RUNNING", "BUS_OFF", etc.
```

## 🛠️ Hardware Reference

| Component | Setting | Value |
|-----------|---------|-------|
| TWAI TX Pin | `HW_TWAI_TX_PIN` | GPIO 20 |
| TWAI RX Pin | `HW_TWAI_RX_PIN` | GPIO 19 |
| I2C SDA | `HW_I2C_SDA_PIN` | GPIO 8 |
| I2C SCL | `HW_I2C_SCL_PIN` | GPIO 9 |
| Expander Address | `HW_CAN_GATE_I2C_ADDR` | 0x38 |
| Gate Bit | `HW_CAN_GATE_BIT` | 5 (USB_SEL) |
| Gate Value (Primary) | `HW_CAN_GATE_VALUE_PRIMARY` | 0x20 (bit 5 HIGH) |
| Gate Value (Alt 1) | `HW_CAN_GATE_VALUE_ALT1` | 0x43 |
| Gate Value (Alt 2) | `HW_CAN_GATE_VALUE_ALT2` | 0x07 |

## 📝 Summary for Team Communication

**Subject: CAN Hardware Contract - Required Reading**

The Waveshare ESP32-S3 board's CAN transceiver requires specific I2C expander configuration before TWAI can receive frames. We've implemented a hardware abstraction layer to make this foolproof:

1. **All CAN operations** must go through `can_hw_begin()` / `can_hw_end()`
2. **Never call** `twai_driver_install()` / `twai_start()` directly
3. **Hardware config** is centralized in `hardware_config.h` (DO NOT EDIT)
4. **Initialization order**: Panel → CAN (uses the same I2C bus)
5. **Self-test** is available via `can_hw_begin(..., verify=true)`

This prevents the "CAN appears working but receives 0 frames" bug that occurred when code changes accidentally bypassed the gate initialization.

See `HARDWARE_CONTRACT.md` for full details.
