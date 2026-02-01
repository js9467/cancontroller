# Quick Reference: Waveshare ESP32-S3 CAN Hardware Contract

## ✅ CORRECT USAGE

### Setup and Initialization
```cpp
#include "can_manager.h"  // High-level CAN interface
#include "can_hw.h"       // Hardware abstraction layer (if needed)

void setup() {
    // ... initialize panel first ...
    panel->init();
    panel->begin();  // I2C bus is now ready
    
    // Initialize CAN with verification
    CanManager::instance().begin(250000, true);  // bitrate, verify
    
    // Now CAN is ready to use
}
```

### Sending CAN Frames
```cpp
void sendCanFrame() {
    CanFrameConfig frame = { /* ... */ };
    CanManager::instance().sendFrame(frame);
}
```

### Receiving CAN Frames
```cpp
void receiveCanFrames() {
    CanRxMessage msg;
    if (CanManager::instance().receiveMessage(msg, 100)) {
        // Process message
    }
}
```

### Restart/Recovery
```cpp
void restartCan() {
    // Recovery from bus-off or errors
    CanManager::instance().stop();
    delay(100);
    CanManager::instance().begin(250000, true);  // Gate re-applied automatically
}
```

## ❌ FORBIDDEN - DO NOT DO THIS

### Never call TWAI functions directly
```cpp
// WRONG ❌
void WRONG_initCAN() {
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(...);
    twai_driver_install(...);  // ❌ Gate not applied!
    twai_start();              // ❌ Will receive 0 frames!
}
```

### Never write to CH422G expander directly
```cpp
// WRONG ❌
void WRONG_setGate() {
    Wire.beginTransmission(0x38);  // ❌ Bypasses abstraction!
    Wire.write(0x20);
    Wire.endTransmission();
}
```

### Never initialize CAN before panel
```cpp
// WRONG ❌
void WRONG_setupOrder() {
    CanManager::instance().begin();  // ❌ I2C bus not ready yet!
    panel->init();
    panel->begin();
}
```

## 📊 DIAGNOSTICS AND VERIFICATION

### Check if CAN is running
```cpp
if (CanManager::instance().isReady()) {
    // CAN is operational
}
```

### Get detailed status
```cpp
const char* status = can_hw_get_status_string();
Serial.printf("CAN Status: %s\n", status);  // "RUNNING", "BUS_OFF", etc.
```

### Count frames to verify bus health
```cpp
int frame_count = can_hw_count_frames(1000);  // Listen for 1 second
if (frame_count == 0) {
    Serial.println("No CAN frames - check bus connection");
} else {
    Serial.printf("Received %d frames - bus is active\n", frame_count);
}
```

## 🔧 HARDWARE CONSTANTS (READ-ONLY)

Defined in `hardware_config.h` - DO NOT MODIFY DIRECTLY

- `HW_TWAI_TX_PIN` = GPIO 20
- `HW_TWAI_RX_PIN` = GPIO 19
- `HW_I2C_SDA_PIN` = GPIO 8
- `HW_I2C_SCL_PIN` = GPIO 9
- `HW_CAN_GATE_I2C_ADDR` = 0x38 (CH422G expander)
- `HW_CAN_GATE_BIT` = 5 (USB_SEL - HIGH for CAN mode)

## 📚 MORE INFORMATION

See `HARDWARE_CONTRACT.md` for:
- Detailed explanation of the hardware requirement
- Why the gate is necessary
- Complete initialization sequence
- Troubleshooting guide
- Team communication template
