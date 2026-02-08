/**
 * @file test_port_demand_integration.cpp
 * @brief Unit tests for PortSystem DemandSystem integration (Epic 8, Ticket E8-018)
 *
 * Tests cover:
 * - PortSystem IPortProvider methods return real demand bonus values
 * - Port data management (add, remove, clear)
 * - Global demand bonus via IPortProvider interface
 * - Local demand bonus via IPortProvider interface
 * - Port state queries (capacity, count, has_operational)
 * - IPortProvider polymorphism works correctly
 * - StubPortProvider returns 0 (fallback behavior)
 * - Trade income caching
 */

#include <sims3000/port/PortSystem.h>
#include <sims3000/port/DemandBonus.h>
#include <sims3000/port/PortTypes.h>
#include <sims3000/building/ForwardDependencyInterfaces.h>
#include <sims3000/building/ForwardDependencyStubs.h>
#include <cassert>
#include <cstdio>
#include <cmath>
#include <vector>

using namespace sims3000::port;
using namespace sims3000::building;

// =============================================================================
// Helper
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
// Port Data Management Tests
// =============================================================================

static void test_add_port() {
    TEST("add_port stores port data");
    PortSystem sys(64, 64);
    assert(sys.get_ports().empty());

    PortData port = {PortType::Aero, 1000, true, 1, 10, 20};
    sys.add_port(port);

    assert(sys.get_ports().size() == 1);
    assert(sys.get_ports()[0].port_type == PortType::Aero);
    assert(sys.get_ports()[0].capacity == 1000);
    assert(sys.get_ports()[0].owner == 1);
    assert(sys.get_ports()[0].x == 10);
    assert(sys.get_ports()[0].y == 20);
    PASS();
}

static void test_add_multiple_ports() {
    TEST("add_port stores multiple ports");
    PortSystem sys(64, 64);

    sys.add_port({PortType::Aero, 500, true, 1, 10, 10});
    sys.add_port({PortType::Aqua, 2000, true, 1, 30, 30});
    sys.add_port({PortType::Aero, 800, true, 2, 50, 50});

    assert(sys.get_ports().size() == 3);
    PASS();
}

static void test_remove_port() {
    TEST("remove_port removes matching port");
    PortSystem sys(64, 64);

    sys.add_port({PortType::Aero, 500, true, 1, 10, 10});
    sys.add_port({PortType::Aqua, 2000, true, 1, 30, 30});
    assert(sys.get_ports().size() == 2);

    sys.remove_port(1, 10, 10);
    assert(sys.get_ports().size() == 1);
    assert(sys.get_ports()[0].port_type == PortType::Aqua);
    PASS();
}

static void test_remove_port_no_match() {
    TEST("remove_port does nothing if no match");
    PortSystem sys(64, 64);

    sys.add_port({PortType::Aero, 500, true, 1, 10, 10});
    sys.remove_port(2, 10, 10);  // Wrong owner
    assert(sys.get_ports().size() == 1);

    sys.remove_port(1, 99, 99);  // Wrong position
    assert(sys.get_ports().size() == 1);
    PASS();
}

static void test_clear_ports() {
    TEST("clear_ports removes all ports");
    PortSystem sys(64, 64);

    sys.add_port({PortType::Aero, 500, true, 1, 10, 10});
    sys.add_port({PortType::Aqua, 2000, true, 2, 30, 30});
    assert(sys.get_ports().size() == 2);

    sys.clear_ports();
    assert(sys.get_ports().empty());
    PASS();
}

// =============================================================================
// Global Demand Bonus via IPortProvider
// =============================================================================

static void test_global_demand_bonus_aero_exchange() {
    TEST("get_global_demand_bonus: aero ports boost Exchange");
    PortSystem sys(64, 64);

    sys.add_port({PortType::Aero, 600, true, 1, 10, 10});  // Medium: +10

    float bonus = sys.get_global_demand_bonus(1, 1);  // zone_type=1 (Exchange)
    assert(approx_eq(bonus, 10.0f));
    PASS();
}

static void test_global_demand_bonus_aqua_fabrication() {
    TEST("get_global_demand_bonus: aqua ports boost Fabrication");
    PortSystem sys(64, 64);

    sys.add_port({PortType::Aqua, 2500, true, 1, 10, 10});  // Large: +15

    float bonus = sys.get_global_demand_bonus(2, 1);  // zone_type=2 (Fabrication)
    assert(approx_eq(bonus, 15.0f));
    PASS();
}

static void test_global_demand_bonus_multiple_ports() {
    TEST("get_global_demand_bonus: multiple ports stack");
    PortSystem sys(64, 64);

    sys.add_port({PortType::Aero, 200, true, 1, 10, 10});   // Small: +5
    sys.add_port({PortType::Aero, 1000, true, 1, 20, 20});  // Medium: +10

    float bonus = sys.get_global_demand_bonus(1, 1);
    assert(approx_eq(bonus, 15.0f));
    PASS();
}

static void test_global_demand_bonus_capped() {
    TEST("get_global_demand_bonus: capped at 30");
    PortSystem sys(64, 64);

    sys.add_port({PortType::Aero, 2500, true, 1, 10, 10});
    sys.add_port({PortType::Aero, 3000, true, 1, 20, 20});
    sys.add_port({PortType::Aero, 2000, true, 1, 30, 30});

    float bonus = sys.get_global_demand_bonus(1, 1);
    assert(approx_eq(bonus, 30.0f));
    PASS();
}

static void test_global_demand_bonus_non_operational_ignored() {
    TEST("get_global_demand_bonus: non-operational ports ignored");
    PortSystem sys(64, 64);

    sys.add_port({PortType::Aero, 2000, false, 1, 10, 10});  // NOT operational
    sys.add_port({PortType::Aero, 500, true, 1, 20, 20});     // Operational: +10

    float bonus = sys.get_global_demand_bonus(1, 1);
    assert(approx_eq(bonus, 10.0f));
    PASS();
}

static void test_global_demand_bonus_owner_filtered() {
    TEST("get_global_demand_bonus: filters by owner");
    PortSystem sys(64, 64);

    sys.add_port({PortType::Aero, 1000, true, 1, 10, 10});  // Player 1: +10
    sys.add_port({PortType::Aero, 2000, true, 2, 20, 20});  // Player 2: +15

    float bonus_p1 = sys.get_global_demand_bonus(1, 1);
    assert(approx_eq(bonus_p1, 10.0f));

    float bonus_p2 = sys.get_global_demand_bonus(1, 2);
    assert(approx_eq(bonus_p2, 15.0f));
    PASS();
}

static void test_global_demand_bonus_no_ports() {
    TEST("get_global_demand_bonus: no ports returns 0");
    PortSystem sys(64, 64);

    float bonus = sys.get_global_demand_bonus(1, 1);
    assert(approx_eq(bonus, 0.0f));
    PASS();
}

static void test_global_demand_bonus_habitation_returns_zero() {
    TEST("get_global_demand_bonus: Habitation returns 0");
    PortSystem sys(64, 64);

    sys.add_port({PortType::Aero, 2000, true, 1, 10, 10});

    float bonus = sys.get_global_demand_bonus(0, 1);  // zone_type=0 (Habitation)
    assert(approx_eq(bonus, 0.0f));
    PASS();
}

// =============================================================================
// Local Demand Bonus via IPortProvider
// =============================================================================

static void test_local_demand_bonus_aero_habitation() {
    TEST("get_local_demand_bonus: aero ports boost Habitation nearby");
    PortSystem sys(64, 64);

    sys.add_port({PortType::Aero, 1000, true, 1, 10, 10});

    // Within 20-tile Manhattan distance
    float bonus = sys.get_local_demand_bonus(0, 15, 15, 1);  // zone_type=0 (Habitation)
    assert(approx_eq(bonus, 5.0f));  // LOCAL_BONUS_AERO_HABITATION
    PASS();
}

static void test_local_demand_bonus_aqua_exchange() {
    TEST("get_local_demand_bonus: aqua ports boost Exchange nearby");
    PortSystem sys(64, 64);

    sys.add_port({PortType::Aqua, 1000, true, 1, 10, 10});

    // Within 25-tile Manhattan distance
    float bonus = sys.get_local_demand_bonus(1, 15, 15, 1);  // zone_type=1 (Exchange)
    assert(approx_eq(bonus, 10.0f));  // LOCAL_BONUS_AQUA_EXCHANGE
    PASS();
}

static void test_local_demand_bonus_out_of_range() {
    TEST("get_local_demand_bonus: out of range returns 0");
    PortSystem sys(64, 64);

    sys.add_port({PortType::Aero, 1000, true, 1, 10, 10});

    // Manhattan distance = 50, exceeds 20-tile radius
    float bonus = sys.get_local_demand_bonus(0, 35, 35, 1);
    assert(approx_eq(bonus, 0.0f));
    PASS();
}

static void test_local_demand_bonus_non_operational() {
    TEST("get_local_demand_bonus: non-operational port returns 0");
    PortSystem sys(64, 64);

    sys.add_port({PortType::Aero, 1000, false, 1, 10, 10});  // NOT operational

    float bonus = sys.get_local_demand_bonus(0, 12, 12, 1);
    assert(approx_eq(bonus, 0.0f));
    PASS();
}

static void test_local_demand_bonus_multiple_ports_stack() {
    TEST("get_local_demand_bonus: multiple nearby ports stack");
    PortSystem sys(64, 64);

    sys.add_port({PortType::Aero, 1000, true, 1, 10, 10});
    sys.add_port({PortType::Aero, 500, true, 1, 12, 12});

    // Both within range of query point
    float bonus = sys.get_local_demand_bonus(0, 11, 11, 1);
    assert(approx_eq(bonus, 10.0f));  // 5.0 + 5.0
    PASS();
}

// =============================================================================
// Port State Queries via IPortProvider
// =============================================================================

static void test_port_capacity_from_data() {
    TEST("get_port_capacity: sums operational port capacities");
    PortSystem sys(64, 64);

    sys.add_port({PortType::Aero, 500, true, 1, 10, 10});
    sys.add_port({PortType::Aero, 1000, true, 1, 20, 20});
    sys.add_port({PortType::Aero, 300, false, 1, 30, 30});  // NOT operational

    uint8_t aero = static_cast<uint8_t>(PortType::Aero);
    uint32_t cap = sys.get_port_capacity(aero, 1);
    assert(cap == 1500);  // 500 + 1000, not 300
    PASS();
}

static void test_has_operational_port_true() {
    TEST("has_operational_port: returns true when operational port exists");
    PortSystem sys(64, 64);

    sys.add_port({PortType::Aqua, 1000, true, 1, 10, 10});

    uint8_t aqua = static_cast<uint8_t>(PortType::Aqua);
    assert(sys.has_operational_port(aqua, 1) == true);
    PASS();
}

static void test_has_operational_port_false() {
    TEST("has_operational_port: returns false when no operational port");
    PortSystem sys(64, 64);

    sys.add_port({PortType::Aqua, 1000, false, 1, 10, 10});  // NOT operational

    uint8_t aqua = static_cast<uint8_t>(PortType::Aqua);
    assert(sys.has_operational_port(aqua, 1) == false);
    PASS();
}

static void test_port_count() {
    TEST("get_port_count: counts all ports of type regardless of operational");
    PortSystem sys(64, 64);

    sys.add_port({PortType::Aero, 500, true, 1, 10, 10});
    sys.add_port({PortType::Aero, 300, false, 1, 20, 20});
    sys.add_port({PortType::Aqua, 1000, true, 1, 30, 30});

    uint8_t aero = static_cast<uint8_t>(PortType::Aero);
    uint8_t aqua = static_cast<uint8_t>(PortType::Aqua);
    assert(sys.get_port_count(aero, 1) == 2);
    assert(sys.get_port_count(aqua, 1) == 1);
    PASS();
}

static void test_port_count_empty() {
    TEST("get_port_count: returns 0 when no ports");
    PortSystem sys(64, 64);

    uint8_t aero = static_cast<uint8_t>(PortType::Aero);
    assert(sys.get_port_count(aero, 1) == 0);
    PASS();
}

// =============================================================================
// IPortProvider Polymorphism Tests
// =============================================================================

static void test_polymorphism_port_provider() {
    TEST("PortSystem usable through IPortProvider pointer with real data");
    PortSystem sys(64, 64);
    sys.add_port({PortType::Aero, 600, true, 1, 10, 10});

    IPortProvider* provider = &sys;

    // Global demand bonus should return real value
    float bonus = provider->get_global_demand_bonus(1, 1);
    assert(approx_eq(bonus, 10.0f));

    // Port capacity should return real value
    uint8_t aero = static_cast<uint8_t>(PortType::Aero);
    assert(provider->get_port_capacity(aero, 1) == 600);
    assert(provider->has_operational_port(aero, 1) == true);
    assert(provider->get_port_count(aero, 1) == 1);
    PASS();
}

static void test_polymorphism_stub_returns_zero() {
    TEST("StubPortProvider returns 0 for all demand queries");
    StubPortProvider stub;
    IPortProvider* provider = &stub;

    assert(approx_eq(provider->get_global_demand_bonus(1, 1), 0.0f));
    assert(approx_eq(provider->get_local_demand_bonus(0, 10, 10, 1), 0.0f));
    assert(provider->get_port_capacity(0, 1) == 0);
    assert(provider->has_operational_port(0, 1) == false);
    PASS();
}

static void test_polymorphism_switchable() {
    TEST("IPortProvider pointer can switch between real and stub");
    PortSystem real_sys(64, 64);
    real_sys.add_port({PortType::Aero, 1000, true, 1, 10, 10});

    StubPortProvider stub;

    IPortProvider* provider = &real_sys;
    float real_bonus = provider->get_global_demand_bonus(1, 1);
    assert(approx_eq(real_bonus, 10.0f));

    provider = &stub;
    float stub_bonus = provider->get_global_demand_bonus(1, 1);
    assert(approx_eq(stub_bonus, 0.0f));
    PASS();
}

// =============================================================================
// Trade Income Cache Tests
// =============================================================================

static void test_trade_income_cache() {
    TEST("set_cached_trade_income/get_trade_income round trips");
    PortSystem sys(64, 64);

    sys.set_cached_trade_income(1, 5000);
    assert(sys.get_trade_income(1) == 5000);

    sys.set_cached_trade_income(2, 12000);
    assert(sys.get_trade_income(2) == 12000);

    // Player 1 unaffected
    assert(sys.get_trade_income(1) == 5000);
    PASS();
}

static void test_trade_income_default_zero() {
    TEST("get_trade_income returns 0 by default");
    PortSystem sys(64, 64);
    assert(sys.get_trade_income(0) == 0);
    assert(sys.get_trade_income(1) == 0);
    assert(sys.get_trade_income(4) == 0);
    PASS();
}

static void test_trade_income_out_of_range() {
    TEST("get_trade_income returns 0 for out of range owner");
    PortSystem sys(64, 64);
    sys.set_cached_trade_income(1, 999);
    // Owner 255 is out of range of MAX_PLAYERS+1 array
    assert(sys.get_trade_income(255) == 0);
    PASS();
}

// =============================================================================
// Tick Integration Tests
// =============================================================================

static void test_tick_with_ports() {
    TEST("tick executes without crash when ports are present");
    PortSystem sys(64, 64);
    sys.add_port({PortType::Aero, 1000, true, 1, 10, 10});
    sys.add_port({PortType::Aqua, 2000, true, 1, 30, 30});

    sys.tick(0.05f);

    // Ports still present after tick
    assert(sys.get_ports().size() == 2);
    PASS();
}

static void test_demand_bonus_after_port_change() {
    TEST("demand bonus updates after adding/removing ports");
    PortSystem sys(64, 64);

    // No ports -> 0 bonus
    assert(approx_eq(sys.get_global_demand_bonus(1, 1), 0.0f));

    // Add port -> bonus appears
    sys.add_port({PortType::Aero, 1000, true, 1, 10, 10});
    assert(approx_eq(sys.get_global_demand_bonus(1, 1), 10.0f));

    // Remove port -> bonus goes away
    sys.remove_port(1, 10, 10);
    assert(approx_eq(sys.get_global_demand_bonus(1, 1), 0.0f));
    PASS();
}

// =============================================================================
// Main
// =============================================================================

int main() {
    printf("=== PortSystem Demand Integration Tests (E8-018) ===\n");

    // Port data management
    test_add_port();
    test_add_multiple_ports();
    test_remove_port();
    test_remove_port_no_match();
    test_clear_ports();

    // Global demand bonus
    test_global_demand_bonus_aero_exchange();
    test_global_demand_bonus_aqua_fabrication();
    test_global_demand_bonus_multiple_ports();
    test_global_demand_bonus_capped();
    test_global_demand_bonus_non_operational_ignored();
    test_global_demand_bonus_owner_filtered();
    test_global_demand_bonus_no_ports();
    test_global_demand_bonus_habitation_returns_zero();

    // Local demand bonus
    test_local_demand_bonus_aero_habitation();
    test_local_demand_bonus_aqua_exchange();
    test_local_demand_bonus_out_of_range();
    test_local_demand_bonus_non_operational();
    test_local_demand_bonus_multiple_ports_stack();

    // Port state queries
    test_port_capacity_from_data();
    test_has_operational_port_true();
    test_has_operational_port_false();
    test_port_count();
    test_port_count_empty();

    // Polymorphism
    test_polymorphism_port_provider();
    test_polymorphism_stub_returns_zero();
    test_polymorphism_switchable();

    // Trade income cache
    test_trade_income_cache();
    test_trade_income_default_zero();
    test_trade_income_out_of_range();

    // Tick integration
    test_tick_with_ports();
    test_demand_bonus_after_port_change();

    printf("\n=== Results: %d/%d passed ===\n", tests_passed, tests_total);
    return (tests_passed == tests_total) ? 0 : 1;
}
