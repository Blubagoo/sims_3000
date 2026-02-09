/**
 * @file test_expense_tracking.cpp
 * @brief Unit tests for ExpenseTracking (E11-011)
 */

#include "sims3000/economy/ExpenseTracking.h"
#include <cassert>
#include <cstdio>

using namespace sims3000::economy;

// ============================================================================
// build_expense_breakdown Tests
// ============================================================================

void test_build_expense_breakdown_basic() {
    printf("Testing build_expense_breakdown basic...\n");

    InfrastructureMaintenanceResult infra = {100, 50, 30, 80, 260};
    ServiceMaintenanceSummary services = {100, 120, 300, 200, 720};

    auto breakdown = build_expense_breakdown(infra, services, 150, 500, 75);

    assert(breakdown.infrastructure_maintenance == 260);
    assert(breakdown.service_maintenance == 720);
    assert(breakdown.energy_maintenance == 150);
    assert(breakdown.bond_payments == 500);
    assert(breakdown.ordinance_costs == 75);
    assert(breakdown.total == 1705);

    printf("  PASS: Basic breakdown maps correctly (total=1705)\n");
}

void test_build_expense_breakdown_zero_all() {
    printf("Testing build_expense_breakdown all zeros...\n");

    InfrastructureMaintenanceResult infra = {0, 0, 0, 0, 0};
    ServiceMaintenanceSummary services = {0, 0, 0, 0, 0};

    auto breakdown = build_expense_breakdown(infra, services, 0, 0, 0);

    assert(breakdown.infrastructure_maintenance == 0);
    assert(breakdown.service_maintenance == 0);
    assert(breakdown.energy_maintenance == 0);
    assert(breakdown.bond_payments == 0);
    assert(breakdown.ordinance_costs == 0);
    assert(breakdown.total == 0);

    printf("  PASS: All zero inputs produce zero breakdown\n");
}

void test_build_expense_breakdown_infra_only() {
    printf("Testing build_expense_breakdown infrastructure only...\n");

    InfrastructureMaintenanceResult infra = {50, 20, 30, 0, 100};
    ServiceMaintenanceSummary services = {0, 0, 0, 0, 0};

    auto breakdown = build_expense_breakdown(infra, services, 0, 0, 0);

    assert(breakdown.infrastructure_maintenance == 100);
    assert(breakdown.total == 100);

    printf("  PASS: Infrastructure only\n");
}

void test_build_expense_breakdown_services_only() {
    printf("Testing build_expense_breakdown services only...\n");

    InfrastructureMaintenanceResult infra = {0, 0, 0, 0, 0};
    ServiceMaintenanceSummary services = {100, 120, 300, 200, 720};

    auto breakdown = build_expense_breakdown(infra, services, 0, 0, 0);

    assert(breakdown.service_maintenance == 720);
    assert(breakdown.total == 720);

    printf("  PASS: Services only\n");
}

void test_build_expense_breakdown_bonds_only() {
    printf("Testing build_expense_breakdown bonds only...\n");

    InfrastructureMaintenanceResult infra = {0, 0, 0, 0, 0};
    ServiceMaintenanceSummary services = {0, 0, 0, 0, 0};

    auto breakdown = build_expense_breakdown(infra, services, 0, 2500, 0);

    assert(breakdown.bond_payments == 2500);
    assert(breakdown.total == 2500);

    printf("  PASS: Bond payments only\n");
}

// ============================================================================
// ExpenseHistory Tests
// ============================================================================

void test_expense_history_initial_state() {
    printf("Testing ExpenseHistory initial state...\n");

    ExpenseHistory history;

    assert(history.current_index == 0);
    assert(history.count == 0);
    assert(history.get_average() == 0);
    assert(history.get_trend() == 0);

    printf("  PASS: Initial state is zero\n");
}

void test_expense_history_record_single() {
    printf("Testing ExpenseHistory record single...\n");

    ExpenseHistory history;
    history.record(500);

    assert(history.count == 1);
    assert(history.get_average() == 500);
    assert(history.get_trend() == 0);

    printf("  PASS: Single record works\n");
}

void test_expense_history_record_multiple() {
    printf("Testing ExpenseHistory record multiple...\n");

    ExpenseHistory history;
    history.record(100);
    history.record(200);
    history.record(300);
    history.record(400);

    assert(history.count == 4);
    assert(history.get_average() == 250); // (100+200+300+400)/4

    printf("  PASS: Multiple records averaged correctly\n");
}

void test_expense_history_circular_wrap() {
    printf("Testing ExpenseHistory circular buffer wrap...\n");

    ExpenseHistory history;
    for (int i = 1; i <= 12; ++i) {
        history.record(i * 10);
    }

    assert(history.count == 12);
    assert(history.current_index == 0);

    // Record 13th entry, overwrites slot 0
    history.record(9999);
    assert(history.count == 12);
    assert(history.current_index == 1);
    assert(history.phases[0] == 9999);

    printf("  PASS: Circular buffer wraps correctly\n");
}

void test_expense_history_trend_growing() {
    printf("Testing ExpenseHistory trend growing...\n");

    ExpenseHistory history;
    history.record(50);
    history.record(50);
    history.record(150);
    history.record(150);

    int64_t trend = history.get_trend();
    assert(trend > 0);

    printf("  PASS: Growing trend is positive (%lld)\n", (long long)trend);
}

void test_expense_history_trend_shrinking() {
    printf("Testing ExpenseHistory trend shrinking...\n");

    ExpenseHistory history;
    history.record(400);
    history.record(400);
    history.record(100);
    history.record(100);

    int64_t trend = history.get_trend();
    assert(trend < 0);

    printf("  PASS: Shrinking trend is negative (%lld)\n", (long long)trend);
}

void test_expense_history_trend_flat() {
    printf("Testing ExpenseHistory trend flat...\n");

    ExpenseHistory history;
    for (int i = 0; i < 6; ++i) {
        history.record(200);
    }

    int64_t trend = history.get_trend();
    assert(trend == 0);

    printf("  PASS: Flat trend is zero\n");
}

// ============================================================================
// apply_expenses_to_treasury Tests
// ============================================================================

void test_apply_expenses_to_treasury() {
    printf("Testing apply_expenses_to_treasury...\n");

    TreasuryState treasury;
    ExpenseBreakdown expenses;
    expenses.infrastructure_maintenance = 200;
    expenses.service_maintenance = 500;
    expenses.energy_maintenance = 100;
    expenses.bond_payments = 300;
    expenses.ordinance_costs = 50;
    expenses.total = 1150;

    apply_expenses_to_treasury(treasury, expenses);

    assert(treasury.infrastructure_maintenance == 200);
    assert(treasury.service_maintenance == 500);
    assert(treasury.energy_maintenance == 100);
    assert(treasury.bond_payments == 300);
    assert(treasury.ordinance_costs == 50);
    assert(treasury.last_expense == 1150);

    printf("  PASS: Treasury expense fields updated correctly\n");
}

void test_apply_expenses_does_not_change_balance() {
    printf("Testing apply_expenses does not change balance...\n");

    TreasuryState treasury;
    int64_t original_balance = treasury.balance;

    ExpenseBreakdown expenses;
    expenses.total = 5000;
    expenses.service_maintenance = 5000;

    apply_expenses_to_treasury(treasury, expenses);

    assert(treasury.balance == original_balance);

    printf("  PASS: Balance unchanged after apply_expenses\n");
}

void test_apply_expenses_overwrites_previous() {
    printf("Testing apply_expenses overwrites previous values...\n");

    TreasuryState treasury;
    treasury.infrastructure_maintenance = 9999;
    treasury.last_expense = 9999;

    ExpenseBreakdown expenses;
    expenses.infrastructure_maintenance = 42;
    expenses.service_maintenance = 0;
    expenses.energy_maintenance = 0;
    expenses.bond_payments = 0;
    expenses.ordinance_costs = 0;
    expenses.total = 42;

    apply_expenses_to_treasury(treasury, expenses);

    assert(treasury.infrastructure_maintenance == 42);
    assert(treasury.last_expense == 42);

    printf("  PASS: Previous values overwritten\n");
}

void test_expense_breakdown_default_values() {
    printf("Testing ExpenseBreakdown default values...\n");

    ExpenseBreakdown breakdown;

    assert(breakdown.infrastructure_maintenance == 0);
    assert(breakdown.service_maintenance == 0);
    assert(breakdown.energy_maintenance == 0);
    assert(breakdown.bond_payments == 0);
    assert(breakdown.ordinance_costs == 0);
    assert(breakdown.total == 0);

    printf("  PASS: All defaults are zero\n");
}

// ============================================================================
// Main
// ============================================================================

int main() {
    printf("=== Expense Tracking Unit Tests (E11-011) ===\n\n");

    // build_expense_breakdown tests
    test_build_expense_breakdown_basic();
    test_build_expense_breakdown_zero_all();
    test_build_expense_breakdown_infra_only();
    test_build_expense_breakdown_services_only();
    test_build_expense_breakdown_bonds_only();

    // ExpenseHistory tests
    test_expense_history_initial_state();
    test_expense_history_record_single();
    test_expense_history_record_multiple();
    test_expense_history_circular_wrap();
    test_expense_history_trend_growing();
    test_expense_history_trend_shrinking();
    test_expense_history_trend_flat();

    // apply_expenses_to_treasury tests
    test_apply_expenses_to_treasury();
    test_apply_expenses_does_not_change_balance();
    test_apply_expenses_overwrites_previous();

    // Struct defaults
    test_expense_breakdown_default_values();

    printf("\n=== All tests passed! (16 tests) ===\n");
    return 0;
}
