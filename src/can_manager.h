#pragma once

#include <Arduino.h>
#include <hal/gpio_types.h>
#include <vector>

#include "config_types.h"

// Struct for received CAN messages (different from CanMessage in config_types.h)
struct CanRxMessage {
    uint32_t identifier;
    uint8_t data[8];
    uint8_t length;
    uint32_t timestamp;
};

class CanManager {
public:
    static CanManager& instance();

    // GPIO pin configuration - VERIFIED WORKING:
    // TX=GPIO20, RX=GPIO19 is the CORRECT configuration for this board
    // (The pins were incorrectly swapped in a recent commit)
    static constexpr gpio_num_t DEFAULT_TX_PIN = static_cast<gpio_num_t>(20);
    static constexpr gpio_num_t DEFAULT_RX_PIN = static_cast<gpio_num_t>(19);

    bool begin(gpio_num_t tx_pin = DEFAULT_TX_PIN, gpio_num_t rx_pin = DEFAULT_RX_PIN, std::uint32_t bitrate = 250000);
    void stop();
    bool sendButtonAction(const ButtonConfig& button);
    bool sendButtonReleaseAction(const ButtonConfig& button);
    bool sendFrame(const CanFrameConfig& frame);
    
    bool receiveMessage(CanRxMessage& msg, uint32_t timeout_ms = 10);
    std::vector<CanRxMessage> receiveAll(uint32_t timeout_ms = 100);

    // Infinitybox-specific command sequences (J1939 protocol)
    bool sendInfinityboxOutput1On();
    bool sendInfinityboxOutput1Off();
    bool sendInfinityboxOutput9On();
    bool sendInfinityboxOutput9Off();

    // Helper for sending J1939 PGN (used by background tasks)
    bool sendJ1939Pgn(uint8_t priority, uint32_t pgn, uint8_t source_addr, const uint8_t data[8]);

    bool isReady() const { return ready_; }
    gpio_num_t txPin() const { return tx_pin_; }
    gpio_num_t rxPin() const { return rx_pin_; }

private:
    CanManager() = default;

    bool ready_ = false;
    gpio_num_t tx_pin_ = DEFAULT_TX_PIN;
    gpio_num_t rx_pin_ = DEFAULT_RX_PIN;
    std::uint32_t bitrate_ = 250000;

    std::uint32_t buildIdentifier(const CanFrameConfig& frame) const;
};
