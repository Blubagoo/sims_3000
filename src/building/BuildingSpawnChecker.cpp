/**
 * @file BuildingSpawnChecker.cpp
 * @brief Implementation of BuildingSpawnChecker (Ticket 4-024)
 */

#include <sims3000/building/BuildingSpawnChecker.h>
#include <sims3000/zone/ZoneSystem.h>
#include <sims3000/building/BuildingGrid.h>
#include <sims3000/terrain/ITerrainQueryable.h>
#include <sims3000/building/ForwardDependencyInterfaces.h>

namespace sims3000 {
namespace building {

BuildingSpawnChecker::BuildingSpawnChecker(
    const zone::ZoneSystem* zone_system,
    const BuildingGrid* building_grid,
    const terrain::ITerrainQueryable* terrain,
    const ITransportProvider* transport,
    const IEnergyProvider* energy,
    const IFluidProvider* fluid)
    : m_zone_system(zone_system)
    , m_building_grid(building_grid)
    , m_terrain(terrain)
    , m_transport(transport)
    , m_energy(energy)
    , m_fluid(fluid)
{
}

bool BuildingSpawnChecker::can_spawn_building(std::int32_t x, std::int32_t y, std::uint8_t player_id) const {
    // (1) Zone exists at (x,y) AND is in Designated state
    if (m_zone_system) {
        zone::ZoneType zone_type;
        if (!m_zone_system->get_zone_type(x, y, zone_type)) {
            return false; // No zone at position
        }

        zone::ZoneState zone_state;
        if (!m_zone_system->get_zone_state(x, y, zone_state)) {
            return false; // Should not happen if zone exists, but guard anyway
        }
        if (zone_state != zone::ZoneState::Designated) {
            return false; // Zone not in Designated state
        }

        // (2) Demand > 0 for zone_type
        std::int8_t demand = m_zone_system->get_demand_for_type(zone_type, player_id);
        if (demand <= 0) {
            return false;
        }
    } else {
        return false; // Cannot check without zone system
    }

    // (3) BuildingGrid tile not occupied
    if (m_building_grid) {
        if (m_building_grid->is_tile_occupied(x, y)) {
            return false;
        }
    }

    // (4) Terrain is buildable (if terrain interface provided)
    if (m_terrain) {
        if (!m_terrain->is_buildable(x, y)) {
            return false;
        }
    }

    // (5) Road accessible within Chebyshev distance 3 (CCR-007)
    if (m_transport) {
        if (!m_transport->is_road_accessible_at(
                static_cast<std::uint32_t>(x),
                static_cast<std::uint32_t>(y), 3)) {
            return false;
        }
    }

    // (6) Power available (stub)
    if (m_energy) {
        if (!m_energy->is_powered_at(
                static_cast<std::uint32_t>(x),
                static_cast<std::uint32_t>(y),
                static_cast<std::uint32_t>(player_id))) {
            return false;
        }
    }

    // (7) Fluid available (stub)
    if (m_fluid) {
        if (!m_fluid->has_fluid_at(
                static_cast<std::uint32_t>(x),
                static_cast<std::uint32_t>(y),
                static_cast<std::uint32_t>(player_id))) {
            return false;
        }
    }

    return true;
}

bool BuildingSpawnChecker::can_spawn_footprint(std::int32_t x, std::int32_t y,
                                                std::uint8_t w, std::uint8_t h,
                                                std::uint8_t player_id) const {
    for (std::int32_t dy = 0; dy < static_cast<std::int32_t>(h); ++dy) {
        for (std::int32_t dx = 0; dx < static_cast<std::int32_t>(w); ++dx) {
            if (!can_spawn_building(x + dx, y + dy, player_id)) {
                return false;
            }
        }
    }
    return true;
}

} // namespace building
} // namespace sims3000
