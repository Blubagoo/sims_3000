/**
 * @file BuildingSpawningLoop.cpp
 * @brief Implementation of BuildingSpawningLoop (Ticket 4-026)
 */

#include <sims3000/building/BuildingSpawningLoop.h>
#include <sims3000/zone/ZoneSystem.h>
#include <sims3000/zone/ZoneTypes.h>

namespace sims3000 {
namespace building {

BuildingSpawningLoop::BuildingSpawningLoop(
    BuildingFactory* factory,
    BuildingSpawnChecker* checker,
    const BuildingTemplateRegistry* registry,
    zone::ZoneSystem* zone_system,
    BuildingGrid* grid)
    : m_factory(factory)
    , m_checker(checker)
    , m_registry(registry)
    , m_zone_system(zone_system)
    , m_grid(grid)
    , m_config()
    , m_total_spawned(0)
{
}

void BuildingSpawningLoop::tick(uint32_t current_tick) {
    if (!m_factory || !m_checker || !m_registry || !m_zone_system || !m_grid) {
        return;
    }

    if (m_config.scan_interval == 0) {
        return;
    }

    // For each overseer, check if their staggered scan interval has arrived
    for (uint8_t player_id = 0; player_id < zone::MAX_OVERSEERS; ++player_id) {
        uint32_t staggered_tick = current_tick + static_cast<uint32_t>(player_id) * m_config.stagger_offset;
        if (staggered_tick % m_config.scan_interval == 0) {
            scan_for_overseer(player_id, current_tick);
        }
    }
}

void BuildingSpawningLoop::scan_for_overseer(uint8_t player_id, uint32_t current_tick) {
    uint32_t spawn_count = 0;

    // Get the zone grid dimensions
    const auto& zone_grid = m_zone_system->get_grid();
    uint16_t grid_width = zone_grid.getWidth();
    uint16_t grid_height = zone_grid.getHeight();

    // Iterate all zone grid positions looking for designated zones
    for (int32_t y = 0; y < static_cast<int32_t>(grid_height) && spawn_count < m_config.max_spawns_per_scan; ++y) {
        for (int32_t x = 0; x < static_cast<int32_t>(grid_width) && spawn_count < m_config.max_spawns_per_scan; ++x) {
            // Check if there's a zone at this position
            if (!m_zone_system->is_zoned(x, y)) {
                continue;
            }

            // Check zone state is Designated
            zone::ZoneState zone_state;
            if (!m_zone_system->get_zone_state(x, y, zone_state)) {
                continue;
            }
            if (zone_state != zone::ZoneState::Designated) {
                continue;
            }

            // Check spawn preconditions via BuildingSpawnChecker
            if (!m_checker->can_spawn_building(x, y, player_id)) {
                continue;
            }

            // Get zone type and density for template selection
            zone::ZoneType zone_type;
            if (!m_zone_system->get_zone_type(x, y, zone_type)) {
                continue;
            }

            zone::ZoneDensity zone_density;
            if (!m_zone_system->get_zone_density(x, y, zone_density)) {
                continue;
            }

            // Convert zone types to building types
            ZoneBuildingType building_zone_type = static_cast<ZoneBuildingType>(static_cast<uint8_t>(zone_type));
            DensityLevel density_level = static_cast<DensityLevel>(static_cast<uint8_t>(zone_density));

            // Get neighbor template IDs from BuildingGrid (4 orthogonal positions)
            std::vector<uint32_t> neighbor_ids;
            // Up
            uint32_t up_id = m_grid->get_building_at(x, y - 1);
            neighbor_ids.push_back(up_id);
            // Down
            uint32_t down_id = m_grid->get_building_at(x, y + 1);
            neighbor_ids.push_back(down_id);
            // Left
            uint32_t left_id = m_grid->get_building_at(x - 1, y);
            neighbor_ids.push_back(left_id);
            // Right
            uint32_t right_id = m_grid->get_building_at(x + 1, y);
            neighbor_ids.push_back(right_id);

            // For neighbor template IDs, look up the actual template_id from entities
            std::vector<uint32_t> neighbor_template_ids;
            for (uint32_t eid : neighbor_ids) {
                if (eid != INVALID_ENTITY) {
                    const BuildingEntity* entity = m_factory->get_entity(eid);
                    if (entity) {
                        neighbor_template_ids.push_back(entity->building.template_id);
                        continue;
                    }
                }
                neighbor_template_ids.push_back(0);
            }

            // Default desirability (50.0f stub value)
            float desirability = 50.0f;

            // Select a template
            TemplateSelectionResult selection = select_template(
                *m_registry,
                building_zone_type,
                density_level,
                desirability,
                x, y,
                static_cast<uint64_t>(current_tick),
                neighbor_template_ids
            );

            // If no valid template was selected, skip
            if (selection.template_id == 0) {
                continue;
            }

            // Get the full template
            const BuildingTemplate& templ = m_registry->get_template(selection.template_id);

            // Spawn the building
            m_factory->spawn_building(templ, selection, x, y, player_id, current_tick);
            ++spawn_count;
            ++m_total_spawned;
        }
    }
}

void BuildingSpawningLoop::set_config(const SpawningConfig& config) {
    m_config = config;
}

const SpawningConfig& BuildingSpawningLoop::get_config() const {
    return m_config;
}

uint32_t BuildingSpawningLoop::get_total_spawned() const {
    return m_total_spawned;
}

} // namespace building
} // namespace sims3000
