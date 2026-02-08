/**
 * @file MaintenanceAPI.h
 * @brief Maintenance application API for Epic 7 (Ticket E7-027)
 *
 * Provides apply_maintenance() which restores health to a pathway segment,
 * recalculates capacity, and records the maintenance tick.
 *
 * Health restoration is capped at 255 (maximum).
 *
 * Uses canonical alien terminology per /docs/canon/terminology.yaml
 */

#pragma once

#include <sims3000/transport/RoadComponent.h>
#include <sims3000/transport/CapacityDegradation.h>
#include <cstdint>

namespace sims3000 {
namespace transport {

/**
 * @brief Apply maintenance to restore health.
 *
 * Restores health by health_restored points (capped at 255),
 * recalculates current_capacity from the new health, and updates
 * last_maintained_tick to the current tick.
 *
 * @param road The road component to maintain (health, current_capacity,
 *             last_maintained_tick will be modified).
 * @param health_restored Amount of health to restore (0-255).
 * @param current_tick The current simulation tick.
 */
inline void apply_maintenance(RoadComponent& road, uint8_t health_restored, uint32_t current_tick) {
    uint16_t new_health = static_cast<uint16_t>(road.health) + health_restored;
    road.health = (new_health > 255) ? 255 : static_cast<uint8_t>(new_health);
    update_capacity_from_health(road);
    road.last_maintained_tick = current_tick;
}

} // namespace transport
} // namespace sims3000
