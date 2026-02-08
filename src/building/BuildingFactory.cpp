/**
 * @file BuildingFactory.cpp
 * @brief Building entity creation and management implementation (ticket 4-025)
 */

#include <sims3000/building/BuildingFactory.h>
#include <sims3000/zone/ZoneSystem.h>
#include <sims3000/zone/ZoneTypes.h>
#include <algorithm>

namespace sims3000 {
namespace building {

BuildingFactory::BuildingFactory(BuildingGrid* grid, zone::ZoneSystem* zone_system)
    : m_grid(grid)
    , m_zone_system(zone_system)
    , m_entities()
    , m_next_entity_id(1)
{}

uint32_t BuildingFactory::spawn_building(
    const BuildingTemplate& templ,
    const TemplateSelectionResult& selection,
    int32_t grid_x, int32_t grid_y,
    uint8_t owner_id,
    uint32_t current_tick)
{
    // 1. Generate unique entity_id
    uint32_t entity_id = m_next_entity_id++;

    // 2. Create BuildingEntity
    BuildingEntity entity;
    entity.entity_id = entity_id;
    entity.grid_x = grid_x;
    entity.grid_y = grid_y;
    entity.owner_id = owner_id;

    // 3. Initialize BuildingComponent from template and selection
    entity.building.template_id = templ.template_id;
    entity.building.setZoneBuildingType(templ.zone_type);
    entity.building.setDensityLevel(templ.density);
    entity.building.setBuildingState(BuildingState::Materializing);
    entity.building.level = 1;
    entity.building.health = 255;
    entity.building.capacity = templ.base_capacity;
    entity.building.current_occupancy = 0;
    entity.building.footprint_w = templ.footprint_w;
    entity.building.footprint_h = templ.footprint_h;
    entity.building.rotation = selection.rotation;
    entity.building.color_accent_index = selection.color_accent_index;
    entity.building.state_changed_tick = current_tick;
    entity.building.abandon_timer = 0;

    // 4. Initialize ConstructionComponent from template
    entity.construction = ConstructionComponent(templ.construction_ticks, templ.construction_cost);
    entity.has_construction = true;
    entity.has_debris = false;

    // 5. Register footprint in BuildingGrid
    if (m_grid) {
        m_grid->set_footprint(grid_x, grid_y, templ.footprint_w, templ.footprint_h, entity_id);
    }

    // 6. Set zone state to Occupied for all footprint tiles
    if (m_zone_system) {
        for (int32_t dy = 0; dy < templ.footprint_h; ++dy) {
            for (int32_t dx = 0; dx < templ.footprint_w; ++dx) {
                m_zone_system->set_zone_state(grid_x + dx, grid_y + dy, zone::ZoneState::Occupied);
            }
        }
    }

    // 7. Store entity
    m_entities.push_back(entity);

    return entity_id;
}

const BuildingEntity* BuildingFactory::get_entity(uint32_t entity_id) const {
    for (const auto& entity : m_entities) {
        if (entity.entity_id == entity_id) {
            return &entity;
        }
    }
    return nullptr;
}

BuildingEntity* BuildingFactory::get_entity_mut(uint32_t entity_id) {
    for (auto& entity : m_entities) {
        if (entity.entity_id == entity_id) {
            return &entity;
        }
    }
    return nullptr;
}

const std::vector<BuildingEntity>& BuildingFactory::get_entities() const {
    return m_entities;
}

std::vector<BuildingEntity>& BuildingFactory::get_entities_mut() {
    return m_entities;
}

bool BuildingFactory::remove_entity(uint32_t entity_id) {
    auto it = std::find_if(m_entities.begin(), m_entities.end(),
        [entity_id](const BuildingEntity& e) { return e.entity_id == entity_id; });

    if (it != m_entities.end()) {
        m_entities.erase(it);
        return true;
    }
    return false;
}

} // namespace building
} // namespace sims3000
