# CAN Monitor Test Script

## Quick Test Commands

Once firmware is uploaded and CAN monitor is open in browser:

### 1. Check CAN Status
```
canstatus
```
Expected output:
```
CAN Ready: YES
TX Pin: GPIO20
RX Pin: GPIO19
Bitrate: 250 kbps
```

### 2. Send Test Frame to POWERCELL FRONT (Address 1)
```
cansend FF01 FF 03 00 00 00 00 00 00
```
This turns on:
- All outputs 1-8 (Byte 0 = 0xFF)
- Outputs 9 and 10 (Byte 1 = 0x03)
- No soft-start or PWM

**Watch the monitor** - you should see this frame appear immediately!

### 3. Test Individual Outputs

Turn on ONLY Output 1 (Left Turn Front):
```
cansend FF01 01 00 00 00 00 00 00 00
```

Turn on ONLY Output 2 (Right Turn Front):
```
cansend FF01 02 00 00 00 00 00 00 00
```

Turn on ONLY Output 7 (High Beams):
```
cansend FF01 40 00 00 00 00 00 00 00
```

### 4. Test POWERCELL REAR (Address 2)

All outputs ON:
```
cansend FF02 FF 03 00 00 00 00 00 00
```

Brake lights only (Output 3):
```
cansend FF02 04 00 00 00 00 00 00 00
```

### 5. Test with Soft-Start

Turn on Output 1 with soft-start enabled:
```
cansend FF01 01 00 01 00 00 00 00 00
```
- Byte 0 = 0x01 (Output 1 ON)
- Byte 2 = 0x01 (Soft-start on Output 1)

### 6. Test with PWM

Turn on Output 5 with PWM enabled:
```
cansend FF01 10 00 00 10 00 00 00 00
```
- Byte 0 = 0x10 (Output 5 ON, binary 00010000)
- Byte 3 = 0x10 (PWM on Output 5)

### 7. Monitor Bus Traffic
```
canmon
```
This will display all CAN frames for 10 seconds in Serial console.
**Also visible in web monitor!**

### 8. Test Flash Pattern

Manually simulate flash by alternating frames every 500ms:

Frame 1 (ON):
```
cansend FF01 01 00 00 00 00 00 00 00
```

Wait 500ms, then Frame 2 (OFF):
```
cansend FF01 00 00 00 00 00 00 00 00
```

Repeat - you should see the pattern in the monitor!

---

## Expected Monitor Behavior

### Successful Frame Transmission

You should see in the web monitor:

| Time | ID (Hex) | PGN | SA | DA | Data (Hex) | Decoded |
|------|----------|-----|----|----|-----------|---------|
| 14:23:45.123 | 18FF01F9 | 0xFF01 | F9 | FF | FF 03 00 00 00 00 00 00 | POWERCELL FRONT:<br>OUT1-8: 11111111<br>OUT9-10: 11<br>SS: 00 PWM: 00 |

### Frame Breakdown

For ID `18FF01F9`:
- `18` = Priority 6 (bits 26-28 = 6, or 0b110)
- `FF01` = PGN (0xFF01 = POWERCELL FRONT)
- `F9` = Source Address (your ESP32)
- `FF` = Destination (broadcast)

---

## Debugging Checklist

If frames don't appear:

- [ ] CAN monitor page is open and WebSocket shows "Connected"
- [ ] `canstatus` shows "CAN Ready: YES"
- [ ] POWERCELL modules are powered and connected to bus
- [ ] CAN H and CAN L are correctly wired
- [ ] Bus has proper termination (120Ω at each end)
- [ ] No short circuits between CAN H and CAN L

If frames appear but POWERCELL doesn't respond:

- [ ] POWERCELL address matches PGN (0xFF01 = Address 1)
- [ ] POWERCELL is configured for J1939 mode
- [ ] POWERCELL is using 250 kbps bitrate
- [ ] Data bytes follow NGX format (not legacy format)

---

## Integration Test

Once basic frames work, test with behavior engine:

### Via Serial (Old Way)
```
ibox left_turn_signal_front flash
```

### Via Touchscreen (Production)
- Tap "Left Turn" button
- Set to "Flash" mode
- Activate

### What to Look For

In CAN monitor, you should see:
1. **Continuous frames at ~20 Hz** (every 50ms)
2. **Byte 0 toggling bit 0** (0x01 → 0x00 → 0x01...)
3. **Other outputs remain stable** (if other functions are active)
4. **No gaps in transmission** (engine maintains 20 Hz even during flash)

---

## Performance Check

Monitor frame rate display:
- **Expected**: 20-40 frames/second (depending on how many devices are active)
- **Good**: Steady rate, no drops
- **Bad**: Frames come in bursts, then gaps

If you see gaps:
- Behavior engine timing issue
- LVGL blocking CAN task
- Mutex contention in main loop

---

## Final Validation

1. Open monitor: http://192.168.4.250/can-monitor
2. Send test frame: `cansend FF01 FF 03 00 00 00 00 00 00`
3. Verify frame appears in table within 100ms
4. Check decoded output shows "POWERCELL FRONT"
5. Try pausing, clearing, resuming
6. Close browser tab (should auto-reconnect if reopened)

✅ **If all of this works, CAN monitoring is ready for debugging your behavior engine!**
