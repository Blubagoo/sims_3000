/**
 * @file test_budget_cycle.cpp
 * @brief Unit tests for BudgetCycle (E11-012)
 */

#include "sims3000/economy/BudgetCycle.h"
#include <cassert>
#include <cstdio>

using namespace sims3000::economy;

// ============================================================================
// Helper: create a simple BudgetCycleInput
// ============================================================================

static BudgetCycleInput make_input(int64_t income_total, int64_t expense_total) {
    BudgetCycleInput input;
    input.income.total = income_total;
    input.income.habitation_tribute = income_total; // simplify for testing
    input.expenses.total = expense_total;
    input.expenses.infrastructure_maintenance = expense_total; // simplify
    return input;
}

// ============================================================================
// Budget Cycle: Surplus Tests
// ============================================================================

void test_surplus_cycle() {
    printf("Testing surplus budget cycle...\n");

    TreasuryState treasury;
    treasury.balance = 20000;

    auto input = make_input(5000, 3000);
    auto result = process_budget_cycle(treasury, input, 0);

    assert(result.net_change == 2000);
    assert(result.new_balance == 22000);
    assert(treasury.balance == 22000);
    assert(result.is_deficit == false);

    printf("  PASS: Surplus cycle: 20000 + (5000 - 3000) = 22000\n");
}

void test_surplus_event() {
    printf("Testing surplus cycle event...\n");

    TreasuryState treasury;
    treasury.balance = 10000;

    auto input = make_input(3000, 1000);
    auto result = process_budget_cycle(treasury, input, 2);

    assert(result.event.player_id == 2);
    assert(result.event.income == 3000);
    assert(result.event.expenses == 1000);
    assert(result.event.net_change == 2000);
    assert(result.event.balance_after == 12000);

    printf("  PASS: Event fields correct for surplus\n");
}

// ============================================================================
// Budget Cycle: Deficit Tests
// ============================================================================

void test_deficit_cycle() {
    printf("Testing deficit budget cycle...\n");

    TreasuryState treasury;
    treasury.balance = 1000;

    auto input = make_input(500, 2000);
    auto result = process_budget_cycle(treasury, input, 0);

    assert(result.net_change == -1500);
    assert(result.new_balance == -500);
    assert(treasury.balance == -500);
    assert(result.is_deficit == true);

    printf("  PASS: Deficit cycle: 1000 + (500 - 2000) = -500\n");
}

void test_deficit_from_positive() {
    printf("Testing transition from positive to deficit...\n");

    TreasuryState treasury;
    treasury.balance = 100;

    auto input = make_input(0, 200);
    auto result = process_budget_cycle(treasury, input, 0);

    assert(result.new_balance == -100);
    assert(result.is_deficit == true);

    printf("  PASS: Positive to deficit transition\n");
}

void test_already_in_deficit() {
    printf("Testing cycle when already in deficit...\n");

    TreasuryState treasury;
    treasury.balance = -5000;

    auto input = make_input(1000, 500);
    auto result = process_budget_cycle(treasury, input, 0);

    assert(result.net_change == 500);
    assert(result.new_balance == -4500);
    assert(result.is_deficit == true); // still negative

    printf("  PASS: Still in deficit: -5000 + 500 = -4500\n");
}

// ============================================================================
// Budget Cycle: Zero Income/Expense Tests
// ============================================================================

void test_zero_income() {
    printf("Testing zero income budget cycle...\n");

    TreasuryState treasury;
    treasury.balance = 10000;

    auto input = make_input(0, 500);
    auto result = process_budget_cycle(treasury, input, 0);

    assert(result.net_change == -500);
    assert(result.new_balance == 9500);
    assert(result.is_deficit == false);

    printf("  PASS: Zero income: 10000 + (0 - 500) = 9500\n");
}

void test_zero_expenses() {
    printf("Testing zero expenses budget cycle...\n");

    TreasuryState treasury;
    treasury.balance = 10000;

    auto input = make_input(3000, 0);
    auto result = process_budget_cycle(treasury, input, 0);

    assert(result.net_change == 3000);
    assert(result.new_balance == 13000);

    printf("  PASS: Zero expenses: 10000 + (3000 - 0) = 13000\n");
}

void test_zero_both() {
    printf("Testing zero income and expenses...\n");

    TreasuryState treasury;
    treasury.balance = 5000;

    auto input = make_input(0, 0);
    auto result = process_budget_cycle(treasury, input, 0);

    assert(result.net_change == 0);
    assert(result.new_balance == 5000);
    assert(result.is_deficit == false);

    printf("  PASS: Zero both: balance unchanged at 5000\n");
}

// ============================================================================
// Budget Cycle: Treasury Field Updates
// ============================================================================

void test_treasury_income_fields_updated() {
    printf("Testing treasury income fields updated...\n");

    TreasuryState treasury;
    BudgetCycleInput input;
    input.income.habitation_tribute = 100;
    input.income.exchange_tribute = 200;
    input.income.fabrication_tribute = 300;
    input.income.other_income = 50;
    input.income.total = 650;
    input.expenses.total = 0;

    process_budget_cycle(treasury, input, 0);

    assert(treasury.habitation_tribute == 100);
    assert(treasury.exchange_tribute == 200);
    assert(treasury.fabrication_tribute == 300);
    assert(treasury.other_income == 50);
    assert(treasury.last_income == 650);

    printf("  PASS: Treasury income fields updated\n");
}

void test_treasury_expense_fields_updated() {
    printf("Testing treasury expense fields updated...\n");

    TreasuryState treasury;
    BudgetCycleInput input;
    input.income.total = 0;
    input.expenses.infrastructure_maintenance = 100;
    input.expenses.service_maintenance = 200;
    input.expenses.energy_maintenance = 50;
    input.expenses.bond_payments = 300;
    input.expenses.ordinance_costs = 25;
    input.expenses.total = 675;

    process_budget_cycle(treasury, input, 0);

    assert(treasury.infrastructure_maintenance == 100);
    assert(treasury.service_maintenance == 200);
    assert(treasury.energy_maintenance == 50);
    assert(treasury.bond_payments == 300);
    assert(treasury.ordinance_costs == 25);
    assert(treasury.last_expense == 675);

    printf("  PASS: Treasury expense fields updated\n");
}

// ============================================================================
// Bond Payment Calculation Tests (pure function)
// ============================================================================

void test_calculate_bond_payments_single() {
    printf("Testing calculate_bond_payments single bond...\n");

    CreditAdvance bond;
    bond.principal = 12000;
    bond.remaining_principal = 12000;
    bond.interest_rate_basis_points = 600; // 6%
    bond.term_phases = 12;
    bond.phases_remaining = 12;

    std::vector<CreditAdvance> bonds = {bond};
    auto result = calculate_bond_payments(bonds);

    // principal_payment = 12000 / 12 = 1000
    // interest_payment = (12000 * 600) / (10000 * 12) = 7200000 / 120000 = 60
    // total = 1060
    assert(result.principal_paid == 1000);
    assert(result.interest_paid == 60);
    assert(result.total_payment == 1060);
    assert(result.bonds_matured == 0); // phases_remaining=12, not maturing

    printf("  PASS: Single bond payment: principal=1000, interest=60, total=1060\n");
}

void test_calculate_bond_payments_maturing() {
    printf("Testing calculate_bond_payments maturing bond...\n");

    CreditAdvance bond;
    bond.principal = 6000;
    bond.remaining_principal = 500;
    bond.interest_rate_basis_points = 750;
    bond.term_phases = 12;
    bond.phases_remaining = 1; // last phase!

    std::vector<CreditAdvance> bonds = {bond};
    auto result = calculate_bond_payments(bonds);

    assert(result.bonds_matured == 1);
    assert(result.principal_paid == 500); // 6000/12 = 500

    printf("  PASS: Maturing bond detected\n");
}

void test_calculate_bond_payments_multiple() {
    printf("Testing calculate_bond_payments multiple bonds...\n");

    CreditAdvance bond1;
    bond1.principal = 12000;
    bond1.remaining_principal = 12000;
    bond1.interest_rate_basis_points = 600;
    bond1.term_phases = 12;
    bond1.phases_remaining = 12;

    CreditAdvance bond2;
    bond2.principal = 24000;
    bond2.remaining_principal = 24000;
    bond2.interest_rate_basis_points = 1000; // 10%
    bond2.term_phases = 24;
    bond2.phases_remaining = 24;

    std::vector<CreditAdvance> bonds = {bond1, bond2};
    auto result = calculate_bond_payments(bonds);

    // bond1: principal=1000, interest=60, total=1060
    // bond2: principal=1000, interest=(24000*1000)/(120000)=200, total=1200
    assert(result.principal_paid == 2000);
    assert(result.interest_paid == 260);
    assert(result.total_payment == 2260);
    assert(result.bonds_matured == 0);

    printf("  PASS: Multiple bond payments summed correctly\n");
}

void test_calculate_bond_payments_empty() {
    printf("Testing calculate_bond_payments empty vector...\n");

    std::vector<CreditAdvance> bonds;
    auto result = calculate_bond_payments(bonds);

    assert(result.total_payment == 0);
    assert(result.principal_paid == 0);
    assert(result.interest_paid == 0);
    assert(result.bonds_matured == 0);

    printf("  PASS: Empty bonds vector returns zeros\n");
}

void test_calculate_bond_payments_zero_interest() {
    printf("Testing calculate_bond_payments zero interest bond...\n");

    CreditAdvance bond;
    bond.principal = 10000;
    bond.remaining_principal = 5000;
    bond.interest_rate_basis_points = 0;
    bond.term_phases = 10;
    bond.phases_remaining = 5;

    std::vector<CreditAdvance> bonds = {bond};
    auto result = calculate_bond_payments(bonds);

    assert(result.principal_paid == 1000); // 10000/10
    assert(result.interest_paid == 0);
    assert(result.total_payment == 1000);

    printf("  PASS: Zero interest bond\n");
}

// ============================================================================
// Bond Payment Processing Tests (mutating)
// ============================================================================

void test_process_bond_payments_decrements_phases() {
    printf("Testing process_bond_payments decrements phases...\n");

    CreditAdvance bond;
    bond.principal = 12000;
    bond.remaining_principal = 12000;
    bond.interest_rate_basis_points = 600;
    bond.term_phases = 12;
    bond.phases_remaining = 12;

    std::vector<CreditAdvance> bonds = {bond};
    process_bond_payments(bonds);

    assert(bonds.size() == 1);
    assert(bonds[0].phases_remaining == 11);
    assert(bonds[0].remaining_principal == 11000); // 12000 - 1000

    printf("  PASS: Phases decremented and principal reduced\n");
}

void test_process_bond_payments_removes_matured() {
    printf("Testing process_bond_payments removes matured bonds...\n");

    CreditAdvance bond;
    bond.principal = 6000;
    bond.remaining_principal = 500;
    bond.interest_rate_basis_points = 750;
    bond.term_phases = 12;
    bond.phases_remaining = 1; // will mature

    std::vector<CreditAdvance> bonds = {bond};
    auto result = process_bond_payments(bonds);

    assert(bonds.empty());
    assert(result.bonds_matured == 1);

    printf("  PASS: Matured bond removed from vector\n");
}

void test_process_bond_payments_mixed() {
    printf("Testing process_bond_payments mixed maturity...\n");

    CreditAdvance bond1;
    bond1.principal = 6000;
    bond1.remaining_principal = 500;
    bond1.interest_rate_basis_points = 750;
    bond1.term_phases = 12;
    bond1.phases_remaining = 1; // will mature

    CreditAdvance bond2;
    bond2.principal = 24000;
    bond2.remaining_principal = 24000;
    bond2.interest_rate_basis_points = 1000;
    bond2.term_phases = 24;
    bond2.phases_remaining = 24; // stays

    std::vector<CreditAdvance> bonds = {bond1, bond2};
    auto result = process_bond_payments(bonds);

    assert(bonds.size() == 1);
    assert(result.bonds_matured == 1);
    assert(bonds[0].phases_remaining == 23);
    assert(bonds[0].remaining_principal == 23000); // 24000 - 1000

    printf("  PASS: Matured bond removed, active bond updated\n");
}

void test_process_bond_payments_zero_term() {
    printf("Testing process_bond_payments zero-term bond...\n");

    CreditAdvance bond;
    bond.principal = 1000;
    bond.remaining_principal = 1000;
    bond.interest_rate_basis_points = 500;
    bond.term_phases = 0; // edge case
    bond.phases_remaining = 0;

    std::vector<CreditAdvance> bonds = {bond};
    auto result = process_bond_payments(bonds);

    // Zero-term bond should be removed immediately
    assert(bonds.empty());
    assert(result.bonds_matured == 1);

    printf("  PASS: Zero-term bond handled\n");
}

// ============================================================================
// Budget Cycle: Balance at exactly zero
// ============================================================================

void test_balance_exactly_zero() {
    printf("Testing balance at exactly zero...\n");

    TreasuryState treasury;
    treasury.balance = 500;

    auto input = make_input(0, 500);
    auto result = process_budget_cycle(treasury, input, 0);

    assert(result.new_balance == 0);
    assert(result.is_deficit == false); // zero is not deficit

    printf("  PASS: Balance at zero is not deficit\n");
}

// ============================================================================
// Main
// ============================================================================

int main() {
    printf("=== Budget Cycle Unit Tests (E11-012) ===\n\n");

    // Surplus tests
    test_surplus_cycle();
    test_surplus_event();

    // Deficit tests
    test_deficit_cycle();
    test_deficit_from_positive();
    test_already_in_deficit();

    // Zero income/expense tests
    test_zero_income();
    test_zero_expenses();
    test_zero_both();

    // Treasury field update tests
    test_treasury_income_fields_updated();
    test_treasury_expense_fields_updated();

    // Bond calculation tests (pure)
    test_calculate_bond_payments_single();
    test_calculate_bond_payments_maturing();
    test_calculate_bond_payments_multiple();
    test_calculate_bond_payments_empty();
    test_calculate_bond_payments_zero_interest();

    // Bond processing tests (mutating)
    test_process_bond_payments_decrements_phases();
    test_process_bond_payments_removes_matured();
    test_process_bond_payments_mixed();
    test_process_bond_payments_zero_term();

    // Edge cases
    test_balance_exactly_zero();

    printf("\n=== All tests passed! (20 tests) ===\n");
    return 0;
}
