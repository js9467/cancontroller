/**
 * @file board_config.h
 * 
 * HARDWARE CONFIGURATION - Board Support Package
 * 
 * NOTE: All hardware constants have been moved to main.cpp.
 *       This file is kept for reference only.
 */

#pragma once

#include <Arduino.h>
#include <stdint.h>

// Hardware constants (reference only - actual values are in main.cpp)
constexpr int   I2C_MASTER_NUM          = 1;
constexpr int   I2C_PIN_SDA             = 8;
constexpr int   I2C_PIN_SCL             = 9;
constexpr uint32_t I2C_CLOCK_HZ         = 100000;

constexpr uint8_t TP_RST_MASK           = (1 << 1);
constexpr uint8_t LCD_RST_MASK          = (1 << 3);
constexpr uint8_t SD_CS_MASK            = (1 << 4);
constexpr uint8_t USB_SEL_MASK          = (1 << 5);
constexpr uint8_t SAFE_MASK             = TP_RST_MASK | LCD_RST_MASK | SD_CS_MASK | USB_SEL_MASK;

