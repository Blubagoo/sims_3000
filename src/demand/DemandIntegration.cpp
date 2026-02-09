/**
 * @file DemandIntegration.cpp
 * @brief Implementation of demand integration helpers (Ticket E10-049)
 */

#include "sims3000/demand/DemandIntegration.h"
#include "sims3000/building/ForwardDependencyInterfaces.h"
#include <algorithm>

namespace sims3000 {
namespace demand {

bool should_spawn_building(const building::IDemandProvider& provider,
                            uint8_t zone_type,
                            uint32_t player_id) {
    // Use the IDemandProvider's has_positive_demand() method
    return provider.has_positive_demand(zone_type, player_id);
}

int8_t get_growth_pressure(const building::IDemandProvider& provider,
                            uint8_t zone_type,
                            uint32_t player_id) {
    // Get raw demand value
    float demand = provider.get_demand(zone_type, player_id);

    // Clamp to [-100, +100] and convert to int8_t
    int32_t clamped = static_cast<int32_t>(demand);
    clamped = std::max(-100, std::min(100, clamped));

    return static_cast<int8_t>(clamped);
}

bool should_upgrade_building(const building::IDemandProvider& provider,
                              uint8_t zone_type,
                              uint32_t player_id,
                              int8_t threshold) {
    // Get demand value
    float demand = provider.get_demand(zone_type, player_id);

    // Check if demand exceeds threshold
    return demand > static_cast<float>(threshold);
}

bool should_downgrade_building(const building::IDemandProvider& provider,
                                uint8_t zone_type,
                                uint32_t player_id,
                                int8_t threshold) {
    // Get demand value
    float demand = provider.get_demand(zone_type, player_id);

    // Check if demand is below threshold
    return demand < static_cast<float>(threshold);
}

} // namespace demand
} // namespace sims3000
