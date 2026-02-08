/**
 * @file BuildingSpawningLoop.h
 * @brief Building spawning loop for zone-based building creation (Ticket 4-026)
 *
 * Implements the main spawning loop that scans designated zones and spawns
 * buildings when preconditions are met. Each overseer gets staggered scans
 * to distribute CPU load across ticks.
 *
 * @see /docs/epics/epic-4/tickets.md (ticket 4-026)
 */

#ifndef SIMS3000_BUILDING_BUILDINGSPAWNINGLOOP_H
#define SIMS3000_BUILDING_BUILDINGSPAWNINGLOOP_H

#include <sims3000/building/BuildingFactory.h>
#include <sims3000/building/BuildingSpawnChecker.h>
#include <sims3000/building/BuildingTemplate.h>
#include <sims3000/building/BuildingGrid.h>
#include <sims3000/building/TemplateSelector.h>
#include <cstdint>

// Forward declarations
namespace sims3000 {
namespace zone {
    class ZoneSystem;
} // namespace zone
} // namespace sims3000

namespace sims3000 {
namespace building {

/**
 * @struct SpawningConfig
 * @brief Configuration parameters for the building spawning loop.
 */
struct SpawningConfig {
    uint32_t scan_interval = 20;      ///< Ticks between scans
    uint32_t max_spawns_per_scan = 3; ///< Cap per overseer per scan
    uint32_t stagger_offset = 5;      ///< Tick offset per player_id
};

/**
 * @class BuildingSpawningLoop
 * @brief Scans designated zones and spawns buildings when preconditions are met.
 *
 * Each tick, checks if any overseer's staggered scan interval has arrived.
 * For eligible overseers, iterates the zone grid looking for Designated zones
 * where buildings can spawn, selects a template, and creates the building.
 */
class BuildingSpawningLoop {
public:
    /**
     * @brief Construct BuildingSpawningLoop with dependency injection.
     *
     * @param factory Pointer to BuildingFactory for entity creation.
     * @param checker Pointer to BuildingSpawnChecker for precondition validation.
     * @param registry Pointer to BuildingTemplateRegistry for template lookup.
     * @param zone_system Pointer to ZoneSystem for zone queries.
     * @param grid Pointer to BuildingGrid for spatial queries.
     */
    BuildingSpawningLoop(
        BuildingFactory* factory,
        BuildingSpawnChecker* checker,
        const BuildingTemplateRegistry* registry,
        zone::ZoneSystem* zone_system,
        BuildingGrid* grid
    );

    /**
     * @brief Call each simulation tick.
     *
     * For each overseer, checks if their staggered scan interval has arrived.
     * If so, scans designated zones and spawns buildings up to the per-scan cap.
     *
     * @param current_tick Current simulation tick.
     */
    void tick(uint32_t current_tick);

    /**
     * @brief Set spawning configuration.
     * @param config The SpawningConfig to apply.
     */
    void set_config(const SpawningConfig& config);

    /**
     * @brief Get current spawning configuration.
     * @return Const reference to current SpawningConfig.
     */
    const SpawningConfig& get_config() const;

    /**
     * @brief Get total number of buildings spawned since creation.
     * @return Total spawn count.
     */
    uint32_t get_total_spawned() const;

private:
    BuildingFactory* m_factory;
    BuildingSpawnChecker* m_checker;
    const BuildingTemplateRegistry* m_registry;
    zone::ZoneSystem* m_zone_system;
    BuildingGrid* m_grid;
    SpawningConfig m_config;
    uint32_t m_total_spawned = 0;

    /**
     * @brief Scan and spawn buildings for a single overseer.
     *
     * Iterates the zone grid looking for Designated zones owned by this player.
     * For each valid position, checks spawn preconditions, selects a template,
     * and spawns the building. Stops after max_spawns_per_scan.
     *
     * @param player_id Overseer ID to scan for.
     * @param current_tick Current simulation tick.
     */
    void scan_for_overseer(uint8_t player_id, uint32_t current_tick);
};

} // namespace building
} // namespace sims3000

#endif // SIMS3000_BUILDING_BUILDINGSPAWNINGLOOP_H
