/**
 * @file test_port_income_ui.cpp
 * @brief Unit tests for trade income breakdown UI data (Epic 8, Ticket E8-021)
 *
 * Tests cover:
 * - Per-port income details available
 * - Per-trade-deal income available
 * - Historical income tracking (last 12 phases)
 * - PortIncomeDetail struct correctness
 * - TradeIncomeUIData aggregate data
 * - Empty system returns sensible defaults
 */

#include <sims3000/port/PortSystem.h>
#include <sims3000/port/PortIncomeUI.h>
#include <sims3000/port/TradeIncome.h>
#include <sims3000/port/DemandBonus.h>
#include <sims3000/port/TradeAgreementComponent.h>
#include <cassert>
#include <cstdio>
#include <cmath>

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
// PortIncomeDetail Tests
// =============================================================================

static void test_port_income_detail_defaults() {
    TEST("PortIncomeDetail: default values");
    PortIncomeDetail detail;
    assert(detail.entity_id == 0);
    assert(detail.port_type == PortType::Aero);
    assert(detail.income == 0);
    assert(detail.capacity == 0);
    assert(detail.utilization == 0);
    PASS();
}

// =============================================================================
// TradeIncomeUIData Tests
// =============================================================================

static void test_ui_data_empty_system() {
    TEST("get_trade_income_ui_data: empty system");
    PortSystem sys(100, 100);
    sys.tick(0.05f);

    TradeIncomeUIData ui_data = sys.get_trade_income_ui_data(1);
    assert(ui_data.port_details.empty());
    assert(ui_data.breakdown.total == 0);
    PASS();
}

static void test_ui_data_per_port_details() {
    TEST("get_trade_income_ui_data: per-port income details");
    PortSystem sys(100, 100);

    PortData p1 = {PortType::Aero, 1000, true, 1, 10, 10};
    PortData p2 = {PortType::Aqua, 2000, true, 1, 20, 20};
    sys.add_port(p1);
    sys.add_port(p2);
    sys.tick(0.05f);

    TradeIncomeUIData ui_data = sys.get_trade_income_ui_data(1);
    assert(ui_data.port_details.size() == 2);

    // First port: aero, medium
    assert(ui_data.port_details[0].port_type == PortType::Aero);
    assert(ui_data.port_details[0].capacity == 1000);
    assert(ui_data.port_details[0].utilization == 70);  // 0.7 * 100
    assert(ui_data.port_details[0].income == 560);      // 1000 * 0.7 * 0.8

    // Second port: aqua, large
    assert(ui_data.port_details[1].port_type == PortType::Aqua);
    assert(ui_data.port_details[1].capacity == 2000);
    assert(ui_data.port_details[1].utilization == 90);  // 0.9 * 100
    assert(ui_data.port_details[1].income == 1080);     // 2000 * 0.9 * 0.6
    PASS();
}

static void test_ui_data_non_operational_port() {
    TEST("get_trade_income_ui_data: non-operational port shows zero");
    PortSystem sys(100, 100);

    PortData port = {PortType::Aero, 1000, false, 1, 10, 10};
    sys.add_port(port);
    sys.tick(0.05f);

    TradeIncomeUIData ui_data = sys.get_trade_income_ui_data(1);
    assert(ui_data.port_details.size() == 1);
    assert(ui_data.port_details[0].income == 0);
    assert(ui_data.port_details[0].utilization == 0);
    PASS();
}

static void test_ui_data_breakdown_matches() {
    TEST("get_trade_income_ui_data: breakdown matches get_trade_income_breakdown");
    PortSystem sys(100, 100);

    PortData port = {PortType::Aero, 1000, true, 1, 10, 10};
    sys.add_port(port);
    sys.tick(0.05f);

    TradeIncomeUIData ui_data = sys.get_trade_income_ui_data(1);
    TradeIncomeBreakdown bd = sys.get_trade_income_breakdown(1);

    assert(ui_data.breakdown.aero_income == bd.aero_income);
    assert(ui_data.breakdown.aqua_income == bd.aqua_income);
    assert(ui_data.breakdown.trade_deal_bonuses == bd.trade_deal_bonuses);
    assert(ui_data.breakdown.total == bd.total);
    PASS();
}

static void test_ui_data_with_trade_deal() {
    TEST("get_trade_income_ui_data: per-port income reflects trade deals");
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

    TradeIncomeUIData ui_data = sys.get_trade_income_ui_data(1);
    assert(ui_data.port_details.size() == 1);
    // Base: 560, with 1.2x: 672
    assert(ui_data.port_details[0].income == 672);
    PASS();
}

static void test_ui_data_filters_by_owner() {
    TEST("get_trade_income_ui_data: filters ports by owner");
    PortSystem sys(100, 100);

    PortData p1 = {PortType::Aero, 1000, true, 1, 10, 10};
    PortData p2 = {PortType::Aqua, 2000, true, 2, 20, 20};
    sys.add_port(p1);
    sys.add_port(p2);
    sys.tick(0.05f);

    TradeIncomeUIData ui_p1 = sys.get_trade_income_ui_data(1);
    TradeIncomeUIData ui_p2 = sys.get_trade_income_ui_data(2);

    assert(ui_p1.port_details.size() == 1);
    assert(ui_p1.port_details[0].port_type == PortType::Aero);

    assert(ui_p2.port_details.size() == 1);
    assert(ui_p2.port_details[0].port_type == PortType::Aqua);
    PASS();
}

// =============================================================================
// Income History Tests
// =============================================================================

static void test_income_history_initial() {
    TEST("income_history: all zeros initially");
    PortSystem sys(100, 100);

    TradeIncomeUIData ui_data = sys.get_trade_income_ui_data(1);
    for (std::size_t i = 0; i < INCOME_HISTORY_SIZE; ++i) {
        assert(ui_data.income_history[i] == 0);
    }
    PASS();
}

static void test_income_history_single_tick() {
    TEST("income_history: records income after one tick");
    PortSystem sys(100, 100);

    PortData port = {PortType::Aero, 1000, true, 1, 10, 10};
    sys.add_port(port);
    sys.tick(0.05f);

    TradeIncomeUIData ui_data = sys.get_trade_income_ui_data(1);

    // After 1 tick, the most recent entry (last in chronological order) should have 560
    // The history is reordered from circular buffer to chronological
    // Index 0 is oldest, index 11 is newest after 1 tick
    bool found_income = false;
    for (std::size_t i = 0; i < INCOME_HISTORY_SIZE; ++i) {
        if (ui_data.income_history[i] == 560) {
            found_income = true;
        }
    }
    assert(found_income);
    PASS();
}

static void test_income_history_multiple_ticks() {
    TEST("income_history: tracks multiple phases");
    PortSystem sys(100, 100);

    PortData port = {PortType::Aero, 1000, true, 1, 10, 10};
    sys.add_port(port);

    // Tick 3 times
    sys.tick(0.05f);
    sys.tick(0.05f);
    sys.tick(0.05f);

    TradeIncomeUIData ui_data = sys.get_trade_income_ui_data(1);

    // Should have income=560 in at least 3 entries
    int count_560 = 0;
    for (std::size_t i = 0; i < INCOME_HISTORY_SIZE; ++i) {
        if (ui_data.income_history[i] == 560) {
            count_560++;
        }
    }
    assert(count_560 == 3);
    PASS();
}

static void test_income_history_circular_wrapping() {
    TEST("income_history: wraps around after 12 phases");
    PortSystem sys(100, 100);

    PortData port = {PortType::Aero, 1000, true, 1, 10, 10};
    sys.add_port(port);

    // Tick 15 times (wraps around the 12-entry buffer)
    for (int i = 0; i < 15; ++i) {
        sys.tick(0.05f);
    }

    TradeIncomeUIData ui_data = sys.get_trade_income_ui_data(1);

    // All 12 entries should have income=560 (since port config unchanged)
    for (std::size_t i = 0; i < INCOME_HISTORY_SIZE; ++i) {
        assert(ui_data.income_history[i] == 560);
    }
    PASS();
}

static void test_income_history_changing_income() {
    TEST("income_history: tracks changing income values");
    PortSystem sys(100, 100);

    // Start with small port: 200 * 0.5 * 0.8 = 80
    PortData port = {PortType::Aero, 200, true, 1, 10, 10};
    sys.add_port(port);
    sys.tick(0.05f);

    // Upgrade to medium port: 1000 * 0.7 * 0.8 = 560
    sys.clear_ports();
    port.capacity = 1000;
    sys.add_port(port);
    sys.tick(0.05f);

    TradeIncomeUIData ui_data = sys.get_trade_income_ui_data(1);

    // Should have both 80 and 560 in history
    bool found_80 = false;
    bool found_560 = false;
    for (std::size_t i = 0; i < INCOME_HISTORY_SIZE; ++i) {
        if (ui_data.income_history[i] == 80) found_80 = true;
        if (ui_data.income_history[i] == 560) found_560 = true;
    }
    assert(found_80);
    assert(found_560);
    PASS();
}

// =============================================================================
// Main
// =============================================================================

int main() {
    printf("=== Port Income UI Tests (E8-021) ===\n");

    // Struct defaults
    test_port_income_detail_defaults();

    // UI data queries
    test_ui_data_empty_system();
    test_ui_data_per_port_details();
    test_ui_data_non_operational_port();
    test_ui_data_breakdown_matches();
    test_ui_data_with_trade_deal();
    test_ui_data_filters_by_owner();

    // Income history
    test_income_history_initial();
    test_income_history_single_tick();
    test_income_history_multiple_ticks();
    test_income_history_circular_wrapping();
    test_income_history_changing_income();

    printf("\n=== Results: %d/%d passed ===\n", tests_passed, tests_total);
    return (tests_passed == tests_total) ? 0 : 1;
}
