/**
 * @file FluidEnums.h
 * @brief Fluid system enumerations for Epic 6
 *
 * Defines:
 * - FluidPoolState enum: Fluid pool health states
 * - FluidProducerType enum: Fluid producer facility types
 * - Helper functions: fluid_pool_state_to_string(), fluid_producer_type_to_string()
 * - INVALID_ENTITY_ID constant (UINT32_MAX)
 * - MAX_PLAYERS constant (4)
 *
 * Uses canonical alien terminology per /docs/canon/terminology.yaml
 * (fluid not water, extractor not pump)
 */

#pragma once

#include <cstdint>

namespace sims3000 {
namespace fluid {

/**
 * @enum FluidPoolState
 * @brief Health state of the fluid pool.
 *
 * Represents overall fluid supply health:
 * - Healthy: Supply meets or exceeds demand
 * - Marginal: Supply is close to demand threshold
 * - Deficit: Demand exceeds supply
 * - Collapse: Critical failure, widespread outages
 */
enum class FluidPoolState : uint8_t {
    Healthy  = 0,   ///< Supply meets or exceeds demand
    Marginal = 1,   ///< Supply is near demand threshold
    Deficit  = 2,   ///< Demand exceeds supply
    Collapse = 3    ///< Critical failure, widespread outages
};

/**
 * @enum FluidProducerType
 * @brief Fluid producer facility types.
 *
 * - Extractor: Extracts fluid from underground sources
 * - Reservoir: Stores and distributes collected fluid
 */
enum class FluidProducerType : uint8_t {
    Extractor  = 0,   ///< Fluid extractor (pump equivalent)
    Reservoir  = 1    ///< Fluid reservoir (storage/distribution)
};

/// Total number of fluid producer types
constexpr uint8_t FLUID_PRODUCER_TYPE_COUNT = 2;

/// Maximum number of players
constexpr uint8_t MAX_PLAYERS = 4;

/// Invalid entity ID sentinel value
constexpr uint32_t INVALID_ENTITY_ID = UINT32_MAX;

/**
 * @brief Convert FluidPoolState enum to a human-readable string.
 *
 * @param state The FluidPoolState to convert.
 * @return Null-terminated string name of the pool state.
 */
inline const char* fluid_pool_state_to_string(FluidPoolState state) {
    switch (state) {
        case FluidPoolState::Healthy:  return "Healthy";
        case FluidPoolState::Marginal: return "Marginal";
        case FluidPoolState::Deficit:  return "Deficit";
        case FluidPoolState::Collapse: return "Collapse";
        default:                       return "Unknown";
    }
}

/**
 * @brief Convert FluidProducerType enum to a human-readable string.
 *
 * @param type The FluidProducerType to convert.
 * @return Null-terminated string name of the producer type.
 */
inline const char* fluid_producer_type_to_string(FluidProducerType type) {
    switch (type) {
        case FluidProducerType::Extractor: return "Extractor";
        case FluidProducerType::Reservoir: return "Reservoir";
        default:                           return "Unknown";
    }
}

} // namespace fluid
} // namespace sims3000
