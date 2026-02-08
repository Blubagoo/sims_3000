/**
 * @file test_port_economy_integration.cpp
 * @brief Unit tests for EconomySystem integration (Epic 8, Ticket E8-020)
 *
 * Tests cover:
 * - get_trade_income_breakdown() returns correct breakdown per player
 * - PortSystem::tick() wires calculate_trade_income() to real TradeIncome functions
 * - Trade agreement management (add, clear, get)
 * - IPortProvider::get_trade_income() updated by tick
 * - Income breakdown available for UI display
 * - Trade deal costs reported as expenses
 * - Multiple players have independent breakdowns
 */

#include <sims3000/port/PortSystem.h>
#include <sims3000/port/TradeIncome.h>
#include <sims3000/port/DemandBonus.h>
#include <sims3000/port/TradeAgreementComponent.h>
#include <cassert>
#include <cstdio>
#include <cmath>
#include <vector>

using namespace sims3000::port;

// =============================================================================
// Helpers
// =============================================================================

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
// get_trade_income_breakdown() Tests
// =============================================================================

static void test_breakdown_empty_system() {
    TEST("get_trade_income_breakdown: empty system returns zeros");
    PortSystem sys(100, 100);

    TradeIncomeBreakdown bd = sys.get_trade_income_breakdown(1);
    assert(bd.aero_income == 0);
    assert(bd.aqua_income == 0);
    assert(bd.trade_deal_bonuses == 0);
    assert(bd.total == 0);
    PASS();
}

static void test_breakdown_after_tick() {
    TEST("get_trade_income_breakdown: populated after tick");
    PortSystem sys(100, 100);

    // Add an operational aero port for player 1
    PortData port = {PortType::Aero, 1000, true, 1, 10, 10};
    sys.add_port(port);

    sys.tick(0.05f);

    TradeIncomeBreakdown bd = sys.get_trade_income_breakdown(1);
    // Medium port: 1000 * 0.7 * 0.8 * 1.0 = 560
    assert(bd.aero_income == 560);
    assert(bd.aqua_income == 0);
    assert(bd.total == 560);
    PASS();
}

static void test_breakdown_with_agreements() {
    TEST("get_trade_income_breakdown: includes trade deal bonuses");
    PortSystem sys(100, 100);

    PortData port = {PortType::Aero, 1000, true, 1, 10, 10};
    sys.add_port(port);

    TradeAgreementComponent agree = {};
    agree.party_a = 0;
    agree.party_b = 1;
    agree.agreement_type = TradeAgreementType::Premium;
    agree.income_bonus_percent = 120;
    agree.cycles_remaining = 300;
    sys.add_trade_agreement(agree);

    sys.tick(0.05f);

    TradeIncomeBreakdown bd = sys.get_trade_income_breakdown(1);
    // Base: 560, with 1.2x: 672, bonus: 112
    assert(bd.aero_income == 672);
    assert(bd.trade_deal_bonuses > 0);
    assert(bd.total == 672);
    PASS();
}

static void test_breakdown_invalid_owner() {
    TEST("get_trade_income_breakdown: invalid owner returns zeros");
    PortSystem sys(100, 100);
    TradeIncomeBreakdown bd = sys.get_trade_income_breakdown(255);
    assert(bd.total == 0);
    PASS();
}

// =============================================================================
// Trade Agreement Management Tests
// =============================================================================

static void test_add_trade_agreement() {
    TEST("add_trade_agreement: agreement is stored");
    PortSystem sys(100, 100);

    TradeAgreementComponent agree = {};
    agree.party_a = 0;
    agree.party_b = 1;
    agree.agreement_type = TradeAgreementType::Basic;
    agree.income_bonus_percent = 80;
    agree.cycles_remaining = 100;

    sys.add_trade_agreement(agree);
    assert(sys.get_trade_agreements().size() == 1);
    assert(sys.get_trade_agreements()[0].party_b == 1);
    PASS();
}

static void test_clear_trade_agreements() {
    TEST("clear_trade_agreements: removes all agreements");
    PortSystem sys(100, 100);

    TradeAgreementComponent agree = {};
    agree.party_b = 1;
    sys.add_trade_agreement(agree);
    sys.add_trade_agreement(agree);

    assert(sys.get_trade_agreements().size() == 2);
    sys.clear_trade_agreements();
    assert(sys.get_trade_agreements().size() == 0);
    PASS();
}

// =============================================================================
// IPortProvider::get_trade_income() Integration Tests
// =============================================================================

static void test_get_trade_income_updated_by_tick() {
    TEST("get_trade_income: updated by tick phase");
    PortSystem sys(100, 100);

    // Initially zero
    assert(sys.get_trade_income(1) == 0);

    // Add port and tick
    PortData port = {PortType::Aqua, 2000, true, 1, 5, 5};
    sys.add_port(port);
    sys.tick(0.05f);

    // Large aqua: 2000 * 0.9 * 0.6 * 1.0 = 1080
    assert(sys.get_trade_income(1) == 1080);
    PASS();
}

static void test_trade_income_consistency() {
    TEST("get_trade_income matches breakdown total");
    PortSystem sys(100, 100);

    PortData p1 = {PortType::Aero, 1000, true, 1, 10, 10};
    PortData p2 = {PortType::Aqua, 2000, true, 1, 20, 20};
    sys.add_port(p1);
    sys.add_port(p2);
    sys.tick(0.05f);

    int64_t income = sys.get_trade_income(1);
    TradeIncomeBreakdown bd = sys.get_trade_income_breakdown(1);
    assert(income == bd.total);
    PASS();
}

// =============================================================================
// Multi-Player Tests
// =============================================================================

static void test_multi_player_breakdowns() {
    TEST("get_trade_income_breakdown: independent per player");
    PortSystem sys(100, 100);

    PortData p1 = {PortType::Aero, 1000, true, 1, 10, 10};
    PortData p2 = {PortType::Aqua, 2000, true, 2, 20, 20};
    sys.add_port(p1);
    sys.add_port(p2);

    sys.tick(0.05f);

    TradeIncomeBreakdown bd1 = sys.get_trade_income_breakdown(1);
    TradeIncomeBreakdown bd2 = sys.get_trade_income_breakdown(2);

    assert(bd1.aero_income == 560);
    assert(bd1.aqua_income == 0);
    assert(bd2.aero_income == 0);
    assert(bd2.aqua_income == 1080);
    PASS();
}

static void test_trade_deal_costs_as_expenses() {
    TEST("trade deal costs visible in breakdown");
    PortSystem sys(100, 100);

    PortData port = {PortType::Aero, 1000, true, 1, 10, 10};
    sys.add_port(port);

    // Add a trade agreement with reduced income (None tier = 50%)
    TradeAgreementComponent agree = {};
    agree.party_a = 0;
    agree.party_b = 1;
    agree.agreement_type = TradeAgreementType::None;
    agree.income_bonus_percent = 50;
    agree.cycles_remaining = 100;
    sys.add_trade_agreement(agree);

    sys.tick(0.05f);

    TradeIncomeBreakdown bd = sys.get_trade_income_breakdown(1);
    // Base: 560, with 0.5x: 280
    // trade_deal_bonuses should be negative (income reduction)
    assert(bd.trade_deal_bonuses < 0);
    assert(bd.total == 280);
    PASS();
}

static void test_non_operational_port_no_income() {
    TEST("non-operational port contributes zero income after tick");
    PortSystem sys(100, 100);

    PortData port = {PortType::Aero, 1000, false, 1, 10, 10};
    sys.add_port(port);
    sys.tick(0.05f);

    TradeIncomeBreakdown bd = sys.get_trade_income_breakdown(1);
    assert(bd.total == 0);
    PASS();
}

// =============================================================================
// Main
// =============================================================================

int main() {
    printf("=== Port Economy Integration Tests (E8-020) ===\n");

    // Breakdown queries
    test_breakdown_empty_system();
    test_breakdown_after_tick();
    test_breakdown_with_agreements();
    test_breakdown_invalid_owner();

    // Agreement management
    test_add_trade_agreement();
    test_clear_trade_agreements();

    // IPortProvider integration
    test_get_trade_income_updated_by_tick();
    test_trade_income_consistency();

    // Multi-player
    test_multi_player_breakdowns();

    // Expense reporting
    test_trade_deal_costs_as_expenses();
    test_non_operational_port_no_income();

    printf("\n=== Results: %d/%d passed ===\n", tests_passed, tests_total);
    return (tests_passed == tests_total) ? 0 : 1;
}
