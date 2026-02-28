/**
 * @file inmotion_can.h
 * @brief inMOTION NGX CAN frame builder, feedback parser, and window/lock commands
 *
 * Protocol: Infinitybox 852-086A8 Rev2 Quick Start Guide
 *
 * ── COMMAND BYTE FORMAT (each output = one byte, bit numbering MSB→LSB) ─────
 *   bit[7]    Modifier — MUST be 1 or message is ignored
 *   bit[6]    Unused
 *   bits[5:4] Personality
 *               00 = OFF
 *               01 = Track  (output follows command state)
 *               10 = Timed  (run for duration set in bits[3:0])
 *               11 = Express (run until H-bridge current limit → auto-stop at end of travel)
 *   bits[3:0] Timer value (0.25 s/count, Timed mode only; ignored otherwise)
 *
 *   Quick reference:
 *     0x80 = OFF        0x90 = Track ON
 *     0xB0 = Express    0xA4 = Timed 1 s (4 × 0.25 s)
 *
 * ── BYTE POSITIONS PER MODULE ─────────────────────────────────────────────
 *   Byte 0: Relay 1A   Byte 1: Relay 1B   Byte 2: Relay 2A   Byte 3: Relay 2B
 *   Byte 4: Output 1   Byte 5: Output 2   Byte 6: Output 3   Byte 7: Output 4
 *
 * ── CAN IDs (J1939, 250 kb/s, priority 6) ────────────────────────────────
 *   Position          Command ID (we send)   Response ID (we receive)
 *   Driver Front      0x18FF031A             0x18FF331B
 *   Passenger Front   0x18FF041A             0x18FF341B
 *   Driver Rear       0x18FF051A             0x18FF351B
 *   Passenger Rear    0x18FF061A             0x18FF361B
 *
 * ── RESPONSE BYTE LAYOUT ─────────────────────────────────────────────────
 *   Byte 0: Relay 1 status   bit[4]=ON, bit[0]=current-limit reached
 *   Byte 1: Relay 2 status   bit[4]=ON, bit[0]=current-limit reached
 *   Bytes 4-5: H-bridge current (196 mA/count)
 *   Bytes 6-7: MOSFET current aggregate (20 mA/count)
 *   Broadcast every 500 ms OR on any state/current-limit change.
 *
 * ── WINDOW WIRING CONVENTION ─────────────────────────────────────────────
 *   Relay 1A → Window UP    Relay 1B → Window DOWN
 *   (Express personality: inMOTION auto-cuts on current spike at full travel)
 *
 * ── LOCK WIRING CONVENTION ───────────────────────────────────────────────
 *   Relay 2A → LOCK pulse   Relay 2B → UNLOCK pulse
 *   (Timed personality: configurable pulse width, default 0.5 s)
 */

#pragma once

#include <Arduino.h>
#include "can_manager.h"
#include "app_state.h"

namespace InMotionNGX {

// ── COMMAND BYTE CONSTANTS ────────────────────────────────────────────────

constexpr uint8_t MODIFIER            = 0x80;   ///< bit[7] mandatory
constexpr uint8_t PERSONALITY_OFF     = 0x00;   ///< bits[5:4] = 00
constexpr uint8_t PERSONALITY_TRACK   = 0x10;   ///< bits[5:4] = 01
constexpr uint8_t PERSONALITY_TIMED   = 0x20;   ///< bits[5:4] = 10
constexpr uint8_t PERSONALITY_EXPRESS = 0x30;   ///< bits[5:4] = 11

constexpr uint8_t CMD_OFF     = MODIFIER | PERSONALITY_OFF;     ///< 0x80
constexpr uint8_t CMD_TRACK   = MODIFIER | PERSONALITY_TRACK;   ///< 0x90
constexpr uint8_t CMD_EXPRESS = MODIFIER | PERSONALITY_EXPRESS; ///< 0xB0

/// @brief Build a Timed command byte.
/// @param quarter_seconds  Duration in 0.25 s steps; range 1–15 (clipped).
inline uint8_t cmdTimed(uint8_t quarter_seconds) {
    quarter_seconds = (quarter_seconds < 1) ? 1 : (quarter_seconds > 15 ? 15 : quarter_seconds);
    return MODIFIER | PERSONALITY_TIMED | (quarter_seconds & 0x0F);
}

// ── MODULE DEFINITIONS ────────────────────────────────────────────────────

struct Module {
    const char* name;
    uint8_t cmd_pgn_ps;   ///< PS byte of command PGN  → CAN ID 0x18FF<ps><sa>
    uint8_t cmd_sa;       ///< SA byte for command frames
    uint8_t resp_pgn_ps;  ///< PS byte of response PGN
    uint8_t resp_sa;      ///< SA byte of response frames
};

// The four installed inMOTION NGX modules
constexpr Module kDriverFront    = { "Driver Front",    0x03, 0x1A, 0x33, 0x1B };
constexpr Module kPassengerFront = { "Passenger Front", 0x04, 0x1A, 0x34, 0x1B };
constexpr Module kDriverRear     = { "Driver Rear",     0x05, 0x1A, 0x35, 0x1B };
constexpr Module kPassengerRear  = { "Passenger Rear",  0x06, 0x1A, 0x36, 0x1B };

/// All modules in window_id order: 0=DF, 1=PF, 2=DR, 3=PR
constexpr const Module* kModules[4] = {
    &kDriverFront, &kPassengerFront, &kDriverRear, &kPassengerRear
};

// ── RELAY / OUTPUT BYTE INDICES ───────────────────────────────────────────

constexpr uint8_t BYTE_RELAY_1A = 0;
constexpr uint8_t BYTE_RELAY_1B = 1;
constexpr uint8_t BYTE_RELAY_2A = 2;
constexpr uint8_t BYTE_RELAY_2B = 3;
constexpr uint8_t BYTE_OUTPUT_1 = 4;
constexpr uint8_t BYTE_OUTPUT_2 = 5;
constexpr uint8_t BYTE_OUTPUT_3 = 6;
constexpr uint8_t BYTE_OUTPUT_4 = 7;

// ── RESPONSE BIT MASKS ────────────────────────────────────────────────────

constexpr uint8_t RESP_ON_BIT        = 0x10;  ///< bit[4] high when output is ON
constexpr uint8_t RESP_CURRENT_LIMIT = 0x01;  ///< bit[0] high when current limit exceeded (stall / end-of-travel)

// ── FRAME BUILDER ─────────────────────────────────────────────────────────

/**
 * @brief Send an 8-byte command frame to an inMOTION NGX module.
 *        Bytes set to 0x00 are "don't-care" (modifier bit = 0, module ignores them).
 */
inline void sendCommand(const Module& m, const uint8_t data[8]) {
    // PGN (PDU2 broadcast): 0xFF00 | PS byte
    const uint32_t pgn = (static_cast<uint32_t>(0xFF) << 8) | m.cmd_pgn_ps;
    CanManager::instance().sendJ1939Pgn(6, pgn, m.cmd_sa, data);
}

// ── RELAY 1 (H-BRIDGE 1 — WINDOWS) ───────────────────────────────────────

/// Drive Relay 1A in Express mode (window UP, auto-stops at top).
/// NOTE: Bytes 0 and 1 are swapped in this module — byte 0 is Relay 1B, byte 1 is Relay 1A!
inline void relay1A_Express(const Module& m) {
    uint8_t d[8] = { 0x00, CMD_EXPRESS, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };  // byte 1 = Relay 1A (UP)
    sendCommand(m, d);
}

/// Drive Relay 1B in Express mode (window DOWN, auto-stops at bottom).
inline void relay1B_Express(const Module& m) {
    uint8_t d[8] = { CMD_EXPRESS, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };  // byte 0 = Relay 1B (DOWN)
    sendCommand(m, d);
}

/// Immediately stop both Relay 1 directions (kill window motor).
inline void relay1_Stop(const Module& m) {
    uint8_t d[8] = { CMD_OFF, CMD_OFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };  // Both bytes = OFF
    sendCommand(m, d);
}

// ── RELAY 2 (H-BRIDGE 2 — LOCKS) ────────────────────────────────────────

/// Drive Relay 2A in Express mode (hold direction A until stall).
inline void relay2A_Express(const Module& m) {
    uint8_t d[8] = { 0x00, 0x00, CMD_EXPRESS, 0x00, 0x00, 0x00, 0x00, 0x00 };
    sendCommand(m, d);
}

/// Drive Relay 2B in Express mode.
inline void relay2B_Express(const Module& m) {
    uint8_t d[8] = { 0x00, 0x00, 0x00, CMD_EXPRESS, 0x00, 0x00, 0x00, 0x00 };
    sendCommand(m, d);
}

/// Drive Relay 2A with a timed pulse (lock solenoid).
inline void relay2A_Timed(const Module& m, uint8_t quarter_seconds = 2) {
    uint8_t d[8] = { 0x00, 0x00, cmdTimed(quarter_seconds), 0x00, 0x00, 0x00, 0x00, 0x00 };
    sendCommand(m, d);
}

/// Drive Relay 2B with a timed pulse (unlock solenoid).
inline void relay2B_Timed(const Module& m, uint8_t quarter_seconds = 2) {
    uint8_t d[8] = { 0x00, 0x00, 0x00, cmdTimed(quarter_seconds), 0x00, 0x00, 0x00, 0x00 };
    sendCommand(m, d);
}

/// Stop both Relay 2 directions.
inline void relay2_Stop(const Module& m) {
    uint8_t d[8] = { 0x00, 0x00, CMD_OFF, CMD_OFF, 0x00, 0x00, 0x00, 0x00 };
    sendCommand(m, d);
}

// ── MOSFET OUTPUTS (AUX) ─────────────────────────────────────────────────

/// Set an aux output (1–4) on or off using Track personality.
inline void setOutput(const Module& m, uint8_t output_num, bool on) {
    if (output_num < 1 || output_num > 4) return;
    uint8_t d[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    d[BYTE_OUTPUT_1 + (output_num - 1)] = on ? CMD_TRACK : CMD_OFF;
    sendCommand(m, d);
}

// ── HIGH-LEVEL: WINDOW CONTROL ────────────────────────────────────────────

/**
 * @brief Raise window (window_id: 0=DriverFront, 1=PassFront, 2=DriverRear, 3=PassRear).
 *        Uses Express personality — inMOTION auto-cuts when motor stalls at top.
 *        AppState transitions: OPENING → OPEN (via feedback in parseRxFrame).
 */
inline void windowUp(uint8_t window_id) {
    if (window_id >= 4) return;
    AppState::getInstance().setWindowState(window_id, WindowState::OPENING);
    // Bytes 0 and 1 are swapped: byte 1=Relay 1A (UP), byte 0=Relay 1B (DOWN)
    uint8_t d[8] = { CMD_OFF, CMD_EXPRESS, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    sendCommand(*kModules[window_id], d);
    Serial.printf("[inMOTION] Window %d UP (Express)\n", window_id);
}

/**
 * @brief Lower window.
 *        AppState transitions: CLOSING → CLOSED (via feedback in parseRxFrame).
 */
inline void windowDown(uint8_t window_id) {
    if (window_id >= 4) return;
    AppState::getInstance().setWindowState(window_id, WindowState::CLOSING);
    // Bytes 0 and 1 are swapped: byte 0=Relay 1B (DOWN), byte 1=Relay 1A (UP)
    uint8_t d[8] = { CMD_EXPRESS, CMD_OFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    sendCommand(*kModules[window_id], d);
    Serial.printf("[inMOTION] Window %d DOWN (Express)\n", window_id);
}

/**
 * @brief Stop window motor immediately (user released button, or safety stop).
 */
inline void windowStop(uint8_t window_id) {
    if (window_id >= 4) return;
    relay1_Stop(*kModules[window_id]);
    Serial.printf("[inMOTION] Window %d STOP\n", window_id);
}

/**
 * @brief Raise all four windows simultaneously.
 */
inline void allWindowsUp() {
    for (uint8_t i = 0; i < 4; i++) windowUp(i);
}

/**
 * @brief Lower all four windows simultaneously.
 */
inline void allWindowsDown() {
    for (uint8_t i = 0; i < 4; i++) windowDown(i);
}

// ── HIGH-LEVEL: DOOR LOCK CONTROL ────────────────────────────────────────

/**
 * @brief Lock a single door (Relay 2A timed pulse).
 * @param door_id  0=DriverFront, 1=PassFront, 2=DriverRear, 3=PassRear
 * @param pulse_ms Pulse duration in ms (rounded to 0.25 s; default 500 ms)
 */
inline void lockDoor(uint8_t door_id, uint16_t pulse_ms = 500) {
    if (door_id >= 4) return;
    const uint8_t counts = static_cast<uint8_t>((pulse_ms + 124) / 250);  // round to nearest 0.25 s
    relay2A_Timed(*kModules[door_id], counts);
    AppState::getInstance().setLockState(door_id, LockState::LOCKED);
    Serial.printf("[inMOTION] Door %d LOCK (%.2f s pulse)\n", door_id, counts * 0.25f);
}

/**
 * @brief Unlock a single door (Relay 2B timed pulse).
 */
inline void unlockDoor(uint8_t door_id, uint16_t pulse_ms = 500) {
    if (door_id >= 4) return;
    const uint8_t counts = static_cast<uint8_t>((pulse_ms + 124) / 250);
    relay2B_Timed(*kModules[door_id], counts);
    AppState::getInstance().setLockState(door_id, LockState::UNLOCKED);
    Serial.printf("[inMOTION] Door %d UNLOCK (%.2f s pulse)\n", door_id, counts * 0.25f);
}

/**
 * @brief Lock all four doors simultaneously.
 */
inline void lockAll(uint16_t pulse_ms = 500) {
    for (uint8_t i = 0; i < 4; i++) lockDoor(i, pulse_ms);
}

/**
 * @brief Unlock all four doors simultaneously.
 */
inline void unlockAll(uint16_t pulse_ms = 500) {
    for (uint8_t i = 0; i < 4; i++) unlockDoor(i, pulse_ms);
}

// ── LIVE FEEDBACK CACHE ───────────────────────────────────────────────────

struct ModuleStatus {
    bool     relay1_on            = false;
    bool     relay1_current_limit = false;  ///< Window reached end-of-travel / motor stalled
    bool     relay2_on            = false;
    bool     relay2_current_limit = false;
    float    relay1_amps          = 0.0f;   ///< H-bridge 1 current (196 mA/count)
    float    relay2_amps          = 0.0f;   ///< H-bridge 2 current (196 mA/count)
    float    mosfet_amps          = 0.0f;   ///< Aggregate MOSFET current (20 mA/count)
    uint32_t last_rx_ms           = 0;
    bool     alive                = false;  ///< true if heard within last 2 s
};

/// Cached status for each module, indexed 0=DF, 1=PF, 2=DR, 3=PR
static ModuleStatus g_status[4];

/// Returns true if we've heard from module within last 2 s.
inline bool isAlive(uint8_t module_idx) {
    if (module_idx >= 4) return false;
    return (millis() - g_status[module_idx].last_rx_ms) < 2000;
}

inline const ModuleStatus& getStatus(uint8_t module_idx) {
    return g_status[module_idx < 4 ? module_idx : 0];
}

// ── RX FRAME PARSER ───────────────────────────────────────────────────────

/**
 * @brief Call from CAN receive loop for every extended (29-bit) frame.
 *
 * @param pgn  Extracted PGN = (can_id >> 8) & 0x3FFFF  (as used in can_rx_task)
 * @param sa   Extracted SA  = can_id & 0xFF
 * @param data 8-byte payload
 * @return true if the frame was an inMOTION response frame (consumed it)
 */
inline bool parseRxFrame(uint32_t pgn, uint8_t sa, const uint8_t data[8]) {
    // inMOTION response PGNs: 0xFF33–0xFF36 with SA = 0x1B
    uint8_t pf = (pgn >> 8) & 0xFF;
    uint8_t ps = (pgn     ) & 0xFF;

    if (pf != 0xFF) return false;

    // Match to a module by (resp_pgn_ps, resp_sa)
    int idx = -1;
    for (int i = 0; i < 4; i++) {
        if (ps == kModules[i]->resp_pgn_ps && sa == kModules[i]->resp_sa) {
            idx = i;
            break;
        }
    }
    if (idx < 0) return false;

    ModuleStatus& s = g_status[idx];
    s.relay1_on            = (data[0] & RESP_ON_BIT)        != 0;
    s.relay1_current_limit = (data[0] & RESP_CURRENT_LIMIT) != 0;
    s.relay2_on            = (data[1] & RESP_ON_BIT)        != 0;
    s.relay2_current_limit = (data[1] & RESP_CURRENT_LIMIT) != 0;
    s.relay1_amps          = data[4] * 0.196f;
    s.relay2_amps          = data[5] * 0.196f;
    s.mosfet_amps          = data[6] * 0.020f;
    s.last_rx_ms           = millis();
    s.alive                = true;

    // ── Update window state from current-limit feedback ───────────────────
    // When Express mode finishes (motor stalled), inMOTION broadcasts
    // current-limit bit HIGH while simultaneously cutting output.
    auto& app = AppState::getInstance();
    WindowState ws = app.getWindowState(static_cast<uint8_t>(idx));

    if (s.relay1_current_limit) {
        // Motor stalled — resolve to final state based on direction we were running
        if (ws == WindowState::OPENING) {
            app.setWindowState(static_cast<uint8_t>(idx), WindowState::OPEN);
            Serial.printf("[inMOTION] Window %d reached TOP (current limit)\n", idx);
        } else if (ws == WindowState::CLOSING) {
            app.setWindowState(static_cast<uint8_t>(idx), WindowState::CLOSED);
            Serial.printf("[inMOTION] Window %d reached BOTTOM (current limit)\n", idx);
        }
    } else if (!s.relay1_on) {
        // H-bridge de-energised without stall → user commanded stop, leave state as-is
    }

    return true;
}

}  // namespace InMotionNGX
