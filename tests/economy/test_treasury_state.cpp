/**
 * @file test_treasury_state.cpp
 * @brief Unit tests for TreasuryState (E11-002)
 */

#include "sims3000/economy/TreasuryState.h"
#include <cassert>
#include <cstdio>

using namespace sims3000::economy;

void test_starting_balance() {
    printf("Testing TreasuryState starting balance...\n");

    TreasuryState ts;

    assert(ts.balance == 20000);

    printf("  PASS: Starting balance is 20000 credits\n");
}

void test_income_defaults() {
    printf("Testing TreasuryState income defaults...\n");

    TreasuryState ts;

    assert(ts.last_income == 0);
    assert(ts.habitation_tribute == 0);
    assert(ts.exchange_tribute == 0);
    assert(ts.fabrication_tribute == 0);
    assert(ts.other_income == 0);

    printf("  PASS: All income fields default to 0\n");
}

void test_expense_defaults() {
    printf("Testing TreasuryState expense defaults...\n");

    TreasuryState ts;

    assert(ts.last_expense == 0);
    assert(ts.infrastructure_maintenance == 0);
    assert(ts.service_maintenance == 0);
    assert(ts.energy_maintenance == 0);
    assert(ts.bond_payments == 0);
    assert(ts.ordinance_costs == 0);

    printf("  PASS: All expense fields default to 0\n");
}

void test_tribute_rate_defaults() {
    printf("Testing TreasuryState tribute rate defaults...\n");

    TreasuryState ts;

    assert(ts.tribute_rate_habitation == 7);
    assert(ts.tribute_rate_exchange == 7);
    assert(ts.tribute_rate_fabrication == 7);

    printf("  PASS: All tribute rates default to 7%%\n");
}

void test_funding_defaults() {
    printf("Testing TreasuryState funding level defaults...\n");

    TreasuryState ts;

    assert(ts.funding_enforcer == 100);
    assert(ts.funding_hazard_response == 100);
    assert(ts.funding_medical == 100);
    assert(ts.funding_education == 100);

    printf("  PASS: All funding levels default to 100%%\n");
}

void test_phase_tracking_defaults() {
    printf("Testing TreasuryState phase tracking defaults...\n");

    TreasuryState ts;

    assert(ts.last_processed_phase == 0);

    printf("  PASS: last_processed_phase defaults to 0\n");
}

void test_flag_defaults() {
    printf("Testing TreasuryState flag defaults...\n");

    TreasuryState ts;

    assert(ts.deficit_warning_sent == false);
    assert(ts.emergency_bond_active == false);

    printf("  PASS: All flags default to false\n");
}

void test_active_bonds_empty() {
    printf("Testing TreasuryState active_bonds starts empty...\n");

    TreasuryState ts;

    assert(ts.active_bonds.empty());

    printf("  PASS: active_bonds vector is empty by default\n");
}

void test_active_bonds_add() {
    printf("Testing TreasuryState adding bonds...\n");

    TreasuryState ts;

    CreditAdvance bond;
    bond.principal = 25000;
    bond.remaining_principal = 25000;
    bond.interest_rate_basis_points = 750;
    bond.term_phases = 24;
    bond.phases_remaining = 24;
    bond.is_emergency = false;

    ts.active_bonds.push_back(bond);

    assert(ts.active_bonds.size() == 1);
    assert(ts.active_bonds[0].principal == 25000);
    assert(ts.active_bonds[0].interest_rate_basis_points == 750);

    printf("  PASS: Can add bonds to active_bonds\n");
}

int main() {
    printf("=== TreasuryState Unit Tests (E11-002) ===\n\n");

    test_starting_balance();
    test_income_defaults();
    test_expense_defaults();
    test_tribute_rate_defaults();
    test_funding_defaults();
    test_phase_tracking_defaults();
    test_flag_defaults();
    test_active_bonds_empty();
    test_active_bonds_add();

    printf("\n=== All tests passed! ===\n");
    return 0;
}
