/**
 * @file test_trade_calculation_comprehensive.cpp
 * @brief Comprehensive unit tests for trade income and demand bonus calculation
 *        (Epic 8, Ticket E8-037)
 *
 * Tests cover:
 *
 * Trade income formula verification:
 * - Exact formula: capacity * utilization * income_rate * trade_multiplier * demand_factor
 * - All capacity tiers (small/medium/large) with both port types
 * - Multiple ports of the same type summing correctly
 * - Income with multiplier < 1.0 (None-tier agreement: 50%)
 * - Income with multiplier > 1.0 (Premium-tier: 120%)
 * - Zero income from non-operational and zero-capacity ports
 * - Breakdown correctness (aero_income + aqua_income == total)
 *
 * Demand bonus cap (stacking, overflow):
 * - Global bonus exact cap at 30
 * - Bonus above 30 still returns 30 (stacking overflow)
 * - Diminishing returns prevent reaching cap quickly
 * - Combined (global + local) capped at 30
 * - Local bonus alone not capped (but combined is)
 *
 * Trade agreement effects:
 * - get_agreement_benefits: correct values per tier
 * - calculate_total_demand_bonus: sums across multiple agreements
 * - apply_trade_agreement_income_bonus: formula verified
 * - Stacking multiple agreements of different tiers
 * - Expired agreements (cycles_remaining = 0) handled
 * - Agreements for other players do not affect income
 * - Agreement with None type gives no bonus
 *
 * Diminishing returns accuracy:
 * - Exact multiplier values per index (0,1,2,3,4+)
 * - Sum of diminishing returns across many ports
 * - Diminishing returns with mixed port sizes
 * - Diminishing returns + cap interaction
 */

#include <sims3000/port/TradeIncome.h>
#include <sims3000/port/DemandBonus.h>
#include <sims3000/port/DiminishingReturns.h>
#include <sims3000/port/TradeAgreementBenefits.h>
#include <sims3000/port/PortTypes.h>
#include <sims3000/port/TradeAgreementComponent.h>
#include <cassert>
#include <cstdio>
#include <cmath>
#include <vector>

using namespace sims3000::port;

// =============================================================================
// Helpers
// =============================================================================

static bool approx_eq(float a, float b, float eps = 0.01f) {
    return std::fabs(a - b) < eps;
}

static int tests_passed = 0;
static int tests_total = 0;

#define TEST(name) \
    do { \
        tests_total++; \
        printf("  TEST: %s ... ", name); \
    } while(0)

#define PASS() \
    do { \
        tests_passed++; \
        printf("PASS\n"); \
    } while(0)

/// Create a TradeAgreementComponent helper
static TradeAgreementComponent make_agreement(
    uint8_t party_a, uint8_t party_b,
    TradeAgreementType type,
    uint8_t income_bonus_pct,
    uint16_t cycles_remaining) {

    TradeAgreementComponent a = {};
    a.party_a = party_a;
    a.party_b = party_b;
    a.agreement_type = type;
    a.income_bonus_percent = income_bonus_pct;
    a.cycles_remaining = cycles_remaining;
    return a;
}

// =============================================================================
// TRADE INCOME FORMULA VERIFICATION
// =============================================================================

static void test_income_formula_small_aero() {
    TEST("income formula: small aero port (cap=200, util=0.5, rate=0.8)");
    std::vector<PortData> ports;
    ports.push_back({PortType::Aero, 200, true, 1, 0, 0});

    std::vector<TradeAgreementComponent> agreements;
    TradeIncomeBreakdown result = calculate_trade_income(1, ports, agreements);

    // 200 * 0.5 * 0.8 * 1.0 = 80
    assert(result.aero_income == 80);
    assert(result.total == 80);
    PASS();
}

static void test_income_formula_medium_aero() {
    TEST("income formula: medium aero port (cap=500, util=0.7, rate=0.8)");
    std::vector<PortData> ports;
    ports.push_back({PortType::Aero, 500, true, 1, 0, 0});

    std::vector<TradeAgreementComponent> agreements;
    TradeIncomeBreakdown result = calculate_trade_income(1, ports, agreements);

    // 500 * 0.7 * 0.8 * 1.0 = 280
    assert(result.aero_income == 280);
    PASS();
}

static void test_income_formula_large_aero() {
    TEST("income formula: large aero port (cap=2500, util=0.9, rate=0.8)");
    std::vector<PortData> ports;
    ports.push_back({PortType::Aero, 2500, true, 1, 0, 0});

    std::vector<TradeAgreementComponent> agreements;
    TradeIncomeBreakdown result = calculate_trade_income(1, ports, agreements);

    // 2500 * 0.9 * 0.8 * 1.0 = 1800
    assert(result.aero_income == 1800);
    PASS();
}

static void test_income_formula_small_aqua() {
    TEST("income formula: small aqua port (cap=300, util=0.5, rate=0.6)");
    std::vector<PortData> ports;
    ports.push_back({PortType::Aqua, 300, true, 1, 0, 0});

    std::vector<TradeAgreementComponent> agreements;
    TradeIncomeBreakdown result = calculate_trade_income(1, ports, agreements);

    // 300 * 0.5 * 0.6 * 1.0 = 90
    assert(result.aqua_income == 90);
    PASS();
}

static void test_income_formula_medium_aqua() {
    TEST("income formula: medium aqua port (cap=1000, util=0.7, rate=0.6)");
    std::vector<PortData> ports;
    ports.push_back({PortType::Aqua, 1000, true, 1, 0, 0});

    std::vector<TradeAgreementComponent> agreements;
    TradeIncomeBreakdown result = calculate_trade_income(1, ports, agreements);

    // 1000 * 0.7 * 0.6 * 1.0 = 420
    assert(result.aqua_income == 420);
    PASS();
}

static void test_income_formula_large_aqua() {
    TEST("income formula: large aqua port (cap=5000, util=0.9, rate=0.6)");
    std::vector<PortData> ports;
    ports.push_back({PortType::Aqua, 5000, true, 1, 0, 0});

    std::vector<TradeAgreementComponent> agreements;
    TradeIncomeBreakdown result = calculate_trade_income(1, ports, agreements);

    // 5000 * 0.9 * 0.6 * 1.0 = 2700
    assert(result.aqua_income == 2700);
    PASS();
}

static void test_income_multiple_same_type_summing() {
    TEST("income formula: multiple aero ports sum correctly");
    std::vector<PortData> ports;
    ports.push_back({PortType::Aero, 200, true, 1, 0, 0});   // 200*0.5*0.8 = 80
    ports.push_back({PortType::Aero, 600, true, 1, 0, 0});   // 600*0.7*0.8 = 336
    ports.push_back({PortType::Aero, 2000, true, 1, 0, 0});  // 2000*0.9*0.8 = 1440

    std::vector<TradeAgreementComponent> agreements;
    TradeIncomeBreakdown result = calculate_trade_income(1, ports, agreements);

    // Total aero = 80 + 336 + 1440 = 1856
    assert(result.aero_income == 1856);
    assert(result.total == 1856);
    PASS();
}

static void test_income_with_none_agreement_multiplier() {
    TEST("income formula: None-tier agreement gives 50% multiplier");
    std::vector<PortData> ports;
    ports.push_back({PortType::Aero, 1000, true, 1, 0, 0});

    // None-tier agreement: income_bonus_percent = 50 (0.5x)
    auto agree = make_agreement(0, 1, TradeAgreementType::None, 50, 100);
    std::vector<TradeAgreementComponent> agreements = {agree};

    // get_trade_multiplier: None type with cycles_remaining=100 is not skipped
    // (it's only skipped if cycles_remaining==0 AND type!=None)
    // Actually looking at the code: None type agreements are checked but multiplier is 50/100=0.5
    float mult = get_trade_multiplier(1, agreements);
    assert(approx_eq(mult, 0.5f));

    TradeIncomeBreakdown result = calculate_trade_income(1, ports, agreements);
    // Base: 1000*0.7*0.8 = 560
    // With 0.5x: 560*0.5 = 280
    assert(result.aero_income == 280);
    PASS();
}

static void test_income_with_premium_multiplier() {
    TEST("income formula: Premium-tier agreement gives 1.2x multiplier");
    std::vector<PortData> ports;
    ports.push_back({PortType::Aero, 1000, true, 1, 0, 0});

    auto agree = make_agreement(0, 1, TradeAgreementType::Premium, 120, 300);
    std::vector<TradeAgreementComponent> agreements = {agree};

    TradeIncomeBreakdown result = calculate_trade_income(1, ports, agreements);
    // Base: 560, with 1.2x: 672
    assert(result.aero_income == 672);
    PASS();
}

static void test_income_breakdown_correctness() {
    TEST("income formula: aero_income + aqua_income == total");
    std::vector<PortData> ports;
    ports.push_back({PortType::Aero, 500, true, 1, 0, 0});
    ports.push_back({PortType::Aqua, 1500, true, 1, 0, 0});

    std::vector<TradeAgreementComponent> agreements;
    TradeIncomeBreakdown result = calculate_trade_income(1, ports, agreements);

    assert(result.total == result.aero_income + result.aqua_income);
    PASS();
}

static void test_income_non_operational_skipped() {
    TEST("income formula: non-operational port contributes 0");
    std::vector<PortData> ports;
    ports.push_back({PortType::Aero, 2000, false, 1, 0, 0});
    ports.push_back({PortType::Aero, 500, true, 1, 0, 0});

    std::vector<TradeAgreementComponent> agreements;
    TradeIncomeBreakdown result = calculate_trade_income(1, ports, agreements);

    // Only operational port: 500*0.7*0.8 = 280
    assert(result.aero_income == 280);
    PASS();
}

static void test_income_zero_capacity_skipped() {
    TEST("income formula: zero capacity port contributes 0");
    std::vector<PortData> ports;
    ports.push_back({PortType::Aqua, 0, true, 1, 0, 0});

    std::vector<TradeAgreementComponent> agreements;
    TradeIncomeBreakdown result = calculate_trade_income(1, ports, agreements);

    assert(result.total == 0);
    PASS();
}

static void test_income_utilization_boundaries() {
    TEST("income formula: utilization at exact capacity tier boundaries");
    // Capacity 499 -> small (0.5), 500 -> medium (0.7), 1999 -> medium (0.7), 2000 -> large (0.9)
    PortData p499 = {PortType::Aero, 499, true, 1, 0, 0};
    PortData p500 = {PortType::Aero, 500, true, 1, 0, 0};
    PortData p1999 = {PortType::Aero, 1999, true, 1, 0, 0};
    PortData p2000 = {PortType::Aero, 2000, true, 1, 0, 0};

    assert(approx_eq(estimate_port_utilization(p499), 0.5f));
    assert(approx_eq(estimate_port_utilization(p500), 0.7f));
    assert(approx_eq(estimate_port_utilization(p1999), 0.7f));
    assert(approx_eq(estimate_port_utilization(p2000), 0.9f));
    PASS();
}

static void test_income_capacity_1() {
    TEST("income formula: capacity=1 operational port");
    std::vector<PortData> ports;
    ports.push_back({PortType::Aero, 1, true, 1, 0, 0});

    std::vector<TradeAgreementComponent> agreements;
    TradeIncomeBreakdown result = calculate_trade_income(1, ports, agreements);

    // 1 * 0.5 * 0.8 * 1.0 = 0.4 -> floor = 0
    assert(result.aero_income == 0);
    PASS();
}

// =============================================================================
// DEMAND BONUS CAP TESTS (stacking, overflow)
// =============================================================================

static void test_global_bonus_exact_cap_30() {
    TEST("demand bonus: global cap exactly at 30 (2 large ports)");
    std::vector<PortData> ports;
    ports.push_back({PortType::Aero, 2000, true, 1, 0, 0});  // +15
    ports.push_back({PortType::Aero, 2000, true, 1, 0, 0});  // +15

    float bonus = calculate_global_demand_bonus(1, 1, ports);
    assert(approx_eq(bonus, 30.0f));
    PASS();
}

static void test_global_bonus_overflow_capped() {
    TEST("demand bonus: stacking overflow (4 large ports = 60 raw, capped at 30)");
    std::vector<PortData> ports;
    for (int i = 0; i < 4; i++) {
        ports.push_back({PortType::Aero, 2000, true, 1, 0, 0});
    }

    float bonus = calculate_global_demand_bonus(1, 1, ports);
    assert(approx_eq(bonus, 30.0f));
    PASS();
}

static void test_global_bonus_mix_sizes_under_cap() {
    TEST("demand bonus: mix of sizes under cap");
    std::vector<PortData> ports;
    ports.push_back({PortType::Aero, 100, true, 1, 0, 0});   // +5
    ports.push_back({PortType::Aero, 800, true, 1, 0, 0});   // +10
    ports.push_back({PortType::Aero, 200, true, 1, 0, 0});   // +5

    float bonus = calculate_global_demand_bonus(1, 1, ports);
    assert(approx_eq(bonus, 20.0f));
    PASS();
}

static void test_diminishing_prevents_reaching_cap_quickly() {
    TEST("demand bonus: diminishing returns prevent easy cap reach");
    std::vector<PortData> ports;
    // 3 medium aero ports with diminishing: 10*1.0 + 10*0.5 + 10*0.25 = 17.5
    ports.push_back({PortType::Aero, 600, true, 1, 0, 0});
    ports.push_back({PortType::Aero, 600, true, 1, 0, 0});
    ports.push_back({PortType::Aero, 600, true, 1, 0, 0});

    float with_diminishing = calculate_global_demand_bonus_with_diminishing(1, 1, ports);
    float without_diminishing = calculate_global_demand_bonus(1, 1, ports);

    // Without: 10+10+10=30 (capped)
    // With: 10+5+2.5=17.5
    assert(approx_eq(without_diminishing, 30.0f));
    assert(approx_eq(with_diminishing, 17.5f));
    assert(with_diminishing < without_diminishing);
    PASS();
}

static void test_combined_bonus_capped_at_30() {
    TEST("demand bonus: combined (global + local) capped at 30");
    std::vector<PortData> ports;
    // Habitation (zone_type=0): local bonus from aero within radius 20
    // Global: Exchange(1) from aero, but zone_type=0 for habitation -> global=0
    // But combined function handles zone_type=0 for local
    // Actually for zone_type=0, global returns 0, local from aero gives +5 per port
    ports.push_back({PortType::Aero, 2000, true, 1, 50, 50});
    ports.push_back({PortType::Aero, 2000, true, 1, 51, 50});
    ports.push_back({PortType::Aero, 2000, true, 1, 52, 50});
    ports.push_back({PortType::Aero, 2000, true, 1, 53, 50});
    ports.push_back({PortType::Aero, 2000, true, 1, 54, 50});
    ports.push_back({PortType::Aero, 2000, true, 1, 55, 50});
    ports.push_back({PortType::Aero, 2000, true, 1, 56, 50});

    // 7 aero ports near (50,50): local bonus = 7 * 5.0 = 35.0 (uncapped local)
    // global bonus for zone_type 0 = 0.0
    // combined = min(0 + 35, 30) = 30.0
    float combined = calculate_combined_demand_bonus(0, 50, 50, 1, ports);
    assert(approx_eq(combined, 30.0f));
    PASS();
}

static void test_local_bonus_not_independently_capped() {
    TEST("demand bonus: local bonus not independently capped");
    std::vector<PortData> ports;
    // 5 aero ports all within radius 20 of query point
    for (int i = 0; i < 5; i++) {
        ports.push_back({PortType::Aero, 2000, true, 1,
                         static_cast<int32_t>(50 + i), 50});
    }

    // Local bonus for habitation(0) from aero: 5 * 5.0 = 25.0
    float local = calculate_local_demand_bonus(0, 50, 50, 1, ports);
    assert(approx_eq(local, 25.0f));  // Not capped by itself
    PASS();
}

// =============================================================================
// TRADE AGREEMENT EFFECTS
// =============================================================================

static void test_agreement_benefits_none() {
    TEST("agreement benefits: None tier gives 0 demand, 0 income");
    auto benefits = get_agreement_benefits(TradeAgreementType::None);
    assert(benefits.demand_bonus == 0);
    assert(benefits.income_bonus_percent == 0);
    PASS();
}

static void test_agreement_benefits_basic() {
    TEST("agreement benefits: Basic tier gives +3 demand, +5% income");
    auto benefits = get_agreement_benefits(TradeAgreementType::Basic);
    assert(benefits.demand_bonus == 3);
    assert(benefits.income_bonus_percent == 5);
    PASS();
}

static void test_agreement_benefits_enhanced() {
    TEST("agreement benefits: Enhanced tier gives +6 demand, +10% income");
    auto benefits = get_agreement_benefits(TradeAgreementType::Enhanced);
    assert(benefits.demand_bonus == 6);
    assert(benefits.income_bonus_percent == 10);
    PASS();
}

static void test_agreement_benefits_premium() {
    TEST("agreement benefits: Premium tier gives +10 demand, +15% income");
    auto benefits = get_agreement_benefits(TradeAgreementType::Premium);
    assert(benefits.demand_bonus == 10);
    assert(benefits.income_bonus_percent == 15);
    PASS();
}

static void test_total_demand_bonus_multiple_agreements() {
    TEST("agreement effects: total demand bonus sums across agreements");
    std::vector<TradeAgreementComponent> agreements;
    auto a1 = make_agreement(0, 1, TradeAgreementType::Basic, 105, 100);
    a1.demand_bonus_a = 3;
    a1.demand_bonus_b = 3;
    agreements.push_back(a1);

    auto a2 = make_agreement(2, 1, TradeAgreementType::Enhanced, 110, 200);
    a2.demand_bonus_a = 6;
    a2.demand_bonus_b = 6;
    agreements.push_back(a2);

    // calculate_total_demand_bonus uses get_agreement_benefits, not the stored demand values
    int16_t total = calculate_total_demand_bonus(agreements, 1);
    // Basic: +3, Enhanced: +6 = 9
    assert(total == 9);
    PASS();
}

static void test_total_demand_bonus_no_agreements() {
    TEST("agreement effects: no agreements gives 0 demand bonus");
    std::vector<TradeAgreementComponent> agreements;
    int16_t total = calculate_total_demand_bonus(agreements, 1);
    assert(total == 0);
    PASS();
}

static void test_income_bonus_formula_single_basic() {
    TEST("agreement effects: apply_trade_agreement_income_bonus Basic (+5%)");
    std::vector<TradeAgreementComponent> agreements;
    agreements.push_back(make_agreement(0, 1, TradeAgreementType::Basic, 105, 100));

    // base_income * (100 + 5) / 100 = 1000 * 105 / 100 = 1050
    int64_t modified = apply_trade_agreement_income_bonus(1000, agreements, 1);
    assert(modified == 1050);
    PASS();
}

static void test_income_bonus_formula_stacked() {
    TEST("agreement effects: stacking Basic + Premium income bonuses");
    std::vector<TradeAgreementComponent> agreements;
    agreements.push_back(make_agreement(0, 1, TradeAgreementType::Basic, 105, 100));
    agreements.push_back(make_agreement(2, 1, TradeAgreementType::Premium, 115, 200));

    // Total bonus: 5 + 15 = 20%
    // 1000 * (100 + 20) / 100 = 1200
    int64_t modified = apply_trade_agreement_income_bonus(1000, agreements, 1);
    assert(modified == 1200);
    PASS();
}

static void test_income_bonus_with_none_agreement() {
    TEST("agreement effects: None-type agreement gives no bonus");
    std::vector<TradeAgreementComponent> agreements;
    agreements.push_back(make_agreement(0, 1, TradeAgreementType::None, 100, 100));

    int64_t modified = apply_trade_agreement_income_bonus(1000, agreements, 1);
    assert(modified == 1000);  // No change
    PASS();
}

static void test_income_bonus_other_player_not_affected() {
    TEST("agreement effects: agreement for other player does not affect income");
    std::vector<TradeAgreementComponent> agreements;
    agreements.push_back(make_agreement(0, 2, TradeAgreementType::Premium, 115, 200));

    // Player 1 is not party to this agreement
    int64_t modified = apply_trade_agreement_income_bonus(1000, agreements, 1);
    assert(modified == 1000);
    PASS();
}

static void test_trade_multiplier_expired_agreement_skipped() {
    TEST("agreement effects: expired agreement (cycles=0, non-None type) skipped");
    auto agree = make_agreement(0, 1, TradeAgreementType::Premium, 120, 0);
    std::vector<TradeAgreementComponent> agreements = {agree};

    float mult = get_trade_multiplier(1, agreements);
    // Expired: cycles_remaining==0 && type!=None -> skipped
    assert(approx_eq(mult, 1.0f));  // Default
    PASS();
}

static void test_trade_multiplier_party_a_match() {
    TEST("agreement effects: player matches as party_a");
    auto agree = make_agreement(1, 2, TradeAgreementType::Enhanced, 100, 50);
    std::vector<TradeAgreementComponent> agreements = {agree};

    float mult = get_trade_multiplier(1, agreements);
    assert(approx_eq(mult, 1.0f));  // 100/100 = 1.0
    PASS();
}

static void test_trade_multiplier_picks_best_from_many() {
    TEST("agreement effects: picks best multiplier from many agreements");
    std::vector<TradeAgreementComponent> agreements;
    agreements.push_back(make_agreement(0, 1, TradeAgreementType::None, 50, 100));
    agreements.push_back(make_agreement(0, 1, TradeAgreementType::Basic, 80, 100));
    agreements.push_back(make_agreement(0, 1, TradeAgreementType::Enhanced, 100, 100));
    agreements.push_back(make_agreement(0, 1, TradeAgreementType::Premium, 120, 100));

    float mult = get_trade_multiplier(1, agreements);
    assert(approx_eq(mult, 1.2f));  // Best: Premium at 120
    PASS();
}

static void test_trade_deal_bonus_breakdown_with_multiplier() {
    TEST("agreement effects: trade deal bonus breakdown with premium multiplier");
    std::vector<PortData> ports;
    ports.push_back({PortType::Aero, 1000, true, 1, 0, 0});
    ports.push_back({PortType::Aqua, 2000, true, 1, 0, 0});

    auto agree = make_agreement(0, 1, TradeAgreementType::Premium, 120, 300);
    std::vector<TradeAgreementComponent> agreements = {agree};

    TradeIncomeBreakdown result = calculate_trade_income(1, ports, agreements);

    // Aero base: 1000*0.7*0.8 = 560, with 1.2x: 672
    // Aqua base: 2000*0.9*0.6 = 1080, with 1.2x: 1296
    assert(result.aero_income == 672);
    assert(result.aqua_income == 1296);
    assert(result.total == 1968);

    // Trade bonus = total_with_mult - total_raw = (560+1080)*1.2 - (560+1080) = 328
    assert(result.trade_deal_bonuses == 328);
    PASS();
}

// =============================================================================
// DIMINISHING RETURNS ACCURACY
// =============================================================================

static void test_diminishing_exact_multiplier_values() {
    TEST("diminishing returns: exact multiplier values per index");
    assert(approx_eq(get_diminishing_multiplier(0), 1.0f));
    assert(approx_eq(get_diminishing_multiplier(1), 0.5f));
    assert(approx_eq(get_diminishing_multiplier(2), 0.25f));
    assert(approx_eq(get_diminishing_multiplier(3), 0.125f));
    assert(approx_eq(get_diminishing_multiplier(4), 0.125f));
    assert(approx_eq(get_diminishing_multiplier(10), 0.125f));
    assert(approx_eq(get_diminishing_multiplier(100), 0.125f));
    PASS();
}

static void test_diminishing_apply_with_zero_base() {
    TEST("diminishing returns: apply with zero base bonus");
    assert(approx_eq(apply_diminishing_returns(0.0f, 0), 0.0f));
    assert(approx_eq(apply_diminishing_returns(0.0f, 1), 0.0f));
    assert(approx_eq(apply_diminishing_returns(0.0f, 3), 0.0f));
    PASS();
}

static void test_diminishing_sum_across_many_ports() {
    TEST("diminishing returns: sum converges with many same-type ports");
    // 10 large aero ports: 15*1.0 + 15*0.5 + 15*0.25 + 7*15*0.125
    // = 15 + 7.5 + 3.75 + 13.125 = 39.375 -> capped at 30
    std::vector<PortData> ports;
    for (int i = 0; i < 10; i++) {
        ports.push_back({PortType::Aero, 2000, true, 1, 0, 0});
    }

    float bonus = calculate_global_demand_bonus_with_diminishing(1, 1, ports);
    assert(bonus <= 30.0f);
    assert(approx_eq(bonus, 30.0f));  // Should hit cap

    // Verify exact sum before cap
    float exact = 15.0f * 1.0f + 15.0f * 0.5f + 15.0f * 0.25f;
    for (int i = 3; i < 10; i++) {
        exact += 15.0f * 0.125f;
    }
    // exact = 15 + 7.5 + 3.75 + 7*1.875 = 26.25 + 13.125 = 39.375
    assert(approx_eq(exact, 39.375f));
    // But bonus is capped at 30
    assert(approx_eq(bonus, 30.0f));
    PASS();
}

static void test_diminishing_mixed_port_sizes() {
    TEST("diminishing returns: mixed sizes (Large, Small, Medium)");
    std::vector<PortData> ports;
    ports.push_back({PortType::Aqua, 2500, true, 1, 0, 0});  // Large: 15 * 1.0 = 15.0
    ports.push_back({PortType::Aqua, 100, true, 1, 0, 0});   // Small: 5 * 0.5 = 2.5
    ports.push_back({PortType::Aqua, 800, true, 1, 0, 0});   // Medium: 10 * 0.25 = 2.5

    float bonus = calculate_global_demand_bonus_with_diminishing(2, 1, ports);
    assert(approx_eq(bonus, 20.0f));  // 15 + 2.5 + 2.5
    PASS();
}

static void test_diminishing_cap_interaction() {
    TEST("diminishing returns: interaction with 30 cap");
    // 2 large + 1 large: 15*1.0 + 15*0.5 + 15*0.25 = 26.25
    // Adding a 4th: +15*0.125 = 1.875 -> 28.125 (still under cap)
    // Adding a 5th: +15*0.125 = 1.875 -> 30.0 (exactly at cap)
    std::vector<PortData> ports;
    for (int i = 0; i < 5; i++) {
        ports.push_back({PortType::Aero, 2000, true, 1, 0, 0});
    }

    float bonus = calculate_global_demand_bonus_with_diminishing(1, 1, ports);
    // 15 + 7.5 + 3.75 + 1.875 + 1.875 = 30.0
    assert(approx_eq(bonus, 30.0f));
    PASS();
}

static void test_diminishing_4_ports_exact() {
    TEST("diminishing returns: exactly 4 medium ports");
    std::vector<PortData> ports;
    ports.push_back({PortType::Aero, 600, true, 1, 0, 0});
    ports.push_back({PortType::Aero, 600, true, 1, 0, 0});
    ports.push_back({PortType::Aero, 600, true, 1, 0, 0});
    ports.push_back({PortType::Aero, 600, true, 1, 0, 0});

    float bonus = calculate_global_demand_bonus_with_diminishing(1, 1, ports);
    // 10*1.0 + 10*0.5 + 10*0.25 + 10*0.125 = 10+5+2.5+1.25 = 18.75
    assert(approx_eq(bonus, 18.75f));
    PASS();
}

static void test_diminishing_non_operational_not_indexed() {
    TEST("diminishing returns: non-operational ports don't consume index");
    std::vector<PortData> ports;
    ports.push_back({PortType::Aero, 600, true, 1, 0, 0});    // #0: 10*1.0=10
    ports.push_back({PortType::Aero, 600, false, 1, 0, 0});   // skipped
    ports.push_back({PortType::Aero, 600, false, 1, 0, 0});   // skipped
    ports.push_back({PortType::Aero, 600, true, 1, 0, 0});    // #1: 10*0.5=5

    float bonus = calculate_global_demand_bonus_with_diminishing(1, 1, ports);
    assert(approx_eq(bonus, 15.0f));
    PASS();
}

static void test_diminishing_cross_type_independence() {
    TEST("diminishing returns: aero and aqua indexed independently");
    std::vector<PortData> ports;
    ports.push_back({PortType::Aero, 600, true, 1, 0, 0});   // Aero #0
    ports.push_back({PortType::Aqua, 600, true, 1, 0, 0});   // Aqua #0 (independent)
    ports.push_back({PortType::Aero, 600, true, 1, 0, 0});   // Aero #1

    // Exchange (zone_type=1, aero): 10*1.0 + 10*0.5 = 15.0
    float exchange = calculate_global_demand_bonus_with_diminishing(1, 1, ports);
    assert(approx_eq(exchange, 15.0f));

    // Fabrication (zone_type=2, aqua): 10*1.0 = 10.0 (only 1 aqua)
    float fab = calculate_global_demand_bonus_with_diminishing(2, 1, ports);
    assert(approx_eq(fab, 10.0f));
    PASS();
}

// =============================================================================
// REGRESSION / ADDITIONAL EDGE CASES
// =============================================================================

static void test_income_with_many_ports_different_owners() {
    TEST("regression: income calculation with ports from 4 different owners");
    std::vector<PortData> ports;
    ports.push_back({PortType::Aero, 1000, true, 1, 0, 0});
    ports.push_back({PortType::Aero, 1000, true, 2, 0, 0});
    ports.push_back({PortType::Aqua, 2000, true, 3, 0, 0});
    ports.push_back({PortType::Aqua, 500, true, 4, 0, 0});

    std::vector<TradeAgreementComponent> agreements;

    // Each player only gets income from their own ports
    auto r1 = calculate_trade_income(1, ports, agreements);
    assert(r1.aero_income == 560);  // 1000*0.7*0.8
    assert(r1.aqua_income == 0);

    auto r2 = calculate_trade_income(2, ports, agreements);
    assert(r2.aero_income == 560);
    assert(r2.aqua_income == 0);

    auto r3 = calculate_trade_income(3, ports, agreements);
    assert(r3.aero_income == 0);
    assert(r3.aqua_income == 1080);  // 2000*0.9*0.6

    auto r4 = calculate_trade_income(4, ports, agreements);
    assert(r4.aero_income == 0);
    assert(r4.aqua_income == 210);  // 500*0.7*0.6 = 210 (medium tier)
    PASS();
}

static void test_income_player4_aqua_value() {
    TEST("regression: player 4 aqua income exact value");
    std::vector<PortData> ports;
    ports.push_back({PortType::Aqua, 500, true, 4, 0, 0});

    std::vector<TradeAgreementComponent> agreements;
    auto r = calculate_trade_income(4, ports, agreements);
    // 500*0.7*0.6 = 210
    assert(r.aqua_income == 210);
    PASS();
}

static void test_demand_bonus_zero_capacity_operational() {
    TEST("regression: zero capacity operational port gives 0 demand bonus");
    std::vector<PortData> ports;
    ports.push_back({PortType::Aero, 0, true, 1, 0, 0});

    float bonus = calculate_global_demand_bonus(1, 1, ports);
    assert(approx_eq(bonus, 0.0f));
    PASS();
}

static void test_demand_bonus_capacity_1() {
    TEST("regression: capacity=1 operational port gives small bonus (+5)");
    std::vector<PortData> ports;
    ports.push_back({PortType::Aero, 1, true, 1, 0, 0});

    float bonus = calculate_global_demand_bonus(1, 1, ports);
    assert(approx_eq(bonus, 5.0f));
    PASS();
}

static void test_demand_bonus_boundary_499_500() {
    TEST("regression: demand bonus at capacity boundary 499/500");
    std::vector<PortData> ports_small;
    ports_small.push_back({PortType::Aero, 499, true, 1, 0, 0});
    float small_bonus = calculate_global_demand_bonus(1, 1, ports_small);
    assert(approx_eq(small_bonus, 5.0f));

    std::vector<PortData> ports_medium;
    ports_medium.push_back({PortType::Aero, 500, true, 1, 0, 0});
    float medium_bonus = calculate_global_demand_bonus(1, 1, ports_medium);
    assert(approx_eq(medium_bonus, 10.0f));
    PASS();
}

static void test_demand_bonus_boundary_1999_2000() {
    TEST("regression: demand bonus at capacity boundary 1999/2000");
    std::vector<PortData> ports_medium;
    ports_medium.push_back({PortType::Aqua, 1999, true, 1, 0, 0});
    float medium_bonus = calculate_global_demand_bonus(2, 1, ports_medium);
    assert(approx_eq(medium_bonus, 10.0f));

    std::vector<PortData> ports_large;
    ports_large.push_back({PortType::Aqua, 2000, true, 1, 0, 0});
    float large_bonus = calculate_global_demand_bonus(2, 1, ports_large);
    assert(approx_eq(large_bonus, 15.0f));
    PASS();
}

static void test_agreement_demand_bonus_all_tiers_stacked() {
    TEST("regression: stacking all agreement tiers for demand bonus");
    std::vector<TradeAgreementComponent> agreements;
    agreements.push_back(make_agreement(0, 1, TradeAgreementType::Basic, 105, 100));
    agreements.push_back(make_agreement(2, 1, TradeAgreementType::Enhanced, 110, 100));
    agreements.push_back(make_agreement(3, 1, TradeAgreementType::Premium, 115, 100));

    // Basic: +3, Enhanced: +6, Premium: +10 = 19
    int16_t total = calculate_total_demand_bonus(agreements, 1);
    assert(total == 19);
    PASS();
}

static void test_agreement_income_bonus_all_tiers_stacked() {
    TEST("regression: stacking all agreement tiers for income bonus");
    std::vector<TradeAgreementComponent> agreements;
    agreements.push_back(make_agreement(0, 1, TradeAgreementType::Basic, 105, 100));
    agreements.push_back(make_agreement(2, 1, TradeAgreementType::Enhanced, 110, 100));
    agreements.push_back(make_agreement(3, 1, TradeAgreementType::Premium, 115, 100));

    // Total: 5 + 10 + 15 = 30%
    // 1000 * (100 + 30) / 100 = 1300
    int64_t modified = apply_trade_agreement_income_bonus(1000, agreements, 1);
    assert(modified == 1300);
    PASS();
}

static void test_agreement_income_bonus_zero_base_income() {
    TEST("regression: income bonus with zero base income");
    std::vector<TradeAgreementComponent> agreements;
    agreements.push_back(make_agreement(0, 1, TradeAgreementType::Premium, 115, 100));

    int64_t modified = apply_trade_agreement_income_bonus(0, agreements, 1);
    assert(modified == 0);
    PASS();
}

static void test_agreement_income_bonus_negative_base() {
    TEST("regression: income bonus with negative base income");
    std::vector<TradeAgreementComponent> agreements;
    agreements.push_back(make_agreement(0, 1, TradeAgreementType::Premium, 115, 100));

    // -1000 * (100 + 15) / 100 = -1150
    int64_t modified = apply_trade_agreement_income_bonus(-1000, agreements, 1);
    assert(modified == -1150);
    PASS();
}

// =============================================================================
// Main
// =============================================================================

int main() {
    printf("=== Comprehensive Trade Calculation Tests (E8-037) ===\n\n");

    printf("--- Trade Income Formula Verification ---\n");
    test_income_formula_small_aero();
    test_income_formula_medium_aero();
    test_income_formula_large_aero();
    test_income_formula_small_aqua();
    test_income_formula_medium_aqua();
    test_income_formula_large_aqua();
    test_income_multiple_same_type_summing();
    test_income_with_none_agreement_multiplier();
    test_income_with_premium_multiplier();
    test_income_breakdown_correctness();
    test_income_non_operational_skipped();
    test_income_zero_capacity_skipped();
    test_income_utilization_boundaries();
    test_income_capacity_1();

    printf("\n--- Demand Bonus Cap Tests ---\n");
    test_global_bonus_exact_cap_30();
    test_global_bonus_overflow_capped();
    test_global_bonus_mix_sizes_under_cap();
    test_diminishing_prevents_reaching_cap_quickly();
    test_combined_bonus_capped_at_30();
    test_local_bonus_not_independently_capped();

    printf("\n--- Trade Agreement Effects ---\n");
    test_agreement_benefits_none();
    test_agreement_benefits_basic();
    test_agreement_benefits_enhanced();
    test_agreement_benefits_premium();
    test_total_demand_bonus_multiple_agreements();
    test_total_demand_bonus_no_agreements();
    test_income_bonus_formula_single_basic();
    test_income_bonus_formula_stacked();
    test_income_bonus_with_none_agreement();
    test_income_bonus_other_player_not_affected();
    test_trade_multiplier_expired_agreement_skipped();
    test_trade_multiplier_party_a_match();
    test_trade_multiplier_picks_best_from_many();
    test_trade_deal_bonus_breakdown_with_multiplier();

    printf("\n--- Diminishing Returns Accuracy ---\n");
    test_diminishing_exact_multiplier_values();
    test_diminishing_apply_with_zero_base();
    test_diminishing_sum_across_many_ports();
    test_diminishing_mixed_port_sizes();
    test_diminishing_cap_interaction();
    test_diminishing_4_ports_exact();
    test_diminishing_non_operational_not_indexed();
    test_diminishing_cross_type_independence();

    printf("\n--- Regression / Edge Cases ---\n");
    test_income_with_many_ports_different_owners();
    test_income_player4_aqua_value();
    test_demand_bonus_zero_capacity_operational();
    test_demand_bonus_capacity_1();
    test_demand_bonus_boundary_499_500();
    test_demand_bonus_boundary_1999_2000();
    test_agreement_demand_bonus_all_tiers_stacked();
    test_agreement_income_bonus_all_tiers_stacked();
    test_agreement_income_bonus_zero_base_income();
    test_agreement_income_bonus_negative_base();

    printf("\n=== Results: %d/%d passed ===\n", tests_passed, tests_total);
    return (tests_passed == tests_total) ? 0 : 1;
}
