/**
 * @file RailSystem.h
 * @brief RailSystem class skeleton for Epic 7 (Ticket E7-032)
 *
 * RailSystem manages rail segments and terminals for the transit network.
 * Implements ISimulatable interface (duck-typed) at priority 47,
 * running after TransportSystem (45).
 *
 * Tick phases:
 * 1. Update power states from energy provider (stub: all powered)
 * 2. Update active states (powered + terminal connection check)
 * 3. Calculate terminal coverage effects (stub)
 *
 * Uses canonical alien terminology per /docs/canon/terminology.yaml
 */

#pragma once

#include <sims3000/transport/RailComponent.h>
#include <sims3000/transport/TerminalComponent.h>
#include <sims3000/building/ForwardDependencyInterfaces.h>
#include <cstdint>
#include <vector>

namespace sims3000 {
namespace transport {

/**
 * @class RailSystem
 * @brief Top-level system managing rail segments and terminals.
 *
 * Implements ISimulatable interface (duck-typed, not inherited) at priority 47.
 * Runs after TransportSystem (priority 45).
 *
 * Power dependency is injected via set_energy_provider(). Until an energy
 * provider is set, all rails are considered powered (stub behavior).
 */
class RailSystem {
public:
    /// Simulation tick priority (runs after TransportSystem at 45)
    static constexpr int TICK_PRIORITY = 47;

    /// Maximum number of players (overseers) supported
    static constexpr uint8_t MAX_PLAYERS = 4;

    /// Per-entity rail storage
    struct RailEntry {
        uint32_t entity_id;
        RailComponent component;
        int32_t x;
        int32_t y;
    };

    /// Per-entity terminal storage
    struct TerminalEntry {
        uint32_t entity_id;
        TerminalComponent component;
        int32_t x;
        int32_t y;
    };

    /**
     * @brief Construct RailSystem with map dimensions.
     *
     * @param map_width Map width in tiles.
     * @param map_height Map height in tiles.
     */
    RailSystem(uint32_t map_width, uint32_t map_height);

    // =========================================================================
    // ISimulatable interface (duck-typed)
    // =========================================================================

    /**
     * @brief Execute one simulation tick.
     *
     * Phases:
     * 1. Update power states from energy provider
     * 2. Update active states (powered + terminal adjacency)
     * 3. Calculate terminal coverage effects (stub)
     *
     * @param delta_time Time delta for this tick (unused in current stub).
     */
    void tick(float delta_time);

    /**
     * @brief Get the tick priority for execution ordering.
     * @return TICK_PRIORITY (47).
     */
    int get_priority() const { return TICK_PRIORITY; }

    // =========================================================================
    // Rail management
    // =========================================================================

    /**
     * @brief Place a rail segment at the given grid position.
     *
     * @param x Grid X coordinate.
     * @param y Grid Y coordinate.
     * @param type Rail type (Surface, Elevated, Subterra).
     * @param owner Owning player ID (0-3).
     * @return Entity ID of the placed rail, or 0 if placement failed.
     */
    uint32_t place_rail(int32_t x, int32_t y, RailType type, uint8_t owner);

    /**
     * @brief Remove a rail segment by entity ID.
     *
     * @param entity_id Entity ID of the rail to remove.
     * @param owner Owning player ID (must match for removal).
     * @return true if the rail was found and removed.
     */
    bool remove_rail(uint32_t entity_id, uint8_t owner);

    // =========================================================================
    // Terminal management
    // =========================================================================

    /**
     * @brief Place a terminal at the given grid position.
     *
     * @param x Grid X coordinate.
     * @param y Grid Y coordinate.
     * @param type Terminal type (SurfaceStation, SubterraStation, IntermodalHub).
     * @param owner Owning player ID (0-3).
     * @return Entity ID of the placed terminal, or 0 if placement failed.
     */
    uint32_t place_terminal(int32_t x, int32_t y, TerminalType type, uint8_t owner);

    /**
     * @brief Remove a terminal by entity ID.
     *
     * @param entity_id Entity ID of the terminal to remove.
     * @param owner Owning player ID (must match for removal).
     * @return true if the terminal was found and removed.
     */
    bool remove_terminal(uint32_t entity_id, uint8_t owner);

    // =========================================================================
    // Power dependency
    // =========================================================================

    /**
     * @brief Set the energy provider for power state queries.
     *
     * Until set, all rails and terminals are considered powered (fallback).
     * When set, power states are queried from the provider each tick.
     *
     * @param provider Non-owning pointer to the energy provider, or nullptr.
     */
    void set_energy_provider(const building::IEnergyProvider* provider);

    // =========================================================================
    // Terminal placement validation (E7-034)
    // =========================================================================

    /**
     * @brief Check if a terminal can be placed at the given position.
     *
     * Validates:
     * 1. Position is in bounds
     * 2. Position is not occupied by another terminal (any player)
     * 3. Adjacent rail track exists (check N/S/E/W for any rail entity)
     *
     * Power check is NOT performed at placement time (deferred to tick).
     *
     * @param x Grid X coordinate.
     * @param y Grid Y coordinate.
     * @param owner Owning player ID (0-3).
     * @return true if placement is valid.
     */
    bool can_place_terminal(int32_t x, int32_t y, uint8_t owner) const;

    /**
     * @brief Check if a terminal meets activation requirements.
     *
     * Terminal activation requires:
     * - is_powered = true (from energy provider or fallback)
     * - Adjacent rail track exists (N/S/E/W)
     *
     * Called during tick phase 2 to determine terminal active state.
     *
     * @param terminal The terminal entry to check.
     * @return true if the terminal should be active.
     */
    bool check_terminal_activation(const TerminalEntry& terminal) const;

    // =========================================================================
    // Queries
    // =========================================================================

    /**
     * @brief Check if a rail segment is powered.
     * @param entity_id Entity ID of the rail.
     * @return true if the rail has power.
     */
    bool is_rail_powered(uint32_t entity_id) const;

    /**
     * @brief Check if a terminal is active.
     * @param entity_id Entity ID of the terminal.
     * @return true if the terminal is operational (powered and connected).
     */
    bool is_terminal_active(uint32_t entity_id) const;

    /**
     * @brief Get the coverage radius of a terminal.
     * @param entity_id Entity ID of the terminal.
     * @return Coverage radius in tiles, or 0 if not found.
     */
    uint8_t get_terminal_coverage_radius(uint32_t entity_id) const;

    // =========================================================================
    // Coverage queries (E7-035)
    // =========================================================================

    /**
     * @brief Check if position is within any active terminal's coverage radius.
     *
     * Searches all terminals for the given owner and checks if the position
     * falls within any active terminal's coverage_radius (Manhattan distance).
     *
     * @param x Grid X coordinate.
     * @param y Grid Y coordinate.
     * @param owner Owning player ID (0-3).
     * @return true if position is within coverage of an active terminal.
     */
    bool is_in_terminal_coverage(int32_t x, int32_t y, uint8_t owner) const;

    /**
     * @brief Get traffic reduction percentage at position (0-100).
     *
     * Buildings within coverage_radius of an active terminal get reduced
     * traffic contribution. Reduction is 50% at the terminal, with linear
     * falloff to 0% at the radius edge.
     *
     * If multiple terminals cover a position, the maximum reduction applies.
     *
     * @param x Grid X coordinate.
     * @param y Grid Y coordinate.
     * @param owner Owning player ID (0-3).
     * @return Reduction percentage (0 = no reduction, 50 = at terminal, 0 at edge).
     */
    uint8_t get_traffic_reduction_at(int32_t x, int32_t y, uint8_t owner) const;

    /**
     * @brief Calculate traffic reduction for a building at position (E7-045).
     *
     * Equivalent to get_traffic_reduction_at. Returns reduction factor 0-100
     * (percentage to reduce traffic by). 50% at terminal, linear falloff to
     * 0% at radius edge. Only active terminals contribute.
     *
     * @param x Grid X coordinate.
     * @param y Grid Y coordinate.
     * @param owner Owning player ID (0-3).
     * @return Reduction percentage (0 = no reduction, 50 = max at terminal).
     */
    uint8_t calculate_traffic_reduction(int32_t x, int32_t y, uint8_t owner) const;

    // =========================================================================
    // State queries
    // =========================================================================

    /**
     * @brief Get the number of rail segments owned by a player.
     * @param owner Player ID (0-3).
     * @return Number of rail segments owned by this player.
     */
    uint32_t get_rail_count(uint8_t owner) const;

    /**
     * @brief Get the number of terminals owned by a player.
     * @param owner Player ID (0-3).
     * @return Number of terminals owned by this player.
     */
    uint32_t get_terminal_count(uint8_t owner) const;

    // =========================================================================
    // Position queries
    // =========================================================================

    /**
     * @brief Check if any rail exists at exactly the given position (any player).
     *
     * Searches all players' rail entries for a rail at the given coordinates.
     *
     * @param x Grid X coordinate.
     * @param y Grid Y coordinate.
     * @return true if a rail segment exists at (x, y).
     */
    bool has_rail_at(int32_t x, int32_t y) const;

private:
    uint32_t map_width_;
    uint32_t map_height_;
    uint32_t next_entity_id_ = 1;

    /// Energy provider for power state queries (nullptr = all powered fallback)
    const building::IEnergyProvider* energy_provider_ = nullptr;

    /// Per-player rail tracking (indexed by owner 0-3)
    std::vector<RailEntry> rails_[MAX_PLAYERS];

    /// Per-player terminal tracking (indexed by owner 0-3)
    std::vector<TerminalEntry> terminals_[MAX_PLAYERS];

    // =========================================================================
    // Internal tick phases
    // =========================================================================

    /// Phase 1: Update power states from energy provider
    void update_power_states();

    /// Phase 2: Update active states (powered + terminal connection)
    void update_active_states();

    /// Phase 3: Calculate terminal coverage effects (stub)
    void update_terminal_coverage();

    // =========================================================================
    // Internal helpers
    // =========================================================================

    /// Check if any rail exists adjacent (N/S/E/W) to the given position (any player)
    bool has_adjacent_rail(int32_t x, int32_t y) const;

    /// Check if a terminal already exists at the given position (any player)
    bool has_terminal_at(int32_t x, int32_t y) const;
};

} // namespace transport
} // namespace sims3000
