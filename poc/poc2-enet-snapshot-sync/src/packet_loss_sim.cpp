#include "packet_loss_sim.h"

PacketLossSim::PacketLossSim(uint32_t loss_percent, uint32_t seed)
    : loss_percent_(loss_percent)
    , rng_state_(seed)
{
}

uint32_t PacketLossSim::next_rng() {
    rng_state_ ^= rng_state_ << 13;
    rng_state_ ^= rng_state_ >> 17;
    rng_state_ ^= rng_state_ << 5;
    return rng_state_;
}

bool PacketLossSim::should_drop() {
    if (loss_percent_ == 0) return false;
    return (next_rng() % 100) < loss_percent_;
}
