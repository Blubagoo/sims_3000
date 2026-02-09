/**
 * @file test_demand_system.cpp
 * @brief Unit tests for DemandSystem skeleton (Ticket E10-042)
 *
 * Tests cover:
 * - Construction and defaults
 * - ISimulatable interface (priority, name)
 * - Player management (add/remove/has)
 * - IDemandProvider interface (get_demand, get_demand_cap, has_positive_demand)
 * - Default demand values (all zero)
 * - Manual demand data mutation and retrieval
 * - tick() runs without crash
 * - Frequency gating constant
 */

#include <sims3000/demand/DemandSystem.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

using namespace sims3000;
using namespace sims3000::demand;

// Test result tracking
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) void test_##name()
#define RUN_TEST(name) do { \
    printf("Running %s...", #name); \
    test_##name(); \
    printf(" PASSED\n"); \
    tests_passed++; \
} while(0)

#define ASSERT(condition) do { \
    if (!(condition)) { \
        printf("\n  FAILED: %s (line %d)\n", #condition, __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

#define ASSERT_EQ(a, b) do { \
    if ((a) != (b)) { \
        printf("\n  FAILED: %s == %s (line %d)\n", #a, #b, __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

#define ASSERT_FLOAT_EQ(a, b) do { \
    float _a = (a); float _b = (b); \
    if (_a != _b) { \
        printf("\n  FAILED: %s (%.2f) == %s (%.2f) (line %d)\n", #a, _a, #b, _b, __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

// =============================================================================
// Minimal ISimulationTime stub for testing tick()
// =============================================================================

class StubSimulationTime : public ISimulationTime {
public:
    SimulationTick tick_value = 0;

    SimulationTick getCurrentTick() const override { return tick_value; }
    float getTickDelta() const override { return 0.05f; }
    float getInterpolation() const override { return 0.0f; }
    double getTotalTime() const override { return static_cast<double>(tick_value) * 0.05; }
};

// =============================================================================
// Construction Tests
// =============================================================================

TEST(construction) {
    DemandSystem system;
    // Should not crash, and no players should be active
    ASSERT(!system.has_player(0));
    ASSERT(!system.has_player(1));
    ASSERT(!system.has_player(2));
    ASSERT(!system.has_player(3));
}

// =============================================================================
// ISimulatable Interface Tests
// =============================================================================

TEST(get_priority) {
    DemandSystem system;
    ASSERT_EQ(system.getPriority(), 52);
}

TEST(get_name) {
    DemandSystem system;
    ASSERT_EQ(strcmp(system.getName(), "DemandSystem"), 0);
}

// =============================================================================
// Player Management Tests
// =============================================================================

TEST(add_player) {
    DemandSystem system;
    system.add_player(0);
    ASSERT(system.has_player(0));
    ASSERT(!system.has_player(1));
}

TEST(add_multiple_players) {
    DemandSystem system;
    system.add_player(0);
    system.add_player(1);
    system.add_player(2);
    system.add_player(3);
    ASSERT(system.has_player(0));
    ASSERT(system.has_player(1));
    ASSERT(system.has_player(2));
    ASSERT(system.has_player(3));
}

TEST(remove_player) {
    DemandSystem system;
    system.add_player(0);
    ASSERT(system.has_player(0));
    system.remove_player(0);
    ASSERT(!system.has_player(0));
}

TEST(has_player_out_of_range) {
    DemandSystem system;
    ASSERT(!system.has_player(4));
    ASSERT(!system.has_player(255));
}

TEST(add_player_out_of_range) {
    DemandSystem system;
    // Should not crash
    system.add_player(4);
    system.add_player(255);
    ASSERT(!system.has_player(4));
}

TEST(remove_player_out_of_range) {
    DemandSystem system;
    // Should not crash
    system.remove_player(4);
    system.remove_player(255);
}

TEST(add_player_resets_data) {
    DemandSystem system;
    system.add_player(0);
    system.get_demand_data_mut(0).habitation_demand = 50;
    ASSERT_FLOAT_EQ(system.get_demand(0, 0), 50.0f);

    // Re-adding should reset
    system.add_player(0);
    ASSERT_FLOAT_EQ(system.get_demand(0, 0), 0.0f);
}

// =============================================================================
// IDemandProvider: get_demand Tests
// =============================================================================

TEST(get_demand_returns_zero_initially) {
    DemandSystem system;
    system.add_player(0);

    // All zone types should return 0 initially
    ASSERT_FLOAT_EQ(system.get_demand(0, 0), 0.0f); // habitation
    ASSERT_FLOAT_EQ(system.get_demand(1, 0), 0.0f); // exchange
    ASSERT_FLOAT_EQ(system.get_demand(2, 0), 0.0f); // fabrication
}

TEST(get_demand_inactive_player) {
    DemandSystem system;
    // No players added — should return 0
    ASSERT_FLOAT_EQ(system.get_demand(0, 0), 0.0f);
    ASSERT_FLOAT_EQ(system.get_demand(1, 0), 0.0f);
    ASSERT_FLOAT_EQ(system.get_demand(2, 0), 0.0f);
}

TEST(get_demand_invalid_player) {
    DemandSystem system;
    // Out of range player
    ASSERT_FLOAT_EQ(system.get_demand(0, 10), 0.0f);
    ASSERT_FLOAT_EQ(system.get_demand(0, 255), 0.0f);
}

TEST(get_demand_invalid_zone_type) {
    DemandSystem system;
    system.add_player(0);
    // Invalid zone type should return 0
    ASSERT_FLOAT_EQ(system.get_demand(3, 0), 0.0f);
    ASSERT_FLOAT_EQ(system.get_demand(255, 0), 0.0f);
}

// =============================================================================
// IDemandProvider: get_demand_cap Tests
// =============================================================================

TEST(get_demand_cap_returns_zero_initially) {
    DemandSystem system;
    system.add_player(0);

    ASSERT_EQ(system.get_demand_cap(0, 0), 0u); // habitation
    ASSERT_EQ(system.get_demand_cap(1, 0), 0u); // exchange
    ASSERT_EQ(system.get_demand_cap(2, 0), 0u); // fabrication
}

TEST(get_demand_cap_inactive_player) {
    DemandSystem system;
    ASSERT_EQ(system.get_demand_cap(0, 0), 0u);
}

TEST(get_demand_cap_invalid_zone_type) {
    DemandSystem system;
    system.add_player(0);
    ASSERT_EQ(system.get_demand_cap(3, 0), 0u);
    ASSERT_EQ(system.get_demand_cap(255, 0), 0u);
}

// =============================================================================
// IDemandProvider: has_positive_demand Tests
// =============================================================================

TEST(has_positive_demand_false_initially) {
    DemandSystem system;
    system.add_player(0);

    ASSERT(!system.has_positive_demand(0, 0));
    ASSERT(!system.has_positive_demand(1, 0));
    ASSERT(!system.has_positive_demand(2, 0));
}

TEST(has_positive_demand_inactive_player) {
    DemandSystem system;
    ASSERT(!system.has_positive_demand(0, 0));
}

// =============================================================================
// Manual Demand Data Mutation and Retrieval Tests
// =============================================================================

TEST(set_and_get_habitation_demand) {
    DemandSystem system;
    system.add_player(0);
    system.get_demand_data_mut(0).habitation_demand = 75;
    ASSERT_FLOAT_EQ(system.get_demand(0, 0), 75.0f);
}

TEST(set_and_get_exchange_demand) {
    DemandSystem system;
    system.add_player(0);
    system.get_demand_data_mut(0).exchange_demand = -30;
    ASSERT_FLOAT_EQ(system.get_demand(1, 0), -30.0f);
}

TEST(set_and_get_fabrication_demand) {
    DemandSystem system;
    system.add_player(0);
    system.get_demand_data_mut(0).fabrication_demand = 100;
    ASSERT_FLOAT_EQ(system.get_demand(2, 0), 100.0f);
}

TEST(set_and_get_demand_caps) {
    DemandSystem system;
    system.add_player(0);

    DemandData& data = system.get_demand_data_mut(0);
    data.habitation_cap = 1000;
    data.exchange_cap = 500;
    data.fabrication_cap = 2000;

    ASSERT_EQ(system.get_demand_cap(0, 0), 1000u);
    ASSERT_EQ(system.get_demand_cap(1, 0), 500u);
    ASSERT_EQ(system.get_demand_cap(2, 0), 2000u);
}

TEST(has_positive_demand_after_set) {
    DemandSystem system;
    system.add_player(0);

    system.get_demand_data_mut(0).habitation_demand = 50;
    ASSERT(system.has_positive_demand(0, 0));

    // Negative demand
    system.get_demand_data_mut(0).exchange_demand = -10;
    ASSERT(!system.has_positive_demand(1, 0));

    // Zero demand
    system.get_demand_data_mut(0).fabrication_demand = 0;
    ASSERT(!system.has_positive_demand(2, 0));
}

TEST(get_demand_data_const) {
    DemandSystem system;
    system.add_player(1);
    system.get_demand_data_mut(1).habitation_demand = 42;

    const DemandSystem& const_sys = system;
    const DemandData& data = const_sys.get_demand_data(1);
    ASSERT_EQ(data.habitation_demand, 42);
}

TEST(get_demand_data_invalid_player) {
    DemandSystem system;
    // No players active — should return empty data
    const DemandData& data = system.get_demand_data(0);
    ASSERT_EQ(data.habitation_demand, 0);
    ASSERT_EQ(data.exchange_demand, 0);
    ASSERT_EQ(data.fabrication_demand, 0);
}

TEST(multiple_players_independent) {
    DemandSystem system;
    system.add_player(0);
    system.add_player(1);

    system.get_demand_data_mut(0).habitation_demand = 50;
    system.get_demand_data_mut(1).habitation_demand = -25;

    ASSERT_FLOAT_EQ(system.get_demand(0, 0), 50.0f);
    ASSERT_FLOAT_EQ(system.get_demand(0, 1), -25.0f);
}

// =============================================================================
// tick() Tests
// =============================================================================

TEST(tick_no_crash_no_players) {
    DemandSystem system;
    StubSimulationTime time;
    time.tick_value = 0;
    system.tick(time);
    // Should not crash
}

TEST(tick_no_crash_with_players) {
    DemandSystem system;
    system.add_player(0);
    system.add_player(1);

    StubSimulationTime time;
    for (uint64_t t = 0; t < 20; ++t) {
        time.tick_value = t;
        system.tick(time);
    }
    // Should not crash after 20 ticks
}

TEST(tick_no_crash_after_remove) {
    DemandSystem system;
    system.add_player(0);

    StubSimulationTime time;
    time.tick_value = 0;
    system.tick(time);

    system.remove_player(0);
    time.tick_value = 5;
    system.tick(time);
    // Should not crash
}

// =============================================================================
// Frequency Gating Constant Test
// =============================================================================

TEST(demand_cycle_ticks_constant) {
    ASSERT_EQ(DemandSystem::DEMAND_CYCLE_TICKS, 5u);
}

// =============================================================================
// Main Entry Point
// =============================================================================

int main() {
    printf("=== DemandSystem Unit Tests (Ticket E10-042) ===\n\n");

    // Construction
    RUN_TEST(construction);

    // ISimulatable interface
    RUN_TEST(get_priority);
    RUN_TEST(get_name);

    // Player management
    RUN_TEST(add_player);
    RUN_TEST(add_multiple_players);
    RUN_TEST(remove_player);
    RUN_TEST(has_player_out_of_range);
    RUN_TEST(add_player_out_of_range);
    RUN_TEST(remove_player_out_of_range);
    RUN_TEST(add_player_resets_data);

    // get_demand
    RUN_TEST(get_demand_returns_zero_initially);
    RUN_TEST(get_demand_inactive_player);
    RUN_TEST(get_demand_invalid_player);
    RUN_TEST(get_demand_invalid_zone_type);

    // get_demand_cap
    RUN_TEST(get_demand_cap_returns_zero_initially);
    RUN_TEST(get_demand_cap_inactive_player);
    RUN_TEST(get_demand_cap_invalid_zone_type);

    // has_positive_demand
    RUN_TEST(has_positive_demand_false_initially);
    RUN_TEST(has_positive_demand_inactive_player);

    // Manual data mutation
    RUN_TEST(set_and_get_habitation_demand);
    RUN_TEST(set_and_get_exchange_demand);
    RUN_TEST(set_and_get_fabrication_demand);
    RUN_TEST(set_and_get_demand_caps);
    RUN_TEST(has_positive_demand_after_set);
    RUN_TEST(get_demand_data_const);
    RUN_TEST(get_demand_data_invalid_player);
    RUN_TEST(multiple_players_independent);

    // tick()
    RUN_TEST(tick_no_crash_no_players);
    RUN_TEST(tick_no_crash_with_players);
    RUN_TEST(tick_no_crash_after_remove);

    // Constants
    RUN_TEST(demand_cycle_ticks_constant);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
