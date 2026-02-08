/**
 * @file DebrisClearSystem.h
 * @brief Debris auto-clear system for removing debris after timer expiry (Epic 4, ticket 4-031)
 *
 * Manages debris lifecycle: auto-clears debris when timer expires,
 * supports manual clearing by overseer with cost deduction.
 * Removes entities from BuildingFactory when debris is cleared.
 *
 * @see /docs/epics/epic-4/tickets.md (ticket 4-031)
 */

#ifndef SIMS3000_BUILDING_DEBRISCLEARSYSTEM_H
#define SIMS3000_BUILDING_DEBRISCLEARSYSTEM_H

#include <sims3000/building/BuildingFactory.h>
#include <sims3000/building/BuildingGrid.h>
#include <sims3000/building/BuildingEvents.h>
#include <sims3000/building/ForwardDependencyInterfaces.h>
#include <cstdint>
#include <vector>

namespace sims3000 {
namespace building {

/**
 * @struct DebrisClearConfig
 * @brief Configuration for debris clearing costs.
 */
struct DebrisClearConfig {
    uint32_t manual_clear_cost = 10; ///< Cost in credits to manually clear debris
};

/**
 * @class DebrisClearSystem
 * @brief Manages debris auto-clearing and manual clearing.
 *
 * Each tick:
 * - Iterates all Deconstructed entities with debris data
 * - Decrements debris timer
 * - When timer reaches 0, removes entity and emits DebrisClearedEvent
 *
 * Manual clearing:
 * - Validates entity exists and is Deconstructed
 * - Deducts cost from player credits
 * - Immediately removes entity
 */
class DebrisClearSystem {
public:
    /**
     * @brief Construct DebrisClearSystem with dependencies.
     *
     * @param factory Pointer to BuildingFactory for entity access and removal.
     * @param grid Pointer to BuildingGrid (not used directly, debris already cleared from grid).
     * @param credits Pointer to ICreditProvider for manual clear cost deduction.
     */
    DebrisClearSystem(BuildingFactory* factory, BuildingGrid* grid,
                      ICreditProvider* credits);

    /**
     * @brief Tick: decrement timers and clear expired debris.
     *
     * Should be called once per simulation tick.
     */
    void tick();

    /**
     * @brief Manually clear debris by overseer.
     *
     * Validates entity exists and is in Deconstructed state with debris,
     * deducts manual_clear_cost from player credits, and removes entity.
     *
     * @param entity_id Entity ID of debris to clear.
     * @param player_id Requesting overseer's PlayerID.
     * @return true if debris was cleared, false otherwise.
     */
    bool handle_clear_debris(uint32_t entity_id, uint8_t player_id);

    /**
     * @brief Set debris clear configuration.
     * @param config The DebrisClearConfig to apply.
     */
    void set_config(const DebrisClearConfig& config);

    /**
     * @brief Get pending debris cleared events.
     * @return Const reference to pending events vector.
     */
    const std::vector<DebrisClearedEvent>& get_pending_events() const;

    /**
     * @brief Clear all pending debris cleared events.
     */
    void clear_pending_events();

private:
    BuildingFactory* m_factory;                         ///< Factory for entity access/removal
    BuildingGrid* m_grid;                               ///< Grid (for reference)
    ICreditProvider* m_credits;                         ///< Credit provider for manual clear
    DebrisClearConfig m_config;                          ///< Configuration
    std::vector<DebrisClearedEvent> m_pending_events;   ///< Pending events
};

} // namespace building
} // namespace sims3000

#endif // SIMS3000_BUILDING_DEBRISCLEARSYSTEM_H
