/**
 * @file RevenueTracking.cpp
 * @brief Implementation of revenue breakdown and income history (Ticket E11-008)
 */

#include "sims3000/economy/RevenueTracking.h"

namespace sims3000 {
namespace economy {

// ============================================================================
// build_income_breakdown
// ============================================================================

IncomeBreakdown build_income_breakdown(const AggregateTributeResult& tribute,
                                       int64_t other_income) {
    IncomeBreakdown breakdown;
    breakdown.habitation_tribute = tribute.habitation_total;
    breakdown.exchange_tribute = tribute.exchange_total;
    breakdown.fabrication_tribute = tribute.fabrication_total;
    breakdown.other_income = other_income;
    breakdown.total = tribute.habitation_total
                    + tribute.exchange_total
                    + tribute.fabrication_total
                    + other_income;
    return breakdown;
}

// ============================================================================
// IncomeHistory
// ============================================================================

void IncomeHistory::record(int64_t income) {
    phases[current_index] = income;
    current_index = (current_index + 1) % HISTORY_SIZE;
    if (count < HISTORY_SIZE) {
        ++count;
    }
}

int64_t IncomeHistory::get_average() const {
    if (count == 0) {
        return 0;
    }
    int64_t sum = 0;
    for (size_t i = 0; i < count; ++i) {
        sum += phases[i];
    }
    return sum / static_cast<int64_t>(count);
}

int64_t IncomeHistory::get_trend() const {
    if (count < 2) {
        return 0;
    }

    // Split recorded entries into older half and recent half.
    // Entries are in the circular buffer; the most recent entry
    // is at (current_index - 1 + HISTORY_SIZE) % HISTORY_SIZE.
    size_t half = count / 2;
    size_t older_count = count - half;  // older portion (may be larger by 1)

    // Walk backwards from the most recent entry
    int64_t recent_sum = 0;
    int64_t older_sum = 0;

    for (size_t i = 0; i < count; ++i) {
        // i=0 is the most recent, i=count-1 is the oldest
        size_t idx = (current_index + HISTORY_SIZE - 1 - i) % HISTORY_SIZE;
        if (i < half) {
            recent_sum += phases[idx];
        } else {
            older_sum += phases[idx];
        }
    }

    int64_t recent_avg = recent_sum / static_cast<int64_t>(half);
    int64_t older_avg = older_sum / static_cast<int64_t>(older_count);

    return recent_avg - older_avg;
}

// ============================================================================
// apply_income_to_treasury
// ============================================================================

void apply_income_to_treasury(TreasuryState& treasury, const IncomeBreakdown& income) {
    treasury.habitation_tribute = income.habitation_tribute;
    treasury.exchange_tribute = income.exchange_tribute;
    treasury.fabrication_tribute = income.fabrication_tribute;
    treasury.other_income = income.other_income;
    treasury.last_income = income.total;
}

} // namespace economy
} // namespace sims3000
