/**
 * @file EducationBonus.h
 * @brief Education bonus to land value (Ticket E9-042)
 *
 * Provides the interface contract and calculation for land value bonus
 * from education service coverage. This is a header-only module that
 * defines the contract for integration with the LandValueSystem (Epic 10).
 *
 * Land value formula: multiplier = 1.0 + (coverage * 0.10)
 * - At 100% coverage: +10% land value (multiplier 1.1)
 * - At 0% coverage: no bonus (multiplier 1.0)
 * - Bonus applies colony-wide (not per-tile for education)
 *
 * @see /docs/epics/epic-9/tickets.md (ticket E9-042)
 */

#pragma once

#include <algorithm>

namespace sims3000 {
namespace services {

// ============================================================================
// Constants
// ============================================================================

/// Maximum education bonus to land value (10%)
constexpr float EDUCATION_LAND_VALUE_BONUS = 0.10f;

// ============================================================================
// Education Land Value Calculation
// ============================================================================

/**
 * @brief Calculate land value multiplier from education coverage.
 *
 * Returns a multiplier to apply to land value.
 * - 0.0 coverage -> 1.0 (no bonus)
 * - 0.5 coverage -> 1.05 (+5%)
 * - 1.0 coverage -> 1.1 (+10%)
 *
 * Input is clamped to [0.0, 1.0].
 *
 * @param education_coverage Coverage value 0.0-1.0 from IServiceQueryable
 * @return Multiplier for land value (1.0 at zero coverage, 1.1 at full)
 */
inline float calculate_education_land_value_multiplier(float education_coverage) {
    float clamped = std::max(0.0f, std::min(1.0f, education_coverage));
    return 1.0f + (clamped * EDUCATION_LAND_VALUE_BONUS);
}

} // namespace services
} // namespace sims3000
