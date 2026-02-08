/**
 * @file DemolitionHandler.cpp
 * @brief Overseer demolition handler implementation (ticket 4-030)
 */

#include <sims3000/building/DemolitionHandler.h>
#include <sims3000/zone/ZoneSystem.h>

namespace sims3000 {
namespace building {

DemolitionHandler::DemolitionHandler(BuildingFactory* factory, BuildingGrid* grid,
                                     ICreditProvider* credits, zone::ZoneSystem* zone_system)
    : m_factory(factory)
    , m_grid(grid)
    , m_credits(credits)
    , m_zone_system(zone_system)
    , m_cost_config()
    , m_pending_events()
{}

DemolitionResult DemolitionHandler::handle_demolish(uint32_t entity_id, uint8_t player_id) {
    DemolitionResult result;

    // 1. Find entity by ID
    BuildingEntity* entity = m_factory ? m_factory->get_entity_mut(entity_id) : nullptr;
    if (!entity) {
        result.success = false;
        result.reason = DemolitionResult::Reason::EntityNotFound;
        return result;
    }

    // 2. Validate ownership
    if (entity->owner_id != player_id) {
        result.success = false;
        result.reason = DemolitionResult::Reason::NotOwned;
        return result;
    }

    // 3. Validate not already Deconstructed
    if (entity->building.isInState(BuildingState::Deconstructed)) {
        result.success = false;
        result.reason = DemolitionResult::Reason::AlreadyDeconstructed;
        return result;
    }

    // 4. Calculate cost
    uint32_t cost = calculate_cost(*entity);

    // 5. Deduct credits
    if (m_credits && cost > 0) {
        if (!m_credits->deduct_credits(player_id, static_cast<int64_t>(cost))) {
            result.success = false;
            result.cost = cost;
            result.reason = DemolitionResult::Reason::InsufficientCredits;
            return result;
        }
    }

    // 6. Execute demolition
    execute_demolition(*entity, true);

    result.success = true;
    result.cost = cost;
    result.reason = DemolitionResult::Reason::Ok;
    return result;
}

DemolitionResult DemolitionHandler::handle_demolition_request(int32_t grid_x, int32_t grid_y) {
    DemolitionResult result;

    // Find entity by grid position
    if (!m_grid) {
        result.success = false;
        result.reason = DemolitionResult::Reason::EntityNotFound;
        return result;
    }

    uint32_t entity_id = m_grid->get_building_at(grid_x, grid_y);
    if (entity_id == INVALID_ENTITY) {
        result.success = false;
        result.reason = DemolitionResult::Reason::EntityNotFound;
        return result;
    }

    BuildingEntity* entity = m_factory ? m_factory->get_entity_mut(entity_id) : nullptr;
    if (!entity) {
        result.success = false;
        result.reason = DemolitionResult::Reason::EntityNotFound;
        return result;
    }

    // Already deconstructed check
    if (entity->building.isInState(BuildingState::Deconstructed)) {
        result.success = false;
        result.reason = DemolitionResult::Reason::AlreadyDeconstructed;
        return result;
    }

    // System-initiated: no cost, no ownership check
    execute_demolition(*entity, false);

    result.success = true;
    result.cost = 0;
    result.reason = DemolitionResult::Reason::Ok;
    return result;
}

void DemolitionHandler::set_cost_config(const DemolitionCostConfig& config) {
    m_cost_config = config;
}

const std::vector<BuildingDeconstructedEvent>& DemolitionHandler::get_pending_events() const {
    return m_pending_events;
}

void DemolitionHandler::clear_pending_events() {
    m_pending_events.clear();
}

uint32_t DemolitionHandler::calculate_cost(const BuildingEntity& entity) const {
    float state_modifier = 0.0f;

    switch (entity.building.getBuildingState()) {
        case BuildingState::Materializing:
            state_modifier = m_cost_config.materializing_modifier;
            break;
        case BuildingState::Active:
            state_modifier = m_cost_config.active_modifier;
            break;
        case BuildingState::Abandoned:
            state_modifier = m_cost_config.abandoned_modifier;
            break;
        case BuildingState::Derelict:
            state_modifier = m_cost_config.derelict_modifier;
            break;
        case BuildingState::Deconstructed:
            return 0;
    }

    // Get construction cost from the construction component if available,
    // otherwise use 0
    uint32_t construction_cost = entity.construction.construction_cost;

    float cost_f = static_cast<float>(construction_cost) * m_cost_config.base_cost_ratio * state_modifier;
    return static_cast<uint32_t>(cost_f);
}

void DemolitionHandler::execute_demolition(BuildingEntity& entity, bool player_initiated) {
    // Set state to Deconstructed
    entity.building.setBuildingState(BuildingState::Deconstructed);
    entity.has_construction = false;

    // Add debris data
    entity.has_debris = true;
    entity.debris = DebrisComponent(
        entity.building.template_id,
        entity.building.footprint_w,
        entity.building.footprint_h
    );

    // Clear grid footprint
    if (m_grid) {
        m_grid->clear_footprint(
            entity.grid_x, entity.grid_y,
            entity.building.footprint_w, entity.building.footprint_h
        );
    }

    // Emit event
    BuildingDeconstructedEvent event(
        entity.entity_id,
        entity.owner_id,
        entity.grid_x,
        entity.grid_y,
        player_initiated
    );
    m_pending_events.push_back(event);
}

} // namespace building
} // namespace sims3000
