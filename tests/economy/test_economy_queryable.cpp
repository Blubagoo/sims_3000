/**
 * @file test_economy_queryable.cpp
 * @brief Unit tests for IEconomyQueryable real implementation (E11-005)
 *
 * Validates:
 * - EconomySystem implements IEconomyQueryable (polymorphism)
 * - Tribute rate queries return real TreasuryState values
 * - Treasury balance queries
 * - can_afford checks
 * - Funding level queries (per service type)
 * - Statistics queries (last_income, last_expense)
 * - Bond queries (total_debt, bond_count, can_issue_bond)
 * - ICreditProvider (deduct_credits, has_credits)
 * - StubEconomyQueryable updated interface
 */

#include <cassert>
#include <cmath>
#include <cstdio>
#include <memory>

#include "sims3000/economy/EconomySystem.h"
#include "sims3000/economy/IEconomyQueryable.h"
#include "sims3000/economy/StubEconomyQueryable.h"
#include "sims3000/economy/CreditAdvance.h"
#include "sims3000/building/ForwardDependencyInterfaces.h"

using namespace sims3000;
using namespace sims3000::economy;

// --------------------------------------------------------------------------
// Test: EconomySystem implements IEconomyQueryable via polymorphism
// --------------------------------------------------------------------------
static void test_polymorphism() {
    EconomySystem system;
    system.activate_player(0);

    IEconomyQueryable* queryable = &system;

    // Should work through base pointer
    float rate = queryable->get_tribute_rate(0);
    assert(std::fabs(rate - 7.0f) < 0.001f && "Default tribute rate should be 7.0");

    std::printf("  PASS: EconomySystem implements IEconomyQueryable\n");
}

// --------------------------------------------------------------------------
// Test: Tribute rate queries return real values
// --------------------------------------------------------------------------
static void test_tribute_rates() {
    EconomySystem system;
    system.activate_player(0);

    // Default rates: all 7%
    assert(std::fabs(system.get_tribute_rate(0) - 7.0f) < 0.001f && "Habitation default 7%");
    assert(std::fabs(system.get_tribute_rate(1) - 7.0f) < 0.001f && "Exchange default 7%");
    assert(std::fabs(system.get_tribute_rate(2) - 7.0f) < 0.001f && "Fabrication default 7%");

    // Modify rates directly on treasury
    system.get_treasury(0).tribute_rate_habitation = 10;
    system.get_treasury(0).tribute_rate_exchange = 5;
    system.get_treasury(0).tribute_rate_fabrication = 15;

    assert(std::fabs(system.get_tribute_rate(0) - 10.0f) < 0.001f && "Habitation should be 10%");
    assert(std::fabs(system.get_tribute_rate(1) - 5.0f) < 0.001f && "Exchange should be 5%");
    assert(std::fabs(system.get_tribute_rate(2) - 15.0f) < 0.001f && "Fabrication should be 15%");

    // Unknown zone type returns default 7.0
    assert(std::fabs(system.get_tribute_rate(3) - 7.0f) < 0.001f && "Unknown zone type should be 7%");

    std::printf("  PASS: tribute rate queries return real values\n");
}

// --------------------------------------------------------------------------
// Test: Per-player tribute rate queries
// --------------------------------------------------------------------------
static void test_tribute_rates_per_player() {
    EconomySystem system;
    system.activate_player(0);
    system.activate_player(1);

    // Set different rates for each player
    system.get_treasury(0).tribute_rate_habitation = 10;
    system.get_treasury(1).tribute_rate_habitation = 15;

    assert(std::fabs(system.get_tribute_rate(0, 0) - 10.0f) < 0.001f && "Player 0 habitation 10%");
    assert(std::fabs(system.get_tribute_rate(0, 1) - 15.0f) < 0.001f && "Player 1 habitation 15%");

    // Default overload uses player 0
    assert(std::fabs(system.get_tribute_rate(0) - 10.0f) < 0.001f && "Default player 0");

    std::printf("  PASS: per-player tribute rate queries\n");
}

// --------------------------------------------------------------------------
// Test: Average tribute rate
// --------------------------------------------------------------------------
static void test_average_tribute_rate() {
    EconomySystem system;
    system.activate_player(0);

    // Default: (7 + 7 + 7) / 3 = 7.0
    assert(std::fabs(system.get_average_tribute_rate() - 7.0f) < 0.001f && "Default average 7%");

    // Set different rates: (10 + 5 + 15) / 3 = 10.0
    system.get_treasury(0).tribute_rate_habitation = 10;
    system.get_treasury(0).tribute_rate_exchange = 5;
    system.get_treasury(0).tribute_rate_fabrication = 15;

    assert(std::fabs(system.get_average_tribute_rate() - 10.0f) < 0.001f && "Average should be 10%");

    std::printf("  PASS: average tribute rate\n");
}

// --------------------------------------------------------------------------
// Test: Treasury balance queries
// --------------------------------------------------------------------------
static void test_treasury_balance() {
    EconomySystem system;
    system.activate_player(0);

    assert(system.get_treasury_balance(0) == 20000 && "Default balance 20000");

    system.get_treasury(0).balance = 50000;
    assert(system.get_treasury_balance(0) == 50000 && "Modified balance 50000");

    // Invalid player returns 0
    assert(system.get_treasury_balance(5) == 0 && "Invalid player returns 0");

    std::printf("  PASS: treasury balance queries\n");
}

// --------------------------------------------------------------------------
// Test: can_afford checks
// --------------------------------------------------------------------------
static void test_can_afford() {
    EconomySystem system;
    system.activate_player(0);

    // Default balance 20000
    assert(system.can_afford(20000, 0) && "Can afford exactly 20000");
    assert(system.can_afford(10000, 0) && "Can afford less than balance");
    assert(!system.can_afford(20001, 0) && "Cannot afford more than balance");
    assert(system.can_afford(0, 0) && "Can afford 0");
    assert(system.can_afford(-100, 0) && "Can afford negative amount");

    // Invalid player
    assert(!system.can_afford(1, 5) && "Invalid player cannot afford anything");

    std::printf("  PASS: can_afford checks\n");
}

// --------------------------------------------------------------------------
// Test: Funding level queries
// --------------------------------------------------------------------------
static void test_funding_levels() {
    EconomySystem system;
    system.activate_player(0);

    // Defaults: all 100%
    assert(system.get_funding_level(0, 0) == 100 && "Enforcer default 100%");
    assert(system.get_funding_level(1, 0) == 100 && "HazardResponse default 100%");
    assert(system.get_funding_level(2, 0) == 100 && "Medical default 100%");
    assert(system.get_funding_level(3, 0) == 100 && "Education default 100%");

    // Modify funding levels
    system.get_treasury(0).funding_enforcer = 50;
    system.get_treasury(0).funding_hazard_response = 75;
    system.get_treasury(0).funding_medical = 120;
    system.get_treasury(0).funding_education = 150;

    assert(system.get_funding_level(0, 0) == 50 && "Enforcer should be 50%");
    assert(system.get_funding_level(1, 0) == 75 && "HazardResponse should be 75%");
    assert(system.get_funding_level(2, 0) == 120 && "Medical should be 120%");
    assert(system.get_funding_level(3, 0) == 150 && "Education should be 150%");

    // Unknown service type returns default
    assert(system.get_funding_level(4, 0) == 100 && "Unknown service type default 100%");

    // Invalid player returns default
    assert(system.get_funding_level(0, 5) == 100 && "Invalid player default 100%");

    std::printf("  PASS: funding level queries\n");
}

// --------------------------------------------------------------------------
// Test: Statistics queries (last_income, last_expense)
// --------------------------------------------------------------------------
static void test_statistics() {
    EconomySystem system;
    system.activate_player(0);

    // Defaults: 0
    assert(system.get_last_income(0) == 0 && "Default last income 0");
    assert(system.get_last_expense(0) == 0 && "Default last expense 0");

    // Modify
    system.get_treasury(0).last_income = 5000;
    system.get_treasury(0).last_expense = 3000;

    assert(system.get_last_income(0) == 5000 && "Last income 5000");
    assert(system.get_last_expense(0) == 3000 && "Last expense 3000");

    // Invalid player returns 0
    assert(system.get_last_income(5) == 0 && "Invalid player income 0");
    assert(system.get_last_expense(5) == 0 && "Invalid player expense 0");

    std::printf("  PASS: statistics queries\n");
}

// --------------------------------------------------------------------------
// Test: Bond queries
// --------------------------------------------------------------------------
static void test_bond_queries() {
    EconomySystem system;
    system.activate_player(0);

    // Defaults: no bonds
    assert(system.get_total_debt(0) == 0 && "Default total debt 0");
    assert(system.get_bond_count(0) == 0 && "Default bond count 0");
    assert(system.can_issue_bond(0) && "Can issue bond with no existing bonds");

    // Add some bonds
    CreditAdvance bond1;
    bond1.principal = 5000;
    bond1.remaining_principal = 4000;
    bond1.interest_rate_basis_points = 500;
    bond1.term_phases = 12;
    bond1.phases_remaining = 10;
    bond1.is_emergency = false;

    CreditAdvance bond2;
    bond2.principal = 25000;
    bond2.remaining_principal = 20000;
    bond2.interest_rate_basis_points = 750;
    bond2.term_phases = 24;
    bond2.phases_remaining = 20;
    bond2.is_emergency = false;

    system.get_treasury(0).active_bonds.push_back(bond1);
    system.get_treasury(0).active_bonds.push_back(bond2);

    assert(system.get_total_debt(0) == 24000 && "Total debt = 4000 + 20000");
    assert(system.get_bond_count(0) == 2 && "Bond count should be 2");
    assert(system.can_issue_bond(0) && "Can still issue bonds (2 < 5)");

    // Fill to MAX_BONDS_PER_PLAYER (5)
    system.get_treasury(0).active_bonds.push_back(bond1);
    system.get_treasury(0).active_bonds.push_back(bond1);
    system.get_treasury(0).active_bonds.push_back(bond1);

    assert(system.get_bond_count(0) == 5 && "Bond count should be 5");
    assert(!system.can_issue_bond(0) && "Cannot issue bond at max capacity");

    // Invalid player
    assert(system.get_total_debt(5) == 0 && "Invalid player debt 0");
    assert(system.get_bond_count(5) == 0 && "Invalid player bond count 0");
    assert(!system.can_issue_bond(5) && "Invalid player cannot issue bond");

    std::printf("  PASS: bond queries\n");
}

// --------------------------------------------------------------------------
// Test: ICreditProvider - deduct_credits
// --------------------------------------------------------------------------
static void test_deduct_credits() {
    EconomySystem system;
    system.activate_player(0);

    building::ICreditProvider* provider = &system;

    // Deduct from default 20000
    bool result = provider->deduct_credits(0, 5000);
    assert(result && "deduct_credits should return true");
    assert(system.get_treasury_balance(0) == 15000 && "Balance should be 15000 after deduction");

    // Reject insufficient funds (E11-GD-002: no deficit spending)
    result = provider->deduct_credits(0, 20000);
    assert(!result && "deduct_credits should reject insufficient funds");
    assert(system.get_treasury_balance(0) == 15000 && "Balance should remain 15000 after rejection");

    // Invalid player
    result = provider->deduct_credits(5, 100);
    assert(!result && "Invalid player deduction should fail");

    std::printf("  PASS: ICreditProvider deduct_credits\n");
}

// --------------------------------------------------------------------------
// Test: ICreditProvider - has_credits
// --------------------------------------------------------------------------
static void test_has_credits() {
    EconomySystem system;
    system.activate_player(0);

    building::ICreditProvider* provider = &system;

    assert(provider->has_credits(0, 20000) && "Has exactly 20000");
    assert(provider->has_credits(0, 10000) && "Has more than 10000");
    assert(!provider->has_credits(0, 20001) && "Does not have 20001");

    // Invalid player
    assert(!provider->has_credits(5, 0) && "Invalid player has no credits");

    std::printf("  PASS: ICreditProvider has_credits\n");
}

// --------------------------------------------------------------------------
// Test: StubEconomyQueryable implements expanded interface
// --------------------------------------------------------------------------
static void test_stub_expanded_interface() {
    StubEconomyQueryable stub;
    IEconomyQueryable* queryable = &stub;

    // Original methods
    assert(std::fabs(queryable->get_tribute_rate(0) - 7.0f) < 0.001f);
    assert(std::fabs(queryable->get_tribute_rate(0, 0) - 7.0f) < 0.001f);
    assert(std::fabs(queryable->get_average_tribute_rate() - 7.0f) < 0.001f);

    // New methods
    assert(queryable->get_treasury_balance(0) == 20000);
    assert(queryable->can_afford(20000, 0));
    assert(!queryable->can_afford(20001, 0));
    assert(queryable->get_funding_level(0, 0) == 100);
    assert(queryable->get_last_income(0) == 0);
    assert(queryable->get_last_expense(0) == 0);
    assert(queryable->get_total_debt(0) == 0);
    assert(queryable->get_bond_count(0) == 0);
    assert(queryable->can_issue_bond(0));

    std::printf("  PASS: StubEconomyQueryable implements expanded interface\n");
}

// --------------------------------------------------------------------------
// Test: EconomySystem as IEconomyQueryable via unique_ptr
// --------------------------------------------------------------------------
static void test_unique_ptr_polymorphism() {
    auto system = std::make_unique<EconomySystem>();
    system->activate_player(0);

    // Cast to IEconomyQueryable
    IEconomyQueryable* queryable = system.get();
    assert(std::fabs(queryable->get_tribute_rate(0) - 7.0f) < 0.001f);
    assert(queryable->get_treasury_balance(0) == 20000);
    assert(queryable->can_afford(10000, 0));
    assert(queryable->get_funding_level(0, 0) == 100);

    std::printf("  PASS: EconomySystem as IEconomyQueryable via unique_ptr\n");
}

// --------------------------------------------------------------------------
// Test: ICreditProvider via unique_ptr
// --------------------------------------------------------------------------
static void test_credit_provider_polymorphism() {
    auto system = std::make_unique<EconomySystem>();
    system->activate_player(0);

    building::ICreditProvider* provider = system.get();
    assert(provider->has_credits(0, 20000));
    assert(provider->deduct_credits(0, 1000));
    assert(system->get_treasury_balance(0) == 19000);

    std::printf("  PASS: ICreditProvider via unique_ptr\n");
}

// --------------------------------------------------------------------------
// Main
// --------------------------------------------------------------------------
int main() {
    std::printf("=== EconomyQueryable Unit Tests (E11-005) ===\n\n");

    test_polymorphism();
    test_tribute_rates();
    test_tribute_rates_per_player();
    test_average_tribute_rate();
    test_treasury_balance();
    test_can_afford();
    test_funding_levels();
    test_statistics();
    test_bond_queries();
    test_deduct_credits();
    test_has_credits();
    test_stub_expanded_interface();
    test_unique_ptr_polymorphism();
    test_credit_provider_polymorphism();

    std::printf("\n=== All EconomyQueryable tests passed! ===\n");
    return 0;
}
