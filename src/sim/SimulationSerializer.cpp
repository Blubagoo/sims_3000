#include <sims3000/sim/SimulationSerializer.h>
#include <cstring>

namespace sims3000::sim {

    constexpr uint32_t MAGIC_NUMBER = 0x33494D53;  // 'SIM3'
    constexpr uint32_t FORMAT_VERSION = 1;

    size_t serialize_header(const SimulationStateHeader& header, uint8_t* buffer, size_t buffer_size) {
        if (buffer_size < sizeof(SimulationStateHeader)) {
            return 0;  // Buffer too small
        }

        // Simple memcpy since header is POD
        std::memcpy(buffer, &header, sizeof(SimulationStateHeader));
        return sizeof(SimulationStateHeader);
    }

    bool deserialize_header(const uint8_t* buffer, size_t size, SimulationStateHeader& out) {
        if (size < sizeof(SimulationStateHeader)) {
            return false;  // Buffer too small
        }

        // Simple memcpy since header is POD
        std::memcpy(&out, buffer, sizeof(SimulationStateHeader));
        return true;
    }

    bool validate_header(const SimulationStateHeader& header) {
        // Check magic number
        if (header.magic != MAGIC_NUMBER) {
            return false;
        }

        // Check version
        if (header.version != FORMAT_VERSION) {
            return false;
        }

        // Check grid dimensions (must be non-zero and reasonable)
        if (header.grid_width == 0 || header.grid_height == 0) {
            return false;
        }

        // Check grid dimensions are not unreasonably large (max 65535 is uint16_t max, but let's be reasonable)
        if (header.grid_width > 10000 || header.grid_height > 10000) {
            return false;
        }

        // Check number of players is reasonable (1-8 players typical)
        if (header.num_players == 0 || header.num_players > 16) {
            return false;
        }

        // Phase should be less than some reasonable maximum (e.g., 10 phases per cycle)
        if (header.phase > 10) {
            return false;
        }

        // Speed should be a valid enum value (typically 0-4: Paused, Slow, Normal, Fast, VeryFast)
        if (header.speed > 10) {
            return false;
        }

        return true;
    }

    SimulationStateHeader create_header(uint64_t tick, uint32_t cycle, uint8_t phase,
                                          uint8_t speed, uint16_t grid_w, uint16_t grid_h,
                                          uint8_t num_players) {
        SimulationStateHeader header;
        header.magic = MAGIC_NUMBER;
        header.version = FORMAT_VERSION;
        header.tick_count = tick;
        header.cycle = cycle;
        header.phase = phase;
        header.speed = speed;
        header.grid_width = grid_w;
        header.grid_height = grid_h;
        header.num_players = num_players;
        header.reserved = 0;
        return header;
    }
}
