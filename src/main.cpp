/**
 * @file main.cpp
 * Bronco Controls - Web-configurable automotive HMI
 *
 * Boots the LVGL runtime, loads configuration from LittleFS,
 * and exposes a WiFi + web interface for live customization.
 * Version display and OTA update support.
 */

#include <Arduino.h>
#include <ESP_IOExpander_Library.h>
#include <ESP_Panel_Library.h>
#include <ESP_Panel_Conf.h>
#include <lvgl.h>
#include <WiFi.h>
#include <Wire.h>
#include <string>
#include <esp_ota_ops.h>

#include "can_manager.h"
#include "config_manager.h"
#include "ui_builder.h"
#include "ui_theme.h"
#include "web_server.h"
#include "ota_manager.h"
#include "version_auto.h"

// IO pin definitions for the Waveshare ESP32-S3-Touch-LCD-4.3 board
#define TP_RST 1
#define LCD_BL 2
#define LCD_RST 3
#define SD_CS 4
#define USB_SEL 5

// I2C definitions for the external IO expander (CH422G)
#define I2C_MASTER_NUM 1
#define I2C_MASTER_SDA_IO 8
#define I2C_MASTER_SCL_IO 9

// LVGL port configuration
#define LVGL_TICK_PERIOD_MS     (2)
#define LVGL_TASK_MAX_DELAY_MS  (500)
#define LVGL_TASK_MIN_DELAY_MS  (1)
#define LVGL_TASK_STACK_SIZE    (6 * 1024)
#define LVGL_TASK_PRIORITY      (2)
#define LVGL_BUF_SIZE           (ESP_PANEL_LCD_H_RES * 40)

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

// Forward declarations for LVGL helpers
void lvgl_port_lock(int timeout_ms);
void lvgl_port_unlock();

#if ESP_PANEL_LCD_BUS_TYPE == ESP_PANEL_BUS_TYPE_RGB
void lvgl_port_disp_flush(lv_disp_drv_t* disp, const lv_area_t* area, lv_color_t* color_p) {
    panel->getLcd()->drawBitmap(area->x1, area->y1, area->x2 + 1, area->y2 + 1, color_p);
    lv_disp_flush_ready(disp);
}
#else
void lvgl_port_disp_flush(lv_disp_drv_t* disp, const lv_area_t* area, lv_color_t* color_p) {
    panel->getLcd()->drawBitmap(area->x1, area->y1, area->x2 + 1, area->y2 + 1, color_p);
}

bool notify_lvgl_flush_ready(void* user_ctx) {
    lv_disp_drv_t* disp_driver = static_cast<lv_disp_drv_t*>(user_ctx);
    lv_disp_flush_ready(disp_driver);
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
    const TickType_t timeout_ticks = (timeout_ms < 0) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    xSemaphoreTakeRecursive(lvgl_mux, timeout_ticks);
}

void lvgl_port_unlock() {
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
    
    // CRITICAL: Mark OTA partition as valid IMMEDIATELY to prevent rollback
    // This must be the FIRST thing we do after Serial starts
    const esp_partition_t* running = esp_ota_get_running_partition();
    esp_ota_img_states_t ota_state;
    if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK) {
        if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
            Serial.println("[OTA] New firmware verified - marking partition as valid");
            esp_ota_mark_app_valid_cancel_rollback();
        }
    }
    
    Serial.println();
    Serial.println("=================================");
    Serial.println(" Bronco Controls - Web Config ");
    Serial.println("=================================");
    Serial.printf(" Firmware Version: %s\n", APP_VERSION);

    const PanelConfig &panelConfig = SelectPanelConfig();
    Serial.printf(" Panel Variant: %s\n", panelConfig.name);

    // Initialize LVGL core
    lv_init();

    // Initialize display panel
    panel = new ESP_Panel();

    // Configure IO expander - create and add to panel, let panel->init() handle full initialization
    ESP_IOExpander* expander = new ESP_IOExpander_CH422G(I2C_MASTER_NUM, ESP_IO_EXPANDER_I2C_CH422G_ADDRESS_000);
    expander->init();
    expander->begin();
    expander->multiPinMode(TP_RST | LCD_RST | SD_CS | USB_SEL, OUTPUT);
    expander->multiDigitalWrite(TP_RST | LCD_RST | SD_CS, HIGH);
    
    // CRITICAL: Set USB_SEL HIGH to enable CAN transceiver
    // Without this, the SN65HVD230 CAN transceiver remains unpowered/disabled
    // and GPIO19 RX will not receive any CAN messages
    expander->digitalWrite(USB_SEL, HIGH);
    Serial.println("[Boot] IO Expander configured (USB_SEL=HIGH for CAN transceiver)");
    panel->addIOExpander(expander);

    // LVGL draw buffers in PSRAM
    static lv_disp_draw_buf_t draw_buf;
    uint8_t* buf = static_cast<uint8_t*>(heap_caps_calloc(1, LVGL_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_SPIRAM));
    if (!buf) {
        Serial.println("[Error] Unable to allocate LVGL buffer in PSRAM");
        while (true) {
            delay(1000);
        }
    }
    lv_disp_draw_buf_init(&draw_buf, buf, nullptr, LVGL_BUF_SIZE);

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

    panel->init();
#if ESP_PANEL_LCD_BUS_TYPE != ESP_PANEL_BUS_TYPE_RGB
    panel->getLcd()->setCallback(notify_lvgl_flush_ready, &disp_drv);
#endif
    panel->begin();

    // CRITICAL: Re-ensure USB_SEL is HIGH after panel initialization
    // The ESP_IOExpander may have been reset during panel->init()
    // USB_SEL (bit 5) must be HIGH to power the CAN transceiver
    delay(50);
    if (expander) {
        expander->digitalWrite(USB_SEL, HIGH);
        Serial.println("[Boot] USB_SEL re-confirmed HIGH for CAN transceiver");
    }

    // === CAN INITIALIZATION (after I2C is ready) ===
    Serial.println("\n[CAN] Initializing CAN bus...");
    CanManager::instance().begin();
    
    if (CanManager::instance().isReady()) {
        Serial.println("[CAN] ✓ TWAI driver initialized successfully!");
        Serial.printf("[CAN]   TX=GPIO%d, RX=GPIO%d\n", CanManager::instance().txPin(), CanManager::instance().rxPin());
    } else {
        Serial.println("[CAN] ✗ TWAI driver FAILED - CAN will not work");
    }
    Serial.println();

    // Enable backlight
    auto *backlight = panel->getBacklight();
    if (backlight) {
        backlight->on();
        backlight->setBrightness(255);
    }

    // Start LVGL background task
    lvgl_mux = xSemaphoreCreateRecursiveMutex();
    xTaskCreate(lvgl_port_task, "lvgl", LVGL_TASK_STACK_SIZE, nullptr, LVGL_TASK_PRIORITY, nullptr);

    // Load configuration from flash
    if (!ConfigManager::instance().begin()) {
        Serial.println("[Config] Failed to mount LittleFS; factory defaults applied.");
    }

    // Auto-detect firmware version if it differs from APP_VERSION (after OTA update)
    auto& config = ConfigManager::instance().getConfig();
    if (config.version != APP_VERSION) {
        Serial.printf("[Boot] Firmware version changed: %s -> %s\n", config.version.c_str(), APP_VERSION);
        config.version = APP_VERSION;
        ConfigManager::instance().save();
        Serial.println("[Boot] Version updated and saved");
    }

    // CAN was already initialized before panel (see above)
    // Build the themed UI once before networking spins up
    lvgl_port_lock(-1);
    UITheme::init();
    UIBuilder::instance().begin();
    UIBuilder::instance().applyConfig(ConfigManager::instance().getConfig());
    lvgl_port_unlock();

    // Launch WiFi access point + web server
    WebServerManager::instance().begin();
    OTAUpdateManager::instance().begin();

    Serial.println("=================================");
    Serial.println(" Touch the screen or open http://192.168.4.250 ");
    Serial.println("=================================");
}

void loop() {
    static uint32_t last_network_push_ms = 0;
    static uint32_t ap_start_ms = 0;
    static bool ap_shutdown_complete = false;
    static std::string last_ota_status_pushed;
    
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
            // Poll POWERCELL NGX: canpoll <address>
            int address = cmd.substring(8).toInt();
            if (address >= 1 && address <= 16) {
                Serial.printf("[CAN] Polling POWERCELL NGX at address %d\n", address);
                
                // Build polling CAN ID: FF4X with source address 0x63
                uint32_t pgn = 0xFF40 + (address == 16 ? 0 : address);
                CanFrameConfig frame;
                frame.enabled = true;
                frame.pgn = pgn;
                frame.priority = 6;
                frame.source_address = 0x63;
                frame.destination_address = 0xFF;
                frame.data = {0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
                
                if (CanManager::instance().sendFrame(frame)) {
                    Serial.println("[CAN] Poll message sent - listening for response...");
                    
                    // Listen for response (FF5X)
                    uint32_t start = millis();
                    while (millis() - start < 1000) {
                        CanRxMessage msg;
                        if (CanManager::instance().receiveMessage(msg, 50)) {
                            Serial.printf("[CAN] RX ID: 0x%08lX, DLC: %d, Data: ", msg.identifier, msg.length);
                            for (uint8_t i = 0; i < msg.length; i++) {
                                Serial.printf("%02X ", msg.data[i]);
                            }
                            Serial.println();
                        }
                    }
                } else {
                    Serial.println("[CAN] Failed to send poll message");
                }
            } else {
                Serial.println("[CMD] Usage: canpoll <1-16>");
            }
        } else if (cmd == "canmon") {
            // Monitor CAN bus for 10 seconds
            Serial.println("[CAN] Monitoring CAN bus for 10 seconds...");
            uint32_t start = millis();
            int count = 0;
            while (millis() - start < 10000) {
                CanRxMessage msg;
                if (CanManager::instance().receiveMessage(msg, 100)) {
                    count++;
                    Serial.printf("[CAN] #%d ID: 0x%08lX, DLC: %d, Data: ", count, msg.identifier, msg.length);
                    for (uint8_t i = 0; i < msg.length; i++) {
                        Serial.printf("%02X ", msg.data[i]);
                    }
                    Serial.println();
                }
            }
            Serial.printf("[CAN] Monitoring complete. Received %d messages.\n", count);
        } else if (cmd.startsWith("canconfig ")) {
            // Send configuration to POWERCELL NGX: canconfig <address>
            int address = cmd.substring(10).toInt();
            if (address >= 1 && address <= 16) {
                Serial.printf("[CAN] Configuring POWERCELL NGX at address %d\n", address);
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
                    Serial.println("[CAN] Configuration sent! Power cycle the POWERCELL NGX to apply.");
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
        } else if (cmd == "help" || cmd == "?") {
            Serial.println("\n=== Serial Commands ===");
            Serial.println("BRIGHTNESS:");
            Serial.println("  b <0-100>        - Set brightness (e.g., 'b 50')");
            Serial.println("  brightness <0-100> - Set brightness");
            Serial.println("  blinfo           - Print backlight pin/PWM info");
            Serial.println("  btest            - Step brightness 100->0->100");
            Serial.println("CAN BUS (Infinitybox POWERCELL NGX):");
            Serial.println("  canstatus        - Show CAN bus status");
            Serial.println("  canpoll <1-16>   - Poll POWERCELL NGX at address");
            Serial.println("  canconfig <1-16> - Configure POWERCELL NGX (default settings)");
            Serial.println("  canmon           - Monitor CAN bus for 10 seconds");
            Serial.println("  cansend <pgn> <data> - Send raw CAN frame");
            Serial.println("                     Example: cansend FF41 11 00 00 00 00 00 00 00");
            Serial.println("GENERAL:");
            Serial.println("  help or ?        - Show this help");
            Serial.println("======================\n");
        } else if (cmd.length() > 0) {
            Serial.printf("[CMD] Unknown command: '%s' (type 'help' for commands)\n", cmd.c_str());
        }
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

    WebServerManager::instance().loop();
    vTaskDelay(pdMS_TO_TICKS(50));
}
