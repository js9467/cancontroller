/**
 * @file board_bsp.cpp
 * 
 * BOARD SUPPORT PACKAGE - Minimal Implementation
 * NOTE: All hardware initialization has been moved to main.cpp for simplicity
 *       This file is kept for compatibility but is mostly empty.
 */

#include "board_bsp.h"

namespace Board {

// Placeholder implementations - actual initialization is in main.cpp
void initI2C() {
    // Handled in main.cpp
}

void startMuxWatchdog(ESP_IOExpander* expander) {
    // Handled in main.cpp - task created in setup()
}

}  // namespace Board

