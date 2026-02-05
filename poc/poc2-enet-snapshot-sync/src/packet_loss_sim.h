#pragma once

#include <cstdint>

class PacketLossSim {
public:
    // loss_percent: 0-100, chance of dropping each packet
    explicit PacketLossSim(uint32_t loss_percent = 0, uint32_t seed = 12345);

    // Returns true if the packet should be dropped
    bool should_drop();

    uint32_t loss_percent() const { return loss_percent_; }

private:
    uint32_t loss_percent_;
    uint32_t rng_state_;

    uint32_t next_rng();
};
