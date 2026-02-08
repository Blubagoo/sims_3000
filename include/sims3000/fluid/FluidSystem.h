/**
 * @file FluidSystem.h
 * @brief Top-level FluidSystem class skeleton for Epic 6 (Ticket 6-009)
 *
 * FluidSystem orchestrates all fluid subsystems:
 * - FluidCoverageGrid: spatial coverage tracking
 * - PerPlayerFluidPool: per-player supply/demand aggregation
 * - Extractor management: registration of fluid extractors
 * - Reservoir management: registration of fluid reservoirs
 * - Consumer management: registration of fluid consumers
 * - Conduit management: fluid distribution network
 *
 * Implements ISimulatable interface (duck-typed, matching EnergySystem pattern)
 * at priority 20 per canonical interface spec (after energy at 10).
 *
 * Implements IFluidProvider interface for fluid state queries from
 * downstream systems (BuildingSystem, ZoneSystem).
 *
 * @see /docs/canon/interfaces.yaml (fluid.FluidSystem)
 * @see /docs/epics/epic-6/tickets.md (ticket 6-009)
 */

#pragma once

#include <sims3000/fluid/FluidCoverageGrid.h>
#include <sims3000/fluid/PerPlayerFluidPool.h>
#include <sims3000/fluid/FluidEnums.h>
#include <sims3000/fluid/FluidProducerComponent.h>
#include <sims3000/fluid/FluidReservoirComponent.h>
#include <sims3000/fluid/FluidConduitComponent.h>
#include <sims3000/fluid/FluidEvents.h>
#include <sims3000/building/ForwardDependencyInterfaces.h>
#include <entt/entt.hpp>
#include <cstdint>
#include <unordered_map>
#include <vector>

// Forward declarations
namespace sims3000 {
namespace terrain {
    class ITerrainQueryable;
} // namespace terrain
} // namespace sims3000

namespace sims3000 {
namespace fluid {

/**
 * @class FluidSystem
 * @brief Top-level system orchestrating fluid extraction, storage, distribution, and coverage.
 *
 * Implements ISimulatable interface (duck-typed, not inherited to avoid diamond
 * with other systems) at priority 20. Fluid runs after energy (10) and before
 * zones (30) and buildings (40).
 *
 * Implements IFluidProvider for downstream systems to query fluid state.
 *
 * Construction requires map dimensions and an optional terrain pointer.
 * The terrain pointer is used by later tickets for water distance queries
 * and extractor placement validation.
 */
class FluidSystem : public building::IFluidProvider {
public:
    /**
     * @brief Construct FluidSystem with map dimensions and optional terrain.
     *
     * Initializes the coverage grid to the given map size and all per-player
     * pools to default (Healthy) state. Zeroes all dirty flags.
     *
     * @param map_width Map width in tiles.
     * @param map_height Map height in tiles.
     * @param terrain Pointer to terrain query interface (may be nullptr).
     */
    FluidSystem(uint32_t map_width, uint32_t map_height,
                terrain::ITerrainQueryable* terrain = nullptr);

    // =========================================================================
    // Registry and dependency injection
    // =========================================================================

    /**
     * @brief Set the ECS registry pointer for component queries.
     *
     * Must be called before placement methods or has_fluid will return
     * real values. If not set (or set to nullptr), those methods return
     * safe defaults (false / 0).
     *
     * @param registry Non-owning pointer to the ECS registry.
     */
    void set_registry(entt::registry* registry);

    /**
     * @brief Set the energy provider for power state queries.
     *
     * Extractors require energy to operate. The energy provider is
     * queried during output calculation to determine if extractors
     * are powered.
     *
     * @param provider Non-owning pointer to the energy provider.
     */
    void set_energy_provider(building::IEnergyProvider* provider);

    // =========================================================================
    // ISimulatable interface (duck-typed)
    // =========================================================================

    /**
     * @brief Called every simulation tick.
     *
     * Orchestrates the full fluid pipeline:
     * Phase 0: clear_transition_events()
     * Phase 1: (reserved for future)
     * Phase 2: update_extractor_outputs()
     * Phase 3: update_reservoir_totals()
     * Phase 4: recalculate_coverage() if dirty
     * Phase 5: aggregate_consumption()
     * Phase 6: calculate_pool() + calculate_pool_state() + apply_reservoir_buffering()
     * Phase 7: distribute_fluid()
     * Phase 8: update_conduit_active_states()
     * Phase 9: emit_state_change_events()
     *
     * @param delta_time Time since last tick in seconds.
     */
    void tick(float delta_time);

    /**
     * @brief Get execution priority (lower = earlier).
     * @return 20 per canonical interface spec (fluid runs after energy at 10).
     */
    int get_priority() const;

    // =========================================================================
    // IFluidProvider interface implementation
    //
    // No grace period for fluid (CCR-006) - reservoir buffer serves this
    // purpose. Fluid cuts off immediately when pool surplus goes negative.
    // =========================================================================

    /**
     * @brief Check if entity is currently receiving fluid.
     *
     * Queries the FluidComponent on the entity via the ECS registry.
     * Returns false if no registry is set, or if the entity does not
     * exist, or if it lacks a FluidComponent.
     *
     * @param entity_id The entity to check.
     * @return true if entity is receiving fluid, false otherwise.
     */
    bool has_fluid(uint32_t entity_id) const override;

    /**
     * @brief Check if position has fluid coverage and surplus.
     *
     * Returns true if the tile at (x, y) is in coverage for the given
     * player AND that player's fluid pool has non-negative surplus.
     *
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @param player_id Owner player ID.
     * @return true if position has fluid coverage AND pool has surplus.
     */
    bool has_fluid_at(uint32_t x, uint32_t y, uint32_t player_id) const override;

    // =========================================================================
    // Registration methods
    // =========================================================================

    /**
     * @brief Register a fluid extractor entity for a player.
     * @param entity_id Entity ID of the extractor.
     * @param owner Owning player ID (0-3).
     */
    void register_extractor(uint32_t entity_id, uint8_t owner);

    /**
     * @brief Unregister a fluid extractor entity for a player.
     * @param entity_id Entity ID of the extractor.
     * @param owner Owning player ID (0-3).
     */
    void unregister_extractor(uint32_t entity_id, uint8_t owner);

    /**
     * @brief Register a fluid reservoir entity for a player.
     * @param entity_id Entity ID of the reservoir.
     * @param owner Owning player ID (0-3).
     */
    void register_reservoir(uint32_t entity_id, uint8_t owner);

    /**
     * @brief Unregister a fluid reservoir entity for a player.
     * @param entity_id Entity ID of the reservoir.
     * @param owner Owning player ID (0-3).
     */
    void unregister_reservoir(uint32_t entity_id, uint8_t owner);

    /**
     * @brief Register a fluid consumer entity for a player.
     * @param entity_id Entity ID of the consumer.
     * @param owner Owning player ID (0-3).
     */
    void register_consumer(uint32_t entity_id, uint8_t owner);

    /**
     * @brief Unregister a fluid consumer entity for a player.
     * @param entity_id Entity ID of the consumer.
     * @param owner Owning player ID (0-3).
     */
    void unregister_consumer(uint32_t entity_id, uint8_t owner);

    // =========================================================================
    // Position registration methods
    // =========================================================================

    /**
     * @brief Register an extractor entity's grid position for spatial lookup.
     * @param entity_id Entity ID of the extractor.
     * @param owner Owning player ID (0-3).
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     */
    void register_extractor_position(uint32_t entity_id, uint8_t owner,
                                     uint32_t x, uint32_t y);

    /**
     * @brief Register a reservoir entity's grid position for spatial lookup.
     * @param entity_id Entity ID of the reservoir.
     * @param owner Owning player ID (0-3).
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     */
    void register_reservoir_position(uint32_t entity_id, uint8_t owner,
                                     uint32_t x, uint32_t y);

    /**
     * @brief Register a consumer entity's grid position for spatial lookup.
     * @param entity_id Entity ID of the consumer.
     * @param owner Owning player ID (0-3).
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     */
    void register_consumer_position(uint32_t entity_id, uint8_t owner,
                                    uint32_t x, uint32_t y);

    // =========================================================================
    // Placement validation
    // =========================================================================

    /**
     * @brief Validate conduit placement at a grid position.
     *
     * Checks in order:
     * 1. Bounds check: x < map_width, y < map_height
     * 2. Owner check: owner < MAX_PLAYERS
     * 3. Terrain buildable check (if terrain != nullptr)
     *
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @param owner Owning player ID (0-3).
     * @return true if placement is valid, false otherwise.
     */
    bool validate_conduit_placement(uint32_t x, uint32_t y, uint8_t owner) const;

    // =========================================================================
    // Placement methods
    // =========================================================================

    /**
     * @brief Place a fluid extractor at a grid position.
     *
     * Creates an entity with FluidProducerComponent, registers the
     * extractor and its position, marks coverage dirty, and emits
     * ExtractorPlacedEvent.
     *
     * Requires set_registry() to have been called. Returns INVALID_ENTITY_ID
     * on failure (no registry or out-of-bounds).
     *
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @param owner Owning player ID (0-3).
     * @return Entity ID of the created extractor, or INVALID_ENTITY_ID on failure.
     */
    uint32_t place_extractor(uint32_t x, uint32_t y, uint8_t owner);

    /**
     * @brief Place a fluid reservoir at a grid position.
     *
     * Creates an entity with FluidReservoirComponent + FluidProducerComponent,
     * registers the reservoir and its position, marks coverage dirty, and emits
     * ReservoirPlacedEvent.
     *
     * Requires set_registry() to have been called. Returns INVALID_ENTITY_ID
     * on failure.
     *
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @param owner Owning player ID (0-3).
     * @return Entity ID of the created reservoir, or INVALID_ENTITY_ID on failure.
     */
    uint32_t place_reservoir(uint32_t x, uint32_t y, uint8_t owner);

    /**
     * @brief Place a fluid conduit at a grid position.
     *
     * Validates placement, creates an entity with FluidConduitComponent
     * (coverage_radius=3, is_connected=false, is_active=false, conduit_level=1),
     * registers the conduit position, marks coverage dirty, and emits
     * FluidConduitPlacedEvent.
     *
     * Cost deduction stubbed: not yet deducted, needs ICreditProvider.
     *
     * Requires set_registry() to have been called. Returns INVALID_ENTITY_ID
     * on failure.
     *
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @param owner Owning player ID (0-3).
     * @return Entity ID of the created conduit, or INVALID_ENTITY_ID on failure.
     */
    uint32_t place_conduit(uint32_t x, uint32_t y, uint8_t owner);

    /**
     * @brief Remove a fluid conduit from the grid.
     *
     * Validates the entity exists and has a FluidConduitComponent,
     * unregisters the conduit position, emits FluidConduitRemovedEvent,
     * marks coverage dirty for the owner, and destroys the entity.
     *
     * @param entity_id Entity ID of the conduit to remove.
     * @param owner Owning player ID (0-3).
     * @param x X coordinate (column) of the conduit.
     * @param y Y coordinate (row) of the conduit.
     * @return true on success, false on failure.
     */
    bool remove_conduit(uint32_t entity_id, uint8_t owner, uint32_t x, uint32_t y);

    // =========================================================================
    // Conduit preview
    // =========================================================================

    /**
     * @brief Preview coverage delta if a conduit were placed at (x, y).
     *
     * Returns the list of tiles that would GAIN coverage for the given owner
     * if a conduit were placed at (x, y). Does not modify any state.
     *
     * Algorithm:
     * 1. Get current coverage state
     * 2. Simulate adding conduit: mark_coverage_radius around (x,y) with radius=3
     * 3. Find tiles that are now covered but weren't before
     * 4. Return those tiles as the delta
     *
     * Performance target: <5ms for real-time preview.
     *
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @param owner Owning player ID (0-3).
     * @return Vector of (x, y) tile pairs that would gain coverage.
     */
    std::vector<std::pair<uint32_t, uint32_t>> preview_conduit_coverage(
        uint32_t x, uint32_t y, uint8_t owner) const;

    // =========================================================================
    // Coverage queries
    // =========================================================================

    /**
     * @brief Check if a tile is in coverage for a specific owner.
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @param owner The overseer_id to check for.
     * @return true if the tile is covered by the specified owner.
     */
    bool is_in_coverage(uint32_t x, uint32_t y, uint8_t owner) const;

    /**
     * @brief Get the coverage owner at a tile position.
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @return The overseer_id (1-4) or 0 if uncovered.
     */
    uint8_t get_coverage_at(uint32_t x, uint32_t y) const;

    /**
     * @brief Get the number of tiles covered by a specific owner.
     * @param owner The overseer_id to count.
     * @return Number of tiles covered by the specified owner.
     */
    uint32_t get_coverage_count(uint8_t owner) const;

    /**
     * @brief Check if coverage is dirty for a specific player.
     * @param owner Player ID (0-3).
     * @return true if coverage needs recomputation.
     */
    bool is_coverage_dirty(uint8_t owner) const;

    // =========================================================================
    // Pool queries
    // =========================================================================

    /**
     * @brief Get the fluid pool for a specific player.
     * @param owner Player ID (0-3).
     * @return Const reference to the player's fluid pool.
     */
    const PerPlayerFluidPool& get_pool(uint8_t owner) const;

    /**
     * @brief Get the pool health state for a specific player.
     * @param owner Player ID (0-3).
     * @return The FluidPoolState for the player.
     */
    FluidPoolState get_pool_state(uint8_t owner) const;

    // =========================================================================
    // Fluid distribution (Ticket 6-019)
    // =========================================================================

    /**
     * @brief Distribute fluid to consumers using all-or-nothing semantics.
     *
     * Called in tick() phase 7, after reservoir buffering. Per CCR-002,
     * NO priority rationing -- all consumers are treated equally.
     *
     * DESIGN NOTE (CCR-002): Fluid uses all-or-nothing distribution.
     * Unlike EnergySystem which has 4-tier priority-based rationing
     * (Critical, Important, Normal, Low), FluidSystem distributes
     * equally to all consumers. During deficit, ALL consumers lose
     * fluid simultaneously. See ticket 6-020 for rationale.
     *
     * Distribution logic:
     * - If pool.surplus >= 0 (after reservoir buffering): ALL consumers in
     *   coverage get full fluid (fluid_received = fluid_required, has_fluid = true)
     * - If pool.surplus < 0 (after reservoir drain exhausted): ALL consumers
     *   lose fluid (fluid_received = 0, has_fluid = false)
     * - Consumers OUTSIDE coverage always: fluid_received = 0, has_fluid = false
     *
     * @param owner Player ID (0-3).
     * @see Ticket 6-019: Fluid Distribution (All-or-Nothing)
     * @see Ticket 6-020: No-Rationing Design Note (CCR-002)
     */
    void distribute_fluid(uint8_t owner);

    /**
     * @brief Snapshot has_fluid states for all consumers before distribution.
     *
     * Called before distribute_fluid to capture previous state for change
     * detection. Stores has_fluid for each consumer entity.
     *
     * @param owner Player ID (0-3).
     */
    void snapshot_fluid_states(uint8_t owner);

    /**
     * @brief Emit FluidStateChangedEvent for consumers whose has_fluid changed.
     *
     * Compares current has_fluid with snapshot taken before distribution.
     * Emits FluidStateChangedEvent for each consumer that transitioned.
     *
     * @param owner Player ID (0-3).
     */
    void emit_state_change_events(uint8_t owner);

    // =========================================================================
    // Entity count accessors (for testing)
    // =========================================================================

    /**
     * @brief Get the number of registered extractors for a player.
     * @param owner Player ID (0-3).
     * @return Number of registered extractor entities.
     */
    uint32_t get_extractor_count(uint8_t owner) const;

    /**
     * @brief Get the number of registered reservoirs for a player.
     * @param owner Player ID (0-3).
     * @return Number of registered reservoir entities.
     */
    uint32_t get_reservoir_count(uint8_t owner) const;

    /**
     * @brief Get the number of registered consumers for a player.
     * @param owner Player ID (0-3).
     * @return Number of registered consumer entities.
     */
    uint32_t get_consumer_count(uint8_t owner) const;

    /**
     * @brief Get the number of registered conduit positions for a player.
     * @param owner Player ID (0-3).
     * @return Number of conduit positions registered.
     */
    uint32_t get_conduit_position_count(uint8_t owner) const;

    // =========================================================================
    // Event accessors
    // =========================================================================

    /**
     * @brief Get state change events emitted during the last tick.
     * @return Const reference to the vector of FluidStateChangedEvent.
     */
    const std::vector<FluidStateChangedEvent>& get_state_changed_events() const;

    /**
     * @brief Get deficit began events emitted during the last tick.
     * @return Const reference to the vector of FluidDeficitBeganEvent.
     */
    const std::vector<FluidDeficitBeganEvent>& get_deficit_began_events() const;

    /**
     * @brief Get deficit ended events emitted during the last tick.
     * @return Const reference to the vector of FluidDeficitEndedEvent.
     */
    const std::vector<FluidDeficitEndedEvent>& get_deficit_ended_events() const;

    /**
     * @brief Get collapse began events emitted during the last tick.
     * @return Const reference to the vector of FluidCollapseBeganEvent.
     */
    const std::vector<FluidCollapseBeganEvent>& get_collapse_began_events() const;

    /**
     * @brief Get collapse ended events emitted during the last tick.
     * @return Const reference to the vector of FluidCollapseEndedEvent.
     */
    const std::vector<FluidCollapseEndedEvent>& get_collapse_ended_events() const;

    /**
     * @brief Get conduit placed events emitted during the last tick.
     * @return Const reference to the vector of FluidConduitPlacedEvent.
     */
    const std::vector<FluidConduitPlacedEvent>& get_conduit_placed_events() const;

    /**
     * @brief Get conduit removed events emitted during the last tick.
     * @return Const reference to the vector of FluidConduitRemovedEvent.
     */
    const std::vector<FluidConduitRemovedEvent>& get_conduit_removed_events() const;

    /**
     * @brief Get extractor placed events emitted during the last tick.
     * @return Const reference to the vector of ExtractorPlacedEvent.
     */
    const std::vector<ExtractorPlacedEvent>& get_extractor_placed_events() const;

    /**
     * @brief Get extractor removed events emitted during the last tick.
     * @return Const reference to the vector of ExtractorRemovedEvent.
     */
    const std::vector<ExtractorRemovedEvent>& get_extractor_removed_events() const;

    /**
     * @brief Get reservoir placed events emitted during the last tick.
     * @return Const reference to the vector of ReservoirPlacedEvent.
     */
    const std::vector<ReservoirPlacedEvent>& get_reservoir_placed_events() const;

    /**
     * @brief Get reservoir removed events emitted during the last tick.
     * @return Const reference to the vector of ReservoirRemovedEvent.
     */
    const std::vector<ReservoirRemovedEvent>& get_reservoir_removed_events() const;

    /**
     * @brief Get reservoir level changed events emitted during the last tick.
     * @return Const reference to the vector of ReservoirLevelChangedEvent.
     */
    const std::vector<ReservoirLevelChangedEvent>& get_reservoir_level_changed_events() const;

    /**
     * @brief Clear all transition event buffers.
     *
     * Called at the start of each tick() to reset event buffers before
     * new events are emitted during pool state transition detection.
     */
    void clear_transition_events();

    // =========================================================================
    // Building event handlers
    // =========================================================================

    /**
     * @brief Handle a building construction event from BuildingSystem.
     *
     * Checks the entity in the registry for fluid-related components:
     * - If entity has FluidComponent: registers as consumer + position.
     * - If entity has FluidProducerComponent: registers as extractor + position,
     *   and marks coverage dirty for the owner.
     *
     * @param entity_id The constructed building entity ID.
     * @param owner Owning player ID (0-3).
     * @param grid_x Grid X coordinate of the building.
     * @param grid_y Grid Y coordinate of the building.
     */
    void on_building_constructed(uint32_t entity_id, uint8_t owner,
                                 int32_t grid_x, int32_t grid_y);

    /**
     * @brief Handle a building deconstruction event.
     *
     * Checks if the entity was registered as a consumer or producer:
     * - If consumer: unregisters consumer and consumer position.
     * - If producer: unregisters extractor/reservoir and position,
     *   marks coverage dirty for the owner.
     *
     * @param entity_id The deconstructed building entity ID.
     * @param owner Owning player ID (0-3).
     * @param grid_x Grid X coordinate of the building.
     * @param grid_y Grid Y coordinate of the building.
     */
    void on_building_deconstructed(uint32_t entity_id, uint8_t owner,
                                   int32_t grid_x, int32_t grid_y);

    // =========================================================================
    // Map dimension accessors
    // =========================================================================

    /**
     * @brief Get map width in tiles.
     * @return Map width.
     */
    uint32_t get_map_width() const;

    /**
     * @brief Get map height in tiles.
     * @return Map height.
     */
    uint32_t get_map_height() const;

private:
    // =========================================================================
    // Tick pipeline methods (private)
    // =========================================================================

    /**
     * @brief Update extractor outputs based on power state and water distance.
     *
     * Called during tick phase 2. For each extractor per player:
     * - Gets FluidProducerComponent from registry
     * - Checks power state via energy provider
     * - Looks up water distance from extractor position and terrain
     * - Calculates water_factor via calculate_water_factor()
     * - Sets current_output = base_output * water_factor * (powered ? 1 : 0)
     * - Sets is_operational = powered AND distance <= max_operational_distance
     * - Accumulates total generation into pool
     *
     * @see Ticket 6-014: Extractor Registration and Output Calculation
     */
    void update_extractor_outputs();

    /**
     * @brief Update reservoir totals for all players.
     *
     * Called during tick phase 3. For each reservoir per player:
     * - Gets FluidReservoirComponent from registry
     * - Sums current_level into pool.total_reservoir_stored
     * - Sums capacity into pool.total_reservoir_capacity
     * - Counts active reservoirs into pool.reservoir_count
     *
     * @see Ticket 6-015: Reservoir Registration and Storage Management
     */
    void update_reservoir_totals();

    // =========================================================================
    // Pool calculation (Ticket 6-017)
    // =========================================================================

    /**
     * @brief Calculate the fluid pool for a specific player.
     *
     * Populates the PerPlayerFluidPool for the given owner:
     * - available = total_generated + total_reservoir_stored
     * - surplus = available - total_consumed (can be negative)
     * - Stores previous state, then calculates new state via calculate_pool_state()
     *
     * Called in tick() phase 6 after coverage recalc and output updates.
     *
     * @param owner Player ID (0-3).
     * @see Ticket 6-017: Pool Calculation
     */
    void calculate_pool(uint8_t owner);

    /**
     * @brief Determine pool health state from pool aggregates.
     *
     * State logic:
     * - No generation AND no consumption: Healthy (empty grid)
     * - surplus >= 10% of available: Healthy
     * - surplus >= 0: Marginal
     * - surplus < 0 AND reservoir_stored > 0: Deficit
     * - surplus < 0 AND reservoir_stored == 0: Collapse
     *
     * @param pool The pool to evaluate.
     * @return The FluidPoolState for the pool.
     */
    static FluidPoolState calculate_pool_state(const PerPlayerFluidPool& pool);

    // =========================================================================
    // Reservoir buffering (Ticket 6-018)
    // =========================================================================

    /**
     * @brief Apply reservoir fill/drain logic based on pool surplus.
     *
     * Called in tick() after pool calculation:
     * - If surplus > 0 (excess fluid): FILL reservoirs proportionally
     *   by remaining capacity. Fill amount per reservoir limited by fill_rate.
     * - If surplus < 0 (deficit): DRAIN reservoirs proportionally
     *   by current_level. Drain amount per reservoir limited by drain_rate.
     *   If deficit_remaining reaches 0 after drain, pool is buffered
     *   (Deficit not Collapse).
     *
     * Emits ReservoirLevelChangedEvent for each reservoir whose level changes.
     *
     * @param owner Player ID (0-3).
     * @see Ticket 6-018: Pool State Machine and Reservoir Buffering
     */
    void apply_reservoir_buffering(uint8_t owner);

    // =========================================================================
    // Pool state transition detection (Ticket 6-018)
    // =========================================================================

    /**
     * @brief Detect pool state transitions and emit appropriate events.
     *
     * Compares pool.previous_state to pool.state:
     * - Healthy/Marginal -> Deficit: emit FluidDeficitBeganEvent
     * - Deficit -> Healthy/Marginal: emit FluidDeficitEndedEvent
     * - Any -> Collapse: emit FluidCollapseBeganEvent
     * - Collapse -> Any: emit FluidCollapseEndedEvent
     *
     * Updates pool.previous_state to pool.state after detection.
     *
     * @param owner Player ID (0-3).
     * @see Ticket 6-018: Pool State Machine and Reservoir Buffering
     */
    void detect_pool_state_transitions(uint8_t owner);

    // =========================================================================
    // Conduit active state (Ticket 6-032)
    // =========================================================================

    /**
     * @brief Update conduit active states for rendering.
     *
     * For each conduit of the given owner:
     *   is_active = is_connected AND (pool.total_generated > 0)
     *
     * Called in tick() phase 8 (after distribution).
     * is_active is used by the rendering system for flow pulse visual.
     *
     * @param owner Player ID (0-3).
     * @see Ticket 6-032: Conduit Active State for Rendering
     */
    void update_conduit_active_states(uint8_t owner);

    // =========================================================================
    // Spatial lookup helpers
    // =========================================================================

    /**
     * @brief Pack two 32-bit coordinates into a single 64-bit key.
     * @param x X coordinate.
     * @param y Y coordinate.
     * @return Packed 64-bit key (x in upper 32 bits, y in lower 32 bits).
     */
    static uint64_t pack_position(uint32_t x, uint32_t y);

    /**
     * @brief Unpack X coordinate from a packed 64-bit position key.
     * @param packed Packed position key.
     * @return X coordinate (upper 32 bits).
     */
    static uint32_t unpack_x(uint64_t packed);

    /**
     * @brief Unpack Y coordinate from a packed 64-bit position key.
     * @param packed Packed position key.
     * @return Y coordinate (lower 32 bits).
     */
    static uint32_t unpack_y(uint64_t packed);

    // =========================================================================
    // Private members
    // =========================================================================

    /// ECS registry pointer for component queries (non-owning, may be nullptr)
    entt::registry* m_registry = nullptr;

    /// Terrain query interface (non-owning, may be nullptr)
    terrain::ITerrainQueryable* m_terrain = nullptr;

    /// Energy provider for power state queries (non-owning, may be nullptr)
    building::IEnergyProvider* m_energy_provider = nullptr;

    /// Coverage grid (spatial coverage tracking)
    FluidCoverageGrid m_coverage_grid;

    /// Per-player fluid pools
    PerPlayerFluidPool m_pools[MAX_PLAYERS];

    /// Per-player coverage dirty flags
    bool m_coverage_dirty[MAX_PLAYERS];

    /// Per-player extractor entity ID lists
    std::vector<uint32_t> m_extractor_ids[MAX_PLAYERS];

    /// Per-player reservoir entity ID lists
    std::vector<uint32_t> m_reservoir_ids[MAX_PLAYERS];

    /// Per-player extractor spatial lookup: packed(x,y) -> entity_id
    std::unordered_map<uint64_t, uint32_t> m_extractor_positions[MAX_PLAYERS];

    /// Per-player reservoir spatial lookup: packed(x,y) -> entity_id
    std::unordered_map<uint64_t, uint32_t> m_reservoir_positions[MAX_PLAYERS];

    /// Per-player conduit spatial lookup: packed(x,y) -> entity_id
    std::unordered_map<uint64_t, uint32_t> m_conduit_positions[MAX_PLAYERS];

    /// Per-player consumer spatial lookup: packed(x,y) -> entity_id
    std::unordered_map<uint64_t, uint32_t> m_consumer_positions[MAX_PLAYERS];

    /// Per-player consumer entity ID lists
    std::vector<uint32_t> m_consumer_ids[MAX_PLAYERS];

    /// Map dimensions (cached for accessors)
    uint32_t m_map_width;
    uint32_t m_map_height;

    /// Per-player previous has_fluid snapshot for state change detection (Ticket 6-022)
    std::unordered_map<uint32_t, bool> m_prev_has_fluid[MAX_PLAYERS];

    // =========================================================================
    // Event buffers (one per event type)
    // =========================================================================

    std::vector<FluidStateChangedEvent> m_state_changed_events;
    std::vector<FluidDeficitBeganEvent> m_deficit_began_events;
    std::vector<FluidDeficitEndedEvent> m_deficit_ended_events;
    std::vector<FluidCollapseBeganEvent> m_collapse_began_events;
    std::vector<FluidCollapseEndedEvent> m_collapse_ended_events;
    std::vector<FluidConduitPlacedEvent> m_conduit_placed_events;
    std::vector<FluidConduitRemovedEvent> m_conduit_removed_events;
    std::vector<ExtractorPlacedEvent> m_extractor_placed_events;
    std::vector<ExtractorRemovedEvent> m_extractor_removed_events;
    std::vector<ReservoirPlacedEvent> m_reservoir_placed_events;
    std::vector<ReservoirRemovedEvent> m_reservoir_removed_events;
    std::vector<ReservoirLevelChangedEvent> m_reservoir_level_changed_events;
};

} // namespace fluid
} // namespace sims3000
