/**
 * @file test_credit_advance.cpp
 * @brief Unit tests for CreditAdvance and BondConfig (E11-003)
 */

#include "sims3000/economy/CreditAdvance.h"
#include <cassert>
#include <cstdio>
#include <type_traits>

using namespace sims3000::economy;

void test_credit_advance_trivially_copyable() {
    printf("Testing CreditAdvance is trivially copyable...\n");

    static_assert(std::is_trivially_copyable<CreditAdvance>::value,
                  "CreditAdvance must be trivially copyable");

    printf("  PASS: CreditAdvance is trivially copyable\n");
}

void test_credit_advance_size() {
    printf("Testing CreditAdvance size...\n");

    assert(sizeof(CreditAdvance) <= 32);

    printf("  PASS: CreditAdvance size is %zu bytes (<= 32)\n",
           sizeof(CreditAdvance));
}

void test_credit_advance_defaults() {
    printf("Testing CreditAdvance default values...\n");

    CreditAdvance ca;

    assert(ca.principal == 0);
    assert(ca.remaining_principal == 0);
    assert(ca.interest_rate_basis_points == 0);
    assert(ca.term_phases == 0);
    assert(ca.phases_remaining == 0);
    assert(ca.is_emergency == false);

    printf("  PASS: CreditAdvance defaults are correct\n");
}

void test_bond_config_small() {
    printf("Testing BOND_SMALL config...\n");

    assert(BOND_SMALL.principal == 5000);
    assert(BOND_SMALL.interest_rate == 500);
    assert(BOND_SMALL.term_phases == 12);
    assert(BOND_SMALL.is_emergency == false);

    printf("  PASS: BOND_SMALL values are correct\n");
}

void test_bond_config_standard() {
    printf("Testing BOND_STANDARD config...\n");

    assert(BOND_STANDARD.principal == 25000);
    assert(BOND_STANDARD.interest_rate == 750);
    assert(BOND_STANDARD.term_phases == 24);
    assert(BOND_STANDARD.is_emergency == false);

    printf("  PASS: BOND_STANDARD values are correct\n");
}

void test_bond_config_large() {
    printf("Testing BOND_LARGE config...\n");

    assert(BOND_LARGE.principal == 100000);
    assert(BOND_LARGE.interest_rate == 1000);
    assert(BOND_LARGE.term_phases == 48);
    assert(BOND_LARGE.is_emergency == false);

    printf("  PASS: BOND_LARGE values are correct\n");
}

void test_bond_config_emergency() {
    printf("Testing BOND_EMERGENCY config...\n");

    assert(BOND_EMERGENCY.principal == 25000);
    assert(BOND_EMERGENCY.interest_rate == 1500);
    assert(BOND_EMERGENCY.term_phases == 12);
    assert(BOND_EMERGENCY.is_emergency == true);

    printf("  PASS: BOND_EMERGENCY values are correct\n");
}

void test_max_bonds_per_player() {
    printf("Testing MAX_BONDS_PER_PLAYER...\n");

    assert(MAX_BONDS_PER_PLAYER == 5);

    printf("  PASS: MAX_BONDS_PER_PLAYER is 5\n");
}

void test_bond_type_enum() {
    printf("Testing BondType enum values...\n");

    assert(static_cast<uint8_t>(BondType::Small) == 0);
    assert(static_cast<uint8_t>(BondType::Standard) == 1);
    assert(static_cast<uint8_t>(BondType::Large) == 2);
    assert(static_cast<uint8_t>(BondType::Emergency) == 3);

    printf("  PASS: BondType enum values are correct\n");
}

int main() {
    printf("=== CreditAdvance Unit Tests (E11-003) ===\n\n");

    test_credit_advance_trivially_copyable();
    test_credit_advance_size();
    test_credit_advance_defaults();
    test_bond_config_small();
    test_bond_config_standard();
    test_bond_config_large();
    test_bond_config_emergency();
    test_max_bonds_per_player();
    test_bond_type_enum();

    printf("\n=== All tests passed! ===\n");
    return 0;
}
