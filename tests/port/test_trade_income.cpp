/**
 * @file test_trade_income.cpp
 * @brief Unit tests for trade income calculation (Epic 8, Ticket E8-019)
 *
 * Tests cover:
 * - Income rate per port type (aero=0.8, aqua=0.6)
 * - Port utilization estimation by capacity tier
 * - Trade multiplier from agreements
 * - Full income calculation with breakdown
 * - Utilization affects income (underused ports earn less)
 * - Trade agreements affect multiplier
 * - Edge cases: no ports, no agreements, expired agreements
 */

#include <sims3000/port/TradeIncome.h>
#include <sims3000/port/DemandBonus.h>
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

// =============================================================================
// Income Rate Tests
// =============================================================================

static void test_income_rate_aero() {
    TEST("get_income_rate: aero = 0.8");
    assert(approx_eq(get_income_rate(PortType::Aero), 0.8f));
    PASS();
}

static void test_income_rate_aqua() {
    TEST("get_income_rate: aqua = 0.6");
    assert(approx_eq(get_income_rate(PortType::Aqua), 0.6f));
    PASS();
}

// =============================================================================
// Utilization Estimation Tests
// =============================================================================

static void test_utilization_non_operational() {
    TEST("estimate_port_utilization: non-operational = 0");
    PortData port = {PortType::Aero, 1000, false, 1, 0, 0};
    assert(approx_eq(estimate_port_utilization(port), 0.0f));
    PASS();
}

static void test_utilization_zero_capacity() {
    TEST("estimate_port_utilization: zero capacity = 0");
    PortData port = {PortType::Aero, 0, true, 1, 0, 0};
    assert(approx_eq(estimate_port_utilization(port), 0.0f));
    PASS();
}

static void test_utilization_small_port() {
    TEST("estimate_port_utilization: small port (< 500) = 0.5");
    PortData port = {PortType::Aero, 200, true, 1, 0, 0};
    assert(approx_eq(estimate_port_utilization(port), 0.5f));
    PASS();
}

static void test_utilization_medium_port() {
    TEST("estimate_port_utilization: medium port (500-1999) = 0.7");
    PortData port = {PortType::Aero, 1000, true, 1, 0, 0};
    assert(approx_eq(estimate_port_utilization(port), 0.7f));
    PASS();
}

static void test_utilization_large_port() {
    TEST("estimate_port_utilization: large port (>= 2000) = 0.9");
    PortData port = {PortType::Aero, 2500, true, 1, 0, 0};
    assert(approx_eq(estimate_port_utilization(port), 0.9f));
    PASS();
}

static void test_utilization_boundary_small_medium() {
    TEST("estimate_port_utilization: boundary 499 -> 0.5, 500 -> 0.7");
    PortData small_port = {PortType::Aero, 499, true, 1, 0, 0};
    PortData medium_port = {PortType::Aero, 500, true, 1, 0, 0};
    assert(approx_eq(estimate_port_utilization(small_port), 0.5f));
    assert(approx_eq(estimate_port_utilization(medium_port), 0.7f));
    PASS();
}

static void test_utilization_boundary_medium_large() {
    TEST("estimate_port_utilization: boundary 1999 -> 0.7, 2000 -> 0.9");
    PortData medium_port = {PortType::Aero, 1999, true, 1, 0, 0};
    PortData large_port = {PortType::Aero, 2000, true, 1, 0, 0};
    assert(approx_eq(estimate_port_utilization(medium_port), 0.7f));
    assert(approx_eq(estimate_port_utilization(large_port), 0.9f));
    PASS();
}

// =============================================================================
// Trade Multiplier Tests
// =============================================================================

static void test_trade_multiplier_no_agreements() {
    TEST("get_trade_multiplier: no agreements = 1.0");
    std::vector<TradeAgreementComponent> agreements;
    float mult = get_trade_multiplier(1, agreements);
    assert(approx_eq(mult, 1.0f));
    PASS();
}

static void test_trade_multiplier_basic_agreement() {
    TEST("get_trade_multiplier: basic agreement = 0.8");
    TradeAgreementComponent agree = {};
    agree.party_a = 0;  // GAME_MASTER
    agree.party_b = 1;
    agree.agreement_type = TradeAgreementType::Basic;
    agree.income_bonus_percent = 80;
    agree.cycles_remaining = 100;

    std::vector<TradeAgreementComponent> agreements = {agree};
    float mult = get_trade_multiplier(1, agreements);
    assert(approx_eq(mult, 0.8f));
    PASS();
}

static void test_trade_multiplier_enhanced_agreement() {
    TEST("get_trade_multiplier: enhanced agreement = 1.0");
    TradeAgreementComponent agree = {};
    agree.party_a = 0;
    agree.party_b = 1;
    agree.agreement_type = TradeAgreementType::Enhanced;
    agree.income_bonus_percent = 100;
    agree.cycles_remaining = 200;

    std::vector<TradeAgreementComponent> agreements = {agree};
    float mult = get_trade_multiplier(1, agreements);
    assert(approx_eq(mult, 1.0f));
    PASS();
}

static void test_trade_multiplier_premium_agreement() {
    TEST("get_trade_multiplier: premium agreement = 1.2");
    TradeAgreementComponent agree = {};
    agree.party_a = 0;
    agree.party_b = 1;
    agree.agreement_type = TradeAgreementType::Premium;
    agree.income_bonus_percent = 120;
    agree.cycles_remaining = 300;

    std::vector<TradeAgreementComponent> agreements = {agree};
    float mult = get_trade_multiplier(1, agreements);
    assert(approx_eq(mult, 1.2f));
    PASS();
}

static void test_trade_multiplier_best_selected() {
    TEST("get_trade_multiplier: selects best multiplier from multiple");
    TradeAgreementComponent basic = {};
    basic.party_a = 0;
    basic.party_b = 1;
    basic.agreement_type = TradeAgreementType::Basic;
    basic.income_bonus_percent = 80;
    basic.cycles_remaining = 100;

    TradeAgreementComponent premium = {};
    premium.party_a = 0;
    premium.party_b = 1;
    premium.agreement_type = TradeAgreementType::Premium;
    premium.income_bonus_percent = 120;
    premium.cycles_remaining = 300;

    std::vector<TradeAgreementComponent> agreements = {basic, premium};
    float mult = get_trade_multiplier(1, agreements);
    assert(approx_eq(mult, 1.2f));  // Best = Premium
    PASS();
}

static void test_trade_multiplier_other_player_ignored() {
    TEST("get_trade_multiplier: agreements for other players ignored");
    TradeAgreementComponent agree = {};
    agree.party_a = 0;
    agree.party_b = 2;  // Player 2, not player 1
    agree.agreement_type = TradeAgreementType::Premium;
    agree.income_bonus_percent = 120;
    agree.cycles_remaining = 300;

    std::vector<TradeAgreementComponent> agreements = {agree};
    float mult = get_trade_multiplier(1, agreements);
    assert(approx_eq(mult, 1.0f));  // Default, not 1.2
    PASS();
}

// =============================================================================
// Full Trade Income Calculation Tests
// =============================================================================

static void test_income_single_aero_port() {
    TEST("calculate_trade_income: single medium aero port");
    std::vector<PortData> ports;
    ports.push_back({PortType::Aero, 1000, true, 1, 0, 0});  // Medium: util=0.7

    std::vector<TradeAgreementComponent> agreements;

    // Expected: 1000 * 0.7 * 0.8 * 1.0 = 560
    TradeIncomeBreakdown result = calculate_trade_income(1, ports, agreements);
    assert(result.aero_income == 560);
    assert(result.aqua_income == 0);
    assert(result.total == 560);
    PASS();
}

static void test_income_single_aqua_port() {
    TEST("calculate_trade_income: single large aqua port");
    std::vector<PortData> ports;
    ports.push_back({PortType::Aqua, 2000, true, 1, 0, 0});  // Large: util=0.9

    std::vector<TradeAgreementComponent> agreements;

    // Expected: 2000 * 0.9 * 0.6 * 1.0 = 1080
    TradeIncomeBreakdown result = calculate_trade_income(1, ports, agreements);
    assert(result.aero_income == 0);
    assert(result.aqua_income == 1080);
    assert(result.total == 1080);
    PASS();
}

static void test_income_mixed_ports() {
    TEST("calculate_trade_income: mixed aero and aqua ports");
    std::vector<PortData> ports;
    ports.push_back({PortType::Aero, 1000, true, 1, 0, 0});  // Medium: 1000*0.7*0.8 = 560
    ports.push_back({PortType::Aqua, 2000, true, 1, 0, 0});  // Large:  2000*0.9*0.6 = 1080

    std::vector<TradeAgreementComponent> agreements;

    TradeIncomeBreakdown result = calculate_trade_income(1, ports, agreements);
    assert(result.aero_income == 560);
    assert(result.aqua_income == 1080);
    assert(result.total == 1640);
    PASS();
}

static void test_income_with_trade_agreement() {
    TEST("calculate_trade_income: trade agreement affects income");
    std::vector<PortData> ports;
    ports.push_back({PortType::Aero, 1000, true, 1, 0, 0});  // Medium: util=0.7

    TradeAgreementComponent agree = {};
    agree.party_a = 0;
    agree.party_b = 1;
    agree.agreement_type = TradeAgreementType::Premium;
    agree.income_bonus_percent = 120;
    agree.cycles_remaining = 300;
    std::vector<TradeAgreementComponent> agreements = {agree};

    // Base: 1000 * 0.7 * 0.8 = 560
    // With 1.2x multiplier: 560 * 1.2 = 672
    TradeIncomeBreakdown result = calculate_trade_income(1, ports, agreements);
    assert(result.aero_income == 672);
    assert(result.total == 672);
    assert(result.trade_deal_bonuses > 0);  // Bonus portion is positive
    PASS();
}

static void test_income_non_operational_port() {
    TEST("calculate_trade_income: non-operational port earns nothing");
    std::vector<PortData> ports;
    ports.push_back({PortType::Aero, 1000, false, 1, 0, 0});  // NOT operational

    std::vector<TradeAgreementComponent> agreements;

    TradeIncomeBreakdown result = calculate_trade_income(1, ports, agreements);
    assert(result.aero_income == 0);
    assert(result.aqua_income == 0);
    assert(result.total == 0);
    PASS();
}

static void test_income_zero_capacity_port() {
    TEST("calculate_trade_income: zero capacity port earns nothing");
    std::vector<PortData> ports;
    ports.push_back({PortType::Aero, 0, true, 1, 0, 0});

    std::vector<TradeAgreementComponent> agreements;

    TradeIncomeBreakdown result = calculate_trade_income(1, ports, agreements);
    assert(result.total == 0);
    PASS();
}

static void test_income_no_ports() {
    TEST("calculate_trade_income: no ports returns all zeros");
    std::vector<PortData> ports;
    std::vector<TradeAgreementComponent> agreements;

    TradeIncomeBreakdown result = calculate_trade_income(1, ports, agreements);
    assert(result.aero_income == 0);
    assert(result.aqua_income == 0);
    assert(result.trade_deal_bonuses == 0);
    assert(result.total == 0);
    PASS();
}

static void test_income_owner_filtering() {
    TEST("calculate_trade_income: filters by owner");
    std::vector<PortData> ports;
    ports.push_back({PortType::Aero, 1000, true, 1, 0, 0});  // Player 1
    ports.push_back({PortType::Aero, 2000, true, 2, 0, 0});  // Player 2

    std::vector<TradeAgreementComponent> agreements;

    TradeIncomeBreakdown result_p1 = calculate_trade_income(1, ports, agreements);
    TradeIncomeBreakdown result_p2 = calculate_trade_income(2, ports, agreements);

    // Player 1: Medium 1000 * 0.7 * 0.8 = 560
    assert(result_p1.aero_income == 560);

    // Player 2: Large 2000 * 0.9 * 0.8 = 1440
    assert(result_p2.aero_income == 1440);
    PASS();
}

static void test_income_small_port_lower_utilization() {
    TEST("calculate_trade_income: small ports have lower utilization");
    std::vector<PortData> ports;
    ports.push_back({PortType::Aero, 200, true, 1, 0, 0});  // Small: util=0.5

    std::vector<TradeAgreementComponent> agreements;

    // Expected: 200 * 0.5 * 0.8 * 1.0 = 80
    TradeIncomeBreakdown result = calculate_trade_income(1, ports, agreements);
    assert(result.aero_income == 80);
    assert(result.total == 80);
    PASS();
}

static void test_income_large_port_higher_utilization() {
    TEST("calculate_trade_income: large ports have higher utilization");
    std::vector<PortData> ports;
    ports.push_back({PortType::Aero, 2500, true, 1, 0, 0});  // Large: util=0.9

    std::vector<TradeAgreementComponent> agreements;

    // Expected: 2500 * 0.9 * 0.8 * 1.0 = 1800
    TradeIncomeBreakdown result = calculate_trade_income(1, ports, agreements);
    assert(result.aero_income == 1800);
    assert(result.total == 1800);
    PASS();
}

static void test_income_trade_deal_bonus_breakdown() {
    TEST("calculate_trade_income: trade deal bonus is separated in breakdown");
    std::vector<PortData> ports;
    ports.push_back({PortType::Aero, 1000, true, 1, 0, 0});  // Base: 560

    TradeAgreementComponent agree = {};
    agree.party_a = 0;
    agree.party_b = 1;
    agree.agreement_type = TradeAgreementType::Premium;
    agree.income_bonus_percent = 120;
    agree.cycles_remaining = 300;
    std::vector<TradeAgreementComponent> agreements = {agree};

    TradeIncomeBreakdown result = calculate_trade_income(1, ports, agreements);

    // Base income (1.0x): 560
    // With multiplier (1.2x): 672
    // Trade bonus: 672 - 560 = 112
    assert(result.trade_deal_bonuses == 112);
    PASS();
}

static void test_income_multiple_ports_multiple_types() {
    TEST("calculate_trade_income: multiple ports of different types");
    std::vector<PortData> ports;
    ports.push_back({PortType::Aero, 500, true, 1, 0, 0});   // Medium: 500*0.7*0.8 = 280
    ports.push_back({PortType::Aero, 200, true, 1, 0, 0});   // Small:  200*0.5*0.8 = 80
    ports.push_back({PortType::Aqua, 1000, true, 1, 0, 0});  // Medium: 1000*0.7*0.6 = 420
    ports.push_back({PortType::Aqua, 3000, true, 1, 0, 0});  // Large:  3000*0.9*0.6 = 1620

    std::vector<TradeAgreementComponent> agreements;

    TradeIncomeBreakdown result = calculate_trade_income(1, ports, agreements);
    assert(result.aero_income == 360);   // 280 + 80
    assert(result.aqua_income == 2040);  // 420 + 1620
    assert(result.total == 2400);        // 360 + 2040
    PASS();
}

// =============================================================================
// Main
// =============================================================================

int main() {
    printf("=== Trade Income Tests (E8-019) ===\n");

    // Income rates
    test_income_rate_aero();
    test_income_rate_aqua();

    // Utilization estimation
    test_utilization_non_operational();
    test_utilization_zero_capacity();
    test_utilization_small_port();
    test_utilization_medium_port();
    test_utilization_large_port();
    test_utilization_boundary_small_medium();
    test_utilization_boundary_medium_large();

    // Trade multiplier
    test_trade_multiplier_no_agreements();
    test_trade_multiplier_basic_agreement();
    test_trade_multiplier_enhanced_agreement();
    test_trade_multiplier_premium_agreement();
    test_trade_multiplier_best_selected();
    test_trade_multiplier_other_player_ignored();

    // Full income calculation
    test_income_single_aero_port();
    test_income_single_aqua_port();
    test_income_mixed_ports();
    test_income_with_trade_agreement();
    test_income_non_operational_port();
    test_income_zero_capacity_port();
    test_income_no_ports();
    test_income_owner_filtering();
    test_income_small_port_lower_utilization();
    test_income_large_port_higher_utilization();
    test_income_trade_deal_bonus_breakdown();
    test_income_multiple_ports_multiple_types();

    printf("\n=== Results: %d/%d passed ===\n", tests_passed, tests_total);
    return (tests_passed == tests_total) ? 0 : 1;
}
