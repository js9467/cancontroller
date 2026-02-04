#include "can_manager.h"
#include "hardware_config.h"

#include <driver/twai.h>
#include <freertos/FreeRTOS.h>

#include <algorithm>

// No longer needed - expander handles mux
// Wire removed to prevent I2C conflict with panel library

// No longer needed - expander handles mux
// Wire removed to prevent I2C conflict with panel library

// MUX control now handled by g_expander in main.cpp
// This function is deprecated but kept for compatibility
static void force_can_mux_direct() {
    // No-op: mux is set by main.cpp after panel init
    // See force_can_mux_hardware() and mux_watchdog_task()
}

CanManager& CanManager::instance() {
    static CanManager manager;
    return manager;
}

void CanManager::forceCanMux() {
    force_can_mux_direct();
}

bool CanManager::begin(gpio_num_t tx_pin, gpio_num_t rx_pin, std::uint32_t bitrate) {
    tx_pin_ = tx_pin;
    rx_pin_ = rx_pin;
    bitrate_ = bitrate;

    Serial.printf("[CanManager] Initializing TWAI on TX=GPIO%d, RX=GPIO%d, Bitrate=%lu\\n", tx_pin_, rx_pin_, bitrate_);

    // CRITICAL: Assert CAN mux immediately before TWAI operations
    Serial.println("[CanManager] Forcing USB_SEL to CAN mode...");
    forceCanMux();

    // Start in LISTEN_ONLY to verify bus traffic before allowing TX
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(tx_pin_, rx_pin_, TWAI_MODE_LISTEN_ONLY);
    g_config.tx_queue_len = 8;
    g_config.rx_queue_len = 16;
    g_config.alerts_enabled = TWAI_ALERT_RX_DATA | TWAI_ALERT_BUS_ERROR | TWAI_ALERT_BUS_OFF | TWAI_ALERT_ERR_PASS;

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
        twai_driver_uninstall();  // Clean up on failure
        ready_ = false;
        return false;
    }

    Serial.println("[CanManager] TWAI started in LISTEN_ONLY mode, checking for bus traffic...");
    
    // Count RX frames for 1 second to verify bus is alive
    uint32_t rx_count = 0;
    uint32_t start_time = millis();
    while (millis() - start_time < 1000) {
        twai_message_t msg;
        if (twai_receive(&msg, pdMS_TO_TICKS(10)) == ESP_OK) {
            rx_count++;
        }
    }
    
    Serial.printf("[CanManager] Received %lu frames in 1 second\\n", rx_count);

    // Record whether we saw traffic, but ALWAYS switch to NORMAL so we can transmit
    bus_alive_ = (rx_count >= 3);
    if (bus_alive_) {
        Serial.println("[CanManager] Bus traffic detected - enabling NORMAL TX/RX mode");
    } else {
        Serial.println("[CanManager] WARNING: No bus traffic detected - enabling NORMAL mode anyway so this node can transmit");
    }

    // Switch to NORMAL mode for TX/RX
    twai_stop();
    twai_driver_uninstall();

    // CRITICAL: Re-assert CAN mux before reinstalling TWAI
    Serial.println("[CanManager] Re-asserting CAN mux before NORMAL mode...");
    forceCanMux();
    
    g_config.mode = TWAI_MODE_NORMAL;
    g_config.alerts_enabled = TWAI_ALERT_TX_SUCCESS | TWAI_ALERT_TX_FAILED | TWAI_ALERT_BUS_ERROR | TWAI_ALERT_BUS_OFF | TWAI_ALERT_ERR_PASS;
    
    if (twai_driver_install(&g_config, &t_config, &f_config) != ESP_OK) {
        Serial.println("[CanManager] Failed to reinstall TWAI in NORMAL mode");
        ready_ = false;
        return false;
    }
    
    if (twai_start() != ESP_OK) {
        Serial.println("[CanManager] Failed to restart TWAI in NORMAL mode");
        twai_driver_uninstall();  // Clean up on failure
        ready_ = false;
        return false;
    }

    ready_ = true;
    Serial.println("[CanManager] TWAI bus ready in NORMAL mode at 250 kbps");
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

    // If we haven't seen any traffic yet, warn but allow TX so this node
    // can be the first talker on the bus.
    if (!bus_alive_) {
        Serial.println("[CanManager] WARNING: TX while bus_alive_ == false (no startup traffic seen); attempting anyway");
    }

    // Re-assert CAN mux before TX (belt and suspenders)
    forceCanMux();

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
