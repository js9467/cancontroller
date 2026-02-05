/**
 * @file main.cpp
 * Bronco Controls - Web-configurable automotive HMI
 *
 * Boots the LVGL runtime, loads configuration from LittleFS,
 * and exposes a WiFi + web interface for live customization.
 * Version display and OTA update support.
 * 
 * HARDWARE INITIALIZATION: Direct calls (no BSP abstraction).
 * All core stability fixes are in this file:
 *   1. Synchronous LVGL flush (no async callback)
 *   2. Double-buffer LVGL (prevents tearing)
 *   3. Mux watchdog (reasserts USB_SEL every 1s)
 *   4. LVGL mutex created immediately after lv_init()
 */

#include <Arduino.h>
#include <ESP_Panel_Library.h>
#include <ESP_Panel_Conf.h>
#include <ESP_IOExpander_Library.h>
#include <lvgl.h>
#include <WiFi.h>
#include <string>
#include <esp_ota_ops.h>
#include <esp_system.h>
#include <rom/rtc.h>

#include "can_manager.h"
#include "config_manager.h"
#include "ipm1_can_system.h"
#include "ui_builder.h"
#include "ui_theme.h"
#include "web_server.h"
#include "ota_manager.h"
#include "version_auto.h"
#include "hardware_config.h"
#include "infinitybox_control.h"
#include "behavioral_output_integration.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <Preferences.h>

// CAN message queue structure
struct CanFrame {
    uint32_t id;
    bool ext;
    uint8_t dlc;
    uint8_t data[8];
    uint32_t timestamp_ms;
};

static QueueHandle_t g_can_queue = nullptr;
static constexpr size_t CAN_QUEUE_SIZE = 128;

enum class PanelVariant : uint8_t {
    kFourPointThreeInch = BRONCO_PANEL_VARIANT_4_3,
    kSevenInch = BRONCO_PANEL_VARIANT_7_0,
};

struct PanelConfig {
    PanelVariant variant;
    uint16_t width;
    uint16_t height;
    const char *name;
    uint32_t colorDepth;
};

static constexpr PanelConfig kPanelConfigs[] = {
    { PanelVariant::kFourPointThreeInch, 800, 480, "Waveshare 4.3", 16 },
    { PanelVariant::kSevenInch, 800, 480, "Waveshare 7.0", 16 },
};

static const PanelConfig& SelectPanelConfig()
{
    const uint32_t configuredVariant = BRONCO_PANEL_VARIANT;
    for (const auto &config : kPanelConfigs) {
        if (static_cast<uint32_t>(config.variant) == configuredVariant) {
            return config;
        }
    }
    return kPanelConfigs[0];
}

// Globals
ESP_Panel* panel = nullptr;
SemaphoreHandle_t lvgl_mux = nullptr;
static bool g_disable_ota = false;
static ESP_IOExpander* g_expander = nullptr;
static volatile bool g_safe_boot_requested = false;
static volatile uint32_t g_can_frames_received = 0;
Preferences g_prefs;

// LVGL Task timing constants
constexpr uint32_t LVGL_TASK_STACK_SIZE = 6 * 1024;
constexpr uint32_t LVGL_TASK_PRIORITY = 2;
constexpr uint32_t LVGL_TASK_MAX_DELAY_MS = 500;
constexpr uint32_t LVGL_TASK_MIN_DELAY_MS = 1;

// Forward declarations for LVGL helpers
void lvgl_port_lock(int timeout_ms);
void lvgl_port_unlock();

// Forward declarations for FreeRTOS tasks
void mux_watchdog_task(void*);
void suspension_tx_task(void*);
void can_rx_task(void*);
void health_monitor_task(void*);

// NOTE: Expander initialization moved to AFTER panel init
// No early hardware init needed - panel handles everything

// Safe boot detection: check if touch screen is being held during boot
bool detect_safe_boot() {
    if (!panel || !panel->getLcdTouch()) return false;
    
    panel->getLcdTouch()->readData();
    bool touched = panel->getLcdTouch()->getTouchState();
    
    if (!touched) return false;
    
    TouchPoint point = panel->getLcdTouch()->getPoint();
    // Top-left corner (within 100x100 pixels)
    return (point.x < 100 && point.y < 100);
}

// Factory reset: wipe all persistent storage
void factory_reset() {
    Serial.println("\n[FACTORY RESET] Wiping all settings...");
    
    // Clear NVS
    g_prefs.begin("app", false);
    g_prefs.clear();
    g_prefs.end();
    
    // Clear config manager storage
    ConfigManager::instance().factoryReset();
    
    Serial.println("[FACTORY RESET] Complete. Rebooting...");
    delay(1000);
    ESP.restart();
}

// Mux watchdog - reasserts all control pins (SAFE_MASK) to prevent flips from library code
// Runs every 1 second on core 1, ensuring USB_SEL stays HIGH and other outputs stable
void mux_watchdog_task(void*) {
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        if (g_expander) {
            // Reassert safe state using hardware_config.h constants
            // (multiDigitalWrite returns void, so no error checking)
            g_expander->multiDigitalWrite(HW_CH422G_SAFE_MASK, HIGH);
        }
    }
}

// Suspension TX task - sends 0x737 command every 300ms (separate from Infinitybox)
void suspension_tx_task(void*) {
    Serial.println("[Suspension] TX task started - 300ms cadence");
    
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(300));  // 300ms interval per spec
        
        // Only send if CAN is ready
        if (CanManager::instance().isReady()) {
            CanManager::instance().sendSuspensionCommand();
        }
    }
}

// CAN receive task - runs on core 1, never blocks UI
// Pulls messages from TWAI and pushes to queue for processing
void can_rx_task(void* param) {
    Serial.println("[CAN-TASK] RX task started on core 1");
    CanRxMessage msg;
    
    for (;;) {
        // Non-blocking receive with short timeout
        if (CanManager::instance().receiveMessage(msg, 50)) {
            // Check for suspension status frame (0x738)
            // Standard IDs are 11-bit (0-0x7FF), extended are 29-bit
            bool is_standard = (msg.identifier <= 0x7FF);
            if (is_standard && msg.identifier == 0x738 && msg.length == 8) {
                CanManager::instance().parseSuspensionStatus(msg.data);
            }
            if (!is_standard && msg.length == 8) {
                const uint32_t pgn = (msg.identifier >> 8) & 0x3FFFF;
                CanManager::instance().updatePowercellStatusFromPgn(pgn, msg.data);
            }
            
            // Queue frame for general processing (Infinitybox, diagnostics, etc.)
            CanFrame frame;
            frame.id = msg.identifier;
            frame.ext = (msg.identifier & 0x80000000) != 0;  // Infer from ID
            frame.dlc = msg.length;
            memcpy(frame.data, msg.data, 8);
            frame.timestamp_ms = millis();
            
            // Non-blocking queue send - drop if full (don't block CAN RX)
            if (xQueueSend(g_can_queue, &frame, 0) == pdTRUE) {
                g_can_frames_received++;
            }
        }
        
        // Always yield to prevent watchdog
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

// Health monitoring task - watches for memory leaks and system issues
void health_monitor_task(void* param) {
    Serial.println("[HEALTH] Monitor started");
    uint32_t last_heap = 0;
    uint32_t heap_drop_count = 0;
    
    for (;;) {
        uint32_t heap = ESP.getFreeHeap();
        uint32_t psram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
        uint32_t can_fps = g_can_frames_received;
        g_can_frames_received = 0;  // Reset counter
        
        // Detect sustained heap loss (possible leak)
        if (last_heap > 0 && heap < last_heap - 1024) {
            heap_drop_count++;
            if (heap_drop_count > 5) {
                Serial.printf("[HEALTH] WARNING: Heap dropped %lu bytes in 10s\n", last_heap - heap);
            }
        } else {
            heap_drop_count = 0;
        }
        last_heap = heap;
        
        Serial.printf("[HEALTH] heap=%lu psram=%lu can_fps=%lu\n", heap, psram, can_fps);
        
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

#if ESP_PANEL_LCD_BUS_TYPE == ESP_PANEL_BUS_TYPE_RGB
void lvgl_port_disp_flush(lv_disp_drv_t* disp, const lv_area_t* area, lv_color_t* color_p) {
    panel->getLcd()->drawBitmap(area->x1, area->y1, area->x2 + 1, area->y2 + 1, color_p);
    lv_disp_flush_ready(disp);
}
#else
void lvgl_port_disp_flush(lv_disp_drv_t* disp, const lv_area_t* area, lv_color_t* color_p) {
    panel->getLcd()->drawBitmap(area->x1, area->y1, area->x2 + 1, area->y2 + 1, color_p);
    lv_disp_flush_ready(disp);  // Always call synchronously
}

bool notify_lvgl_flush_ready(void* user_ctx) {
    // No longer needed with synchronous flush
    return false;
}
#endif

#if ESP_PANEL_USE_LCD_TOUCH
void lvgl_port_tp_read(lv_indev_drv_t* indev, lv_indev_data_t* data) {
    panel->getLcdTouch()->readData();

    bool touched = panel->getLcdTouch()->getTouchState();
    if (!touched) {
        data->state = LV_INDEV_STATE_REL;
        return;
    }

    TouchPoint point = panel->getLcdTouch()->getPoint();
    data->state = LV_INDEV_STATE_PR;
    data->point.x = point.x;
    data->point.y = point.y;
}
#endif

void lvgl_port_lock(int timeout_ms) {
    // Guard: if mutex not yet created, don't crash - just return
    if (!lvgl_mux) {
        return;
    }
    const TickType_t timeout_ticks = (timeout_ms < 0) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    xSemaphoreTakeRecursive(lvgl_mux, timeout_ticks);
}

void lvgl_port_unlock() {
    // Guard: if mutex not yet created, don't crash
    if (!lvgl_mux) {
        return;
    }
    xSemaphoreGiveRecursive(lvgl_mux);
}

void lvgl_port_task(void* arg) {
    Serial.println("[LVGL] Task started");

    uint32_t task_delay_ms = LVGL_TASK_MAX_DELAY_MS;
    while (true) {
        lvgl_port_lock(-1);
        task_delay_ms = lv_timer_handler();
        lvgl_port_unlock();

        if (task_delay_ms > LVGL_TASK_MAX_DELAY_MS) {
            task_delay_ms = LVGL_TASK_MAX_DELAY_MS;
        } else if (task_delay_ms < LVGL_TASK_MIN_DELAY_MS) {
            task_delay_ms = LVGL_TASK_MIN_DELAY_MS;
        }

        vTaskDelay(pdMS_TO_TICKS(task_delay_ms));
    }
}

void setup() {
    Serial.begin(115200);
    Serial.flush();
    delay(500);  // Allow serial to stabilize - increased delay
    
    // CRITICAL: Print immediately to confirm setup() is reached
    Serial.println("\n\n\n*** SETUP() STARTED ***");
    Serial.flush();
    
    // Print reset reason for diagnostics (brownout, WDT, panic, etc.)
    esp_reset_reason_t reset_reason = esp_reset_reason();
    Serial.println();
    Serial.println("=================================");
    Serial.println(" Bronco Controls - Web Config ");
    Serial.println("=================================");
    Serial.printf(" Firmware Version: %s\n", APP_VERSION);
    Serial.print(" Reset Reason: ");
    switch (reset_reason) {
        case ESP_RST_POWERON:   Serial.println("Power-on"); break;
        case ESP_RST_SW:        Serial.println("Software reset"); break;
        case ESP_RST_PANIC:     Serial.println("Exception/panic"); break;
        case ESP_RST_INT_WDT:   Serial.println("Interrupt watchdog"); break;
        case ESP_RST_TASK_WDT:  Serial.println("Task watchdog"); break;
        case ESP_RST_WDT:       Serial.println("Other watchdog"); break;
        case ESP_RST_DEEPSLEEP: Serial.println("Deep sleep"); break;
        case ESP_RST_BROWNOUT:  Serial.println("Brownout (power issue!)"); break;
        case ESP_RST_SDIO:      Serial.println("SDIO"); break;
        default:                Serial.printf("Unknown (%d)\n", reset_reason); break;
    }
    
    // Print memory status
    Serial.printf(" Free Heap: %lu bytes\n", esp_get_free_heap_size());
    Serial.printf(" Free PSRAM: %lu bytes\n", heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
    Serial.printf(" Total PSRAM: %lu bytes\n", heap_caps_get_total_size(MALLOC_CAP_SPIRAM));
    
    // CRITICAL: Mark OTA partition as valid IMMEDIATELY to prevent rollback
    const esp_partition_t* running = esp_ota_get_running_partition();
    esp_ota_img_states_t ota_state;
    if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK) {
        if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
            Serial.println("[OTA] New firmware verified - marking partition as valid");
            esp_ota_mark_app_valid_cancel_rollback();
        }
    }

    const PanelConfig &panelConfig = SelectPanelConfig();
    Serial.printf(" Panel Variant: %s\n", panelConfig.name);
    Serial.println("=================================");

    // Initialize LVGL core
    lv_init();
    
    // CREATE LVGL MUTEX IMMEDIATELY after lv_init() before any lvgl_port_lock() calls
    // This prevents undefined behavior if lock is called before mutex exists
    lvgl_mux = xSemaphoreCreateRecursiveMutex();
    if (!lvgl_mux) {
        Serial.println("[ERROR] Failed to create LVGL mutex!");
        while (true) delay(1000);
    }

    // Create display panel - library will handle all I2C initialization
    Serial.println("[PANEL] Creating ESP_Panel object...");
    panel = new ESP_Panel();

    // LVGL draw buffers - DOUBLE BUFFER in PSRAM for safety
    Serial.println("[LVGL] Allocating double buffers in PSRAM...");
    static lv_disp_draw_buf_t draw_buf;
    const uint32_t buf_sz = ESP_PANEL_LCD_H_RES * 40;
    const uint32_t buf_bytes = buf_sz * sizeof(lv_color_t);
    
    uint8_t* buf1 = static_cast<uint8_t*>(heap_caps_calloc(1, buf_bytes, MALLOC_CAP_SPIRAM));
    if (!buf1) {
        Serial.printf("[ERROR] Unable to allocate LVGL buffer 1 (%lu bytes)\n", buf_bytes);
        Serial.printf("[ERROR] Free PSRAM: %lu bytes\n", heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
        while (true) delay(1000);
    }
    uint8_t* buf2 = static_cast<uint8_t*>(heap_caps_calloc(1, buf_bytes, MALLOC_CAP_SPIRAM));
    if (!buf2) {
        Serial.printf("[ERROR] Unable to allocate LVGL buffer 2 (%lu bytes)\n", buf_bytes);
        Serial.printf("[ERROR] Free PSRAM: %lu bytes\n", heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
        while (true) delay(1000);
    }
    lv_disp_draw_buf_init(&draw_buf, buf1, buf2, buf_sz);
    Serial.printf("[LVGL] ✓ Allocated 2x %lu byte buffers in PSRAM\n", buf_bytes);
    Serial.printf("[LVGL] Free PSRAM after allocation: %lu bytes\n", heap_caps_get_free_size(MALLOC_CAP_SPIRAM));

    // Register display driver
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = panelConfig.width;
    disp_drv.ver_res = panelConfig.height;
    disp_drv.flush_cb = lvgl_port_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

#if ESP_PANEL_USE_LCD_TOUCH
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = lvgl_port_tp_read;
    lv_indev_drv_register(&indev_drv);
#endif

    Serial.println("[Boot] Calling panel->init()...");
    panel->init();
    Serial.println("[Boot] ✓ panel->init() completed");
    
    Serial.println("[Boot] Calling panel->begin()...");
    panel->begin();
    Serial.println("[Boot] ✓ panel->begin() completed");
    
    // NOW create and configure expander AFTER panel owns I2C
    Serial.println("[EXPANDER] Creating CH422G expander...");
    g_expander = new ESP_IOExpander_CH422G(HW_I2C_BUS_NUM, ESP_IO_EXPANDER_I2C_CH422G_ADDRESS_000);
    if (g_expander) {
        g_expander->init();
        g_expander->begin();
        
        // Configure expander pins using hardware_config.h constants
        g_expander->multiPinMode(HW_CH422G_SAFE_MASK, OUTPUT);
        g_expander->multiDigitalWrite(HW_CH422G_SAFE_MASK, HIGH);
        Serial.println("[EXPANDER] ✓ CAN mux set to USB_SEL HIGH");
    } else {
        Serial.println("[EXPANDER] ✗ Failed to create expander");
    }
    
    // SAFE BOOT CHECK: Hold top-left corner during boot to factory reset
    Serial.println("\n[SAFE BOOT] Checking for factory reset request (hold top-left)...");
    delay(500);  // Give touch controller time to stabilize
    
    for (int i = 0; i < 30; i++) {  // Check for 3 seconds
        if (detect_safe_boot()) {
            Serial.printf("[SAFE BOOT] Detected! Hold for %d more...\n", 30 - i);
            g_safe_boot_requested = true;
        } else if (g_safe_boot_requested) {
            // Released early
            g_safe_boot_requested = false;
            Serial.println("[SAFE BOOT] Released - cancelled");
            break;
        }
        delay(100);
    }
    
    if (g_safe_boot_requested) {
        factory_reset();  // This will reboot
    }
    Serial.println("[SAFE BOOT] Normal boot\n");

    // Create CAN message queue
    g_can_queue = xQueueCreate(CAN_QUEUE_SIZE, sizeof(CanFrame));
    if (!g_can_queue) {
        Serial.println("[ERROR] Failed to create CAN queue!");
    }

    // Start mux watchdog to keep USB_SEL HIGH
    xTaskCreatePinnedToCore(mux_watchdog_task, "mux_wd", 4096, nullptr, 1, nullptr, 1);
    Serial.println("[WATCHDOG] ✓ Mux watchdog started");
    
    // Start suspension TX task on core 1 (300ms cadence)
    xTaskCreatePinnedToCore(suspension_tx_task, "susp_tx", 4096, nullptr, 2, nullptr, 1);
    Serial.println("[SUSPENSION] ✓ Suspension TX task started (300ms)");
    
    // Start CAN RX task on core 1 (separate from UI)
    xTaskCreatePinnedToCore(can_rx_task, "can_rx", 4096, nullptr, 3, nullptr, 1);
    Serial.println("[CAN-TASK] ✓ CAN RX task started on core 1");
    
    // Start health monitor
    xTaskCreatePinnedToCore(health_monitor_task, "health", 3072, nullptr, 1, nullptr, 1);
    Serial.println("[HEALTH] ✓ Health monitor started");

    // === CAN INITIALIZATION ===
    Serial.println("\n[CAN] Initializing CAN bus...");
    CanManager::instance().setExpander(g_expander);
    CanManager::instance().begin();
    
    if (CanManager::instance().isReady()) {
        Serial.println("[CAN] ✓ TWAI driver initialized successfully!");
        Serial.printf("[CAN]   TX=GPIO%d, RX=GPIO%d\n", CanManager::instance().txPin(), CanManager::instance().rxPin());
    } else {
        Serial.println("[CAN] ✗ TWAI driver FAILED - CAN will not work");
    }
    Serial.println();

    // Enable backlight
    Serial.println("[Boot] Getting backlight...");
    auto *backlight = panel->getBacklight();
    if (backlight) {
        Serial.println("[Boot] ✓ Backlight found, turning on...");
        backlight->on();
        backlight->setBrightness(255);
        Serial.println("[Boot] ✓ Backlight enabled at 100%");
    } else {
        Serial.println("[Boot] ✗ ERROR: Backlight is NULL!");
    }

    // Start LVGL background task (note: lvgl_mux already created at top of setup())
    xTaskCreate(lvgl_port_task, "lvgl", LVGL_TASK_STACK_SIZE, nullptr, LVGL_TASK_PRIORITY, nullptr);

    // Boot-safe config bypass: if reset was due to panic/WDT, offer recovery
    bool skip_config = false;
    if (reset_reason == ESP_RST_PANIC || reset_reason == ESP_RST_INT_WDT || 
        reset_reason == ESP_RST_TASK_WDT || reset_reason == ESP_RST_WDT) {
        Serial.println("[BOOT] WARNING: Last reset was abnormal - config may be corrupted");
        Serial.println("[BOOT] To skip config loading, send 'safe' command in next 3 seconds...");
        uint32_t safe_start = millis();
        while (millis() - safe_start < 3000) {
            if (Serial.available()) {
                String cmd = Serial.readStringUntil('\n');
                cmd.trim();
                if (cmd == "safe") {
                    skip_config = true;
                    Serial.println("[BOOT] SAFE MODE: Skipping config load, using defaults");
                    break;
                }
            }
            delay(100);
        }
    }

    // Load configuration from flash (unless safe mode)
    if (skip_config) {
        Serial.println("[Config] Safe mode - not loading config from flash");
    } else if (!ConfigManager::instance().begin()) {
        Serial.println("[Config] Failed to mount LittleFS; factory defaults applied.");
    }

    if (!Ipm1CanSystem::instance().begin()) {
        Serial.println("[IPM1] Failed to load system JSON; default system retained");
    }

    // Auto-detect firmware version if it differs from APP_VERSION (after OTA update)
    auto& config = ConfigManager::instance().getConfig();
    if (config.version != APP_VERSION) {
        Serial.printf("[Boot] Firmware version changed: %s -> %s\n", config.version.c_str(), APP_VERSION);
        config.version = APP_VERSION;
        ConfigManager::instance().save();
        Serial.println("[Boot] Version updated and saved");
    }

    // Build the themed UI once before networking spins up
    lvgl_port_lock(-1);
    UITheme::init();
    UIBuilder::instance().begin();
    UIBuilder::instance().applyConfig(ConfigManager::instance().getConfig());
    lvgl_port_unlock();

    // Launch WiFi access point + web server
    WebServerManager::instance().begin();
    OTAUpdateManager::instance().begin();

    // Initialize Behavioral Output System first (provides behavior engine + CAN frame synthesis)
    Serial.println("[BEHAVIORAL] Initializing behavioral output control framework...");
    initBehavioralOutputSystem(&WebServerManager::instance().getServer());
    Serial.println("[BEHAVIORAL] ✓ Behavioral output system ready");

    // Initialize Infinitybox control system with behavior engine linkage
    Serial.println("[IBOX] Initializing Infinitybox IPM1 control system...");
    if (InfinityboxControl::InfinityboxController::instance().begin(&Ipm1CanSystem::instance(), &behaviorEngine)) {
        Serial.println("[IBOX] ✓ Infinitybox system ready");
    } else {
        Serial.println("[IBOX] ✗ Failed to initialize Infinitybox system");
    }

    Serial.println("=================================" );
    Serial.println(" Touch the screen or open http://192.168.4.250 ");
    Serial.println(" Behavioral UI: http://192.168.4.250/behavioral ");
    Serial.println("=================================");
}

void loop() {
    static uint32_t last_network_push_ms = 0;
    static uint32_t ap_start_ms = 0;
    static bool ap_shutdown_complete = false;
    static std::string last_ota_status_pushed;
    static uint32_t canmon_start_ms = 0;
    static int canmon_count = 0;
    static bool canmon_active = false;
    static uint32_t last_can_stats_ms = 0;
    static uint32_t can_frames_processed = 0;
    
    // Serial command handler for brightness testing
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();
        
        if (cmd.startsWith("b ") || cmd.startsWith("brightness ")) {
            int value = cmd.substring(cmd.indexOf(' ') + 1).toInt();
            if (value >= 0 && value <= 100) {
                lvgl_port_lock(-1);
                UIBuilder::instance().setBrightness(static_cast<uint8_t>(value));
                lvgl_port_unlock();
                Serial.printf("[CMD] Brightness set to %d%%\n", value);
            } else {
                Serial.println("[CMD] Usage: b <0-100> or brightness <0-100>");
            }
        } else if (cmd == "blinfo") {
            Serial.println("\n=== Backlight Info ===");
#ifdef ESP_PANEL_LCD_IO_BL
            Serial.printf("ESP_PANEL_LCD_IO_BL = %d\n", (int)ESP_PANEL_LCD_IO_BL);
#endif
#ifdef ESP_PANEL_LCD_BL_USE_PWM
            Serial.printf("ESP_PANEL_LCD_BL_USE_PWM = %d\n", (int)ESP_PANEL_LCD_BL_USE_PWM);
#endif
#ifdef ESP_PANEL_LCD_BL_PWM_FREQ_HZ
            Serial.printf("ESP_PANEL_LCD_BL_PWM_FREQ_HZ = %d\n", (int)ESP_PANEL_LCD_BL_PWM_FREQ_HZ);
#endif
#ifdef ESP_PANEL_LCD_SPI_IO_CS
            Serial.printf("ESP_PANEL_LCD_SPI_IO_CS = %d\n", (int)ESP_PANEL_LCD_SPI_IO_CS);
#endif
#ifdef ESP_PANEL_LCD_SPI_IO_MOSI
            Serial.printf("ESP_PANEL_LCD_SPI_IO_MOSI = %d\n", (int)ESP_PANEL_LCD_SPI_IO_MOSI);
#endif
            if (panel && panel->getBacklight()) {
                Serial.println("Backlight object: present");
            } else {
                Serial.println("Backlight object: NOT available");
            }
            Serial.println("Tip: if BL pin overlaps LCD pins, PWM changes can garble the display.");
            Serial.println("======================\n");
        } else if (cmd.startsWith("btest")) {
            Serial.println("[CMD] Brightness test: 100 -> 0 -> 100");
            for (int value = 100; value >= 0; value -= 10) {
                lvgl_port_lock(-1);
                UIBuilder::instance().setBrightness(static_cast<uint8_t>(value));
                lvgl_port_unlock();
                Serial.printf("[CMD] b=%d%%\n", value);
                delay(400);
            }
            for (int value = 0; value <= 100; value += 10) {
                lvgl_port_lock(-1);
                UIBuilder::instance().setBrightness(static_cast<uint8_t>(value));
                lvgl_port_unlock();
                Serial.printf("[CMD] b=%d%%\n", value);
                delay(400);
            }
        } else if (cmd.startsWith("canpoll ")) {
            // Poll Powercell module: canpoll <address> (non-blocking)
            int address = cmd.substring(8).toInt();
            if (address >= 1 && address <= 16) {
                Serial.printf("[CAN] Sending poll to address %d (check with canmon for response)\n", address);
                
                // Build polling CAN ID: FF4X with source address 0x63
                uint32_t pgn = 0xFF40 + address;
                CanFrameConfig frame;
                frame.enabled = true;
                frame.pgn = pgn;
                frame.priority = 6;
                frame.source_address = 0x63;
                frame.destination_address = 0xFF;
                frame.data = {0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
                
                if (CanManager::instance().sendFrame(frame)) {
                    Serial.println("[CAN] ✓ Poll sent");
                } else {
                    Serial.println("[CAN] ✗ Failed to send poll");
                }
            } else {
                Serial.println("[CMD] Usage: canpoll <1-16>");
            }
        } else if (cmd == "canmon") {
            // Start non-blocking CAN monitoring
            if (!canmon_active) {
                canmon_active = true;
                canmon_start_ms = millis();
                canmon_count = 0;
                Serial.println("[CAN] *** Monitoring CAN bus for 10 seconds (non-blocking) ***");
            }
        } else if (cmd.startsWith("canconfig ")) {
            // Send configuration to Powercell module: canconfig <address>
            int address = cmd.substring(10).toInt();
            if (address >= 1 && address <= 16) {
                Serial.printf("[CAN] Configuring Powercell at address %d\n", address);
                Serial.println("[CAN] Config: 250kb/s, 10s LOC timer, 250ms reporting, 200Hz PWM");
                
                // Build configuration CAN ID: FF4X with source address 0x63
                uint32_t pgn = 0xFF40 + (address == 16 ? 0 : address);
                CanFrameConfig frame;
                frame.enabled = true;
                frame.pgn = pgn;
                frame.priority = 6;
                frame.source_address = 0x63;
                frame.destination_address = 0xFF;
                // Configuration: 0x99 confirmation, 0x01 (250kb/s, 10s, 250ms, 200Hz), all outputs maintain state, config rev 0
                frame.data = {0x99, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
                
                if (CanManager::instance().sendFrame(frame)) {
                    Serial.println("[CAN] Configuration sent! Power cycle the Powercell to apply.");
                } else {
                    Serial.println("[CAN] Failed to send configuration");
                }
            } else {
                Serial.println("[CMD] Usage: canconfig <1-16>");
            }
        } else if (cmd.startsWith("cansend ")) {
            // Raw CAN send: cansend <pgn_hex> <byte0> <byte1> ...
            String params = cmd.substring(8);
            params.trim();
            int spaceIdx = params.indexOf(' ');
            if (spaceIdx > 0) {
                String pgnStr = params.substring(0, spaceIdx);
                uint32_t pgn = strtoul(pgnStr.c_str(), nullptr, 16);
                
                CanFrameConfig frame;
                frame.enabled = true;
                frame.pgn = pgn;
                frame.priority = 6;
                frame.source_address = 0x63;
                frame.destination_address = 0xFF;
                frame.data = {0, 0, 0, 0, 0, 0, 0, 0};
                
                String dataStr = params.substring(spaceIdx + 1);
                int idx = 0;
                int byteCount = 0;
                while (idx < dataStr.length() && byteCount < 8) {
                    while (idx < dataStr.length() && dataStr.charAt(idx) == ' ') idx++;
                    if (idx >= dataStr.length()) break;
                    
                    String byteStr = "";
                    while (idx < dataStr.length() && dataStr.charAt(idx) != ' ') {
                        byteStr += dataStr.charAt(idx++);
                    }
                    if (byteStr.length() > 0) {
                        frame.data[byteCount++] = strtoul(byteStr.c_str(), nullptr, 16);
                    }
                }
                
                Serial.printf("[CAN] Sending PGN 0x%04lX with %d bytes\n", pgn, byteCount);
                if (CanManager::instance().sendFrame(frame)) {
                    Serial.println("[CAN] Message sent successfully");
                } else {
                    Serial.println("[CAN] Failed to send message");
                }
            } else {
                Serial.println("[CMD] Usage: cansend <pgn_hex> <byte0_hex> <byte1_hex> ...");
                Serial.println("[CMD] Example: cansend FF41 11 00 00 00 00 00 00 00");
            }
        } else if (cmd == "canstatus") {
            Serial.println("\n=== CAN Bus Status ===");
            Serial.printf("CAN Ready: %s\n", CanManager::instance().isReady() ? "YES" : "NO");
            Serial.printf("TX Pin: GPIO%u\n", (unsigned)CanManager::instance().txPin());
            Serial.printf("RX Pin: GPIO%u\n", (unsigned)CanManager::instance().rxPin());
            Serial.println("Bitrate: 250 kbps");
            Serial.println("Mode: NO_ACK (for testing without termination)");
            Serial.println("======================\n");
        } else if (cmd.startsWith("canreinit ")) {
            // Reinitialize CAN with custom pins: canreinit <tx_pin> <rx_pin>
            String params = cmd.substring(9);
            params.trim();
            int spaceIdx = params.indexOf(' ');
            if (spaceIdx > 0) {
                int tx = params.substring(0, spaceIdx).toInt();
                int rx = params.substring(spaceIdx + 1).toInt();
                Serial.printf("[CAN] Reinit with TX=%d RX=%d at 250kbps...\n", tx, rx);
                CanManager::instance().stop();
                if (CanManager::instance().begin(static_cast<gpio_num_t>(tx), static_cast<gpio_num_t>(rx), 250000)) {
                    Serial.println("[CAN] Reinitialized successfully");
                } else {
                    Serial.println("[CAN] Reinit failed");
                }
            } else {
                Serial.println("[CMD] Usage: canreinit <tx_pin> <rx_pin>");
            }
        } else if (cmd == "otaoff") {
            g_disable_ota = true;
            Serial.println("[OTA] Auto-update disabled for testing");
        } else if (cmd == "otaon") {
            g_disable_ota = false;
            Serial.println("[OTA] Auto-update enabled");
        } else if (cmd.startsWith("ibox ")) {
            // Infinitybox control commands: ibox <function> <action> [params]
            String params = cmd.substring(5);
            params.trim();
            int spaceIdx = params.indexOf(' ');
            
            if (spaceIdx > 0) {
                String function = params.substring(0, spaceIdx);
                String action = params.substring(spaceIdx + 1);
                action.trim();
                
                // Convert underscores to spaces for function names
                function.replace('_', ' ');
                
                // Parse action
                if (action == "on") {
                    if (InfinityboxControl::InfinityboxController::instance().activateFunction(function.c_str(), true)) {
                        Serial.printf("[IBOX] ✓ %s ON\n", function.c_str());
                    } else {
                        Serial.printf("[IBOX] ✗ Failed to activate %s\n", function.c_str());
                    }
                } else if (action == "off") {
                    if (InfinityboxControl::InfinityboxController::instance().deactivateFunction(function.c_str())) {
                        Serial.printf("[IBOX] ✓ %s OFF\n", function.c_str());
                    } else {
                        Serial.printf("[IBOX] ✗ Failed to deactivate %s\n", function.c_str());
                    }
                } else if (action == "flash") {
                    if (InfinityboxControl::InfinityboxController::instance().activateFunctionFlash(function.c_str(), 500, 500, 0)) {
                        Serial.printf("[IBOX] ✓ %s FLASHING\n", function.c_str());
                    } else {
                        Serial.printf("[IBOX] ✗ Failed to flash %s\n", function.c_str());
                    }
                } else if (action.startsWith("fade ")) {
                    int level = action.substring(5).toInt();
                    if (level >= 0 && level <= 100) {
                        if (InfinityboxControl::InfinityboxController::instance().activateFunctionFade(function.c_str(), level, 1000)) {
                            Serial.printf("[IBOX] ✓ %s FADE to %d%%\n", function.c_str(), level);
                        } else {
                            Serial.printf("[IBOX] ✗ Failed to fade %s\n", function.c_str());
                        }
                    } else {
                        Serial.println("[CMD] Usage: ibox <function> fade <0-100>");
                    }
                } else {
                    Serial.println("[CMD] Actions: on, off, flash, fade <level>");
                }
            } else {
                Serial.println("[CMD] Usage: ibox <function> <action>");
                Serial.println("[CMD] Example: ibox headlights on");
                Serial.println("[CMD] Example: ibox left_turn_signal_front flash");
                Serial.println("[CMD] Example: ibox interior_lights fade 50");
            }
        } else if (cmd == "iboxlist") {
            auto names = InfinityboxControl::InfinityboxController::instance().getAllFunctionNames();
            Serial.printf("\n=== Infinitybox Functions (%d) ===\n", names.size());
            for (const auto& name : names) {
                const auto* func = InfinityboxControl::InfinityboxController::instance().getFunction(name);
                if (func) {
                    Serial.printf("  %s\n", name.c_str());
                    Serial.print("    Behaviors: ");
                    for (size_t i = 0; i < func->allowed_behaviors.size(); i++) {
                        Serial.print(InfinityboxControl::behaviorToString(func->allowed_behaviors[i]));
                        if (i < func->allowed_behaviors.size() - 1) Serial.print(", ");
                    }
                    Serial.println();
                    if (!func->requires.empty()) {
                        Serial.print("    Requires: ");
                        for (size_t i = 0; i < func->requires.size(); i++) {
                            Serial.print(func->requires[i].c_str());
                            if (i < func->requires.size() - 1) Serial.print(", ");
                        }
                        Serial.println();
                    }
                    if (!func->blocked_when.empty()) {
                        Serial.print("    Blocked when: ");
                        for (size_t i = 0; i < func->blocked_when.size(); i++) {
                            Serial.print(func->blocked_when[i].c_str());
                            if (i < func->blocked_when.size() - 1) Serial.print(", ");
                        }
                        Serial.println();
                    }
                }
            }
            Serial.println("================================\n");
        } else if (cmd == "iboxstatus") {
            InfinityboxControl::InfinityboxController::instance().printStatus();
        } else if (cmd == "security on") {
            InfinityboxControl::InfinityboxController::instance().setSecurityActive(true);
        } else if (cmd == "security off") {
            InfinityboxControl::InfinityboxController::instance().setSecurityActive(false);
        } else if (cmd == "ignition on") {
            InfinityboxControl::InfinityboxController::instance().setIgnitionOn(true);
            InfinityboxControl::InfinityboxController::instance().activateFunction("Ignition", true);
        } else if (cmd == "ignition off") {
            InfinityboxControl::InfinityboxController::instance().setIgnitionOn(false);
            InfinityboxControl::InfinityboxController::instance().deactivateFunction("Ignition");
        } else if (cmd == "otaoff") {
            g_disable_ota = true;
            Serial.println("[OTA] Auto-update disabled for testing");
        } else if (cmd == "otaon") {
            g_disable_ota = false;
            Serial.println("[OTA] Auto-update enabled");
        } else if (cmd == "help" || cmd == "?") {
            Serial.println("\n=== Serial Commands ===");
            Serial.println("BRIGHTNESS:");
            Serial.println("  b <0-100>        - Set brightness (e.g., 'b 50')");
            Serial.println("  brightness <0-100> - Set brightness");
            Serial.println("  blinfo           - Print backlight pin/PWM info");
            Serial.println("  btest            - Step brightness 100->0->100");
            Serial.println("CAN BUS (Powercell modules):");
            Serial.println("  canstatus        - Show CAN bus status");
            Serial.println("  canpoll <1-16>   - Poll Powercell at address");
            Serial.println("  canconfig <1-16> - Configure Powercell (default settings)");
            Serial.println("  canmon           - Monitor CAN bus for 10 seconds");
            Serial.println("  cansend <pgn> <data> - Send raw CAN frame");
            Serial.println("                     Example: cansend FF41 11 00 00 00 00 00 00 00");
            Serial.println("INFINITYBOX (IPM1 System):");
            Serial.println("  ibox <function> on|off|flash - Control function");
            Serial.println("  ibox <function> fade <0-100> - Fade to level");
            Serial.println("  iboxlist         - List all functions and behaviors");
            Serial.println("  iboxstatus       - Show active functions and state");
            Serial.println("  security on|off  - Enable/disable security interlock");
            Serial.println("  ignition on|off  - Turn ignition on/off");
            Serial.println("GENERAL:");
            Serial.println("  help or ?        - Show this help");
            Serial.println("======================\n");
        } else if (cmd.length() > 0) {
            Serial.printf("[CMD] Unknown command: '%s' (type 'help' for commands)\n", cmd.c_str());
        }
    }
    
    // ===== CAN QUEUE PROCESSING (non-blocking, from dedicated task) =====
    CanFrame frame;
    while (xQueueReceive(g_can_queue, &frame, 0) == pdTRUE) {
        can_frames_processed++;
        
        // Broadcast to WebSocket clients for CAN monitor
        CanRxMessage ws_msg;
        ws_msg.identifier = frame.id;
        ws_msg.length = frame.dlc;
        memcpy(ws_msg.data, frame.data, 8);
        ws_msg.timestamp = frame.timestamp_ms;
        WebServerManager::instance().broadcastCanFrame(ws_msg, false);
        
        // Only print if actively monitoring (not every frame!)
        if (canmon_active) {
            canmon_count++;
            Serial.printf("[CAN] #%d ID: 0x%08lX, DLC: %d, Data: ", canmon_count, frame.id, frame.dlc);
            for (uint8_t i = 0; i < frame.dlc; i++) {
                Serial.printf("%02X ", frame.data[i]);
            }
            Serial.println();
        }
        
        // TODO: Process CAN data and update UI model here
        // Do NOT call LVGL functions directly - set flags/state instead
    }
    
    // Print CAN stats periodically (not every frame)
    uint32_t now_ms = millis();
    if (now_ms - last_can_stats_ms >= 5000) {
        if (can_frames_processed > 0) {
            Serial.printf("[CAN-STATS] Processed %lu frames in last 5s\n", can_frames_processed);
        }
        can_frames_processed = 0;
        last_can_stats_ms = now_ms;
    }
    
    // Check if monitoring period ended
    if (canmon_active && (now_ms - canmon_start_ms >= 10000)) {
        Serial.printf("[CAN] *** Monitoring complete. Displayed %d messages. ***\n", canmon_count);
        canmon_active = false;
    }
    
    // Record AP start time on first loop
    if (ap_start_ms == 0) {
        ap_start_ms = millis();
    }
    
    // Shutdown AP after 90 seconds
    if (!ap_shutdown_complete && (millis() - ap_start_ms >= 90000)) {
        WebServerManager::instance().disableAP();
        ap_shutdown_complete = true;
        Serial.println("[WiFi] AP disabled after 90 seconds");
    }

    if (UIBuilder::instance().consumeDirtyFlag()) {
        lvgl_port_lock(-1);
        UIBuilder::instance().applyConfig(ConfigManager::instance().getConfig());
        lvgl_port_unlock();
    }

    const uint32_t now = millis();
    WifiStatusSnapshot snapshot = WebServerManager::instance().getStatusSnapshot();
    if (now - last_network_push_ms >= 1000) {
        std::string ap_ip = snapshot.ap_ip.toString().c_str();
        std::string sta_ip = snapshot.sta_ip.toString().c_str();
        lvgl_port_lock(-1);
        UIBuilder::instance().updateNetworkStatus(ap_ip, sta_ip, snapshot.sta_connected, snapshot.sta_ssid);
        lvgl_port_unlock();
        last_network_push_ms = now;
    }

    if (!g_disable_ota) {
        OTAUpdateManager& ota = OTAUpdateManager::instance();
        ota.loop(snapshot);

        const std::string& ota_status = ota.lastStatus();
        if (ota_status != last_ota_status_pushed) {
            lvgl_port_lock(-1);
            UIBuilder::instance().updateOtaStatus(ota_status);
            lvgl_port_unlock();
            last_ota_status_pushed = ota_status;
        }
    }

    // Update Infinitybox behavior engines (flash, fade, timed)
    InfinityboxControl::InfinityboxController::instance().loop();

    // Update Behavioral Output System
    updateBehavioralOutputSystem();

    WebServerManager::instance().loop();
    vTaskDelay(pdMS_TO_TICKS(50));
}
