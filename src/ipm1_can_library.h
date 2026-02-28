#pragma once

#include <cstdint>
#include "config_types.h"

namespace Ipm1Can {

constexpr std::uint8_t kSourceAddress = 0x63;
constexpr std::uint8_t kBroadcastAddress = 0xFF;

inline std::uint32_t NormalizePowercellPgn(std::uint8_t cell_address, std::uint32_t base) {
    return base + cell_address;
}

inline CanFrameConfig powercellConfig(std::uint8_t cell_address, std::uint8_t config_byte = 0x01) {
    CanFrameConfig frame;
    frame.enabled = true;
    frame.pgn = NormalizePowercellPgn(cell_address, 0xFF40);
    frame.priority = 6;
    frame.source_address = kSourceAddress;
    frame.destination_address = kBroadcastAddress;
    frame.data = {0x99, config_byte, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    frame.length = 8;
    return frame;
}

inline CanFrameConfig powercellOutput(std::uint8_t cell_address, std::uint8_t output, std::uint8_t value) {
    CanFrameConfig frame;
    frame.enabled = true;
    frame.pgn = NormalizePowercellPgn(cell_address, 0xFF00);
    frame.priority = 6;
    frame.source_address = kSourceAddress;
    frame.destination_address = kBroadcastAddress;
    frame.data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    frame.length = 8;

    const bool on = (value > 0);
    if (on) {
        if (output >= 1 && output <= 8) {
            frame.data[0] |= static_cast<std::uint8_t>(1u << (8 - output));
        } else if (output == 9) {
            frame.data[1] |= 0x80;
        } else if (output == 10) {
            frame.data[1] |= 0x40;
        }
    }

    return frame;
}

inline CanFrameConfig powercellPoll(std::uint8_t cell_address) {
    CanFrameConfig frame;
    frame.enabled = true;
    frame.pgn = NormalizePowercellPgn(cell_address, 0xFF00);
    frame.priority = 6;
    frame.source_address = kSourceAddress;
    frame.destination_address = kBroadcastAddress;
    frame.data = {0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    frame.length = 8;
    return frame;
}

}  // namespace Ipm1Can
