/**
 * @file RevenueTracking.h
 * @brief Revenue breakdown and income history tracking (Ticket E11-008)
 *
 * Provides IncomeBreakdown for categorized income tracking,
 * IncomeHistory for circular-buffer phase history (last 12 phases),
 * and functions to build breakdowns from tribute aggregates and
 * apply them to TreasuryState.
 */

#ifndef SIMS3000_ECONOMY_REVENUETRACKING_H
#define SIMS3000_ECONOMY_REVENUETRACKING_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <sims3000/economy/TributeCalculation.h>
#include <sims3000/economy/TreasuryState.h>

namespace sims3000 {
namespace economy {

// ============================================================================
// Income Breakdown
// ============================================================================

/**
 * @struct IncomeBreakdown
 * @brief Categorized income totals for a single budget phase.
 */
struct IncomeBreakdown {
    int64_t habitation_tribute = 0;   ///< Tribute from habitation zones
    int64_t exchange_tribute = 0;     ///< Tribute from exchange zones
    int64_t fabrication_tribute = 0;  ///< Tribute from fabrication zones
    int64_t other_income = 0;         ///< Miscellaneous income
    int64_t total = 0;                ///< Sum of all income categories
};

/**
 * @brief Build an IncomeBreakdown from an AggregateTributeResult.
 *
 * Maps tribute zone totals to income categories and computes the total.
 *
 * @param tribute Aggregate tribute results from TributeCalculation.
 * @param other_income Additional income from non-tribute sources.
 * @return IncomeBreakdown with categorized totals.
 */
IncomeBreakdown build_income_breakdown(const AggregateTributeResult& tribute,
                                       int64_t other_income = 0);

// ============================================================================
// Income History (Circular Buffer)
// ============================================================================

/**
 * @struct IncomeHistory
 * @brief Tracks the last 12 phases of total income for trend analysis.
 *
 * Uses a circular buffer. Phases that have not yet been recorded
 * contain zero values.
 */
struct IncomeHistory {
    static constexpr size_t HISTORY_SIZE = 12;

    std::array<int64_t, HISTORY_SIZE> phases{};  ///< Circular buffer of income values
    size_t current_index = 0;                     ///< Next write position
    size_t count = 0;                             ///< Number of recorded entries (max HISTORY_SIZE)

    /**
     * @brief Record a new income value, advancing the circular buffer.
     * @param income Total income for the completed phase.
     */
    void record(int64_t income);

    /**
     * @brief Get the average income across all recorded phases.
     * @return Average income, or 0 if no phases recorded.
     */
    int64_t get_average() const;

    /**
     * @brief Get the income trend (positive = growing, negative = shrinking).
     *
     * Compares the average of the most recent half of recorded entries
     * against the older half. Returns 0 if fewer than 2 entries.
     *
     * @return Trend value (recent_avg - older_avg).
     */
    int64_t get_trend() const;
};

// ============================================================================
// Treasury Application
// ============================================================================

/**
 * @brief Apply an IncomeBreakdown to TreasuryState income fields.
 *
 * Updates the per-category income fields and last_income total.
 * Does NOT modify balance (that is done by the budget cycle).
 *
 * @param treasury The treasury state to update.
 * @param income The income breakdown to apply.
 */
void apply_income_to_treasury(TreasuryState& treasury, const IncomeBreakdown& income);

} // namespace economy
} // namespace sims3000

#endif // SIMS3000_ECONOMY_REVENUETRACKING_H
