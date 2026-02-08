/**
 * @file TransportEnums.h
 * @brief Transport system enumerations for Epic 7 (Ticket E7-001)
 *
 * Defines:
 * - PathwayType enum: Transport pathway types (5 total)
 * - PathwayDirection enum: Pathway directional modes
 *
 * Uses canonical alien terminology per /docs/canon/terminology.yaml
 * (pathway, transit_corridor - not road/highway)
 */

#pragma once

#include <cstdint>

namespace sims3000 {
namespace transport {

/**
 * @enum PathwayType
 * @brief Transport pathway types.
 *
 * Defines the different classes of pathways that can be placed on the grid.
 * Uses alien terminology: "pathway" instead of "road", "transit corridor"
 * instead of "highway".
 */
enum class PathwayType : uint8_t {
    BasicPathway    = 0,   ///< Basic surface pathway (road equivalent)
    TransitCorridor = 1,   ///< High-capacity transit corridor (highway equivalent)
    Pedestrian      = 2,   ///< Pedestrian-only pathway (sidewalk/walkway)
    Bridge          = 3,   ///< Elevated bridge pathway (spans obstacles)
    Tunnel          = 4    ///< Underground tunnel pathway (passes under terrain)
};

/// Total number of pathway types
constexpr uint8_t PATHWAY_TYPE_COUNT = 5;

/**
 * @enum PathwayDirection
 * @brief Directional mode for a pathway segment.
 *
 * Controls traffic flow direction on a pathway. Bidirectional is the default.
 * One-way directions restrict flow to a single cardinal direction.
 */
enum class PathwayDirection : uint8_t {
    Bidirectional = 0,   ///< Traffic flows in both directions (default)
    OneWayNorth   = 1,   ///< Traffic flows north only
    OneWaySouth   = 2,   ///< Traffic flows south only
    OneWayEast    = 3,   ///< Traffic flows east only
    OneWayWest    = 4    ///< Traffic flows west only
};

/// Total number of pathway direction modes
constexpr uint8_t PATHWAY_DIRECTION_COUNT = 5;

/**
 * @brief Convert PathwayType enum to a human-readable string.
 *
 * @param type The PathwayType to convert.
 * @return Null-terminated string name of the pathway type.
 */
inline const char* pathway_type_to_string(PathwayType type) {
    switch (type) {
        case PathwayType::BasicPathway:    return "BasicPathway";
        case PathwayType::TransitCorridor: return "TransitCorridor";
        case PathwayType::Pedestrian:      return "Pedestrian";
        case PathwayType::Bridge:          return "Bridge";
        case PathwayType::Tunnel:          return "Tunnel";
        default:                           return "Unknown";
    }
}

/**
 * @brief Convert PathwayDirection enum to a human-readable string.
 *
 * @param dir The PathwayDirection to convert.
 * @return Null-terminated string name of the pathway direction.
 */
inline const char* pathway_direction_to_string(PathwayDirection dir) {
    switch (dir) {
        case PathwayDirection::Bidirectional: return "Bidirectional";
        case PathwayDirection::OneWayNorth:   return "OneWayNorth";
        case PathwayDirection::OneWaySouth:   return "OneWaySouth";
        case PathwayDirection::OneWayEast:    return "OneWayEast";
        case PathwayDirection::OneWayWest:    return "OneWayWest";
        default:                             return "Unknown";
    }
}

/**
 * @brief Check if a PathwayDirection is a one-way direction.
 *
 * @param dir The PathwayDirection to check.
 * @return true if the direction is one-way (not bidirectional).
 */
inline bool is_one_way(PathwayDirection dir) {
    return dir != PathwayDirection::Bidirectional;
}

} // namespace transport
} // namespace sims3000
