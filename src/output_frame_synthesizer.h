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

inline uint16_t g_lastMastercellBitmap = 0;
inline uint8_t g_lastMastercellData0 = 0;
inline bool g_lastMastercellValid = false;

// ═══════════════════════════════════════════════════════════════════════════
// CELL STATE ACCUMULATOR
// ═══════════════════════════════════════════════════════════════════════════

struct CellState {
    uint8_t address = 1;
    uint16_t outputBitmap = 0;         // Bits 0-9 for outputs 1-10 (ON/OFF) - Track mode
    bool hasChanges = false;
    // Note: softStart and PWM not implemented yet (Track-only mode)
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
        if (!_forceTransmit && (now - _lastTransmit < _transmitInterval)) {
            return;
        }
        _lastTransmit = now;
        
        const auto& outputs = _engine->getOutputs();
        std::map<uint8_t, uint16_t> nextBitmaps;
        std::map<uint8_t, bool> cellHasActive;
        
        // Build complete desired state per cell
        for (const auto& [id, output] : outputs) {
            uint8_t cellAddr = output.cellAddress;
            uint8_t outNum = output.outputNumber;
            
            // Validate cell address and output number
            if (cellAddr < 1 || cellAddr > 16) continue;
            if (outNum < 1 || outNum > 10) continue;
            
            // Ensure cell entry exists
            uint16_t& bitmap = nextBitmaps[cellAddr];
            if (output.isActive) {
                cellHasActive[cellAddr] = true;
            }

            if (output.currentState) {
                bitmap |= static_cast<uint16_t>(1u << (outNum - 1));
            }
        }
        
        // Transmit for cells with active outputs (continuous) or just-deactivated outputs (one final OFF)
        for (const auto& [addr, bitmap] : nextBitmaps) {
            CellState& state = _cellStateCache[addr];
            state.address = addr;

            const uint16_t previous = state.outputBitmap;
            state.outputBitmap = bitmap;
            state.hasChanges = (state.outputBitmap != previous);

            const bool hasActive = (cellHasActive.find(addr) != cellHasActive.end());
            const bool hadActive = (_cellActiveCache.find(addr) != _cellActiveCache.end()) && _cellActiveCache[addr];
            const bool shouldTransmit = _forceTransmit || hasActive || hadActive;

            if (shouldTransmit) {
                _transmitCellState(state);
            }

            _cellActiveCache[addr] = hasActive;
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
    std::map<uint8_t, bool> _cellActiveCache;
    
    // ───────────────────────────────────────────────────────────────────────
    // POWERCELL FRAME CONSTRUCTION
    // ───────────────────────────────────────────────────────────────────────
    
    void _transmitCellState(const CellState& state) {
        // Guard against invalid address (prevents accidental FF00 PGN)
        if (state.address < 1 || state.address > 16) return;
        
        // POWERCELL NGX Track control PGN: 0xFF00 + cellAddress
        // Cell 1 → 0xFF01, Cell 2 → 0xFF02, etc.
        uint32_t pgn = 0xFF00 + state.address;
        
        // Track personality frame format:
        // - First 10 bits control outputs 1-10 in order
        // - Byte 0 uses MSB-first mapping: OUT1=bit7 ... OUT8=bit0
        // - Byte 1 uses bits 7-6 for OUT9/OUT10
        // - Bytes 2-7 must be 0x00 (Track supersedes Soft-Start and PWM)
        uint8_t data[8] = {0};
        for (uint8_t out = 1; out <= 8; ++out) {
            if (state.outputBitmap & (1u << (out - 1))) {
                data[0] |= static_cast<uint8_t>(1u << (8 - out));
            }
        }
        if (state.outputBitmap & (1u << 8)) {
            data[1] |= 0x80;  // OUT9
        }
        if (state.outputBitmap & (1u << 9)) {
            data[1] |= 0x40;  // OUT10
        }
        // data[2..7] remain 0x00 for Track-only mode
        
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
