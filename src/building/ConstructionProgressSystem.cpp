/**
 * @file ConstructionProgressSystem.cpp
 * @brief Construction progress system implementation (ticket 4-027)
 */

#include <sims3000/building/ConstructionProgressSystem.h>

namespace sims3000 {
namespace building {

ConstructionProgressSystem::ConstructionProgressSystem(BuildingFactory* factory)
    : m_factory(factory)
    , m_pending_constructed_events()
{}

void ConstructionProgressSystem::tick(uint32_t current_tick) {
    if (!m_factory) return;

    auto& entities = m_factory->get_entities_mut();

    for (auto& entity : entities) {
        // Only process entities that are under construction
        if (!entity.has_construction) continue;
        if (!entity.building.isInState(BuildingState::Materializing)) continue;

        // Advance construction by one tick
        // ConstructionComponent::tick() handles paused state internally
        entity.construction.tick();

        // Check if construction just completed
        if (entity.construction.isComplete()) {
            // Transition to Active state
            entity.building.setBuildingState(BuildingState::Active);
            entity.building.state_changed_tick = current_tick;
            entity.has_construction = false;

            // Emit BuildingConstructedEvent
            BuildingConstructedEvent event(
                entity.entity_id,
                entity.owner_id,
                entity.building.getZoneBuildingType(),
                entity.grid_x,
                entity.grid_y,
                entity.building.template_id
            );
            m_pending_constructed_events.push_back(event);
        }
    }
}

const std::vector<BuildingConstructedEvent>& ConstructionProgressSystem::get_pending_constructed_events() const {
    return m_pending_constructed_events;
}

void ConstructionProgressSystem::clear_pending_constructed_events() {
    m_pending_constructed_events.clear();
}

} // namespace building
} // namespace sims3000
