#include "can_manager.h"
#include "hardware_config.h"
#include "web_server.h"

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
    // Initialize suspension mutex on first access
    if (!manager.suspension_mutex_) {
        manager.suspension_mutex_ = xSemaphoreCreateMutex();
    }
    if (!manager.powercell_mutex_) {
        manager.powercell_mutex_ = xSemaphoreCreateMutex();
    }
    return manager;
}

namespace {

bool decodePowercellStatusPgn(uint32_t pgn, uint8_t& cellAddress, uint8_t& bankStart) {
    if (pgn >= 0xFF10 && pgn <= 0xFF1F) {
        bankStart = 1;
    } else if (pgn >= 0xFF20 && pgn <= 0xFF2F) {
        bankStart = 6;
    } else if (pgn >= 0xFF50 && pgn <= 0xFF5F) {
        bankStart = 1;
    } else if (pgn >= 0xFF60 && pgn <= 0xFF6F) {
        bankStart = 6;
    } else {
        return false;
    }

    cellAddress = static_cast<uint8_t>(pgn & 0x0F);
    if (cellAddress == 0) {
        cellAddress = 16;
    }

    return true;
}

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
    
    // Filter config: accept both EXT (Infinitybox J1939) and STD (Suspension 0x737/0x738)
    // ACCEPT_ALL allows both frame types - this is critical for dual-format bus operation
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
        Serial.printf("[CanManager]  TX FAILED (err=%d)\n", static_cast<int>(result));
        // Log the bus state for debugging
        if (twai_get_status_info(&status) == ESP_OK) {
            Serial.printf("[CanManager]   Bus state: %d, TX errors: %lu, RX errors: %lu\n",
                         status.state, status.tx_error_counter, status.rx_error_counter);
        }
        return false;
    }

    Serial.println("[CanManager] === TX SUCCESS");
    return true;
}

bool CanManager::sendStandardFrame(uint16_t identifier, const uint8_t data[8], uint8_t length) {
    if (!ready_) {
        Serial.println("[CanManager] TWAI bus not initialized");
        return false;
    }

    if (length == 0) {
        Serial.println("[CanManager] Standard TX length is 0");
        return false;
    }

    forceCanMux();

    twai_message_t msg = {};
    msg.identifier = identifier & 0x7FF;
    msg.extd = 0;
    msg.data_length_code = length > 8 ? 8 : length;
    memcpy(msg.data, data, msg.data_length_code);

    esp_err_t result = twai_transmit(&msg, pdMS_TO_TICKS(50));
    if (result != ESP_OK) {
        Serial.printf("[CanManager]  STD TX FAILED (err=%d)\n", static_cast<int>(result));
        return false;
    }

    CanRxMessage ws_msg;
    ws_msg.identifier = msg.identifier;
    ws_msg.length = msg.data_length_code;
    memcpy(ws_msg.data, msg.data, 8);
    ws_msg.timestamp = millis();
    WebServerManager::instance().broadcastCanFrame(ws_msg, true);

    Serial.printf("[CanManager]  STD TX ID=0x%03X Len=%d\n", msg.identifier, msg.data_length_code);
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

    // Re-assert CAN mux before TX (required on this hardware — same as sendFrame/sendStandardFrame)
    forceCanMux();

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

bool CanManager::updatePowercellStatusFromPgn(uint32_t pgn, const uint8_t data[8]) {
    uint8_t cellAddress = 0;
    uint8_t bankStart = 0;
    if (!decodePowercellStatusPgn(pgn, cellAddress, bankStart)) {
        return false;
    }

    if (cellAddress < 1 || cellAddress > kPowercellMaxAddress) {
        return false;
    }

    if (powercell_mutex_) {
        xSemaphoreTake(powercell_mutex_, portMAX_DELAY);
    }

    PowercellCellStatus& cell = powercell_status_[cellAddress - 1];
    cell.last_seen_ms = millis();
    cell.voltage_raw = data[6];
    cell.temperature_c = static_cast<int8_t>(data[7]);

    for (uint8_t i = 0; i < 5; ++i) {
        const uint8_t outputNumber = static_cast<uint8_t>(bankStart + i);
        if (outputNumber > kPowercellOutputsPerCell) {
            continue;
        }

        PowercellOutputState& out = cell.outputs[outputNumber - 1];
        out.valid = true;
        const uint8_t bitIndex = static_cast<uint8_t>(7 - i);
        out.on = ((data[0] >> bitIndex) & 0x01) != 0;
        out.current_raw = data[1 + i];
        out.last_seen_ms = cell.last_seen_ms;
    }

    if (powercell_mutex_) {
        xSemaphoreGive(powercell_mutex_);
    }

    return true;
}

PowercellOutputState CanManager::getPowercellOutputState(uint8_t cell_address, uint8_t output_number) const {
    PowercellOutputState result;
    if (cell_address < 1 || cell_address > kPowercellMaxAddress) {
        return result;
    }
    if (output_number < 1 || output_number > kPowercellOutputsPerCell) {
        return result;
    }

    if (powercell_mutex_) {
        xSemaphoreTake(powercell_mutex_, portMAX_DELAY);
    }

    result = powercell_status_[cell_address - 1].outputs[output_number - 1];

    if (powercell_mutex_) {
        xSemaphoreGive(powercell_mutex_);
    }

    return result;
}

PowercellCellTelemetry CanManager::getPowercellCellTelemetry(uint8_t cell_address) const {
    PowercellCellTelemetry result;
    if (cell_address < 1 || cell_address > kPowercellMaxAddress) {
        return result;
    }

    if (powercell_mutex_) {
        xSemaphoreTake(powercell_mutex_, portMAX_DELAY);
    }

    const PowercellCellStatus& cell = powercell_status_[cell_address - 1];
    result.valid = (cell.last_seen_ms > 0);
    result.voltage_raw = cell.voltage_raw;
    result.temperature_c = cell.temperature_c;
    result.last_seen_ms = cell.last_seen_ms;

    if (powercell_mutex_) {
        xSemaphoreGive(powercell_mutex_);
    }

    return result;
}

// ============================================================================
// SUSPENSION CONTROL (Separate from Infinitybox pipeline)
// ============================================================================

void CanManager::updateSuspensionState(const SuspensionState& state) {
    if (suspension_mutex_) {
        xSemaphoreTake(suspension_mutex_, portMAX_DELAY);
        suspension_state_ = state;
        xSemaphoreGive(suspension_mutex_);
    }
}

SuspensionState CanManager::getSuspensionState() const {
    SuspensionState state;
    if (suspension_mutex_) {
        xSemaphoreTake(suspension_mutex_, portMAX_DELAY);
        state = suspension_state_;
        xSemaphoreGive(suspension_mutex_);
    }
    return state;
}

SuspensionCANStats CanManager::getSuspensionStats() const {
    SuspensionCANStats stats;
    if (suspension_mutex_) {
        xSemaphoreTake(suspension_mutex_, portMAX_DELAY);
        stats = suspension_stats_;
        xSemaphoreGive(suspension_mutex_);
    }
    return stats;
}

void CanManager::setSuspensionMessagingEnabled(bool enabled) {
    if (suspension_mutex_) {
        xSemaphoreTake(suspension_mutex_, portMAX_DELAY);
        suspension_messaging_enabled_ = enabled;
        xSemaphoreGive(suspension_mutex_);
    } else {
        suspension_messaging_enabled_ = enabled;
    }
}

bool CanManager::isSuspensionMessagingEnabled() const {
    bool enabled = true;
    if (suspension_mutex_) {
        xSemaphoreTake(suspension_mutex_, portMAX_DELAY);
        enabled = suspension_messaging_enabled_;
        xSemaphoreGive(suspension_mutex_);
    } else {
        enabled = suspension_messaging_enabled_;
    }
    return enabled;
}

bool CanManager::sendSuspensionCommand() {
    if (!ready_) {
        Serial.println("[Suspension] CAN not ready");
        return false;
    }

    if (!isSuspensionMessagingEnabled()) {
        return true;
    }

    // Get current state
    SuspensionState state = getSuspensionState();

    // Hard guard: never transmit if power_on is false.
    // Sending byte7=0x00 (OFF) fights any other device on the bus that has the TCU enabled,
    // causing the TCU relay to toggle repeatedly (audible clicking).
    if (!state.power_on) {
        return true;
    }

    // Build 8-byte payload for 0x737 (TCU S15 protocol):
    // Byte 0-2: Reserved (must be 0x00)
    // Byte 3: Pitch damper setting (0x01-0x05), 0x00 = no change
    // Byte 4: Roll  damper setting (0x01-0x05), 0x00 = no change
    // Byte 5: Rear  damper setting (0x01-0x05), 0x00 = no change
    // Byte 6: Front damper setting (0x01-0x05), 0x00 = no change
    // Byte 7: Power ON=0x30, OFF=0x00
    uint8_t data[8] = {0};
    data[3] = state.pitch_setting;
    data[4] = state.roll_setting;
    data[5] = state.rear_setting;
    data[6] = state.front_setting;
    data[7] = state.power_on ? 0x30 : 0x00;

    // Build standard 11-bit CAN frame (0x737)
    twai_message_t msg = {};
    msg.identifier = 0x737;
    msg.extd = 0;  // Standard 11-bit ID
    msg.data_length_code = 8;
    memcpy(msg.data, data, 8);

    // Non-blocking transmit
    esp_err_t result = twai_transmit(&msg, pdMS_TO_TICKS(50));
    
    // Update stats
    if (suspension_mutex_) {
        xSemaphoreTake(suspension_mutex_, portMAX_DELAY);
        if (result == ESP_OK) {
            suspension_stats_.tx_count++;
            suspension_stats_.last_tx_ms = millis();
            memcpy(suspension_stats_.last_tx_data, data, 8);
        } else {
            suspension_stats_.tx_fail_count++;
        }
        xSemaphoreGive(suspension_mutex_);
    }

    if (result != ESP_OK) {
        Serial.printf("[Suspension] TX fail: %s\n", esp_err_to_name(result));
        return false;
    }

    // Broadcast TX frame to CAN monitor clients so suspension traffic is visible
    CanRxMessage ws_msg;
    ws_msg.identifier = msg.identifier;
    ws_msg.length = msg.data_length_code;
    memcpy(ws_msg.data, msg.data, 8);
    ws_msg.timestamp = millis();
    WebServerManager::instance().broadcastCanFrame(ws_msg, true);

    Serial.printf("[Suspension] TX 0x737: %02X %02X %02X %02X %02X %02X %02X %02X\n",
                  data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);
    return true;
}

void CanManager::parseSuspensionStatus(const uint8_t data[8]) {
    // Parse 0x738 status frame (TCU S15 protocol):
    // Byte 0: Front confirmed setting (0=S1, 1=S2, 2=S3, 3=S4, 4=S5)
    // Byte 1: Rear confirmed setting
    // Byte 2: Roll confirmed setting
    // Byte 3: Pitch confirmed setting
    // Byte 4: Error FR (0x02=short circuit, 0x08=open lead)
    // Byte 5: Error FL
    // Byte 6: Error RR
    // Byte 7: Error RL

    if (suspension_mutex_) {
        xSemaphoreTake(suspension_mutex_, portMAX_DELAY);

        suspension_state_.actual_front = data[0];
        suspension_state_.actual_rear  = data[1];
        suspension_state_.actual_roll  = data[2];
        suspension_state_.actual_pitch = data[3];
        suspension_state_.error_fr = data[4];
        suspension_state_.error_fl = data[5];
        suspension_state_.error_rr = data[6];
        suspension_state_.error_rl = data[7];
        suspension_state_.last_feedback_ms = millis();

        // If we are not actively commanding the suspension, sync our settings
        // to match what the TCU is actually doing (bidirectional awareness).
        // actual_* is 0-indexed (0=S1 .. 4=S5); settings are 1-indexed.
        if (!suspension_state_.power_on) {
            auto toSetting = [](uint8_t v) -> uint8_t { return (v <= 4) ? (v + 1) : 1; };
            suspension_state_.front_setting = toSetting(data[0]);
            suspension_state_.rear_setting  = toSetting(data[1]);
            suspension_state_.roll_setting  = toSetting(data[2]);
            suspension_state_.pitch_setting = toSetting(data[3]);
        }

        suspension_stats_.rx_count++;
        suspension_stats_.last_rx_ms = millis();
        memcpy(suspension_stats_.last_rx_data, data, 8);

        xSemaphoreGive(suspension_mutex_);
    }

    suspension_ui_dirty_.store(true);

    Serial.printf("[Suspension] RX 0x738: %02X %02X %02X %02X %02X %02X %02X %02X "
                  "(Front=S%d Rear=S%d Roll=S%d Pitch=S%d ErrFR=%02X ErrFL=%02X ErrRR=%02X ErrRL=%02X)\n",
                  data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7],
                  data[0]+1, data[1]+1, data[2]+1, data[3]+1,
                  data[4], data[5], data[6], data[7]);
}

void CanManager::parseSuspensionCommand(const uint8_t data[8]) {
    // Sniff 0x737 commands from other controllers (e.g. stock TCU head unit).
    // TX byte layout: D3=pitch(1-5), D4=roll(1-5), D5=rear(1-5), D6=front(1-5), D7=0x30(ON)/0x00(OFF)
    // Only update our state when we are not actively commanding (avoids conflict).
    if (!isSuspensionMessagingEnabled()) return;

    if (suspension_mutex_) {
        xSemaphoreTake(suspension_mutex_, portMAX_DELAY);
        if (!suspension_state_.power_on) {
            uint8_t f  = data[6];  // front (1-5)
            uint8_t r  = data[5];  // rear  (1-5)
            uint8_t ro = data[4];  // roll  (1-5)
            uint8_t p  = data[3];  // pitch (1-5)
            if (f  >= 1 && f  <= 5) suspension_state_.front_setting = f;
            if (r  >= 1 && r  <= 5) suspension_state_.rear_setting  = r;
            if (ro >= 1 && ro <= 5) suspension_state_.roll_setting  = ro;
            if (p  >= 1 && p  <= 5) suspension_state_.pitch_setting = p;
        }
        xSemaphoreGive(suspension_mutex_);
    }

    suspension_ui_dirty_.store(true);
    Serial.printf("[Suspension] Sniffed 0x737: Front=S%d Rear=S%d Roll=S%d Pitch=S%d Power=%s\n",
                  data[6], data[5], data[4], data[3], data[7] == 0x30 ? "ON" : "OFF");
}

