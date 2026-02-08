/**
 * @file DemolitionHandler.h
 * @brief Overseer demolition handler for building deconstruction (Epic 4, ticket 4-030)
 *
 * Handles player-initiated and system-initiated building demolition.
 * Validates ownership, calculates demolition cost based on building state,
 * deducts credits, transitions building to Deconstructed state, and
 * creates debris.
 *
 * @see /docs/epics/epic-4/tickets.md (ticket 4-030)
 */

#ifndef SIMS3000_BUILDING_DEMOLITIONHANDLER_H
#define SIMS3000_BUILDING_DEMOLITIONHANDLER_H

#include <sims3000/building/BuildingFactory.h>
#include <sims3000/building/BuildingGrid.h>
#include <sims3000/building/BuildingEvents.h>
#include <sims3000/building/ForwardDependencyInterfaces.h>
#include <cstdint>
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
 * @struct DemolitionCostConfig
 * @brief Configuration for demolition cost calculation.
 *
 * Demolition cost = construction_cost * base_cost_ratio * state_modifier
 * State modifiers:
 * - Materializing: 50% (partial refund for cancellation)
 * - Active: 100% (full demolition cost)
 * - Abandoned: 10% (mostly neglected)
 * - Derelict: 0% (free demolition, building is worthless)
 */
struct DemolitionCostConfig {
    float base_cost_ratio = 0.25f;          ///< 25% of construction cost
    float materializing_modifier = 0.5f;    ///< Modifier for Materializing state
    float active_modifier = 1.0f;           ///< Modifier for Active state
    float abandoned_modifier = 0.1f;        ///< Modifier for Abandoned state
    float derelict_modifier = 0.0f;         ///< Modifier for Derelict state (free)
};

/**
 * @struct DemolitionResult
 * @brief Result of a demolition attempt.
 *
 * Contains success flag, cost deducted, and reason for failure.
 */
struct DemolitionResult {
    bool success;           ///< Whether demolition succeeded
    uint32_t cost;          ///< Cost deducted (0 if failed)

    /// Reason for demolition result
    enum class Reason : uint8_t {
        Ok = 0,                     ///< Demolition succeeded
        EntityNotFound,             ///< Entity ID not found in factory
        NotOwned,                   ///< Entity not owned by requesting player
        AlreadyDeconstructed,       ///< Entity already in Deconstructed state
        InsufficientCredits         ///< Player lacks credits for demolition
    } reason;

    DemolitionResult() : success(false), cost(0), reason(Reason::EntityNotFound) {}
};

/**
 * @class DemolitionHandler
 * @brief Handles building demolition requests.
 *
 * Supports two demolition flows:
 * 1. Player-initiated: handle_demolish(entity_id, player_id)
 *    - Validates ownership, calculates cost, deducts credits
 * 2. System-initiated (de-zone flow): handle_demolition_request(grid_x, grid_y)
 *    - Finds entity by grid position, demolishes without ownership check
 */
class DemolitionHandler {
public:
    /**
     * @brief Construct DemolitionHandler with dependencies.
     *
     * @param factory Pointer to BuildingFactory for entity access.
     * @param grid Pointer to BuildingGrid for footprint clearing.
     * @param credits Pointer to ICreditProvider for cost deduction.
     * @param zone_system Pointer to ZoneSystem for zone state updates.
     */
    DemolitionHandler(BuildingFactory* factory, BuildingGrid* grid,
                      ICreditProvider* credits, zone::ZoneSystem* zone_system);

    /**
     * @brief Handle player-initiated demolition.
     *
     * Validates ownership, calculates state-dependent cost, deducts credits,
     * and transitions building to Deconstructed state with debris.
     *
     * @param entity_id Entity ID of building to demolish.
     * @param player_id Requesting overseer's PlayerID.
     * @return DemolitionResult with success, cost, and reason.
     */
    DemolitionResult handle_demolish(uint32_t entity_id, uint8_t player_id);

    /**
     * @brief Handle system-initiated demolition (from de-zone flow).
     *
     * Finds entity by grid position, demolishes without ownership check
     * and without cost (player_initiated = false).
     *
     * @param grid_x Grid X coordinate.
     * @param grid_y Grid Y coordinate.
     * @return DemolitionResult with success and reason.
     */
    DemolitionResult handle_demolition_request(int32_t grid_x, int32_t grid_y);

    /**
     * @brief Set demolition cost configuration.
     * @param config The DemolitionCostConfig to apply.
     */
    void set_cost_config(const DemolitionCostConfig& config);

    /**
     * @brief Get pending deconstructed events.
     * @return Const reference to pending events vector.
     */
    const std::vector<BuildingDeconstructedEvent>& get_pending_events() const;

    /**
     * @brief Clear all pending deconstructed events.
     */
    void clear_pending_events();

private:
    BuildingFactory* m_factory;                                 ///< Factory for entity access
    BuildingGrid* m_grid;                                       ///< Grid for footprint clearing
    ICreditProvider* m_credits;                                 ///< Credit provider for cost
    zone::ZoneSystem* m_zone_system;                            ///< Zone system for state updates
    DemolitionCostConfig m_cost_config;                         ///< Cost configuration
    std::vector<BuildingDeconstructedEvent> m_pending_events;   ///< Pending events

    /**
     * @brief Calculate demolition cost for an entity.
     * @param entity The building entity.
     * @return Demolition cost in credits.
     */
    uint32_t calculate_cost(const BuildingEntity& entity) const;

    /**
     * @brief Execute demolition on an entity.
     *
     * Sets state to Deconstructed, clears grid footprint, adds debris data,
     * and emits BuildingDeconstructedEvent.
     *
     * @param entity The building entity to demolish.
     * @param player_initiated True if overseer-initiated, false if system-initiated.
     */
    void execute_demolition(BuildingEntity& entity, bool player_initiated);
};

} // namespace building
} // namespace sims3000

#endif // SIMS3000_BUILDING_DEMOLITIONHANDLER_H
