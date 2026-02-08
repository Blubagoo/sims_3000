/**
 * @file test_port_integration_epic10_11.cpp
 * @brief Integration tests for DemandSystem (Epic 10) and EconomySystem (Epic 11)
 *        integration points via IPortProvider (Ticket E8-038)
 *
 * Since Epic 10 (DemandSystem) and Epic 11 (EconomySystem) don't exist yet,
 * these tests verify that the IPortProvider interface returns correct values
 * when PortSystem is configured with ports and trade agreements.
 *
 * Tests cover:
 * - IPortProvider::get_global_demand_bonus returns correct values
 * - IPortProvider::get_local_demand_bonus returns correct position-based values
 * - IPortProvider::get_trade_income returns correct income after tick
 * - IPortProvider::get_port_capacity returns aggregated capacity
 * - IPortProvider::has_operational_port correctly reports status
 * - StubPortProvider returns safe defaults (no ports = neutral)
 * - Budget cycle includes port income/expenses via trade agreements
 */

#include <sims3000/port/PortSystem.h>
#include <sims3000/port/DemandBonus.h>
#include <sims3000/port/TradeIncome.h>
#include <sims3000/port/TradeAgreementComponent.h>
#include <sims3000/building/ForwardDependencyInterfaces.h>
#include <sims3000/building/ForwardDependencyStubs.h>
#include <sims3000/port/PortTypes.h>
#include <cassert>
#include <cstdio>
#include <cmath>
#include <vector>

using namespace sims3000::port;
using namespace sims3000::building;

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

static bool approx(float a, float b, float eps = 0.01f) {
    return std::fabs(a - b) < eps;
}

// =============================================================================
// Helper: Create PortSystem with ports configured for testing
// =============================================================================

static PortSystem create_system_with_aero_port(uint8_t owner, uint16_t capacity,
                                                int32_t x, int32_t y) {
    PortSystem sys(100, 100);
    PortData port = {PortType::Aero, capacity, true, owner, x, y};
    sys.add_port(port);
    return sys;
}

static PortSystem create_system_with_aqua_port(uint8_t owner, uint16_t capacity,
                                                int32_t x, int32_t y) {
    PortSystem sys(100, 100);
    PortData port = {PortType::Aqua, capacity, true, owner, x, y};
    sys.add_port(port);
    return sys;
}

// =============================================================================
// E8-038: get_global_demand_bonus integration tests
// =============================================================================

static void test_global_demand_bonus_no_ports() {
    TEST("get_global_demand_bonus: zero with no ports");
    PortSystem sys(100, 100);
    IPortProvider* provider = &sys;

    // zone_type 0 = Habitation, 1 = Exchange, 2 = Fabrication
    assert(approx(provider->get_global_demand_bonus(0, 1), 0.0f));
    assert(approx(provider->get_global_demand_bonus(1, 1), 0.0f));
    assert(approx(provider->get_global_demand_bonus(2, 1), 0.0f));
    PASS();
}

static void test_global_demand_bonus_small_aero_port() {
    TEST("get_global_demand_bonus: small aero port boosts exchange (zone 1)");
    PortSystem sys = create_system_with_aero_port(1, 200, 10, 10);
    sys.tick(0.05f);
    IPortProvider* provider = &sys;

    // Aero ports boost Exchange demand (zone_type 1)
    // Small port (capacity < 500) gives +5.0 bonus
    float bonus = provider->get_global_demand_bonus(1, 1);
    assert(approx(bonus, DEMAND_BONUS_SMALL));
    PASS();
}

static void test_global_demand_bonus_medium_aero_port() {
    TEST("get_global_demand_bonus: medium aero port boosts exchange (zone 1)");
    PortSystem sys = create_system_with_aero_port(1, 1000, 10, 10);
    sys.tick(0.05f);
    IPortProvider* provider = &sys;

    // Medium port (capacity 500-1999) gives +10.0 bonus
    float bonus = provider->get_global_demand_bonus(1, 1);
    assert(approx(bonus, DEMAND_BONUS_MEDIUM));
    PASS();
}

static void test_global_demand_bonus_large_aero_port() {
    TEST("get_global_demand_bonus: large aero port boosts exchange (zone 1)");
    PortSystem sys = create_system_with_aero_port(1, 3000, 10, 10);
    sys.tick(0.05f);
    IPortProvider* provider = &sys;

    // Large port (capacity >= 2000) gives +15.0 bonus
    float bonus = provider->get_global_demand_bonus(1, 1);
    assert(approx(bonus, DEMAND_BONUS_LARGE));
    PASS();
}

static void test_global_demand_bonus_aqua_port_fabrication() {
    TEST("get_global_demand_bonus: aqua port boosts fabrication (zone 2)");
    PortSystem sys = create_system_with_aqua_port(1, 600, 10, 10);
    sys.tick(0.05f);
    IPortProvider* provider = &sys;

    // Aqua ports boost Fabrication demand (zone_type 2)
    // Medium port gives +10.0
    float bonus = provider->get_global_demand_bonus(2, 1);
    assert(approx(bonus, DEMAND_BONUS_MEDIUM));
    PASS();
}

static void test_global_demand_bonus_wrong_zone_type() {
    TEST("get_global_demand_bonus: aero port does not boost fabrication");
    PortSystem sys = create_system_with_aero_port(1, 1000, 10, 10);
    sys.tick(0.05f);
    IPortProvider* provider = &sys;

    // Aero port should not boost zone_type 2 (Fabrication)
    float bonus = provider->get_global_demand_bonus(2, 1);
    assert(approx(bonus, 0.0f));

    // Aero port should not boost zone_type 0 (Habitation) globally
    bonus = provider->get_global_demand_bonus(0, 1);
    assert(approx(bonus, 0.0f));
    PASS();
}

static void test_global_demand_bonus_different_owner() {
    TEST("get_global_demand_bonus: port only contributes to its owner");
    PortSystem sys = create_system_with_aero_port(1, 1000, 10, 10);
    sys.tick(0.05f);
    IPortProvider* provider = &sys;

    // Player 1 owns the port
    float bonus_p1 = provider->get_global_demand_bonus(1, 1);
    assert(approx(bonus_p1, DEMAND_BONUS_MEDIUM));

    // Player 2 does not own the port
    float bonus_p2 = provider->get_global_demand_bonus(1, 2);
    assert(approx(bonus_p2, 0.0f));
    PASS();
}

static void test_global_demand_bonus_capped() {
    TEST("get_global_demand_bonus: capped at MAX_TOTAL_DEMAND_BONUS (30.0)");
    PortSystem sys(100, 100);
    // Add 3 large aero ports = 3 * 15.0 = 45.0, capped at 30.0
    sys.add_port({PortType::Aero, 3000, true, 1, 10, 10});
    sys.add_port({PortType::Aero, 3000, true, 1, 20, 20});
    sys.add_port({PortType::Aero, 3000, true, 1, 30, 30});
    sys.tick(0.05f);
    IPortProvider* provider = &sys;

    float bonus = provider->get_global_demand_bonus(1, 1);
    assert(approx(bonus, MAX_TOTAL_DEMAND_BONUS));
    PASS();
}

static void test_global_demand_bonus_non_operational_ignored() {
    TEST("get_global_demand_bonus: non-operational port ignored");
    PortSystem sys(100, 100);
    // Non-operational port should not contribute
    sys.add_port({PortType::Aero, 1000, false, 1, 10, 10});
    sys.tick(0.05f);
    IPortProvider* provider = &sys;

    float bonus = provider->get_global_demand_bonus(1, 1);
    assert(approx(bonus, 0.0f));
    PASS();
}

// =============================================================================
// E8-038: get_local_demand_bonus integration tests
// =============================================================================

static void test_local_demand_bonus_no_ports() {
    TEST("get_local_demand_bonus: zero with no ports");
    PortSystem sys(100, 100);
    IPortProvider* provider = &sys;

    float bonus = provider->get_local_demand_bonus(0, 50, 50, 1);
    assert(approx(bonus, 0.0f));
    PASS();
}

static void test_local_demand_bonus_aero_within_radius() {
    TEST("get_local_demand_bonus: aero port within radius boosts habitation");
    PortSystem sys(100, 100);
    // Aero port at (50, 50), checking at (55, 55) = Manhattan distance 10 < 20
    sys.add_port({PortType::Aero, 1000, true, 1, 50, 50});
    sys.tick(0.05f);
    IPortProvider* provider = &sys;

    // zone_type 0 = Habitation, Aero ports boost Habitation within 20 tiles
    float bonus = provider->get_local_demand_bonus(0, 55, 55, 1);
    assert(approx(bonus, LOCAL_BONUS_AERO_HABITATION));
    PASS();
}

static void test_local_demand_bonus_aero_outside_radius() {
    TEST("get_local_demand_bonus: aero port outside radius gives no bonus");
    PortSystem sys(100, 100);
    // Aero port at (10, 10), checking at (50, 50) = Manhattan distance 80 > 20
    sys.add_port({PortType::Aero, 1000, true, 1, 10, 10});
    sys.tick(0.05f);
    IPortProvider* provider = &sys;

    float bonus = provider->get_local_demand_bonus(0, 50, 50, 1);
    assert(approx(bonus, 0.0f));
    PASS();
}

static void test_local_demand_bonus_aqua_within_radius() {
    TEST("get_local_demand_bonus: aqua port within radius boosts exchange");
    PortSystem sys(100, 100);
    // Aqua port at (50, 50), checking at (60, 60) = Manhattan distance 20 < 25
    sys.add_port({PortType::Aqua, 1000, true, 1, 50, 50});
    sys.tick(0.05f);
    IPortProvider* provider = &sys;

    // zone_type 1 = Exchange, Aqua ports boost Exchange within 25 tiles
    float bonus = provider->get_local_demand_bonus(1, 60, 60, 1);
    assert(approx(bonus, LOCAL_BONUS_AQUA_EXCHANGE));
    PASS();
}

static void test_local_demand_bonus_position_sensitivity() {
    TEST("get_local_demand_bonus: different positions give different results");
    PortSystem sys(100, 100);
    sys.add_port({PortType::Aero, 1000, true, 1, 50, 50});
    sys.tick(0.05f);
    IPortProvider* provider = &sys;

    // Close position (within radius)
    float close = provider->get_local_demand_bonus(0, 55, 55, 1);
    // Far position (outside radius)
    float far = provider->get_local_demand_bonus(0, 90, 90, 1);

    assert(close > 0.0f);
    assert(approx(far, 0.0f));
    PASS();
}

static void test_local_demand_bonus_stacking() {
    TEST("get_local_demand_bonus: multiple nearby ports stack");
    PortSystem sys(100, 100);
    sys.add_port({PortType::Aero, 1000, true, 1, 50, 50});
    sys.add_port({PortType::Aero, 1000, true, 1, 55, 50});
    sys.tick(0.05f);
    IPortProvider* provider = &sys;

    // Both ports are within 20 tiles of (52, 50)
    float bonus = provider->get_local_demand_bonus(0, 52, 50, 1);
    // Should be 2 * LOCAL_BONUS_AERO_HABITATION
    assert(approx(bonus, LOCAL_BONUS_AERO_HABITATION * 2.0f));
    PASS();
}

// =============================================================================
// E8-038: get_trade_income integration tests
// =============================================================================

static void test_trade_income_no_ports() {
    TEST("get_trade_income: zero with no ports");
    PortSystem sys(100, 100);
    sys.tick(0.05f);
    IPortProvider* provider = &sys;

    assert(provider->get_trade_income(1) == 0);
    PASS();
}

static void test_trade_income_with_aero_port() {
    TEST("get_trade_income: correct income with aero port after tick");
    PortSystem sys(100, 100);
    // Medium aero port (capacity 1000, utilization ~0.7)
    sys.add_port({PortType::Aero, 1000, true, 1, 10, 10});
    sys.tick(0.05f);
    IPortProvider* provider = &sys;

    // Expected: 1000 * 0.7 * 0.8 * 1.0 = 560
    int64_t income = provider->get_trade_income(1);
    assert(income == 560);
    PASS();
}

static void test_trade_income_with_aqua_port() {
    TEST("get_trade_income: correct income with aqua port after tick");
    PortSystem sys(100, 100);
    // Medium aqua port (capacity 1000, utilization ~0.7)
    sys.add_port({PortType::Aqua, 1000, true, 1, 10, 10});
    sys.tick(0.05f);
    IPortProvider* provider = &sys;

    // Expected: 1000 * 0.7 * 0.6 * 1.0 = 420
    int64_t income = provider->get_trade_income(1);
    assert(income == 420);
    PASS();
}

static void test_trade_income_multiple_ports() {
    TEST("get_trade_income: aggregates income from multiple ports");
    PortSystem sys(100, 100);
    sys.add_port({PortType::Aero, 1000, true, 1, 10, 10});
    sys.add_port({PortType::Aqua, 1000, true, 1, 20, 20});
    sys.tick(0.05f);
    IPortProvider* provider = &sys;

    // Aero: 1000 * 0.7 * 0.8 = 560, Aqua: 1000 * 0.7 * 0.6 = 420
    // Total = 980
    int64_t income = provider->get_trade_income(1);
    assert(income == 980);
    PASS();
}

static void test_trade_income_with_trade_agreement() {
    TEST("get_trade_income: modified by trade agreement multiplier");
    PortSystem sys(100, 100);
    sys.add_port({PortType::Aero, 1000, true, 1, 10, 10});

    // Add a Premium NPC trade agreement for player 1
    TradeAgreementComponent agreement;
    agreement.party_a = 0; // GAME_MASTER/NPC
    agreement.party_b = 1;
    agreement.agreement_type = TradeAgreementType::Premium;
    agreement.cycles_remaining = 100;
    agreement.income_bonus_percent = 120; // 1.2x
    sys.add_trade_agreement(agreement);

    sys.tick(0.05f);
    IPortProvider* provider = &sys;

    // Base income: 1000 * 0.7 * 0.8 = 560
    // With Premium multiplier (1.2x): 560 * 1.2 = 672
    int64_t income = provider->get_trade_income(1);
    assert(income == 672);
    PASS();
}

static void test_trade_income_non_operational_port() {
    TEST("get_trade_income: non-operational port generates no income");
    PortSystem sys(100, 100);
    sys.add_port({PortType::Aero, 1000, false, 1, 10, 10});
    sys.tick(0.05f);
    IPortProvider* provider = &sys;

    assert(provider->get_trade_income(1) == 0);
    PASS();
}

static void test_trade_income_per_player_isolation() {
    TEST("get_trade_income: each player has independent income");
    PortSystem sys(100, 100);
    sys.add_port({PortType::Aero, 1000, true, 1, 10, 10});
    sys.add_port({PortType::Aqua, 500, true, 2, 20, 20});
    sys.tick(0.05f);
    IPortProvider* provider = &sys;

    int64_t income_p1 = provider->get_trade_income(1);
    int64_t income_p2 = provider->get_trade_income(2);

    // Player 1: medium aero = 1000 * 0.7 * 0.8 = 560
    assert(income_p1 == 560);
    // Player 2: medium aqua = 500 * 0.7 * 0.6 = 210
    assert(income_p2 == 210);
    PASS();
}

// =============================================================================
// E8-038: get_port_capacity integration tests
// =============================================================================

static void test_port_capacity_no_ports() {
    TEST("get_port_capacity: zero with no ports");
    PortSystem sys(100, 100);
    IPortProvider* provider = &sys;

    uint8_t aero = static_cast<uint8_t>(PortType::Aero);
    uint8_t aqua = static_cast<uint8_t>(PortType::Aqua);
    assert(provider->get_port_capacity(aero, 1) == 0);
    assert(provider->get_port_capacity(aqua, 1) == 0);
    PASS();
}

static void test_port_capacity_single_port() {
    TEST("get_port_capacity: returns capacity of single port");
    PortSystem sys(100, 100);
    sys.add_port({PortType::Aero, 1000, true, 1, 10, 10});
    sys.tick(0.05f);
    IPortProvider* provider = &sys;

    uint8_t aero = static_cast<uint8_t>(PortType::Aero);
    assert(provider->get_port_capacity(aero, 1) == 1000);
    PASS();
}

static void test_port_capacity_aggregated() {
    TEST("get_port_capacity: aggregates capacity from multiple ports of same type");
    PortSystem sys(100, 100);
    sys.add_port({PortType::Aero, 500, true, 1, 10, 10});
    sys.add_port({PortType::Aero, 800, true, 1, 20, 20});
    sys.tick(0.05f);
    IPortProvider* provider = &sys;

    uint8_t aero = static_cast<uint8_t>(PortType::Aero);
    assert(provider->get_port_capacity(aero, 1) == 1300);
    PASS();
}

static void test_port_capacity_type_separation() {
    TEST("get_port_capacity: separates by port type");
    PortSystem sys(100, 100);
    sys.add_port({PortType::Aero, 500, true, 1, 10, 10});
    sys.add_port({PortType::Aqua, 800, true, 1, 20, 20});
    sys.tick(0.05f);
    IPortProvider* provider = &sys;

    uint8_t aero = static_cast<uint8_t>(PortType::Aero);
    uint8_t aqua = static_cast<uint8_t>(PortType::Aqua);
    assert(provider->get_port_capacity(aero, 1) == 500);
    assert(provider->get_port_capacity(aqua, 1) == 800);
    PASS();
}

static void test_port_capacity_owner_isolation() {
    TEST("get_port_capacity: separates by owner");
    PortSystem sys(100, 100);
    sys.add_port({PortType::Aero, 500, true, 1, 10, 10});
    sys.add_port({PortType::Aero, 800, true, 2, 20, 20});
    sys.tick(0.05f);
    IPortProvider* provider = &sys;

    uint8_t aero = static_cast<uint8_t>(PortType::Aero);
    assert(provider->get_port_capacity(aero, 1) == 500);
    assert(provider->get_port_capacity(aero, 2) == 800);
    PASS();
}

// =============================================================================
// E8-038: has_operational_port integration tests
// =============================================================================

static void test_has_operational_port_none() {
    TEST("has_operational_port: false with no ports");
    PortSystem sys(100, 100);
    IPortProvider* provider = &sys;

    uint8_t aero = static_cast<uint8_t>(PortType::Aero);
    assert(!provider->has_operational_port(aero, 1));
    PASS();
}

static void test_has_operational_port_operational() {
    TEST("has_operational_port: true with operational port");
    PortSystem sys(100, 100);
    sys.add_port({PortType::Aero, 1000, true, 1, 10, 10});
    sys.tick(0.05f);
    IPortProvider* provider = &sys;

    uint8_t aero = static_cast<uint8_t>(PortType::Aero);
    assert(provider->has_operational_port(aero, 1));
    PASS();
}

static void test_has_operational_port_non_operational() {
    TEST("has_operational_port: false with non-operational port");
    PortSystem sys(100, 100);
    sys.add_port({PortType::Aero, 1000, false, 1, 10, 10});
    sys.tick(0.05f);
    IPortProvider* provider = &sys;

    uint8_t aero = static_cast<uint8_t>(PortType::Aero);
    assert(!provider->has_operational_port(aero, 1));
    PASS();
}

static void test_has_operational_port_wrong_type() {
    TEST("has_operational_port: false for different port type");
    PortSystem sys(100, 100);
    sys.add_port({PortType::Aero, 1000, true, 1, 10, 10});
    sys.tick(0.05f);
    IPortProvider* provider = &sys;

    uint8_t aqua = static_cast<uint8_t>(PortType::Aqua);
    assert(!provider->has_operational_port(aqua, 1));
    PASS();
}

static void test_has_operational_port_wrong_owner() {
    TEST("has_operational_port: false for different owner");
    PortSystem sys(100, 100);
    sys.add_port({PortType::Aero, 1000, true, 1, 10, 10});
    sys.tick(0.05f);
    IPortProvider* provider = &sys;

    uint8_t aero = static_cast<uint8_t>(PortType::Aero);
    assert(!provider->has_operational_port(aero, 2));
    PASS();
}

// =============================================================================
// E8-038: StubPortProvider tests (safe defaults for DemandSystem/EconomySystem)
// =============================================================================

static void test_stub_global_demand_bonus() {
    TEST("StubPortProvider::get_global_demand_bonus returns 0.0 (neutral)");
    StubPortProvider stub;
    IPortProvider* provider = &stub;

    assert(approx(provider->get_global_demand_bonus(0, 1), 0.0f));
    assert(approx(provider->get_global_demand_bonus(1, 1), 0.0f));
    assert(approx(provider->get_global_demand_bonus(2, 1), 0.0f));
    PASS();
}

static void test_stub_local_demand_bonus() {
    TEST("StubPortProvider::get_local_demand_bonus returns 0.0 (neutral)");
    StubPortProvider stub;
    IPortProvider* provider = &stub;

    assert(approx(provider->get_local_demand_bonus(0, 50, 50, 1), 0.0f));
    assert(approx(provider->get_local_demand_bonus(1, 10, 20, 2), 0.0f));
    PASS();
}

static void test_stub_trade_income() {
    TEST("StubPortProvider::get_trade_income returns 0 (no income)");
    StubPortProvider stub;
    IPortProvider* provider = &stub;

    assert(provider->get_trade_income(1) == 0);
    assert(provider->get_trade_income(2) == 0);
    assert(provider->get_trade_income(0) == 0);
    PASS();
}

static void test_stub_port_capacity() {
    TEST("StubPortProvider::get_port_capacity returns 0 (no ports)");
    StubPortProvider stub;
    IPortProvider* provider = &stub;

    uint8_t aero = static_cast<uint8_t>(PortType::Aero);
    uint8_t aqua = static_cast<uint8_t>(PortType::Aqua);
    assert(provider->get_port_capacity(aero, 1) == 0);
    assert(provider->get_port_capacity(aqua, 1) == 0);
    PASS();
}

static void test_stub_has_operational_port() {
    TEST("StubPortProvider::has_operational_port returns false (no ports)");
    StubPortProvider stub;
    IPortProvider* provider = &stub;

    uint8_t aero = static_cast<uint8_t>(PortType::Aero);
    uint8_t aqua = static_cast<uint8_t>(PortType::Aqua);
    assert(!provider->has_operational_port(aero, 1));
    assert(!provider->has_operational_port(aqua, 1));
    PASS();
}

static void test_stub_port_utilization() {
    TEST("StubPortProvider::get_port_utilization returns 0.0 (idle)");
    StubPortProvider stub;
    IPortProvider* provider = &stub;

    uint8_t aero = static_cast<uint8_t>(PortType::Aero);
    assert(approx(provider->get_port_utilization(aero, 1), 0.0f));
    PASS();
}

static void test_stub_port_count() {
    TEST("StubPortProvider::get_port_count returns 0");
    StubPortProvider stub;
    IPortProvider* provider = &stub;

    uint8_t aero = static_cast<uint8_t>(PortType::Aero);
    assert(provider->get_port_count(aero, 1) == 0);
    PASS();
}

static void test_stub_external_connections() {
    TEST("StubPortProvider::get_external_connection_count returns 0");
    StubPortProvider stub;
    IPortProvider* provider = &stub;

    assert(provider->get_external_connection_count(1) == 0);
    assert(!provider->is_connected_to_edge(0, 1));
    PASS();
}

static void test_stub_restrictive_mode_same_defaults() {
    TEST("StubPortProvider: restrictive mode returns same defaults (ports are opt-in)");
    StubPortProvider stub;
    stub.set_debug_restrictive(true);
    IPortProvider* provider = &stub;

    // Since ports are opt-in infrastructure, defaults are already restrictive
    assert(provider->get_port_capacity(0, 1) == 0);
    assert(!provider->has_operational_port(0, 1));
    assert(provider->get_trade_income(1) == 0);
    assert(approx(provider->get_global_demand_bonus(1, 1), 0.0f));
    assert(approx(provider->get_local_demand_bonus(0, 50, 50, 1), 0.0f));
    PASS();
}

// =============================================================================
// E8-038: Budget cycle integration (port income/expenses via trade agreements)
// =============================================================================

static void test_budget_trade_income_breakdown() {
    TEST("Budget: get_trade_income_breakdown includes port income");
    PortSystem sys(100, 100);
    sys.add_port({PortType::Aero, 1000, true, 1, 10, 10});
    sys.tick(0.05f);

    TradeIncomeBreakdown bd = sys.get_trade_income_breakdown(1);
    // Medium aero: 1000 * 0.7 * 0.8 * 1.0 = 560
    assert(bd.aero_income == 560);
    assert(bd.aqua_income == 0);
    assert(bd.total == 560);
    PASS();
}

static void test_budget_mixed_port_income() {
    TEST("Budget: income breakdown separates aero and aqua income");
    PortSystem sys(100, 100);
    sys.add_port({PortType::Aero, 1000, true, 1, 10, 10});
    sys.add_port({PortType::Aqua, 1000, true, 1, 20, 20});
    sys.tick(0.05f);

    TradeIncomeBreakdown bd = sys.get_trade_income_breakdown(1);
    assert(bd.aero_income == 560);  // 1000 * 0.7 * 0.8
    assert(bd.aqua_income == 420);  // 1000 * 0.7 * 0.6
    assert(bd.total == 980);
    PASS();
}

static void test_budget_trade_deal_bonuses() {
    TEST("Budget: income breakdown shows trade deal bonuses");
    PortSystem sys(100, 100);
    sys.add_port({PortType::Aero, 1000, true, 1, 10, 10});

    // Add a Premium agreement with 1.2x multiplier
    TradeAgreementComponent agreement;
    agreement.party_a = 0;
    agreement.party_b = 1;
    agreement.agreement_type = TradeAgreementType::Premium;
    agreement.cycles_remaining = 100;
    agreement.income_bonus_percent = 120;
    sys.add_trade_agreement(agreement);

    sys.tick(0.05f);

    TradeIncomeBreakdown bd = sys.get_trade_income_breakdown(1);
    // Base aero income: 560 (1000 * 0.7 * 0.8)
    // With 1.2x: 560 * 1.2 = 672
    // Trade deal bonus = 672 - 560 = 112
    assert(bd.total == 672);
    assert(bd.trade_deal_bonuses == 112);
    PASS();
}

static void test_budget_no_income_for_wrong_player() {
    TEST("Budget: income breakdown is zero for player without ports");
    PortSystem sys(100, 100);
    sys.add_port({PortType::Aero, 1000, true, 1, 10, 10});
    sys.tick(0.05f);

    TradeIncomeBreakdown bd = sys.get_trade_income_breakdown(2);
    assert(bd.aero_income == 0);
    assert(bd.aqua_income == 0);
    assert(bd.trade_deal_bonuses == 0);
    assert(bd.total == 0);
    PASS();
}

// =============================================================================
// E8-038: IPortProvider polymorphism through PortSystem
// =============================================================================

static void test_polymorphic_access_full() {
    TEST("IPortProvider polymorphism: full query suite through pointer");
    PortSystem sys(100, 100);
    sys.add_port({PortType::Aero, 1000, true, 1, 50, 50});
    sys.add_port({PortType::Aqua, 800, true, 1, 50, 55});
    sys.tick(0.05f);

    IPortProvider* provider = &sys;

    // Capacity
    uint8_t aero = static_cast<uint8_t>(PortType::Aero);
    uint8_t aqua = static_cast<uint8_t>(PortType::Aqua);
    assert(provider->get_port_capacity(aero, 1) == 1000);
    assert(provider->get_port_capacity(aqua, 1) == 800);

    // Operational
    assert(provider->has_operational_port(aero, 1));
    assert(provider->has_operational_port(aqua, 1));

    // Global demand
    float aero_bonus = provider->get_global_demand_bonus(1, 1); // Exchange from Aero
    assert(aero_bonus > 0.0f);
    float aqua_bonus = provider->get_global_demand_bonus(2, 1); // Fabrication from Aqua
    assert(aqua_bonus > 0.0f);

    // Local demand
    float local = provider->get_local_demand_bonus(0, 55, 55, 1);
    assert(local > 0.0f); // Aero port within 20 tiles boosts Habitation

    // Trade income
    int64_t income = provider->get_trade_income(1);
    assert(income > 0);

    PASS();
}

// =============================================================================
// Main
// =============================================================================

int main() {
    printf("=== Port Integration Tests - Epic 10/11 (Ticket E8-038) ===\n");

    // Global demand bonus tests
    test_global_demand_bonus_no_ports();
    test_global_demand_bonus_small_aero_port();
    test_global_demand_bonus_medium_aero_port();
    test_global_demand_bonus_large_aero_port();
    test_global_demand_bonus_aqua_port_fabrication();
    test_global_demand_bonus_wrong_zone_type();
    test_global_demand_bonus_different_owner();
    test_global_demand_bonus_capped();
    test_global_demand_bonus_non_operational_ignored();

    // Local demand bonus tests
    test_local_demand_bonus_no_ports();
    test_local_demand_bonus_aero_within_radius();
    test_local_demand_bonus_aero_outside_radius();
    test_local_demand_bonus_aqua_within_radius();
    test_local_demand_bonus_position_sensitivity();
    test_local_demand_bonus_stacking();

    // Trade income tests
    test_trade_income_no_ports();
    test_trade_income_with_aero_port();
    test_trade_income_with_aqua_port();
    test_trade_income_multiple_ports();
    test_trade_income_with_trade_agreement();
    test_trade_income_non_operational_port();
    test_trade_income_per_player_isolation();

    // Port capacity tests
    test_port_capacity_no_ports();
    test_port_capacity_single_port();
    test_port_capacity_aggregated();
    test_port_capacity_type_separation();
    test_port_capacity_owner_isolation();

    // has_operational_port tests
    test_has_operational_port_none();
    test_has_operational_port_operational();
    test_has_operational_port_non_operational();
    test_has_operational_port_wrong_type();
    test_has_operational_port_wrong_owner();

    // StubPortProvider tests
    test_stub_global_demand_bonus();
    test_stub_local_demand_bonus();
    test_stub_trade_income();
    test_stub_port_capacity();
    test_stub_has_operational_port();
    test_stub_port_utilization();
    test_stub_port_count();
    test_stub_external_connections();
    test_stub_restrictive_mode_same_defaults();

    // Budget cycle integration tests
    test_budget_trade_income_breakdown();
    test_budget_mixed_port_income();
    test_budget_trade_deal_bonuses();
    test_budget_no_income_for_wrong_player();

    // Polymorphism test
    test_polymorphic_access_full();

    printf("\n=== Results: %d/%d passed ===\n", tests_passed, tests_total);
    return (tests_passed == tests_total) ? 0 : 1;
}
