/**
 * @file ConstructionProgressSystem.h
 * @brief Construction progress system for advancing building construction (Epic 4, ticket 4-027)
 *
 * Ticks construction progress for all Materializing buildings each simulation tick.
 * When construction completes, transitions the building to Active state and emits
 * a BuildingConstructedEvent.
 *
 * @see /docs/epics/epic-4/tickets.md (ticket 4-027)
 */

#ifndef SIMS3000_BUILDING_CONSTRUCTIONPROGRESSSYSTEM_H
#define SIMS3000_BUILDING_CONSTRUCTIONPROGRESSSYSTEM_H

#include <sims3000/building/BuildingFactory.h>
#include <sims3000/building/BuildingEvents.h>
#include <cstdint>
#include <vector>

namespace sims3000 {
namespace building {

/**
 * @class ConstructionProgressSystem
 * @brief Advances construction for all Materializing buildings.
 *
 * Each tick:
 * - Iterates all entities in the factory
 * - For entities with has_construction == true and state == Materializing:
 *   - Calls construction.tick() to increment ticks_elapsed
 *   - If construction.isComplete(): transitions to Active, emits event
 * - Paused constructions are not advanced (handled by ConstructionComponent::tick())
 */
class ConstructionProgressSystem {
public:
    /**
     * @brief Construct with reference to BuildingFactory.
     * @param factory Pointer to BuildingFactory containing entities.
     */
    ConstructionProgressSystem(BuildingFactory* factory);

    /**
     * @brief Advance construction for all Materializing buildings.
     *
     * Should be called once per simulation tick.
     *
     * @param current_tick Current simulation tick number.
     */
    void tick(uint32_t current_tick);

    /**
     * @brief Get pending constructed events.
     * @return Const reference to pending events vector.
     */
    const std::vector<BuildingConstructedEvent>& get_pending_constructed_events() const;

    /**
     * @brief Clear all pending constructed events.
     */
    void clear_pending_constructed_events();

private:
    BuildingFactory* m_factory;                                 ///< Factory containing entities
    std::vector<BuildingConstructedEvent> m_pending_constructed_events; ///< Pending events
};

} // namespace building
} // namespace sims3000

#endif // SIMS3000_BUILDING_CONSTRUCTIONPROGRESSSYSTEM_H
