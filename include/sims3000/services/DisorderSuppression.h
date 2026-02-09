/**
 * @file DisorderSuppression.h
 * @brief Disorder suppression from enforcer coverage (Ticket E9-040)
 *
 * Provides the interface contract and calculation for disorder suppression
 * based on enforcer service coverage. This is a header-only module that
 * defines the contract for integration with the DisorderSystem (Epic 10).
 *
 * Suppression formula: disorder_generation *= (1 - coverage * 0.7)
 * - At 100% coverage: 70% reduction in disorder generation (multiplier 0.3)
 * - At 0% coverage: no reduction (multiplier 1.0)
 *
 * @see /docs/epics/epic-9/tickets.md (ticket E9-040)
 */

#pragma once

#include <algorithm>

namespace sims3000 {
namespace services {

// ============================================================================
// Constants
// ============================================================================

/// Maximum disorder suppression from enforcer coverage (70%)
constexpr float ENFORCER_SUPPRESSION_FACTOR = 0.7f;

// ============================================================================
// Disorder Suppression Calculation
// ============================================================================

/**
 * @brief Calculate disorder suppression multiplier from enforcer coverage.
 *
 * Returns a multiplier to apply to disorder generation rate.
 * - 0.0 coverage -> 1.0 (no suppression)
 * - 0.5 coverage -> 0.65 (35% suppression)
 * - 1.0 coverage -> 0.3 (70% suppression)
 *
 * Input is clamped to [0.0, 1.0].
 *
 * @param enforcer_coverage Coverage value 0.0-1.0 from IServiceQueryable
 * @return Multiplier for disorder generation (0.3 at full coverage, 1.0 at zero)
 */
inline float calculate_disorder_suppression(float enforcer_coverage) {
    float clamped = std::max(0.0f, std::min(1.0f, enforcer_coverage));
    return 1.0f - (clamped * ENFORCER_SUPPRESSION_FACTOR);
}

} // namespace services
} // namespace sims3000
