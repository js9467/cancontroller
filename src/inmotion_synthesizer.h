#pragma once

#include "output_behavior_engine.h"
#include "inmotion_can.h"
#include <map>
#include <array>

/**
 * ==================================================================
 *  INMOTION FRAME SYNTHESIZER
 *
 *  Transmits behavioral output states to inMOTION NGX modules
 *
 *  Key Responsibility:
 *  - Collect current state from inMOTION outputs
 *  - Group by module address
 *  - Generate 8-byte command frames per module
 *  - Send frames when states change
 * ==================================================================
 */

namespace BehavioralOutput {

class InMotionSynthesizer {
public:
    InMotionSynthesizer(BehaviorEngine* engine)
        : _engine(engine), _lastTransmit(0), _transmitInterval(100) {}
    
    void setTransmitInterval(uint16_t interval_ms) {
        _transmitInterval = interval_ms;
    }
    
    void update() {
        if (!_engine) return;
        
        unsigned long now = millis();
        if (now - _lastTransmit < _transmitInterval) {
            return;
        }
        _lastTransmit = now;
        
        const auto& outputs = _engine->getOutputs();
        
        // Build desired command frames for every registered inMOTION output.
        //   - Active output   → CMD_TRACK / CMD_EXPRESS / cmdTimed  (turn on)
        //   - Inactive output → CMD_OFF (0x80)  (explicit OFF, not 0x00 don't-care)
        //
        // Using 0x00 (don't-care) for inactive outputs leaves the inMOTION module
        // in its last commanded state — AUX MOSFETs would never turn off.
        // Explicit CMD_OFF fixes this.
        std::map<uint8_t, std::array<uint8_t, 8>> frames;
        std::map<uint8_t, bool> hasActiveOutput;  // addr -> any output currently on

        // Step 1: Initialise every registered output slot to CMD_OFF.
        for (const auto& [id, output] : outputs) {
            if (output.deviceType != "INMOTION") continue;
            uint8_t addr = output.cellAddress;
            uint8_t outNum = output.outputNumber;
            if (addr < 1 || addr > 16) continue;
            if (outNum < 1 || outNum > 8) continue;
            if (!frames.count(addr)) {
                frames[addr].fill(0x00);
                hasActiveOutput[addr] = false;
            }
            frames[addr][outNum - 1] = InMotionNGX::CMD_OFF;
        }

        // Step 2: Apply the commanded state for every active output.
        for (const auto& [id, output] : outputs) {
            if (output.deviceType != "INMOTION") continue;
            if (!output.isActive || !output.currentState) continue;
            uint8_t addr = output.cellAddress;
            uint8_t outNum = output.outputNumber;
            if (addr < 1 || addr > 16) continue;
            if (outNum < 1 || outNum > 8) continue;

            uint8_t cmd;
            if (output.behavior.type == BehaviorType::EXPRESS) {
                cmd = InMotionNGX::CMD_EXPRESS;  // 0xB0
            } else if (output.behavior.type == BehaviorType::PULSE &&
                       output.behavior.duration_ms > 0) {
                uint8_t quarter_secs = (output.behavior.duration_ms + 124) / 250;
                cmd = InMotionNGX::cmdTimed(quarter_secs);
            } else {
                cmd = InMotionNGX::CMD_TRACK;  // 0x90
            }
            frames[addr][outNum - 1] = cmd;
            hasActiveOutput[addr] = true;
        }

        // Step 3: Transmit.
        // ACTIVE module  → always re-send every interval.  inMOTION NGX uses a
        //   CAN watchdog: if the controller stops refreshing CMD_TRACK / CMD_EXPRESS
        //   the module de-energises the relay/MOSFET for safety.  We must keep
        //   sending the frame every _transmitInterval ms while any output is on.
        // IDLE module    → send only when the frame changed vs the last transmission
        //   (delivers CMD_OFF exactly once on the active→idle transition, then
        //   stops to avoid unnecessary CAN traffic).
        for (auto& [addr, frame] : frames) {
            bool active = hasActiveOutput.count(addr) && hasActiveOutput[addr];
            if (active) {
                _transmitFrame(addr, frame.data());
                _prevFrames[addr] = frame;
            } else {
                auto prev_it = _prevFrames.find(addr);
                bool changed = (prev_it == _prevFrames.end()) ||
                               (prev_it->second != frame);
                if (changed) {
                    _transmitFrame(addr, frame.data());
                    _prevFrames[addr] = frame;
                }
            }
        }
    }
    
private:
    BehaviorEngine* _engine;
    unsigned long _lastTransmit;
    uint16_t _transmitInterval;
    std::map<uint8_t, std::array<uint8_t, 8>> _prevFrames;  // last transmitted frame per module address
    
    void _transmitFrame(uint8_t addr, const uint8_t data[8]) {
        // Use InMotionNGX::sendCommand via makeModuleByAddress
        InMotionNGX::Module m = InMotionNGX::makeModuleByAddress(addr);
        InMotionNGX::sendCommand(m, data);
        
        Serial.printf("[InMotionSynth] Sent frame to addr=%d: %02X %02X %02X %02X %02X %02X %02X %02X\n",
            addr, data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);
    }
};

} // namespace BehavioralOutput
