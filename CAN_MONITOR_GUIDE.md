# CAN Bus Monitor - Quick Start Guide

## 🎯 Purpose

Real-time CAN frame monitoring through the web interface - **NO MORE GPIO PIN CONFLICTS!**

This eliminates the need to switch between configurations when debugging CAN communication with POWERCELL NGX modules.

## 🚀 How to Use

### 1. Upload Firmware
```powershell
pio run -e waveshare_7in -t upload
```

### 2. Connect to Device WiFi
- SSID: **CAN-Control** (or your configured AP name)
- Default IP: **192.168.4.250**

### 3. Open CAN Monitor
Navigate to: **http://192.168.4.250/can-monitor**

Or click the "CAN Monitor" link from the main page (if you add it).

## 📊 What You'll See

### Live Dashboard
- **WebSocket Status**: Connection to device
- **Frames Received**: Total frame count
- **Frame Rate**: Frames per second

### Frame Table (Real-Time)
| Column | Description |
|--------|-------------|
| **Time** | Timestamp (HH:MM:SS.mmm) |
| **ID (Hex)** | Full 29-bit J1939 identifier |
| **PGN** | Parameter Group Number (extracted) |
| **SA** | Source Address |
| **DA** | Destination Address |
| **Data (Hex)** | Raw data bytes |
| **Decoded** | POWERCELL NGX interpretation |

### Decoded POWERCELL Frames

When a frame matches POWERCELL NGX format (PGN 0xFF01-0xFF0A):

```
POWERCELL FRONT:
OUT1-8: 11111111  ← All outputs ON
OUT9-10: 11        ← Outputs 9-10 ON
SS: 00 PWM: 00    ← No soft-start or PWM
```

**Byte Breakdown:**
- Byte 0: Outputs 1-8 bitmap (bit 0 = OUT1, bit 7 = OUT8)
- Byte 1: Outputs 9-10 bitmap (bit 0 = OUT9, bit 1 = OUT10)
- Byte 2: Soft-start enable bitmap
- Byte 3: PWM enable bitmap
- Bytes 4-7: Reserved (always 0x00)

## 🎮 Controls

| Button | Function |
|--------|----------|
| **🗑️ Clear** | Clear all frames from table |
| **⏸️ Pause** | Pause live updates (resume with ▶️) |
| **🏠 Back to Home** | Return to main config page |
| **Auto-scroll** | Keep newest frames at top |
| **Decode POWERCELL** | Show/hide frame decoding |

## 🔍 Debugging Workflows

### Check if POWERCELL is Responding

1. Open CAN Monitor
2. Send a test frame from Serial or web API:
   ```
   cansend FF01 FF 03 00 00 00 00 00 00
   ```
3. Watch for outgoing frame (ID ending in F9 = your device)
4. Look for response from POWERCELL (ID ending in 01 = address 1)

### Verify Flash Behavior

1. Trigger flash on an output (e.g., left turn signal)
2. Watch frames in monitor - you should see:
   - Alternating patterns every ~500ms
   - Byte 0 toggling specific bits
   - Other bytes unchanged (state preservation)

### Diagnose "All Outputs Turn Off"

If outputs are turning off unexpectedly:

1. Monitor frames for 10-20 seconds
2. Look for frames with **all zeros** in data bytes
3. Check if PGN matches expected POWERCELL address
4. If zeros appear: **Shadow state is being cleared somewhere**
5. If no frames: **Behavior engine stopped transmitting**

### Test Scene Activation

1. Activate a scene (e.g., "Hazard Lights")
2. You should see:
   - **One frame per POWERCELL** (FRONT + REAR)
   - Multiple output bits set simultaneously
   - Frames continuing at 20 Hz refresh rate

## 🌐 API Endpoints (Alternative Access)

### WebSocket (Real-Time)
```javascript
ws://192.168.4.250/ws/can
```

Receives JSON messages:
```json
{
  "type": "can_frame",
  "id": "18FF01F9",
  "timestamp": 12345678,
  "data": [255, 3, 0, 0, 0, 0, 0, 0]
}
```

### HTTP Poll (Batch)
```
GET http://192.168.4.250/api/can/receive?timeout=500
```

Returns up to 500ms worth of frames:
```json
{
  "count": 42,
  "messages": [
    {
      "id": "18FF01F9",
      "timestamp": 12345,
      "data": [255, 3, 0, 0, 0, 0, 0, 0]
    }
  ]
}
```

### Send Test Frame
```bash
POST http://192.168.4.250/api/can/send
Content-Type: application/json

{
  "pgn": 65281,
  "priority": 6,
  "source": 249,
  "destination": 255,
  "data": [255, 3, 0, 0, 0, 0, 0, 0]
}
```

## 🛠️ Troubleshooting

### "WebSocket Disconnected"
- Check WiFi connection
- Refresh page
- Monitor will auto-reconnect after 3 seconds

### "Waiting for CAN frames..."
- Verify CAN bus is initialized: Serial command `canstatus`
- Check physical connections (CAN H/L, termination)
- Send test frame to verify TX path

### No Frames Decoded
- Enable "Decode POWERCELL" checkbox
- Frames only decode if PGN is 0xFF01-0xFF0A
- Other frames show "-" in Decoded column

### Frame Rate is Zero
- No traffic on bus (expected if devices are idle)
- CAN bus not connected
- Wrong TX/RX pins (unlikely with current config)

## 💡 Pro Tips

1. **Leave monitor open** while using touchscreen - see exactly what frames are sent
2. **Use Pause** to freeze display and examine specific frames
3. **Compare timestamps** to verify 20 Hz transmission (50ms between frames)
4. **Watch for gaps** - any period >50ms without frames = engine stopped
5. **Filter by SA** - look at Source Address to identify which device sent frame

## 📝 Integration with Behavior Engine

The CAN monitor is **read-only** - it doesn't interfere with normal operation:

- ✅ Behavior engine continues running
- ✅ Touchscreen inputs work normally  
- ✅ Scenes activate as usual
- ✅ Flash/fade timers unaffected
- ✅ Zero performance impact when no clients connected

Monitor is perfect for **development**, **debugging**, and **verification** - not for production use.

---

## Next Steps

Once you've verified frames are correct:

1. Check behavior engine timing (20 Hz = 50ms period)
2. Verify shadow state merging logic
3. Confirm scene composition includes all required outputs
4. Test state persistence across button presses

Good luck! 🚗💨
