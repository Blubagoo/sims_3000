/**
 * @file BuildingSpawnChecker.h
 * @brief Standalone building spawn precondition checker (Ticket 4-024)
 *
 * BuildingSpawnChecker validates ALL preconditions for spawning a building
 * at a given tile or footprint. Dependencies are injected via constructor.
 *
 * Checks performed (in order):
 * 1. Zone exists and is in Designated state
 * 2. Demand > 0 for zone type
 * 3. BuildingGrid tile not occupied
 * 4. Terrain is buildable
 * 5. Road accessible within Chebyshev distance 3 (CCR-007)
 * 6. Power available (stub)
 * 7. Fluid available (stub)
 *
 * @see /docs/epics/epic-4/tickets.md (ticket 4-024)
 */

#ifndef SIMS3000_BUILDING_BUILDINGSPAWNCHECKER_H
#define SIMS3000_BUILDING_BUILDINGSPAWNCHECKER_H

#include <cstdint>

// Forward declarations
namespace sims3000 {
namespace zone {
    class ZoneSystem;
} // namespace zone
namespace terrain {
    class ITerrainQueryable;
} // namespace terrain
namespace building {
    class BuildingGrid;
    class ITransportProvider;
    class IEnergyProvider;
    class IFluidProvider;
} // namespace building
} // namespace sims3000

namespace sims3000 {
namespace building {

/**
 * @class BuildingSpawnChecker
 * @brief Validates all preconditions for building spawn at a tile or footprint.
 *
 * All dependencies are injected via constructor. Null dependencies are handled
 * gracefully (null terrain skips buildability check, null stubs default to permissive).
 */
class BuildingSpawnChecker {
public:
    /**
     * @brief Construct BuildingSpawnChecker with dependency injection.
     *
     * @param zone_system Pointer to zone system for zone queries.
     * @param building_grid Pointer to building grid for occupancy checks.
     * @param terrain Pointer to terrain query interface for buildability.
     * @param transport Pointer to transport provider for road access (stub).
     * @param energy Pointer to energy provider for power (stub).
     * @param fluid Pointer to fluid provider for fluid (stub).
     */
    BuildingSpawnChecker(
        const zone::ZoneSystem* zone_system,
        const BuildingGrid* building_grid,
        const terrain::ITerrainQueryable* terrain,
        const ITransportProvider* transport,
        const IEnergyProvider* energy,
        const IFluidProvider* fluid
    );

    /**
     * @brief Check if a building can spawn at a single tile.
     *
     * Checks all preconditions in order. Returns true only if ALL pass.
     *
     * @param x X coordinate.
     * @param y Y coordinate.
     * @param player_id Owning overseer ID.
     * @return true if all spawn conditions are met.
     */
    bool can_spawn_building(std::int32_t x, std::int32_t y, std::uint8_t player_id) const;

    /**
     * @brief Check if a building can spawn across a multi-tile footprint.
     *
     * Calls can_spawn_building for ALL tiles in the rectangle.
     * Returns false if any tile fails.
     *
     * @param x Top-left X coordinate.
     * @param y Top-left Y coordinate.
     * @param w Width in tiles.
     * @param h Height in tiles.
     * @param player_id Owning overseer ID.
     * @return true if all tiles pass spawn conditions.
     */
    bool can_spawn_footprint(std::int32_t x, std::int32_t y,
                              std::uint8_t w, std::uint8_t h,
                              std::uint8_t player_id) const;

private:
    const zone::ZoneSystem* m_zone_system;
    const BuildingGrid* m_building_grid;
    const terrain::ITerrainQueryable* m_terrain;
    const ITransportProvider* m_transport;
    const IEnergyProvider* m_energy;
    const IFluidProvider* m_fluid;
};

} // namespace building
} // namespace sims3000

#endif // SIMS3000_BUILDING_BUILDINGSPAWNCHECKER_H
