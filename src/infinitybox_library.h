#pragma once

#include <cstdint>
#include <array>
#include "config_types.h"

/**
 * InfinityBox CAN Protocol Library
 * 
 * This library provides standard frame configurations for InfinityBox devices
 * including POWERCELL NGX, inMOTION NGX, and other modules.
 * 
 * All frames use J1939 protocol with 250 kbps bitrate.
 * Default source address: 0x63 (required by most InfinityBox devices)
 */

namespace InfinityBox {

// Common source address for InfinityBox communication
constexpr uint8_t SOURCE_ADDRESS = 0x63;
constexpr uint8_t BROADCAST_ADDRESS = 0xFF;

// ========== POWERCELL NGX Frames ==========
// POWERCELL uses PGN range 0xFF40-0xFF4F for configuration (16 addresses)
// and 0xFF50-0xFF5F for output control

/**
 * Create a POWERCELL configuration frame
 * @param cell_address Cell address (1-16)
 * @param config_byte Configuration byte (default: 0x01 for 250kb/s, 10s LOC, 250ms reporting, 200Hz PWM)
 * @return CanFrameConfig for configuration
 */
inline CanFrameConfig powercellConfig(uint8_t cell_address, uint8_t config_byte = 0x01) {
    CanFrameConfig frame;
    frame.enabled = true;
    frame.pgn = 0xFF40 + (cell_address == 16 ? 0 : cell_address);
    frame.priority = 6;
    frame.source_address = SOURCE_ADDRESS;
    frame.destination_address = BROADCAST_ADDRESS;
    frame.data = {0x99, config_byte, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    frame.length = 8;
    return frame;
}

/**
 * Create a POWERCELL output control frame
 * @param cell_address Cell address (1-16)
 * @param output Output number (1-8)
 * @param state Output state (0x00 = off, 0xFF = on)
 * @return CanFrameConfig for output control
 */
inline CanFrameConfig powercellOutput(uint8_t cell_address, uint8_t output, uint8_t state) {
    CanFrameConfig frame;
    frame.enabled = true;
    frame.pgn = 0xFF50 + (cell_address == 16 ? 0 : cell_address);
    frame.priority = 6;
    frame.source_address = SOURCE_ADDRESS;
    frame.destination_address = BROADCAST_ADDRESS;
    frame.data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    
    if (output >= 1 && output <= 8) {
        frame.data[output - 1] = state;
    }
    frame.length = 8;
    return frame;
}

/**
 * Poll POWERCELL for status
 * @param cell_address Cell address (1-16)
 * @return CanFrameConfig for polling
 */
inline CanFrameConfig powercellPoll(uint8_t cell_address) {
    CanFrameConfig frame;
    frame.enabled = true;
    frame.pgn = 0xFF50 + (cell_address == 16 ? 0 : cell_address);
    frame.priority = 6;
    frame.source_address = SOURCE_ADDRESS;
    frame.destination_address = BROADCAST_ADDRESS;
    frame.data = {0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    frame.length = 8;
    return frame;
}

// ========== inMOTION NGX Frames ==========
// inMOTION uses PGN 0xFEF9 for motor control commands

/**
 * Create an inMOTION motor control frame
 * @param motor_id Motor identifier (1-255)
 * @param position Target position (0-255, where 127 = center)
 * @param speed Speed setting (0-255)
 * @return CanFrameConfig for motor control
 */
inline CanFrameConfig inmotionControl(uint8_t motor_id, uint8_t position, uint8_t speed = 128) {
    CanFrameConfig frame;
    frame.enabled = true;
    frame.pgn = 0xFEF9;
    frame.priority = 3;
    frame.source_address = SOURCE_ADDRESS;
    frame.destination_address = BROADCAST_ADDRESS;
    frame.data = {motor_id, position, speed, 0x00, 0x00, 0x00, 0x00, 0x00};
    frame.length = 8;
    return frame;
}

/**
 * inMOTION stop command
 * @param motor_id Motor identifier
 * @return CanFrameConfig to stop motor
 */
inline CanFrameConfig inmotionStop(uint8_t motor_id) {
    return inmotionControl(motor_id, 127, 0);  // Center position, zero speed
}

// ========== KEYPAD NGX Frames ==========
// KEYPAD uses PGN 0xFEFA for backlight and LED control

/**
 * Create a KEYPAD backlight control frame
 * @param brightness Brightness level (0-255)
 * @return CanFrameConfig for backlight control
 */
inline CanFrameConfig keypadBacklight(uint8_t brightness) {
    CanFrameConfig frame;
    frame.enabled = true;
    frame.pgn = 0xFEFA;
    frame.priority = 6;
    frame.source_address = SOURCE_ADDRESS;
    frame.destination_address = BROADCAST_ADDRESS;
    frame.data = {0x01, brightness, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    frame.length = 8;
    return frame;
}

/**
 * Create a KEYPAD LED control frame
 * @param led_id LED identifier (1-16)
 * @param state LED state (0x00 = off, 0xFF = on)
 * @param color RGB color (optional, for RGB LEDs)
 * @return CanFrameConfig for LED control
 */
inline CanFrameConfig keypadLED(uint8_t led_id, uint8_t state, uint32_t color = 0xFFFFFF) {
    CanFrameConfig frame;
    frame.enabled = true;
    frame.pgn = 0xFEFA;
    frame.priority = 6;
    frame.source_address = SOURCE_ADDRESS;
    frame.destination_address = BROADCAST_ADDRESS;
    frame.data = {
        0x02,  // LED command
        led_id,
        state,
        static_cast<uint8_t>((color >> 16) & 0xFF),  // Red
        static_cast<uint8_t>((color >> 8) & 0xFF),   // Green
        static_cast<uint8_t>(color & 0xFF),          // Blue
        0x00,
        0x00
    };
    frame.length = 8;
    return frame;
}

// ========== Common Presets ==========

/**
 * Standard turn signal flash pattern
 * Creates a frame suitable for flash_count=6, flash_interval_ms=500 (3 seconds total)
 */
inline CanFrameConfig turnSignal(uint8_t cell_address, uint8_t output, bool left = true) {
    return powercellOutput(cell_address, output, 0xFF);
}

/**
 * Standard brake light pattern
 * Creates a frame suitable for hold_duration_ms=0 (toggle on/off)
 */
inline CanFrameConfig brakeLight(uint8_t cell_address, uint8_t output) {
    return powercellOutput(cell_address, output, 0xFF);
}

/**
 * Power window pattern
 * Creates a frame suitable for hold_duration_ms=10000 (10 seconds)
 */
inline CanFrameConfig powerWindow(uint8_t cell_address, uint8_t output, bool up = true) {
    return powercellOutput(cell_address, output, 0xFF);
}

} // namespace InfinityBox
