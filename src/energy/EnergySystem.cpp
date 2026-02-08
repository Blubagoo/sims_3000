/**
 * @file EnergySystem.cpp
 * @brief Implementation of EnergySystem (Tickets 5-008, 5-009, 5-010, 5-014, 5-015, 5-016)
 *
 * 5-008: Skeleton with stub implementations.
 * 5-009: Real IEnergyProvider implementation via ECS registry queries.
 * 5-010: Nexus output calculation and per-tick update integration.
 * 5-014: BFS flood-fill coverage calculation and spatial position tracking.
 * 5-015: Coverage dirty flag event handlers and tick-driven recalculation.
 * 5-016: Ownership boundary enforcement check point in coverage BFS.
 * 5-024: Terrain bonus efficiency (ridges for wind).
 * 5-025: IContaminationSource and get_contamination_sources().
 * 5-026: Nexus placement validation.
 * 5-027: Conduit placement and validation.
 *
 * IEnergyProvider methods (is_powered, is_powered_at, get_energy_required,
 * get_energy_received) now query EnergyComponent on entities through the
 * entt::registry pointer set via set_registry(). If no registry is set,
 * safe defaults (false / 0) are returned.
 *
 * @see EnergySystem.h for class documentation.
 */

#include <sims3000/energy/EnergySystem.h>
#include <sims3000/energy/EnergyComponent.h>
#include <sims3000/energy/EnergyConduitComponent.h>
#include <sims3000/energy/NexusTypeConfig.h>
#include <sims3000/terrain/ITerrainQueryable.h>
#include <algorithm>
#include <cmath>
#include <queue>
#include <unordered_set>

namespace sims3000 {
namespace energy {

// =============================================================================
// Construction
// =============================================================================

EnergySystem::EnergySystem(uint32_t map_width, uint32_t map_height,
                           terrain::ITerrainQueryable* terrain)
    : m_registry(nullptr)
    , m_coverage_grid(map_width, map_height)
    , m_pools{}
    , m_coverage_dirty{}
    , m_nexus_ids{}
    , m_consumer_ids{}
    , m_terrain(terrain)
    , m_map_width(map_width)
    , m_map_height(map_height)
{
    // Initialize per-player pools with owner IDs
    for (uint8_t i = 0; i < MAX_PLAYERS; ++i) {
        m_pools[i].owner = i;
        m_coverage_dirty[i] = false;
    }
}

// =============================================================================
// ISimulatable interface (duck-typed)
// =============================================================================

void EnergySystem::tick(float /*delta_time*/) {
    // Clear transition event buffers before any phase (Ticket 5-021)
    clear_transition_events();

    // 0. Age all nexuses (Ticket 5-022)
    //    Must happen before output calculation so age_factor is current.
    if (m_registry) {
        for (uint8_t i = 0; i < MAX_PLAYERS; ++i) {
            for (uint32_t eid : m_nexus_ids[i]) {
                auto entity = static_cast<entt::entity>(eid);
                if (!m_registry->valid(entity)) {
                    continue;
                }
                auto* comp = m_registry->try_get<EnergyProducerComponent>(entity);
                if (comp) {
                    float old_age_factor = comp->age_factor;
                    update_nexus_aging(*comp);

                    // Emit NexusAgedEvent when efficiency drops past 10% threshold
                    // Check if we crossed a 0.10 boundary (e.g. 1.0->0.9, 0.9->0.8, etc.)
                    int old_bucket = static_cast<int>(old_age_factor * 10.0f);
                    int new_bucket = static_cast<int>(comp->age_factor * 10.0f);
                    if (new_bucket < old_bucket) {
                        on_nexus_aged(NexusAgedEvent(eid, i, comp->age_factor));
                    }
                }
            }
        }
    }

    // 1. Update nexus outputs for all players (Ticket 5-010)
    for (uint8_t i = 0; i < MAX_PLAYERS; ++i) {
        update_all_nexus_outputs(i);
    }

    // 2. Coverage BFS recomputation for dirty players (Ticket 5-015)
    for (uint8_t i = 0; i < MAX_PLAYERS; ++i) {
        if (m_coverage_dirty[i]) {
            recalculate_coverage(i);
        }
    }

    // 3. Pool calculation for all players (Ticket 5-012)
    //    Aggregates generation, consumption, surplus, and counts.
    for (uint8_t i = 0; i < MAX_PLAYERS; ++i) {
        calculate_pool(i);
    }

    // 4. Pool state machine (Ticket 5-013)
    for (uint8_t i = 0; i < MAX_PLAYERS; ++i) {
        m_pools[i].state = calculate_pool_state(m_pools[i]);
        detect_pool_state_transitions(i);
    }

    // 5. Snapshot power states before distribution (Ticket 5-020)
    for (uint8_t i = 0; i < MAX_PLAYERS; ++i) {
        snapshot_power_states(i);
    }

    // 5b. Energy distribution (Ticket 5-018, 5-019)
    for (uint8_t i = 0; i < MAX_PLAYERS; ++i) {
        distribute_energy(i);
    }

    // 6. Update conduit active states (Ticket 5-030)
    for (uint8_t i = 0; i < MAX_PLAYERS; ++i) {
        update_conduit_active_states(i);
    }

    // 7. Emit energy state change events (Ticket 5-020)
    m_state_change_events.clear();
    for (uint8_t i = 0; i < MAX_PLAYERS; ++i) {
        emit_state_change_events(i);
    }
}

int EnergySystem::get_priority() const {
    return 10;
}

// =============================================================================
// Registry access (Ticket 5-009)
// =============================================================================

void EnergySystem::set_registry(entt::registry* registry) {
    m_registry = registry;
}

// =============================================================================
// IEnergyProvider interface (Ticket 5-009)
// =============================================================================

bool EnergySystem::is_powered(uint32_t entity_id) const {
    if (!m_registry) {
        return false;
    }
    auto entity = static_cast<entt::entity>(entity_id);
    if (!m_registry->valid(entity)) {
        return false;
    }
    const auto* ec = m_registry->try_get<EnergyComponent>(entity);
    if (!ec) {
        return false;
    }
    return ec->is_powered;
}

bool EnergySystem::is_powered_at(uint32_t x, uint32_t y, uint32_t player_id) const {
    if (player_id >= MAX_PLAYERS) {
        return false;
    }
    // Coverage grid stores overseer_id (1-based): overseer_id = player_id + 1
    auto overseer_id = static_cast<uint8_t>(player_id + 1);
    if (!is_in_coverage(x, y, overseer_id)) {
        return false;
    }
    // Pool is indexed by player_id (0-based)
    return m_pools[player_id].surplus >= 0;
}

// =============================================================================
// Nexus aging (Ticket 5-022)
// =============================================================================

/// Default decay rate for nexus aging (slow decay over thousands of ticks)
static constexpr float NEXUS_AGING_DECAY_RATE = 0.0001f;

/**
 * @brief Get the aging floor for a given NexusType.
 *
 * Returns the type-specific minimum efficiency per CCR-006:
 *   Carbon=0.60, Petro=0.65, Gaseous=0.70, Nuclear=0.75, Wind=0.80, Solar=0.85
 *
 * Falls back to the NexusTypeConfig aging_floor for MVP types,
 * or 0.60f for non-MVP types.
 */
static float get_aging_floor(NexusType type) {
    if (static_cast<uint8_t>(type) < NEXUS_TYPE_MVP_COUNT) {
        return get_nexus_config(type).aging_floor;
    }
    return 0.60f; // fallback for non-MVP types
}

void EnergySystem::update_nexus_aging(EnergyProducerComponent& comp) {
    // Increment ticks_since_built, capped at 65535 (uint16_t max)
    if (comp.ticks_since_built < 65535) {
        comp.ticks_since_built++;
    }

    // Calculate age_factor using asymptotic decay curve
    NexusType type = static_cast<NexusType>(comp.nexus_type);
    float floor = get_aging_floor(type);
    float ticks = static_cast<float>(comp.ticks_since_built);
    comp.age_factor = floor + (1.0f - floor) * std::exp(-NEXUS_AGING_DECAY_RATE * ticks);
}

// =============================================================================
// Nexus output calculation (Ticket 5-010)
// =============================================================================

/// Weather stub factor applied to Wind and Solar nexuses
static constexpr float WEATHER_STUB_FACTOR = 0.75f;

void EnergySystem::update_nexus_output(EnergyProducerComponent& comp) {
    if (!comp.is_online) {
        comp.current_output = 0;
        comp.contamination_output = 0;  // CCR-007: no contamination when offline
        return;
    }

    float output = static_cast<float>(comp.base_output) * comp.efficiency * comp.age_factor;

    // Wind and Solar are variable output types - apply weather stub factor
    NexusType type = static_cast<NexusType>(comp.nexus_type);
    if (type == NexusType::Wind || type == NexusType::Solar) {
        output *= WEATHER_STUB_FACTOR;
    }

    comp.current_output = static_cast<uint32_t>(output);

    // CCR-007: zero contamination when not producing
    if (comp.current_output == 0) {
        comp.contamination_output = 0;
    }
}

void EnergySystem::update_all_nexus_outputs(uint8_t owner) {
    if (owner >= MAX_PLAYERS || !m_registry) {
        return;
    }
    for (uint32_t eid : m_nexus_ids[owner]) {
        auto entity = static_cast<entt::entity>(eid);
        if (!m_registry->valid(entity)) {
            continue;
        }
        auto* comp = m_registry->try_get<EnergyProducerComponent>(entity);
        if (comp) {
            update_nexus_output(*comp);

            // Ticket 5-024: Apply terrain efficiency bonus if nexus has a position
            if (comp->current_output > 0) {
                NexusType ntype = static_cast<NexusType>(comp->nexus_type);
                // Look up nexus position for terrain bonus query
                for (const auto& pos_pair : m_nexus_positions[owner]) {
                    if (pos_pair.second == eid) {
                        uint32_t nx = unpack_x(pos_pair.first);
                        uint32_t ny = unpack_y(pos_pair.first);
                        float bonus = get_terrain_efficiency_bonus(ntype, nx, ny);
                        if (bonus != 1.0f) {
                            comp->current_output = static_cast<uint32_t>(
                                static_cast<float>(comp->current_output) * bonus);
                        }
                        break;
                    }
                }
            }
        }
    }
}

uint32_t EnergySystem::get_total_generation(uint8_t owner) const {
    if (owner >= MAX_PLAYERS || !m_registry) {
        return 0;
    }
    uint32_t total = 0;
    for (uint32_t eid : m_nexus_ids[owner]) {
        auto entity = static_cast<entt::entity>(eid);
        if (!m_registry->valid(entity)) {
            continue;
        }
        const auto* comp = m_registry->try_get<EnergyProducerComponent>(entity);
        if (comp) {
            total += comp->current_output;
        }
    }
    return total;
}

// =============================================================================
// Placement validation (Ticket 5-026, 5-027)
// =============================================================================

PlacementResult EnergySystem::validate_nexus_placement(NexusType type, uint32_t x,
                                                       uint32_t y, uint8_t owner) const {
    PlacementResult result;

    // 1. Bounds check
    if (x >= m_map_width || y >= m_map_height) {
        result.success = false;
        result.reason = "Position out of bounds";
        return result;
    }

    // 2. Ownership check (stub: always true for now)
    // Future: check if player owns the tile at (x, y)
    (void)owner;

    // 3. Terrain buildable check
    if (m_terrain) {
        if (!m_terrain->is_buildable(static_cast<int32_t>(x), static_cast<int32_t>(y))) {
            result.success = false;
            result.reason = "Terrain not buildable";
            return result;
        }
    }
    // If terrain is nullptr, default to buildable (stub)

    // 4. No existing structure check (stub: always passes for now)
    // Future: check if there's already a structure at (x, y)

    // 5. Type-specific terrain requirements
    // Hydro and Geothermal are stubbed as always valid for MVP
    (void)type;

    result.success = true;
    result.reason = "";
    return result;
}

PlacementResult EnergySystem::validate_conduit_placement(uint32_t x, uint32_t y,
                                                         uint8_t owner) const {
    PlacementResult result;

    // 1. Bounds check
    if (x >= m_map_width || y >= m_map_height) {
        result.success = false;
        result.reason = "Position out of bounds";
        return result;
    }

    // 2. Ownership check (stub: always true for now)
    // Future: check if player owns the tile at (x, y)
    (void)owner;

    // 3. Terrain buildable check
    if (m_terrain) {
        if (!m_terrain->is_buildable(static_cast<int32_t>(x), static_cast<int32_t>(y))) {
            result.success = false;
            result.reason = "Terrain not buildable";
            return result;
        }
    }
    // If terrain is nullptr, default to buildable (stub)

    // 4. No existing structure check (stub: always passes for now)
    // Future: check if there's already a structure at (x, y)

    result.success = true;
    result.reason = "";
    return result;
}

uint32_t EnergySystem::place_nexus(NexusType type, uint32_t x, uint32_t y, uint8_t owner) {
    // Require registry
    if (!m_registry) {
        return INVALID_ENTITY_ID;
    }

    // Validate placement
    PlacementResult validation = validate_nexus_placement(type, x, y, owner);
    if (!validation.success) {
        return INVALID_ENTITY_ID;
    }

    // Create entity with EnergyProducerComponent
    auto entity = m_registry->create();
    uint32_t entity_id = static_cast<uint32_t>(entity);

    // Initialize producer component from NexusTypeConfig
    const NexusTypeConfig& config = get_nexus_config(type);
    EnergyProducerComponent producer{};
    producer.base_output = config.base_output;
    producer.current_output = 0;
    producer.efficiency = 1.0f;
    producer.age_factor = 1.0f;
    producer.ticks_since_built = 0;
    producer.nexus_type = static_cast<uint8_t>(type);
    producer.is_online = true;
    producer.contamination_output = config.contamination;
    m_registry->emplace<EnergyProducerComponent>(entity, producer);

    // Register nexus and position
    register_nexus(entity_id, owner);
    register_nexus_position(entity_id, owner, x, y);

    // Coverage dirty is already set by register_nexus and register_nexus_position

    return entity_id;
}

uint32_t EnergySystem::place_conduit(uint32_t x, uint32_t y, uint8_t owner) {
    // Require registry
    if (!m_registry) {
        return INVALID_ENTITY_ID;
    }

    // Validate placement
    PlacementResult validation = validate_conduit_placement(x, y, owner);
    if (!validation.success) {
        return INVALID_ENTITY_ID;
    }

    // Create entity with EnergyConduitComponent
    auto entity = m_registry->create();
    uint32_t entity_id = static_cast<uint32_t>(entity);

    EnergyConduitComponent conduit{};
    conduit.coverage_radius = 3;
    conduit.is_connected = false;
    conduit.is_active = false;
    conduit.conduit_level = 1;
    m_registry->emplace<EnergyConduitComponent>(entity, conduit);

    // Register conduit position
    register_conduit_position(entity_id, owner, x, y);

    // Coverage dirty is already set by register_conduit_position

    // Cost deduction stub: DEFAULT_CONDUIT_COST = 10 credits (not actually deducted yet)
    // Future: call ICreditProvider.deduct_credits(owner, DEFAULT_CONDUIT_COST)

    // Emit ConduitPlacedEvent
    on_conduit_placed(ConduitPlacedEvent(entity_id, owner,
                                         static_cast<int32_t>(x),
                                         static_cast<int32_t>(y)));

    return entity_id;
}

// =============================================================================
// Conduit removal (Ticket 5-029)
// =============================================================================

bool EnergySystem::remove_conduit(uint32_t entity_id, uint8_t owner,
                                   uint32_t x, uint32_t y) {
    // Require registry
    if (!m_registry) {
        return false;
    }

    // Validate owner
    if (owner >= MAX_PLAYERS) {
        return false;
    }

    // Validate entity exists
    auto entity = static_cast<entt::entity>(entity_id);
    if (!m_registry->valid(entity)) {
        return false;
    }

    // Validate entity has EnergyConduitComponent
    if (!m_registry->all_of<EnergyConduitComponent>(entity)) {
        return false;
    }

    // Unregister conduit position (also sets coverage_dirty)
    unregister_conduit_position(entity_id, owner, x, y);

    // Emit ConduitRemovedEvent
    on_conduit_removed(ConduitRemovedEvent(entity_id, owner,
                                            static_cast<int32_t>(x),
                                            static_cast<int32_t>(y)));

    // Destroy entity from registry
    m_registry->destroy(entity);

    return true;
}

// =============================================================================
// Nexus management
// =============================================================================

void EnergySystem::register_nexus(uint32_t entity_id, uint8_t owner) {
    if (owner >= MAX_PLAYERS) {
        return;
    }
    m_nexus_ids[owner].push_back(entity_id);
    m_coverage_dirty[owner] = true;
}

void EnergySystem::unregister_nexus(uint32_t entity_id, uint8_t owner) {
    if (owner >= MAX_PLAYERS) {
        return;
    }
    auto& ids = m_nexus_ids[owner];
    auto it = std::find(ids.begin(), ids.end(), entity_id);
    if (it != ids.end()) {
        ids.erase(it);
        m_coverage_dirty[owner] = true;
    }
}

// =============================================================================
// Consumer management
// =============================================================================

void EnergySystem::register_consumer(uint32_t entity_id, uint8_t owner) {
    if (owner >= MAX_PLAYERS) {
        return;
    }
    m_consumer_ids[owner].push_back(entity_id);
}

void EnergySystem::unregister_consumer(uint32_t entity_id, uint8_t owner) {
    if (owner >= MAX_PLAYERS) {
        return;
    }
    auto& ids = m_consumer_ids[owner];
    auto it = std::find(ids.begin(), ids.end(), entity_id);
    if (it != ids.end()) {
        ids.erase(it);
    }
}

// =============================================================================
// Consumer aggregation (Ticket 5-011)
// =============================================================================

void EnergySystem::register_consumer_position(uint32_t entity_id, uint8_t owner,
                                              uint32_t x, uint32_t y) {
    if (owner >= MAX_PLAYERS) {
        return;
    }
    uint64_t key = pack_position(x, y);
    m_consumer_positions[owner][key] = entity_id;
}

void EnergySystem::unregister_consumer_position(uint32_t /*entity_id*/, uint8_t owner,
                                                uint32_t x, uint32_t y) {
    if (owner >= MAX_PLAYERS) {
        return;
    }
    uint64_t key = pack_position(x, y);
    m_consumer_positions[owner].erase(key);
}

uint32_t EnergySystem::get_consumer_position_count(uint8_t owner) const {
    if (owner >= MAX_PLAYERS) {
        return 0;
    }
    return static_cast<uint32_t>(m_consumer_positions[owner].size());
}

uint32_t EnergySystem::aggregate_consumption(uint8_t owner) const {
    if (owner >= MAX_PLAYERS || !m_registry) {
        return 0;
    }
    // Coverage grid uses overseer_id (1-based): overseer_id = player_id + 1
    uint8_t overseer_id = owner + 1;

    uint32_t total = 0;
    for (const auto& pair : m_consumer_positions[owner]) {
        uint32_t x = unpack_x(pair.first);
        uint32_t y = unpack_y(pair.first);

        // Only count consumers that are in coverage
        if (!m_coverage_grid.is_in_coverage(x, y, overseer_id)) {
            continue;
        }

        uint32_t entity_id = pair.second;
        auto entity = static_cast<entt::entity>(entity_id);
        if (!m_registry->valid(entity)) {
            continue;
        }
        const auto* ec = m_registry->try_get<EnergyComponent>(entity);
        if (!ec) {
            continue;
        }
        total += ec->energy_required;
    }
    return total;
}

// =============================================================================
// Coverage queries
// =============================================================================

bool EnergySystem::is_in_coverage(uint32_t x, uint32_t y, uint8_t owner) const {
    return m_coverage_grid.is_in_coverage(x, y, owner);
}

uint8_t EnergySystem::get_coverage_at(uint32_t x, uint32_t y) const {
    return m_coverage_grid.get_coverage_owner(x, y);
}

uint32_t EnergySystem::get_coverage_count(uint8_t owner) const {
    return m_coverage_grid.get_coverage_count(owner);
}

// =============================================================================
// Pool calculation (Ticket 5-012)
// =============================================================================

void EnergySystem::calculate_pool(uint8_t owner) {
    if (owner >= MAX_PLAYERS) {
        return;
    }
    PerPlayerEnergyPool& pool = m_pools[owner];
    pool.total_generated = get_total_generation(owner);
    pool.total_consumed = aggregate_consumption(owner);
    pool.surplus = static_cast<int32_t>(pool.total_generated)
                 - static_cast<int32_t>(pool.total_consumed);
    pool.nexus_count = get_nexus_count(owner);
    pool.consumer_count = get_consumer_count(owner);
}

// =============================================================================
// Pool queries
// =============================================================================

const PerPlayerEnergyPool& EnergySystem::get_pool(uint8_t owner) const {
    if (owner >= MAX_PLAYERS) {
        // Return player 0 pool as safe fallback
        return m_pools[0];
    }
    return m_pools[owner];
}

PerPlayerEnergyPool& EnergySystem::get_pool_mut(uint8_t owner) {
    if (owner >= MAX_PLAYERS) {
        // Return player 0 pool as safe fallback
        return m_pools[0];
    }
    return m_pools[owner];
}

EnergyPoolState EnergySystem::get_pool_state(uint8_t owner) const {
    if (owner >= MAX_PLAYERS) {
        return EnergyPoolState::Healthy;
    }
    return m_pools[owner].state;
}

// =============================================================================
// Pool state machine (Ticket 5-013)
// =============================================================================

EnergyPoolState EnergySystem::calculate_pool_state(const PerPlayerEnergyPool& pool) {
    // Calculate thresholds
    float buffer_threshold = static_cast<float>(pool.total_generated) * BUFFER_THRESHOLD_PERCENT;
    float collapse_threshold = static_cast<float>(pool.total_consumed) * COLLAPSE_THRESHOLD_PERCENT;

    float surplus_f = static_cast<float>(pool.surplus);

    if (surplus_f >= buffer_threshold) {
        return EnergyPoolState::Healthy;
    }
    if (surplus_f >= 0.0f) {
        return EnergyPoolState::Marginal;
    }
    // surplus < 0
    if (surplus_f <= -collapse_threshold) {
        return EnergyPoolState::Collapse;
    }
    return EnergyPoolState::Deficit;
}

void EnergySystem::detect_pool_state_transitions(uint8_t owner) {
    if (owner >= MAX_PLAYERS) {
        return;
    }

    PerPlayerEnergyPool& pool = m_pools[owner];
    EnergyPoolState prev = pool.previous_state;
    EnergyPoolState curr = pool.state;

    // Transition INTO Deficit or Collapse (from Healthy or Marginal)
    if ((curr == EnergyPoolState::Deficit || curr == EnergyPoolState::Collapse) &&
        (prev == EnergyPoolState::Healthy || prev == EnergyPoolState::Marginal)) {
        // Emit deficit began event (Ticket 5-021)
        m_deficit_began_events.emplace_back(owner, pool.surplus, pool.consumer_count);
    }

    // Transition OUT OF Deficit or Collapse (to Healthy or Marginal)
    if ((curr == EnergyPoolState::Healthy || curr == EnergyPoolState::Marginal) &&
        (prev == EnergyPoolState::Deficit || prev == EnergyPoolState::Collapse)) {
        // Emit deficit ended event (Ticket 5-021)
        m_deficit_ended_events.emplace_back(owner, pool.surplus);
    }

    // Transition INTO Collapse
    if (curr == EnergyPoolState::Collapse && prev != EnergyPoolState::Collapse) {
        // Emit grid collapse began event (Ticket 5-021)
        m_collapse_began_events.emplace_back(owner, pool.surplus);
    }

    // Transition OUT OF Collapse
    if (curr != EnergyPoolState::Collapse && prev == EnergyPoolState::Collapse) {
        // Emit grid collapse ended event (Ticket 5-021)
        m_collapse_ended_events.emplace_back(owner);
    }

    pool.previous_state = curr;
}

// =============================================================================
// Energy distribution (Ticket 5-018)
// =============================================================================

void EnergySystem::distribute_energy(uint8_t owner) {
    if (owner >= MAX_PLAYERS || !m_registry) {
        return;
    }

    uint8_t overseer_id = owner + 1;
    bool has_surplus = m_pools[owner].surplus >= 0;

    if (!has_surplus) {
        // Deficit/Collapse: apply priority-based rationing (Ticket 5-019)
        apply_rationing(owner);
        return;
    }

    for (const auto& pair : m_consumer_positions[owner]) {
        uint32_t x = unpack_x(pair.first);
        uint32_t y = unpack_y(pair.first);
        uint32_t entity_id = pair.second;

        auto entity = static_cast<entt::entity>(entity_id);
        if (!m_registry->valid(entity)) {
            continue;
        }
        auto* ec = m_registry->try_get<EnergyComponent>(entity);
        if (!ec) {
            continue;
        }

        // Check if consumer is in coverage
        if (!m_coverage_grid.is_in_coverage(x, y, overseer_id)) {
            // Outside coverage: always unpowered
            ec->is_powered = false;
            ec->energy_received = 0;
            continue;
        }

        // Healthy/Marginal: all consumers in coverage get powered
        ec->is_powered = true;
        ec->energy_received = ec->energy_required;
    }
}

// =============================================================================
// Energy component queries (Ticket 5-009)
// =============================================================================

uint32_t EnergySystem::get_energy_required(uint32_t entity_id) const {
    if (!m_registry) {
        return 0;
    }
    auto entity = static_cast<entt::entity>(entity_id);
    if (!m_registry->valid(entity)) {
        return 0;
    }
    const auto* ec = m_registry->try_get<EnergyComponent>(entity);
    if (!ec) {
        return 0;
    }
    return ec->energy_required;
}

uint32_t EnergySystem::get_energy_received(uint32_t entity_id) const {
    if (!m_registry) {
        return 0;
    }
    auto entity = static_cast<entt::entity>(entity_id);
    if (!m_registry->valid(entity)) {
        return 0;
    }
    const auto* ec = m_registry->try_get<EnergyComponent>(entity);
    if (!ec) {
        return 0;
    }
    return ec->energy_received;
}

// =============================================================================
// Coverage dirty management
// =============================================================================

void EnergySystem::mark_coverage_dirty(uint8_t owner) {
    if (owner >= MAX_PLAYERS) {
        return;
    }
    m_coverage_dirty[owner] = true;
}

bool EnergySystem::is_coverage_dirty(uint8_t owner) const {
    if (owner >= MAX_PLAYERS) {
        return false;
    }
    return m_coverage_dirty[owner];
}

// =============================================================================
// Event handlers (Ticket 5-015)
// =============================================================================

void EnergySystem::on_conduit_placed(const ConduitPlacedEvent& event) {
    if (event.owner_id >= MAX_PLAYERS) {
        return;
    }
    m_coverage_dirty[event.owner_id] = true;
}

void EnergySystem::on_conduit_removed(const ConduitRemovedEvent& event) {
    if (event.owner_id >= MAX_PLAYERS) {
        return;
    }
    m_coverage_dirty[event.owner_id] = true;
}

void EnergySystem::on_nexus_placed(const NexusPlacedEvent& event) {
    if (event.owner_id >= MAX_PLAYERS) {
        return;
    }
    m_coverage_dirty[event.owner_id] = true;
}

void EnergySystem::on_nexus_removed(const NexusRemovedEvent& event) {
    if (event.owner_id >= MAX_PLAYERS) {
        return;
    }
    m_coverage_dirty[event.owner_id] = true;
}

void EnergySystem::on_nexus_aged(const NexusAgedEvent& /*event*/) {
    // Ticket 5-022: NexusAgedEvent handler stub.
    // Future subscribers (UISystem, StatisticsSystem) will react to this.
    // No action needed internally for now.
}

// =============================================================================
// Ownership boundary enforcement (Ticket 5-016)
// =============================================================================

bool EnergySystem::can_extend_coverage_to(uint32_t /*x*/, uint32_t /*y*/,
                                           uint8_t /*owner*/) const {
    // Currently always returns true since there is no territory/ownership
    // system yet. When territory boundaries are implemented, this method
    // should check:
    //   - Returns true if tile_owner == owner OR tile_owner == GAME_MASTER (unclaimed)
    //   - Returns false if tile_owner belongs to a different player
    return true;
}

// =============================================================================
// Grid accessors
// =============================================================================

const CoverageGrid& EnergySystem::get_coverage_grid() const {
    return m_coverage_grid;
}

CoverageGrid& EnergySystem::get_coverage_grid_mut() {
    return m_coverage_grid;
}

uint32_t EnergySystem::get_map_width() const {
    return m_map_width;
}

uint32_t EnergySystem::get_map_height() const {
    return m_map_height;
}

// =============================================================================
// Entity list accessors
// =============================================================================

uint32_t EnergySystem::get_nexus_count(uint8_t owner) const {
    if (owner >= MAX_PLAYERS) {
        return 0;
    }
    return static_cast<uint32_t>(m_nexus_ids[owner].size());
}

uint32_t EnergySystem::get_consumer_count(uint8_t owner) const {
    if (owner >= MAX_PLAYERS) {
        return 0;
    }
    return static_cast<uint32_t>(m_consumer_ids[owner].size());
}

// =============================================================================
// Spatial position tracking (Ticket 5-014)
// =============================================================================

void EnergySystem::register_conduit_position(uint32_t entity_id, uint8_t owner,
                                             uint32_t x, uint32_t y) {
    if (owner >= MAX_PLAYERS) {
        return;
    }
    uint64_t key = pack_position(x, y);
    m_conduit_positions[owner][key] = entity_id;
    m_coverage_dirty[owner] = true;
}

void EnergySystem::unregister_conduit_position(uint32_t /*entity_id*/, uint8_t owner,
                                               uint32_t x, uint32_t y) {
    if (owner >= MAX_PLAYERS) {
        return;
    }
    uint64_t key = pack_position(x, y);
    m_conduit_positions[owner].erase(key);
    m_coverage_dirty[owner] = true;
}

void EnergySystem::register_nexus_position(uint32_t entity_id, uint8_t owner,
                                           uint32_t x, uint32_t y) {
    if (owner >= MAX_PLAYERS) {
        return;
    }
    uint64_t key = pack_position(x, y);
    m_nexus_positions[owner][key] = entity_id;
    m_coverage_dirty[owner] = true;
}

void EnergySystem::unregister_nexus_position(uint32_t /*entity_id*/, uint8_t owner,
                                             uint32_t x, uint32_t y) {
    if (owner >= MAX_PLAYERS) {
        return;
    }
    uint64_t key = pack_position(x, y);
    m_nexus_positions[owner].erase(key);
    m_coverage_dirty[owner] = true;
}

uint32_t EnergySystem::get_conduit_position_count(uint8_t owner) const {
    if (owner >= MAX_PLAYERS) {
        return 0;
    }
    return static_cast<uint32_t>(m_conduit_positions[owner].size());
}

uint32_t EnergySystem::get_nexus_position_count(uint8_t owner) const {
    if (owner >= MAX_PLAYERS) {
        return 0;
    }
    return static_cast<uint32_t>(m_nexus_positions[owner].size());
}

// =============================================================================
// Spatial lookup helpers (Ticket 5-014)
// =============================================================================

uint64_t EnergySystem::pack_position(uint32_t x, uint32_t y) {
    return (static_cast<uint64_t>(x) << 32) | static_cast<uint64_t>(y);
}

uint32_t EnergySystem::unpack_x(uint64_t packed) {
    return static_cast<uint32_t>(packed >> 32);
}

uint32_t EnergySystem::unpack_y(uint64_t packed) {
    return static_cast<uint32_t>(packed & 0xFFFFFFFF);
}

// =============================================================================
// Coverage BFS algorithm (Ticket 5-014)
// =============================================================================

void EnergySystem::mark_coverage_radius(uint32_t cx, uint32_t cy, uint8_t radius,
                                        uint8_t owner_id) {
    // Calculate clamped bounds for the square coverage area.
    // Use int64_t to safely handle underflow when cx < radius.
    int64_t min_x = static_cast<int64_t>(cx) - static_cast<int64_t>(radius);
    int64_t min_y = static_cast<int64_t>(cy) - static_cast<int64_t>(radius);
    int64_t max_x = static_cast<int64_t>(cx) + static_cast<int64_t>(radius);
    int64_t max_y = static_cast<int64_t>(cy) + static_cast<int64_t>(radius);

    // Clamp to grid bounds
    if (min_x < 0) min_x = 0;
    if (min_y < 0) min_y = 0;
    if (max_x >= static_cast<int64_t>(m_map_width)) max_x = static_cast<int64_t>(m_map_width) - 1;
    if (max_y >= static_cast<int64_t>(m_map_height)) max_y = static_cast<int64_t>(m_map_height) - 1;

    // Mark all cells in the clamped square
    for (int64_t y = min_y; y <= max_y; ++y) {
        for (int64_t x = min_x; x <= max_x; ++x) {
            m_coverage_grid.set(static_cast<uint32_t>(x), static_cast<uint32_t>(y), owner_id);
        }
    }
}

void EnergySystem::recalculate_coverage(uint8_t owner) {
    if (owner >= MAX_PLAYERS) {
        return;
    }

    // The coverage owner_id is owner+1 (0 means uncovered in the grid)
    uint8_t owner_id = owner + 1;

    // Step 0: Reset all conduits' is_connected to false for this owner (Ticket 5-028)
    if (m_registry) {
        for (const auto& pair : m_conduit_positions[owner]) {
            uint32_t entity_id = pair.second;
            auto entity = static_cast<entt::entity>(entity_id);
            if (m_registry->valid(entity)) {
                auto* conduit = m_registry->try_get<EnergyConduitComponent>(entity);
                if (conduit) {
                    conduit->is_connected = false;
                }
            }
        }
    }

    // Step 1: Clear all existing coverage for this owner
    m_coverage_grid.clear_all_for_owner(owner_id);

    // Step 2: Seed BFS from nexus positions
    std::queue<uint64_t> frontier;
    std::unordered_set<uint64_t> visited;

    for (const auto& pair : m_nexus_positions[owner]) {
        uint64_t packed_pos = pair.first;
        uint32_t entity_id = pair.second;
        uint32_t nx = unpack_x(packed_pos);
        uint32_t ny = unpack_y(packed_pos);

        // Determine nexus coverage radius from NexusTypeConfig
        uint8_t radius = 8; // default fallback (Carbon radius)
        if (m_registry) {
            auto entity = static_cast<entt::entity>(entity_id);
            if (m_registry->valid(entity)) {
                const auto* producer = m_registry->try_get<EnergyProducerComponent>(entity);
                if (producer) {
                    NexusType ntype = static_cast<NexusType>(producer->nexus_type);
                    const NexusTypeConfig& config = get_nexus_config(ntype);
                    radius = config.coverage_radius;
                }
            }
        }

        // Mark coverage area around the nexus
        mark_coverage_radius(nx, ny, radius, owner_id);

        // Add nexus position to BFS frontier
        if (visited.find(packed_pos) == visited.end()) {
            visited.insert(packed_pos);
            frontier.push(packed_pos);
        }
    }

    // Step 3: BFS through conduit network
    // 4-directional neighbor offsets: right, left, down, up
    static const int32_t dx[] = { 1, -1, 0, 0 };
    static const int32_t dy[] = { 0, 0, 1, -1 };

    while (!frontier.empty()) {
        uint64_t current = frontier.front();
        frontier.pop();

        uint32_t cx = unpack_x(current);
        uint32_t cy = unpack_y(current);

        // Check 4-directional neighbors
        for (int i = 0; i < 4; ++i) {
            int64_t neighbor_x = static_cast<int64_t>(cx) + dx[i];
            int64_t neighbor_y = static_cast<int64_t>(cy) + dy[i];

            // Bounds check
            if (neighbor_x < 0 || neighbor_x >= static_cast<int64_t>(m_map_width) ||
                neighbor_y < 0 || neighbor_y >= static_cast<int64_t>(m_map_height)) {
                continue;
            }

            uint32_t nbx = static_cast<uint32_t>(neighbor_x);
            uint32_t nby = static_cast<uint32_t>(neighbor_y);
            uint64_t neighbor_key = pack_position(nbx, nby);

            // Skip if already visited
            if (visited.find(neighbor_key) != visited.end()) {
                continue;
            }

            // Ownership boundary check (Ticket 5-016): verify coverage
            // can extend to this tile for the given owner. Currently always
            // returns true, but provides the integration point for when
            // territory boundaries are implemented.
            if (!can_extend_coverage_to(nbx, nby, owner)) {
                continue;
            }

            // Check if there's a conduit at this position for this owner
            auto conduit_it = m_conduit_positions[owner].find(neighbor_key);
            if (conduit_it == m_conduit_positions[owner].end()) {
                continue;
            }

            // Found a conduit - mark it visited and add to frontier
            visited.insert(neighbor_key);
            frontier.push(neighbor_key);

            // Determine conduit coverage radius and mark as connected (Ticket 5-028)
            uint8_t conduit_radius = 3; // default from EnergyConduitComponent
            if (m_registry) {
                auto entity = static_cast<entt::entity>(conduit_it->second);
                if (m_registry->valid(entity)) {
                    auto* conduit = m_registry->try_get<EnergyConduitComponent>(entity);
                    if (conduit) {
                        conduit_radius = conduit->coverage_radius;
                        conduit->is_connected = true; // Ticket 5-028: reachable from nexus
                    }
                }
            }

            // Mark coverage area around the conduit
            mark_coverage_radius(nbx, nby, conduit_radius, owner_id);
        }
    }

    // Clear dirty flag after recalculation
    m_coverage_dirty[owner] = false;
}

// =============================================================================
// Conduit placement preview (Ticket 5-031)
// =============================================================================

std::vector<std::pair<uint32_t, uint32_t>> EnergySystem::preview_conduit_coverage(
    uint32_t x, uint32_t y, uint8_t owner) const {
    std::vector<std::pair<uint32_t, uint32_t>> delta;

    // Validate inputs
    if (owner >= MAX_PLAYERS) {
        return delta;
    }
    if (x >= m_map_width || y >= m_map_height) {
        return delta;
    }

    // Check connectivity: the hypothetical conduit must be adjacent
    // (4-directional) to an existing conduit or nexus for this owner.
    static const int32_t dx[] = { -1, 1, 0, 0 };
    static const int32_t dy[] = { 0, 0, -1, 1 };

    bool connected = false;
    for (int i = 0; i < 4; ++i) {
        int64_t nx = static_cast<int64_t>(x) + dx[i];
        int64_t ny = static_cast<int64_t>(y) + dy[i];

        // Bounds check
        if (nx < 0 || nx >= static_cast<int64_t>(m_map_width) ||
            ny < 0 || ny >= static_cast<int64_t>(m_map_height)) {
            continue;
        }

        uint64_t neighbor_key = pack_position(static_cast<uint32_t>(nx),
                                               static_cast<uint32_t>(ny));

        // Check conduit positions
        if (m_conduit_positions[owner].find(neighbor_key) != m_conduit_positions[owner].end()) {
            connected = true;
            break;
        }

        // Check nexus positions
        if (m_nexus_positions[owner].find(neighbor_key) != m_nexus_positions[owner].end()) {
            connected = true;
            break;
        }
    }

    if (!connected) {
        return delta;
    }

    // Calculate coverage delta with default conduit coverage_radius = 3
    const uint8_t coverage_radius = 3;
    uint8_t overseer_id = owner + 1;

    // Compute clamped bounds
    int64_t min_x = static_cast<int64_t>(x) - static_cast<int64_t>(coverage_radius);
    int64_t min_y = static_cast<int64_t>(y) - static_cast<int64_t>(coverage_radius);
    int64_t max_x = static_cast<int64_t>(x) + static_cast<int64_t>(coverage_radius);
    int64_t max_y = static_cast<int64_t>(y) + static_cast<int64_t>(coverage_radius);

    if (min_x < 0) min_x = 0;
    if (min_y < 0) min_y = 0;
    if (max_x >= static_cast<int64_t>(m_map_width)) max_x = static_cast<int64_t>(m_map_width) - 1;
    if (max_y >= static_cast<int64_t>(m_map_height)) max_y = static_cast<int64_t>(m_map_height) - 1;

    // Collect tiles that would gain coverage
    for (int64_t ty = min_y; ty <= max_y; ++ty) {
        for (int64_t tx = min_x; tx <= max_x; ++tx) {
            uint32_t tile_x = static_cast<uint32_t>(tx);
            uint32_t tile_y = static_cast<uint32_t>(ty);
            if (m_coverage_grid.get_coverage_owner(tile_x, tile_y) != overseer_id) {
                delta.emplace_back(tile_x, tile_y);
            }
        }
    }

    return delta;
}

// =============================================================================
// Conduit active state (Ticket 5-030)
// =============================================================================

void EnergySystem::update_conduit_active_states(uint8_t owner) {
    if (owner >= MAX_PLAYERS || !m_registry) {
        return;
    }

    const PerPlayerEnergyPool& pool = m_pools[owner];
    bool has_generation = (pool.total_generated > 0);

    for (const auto& pair : m_conduit_positions[owner]) {
        uint32_t entity_id = pair.second;
        auto entity = static_cast<entt::entity>(entity_id);
        if (!m_registry->valid(entity)) {
            continue;
        }
        auto* conduit = m_registry->try_get<EnergyConduitComponent>(entity);
        if (!conduit) {
            continue;
        }
        conduit->is_active = (conduit->is_connected && has_generation);
    }
}

// =============================================================================
// Terrain efficiency bonus (Ticket 5-024)
// =============================================================================

float EnergySystem::get_terrain_efficiency_bonus(NexusType type, uint32_t x, uint32_t y) const {
    // If no terrain interface is available, return default (no bonus)
    if (!m_terrain) {
        return 1.0f;
    }

    // Only Wind nexuses get terrain bonuses (ridges)
    if (type != NexusType::Wind) {
        return 1.0f;
    }

    // Query terrain type at position
    terrain::TerrainType terrain_type = m_terrain->get_terrain_type(
        static_cast<int32_t>(x), static_cast<int32_t>(y));

    // Wind nexuses on Ridge terrain get +20% bonus
    if (terrain_type == terrain::TerrainType::Ridge) {
        return 1.2f;
    }

    return 1.0f;
}

// =============================================================================
// Contamination source queries (Ticket 5-025)
// =============================================================================

std::vector<ContaminationSourceData> EnergySystem::get_contamination_sources(uint8_t owner) const {
    std::vector<ContaminationSourceData> result;

    if (owner >= MAX_PLAYERS || !m_registry) {
        return result;
    }

    // Iterate all nexus positions for this owner
    for (const auto& pos_pair : m_nexus_positions[owner]) {
        uint32_t entity_id = pos_pair.second;
        uint32_t nx = unpack_x(pos_pair.first);
        uint32_t ny = unpack_y(pos_pair.first);

        auto entity = static_cast<entt::entity>(entity_id);
        if (!m_registry->valid(entity)) {
            continue;
        }

        const auto* comp = m_registry->try_get<EnergyProducerComponent>(entity);
        if (!comp) {
            continue;
        }

        // Only include nexuses that are online with actual output and contamination
        if (!comp->is_online || comp->current_output == 0 || comp->contamination_output == 0) {
            continue;
        }

        // Determine contamination radius from NexusTypeConfig coverage_radius
        NexusType ntype = static_cast<NexusType>(comp->nexus_type);
        uint8_t radius = 8; // default fallback
        if (static_cast<uint8_t>(ntype) < NEXUS_TYPE_MVP_COUNT) {
            radius = get_nexus_config(ntype).coverage_radius;
        }

        ContaminationSourceData source;
        source.entity_id = entity_id;
        source.owner_id = owner;
        source.contamination_output = comp->contamination_output;
        source.type = ContaminationType::Energy;
        source.x = nx;
        source.y = ny;
        source.radius = radius;
        result.push_back(source);
    }

    return result;
}

// =============================================================================
// Building event handler (Ticket 5-032)
// =============================================================================

// =============================================================================
// Priority-based rationing (Ticket 5-019)
// =============================================================================

void EnergySystem::apply_rationing(uint8_t owner) {
    if (owner >= MAX_PLAYERS || !m_registry) {
        return;
    }

    uint8_t overseer_id = owner + 1;

    // Struct to hold consumer info for sorting
    struct ConsumerInfo {
        uint32_t entity_id;
        uint8_t priority;
        uint32_t energy_required;
    };

    std::vector<ConsumerInfo> in_coverage;

    // Collect all consumers in coverage for this owner
    for (const auto& pair : m_consumer_positions[owner]) {
        uint32_t x = unpack_x(pair.first);
        uint32_t y = unpack_y(pair.first);
        uint32_t entity_id = pair.second;

        auto entity = static_cast<entt::entity>(entity_id);
        if (!m_registry->valid(entity)) {
            continue;
        }
        auto* ec = m_registry->try_get<EnergyComponent>(entity);
        if (!ec) {
            continue;
        }

        // Check coverage
        if (!m_coverage_grid.is_in_coverage(x, y, overseer_id)) {
            // Outside coverage: always unpowered
            ec->is_powered = false;
            ec->energy_received = 0;
            continue;
        }

        ConsumerInfo info;
        info.entity_id = entity_id;
        info.priority = ec->priority;
        info.energy_required = ec->energy_required;
        in_coverage.push_back(info);
    }

    // Sort by priority ascending (1=Critical first), then entity_id ascending for determinism
    std::sort(in_coverage.begin(), in_coverage.end(),
              [](const ConsumerInfo& a, const ConsumerInfo& b) {
                  if (a.priority != b.priority) {
                      return a.priority < b.priority;
                  }
                  return a.entity_id < b.entity_id;
              });

    // Available energy = pool.total_generated
    uint32_t available = m_pools[owner].total_generated;

    // Iterate sorted list and allocate energy
    for (const auto& info : in_coverage) {
        auto entity = static_cast<entt::entity>(info.entity_id);
        auto* ec = m_registry->try_get<EnergyComponent>(entity);
        if (!ec) {
            continue;
        }

        if (available >= info.energy_required) {
            ec->is_powered = true;
            ec->energy_received = info.energy_required;
            available -= info.energy_required;
        } else {
            ec->is_powered = false;
            ec->energy_received = 0;
        }
    }
}

// =============================================================================
// Energy state change events (Ticket 5-020)
// =============================================================================

void EnergySystem::snapshot_power_states(uint8_t owner) {
    if (owner >= MAX_PLAYERS || !m_registry) {
        return;
    }

    m_prev_powered[owner].clear();

    for (uint32_t eid : m_consumer_ids[owner]) {
        auto entity = static_cast<entt::entity>(eid);
        if (!m_registry->valid(entity)) {
            continue;
        }
        const auto* ec = m_registry->try_get<EnergyComponent>(entity);
        if (!ec) {
            continue;
        }
        m_prev_powered[owner][eid] = ec->is_powered;
    }
}

void EnergySystem::emit_state_change_events(uint8_t owner) {
    if (owner >= MAX_PLAYERS || !m_registry) {
        return;
    }

    // Note: we do NOT clear m_state_change_events here per-player.
    // The buffer is cleared once at the start of tick phase 7 loop,
    // before processing all players.

    for (uint32_t eid : m_consumer_ids[owner]) {
        auto entity = static_cast<entt::entity>(eid);
        if (!m_registry->valid(entity)) {
            continue;
        }
        const auto* ec = m_registry->try_get<EnergyComponent>(entity);
        if (!ec) {
            continue;
        }

        bool current_powered = ec->is_powered;
        bool prev_powered = false;

        auto it = m_prev_powered[owner].find(eid);
        if (it != m_prev_powered[owner].end()) {
            prev_powered = it->second;
        }

        if (current_powered != prev_powered) {
            m_state_change_events.emplace_back(eid, owner, prev_powered, current_powered);
        }
    }
}

const std::vector<EnergyStateChangedEvent>& EnergySystem::get_state_change_events() const {
    return m_state_change_events;
}

// =============================================================================
// Building event handler (Ticket 5-032)
// =============================================================================

void EnergySystem::on_building_constructed(uint32_t entity_id, uint8_t owner,
                                            int32_t grid_x, int32_t grid_y) {
    if (owner >= MAX_PLAYERS || !m_registry) {
        return;
    }

    auto entity = static_cast<entt::entity>(entity_id);
    if (!m_registry->valid(entity)) {
        return;
    }

    // Check if entity has EnergyComponent (consumer)
    if (m_registry->all_of<EnergyComponent>(entity)) {
        register_consumer(entity_id, owner);
        register_consumer_position(entity_id, owner,
                                   static_cast<uint32_t>(grid_x),
                                   static_cast<uint32_t>(grid_y));
    }

    // Check if entity has EnergyProducerComponent (nexus)
    if (m_registry->all_of<EnergyProducerComponent>(entity)) {
        register_nexus(entity_id, owner);
        register_nexus_position(entity_id, owner,
                                static_cast<uint32_t>(grid_x),
                                static_cast<uint32_t>(grid_y));
        mark_coverage_dirty(owner);
    }
}

// =============================================================================
// Building deconstruction handler (Ticket 5-033)
// =============================================================================

void EnergySystem::on_building_deconstructed(uint32_t entity_id, uint8_t owner,
                                              int32_t grid_x, int32_t grid_y) {
    if (owner >= MAX_PLAYERS) {
        return;
    }

    // Check if entity was a consumer (check m_consumer_ids[owner] contains entity_id)
    {
        auto& ids = m_consumer_ids[owner];
        auto it = std::find(ids.begin(), ids.end(), entity_id);
        if (it != ids.end()) {
            unregister_consumer(entity_id, owner);
            unregister_consumer_position(entity_id, owner,
                                         static_cast<uint32_t>(grid_x),
                                         static_cast<uint32_t>(grid_y));
        }
    }

    // Check if entity was a producer (check m_nexus_ids[owner] contains entity_id)
    {
        auto& ids = m_nexus_ids[owner];
        auto it = std::find(ids.begin(), ids.end(), entity_id);
        if (it != ids.end()) {
            unregister_nexus(entity_id, owner);
            unregister_nexus_position(entity_id, owner,
                                      static_cast<uint32_t>(grid_x),
                                      static_cast<uint32_t>(grid_y));
            mark_coverage_dirty(owner);
        }
    }
}

// =============================================================================
// Pool state transition event queries (Ticket 5-021)
// =============================================================================

const std::vector<EnergyDeficitBeganEvent>& EnergySystem::get_deficit_began_events() const {
    return m_deficit_began_events;
}

const std::vector<EnergyDeficitEndedEvent>& EnergySystem::get_deficit_ended_events() const {
    return m_deficit_ended_events;
}

const std::vector<GridCollapseBeganEvent>& EnergySystem::get_collapse_began_events() const {
    return m_collapse_began_events;
}

const std::vector<GridCollapseEndedEvent>& EnergySystem::get_collapse_ended_events() const {
    return m_collapse_ended_events;
}

void EnergySystem::clear_transition_events() {
    m_deficit_began_events.clear();
    m_deficit_ended_events.clear();
    m_collapse_began_events.clear();
    m_collapse_ended_events.clear();
}

} // namespace energy
} // namespace sims3000
