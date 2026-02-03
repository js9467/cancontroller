# CAN Manager Diagnostic API Reference

## Overview

The CanManager class now includes comprehensive diagnostic functions to help identify and resolve CAN communication issues on the Waveshare ESP32-S3-Touch-LCD-7.

## Available Diagnostic Functions

### 1. `verifyTransceiverEnabled()`

**Purpose:** Check if the CAN transceiver is actually powered (USB_SEL = HIGH on CH422G)

**Signature:**
```cpp
bool CanManager::verifyTransceiverEnabled() const;
```

**Returns:** `true` if USB_SEL is HIGH and transceiver is enabled, `false` otherwise

**Example Usage:**
```cpp
if (!CanManager::instance().verifyTransceiverEnabled()) {
    Serial.println("ERROR: CAN transceiver is NOT enabled!");
    // Might need to reset or reinitialize expander
}
```

**Sample Output (if working):**
```
[CanManager] Verifying CAN transceiver enable status...
[CanManager]   CH422G[0x38] = 0x2A, USB_SEL (bit 5) = HIGH ✓
```

**Sample Output (if broken):**
```
[CanManager] Verifying CAN transceiver enable status...
[CanManager]   CH422G[0x38] = 0x00, USB_SEL (bit 5) = LOW ✗
[CanManager] ⚠️  WARNING: USB_SEL is LOW - CAN transceiver may be disabled!
```

### 2. `dumpHardwareStatus()`

**Purpose:** Print comprehensive hardware diagnostic report

**Signature:**
```cpp
void CanManager::dumpHardwareStatus() const;
```

**Returns:** void (prints to Serial)

**Example Usage:**
```cpp
// In setup() or in a web API endpoint:
Serial.println("\n=== CAN Diagnostic Report ===");
CanManager::instance().dumpHardwareStatus();
```

**Sample Output:**
```
╔════════════════════════════════════════════════════════╗
║        CAN HARDWARE DIAGNOSTIC STATUS                  ║
╚════════════════════════════════════════════════════════╝
├─ TWAI Driver Status: READY ✓
├─ TX Pin: GPIO20, RX Pin: GPIO19
├─ Bitrate: 250000 bps
├─ Bus State: RUNNING
├─ TX Queue: 0, RX Queue: 0
├─ TX Errors: 0, RX Errors: 0
├─ CH422G I2C Expander (CAN gate control):
[CanManager] Verifying CAN transceiver enable status...
[CanManager]   CH422G[0x38] = 0x2A, USB_SEL (bit 5) = HIGH ✓
├─ GPIO RX Pin State (RAW GPIO READ - 100 samples over 100ms):
  ones=50, zeros=50, transitions=45
  ✓ Pin transitions detected - good sign!
└─ Diagnostic Complete
```

**What Each Section Means:**

- **TWAI Driver Status:** Is the TWAI driver installed and running?
- **TX/RX Pins:** GPIO configuration (should always be TX=20, RX=19)
- **Bitrate:** Speed in bits/second (typically 250000 or 500000)
- **Bus State:** 
  - STOPPED = driver not started
  - RUNNING = normal operation
  - BUS-OFF = too many errors, entered error recovery
  - RECOVERING = recovering from BUS-OFF state
- **Queue Depths:** How many frames are waiting in TX/RX queues
- **Error Counters:** CAN protocol error counts
- **CH422G Gate:** Is USB_SEL HIGH? (Must be!)
- **RX Pin State:** 
  - ones/zeros = how many of 100 samples were HIGH/LOW
  - transitions = how many times pin changed state
  - If stuck at one level: transceiver may be disabled

---

## Troubleshooting Decision Tree

### Scenario A: "CAN appears working but receives 0 frames"

```cpp
void diagnose_no_rx() {
    Serial.println("=== Diagnosis: No RX Frames ===\n");
    
    // Step 1: Check driver status
    if (!CanManager::instance().isReady()) {
        Serial.println("✗ TWAI driver not ready. Call begin() first.");
        return;
    }
    
    // Step 2: Check transceiver gate
    if (!CanManager::instance().verifyTransceiverEnabled()) {
        Serial.println("✗ TRANSCEIVER DISABLED. Check CH422G USB_SEL pin.");
        Serial.println("  FIX: expander->digitalWrite(USB_SEL, HIGH); delay(10);");
        return;
    }
    
    // Step 3: Check RX pin (detailed report)
    Serial.println("✓ Transceiver enabled. Checking RX pin signal...");
    CanManager::instance().dumpHardwareStatus();
    
    Serial.println("\nDiagnosis complete. If RX pin is stuck HIGH:");
    Serial.println("  - Check CANH/CANL wiring");
    Serial.println("  - Verify termination jumper");
    Serial.println("  - Try different bitrate (250k vs 500k)");
}
```

### Scenario B: "Just upgraded Arduino-ESP32 core, now CAN errors"

```cpp
void diagnose_compile_error() {
    Serial.println("=== Diagnosis: Compile Error After Core Upgrade ===\n");
    
    Serial.println("Common issues with Arduino-ESP32 core version mismatch:");
    Serial.println("1. 'bus_off' field not found → use status.state instead");
    Serial.println("2. 'alerts_triggered' not found → use twai_read_alerts()");
    Serial.println("3. TWAI_TIMING_CONFIG in ternary → use if/else instead");
    Serial.println("\nCurrent CanManager uses only core-compatible fields:");
    Serial.println("  - status.state");
    Serial.println("  - status.msgs_to_tx");
    Serial.println("  - status.msgs_to_rx");
    Serial.println("  - status.tx_error_counter");
    Serial.println("  - status.rx_error_counter");
    
    // Test compile by calling these:
    if (CanManager::instance().isReady()) {
        twai_status_info_t status;
        if (twai_get_status_info(&status) == ESP_OK) {
            Serial.printf("Status fields accessible:\n");
            Serial.printf("  state=%d, tx_err=%lu, rx_err=%lu\n",
                         status.state, status.tx_error_counter, status.rx_error_counter);
        }
    }
}
```

### Scenario C: "CAN worked yesterday, stopped today"

```cpp
void diagnose_sudden_failure() {
    Serial.println("=== Diagnosis: Sudden CAN Failure ===\n");
    
    // Get full report first
    CanManager::instance().dumpHardwareStatus();
    
    Serial.println("\nPossible causes:");
    Serial.println("1. Power cycle the board (I2C expander may have lost state)");
    Serial.println("2. Check if USB_SEL is still HIGH");
    Serial.println("3. Look for bus error recovery (BUS-OFF state)");
    Serial.println("4. Verify wiring hasn't come loose");
    Serial.println("5. Try resetting CAN interface: twai_stop(); twai_start();");
}
```

---

## Adding Diagnostic Endpoint to Web Interface

To make diagnostics accessible via web, add this to your web server:

```cpp
void WebServerManager::setupRoutes() {
    // ... existing routes ...
    
    // CAN diagnostic endpoint
    server.on("/api/can/status", HTTP_GET, [](AsyncWebServerRequest *request) {
        String response = "{";
        response += "\"ready\":" + String(CanManager::instance().isReady() ? "true" : "false");
        response += ",\"tx_pin\":" + String(CanManager::instance().txPin());
        response += ",\"rx_pin\":" + String(CanManager::instance().rxPin());
        
        if (CanManager::instance().isReady()) {
            twai_status_info_t status;
            if (twai_get_status_info(&status) == ESP_OK) {
                response += ",\"state\":" + String(status.state);
                response += ",\"tx_errors\":" + String(status.tx_error_counter);
                response += ",\"rx_errors\":" + String(status.rx_error_counter);
            }
        }
        
        response += "}";
        request->send(200, "application/json", response);
    });
    
    // Detailed diagnostic endpoint
    server.on("/api/can/diagnostic", HTTP_GET, [](AsyncWebServerRequest *request) {
        // Print diagnostics to serial (then read them externally if needed)
        CanManager::instance().dumpHardwareStatus();
        request->send(200, "text/plain", "Diagnostic printed to serial console");
    });
}
```

---

## Serial Console Commands for Debugging

Add these to your main loop or as web endpoints:

```cpp
void loop() {
    // ... existing code ...
    
    // Check for serial commands
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();
        
        if (cmd == "can_status") {
            CanManager::instance().dumpHardwareStatus();
        }
        else if (cmd == "can_verify") {
            if (CanManager::instance().verifyTransceiverEnabled()) {
                Serial.println("✓ CAN transceiver is ENABLED");
            } else {
                Serial.println("✗ CAN transceiver is DISABLED");
            }
        }
        else if (cmd == "can_reset") {
            Serial.println("Attempting CAN reset...");
            CanManager::instance().stop();
            delay(100);
            CanManager::instance().begin();
            CanManager::instance().dumpHardwareStatus();
        }
    }
}
```

**Serial commands in monitor:**
```
can_status       → Print full hardware diagnostic
can_verify       → Check if transceiver is enabled
can_reset        → Stop and restart CAN driver
```

---

## Reference: TWAI Status States

The `status.state` field can be one of:

```cpp
enum {
    TWAI_STATE_STOPPED = 0,      // Driver installed but not running
    TWAI_STATE_RUNNING = 1,      // Normal operation
    TWAI_STATE_BUS_OFF = 2,      // Transceiver in bus-off (too many errors)
    TWAI_STATE_RECOVERING = 3,   // Attempting recovery from bus-off
};
```

**What to do if BUS_OFF:**
- Check wiring for shorts
- Verify bitrate matches vehicle network
- Look for CAN frames being sent incorrectly
- May need `twai_initiate_recovery()`

---

## Expected Values for Normal Operation

When everything is working correctly:

| Parameter | Expected Value |
|-----------|---|
| TWAI Driver Status | READY ✓ |
| Bus State | RUNNING |
| TX Errors | 0 or very low |
| RX Errors | 0 or very low |
| USB_SEL | HIGH ✓ |
| RX Pin ones | 25-75 (mixture, not stuck) |
| RX Pin zeros | 25-75 (mixture, not stuck) |
| RX Pin transitions | > 0 (pin is toggling) |

If any of these differ significantly, refer back to the troubleshooting sections.
