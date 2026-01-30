#include "can_manager.h"

#include <driver/twai.h>
#include <freertos/FreeRTOS.h>
#include <Wire.h>

#include <algorithm>

// CH422G I2C configuration for CAN transceiver power
// NOTE: CH422G uses REGISTER addresses as I2C device addresses (unique protocol)
#define CH422G_REG_WR_IO    0x38  // Output control register I2C address
#define CH422G_USB_SEL_HIGH 0x2A  // USB_SEL (bit 5) = HIGH, enables CAN transceiver
#define I2C_SDA_PIN         8
#define I2C_SCL_PIN         9

CanManager& CanManager::instance() {
    static CanManager manager;
    return manager;
}

bool CanManager::begin(gpio_num_t tx_pin, gpio_num_t rx_pin, std::uint32_t bitrate) {
    tx_pin_ = tx_pin;
    rx_pin_ = rx_pin;
    bitrate_ = bitrate;

    Serial.printf("[CanManager] Initializing TWAI on TX=GPIO%d, RX=GPIO%d, Bitrate=%lu\n", tx_pin_, rx_pin_, bitrate_);

    // CRITICAL: Enable CAN transceiver via CH422G I2C expander BEFORE starting TWAI
    // The SN65HVD230 CAN transceiver power is controlled by USB_SEL pin on CH422G
    // Without this, GPIO19 RX will not receive any CAN messages
    // NOTE: CH422G has unique I2C protocol - register address IS the I2C device address
    // I2C is already initialized by panel library - just write directly
    Serial.println("[CanManager] Enabling CAN transceiver via CH422G...");
    
    // Write to CH422G register 0x38 (WR_IO) to set USB_SEL HIGH
    // beginTransmission takes the REGISTER address, not a device address!
    Wire.beginTransmission(CH422G_REG_WR_IO);  // 0x38, not 0x24!
    Wire.write(CH422G_USB_SEL_HIGH);            // 0x2A
    int i2c_result = Wire.endTransmission();
    
    if (i2c_result == 0) {
        Serial.println("[CanManager] ✓ CAN transceiver enabled (USB_SEL=HIGH)");
    } else {
        Serial.printf("[CanManager] ⚠ CH422G I2C write failed (err=%d) - CAN may not work\n", i2c_result);
    }
    
    delay(50); // Give transceiver time to power up

    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(tx_pin_, rx_pin_, TWAI_MODE_NORMAL);
    g_config.tx_queue_len = 8;
    g_config.rx_queue_len = 16;
    g_config.alerts_enabled = TWAI_ALERT_TX_SUCCESS | TWAI_ALERT_TX_FAILED | TWAI_ALERT_BUS_ERROR | TWAI_ALERT_BUS_OFF | TWAI_ALERT_ERR_PASS;

    if (bitrate_ != 250000) {
        Serial.println("[CanManager] Unsupported bitrate requested. Falling back to 250 kbps.");
    }

    const twai_timing_config_t t_config = TWAI_TIMING_CONFIG_250KBITS();
    const twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    if (twai_driver_install(&g_config, &t_config, &f_config) != ESP_OK) {
        Serial.println("[CanManager] Failed to install TWAI driver");
        ready_ = false;
        return false;
    }

    if (twai_start() != ESP_OK) {
        Serial.println("[CanManager] Failed to start TWAI driver");
        ready_ = false;
        return false;
    }

    ready_ = true;
    Serial.println("[CanManager] TWAI bus ready at 250 kbps");
    return true;
}

void CanManager::stop() {
    if (!ready_) {
        return;
    }
    twai_stop();
    twai_driver_uninstall();
    ready_ = false;
    Serial.println("[CanManager] TWAI driver stopped");
}

bool CanManager::sendButtonAction(const ButtonConfig& button) {
    if (!button.can.enabled) {
        Serial.printf("[CanManager] Button '%s' has no CAN frame assigned\n", button.label.c_str());
        return false;
    }
    return sendFrame(button.can);
}

bool CanManager::sendButtonReleaseAction(const ButtonConfig& button) {
    if (!button.can_off.enabled) {
        Serial.printf("[CanManager] Button '%s' has no CAN OFF frame assigned\n", button.label.c_str());
        return false;
    }
    return sendFrame(button.can_off);
}

bool CanManager::sendFrame(const CanFrameConfig& frame) {
    if (!ready_) {
        Serial.println("[CanManager] TWAI bus not initialized");
        return false;
    }

    // Check for bus errors and recover if needed
    twai_status_info_t status;
    if (twai_get_status_info(&status) == ESP_OK) {
        if (status.state == TWAI_STATE_BUS_OFF) {
            Serial.println("[CanManager] Bus-off detected, initiating recovery");
            twai_initiate_recovery();
            vTaskDelay(pdMS_TO_TICKS(100));
        } else if (status.state == TWAI_STATE_RECOVERING) {
            Serial.println("[CanManager] Bus is recovering, waiting...");
            vTaskDelay(pdMS_TO_TICKS(50));
        }
    }

    twai_message_t message = {};
    message.identifier = buildIdentifier(frame);
    message.extd = 1;
    message.data_length_code = frame.length;  // Use actual data length, not frame.data.size()
    for (std::size_t i = 0; i < frame.length; ++i) {
        message.data[i] = frame.data[i];
    }

    // VERBOSE LOGGING: Show exactly what we're sending
    Serial.printf("[CanManager] TX Frame: ID=0x%08lX, Len=%d, Data=", 
                  static_cast<unsigned long>(message.identifier), message.data_length_code);
    for (std::size_t i = 0; i < message.data_length_code; ++i) {
        Serial.printf("%02X ", message.data[i]);
    }
    Serial.println();
    Serial.printf("[CanManager]   PGN=0x%05lX, Pri=%u, SA=0x%02X, DA=0x%02X\n",
                  static_cast<unsigned long>(frame.pgn), frame.priority, 
                  frame.source_address, frame.destination_address);

    esp_err_t result = twai_transmit(&message, pdMS_TO_TICKS(50));
    if (result != ESP_OK) {
        Serial.printf("[CanManager] ✗ TX FAILED (err=%d)\n", static_cast<int>(result));
        // Log the bus state for debugging
        if (twai_get_status_info(&status) == ESP_OK) {
            Serial.printf("[CanManager]   Bus state: %d, TX errors: %lu, RX errors: %lu\n",
                         status.state, status.tx_error_counter, status.rx_error_counter);
        }
        return false;
    }

    Serial.println("[CanManager] ✓ TX SUCCESS");
    return true;
}

std::uint32_t CanManager::buildIdentifier(const CanFrameConfig& frame) const {
    const std::uint8_t priority = frame.priority & 0x7;
    const std::uint8_t data_page = (frame.pgn >> 16) & 0x01;
    const std::uint8_t pdu_format = (frame.pgn >> 8) & 0xFF;
    std::uint8_t pdu_specific = frame.pgn & 0xFF;

    if (pdu_format < 240) {
        // PDU1 - destination specific
        pdu_specific = frame.destination_address;
    }

    return (static_cast<std::uint32_t>(priority) << 26) |
           (static_cast<std::uint32_t>(0) << 25) |
           (static_cast<std::uint32_t>(data_page) << 24) |
           (static_cast<std::uint32_t>(pdu_format) << 16) |
           (static_cast<std::uint32_t>(pdu_specific) << 8) |
           (static_cast<std::uint32_t>(frame.source_address));
}

bool CanManager::receiveMessage(CanRxMessage& msg, uint32_t timeout_ms) {
    if (!ready_) {
        return false;
    }

    twai_message_t rx_msg;
    esp_err_t result = twai_receive(&rx_msg, pdMS_TO_TICKS(timeout_ms));
    
    if (result != ESP_OK) {
        return false;
    }

    msg.identifier = rx_msg.identifier;
    msg.length = rx_msg.data_length_code;
    msg.timestamp = millis();
    
    for (uint8_t i = 0; i < msg.length && i < 8; i++) {
        msg.data[i] = rx_msg.data[i];
    }

    return true;
}

std::vector<CanRxMessage> CanManager::receiveAll(uint32_t timeout_ms) {
    std::vector<CanRxMessage> messages;
    
    uint32_t start_time = millis();
    while (millis() - start_time < timeout_ms) {
        CanRxMessage msg;
        if (receiveMessage(msg, 10)) {
            messages.push_back(msg);
        } else {
            break;
        }
    }
    
    return messages;
}

// Helper for J1939 PGN transmission (non-blocking, no ACK wait)
bool CanManager::sendJ1939Pgn(uint8_t priority, uint32_t pgn, uint8_t source_addr, const uint8_t data[8]) {
    if (!ready_) {
        Serial.println("[CanManager] TWAI not ready");
        return false;
    }

    // Build J1939 29-bit identifier: [Priority(3) | Reserved(1) | DataPage(1) | PDU Format(8) | PDU Specific(8) | Source Address(8)]
    uint32_t identifier = ((uint32_t)(priority & 0x7) << 26) | ((pgn & 0x3FFFF) << 8) | source_addr;

    twai_message_t msg = {};
    msg.identifier = identifier;
    msg.extd = 1;  // Extended 29-bit ID
    msg.data_length_code = 8;
    memcpy(msg.data, data, 8);

    // Non-blocking transmit with 50ms timeout
    esp_err_t result = twai_transmit(&msg, pdMS_TO_TICKS(50));
    if (result != ESP_OK) {
        Serial.printf("[CanManager] TX queue fail: %s\n", esp_err_to_name(result));
        return false;
    }

    Serial.printf("[CanManager] TX PGN=0x%05lX data=%02X %02X %02X %02X %02X %02X %02X %02X\n",
                  (unsigned long)pgn, data[0], data[1], data[2], data[3], 
                  data[4], data[5], data[6], data[7]);
    return true;
}

// Background task for Infinitybox Output1 ON sequence
static void infinityboxOutput1OnTask(void* pvParameters) {
    const uint8_t SA_TOOL = 0x80;
    const uint32_t PGN_FF01 = 0x00FF01;
    const uint32_t PGN_FF02 = 0x00FF02;

    vTaskDelay(pdMS_TO_TICKS(100));  // Let web handler return first

    Serial.println("[Task] Infinitybox Output1 ON sequence starting");

    // Baseline FF02 00
    uint8_t ff02_00[8] = {0x00, 0, 0, 0, 0, 0, 0, 0};
    if (CanManager::instance().sendJ1939Pgn(6, PGN_FF02, SA_TOOL, ff02_00)) {
        vTaskDelay(pdMS_TO_TICKS(10));

        // FF01 A0 00
        uint8_t ff01_a0[8] = {0xA0, 0x00, 0, 0, 0, 0, 0, 0};
        if (CanManager::instance().sendJ1939Pgn(6, PGN_FF01, SA_TOOL, ff01_a0)) {
            vTaskDelay(pdMS_TO_TICKS(10));

            // FF02 80 00
            uint8_t ff02_80[8] = {0x80, 0x00, 0, 0, 0, 0, 0, 0};
            if (CanManager::instance().sendJ1939Pgn(6, PGN_FF02, SA_TOOL, ff02_80)) {
                vTaskDelay(pdMS_TO_TICKS(10));

                // FF01 20 00
                uint8_t ff01_20[8] = {0x20, 0x00, 0, 0, 0, 0, 0, 0};
                if (CanManager::instance().sendJ1939Pgn(6, PGN_FF01, SA_TOOL, ff01_20)) {
                    vTaskDelay(pdMS_TO_TICKS(10));

                    // FF02 back to 00
                    CanManager::instance().sendJ1939Pgn(6, PGN_FF02, SA_TOOL, ff02_00);
                }
            }
        }
    }

    Serial.println("[Task] Infinitybox Output1 ON sequence complete");
    vTaskDelete(NULL);
}

// Non-blocking wrapper - starts background task
bool CanManager::sendInfinityboxOutput1On() {
    if (!ready_) {
        Serial.println("[CanManager] TWAI not ready");
        return false;
    }
    if (xTaskCreate(infinityboxOutput1OnTask, "Inf1On", 2048, NULL, 1, NULL) == pdTRUE) {
        Serial.println("[CanManager] Started Output1 ON background task");
        return true;
    }
    return false;
}

// Background task for Infinitybox Output1 OFF sequence
static void infinityboxOutput1OffTask(void* pvParameters) {
    const uint8_t SA_TOOL = 0x80;
    const uint32_t PGN_FF01 = 0x00FF01;
    const uint32_t PGN_FF02 = 0x00FF02;

    Serial.println("[Task] Infinitybox Output1 OFF sequence starting");

    uint8_t ff02_00[8] = {0x00, 0, 0, 0, 0, 0, 0, 0};
    uint8_t ff01_20[8] = {0x20, 0x00, 0, 0, 0, 0, 0, 0};

    CanManager::instance().sendJ1939Pgn(6, PGN_FF02, SA_TOOL, ff02_00);
    vTaskDelay(pdMS_TO_TICKS(10));
    CanManager::instance().sendJ1939Pgn(6, PGN_FF01, SA_TOOL, ff01_20);
    vTaskDelay(pdMS_TO_TICKS(10));
    CanManager::instance().sendJ1939Pgn(6, PGN_FF02, SA_TOOL, ff02_00);

    Serial.println("[Task] Infinitybox Output1 OFF sequence complete");
    vTaskDelete(NULL);
}

// Non-blocking wrapper - starts background task
bool CanManager::sendInfinityboxOutput1Off() {
    if (!ready_) {
        Serial.println("[CanManager] TWAI not ready");
        return false;
    }
    if (xTaskCreate(infinityboxOutput1OffTask, "Inf1Off", 2048, NULL, 1, NULL) == pdTRUE) {
        Serial.println("[CanManager] Started Output1 OFF background task");
        return true;
    }
    return false;
}

// Background task for Infinitybox Output9 ON sequence
// EXACT 5-message sequence from working sketch
static void infinityboxOutput9OnTask(void* pvParameters) {
    const uint8_t SA_TOOL = 0x80;
    const uint32_t PGN_FF01 = 0x00FF01;
    const uint32_t PGN_FF02 = 0x00FF02;

    vTaskDelay(pdMS_TO_TICKS(100));  // Let web handler return first

    Serial.println("[Task] Infinitybox Output9 ON sequence starting (5 messages)");

    uint8_t ff02_00[8] = {0x00, 0, 0, 0, 0, 0, 0, 0};
    uint8_t ff02_80[8] = {0x80, 0x00, 0, 0, 0, 0, 0, 0};
    uint8_t ff01_2080[8] = {0x20, 0x80, 0, 0, 0, 0, 0, 0};

    // Message 1: FF02 00
    if (CanManager::instance().sendJ1939Pgn(6, PGN_FF02, SA_TOOL, ff02_00)) {
        vTaskDelay(pdMS_TO_TICKS(10));
        // Message 2: FF01 20 80
        if (CanManager::instance().sendJ1939Pgn(6, PGN_FF01, SA_TOOL, ff01_2080)) {
            vTaskDelay(pdMS_TO_TICKS(10));
            // Message 3: FF02 80
            if (CanManager::instance().sendJ1939Pgn(6, PGN_FF02, SA_TOOL, ff02_80)) {
                vTaskDelay(pdMS_TO_TICKS(10));
                // Message 4: FF01 20 80 (repeat)
                if (CanManager::instance().sendJ1939Pgn(6, PGN_FF01, SA_TOOL, ff01_2080)) {
                    vTaskDelay(pdMS_TO_TICKS(10));
                    // Message 5: FF02 00
                    CanManager::instance().sendJ1939Pgn(6, PGN_FF02, SA_TOOL, ff02_00);
                }
            }
        }
    }

    Serial.println("[Task] Infinitybox Output9 ON sequence complete");
    vTaskDelete(NULL);
}

// Non-blocking wrapper - starts background task
bool CanManager::sendInfinityboxOutput9On() {
    if (!ready_) {
        Serial.println("[CanManager] TWAI not ready");
        return false;
    }
    if (xTaskCreate(infinityboxOutput9OnTask, "Inf9On", 4096, NULL, 1, NULL) == pdTRUE) {
        Serial.println("[CanManager] Started Output9 ON background task");
        return true;
    }
    return false;
}

// Background task for Infinitybox Output9 OFF sequence
// EXACT 3-message sequence from working sketch
static void infinityboxOutput9OffTask(void* pvParameters) {
    const uint8_t SA_TOOL = 0x80;
    const uint32_t PGN_FF01 = 0x00FF01;
    const uint32_t PGN_FF02 = 0x00FF02;

    vTaskDelay(pdMS_TO_TICKS(100));  // Let web handler return first

    Serial.println("[Task] Infinitybox Output9 OFF sequence starting (3 messages)");

    uint8_t ff02_00[8] = {0x00, 0, 0, 0, 0, 0, 0, 0};
    uint8_t ff01_20[8] = {0x20, 0x00, 0, 0, 0, 0, 0, 0};

    // Message 1: FF02 00
    if (CanManager::instance().sendJ1939Pgn(6, PGN_FF02, SA_TOOL, ff02_00)) {
        vTaskDelay(pdMS_TO_TICKS(10));
        // Message 2: FF01 20 00
        if (CanManager::instance().sendJ1939Pgn(6, PGN_FF01, SA_TOOL, ff01_20)) {
            vTaskDelay(pdMS_TO_TICKS(10));
            // Message 3: FF02 00
            CanManager::instance().sendJ1939Pgn(6, PGN_FF02, SA_TOOL, ff02_00);
        }
    }

    Serial.println("[Task] Infinitybox Output9 OFF sequence complete");
    vTaskDelete(NULL);
}

// Non-blocking wrapper - starts background task
bool CanManager::sendInfinityboxOutput9Off() {
    if (!ready_) {
        Serial.println("[CanManager] TWAI not ready");
        return false;
    }
    if (xTaskCreate(infinityboxOutput9OffTask, "Inf9Off", 4096, NULL, 1, NULL) == pdTRUE) {
        Serial.println("[CanManager] Started Output9 OFF background task");
        return true;
    }
    return false;
}
