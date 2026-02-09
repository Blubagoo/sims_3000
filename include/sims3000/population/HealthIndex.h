/**
 * @file HealthIndex.h
 * @brief Health index calculation system (Ticket E10-029)
 *
 * Calculates the health index (0-100) based on:
 * - Medical coverage from IServiceQueryable
 * - Contamination level from environment
 * - Fluid availability and coverage
 *
 * The health index modifies population growth, death rates, and migration.
 */

#ifndef SIMS3000_POPULATION_HEALTHINDEX_H
#define SIMS3000_POPULATION_HEALTHINDEX_H

#include <sims3000/population/PopulationData.h>
#include <cstdint>

namespace sims3000 {
namespace population {

// ============================================================================
// Constants
// ============================================================================

/// Base health index before modifiers (neutral)
constexpr uint8_t BASE_HEALTH = 50;

/// Maximum positive/negative modifier from medical coverage
constexpr float MEDICAL_MODIFIER_MAX = 25.0f;  // +/- 25

/// Maximum negative modifier from contamination
constexpr float CONTAMINATION_MODIFIER_MAX = 30.0f;  // up to -30

/// Maximum positive/negative modifier from fluid availability
constexpr float FLUID_MODIFIER_MAX = 10.0f;  // +/- 10

// ============================================================================
// Input/Output Structures
// ============================================================================

/**
 * @struct HealthInput
 * @brief Input data for health index calculation.
 *
 * Typically sourced from:
 * - medical_coverage: IServiceQueryable for medical buildings
 * - contamination_level: ContaminationGrid average / 255.0
 * - has_fluid: FluidSystem::is_active()
 * - fluid_coverage: IServiceQueryable for fluid network
 */
struct HealthInput {
    float medical_coverage;      ///< Medical coverage (0.0 - 1.0)
    float contamination_level;   ///< Contamination severity (0.0 - 1.0)
    bool has_fluid;              ///< Does the city have any fluid supply?
    float fluid_coverage;        ///< Fluid network coverage (0.0 - 1.0)
};

/**
 * @struct HealthResult
 * @brief Result of health index calculation.
 *
 * Includes the final health index and breakdown of modifiers.
 * The modifiers can be displayed in the UI to show players what
 * affects health.
 */
struct HealthResult {
    uint8_t health_index;           ///< Final health index (0-100)
    float medical_modifier;         ///< Contribution from medical coverage
    float contamination_modifier;   ///< Contribution from contamination (negative)
    float fluid_modifier;           ///< Contribution from fluid availability
};

// ============================================================================
// Functions
// ============================================================================

/**
 * @brief Calculate health index from input factors.
 *
 * Algorithm:
 * - medical_mod = (coverage - 0.5) * 2 * MEDICAL_MODIFIER_MAX
 *   → Range: [-25, +25]
 *
 * - contam_mod = -contamination_level * CONTAMINATION_MODIFIER_MAX
 *   → Range: [-30, 0]
 *
 * - fluid_mod = has_fluid ? ((fluid_coverage - 0.5) * 2 * FLUID_MODIFIER_MAX) : -FLUID_MODIFIER_MAX
 *   → Range: [-10, +10] if has_fluid, else -10
 *
 * - health = clamp(BASE_HEALTH + medical + contam + fluid, 0, 100)
 *
 * @param input Health input factors.
 * @return HealthResult with calculated index and modifier breakdown.
 */
HealthResult calculate_health_index(const HealthInput& input);

/**
 * @brief Apply health index calculation to PopulationData.
 *
 * Convenience function that calls calculate_health_index() and
 * updates the PopulationData.health_index field.
 *
 * @param pop PopulationData to update.
 * @param input Health input factors.
 */
void apply_health_index(PopulationData& pop, const HealthInput& input);

} // namespace population
} // namespace sims3000

#endif // SIMS3000_POPULATION_HEALTHINDEX_H
