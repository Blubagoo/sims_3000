/**
 * @file ServicesSystem.cpp
 * @brief Implementation of ServicesSystem orchestrator (Epic 9, Tickets E9-003, E9-011)
 *
 * E9-003: Core ServicesSystem skeleton with ISimulatable interface.
 * E9-011: Per-type-per-player dirty flags and recalculation triggers.
 *
 * Tick phases:
 * 1. Update service building states (stub)
 * 2. Recalculate dirty coverage grids (E9-011)
 * 3. Apply service effects to population (stub)
 *
 * @see ServicesSystem.h for class documentation.
 */

#include <sims3000/services/ServicesSystem.h>
#include <sims3000/services/ServiceCoverageGrid.h>
#include <sims3000/services/CoverageCalculation.h>

namespace sims3000 {
namespace services {

// =============================================================================
// Construction / Destruction
// =============================================================================

ServicesSystem::ServicesSystem() = default;

ServicesSystem::~ServicesSystem() {
    if (m_initialized) {
        cleanup();
    }
}

// =============================================================================
// ISimulatable interface
// =============================================================================

void ServicesSystem::tick(const ISimulationTime& time) {
    (void)time;

    if (!m_initialized) {
        return;
    }

    // Phase 1: Update service building operational states (stub)

    // Phase 2: Recalculate coverage grids for dirty type+player combinations
    recalculate_if_dirty();

    // Phase 3: Apply service effects (enforcer suppression, hazard response, etc.) (stub)
}

// =============================================================================
// Dirty Flag Management (E9-011)
// =============================================================================

void ServicesSystem::mark_dirty(ServiceType type, uint8_t player_id) {
    if (player_id >= MAX_PLAYERS) {
        return;
    }
    uint8_t type_idx = static_cast<uint8_t>(type);
    if (type_idx >= SERVICE_TYPE_COUNT) {
        return;
    }
    m_dirty[type_idx][player_id] = true;
}

void ServicesSystem::mark_all_dirty(uint8_t player_id) {
    if (player_id >= MAX_PLAYERS) {
        return;
    }
    for (uint8_t t = 0; t < SERVICE_TYPE_COUNT; ++t) {
        m_dirty[t][player_id] = true;
    }
}

bool ServicesSystem::is_dirty(ServiceType type, uint8_t player_id) const {
    if (player_id >= MAX_PLAYERS) {
        return false;
    }
    uint8_t type_idx = static_cast<uint8_t>(type);
    if (type_idx >= SERVICE_TYPE_COUNT) {
        return false;
    }
    return m_dirty[type_idx][player_id];
}

bool ServicesSystem::isCoverageDirty() const {
    for (uint8_t t = 0; t < SERVICE_TYPE_COUNT; ++t) {
        for (uint8_t p = 0; p < MAX_PLAYERS; ++p) {
            if (m_dirty[t][p]) {
                return true;
            }
        }
    }
    return false;
}

ServiceCoverageGrid* ServicesSystem::get_coverage_grid(ServiceType type, uint8_t player_id) const {
    if (player_id >= MAX_PLAYERS) {
        return nullptr;
    }
    uint8_t type_idx = static_cast<uint8_t>(type);
    if (type_idx >= SERVICE_TYPE_COUNT) {
        return nullptr;
    }
    return m_coverage_grids[type_idx][player_id].get();
}

void ServicesSystem::recalculate_if_dirty() {
    for (uint8_t t = 0; t < SERVICE_TYPE_COUNT; ++t) {
        for (uint8_t p = 0; p < MAX_PLAYERS; ++p) {
            if (!m_dirty[t][p]) {
                continue;
            }

            // Lazy allocate grid on first recalculation
            if (!m_coverage_grids[t][p]) {
                m_coverage_grids[t][p] = std::make_unique<ServiceCoverageGrid>(
                    m_map_width, m_map_height);
            }

            // Collect building data for this type+player
            // Note: In the current architecture, entity data comes from outside.
            // For now, the building data vector is empty since we don't have
            // ECS access here. The calculate_radius_coverage function will
            // still clear the grid, which is correct behavior.
            // Future tickets will wire up ECS component queries to populate
            // the building data vector before calling calculate_radius_coverage.
            std::vector<ServiceBuildingData> buildings;
            // TODO: Populate buildings from ECS component queries for type t, player p

            calculate_radius_coverage(*m_coverage_grids[t][p], buildings);

            // Mark clean after recalculation
            m_dirty[t][p] = false;
        }
    }
}

// =============================================================================
// Building Event Handlers (E9-012, updated for E9-011)
// =============================================================================

void ServicesSystem::on_building_constructed(uint32_t entity_id, uint8_t owner_id) {
    if (owner_id >= MAX_PLAYERS) {
        return;
    }

    // Add to per-player tracking vector
    m_service_entities[owner_id].push_back(entity_id);

    // Mark all service types dirty for this player (E9-011)
    // We mark all types because we don't know the service type from entity_id alone.
    // Future tickets will pass ServiceType to narrow the dirty marking.
    mark_all_dirty(owner_id);
}

void ServicesSystem::on_building_deconstructed(uint32_t entity_id, uint8_t owner_id) {
    if (owner_id >= MAX_PLAYERS) {
        return;
    }

    // Remove from per-player tracking vector
    auto& entities = m_service_entities[owner_id];
    for (size_t i = 0; i < entities.size(); ++i) {
        if (entities[i] == entity_id) {
            // Swap with last element and pop (order doesn't matter)
            entities[i] = entities.back();
            entities.pop_back();
            break;
        }
    }

    // Mark all service types dirty for this player (E9-011)
    mark_all_dirty(owner_id);
}

void ServicesSystem::on_building_power_changed(uint32_t entity_id, uint8_t owner_id) {
    (void)entity_id;

    if (owner_id >= MAX_PLAYERS) {
        return;
    }

    // Mark all service types dirty for this player (E9-011)
    mark_all_dirty(owner_id);
}

// =============================================================================
// Lifecycle
// =============================================================================

void ServicesSystem::init(uint32_t map_width, uint32_t map_height) {
    m_map_width = map_width;
    m_map_height = map_height;

    // Clear dirty flags
    for (uint8_t t = 0; t < SERVICE_TYPE_COUNT; ++t) {
        for (uint8_t p = 0; p < MAX_PLAYERS; ++p) {
            m_dirty[t][p] = false;
            m_coverage_grids[t][p].reset();
        }
    }

    // Clear any existing entity tracking
    for (auto& entities : m_service_entities) {
        entities.clear();
    }

    m_initialized = true;
}

void ServicesSystem::cleanup() {
    // Clear per-player entity tracking
    for (auto& entities : m_service_entities) {
        entities.clear();
    }

    // Release coverage grids and clear dirty flags
    for (uint8_t t = 0; t < SERVICE_TYPE_COUNT; ++t) {
        for (uint8_t p = 0; p < MAX_PLAYERS; ++p) {
            m_coverage_grids[t][p].reset();
            m_dirty[t][p] = false;
        }
    }

    m_map_width = 0;
    m_map_height = 0;
    m_initialized = false;
}

} // namespace services
} // namespace sims3000
