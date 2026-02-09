/**
 * @file test_bond_repayment.cpp
 * @brief Unit tests for BondRepayment (E11-017)
 *
 * Tests: detailed payment calculation, principal/interest split, remaining
 * principal decrease, bond maturation and removal, BondPaidOffEvent emission,
 * multiple bonds, get_total_debt, edge cases.
 */

#include "sims3000/economy/BondRepayment.h"
#include <cassert>
#include <cstdio>

using namespace sims3000::economy;

// ---------------------------------------------------------------------------
// Helper: create a standard test bond
// ---------------------------------------------------------------------------

static CreditAdvance make_bond(int64_t principal, uint16_t rate_bps,
                               uint16_t term, uint16_t remaining,
                               bool emergency = false) {
    CreditAdvance b;
    b.principal = principal;
    b.remaining_principal = principal; // full at start
    b.interest_rate_basis_points = rate_bps;
    b.term_phases = term;
    b.phases_remaining = remaining;
    b.is_emergency = emergency;
    return b;
}

static CreditAdvance make_bond_with_remaining(int64_t principal,
                                               int64_t remaining_principal,
                                               uint16_t rate_bps,
                                               uint16_t term,
                                               uint16_t phases_remaining,
                                               bool emergency = false) {
    CreditAdvance b;
    b.principal = principal;
    b.remaining_principal = remaining_principal;
    b.interest_rate_basis_points = rate_bps;
    b.term_phases = term;
    b.phases_remaining = phases_remaining;
    b.is_emergency = emergency;
    return b;
}

// ---------------------------------------------------------------------------
// Test: calculate_detailed_bond_payments with single bond
// ---------------------------------------------------------------------------

void test_calculate_single_bond_payment() {
    printf("Testing calculate_detailed_bond_payments with single bond...\n");

    std::vector<CreditAdvance> bonds;
    // Small bond: 5000 principal, 500 bps (5%), 12 phases
    bonds.push_back(make_bond(5000, 500, 12, 12));

    auto result = calculate_detailed_bond_payments(bonds, 1);

    assert(result.payments.size() == 1);

    // principal_payment = 5000 / 12 = 416
    assert(result.payments[0].principal_payment == 416);

    // interest_payment = (5000 * 500) / (10000 * 12) = 2500000 / 120000 = 20
    assert(result.payments[0].interest_payment == 20);

    assert(result.payments[0].total_payment == 416 + 20);
    assert(result.payments[0].bond_index == 0);
    assert(result.payments[0].is_final_payment == false);

    assert(result.total_principal_paid == 416);
    assert(result.total_interest_paid == 20);
    assert(result.total_payment == 436);
    assert(result.matured_events.empty());

    printf("  PASS: Single bond payment calculated correctly\n");
}

// ---------------------------------------------------------------------------
// Test: principal payment calculation
// ---------------------------------------------------------------------------

void test_principal_payment_formula() {
    printf("Testing principal payment formula...\n");

    // Standard bond: 25000 / 24 = 1041
    std::vector<CreditAdvance> bonds;
    bonds.push_back(make_bond(25000, 750, 24, 24));

    auto result = calculate_detailed_bond_payments(bonds, 0);
    assert(result.payments[0].principal_payment == 1041);

    printf("  PASS: Principal payment = principal / term_phases\n");
}

// ---------------------------------------------------------------------------
// Test: interest payment calculation
// ---------------------------------------------------------------------------

void test_interest_payment_formula() {
    printf("Testing interest payment formula...\n");

    // Bond: 100000 principal, 1000 bps (10%), remaining 100000
    // interest = (100000 * 1000) / (10000 * 12) = 100000000 / 120000 = 833
    std::vector<CreditAdvance> bonds;
    bonds.push_back(make_bond(100000, 1000, 48, 48));

    auto result = calculate_detailed_bond_payments(bonds, 0);
    assert(result.payments[0].interest_payment == 833);

    printf("  PASS: Interest = (remaining * rate_bps) / (10000 * 12)\n");
}

// ---------------------------------------------------------------------------
// Test: interest decreases as principal is paid down
// ---------------------------------------------------------------------------

void test_interest_decreases_with_principal() {
    printf("Testing interest decreases as principal is paid down...\n");

    // Bond with half principal remaining
    // principal=10000, remaining=5000, 1000 bps, 10 phases, 5 remaining
    std::vector<CreditAdvance> bonds;
    bonds.push_back(make_bond_with_remaining(10000, 5000, 1000, 10, 5));

    auto result = calculate_detailed_bond_payments(bonds, 0);

    // principal_payment = 10000 / 10 = 1000
    assert(result.payments[0].principal_payment == 1000);

    // interest = (5000 * 1000) / (10000 * 12) = 5000000 / 120000 = 41
    assert(result.payments[0].interest_payment == 41);

    printf("  PASS: Interest based on remaining_principal, not original\n");
}

// ---------------------------------------------------------------------------
// Test: bond maturation detection (phases_remaining == 1)
// ---------------------------------------------------------------------------

void test_bond_maturation_detection() {
    printf("Testing bond maturation detection...\n");

    std::vector<CreditAdvance> bonds;
    // Bond with 1 phase remaining -> will mature
    bonds.push_back(make_bond_with_remaining(12000, 1000, 500, 12, 1));

    auto result = calculate_detailed_bond_payments(bonds, 3);

    assert(result.payments[0].is_final_payment == true);
    assert(result.matured_events.size() == 1);
    assert(result.matured_events[0].player_id == 3);
    assert(result.matured_events[0].principal == 12000);
    assert(result.matured_events[0].was_emergency == false);

    printf("  PASS: Bond with phases_remaining=1 detected as maturing\n");
}

// ---------------------------------------------------------------------------
// Test: process removes matured bonds
// ---------------------------------------------------------------------------

void test_process_removes_matured_bonds() {
    printf("Testing process_detailed_bond_payments removes matured bonds...\n");

    std::vector<CreditAdvance> bonds;
    // Bond about to mature (1 phase left)
    bonds.push_back(make_bond_with_remaining(12000, 1000, 500, 12, 1));
    // Bond with many phases left
    bonds.push_back(make_bond(25000, 750, 24, 24));

    auto result = process_detailed_bond_payments(bonds, 1);

    // First bond should be removed
    assert(bonds.size() == 1);
    // Remaining bond is the 25000 one
    assert(bonds[0].principal == 25000);

    // Matured events
    assert(result.matured_events.size() == 1);
    assert(result.matured_events[0].principal == 12000);

    printf("  PASS: Matured bond removed, remaining bond kept\n");
}

// ---------------------------------------------------------------------------
// Test: process decreases remaining_principal
// ---------------------------------------------------------------------------

void test_process_decreases_remaining_principal() {
    printf("Testing process decreases remaining_principal...\n");

    std::vector<CreditAdvance> bonds;
    bonds.push_back(make_bond(12000, 500, 12, 12));

    int64_t original_remaining = bonds[0].remaining_principal;
    int64_t expected_pp = 12000 / 12; // 1000

    process_detailed_bond_payments(bonds, 0);

    assert(bonds.size() == 1);
    assert(bonds[0].remaining_principal == original_remaining - expected_pp);
    assert(bonds[0].phases_remaining == 11);

    printf("  PASS: Remaining principal decreased by principal_payment\n");
}

// ---------------------------------------------------------------------------
// Test: process decrements phases_remaining
// ---------------------------------------------------------------------------

void test_process_decrements_phases() {
    printf("Testing process decrements phases_remaining...\n");

    std::vector<CreditAdvance> bonds;
    bonds.push_back(make_bond(5000, 500, 12, 6));

    process_detailed_bond_payments(bonds, 0);

    assert(bonds[0].phases_remaining == 5);

    printf("  PASS: phases_remaining decremented from 6 to 5\n");
}

// ---------------------------------------------------------------------------
// Test: multiple bonds processed
// ---------------------------------------------------------------------------

void test_multiple_bonds_processed() {
    printf("Testing multiple bonds processed in one call...\n");

    std::vector<CreditAdvance> bonds;
    bonds.push_back(make_bond(5000, 500, 12, 12));
    bonds.push_back(make_bond(25000, 750, 24, 24));
    bonds.push_back(make_bond(100000, 1000, 48, 48));

    auto result = calculate_detailed_bond_payments(bonds, 0);

    assert(result.payments.size() == 3);
    assert(result.payments[0].bond_index == 0);
    assert(result.payments[1].bond_index == 1);
    assert(result.payments[2].bond_index == 2);

    // Totals should be sum of individual payments
    int64_t expected_total_pp = 0;
    int64_t expected_total_ip = 0;
    for (const auto& p : result.payments) {
        expected_total_pp += p.principal_payment;
        expected_total_ip += p.interest_payment;
    }
    assert(result.total_principal_paid == expected_total_pp);
    assert(result.total_interest_paid == expected_total_ip);
    assert(result.total_payment == expected_total_pp + expected_total_ip);

    printf("  PASS: 3 bonds processed with correct totals\n");
}

// ---------------------------------------------------------------------------
// Test: BondPaidOffEvent for emergency bond
// ---------------------------------------------------------------------------

void test_paid_off_event_emergency() {
    printf("Testing BondPaidOffEvent for emergency bond...\n");

    std::vector<CreditAdvance> bonds;
    bonds.push_back(make_bond_with_remaining(25000, 2083, 1500, 12, 1, true));

    auto result = calculate_detailed_bond_payments(bonds, 5);

    assert(result.matured_events.size() == 1);
    assert(result.matured_events[0].player_id == 5);
    assert(result.matured_events[0].principal == 25000);
    assert(result.matured_events[0].was_emergency == true);
    assert(result.matured_events[0].total_interest_paid > 0);

    printf("  PASS: Emergency bond paid-off event has was_emergency=true\n");
}

// ---------------------------------------------------------------------------
// Test: get_total_debt
// ---------------------------------------------------------------------------

void test_get_total_debt_empty() {
    printf("Testing get_total_debt with empty bonds...\n");

    std::vector<CreditAdvance> bonds;
    assert(get_total_debt(bonds) == 0);

    printf("  PASS: Total debt of empty vector is 0\n");
}

void test_get_total_debt_single() {
    printf("Testing get_total_debt with single bond...\n");

    std::vector<CreditAdvance> bonds;
    bonds.push_back(make_bond(25000, 750, 24, 24));
    assert(get_total_debt(bonds) == 25000);

    printf("  PASS: Total debt equals remaining_principal\n");
}

void test_get_total_debt_multiple() {
    printf("Testing get_total_debt with multiple bonds...\n");

    std::vector<CreditAdvance> bonds;
    bonds.push_back(make_bond(5000, 500, 12, 12));
    bonds.push_back(make_bond_with_remaining(25000, 12000, 750, 24, 12));
    bonds.push_back(make_bond(100000, 1000, 48, 48));

    assert(get_total_debt(bonds) == 5000 + 12000 + 100000);

    printf("  PASS: Total debt sums all remaining_principal values\n");
}

// ---------------------------------------------------------------------------
// Test: no bonds produces empty result
// ---------------------------------------------------------------------------

void test_empty_bonds() {
    printf("Testing with no active bonds...\n");

    std::vector<CreditAdvance> bonds;
    auto result = calculate_detailed_bond_payments(bonds, 0);

    assert(result.payments.empty());
    assert(result.total_principal_paid == 0);
    assert(result.total_interest_paid == 0);
    assert(result.total_payment == 0);
    assert(result.matured_events.empty());

    printf("  PASS: Empty bonds produce empty result\n");
}

// ---------------------------------------------------------------------------
// Test: zero term_phases edge case
// ---------------------------------------------------------------------------

void test_zero_term_phases() {
    printf("Testing bond with term_phases=0...\n");

    std::vector<CreditAdvance> bonds;
    CreditAdvance b;
    b.principal = 1000;
    b.remaining_principal = 1000;
    b.interest_rate_basis_points = 500;
    b.term_phases = 0;
    b.phases_remaining = 0;
    b.is_emergency = false;
    bonds.push_back(b);

    auto result = calculate_detailed_bond_payments(bonds, 0);

    assert(result.payments.size() == 1);
    assert(result.payments[0].principal_payment == 0);
    assert(result.payments[0].interest_payment == 0);
    assert(result.payments[0].is_final_payment == true);

    printf("  PASS: Zero term_phases handled gracefully\n");
}

// ---------------------------------------------------------------------------
// Test: all bonds mature simultaneously
// ---------------------------------------------------------------------------

void test_all_bonds_mature() {
    printf("Testing all bonds maturing simultaneously...\n");

    std::vector<CreditAdvance> bonds;
    bonds.push_back(make_bond_with_remaining(5000, 416, 500, 12, 1));
    bonds.push_back(make_bond_with_remaining(25000, 1041, 750, 24, 1));

    auto result = process_detailed_bond_payments(bonds, 2);

    assert(bonds.empty());
    assert(result.matured_events.size() == 2);
    assert(result.matured_events[0].player_id == 2);
    assert(result.matured_events[1].player_id == 2);

    printf("  PASS: All bonds removed when all mature\n");
}

// ---------------------------------------------------------------------------
// Test: total_interest_paid in BondPaidOffEvent is positive
// ---------------------------------------------------------------------------

void test_total_interest_paid_positive() {
    printf("Testing total_interest_paid is positive for maturing bond...\n");

    std::vector<CreditAdvance> bonds;
    // Standard bond about to mature
    bonds.push_back(make_bond_with_remaining(25000, 1041, 750, 24, 1));

    auto result = calculate_detailed_bond_payments(bonds, 0);

    assert(result.matured_events.size() == 1);
    assert(result.matured_events[0].total_interest_paid > 0);

    printf("  PASS: Total interest paid is positive (%lld)\n",
           static_cast<long long>(result.matured_events[0].total_interest_paid));
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main() {
    printf("=== BondRepayment Unit Tests (E11-017) ===\n\n");

    // Calculation tests
    test_calculate_single_bond_payment();
    test_principal_payment_formula();
    test_interest_payment_formula();
    test_interest_decreases_with_principal();
    test_bond_maturation_detection();

    // Processing tests
    test_process_removes_matured_bonds();
    test_process_decreases_remaining_principal();
    test_process_decrements_phases();
    test_multiple_bonds_processed();

    // Event tests
    test_paid_off_event_emergency();
    test_total_interest_paid_positive();

    // get_total_debt tests
    test_get_total_debt_empty();
    test_get_total_debt_single();
    test_get_total_debt_multiple();

    // Edge cases
    test_empty_bonds();
    test_zero_term_phases();
    test_all_bonds_mature();

    printf("\n=== All BondRepayment tests passed! (17 tests) ===\n");
    return 0;
}
