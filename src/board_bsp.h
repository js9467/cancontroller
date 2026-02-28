/**
 * @file board_bsp.h
 * 
 * BOARD SUPPORT PACKAGE - Minimal API Header
 * 
 * NOTE: All hardware initialization has been moved to main.cpp.
 *       This file is kept for compatibility only.
 */

#pragma once

#include <ESP_Panel_Library.h>
#include <ESP_IOExpander_Library.h>

namespace Board {

void initI2C();
void startMuxWatchdog(ESP_IOExpander* expander);

}  // namespace Board
