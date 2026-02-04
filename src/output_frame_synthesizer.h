#pragma once

#include "output_behavior_engine.h"
#include "ipm1_can_library.h"
#include <map>
#include <set>
#include <vector>

/**
 * ╔═══════════════════════════════════════════════════════════════════════════╗
 * ║  POWERCELL FRAME SYNTHESIZER                                              ║
 * ║                                                                           ║
 * ║  Merges behavioral output states into complete POWERCELL NGX CAN frames  ║
 * ║                                                                           ║
 * ║  Key Responsibility:                                                      ║
 * ║    - Collect current state from all outputs                              ║
 * ║    - Group by cell address                                               ║
 * ║    - Generate complete 8-byte state frames per cell                      ║
 * ║    - Send frames at consistent intervals                                 ║
 * ║                                                                           ║
 * ║  Critical Rule:                                                           ║
 * ║    NEVER send partial state updates to POWERCELL.                        ║
 * ║    Each frame MUST represent the complete desired output state.          ║
 * ╚═══════════════════════════════════════════════════════════════════════════╝
 */

namespace BehavioralOutput {

// ═══════════════════════════════════════════════════════════════════════════
// CELL STATE ACCUMULATOR
// ═══════════════════════════════════════════════════════════════════════════

struct CellState {
    uint8_t address = 1;
    uint16_t outputBitmap = 0;         // Bits 0-9 for outputs 1-10 (ON/OFF)
    uint16_t softStartBitmap = 0;      // Bits 0-9 for soft-start enable
    uint16_t pwmEnableBitmap = 0;      // Bits 0-9 for PWM enable
    bool hasChanges = false;
};

// ═══════════════════════════════════════════════════════════════════════════
// FRAME SYNTHESIZER
// ═══════════════════════════════════════════════════════════════════════════

class PowercellSynthesizer {
public:
    PowercellSynthesizer(BehaviorEngine* engine, std::function<void(uint32_t, uint8_t*)> sendFunc)
        : _engine(engine), _sendFrame(sendFunc), _lastTransmit(0), _transmitInterval(50) {
        // Initialize cell state cache
        _cellStateCache.clear();
    }
    
    // ───────────────────────────────────────────────────────────────────────
    // CONFIGURATION
    // ───────────────────────────────────────────────────────────────────────
    
    void setTransmitInterval(uint16_t interval_ms) {
        _transmitInterval = interval_ms;
    }
    
    void setForceTransmit(bool force) {
        _forceTransmit = force;
    }
    
    // ───────────────────────────────────────────────────────────────────────
    // FRAME SYNTHESIS & TRANSMISSION
    // ───────────────────────────────────────────────────────────────────────
    
    void update() {
        if (!_engine) return;
        
        unsigned long now = millis();
        
        // Check if it's time to transmit
        if (!_forceTransmit && (now - _lastTransmit < _transmitInterval)) {
            return;
        }
        
        _lastTransmit = now;
        
        // Get all outputs and update cell state cache
        const auto& outputs = _engine->getOutputs();
        std::set<uint8_t> activeCells;
        
        for (const auto& [id, output] : outputs) {
            uint8_t cellAddr = output.cellAddress;
            uint8_t outNum = output.outputNumber;
            
            // Validate output number (1-10 for POWERCELL NGX)
            if (outNum < 1 || outNum > 10) continue;
            
            // Initialize cell state if needed
            if (_cellStateCache.find(cellAddr) == _cellStateCache.end()) {
                CellState state;
                state.address = cellAddr;
                _cellStateCache[cellAddr] = state;
            }
            
            // Update this output's bit in the cache
            CellState& state = _cellStateCache[cellAddr];
            
            // Set/clear output bit (output numbers are 1-based, bits are 0-based)
            uint16_t bitMask = (1 << (outNum - 1));
            if (output.currentState) {
                state.outputBitmap |= bitMask;   // Set bit = ON
            } else {
                state.outputBitmap &= ~bitMask;  // Clear bit = OFF
            }
            
            // Handle soft-start bit
            if (output.softStart) {
                state.softStartBitmap |= bitMask;
            } else {
                state.softStartBitmap &= ~bitMask;
            }
            
            // Handle PWM enable bit
            if (output.pwmEnable) {
                state.pwmEnableBitmap |= bitMask;
            } else {
                state.pwmEnableBitmap &= ~bitMask;
            }
            
            state.hasChanges = true;
            activeCells.insert(cellAddr);
        }
        
        // Transmit frames for all cells that have had any changes
        for (uint8_t addr : activeCells) {
            if (_cellStateCache.find(addr) != _cellStateCache.end()) {
                CellState& state = _cellStateCache[addr];
                if (state.hasChanges || _forceTransmit) {
                    _transmitCellState(state);
                    state.hasChanges = false; // Clear change flag after transmit
                }
            }
        }
    }
    
    // ───────────────────────────────────────────────────────────────────────
    // MANUAL FRAME TRANSMISSION
    // ───────────────────────────────────────────────────────────────────────
    
    void transmitImmediate() {
        _forceTransmit = true;
        update();
        _forceTransmit = false;
    }

private:
    BehaviorEngine* _engine;
    std::function<void(uint32_t, uint8_t*)> _sendFrame;
    
    unsigned long _lastTransmit;
    uint16_t _transmitInterval;
    bool _forceTransmit = false;
    
    // Cell state cache - preserves output values across updates
    std::map<uint8_t, CellState> _cellStateCache;
    
    // ───────────────────────────────────────────────────────────────────────
    // POWERCELL FRAME CONSTRUCTION
    // ───────────────────────────────────────────────────────────────────────
    
    void _transmitCellState(const CellState& state) {
        // Build POWERCELL NGX bitmap frame
        // PGN: 0xFF50 base normalized per cell address
        uint32_t pgn = Ipm1Can::NormalizePowercellPgn(state.address, 0xFF50);
        
        // Data format per POWERCELL NGX specification (8 bytes):
        // Byte 0: Output ON/OFF bitmap (Outputs 1-8)
        // Byte 1: Output ON/OFF bitmap (Outputs 9-10)
        // Byte 2: Soft-Start enable bitmap (Outputs 1-10)
        // Byte 3: PWM enable bitmap (Outputs 1-10)
        // Bytes 4-7: Reserved (0x00)
        uint8_t data[8] = {0};
        
        // Byte 0: Outputs 1-8 ON/OFF (bit 0 = output 1, bit 7 = output 8)
        data[0] = (uint8_t)(state.outputBitmap & 0xFF);
        
        // Byte 1: Outputs 9-10 ON/OFF (bit 0 = output 9, bit 1 = output 10)
        data[1] = (uint8_t)((state.outputBitmap >> 8) & 0x03);
        
        // Byte 2: Soft-start enable bitmap
        // Outputs 1-8 in bits 0-7, outputs 9-10 packed into upper bits
        data[2] = (uint8_t)(state.softStartBitmap & 0xFF);           // Outputs 1-8
        data[2] |= (uint8_t)(((state.softStartBitmap >> 8) & 0x03) << 6);  // Outputs 9-10 in bits 6-7
        
        // Byte 3: PWM enable bitmap
        // Outputs 1-8 in bits 0-7, outputs 9-10 packed into upper bits
        data[3] = (uint8_t)(state.pwmEnableBitmap & 0xFF);           // Outputs 1-8
        data[3] |= (uint8_t)(((state.pwmEnableBitmap >> 8) & 0x03) << 6);  // Outputs 9-10 in bits 6-7
        
        // Bytes 4-7: Reserved (already zeroed)
        
        // Debug: Log frame transmission
        Serial.printf("[POWERCELL] Cell %d → PGN 0x%04X | Byte0=0x%02X Byte1=0x%02X Byte2=0x%02X Byte3=0x%02X\n",
            state.address, pgn, data[0], data[1], data[2], data[3]);
        
        // Send the frame
        if (_sendFrame) {
            _sendFrame(pgn, data);
        }
    }
    
    void _transmitConfigFrame(const CellState& state) {
        // Reserved for future use
    }
};

// ═══════════════════════════════════════════════════════════════════════════
// GENERIC CAN CONTROLLER
// ═══════════════════════════════════════════════════════════════════════════

class GenericCanController {
public:
    GenericCanController(BehaviorEngine* engine, std::function<void(uint32_t, uint8_t*, uint8_t)> sendFunc)
        : _engine(engine), _sendFrame(sendFunc) {}
    
    /**
     * @brief Map an output to a custom CAN frame definition
     * 
     * This allows outputs to control non-POWERCELL devices with arbitrary
     * CAN frame formats. The mapper function receives the current output
     * value (0-255) and returns the CAN ID and data bytes to transmit.
     */
    void mapOutput(const String& outputId, 
                   std::function<void(uint8_t value, uint32_t& canId, uint8_t* data, uint8_t& len)> mapper) {
        _outputMappers[outputId] = mapper;
    }
    
    void update() {
        if (!_engine) return;
        
        const auto& outputs = _engine->getOutputs();
        for (const auto& [id, output] : outputs) {
            if (!output.isActive) continue;
            
            // Check if this output has a custom mapper
            auto it = _outputMappers.find(id);
            if (it != _outputMappers.end()) {
                uint32_t canId = 0;
                uint8_t data[8] = {0};
                uint8_t len = 8;
                
                // Call the mapper
                it->second(output.currentState ? 255 : 0, canId, data, len);
                
                // Send the frame
                if (_sendFrame && canId > 0) {
                    _sendFrame(canId, data, len);
                }
            }
        }
    }

private:
    BehaviorEngine* _engine;
    std::function<void(uint32_t, uint8_t*, uint8_t)> _sendFrame;
    std::map<String, std::function<void(uint8_t, uint32_t&, uint8_t*, uint8_t&)>> _outputMappers;
};

} // namespace BehavioralOutput
