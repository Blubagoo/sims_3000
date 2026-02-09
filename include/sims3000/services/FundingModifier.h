/**
 * @file FundingModifier.h
 * @brief Funding effectiveness modifier for city services (Ticket E9-024)
 *
 * Provides utility functions to scale service effectiveness based on
 * funding level. Since IEconomyQueryable doesn't exist yet, this is
 * a self-contained header-only module.
 *
 * Funding curve: linear, capped at 115% for 150% funding.
 * Formula: modifier = min(funding_pct / 100.0, 1.15)
 * - 0% funding = 0% effectiveness (building disabled)
 * - 50% funding = 50% effectiveness
 * - 100% funding = 100% effectiveness (default)
 * - 150% funding = 115% effectiveness (capped)
 * - 200% funding = 115% effectiveness (capped)
 *
 * @see /docs/epics/epic-9/tickets.md (ticket E9-024)
 */

#pragma once

#include <cstdint>

namespace sims3000 {
namespace services {

// ============================================================================
// Constants
// ============================================================================

/// Maximum funding modifier (115% effectiveness at 150%+ funding)
constexpr float MAX_FUNDING_MODIFIER = 1.15f;

/// Default funding percentage when no economy system exists
constexpr uint8_t DEFAULT_FUNDING_PERCENT = 100;

// ============================================================================
// Funding Calculation Functions
// ============================================================================

/**
 * @brief Calculate the funding modifier from a funding percentage.
 *
 * Linear curve capped at MAX_FUNDING_MODIFIER (1.15).
 * - 0% funding -> 0.0 modifier
 * - 50% funding -> 0.5 modifier
 * - 100% funding -> 1.0 modifier
 * - 150% funding -> 1.15 modifier (capped)
 * - 200% funding -> 1.15 modifier (capped)
 *
 * @param funding_percent Funding level as a percentage (0-255).
 * @return Funding modifier (0.0 to MAX_FUNDING_MODIFIER).
 */
inline float calculate_funding_modifier(uint8_t funding_percent) {
    float raw = static_cast<float>(funding_percent) / 100.0f;
    return raw < MAX_FUNDING_MODIFIER ? raw : MAX_FUNDING_MODIFIER;
}

/**
 * @brief Apply funding modifier to a base effectiveness value.
 *
 * Computes: result = base_effectiveness * calculate_funding_modifier(funding_percent)
 * Result is clamped to [0, 255] and returned as uint8_t.
 *
 * @param base_effectiveness Base effectiveness percentage (0-100 typical).
 * @param funding_percent Funding level as a percentage (0-255).
 * @return Funding-adjusted effectiveness (0-255).
 */
inline uint8_t apply_funding_to_effectiveness(uint8_t base_effectiveness, uint8_t funding_percent) {
    float modifier = calculate_funding_modifier(funding_percent);
    float result = static_cast<float>(base_effectiveness) * modifier;
    if (result < 0.0f) return 0;
    if (result > 255.0f) return 255;
    return static_cast<uint8_t>(result);
}

} // namespace services
} // namespace sims3000
