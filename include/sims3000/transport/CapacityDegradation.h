/**
 * @file CapacityDegradation.h
 * @brief Capacity degradation from health for Epic 7 (Ticket E7-026)
 *
 * Provides update_capacity_from_health() which scales a pathway's
 * current_capacity linearly based on its health value (0-255).
 *
 * - health=255: current_capacity == base_capacity (full)
 * - health=0:   current_capacity == 0 (zero capacity)
 *
 * Uses canonical alien terminology per /docs/canon/terminology.yaml
 */

#pragma once

#include <sims3000/transport/RoadComponent.h>
#include <cstdint>

namespace sims3000 {
namespace transport {

/**
 * @brief Update current_capacity based on health.
 *
 * Capacity scales linearly with health:
 *   current_capacity = (base_capacity * health) / 255
 *
 * @param road The road component to update (current_capacity will be modified).
 */
inline void update_capacity_from_health(RoadComponent& road) {
    road.current_capacity = static_cast<uint16_t>(
        (static_cast<uint32_t>(road.base_capacity) * road.health) / 255
    );
}

} // namespace transport
} // namespace sims3000
