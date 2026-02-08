/**
 * @file ZoneTypes.h
 * @brief Zone type enumerations and ZoneComponent structure for Epic 4
 *
 * Defines the canonical zone data types:
 * - ZoneType enum: Habitation, Exchange, Fabrication (3 zone types)
 * - ZoneDensity enum: LowDensity, HighDensity (2 density levels)
 * - ZoneState enum: Designated, Occupied, Stalled (3 states)
 * - ZoneComponent struct: exactly 4 bytes per zone
 *
 * Supporting structs for zone operations:
 * - ZoneDemandData: RCI demand values per zone type
 * - ZoneCounts: per-overseer zone statistics
 * - ZonePlacementRequest/Result: designation operations
 * - DezoneResult: undesignation operations
 *
 * Uses canonical alien terminology per /docs/canon/terminology.yaml
 */

#ifndef SIMS3000_ZONE_ZONETYPES_H
#define SIMS3000_ZONE_ZONETYPES_H

#include <cstdint>
#include <type_traits>

namespace sims3000 {
namespace zone {

/**
 * @enum ZoneType
 * @brief Canonical alien zone types.
 *
 * Three zone types provide strategic depth without analysis paralysis:
 * - Habitation: Residential zones for colony inhabitants
 * - Exchange: Commercial zones for trade and commerce
 * - Fabrication: Industrial zones for production and manufacturing
 */
enum class ZoneType : std::uint8_t {
    Habitation = 0,   ///< Residential zone (canonical: habitation)
    Exchange = 1,     ///< Commercial zone (canonical: exchange)
    Fabrication = 2   ///< Industrial zone (canonical: fabrication)
};

/// Total number of zone types
constexpr std::uint8_t ZONE_TYPE_COUNT = 3;

/**
 * @enum ZoneDensity
 * @brief Zone density levels.
 *
 * Density is player-chosen at designation time. Low density zones spawn
 * smaller structures, high density zones spawn larger structures.
 */
enum class ZoneDensity : std::uint8_t {
    LowDensity = 0,   ///< Low density (smaller structures)
    HighDensity = 1   ///< High density (larger structures)
};

/// Total number of density levels
constexpr std::uint8_t ZONE_DENSITY_COUNT = 2;

/**
 * @enum ZoneState
 * @brief Zone lifecycle states.
 *
 * - Designated: Zone placed, awaiting structure development
 * - Occupied: Structure has been built in this zone
 * - Stalled: Zone cannot develop (no pathway, no demand, etc.)
 */
enum class ZoneState : std::uint8_t {
    Designated = 0,  ///< Awaiting structure development
    Occupied = 1,    ///< Structure built
    Stalled = 2      ///< Cannot develop (blocked)
};

/// Total number of zone states
constexpr std::uint8_t ZONE_STATE_COUNT = 3;

/**
 * @struct ZoneComponent
 * @brief Atomic unit of zone data at exactly 4 bytes per zone.
 *
 * Compact component for ECS efficiency. Zone entities also have
 * PositionComponent and OwnershipComponent for complete context.
 *
 * Layout:
 * - zone_type: 1 byte (ZoneType enum, 0-2)
 * - density: 1 byte (ZoneDensity enum, 0-1)
 * - state: 1 byte (ZoneState enum, 0-2)
 * - desirability: 1 byte (0-255 cached attractiveness score)
 *
 * Total: 4 bytes per zone, allowing efficient ECS packing.
 */
struct ZoneComponent {
    std::uint8_t zone_type;     ///< ZoneType value (0-2)
    std::uint8_t density;       ///< ZoneDensity value (0-1)
    std::uint8_t state;         ///< ZoneState value (0-2)
    std::uint8_t desirability;  ///< Cached attractiveness (0-255)

    /**
     * @brief Get the zone type as the enum value.
     * @return ZoneType enum value.
     */
    ZoneType getZoneType() const {
        return static_cast<ZoneType>(zone_type);
    }

    /**
     * @brief Set the zone type.
     * @param type ZoneType to set.
     */
    void setZoneType(ZoneType type) {
        zone_type = static_cast<std::uint8_t>(type);
    }

    /**
     * @brief Get the density as the enum value.
     * @return ZoneDensity enum value.
     */
    ZoneDensity getDensity() const {
        return static_cast<ZoneDensity>(density);
    }

    /**
     * @brief Set the density.
     * @param d ZoneDensity to set.
     */
    void setDensity(ZoneDensity d) {
        density = static_cast<std::uint8_t>(d);
    }

    /**
     * @brief Get the state as the enum value.
     * @return ZoneState enum value.
     */
    ZoneState getState() const {
        return static_cast<ZoneState>(state);
    }

    /**
     * @brief Set the state.
     * @param s ZoneState to set.
     */
    void setState(ZoneState s) {
        state = static_cast<std::uint8_t>(s);
    }
};

// Verify ZoneComponent is exactly 4 bytes as required
static_assert(sizeof(ZoneComponent) == 4,
    "ZoneComponent must be exactly 4 bytes");

// Verify ZoneComponent is trivially copyable for network serialization
static_assert(std::is_trivially_copyable<ZoneComponent>::value,
    "ZoneComponent must be trivially copyable");

/**
 * @struct ZoneDemandData
 * @brief RCI (Habitation/Exchange/Fabrication) demand values.
 *
 * Demand ranges from -100 (negative demand) to +100 (positive demand).
 * Zero indicates neutral demand. Demand drives zone development.
 */
struct ZoneDemandData {
    std::int8_t habitation_demand;   ///< Demand for habitation zones (-100 to +100)
    std::int8_t exchange_demand;     ///< Demand for exchange zones (-100 to +100)
    std::int8_t fabrication_demand;  ///< Demand for fabrication zones (-100 to +100)

    /// Default constructor initializes all demands to zero (neutral)
    ZoneDemandData()
        : habitation_demand(0)
        , exchange_demand(0)
        , fabrication_demand(0)
    {}
};

/**
 * @struct ZoneCounts
 * @brief Per-overseer zone statistics.
 *
 * Tracks zone counts by type, density, and state for aggregate queries.
 */
struct ZoneCounts {
    // By type (all densities, all states)
    std::uint32_t habitation_total;
    std::uint32_t exchange_total;
    std::uint32_t fabrication_total;

    // By density (all types, all states)
    std::uint32_t low_density_total;
    std::uint32_t high_density_total;

    // By state (all types, all densities)
    std::uint32_t designated_total;
    std::uint32_t occupied_total;
    std::uint32_t stalled_total;

    // Total zones
    std::uint32_t total;

    /// Default constructor initializes all counts to zero
    ZoneCounts()
        : habitation_total(0)
        , exchange_total(0)
        , fabrication_total(0)
        , low_density_total(0)
        , high_density_total(0)
        , designated_total(0)
        , occupied_total(0)
        , stalled_total(0)
        , total(0)
    {}
};

/**
 * @struct ZonePlacementRequest
 * @brief Request to designate zones in a rectangular area.
 *
 * Used by ZoneSystem.place_zones() for server-authoritative zone designation.
 */
struct ZonePlacementRequest {
    std::int32_t x;           ///< Top-left X coordinate
    std::int32_t y;           ///< Top-left Y coordinate
    std::int32_t width;       ///< Width in tiles (1 for single tile)
    std::int32_t height;      ///< Height in tiles (1 for single tile)
    ZoneType zone_type;       ///< Type of zone to place
    ZoneDensity density;      ///< Density level
    std::uint8_t player_id;   ///< Requesting overseer (PlayerID)
};

/**
 * @struct ZonePlacementResult
 * @brief Result of a zone placement operation.
 *
 * Reports success/failure counts for multi-tile operations.
 */
struct ZonePlacementResult {
    std::uint32_t placed_count;   ///< Number of zones successfully placed
    std::uint32_t skipped_count;  ///< Number of tiles skipped (validation failed)
    std::uint32_t total_cost;     ///< Total credit cost of placed zones
    bool any_placed;              ///< True if at least one zone was placed

    /// Default constructor initializes to failure state
    ZonePlacementResult()
        : placed_count(0)
        , skipped_count(0)
        , total_cost(0)
        , any_placed(false)
    {}
};

/**
 * @struct DezoneResult
 * @brief Result of a zone undesignation operation.
 *
 * Reports success/failure counts for multi-tile de-zoning.
 */
struct DezoneResult {
    std::uint32_t removed_count;              ///< Number of zones successfully removed
    std::uint32_t skipped_count;              ///< Number of tiles skipped (no zone or invalid)
    std::uint32_t demolition_requested_count; ///< Number of occupied zones flagged for demolition
    bool any_removed;                         ///< True if at least one zone was removed

    /// Default constructor initializes to failure state
    DezoneResult()
        : removed_count(0)
        , skipped_count(0)
        , demolition_requested_count(0)
        , any_removed(false)
    {}
};

} // namespace zone
} // namespace sims3000

#endif // SIMS3000_ZONE_ZONETYPES_H
