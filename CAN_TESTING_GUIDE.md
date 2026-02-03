# CAN Testing Guide

## Quick Diagnostic Commands

The firmware now includes web API endpoints for CAN diagnostics:

### 1. Hardware Diagnostic
Prints TWAI status and RX pin state to serial console:
```
GET http://192.168.4.250/api/can/diag
```

Expected output in serial console:
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
├─ GPIO RX Pin State (RAW GPIO READ - 100 samples over 100ms):
  ones=XX, zeros=YY, transitions=ZZ
  ✓ Pin transitions detected - transceiver appears connected!
└─ Diagnostic Complete
```

### 2. CAN Sniffer Test
Listens for CAN frames for specified duration (default 5 seconds):
```
GET http://192.168.4.250/api/can/test?duration=5000
```

Expected output in serial console if transceiver is working:
```
[CanManager] CAN SNIFFER TEST - Listening for 5000 ms...
[RX #1] EXT ID=0x18FEF100 DLC=8  00 00 00 00 00 00 00 00
[RX #2] EXT ID=0x18FF0180 DLC=8  A0 00 00 00 00 00 00 00
[RX #3] EXT ID=0x18FF0280 DLC=8  80 00 00 00 00 00 00 00
...
[CanManager] Test complete: 47 frames received in 5000 ms
```

If NO frames are received, you'll see:
```
[CanManager] Test complete: 0 frames received in 5000 ms
[CanManager] ⚠️  NO FRAMES RECEIVED - CHECK:
  1. Transceiver enable pin (should be HIGH)
  2. CANH/CANL wiring + GND reference
  3. Bitrate match (try 250k and 500k)
  4. Bus termination (120Ω at ends)
  5. Actual bus traffic present
```

## Troubleshooting

### RX Pin Stuck HIGH (transitions=0)
This means:
- **Transceiver is disabled** - USB_SEL pin not HIGH
- **Wrong RX pin** - should be GPIO19
- **Bitrate mismatch** - try changing bitrate in `can_manager.cpp`
- **No termination** - need 120Ω resistor at bus ends

### RX Pin Shows Transitions but NO Frames Received
This means:
- **Bitrate mismatch** - most common issue
  - J1939 (Infinitybox): 250 kbps
  - Ford HS-CAN: 500 kbps
- **Wrong filter settings** - should be ACCEPT_ALL
- **TWAI driver issue** - check error counters

### Transceiver Enable Verification
The `setCanMode(true)` function should force USB_SEL (EXIO5 on CH422G) HIGH.

To verify it's working, check serial console during boot:
```
[CanManager] Force-setting CAN transceiver to ENABLED
[CanManager] USB_SEL set to HIGH (CAN mode)
```

If you see:
```
[CanManager] ⚠️  ERROR: IO Expander not initialized!
```
Then the expander isn't ready before CAN init - check boot sequence.

## Physical Wiring Checklist

1. **CANH** - Yellow/White wire to CAN transceiver CANH
2. **CANL** - Green/White wire to CAN transceiver CANL
3. **GND** - Black wire to CAN ground (CRITICAL - often forgotten!)
4. **Termination** - 120Ω resistor between CANH and CANL at BOTH ends of bus
   - For bench testing with 2 devices, you need ONE 120Ω at each device
   - For single-device testing, you may need to terminate at the device

## Bitrate Configuration

Current default: **250 kbps** (J1939 standard)

To change bitrate, edit `can_manager.cpp`:
```cpp
// For Ford HS-CAN (500k):
const twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
```

Available presets:
- `TWAI_TIMING_CONFIG_25KBITS()` - 25 kbps
- `TWAI_TIMING_CONFIG_50KBITS()` - 50 kbps
- `TWAI_TIMING_CONFIG_100KBITS()` - 100 kbps
- `TWAI_TIMING_CONFIG_125KBITS()` - 125 kbps
- `TWAI_TIMING_CONFIG_250KBITS()` - 250 kbps (default - J1939)
- `TWAI_TIMING_CONFIG_500KBITS()` - 500 kbps (Ford HS-CAN)
- `TWAI_TIMING_CONFIG_800KBITS()` - 800 kbps
- `TWAI_TIMING_CONFIG_1MBITS()` - 1 Mbps

## Serial Monitor Setup

Use PlatformIO serial monitor:
```powershell
pio device monitor --baud 115200
```

Or any serial terminal at 115200 baud, 8N1, on COM5 (or your detected port).
