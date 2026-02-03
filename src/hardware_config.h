/**
 * @file hardware_config.h
 * @brief Hardware Configuration for Waveshare ESP32-S3-Touch-LCD-7
 * 
 * ⚠️  SINGLE SOURCE OF TRUTH FOR ALL HARDWARE CONFIGURATION ⚠️
 * 
 * Last validated: 2026-02-02 with firmware v2.1.3
 * If you modify ANY value in this file, document the change with date and reason.
 * 
 * COMPILE-TIME PROTECTION: Static assertions prevent accidental modifications.
 */

#pragma once

#include <cstdint>

// ============================================================================
// I2C BUS CONFIGURATION
// ============================================================================

// I2C bus number (ESP32-S3 has 2 I2C buses: 0 and 1)
// CRITICAL: Must match ESP_Panel library configuration (ESP_PANEL_LCD_TOUCH_BUS_HOST_ID)
constexpr uint8_t HW_I2C_BUS_NUM = 0;   // I2C bus 0 (shared with touch controller)

// I2C GPIO pins - LOCKED: DO NOT CHANGE
constexpr uint8_t HW_I2C_SDA_PIN = 8;   // I2C data pin (shared: touch + CH422G)
constexpr uint8_t HW_I2C_SCL_PIN = 9;   // I2C clock pin (shared: touch + CH422G)

// I2C clock speed
constexpr uint32_t HW_I2C_CLOCK_HZ = 100000;  // 100 kHz (safe for all devices)

// ============================================================================
// CH422G I2C EXPANDER CONFIGURATION
// ============================================================================

// CH422G I2C address - LOCKED: DO NOT CHANGE
constexpr uint8_t HW_CH422G_I2C_ADDR = 0x00;  // Factory default address (A2=A1=A0=LOW)

// CH422G Pin Definitions (bit positions in output register)
constexpr uint8_t HW_CH422G_PIN_UNKNOWN_0 = 0;  // Bit 0 - unknown function
constexpr uint8_t HW_CH422G_PIN_TP_RST = 1;     // Bit 1 - Touch reset (active LOW)
constexpr uint8_t HW_CH422G_PIN_UNKNOWN_2 = 2;  // Bit 2 - unknown function
constexpr uint8_t HW_CH422G_PIN_LCD_RST = 3;    // Bit 3 - LCD reset (active LOW)
constexpr uint8_t HW_CH422G_PIN_SD_CS = 4;      // Bit 4 - SD card chip select (active LOW)
constexpr uint8_t HW_CH422G_PIN_USB_SEL = 5;    // Bit 5 - USB/CAN mux (HIGH=CAN, LOW=USB)
constexpr uint8_t HW_CH422G_PIN_UNKNOWN_6 = 6;  // Bit 6 - unknown function
constexpr uint8_t HW_CH422G_PIN_UNKNOWN_7 = 7;  // Bit 7 - unknown function

// CH422G Pin Masks
constexpr uint8_t HW_CH422G_MASK_TP_RST = (1 << HW_CH422G_PIN_TP_RST);
constexpr uint8_t HW_CH422G_MASK_LCD_RST = (1 << HW_CH422G_PIN_LCD_RST);
constexpr uint8_t HW_CH422G_MASK_SD_CS = (1 << HW_CH422G_PIN_SD_CS);
constexpr uint8_t HW_CH422G_MASK_USB_SEL = (1 << HW_CH422G_PIN_USB_SEL);

// Safe state mask: All managed outputs (reset pins HIGH, CS HIGH, CAN selected)
constexpr uint8_t HW_CH422G_SAFE_MASK = (HW_CH422G_MASK_TP_RST | 
                                          HW_CH422G_MASK_LCD_RST | 
                                          HW_CH422G_MASK_SD_CS | 
                                          HW_CH422G_MASK_USB_SEL);

// ============================================================================
// CAN (TWAI) HARDWARE CONFIGURATION
// ============================================================================

// TWAI (CAN) GPIO pins - LOCKED: DO NOT CHANGE
constexpr uint8_t HW_TWAI_TX_PIN = 20;  // CAN TX pin
constexpr uint8_t HW_TWAI_RX_PIN = 19;  // CAN RX pin

// CAN transceiver is gated by CH422G USB_SEL pin
// HIGH = CAN mode (transceiver enabled)
// LOW = USB mode (transceiver disabled)
constexpr uint8_t HW_CAN_GATE_BIT = HW_CH422G_PIN_USB_SEL;  // USB_SEL controls CAN gate

// CAN timing parameters - SAFE TO ADJUST
constexpr uint32_t HW_CAN_GATE_SETTLE_MS = 10;      // Hardware settle time after gate write
constexpr uint32_t HW_CAN_GATE_RETRY_DELAY_MS = 50; // Retry delay if gate write fails
constexpr uint8_t HW_CAN_GATE_MAX_RETRIES = 3;      // Max retries for gate write

// ============================================================================
// DISPLAY PANEL CONFIGURATION (Reference Only)
// ============================================================================
// NOTE: Panel GPIOs are configured in lib/ESP_Panel_Conf.h
// This section is for documentation only

// Touch controller: GT911 on I2C bus 0 (GPIO9=SCL, GPIO8=SDA)
// LCD controller: ST7262 RGB parallel interface (16 data pins + control)
// Backlight: PWM on GPIO6

// ============================================================================
// COMPILE-TIME PROTECTION
// ============================================================================
// These assertions will FAIL the build if critical values are changed

static_assert(HW_I2C_BUS_NUM == 0,
              "⚠️  HARDWARE VIOLATION: I2C bus MUST be 0 to match ESP_Panel library");

static_assert(HW_TWAI_TX_PIN == 20 && HW_TWAI_RX_PIN == 19,
              "⚠️  HARDWARE VIOLATION: TWAI pins MUST be TX=20, RX=19 for Waveshare ESP32-S3");

static_assert(HW_I2C_SDA_PIN == 8 && HW_I2C_SCL_PIN == 9,
              "⚠️  HARDWARE VIOLATION: I2C pins MUST be SDA=8, SCL=9 for Waveshare ESP32-S3");

static_assert(HW_CH422G_I2C_ADDR == 0x00,
              "⚠️  HARDWARE VIOLATION: CH422G I2C address MUST be 0x00");

static_assert(HW_CH422G_PIN_USB_SEL == 5,
              "⚠️  HARDWARE VIOLATION: USB_SEL bit MUST be 5");

// ============================================================================
// CHANGE LOG
// ============================================================================
// 2026-02-02: Consolidated all hardware config from main.cpp and board_config.h
//             Fixed I2C bus number: changed from 1 to 0 (matches panel library)
//             Added CH422G pin definitions with bit masks
//             Documented all pin functions
