#pragma once

#include <cstdint>
#include <cstddef>

namespace sims3000::sim {
    // Header for serialized simulation state
#pragma pack(push, 1)
    struct SimulationStateHeader {
        uint32_t magic;            // 'SIM3' = 0x33494D53
        uint32_t version;          // Format version = 1
        uint64_t tick_count;       // Current tick
        uint32_t cycle;            // Current cycle
        uint16_t grid_width;
        uint16_t grid_height;
        uint8_t phase;             // Current phase
        uint8_t speed;             // SimulationSpeed as uint8_t
        uint8_t num_players;
        uint8_t reserved;          // Padding
    };
#pragma pack(pop)

    static_assert(sizeof(SimulationStateHeader) == 28, "Header must be 28 bytes");

    // Serialize simulation state header to buffer
    size_t serialize_header(const SimulationStateHeader& header, uint8_t* buffer, size_t buffer_size);

    // Deserialize header from buffer
    bool deserialize_header(const uint8_t* buffer, size_t size, SimulationStateHeader& out);

    // Validate a header
    bool validate_header(const SimulationStateHeader& header);

    // Create header from current simulation state
    SimulationStateHeader create_header(uint64_t tick, uint32_t cycle, uint8_t phase,
                                          uint8_t speed, uint16_t grid_w, uint16_t grid_h,
                                          uint8_t num_players);
}
