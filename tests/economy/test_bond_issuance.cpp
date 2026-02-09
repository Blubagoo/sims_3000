/**
 * @file test_bond_issuance.cpp
 * @brief Unit tests for BondIssuance (E11-016)
 *
 * Tests: get_bond_config, can_issue_bond validation, issue_bond execution,
 *        max bonds check, large bond population requirement, emergency not manual,
 *        principal added, bond struct fields, event data.
 */

#include "sims3000/economy/BondIssuance.h"
#include <cassert>
#include <cstdio>

using namespace sims3000::economy;

// ---------------------------------------------------------------------------
// get_bond_config
// ---------------------------------------------------------------------------

void test_get_bond_config_small() {
    printf("Testing get_bond_config(Small)...\n");

    const auto& cfg = get_bond_config(BondType::Small);
    assert(cfg.principal == 5000);
    assert(cfg.interest_rate == 500);
    assert(cfg.term_phases == 12);
    assert(cfg.is_emergency == false);

    printf("  PASS: Small bond config correct\n");
}

void test_get_bond_config_standard() {
    printf("Testing get_bond_config(Standard)...\n");

    const auto& cfg = get_bond_config(BondType::Standard);
    assert(cfg.principal == 25000);
    assert(cfg.interest_rate == 750);
    assert(cfg.term_phases == 24);
    assert(cfg.is_emergency == false);

    printf("  PASS: Standard bond config correct\n");
}

void test_get_bond_config_large() {
    printf("Testing get_bond_config(Large)...\n");

    const auto& cfg = get_bond_config(BondType::Large);
    assert(cfg.principal == 100000);
    assert(cfg.interest_rate == 1000);
    assert(cfg.term_phases == 48);
    assert(cfg.is_emergency == false);

    printf("  PASS: Large bond config correct\n");
}

void test_get_bond_config_emergency() {
    printf("Testing get_bond_config(Emergency)...\n");

    const auto& cfg = get_bond_config(BondType::Emergency);
    assert(cfg.principal == 25000);
    assert(cfg.interest_rate == 1500);
    assert(cfg.term_phases == 12);
    assert(cfg.is_emergency == true);

    printf("  PASS: Emergency bond config correct\n");
}

// ---------------------------------------------------------------------------
// can_issue_bond: basic validation
// ---------------------------------------------------------------------------

void test_can_issue_small_bond() {
    printf("Testing can_issue_bond(Small)...\n");

    TreasuryState ts;
    assert(can_issue_bond(ts, BondType::Small) == true);

    printf("  PASS: Can issue small bond with empty treasury\n");
}

void test_can_issue_standard_bond() {
    printf("Testing can_issue_bond(Standard)...\n");

    TreasuryState ts;
    assert(can_issue_bond(ts, BondType::Standard) == true);

    printf("  PASS: Can issue standard bond\n");
}

// ---------------------------------------------------------------------------
// Emergency bonds cannot be issued manually
// ---------------------------------------------------------------------------

void test_cannot_issue_emergency_manually() {
    printf("Testing emergency bond cannot be issued manually...\n");

    TreasuryState ts;
    assert(can_issue_bond(ts, BondType::Emergency) == false);

    printf("  PASS: Emergency bond rejected for manual issuance\n");
}

// ---------------------------------------------------------------------------
// Large bond population requirement
// ---------------------------------------------------------------------------

void test_large_bond_requires_population() {
    printf("Testing large bond requires population > 5000...\n");

    TreasuryState ts;

    // population = 0 (default)
    assert(can_issue_bond(ts, BondType::Large, 0) == false);

    // population = 5000 (at threshold, not above)
    assert(can_issue_bond(ts, BondType::Large, 5000) == false);

    // population = 5001 (above threshold)
    assert(can_issue_bond(ts, BondType::Large, 5001) == true);

    // population = 10000 (well above)
    assert(can_issue_bond(ts, BondType::Large, 10000) == true);

    printf("  PASS: Large bond population requirement enforced\n");
}

// ---------------------------------------------------------------------------
// Max bonds per player
// ---------------------------------------------------------------------------

void test_max_bonds_per_player() {
    printf("Testing max bonds per player (%d)...\n", MAX_BONDS_PER_PLAYER);

    TreasuryState ts;

    // Fill up to max
    for (int i = 0; i < MAX_BONDS_PER_PLAYER; ++i) {
        ts.active_bonds.push_back(CreditAdvance{});
    }

    assert(can_issue_bond(ts, BondType::Small) == false);
    assert(can_issue_bond(ts, BondType::Standard) == false);
    assert(can_issue_bond(ts, BondType::Large, 10000) == false);

    printf("  PASS: Cannot issue bonds when at max (%d)\n", MAX_BONDS_PER_PLAYER);
}

void test_can_issue_below_max() {
    printf("Testing can issue when below max bonds...\n");

    TreasuryState ts;

    // Fill to one below max
    for (int i = 0; i < MAX_BONDS_PER_PLAYER - 1; ++i) {
        ts.active_bonds.push_back(CreditAdvance{});
    }

    assert(can_issue_bond(ts, BondType::Small) == true);

    printf("  PASS: Can issue bond when at %d/%d\n", MAX_BONDS_PER_PLAYER - 1, MAX_BONDS_PER_PLAYER);
}

// ---------------------------------------------------------------------------
// issue_bond: successful issuance
// ---------------------------------------------------------------------------

void test_issue_small_bond() {
    printf("Testing issue_bond(Small)...\n");

    TreasuryState ts;
    ts.balance = 10000;

    auto result = issue_bond(ts, BondType::Small, 1);

    assert(result.success == true);
    assert(result.principal_added == 5000);
    assert(result.bond.principal == 5000);
    assert(result.bond.remaining_principal == 5000);
    assert(result.bond.interest_rate_basis_points == 500);
    assert(result.bond.term_phases == 12);
    assert(result.bond.phases_remaining == 12);
    assert(result.bond.is_emergency == false);

    // Treasury updated
    assert(ts.balance == 15000); // 10000 + 5000
    assert(ts.active_bonds.size() == 1);
    assert(ts.active_bonds[0].principal == 5000);

    printf("  PASS: Small bond issued, principal added to treasury\n");
}

void test_issue_standard_bond() {
    printf("Testing issue_bond(Standard)...\n");

    TreasuryState ts;
    ts.balance = 0;

    auto result = issue_bond(ts, BondType::Standard, 2);

    assert(result.success == true);
    assert(result.principal_added == 25000);
    assert(ts.balance == 25000);
    assert(ts.active_bonds.size() == 1);

    printf("  PASS: Standard bond issued\n");
}

void test_issue_large_bond_with_population() {
    printf("Testing issue_bond(Large) with sufficient population...\n");

    TreasuryState ts;
    ts.balance = 5000;

    auto result = issue_bond(ts, BondType::Large, 0, 6000);

    assert(result.success == true);
    assert(result.principal_added == 100000);
    assert(ts.balance == 105000);
    assert(ts.active_bonds.size() == 1);
    assert(ts.active_bonds[0].principal == 100000);

    printf("  PASS: Large bond issued with population 6000\n");
}

// ---------------------------------------------------------------------------
// issue_bond: validation failures
// ---------------------------------------------------------------------------

void test_issue_emergency_fails() {
    printf("Testing issue_bond(Emergency) fails manually...\n");

    TreasuryState ts;
    int64_t original_balance = ts.balance;

    auto result = issue_bond(ts, BondType::Emergency, 0);

    assert(result.success == false);
    assert(result.principal_added == 0);
    assert(ts.balance == original_balance);
    assert(ts.active_bonds.empty());

    printf("  PASS: Emergency bond issuance rejected\n");
}

void test_issue_large_without_population_fails() {
    printf("Testing issue_bond(Large) fails without population...\n");

    TreasuryState ts;
    int64_t original_balance = ts.balance;

    auto result = issue_bond(ts, BondType::Large, 0, 3000);

    assert(result.success == false);
    assert(ts.balance == original_balance);
    assert(ts.active_bonds.empty());

    printf("  PASS: Large bond rejected with population 3000\n");
}

void test_issue_at_max_bonds_fails() {
    printf("Testing issue_bond at max bonds fails...\n");

    TreasuryState ts;
    for (int i = 0; i < MAX_BONDS_PER_PLAYER; ++i) {
        ts.active_bonds.push_back(CreditAdvance{});
    }
    int64_t original_balance = ts.balance;

    auto result = issue_bond(ts, BondType::Small, 0);

    assert(result.success == false);
    assert(ts.balance == original_balance);
    assert(static_cast<int>(ts.active_bonds.size()) == MAX_BONDS_PER_PLAYER);

    printf("  PASS: Bond issuance rejected at max\n");
}

// ---------------------------------------------------------------------------
// Multiple bond issuance
// ---------------------------------------------------------------------------

void test_multiple_bond_issuance() {
    printf("Testing multiple bond issuance...\n");

    TreasuryState ts;
    ts.balance = 0;

    // Issue 3 small bonds
    for (int i = 0; i < 3; ++i) {
        auto result = issue_bond(ts, BondType::Small, 0);
        assert(result.success == true);
    }

    assert(ts.balance == 15000); // 3 * 5000
    assert(ts.active_bonds.size() == 3);

    printf("  PASS: 3 small bonds issued, balance = 15000\n");
}

// ---------------------------------------------------------------------------
// Bond struct fields correctness
// ---------------------------------------------------------------------------

void test_bond_struct_fields() {
    printf("Testing bond struct fields after issuance...\n");

    TreasuryState ts;
    auto result = issue_bond(ts, BondType::Standard, 5);

    assert(result.success == true);
    const auto& bond = result.bond;
    assert(bond.principal == 25000);
    assert(bond.remaining_principal == 25000);
    assert(bond.interest_rate_basis_points == 750);
    assert(bond.term_phases == 24);
    assert(bond.phases_remaining == 24);
    assert(bond.is_emergency == false);

    printf("  PASS: All bond struct fields correctly populated\n");
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main() {
    printf("=== BondIssuance Unit Tests (E11-016) ===\n\n");

    // Bond configs
    test_get_bond_config_small();
    test_get_bond_config_standard();
    test_get_bond_config_large();
    test_get_bond_config_emergency();

    // Validation
    test_can_issue_small_bond();
    test_can_issue_standard_bond();
    test_cannot_issue_emergency_manually();
    test_large_bond_requires_population();
    test_max_bonds_per_player();
    test_can_issue_below_max();

    // Issuance
    test_issue_small_bond();
    test_issue_standard_bond();
    test_issue_large_bond_with_population();

    // Validation failures
    test_issue_emergency_fails();
    test_issue_large_without_population_fails();
    test_issue_at_max_bonds_fails();

    // Multiple issuance
    test_multiple_bond_issuance();

    // Field correctness
    test_bond_struct_fields();

    printf("\n=== All BondIssuance tests passed! ===\n");
    return 0;
}
