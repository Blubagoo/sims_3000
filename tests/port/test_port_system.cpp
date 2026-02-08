/**
 * @file test_port_system.cpp
 * @brief Tests for PortSystem orchestrator (Epic 8, Ticket E8-006)
 *
 * Tests:
 * - Construction and initialization
 * - Priority 48
 * - IPortProvider stub returns (safe defaults)
 * - Tick executes without crash
 * - Map dimension queries
 * - StubPortProvider from ForwardDependencyStubs.h
 */

#include <sims3000/port/PortSystem.h>
#include <sims3000/building/ForwardDependencyStubs.h>
#include <sims3000/port/PortTypes.h>
#include <cassert>
#include <cstdio>
#include <cmath>

using namespace sims3000::port;
using namespace sims3000::building;

// =============================================================================
// Helper
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
// Construction Tests
// =============================================================================

static void test_construction() {
    TEST("Construction with map dimensions");
    PortSystem sys(64, 64);
    assert(sys.get_map_width() == 64);
    assert(sys.get_map_height() == 64);
    PASS();
}

static void test_construction_various_sizes() {
    TEST("Construction with various map sizes");
    PortSystem sys1(32, 32);
    assert(sys1.get_map_width() == 32);
    assert(sys1.get_map_height() == 32);

    PortSystem sys2(128, 256);
    assert(sys2.get_map_width() == 128);
    assert(sys2.get_map_height() == 256);

    PortSystem sys3(1, 1);
    assert(sys3.get_map_width() == 1);
    assert(sys3.get_map_height() == 1);
    PASS();
}

// =============================================================================
// Priority Tests
// =============================================================================

static void test_priority() {
    TEST("Priority is 48");
    PortSystem sys(32, 32);
    assert(sys.get_priority() == 48);
    assert(sys.get_priority() == PortSystem::TICK_PRIORITY);
    assert(PortSystem::TICK_PRIORITY == 48);
    PASS();
}

static void test_priority_ordering() {
    TEST("Priority 48 runs after RailSystem (47), before PopulationSystem (50)");
    assert(PortSystem::TICK_PRIORITY > 47);  // After RailSystem
    assert(PortSystem::TICK_PRIORITY < 50);  // Before PopulationSystem
    PASS();
}

// =============================================================================
// IPortProvider Stub Return Tests
// =============================================================================

static void test_port_capacity_stub() {
    TEST("get_port_capacity returns 0 (stub)");
    PortSystem sys(64, 64);
    uint8_t aero = static_cast<uint8_t>(PortType::Aero);
    uint8_t aqua = static_cast<uint8_t>(PortType::Aqua);
    assert(sys.get_port_capacity(aero, 0) == 0);
    assert(sys.get_port_capacity(aqua, 0) == 0);
    assert(sys.get_port_capacity(aero, 1) == 0);
    assert(sys.get_port_capacity(aqua, 3) == 0);
    PASS();
}

static void test_port_utilization_stub() {
    TEST("get_port_utilization returns 0.0f (stub)");
    PortSystem sys(64, 64);
    uint8_t aero = static_cast<uint8_t>(PortType::Aero);
    assert(std::fabs(sys.get_port_utilization(aero, 0)) < 0.001f);
    assert(std::fabs(sys.get_port_utilization(aero, 1)) < 0.001f);
    PASS();
}

static void test_has_operational_port_stub() {
    TEST("has_operational_port returns false (stub)");
    PortSystem sys(64, 64);
    uint8_t aero = static_cast<uint8_t>(PortType::Aero);
    uint8_t aqua = static_cast<uint8_t>(PortType::Aqua);
    assert(!sys.has_operational_port(aero, 0));
    assert(!sys.has_operational_port(aqua, 0));
    assert(!sys.has_operational_port(aero, 3));
    PASS();
}

static void test_port_count_stub() {
    TEST("get_port_count returns 0 (stub)");
    PortSystem sys(64, 64);
    uint8_t aero = static_cast<uint8_t>(PortType::Aero);
    uint8_t aqua = static_cast<uint8_t>(PortType::Aqua);
    assert(sys.get_port_count(aero, 0) == 0);
    assert(sys.get_port_count(aqua, 0) == 0);
    PASS();
}

static void test_global_demand_bonus_stub() {
    TEST("get_global_demand_bonus returns 0.0f (stub)");
    PortSystem sys(64, 64);
    assert(std::fabs(sys.get_global_demand_bonus(0, 0)) < 0.001f);
    assert(std::fabs(sys.get_global_demand_bonus(1, 1)) < 0.001f);
    PASS();
}

static void test_local_demand_bonus_stub() {
    TEST("get_local_demand_bonus returns 0.0f (stub)");
    PortSystem sys(64, 64);
    assert(std::fabs(sys.get_local_demand_bonus(0, 10, 10, 0)) < 0.001f);
    assert(std::fabs(sys.get_local_demand_bonus(1, 32, 32, 1)) < 0.001f);
    PASS();
}

static void test_external_connection_count_stub() {
    TEST("get_external_connection_count returns 0 (stub)");
    PortSystem sys(64, 64);
    assert(sys.get_external_connection_count(0) == 0);
    assert(sys.get_external_connection_count(1) == 0);
    assert(sys.get_external_connection_count(3) == 0);
    PASS();
}

static void test_is_connected_to_edge_stub() {
    TEST("is_connected_to_edge returns false (stub)");
    PortSystem sys(64, 64);
    uint8_t north = static_cast<uint8_t>(MapEdge::North);
    uint8_t south = static_cast<uint8_t>(MapEdge::South);
    uint8_t east = static_cast<uint8_t>(MapEdge::East);
    uint8_t west = static_cast<uint8_t>(MapEdge::West);
    assert(!sys.is_connected_to_edge(north, 0));
    assert(!sys.is_connected_to_edge(south, 0));
    assert(!sys.is_connected_to_edge(east, 1));
    assert(!sys.is_connected_to_edge(west, 3));
    PASS();
}

static void test_trade_income_stub() {
    TEST("get_trade_income returns 0 (stub)");
    PortSystem sys(64, 64);
    assert(sys.get_trade_income(0) == 0);
    assert(sys.get_trade_income(1) == 0);
    assert(sys.get_trade_income(3) == 0);
    PASS();
}

// =============================================================================
// Tick Tests
// =============================================================================

static void test_tick_no_crash() {
    TEST("Tick executes without crash");
    PortSystem sys(64, 64);
    sys.tick(0.05f);
    PASS();
}

static void test_multiple_ticks() {
    TEST("Multiple ticks execute without crash");
    PortSystem sys(64, 64);
    for (int i = 0; i < 200; ++i) {
        sys.tick(0.05f);
    }
    PASS();
}

static void test_tick_zero_delta() {
    TEST("Tick with zero delta_time executes without crash");
    PortSystem sys(64, 64);
    sys.tick(0.0f);
    PASS();
}

static void test_tick_large_delta() {
    TEST("Tick with large delta_time executes without crash");
    PortSystem sys(64, 64);
    sys.tick(10.0f);
    PASS();
}

// =============================================================================
// IPortProvider Polymorphism Tests
// =============================================================================

static void test_iport_provider_polymorphism() {
    TEST("PortSystem usable through IPortProvider pointer");
    PortSystem sys(64, 64);
    IPortProvider* provider = &sys;
    uint8_t aero = static_cast<uint8_t>(PortType::Aero);

    assert(provider->get_port_capacity(aero, 0) == 0);
    assert(std::fabs(provider->get_port_utilization(aero, 0)) < 0.001f);
    assert(!provider->has_operational_port(aero, 0));
    assert(provider->get_port_count(aero, 0) == 0);
    assert(std::fabs(provider->get_global_demand_bonus(0, 0)) < 0.001f);
    assert(std::fabs(provider->get_local_demand_bonus(0, 10, 10, 0)) < 0.001f);
    assert(provider->get_external_connection_count(0) == 0);
    assert(!provider->is_connected_to_edge(0, 0));
    assert(provider->get_trade_income(0) == 0);
    PASS();
}

// =============================================================================
// StubPortProvider Tests
// =============================================================================

static void test_stub_port_provider_defaults() {
    TEST("StubPortProvider returns safe defaults");
    StubPortProvider stub;
    uint8_t aero = static_cast<uint8_t>(PortType::Aero);
    uint8_t aqua = static_cast<uint8_t>(PortType::Aqua);

    assert(stub.get_port_capacity(aero, 0) == 0);
    assert(stub.get_port_capacity(aqua, 1) == 0);
    assert(std::fabs(stub.get_port_utilization(aero, 0)) < 0.001f);
    assert(!stub.has_operational_port(aero, 0));
    assert(!stub.has_operational_port(aqua, 0));
    assert(stub.get_port_count(aero, 0) == 0);
    assert(std::fabs(stub.get_global_demand_bonus(0, 0)) < 0.001f);
    assert(std::fabs(stub.get_local_demand_bonus(0, 5, 5, 0)) < 0.001f);
    assert(stub.get_external_connection_count(0) == 0);
    assert(!stub.is_connected_to_edge(0, 0));
    assert(stub.get_trade_income(0) == 0);
    PASS();
}

static void test_stub_port_provider_restrictive() {
    TEST("StubPortProvider restrictive mode");
    StubPortProvider stub;
    assert(!stub.is_debug_restrictive());

    stub.set_debug_restrictive(true);
    assert(stub.is_debug_restrictive());

    // Stub returns same values regardless of restrictive mode
    // (ports are opt-in, so defaults are already restrictive)
    assert(stub.get_port_capacity(0, 0) == 0);
    assert(!stub.has_operational_port(0, 0));
    assert(stub.get_trade_income(0) == 0);
    PASS();
}

static void test_stub_port_provider_polymorphism() {
    TEST("StubPortProvider usable through IPortProvider pointer");
    StubPortProvider stub;
    IPortProvider* provider = &stub;

    assert(provider->get_port_capacity(0, 0) == 0);
    assert(!provider->has_operational_port(0, 0));
    assert(provider->get_trade_income(0) == 0);
    PASS();
}

// =============================================================================
// MAX_PLAYERS Tests
// =============================================================================

static void test_max_players() {
    TEST("MAX_PLAYERS is 4");
    assert(PortSystem::MAX_PLAYERS == 4);
    PASS();
}

// =============================================================================
// Main
// =============================================================================

int main() {
    printf("=== PortSystem Tests (E8-006, E8-007) ===\n");

    // Construction
    test_construction();
    test_construction_various_sizes();

    // Priority
    test_priority();
    test_priority_ordering();

    // IPortProvider stubs
    test_port_capacity_stub();
    test_port_utilization_stub();
    test_has_operational_port_stub();
    test_port_count_stub();
    test_global_demand_bonus_stub();
    test_local_demand_bonus_stub();
    test_external_connection_count_stub();
    test_is_connected_to_edge_stub();
    test_trade_income_stub();

    // Tick
    test_tick_no_crash();
    test_multiple_ticks();
    test_tick_zero_delta();
    test_tick_large_delta();

    // Polymorphism
    test_iport_provider_polymorphism();

    // StubPortProvider
    test_stub_port_provider_defaults();
    test_stub_port_provider_restrictive();
    test_stub_port_provider_polymorphism();

    // Constants
    test_max_players();

    printf("\n=== Results: %d/%d passed ===\n", tests_passed, tests_total);
    return (tests_passed == tests_total) ? 0 : 1;
}
