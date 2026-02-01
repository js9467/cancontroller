# ⚠️ HARDWARE CONFIGURATION LOCK ⚠️

## CRITICAL: DO NOT MODIFY THESE FILES

The following files contain **hardware-specific constants** that were discovered through extensive testing and are **THE ONLY VALUES** that enable CAN functionality on Waveshare ESP32-S3 hardware.

### 🔒 LOCKED FILES

1. **`src/hardware_config.h`** - Hardware pin definitions and constants
2. **`src/can_hw.cpp`** - CAN hardware initialization sequence
3. **`src/can_hw.h`** - CAN hardware interface

### 🚫 IMMUTABLE VALUES

These values are protected by **compile-time assertions**. If you change them, the build will FAIL:

```cpp
HW_CAN_GATE_I2C_ADDR   = 0x27   // CH422G I2C address
HW_CAN_GATE_VALUE_PRIMARY = 0x43  // Gate configuration byte
HW_CAN_GATE_BIT        = 5      // USB_SEL bit
HW_TWAI_TX_PIN         = 20     // CAN TX pin
HW_TWAI_RX_PIN         = 19     // CAN RX pin
HW_I2C_SDA_PIN         = 8      // I2C data pin
HW_I2C_SCL_PIN         = 9      // I2C clock pin
```

### ⚡ WHY THESE VALUES MATTER

**Waveshare ESP32-S3 Hardware Quirk:**
- The CAN transceiver is NOT directly wired to the TWAI peripheral
- It is **gated** by a CH422G I2C I/O expander
- Without the correct gate configuration (0x43 to address 0x27), the CAN bus appears operational but **receives ZERO frames**

**Critical Initialization Sequence:**
1. ESP_Panel library initializes I2C for GT911 touch controller
2. CAN code writes gate value 0x43 to CH422G at address 0x27 (using existing I2C bus)
3. Wait 10ms for hardware settle
4. Install and start TWAI driver

**Breaking This Sequence Results In:**
- ✗ "i2c driver install error" (if Wire.begin() called twice)
- ✗ Device boot loop and black screen
- ✗ TWAI reports bus operational but receives 0 frames
- ✗ Complete loss of CAN functionality

### 📋 WHAT YOU CAN SAFELY MODIFY

You CAN change these application-level settings:
- CAN bitrate (in application code)
- CAN filter configuration
- Application logic for processing CAN messages
- UI elements and display code
- WiFi/OTA configuration

### 🛡️ PROTECTION MECHANISMS

1. **Compile-Time Assertions** - Build fails if critical values change
2. **Read-Only Comments** - Clear warnings throughout hardware_config.h
3. **This Document** - Central reference for why these values are locked
4. **HARDWARE_CONTRACT.md** - Detailed technical explanation

### 🔧 IF YOU ABSOLUTELY MUST MODIFY

**DON'T.** But if you have a different board or hardware revision:

1. Read `HARDWARE_CONTRACT.md` completely
2. Use `tools/can_fix_utility.cpp` to discover your hardware's gate address/value
3. Update ALL occurrences:
   - `src/hardware_config.h` 
   - Compile-time assertions
   - This documentation
4. Test extensively before deploying

### 📚 REFERENCE DOCUMENTATION

- **HARDWARE_CONTRACT.md** - Full technical explanation
- **QUICK_REFERENCE_CAN.md** - Developer quick reference  
- **Working reference sketch** - Verified gate configuration (user provided)

---

**Last Updated:** February 1, 2026  
**Verified Working Configuration:** v2.0.1+  
**Board:** Waveshare ESP32-S3-Touch-LCD-7  
**Tested By:** User hardware validation with CAN bus sniffer
