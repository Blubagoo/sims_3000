/**
 * @file ContaminationType.h
 * @brief Contamination type enumeration and utilities (Epic 10)
 *
 * Defines the four contamination source types and provides string conversion
 * for debug/UI display.
 *
 * @see E10-071
 */

#ifndef SIMS3000_CONTAMINATION_CONTAMINATIONTYPE_H
#define SIMS3000_CONTAMINATION_CONTAMINATIONTYPE_H

#include <cstdint>

namespace sims3000 {
namespace contamination {

/**
 * @enum ContaminationType
 * @brief Types of environmental contamination sources.
 *
 * Each contamination source has a type that determines how it spreads,
 * what mitigates it, and how it affects land value and health.
 */
enum class ContaminationType : uint8_t {
    Industrial = 0,  ///< Factory/fabrication zone emissions
    Traffic = 1,     ///< Road traffic exhaust/noise
    Energy = 2,      ///< Power plant emissions
    Terrain = 3      ///< Natural terrain contamination (swamps, etc.)
};

/// Total number of contamination types
constexpr uint8_t CONTAMINATION_TYPE_COUNT = 4;

/**
 * @brief Convert a ContaminationType to its string representation.
 * @param type The contamination type to convert.
 * @return Null-terminated string name of the type, or "Unknown" for invalid values.
 */
inline const char* contamination_type_to_string(ContaminationType type) {
    switch (type) {
        case ContaminationType::Industrial: return "Industrial";
        case ContaminationType::Traffic:    return "Traffic";
        case ContaminationType::Energy:     return "Energy";
        case ContaminationType::Terrain:    return "Terrain";
        default:                            return "Unknown";
    }
}

} // namespace contamination
} // namespace sims3000

#endif // SIMS3000_CONTAMINATION_CONTAMINATIONTYPE_H
