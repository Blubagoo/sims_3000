/**
 * @file BuildingDowngradeSystem.h
 * @brief Building downgrade system for level regression (Ticket 4-033)
 *
 * Manages building level downgrades based on:
 * - Building must be Active state
 * - Level > min_level
 * - Land value drops below min_land_value for current level (level * 50)
 * - Sustained negative demand for DOWNGRADE_DELAY ticks
 *
 * On downgrade:
 * - Level decremented
 * - Capacity recalculated with level multiplier
 * - BuildingDowngradedEvent emitted
 * - state_changed_tick updated
 * - No credit cost for downgrades
 */

#ifndef SIMS3000_BUILDING_BUILDINGDOWNGRADESYSTEM_H
#define SIMS3000_BUILDING_BUILDINGDOWNGRADESYSTEM_H

#include <sims3000/building/BuildingFactory.h>
#include <sims3000/building/BuildingEvents.h>
#include <cstdint>
#include <array>
#include <vector>

// Forward declaration
namespace sims3000 {
namespace zone {
    class ZoneSystem;
} // namespace zone
} // namespace sims3000

namespace sims3000 {
namespace building {

/**
 * @struct DowngradeConfig
 * @brief Configuration parameters for building downgrade system.
 */
struct DowngradeConfig {
    uint32_t downgrade_delay = 100;  ///< Ticks of sustained negative conditions before downgrade
    uint32_t check_interval = 10;    ///< Check every N ticks
    uint8_t min_level = 1;           ///< Minimum building level (cannot go below this)
    std::array<float, 6> level_multipliers = {0.0f, 1.0f, 1.5f, 2.0f, 2.5f, 3.0f}; ///< Index 0 unused
};

/**
 * @class BuildingDowngradeSystem
 * @brief Manages building level downgrades based on conditions.
 *
 * Each tick (at check_interval), evaluates all Active buildings for
 * downgrade eligibility. When conditions are met, decrements level,
 * recalculates capacity, and emits BuildingDowngradedEvent.
 */
class BuildingDowngradeSystem {
public:
    /**
     * @brief Construct BuildingDowngradeSystem with dependency injection.
     *
     * @param factory Pointer to BuildingFactory for entity access.
     * @param zone_system Pointer to ZoneSystem for demand queries.
     */
    BuildingDowngradeSystem(BuildingFactory* factory, zone::ZoneSystem* zone_system);

    /**
     * @brief Process downgrade checks for all building entities.
     *
     * Called each simulation tick. Only performs checks at check_interval.
     *
     * @param current_tick Current simulation tick.
     */
    void tick(uint32_t current_tick);

    /**
     * @brief Set downgrade configuration.
     * @param config The DowngradeConfig to apply.
     */
    void set_config(const DowngradeConfig& config);

    /**
     * @brief Get current downgrade configuration.
     * @return Const reference to current DowngradeConfig.
     */
    const DowngradeConfig& get_config() const;

    /**
     * @brief Get pending downgrade events.
     * @return Const reference to pending downgrade events vector.
     */
    const std::vector<BuildingDowngradedEvent>& get_pending_events() const;

    /**
     * @brief Clear all pending downgrade events.
     */
    void clear_pending_events();

private:
    BuildingFactory* m_factory;
    zone::ZoneSystem* m_zone_system;
    DowngradeConfig m_config;
    std::vector<BuildingDowngradedEvent> m_pending_events;

    /**
     * @brief Check if a building entity meets downgrade conditions.
     *
     * Conditions (either triggers downgrade):
     * - Land value below level * 50 threshold
     * - Sustained negative demand for downgrade_delay ticks
     *
     * @param entity The building entity to check.
     * @param current_tick Current simulation tick.
     * @return true if conditions are met.
     */
    bool check_downgrade_conditions(const BuildingEntity& entity, uint32_t current_tick) const;

    /**
     * @brief Execute a downgrade on a building entity.
     *
     * - Decrements level
     * - Recalculates capacity: base_capacity * level_multipliers[level]
     * - Emits BuildingDowngradedEvent
     * - Updates state_changed_tick
     *
     * @param entity The building entity to downgrade.
     * @param current_tick Current simulation tick.
     */
    void execute_downgrade(BuildingEntity& entity, uint32_t current_tick);
};

} // namespace building
} // namespace sims3000

#endif // SIMS3000_BUILDING_BUILDINGDOWNGRADESYSTEM_H
