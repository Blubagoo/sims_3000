/**
 * @file DemandIntegration.h
 * @brief Demand integration helper for BuildingSystem (Ticket E10-049)
 *
 * Provides helper functions for BuildingSystem to query DemandSystem
 * via the IDemandProvider interface for building spawn/upgrade/downgrade
 * decisions.
 *
 * This module replaces the stub demand provider in BuildingSystem with
 * real demand-driven building growth logic.
 */

#ifndef SIMS3000_DEMAND_DEMANDINTEGRATION_H
#define SIMS3000_DEMAND_DEMANDINTEGRATION_H

#include <cstdint>

// Forward declaration to avoid circular include
namespace sims3000 {
namespace building {
    class IDemandProvider;
} // namespace building
} // namespace sims3000

namespace sims3000 {
namespace demand {

/**
 * @brief Check if a building should spawn based on demand.
 *
 * Uses IDemandProvider::has_positive_demand() to determine if there is
 * growth pressure for this zone type. BuildingSystem should call this
 * before spawning new zone buildings.
 *
 * @param provider The demand provider (typically DemandSystem).
 * @param zone_type Zone type (0=Habitation, 1=Exchange, 2=Fabrication).
 * @param player_id Player ID (0-3).
 * @return true if demand > 0 for this zone type, false otherwise.
 */
bool should_spawn_building(const building::IDemandProvider& provider,
                            uint8_t zone_type,
                            uint32_t player_id);

/**
 * @brief Get growth pressure for a zone type.
 *
 * Returns the current demand value clamped to [-100, +100] range.
 * Positive values indicate growth pressure, negative values indicate
 * decline pressure.
 *
 * @param provider The demand provider (typically DemandSystem).
 * @param zone_type Zone type (0=Habitation, 1=Exchange, 2=Fabrication).
 * @param player_id Player ID (0-3).
 * @return Demand value clamped to [-100, +100].
 */
int8_t get_growth_pressure(const building::IDemandProvider& provider,
                            uint8_t zone_type,
                            uint32_t player_id);

/**
 * @brief Check if a building should upgrade based on demand.
 *
 * Buildings should upgrade when demand exceeds a threshold (default 50).
 * This indicates strong growth pressure for this zone type.
 *
 * @param provider The demand provider (typically DemandSystem).
 * @param zone_type Zone type (0=Habitation, 1=Exchange, 2=Fabrication).
 * @param player_id Player ID (0-3).
 * @param threshold Demand threshold for upgrade (default 50).
 * @return true if demand > threshold, false otherwise.
 */
bool should_upgrade_building(const building::IDemandProvider& provider,
                              uint8_t zone_type,
                              uint32_t player_id,
                              int8_t threshold = 50);

/**
 * @brief Check if a building should downgrade based on demand.
 *
 * Buildings should downgrade when demand falls below a threshold (default -50).
 * This indicates strong decline pressure for this zone type.
 *
 * @param provider The demand provider (typically DemandSystem).
 * @param zone_type Zone type (0=Habitation, 1=Exchange, 2=Fabrication).
 * @param player_id Player ID (0-3).
 * @param threshold Demand threshold for downgrade (default -50).
 * @return true if demand < threshold, false otherwise.
 */
bool should_downgrade_building(const building::IDemandProvider& provider,
                                uint8_t zone_type,
                                uint32_t player_id,
                                int8_t threshold = -50);

} // namespace demand
} // namespace sims3000

#endif // SIMS3000_DEMAND_DEMANDINTEGRATION_H
