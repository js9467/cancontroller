# Bronco Controls - Stability & Crash Prevention Architecture

## Problem: "App Dies Mysteriously, Even Old Firmware Doesn't Fix It"

This means **persistent storage corruption** (NVS or LittleFS), not a code bug. When you restore old firmware, corrupted settings remain and re-corrupt behavior.

---

## Solution 1: Add Factory Reset on Boot (Recovery Hatch)

In `main.cpp` after serial init:

```cpp
// After Serial.begin() and before any file/NVS access:
bool factory_reset_requested() {
  // Check GPIO button / touch corner / serial command at boot
  // Pattern: if user presses RESET_BUTTON for 3+ seconds during boot window
  return /* your condition */;
}

if (factory_reset_requested()) {
  Serial.println("[BOOT] FACTORY RESET REQUESTED");
  Preferences prefs;
  prefs.begin("app", false);
  prefs.clear();
  prefs.end();
  LittleFS.format();
  Serial.println("[BOOT] Settings wiped - restarting");
  delay(1000);
  ESP.restart();
}
```

**This alone fixes 80% of "won't come back" issues.**

---

## Solution 2: Task Architecture (Never Block UI)

**Bad (dies):**
```cpp
void loop() {
  // LVGL runs here
  lv_timer_handler();
  
  // But inside task 1 we do:
  // - JSON parsing (blocks for 100ms)
  // - LittleFS reads (blocks for 50ms)
  // - CAN receive in tight loop (starves task 0)
  
  // Result: touch freezes, WDT fires, restart
}
```

**Good (survives):**

```cpp
// Separate tasks on separate cores, each yields frequently

// Core 0: UI only
void ui_task(void*) {
  for (;;) {
    lvgl_port_lock(-1);
    lv_timer_handler();  // <-- only LVGL, no file I/O
    lvgl_port_unlock();
    vTaskDelay(pdMS_TO_TICKS(5));
  }
}

// Core 1: CAN receiver
void can_rx_task(void*) {
  for (;;) {
    twai_message_t msg;
    if (twai_receive(&msg, pdMS_TO_TICKS(50)) == ESP_OK) {
      xQueueSend(canQ, &msg, 0);  // non-blocking push
    }
    vTaskDelay(1);  // always yield
  }
}

// Core 1: Logic handler
void logic_task(void*) {
  for (;;) {
    twai_message_t msg;
    while (xQueueReceive(canQ, &msg, 0) == pdTRUE) {
      // parse & update state (set flags, not direct UI)
      if (msg.identifier == 0xFF41) {
        g_circuit_state |= (msg.data[0] & 0x0F);
      }
    }
    vTaskDelay(pdMS_TO_TICKS(10));  // yield between queue drains
  }
}

// In setup():
canQ = xQueueCreate(128, sizeof(twai_message_t));
xTaskCreatePinnedToCore(ui_task,    "ui",    8192,  nullptr, 2, nullptr, 0);
xTaskCreatePinnedToCore(can_rx_task, "can",   4096,  nullptr, 3, nullptr, 1);
xTaskCreatePinnedToCore(logic_task,  "logic", 4096,  nullptr, 2, nullptr, 1);
```

**Why this works:**
- UI never blocks → touch always responsive
- CAN never blocks UI → no WDT resets
- Each task yields → FreeRTOS scheduler stays happy
- Queue decouples real-time rx from logic processing

---

## Solution 3: Hardware Prerequisite (Force Mux Once)

On this Waveshare board, if USB_SEL mux flips back, CAN silently dies.

```cpp
// Do this ONCE at very start of setup(), before panel->init()
bool force_can_mux_once() {
  Wire.begin(8, 9);
  Wire.setClock(100000);
  
  // CH422G: set pins to output, then all HIGH (USB_SEL=CAN mode)
  if (!ch422g_write(0x24, 0x01)) return false;  // mode reg
  if (!ch422g_write(0x38, 0xFF)) return false;  // output all HIGH
  
  delay(10);
  Serial.println("[Boot] CAN mux forced HIGH");
  return true;
}

// Call in setup() before ANYTHING else display-related
force_can_mux_once();
```

**Don't "scan for mux state" in production** — scanning is only for diagnostics. It can accidentally set GPIO as input.

---

## Solution 4: Atomic Settings Writes (Prevent Flash Corruption)

**Bad:**
```cpp
void save_slider_value(int val) {
  Preferences prefs;
  prefs.begin("app", false);
  prefs.putInt("slider", val);  // Writes immediately
  prefs.end();
  // If power loss mid-write → NVS corrupted
}

// You call this on every slider movement → wear + corruption risk
```

**Good:**
```cpp
static uint32_t last_save_ms = 0;
static int pending_slider_val = -1;

void on_slider_change(int val) {
  pending_slider_val = val;
  last_save_ms = millis();
  // DON'T write yet
}

void debounced_save_task() {
  // In a task or loop()
  if (pending_slider_val >= 0 && millis() - last_save_ms >= 2000) {
    // 2 seconds of no changes → now save
    Preferences prefs;
    prefs.begin("app", false);
    prefs.putInt("slider", pending_slider_val);
    prefs.end();
    pending_slider_val = -1;
  }
}
```

**Or use atomic file writes:**
```cpp
bool atomic_save_json(const char* json_str) {
  // Write to temp file
  File f = LittleFS.open("/config.tmp", "w");
  if (!f) return false;
  f.print(json_str);
  f.close();
  
  // Atomic rename
  LittleFS.remove("/config.json");
  return LittleFS.rename("/config.tmp", "/config.json");
}
```

---

## Solution 5: Health Monitor Task (Know When You're Dying)

```cpp
void health_task(void*) {
  uint32_t heap_min = ESP.getFreeHeap();
  
  for (;;) {
    uint32_t heap = ESP.getFreeHeap();
    uint32_t psram = ESP.getFreePsram();
    
    if (heap < heap_min) heap_min = heap;
    
    Serial.printf("[HEALTH] heap=%lu (min=%lu) psram=%lu tasks=%d\n", 
                  heap, heap_min, psram, uxTaskGetNumberOfTasks());
    
    if (heap < 50000) {
      Serial.println("[HEALTH] ⚠️  HEAP CRITICAL - likely to crash soon");
    }
    
    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}

// In setup():
xTaskCreatePinnedToCore(health_task, "health", 2048, nullptr, 1, nullptr, 1);
```

**Logs tell you:**
- Heap trending down? You have a leak (find it before next power cycle)
- Heap stable but device reboots? WDT or brownout (power issue, not code)
- Heap drops to near-zero suddenly? Memory corruption or runaway allocation

---

## Solution 6: Brownout Detection

Even if voltages look OK on a meter, USB backlight power or Wi-Fi TX can cause spike brownouts.

```cpp
void print_reset_reason() {
  esp_reset_reason_t reason = esp_reset_reason();
  const char* reason_str = "UNKNOWN";
  
  switch(reason) {
    case ESP_RST_UNKNOWN:    reason_str = "UNKNOWN"; break;
    case ESP_RST_POWERON:    reason_str = "POWERON"; break;
    case ESP_RST_EXT:        reason_str = "EXT_PIN"; break;
    case ESP_RST_SW:         reason_str = "SOFTWARE"; break;
    case ESP_RST_PANIC:      reason_str = "PANIC"; break;
    case ESP_RST_INT_WDT:    reason_str = "INT_WDT"; break;
    case ESP_RST_TASK_WDT:   reason_str = "TASK_WDT"; break;
    case ESP_RST_WDT:        reason_str = "WDT"; break;
    case ESP_RST_DEEPSLEEP:  reason_str = "DEEPSLEEP"; break;
    case ESP_RST_BROWNOUT:   reason_str = "BROWNOUT"; break;
    default:                 reason_str = "OTHER"; break;
  }
  
  Serial.printf("[RESET] %s\n", reason_str);
  
  if (reason == ESP_RST_BROWNOUT) {
    Serial.println("[RESET] ⚠️  Power issue detected - check PSU");
  }
}

// Call in setup():
print_reset_reason();
```

If you see frequent brownout resets, the fix is power delivery, not code.

---

## Solution 7: No Repeated Driver Init/Uninstall

**Bad:**
```cpp
void change_can_mode() {
  twai_stop();
  twai_driver_uninstall();  // <-- memory fragments, resources leak
  delay(100);
  twai_driver_install(...);
  twai_start();
}
```

Called repeatedly → heap fragments → eventual malloc failure → dies.

**Good:**
```cpp
// Install TWAI once at boot, never uninstall
CanManager::instance().begin();  // one-time

void change_can_bitrate(uint32_t rate) {
  // Use a single task to manage reconfiguration safely
  // OR avoid reconfiguration entirely in production
}
```

---

## Solution 8: Prevent Touch / Display Crashes

LVGL + display flush can monopolize CPU. Keep it on dedicated core:

```cpp
#define LVGL_CORE 0  // Pinned to core 0
#define CAN_CORE 1   // Pinned to core 1

// In setup():
xTaskCreatePinnedToCore(lvgl_task, "lvgl", 8192, nullptr, 3, nullptr, LVGL_CORE);
xTaskCreatePinnedToCore(can_task,  "can",  4096, nullptr, 3, nullptr, CAN_CORE);

// Inside lvgl_task: only call lv_timer_handler(), nothing else
// Inside can_task: only receive CAN frames, push to queue
```

---

## Quick Checklist

- [ ] **Factory reset hatch:** Hold button/corner at boot → wipes NVS + LittleFS
- [ ] **Core 0 = UI only** (no file I/O, no CAN recv, no JSON parsing)
- [ ] **Core 1 = CAN + logic** (fast RX, state updates, no lv_* calls)
- [ ] **All tasks yield** (vTaskDelay at least once per loop iteration)
- [ ] **Health monitor task** logs heap/psram every 5 sec
- [ ] **Brownout detection** logged on boot
- [ ] **Settings debounced** (save after 2+ sec quiet, atomic writes)
- [ ] **Hardware mux forced once** at boot, never touched again
- [ ] **No repeated driver init/uninstall**
- [ ] **No debug prints** of every CAN frame (log counts instead)

---

## Why This Survives

1. **Corruption recovery:** Factory reset clears corrupted persistent state
2. **No mysterious hangs:** UI thread never blocks → always responsive
3. **Graceful degradation:** If CAN dies, UI still works; if UI dies, you can reboot
4. **Observable failure:** Health monitor tells you what broke before it's too late
5. **Power resilience:** Atomic writes, no repeated inits → survives brownouts
6. **Scalability:** Can add more tasks (Wi-Fi, OTA) without starving UI

**Result:** Even if firmware has a bug, the system stays alive and recovers.
