/**
 * @file test_economy_integration.cpp
 * @brief Integration tests for economy system with Epic 9/10 systems (E11-024)
 *
 * Tests cross-system data flows: service funding, demand modifiers,
 * full budget cycles, construction, ordinances, and history tracking.
 */

#include "sims3000/economy/TreasuryState.h"
#include "sims3000/economy/TributeCalculation.h"
#include "sims3000/economy/TributeRateConfig.h"
#include "sims3000/economy/InfrastructureMaintenance.h"
#include "sims3000/economy/ServiceMaintenance.h"
#include "sims3000/economy/RevenueTracking.h"
#include "sims3000/economy/ExpenseTracking.h"
#include "sims3000/economy/BudgetCycle.h"
#include "sims3000/economy/FundingLevel.h"
#include "sims3000/economy/ServiceFundingIntegration.h"
#include "sims3000/economy/TributeDemandModifier.h"
#include "sims3000/economy/DeficitHandling.h"
#include "sims3000/economy/EmergencyBond.h"
#include "sims3000/economy/BondIssuance.h"
#include "sims3000/economy/BondRepayment.h"
#include "sims3000/economy/ConstructionCost.h"
#include "sims3000/economy/Ordinance.h"
#include <cassert>
#include <cstdio>
#include <cmath>

using namespace sims3000::economy;

// ============================================================================
// Category 1: Service Funding Integration
// ============================================================================

void test_service_funding_effectiveness() {
    printf("Testing service funding effectiveness...\n");

    // At 100% funding, effectiveness should be 1.0
    auto r100 = calculate_funded_effectiveness(0, 1.0f, 100);
    assert(std::fabs(r100.effectiveness_factor - 1.0f) < 0.01f);
    assert(std::fabs(r100.final_effectiveness - 1.0f) < 0.01f);

    // At 50% funding, effectiveness should be 0.65 (diminishing returns)
    auto r50 = calculate_funded_effectiveness(0, 1.0f, 50);
    assert(std::fabs(r50.effectiveness_factor - 0.65f) < 0.01f);
    assert(std::fabs(r50.final_effectiveness - 0.65f) < 0.01f);

    // At 150% funding, effectiveness should be 1.10 (capped)
    auto r150 = calculate_funded_effectiveness(0, 1.0f, 150);
    assert(std::fabs(r150.effectiveness_factor - 1.10f) < 0.01f);

    // At 0% funding, effectiveness should be 0.0
    auto r0 = calculate_funded_effectiveness(0, 1.0f, 0);
    assert(std::fabs(r0.effectiveness_factor - 0.0f) < 0.01f);

    printf("  PASS: Effectiveness curve: 0%%=0.0, 50%%=0.65, 100%%=1.0, 150%%=1.10\n");
}

void test_service_cost_scaling() {
    printf("Testing service cost scales with funding level...\n");

    // Medical at 100% funding
    ServiceMaintenanceInput input;
    input.service_type = 2; // Medical
    input.base_cost = SERVICE_COST_MEDICAL; // 300
    input.funding_level = 100;
    auto r100 = calculate_service_maintenance(input);

    // Medical at 150% funding
    input.funding_level = 150;
    auto r150 = calculate_service_maintenance(input);

    // Higher funding = higher cost
    assert(r150.actual_cost > r100.actual_cost);
    assert(r100.actual_cost == 300);
    assert(r150.actual_cost == 450); // 300 * 1.5

    printf("  PASS: Medical cost: 100%%=%lld, 150%%=%lld\n",
           static_cast<long long>(r100.actual_cost),
           static_cast<long long>(r150.actual_cost));
}

void test_funding_default_full_effectiveness() {
    printf("Testing default 100%% funding = full effectiveness...\n");

    TreasuryState treasury; // all defaults at 100%

    auto all = calculate_all_funded_effectiveness(treasury);

    for (int i = 0; i < 4; ++i) {
        assert(all.services[i].funding_level == 100);
        assert(std::fabs(all.services[i].effectiveness_factor - 1.0f) < 0.01f);
        assert(std::fabs(all.services[i].final_effectiveness - 1.0f) < 0.01f);
    }

    printf("  PASS: All 4 services at default 100%% = full effectiveness\n");
}

// ============================================================================
// Category 2: Demand Integration
// ============================================================================

void test_demand_high_tribute_reduces_demand() {
    printf("Testing high tribute reduces demand...\n");

    // 15% tribute -> significant negative modifier
    int modifier = calculate_tribute_demand_modifier(15);
    assert(modifier < 0);
    // Tier 4: -20 base - 5 * (15-12) = -20 - 15 = -35
    assert(modifier == -35);

    // 20% tribute -> max penalty
    int modifier20 = calculate_tribute_demand_modifier(20);
    // Tier 5: -40 base - 5 * (20-16) = -40 - 20 = -60
    assert(modifier20 == -60);

    printf("  PASS: 15%% tribute modifier = %d, 20%% = %d\n", modifier, modifier20);
}

void test_demand_low_tribute_bonus() {
    printf("Testing low tribute (0-3%%) gives +15 bonus...\n");

    assert(calculate_tribute_demand_modifier(0) == 15);
    assert(calculate_tribute_demand_modifier(1) == 15);
    assert(calculate_tribute_demand_modifier(2) == 15);
    assert(calculate_tribute_demand_modifier(3) == 15);

    printf("  PASS: All rates 0-3%% return +15\n");
}

void test_demand_neutral_at_7_percent() {
    printf("Testing 7%% tribute is neutral (0 modifier)...\n");

    assert(calculate_tribute_demand_modifier(4) == 0);
    assert(calculate_tribute_demand_modifier(5) == 0);
    assert(calculate_tribute_demand_modifier(6) == 0);
    assert(calculate_tribute_demand_modifier(7) == 0);

    printf("  PASS: Rates 4-7%% all return 0 modifier\n");
}

void test_demand_zone_tribute_modifier() {
    printf("Testing zone-specific tribute demand modifier...\n");

    TreasuryState treasury;
    treasury.tribute_rate_habitation = 2;   // low -> +15
    treasury.tribute_rate_exchange = 7;     // neutral -> 0
    treasury.tribute_rate_fabrication = 15; // high -> -35

    assert(get_zone_tribute_modifier(treasury, 0) == 15);
    assert(get_zone_tribute_modifier(treasury, 1) == 0);
    assert(get_zone_tribute_modifier(treasury, 2) == -35);

    printf("  PASS: Per-zone modifiers: hab=+15, exch=0, fab=-35\n");
}

// ============================================================================
// Category 3: Full Budget Cycle
// ============================================================================

void test_full_cycle_surplus() {
    printf("Testing full budget cycle with surplus...\n");

    TreasuryState treasury;
    treasury.balance = 20000;

    // Build income from 3 habitation buildings
    TributeInput input;
    input.base_value = constants::BASE_TRIBUTE_HABITATION_HIGH; // 200
    input.density_level = 1;
    input.tribute_modifier = 1.0f;
    input.current_occupancy = 100;
    input.capacity = 100;
    input.sector_value = 128;
    input.tribute_rate = 7;

    auto trib = calculate_building_tribute(input);
    int64_t per_building = trib.tribute_amount;

    std::vector<std::pair<ZoneBuildingType, int64_t>> results;
    for (int i = 0; i < 3; ++i) {
        results.push_back({ZoneBuildingType::Habitation, per_building});
    }
    auto agg = aggregate_tribute(results);
    auto income = build_income_breakdown(agg);

    // Build expenses: 10 pathways, 1 enforcer
    InfrastructureMaintenanceResult infra_result = {50, 0, 0, 0, 50}; // 10 * 5
    ServiceMaintenanceSummary svc_summary = {100, 0, 0, 0, 100}; // 1 enforcer at 100%
    auto expenses = build_expense_breakdown(infra_result, svc_summary, 0, 0, 0);

    BudgetCycleInput cycle_input;
    cycle_input.income = income;
    cycle_input.expenses = expenses;

    auto result = process_budget_cycle(treasury, cycle_input, 0);

    assert(result.net_change == income.total - expenses.total);
    assert(result.net_change > 0); // surplus
    assert(treasury.balance == 20000 + result.net_change);
    assert(result.is_deficit == false);

    printf("  PASS: Surplus cycle: income=%lld, expense=%lld, net=%lld, balance=%lld\n",
           static_cast<long long>(income.total),
           static_cast<long long>(expenses.total),
           static_cast<long long>(result.net_change),
           static_cast<long long>(treasury.balance));
}

void test_full_cycle_deficit() {
    printf("Testing full budget cycle with deficit...\n");

    TreasuryState treasury;
    treasury.balance = 100; // very low

    BudgetCycleInput input;
    input.income.total = 200;
    input.income.habitation_tribute = 200;
    input.expenses.total = 1000;
    input.expenses.service_maintenance = 1000;

    auto result = process_budget_cycle(treasury, input, 0);

    assert(result.net_change == -800);
    assert(treasury.balance == -700); // 100 - 800
    assert(result.is_deficit == true);

    printf("  PASS: Deficit cycle: 100 + (200 - 1000) = -700\n");
}

void test_deficit_warning_triggers() {
    printf("Testing deficit warning triggers at -5000...\n");

    TreasuryState treasury;
    treasury.balance = -5001; // below warning threshold

    auto result = check_deficit(treasury, 0);

    assert(result.should_warn == true);
    assert(result.warning_event.balance == -5001);

    // Apply state
    apply_deficit_state(treasury, result);
    assert(treasury.deficit_warning_sent == true);

    // Second check should not warn again
    auto result2 = check_deficit(treasury, 0);
    assert(result2.should_warn == false);

    printf("  PASS: Warning triggered once at -5001\n");
}

void test_emergency_bond_triggers() {
    printf("Testing emergency bond triggers at -10000...\n");

    TreasuryState treasury;
    treasury.balance = -11000; // below emergency threshold

    auto result = check_and_issue_emergency_bond(treasury, 0);

    assert(result.issued == true);
    assert(result.event.principal == BOND_EMERGENCY.principal); // 25000
    assert(treasury.balance == -11000 + 25000); // 14000
    assert(treasury.emergency_bond_active == true);
    assert(treasury.active_bonds.size() == 1);
    assert(treasury.active_bonds[0].is_emergency == true);

    // Second check should not issue another
    treasury.balance = -15000; // force below again
    auto result2 = check_and_issue_emergency_bond(treasury, 0);
    assert(result2.issued == false); // already active

    printf("  PASS: Emergency bond issued, balance: -11000 -> %lld\n",
           static_cast<long long>(-11000 + BOND_EMERGENCY.principal));
}

// ============================================================================
// Category 4: Construction Integration
// ============================================================================

void test_construction_deducts_cost() {
    printf("Testing construction deducts cost from treasury...\n");

    TreasuryState treasury;
    treasury.balance = 20000;

    bool ok = deduct_construction_cost(treasury, construction_costs::SERVICE_STATION); // 2000

    assert(ok);
    assert(treasury.balance == 18000);

    printf("  PASS: Deducted 2000 for service station: 20000 -> 18000\n");
}

void test_cannot_afford_blocks() {
    printf("Testing cannot afford blocks construction...\n");

    TreasuryState treasury;
    treasury.balance = 1000;

    auto result = check_construction_cost(treasury, construction_costs::SERVICE_NEXUS); // 5000

    assert(result.can_afford == false);
    assert(result.cost == 5000);
    assert(result.balance_after == -4000); // projected

    // deduct should also fail
    bool ok = deduct_construction_cost(treasury, construction_costs::SERVICE_NEXUS);
    assert(!ok);
    assert(treasury.balance == 1000); // unchanged

    printf("  PASS: Cannot afford 5000 with balance 1000\n");
}

void test_multiple_constructions() {
    printf("Testing multiple construction deductions...\n");

    TreasuryState treasury;
    treasury.balance = 20000;

    // Build 5 pathways (10 each)
    for (int i = 0; i < 5; ++i) {
        bool ok = deduct_construction_cost(treasury, construction_costs::PATHWAY);
        assert(ok);
    }
    assert(treasury.balance == 19950); // 20000 - 5 * 10

    // Build a service post (500)
    bool ok = deduct_construction_cost(treasury, construction_costs::SERVICE_POST);
    assert(ok);
    assert(treasury.balance == 19450);

    printf("  PASS: Multiple deductions: 20000 -> 19450\n");
}

// ============================================================================
// Category 5: Ordinance Integration
// ============================================================================

void test_ordinance_costs_in_budget() {
    printf("Testing ordinance costs appear in budget...\n");

    OrdinanceState ords;
    ords.enable(OrdinanceType::EnhancedPatrol); // 1000/phase

    int64_t ord_cost = ords.get_total_cost();
    assert(ord_cost == 1000);

    // Build expense breakdown with ordinance
    InfrastructureMaintenanceResult infra = {0, 0, 0, 0, 0};
    ServiceMaintenanceSummary svc = {0, 0, 0, 0, 0};
    auto expenses = build_expense_breakdown(infra, svc, 0, 0, ord_cost);

    assert(expenses.ordinance_costs == 1000);
    assert(expenses.total == 1000);

    // Process budget
    TreasuryState treasury;
    treasury.balance = 20000;

    BudgetCycleInput input;
    input.income.total = 2000;
    input.income.habitation_tribute = 2000;
    input.expenses = expenses;

    auto result = process_budget_cycle(treasury, input, 0);

    assert(result.net_change == 1000); // 2000 - 1000
    assert(treasury.balance == 21000);
    assert(treasury.ordinance_costs == 1000);

    printf("  PASS: Ordinance cost 1000 deducted from budget\n");
}

void test_multiple_ordinances_stack() {
    printf("Testing multiple ordinance costs stack...\n");

    OrdinanceState ords;
    ords.enable(OrdinanceType::EnhancedPatrol);       // 1000
    ords.enable(OrdinanceType::IndustrialScrubbers);   // 2000
    ords.enable(OrdinanceType::FreeTransit);           // 5000

    int64_t total = ords.get_total_cost();
    assert(total == 8000); // 1000 + 2000 + 5000

    // Disable one
    ords.disable(OrdinanceType::FreeTransit);
    total = ords.get_total_cost();
    assert(total == 3000); // 1000 + 2000

    printf("  PASS: 3 ordinances = 8000, after disabling 1 = 3000\n");
}

// ============================================================================
// Category 6: History and Tracking
// ============================================================================

void test_income_history_12_phases() {
    printf("Testing income history circular buffer (12 phases)...\n");

    IncomeHistory history;

    // Record 12 phases of income
    for (int i = 1; i <= 12; ++i) {
        history.record(i * 1000);
    }

    assert(history.count == 12);
    int64_t avg = history.get_average();
    // Average of 1000, 2000, ..., 12000 = 6500
    assert(avg == 6500);

    // Record a 13th entry (wraps around)
    history.record(13000);
    assert(history.count == 12); // still 12

    // Average should now be of 2000..13000 = (2000+13000)/2 * 12 / 12
    // Actually sum = 2+3+...+13 = 90 * 1000 = 90000, avg = 7500
    int64_t avg2 = history.get_average();
    assert(avg2 == 7500);

    printf("  PASS: Circular buffer wraps correctly, avg after 12=%lld, after 13=%lld\n",
           static_cast<long long>(avg), static_cast<long long>(avg2));
}

void test_expense_history_trend() {
    printf("Testing expense history trend detection...\n");

    ExpenseHistory history;

    // Record growing expenses: 100, 200, 300, 400
    history.record(100);
    history.record(200);
    history.record(300);
    history.record(400);

    assert(history.count == 4);
    int64_t trend = history.get_trend();
    // Recent half: 400, 300 -> avg 350
    // Older half: 200, 100 -> avg 150
    // Trend: 350 - 150 = 200
    assert(trend == 200);

    // Record shrinking expenses
    ExpenseHistory shrink;
    shrink.record(400);
    shrink.record(300);
    shrink.record(200);
    shrink.record(100);

    int64_t shrink_trend = shrink.get_trend();
    // Recent half: 100, 200 -> avg 150
    // Older half: 300, 400 -> avg 350
    // Trend: 150 - 350 = -200
    assert(shrink_trend == -200);

    printf("  PASS: Growing trend = %lld, shrinking trend = %lld\n",
           static_cast<long long>(trend), static_cast<long long>(shrink_trend));
}

void test_income_tracking_applied_to_treasury() {
    printf("Testing income tracking applied to treasury...\n");

    // Build a scenario: tribute from 3 zone types
    AggregateTributeResult agg;
    agg.habitation_total = 500;
    agg.exchange_total = 300;
    agg.fabrication_total = 200;
    agg.grand_total = 1000;
    agg.buildings_counted = 10;

    auto income = build_income_breakdown(agg, 50); // 50 other income

    assert(income.total == 1050); // 500 + 300 + 200 + 50

    TreasuryState treasury;
    apply_income_to_treasury(treasury, income);

    assert(treasury.habitation_tribute == 500);
    assert(treasury.exchange_tribute == 300);
    assert(treasury.fabrication_tribute == 200);
    assert(treasury.other_income == 50);
    assert(treasury.last_income == 1050);

    printf("  PASS: Income breakdown applied to treasury fields\n");
}

void test_expense_tracking_applied_to_treasury() {
    printf("Testing expense tracking applied to treasury...\n");

    InfrastructureMaintenanceResult infra = {100, 20, 30, 50, 200};
    ServiceMaintenanceSummary svc = {100, 120, 300, 200, 720};
    auto expenses = build_expense_breakdown(infra, svc, 50, 500, 1000);

    // total = 200 + 720 + 50 + 500 + 1000 = 2470
    assert(expenses.total == 2470);

    TreasuryState treasury;
    apply_expenses_to_treasury(treasury, expenses);

    assert(treasury.infrastructure_maintenance == 200);
    assert(treasury.service_maintenance == 720);
    assert(treasury.energy_maintenance == 50);
    assert(treasury.bond_payments == 500);
    assert(treasury.ordinance_costs == 1000);
    assert(treasury.last_expense == 2470);

    printf("  PASS: Expense breakdown applied to treasury fields\n");
}

// ============================================================================
// Category 7: Multi-phase realistic scenario
// ============================================================================

void test_multi_phase_realistic_scenario() {
    printf("Testing multi-phase realistic budget scenario...\n");

    TreasuryState treasury;
    treasury.balance = 20000;

    IncomeHistory income_hist;
    ExpenseHistory expense_hist;

    // Simulate 5 phases
    for (int phase = 0; phase < 5; ++phase) {
        // Income increases each phase (growing city)
        int64_t income_total = 1000 + phase * 200;
        int64_t expense_total = 800;

        BudgetCycleInput input;
        input.income.habitation_tribute = income_total;
        input.income.total = income_total;
        input.expenses.infrastructure_maintenance = expense_total;
        input.expenses.total = expense_total;

        process_budget_cycle(treasury, input, 0);

        income_hist.record(income_total);
        expense_hist.record(expense_total);
    }

    // After 5 phases: balance should have increased
    // Net per phase: (1000-800), (1200-800), (1400-800), (1600-800), (1800-800)
    // = 200, 400, 600, 800, 1000 = 3000 total net
    assert(treasury.balance == 23000);

    // Income trend should be positive (growing)
    assert(income_hist.get_trend() > 0);

    // Expense trend should be zero (constant)
    assert(expense_hist.get_trend() == 0);

    printf("  PASS: 5 phases: balance 20000 -> %lld, income trend > 0\n",
           static_cast<long long>(treasury.balance));
}

void test_deficit_recovery_resets_flags() {
    printf("Testing deficit recovery resets warning flags...\n");

    TreasuryState treasury;
    treasury.balance = -6000;
    treasury.deficit_warning_sent = true;
    treasury.emergency_bond_active = true;

    // Balance recovers to positive
    treasury.balance = 1000;
    check_deficit_recovery(treasury);

    assert(treasury.deficit_warning_sent == false);
    assert(treasury.emergency_bond_active == false);

    printf("  PASS: Recovery at positive balance resets flags\n");
}

// ============================================================================
// Main
// ============================================================================

int main() {
    printf("=== Economy Integration Tests (E11-024) ===\n\n");

    // Service Funding Integration
    test_service_funding_effectiveness();
    test_service_cost_scaling();
    test_funding_default_full_effectiveness();

    // Demand Integration
    test_demand_high_tribute_reduces_demand();
    test_demand_low_tribute_bonus();
    test_demand_neutral_at_7_percent();
    test_demand_zone_tribute_modifier();

    // Full Budget Cycle
    test_full_cycle_surplus();
    test_full_cycle_deficit();
    test_deficit_warning_triggers();
    test_emergency_bond_triggers();

    // Construction Integration
    test_construction_deducts_cost();
    test_cannot_afford_blocks();
    test_multiple_constructions();

    // Ordinance Integration
    test_ordinance_costs_in_budget();
    test_multiple_ordinances_stack();

    // History and Tracking
    test_income_history_12_phases();
    test_expense_history_trend();
    test_income_tracking_applied_to_treasury();
    test_expense_tracking_applied_to_treasury();

    // Multi-phase scenario
    test_multi_phase_realistic_scenario();
    test_deficit_recovery_resets_flags();

    printf("\n=== All tests passed! (22 tests) ===\n");
    return 0;
}
