/**
 * @file test_deficit_handling.cpp
 * @brief Unit tests for DeficitHandling (E11-015)
 *
 * Tests: constants, no deficit, warning threshold, emergency threshold,
 *        already warned (no re-warn), recovery reset, edge cases,
 *        apply_deficit_state, events.
 */

#include "sims3000/economy/DeficitHandling.h"
#include <cassert>
#include <cstdio>

using namespace sims3000::economy;

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

void test_constants() {
    printf("Testing deficit handling constants...\n");

    assert(constants::DEFICIT_WARNING_THRESHOLD == -5000);
    assert(constants::EMERGENCY_BOND_THRESHOLD == -10000);

    printf("  PASS: Constants have correct values\n");
}

// ---------------------------------------------------------------------------
// No deficit
// ---------------------------------------------------------------------------

void test_no_deficit_positive_balance() {
    printf("Testing check_deficit with positive balance...\n");

    TreasuryState ts;
    ts.balance = 20000;

    auto result = check_deficit(ts, 0);

    assert(result.should_warn == false);
    assert(result.should_offer_bond == false);

    printf("  PASS: No warnings for positive balance\n");
}

void test_no_deficit_zero_balance() {
    printf("Testing check_deficit with zero balance...\n");

    TreasuryState ts;
    ts.balance = 0;

    auto result = check_deficit(ts, 0);

    assert(result.should_warn == false);
    assert(result.should_offer_bond == false);

    printf("  PASS: No warnings for zero balance\n");
}

void test_no_deficit_small_negative() {
    printf("Testing check_deficit with small negative balance...\n");

    TreasuryState ts;
    ts.balance = -1000;

    auto result = check_deficit(ts, 0);

    assert(result.should_warn == false);
    assert(result.should_offer_bond == false);

    printf("  PASS: No warnings for -1000 (above threshold)\n");
}

// ---------------------------------------------------------------------------
// Warning threshold
// ---------------------------------------------------------------------------

void test_warning_at_threshold() {
    printf("Testing check_deficit at warning threshold (-5000)...\n");

    TreasuryState ts;
    ts.balance = -5000;

    auto result = check_deficit(ts, 0);

    // -5000 is not less than -5000, so no warning
    assert(result.should_warn == false);

    printf("  PASS: No warning at exactly -5000 (not less than)\n");
}

void test_warning_below_threshold() {
    printf("Testing check_deficit below warning threshold (-5001)...\n");

    TreasuryState ts;
    ts.balance = -5001;

    auto result = check_deficit(ts, 1);

    assert(result.should_warn == true);
    assert(result.should_offer_bond == false);
    assert(result.warning_event.player_id == 1);
    assert(result.warning_event.balance == -5001);

    printf("  PASS: Warning triggered at -5001\n");
}

void test_warning_at_minus_7000() {
    printf("Testing check_deficit at -7000...\n");

    TreasuryState ts;
    ts.balance = -7000;

    auto result = check_deficit(ts, 2);

    assert(result.should_warn == true);
    assert(result.should_offer_bond == false);
    assert(result.warning_event.player_id == 2);
    assert(result.warning_event.balance == -7000);

    printf("  PASS: Warning but no emergency bond at -7000\n");
}

// ---------------------------------------------------------------------------
// Emergency bond threshold
// ---------------------------------------------------------------------------

void test_emergency_at_threshold() {
    printf("Testing check_deficit at emergency threshold (-10000)...\n");

    TreasuryState ts;
    ts.balance = -10000;

    auto result = check_deficit(ts, 0);

    // -10000 is not less than -10000, so no emergency bond
    assert(result.should_warn == true);  // still below warning threshold
    assert(result.should_offer_bond == false);

    printf("  PASS: No emergency bond at exactly -10000\n");
}

void test_emergency_below_threshold() {
    printf("Testing check_deficit below emergency threshold (-10001)...\n");

    TreasuryState ts;
    ts.balance = -10001;

    auto result = check_deficit(ts, 3);

    assert(result.should_warn == true);
    assert(result.should_offer_bond == true);
    assert(result.bond_event.player_id == 3);
    assert(result.bond_event.bond_principal == 25000); // BOND_EMERGENCY.principal

    printf("  PASS: Both warning and emergency bond at -10001\n");
}

void test_emergency_very_negative() {
    printf("Testing check_deficit at very negative balance (-50000)...\n");

    TreasuryState ts;
    ts.balance = -50000;

    auto result = check_deficit(ts, 0);

    assert(result.should_warn == true);
    assert(result.should_offer_bond == true);

    printf("  PASS: Both warning and emergency bond at -50000\n");
}

// ---------------------------------------------------------------------------
// Already warned (no re-warn)
// ---------------------------------------------------------------------------

void test_no_rewarn_when_already_warned() {
    printf("Testing check_deficit when already warned...\n");

    TreasuryState ts;
    ts.balance = -7000;
    ts.deficit_warning_sent = true;

    auto result = check_deficit(ts, 0);

    assert(result.should_warn == false);
    assert(result.should_offer_bond == false);

    printf("  PASS: No re-warn when deficit_warning_sent is already true\n");
}

void test_no_rebond_when_already_active() {
    printf("Testing check_deficit when emergency bond already active...\n");

    TreasuryState ts;
    ts.balance = -15000;
    ts.deficit_warning_sent = true;
    ts.emergency_bond_active = true;

    auto result = check_deficit(ts, 0);

    assert(result.should_warn == false);
    assert(result.should_offer_bond == false);

    printf("  PASS: No re-offer when emergency_bond_active is true\n");
}

void test_bond_offered_when_warned_but_no_bond() {
    printf("Testing check_deficit: warned but no bond active yet...\n");

    TreasuryState ts;
    ts.balance = -15000;
    ts.deficit_warning_sent = true;
    ts.emergency_bond_active = false;

    auto result = check_deficit(ts, 0);

    assert(result.should_warn == false);      // already warned
    assert(result.should_offer_bond == true);  // bond not yet offered

    printf("  PASS: Bond offered even though warning already sent\n");
}

// ---------------------------------------------------------------------------
// apply_deficit_state
// ---------------------------------------------------------------------------

void test_apply_deficit_state_warning() {
    printf("Testing apply_deficit_state with warning...\n");

    TreasuryState ts;
    ts.balance = -6000;

    auto result = check_deficit(ts, 0);
    assert(result.should_warn == true);

    apply_deficit_state(ts, result);
    assert(ts.deficit_warning_sent == true);
    assert(ts.emergency_bond_active == false);

    printf("  PASS: deficit_warning_sent set to true\n");
}

void test_apply_deficit_state_both() {
    printf("Testing apply_deficit_state with both warning and bond...\n");

    TreasuryState ts;
    ts.balance = -15000;

    auto result = check_deficit(ts, 0);
    assert(result.should_warn == true);
    assert(result.should_offer_bond == true);

    apply_deficit_state(ts, result);
    assert(ts.deficit_warning_sent == true);
    assert(ts.emergency_bond_active == true);

    printf("  PASS: Both flags set to true\n");
}

void test_apply_deficit_state_no_action() {
    printf("Testing apply_deficit_state with no action needed...\n");

    TreasuryState ts;
    ts.balance = 5000;

    auto result = check_deficit(ts, 0);
    assert(result.should_warn == false);
    assert(result.should_offer_bond == false);

    apply_deficit_state(ts, result);
    assert(ts.deficit_warning_sent == false);
    assert(ts.emergency_bond_active == false);

    printf("  PASS: No flags changed when no action needed\n");
}

// ---------------------------------------------------------------------------
// Recovery
// ---------------------------------------------------------------------------

void test_recovery_resets_flags() {
    printf("Testing check_deficit_recovery resets flags...\n");

    TreasuryState ts;
    ts.balance = 0;
    ts.deficit_warning_sent = true;
    ts.emergency_bond_active = true;

    check_deficit_recovery(ts);

    assert(ts.deficit_warning_sent == false);
    assert(ts.emergency_bond_active == false);

    printf("  PASS: Flags reset when balance >= 0\n");
}

void test_recovery_positive_balance() {
    printf("Testing check_deficit_recovery with positive balance...\n");

    TreasuryState ts;
    ts.balance = 5000;
    ts.deficit_warning_sent = true;
    ts.emergency_bond_active = true;

    check_deficit_recovery(ts);

    assert(ts.deficit_warning_sent == false);
    assert(ts.emergency_bond_active == false);

    printf("  PASS: Flags reset with positive balance\n");
}

void test_recovery_no_reset_when_negative() {
    printf("Testing check_deficit_recovery does NOT reset when negative...\n");

    TreasuryState ts;
    ts.balance = -1000;
    ts.deficit_warning_sent = true;
    ts.emergency_bond_active = true;

    check_deficit_recovery(ts);

    assert(ts.deficit_warning_sent == true);
    assert(ts.emergency_bond_active == true);

    printf("  PASS: Flags unchanged when balance is negative\n");
}

// ---------------------------------------------------------------------------
// Full cycle integration
// ---------------------------------------------------------------------------

void test_full_deficit_cycle() {
    printf("Testing full deficit cycle: deficit -> warn -> bond -> recovery...\n");

    TreasuryState ts;
    ts.balance = 20000;

    // Step 1: healthy
    auto r1 = check_deficit(ts, 0);
    assert(!r1.should_warn && !r1.should_offer_bond);

    // Step 2: drop below warning threshold
    ts.balance = -6000;
    auto r2 = check_deficit(ts, 0);
    assert(r2.should_warn && !r2.should_offer_bond);
    apply_deficit_state(ts, r2);

    // Step 3: still in deficit, but already warned
    ts.balance = -8000;
    auto r3 = check_deficit(ts, 0);
    assert(!r3.should_warn && !r3.should_offer_bond);

    // Step 4: drop below emergency threshold
    ts.balance = -12000;
    auto r4 = check_deficit(ts, 0);
    assert(!r4.should_warn && r4.should_offer_bond);
    apply_deficit_state(ts, r4);

    // Step 5: still in deep deficit, both flags set
    ts.balance = -20000;
    auto r5 = check_deficit(ts, 0);
    assert(!r5.should_warn && !r5.should_offer_bond);

    // Step 6: recover
    ts.balance = 100;
    check_deficit_recovery(ts);
    assert(!ts.deficit_warning_sent && !ts.emergency_bond_active);

    // Step 7: can warn again after recovery
    ts.balance = -6000;
    auto r7 = check_deficit(ts, 0);
    assert(r7.should_warn && !r7.should_offer_bond);

    printf("  PASS: Full deficit cycle works correctly\n");
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main() {
    printf("=== DeficitHandling Unit Tests (E11-015) ===\n\n");

    // Constants
    test_constants();

    // No deficit
    test_no_deficit_positive_balance();
    test_no_deficit_zero_balance();
    test_no_deficit_small_negative();

    // Warning threshold
    test_warning_at_threshold();
    test_warning_below_threshold();
    test_warning_at_minus_7000();

    // Emergency bond threshold
    test_emergency_at_threshold();
    test_emergency_below_threshold();
    test_emergency_very_negative();

    // Already warned
    test_no_rewarn_when_already_warned();
    test_no_rebond_when_already_active();
    test_bond_offered_when_warned_but_no_bond();

    // apply_deficit_state
    test_apply_deficit_state_warning();
    test_apply_deficit_state_both();
    test_apply_deficit_state_no_action();

    // Recovery
    test_recovery_resets_flags();
    test_recovery_positive_balance();
    test_recovery_no_reset_when_negative();

    // Full cycle
    test_full_deficit_cycle();

    printf("\n=== All DeficitHandling tests passed! ===\n");
    return 0;
}
