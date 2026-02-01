/**
 * @file hardware_config.h
 * @brief LOCKED Hardware Configuration for Waveshare ESP32-S3-Touch-LCD-7
 * 
 * ⚠️ CRITICAL: DO NOT MODIFY THESE VALUES ⚠️
 * 
 * These constants were validated against working hardware (v2.0.1 backup).
 * Changing ANY value will break CAN communication or cause boot loops.
 * See HARDWARE_LOCK.md for detailed explanation.
 * 
 * COMPILE-TIME PROTECTION: Static assertions will FAIL the build if values change.
 */

#pragma once

#include <cstdint>

// ============================================================================
// CAN HARDWARE CONFIGURATION
// ============================================================================

// TWAI (CAN) GPIO pins - LOCKED: DO NOT CHANGE
constexpr uint8_t HW_TWAI_TX_PIN = 20;  // CAN TX pin
constexpr uint8_t HW_TWAI_RX_PIN = 19;  // CAN RX pin

// I2C GPIO pins (shared with panel) - LOCKED: DO NOT CHANGE
constexpr uint8_t HW_I2C_SDA_PIN = 8;   // I2C data pin
constexpr uint8_t HW_I2C_SCL_PIN = 9;   // I2C clock pin

// CH422G I2C Expander Configuration - LOCKED: DO NOT CHANGE
// The CAN transceiver is gated by the CH422G I2C expander
// Without correct values, CAN appears to work but receives 0 frames!
constexpr uint8_t HW_CAN_GATE_I2C_ADDR = 0x38;      // CH422G WR_IO register address
constexpr uint8_t HW_CAN_GATE_VALUE_PRIMARY = 0x2A; // USB_SEL bit 5 = HIGH (enables CAN)
constexpr uint8_t HW_CAN_GATE_VALUE_ALT1 = 0x43;    // Alternative gate value (from sketch)
constexpr uint8_t HW_CAN_GATE_VALUE_ALT2 = 0x07;    // Alternative gate value (legacy)
constexpr uint8_t HW_CAN_GATE_BIT = 5;              // USB_SEL bit position

// CAN timing and recovery parameters - SAFE TO ADJUST
constexpr uint32_t HW_CAN_GATE_SETTLE_MS = 10;      // Hardware settle time after gate write
constexpr uint32_t HW_CAN_GATE_RETRY_DELAY_MS = 50; // Retry delay if gate write fails
constexpr uint8_t HW_CAN_GATE_MAX_RETRIES = 3;      // Max retries for gate write

// ============================================================================
// COMPILE-TIME PROTECTION
// ============================================================================
// These assertions will FAIL the build if critical values are changed

static_assert(HW_TWAI_TX_PIN == 20 && HW_TWAI_RX_PIN == 19,
              "⚠️  HARDWARE VIOLATION: TWAI pins MUST be TX=20, RX=19 for Waveshare ESP32-S3");

static_assert(HW_I2C_SDA_PIN == 8 && HW_I2C_SCL_PIN == 9,
              "⚠️  HARDWARE VIOLATION: I2C pins MUST be SDA=8, SCL=9 for Waveshare ESP32-S3");

static_assert(HW_CAN_GATE_I2C_ADDR == 0x38,
              "⚠️  HARDWARE VIOLATION: CH422G I2C address MUST be 0x38 (WR_IO register)");

static_assert(HW_CAN_GATE_VALUE_PRIMARY == 0x2A,
              "⚠️  HARDWARE VIOLATION: Primary gate value MUST be 0x2A (USB_SEL HIGH)");

static_assert(HW_CAN_GATE_BIT == 5,
              "⚠️  HARDWARE VIOLATION: USB_SEL bit MUST be 5");

// ============================================================================
// DOCUMENTATION REFERENCES
// ============================================================================
// See HARDWARE_LOCK.md for:
// - Why these values are locked
// - What happens if you change them
// - How to modify safely (if absolutely necessary)
// - Hardware validation history
