/**
 * @file EnergyEnums.h
 * @brief Energy system enumerations for Epic 5
 *
 * Defines:
 * - NexusType enum: Energy nexus facility types (10 total, 6 MVP)
 * - EnergyPoolState enum: Energy pool health states
 * - TerrainRequirement enum: Terrain requirements for nexus placement
 *
 * Uses canonical alien terminology per /docs/canon/terminology.yaml
 */

#pragma once

#include <cstdint>

namespace sims3000 {
namespace energy {

/**
 * @enum NexusType
 * @brief Energy nexus facility types.
 *
 * The first 6 types (Carbon through Hydro) are available in the MVP.
 * The remaining 4 (Geothermal through MicrowaveReceiver) are post-MVP.
 */
enum class NexusType : uint8_t {
    Carbon            = 0,   ///< Carbon-burning nexus (coal equivalent)
    Petrochemical     = 1,   ///< Petrochemical nexus (oil/gas equivalent)
    Gaseous           = 2,   ///< Gaseous nexus (natural gas equivalent)
    Nuclear           = 3,   ///< Nuclear nexus (fission)
    Wind              = 4,   ///< Wind nexus (renewable)
    Solar             = 5,   ///< Solar nexus (renewable)
    Hydro             = 6,   ///< Hydro nexus (renewable, requires water)
    Geothermal        = 7,   ///< Geothermal nexus (requires ember crust)
    Fusion            = 8,   ///< Fusion nexus (advanced)
    MicrowaveReceiver = 9    ///< Microwave receiver nexus (orbital energy)
};

/// Total number of nexus types
constexpr uint8_t NEXUS_TYPE_COUNT = 10;

/// Number of nexus types available in MVP (first 6)
constexpr uint8_t NEXUS_TYPE_MVP_COUNT = 6;

/**
 * @enum EnergyPoolState
 * @brief Health state of the energy pool / matrix.
 *
 * Represents overall energy supply health:
 * - Healthy: Supply meets or exceeds demand
 * - Marginal: Supply is close to demand threshold
 * - Deficit: Demand exceeds supply
 * - Collapse: Critical failure, widespread outages
 */
enum class EnergyPoolState : uint8_t {
    Healthy  = 0,   ///< Supply meets or exceeds demand
    Marginal = 1,   ///< Supply is near demand threshold
    Deficit  = 2,   ///< Demand exceeds supply
    Collapse = 3    ///< Critical failure, widespread outages
};

/**
 * @enum TerrainRequirement
 * @brief Terrain requirements for nexus placement.
 *
 * Some nexus types require specific terrain features:
 * - None: No terrain requirement (most nexus types)
 * - Water: Must be placed adjacent to water (Hydro)
 * - EmberCrust: Must be placed on volcanic/geothermal terrain (Geothermal)
 * - Ridges: Must be placed on elevated ridgelines (Wind, for optimal placement)
 */
enum class TerrainRequirement : uint8_t {
    None       = 0,   ///< No terrain requirement
    Water      = 1,   ///< Requires adjacent water body
    EmberCrust = 2,   ///< Requires volcanic/geothermal terrain
    Ridges     = 3    ///< Requires elevated ridgelines
};

/**
 * @brief Convert NexusType enum to a human-readable string.
 *
 * @param type The NexusType to convert.
 * @return Null-terminated string name of the nexus type.
 */
inline const char* nexus_type_to_string(NexusType type) {
    switch (type) {
        case NexusType::Carbon:            return "Carbon";
        case NexusType::Petrochemical:     return "Petrochemical";
        case NexusType::Gaseous:           return "Gaseous";
        case NexusType::Nuclear:           return "Nuclear";
        case NexusType::Wind:              return "Wind";
        case NexusType::Solar:             return "Solar";
        case NexusType::Hydro:             return "Hydro";
        case NexusType::Geothermal:        return "Geothermal";
        case NexusType::Fusion:            return "Fusion";
        case NexusType::MicrowaveReceiver: return "MicrowaveReceiver";
        default:                           return "Unknown";
    }
}

/**
 * @brief Check if a NexusType is available in the MVP.
 *
 * The first 6 nexus types (Carbon through Hydro) are MVP types.
 *
 * @param type The NexusType to check.
 * @return true if the nexus type is available in the MVP.
 */
inline bool is_mvp_nexus_type(NexusType type) {
    return static_cast<uint8_t>(type) < NEXUS_TYPE_MVP_COUNT;
}

} // namespace energy
} // namespace sims3000
