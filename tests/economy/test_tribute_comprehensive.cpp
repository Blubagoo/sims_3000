/**
 * @file test_tribute_comprehensive.cpp
 * @brief Comprehensive unit tests for tribute and maintenance systems (E11-023)
 *
 * Exercises tribute calculation, maintenance, bonds, and edge cases
 * in realistic multi-building scenarios.
 */

#include "sims3000/economy/TreasuryState.h"
#include "sims3000/economy/TributeCalculation.h"
#include "sims3000/economy/TributeRateConfig.h"
#include "sims3000/economy/InfrastructureMaintenance.h"
#include "sims3000/economy/ServiceMaintenance.h"
#include "sims3000/economy/BondIssuance.h"
#include "sims3000/economy/BondRepayment.h"
#include "sims3000/economy/BudgetCycle.h"
#include "sims3000/economy/RevenueTracking.h"
#include "sims3000/economy/ExpenseTracking.h"
#include <cassert>
#include <cstdio>
#include <cmath>

using namespace sims3000::economy;

// ============================================================================
// Category 1: Treasury Tests
// ============================================================================

void test_starting_balance() {
    printf("Testing starting balance...\n");

    TreasuryState treasury;

    assert(treasury.balance == 20000);
    assert(treasury.last_income == 0);
    assert(treasury.last_expense == 0);
    assert(treasury.tribute_rate_habitation == 7);
    assert(treasury.tribute_rate_exchange == 7);
    assert(treasury.tribute_rate_fabrication == 7);

    printf("  PASS: New player starts with 20,000 credits and default rates\n");
}

void test_income_calculation() {
    printf("Testing income calculation from multiple buildings...\n");

    // Create three habitation buildings with different parameters
    TributeInput hab1;
    hab1.base_value = constants::BASE_TRIBUTE_HABITATION_LOW; // 50
    hab1.density_level = 0;
    hab1.tribute_modifier = 1.0f;
    hab1.current_occupancy = 100;
    hab1.capacity = 100;
    hab1.sector_value = 128;
    hab1.tribute_rate = 7;

    TributeInput hab2;
    hab2.base_value = constants::BASE_TRIBUTE_HABITATION_LOW; // 50
    hab2.density_level = 0;
    hab2.tribute_modifier = 1.0f;
    hab2.current_occupancy = 50;
    hab2.capacity = 100;
    hab2.sector_value = 200;
    hab2.tribute_rate = 7;

    auto result1 = calculate_building_tribute(hab1);
    auto result2 = calculate_building_tribute(hab2);

    int64_t total_tribute = result1.tribute_amount + result2.tribute_amount;

    // Both should produce positive tribute
    assert(result1.tribute_amount > 0);
    assert(result2.tribute_amount > 0);
    assert(total_tribute == result1.tribute_amount + result2.tribute_amount);

    // Aggregate
    std::vector<std::pair<ZoneBuildingType, int64_t>> results;
    results.push_back({ZoneBuildingType::Habitation, result1.tribute_amount});
    results.push_back({ZoneBuildingType::Habitation, result2.tribute_amount});
    auto agg = aggregate_tribute(results);

    assert(agg.habitation_total == total_tribute);
    assert(agg.grand_total == total_tribute);
    assert(agg.buildings_counted == 2);

    printf("  PASS: Income summed correctly from 2 buildings: %lld\n",
           static_cast<long long>(total_tribute));
}

void test_expense_calculation() {
    printf("Testing expense calculation from maintenance...\n");

    // Infrastructure: 10 pathway tiles, 5 energy conduit tiles
    std::vector<std::pair<InfrastructureType, int64_t>> infra_costs;
    for (int i = 0; i < 10; ++i) {
        InfrastructureMaintenanceInput input;
        input.base_cost = MAINTENANCE_PATHWAY; // 5
        input.cost_multiplier = 1.0f;
        infra_costs.push_back({InfrastructureType::Pathway,
                               calculate_infrastructure_cost(input)});
    }
    for (int i = 0; i < 5; ++i) {
        InfrastructureMaintenanceInput input;
        input.base_cost = MAINTENANCE_ENERGY_CONDUIT; // 2
        input.cost_multiplier = 1.0f;
        infra_costs.push_back({InfrastructureType::EnergyConduit,
                               calculate_infrastructure_cost(input)});
    }

    auto infra_result = aggregate_infrastructure_maintenance(infra_costs);

    assert(infra_result.pathway_cost == 50);       // 10 * 5
    assert(infra_result.energy_conduit_cost == 10); // 5 * 2
    assert(infra_result.total == 60);

    printf("  PASS: Expenses summed correctly: infra=%lld\n",
           static_cast<long long>(infra_result.total));
}

void test_budget_cycle_balance_update() {
    printf("Testing budget cycle balance update at phase transition...\n");

    TreasuryState treasury;
    treasury.balance = 20000;

    BudgetCycleInput input;
    input.income.habitation_tribute = 500;
    input.income.exchange_tribute = 300;
    input.income.fabrication_tribute = 200;
    input.income.total = 1000;
    input.expenses.infrastructure_maintenance = 60;
    input.expenses.service_maintenance = 400;
    input.expenses.total = 460;

    auto result = process_budget_cycle(treasury, input, 0);

    assert(result.net_change == 540); // 1000 - 460
    assert(treasury.balance == 20540);
    assert(treasury.last_income == 1000);
    assert(treasury.last_expense == 460);

    printf("  PASS: Balance updated at phase transition: 20000 + 540 = 20540\n");
}

// ============================================================================
// Category 2: Tribute Tests
// ============================================================================

void test_tribute_base_values() {
    printf("Testing tribute base values for all zone types and densities...\n");

    // Habitation
    assert(get_base_tribute_value(ZoneBuildingType::Habitation, 0) == 50);
    assert(get_base_tribute_value(ZoneBuildingType::Habitation, 1) == 200);

    // Exchange
    assert(get_base_tribute_value(ZoneBuildingType::Exchange, 0) == 100);
    assert(get_base_tribute_value(ZoneBuildingType::Exchange, 1) == 400);

    // Fabrication
    assert(get_base_tribute_value(ZoneBuildingType::Fabrication, 0) == 75);
    assert(get_base_tribute_value(ZoneBuildingType::Fabrication, 1) == 300);

    printf("  PASS: All 6 base values correct (3 zones x 2 densities)\n");
}

void test_tribute_rate_application() {
    printf("Testing tribute rate application at 0%%, 7%%, 20%%...\n");

    // Full occupancy, mid land value, modifier 1.0
    TributeInput input;
    input.base_value = 100;
    input.density_level = 0;
    input.tribute_modifier = 1.0f;
    input.current_occupancy = 100;
    input.capacity = 100;
    input.sector_value = 128; // value_factor ~ 1.25

    // 0% rate -> 0 tribute
    input.tribute_rate = 0;
    auto r0 = calculate_building_tribute(input);
    assert(r0.tribute_amount == 0);
    assert(r0.rate_factor < 0.001f);

    // 7% rate -> positive tribute
    input.tribute_rate = 7;
    auto r7 = calculate_building_tribute(input);
    assert(r7.tribute_amount > 0);
    assert(std::fabs(r7.rate_factor - 0.07f) < 0.001f);

    // 20% rate -> higher tribute
    input.tribute_rate = 20;
    auto r20 = calculate_building_tribute(input);
    assert(r20.tribute_amount > r7.tribute_amount);
    assert(std::fabs(r20.rate_factor - 0.20f) < 0.001f);

    printf("  PASS: Rate 0%%=%lld, 7%%=%lld, 20%%=%lld\n",
           static_cast<long long>(r0.tribute_amount),
           static_cast<long long>(r7.tribute_amount),
           static_cast<long long>(r20.tribute_amount));
}

void test_tribute_occupancy_factor() {
    printf("Testing tribute occupancy factor (empty, half, full)...\n");

    TributeInput input;
    input.base_value = 200;
    input.density_level = 1;
    input.tribute_modifier = 1.0f;
    input.capacity = 200;
    input.sector_value = 128;
    input.tribute_rate = 10;

    // Empty building -> 0 tribute
    input.current_occupancy = 0;
    auto empty = calculate_building_tribute(input);
    assert(empty.tribute_amount == 0);
    assert(empty.occupancy_factor < 0.001f);

    // Half full
    input.current_occupancy = 100;
    auto half = calculate_building_tribute(input);
    assert(half.tribute_amount > 0);
    assert(std::fabs(half.occupancy_factor - 0.5f) < 0.01f);

    // Full
    input.current_occupancy = 200;
    auto full = calculate_building_tribute(input);
    assert(full.tribute_amount > half.tribute_amount);
    assert(std::fabs(full.occupancy_factor - 1.0f) < 0.01f);

    // Full should be exactly 2x half
    assert(full.tribute_amount == half.tribute_amount * 2);

    printf("  PASS: Empty=0, Half=%lld, Full=%lld\n",
           static_cast<long long>(half.tribute_amount),
           static_cast<long long>(full.tribute_amount));
}

void test_tribute_sector_value_factor() {
    printf("Testing tribute sector value factor (low, mid, high)...\n");

    TributeInput input;
    input.base_value = 100;
    input.density_level = 0;
    input.tribute_modifier = 1.0f;
    input.current_occupancy = 100;
    input.capacity = 100;
    input.tribute_rate = 10;

    // Low sector value (0) -> value_factor = 0.5
    input.sector_value = 0;
    auto low = calculate_building_tribute(input);
    assert(std::fabs(low.value_factor - 0.5f) < 0.01f);

    // Mid sector value (128) -> value_factor ~ 1.25
    input.sector_value = 128;
    auto mid = calculate_building_tribute(input);
    assert(mid.value_factor > 1.0f && mid.value_factor < 1.5f);

    // High sector value (255) -> value_factor = 2.0
    input.sector_value = 255;
    auto high = calculate_building_tribute(input);
    assert(std::fabs(high.value_factor - 2.0f) < 0.01f);

    // Higher land value = more tribute
    assert(high.tribute_amount > mid.tribute_amount);
    assert(mid.tribute_amount > low.tribute_amount);

    printf("  PASS: Low vf=%.2f, Mid vf=%.2f, High vf=%.2f\n",
           low.value_factor, mid.value_factor, high.value_factor);
}

void test_tribute_rate_change() {
    printf("Testing tribute rate change via set_tribute_rate...\n");

    TreasuryState treasury;
    assert(treasury.tribute_rate_habitation == 7); // default

    auto event = set_tribute_rate(treasury, ZoneBuildingType::Habitation, 12, 0);

    assert(treasury.tribute_rate_habitation == 12);
    assert(event.old_rate == 7);
    assert(event.new_rate == 12);

    // Test clamping above 20
    set_tribute_rate(treasury, ZoneBuildingType::Exchange, 25, 0);
    assert(treasury.tribute_rate_exchange == 20); // clamped

    printf("  PASS: Rate changed from 7 to 12, and clamped 25 to 20\n");
}

// ============================================================================
// Category 3: Maintenance Tests
// ============================================================================

void test_infrastructure_maintenance_per_tile() {
    printf("Testing infrastructure maintenance per-tile costs...\n");

    assert(get_infrastructure_maintenance_rate(InfrastructureType::Pathway) == 5);
    assert(get_infrastructure_maintenance_rate(InfrastructureType::EnergyConduit) == 2);
    assert(get_infrastructure_maintenance_rate(InfrastructureType::FluidConduit) == 3);
    assert(get_infrastructure_maintenance_rate(InfrastructureType::RailTrack) == 8);

    // Test actual calculation with multiplier
    InfrastructureMaintenanceInput input;
    input.base_cost = MAINTENANCE_PATHWAY;
    input.cost_multiplier = 1.5f; // aged road
    int64_t cost = calculate_infrastructure_cost(input);
    assert(cost == 8); // round(5 * 1.5) = 8

    printf("  PASS: All 4 infrastructure types have correct per-tile costs\n");
}

void test_service_maintenance_scaling() {
    printf("Testing service maintenance scaling at 50%%, 100%%, 150%% funding...\n");

    ServiceMaintenanceInput input;
    input.service_type = 0; // Enforcer
    input.base_cost = SERVICE_COST_ENFORCER; // 100

    // 50% funding -> 50 cost
    input.funding_level = 50;
    auto r50 = calculate_service_maintenance(input);
    assert(r50.actual_cost == 50);
    assert(std::fabs(r50.funding_factor - 0.5f) < 0.01f);

    // 100% funding -> 100 cost
    input.funding_level = 100;
    auto r100 = calculate_service_maintenance(input);
    assert(r100.actual_cost == 100);
    assert(std::fabs(r100.funding_factor - 1.0f) < 0.01f);

    // 150% funding -> 150 cost
    input.funding_level = 150;
    auto r150 = calculate_service_maintenance(input);
    assert(r150.actual_cost == 150);
    assert(std::fabs(r150.funding_factor - 1.5f) < 0.01f);

    printf("  PASS: 50%%=%lld, 100%%=%lld, 150%%=%lld\n",
           static_cast<long long>(r50.actual_cost),
           static_cast<long long>(r100.actual_cost),
           static_cast<long long>(r150.actual_cost));
}

void test_combined_maintenance() {
    printf("Testing combined infrastructure + service maintenance...\n");

    // Infrastructure: 20 pathways
    std::vector<std::pair<InfrastructureType, int64_t>> infra_costs;
    for (int i = 0; i < 20; ++i) {
        InfrastructureMaintenanceInput input;
        input.base_cost = MAINTENANCE_PATHWAY;
        input.cost_multiplier = 1.0f;
        infra_costs.push_back({InfrastructureType::Pathway,
                               calculate_infrastructure_cost(input)});
    }
    auto infra = aggregate_infrastructure_maintenance(infra_costs);

    // Services: 1 enforcer, 1 medical
    std::vector<std::pair<uint8_t, int64_t>> svc_costs;
    ServiceMaintenanceInput svc_input;
    svc_input.service_type = 0;
    svc_input.base_cost = SERVICE_COST_ENFORCER; // 100
    svc_input.funding_level = 100;
    auto enf = calculate_service_maintenance(svc_input);
    svc_costs.push_back({0, enf.actual_cost});

    svc_input.service_type = 2;
    svc_input.base_cost = SERVICE_COST_MEDICAL; // 300
    svc_input.funding_level = 100;
    auto med = calculate_service_maintenance(svc_input);
    svc_costs.push_back({2, med.actual_cost});

    auto services = aggregate_service_maintenance(svc_costs);

    int64_t total = infra.total + services.total;

    assert(infra.total == 100);       // 20 * 5
    assert(services.total == 400);    // 100 + 300
    assert(total == 500);

    printf("  PASS: Combined maintenance = %lld (infra=%lld, svc=%lld)\n",
           static_cast<long long>(total),
           static_cast<long long>(infra.total),
           static_cast<long long>(services.total));
}

// ============================================================================
// Category 4: Bond Tests
// ============================================================================

void test_bond_issuance_adds_principal() {
    printf("Testing bond issuance adds principal to treasury...\n");

    TreasuryState treasury;
    treasury.balance = 5000;

    auto result = issue_bond(treasury, BondType::Small, 0);

    assert(result.success);
    assert(result.principal_added == 5000); // BOND_SMALL principal
    assert(treasury.balance == 10000);      // 5000 + 5000
    assert(treasury.active_bonds.size() == 1);

    printf("  PASS: Bond added 5000 to balance: 5000 -> 10000\n");
}

void test_bond_payment_correct() {
    printf("Testing bond payment calculation...\n");

    CreditAdvance bond;
    bond.principal = 12000;
    bond.remaining_principal = 12000;
    bond.interest_rate_basis_points = 600; // 6%
    bond.term_phases = 12;
    bond.phases_remaining = 12;

    std::vector<CreditAdvance> bonds = {bond};
    auto result = calculate_bond_payments(bonds);

    // principal_payment = 12000 / 12 = 1000
    // interest_payment = (12000 * 600) / (10000 * 12) = 60
    assert(result.principal_paid == 1000);
    assert(result.interest_paid == 60);
    assert(result.total_payment == 1060);

    printf("  PASS: Payment = principal 1000 + interest 60 = 1060\n");
}

void test_bond_completion() {
    printf("Testing bond removed after term completes...\n");

    CreditAdvance bond;
    bond.principal = 6000;
    bond.remaining_principal = 500;
    bond.interest_rate_basis_points = 750;
    bond.term_phases = 12;
    bond.phases_remaining = 1; // last phase

    std::vector<CreditAdvance> bonds = {bond};
    auto result = process_bond_payments(bonds);

    assert(bonds.empty()); // removed
    assert(result.bonds_matured == 1);

    printf("  PASS: Bond removed after final phase\n");
}

void test_bond_interest_over_multiple_phases() {
    printf("Testing bond interest calculated over multiple phases...\n");

    CreditAdvance bond;
    bond.principal = 24000;
    bond.remaining_principal = 24000;
    bond.interest_rate_basis_points = 1000; // 10%
    bond.term_phases = 24;
    bond.phases_remaining = 24;

    std::vector<CreditAdvance> bonds = {bond};

    int64_t total_interest = 0;
    int64_t total_principal_paid = 0;

    // Process 3 phases
    for (int phase = 0; phase < 3; ++phase) {
        auto result = process_bond_payments(bonds);
        total_interest += result.interest_paid;
        total_principal_paid += result.principal_paid;
    }

    assert(bonds.size() == 1);
    assert(bonds[0].phases_remaining == 21); // 24 - 3
    assert(total_principal_paid == 3000);     // 3 * (24000/24) = 3 * 1000
    assert(total_interest > 0);
    // Interest should decrease slightly each phase as principal decreases
    // Phase 1: (24000 * 1000) / 120000 = 200
    // Phase 2: (23000 * 1000) / 120000 = 191
    // Phase 3: (22000 * 1000) / 120000 = 183
    assert(total_interest == 200 + 191 + 183);

    printf("  PASS: Interest over 3 phases: %lld (decreasing as principal shrinks)\n",
           static_cast<long long>(total_interest));
}

// ============================================================================
// Category 5: Edge Cases
// ============================================================================

void test_zero_population_no_tribute() {
    printf("Testing zero population (no buildings) produces no tribute...\n");

    std::vector<std::pair<ZoneBuildingType, int64_t>> empty_results;
    auto agg = aggregate_tribute(empty_results);

    assert(agg.grand_total == 0);
    assert(agg.buildings_counted == 0);
    assert(agg.habitation_total == 0);
    assert(agg.exchange_total == 0);
    assert(agg.fabrication_total == 0);

    printf("  PASS: No buildings = no tribute\n");
}

void test_all_buildings_empty() {
    printf("Testing all buildings empty (zero occupancy)...\n");

    TributeInput input;
    input.base_value = 200;
    input.density_level = 1;
    input.tribute_modifier = 1.0f;
    input.current_occupancy = 0; // empty!
    input.capacity = 100;
    input.sector_value = 255;
    input.tribute_rate = 20;

    auto result = calculate_building_tribute(input);

    assert(result.tribute_amount == 0);
    assert(result.occupancy_factor < 0.001f);

    printf("  PASS: Empty building produces 0 tribute regardless of other factors\n");
}

void test_max_tribute_rate() {
    printf("Testing max tribute rate (20%%)...\n");

    TributeInput input;
    input.base_value = 100;
    input.density_level = 0;
    input.tribute_modifier = 1.0f;
    input.current_occupancy = 100;
    input.capacity = 100;
    input.sector_value = 128;
    input.tribute_rate = 20; // max

    auto result = calculate_building_tribute(input);

    assert(result.tribute_amount > 0);
    assert(std::fabs(result.rate_factor - 0.20f) < 0.001f);

    // Verify clamping: setting 25 should clamp to 20
    assert(clamp_tribute_rate(25) == 20);
    assert(clamp_tribute_rate(20) == 20);

    printf("  PASS: Max rate 20%% works correctly, rate_factor=0.20\n");
}

void test_zero_tribute_rate() {
    printf("Testing zero tribute rate (0%%)...\n");

    TributeInput input;
    input.base_value = 500;
    input.density_level = 1;
    input.tribute_modifier = 1.0f;
    input.current_occupancy = 200;
    input.capacity = 200;
    input.sector_value = 255;
    input.tribute_rate = 0; // zero

    auto result = calculate_building_tribute(input);

    assert(result.tribute_amount == 0);
    assert(result.rate_factor < 0.001f);

    printf("  PASS: 0%% rate produces zero tribute\n");
}

void test_negative_balance() {
    printf("Testing treasury can go negative...\n");

    TreasuryState treasury;
    treasury.balance = 100;

    BudgetCycleInput input;
    input.income.total = 0;
    input.expenses.total = 5000;
    input.expenses.infrastructure_maintenance = 5000;

    auto result = process_budget_cycle(treasury, input, 0);

    assert(treasury.balance == -4900); // 100 - 5000
    assert(result.is_deficit == true);
    assert(result.new_balance == -4900);

    printf("  PASS: Balance went negative: 100 - 5000 = -4900\n");
}

void test_zero_capacity_building() {
    printf("Testing building with zero capacity...\n");

    TributeInput input;
    input.base_value = 100;
    input.density_level = 0;
    input.tribute_modifier = 1.0f;
    input.current_occupancy = 0;
    input.capacity = 0; // zero capacity
    input.sector_value = 128;
    input.tribute_rate = 10;

    auto result = calculate_building_tribute(input);

    assert(result.tribute_amount == 0);
    assert(result.occupancy_factor < 0.001f);

    printf("  PASS: Zero-capacity building produces 0 tribute\n");
}

// ============================================================================
// Main
// ============================================================================

int main() {
    printf("=== Comprehensive Tribute & Maintenance Unit Tests (E11-023) ===\n\n");

    // Category 1: Treasury
    test_starting_balance();
    test_income_calculation();
    test_expense_calculation();
    test_budget_cycle_balance_update();

    // Category 2: Tribute
    test_tribute_base_values();
    test_tribute_rate_application();
    test_tribute_occupancy_factor();
    test_tribute_sector_value_factor();
    test_tribute_rate_change();

    // Category 3: Maintenance
    test_infrastructure_maintenance_per_tile();
    test_service_maintenance_scaling();
    test_combined_maintenance();

    // Category 4: Bonds
    test_bond_issuance_adds_principal();
    test_bond_payment_correct();
    test_bond_completion();
    test_bond_interest_over_multiple_phases();

    // Category 5: Edge Cases
    test_zero_population_no_tribute();
    test_all_buildings_empty();
    test_max_tribute_rate();
    test_zero_tribute_rate();
    test_negative_balance();
    test_zero_capacity_building();

    printf("\n=== All tests passed! (22 tests) ===\n");
    return 0;
}
