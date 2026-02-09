/**
 * @file test_revenue_tracking.cpp
 * @brief Unit tests for RevenueTracking (E11-008)
 */

#include "sims3000/economy/RevenueTracking.h"
#include <cassert>
#include <cstdio>

using namespace sims3000::economy;

// ============================================================================
// build_income_breakdown Tests
// ============================================================================

void test_build_income_breakdown_basic() {
    printf("Testing build_income_breakdown basic...\n");

    AggregateTributeResult tribute;
    tribute.habitation_total = 1000;
    tribute.exchange_total = 2000;
    tribute.fabrication_total = 3000;
    tribute.grand_total = 6000;

    auto breakdown = build_income_breakdown(tribute);

    assert(breakdown.habitation_tribute == 1000);
    assert(breakdown.exchange_tribute == 2000);
    assert(breakdown.fabrication_tribute == 3000);
    assert(breakdown.other_income == 0);
    assert(breakdown.total == 6000);

    printf("  PASS: Basic breakdown maps correctly\n");
}

void test_build_income_breakdown_with_other_income() {
    printf("Testing build_income_breakdown with other_income...\n");

    AggregateTributeResult tribute;
    tribute.habitation_total = 500;
    tribute.exchange_total = 700;
    tribute.fabrication_total = 300;
    tribute.grand_total = 1500;

    auto breakdown = build_income_breakdown(tribute, 200);

    assert(breakdown.habitation_tribute == 500);
    assert(breakdown.exchange_tribute == 700);
    assert(breakdown.fabrication_tribute == 300);
    assert(breakdown.other_income == 200);
    assert(breakdown.total == 1700);

    printf("  PASS: Other income added to total\n");
}

void test_build_income_breakdown_zero_tribute() {
    printf("Testing build_income_breakdown with zero tribute...\n");

    AggregateTributeResult tribute;

    auto breakdown = build_income_breakdown(tribute);

    assert(breakdown.habitation_tribute == 0);
    assert(breakdown.exchange_tribute == 0);
    assert(breakdown.fabrication_tribute == 0);
    assert(breakdown.other_income == 0);
    assert(breakdown.total == 0);

    printf("  PASS: Zero tribute returns zero breakdown\n");
}

void test_build_income_breakdown_only_other_income() {
    printf("Testing build_income_breakdown with only other income...\n");

    AggregateTributeResult tribute;
    auto breakdown = build_income_breakdown(tribute, 5000);

    assert(breakdown.total == 5000);
    assert(breakdown.habitation_tribute == 0);
    assert(breakdown.other_income == 5000);

    printf("  PASS: Other income only\n");
}

// ============================================================================
// IncomeHistory Tests
// ============================================================================

void test_income_history_initial_state() {
    printf("Testing IncomeHistory initial state...\n");

    IncomeHistory history;

    assert(history.current_index == 0);
    assert(history.count == 0);
    assert(history.get_average() == 0);
    assert(history.get_trend() == 0);

    printf("  PASS: Initial state is zero\n");
}

void test_income_history_record_single() {
    printf("Testing IncomeHistory record single...\n");

    IncomeHistory history;
    history.record(1000);

    assert(history.count == 1);
    assert(history.get_average() == 1000);
    assert(history.get_trend() == 0); // need at least 2 entries

    printf("  PASS: Single record works\n");
}

void test_income_history_record_multiple() {
    printf("Testing IncomeHistory record multiple...\n");

    IncomeHistory history;
    history.record(100);
    history.record(200);
    history.record(300);

    assert(history.count == 3);
    assert(history.get_average() == 200); // (100+200+300)/3 = 200

    printf("  PASS: Multiple records averaged correctly\n");
}

void test_income_history_circular_buffer_wrap() {
    printf("Testing IncomeHistory circular buffer wrap...\n");

    IncomeHistory history;
    // Fill all 12 slots
    for (int i = 1; i <= 12; ++i) {
        history.record(i * 100);
    }

    assert(history.count == 12);
    assert(history.current_index == 0); // wrapped around

    // Now record one more -- should overwrite slot 0
    history.record(9999);
    assert(history.count == 12);
    assert(history.current_index == 1);
    assert(history.phases[0] == 9999);

    printf("  PASS: Circular buffer wraps correctly\n");
}

void test_income_history_average_full_buffer() {
    printf("Testing IncomeHistory average with full buffer...\n");

    IncomeHistory history;
    for (int i = 0; i < 12; ++i) {
        history.record(100);
    }

    assert(history.get_average() == 100);

    printf("  PASS: Full buffer average correct\n");
}

void test_income_history_trend_growing() {
    printf("Testing IncomeHistory trend growing...\n");

    IncomeHistory history;
    // Record older values (lower) then recent values (higher)
    history.record(100);
    history.record(100);
    history.record(200);
    history.record(200);

    // count=4, half=2: recent=[200,200] avg=200, older=[100,100] avg=100
    int64_t trend = history.get_trend();
    assert(trend > 0);

    printf("  PASS: Growing trend is positive (%lld)\n", (long long)trend);
}

void test_income_history_trend_shrinking() {
    printf("Testing IncomeHistory trend shrinking...\n");

    IncomeHistory history;
    history.record(500);
    history.record(500);
    history.record(100);
    history.record(100);

    int64_t trend = history.get_trend();
    assert(trend < 0);

    printf("  PASS: Shrinking trend is negative (%lld)\n", (long long)trend);
}

void test_income_history_trend_flat() {
    printf("Testing IncomeHistory trend flat...\n");

    IncomeHistory history;
    history.record(300);
    history.record(300);
    history.record(300);
    history.record(300);

    int64_t trend = history.get_trend();
    assert(trend == 0);

    printf("  PASS: Flat trend is zero\n");
}

// ============================================================================
// apply_income_to_treasury Tests
// ============================================================================

void test_apply_income_to_treasury() {
    printf("Testing apply_income_to_treasury...\n");

    TreasuryState treasury;
    IncomeBreakdown income;
    income.habitation_tribute = 1000;
    income.exchange_tribute = 2000;
    income.fabrication_tribute = 3000;
    income.other_income = 500;
    income.total = 6500;

    apply_income_to_treasury(treasury, income);

    assert(treasury.habitation_tribute == 1000);
    assert(treasury.exchange_tribute == 2000);
    assert(treasury.fabrication_tribute == 3000);
    assert(treasury.other_income == 500);
    assert(treasury.last_income == 6500);

    printf("  PASS: Treasury income fields updated correctly\n");
}

void test_apply_income_does_not_change_balance() {
    printf("Testing apply_income does not change balance...\n");

    TreasuryState treasury;
    int64_t original_balance = treasury.balance;

    IncomeBreakdown income;
    income.total = 5000;
    income.habitation_tribute = 5000;

    apply_income_to_treasury(treasury, income);

    assert(treasury.balance == original_balance);

    printf("  PASS: Balance unchanged after apply_income\n");
}

void test_apply_income_overwrites_previous() {
    printf("Testing apply_income overwrites previous values...\n");

    TreasuryState treasury;
    treasury.habitation_tribute = 9999;
    treasury.last_income = 9999;

    IncomeBreakdown income;
    income.habitation_tribute = 100;
    income.exchange_tribute = 0;
    income.fabrication_tribute = 0;
    income.other_income = 0;
    income.total = 100;

    apply_income_to_treasury(treasury, income);

    assert(treasury.habitation_tribute == 100);
    assert(treasury.last_income == 100);

    printf("  PASS: Previous values overwritten\n");
}

void test_income_breakdown_default_values() {
    printf("Testing IncomeBreakdown default values...\n");

    IncomeBreakdown breakdown;

    assert(breakdown.habitation_tribute == 0);
    assert(breakdown.exchange_tribute == 0);
    assert(breakdown.fabrication_tribute == 0);
    assert(breakdown.other_income == 0);
    assert(breakdown.total == 0);

    printf("  PASS: All defaults are zero\n");
}

// ============================================================================
// Main
// ============================================================================

int main() {
    printf("=== Revenue Tracking Unit Tests (E11-008) ===\n\n");

    // build_income_breakdown tests
    test_build_income_breakdown_basic();
    test_build_income_breakdown_with_other_income();
    test_build_income_breakdown_zero_tribute();
    test_build_income_breakdown_only_other_income();

    // IncomeHistory tests
    test_income_history_initial_state();
    test_income_history_record_single();
    test_income_history_record_multiple();
    test_income_history_circular_buffer_wrap();
    test_income_history_average_full_buffer();
    test_income_history_trend_growing();
    test_income_history_trend_shrinking();
    test_income_history_trend_flat();

    // apply_income_to_treasury tests
    test_apply_income_to_treasury();
    test_apply_income_does_not_change_balance();
    test_apply_income_overwrites_previous();

    // Struct defaults
    test_income_breakdown_default_values();

    printf("\n=== All tests passed! (16 tests) ===\n");
    return 0;
}
