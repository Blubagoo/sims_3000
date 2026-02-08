/**
 * @file ZoneSystem.cpp
 * @brief Implementation of ZoneSystem (ticket 4-008)
 */

#include <sims3000/zone/ZoneSystem.h>
#include <sims3000/terrain/ITerrainQueryable.h>
#include <sims3000/building/ForwardDependencyInterfaces.h>
#include <cassert>
#include <algorithm>

namespace sims3000 {
namespace zone {

ZoneSystem::ZoneSystem(terrain::ITerrainQueryable* terrain,
                       building::ITransportProvider* transport,
                       std::uint16_t grid_size)
    : m_terrain(terrain)
    , m_transport(transport)
    , m_grid(grid_size, grid_size)
    , m_zone_counts()
    , m_demand()
    , m_zone_info()
    , m_grid_width(grid_size)
{
    // Initialize zone info storage (same size as grid)
    m_zone_info.resize(static_cast<std::size_t>(grid_size) * grid_size);

    // ZoneCounts and ZoneDemandData are zero-initialized by their default constructors
}

void ZoneSystem::tick(float delta_time) {
    (void)delta_time;
    update_demand();

    // Update desirability every N ticks (Ticket 4-018)
    ++m_tick_counter;
    if (m_desirability_config.update_interval_ticks > 0 &&
        (m_tick_counter % m_desirability_config.update_interval_ticks) == 0) {
        update_all_desirability();
    }
}

int ZoneSystem::get_priority() const {
    return 30;
}

bool ZoneSystem::get_zone_type(std::int32_t x, std::int32_t y, ZoneType& out_type) const {
    const ZoneInfo* info = get_zone_info(x, y);
    if (!info || !info->valid) {
        return false;
    }
    out_type = info->component.getZoneType();
    return true;
}

bool ZoneSystem::get_zone_density(std::int32_t x, std::int32_t y, ZoneDensity& out_density) const {
    const ZoneInfo* info = get_zone_info(x, y);
    if (!info || !info->valid) {
        return false;
    }
    out_density = info->component.getDensity();
    return true;
}

bool ZoneSystem::is_zoned(std::int32_t x, std::int32_t y) const {
    return m_grid.has_zone_at(x, y);
}

std::uint32_t ZoneSystem::get_zone_count(std::uint8_t player_id, ZoneType type) const {
    if (player_id >= MAX_OVERSEERS) {
        return 0;
    }
    const ZoneCounts& counts = m_zone_counts[player_id];
    switch (type) {
        case ZoneType::Habitation:  return counts.habitation_total;
        case ZoneType::Exchange:    return counts.exchange_total;
        case ZoneType::Fabrication: return counts.fabrication_total;
        default: return 0;
    }
}

std::int8_t ZoneSystem::get_demand_for_type(ZoneType type, std::uint8_t player_id) const {
    if (player_id >= MAX_OVERSEERS) {
        return 0;
    }

    // Ticket 4-017: delegate to external provider if available
    if (m_external_demand_provider != nullptr) {
        float raw = m_external_demand_provider->get_demand(
            static_cast<std::uint8_t>(type),
            static_cast<std::uint32_t>(player_id));
        // Clamp to [-100, +100]
        if (raw < -100.0f) raw = -100.0f;
        if (raw > 100.0f) raw = 100.0f;
        return static_cast<std::int8_t>(raw);
    }

    const ZoneDemandData& demand = m_demand[player_id];
    switch (type) {
        case ZoneType::Habitation:  return demand.habitation_demand;
        case ZoneType::Exchange:    return demand.exchange_demand;
        case ZoneType::Fabrication: return demand.fabrication_demand;
        default: return 0;
    }
}

bool ZoneSystem::is_valid_transition(ZoneState from, ZoneState to) {
    if (from == to) {
        return false;
    }
    // Valid transitions:
    // Designated -> Occupied
    // Occupied -> Designated
    // Designated -> Stalled
    // Stalled -> Designated
    if (from == ZoneState::Designated &&
        (to == ZoneState::Occupied || to == ZoneState::Stalled)) {
        return true;
    }
    if ((from == ZoneState::Occupied || from == ZoneState::Stalled) &&
        to == ZoneState::Designated) {
        return true;
    }
    return false;
}

bool ZoneSystem::set_zone_state(std::int32_t x, std::int32_t y, ZoneState new_state) {
    ZoneInfo* info = get_zone_info_mut(x, y);
    if (!info || !info->valid) {
        return false;
    }

    ZoneState old_state = info->component.getState();

    // Validate transition
    if (!is_valid_transition(old_state, new_state)) {
        return false;
    }

    // Update state counts for this player
    std::uint8_t pid = info->player_id;
    if (pid < MAX_OVERSEERS) {
        ZoneCounts& counts = m_zone_counts[pid];

        // Decrement old state count
        switch (old_state) {
            case ZoneState::Designated: if (counts.designated_total > 0) --counts.designated_total; break;
            case ZoneState::Occupied:   if (counts.occupied_total > 0) --counts.occupied_total; break;
            case ZoneState::Stalled:    if (counts.stalled_total > 0) --counts.stalled_total; break;
        }

        // Increment new state count
        switch (new_state) {
            case ZoneState::Designated: ++counts.designated_total; break;
            case ZoneState::Occupied:   ++counts.occupied_total; break;
            case ZoneState::Stalled:    ++counts.stalled_total; break;
        }
    }

    info->component.setState(new_state);

    // Emit state changed event
    std::uint32_t entity_id = m_grid.get_zone_at(x, y);
    m_pending_state_events.emplace_back(entity_id, x, y, old_state, new_state);

    return true;
}

bool ZoneSystem::place_zone(std::int32_t x, std::int32_t y,
                            ZoneType type, ZoneDensity density,
                            std::uint8_t player_id, std::uint32_t entity_id) {
    // Attempt to place in grid
    if (!m_grid.place_zone(x, y, entity_id)) {
        return false;
    }

    // Store zone info
    std::size_t index = static_cast<std::size_t>(y) * m_grid_width + static_cast<std::size_t>(x);
    assert(index < m_zone_info.size());

    ZoneInfo& info = m_zone_info[index];
    info.component.setZoneType(type);
    info.component.setDensity(density);
    info.component.setState(ZoneState::Designated);
    info.component.desirability = 0;
    info.player_id = player_id;
    info.valid = true;

    // Update counts
    if (player_id < MAX_OVERSEERS) {
        ZoneCounts& counts = m_zone_counts[player_id];
        counts.total++;
        counts.designated_total++;

        switch (type) {
            case ZoneType::Habitation:  counts.habitation_total++; break;
            case ZoneType::Exchange:    counts.exchange_total++; break;
            case ZoneType::Fabrication: counts.fabrication_total++; break;
        }

        switch (density) {
            case ZoneDensity::LowDensity:  counts.low_density_total++; break;
            case ZoneDensity::HighDensity: counts.high_density_total++; break;
        }
    }

    return true;
}

const ZoneCounts& ZoneSystem::get_zone_counts(std::uint8_t player_id) const {
    assert(player_id < MAX_OVERSEERS);
    return m_zone_counts[player_id];
}

const ZoneSystem::ZoneInfo* ZoneSystem::get_zone_info(std::int32_t x, std::int32_t y) const {
    if (!m_grid.in_bounds(x, y)) {
        return nullptr;
    }
    std::size_t index = static_cast<std::size_t>(y) * m_grid_width + static_cast<std::size_t>(x);
    if (index >= m_zone_info.size()) {
        return nullptr;
    }
    return &m_zone_info[index];
}

ZoneSystem::ZoneInfo* ZoneSystem::get_zone_info_mut(std::int32_t x, std::int32_t y) {
    if (!m_grid.in_bounds(x, y)) {
        return nullptr;
    }
    std::size_t index = static_cast<std::size_t>(y) * m_grid_width + static_cast<std::size_t>(x);
    if (index >= m_zone_info.size()) {
        return nullptr;
    }
    return &m_zone_info[index];
}

// =========================================================================
// Zone Placement Validation (Ticket 4-011)
// =========================================================================

ZoneSystem::ValidationResult ZoneSystem::validate_zone_placement(
    std::int32_t x, std::int32_t y, std::uint8_t player_id) const
{
    // (1) Bounds check
    if (!m_grid.in_bounds(x, y)) {
        return ValidationResult(false, ValidationResult::Reason::OutOfBounds);
    }

    // (2) Ownership check - valid player must be < MAX_OVERSEERS
    if (player_id >= MAX_OVERSEERS) {
        return ValidationResult(false, ValidationResult::Reason::NotOwned);
    }

    // (3) Terrain buildability check (skip if no terrain interface)
    if (m_terrain != nullptr) {
        if (!m_terrain->is_buildable(x, y)) {
            return ValidationResult(false, ValidationResult::Reason::TerrainNotBuildable);
        }
    }

    // (4) Zone overlap check
    if (m_grid.has_zone_at(x, y)) {
        return ValidationResult(false, ValidationResult::Reason::ZoneAlreadyPresent);
    }

    // (5) No building check yet (no building grid in ZoneSystem)

    return ValidationResult(true, ValidationResult::Reason::Ok);
}

ZonePlacementResult ZoneSystem::validate_zone_area(const ZonePlacementRequest& request) const {
    ZonePlacementResult result;

    for (std::int32_t dy = 0; dy < request.height; ++dy) {
        for (std::int32_t dx = 0; dx < request.width; ++dx) {
            std::int32_t tx = request.x + dx;
            std::int32_t ty = request.y + dy;

            ValidationResult vr = validate_zone_placement(tx, ty, request.player_id);
            if (vr.success) {
                ++result.placed_count;
            } else {
                ++result.skipped_count;
            }
        }
    }

    result.any_placed = result.placed_count > 0;
    return result;
}

// =========================================================================
// Zone State Event Access (Ticket 4-015)
// =========================================================================

const std::vector<ZoneStateChangedEvent>& ZoneSystem::get_pending_state_events() const {
    return m_pending_state_events;
}

void ZoneSystem::clear_pending_state_events() {
    m_pending_state_events.clear();
}

// =========================================================================
// Demand Configuration and Calculation (Ticket 4-016)
// =========================================================================

void ZoneSystem::set_demand_config(const DemandConfig& config) {
    m_demand_config = config;
}

const DemandConfig& ZoneSystem::get_demand_config() const {
    return m_demand_config;
}

ZoneDemandData ZoneSystem::get_zone_demand(std::uint8_t player_id) const {
    if (player_id >= MAX_OVERSEERS) {
        return ZoneDemandData();
    }

    // Ticket 4-017: delegate to external provider if available
    if (m_external_demand_provider != nullptr) {
        ZoneDemandData result;
        result.habitation_demand = get_demand_for_type(ZoneType::Habitation, player_id);
        result.exchange_demand = get_demand_for_type(ZoneType::Exchange, player_id);
        result.fabrication_demand = get_demand_for_type(ZoneType::Fabrication, player_id);
        return result;
    }

    return m_demand[player_id];
}

void ZoneSystem::update_demand() {
    for (std::uint8_t pid = 0; pid < MAX_OVERSEERS; ++pid) {
        const ZoneCounts& counts = m_zone_counts[pid];
        const DemandConfig& cfg = m_demand_config;

        // Calculate demand for each zone type using integer arithmetic
        // demand[type] = base_pressure + stub_factors - supply_saturation
        // supply_saturation = (occupied_count_for_type * 100) / target_zone_count

        // Habitation
        {
            std::int32_t base = static_cast<std::int32_t>(cfg.habitation_base);
            std::int32_t factors = static_cast<std::int32_t>(cfg.population_hab_factor)
                                 + static_cast<std::int32_t>(cfg.utility_factor)
                                 + static_cast<std::int32_t>(cfg.tribute_factor);
            std::int32_t saturation = 0;
            if (cfg.target_zone_count > 0) {
                saturation = (static_cast<std::int32_t>(counts.occupied_total) * 100)
                             / static_cast<std::int32_t>(cfg.target_zone_count);
            }
            std::int32_t raw = base + factors - saturation;

            // Soft cap
            std::int32_t threshold = static_cast<std::int32_t>(cfg.soft_cap_threshold);
            if (raw > threshold) {
                raw = threshold + (raw - threshold) / 2;
            } else if (raw < -threshold) {
                raw = -threshold + (raw + threshold) / 2;
            }

            // Clamp to [-100, +100]
            raw = std::max(static_cast<std::int32_t>(-100), std::min(static_cast<std::int32_t>(100), raw));
            m_demand[pid].habitation_demand = static_cast<std::int8_t>(raw);
        }

        // Exchange
        {
            std::int32_t base = static_cast<std::int32_t>(cfg.exchange_base);
            std::int32_t factors = static_cast<std::int32_t>(cfg.population_exc_factor)
                                 + static_cast<std::int32_t>(cfg.employment_factor)
                                 + static_cast<std::int32_t>(cfg.utility_factor)
                                 + static_cast<std::int32_t>(cfg.tribute_factor);
            std::int32_t saturation = 0;
            if (cfg.target_zone_count > 0) {
                saturation = (static_cast<std::int32_t>(counts.occupied_total) * 100)
                             / static_cast<std::int32_t>(cfg.target_zone_count);
            }
            std::int32_t raw = base + factors - saturation;

            // Soft cap
            std::int32_t threshold = static_cast<std::int32_t>(cfg.soft_cap_threshold);
            if (raw > threshold) {
                raw = threshold + (raw - threshold) / 2;
            } else if (raw < -threshold) {
                raw = -threshold + (raw + threshold) / 2;
            }

            // Clamp to [-100, +100]
            raw = std::max(static_cast<std::int32_t>(-100), std::min(static_cast<std::int32_t>(100), raw));
            m_demand[pid].exchange_demand = static_cast<std::int8_t>(raw);
        }

        // Fabrication
        {
            std::int32_t base = static_cast<std::int32_t>(cfg.fabrication_base);
            std::int32_t factors = static_cast<std::int32_t>(cfg.population_fab_factor)
                                 + static_cast<std::int32_t>(cfg.employment_factor)
                                 + static_cast<std::int32_t>(cfg.utility_factor)
                                 + static_cast<std::int32_t>(cfg.tribute_factor);
            std::int32_t saturation = 0;
            if (cfg.target_zone_count > 0) {
                saturation = (static_cast<std::int32_t>(counts.occupied_total) * 100)
                             / static_cast<std::int32_t>(cfg.target_zone_count);
            }
            std::int32_t raw = base + factors - saturation;

            // Soft cap
            std::int32_t threshold = static_cast<std::int32_t>(cfg.soft_cap_threshold);
            if (raw > threshold) {
                raw = threshold + (raw - threshold) / 2;
            } else if (raw < -threshold) {
                raw = -threshold + (raw + threshold) / 2;
            }

            // Clamp to [-100, +100]
            raw = std::max(static_cast<std::int32_t>(-100), std::min(static_cast<std::int32_t>(100), raw));
            m_demand[pid].fabrication_demand = static_cast<std::int8_t>(raw);
        }
    }
}

// =========================================================================
// Zone State Query (Ticket 4-024)
// =========================================================================

bool ZoneSystem::get_zone_state(std::int32_t x, std::int32_t y, ZoneState& out_state) const {
    const ZoneInfo* info = get_zone_info(x, y);
    if (!info || !info->valid) {
        return false;
    }
    out_state = info->component.getState();
    return true;
}

// =========================================================================
// Desirability Configuration and Calculation (Ticket 4-018)
// =========================================================================

void ZoneSystem::set_desirability_config(const DesirabilityConfig& config) {
    m_desirability_config = config;
}

const DesirabilityConfig& ZoneSystem::get_desirability_config() const {
    return m_desirability_config;
}

void ZoneSystem::update_desirability(std::int32_t x, std::int32_t y, std::uint8_t value) {
    ZoneInfo* info = get_zone_info_mut(x, y);
    if (info && info->valid) {
        info->component.desirability = value;
    }
}

uint8_t ZoneSystem::calculate_desirability(std::int32_t x, std::int32_t y) const {
    const DesirabilityConfig& cfg = m_desirability_config;

    // Terrain score: normalized to 0-255 from get_value_bonus()
    // get_value_bonus returns a float (can be negative for toxic terrain, positive for good)
    // We normalize: treat 0.0 as neutral (128), scale +-100 range to 0-255
    float terrain_bonus = (m_terrain != nullptr) ? m_terrain->get_value_bonus(x, y) : 50.0f;
    // Clamp to 0-255 range (treat the bonus directly as 0-255 range)
    float terrain_score = terrain_bonus;
    if (terrain_score < 0.0f) terrain_score = 0.0f;
    if (terrain_score > 255.0f) terrain_score = 255.0f;

    // Pathway proximity score (stub: always max proximity)
    float pathway_score = 255.0f;

    // Contamination score (stub: no contamination penalty, 255 = best)
    float contamination_score = 255.0f;

    // Service coverage score (stub: neutral)
    float service_score = 128.0f;

    // Weighted sum
    float weighted_sum = terrain_score * cfg.terrain_value_weight
                       + pathway_score * cfg.pathway_proximity_weight
                       + contamination_score * cfg.contamination_weight
                       + service_score * cfg.service_coverage_weight;

    // Clamp to 0-255
    if (weighted_sum < 0.0f) weighted_sum = 0.0f;
    if (weighted_sum > 255.0f) weighted_sum = 255.0f;

    return static_cast<uint8_t>(weighted_sum);
}

void ZoneSystem::update_all_desirability() {
    std::uint16_t grid_size = m_grid_width;
    for (std::int32_t y = 0; y < static_cast<std::int32_t>(grid_size); ++y) {
        for (std::int32_t x = 0; x < static_cast<std::int32_t>(grid_size); ++x) {
            std::size_t index = static_cast<std::size_t>(y) * m_grid_width + static_cast<std::size_t>(x);
            ZoneInfo& info = m_zone_info[index];
            if (info.valid) {
                info.component.desirability = calculate_desirability(x, y);
            }
        }
    }
}

// =========================================================================
// Zone Placement Execution (Ticket 4-012)
// =========================================================================

ZonePlacementResult ZoneSystem::place_zones(const ZonePlacementRequest& request) {
    ZonePlacementResult result;

    for (std::int32_t dy = 0; dy < request.height; ++dy) {
        for (std::int32_t dx = 0; dx < request.width; ++dx) {
            std::int32_t tx = request.x + dx;
            std::int32_t ty = request.y + dy;

            ValidationResult vr = validate_zone_placement(tx, ty, request.player_id);
            if (!vr.success) {
                ++result.skipped_count;
                continue;
            }

            // Assign auto-incrementing entity ID
            std::uint32_t entity_id = m_next_entity_id++;

            // Place the zone
            bool placed = place_zone(tx, ty, request.zone_type, request.density,
                                     request.player_id, entity_id);
            if (!placed) {
                ++result.skipped_count;
                continue;
            }

            ++result.placed_count;

            // Calculate cost for this zone
            std::uint32_t cost = (request.density == ZoneDensity::HighDensity)
                ? m_placement_cost_config.high_density_cost
                : m_placement_cost_config.low_density_cost;
            result.total_cost += cost;

            // Emit ZoneDesignatedEvent
            m_pending_designated_events.emplace_back(
                entity_id, tx, ty,
                request.zone_type, request.density,
                request.player_id);
        }
    }

    result.any_placed = result.placed_count > 0;
    return result;
}

void ZoneSystem::set_placement_cost_config(const PlacementCostConfig& config) {
    m_placement_cost_config = config;
}

const ZoneSystem::PlacementCostConfig& ZoneSystem::get_placement_cost_config() const {
    return m_placement_cost_config;
}

const std::vector<ZoneDesignatedEvent>& ZoneSystem::get_pending_designated_events() const {
    return m_pending_designated_events;
}

void ZoneSystem::clear_pending_designated_events() {
    m_pending_designated_events.clear();
}

// =========================================================================
// De-zoning Implementation (Ticket 4-013)
// =========================================================================

DezoneResult ZoneSystem::remove_zones(std::int32_t x, std::int32_t y,
                                       std::int32_t width, std::int32_t height,
                                       std::uint8_t player_id) {
    DezoneResult result;

    for (std::int32_t dy = 0; dy < height; ++dy) {
        for (std::int32_t dx = 0; dx < width; ++dx) {
            std::int32_t tx = x + dx;
            std::int32_t ty = y + dy;

            // Check zone exists
            const ZoneInfo* info = get_zone_info(tx, ty);
            if (!info || !info->valid) {
                ++result.skipped_count;
                continue;
            }

            // Check ownership
            if (info->player_id != player_id) {
                ++result.skipped_count;
                continue;
            }

            ZoneState state = info->component.getState();
            std::uint32_t entity_id = m_grid.get_zone_at(tx, ty);

            if (state == ZoneState::Occupied) {
                // Occupied zones: emit demolition request, do NOT remove yet
                m_pending_demolition_events.emplace_back(tx, ty, entity_id);
                ++result.demolition_requested_count;
            } else {
                // Designated or Stalled: remove immediately
                ZoneType zone_type = info->component.getZoneType();
                std::uint8_t owner = info->player_id;

                // Emit undesignated event before removal
                m_pending_undesignated_events.emplace_back(
                    entity_id, tx, ty, zone_type, owner);

                remove_zone_at(tx, ty);
                ++result.removed_count;
            }
        }
    }

    result.any_removed = result.removed_count > 0;
    return result;
}

void ZoneSystem::remove_zone_at(std::int32_t x, std::int32_t y) {
    ZoneInfo* info = get_zone_info_mut(x, y);
    if (!info || !info->valid) {
        return;
    }

    std::uint8_t pid = info->player_id;
    if (pid < MAX_OVERSEERS) {
        ZoneCounts& counts = m_zone_counts[pid];

        // Decrement type count
        switch (info->component.getZoneType()) {
            case ZoneType::Habitation:  if (counts.habitation_total > 0) --counts.habitation_total; break;
            case ZoneType::Exchange:    if (counts.exchange_total > 0) --counts.exchange_total; break;
            case ZoneType::Fabrication: if (counts.fabrication_total > 0) --counts.fabrication_total; break;
        }

        // Decrement density count
        switch (info->component.getDensity()) {
            case ZoneDensity::LowDensity:  if (counts.low_density_total > 0) --counts.low_density_total; break;
            case ZoneDensity::HighDensity: if (counts.high_density_total > 0) --counts.high_density_total; break;
        }

        // Decrement state count
        switch (info->component.getState()) {
            case ZoneState::Designated: if (counts.designated_total > 0) --counts.designated_total; break;
            case ZoneState::Occupied:   if (counts.occupied_total > 0) --counts.occupied_total; break;
            case ZoneState::Stalled:    if (counts.stalled_total > 0) --counts.stalled_total; break;
        }

        // Decrement total
        if (counts.total > 0) --counts.total;
    }

    // Clear zone info
    info->valid = false;
    info->component = ZoneComponent();
    info->player_id = 0;

    // Remove from grid
    m_grid.remove_zone(x, y);
}

bool ZoneSystem::finalize_zone_removal(std::int32_t x, std::int32_t y) {
    const ZoneInfo* info = get_zone_info(x, y);
    if (!info || !info->valid) {
        return false;
    }

    std::uint32_t entity_id = m_grid.get_zone_at(x, y);
    ZoneType zone_type = info->component.getZoneType();
    std::uint8_t owner = info->player_id;

    // Emit undesignated event
    m_pending_undesignated_events.emplace_back(
        entity_id, x, y, zone_type, owner);

    remove_zone_at(x, y);
    return true;
}

const std::vector<ZoneUndesignatedEvent>& ZoneSystem::get_pending_undesignated_events() const {
    return m_pending_undesignated_events;
}

void ZoneSystem::clear_pending_undesignated_events() {
    m_pending_undesignated_events.clear();
}

const std::vector<DemolitionRequestEvent>& ZoneSystem::get_pending_demolition_events() const {
    return m_pending_demolition_events;
}

void ZoneSystem::clear_pending_demolition_events() {
    m_pending_demolition_events.clear();
}

// =========================================================================
// External Demand Provider (Ticket 4-017)
// =========================================================================

void ZoneSystem::set_external_demand_provider(building::IDemandProvider* provider) {
    m_external_demand_provider = provider;
}

bool ZoneSystem::has_external_demand_provider() const {
    return m_external_demand_provider != nullptr;
}

// =========================================================================
// Zone Redesignation (Ticket 4-014)
// =========================================================================

RedesignateResult ZoneSystem::redesignate_zone(std::int32_t x, std::int32_t y,
                                                ZoneType new_type, ZoneDensity new_density,
                                                std::uint8_t player_id) {
    // Check zone exists at position
    ZoneInfo* info = get_zone_info_mut(x, y);
    if (!info || !info->valid) {
        return RedesignateResult(false, RedesignateResult::Reason::NoZoneAtPosition);
    }

    // Check ownership
    if (info->player_id != player_id) {
        return RedesignateResult(false, RedesignateResult::Reason::NotOwned);
    }

    ZoneType old_type = info->component.getZoneType();
    ZoneDensity old_density = info->component.getDensity();
    ZoneState state = info->component.getState();

    // Check if same type AND same density
    if (old_type == new_type && old_density == new_density) {
        return RedesignateResult(false, RedesignateResult::Reason::SameTypeAndDensity);
    }

    // Handle based on zone state
    if (state == ZoneState::Occupied) {
        if (old_type != new_type) {
            // Occupied + type change: emit DemolitionRequestEvent
            std::uint32_t entity_id = m_grid.get_zone_at(x, y);
            m_pending_demolition_events.emplace_back(x, y, entity_id);
            return RedesignateResult(false, RedesignateResult::Reason::OccupiedRequiresDemolition, true);
        } else {
            // Occupied + density-only change (CCR-005): directly update density
            if (player_id < MAX_OVERSEERS) {
                ZoneCounts& counts = m_zone_counts[player_id];
                // Decrement old density count
                switch (old_density) {
                    case ZoneDensity::LowDensity:  if (counts.low_density_total > 0) --counts.low_density_total; break;
                    case ZoneDensity::HighDensity: if (counts.high_density_total > 0) --counts.high_density_total; break;
                }
                // Increment new density count
                switch (new_density) {
                    case ZoneDensity::LowDensity:  ++counts.low_density_total; break;
                    case ZoneDensity::HighDensity: ++counts.high_density_total; break;
                }
            }
            info->component.setDensity(new_density);
            return RedesignateResult(true, RedesignateResult::Reason::Ok);
        }
    }

    // Designated or Stalled: directly update type and density
    if (player_id < MAX_OVERSEERS) {
        ZoneCounts& counts = m_zone_counts[player_id];

        // Decrement old type count
        switch (old_type) {
            case ZoneType::Habitation:  if (counts.habitation_total > 0) --counts.habitation_total; break;
            case ZoneType::Exchange:    if (counts.exchange_total > 0) --counts.exchange_total; break;
            case ZoneType::Fabrication: if (counts.fabrication_total > 0) --counts.fabrication_total; break;
        }
        // Increment new type count
        switch (new_type) {
            case ZoneType::Habitation:  ++counts.habitation_total; break;
            case ZoneType::Exchange:    ++counts.exchange_total; break;
            case ZoneType::Fabrication: ++counts.fabrication_total; break;
        }

        // Decrement old density count
        switch (old_density) {
            case ZoneDensity::LowDensity:  if (counts.low_density_total > 0) --counts.low_density_total; break;
            case ZoneDensity::HighDensity: if (counts.high_density_total > 0) --counts.high_density_total; break;
        }
        // Increment new density count
        switch (new_density) {
            case ZoneDensity::LowDensity:  ++counts.low_density_total; break;
            case ZoneDensity::HighDensity: ++counts.high_density_total; break;
        }
    }

    info->component.setZoneType(new_type);
    info->component.setDensity(new_density);

    return RedesignateResult(true, RedesignateResult::Reason::Ok);
}

// =========================================================================
// IZoneQueryable Implementation (Ticket 4-035)
// =========================================================================

bool ZoneSystem::get_zone_type_at(std::int32_t x, std::int32_t y, ZoneType& out_type) const {
    return get_zone_type(x, y, out_type);
}

bool ZoneSystem::get_zone_density_at(std::int32_t x, std::int32_t y, ZoneDensity& out_density) const {
    return get_zone_density(x, y, out_density);
}

bool ZoneSystem::is_zoned_at(std::int32_t x, std::int32_t y) const {
    return is_zoned(x, y);
}

std::uint32_t ZoneSystem::get_zone_count_for(std::uint8_t player_id, ZoneType type) const {
    return get_zone_count(player_id, type);
}

std::vector<GridPosition> ZoneSystem::get_designated_zones(std::uint8_t player_id, ZoneType type) const {
    std::vector<GridPosition> result;

    std::uint16_t grid_size = m_grid_width;
    for (std::int32_t y = 0; y < static_cast<std::int32_t>(grid_size); ++y) {
        for (std::int32_t x = 0; x < static_cast<std::int32_t>(grid_size); ++x) {
            std::size_t index = static_cast<std::size_t>(y) * m_grid_width + static_cast<std::size_t>(x);
            const ZoneInfo& info = m_zone_info[index];
            if (info.valid &&
                info.player_id == player_id &&
                info.component.getZoneType() == type &&
                info.component.getState() == ZoneState::Designated) {
                result.emplace_back(x, y);
            }
        }
    }

    return result;
}

float ZoneSystem::get_demand_for(ZoneType type, std::uint8_t player_id) const {
    return static_cast<float>(get_demand_for_type(type, player_id));
}

} // namespace zone
} // namespace sims3000
