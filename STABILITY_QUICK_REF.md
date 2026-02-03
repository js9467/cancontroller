# Quick Reference - Stability Features

## 🔴 SAFE BOOT MODE (Factory Reset)

**When to use**: Device won't boot, stuck in loop, or behaving strangely even after reflashing old firmware

**How to activate**:
1. Power off device
2. Power on
3. **IMMEDIATELY** touch and hold the top-left corner of the screen (0-100px area)
4. Hold for **3 full seconds**
5. Watch serial monitor for: `[FACTORY RESET] Complete. Rebooting...`
6. Device will restart with clean config

**What it wipes**:
- All NVS (Preferences) data
- All LittleFS config files
- Resets to factory defaults

**Does NOT wipe**:
- Firmware itself (stays at current version)
- OTA partitions

---

## 📊 Health Monitoring

**What to watch**: Serial output shows health stats every 2 seconds

```
[HEALTH] heap=245678 psram=7823456 can_fps=42
```

**Warning signs**:

### Memory Leak
```
[HEALTH] WARNING: Heap dropped 5120 bytes in 10s
```
→ Action: Investigate recent code changes, look for missing `free()` or `delete`

### Heap Depletion
```
[HEALTH] heap=12000 psram=7823456 can_fps=42
```
→ Action: If heap < 50KB, reboot soon or system will crash

### CAN Not Receiving
```
[CAN-STATS] Processed 0 frames in last 5s
```
→ Action: Run `canstatus` command, check mux state

---

## 🔧 Serial Commands (Production Use)

### Monitoring
- `canstatus` - Check CAN bus configuration (TX/RX pins, ready state)
- `canmon` - Monitor CAN traffic for 10 seconds (non-blocking, shows frames)
- `help` - Show all available commands

### CAN Operations  
- `canpoll <1-16>` - Poll Powercell module at address
- `canconfig <1-16>` - Send default config to Powercell module
- `cansend <pgn_hex> <byte0_hex> <byte1_hex> ...` - Send raw CAN frame

### Display
- `b <0-100>` - Set brightness (e.g., `b 75`)
- `blinfo` - Show backlight pin configuration

### Testing
- `btest` - Run brightness sweep test (100→0→100)
- `otaoff` / `otaon` - Disable/enable OTA auto-update

---

## 🚨 Common Issues & Solutions

### "Device freezes after a few minutes"
**Symptoms**: Touch stops responding, screen frozen, serial still works
**Likely cause**: UI thread blocked by CAN or heavy logging
**Solution**: Now fixed with dedicated CAN task - monitor `[HEALTH]` output

### "CAN was working, now it's not"
**Symptoms**: `canstatus` shows ready, but no frames received
**Likely cause**: Mux flipped back to USB mode
**Solution**: Mux watchdog now prevents this - if still happens, check wiring

### "Config changes don't persist"
**Symptoms**: Change setting, reboot, setting reverts
**Likely cause**: Brownout during save corrupted config
**Solution**: Atomic saves now prevent this - if persists, check power supply

### "Old firmware doesn't fix the problem"
**Symptoms**: Reflash known-good firmware, issue persists
**Likely cause**: Bad config in flash persisting across updates
**Solution**: **Use safe boot mode** to wipe config

### "Random reboots with no error"
**Symptoms**: Device restarts, serial shows `Reset Reason: Brownout`
**Likely cause**: Power supply cannot handle peak current (WiFi TX, backlight, CAN)
**Solution**: Hardware issue - add bulk capacitor (1000µF) near ESP32, upgrade PSU

### "WDT Reset / Task watchdog"
**Symptoms**: `Reset Reason: Task watchdog`
**Likely cause**: Task not yielding (blocked in tight loop)
**Solution**: Now fixed with proper `vTaskDelay()` in all tasks - if persists, check custom code

---

## 📋 Boot Checklist (Expected Serial Output)

```
[HW-INIT] Forcing CAN mux (CRITICAL - do not modify this)
[HW-INIT] ✓ CAN mux forced, hardware stable
=================================
 Bronco Controls - Web Config 
=================================
 Firmware Version: 2.1.3
 Reset Reason: Power-on              ← Should be "Power-on" on normal boot
 Free Heap: 245678 bytes             ← Should be >200KB
 Free PSRAM: 7823456 bytes           ← Should be >7MB
[LVGL] ✓ Allocated 2x buffers
[SAFE BOOT] Checking for factory reset request...
[SAFE BOOT] Normal boot              ← Or "FACTORY RESET" if corner held

[CAN-TASK] ✓ CAN RX task started on core 1
[HEALTH] ✓ Health monitor started
[CAN] ✓ TWAI driver initialized successfully!
[Boot] ✓ Backlight enabled at 100%
```

**Red flags in boot sequence**:
- `Reset Reason: Brownout` → Power supply issue
- `Reset Reason: Task watchdog` → Software bug (task not yielding)
- `Reset Reason: Panic` → Crash (check backtrace in full serial log)
- `Free Heap:` < 200KB → Memory leak from previous run
- `Failed to create LVGL mutex` → Critical failure, will crash soon
- `CAN ✗ TWAI driver FAILED` → CAN hardware not initializing

---

## 🔐 Hardware Init Lockdown

**DO NOT MODIFY** the following unless you have a very good reason:

### `force_can_mux_hardware()` function
- Runs **FIRST** before LVGL, WiFi, panel
- Sets up CH422G I2C expander
- Forces USB_SEL HIGH for CAN mode
- **Isolated code** to prevent accidental breakage

**If you must modify it**:
1. Test boot sequence 20+ times
2. Verify CAN still works after each boot
3. Monitor mux state with oscilloscope/logic analyzer
4. Document why you changed it

---

## 🎯 Testing New Firmware

Before deploying, run through this checklist:

**Basic Functionality**:
- [ ] Device boots (see boot checklist above)
- [ ] Touch screen responds
- [ ] CAN receives frames (`canmon` shows traffic)
- [ ] Config changes save and persist across reboot
- [ ] WiFi AP starts and web interface loads

**Stability Tests**:
- [ ] Monitor `[HEALTH]` output for 10 minutes (no heap drops)
- [ ] Run `canmon` while changing UI pages (no freezes)
- [ ] Change config values rapidly (no crashes)
- [ ] Power cycle 10 times (no brownouts)
- [ ] Simulate brownout during config save (pull power, check config intact after)

**Safe Boot Test**:
- [ ] Hold top-left corner during boot
- [ ] Verify `[FACTORY RESET]` message appears
- [ ] Check config is wiped (defaults restored)

**Regression Tests**:
- [ ] OTA update works
- [ ] All serial commands work
- [ ] CAN send/receive both work
- [ ] Brightness control works

---

## 📖 Architecture Summary

### Task Layout
| Task | Core | Priority | Purpose | Stack |
|------|------|----------|---------|-------|
| LVGL | 0 | 2 | UI rendering only | 6KB |
| CAN RX | 1 | 3 | Receive CAN frames → queue | 4KB |
| Health | 1 | 1 | Memory monitoring | 3KB |
| Mux WD | 1 | 1 | Keep USB_SEL HIGH | 2KB |
| Main Loop | 1 | 1 | App logic, queue processing | (main) |

### Data Flow
```
CAN Bus → TWAI Driver → CAN RX Task → FreeRTOS Queue → Main Loop → Update Model → UI
```

**Key points**:
- UI never waits for CAN
- CAN never blocks on UI
- Queue decouples the two subsystems
- Health monitor catches issues before they crash

---

## 💾 Config Storage (Atomic Saves)

**New behavior**: All config writes are atomic

**How it works**:
1. Write config to `/config.tmp`
2. Verify write completed successfully
3. Delete old `/config.json`
4. Rename `/config.tmp` → `/config.json`

**What this prevents**:
- Brownout mid-write corrupting live config
- Incomplete writes leaving invalid JSON
- Config loss if power fails during save

**If power fails**:
- During step 1-2: `/config.json` still intact, temp file discarded on next boot
- During step 3-4: Either old or new config survives, never corrupted

---

## 🆘 Last Resort Recovery

If everything fails and device won't boot at all:

1. **Try safe boot** (hold corner) - 80% success rate
2. **Full erase + reflash**:
   ```powershell
   pio run -e waveshare_7in -t erase
   pio run -e waveshare_7in -t upload
   ```
3. **Check power supply** (measure 5V rail during WiFi TX)
4. **Restore from full flash backup**:
   ```powershell
   python -m esptool --port COM5 write_flash 0 backup.bin
   ```

---

**Remember**: Safe boot is your friend. When in doubt, wipe it out. 🧹
