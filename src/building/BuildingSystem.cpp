/**
 * @file BuildingSystem.cpp
 * @brief Top-level BuildingSystem implementation (Ticket 4-034)
 */

#include <sims3000/building/BuildingSystem.h>
#include <sims3000/zone/ZoneSystem.h>

namespace sims3000 {
namespace building {

BuildingSystem::BuildingSystem(
    zone::ZoneSystem* zone_system,
    terrain::ITerrainQueryable* terrain,
    uint16_t grid_size)
    : m_grid(grid_size, grid_size)
    , m_factory(&m_grid, zone_system)
    , m_registry()
    , m_spawn_checker(zone_system, &m_grid, terrain, nullptr, nullptr, nullptr)
    , m_spawning_loop(&m_factory, &m_spawn_checker, &m_registry, zone_system, &m_grid)
    , m_construction_system(&m_factory)
    , m_state_system(&m_factory, &m_grid, nullptr, nullptr, nullptr)
    , m_demolition_handler(&m_factory, &m_grid, nullptr, zone_system)
    , m_debris_clear_system(&m_factory, &m_grid, nullptr)
    , m_energy(nullptr)
    , m_fluid(nullptr)
    , m_transport(nullptr)
    , m_port(nullptr)
    , m_land_value(nullptr)
    , m_demand(nullptr)
    , m_credits(nullptr)
    , m_tick_count(0)
{
    // Register initial building templates
    register_initial_templates(m_registry);
}

void BuildingSystem::tick(float /*delta_time*/) {
    m_tick_count++;
    m_spawning_loop.tick(m_tick_count);
    m_construction_system.tick(m_tick_count);
    m_state_system.tick(m_tick_count);
    m_debris_clear_system.tick();
}

int BuildingSystem::get_priority() const {
    return 40;
}

// =============================================================================
// Provider setters
// =============================================================================

void BuildingSystem::set_energy_provider(IEnergyProvider* provider) {
    m_energy = provider;
    // Note: BuildingSpawnChecker and BuildingStateTransitionSystem hold
    // raw pointers set at construction time. Since subsystems store these
    // pointers by value and check for null, the providers passed at
    // construction (nullptr) are already stored. To update subsystems,
    // we would need to reconstruct them or add setters. For this epic,
    // providers should be set before tick() is called, via re-construction
    // if needed. The subsystem pointers are set at construction.
}

void BuildingSystem::set_fluid_provider(IFluidProvider* provider) {
    m_fluid = provider;
}

void BuildingSystem::set_transport_provider(ITransportProvider* provider) {
    m_transport = provider;
}

void BuildingSystem::set_port_provider(IPortProvider* provider) {
    m_port = provider;
}

void BuildingSystem::set_land_value_provider(ILandValueProvider* provider) {
    m_land_value = provider;
}

void BuildingSystem::set_demand_provider(IDemandProvider* provider) {
    m_demand = provider;
}

void BuildingSystem::set_credit_provider(ICreditProvider* provider) {
    m_credits = provider;
}

// =============================================================================
// Subsystem access
// =============================================================================

BuildingFactory& BuildingSystem::get_factory() {
    return m_factory;
}

const BuildingFactory& BuildingSystem::get_factory() const {
    return m_factory;
}

BuildingGrid& BuildingSystem::get_grid() {
    return m_grid;
}

const BuildingGrid& BuildingSystem::get_grid() const {
    return m_grid;
}

DemolitionHandler& BuildingSystem::get_demolition_handler() {
    return m_demolition_handler;
}

DebrisClearSystem& BuildingSystem::get_debris_clear_system() {
    return m_debris_clear_system;
}

BuildingSpawningLoop& BuildingSystem::get_spawning_loop() {
    return m_spawning_loop;
}

ConstructionProgressSystem& BuildingSystem::get_construction_system() {
    return m_construction_system;
}

BuildingStateTransitionSystem& BuildingSystem::get_state_system() {
    return m_state_system;
}

// =============================================================================
// IBuildingQueryable implementation
// =============================================================================

uint32_t BuildingSystem::get_building_at(int32_t x, int32_t y) const {
    return m_grid.get_building_at(x, y);
}

bool BuildingSystem::is_tile_occupied(int32_t x, int32_t y) const {
    return m_grid.is_tile_occupied(x, y);
}

bool BuildingSystem::is_footprint_available(int32_t x, int32_t y, uint8_t w, uint8_t h) const {
    return m_grid.is_footprint_available(x, y, w, h);
}

std::vector<uint32_t> BuildingSystem::get_buildings_in_rect(int32_t x, int32_t y, int32_t w, int32_t h) const {
    std::vector<uint32_t> result;
    for (int32_t dy = 0; dy < h; ++dy) {
        for (int32_t dx = 0; dx < w; ++dx) {
            uint32_t eid = m_grid.get_building_at(x + dx, y + dy);
            if (eid != INVALID_ENTITY) {
                // Only add unique entity IDs
                bool found = false;
                for (uint32_t existing : result) {
                    if (existing == eid) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    result.push_back(eid);
                }
            }
        }
    }
    return result;
}

std::vector<uint32_t> BuildingSystem::get_buildings_by_owner(uint8_t player_id) const {
    std::vector<uint32_t> result;
    for (const auto& entity : m_factory.get_entities()) {
        if (entity.owner_id == player_id) {
            result.push_back(entity.entity_id);
        }
    }
    return result;
}

uint32_t BuildingSystem::get_building_count() const {
    return static_cast<uint32_t>(m_factory.get_entities().size());
}

uint32_t BuildingSystem::get_building_count_by_state(BuildingState state) const {
    uint32_t count = 0;
    for (const auto& entity : m_factory.get_entities()) {
        if (entity.building.getBuildingState() == state) {
            ++count;
        }
    }
    return count;
}

std::optional<BuildingState> BuildingSystem::get_building_state(uint32_t entity_id) const {
    const auto* entity = m_factory.get_entity(entity_id);
    if (!entity) {
        return std::nullopt;
    }
    return entity->building.getBuildingState();
}

uint32_t BuildingSystem::get_total_capacity(ZoneBuildingType type, uint8_t player_id) const {
    uint32_t total = 0;
    for (const auto& entity : m_factory.get_entities()) {
        if (entity.building.getZoneBuildingType() == type && entity.owner_id == player_id) {
            total += entity.building.capacity;
        }
    }
    return total;
}

uint32_t BuildingSystem::get_total_occupancy(ZoneBuildingType type, uint8_t player_id) const {
    uint32_t total = 0;
    for (const auto& entity : m_factory.get_entities()) {
        if (entity.building.getZoneBuildingType() == type && entity.owner_id == player_id) {
            total += entity.building.current_occupancy;
        }
    }
    return total;
}

// =============================================================================
// Template registry access
// =============================================================================

const BuildingTemplateRegistry& BuildingSystem::get_template_registry() const {
    return m_registry;
}

uint32_t BuildingSystem::get_tick_count() const {
    return m_tick_count;
}

} // namespace building
} // namespace sims3000
