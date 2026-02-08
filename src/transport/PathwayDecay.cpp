/**
 * @file PathwayDecay.cpp
 * @brief Pathway decay logic implementation for Epic 7 (Ticket E7-025)
 *
 * Implements the decay formula and threshold crossing detection.
 */

#include <sims3000/transport/PathwayDecay.h>
#include <algorithm>

namespace sims3000 {
namespace transport {

bool PathwayDecay::apply_decay(RoadComponent& road, const TrafficComponent* traffic,
                               const DecayConfig& config) {
    if (road.health == 0) {
        return false;  // Already at minimum health
    }

    // Get the health state before decay
    PathwayHealthState state_before = get_health_state(road.health);

    // Calculate decay amount: base_decay * traffic_multiplier
    float multiplier = get_traffic_multiplier(road, traffic, config);
    float decay_amount = static_cast<float>(config.base_decay_per_cycle) * multiplier;

    // Apply decay, clamping to 0
    uint8_t decay_int = static_cast<uint8_t>(std::min(decay_amount, 255.0f));
    if (decay_int == 0 && decay_amount > 0.0f) {
        decay_int = 1;  // Ensure at least 1 point of decay if any decay should occur
    }

    if (decay_int >= road.health) {
        road.health = 0;
    } else {
        road.health -= decay_int;
    }

    // Check if health crossed a threshold boundary
    PathwayHealthState state_after = get_health_state(road.health);
    return state_before != state_after;
}

bool PathwayDecay::should_decay(uint32_t current_tick, const DecayConfig& config) {
    if (config.decay_cycle_ticks == 0) {
        return false;  // Avoid division by zero
    }
    return (current_tick % config.decay_cycle_ticks) == 0;
}

float PathwayDecay::get_traffic_multiplier(const RoadComponent& road, const TrafficComponent* traffic,
                                           const DecayConfig& config) {
    if (traffic == nullptr || road.base_capacity == 0) {
        return 1.0f;
    }

    float flow_ratio = static_cast<float>(traffic->flow_current) / static_cast<float>(road.base_capacity);
    float multiplier = 1.0f + 2.0f * flow_ratio;

    // Cap at max_traffic_multiplier
    float max_mult = static_cast<float>(config.max_traffic_multiplier);
    return std::min(multiplier, max_mult);
}

} // namespace transport
} // namespace sims3000
