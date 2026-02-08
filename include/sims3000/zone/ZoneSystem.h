/**
 * @file ZoneSystem.h
 * @brief ZoneSystem class implementing ISimulatable at priority 30
 *
 * ZoneSystem manages zone designation, demand calculation, and desirability
 * updates. It owns a ZoneGrid for spatial lookups and tracks per-overseer
 * ZoneCounts for aggregate statistics.
 *
 * Dependencies injected via constructor:
 * - ITerrainQueryable*: Terrain queries for zone placement validation
 * - ITransportProvider*: Road proximity checks for zone development
 *
 * @see /docs/canon/interfaces.yaml (ISimulatable, priority 30)
 * @see /docs/epics/epic-4/tickets.md (ticket 4-008)
 */

#ifndef SIMS3000_ZONE_ZONESYSTEM_H
#define SIMS3000_ZONE_ZONESYSTEM_H

#include <sims3000/zone/ZoneTypes.h>
#include <sims3000/zone/ZoneGrid.h>
#include <sims3000/zone/ZoneEvents.h>
#include <sims3000/zone/IZoneQueryable.h>
#include <cstdint>
#include <array>
#include <vector>

// Forward declarations for dependency interfaces
namespace sims3000 {
namespace terrain {
    class ITerrainQueryable;
} // namespace terrain
namespace building {
    class ITransportProvider;
    class IDemandProvider;
} // namespace building
} // namespace sims3000

namespace sims3000 {
namespace zone {

/**
 * @interface ISimulatable
 * @brief Systems that participate in the simulation tick.
 *
 * Ensures consistent update ordering and timing. Systems with lower
 * priority values execute earlier in the tick.
 */
class ISimulatable {
public:
    virtual ~ISimulatable() = default;

    /**
     * @brief Called every simulation tick (server-side).
     * @param delta_time Time since last tick in seconds.
     */
    virtual void tick(float delta_time) = 0;

    /**
     * @brief Get execution priority (lower = earlier).
     * @return Priority value. Default 100.
     */
    virtual int get_priority() const = 0;
};

/// Maximum number of overseers (players) supported
constexpr std::uint8_t MAX_OVERSEERS = 5;

/**
 * @class ZoneSystem
 * @brief Manages zone designation, demand, and desirability.
 *
 * Implements ISimulatable at priority 30 per /docs/canon/interfaces.yaml.
 * Orchestrates demand and desirability updates each simulation tick.
 *
 * Owns:
 * - ZoneGrid: spatial index for zone entities
 * - Per-overseer ZoneCounts: aggregate zone statistics
 * - Per-overseer ZoneDemandData: cached demand values
 */
/**
 * @struct DesirabilityConfig
 * @brief Configurable desirability calculation parameters for ZoneSystem.
 *
 * Controls factor weights and update frequency for per-zone-sector
 * desirability scoring (0-255). Updated every N ticks (not every tick).
 */
struct DesirabilityConfig {
    float terrain_value_weight = 0.4f;       ///< Weight for terrain value bonus
    float pathway_proximity_weight = 0.3f;   ///< Weight for pathway proximity (stub: max)
    float contamination_weight = 0.2f;       ///< Weight for contamination penalty (stub: 0)
    float service_coverage_weight = 0.1f;    ///< Weight for service coverage (stub: neutral)
    uint32_t update_interval_ticks = 10;     ///< Update every N ticks
};

/**
 * @struct DemandConfig
 * @brief Configurable demand calculation parameters for ZoneSystem.
 *
 * Controls base pressures, stub factors, supply saturation thresholds,
 * and soft cap behavior for zone demand calculation.
 */
struct DemandConfig {
    // Base pressure per zone type
    std::int8_t habitation_base = 10;
    std::int8_t exchange_base = 5;
    std::int8_t fabrication_base = 5;

    // Stub factor values (replaced by real systems later)
    std::int8_t population_hab_factor = 20;
    std::int8_t population_exc_factor = 10;
    std::int8_t population_fab_factor = 10;
    std::int8_t employment_factor = 0;
    std::int8_t utility_factor = 10;
    std::int8_t tribute_factor = 0;

    // Supply saturation
    std::uint32_t target_zone_count = 50; ///< Zones per type before saturation kicks in

    // Soft cap
    std::int8_t soft_cap_threshold = 80;
};

/**
 * @struct RedesignateResult
 * @brief Result of a zone redesignation operation (Ticket 4-014).
 *
 * Returned by redesignate_zone() to indicate success/failure and reason.
 */
struct RedesignateResult {
    bool success;
    enum class Reason : std::uint8_t {
        Ok = 0,
        NoZoneAtPosition,
        NotOwned,
        SameTypeAndDensity,
        OccupiedRequiresDemolition
    } reason;
    bool demolition_requested; ///< True if DemolitionRequestEvent was emitted

    RedesignateResult() : success(false), reason(Reason::NoZoneAtPosition), demolition_requested(false) {}
    RedesignateResult(bool s, Reason r, bool demo = false) : success(s), reason(r), demolition_requested(demo) {}
};

class ZoneSystem : public ISimulatable, public IZoneQueryable {
public:
    /**
     * @brief Construct ZoneSystem with dependency injection.
     *
     * @param terrain Pointer to terrain query interface (may be nullptr for testing).
     * @param transport Pointer to transport provider interface (may be nullptr for testing).
     * @param grid_size Grid dimension (must be 128, 256, or 512). Default 256.
     */
    ZoneSystem(terrain::ITerrainQueryable* terrain,
               building::ITransportProvider* transport,
               std::uint16_t grid_size = 256);

    /// ISimulatable: tick at priority 30
    void tick(float delta_time) override;

    /// ISimulatable: priority 30 per canonical interface spec
    int get_priority() const override;

    // =========================================================================
    // Zone Query Methods
    // =========================================================================

    /**
     * @brief Get zone type at grid position.
     * @param x X coordinate.
     * @param y Y coordinate.
     * @param out_type Output zone type.
     * @return true if zone exists at position, false otherwise.
     */
    bool get_zone_type(std::int32_t x, std::int32_t y, ZoneType& out_type) const;

    /**
     * @brief Get zone density at grid position.
     * @param x X coordinate.
     * @param y Y coordinate.
     * @param out_density Output density.
     * @return true if zone exists at position, false otherwise.
     */
    bool get_zone_density(std::int32_t x, std::int32_t y, ZoneDensity& out_density) const;

    /**
     * @brief Check if position is zoned.
     * @param x X coordinate.
     * @param y Y coordinate.
     * @return true if there is a zone at position.
     */
    bool is_zoned(std::int32_t x, std::int32_t y) const;

    /**
     * @brief Get zone count for a specific overseer and type.
     * @param player_id Overseer ID (0-4).
     * @param type Zone type to count.
     * @return Count of zones of that type for the player.
     */
    std::uint32_t get_zone_count(std::uint8_t player_id, ZoneType type) const;

    /**
     * @brief Get demand for zone type (stub returns 0).
     * @param type Zone type.
     * @param player_id Overseer ID.
     * @return Demand value (-100 to +100). Currently returns 0.
     */
    std::int8_t get_demand_for_type(ZoneType type, std::uint8_t player_id) const;

    /**
     * @brief Set zone state at position (callback for BuildingSystem).
     *
     * Validates state transitions. Only these transitions are allowed:
     * - Designated -> Occupied
     * - Occupied -> Designated
     * - Designated -> Stalled
     * - Stalled -> Designated
     *
     * On valid transition, emits a ZoneStateChangedEvent to the pending events queue.
     *
     * @param x X coordinate.
     * @param y Y coordinate.
     * @param new_state New zone state.
     * @return true if state was set, false if no zone at position or invalid transition.
     */
    bool set_zone_state(std::int32_t x, std::int32_t y, ZoneState new_state);

    // =========================================================================
    // Zone Placement Validation (Ticket 4-011)
    // =========================================================================

    /**
     * @struct ValidationResult
     * @brief Result of a single-cell zone placement validation check.
     */
    struct ValidationResult {
        bool success;
        enum class Reason : std::uint8_t {
            Ok = 0,
            OutOfBounds,
            NotOwned,
            TerrainNotBuildable,
            ZoneAlreadyPresent,
            BuildingPresent
        } reason;
        ValidationResult() : success(false), reason(Reason::OutOfBounds) {}
        ValidationResult(bool s, Reason r) : success(s), reason(r) {}
    };

    /**
     * @brief Validate whether a single tile can be zoned.
     *
     * Checks in order: bounds, ownership, terrain buildability, zone overlap.
     * Pathway proximity is NOT checked at designation time (CCR-007).
     *
     * @param x X coordinate.
     * @param y Y coordinate.
     * @param player_id Overseer ID.
     * @return ValidationResult with success flag and reason.
     */
    ValidationResult validate_zone_placement(std::int32_t x, std::int32_t y, std::uint8_t player_id) const;

    /**
     * @brief Validate a rectangular zone placement area.
     *
     * Iterates the rect calling validate_zone_placement per cell.
     *
     * @param request Zone placement request describing the area.
     * @return ZonePlacementResult with placed/skipped counts.
     */
    ZonePlacementResult validate_zone_area(const ZonePlacementRequest& request) const;

    // =========================================================================
    // Zone Placement Execution (Ticket 4-012)
    // =========================================================================

    /**
     * @struct PlacementCostConfig
     * @brief Credit cost configuration for zone placement.
     */
    struct PlacementCostConfig {
        std::uint32_t low_density_cost = 2;   ///< Credit cost per low-density zone
        std::uint32_t high_density_cost = 5;  ///< Credit cost per high-density zone
    };

    /**
     * @brief Place zones in a rectangular area.
     *
     * Iterates the rectangle from request, calls validate_zone_placement() per cell.
     * For valid cells: assigns auto-incrementing entity_id, places zone, emits
     * ZoneDesignatedEvent.
     *
     * @param request Zone placement request describing the area.
     * @return ZonePlacementResult with placed/skipped counts and total cost.
     */
    ZonePlacementResult place_zones(const ZonePlacementRequest& request);

    /**
     * @brief Set placement cost configuration.
     * @param config The PlacementCostConfig to apply.
     */
    void set_placement_cost_config(const PlacementCostConfig& config);

    /**
     * @brief Get current placement cost configuration.
     * @return Const reference to current PlacementCostConfig.
     */
    const PlacementCostConfig& get_placement_cost_config() const;

    /**
     * @brief Get pending zone designated events.
     * @return Const reference to pending designated events vector.
     */
    const std::vector<ZoneDesignatedEvent>& get_pending_designated_events() const;

    /**
     * @brief Clear all pending designated events.
     */
    void clear_pending_designated_events();

    // =========================================================================
    // De-zoning Implementation (Ticket 4-013)
    // =========================================================================

    /**
     * @brief Remove zones in a rectangular area.
     *
     * For each cell in the rectangle:
     * - Designated/Stalled zones: removed immediately, ZoneUndesignatedEvent emitted
     * - Occupied zones: DemolitionRequestEvent emitted (zone not removed yet)
     *
     * @param x Top-left X coordinate.
     * @param y Top-left Y coordinate.
     * @param width Width in tiles.
     * @param height Height in tiles.
     * @param player_id Requesting overseer.
     * @return DezoneResult with removed/skipped/demolition counts.
     */
    DezoneResult remove_zones(std::int32_t x, std::int32_t y,
                              std::int32_t width, std::int32_t height,
                              std::uint8_t player_id);

    /**
     * @brief Finalize zone removal at position (BuildingSystem callback).
     *
     * Called by BuildingSystem after demolition completes to actually remove
     * the zone from the grid and clear zone info.
     *
     * @param x X coordinate.
     * @param y Y coordinate.
     * @return true if zone was removed, false if no zone at position.
     */
    bool finalize_zone_removal(std::int32_t x, std::int32_t y);

    /**
     * @brief Get pending zone undesignated events.
     * @return Const reference to pending undesignated events vector.
     */
    const std::vector<ZoneUndesignatedEvent>& get_pending_undesignated_events() const;

    /**
     * @brief Clear all pending undesignated events.
     */
    void clear_pending_undesignated_events();

    /**
     * @brief Get pending demolition request events.
     * @return Const reference to pending demolition events vector.
     */
    const std::vector<DemolitionRequestEvent>& get_pending_demolition_events() const;

    /**
     * @brief Clear all pending demolition events.
     */
    void clear_pending_demolition_events();

    // =========================================================================
    // Zone Redesignation (Ticket 4-014)
    // =========================================================================

    /**
     * @brief Redesignate a zone to a different type and/or density.
     *
     * Behavior depends on zone state:
     * - Designated/Stalled: directly update type and density
     * - Occupied + type change: emit DemolitionRequestEvent, caller re-zones after
     * - Occupied + density-only change (CCR-005): directly update density
     *
     * @param x X coordinate.
     * @param y Y coordinate.
     * @param new_type New zone type.
     * @param new_density New zone density.
     * @param player_id Requesting overseer.
     * @return RedesignateResult with success flag, reason, and demolition flag.
     */
    RedesignateResult redesignate_zone(std::int32_t x, std::int32_t y,
                                        ZoneType new_type, ZoneDensity new_density,
                                        std::uint8_t player_id);

    // =========================================================================
    // IZoneQueryable Implementation (Ticket 4-035)
    // =========================================================================

    bool get_zone_type_at(std::int32_t x, std::int32_t y, ZoneType& out_type) const override;
    bool get_zone_density_at(std::int32_t x, std::int32_t y, ZoneDensity& out_density) const override;
    bool is_zoned_at(std::int32_t x, std::int32_t y) const override;
    std::uint32_t get_zone_count_for(std::uint8_t player_id, ZoneType type) const override;
    std::vector<GridPosition> get_designated_zones(std::uint8_t player_id, ZoneType type) const override;
    float get_demand_for(ZoneType type, std::uint8_t player_id) const override;

    // =========================================================================
    // External Demand Provider (Ticket 4-017)
    // =========================================================================

    /**
     * @brief Set external demand provider for demand delegation.
     *
     * When set, get_demand_for_type() and get_zone_demand() delegate to this
     * provider instead of using the internal demand calculation.
     *
     * @param provider Pointer to external demand provider (nullptr to clear).
     */
    void set_external_demand_provider(building::IDemandProvider* provider);

    /**
     * @brief Check if an external demand provider is set.
     * @return true if external demand provider is non-null.
     */
    bool has_external_demand_provider() const;

    // =========================================================================
    // Zone State Event Access (Ticket 4-015)
    // =========================================================================

    /**
     * @brief Get pending state change events (for testing/integration).
     * @return Const reference to pending events vector.
     */
    const std::vector<ZoneStateChangedEvent>& get_pending_state_events() const;

    /**
     * @brief Clear all pending state change events.
     */
    void clear_pending_state_events();

    // =========================================================================
    // Demand Configuration and Query (Ticket 4-016)
    // =========================================================================

    /**
     * @brief Set demand calculation configuration.
     * @param config The DemandConfig to apply.
     */
    void set_demand_config(const DemandConfig& config);

    /**
     * @brief Get current demand configuration.
     * @return Const reference to current DemandConfig.
     */
    const DemandConfig& get_demand_config() const;

    /**
     * @brief Get zone demand data for an overseer.
     * @param player_id Overseer ID (0-4).
     * @return ZoneDemandData with current demand values.
     */
    ZoneDemandData get_zone_demand(std::uint8_t player_id) const;

    // =========================================================================
    // Zone State Query (Ticket 4-024)
    // =========================================================================

    /**
     * @brief Get zone state at grid position.
     * @param x X coordinate.
     * @param y Y coordinate.
     * @param out_state Output zone state.
     * @return true if zone exists at position, false otherwise.
     */
    bool get_zone_state(std::int32_t x, std::int32_t y, ZoneState& out_state) const;

    // =========================================================================
    // Desirability Configuration and Query (Ticket 4-018)
    // =========================================================================

    /**
     * @brief Set desirability calculation configuration.
     * @param config The DesirabilityConfig to apply.
     */
    void set_desirability_config(const DesirabilityConfig& config);

    /**
     * @brief Get current desirability configuration.
     * @return Const reference to current DesirabilityConfig.
     */
    const DesirabilityConfig& get_desirability_config() const;

    /**
     * @brief External override for desirability at a position.
     * @param x X coordinate.
     * @param y Y coordinate.
     * @param value Desirability value (0-255).
     */
    void update_desirability(std::int32_t x, std::int32_t y, std::uint8_t value);

    // =========================================================================
    // Zone Placement (for testing and internal use)
    // =========================================================================

    /**
     * @brief Place a zone at position.
     *
     * @param x X coordinate.
     * @param y Y coordinate.
     * @param type Zone type.
     * @param density Zone density.
     * @param player_id Owning overseer.
     * @param entity_id Entity ID to store in grid.
     * @return true if placement succeeded.
     */
    bool place_zone(std::int32_t x, std::int32_t y,
                    ZoneType type, ZoneDensity density,
                    std::uint8_t player_id, std::uint32_t entity_id);

    // =========================================================================
    // Grid Access (for testing)
    // =========================================================================

    /**
     * @brief Get const reference to internal ZoneGrid.
     * @return Const reference to the grid.
     */
    const ZoneGrid& get_grid() const { return m_grid; }

    /**
     * @brief Get zone counts for an overseer.
     * @param player_id Overseer ID (0-4).
     * @return Const reference to ZoneCounts.
     */
    const ZoneCounts& get_zone_counts(std::uint8_t player_id) const;

private:
    /// Terrain query interface (may be nullptr)
    terrain::ITerrainQueryable* m_terrain;

    /// Transport provider interface (may be nullptr)
    building::ITransportProvider* m_transport;

    /// Spatial index for zone entities
    ZoneGrid m_grid;

    /// Per-overseer zone counts
    std::array<ZoneCounts, MAX_OVERSEERS> m_zone_counts;

    /// Per-overseer demand data
    std::array<ZoneDemandData, MAX_OVERSEERS> m_demand;

    /// Internal zone component data (parallel to grid, indexed by entity_id)
    /// For simplicity, we store zone components keyed by grid position
    struct ZoneInfo {
        ZoneComponent component;
        std::uint8_t player_id;
        bool valid;

        ZoneInfo() : component(), player_id(0), valid(false) {}
    };

    /// Zone info storage: one per grid cell (same layout as ZoneGrid)
    std::vector<ZoneInfo> m_zone_info;

    /// Grid width for indexing into m_zone_info
    std::uint16_t m_grid_width;

    /// Get zone info at position (nullptr if no zone)
    const ZoneInfo* get_zone_info(std::int32_t x, std::int32_t y) const;
    ZoneInfo* get_zone_info_mut(std::int32_t x, std::int32_t y);

    /// Pending state change events (Ticket 4-015)
    std::vector<ZoneStateChangedEvent> m_pending_state_events;

    /// Demand configuration (Ticket 4-016)
    DemandConfig m_demand_config;

    /// Desirability configuration (Ticket 4-018)
    DesirabilityConfig m_desirability_config;

    /// Tick counter for desirability update interval (Ticket 4-018)
    uint32_t m_tick_counter = 0;

    /// Update demand values for all overseers (called from tick)
    void update_demand();

    /// Update desirability for all valid zones (Ticket 4-018)
    void update_all_desirability();

    /// Calculate desirability score for a position (Ticket 4-018)
    uint8_t calculate_desirability(std::int32_t x, std::int32_t y) const;

    /// Check if a state transition is valid
    static bool is_valid_transition(ZoneState from, ZoneState to);

    // =========================================================================
    // Ticket 4-012: Zone Placement Execution private members
    // =========================================================================

    /// Auto-incrementing entity ID counter for zone placement
    std::uint32_t m_next_entity_id = 1;

    /// Placement cost configuration
    PlacementCostConfig m_placement_cost_config;

    /// Pending zone designated events
    std::vector<ZoneDesignatedEvent> m_pending_designated_events;

    // =========================================================================
    // Ticket 4-013: De-zoning private members
    // =========================================================================

    /// Pending zone undesignated events
    std::vector<ZoneUndesignatedEvent> m_pending_undesignated_events;

    /// Pending demolition request events
    std::vector<DemolitionRequestEvent> m_pending_demolition_events;

    /// Remove zone at position (internal helper)
    void remove_zone_at(std::int32_t x, std::int32_t y);

    // =========================================================================
    // Ticket 4-017: External demand provider
    // =========================================================================

    /// External demand provider (nullptr = use internal calculation)
    building::IDemandProvider* m_external_demand_provider = nullptr;
};

} // namespace zone
} // namespace sims3000

#endif // SIMS3000_ZONE_ZONESYSTEM_H
