#pragma once

#include <Arduino.h>
#include <hal/gpio_types.h>
#include <array>
#include <vector>

#include "config_types.h"

// Forward declaration
class ESP_IOExpander;

// Struct for received CAN messages (different from CanMessage in config_types.h)
struct CanRxMessage {
    uint32_t identifier;
    uint8_t data[8];
    uint8_t length;
    uint32_t timestamp;
};

// Suspension state management (single source of truth)
struct SuspensionState {
    bool power_on = false;
    uint8_t front_left_percent = 0;    // 0-100%
    uint8_t front_right_percent = 0;   // 0-100%
    uint8_t rear_left_percent = 0;     // 0-100%
    uint8_t rear_right_percent = 0;    // 0-100%
    bool calibration_active = false;
    
    // Actual state from 0x738 feedback
    uint8_t actual_fl_percent = 0;
    uint8_t actual_fr_percent = 0;
    uint8_t actual_rl_percent = 0;
    uint8_t actual_rr_percent = 0;
    uint8_t fault_flags = 0;
    uint32_t last_feedback_ms = 0;
};

// Suspension CAN diagnostics
struct SuspensionCANStats {
    uint32_t tx_count = 0;
    uint32_t tx_fail_count = 0;
    uint32_t rx_count = 0;
    uint32_t last_tx_ms = 0;
    uint32_t last_rx_ms = 0;
    uint8_t last_tx_data[8] = {0};
    uint8_t last_rx_data[8] = {0};
};

struct PowercellOutputState {
    bool valid = false;
    bool on = false;
    uint8_t current_raw = 0;
    uint32_t last_seen_ms = 0;
};

struct PowercellCellTelemetry {
    bool valid = false;
    uint8_t voltage_raw = 0;
    int8_t temperature_c = 0;
    uint32_t last_seen_ms = 0;
};

class CanManager {
public:
    static CanManager& instance();

    // GPIO pin configuration - VERIFIED WORKING:
    // TX=GPIO20, RX=GPIO19 is the CORRECT configuration for this board
    // (The pins were incorrectly swapped in a recent commit)
    static constexpr gpio_num_t DEFAULT_TX_PIN = static_cast<gpio_num_t>(20);
    static constexpr gpio_num_t DEFAULT_RX_PIN = static_cast<gpio_num_t>(19);

    void setExpander(ESP_IOExpander* exp) { expander_ = exp; }
    bool begin(gpio_num_t tx_pin = DEFAULT_TX_PIN, gpio_num_t rx_pin = DEFAULT_RX_PIN, std::uint32_t bitrate = 250000);
    void stop();
    bool sendButtonAction(const ButtonConfig& button);
    bool sendButtonReleaseAction(const ButtonConfig& button);
    bool sendFrame(const CanFrameConfig& frame);
    bool sendStandardFrame(uint16_t identifier, const uint8_t data[8], uint8_t length);
    
    bool receiveMessage(CanRxMessage& msg, uint32_t timeout_ms = 10);
    std::vector<CanRxMessage> receiveAll(uint32_t timeout_ms = 100);

    // Helper for sending J1939 PGN (used by background tasks)
    bool sendJ1939Pgn(uint8_t priority, uint32_t pgn, uint8_t source_addr, const uint8_t data[8]);

    // Suspension control (separate from Infinitybox pipeline)
    void updateSuspensionState(const SuspensionState& state);
    SuspensionState getSuspensionState() const;
    SuspensionCANStats getSuspensionStats() const;
    bool sendSuspensionCommand();  // Sends current state to 0x737
    void parseSuspensionStatus(const uint8_t data[8]);  // Parse 0x738 response

    bool updatePowercellStatusFromPgn(uint32_t pgn, const uint8_t data[8]);
    PowercellOutputState getPowercellOutputState(uint8_t cell_address, uint8_t output_number) const;
    PowercellCellTelemetry getPowercellCellTelemetry(uint8_t cell_address) const;

    bool isReady() const { return ready_; }
    bool isBusAlive() const { return bus_alive_; }
    gpio_num_t txPin() const { return tx_pin_; }
    gpio_num_t rxPin() const { return rx_pin_; }

private:
    CanManager() = default;
    
    // Force USB_SEL to CAN mode using expander
    void forceCanMux();

    ESP_IOExpander* expander_ = nullptr;
    bool ready_ = false;
    bool bus_alive_ = false;
    gpio_num_t tx_pin_ = DEFAULT_TX_PIN;
    gpio_num_t rx_pin_ = DEFAULT_RX_PIN;
    std::uint32_t bitrate_ = 250000;

    // Suspension state (separate from Infinitybox)
    SuspensionState suspension_state_;
    SuspensionCANStats suspension_stats_;
    mutable SemaphoreHandle_t suspension_mutex_ = nullptr;

    static constexpr uint8_t kPowercellMaxAddress = 16;
    static constexpr uint8_t kPowercellOutputsPerCell = 10;

    struct PowercellCellStatus {
        std::array<PowercellOutputState, kPowercellOutputsPerCell> outputs{};
        uint8_t voltage_raw = 0;
        int8_t temperature_c = 0;
        uint32_t last_seen_ms = 0;
    };

    std::array<PowercellCellStatus, kPowercellMaxAddress> powercell_status_{};
    mutable SemaphoreHandle_t powercell_mutex_ = nullptr;

    std::uint32_t buildIdentifier(const CanFrameConfig& frame) const;
};
