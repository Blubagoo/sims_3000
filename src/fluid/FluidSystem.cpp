/**
 * @file FluidSystem.cpp
 * @brief Implementation of FluidSystem (Ticket 6-009)
 *
 * 6-009: Skeleton with stub implementations and full tick pipeline structure.
 *
 * IFluidProvider methods (has_fluid, has_fluid_at) query FluidComponent on
 * entities through the entt::registry pointer set via set_registry(). If no
 * registry is set, safe defaults (false) are returned.
 *
 * Placement methods create entities with appropriate components, register
 * them, and emit placement events.
 *
 * The tick() method outlines all 10 pipeline phases as stubs (comments only)
 * to be filled in by later tickets.
 *
 * @see FluidSystem.h for class documentation.
 */

#include <sims3000/fluid/FluidSystem.h>
#include <sims3000/fluid/FluidComponent.h>
#include <sims3000/fluid/FluidExtractorConfig.h>
#include <sims3000/fluid/FluidReservoirConfig.h>
#include <sims3000/fluid/FluidCoverageBFS.h>
#include <sims3000/terrain/ITerrainQueryable.h>
#include <sims3000/energy/EnergyComponent.h>
#include <algorithm>

// Forward declaration of calculate_water_factor from FluidPlacementValidation.h.
// Cannot include the header directly because it forward-declares entt::registry
// as a class, which conflicts with EnTT's type alias already included via FluidSystem.h.
namespace sims3000 { namespace fluid { float calculate_water_factor(uint8_t distance); } }

namespace sims3000 {
namespace fluid {

// =============================================================================
// Construction
// =============================================================================

FluidSystem::FluidSystem(uint32_t map_width, uint32_t map_height,
                         terrain::ITerrainQueryable* terrain)
    : m_registry(nullptr)
    , m_terrain(terrain)
    , m_energy_provider(nullptr)
    , m_coverage_grid(map_width, map_height)
    , m_pools{}
    , m_coverage_dirty{}
    , m_map_width(map_width)
    , m_map_height(map_height)
{
    // Initialize per-player pools and dirty flags
    for (uint8_t i = 0; i < MAX_PLAYERS; ++i) {
        m_pools[i].clear();
        m_coverage_dirty[i] = false;
    }
}

// =============================================================================
// Registry and dependency injection
// =============================================================================

void FluidSystem::set_registry(entt::registry* registry) {
    m_registry = registry;
}

void FluidSystem::set_energy_provider(building::IEnergyProvider* provider) {
    m_energy_provider = provider;
}

// =============================================================================
// ISimulatable interface (duck-typed)
// =============================================================================

void FluidSystem::tick(float /*delta_time*/) {
    // Phase 0: Clear transition event buffers
    clear_transition_events();

    // Phase 1: (reserved for future use)

    // Phase 2: update_extractor_outputs() (Ticket 6-014)
    update_extractor_outputs();

    // Phase 3: update_reservoir_totals() (Ticket 6-015)
    update_reservoir_totals();

    // Phase 4: recalculate_coverage() if dirty (Ticket 6-011)
    // BFS flood-fill coverage from extractors/reservoirs through conduit network.
    for (uint8_t i = 0; i < MAX_PLAYERS; ++i) {
        if (m_coverage_dirty[i]) {
            BFSContext ctx{
                m_coverage_grid,
                m_extractor_positions[i],
                m_reservoir_positions[i],
                m_conduit_positions[i],
                m_registry,
                i,
                m_map_width,
                m_map_height
            };
            recalculate_coverage(ctx);
            m_coverage_dirty[i] = false;
        }
    }

    // Phase 5: aggregate_consumption() (Ticket 6-016)
    // Sum fluid_required from all consumers in coverage for each player.
    if (m_registry) {
        for (uint8_t owner = 0; owner < MAX_PLAYERS; ++owner) {
            uint32_t total_consumed = 0;
            uint32_t consumers_in_coverage = 0;

            for (uint32_t eid : m_consumer_ids[owner]) {
                auto entity = static_cast<entt::entity>(eid);
                if (!m_registry->valid(entity)) {
                    continue;
                }

                const auto* fc = m_registry->try_get<FluidComponent>(entity);
                if (!fc) {
                    continue;
                }

                // Find consumer position to check coverage
                bool in_coverage = false;
                for (const auto& pos_pair : m_consumer_positions[owner]) {
                    if (pos_pair.second == eid) {
                        uint32_t cx = unpack_x(pos_pair.first);
                        uint32_t cy = unpack_y(pos_pair.first);
                        // Coverage grid stores overseer_id (1-based)
                        uint8_t overseer_id = static_cast<uint8_t>(owner + 1);
                        if (m_coverage_grid.is_in_coverage(cx, cy, overseer_id)) {
                            in_coverage = true;
                        }
                        break;
                    }
                }

                if (in_coverage) {
                    total_consumed += fc->fluid_required;
                    ++consumers_in_coverage;
                }
            }

            m_pools[owner].total_consumed = total_consumed;
            m_pools[owner].consumer_count = consumers_in_coverage;
        }
    }

    // Phase 6: calculate_pool() + apply_reservoir_buffering() + recalculate state
    //          + detect_pool_state_transitions() (Tickets 6-017, 6-018)
    for (uint8_t i = 0; i < MAX_PLAYERS; ++i) {
        calculate_pool(i);
        apply_reservoir_buffering(i);
        // Recalculate state after reservoir buffering (drain may change Collapse to Deficit)
        m_pools[i].state = calculate_pool_state(m_pools[i]);
        detect_pool_state_transitions(i);
    }

    // Phase 7a: snapshot_fluid_states() before distribution (Ticket 6-022)
    for (uint8_t i = 0; i < MAX_PLAYERS; ++i) {
        snapshot_fluid_states(i);
    }

    // Phase 7b: distribute_fluid() (Ticket 6-019)
    // All-or-nothing distribution per CCR-002.
    for (uint8_t i = 0; i < MAX_PLAYERS; ++i) {
        distribute_fluid(i);
    }

    // Phase 8: update_conduit_active_states() (Ticket 6-032)
    for (uint8_t i = 0; i < MAX_PLAYERS; ++i) {
        update_conduit_active_states(i);
    }

    // Phase 9: emit_state_change_events() (Ticket 6-022)
    // Compare previous has_fluid state with current for all consumers
    // and emit FluidStateChangedEvent for changes.
    for (uint8_t i = 0; i < MAX_PLAYERS; ++i) {
        emit_state_change_events(i);
    }
}

int FluidSystem::get_priority() const {
    return 20;
}

// =============================================================================
// IFluidProvider interface implementation
//
// No grace period for fluid (CCR-006) - reservoir buffer serves this purpose.
// Unlike energy's 100-tick grace period, fluid cuts off immediately when
// pool surplus goes negative. Reservoirs provide the buffering mechanism
// instead of a built-in delay.
// =============================================================================

bool FluidSystem::has_fluid(uint32_t entity_id) const {
    if (!m_registry) {
        return false;
    }
    auto entity = static_cast<entt::entity>(entity_id);
    if (!m_registry->valid(entity)) {
        return false;
    }
    const auto* fc = m_registry->try_get<FluidComponent>(entity);
    if (!fc) {
        return false;
    }
    return fc->has_fluid;
}

bool FluidSystem::has_fluid_at(uint32_t x, uint32_t y, uint32_t player_id) const {
    if (player_id >= MAX_PLAYERS) {
        return false;
    }
    // Coverage grid stores overseer_id (1-based): overseer_id = player_id + 1
    auto overseer_id = static_cast<uint8_t>(player_id + 1);
    if (!m_coverage_grid.is_in_coverage(x, y, overseer_id)) {
        return false;
    }
    // Pool is indexed by player_id (0-based)
    return m_pools[player_id].surplus >= 0;
}

// =============================================================================
// Registration methods
// =============================================================================

void FluidSystem::register_extractor(uint32_t entity_id, uint8_t owner) {
    if (owner >= MAX_PLAYERS) {
        return;
    }
    m_extractor_ids[owner].push_back(entity_id);
    m_coverage_dirty[owner] = true;
}

void FluidSystem::unregister_extractor(uint32_t entity_id, uint8_t owner) {
    if (owner >= MAX_PLAYERS) {
        return;
    }
    auto& ids = m_extractor_ids[owner];
    auto it = std::find(ids.begin(), ids.end(), entity_id);
    if (it != ids.end()) {
        ids.erase(it);
        m_coverage_dirty[owner] = true;
    }
}

void FluidSystem::register_reservoir(uint32_t entity_id, uint8_t owner) {
    if (owner >= MAX_PLAYERS) {
        return;
    }
    m_reservoir_ids[owner].push_back(entity_id);
    m_coverage_dirty[owner] = true;
}

void FluidSystem::unregister_reservoir(uint32_t entity_id, uint8_t owner) {
    if (owner >= MAX_PLAYERS) {
        return;
    }
    auto& ids = m_reservoir_ids[owner];
    auto it = std::find(ids.begin(), ids.end(), entity_id);
    if (it != ids.end()) {
        ids.erase(it);
        m_coverage_dirty[owner] = true;
    }
}

void FluidSystem::register_consumer(uint32_t entity_id, uint8_t owner) {
    if (owner >= MAX_PLAYERS) {
        return;
    }
    m_consumer_ids[owner].push_back(entity_id);
}

void FluidSystem::unregister_consumer(uint32_t entity_id, uint8_t owner) {
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
// Position registration methods
// =============================================================================

void FluidSystem::register_extractor_position(uint32_t entity_id, uint8_t owner,
                                              uint32_t x, uint32_t y) {
    if (owner >= MAX_PLAYERS) {
        return;
    }
    uint64_t key = pack_position(x, y);
    m_extractor_positions[owner][key] = entity_id;
    m_coverage_dirty[owner] = true;
}

void FluidSystem::register_reservoir_position(uint32_t entity_id, uint8_t owner,
                                              uint32_t x, uint32_t y) {
    if (owner >= MAX_PLAYERS) {
        return;
    }
    uint64_t key = pack_position(x, y);
    m_reservoir_positions[owner][key] = entity_id;
    m_coverage_dirty[owner] = true;
}

void FluidSystem::register_consumer_position(uint32_t entity_id, uint8_t owner,
                                             uint32_t x, uint32_t y) {
    if (owner >= MAX_PLAYERS) {
        return;
    }
    uint64_t key = pack_position(x, y);
    m_consumer_positions[owner][key] = entity_id;
}

// =============================================================================
// Placement methods
// =============================================================================

uint32_t FluidSystem::place_extractor(uint32_t x, uint32_t y, uint8_t owner) {
    // Require registry
    if (!m_registry) {
        return INVALID_ENTITY_ID;
    }

    // Bounds check
    if (x >= m_map_width || y >= m_map_height) {
        return INVALID_ENTITY_ID;
    }

    // Owner validation
    if (owner >= MAX_PLAYERS) {
        return INVALID_ENTITY_ID;
    }

    // Create entity with FluidProducerComponent
    auto entity = m_registry->create();
    uint32_t entity_id = static_cast<uint32_t>(entity);

    // Initialize producer component from default extractor config
    FluidExtractorConfig config = get_default_extractor_config();
    FluidProducerComponent producer{};
    producer.base_output = config.base_output;
    producer.current_output = 0;
    producer.max_water_distance = config.max_operational_distance;
    producer.current_water_distance = 0;
    producer.is_operational = false;
    producer.producer_type = static_cast<uint8_t>(FluidProducerType::Extractor);
    m_registry->emplace<FluidProducerComponent>(entity, producer);

    // Attach EnergyComponent for power dependency (Ticket 6-026)
    // Extractors require energy to operate: energy_required = 20, priority = 2 (IMPORTANT)
    energy::EnergyComponent energy_comp{};
    energy_comp.energy_required = config.energy_required;
    energy_comp.energy_received = 0;
    energy_comp.is_powered = false;
    energy_comp.priority = config.energy_priority;
    m_registry->emplace<energy::EnergyComponent>(entity, energy_comp);

    // Register extractor and position
    register_extractor(entity_id, owner);
    register_extractor_position(entity_id, owner, x, y);

    // Emit ExtractorPlacedEvent
    m_extractor_placed_events.emplace_back(entity_id, owner, x, y,
                                           static_cast<uint8_t>(0));

    return entity_id;
}

uint32_t FluidSystem::place_reservoir(uint32_t x, uint32_t y, uint8_t owner) {
    // Require registry
    if (!m_registry) {
        return INVALID_ENTITY_ID;
    }

    // Bounds check
    if (x >= m_map_width || y >= m_map_height) {
        return INVALID_ENTITY_ID;
    }

    // Owner validation
    if (owner >= MAX_PLAYERS) {
        return INVALID_ENTITY_ID;
    }

    // Create entity with FluidReservoirComponent + FluidProducerComponent
    auto entity = m_registry->create();
    uint32_t entity_id = static_cast<uint32_t>(entity);

    // Initialize reservoir component from default config
    FluidReservoirConfig res_config = get_default_reservoir_config();
    FluidReservoirComponent reservoir{};
    reservoir.capacity = res_config.capacity;
    reservoir.current_level = 0;
    reservoir.fill_rate = res_config.fill_rate;
    reservoir.drain_rate = res_config.drain_rate;
    reservoir.is_active = false;
    reservoir.reservoir_type = 0;
    m_registry->emplace<FluidReservoirComponent>(entity, reservoir);

    // Also add a FluidProducerComponent for reservoir type
    FluidProducerComponent producer{};
    producer.base_output = 0; // Reservoirs don't produce, they store
    producer.current_output = 0;
    producer.max_water_distance = 255; // Reservoirs don't need water proximity
    producer.current_water_distance = 0;
    producer.is_operational = false;
    producer.producer_type = static_cast<uint8_t>(FluidProducerType::Reservoir);
    m_registry->emplace<FluidProducerComponent>(entity, producer);

    // Register reservoir and position
    register_reservoir(entity_id, owner);
    register_reservoir_position(entity_id, owner, x, y);

    // Emit ReservoirPlacedEvent
    m_reservoir_placed_events.emplace_back(entity_id, owner, x, y);

    return entity_id;
}

// =============================================================================
// Conduit placement validation (Ticket 6-029)
// =============================================================================

bool FluidSystem::validate_conduit_placement(uint32_t x, uint32_t y, uint8_t owner) const {
    // 1. Bounds check
    if (x >= m_map_width || y >= m_map_height) {
        return false;
    }

    // 2. Owner check
    if (owner >= MAX_PLAYERS) {
        return false;
    }

    // 3. Terrain buildable check (if terrain available)
    if (m_terrain) {
        if (!m_terrain->is_buildable(static_cast<int32_t>(x), static_cast<int32_t>(y))) {
            return false;
        }
    }
    // If terrain is nullptr, default to buildable (stub)

    return true;
}

uint32_t FluidSystem::place_conduit(uint32_t x, uint32_t y, uint8_t owner) {
    // Require registry
    if (!m_registry) {
        return INVALID_ENTITY_ID;
    }

    // Validate placement (bounds, owner, terrain)
    if (!validate_conduit_placement(x, y, owner)) {
        return INVALID_ENTITY_ID;
    }

    // Create entity with FluidConduitComponent
    auto entity = m_registry->create();
    uint32_t entity_id = static_cast<uint32_t>(entity);

    FluidConduitComponent conduit{};
    conduit.coverage_radius = 3;
    conduit.is_connected = false;
    conduit.is_active = false;
    conduit.conduit_level = 1;
    m_registry->emplace<FluidConduitComponent>(entity, conduit);

    // Register conduit position
    uint64_t key = pack_position(x, y);
    m_conduit_positions[owner][key] = entity_id;
    m_coverage_dirty[owner] = true;

    // Cost deduction stub: not yet deducted, needs ICreditProvider

    // Emit FluidConduitPlacedEvent
    m_conduit_placed_events.emplace_back(entity_id, owner, x, y);

    return entity_id;
}

bool FluidSystem::remove_conduit(uint32_t entity_id, uint8_t owner,
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

    // Validate entity has FluidConduitComponent
    if (!m_registry->all_of<FluidConduitComponent>(entity)) {
        return false;
    }

    // Unregister conduit position
    uint64_t key = pack_position(x, y);
    m_conduit_positions[owner].erase(key);
    m_coverage_dirty[owner] = true;

    // Emit FluidConduitRemovedEvent
    m_conduit_removed_events.emplace_back(entity_id, owner, x, y);

    // Destroy entity from registry
    m_registry->destroy(entity);

    return true;
}

// =============================================================================
// Conduit placement preview (Ticket 6-033)
// =============================================================================

std::vector<std::pair<uint32_t, uint32_t>> FluidSystem::preview_conduit_coverage(
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
    // (4-directional) to an existing conduit, extractor, or reservoir
    // for this owner.
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

        // Check extractor positions
        if (m_extractor_positions[owner].find(neighbor_key) != m_extractor_positions[owner].end()) {
            connected = true;
            break;
        }

        // Check reservoir positions
        if (m_reservoir_positions[owner].find(neighbor_key) != m_reservoir_positions[owner].end()) {
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
// Coverage queries
// =============================================================================

bool FluidSystem::is_in_coverage(uint32_t x, uint32_t y, uint8_t owner) const {
    return m_coverage_grid.is_in_coverage(x, y, owner);
}

uint8_t FluidSystem::get_coverage_at(uint32_t x, uint32_t y) const {
    return m_coverage_grid.get_coverage_owner(x, y);
}

uint32_t FluidSystem::get_coverage_count(uint8_t owner) const {
    return m_coverage_grid.get_coverage_count(owner);
}

bool FluidSystem::is_coverage_dirty(uint8_t owner) const {
    if (owner >= MAX_PLAYERS) {
        return false;
    }
    return m_coverage_dirty[owner];
}

// =============================================================================
// Pool queries
// =============================================================================

const PerPlayerFluidPool& FluidSystem::get_pool(uint8_t owner) const {
    if (owner >= MAX_PLAYERS) {
        // Return player 0 pool as safe fallback
        return m_pools[0];
    }
    return m_pools[owner];
}

FluidPoolState FluidSystem::get_pool_state(uint8_t owner) const {
    if (owner >= MAX_PLAYERS) {
        return FluidPoolState::Healthy;
    }
    return m_pools[owner].state;
}

// =============================================================================
// Entity count accessors (for testing)
// =============================================================================

uint32_t FluidSystem::get_extractor_count(uint8_t owner) const {
    if (owner >= MAX_PLAYERS) {
        return 0;
    }
    return static_cast<uint32_t>(m_extractor_ids[owner].size());
}

uint32_t FluidSystem::get_reservoir_count(uint8_t owner) const {
    if (owner >= MAX_PLAYERS) {
        return 0;
    }
    return static_cast<uint32_t>(m_reservoir_ids[owner].size());
}

uint32_t FluidSystem::get_consumer_count(uint8_t owner) const {
    if (owner >= MAX_PLAYERS) {
        return 0;
    }
    return static_cast<uint32_t>(m_consumer_ids[owner].size());
}

uint32_t FluidSystem::get_conduit_position_count(uint8_t owner) const {
    if (owner >= MAX_PLAYERS) {
        return 0;
    }
    return static_cast<uint32_t>(m_conduit_positions[owner].size());
}

// =============================================================================
// Event accessors
// =============================================================================

const std::vector<FluidStateChangedEvent>& FluidSystem::get_state_changed_events() const {
    return m_state_changed_events;
}

const std::vector<FluidDeficitBeganEvent>& FluidSystem::get_deficit_began_events() const {
    return m_deficit_began_events;
}

const std::vector<FluidDeficitEndedEvent>& FluidSystem::get_deficit_ended_events() const {
    return m_deficit_ended_events;
}

const std::vector<FluidCollapseBeganEvent>& FluidSystem::get_collapse_began_events() const {
    return m_collapse_began_events;
}

const std::vector<FluidCollapseEndedEvent>& FluidSystem::get_collapse_ended_events() const {
    return m_collapse_ended_events;
}

const std::vector<FluidConduitPlacedEvent>& FluidSystem::get_conduit_placed_events() const {
    return m_conduit_placed_events;
}

const std::vector<FluidConduitRemovedEvent>& FluidSystem::get_conduit_removed_events() const {
    return m_conduit_removed_events;
}

const std::vector<ExtractorPlacedEvent>& FluidSystem::get_extractor_placed_events() const {
    return m_extractor_placed_events;
}

const std::vector<ExtractorRemovedEvent>& FluidSystem::get_extractor_removed_events() const {
    return m_extractor_removed_events;
}

const std::vector<ReservoirPlacedEvent>& FluidSystem::get_reservoir_placed_events() const {
    return m_reservoir_placed_events;
}

const std::vector<ReservoirRemovedEvent>& FluidSystem::get_reservoir_removed_events() const {
    return m_reservoir_removed_events;
}

const std::vector<ReservoirLevelChangedEvent>& FluidSystem::get_reservoir_level_changed_events() const {
    return m_reservoir_level_changed_events;
}

void FluidSystem::clear_transition_events() {
    m_state_changed_events.clear();
    m_deficit_began_events.clear();
    m_deficit_ended_events.clear();
    m_collapse_began_events.clear();
    m_collapse_ended_events.clear();
    m_conduit_placed_events.clear();
    m_conduit_removed_events.clear();
    m_extractor_placed_events.clear();
    m_extractor_removed_events.clear();
    m_reservoir_placed_events.clear();
    m_reservoir_removed_events.clear();
    m_reservoir_level_changed_events.clear();
}

// =============================================================================
// Building event handlers
// =============================================================================

void FluidSystem::on_building_constructed(uint32_t entity_id, uint8_t owner,
                                          int32_t grid_x, int32_t grid_y) {
    if (owner >= MAX_PLAYERS || !m_registry) {
        return;
    }

    // Bounds validation: negative coordinates are invalid
    if (grid_x < 0 || grid_y < 0) {
        return;
    }
    uint32_t x = static_cast<uint32_t>(grid_x);
    uint32_t y = static_cast<uint32_t>(grid_y);
    if (x >= m_map_width || y >= m_map_height) {
        return;
    }

    auto entity = static_cast<entt::entity>(entity_id);
    if (!m_registry->valid(entity)) {
        return;
    }

    // Check if entity has FluidComponent (consumer)
    if (m_registry->all_of<FluidComponent>(entity)) {
        register_consumer(entity_id, owner);
        register_consumer_position(entity_id, owner, x, y);
    }

    // Check if entity has FluidProducerComponent (extractor)
    if (m_registry->all_of<FluidProducerComponent>(entity)) {
        register_extractor(entity_id, owner);
        register_extractor_position(entity_id, owner, x, y);
        m_coverage_dirty[owner] = true;
    }

    // Check if entity has FluidReservoirComponent (reservoir)
    if (m_registry->all_of<FluidReservoirComponent>(entity)) {
        register_reservoir(entity_id, owner);
        register_reservoir_position(entity_id, owner, x, y);
        m_coverage_dirty[owner] = true;
    }
}

void FluidSystem::on_building_deconstructed(uint32_t entity_id, uint8_t owner,
                                            int32_t grid_x, int32_t grid_y) {
    if (owner >= MAX_PLAYERS) {
        return;
    }

    // Bounds validation: negative coordinates are invalid
    if (grid_x < 0 || grid_y < 0) {
        return;
    }
    uint32_t x = static_cast<uint32_t>(grid_x);
    uint32_t y = static_cast<uint32_t>(grid_y);
    if (x >= m_map_width || y >= m_map_height) {
        return;
    }

    // Check if entity was a consumer
    {
        auto& ids = m_consumer_ids[owner];
        auto it = std::find(ids.begin(), ids.end(), entity_id);
        if (it != ids.end()) {
            unregister_consumer(entity_id, owner);
            uint64_t key = pack_position(x, y);
            m_consumer_positions[owner].erase(key);
        }
    }

    // Check if entity was an extractor
    {
        auto& ids = m_extractor_ids[owner];
        auto it = std::find(ids.begin(), ids.end(), entity_id);
        if (it != ids.end()) {
            unregister_extractor(entity_id, owner);
            uint64_t key = pack_position(x, y);
            m_extractor_positions[owner].erase(key);
            m_coverage_dirty[owner] = true;
        }
    }

    // Check if entity was a reservoir
    {
        auto& ids = m_reservoir_ids[owner];
        auto it = std::find(ids.begin(), ids.end(), entity_id);
        if (it != ids.end()) {
            unregister_reservoir(entity_id, owner);
            uint64_t key = pack_position(x, y);
            m_reservoir_positions[owner].erase(key);
            m_coverage_dirty[owner] = true;
        }
    }
}

// =============================================================================
// Map dimension accessors
// =============================================================================

uint32_t FluidSystem::get_map_width() const {
    return m_map_width;
}

uint32_t FluidSystem::get_map_height() const {
    return m_map_height;
}

// =============================================================================
// Spatial lookup helpers
// =============================================================================

uint64_t FluidSystem::pack_position(uint32_t x, uint32_t y) {
    return (static_cast<uint64_t>(x) << 32) | static_cast<uint64_t>(y);
}

uint32_t FluidSystem::unpack_x(uint64_t packed) {
    return static_cast<uint32_t>(packed >> 32);
}

uint32_t FluidSystem::unpack_y(uint64_t packed) {
    return static_cast<uint32_t>(packed & 0xFFFFFFFF);
}

// =============================================================================
// Extractor output calculation (Ticket 6-014)
// =============================================================================

void FluidSystem::update_extractor_outputs() {
    if (!m_registry) {
        return;
    }

    for (uint8_t owner = 0; owner < MAX_PLAYERS; ++owner) {
        uint32_t total_generated = 0;
        uint32_t operational_count = 0;

        for (uint32_t eid : m_extractor_ids[owner]) {
            auto entity = static_cast<entt::entity>(eid);
            if (!m_registry->valid(entity)) {
                continue;
            }

            auto* prod = m_registry->try_get<FluidProducerComponent>(entity);
            if (!prod) {
                continue;
            }

            // Check power state via energy provider
            bool powered = true;
            if (m_energy_provider) {
                powered = m_energy_provider->is_powered(eid);
            }

            // Look up water distance from extractor position
            uint8_t water_distance = 0;
            for (const auto& pos_pair : m_extractor_positions[owner]) {
                if (pos_pair.second == eid) {
                    uint32_t ex = unpack_x(pos_pair.first);
                    uint32_t ey = unpack_y(pos_pair.first);

                    if (m_terrain) {
                        uint32_t raw_dist = m_terrain->get_water_distance(
                            static_cast<int32_t>(ex), static_cast<int32_t>(ey));
                        water_distance = static_cast<uint8_t>(
                            raw_dist > 255 ? 255 : raw_dist);
                    }
                    break;
                }
            }

            prod->current_water_distance = water_distance;

            // Calculate water factor using distance-to-efficiency curve
            float water_factor = calculate_water_factor(water_distance);

            // current_output = base_output * water_factor * (powered ? 1.0 : 0.0)
            float output = static_cast<float>(prod->base_output) * water_factor
                         * (powered ? 1.0f : 0.0f);
            prod->current_output = static_cast<uint32_t>(output);

            // is_operational = powered AND distance <= max_operational_distance
            prod->is_operational = powered
                                 && (water_distance <= prod->max_water_distance);

            if (prod->is_operational) {
                total_generated += prod->current_output;
                ++operational_count;
            }
        }

        m_pools[owner].total_generated = total_generated;
        m_pools[owner].extractor_count = operational_count;
    }
}

// =============================================================================
// Reservoir totals calculation (Ticket 6-015)
// =============================================================================

void FluidSystem::update_reservoir_totals() {
    if (!m_registry) {
        return;
    }

    for (uint8_t owner = 0; owner < MAX_PLAYERS; ++owner) {
        uint32_t total_stored = 0;
        uint32_t total_capacity = 0;
        uint32_t reservoir_count = 0;

        for (uint32_t eid : m_reservoir_ids[owner]) {
            auto entity = static_cast<entt::entity>(eid);
            if (!m_registry->valid(entity)) {
                continue;
            }

            const auto* res = m_registry->try_get<FluidReservoirComponent>(entity);
            if (!res) {
                continue;
            }

            total_stored += res->current_level;
            total_capacity += res->capacity;
            ++reservoir_count;
        }

        m_pools[owner].total_reservoir_stored = total_stored;
        m_pools[owner].total_reservoir_capacity = total_capacity;
        m_pools[owner].reservoir_count = reservoir_count;
    }
}

// =============================================================================
// Pool calculation (Ticket 6-017)
// =============================================================================

void FluidSystem::calculate_pool(uint8_t owner) {
    if (owner >= MAX_PLAYERS) {
        return;
    }

    PerPlayerFluidPool& pool = m_pools[owner];

    // Store previous state before calculating new state
    pool.previous_state = pool.state;

    // total_generated and extractor_count are already set by update_extractor_outputs()
    // total_reservoir_stored, total_reservoir_capacity, and reservoir_count are already set by update_reservoir_totals()
    // total_consumed and consumer_count are already set by Phase 5 aggregate_consumption

    // Calculate available supply
    pool.available = pool.total_generated + pool.total_reservoir_stored;

    // Calculate surplus (can be negative)
    pool.surplus = static_cast<int32_t>(pool.available)
                 - static_cast<int32_t>(pool.total_consumed);

    // Determine pool state
    pool.state = calculate_pool_state(pool);
}

FluidPoolState FluidSystem::calculate_pool_state(const PerPlayerFluidPool& pool) {
    // If no generation and no consumption: Healthy (empty grid)
    if (pool.total_generated == 0 && pool.total_consumed == 0) {
        return FluidPoolState::Healthy;
    }

    // buffer_threshold = 10% of available
    uint32_t buffer_threshold = static_cast<uint32_t>(
        static_cast<float>(pool.available) * 0.10f);

    // If surplus >= buffer_threshold: Healthy
    if (pool.surplus >= static_cast<int32_t>(buffer_threshold)) {
        return FluidPoolState::Healthy;
    }

    // If surplus >= 0: Marginal
    if (pool.surplus >= 0) {
        return FluidPoolState::Marginal;
    }

    // surplus < 0: check reservoir buffer
    // If total_reservoir_stored > 0: Deficit (reservoirs can buffer)
    if (pool.total_reservoir_stored > 0) {
        return FluidPoolState::Deficit;
    }

    // If total_reservoir_stored == 0: Collapse
    return FluidPoolState::Collapse;
}

// =============================================================================
// Reservoir buffering (Ticket 6-018)
// =============================================================================

void FluidSystem::apply_reservoir_buffering(uint8_t owner) {
    if (owner >= MAX_PLAYERS || !m_registry) {
        return;
    }

    PerPlayerFluidPool& pool = m_pools[owner];

    if (pool.surplus > 0) {
        // FILL reservoirs: distribute surplus proportionally by remaining capacity
        int32_t surplus_remaining = pool.surplus;

        // First pass: compute total remaining capacity across all reservoirs
        uint32_t total_remaining_capacity = 0;
        for (uint32_t eid : m_reservoir_ids[owner]) {
            auto entity = static_cast<entt::entity>(eid);
            if (!m_registry->valid(entity)) {
                continue;
            }
            const auto* res = m_registry->try_get<FluidReservoirComponent>(entity);
            if (!res) {
                continue;
            }
            if (res->current_level < res->capacity) {
                total_remaining_capacity += (res->capacity - res->current_level);
            }
        }

        if (total_remaining_capacity == 0) {
            return; // All reservoirs full, nothing to fill
        }

        // Second pass: fill proportionally by remaining capacity
        for (uint32_t eid : m_reservoir_ids[owner]) {
            if (surplus_remaining <= 0) {
                break;
            }

            auto entity = static_cast<entt::entity>(eid);
            if (!m_registry->valid(entity)) {
                continue;
            }
            auto* res = m_registry->try_get<FluidReservoirComponent>(entity);
            if (!res) {
                continue;
            }

            uint32_t remaining_cap = res->capacity - res->current_level;
            if (remaining_cap == 0) {
                continue;
            }

            // Proportional share of surplus based on remaining capacity
            uint32_t proportional_share = static_cast<uint32_t>(
                static_cast<uint64_t>(surplus_remaining) * remaining_cap / total_remaining_capacity);

            // Limit by fill_rate and remaining capacity
            uint32_t fill_amount = proportional_share;
            if (fill_amount > static_cast<uint32_t>(res->fill_rate)) {
                fill_amount = static_cast<uint32_t>(res->fill_rate);
            }
            if (fill_amount > remaining_cap) {
                fill_amount = remaining_cap;
            }
            if (fill_amount > static_cast<uint32_t>(surplus_remaining)) {
                fill_amount = static_cast<uint32_t>(surplus_remaining);
            }

            if (fill_amount > 0) {
                uint32_t old_level = res->current_level;
                res->current_level += fill_amount;
                surplus_remaining -= static_cast<int32_t>(fill_amount);

                // Update pool reservoir stored total
                pool.total_reservoir_stored += fill_amount;
                pool.available += fill_amount;
                pool.surplus = static_cast<int32_t>(pool.available)
                             - static_cast<int32_t>(pool.total_consumed);

                // Emit ReservoirLevelChangedEvent
                m_reservoir_level_changed_events.emplace_back(
                    eid, owner, old_level, res->current_level);
            }
        }
    } else if (pool.surplus < 0) {
        // DRAIN reservoirs: distribute drain proportionally by current_level
        int32_t deficit = -pool.surplus;
        int32_t deficit_remaining = deficit;

        // First pass: compute total stored across all reservoirs
        uint32_t total_stored = 0;
        for (uint32_t eid : m_reservoir_ids[owner]) {
            auto entity = static_cast<entt::entity>(eid);
            if (!m_registry->valid(entity)) {
                continue;
            }
            const auto* res = m_registry->try_get<FluidReservoirComponent>(entity);
            if (!res) {
                continue;
            }
            total_stored += res->current_level;
        }

        if (total_stored == 0) {
            return; // No fluid to drain
        }

        // Second pass: drain proportionally by current_level
        for (uint32_t eid : m_reservoir_ids[owner]) {
            if (deficit_remaining <= 0) {
                break;
            }

            auto entity = static_cast<entt::entity>(eid);
            if (!m_registry->valid(entity)) {
                continue;
            }
            auto* res = m_registry->try_get<FluidReservoirComponent>(entity);
            if (!res) {
                continue;
            }

            if (res->current_level == 0) {
                continue;
            }

            // Proportional share of deficit based on current_level
            uint32_t proportional_share = static_cast<uint32_t>(
                static_cast<uint64_t>(deficit_remaining) * res->current_level / total_stored);
            // Ensure at least 1 unit drained if reservoir has fluid and there's deficit
            if (proportional_share == 0 && res->current_level > 0 && deficit_remaining > 0) {
                proportional_share = 1;
            }

            // Limit by drain_rate and current_level
            uint32_t drain_amount = proportional_share;
            if (drain_amount > static_cast<uint32_t>(res->drain_rate)) {
                drain_amount = static_cast<uint32_t>(res->drain_rate);
            }
            if (drain_amount > res->current_level) {
                drain_amount = res->current_level;
            }
            if (drain_amount > static_cast<uint32_t>(deficit_remaining)) {
                drain_amount = static_cast<uint32_t>(deficit_remaining);
            }

            if (drain_amount > 0) {
                uint32_t old_level = res->current_level;
                res->current_level -= drain_amount;
                deficit_remaining -= static_cast<int32_t>(drain_amount);

                // Update pool reservoir stored total
                pool.total_reservoir_stored -= drain_amount;

                // Emit ReservoirLevelChangedEvent
                m_reservoir_level_changed_events.emplace_back(
                    eid, owner, old_level, res->current_level);
            }
        }

        // Recalculate pool aggregates after drain
        pool.available = pool.total_generated + pool.total_reservoir_stored;
        pool.surplus = static_cast<int32_t>(pool.available)
                     - static_cast<int32_t>(pool.total_consumed);
    }
}

// =============================================================================
// Pool state transition detection (Ticket 6-018)
// =============================================================================

void FluidSystem::detect_pool_state_transitions(uint8_t owner) {
    if (owner >= MAX_PLAYERS) {
        return;
    }

    PerPlayerFluidPool& pool = m_pools[owner];
    FluidPoolState prev = pool.previous_state;
    FluidPoolState curr = pool.state;

    // Transition INTO Deficit (from Healthy or Marginal)
    if ((curr == FluidPoolState::Deficit || curr == FluidPoolState::Collapse) &&
        (prev == FluidPoolState::Healthy || prev == FluidPoolState::Marginal)) {
        m_deficit_began_events.emplace_back(owner, pool.surplus, pool.consumer_count);
    }

    // Transition OUT OF Deficit or Collapse (to Healthy or Marginal)
    if ((curr == FluidPoolState::Healthy || curr == FluidPoolState::Marginal) &&
        (prev == FluidPoolState::Deficit || prev == FluidPoolState::Collapse)) {
        m_deficit_ended_events.emplace_back(owner, pool.surplus);
    }

    // Transition INTO Collapse
    if (curr == FluidPoolState::Collapse && prev != FluidPoolState::Collapse) {
        m_collapse_began_events.emplace_back(owner, pool.surplus);
    }

    // Transition OUT OF Collapse
    if (curr != FluidPoolState::Collapse && prev == FluidPoolState::Collapse) {
        m_collapse_ended_events.emplace_back(owner);
    }

    pool.previous_state = curr;
}

// =============================================================================
// Conduit active state (Ticket 6-032)
// =============================================================================

void FluidSystem::update_conduit_active_states(uint8_t owner) {
    if (owner >= MAX_PLAYERS || !m_registry) {
        return;
    }

    const PerPlayerFluidPool& pool = m_pools[owner];
    bool has_generation = (pool.total_generated > 0);

    for (const auto& pair : m_conduit_positions[owner]) {
        uint32_t entity_id = pair.second;
        auto entity = static_cast<entt::entity>(entity_id);
        if (!m_registry->valid(entity)) {
            continue;
        }
        auto* conduit = m_registry->try_get<FluidConduitComponent>(entity);
        if (!conduit) {
            continue;
        }
        conduit->is_active = (conduit->is_connected && has_generation);
    }
}

// =============================================================================
// Fluid distribution (Ticket 6-019)
// =============================================================================

void FluidSystem::distribute_fluid(uint8_t owner) {
    // DESIGN NOTE (CCR-002): Fluid uses all-or-nothing distribution.
    // Unlike EnergySystem which has 4-tier priority-based rationing,
    // FluidSystem distributes equally to all consumers.
    // During deficit, ALL consumers lose fluid simultaneously.
    // Rationale: "Fluid affects habitability uniformly. You can't say
    // these beings get water and these don't." - Game Designer
    // If future requirements change, ticket 6-020 documents this decision.

    if (owner >= MAX_PLAYERS || !m_registry) {
        return;
    }

    const PerPlayerFluidPool& pool = m_pools[owner];
    uint8_t overseer_id = static_cast<uint8_t>(owner + 1);
    bool has_surplus = (pool.surplus >= 0);

    for (uint32_t eid : m_consumer_ids[owner]) {
        auto entity = static_cast<entt::entity>(eid);
        if (!m_registry->valid(entity)) {
            continue;
        }

        auto* fc = m_registry->try_get<FluidComponent>(entity);
        if (!fc) {
            continue;
        }

        // Check if consumer is in coverage
        bool in_coverage = false;
        for (const auto& pos_pair : m_consumer_positions[owner]) {
            if (pos_pair.second == eid) {
                uint32_t cx = unpack_x(pos_pair.first);
                uint32_t cy = unpack_y(pos_pair.first);
                if (m_coverage_grid.is_in_coverage(cx, cy, overseer_id)) {
                    in_coverage = true;
                }
                break;
            }
        }

        if (!in_coverage) {
            // Outside coverage: always no fluid
            fc->fluid_received = 0;
            fc->has_fluid = false;
            continue;
        }

        // All-or-nothing per CCR-002
        if (has_surplus) {
            // Pool surplus >= 0: ALL consumers in coverage get fluid
            fc->fluid_received = fc->fluid_required;
            fc->has_fluid = true;
        } else {
            // Pool surplus < 0 (after reservoir drain exhausted): ALL lose fluid
            fc->fluid_received = 0;
            fc->has_fluid = false;
        }
    }
}

// =============================================================================
// Fluid state change events (Ticket 6-022)
// =============================================================================

void FluidSystem::snapshot_fluid_states(uint8_t owner) {
    if (owner >= MAX_PLAYERS || !m_registry) {
        return;
    }

    m_prev_has_fluid[owner].clear();

    for (uint32_t eid : m_consumer_ids[owner]) {
        auto entity = static_cast<entt::entity>(eid);
        if (!m_registry->valid(entity)) {
            continue;
        }
        const auto* fc = m_registry->try_get<FluidComponent>(entity);
        if (!fc) {
            continue;
        }
        m_prev_has_fluid[owner][eid] = fc->has_fluid;
    }
}

void FluidSystem::emit_state_change_events(uint8_t owner) {
    if (owner >= MAX_PLAYERS || !m_registry) {
        return;
    }

    for (uint32_t eid : m_consumer_ids[owner]) {
        auto entity = static_cast<entt::entity>(eid);
        if (!m_registry->valid(entity)) {
            continue;
        }
        const auto* fc = m_registry->try_get<FluidComponent>(entity);
        if (!fc) {
            continue;
        }

        bool current_has_fluid = fc->has_fluid;
        bool prev_has_fluid = false;

        auto it = m_prev_has_fluid[owner].find(eid);
        if (it != m_prev_has_fluid[owner].end()) {
            prev_has_fluid = it->second;
        }

        if (current_has_fluid != prev_has_fluid) {
            m_state_changed_events.emplace_back(eid, owner, prev_has_fluid, current_has_fluid);
        }
    }
}

} // namespace fluid
} // namespace sims3000
