/**
 * @file LongevityBonus.h
 * @brief Longevity bonus from medical coverage (Ticket E9-041)
 *
 * Provides the interface contract and calculation for population longevity
 * based on medical service coverage. This is a header-only module that
 * defines the contract for integration with the PopulationSystem (Epic 10).
 *
 * Longevity formula: longevity = 60 + (coverage * 40)
 * - At 100% coverage: 100 cycles longevity
 * - At 0% coverage: 60 cycles longevity
 *
 * @see /docs/epics/epic-9/tickets.md (ticket E9-041)
 */

#pragma once

#include <algorithm>
#include <cstdint>

namespace sims3000 {
namespace services {

// ============================================================================
// Constants
// ============================================================================

/// Base longevity in simulation cycles (no medical coverage)
constexpr uint32_t MEDICAL_BASE_LONGEVITY = 60;

/// Maximum longevity bonus from medical coverage
constexpr uint32_t MEDICAL_MAX_LONGEVITY_BONUS = 40;

// ============================================================================
// Longevity Calculation
// ============================================================================

/**
 * @brief Calculate population longevity from medical coverage.
 *
 * Returns longevity in simulation cycles.
 * - 0.0 coverage -> 60 cycles (base only)
 * - 0.5 coverage -> 80 cycles
 * - 1.0 coverage -> 100 cycles (base + full bonus)
 *
 * Input is clamped to [0.0, 1.0].
 *
 * @param medical_coverage Coverage value 0.0-1.0 from IServiceQueryable
 * @return Longevity in simulation cycles (60-100)
 */
inline uint32_t calculate_longevity(float medical_coverage) {
    float clamped = std::max(0.0f, std::min(1.0f, medical_coverage));
    return MEDICAL_BASE_LONGEVITY + static_cast<uint32_t>(clamped * MEDICAL_MAX_LONGEVITY_BONUS);
}

} // namespace services
} // namespace sims3000
