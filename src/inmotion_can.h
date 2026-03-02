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

// Response byte 0 encodes H-bridge 1 relay states; byte 1 encodes H-bridge 2.
// bit[4] = Relay A of that H-bridge is energized (per spec: response 0x10 for R1A)
// bit[0] = Relay B of that H-bridge is energized (per spec: response 0x01 for R1B)
// NOTE: These are independent relay-active bits — NOT "on" vs "current-limit" for one relay.
constexpr uint8_t RESP_REL_A_ON = 0x10;  ///< bit[4]: Relay A is energized
constexpr uint8_t RESP_REL_B_ON = 0x01;  ///< bit[0]: Relay B is energized

// Legacy aliases kept for any external callers that may reference the old names:
constexpr uint8_t RESP_ON_BIT        = RESP_REL_A_ON;  ///< @deprecated use RESP_REL_A_ON
constexpr uint8_t RESP_CURRENT_LIMIT = RESP_REL_B_ON;  ///< @deprecated use RESP_REL_B_ON

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

// ── H-BRIDGE 1 BYTE SWAP NOTE ───────────────────────────────────────────────
// Per the spec, byte 0 = Relay 1A and byte 1 = Relay 1B.
// In this physical installation the motor wires are reversed, so:
//   byte 0 command → DOWN direction   (response: byte 0 bit[4] = 0x10)
//   byte 1 command → UP direction     (response: byte 0 bit[0] = 0x01)
// All functions below respect this swap via their byte ordering.
// ─────────────────────────────────────────────────────────────────────────────

/// Drive Relay 1A (byte 0 = DOWN in this hardware) in Track mode — stays on until stopped.
inline void relay1A_Track(const Module& m) {
    uint8_t d[8] = { CMD_TRACK, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };  // byte 0 ON, byte 1 don't-care
    sendCommand(m, d);
}

/// Drive Relay 1B (byte 1 = UP in this hardware) in Track mode — stays on until stopped.
inline void relay1B_Track(const Module& m) {
    uint8_t d[8] = { 0x00, CMD_TRACK, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };  // byte 1 ON, byte 0 don't-care
    sendCommand(m, d);
}

/// Drive Relay 1A (DOWN) in Express mode — auto-stops at end of travel via current limit.
/// Only use when H-bridge is known idle (no prior stalled Express outstanding).
inline void relay1A_Express(const Module& m) {
    uint8_t d[8] = { CMD_EXPRESS, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };  // byte 0 Express, byte 1 don't-care
    sendCommand(m, d);
}

/// Drive Relay 1B (UP) in Express mode — auto-stops at end of travel via current limit.
/// Only use when H-bridge is known idle (no prior stalled Express outstanding).
inline void relay1B_Express(const Module& m) {
    uint8_t d[8] = { 0x00, CMD_EXPRESS, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };  // byte 1 Express, byte 0 don't-care
    sendCommand(m, d);
}

/// Immediately stop both Relay 1 directions.
inline void relay1_Stop(const Module& m) {
    uint8_t d[8] = { CMD_OFF, CMD_OFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
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
 *
 *  Uses Track personality (0x90) — motor runs while commanded, stops on windowStop().
 *  Byte layout with hardware swap: byte 1 = UP direction, byte 0 = 0x00 (don't-care).
 *
 *  CRITICAL: byte 0 must be 0x00 (don't-care), NOT CMD_OFF (0x80).
 *  After an Express DOWN stall the H-bridge protection is active; sending CMD_OFF
 *  on byte 0 simultaneously with a new command on byte 1 blocks the new command.
 *  0x00 leaves byte 0 in its current (already-off) state and lets byte 1 fire cleanly.
 *
 *  AppState: immediately transitions to OPENING.
 */
inline void windowUp(uint8_t window_id) {
    if (window_id >= 4) return;
    AppState::getInstance().setWindowState(window_id, WindowState::OPENING);
    // byte 1 = UP direction (Track ON); byte 0 = 0x00 don't-care (as per spec)
    uint8_t d[8] = { 0x00, CMD_TRACK, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    sendCommand(*kModules[window_id], d);
    Serial.printf("[inMOTION] Window %d UP (Track)\n", window_id);
}

/**
 * @brief Lower window.
 *
 *  Uses Track personality (0x90) — motor runs while commanded, stops on windowStop().
 *  Byte layout with hardware swap: byte 0 = DOWN direction, byte 1 = 0x00 (don't-care).
 *
 *  AppState: immediately transitions to CLOSING.
 */
inline void windowDown(uint8_t window_id) {
    if (window_id >= 4) return;
    AppState::getInstance().setWindowState(window_id, WindowState::CLOSING);
    // byte 0 = DOWN direction (Track ON); byte 1 = 0x00 don't-care (as per spec)
    uint8_t d[8] = { CMD_TRACK, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    sendCommand(*kModules[window_id], d);
    Serial.printf("[inMOTION] Window %d DOWN (Track)\n", window_id);
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
    // H-bridge 1 (windows).  With this hardware's byte swap:
    //   relay1a_on = byte-0 relay = DOWN direction active  (response byte 0 bit[4] = 0x10)
    //   relay1b_on = byte-1 relay = UP direction active    (response byte 0 bit[0] = 0x01)
    bool     relay1a_on = false;  ///< Relay 1A energized — DOWN in this hardware
    bool     relay1b_on = false;  ///< Relay 1B energized — UP in this hardware
    // H-bridge 2 (door locks)
    bool     relay2a_on = false;  ///< Relay 2A energized — LOCK
    bool     relay2b_on = false;  ///< Relay 2B energized — UNLOCK
    float    relay1_amps  = 0.0f;   ///< H-bridge 1 current (196 mA/count)
    float    relay2_amps  = 0.0f;   ///< H-bridge 2 current (196 mA/count)
    float    mosfet_amps  = 0.0f;   ///< Aggregate MOSFET current (20 mA/count)
    uint32_t last_rx_ms   = 0;
    bool     alive        = false;  ///< true if heard within last 2 s
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
    // Decode relay status bits per spec:
    //   byte 0 bit[4] (0x10) = Relay 1A active  → DOWN direction in this hardware
    //   byte 0 bit[0] (0x01) = Relay 1B active  → UP direction in this hardware
    //   byte 1 bit[4] (0x10) = Relay 2A active  → LOCK
    //   byte 1 bit[0] (0x01) = Relay 2B active  → UNLOCK
    const bool prev_relay1a = s.relay1a_on;
    const bool prev_relay1b = s.relay1b_on;
    s.relay1a_on  = (data[0] & RESP_REL_A_ON) != 0;  // DOWN motor
    s.relay1b_on  = (data[0] & RESP_REL_B_ON) != 0;  // UP motor
    s.relay2a_on  = (data[1] & RESP_REL_A_ON) != 0;  // LOCK
    s.relay2b_on  = (data[1] & RESP_REL_B_ON) != 0;  // UNLOCK
    s.relay1_amps = data[4] * 0.196f;
    s.relay2_amps = data[5] * 0.196f;
    s.mosfet_amps = data[6] * 0.020f;
    s.last_rx_ms  = millis();
    s.alive       = true;

    // ── Update window AppState from falling edges ──────────────────────────
    // A relay de-energising (falling edge) while a directional move is in
    // progress means the motor stopped — either user-commanded OR reached
    // end-of-travel and the H-bridge cut itself.
    auto& app = AppState::getInstance();
    const WindowState ws = app.getWindowState(static_cast<uint8_t>(idx));

    if (prev_relay1a && !s.relay1a_on && ws == WindowState::CLOSING) {
        // DOWN motor just stopped while we were closing → assume reached bottom
        app.setWindowState(static_cast<uint8_t>(idx), WindowState::CLOSED);
        Serial.printf("[inMOTION] Window %d DOWN motor stopped (CLOSED)\n", idx);
    } else if (prev_relay1b && !s.relay1b_on && ws == WindowState::OPENING) {
        // UP motor just stopped while we were opening → assume reached top
        app.setWindowState(static_cast<uint8_t>(idx), WindowState::OPEN);
        Serial.printf("[inMOTION] Window %d UP motor stopped (OPEN)\n", idx);
    }

    return true;
}

}  // namespace InMotionNGX
