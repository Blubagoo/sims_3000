/**
 * @file BuildingTypes.h
 * @brief Building state and type enumerations for Epic 4
 *
 * Defines the canonical building data types:
 * - BuildingState enum: 5-state lifecycle (Materializing, Active, Abandoned, Derelict, Deconstructed)
 * - ZoneBuildingType enum: Habitation, Exchange, Fabrication (matches zone types)
 * - DensityLevel enum: Low, High (matches zone densities)
 * - ConstructionPhase enum: 4 phases (Foundation, Framework, Exterior, Finalization)
 *
 * Uses canonical alien terminology per /docs/canon/terminology.yaml
 */

#ifndef SIMS3000_BUILDING_BUILDINGTYPES_H
#define SIMS3000_BUILDING_BUILDINGTYPES_H

#include <cstdint>
#include <type_traits>

namespace sims3000 {
namespace building {

/**
 * @enum BuildingState
 * @brief Canonical 5-state building lifecycle.
 *
 * State machine:
 * - Materializing: Construction in progress (has ConstructionComponent)
 * - Active: Fully built and operational
 * - Abandoned: Player left/inactive, decay starting
 * - Derelict: Fully decayed, non-functional
 * - Deconstructed: Demolished, debris state (DebrisComponent)
 *
 * Transitions:
 * - Materializing -> Active (construction completes)
 * - Active -> Abandoned (player abandons/long inactivity)
 * - Active -> Deconstructed (player demolishes)
 * - Abandoned -> Derelict (abandon timer expires)
 * - Derelict -> Deconstructed (decay timer expires)
 * - Deconstructed -> [entity destroyed] (debris cleared)
 */
enum class BuildingState : std::uint8_t {
    Materializing = 0,  ///< Construction in progress
    Active = 1,         ///< Fully built and operational
    Abandoned = 2,      ///< Decay starting (player abandoned)
    Derelict = 3,       ///< Fully decayed, non-functional
    Deconstructed = 4   ///< Demolished, debris state
};

/// Total number of building states
constexpr std::uint8_t BUILDING_STATE_COUNT = 5;

/**
 * @enum ZoneBuildingType
 * @brief Building type matching zone types.
 *
 * Zone buildings match their zone type 1:1.
 * Values intentionally match ZoneType enum for easy conversion.
 */
enum class ZoneBuildingType : std::uint8_t {
    Habitation = 0,   ///< Habitation zone building (residential)
    Exchange = 1,     ///< Exchange zone building (commercial)
    Fabrication = 2   ///< Fabrication zone building (industrial)
};

/// Total number of zone building types
constexpr std::uint8_t ZONE_BUILDING_TYPE_COUNT = 3;

/**
 * @enum DensityLevel
 * @brief Building density level matching zone densities.
 *
 * Values intentionally match ZoneDensity enum for easy conversion.
 */
enum class DensityLevel : std::uint8_t {
    Low = 0,   ///< Low density building
    High = 1   ///< High density building
};

/// Total number of density levels
constexpr std::uint8_t DENSITY_LEVEL_COUNT = 2;

/**
 * @enum ConstructionPhase
 * @brief 4-phase construction progression.
 *
 * Maps to progress percentage:
 * - Foundation: 0-25% progress
 * - Framework: 25-50% progress
 * - Exterior: 50-75% progress
 * - Finalization: 75-100% progress
 *
 * Used by RenderingSystem for visual feedback during Materializing state.
 */
enum class ConstructionPhase : std::uint8_t {
    Foundation = 0,     ///< 0-25% progress (base construction)
    Framework = 1,      ///< 25-50% progress (structure frame)
    Exterior = 2,       ///< 50-75% progress (walls and exterior)
    Finalization = 3    ///< 75-100% progress (details and finishing)
};

/// Total number of construction phases
constexpr std::uint8_t CONSTRUCTION_PHASE_COUNT = 4;

/**
 * @brief Calculate construction phase from progress percentage.
 *
 * Maps progress (0-100) to ConstructionPhase enum:
 * - [0, 25) -> Foundation
 * - [25, 50) -> Framework
 * - [50, 75) -> Exterior
 * - [75, 100] -> Finalization
 *
 * @param progress_percent Progress percentage (0-100).
 * @return ConstructionPhase enum value.
 */
constexpr ConstructionPhase getPhaseFromProgress(std::uint8_t progress_percent) {
    if (progress_percent < 25) return ConstructionPhase::Foundation;
    if (progress_percent < 50) return ConstructionPhase::Framework;
    if (progress_percent < 75) return ConstructionPhase::Exterior;
    return ConstructionPhase::Finalization;
}

/**
 * @brief Calculate progress percentage from ticks elapsed.
 *
 * @param ticks_elapsed Current elapsed construction ticks.
 * @param ticks_total Total construction duration in ticks.
 * @return Progress percentage (0-100).
 */
constexpr std::uint8_t getProgressPercent(std::uint16_t ticks_elapsed, std::uint16_t ticks_total) {
    if (ticks_total == 0) return 100;
    std::uint32_t percent = (static_cast<std::uint32_t>(ticks_elapsed) * 100) / ticks_total;
    return percent > 100 ? 100 : static_cast<std::uint8_t>(percent);
}

/**
 * @brief Check if a BuildingState value is valid.
 * @param value The raw uint8 value to check.
 * @return true if value is a valid BuildingState (0-4).
 */
constexpr bool isValidBuildingState(std::uint8_t value) {
    return value < BUILDING_STATE_COUNT;
}

/**
 * @brief Check if a ZoneBuildingType value is valid.
 * @param value The raw uint8 value to check.
 * @return true if value is a valid ZoneBuildingType (0-2).
 */
constexpr bool isValidZoneBuildingType(std::uint8_t value) {
    return value < ZONE_BUILDING_TYPE_COUNT;
}

/**
 * @brief Check if a DensityLevel value is valid.
 * @param value The raw uint8 value to check.
 * @return true if value is a valid DensityLevel (0-1).
 */
constexpr bool isValidDensityLevel(std::uint8_t value) {
    return value < DENSITY_LEVEL_COUNT;
}

/**
 * @brief Check if a ConstructionPhase value is valid.
 * @param value The raw uint8 value to check.
 * @return true if value is a valid ConstructionPhase (0-3).
 */
constexpr bool isValidConstructionPhase(std::uint8_t value) {
    return value < CONSTRUCTION_PHASE_COUNT;
}

} // namespace building
} // namespace sims3000

#endif // SIMS3000_BUILDING_BUILDINGTYPES_H
