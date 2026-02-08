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
 * Five zone types: three base types plus two port zone types (Epic 8).
 * - Habitation: Residential zones for colony inhabitants
 * - Exchange: Commercial zones for trade and commerce
 * - Fabrication: Industrial zones for production and manufacturing
 * - AeroPort: Air transport port zones (canonical: aero_port) [Epic 8]
 * - AquaPort: Water transport port zones (canonical: aqua_port) [Epic 8]
 *
 * Note: Values 0-2 are base zone types, value 3 is reserved,
 * values 4-5 are port zone types. The gap at 3 is intentional.
 */
enum class ZoneType : std::uint8_t {
    Habitation = 0,   ///< Residential zone (canonical: habitation)
    Exchange = 1,     ///< Commercial zone (canonical: exchange)
    Fabrication = 2,  ///< Industrial zone (canonical: fabrication)
    // Value 3 is intentionally reserved
    AeroPort = 4,     ///< Air transport port zone (canonical: aero_port) [Epic 8]
    AquaPort = 5      ///< Water transport port zone (canonical: aqua_port) [Epic 8]
};

/// Total number of zone types (including port zones; note gap at value 3)
constexpr std::uint8_t ZONE_TYPE_COUNT = 6;

/// Number of base (non-port) zone types
constexpr std::uint8_t BASE_ZONE_TYPE_COUNT = 3;

/// Check if a ZoneType is a port zone type
constexpr bool is_port_zone_type(ZoneType type) {
    return type == ZoneType::AeroPort || type == ZoneType::AquaPort;
}

// =========================================================================
// Zone Overlay Color Constants (RGB, 0-255)
// =========================================================================
// Base zone colors defined in /docs/zone-color-tokens.yaml
// Port zone colors added for Epic 8 (E8-031)

/// Overlay color for Habitation zones: teal-cyan (#00aaaa)
constexpr std::uint8_t ZONE_COLOR_HABITATION_R = 0;
constexpr std::uint8_t ZONE_COLOR_HABITATION_G = 170;
constexpr std::uint8_t ZONE_COLOR_HABITATION_B = 170;

/// Overlay color for Exchange zones: amber/gold (#ffaa00)
constexpr std::uint8_t ZONE_COLOR_EXCHANGE_R = 255;
constexpr std::uint8_t ZONE_COLOR_EXCHANGE_G = 170;
constexpr std::uint8_t ZONE_COLOR_EXCHANGE_B = 0;

/// Overlay color for Fabrication zones: magenta (#ff00aa)
constexpr std::uint8_t ZONE_COLOR_FABRICATION_R = 255;
constexpr std::uint8_t ZONE_COLOR_FABRICATION_G = 0;
constexpr std::uint8_t ZONE_COLOR_FABRICATION_B = 170;

/// Overlay color for AeroPort zones: sky blue (#44aaff) [Epic 8]
constexpr std::uint8_t ZONE_COLOR_AEROPORT_R = 68;
constexpr std::uint8_t ZONE_COLOR_AEROPORT_G = 170;
constexpr std::uint8_t ZONE_COLOR_AEROPORT_B = 255;

/// Overlay color for AquaPort zones: deep ocean blue (#0066cc) [Epic 8]
constexpr std::uint8_t ZONE_COLOR_AQUAPORT_R = 0;
constexpr std::uint8_t ZONE_COLOR_AQUAPORT_G = 102;
constexpr std::uint8_t ZONE_COLOR_AQUAPORT_B = 204;

/// Standard overlay alpha for all zone types (0.15 = ~38/255)
constexpr std::uint8_t ZONE_OVERLAY_ALPHA = 38;

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
 * - zone_type: 1 byte (ZoneType enum, 0-2 base, 4-5 port)
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
    std::uint32_t aeroport_total;    ///< AeroPort zone count [Epic 8]
    std::uint32_t aquaport_total;    ///< AquaPort zone count [Epic 8]

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
        , aeroport_total(0)
        , aquaport_total(0)
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
