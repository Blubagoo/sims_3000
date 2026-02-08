/**
 * @file PathwayDecay.h
 * @brief Pathway decay logic for Epic 7 (Ticket E7-025)
 *
 * Implements decay formula:
 *   health -= base_decay * traffic_multiplier (every decay_cycle_ticks)
 *
 * Traffic multiplier: 1.0 + 2.0 * (flow_current / base_capacity), capped at max_traffic_multiplier.
 * Emits PathwayDeterioratedEvent at health threshold crossings.
 *
 * Uses canonical alien terminology per /docs/canon/terminology.yaml
 */

#pragma once

#include <sims3000/transport/RoadComponent.h>
#include <sims3000/transport/TrafficComponent.h>
#include <sims3000/transport/DecayConfig.h>
#include <cstdint>

namespace sims3000 {
namespace transport {

/**
 * @class PathwayDecay
 * @brief Static utility for applying decay to pathway segments.
 *
 * Decay runs every decay_cycle_ticks (default 100). Traffic load increases
 * the decay rate up to max_traffic_multiplier (default 3x).
 *
 * When health crosses a threshold boundary (Pristine->Good, Good->Worn, etc.),
 * apply_decay returns true so the caller can emit a PathwayDeterioratedEvent.
 */
class PathwayDecay {
public:
    /**
     * @brief Apply decay to a single road segment.
     *
     * Reduces health by base_decay_per_cycle * traffic_multiplier.
     * Health is clamped to 0 (cannot go negative).
     *
     * @param road The road component to decay (health will be modified).
     * @param traffic Optional traffic component (nullptr = no traffic load).
     * @param config Decay configuration parameters.
     * @return true if health crossed a threshold boundary (caller should emit event).
     */
    static bool apply_decay(RoadComponent& road, const TrafficComponent* traffic,
                           const DecayConfig& config = DecayConfig{});

    /**
     * @brief Check if decay should run this tick.
     *
     * Decay runs when current_tick is a multiple of decay_cycle_ticks.
     *
     * @param current_tick The current simulation tick.
     * @param config Decay configuration parameters.
     * @return true if decay should be applied this tick.
     */
    static bool should_decay(uint32_t current_tick, const DecayConfig& config = DecayConfig{});

    /**
     * @brief Get traffic multiplier for decay calculation.
     *
     * Formula: 1.0 + 2.0 * (flow_current / base_capacity)
     * Capped at config.max_traffic_multiplier.
     * Returns 1.0 if traffic is nullptr or base_capacity is 0.
     *
     * @param road The road component (for base_capacity).
     * @param traffic Optional traffic component (nullptr = multiplier 1.0).
     * @param config Decay configuration parameters.
     * @return Traffic multiplier in range [1.0, max_traffic_multiplier].
     */
    static float get_traffic_multiplier(const RoadComponent& road, const TrafficComponent* traffic,
                                       const DecayConfig& config = DecayConfig{});
};

} // namespace transport
} // namespace sims3000
