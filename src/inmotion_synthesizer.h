#pragma once

#include "output_behavior_engine.h"
#include "inmotion_can.h"
#include <map>

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
        
        // Group outputs by module address and build command frames
        std::map<uint8_t, uint8_t[8]> frames;  // addr -> 8-byte command
        std::map<uint8_t, bool> hasChanges;
        
        // Initialize all frames to "don't care" (0x00 = no modifier bit)
        for (const auto& [id, output] : outputs) {
            if (output.deviceType != "INMOTION") continue;
            if (!frames.count(output.cellAddress)) {
                for (int i = 0; i < 8; i++) {
                    frames[output.cellAddress][i] = 0x00;
                }
            }
        }
        
        // Apply each managed output to its byte position
        for (const auto& [id, output] : outputs) {
            if (output.deviceType != "INMOTION") continue;
            if (!output.isActive && !output.currentState) continue;  // Skip inactive/off outputs
            
            uint8_t addr = output.cellAddress;
            uint8_t outNum = output.outputNumber;
            
            if (addr < 1 || addr > 16) continue;
            if (outNum < 1 || outNum > 8) continue;
            
            uint8_t* frame = frames[addr];
            uint8_t byteIndex = 0;
            uint8_t cmd = 0x00;
            
            // Map output number to byte index
            // 1=R1A(byte0), 2=R1B(byte1), 3=R2A(byte2), 4=R2B(byte3), 5-8=Out1-4(bytes4-7)
            if (outNum >= 1 && outNum <= 4) {
                byteIndex = outNum - 1;  // Relays: bytes 0-3
            } else if (outNum >= 5 && outNum <= 8) {
                byteIndex = outNum - 1;  // Outputs: bytes 4-7
            }
            
            // Determine command byte based on behavior type and state
            if (!output.isActive || !output.currentState) {
                cmd = InMotionNGX::CMD_OFF;  // 0x80
            } else if (output.behavior.type == BehaviorType::EXPRESS) {
                cmd = InMotionNGX::CMD_EXPRESS;  // 0xB0
            } else if (output.behavior.type == BehaviorType::PULSE && output.behavior.duration_ms > 0) {
                // Timed pulse: convert duration_ms to 0.25s steps
                uint8_t quarter_secs = (output.behavior.duration_ms + 124) / 250;
                cmd = InMotionNGX::cmdTimed(quarter_secs);
            } else {
                // Default: TRACK mode (stays on until turned off)
                cmd = InMotionNGX::CMD_TRACK;  // 0x90
            }
            
            // Check if this byte changed
            if (frame[byteIndex] != cmd) {
                frame[byteIndex] = cmd;
                hasChanges[addr] = true;
            }
        }
        
        // Transmit frames that have changes
        for (const auto& [addr, changed] : hasChanges) {
            if (changed) {
                _transmitFrame(addr, frames[addr]);
            }
        }
    }
    
private:
    BehaviorEngine* _engine;
    unsigned long _lastTransmit;
    uint16_t _transmitInterval;
    
    void _transmitFrame(uint8_t addr, const uint8_t data[8]) {
        // Use InMotionNGX::sendCommand via makeModuleByAddress
        InMotionNGX::Module m = InMotionNGX::makeModuleByAddress(addr);
        InMotionNGX::sendCommand(m, data);
        
        Serial.printf("[InMotionSynth] Sent frame to addr=%d: %02X %02X %02X %02X %02X %02X %02X %02X\n",
            addr, data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);
    }
};

} // namespace BehavioralOutput
