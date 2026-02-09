/**
 * @file test_construction_cost.cpp
 * @brief Unit tests for ConstructionCost (E11-020)
 *
 * Tests: cost constants, afford check (yes/no), deduction, insufficient funds,
 *        exact balance, zero cost, multiple deductions, all building types.
 */

#include "sims3000/economy/ConstructionCost.h"
#include <cassert>
#include <cstdio>

using namespace sims3000::economy;

// ---------------------------------------------------------------------------
// Cost constants defined
// ---------------------------------------------------------------------------

void test_zone_cost_constants() {
    printf("Testing zone cost constants...\n");

    assert(construction_costs::ZONE_HABITATION_LOW == 100);
    assert(construction_costs::ZONE_HABITATION_HIGH == 500);
    assert(construction_costs::ZONE_EXCHANGE_LOW == 150);
    assert(construction_costs::ZONE_EXCHANGE_HIGH == 750);
    assert(construction_costs::ZONE_FABRICATION_LOW == 200);
    assert(construction_costs::ZONE_FABRICATION_HIGH == 1000);

    printf("  PASS: All zone cost constants correct\n");
}

void test_infrastructure_cost_constants() {
    printf("Testing infrastructure cost constants...\n");

    assert(construction_costs::PATHWAY == 10);
    assert(construction_costs::ENERGY_CONDUIT == 5);
    assert(construction_costs::FLUID_CONDUIT == 8);
    assert(construction_costs::RAIL_TRACK == 25);

    printf("  PASS: All infrastructure cost constants correct\n");
}

void test_service_cost_constants() {
    printf("Testing service building cost constants...\n");

    assert(construction_costs::SERVICE_POST == 500);
    assert(construction_costs::SERVICE_STATION == 2000);
    assert(construction_costs::SERVICE_NEXUS == 5000);

    printf("  PASS: All service building cost constants correct\n");
}

// ---------------------------------------------------------------------------
// check_construction_cost: can afford
// ---------------------------------------------------------------------------

void test_check_can_afford() {
    printf("Testing check_construction_cost - can afford...\n");

    TreasuryState ts;
    ts.balance = 10000;

    auto result = check_construction_cost(ts, 5000);

    assert(result.can_afford == true);
    assert(result.cost == 5000);
    assert(result.balance_after == 5000);

    printf("  PASS: Can afford 5000 with 10000 balance\n");
}

void test_check_cannot_afford() {
    printf("Testing check_construction_cost - cannot afford...\n");

    TreasuryState ts;
    ts.balance = 1000;

    auto result = check_construction_cost(ts, 5000);

    assert(result.can_afford == false);
    assert(result.cost == 5000);
    assert(result.balance_after == -4000); // projected negative

    printf("  PASS: Cannot afford 5000 with 1000 balance\n");
}

void test_check_exact_balance() {
    printf("Testing check_construction_cost - exact balance...\n");

    TreasuryState ts;
    ts.balance = 500;

    auto result = check_construction_cost(ts, 500);

    assert(result.can_afford == true);
    assert(result.cost == 500);
    assert(result.balance_after == 0);

    printf("  PASS: Can afford exactly 500 with 500 balance\n");
}

void test_check_zero_cost() {
    printf("Testing check_construction_cost - zero cost...\n");

    TreasuryState ts;
    ts.balance = 100;

    auto result = check_construction_cost(ts, 0);

    assert(result.can_afford == true);
    assert(result.cost == 0);
    assert(result.balance_after == 100);

    printf("  PASS: Zero cost always affordable\n");
}

// ---------------------------------------------------------------------------
// deduct_construction_cost
// ---------------------------------------------------------------------------

void test_deduct_success() {
    printf("Testing deduct_construction_cost - success...\n");

    TreasuryState ts;
    ts.balance = 20000;

    bool ok = deduct_construction_cost(ts, 5000);

    assert(ok == true);
    assert(ts.balance == 15000);

    printf("  PASS: Deducted 5000, balance now 15000\n");
}

void test_deduct_insufficient_funds() {
    printf("Testing deduct_construction_cost - insufficient funds...\n");

    TreasuryState ts;
    ts.balance = 100;

    bool ok = deduct_construction_cost(ts, 5000);

    assert(ok == false);
    assert(ts.balance == 100); // unchanged

    printf("  PASS: Deduction rejected, balance unchanged\n");
}

void test_deduct_exact_balance() {
    printf("Testing deduct_construction_cost - exact balance...\n");

    TreasuryState ts;
    ts.balance = 2000;

    bool ok = deduct_construction_cost(ts, 2000);

    assert(ok == true);
    assert(ts.balance == 0);

    printf("  PASS: Deducted exact balance, now 0\n");
}

void test_deduct_zero_cost() {
    printf("Testing deduct_construction_cost - zero cost...\n");

    TreasuryState ts;
    ts.balance = 500;

    bool ok = deduct_construction_cost(ts, 0);

    assert(ok == true);
    assert(ts.balance == 500);

    printf("  PASS: Zero cost deduction succeeds, balance unchanged\n");
}

// ---------------------------------------------------------------------------
// Multiple deductions
// ---------------------------------------------------------------------------

void test_multiple_deductions() {
    printf("Testing multiple deductions...\n");

    TreasuryState ts;
    ts.balance = 1000;

    // Deduct pathway costs (10 each)
    for (int i = 0; i < 5; ++i) {
        bool ok = deduct_construction_cost(ts, construction_costs::PATHWAY);
        assert(ok == true);
    }
    assert(ts.balance == 950); // 1000 - 50

    // Deduct a service post (500)
    bool ok = deduct_construction_cost(ts, construction_costs::SERVICE_POST);
    assert(ok == true);
    assert(ts.balance == 450); // 950 - 500

    // Try to deduct a service station (2000) - should fail
    ok = deduct_construction_cost(ts, construction_costs::SERVICE_STATION);
    assert(ok == false);
    assert(ts.balance == 450); // unchanged

    printf("  PASS: Multiple deductions work correctly\n");
}

// ---------------------------------------------------------------------------
// All building types via constants
// ---------------------------------------------------------------------------

void test_all_building_type_costs() {
    printf("Testing all building type costs via check...\n");

    TreasuryState ts;
    ts.balance = 100000;

    // Test each cost constant can be used with check
    auto r1 = check_construction_cost(ts, construction_costs::ZONE_HABITATION_LOW);
    assert(r1.can_afford && r1.cost == 100);

    auto r2 = check_construction_cost(ts, construction_costs::ZONE_FABRICATION_HIGH);
    assert(r2.can_afford && r2.cost == 1000);

    auto r3 = check_construction_cost(ts, construction_costs::SERVICE_NEXUS);
    assert(r3.can_afford && r3.cost == 5000);

    auto r4 = check_construction_cost(ts, construction_costs::RAIL_TRACK);
    assert(r4.can_afford && r4.cost == 25);

    auto r5 = check_construction_cost(ts, construction_costs::ENERGY_CONDUIT);
    assert(r5.can_afford && r5.cost == 5);

    printf("  PASS: All building type constants work with check\n");
}

// ---------------------------------------------------------------------------
// InsufficientFundsEvent struct
// ---------------------------------------------------------------------------

void test_insufficient_funds_event_struct() {
    printf("Testing InsufficientFundsEvent struct...\n");

    InsufficientFundsEvent event;
    event.player_id = 2;
    event.cost = 5000;
    event.balance = 1000;

    assert(event.player_id == 2);
    assert(event.cost == 5000);
    assert(event.balance == 1000);

    printf("  PASS: InsufficientFundsEvent struct fields work\n");
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main() {
    printf("=== ConstructionCost Unit Tests (E11-020) ===\n\n");

    // Constants
    test_zone_cost_constants();
    test_infrastructure_cost_constants();
    test_service_cost_constants();

    // Check affordability
    test_check_can_afford();
    test_check_cannot_afford();
    test_check_exact_balance();
    test_check_zero_cost();

    // Deduction
    test_deduct_success();
    test_deduct_insufficient_funds();
    test_deduct_exact_balance();
    test_deduct_zero_cost();

    // Multiple deductions
    test_multiple_deductions();

    // All building types
    test_all_building_type_costs();

    // Event struct
    test_insufficient_funds_event_struct();

    printf("\n=== All ConstructionCost tests passed! ===\n");
    return 0;
}
