/**
 * @file EnergySystem.h
 * @brief Top-level EnergySystem class skeleton for Epic 5 (Ticket 5-008)
 *
 * EnergySystem orchestrates all energy subsystems:
 * - CoverageGrid: spatial coverage tracking
 * - PerPlayerEnergyPool: per-player supply/demand aggregation
 * - Nexus management: registration of energy producers
 * - Consumer management: registration of energy consumers
 *
 * Implements ISimulatable interface (duck-typed, matching BuildingSystem pattern)
 * at priority 10 per canonical interface spec.
 *
 * Implements IEnergyProvider interface for power state queries from
 * downstream systems (BuildingSystem, ZoneSystem).
 *
 * @see /docs/canon/interfaces.yaml (energy.EnergySystem)
 * @see /docs/epics/epic-5/tickets.md (ticket 5-008)
 * @see Ticket 5-015: Coverage dirty flag event handlers
 * @see Ticket 5-016: Ownership boundary enforcement in coverage
 */

#pragma once

#include <sims3000/energy/CoverageGrid.h>
#include <sims3000/energy/PerPlayerEnergyPool.h>
#include <sims3000/energy/EnergyEnums.h>
#include <sims3000/energy/EnergyProducerComponent.h>
#include <sims3000/energy/EnergyEvents.h>
#include <sims3000/energy/IContaminationSource.h>
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
namespace energy {

/// Maximum number of players (overseers) supported
constexpr uint8_t MAX_PLAYERS = 4;

/// Default conduit placement cost in credits (stub: not actually deducted yet)
constexpr uint32_t DEFAULT_CONDUIT_COST = 10;

/**
 * @struct PlacementResult
 * @brief Result of a placement validation check.
 *
 * Contains a success flag and a human-readable failure reason.
 * Used by validate_nexus_placement() and validate_conduit_placement().
 *
 * @see Ticket 5-026: Nexus Placement Validation
 * @see Ticket 5-027: Energy Conduit Placement and Validation
 */
struct PlacementResult {
    bool success = false;          ///< True if placement is valid
    const char* reason = "";       ///< Human-readable failure reason (empty on success)
};

/// Sentinel value for invalid entity ID (since entity 0 is valid in EnTT)
constexpr uint32_t INVALID_ENTITY_ID = UINT32_MAX;

/**
 * @class EnergySystem
 * @brief Top-level system orchestrating energy production, distribution, and coverage.
 *
 * Implements ISimulatable interface (duck-typed, not inherited to avoid diamond
 * with other systems) at priority 10. Energy runs before zones (30) and
 * buildings (40).
 *
 * Implements IEnergyProvider for downstream systems to query power state.
 *
 * Construction requires map dimensions and an optional terrain pointer.
 * The terrain pointer is used by later tickets for conduit placement cost queries.
 */
class EnergySystem : public building::IEnergyProvider {
public:
    /**
     * @brief Construct EnergySystem with map dimensions and optional terrain.
     *
     * Initializes the coverage grid to the given map size and all per-player
     * pools to default (Healthy) state.
     *
     * @param map_width Map width in tiles.
     * @param map_height Map height in tiles.
     * @param terrain Pointer to terrain query interface (may be nullptr).
     */
    EnergySystem(uint32_t map_width, uint32_t map_height,
                 terrain::ITerrainQueryable* terrain = nullptr);

    // =========================================================================
    // ISimulatable interface (duck-typed)
    // =========================================================================

    /**
     * @brief Called every simulation tick.
     *
     * Stub implementation: empty for now. Will be filled in by later tickets
     * to perform coverage BFS, distribution, pool aggregation, etc.
     *
     * @param delta_time Time since last tick in seconds.
     */
    void tick(float delta_time);

    /**
     * @brief Get execution priority (lower = earlier).
     * @return 10 per canonical interface spec (energy runs before zones/buildings).
     */
    int get_priority() const;

    // =========================================================================
    // Registry access (Ticket 5-009)
    // =========================================================================

    /**
     * @brief Set the ECS registry pointer for component queries.
     *
     * Must be called before is_powered, get_energy_required, or
     * get_energy_received will return real values. If not set (or set
     * to nullptr), those methods return safe defaults (false / 0).
     *
     * @param registry Non-owning pointer to the ECS registry.
     */
    void set_registry(entt::registry* registry);

    // =========================================================================
    // IEnergyProvider interface implementation
    // =========================================================================

    /**
     * @brief Check if entity is currently powered.
     *
     * Queries the EnergyComponent on the entity via the ECS registry.
     * Returns false if no registry is set, or if the entity does not
     * exist, or if it lacks an EnergyComponent.
     *
     * @param entity_id The entity to check.
     * @return true if entity is receiving power, false otherwise.
     */
    bool is_powered(uint32_t entity_id) const override;

    /**
     * @brief Check if position has power coverage and surplus.
     *
     * Returns true if the tile at (x, y) is in coverage for the given
     * player AND that player's energy pool has non-negative surplus.
     *
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @param player_id Owner player ID.
     * @return true if position has power coverage AND pool has surplus.
     */
    bool is_powered_at(uint32_t x, uint32_t y, uint32_t player_id) const override;

    // =========================================================================
    // Nexus aging (Ticket 5-022)
    // =========================================================================

    /**
     * @brief Update nexus aging for a single producer component.
     *
     * Increments ticks_since_built (capped at 65535) and recalculates
     * age_factor using the asymptotic decay curve:
     *   age_factor = floor + (1.0 - floor) * exp(-decay_rate * ticks)
     *
     * Type-specific aging floors per CCR-006:
     *   Carbon=0.60, Petro=0.65, Gaseous=0.70, Nuclear=0.75, Wind=0.80, Solar=0.85
     *
     * Default decay rate: 0.0001f (slow decay over thousands of ticks).
     *
     * @param comp The EnergyProducerComponent to age (modified in-place).
     */
    static void update_nexus_aging(EnergyProducerComponent& comp);

    // =========================================================================
    // Nexus output calculation (Ticket 5-010)
    // =========================================================================

    /**
     * @brief Update a single nexus producer's current_output from its parameters.
     *
     * If !comp.is_online, sets current_output = 0 and contamination_output = 0.
     * Otherwise: current_output = base_output * efficiency * age_factor.
     * For Wind/Solar types (variable output), applies a weather stub factor of 0.75f.
     * Contamination is zeroed when offline or current_output == 0 (CCR-007).
     *
     * @param comp The EnergyProducerComponent to update (modified in-place).
     */
    static void update_nexus_output(EnergyProducerComponent& comp);

    /**
     * @brief Update all nexus outputs for a given player.
     *
     * Iterates all registered nexus entity IDs for the given owner,
     * fetches the EnergyProducerComponent from the registry, and calls
     * update_nexus_output on each.
     *
     * Requires set_registry() to have been called. No-op if registry is nullptr.
     *
     * @param owner Player ID (0-3).
     */
    void update_all_nexus_outputs(uint8_t owner);

    /**
     * @brief Get total energy generation for a player.
     *
     * Sums current_output from all registered nexus entities for the owner.
     * Requires set_registry() to have been called. Returns 0 if registry is nullptr.
     *
     * @param owner Player ID (0-3).
     * @return Total current_output across all registered nexuses.
     */
    uint32_t get_total_generation(uint8_t owner) const;

    // =========================================================================
    // Placement validation (Ticket 5-026, 5-027)
    // =========================================================================

    /**
     * @brief Validate nexus placement at a grid position.
     *
     * Checks in order:
     * 1. Bounds check: (x, y) must be within map dimensions.
     * 2. Ownership check: Player must own the tile (stub: always true).
     * 3. Terrain buildable check: ITerrainQueryable.is_buildable() must
     *    return true. If terrain is nullptr, defaults to true.
     * 4. No existing structure check: No structure at position (stub: always passes).
     * 5. Type-specific terrain requirements: Hydro/Geothermal stubbed as
     *    always valid for MVP.
     *
     * @param type The NexusType to place.
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @param owner Owning player ID (0-3).
     * @return PlacementResult with success flag and failure reason.
     */
    PlacementResult validate_nexus_placement(NexusType type, uint32_t x,
                                             uint32_t y, uint8_t owner) const;

    /**
     * @brief Validate conduit placement at a grid position.
     *
     * Checks in order:
     * 1. Bounds check: (x, y) must be within map dimensions.
     * 2. Ownership check: Player must own the tile (stub: always true).
     * 3. Terrain buildable check: ITerrainQueryable.is_buildable() must
     *    return true. If terrain is nullptr, defaults to true.
     * 4. No existing structure check: No structure at position (stub: always passes).
     *
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @param owner Owning player ID (0-3).
     * @return PlacementResult with success flag and failure reason.
     */
    PlacementResult validate_conduit_placement(uint32_t x, uint32_t y,
                                               uint8_t owner) const;

    /**
     * @brief Place a nexus entity at a grid position.
     *
     * Validates placement, creates an entity in the registry with
     * EnergyProducerComponent initialized from NexusTypeConfig,
     * registers the nexus and its position, marks coverage dirty,
     * and returns the entity ID.
     *
     * Requires set_registry() to have been called. Returns 0 on failure
     * (validation failed or no registry).
     *
     * @param type The NexusType to place.
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @param owner Owning player ID (0-3).
     * @return Entity ID of the created nexus, or 0 on failure.
     */
    uint32_t place_nexus(NexusType type, uint32_t x, uint32_t y, uint8_t owner);

    /**
     * @brief Place a conduit entity at a grid position.
     *
     * Validates placement, creates an entity in the registry with
     * EnergyConduitComponent, registers the conduit position, marks
     * coverage dirty, emits ConduitPlacedEvent, and returns the entity ID.
     *
     * Cost is configurable via DEFAULT_CONDUIT_COST (stub: not deducted yet).
     *
     * Requires set_registry() to have been called. Returns 0 on failure
     * (validation failed or no registry).
     *
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @param owner Owning player ID (0-3).
     * @return Entity ID of the created conduit, or 0 on failure.
     */
    uint32_t place_conduit(uint32_t x, uint32_t y, uint8_t owner);

    /**
     * @brief Remove a conduit entity from the grid.
     *
     * Validates the entity exists and has an EnergyConduitComponent,
     * unregisters the conduit position, emits ConduitRemovedEvent,
     * marks coverage dirty for the owner, and destroys the entity.
     *
     * Requires set_registry() to have been called. Returns false on failure
     * (no registry, invalid entity, or entity lacks conduit component).
     *
     * @param entity_id Entity ID of the conduit to remove.
     * @param owner Owning player ID (0-3).
     * @param x X coordinate (column) of the conduit.
     * @param y Y coordinate (row) of the conduit.
     * @return true on success, false on failure.
     *
     * @see Ticket 5-029: Conduit Removal and Coverage Update
     */
    bool remove_conduit(uint32_t entity_id, uint8_t owner, uint32_t x, uint32_t y);

    // =========================================================================
    // Nexus management
    // =========================================================================

    /**
     * @brief Register an energy nexus entity for a player.
     *
     * Adds the entity ID to the player's nexus list.
     *
     * @param entity_id Entity ID of the nexus.
     * @param owner Owning player ID (0-3).
     */
    void register_nexus(uint32_t entity_id, uint8_t owner);

    /**
     * @brief Unregister an energy nexus entity for a player.
     *
     * Removes the entity ID from the player's nexus list.
     *
     * @param entity_id Entity ID of the nexus.
     * @param owner Owning player ID (0-3).
     */
    void unregister_nexus(uint32_t entity_id, uint8_t owner);

    // =========================================================================
    // Consumer management
    // =========================================================================

    /**
     * @brief Register an energy consumer entity for a player.
     *
     * Adds the entity ID to the player's consumer list.
     *
     * @param entity_id Entity ID of the consumer.
     * @param owner Owning player ID (0-3).
     */
    void register_consumer(uint32_t entity_id, uint8_t owner);

    /**
     * @brief Unregister an energy consumer entity for a player.
     *
     * Removes the entity ID from the player's consumer list.
     *
     * @param entity_id Entity ID of the consumer.
     * @param owner Owning player ID (0-3).
     */
    void unregister_consumer(uint32_t entity_id, uint8_t owner);

    // =========================================================================
    // Consumer aggregation (Ticket 5-011)
    // =========================================================================

    /**
     * @brief Register a consumer entity's grid position for spatial lookup.
     *
     * Stores the mapping from (x,y) -> entity_id in the per-player consumer
     * position map. This enables coverage-based aggregation of energy demand.
     *
     * @param entity_id Entity ID of the consumer.
     * @param owner Owning player ID (0-3).
     * @param x X coordinate (column) of the consumer.
     * @param y Y coordinate (row) of the consumer.
     */
    void register_consumer_position(uint32_t entity_id, uint8_t owner,
                                    uint32_t x, uint32_t y);

    /**
     * @brief Unregister a consumer entity's grid position.
     *
     * Removes the mapping for (x,y) from the per-player consumer position map.
     *
     * @param entity_id Entity ID of the consumer (used for validation).
     * @param owner Owning player ID (0-3).
     * @param x X coordinate (column) of the consumer.
     * @param y Y coordinate (row) of the consumer.
     */
    void unregister_consumer_position(uint32_t entity_id, uint8_t owner,
                                      uint32_t x, uint32_t y);

    /**
     * @brief Get the number of registered consumer positions for a player.
     * @param owner Player ID (0-3).
     * @return Number of consumer positions registered.
     */
    uint32_t get_consumer_position_count(uint8_t owner) const;

    /**
     * @brief Aggregate total energy consumption for a player.
     *
     * Iterates all registered consumer positions for the given owner,
     * checks if each position is in coverage (overseer_id = owner + 1),
     * and sums the energy_required from each consumer's EnergyComponent.
     *
     * Requires set_registry() to have been called. Returns 0 if registry
     * is nullptr.
     *
     * @param owner Player ID (0-3).
     * @return Total energy_required for consumers in coverage.
     */
    uint32_t aggregate_consumption(uint8_t owner) const;

    // =========================================================================
    // Coverage queries
    // =========================================================================

    /**
     * @brief Check if a tile is in coverage for a specific owner.
     *
     * Delegates to the internal CoverageGrid.
     *
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @param owner The overseer_id to check for.
     * @return true if the tile is covered by the specified owner.
     */
    bool is_in_coverage(uint32_t x, uint32_t y, uint8_t owner) const;

    /**
     * @brief Get the coverage owner at a tile position.
     *
     * Delegates to the internal CoverageGrid.
     *
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @return The overseer_id (1-4) or 0 if uncovered.
     */
    uint8_t get_coverage_at(uint32_t x, uint32_t y) const;

    /**
     * @brief Get the number of tiles covered by a specific owner.
     *
     * Delegates to the internal CoverageGrid.
     *
     * @param owner The overseer_id to count.
     * @return Number of tiles covered by the specified owner.
     */
    uint32_t get_coverage_count(uint8_t owner) const;

    // =========================================================================
    // Pool state machine (Ticket 5-013)
    // =========================================================================

    /// Buffer threshold: surplus must be >= this fraction of total_generated for Healthy
    static constexpr float BUFFER_THRESHOLD_PERCENT = 0.10f;

    /// Collapse threshold: deficit must exceed this fraction of total_consumed for Collapse
    static constexpr float COLLAPSE_THRESHOLD_PERCENT = 0.50f;

    /**
     * @brief Calculate the pool state from current pool values.
     *
     * Determines EnergyPoolState based on surplus relative to thresholds:
     * - Healthy:  surplus >= buffer_threshold (10% of total_generated)
     * - Marginal: 0 <= surplus < buffer_threshold
     * - Deficit:  -collapse_threshold < surplus < 0
     * - Collapse: surplus <= -collapse_threshold (50% of total_consumed)
     *
     * @param pool The pool to evaluate.
     * @return The calculated EnergyPoolState.
     */
    static EnergyPoolState calculate_pool_state(const PerPlayerEnergyPool& pool);

    /**
     * @brief Detect pool state transitions and emit events.
     *
     * Compares pool.previous_state to pool.state:
     * - Emits EnergyDeficitBeganEvent when transitioning INTO Deficit or Collapse
     * - Emits EnergyDeficitEndedEvent when transitioning OUT OF Deficit (to Healthy/Marginal)
     * - Emits GridCollapseBeganEvent when transitioning INTO Collapse
     * - Emits GridCollapseEndedEvent when transitioning OUT OF Collapse
     * Updates pool.previous_state = pool.state after detection.
     *
     * @param owner Player ID (0-3).
     */
    void detect_pool_state_transitions(uint8_t owner);

    // =========================================================================
    // Energy distribution (Ticket 5-018)
    // =========================================================================

    /**
     * @brief Distribute energy to consumers for a player.
     *
     * Sets is_powered and energy_received for all consumers of the given owner:
     * - If pool.surplus >= 0: all consumers in coverage get powered
     *   (is_powered = true, energy_received = energy_required)
     * - If pool.surplus < 0: all consumers in coverage get unpowered
     *   (is_powered = false, energy_received = 0)
     * - Consumers outside coverage: is_powered = false, energy_received = 0
     *
     * @param owner Player ID (0-3).
     */
    void distribute_energy(uint8_t owner);

    // =========================================================================
    // Pool calculation (Ticket 5-012)
    // =========================================================================

    /**
     * @brief Calculate the energy pool for a specific player.
     *
     * Populates the PerPlayerEnergyPool for the given owner:
     * - total_generated = get_total_generation(owner)
     * - total_consumed = aggregate_consumption(owner)
     * - surplus = total_generated - total_consumed (as int32_t, can be negative)
     * - nexus_count = get_nexus_count(owner)
     * - consumer_count = get_consumer_count(owner)
     *
     * Called by tick() phase 3 after nexus outputs and consumption are calculated.
     *
     * @param owner Player ID (0-3).
     */
    void calculate_pool(uint8_t owner);

    // =========================================================================
    // Pool queries
    // =========================================================================

    /**
     * @brief Get the energy pool for a specific player.
     *
     * @param owner Player ID (0-3).
     * @return Const reference to the player's energy pool.
     */
    const PerPlayerEnergyPool& get_pool(uint8_t owner) const;

    /**
     * @brief Get mutable reference to the energy pool for a specific player.
     *
     * Used by internal subsystems (distribution, aggregation) and tests
     * to modify pool state directly.
     *
     * @param owner Player ID (0-3).
     * @return Mutable reference to the player's energy pool.
     */
    PerPlayerEnergyPool& get_pool_mut(uint8_t owner);

    /**
     * @brief Get the pool health state for a specific player.
     *
     * @param owner Player ID (0-3).
     * @return The EnergyPoolState for the player.
     */
    EnergyPoolState get_pool_state(uint8_t owner) const;

    // =========================================================================
    // Energy component queries
    // =========================================================================

    /**
     * @brief Get energy required by an entity.
     *
     * Queries the EnergyComponent on the entity via the ECS registry.
     * Returns 0 if no registry is set, or if the entity does not
     * exist, or if it lacks an EnergyComponent.
     *
     * @param entity_id The entity to query.
     * @return Energy units required per tick.
     */
    uint32_t get_energy_required(uint32_t entity_id) const;

    /**
     * @brief Get energy received by an entity.
     *
     * Queries the EnergyComponent on the entity via the ECS registry.
     * Returns 0 if no registry is set, or if the entity does not
     * exist, or if it lacks an EnergyComponent.
     *
     * @param entity_id The entity to query.
     * @return Energy units received this tick.
     */
    uint32_t get_energy_received(uint32_t entity_id) const;

    // =========================================================================
    // Coverage dirty management
    // =========================================================================

    /**
     * @brief Mark coverage as dirty for a specific player.
     *
     * When coverage is dirty, the next tick will recompute the coverage
     * BFS for that player.
     *
     * @param owner Player ID (0-3).
     */
    void mark_coverage_dirty(uint8_t owner);

    /**
     * @brief Check if coverage is dirty for a specific player.
     *
     * @param owner Player ID (0-3).
     * @return true if coverage needs recomputation.
     */
    bool is_coverage_dirty(uint8_t owner) const;

    // =========================================================================
    // Event handlers (Ticket 5-015)
    // =========================================================================

    /**
     * @brief Handle conduit placed event - marks coverage dirty for owner.
     *
     * Called when a conduit is placed on the grid. Sets the coverage dirty
     * flag for the owning player so coverage is recalculated on next tick.
     *
     * @param event The ConduitPlacedEvent with entity, owner, and position.
     */
    void on_conduit_placed(const ConduitPlacedEvent& event);

    /**
     * @brief Handle conduit removed event - marks coverage dirty for owner.
     *
     * Called when a conduit is removed from the grid. Sets the coverage dirty
     * flag for the owning player so coverage is recalculated on next tick.
     *
     * @param event The ConduitRemovedEvent with entity, owner, and position.
     */
    void on_conduit_removed(const ConduitRemovedEvent& event);

    /**
     * @brief Handle nexus placed event - marks coverage dirty for owner.
     *
     * Called when a nexus is placed on the grid. Sets the coverage dirty
     * flag for the owning player so coverage is recalculated on next tick.
     *
     * @param event The NexusPlacedEvent with entity, owner, type, and position.
     */
    void on_nexus_placed(const NexusPlacedEvent& event);

    /**
     * @brief Handle nexus removed event - marks coverage dirty for owner.
     *
     * Called when a nexus is removed from the grid. Sets the coverage dirty
     * flag for the owning player so coverage is recalculated on next tick.
     *
     * @param event The NexusRemovedEvent with entity, owner, and position.
     */
    void on_nexus_removed(const NexusRemovedEvent& event);

    /**
     * @brief Handle nexus aged event - emitted when efficiency crosses threshold.
     *
     * Called when a nexus's age_factor drops past a 10% threshold boundary.
     * Currently a no-op handler; future subscribers (UISystem, etc.) will
     * use this to update displays.
     *
     * @param event The NexusAgedEvent with entity, owner, and new efficiency.
     *
     * @see Ticket 5-022: Nexus Aging and Efficiency Degradation
     */
    void on_nexus_aged(const NexusAgedEvent& event);

    // =========================================================================
    // Ownership boundary enforcement (Ticket 5-016)
    // =========================================================================

    /**
     * @brief Check if coverage can extend to a tile for a given owner.
     *
     * Returns true if the tile at (x, y) can be included in the coverage
     * area for the given owner. Currently always returns true since there
     * is no territory/ownership system yet. The check point is integrated
     * into recalculate_coverage() BFS so it can be activated later when
     * territory boundaries are implemented.
     *
     * Future behavior:
     * - Returns true if tile_owner == owner OR tile_owner == GAME_MASTER (unclaimed)
     * - Returns false if tile_owner belongs to a different player
     *
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @param owner Player ID (0-3).
     * @return true if coverage can extend to this tile for the given owner.
     */
    bool can_extend_coverage_to(uint32_t x, uint32_t y, uint8_t owner) const;

    // =========================================================================
    // Grid accessors
    // =========================================================================

    /**
     * @brief Get const reference to the coverage grid.
     * @return Const reference to CoverageGrid.
     */
    const CoverageGrid& get_coverage_grid() const;

    /**
     * @brief Get mutable reference to the coverage grid.
     *
     * Used by internal subsystems (BFS, tick) and tests to modify
     * coverage directly.
     *
     * @return Mutable reference to CoverageGrid.
     */
    CoverageGrid& get_coverage_grid_mut();

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

    // =========================================================================
    // Entity list accessors (for testing)
    // =========================================================================

    /**
     * @brief Get the number of registered nexuses for a player.
     * @param owner Player ID (0-3).
     * @return Number of registered nexus entities.
     */
    uint32_t get_nexus_count(uint8_t owner) const;

    /**
     * @brief Get the number of registered consumers for a player.
     * @param owner Player ID (0-3).
     * @return Number of registered consumer entities.
     */
    uint32_t get_consumer_count(uint8_t owner) const;

    // =========================================================================
    // Spatial position tracking (Ticket 5-014)
    // =========================================================================

    /**
     * @brief Register a conduit entity's grid position for spatial lookup.
     *
     * Stores the mapping from (x,y) -> entity_id in the per-player conduit
     * position map. This enables O(1) lookup of conduit entities at grid
     * positions during BFS traversal.
     *
     * @param entity_id Entity ID of the conduit.
     * @param owner Owning player ID (0-3).
     * @param x X coordinate (column) of the conduit.
     * @param y Y coordinate (row) of the conduit.
     */
    void register_conduit_position(uint32_t entity_id, uint8_t owner,
                                   uint32_t x, uint32_t y);

    /**
     * @brief Unregister a conduit entity's grid position.
     *
     * Removes the mapping for (x,y) from the per-player conduit position map.
     *
     * @param entity_id Entity ID of the conduit (used for validation).
     * @param owner Owning player ID (0-3).
     * @param x X coordinate (column) of the conduit.
     * @param y Y coordinate (row) of the conduit.
     */
    void unregister_conduit_position(uint32_t entity_id, uint8_t owner,
                                     uint32_t x, uint32_t y);

    /**
     * @brief Register a nexus entity's grid position for spatial lookup.
     *
     * Stores the mapping from (x,y) -> entity_id in the per-player nexus
     * position map. Used during BFS seeding to locate nexus positions.
     *
     * @param entity_id Entity ID of the nexus.
     * @param owner Owning player ID (0-3).
     * @param x X coordinate (column) of the nexus.
     * @param y Y coordinate (row) of the nexus.
     */
    void register_nexus_position(uint32_t entity_id, uint8_t owner,
                                 uint32_t x, uint32_t y);

    /**
     * @brief Unregister a nexus entity's grid position.
     *
     * Removes the mapping for (x,y) from the per-player nexus position map.
     *
     * @param entity_id Entity ID of the nexus (used for validation).
     * @param owner Owning player ID (0-3).
     * @param x X coordinate (column) of the nexus.
     * @param y Y coordinate (row) of the nexus.
     */
    void unregister_nexus_position(uint32_t entity_id, uint8_t owner,
                                   uint32_t x, uint32_t y);

    // =========================================================================
    // Coverage BFS algorithm (Ticket 5-014)
    // =========================================================================

    /**
     * @brief Mark a square coverage area around a center point.
     *
     * Marks all cells within the square [cx-radius, cx+radius] x
     * [cy-radius, cy+radius] as covered by the given owner. Clamps
     * to grid bounds automatically.
     *
     * @param cx Center X coordinate.
     * @param cy Center Y coordinate.
     * @param radius Coverage radius in tiles.
     * @param owner_id Coverage owner (1-4, where owner_id = player_id + 1).
     */
    void mark_coverage_radius(uint32_t cx, uint32_t cy, uint8_t radius,
                              uint8_t owner_id);

    /**
     * @brief Recalculate coverage for a specific player via BFS flood-fill.
     *
     * Algorithm:
     * 1. Clear all existing coverage for this owner.
     * 2. Seed BFS frontier from all nexus positions for this player.
     *    Each nexus marks its coverage_radius around itself.
     * 3. BFS through conduit network: for each frontier position, check
     *    4-directional neighbors for conduits owned by this player.
     *    Each discovered conduit marks its coverage_radius and is added
     *    to the frontier.
     * 4. Continue until frontier is empty.
     *
     * Performance: O(conduits), not O(grid cells).
     * Target: <10ms for 512x512 with 5,000 conduits.
     *
     * @param owner Player ID (0-3).
     */
    void recalculate_coverage(uint8_t owner);

    /**
     * @brief Get the number of registered conduit positions for a player.
     * @param owner Player ID (0-3).
     * @return Number of conduit positions registered.
     */
    uint32_t get_conduit_position_count(uint8_t owner) const;

    /**
     * @brief Get the number of registered nexus positions for a player.
     * @param owner Player ID (0-3).
     * @return Number of nexus positions registered.
     */
    uint32_t get_nexus_position_count(uint8_t owner) const;

    // =========================================================================
    // Conduit placement preview (Ticket 5-031)
    // =========================================================================

    /**
     * @brief Preview coverage delta for a hypothetical conduit placement.
     *
     * Returns the list of tiles that would GAIN coverage if a conduit
     * were placed at (x, y) for the given owner. Only returns non-empty
     * results if the hypothetical conduit would be connected to the
     * existing network (adjacent to an existing conduit or nexus).
     *
     * Uses default conduit coverage_radius = 3.
     *
     * Performance target: <5ms for real-time preview.
     *
     * @param x X coordinate (column) of the hypothetical conduit.
     * @param y Y coordinate (row) of the hypothetical conduit.
     * @param owner Owning player ID (0-3).
     * @return Vector of (x, y) tile positions that would gain coverage.
     *
     * @see Ticket 5-031: Conduit Placement Preview - Coverage Delta
     */
    std::vector<std::pair<uint32_t, uint32_t>> preview_conduit_coverage(
        uint32_t x, uint32_t y, uint8_t owner) const;

    // =========================================================================
    // Conduit active state (Ticket 5-030)
    // =========================================================================

    /**
     * @brief Update is_active flag on all conduits for a specific player.
     *
     * For each conduit position owned by the player, looks up the entity
     * in the registry, gets its EnergyConduitComponent, and sets:
     *   is_active = (is_connected AND pool.total_generated > 0)
     *
     * A conduit is active only if it is reachable from a nexus (is_connected)
     * AND the player's energy pool has nonzero generation. This drives
     * rendering glow effects on conduits.
     *
     * @param owner Player ID (0-3).
     *
     * @see Ticket 5-030: Conduit Active State for Rendering
     */
    void update_conduit_active_states(uint8_t owner);

    // =========================================================================
    // Terrain efficiency bonus (Ticket 5-024)
    // =========================================================================

    /**
     * @brief Get terrain-based efficiency bonus for a nexus at a grid position.
     *
     * Returns a float multiplier applied to nexus output based on terrain.
     * Wind nexuses on Ridge terrain get a +20% bonus (returns 1.2f).
     * All other combinations return 1.0f (no bonus).
     *
     * If m_terrain is nullptr, returns 1.0f (no bonus available).
     *
     * @param type The NexusType to evaluate.
     * @param x X coordinate (column) of the nexus position.
     * @param y Y coordinate (row) of the nexus position.
     * @return Terrain efficiency multiplier (>= 1.0f).
     *
     * @see Ticket 5-024: Terrain Bonus Efficiency (Ridges for Wind)
     */
    float get_terrain_efficiency_bonus(NexusType type, uint32_t x, uint32_t y) const;

    // =========================================================================
    // Contamination source queries (Ticket 5-025)
    // =========================================================================

    /**
     * @brief Get all contamination sources for a specific player.
     *
     * Iterates all registered nexus positions for the owner, checks if
     * each nexus is online with current_output > 0 and contamination_output > 0,
     * and returns ContaminationSourceData for each qualifying nexus.
     *
     * Requires set_registry() to have been called. Returns empty vector if
     * registry is nullptr or owner is invalid.
     *
     * @param owner Player ID (0-3).
     * @return Vector of ContaminationSourceData for active contaminating nexuses.
     *
     * @see Ticket 5-025: IContaminationSource Implementation
     */
    std::vector<ContaminationSourceData> get_contamination_sources(uint8_t owner) const;

    // =========================================================================
    // Priority-based rationing (Ticket 5-019)
    // =========================================================================

    /**
     * @brief Apply priority-based rationing during energy deficit.
     *
     * Called from distribute_energy() when pool.surplus < 0 (deficit/collapse).
     * Collects all consumers in coverage for the owner, sorts them by
     * priority ascending (1=Critical first, 4=Low last) with entity_id
     * tie-breaking, then allocates available energy (pool.total_generated)
     * to consumers in priority order until exhausted.
     *
     * Consumers that receive full allocation are powered; others are unpowered.
     * Consumers outside coverage are always unpowered (handled by distribute_energy).
     *
     * @param owner Player ID (0-3).
     *
     * @see Ticket 5-019: Priority-Based Rationing During Deficit
     */
    void apply_rationing(uint8_t owner);

    // =========================================================================
    // Energy state change events (Ticket 5-020)
    // =========================================================================

    /**
     * @brief Emit state change events for consumers whose power state changed.
     *
     * Compares the previous is_powered state (snapshotted before distribution)
     * with the current state for all consumers of the given owner. Records
     * an EnergyStateChangedEvent for each consumer whose state changed.
     *
     * Clears the event buffer at the start of each call.
     *
     * @param owner Player ID (0-3).
     *
     * @see Ticket 5-020: EnergyStateChangedEvent Emission
     */
    void emit_state_change_events(uint8_t owner);

    /**
     * @brief Get the list of state change events emitted during the last tick.
     *
     * @return Const reference to the vector of EnergyStateChangedEvent.
     */
    const std::vector<EnergyStateChangedEvent>& get_state_change_events() const;

    /**
     * @brief Snapshot the current is_powered state for all consumers of a player.
     *
     * Called before distribution to capture previous state for event emission.
     *
     * @param owner Player ID (0-3).
     */
    void snapshot_power_states(uint8_t owner);

    // =========================================================================
    // Building event handler (Ticket 5-032)
    // =========================================================================

    /**
     * @brief Handle a building construction event from BuildingSystem.
     *
     * Checks the entity in the registry for energy-related components:
     * - If entity has EnergyComponent: registers as consumer + position.
     * - If entity has EnergyProducerComponent: registers as nexus + position,
     *   and marks coverage dirty for the owner.
     *
     * Consumer power state (is_powered) is set on the next tick via
     * the distribution phase.
     *
     * @param entity_id The constructed building entity ID.
     * @param owner Owning player ID (0-3).
     * @param grid_x Grid X coordinate of the building.
     * @param grid_y Grid Y coordinate of the building.
     *
     * @see Ticket 5-032: BuildingConstructedEvent Handler
     */
    void on_building_constructed(uint32_t entity_id, uint8_t owner,
                                 int32_t grid_x, int32_t grid_y);

    // =========================================================================
    // Building deconstruction handler (Ticket 5-033)
    // =========================================================================

    /**
     * @brief Handle a building deconstruction event.
     *
     * Checks if the entity was registered as a consumer or producer:
     * - If consumer: unregisters consumer and consumer position.
     * - If producer (nexus): unregisters nexus and nexus position,
     *   marks coverage dirty for the owner.
     *
     * @param entity_id The deconstructed building entity ID.
     * @param owner Owning player ID (0-3).
     * @param grid_x Grid X coordinate of the building.
     * @param grid_y Grid Y coordinate of the building.
     *
     * @see Ticket 5-033: BuildingDeconstructedEvent Handler
     */
    void on_building_deconstructed(uint32_t entity_id, uint8_t owner,
                                   int32_t grid_x, int32_t grid_y);

    // =========================================================================
    // Pool state transition event queries (Ticket 5-021)
    // =========================================================================

    /**
     * @brief Get deficit began events emitted during the last tick.
     * @return Const reference to the vector of EnergyDeficitBeganEvent.
     */
    const std::vector<EnergyDeficitBeganEvent>& get_deficit_began_events() const;

    /**
     * @brief Get deficit ended events emitted during the last tick.
     * @return Const reference to the vector of EnergyDeficitEndedEvent.
     */
    const std::vector<EnergyDeficitEndedEvent>& get_deficit_ended_events() const;

    /**
     * @brief Get grid collapse began events emitted during the last tick.
     * @return Const reference to the vector of GridCollapseBeganEvent.
     */
    const std::vector<GridCollapseBeganEvent>& get_collapse_began_events() const;

    /**
     * @brief Get grid collapse ended events emitted during the last tick.
     * @return Const reference to the vector of GridCollapseEndedEvent.
     */
    const std::vector<GridCollapseEndedEvent>& get_collapse_ended_events() const;

    /**
     * @brief Clear all transition event buffers.
     *
     * Called at the start of each tick() to reset event buffers before
     * new events are emitted during pool state transition detection.
     */
    void clear_transition_events();

private:
    // =========================================================================
    // Spatial lookup helpers (Ticket 5-014)
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
     * @param packed The packed position key.
     * @return The X coordinate.
     */
    static uint32_t unpack_x(uint64_t packed);

    /**
     * @brief Unpack Y coordinate from a packed 64-bit position key.
     * @param packed The packed position key.
     * @return The Y coordinate.
     */
    static uint32_t unpack_y(uint64_t packed);

    // ECS registry pointer for component queries (non-owning, may be nullptr)
    entt::registry* m_registry;

    // Coverage grid (spatial coverage tracking)
    CoverageGrid m_coverage_grid;

    // Per-player energy pools
    PerPlayerEnergyPool m_pools[MAX_PLAYERS];

    // Per-player coverage dirty flags
    bool m_coverage_dirty[MAX_PLAYERS];

    // Per-player nexus entity ID lists
    std::vector<uint32_t> m_nexus_ids[MAX_PLAYERS];

    // Per-player consumer entity ID lists
    std::vector<uint32_t> m_consumer_ids[MAX_PLAYERS];

    // Per-player consumer spatial lookup: packed(x,y) -> entity_id (Ticket 5-011)
    std::unordered_map<uint64_t, uint32_t> m_consumer_positions[MAX_PLAYERS];

    // Per-player conduit spatial lookup: packed(x,y) -> entity_id (Ticket 5-014)
    std::unordered_map<uint64_t, uint32_t> m_conduit_positions[MAX_PLAYERS];

    // Per-player nexus spatial lookup: packed(x,y) -> entity_id (Ticket 5-014)
    std::unordered_map<uint64_t, uint32_t> m_nexus_positions[MAX_PLAYERS];

    // Terrain query interface (non-owning, may be nullptr)
    terrain::ITerrainQueryable* m_terrain;

    // Map dimensions (cached for accessors)
    uint32_t m_map_width;
    uint32_t m_map_height;

    // Per-player previous power state tracking for event emission (Ticket 5-020)
    std::unordered_map<uint32_t, bool> m_prev_powered[MAX_PLAYERS];

    // State change events buffer (Ticket 5-020)
    std::vector<EnergyStateChangedEvent> m_state_change_events;

    // Pool state transition event buffers (Ticket 5-021)
    std::vector<EnergyDeficitBeganEvent> m_deficit_began_events;
    std::vector<EnergyDeficitEndedEvent> m_deficit_ended_events;
    std::vector<GridCollapseBeganEvent> m_collapse_began_events;
    std::vector<GridCollapseEndedEvent> m_collapse_ended_events;
};

} // namespace energy
} // namespace sims3000
