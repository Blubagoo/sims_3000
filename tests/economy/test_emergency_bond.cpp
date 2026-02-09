/**
 * @file test_emergency_bond.cpp
 * @brief Unit tests for EmergencyBond (E11-018)
 *
 * Tests: auto-issue triggers, disabled auto-bonds, already active,
 * not below threshold, correct bond values, event data, balance changes.
 */

#include "sims3000/economy/EmergencyBond.h"
#include "sims3000/economy/DeficitHandling.h"
#include <cassert>
#include <cstdio>

using namespace sims3000::economy;

// ---------------------------------------------------------------------------
// Test: auto-issue triggers at threshold
// ---------------------------------------------------------------------------

void test_auto_issue_triggers() {
    printf("Testing emergency bond auto-issues below threshold...\n");

    TreasuryState ts;
    ts.balance = -15000; // Below EMERGENCY_BOND_THRESHOLD (-10000)

    auto result = check_and_issue_emergency_bond(ts, 1);

    assert(result.issued == true);
    assert(ts.emergency_bond_active == true);
    assert(ts.active_bonds.size() == 1);

    printf("  PASS: Emergency bond auto-issued at balance -15000\n");
}

// ---------------------------------------------------------------------------
// Test: correct bond values from BOND_EMERGENCY config
// ---------------------------------------------------------------------------

void test_correct_bond_values() {
    printf("Testing emergency bond has correct config values...\n");

    TreasuryState ts;
    ts.balance = -12000;

    auto result = check_and_issue_emergency_bond(ts, 0);

    assert(result.issued == true);
    const auto& bond = ts.active_bonds[0];

    // BOND_EMERGENCY: 25000, 1500 bps (15%), 12 phases, is_emergency=true
    assert(bond.principal == 25000);
    assert(bond.remaining_principal == 25000);
    assert(bond.interest_rate_basis_points == 1500);
    assert(bond.term_phases == 12);
    assert(bond.phases_remaining == 12);
    assert(bond.is_emergency == true);

    printf("  PASS: Bond matches BOND_EMERGENCY config\n");
}

// ---------------------------------------------------------------------------
// Test: balance updated correctly
// ---------------------------------------------------------------------------

void test_balance_updated() {
    printf("Testing balance updated after emergency bond...\n");

    TreasuryState ts;
    ts.balance = -15000;

    auto result = check_and_issue_emergency_bond(ts, 0);

    assert(result.issued == true);
    assert(ts.balance == -15000 + 25000); // = 10000

    printf("  PASS: Balance updated from -15000 to %lld\n",
           static_cast<long long>(ts.balance));
}

// ---------------------------------------------------------------------------
// Test: event data correct
// ---------------------------------------------------------------------------

void test_event_data() {
    printf("Testing EmergencyBondIssuedEvent data...\n");

    TreasuryState ts;
    ts.balance = -20000;

    auto result = check_and_issue_emergency_bond(ts, 3);

    assert(result.issued == true);
    assert(result.event.player_id == 3);
    assert(result.event.principal == 25000);
    assert(result.event.balance_before == -20000);
    assert(result.event.balance_after == -20000 + 25000); // = 5000

    printf("  PASS: Event data correct (player=%d, before=%lld, after=%lld)\n",
           result.event.player_id,
           static_cast<long long>(result.event.balance_before),
           static_cast<long long>(result.event.balance_after));
}

// ---------------------------------------------------------------------------
// Test: not below threshold -- no issuance
// ---------------------------------------------------------------------------

void test_not_below_threshold() {
    printf("Testing no issuance when balance above threshold...\n");

    TreasuryState ts;
    ts.balance = -5000; // Above EMERGENCY_BOND_THRESHOLD (-10000)

    auto result = check_and_issue_emergency_bond(ts, 0);

    assert(result.issued == false);
    assert(ts.active_bonds.empty());
    assert(ts.emergency_bond_active == false);
    assert(ts.balance == -5000);

    printf("  PASS: No emergency bond at -5000\n");
}

// ---------------------------------------------------------------------------
// Test: exactly at threshold -- no issuance (not strictly below)
// ---------------------------------------------------------------------------

void test_at_exact_threshold() {
    printf("Testing no issuance at exact threshold...\n");

    TreasuryState ts;
    ts.balance = constants::EMERGENCY_BOND_THRESHOLD; // -10000

    auto result = check_and_issue_emergency_bond(ts, 0);

    assert(result.issued == false);
    assert(ts.active_bonds.empty());

    printf("  PASS: No emergency bond at exact threshold (%lld)\n",
           static_cast<long long>(constants::EMERGENCY_BOND_THRESHOLD));
}

// ---------------------------------------------------------------------------
// Test: disabled auto-bonds
// ---------------------------------------------------------------------------

void test_disabled_auto_bonds() {
    printf("Testing auto-bonds disabled by player setting...\n");

    TreasuryState ts;
    ts.balance = -15000;

    auto result = check_and_issue_emergency_bond(ts, 0, false);

    assert(result.issued == false);
    assert(ts.active_bonds.empty());
    assert(ts.emergency_bond_active == false);
    assert(ts.balance == -15000);

    printf("  PASS: No emergency bond when auto_bonds_enabled=false\n");
}

// ---------------------------------------------------------------------------
// Test: already active -- no second issuance
// ---------------------------------------------------------------------------

void test_already_active() {
    printf("Testing no second emergency bond when one is active...\n");

    TreasuryState ts;
    ts.balance = -15000;
    ts.emergency_bond_active = true;

    auto result = check_and_issue_emergency_bond(ts, 0);

    assert(result.issued == false);
    assert(ts.active_bonds.empty()); // No new bond added
    assert(ts.balance == -15000);    // Balance unchanged

    printf("  PASS: Second emergency bond rejected\n");
}

// ---------------------------------------------------------------------------
// Test: positive balance -- no issuance
// ---------------------------------------------------------------------------

void test_positive_balance() {
    printf("Testing no issuance with positive balance...\n");

    TreasuryState ts;
    ts.balance = 10000;

    auto result = check_and_issue_emergency_bond(ts, 0);

    assert(result.issued == false);
    assert(ts.active_bonds.empty());
    assert(ts.balance == 10000);

    printf("  PASS: No emergency bond with positive balance\n");
}

// ---------------------------------------------------------------------------
// Test: zero balance -- no issuance
// ---------------------------------------------------------------------------

void test_zero_balance() {
    printf("Testing no issuance with zero balance...\n");

    TreasuryState ts;
    ts.balance = 0;

    auto result = check_and_issue_emergency_bond(ts, 0);

    assert(result.issued == false);

    printf("  PASS: No emergency bond at zero balance\n");
}

// ---------------------------------------------------------------------------
// Test: emergency bond added to existing bonds
// ---------------------------------------------------------------------------

void test_added_to_existing_bonds() {
    printf("Testing emergency bond added alongside existing bonds...\n");

    TreasuryState ts;
    ts.balance = -15000;

    // Pre-existing bonds
    CreditAdvance existing;
    existing.principal = 5000;
    existing.remaining_principal = 3000;
    existing.interest_rate_basis_points = 500;
    existing.term_phases = 12;
    existing.phases_remaining = 6;
    existing.is_emergency = false;
    ts.active_bonds.push_back(existing);

    auto result = check_and_issue_emergency_bond(ts, 0);

    assert(result.issued == true);
    assert(ts.active_bonds.size() == 2);
    assert(ts.active_bonds[0].is_emergency == false); // existing
    assert(ts.active_bonds[1].is_emergency == true);  // new emergency bond

    printf("  PASS: Emergency bond appended to existing bonds\n");
}

// ---------------------------------------------------------------------------
// Test: just below threshold triggers
// ---------------------------------------------------------------------------

void test_just_below_threshold() {
    printf("Testing issuance at just below threshold...\n");

    TreasuryState ts;
    ts.balance = constants::EMERGENCY_BOND_THRESHOLD - 1; // -10001

    auto result = check_and_issue_emergency_bond(ts, 0);

    assert(result.issued == true);

    printf("  PASS: Emergency bond issued at %lld\n",
           static_cast<long long>(constants::EMERGENCY_BOND_THRESHOLD - 1));
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main() {
    printf("=== EmergencyBond Unit Tests (E11-018) ===\n\n");

    test_auto_issue_triggers();
    test_correct_bond_values();
    test_balance_updated();
    test_event_data();
    test_not_below_threshold();
    test_at_exact_threshold();
    test_disabled_auto_bonds();
    test_already_active();
    test_positive_balance();
    test_zero_balance();
    test_added_to_existing_bonds();
    test_just_below_threshold();

    printf("\n=== All EmergencyBond tests passed! (12 tests) ===\n");
    return 0;
}
