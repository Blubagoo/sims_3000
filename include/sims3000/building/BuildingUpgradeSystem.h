/**
 * @file BuildingUpgradeSystem.h
 * @brief Building upgrade system for level progression (Ticket 4-032)
 *
 * Manages building level upgrades based on:
 * - Building must be Active state
 * - Level < max_level
 * - Cooldown elapsed since last state change
 * - Positive demand for zone type
 * - Desirability threshold met
 *
 * On upgrade:
 * - Level incremented
 * - Capacity recalculated with level multiplier
 * - BuildingUpgradedEvent emitted
 * - state_changed_tick updated for cooldown tracking
 */

#ifndef SIMS3000_BUILDING_BUILDINGUPGRADESYSTEM_H
#define SIMS3000_BUILDING_BUILDINGUPGRADESYSTEM_H

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
 * @struct UpgradeConfig
 * @brief Configuration parameters for building upgrade system.
 */
struct UpgradeConfig {
    uint32_t upgrade_cooldown = 200;     ///< Ticks at current level before eligible
    uint32_t check_interval = 10;        ///< Check every N ticks
    uint8_t max_level = 5;               ///< Maximum building level
    std::array<float, 6> level_multipliers = {0.0f, 1.0f, 1.5f, 2.0f, 2.5f, 3.0f}; ///< Index 0 unused
    uint16_t upgrade_animation_ticks = 20; ///< Duration of upgrade animation
};

/**
 * @class BuildingUpgradeSystem
 * @brief Manages building level upgrades based on conditions.
 *
 * Each tick (at check_interval), evaluates all Active buildings for
 * upgrade eligibility. When conditions are met, increments level,
 * recalculates capacity, and emits BuildingUpgradedEvent.
 */
class BuildingUpgradeSystem {
public:
    /**
     * @brief Construct BuildingUpgradeSystem with dependency injection.
     *
     * @param factory Pointer to BuildingFactory for entity access.
     * @param zone_system Pointer to ZoneSystem for demand/desirability queries.
     */
    BuildingUpgradeSystem(BuildingFactory* factory, zone::ZoneSystem* zone_system);

    /**
     * @brief Process upgrade checks for all building entities.
     *
     * Called each simulation tick. Only performs checks at check_interval.
     *
     * @param current_tick Current simulation tick.
     */
    void tick(uint32_t current_tick);

    /**
     * @brief Set upgrade configuration.
     * @param config The UpgradeConfig to apply.
     */
    void set_config(const UpgradeConfig& config);

    /**
     * @brief Get current upgrade configuration.
     * @return Const reference to current UpgradeConfig.
     */
    const UpgradeConfig& get_config() const;

    /**
     * @brief Get pending upgrade events.
     * @return Const reference to pending upgrade events vector.
     */
    const std::vector<BuildingUpgradedEvent>& get_pending_events() const;

    /**
     * @brief Clear all pending upgrade events.
     */
    void clear_pending_events();

private:
    BuildingFactory* m_factory;
    zone::ZoneSystem* m_zone_system;
    UpgradeConfig m_config;
    std::vector<BuildingUpgradedEvent> m_pending_events;

    /**
     * @brief Check if a building entity meets upgrade conditions.
     *
     * Conditions:
     * - State is Active
     * - Level < max_level
     * - Time at current level > upgrade_cooldown
     * - Demand positive for zone_type
     * - Desirability >= level * 50
     *
     * @param entity The building entity to check.
     * @param current_tick Current simulation tick.
     * @return true if all conditions are met.
     */
    bool check_upgrade_conditions(const BuildingEntity& entity, uint32_t current_tick) const;

    /**
     * @brief Execute an upgrade on a building entity.
     *
     * - Increments level
     * - Recalculates capacity: base_capacity * level_multipliers[level]
     * - Emits BuildingUpgradedEvent
     * - Updates state_changed_tick
     *
     * @param entity The building entity to upgrade.
     * @param current_tick Current simulation tick.
     */
    void execute_upgrade(BuildingEntity& entity, uint32_t current_tick);
};

} // namespace building
} // namespace sims3000

#endif // SIMS3000_BUILDING_BUILDINGUPGRADESYSTEM_H
