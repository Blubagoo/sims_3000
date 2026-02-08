/**
 * @file DebrisClearSystem.cpp
 * @brief Debris auto-clear system implementation (ticket 4-031)
 */

#include <sims3000/building/DebrisClearSystem.h>
#include <algorithm>

namespace sims3000 {
namespace building {

DebrisClearSystem::DebrisClearSystem(BuildingFactory* factory, BuildingGrid* grid,
                                     ICreditProvider* credits)
    : m_factory(factory)
    , m_grid(grid)
    , m_credits(credits)
    , m_config()
    , m_pending_events()
{}

void DebrisClearSystem::tick() {
    if (!m_factory) return;

    // Collect entity IDs to remove after iteration
    std::vector<uint32_t> to_remove;

    auto& entities = m_factory->get_entities_mut();

    for (auto& entity : entities) {
        // Only process Deconstructed entities with debris
        if (!entity.building.isInState(BuildingState::Deconstructed)) continue;
        if (!entity.has_debris) continue;

        // Decrement timer
        entity.debris.tick();

        // Check if timer expired
        if (entity.debris.isExpired()) {
            // Emit event before removal
            DebrisClearedEvent event(entity.entity_id, entity.grid_x, entity.grid_y);
            m_pending_events.push_back(event);

            to_remove.push_back(entity.entity_id);
        }
    }

    // Remove expired entities
    for (uint32_t id : to_remove) {
        m_factory->remove_entity(id);
    }
}

bool DebrisClearSystem::handle_clear_debris(uint32_t entity_id, uint8_t player_id) {
    if (!m_factory) return false;

    // Find entity
    BuildingEntity* entity = m_factory->get_entity_mut(entity_id);
    if (!entity) return false;

    // Validate state
    if (!entity->building.isInState(BuildingState::Deconstructed)) return false;
    if (!entity->has_debris) return false;

    // Deduct cost
    if (m_credits && m_config.manual_clear_cost > 0) {
        if (!m_credits->deduct_credits(player_id, static_cast<int64_t>(m_config.manual_clear_cost))) {
            return false;
        }
    }

    // Emit event
    DebrisClearedEvent event(entity->entity_id, entity->grid_x, entity->grid_y);
    m_pending_events.push_back(event);

    // Remove entity
    m_factory->remove_entity(entity_id);

    return true;
}

void DebrisClearSystem::set_config(const DebrisClearConfig& config) {
    m_config = config;
}

const std::vector<DebrisClearedEvent>& DebrisClearSystem::get_pending_events() const {
    return m_pending_events;
}

void DebrisClearSystem::clear_pending_events() {
    m_pending_events.clear();
}

} // namespace building
} // namespace sims3000
