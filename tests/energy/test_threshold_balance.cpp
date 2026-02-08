/**
 * @file test_threshold_balance.cpp
 * @brief Unit tests for balance deficit/collapse thresholds (Ticket 5-039)
 *
 * Tests cover:
 * - BUFFER_THRESHOLD_PERCENT == 0.10f
 * - COLLAPSE_THRESHOLD_PERCENT == 0.50f
 * - Edge case scenarios at exact threshold boundaries
 * - State machine uses configurable thresholds correctly
 * - Threshold-driven transitions via calculate_pool_state()
 * - Integration: pool state transitions with detect_pool_state_transitions()
 */

#include <sims3000/energy/EnergySystem.h>
#include <sims3000/energy/EnergyComponent.h>
#include <sims3000/energy/EnergyProducerComponent.h>
#include <sims3000/energy/EnergyEnums.h>
#include <sims3000/energy/PerPlayerEnergyPool.h>
#include <entt/entt.hpp>
#include <cstdio>
#include <cstdlib>
#include <cmath>

using namespace sims3000::energy;

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
    if (std::fabs((a) - (b)) > 1e-6f) { \
        printf("\n  FAILED: %s == %s (got %f vs %f, line %d)\n", \
               #a, #b, (double)(a), (double)(b), __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

// =============================================================================
// Helper: create a pool with specified values
// =============================================================================

static PerPlayerEnergyPool make_pool(uint32_t generated, uint32_t consumed) {
    PerPlayerEnergyPool pool{};
    pool.total_generated = generated;
    pool.total_consumed = consumed;
    pool.surplus = static_cast<int32_t>(generated) - static_cast<int32_t>(consumed);
    return pool;
}

// =============================================================================
// Task 1: Verify threshold constants have correct values
// =============================================================================

TEST(buffer_threshold_equals_ten_percent) {
    ASSERT_FLOAT_EQ(EnergySystem::BUFFER_THRESHOLD_PERCENT, 0.10f);
}

TEST(collapse_threshold_equals_fifty_percent) {
    ASSERT_FLOAT_EQ(EnergySystem::COLLAPSE_THRESHOLD_PERCENT, 0.50f);
}

TEST(buffer_threshold_is_constexpr) {
    // Verify the constant is usable in a constexpr context
    constexpr float val = EnergySystem::BUFFER_THRESHOLD_PERCENT;
    ASSERT(val > 0.0f);
    ASSERT(val < 1.0f);
}

TEST(collapse_threshold_is_constexpr) {
    constexpr float val = EnergySystem::COLLAPSE_THRESHOLD_PERCENT;
    ASSERT(val > 0.0f);
    ASSERT(val <= 1.0f);
}

// =============================================================================
// Task 2: Buffer threshold edge cases (Healthy <-> Marginal boundary)
// =============================================================================

TEST(exact_buffer_boundary_is_healthy) {
    // generated=1000, consumed=900 => surplus=100
    // buffer_threshold = 1000 * 0.10 = 100.0
    // surplus(100) >= buffer_threshold(100) => Healthy
    PerPlayerEnergyPool pool = make_pool(1000, 900);
    ASSERT_EQ(EnergySystem::calculate_pool_state(pool), EnergyPoolState::Healthy);
}

TEST(one_below_buffer_boundary_is_marginal) {
    // generated=1000, consumed=901 => surplus=99
    // buffer_threshold = 1000 * 0.10 = 100.0
    // surplus(99) < buffer_threshold(100) AND surplus >= 0 => Marginal
    PerPlayerEnergyPool pool = make_pool(1000, 901);
    ASSERT_EQ(EnergySystem::calculate_pool_state(pool), EnergyPoolState::Marginal);
}

TEST(one_above_buffer_boundary_is_healthy) {
    // generated=1000, consumed=899 => surplus=101
    // buffer_threshold = 100.0
    // surplus(101) >= 100 => Healthy
    PerPlayerEnergyPool pool = make_pool(1000, 899);
    ASSERT_EQ(EnergySystem::calculate_pool_state(pool), EnergyPoolState::Healthy);
}

TEST(zero_surplus_is_marginal) {
    // generated=1000, consumed=1000 => surplus=0
    // buffer_threshold = 100.0
    // 0 < 100 AND 0 >= 0 => Marginal
    PerPlayerEnergyPool pool = make_pool(1000, 1000);
    ASSERT_EQ(EnergySystem::calculate_pool_state(pool), EnergyPoolState::Marginal);
}

TEST(buffer_threshold_scales_with_generation) {
    // Small generation: generated=100, consumed=90 => surplus=10
    // buffer_threshold = 100 * 0.10 = 10.0
    // surplus(10) >= 10 => Healthy
    PerPlayerEnergyPool pool_small = make_pool(100, 90);
    ASSERT_EQ(EnergySystem::calculate_pool_state(pool_small), EnergyPoolState::Healthy);

    // Same ratio but larger scale: generated=10000, consumed=9000 => surplus=1000
    // buffer_threshold = 10000 * 0.10 = 1000.0
    // surplus(1000) >= 1000 => Healthy
    PerPlayerEnergyPool pool_large = make_pool(10000, 9000);
    ASSERT_EQ(EnergySystem::calculate_pool_state(pool_large), EnergyPoolState::Healthy);
}

// =============================================================================
// Task 3: Collapse threshold edge cases (Deficit <-> Collapse boundary)
// =============================================================================

TEST(exact_collapse_boundary_is_collapse) {
    // generated=500, consumed=1000 => surplus=-500
    // collapse_threshold = 1000 * 0.50 = 500.0
    // surplus(-500) <= -collapse_threshold(-500) => Collapse
    PerPlayerEnergyPool pool = make_pool(500, 1000);
    ASSERT_EQ(EnergySystem::calculate_pool_state(pool), EnergyPoolState::Collapse);
}

TEST(one_above_collapse_boundary_is_deficit) {
    // generated=501, consumed=1000 => surplus=-499
    // collapse_threshold = 1000 * 0.50 = 500.0
    // -500 < surplus(-499) < 0 => Deficit
    PerPlayerEnergyPool pool = make_pool(501, 1000);
    ASSERT_EQ(EnergySystem::calculate_pool_state(pool), EnergyPoolState::Deficit);
}

TEST(one_below_collapse_boundary_is_collapse) {
    // generated=499, consumed=1000 => surplus=-501
    // collapse_threshold = 500.0
    // surplus(-501) <= -500 => Collapse
    PerPlayerEnergyPool pool = make_pool(499, 1000);
    ASSERT_EQ(EnergySystem::calculate_pool_state(pool), EnergyPoolState::Collapse);
}

TEST(collapse_threshold_scales_with_consumption) {
    // Small consumption: consumed=200, generated=0 => surplus=-200
    // collapse_threshold = 200 * 0.50 = 100.0
    // surplus(-200) <= -100 => Collapse
    PerPlayerEnergyPool pool_small = make_pool(0, 200);
    ASSERT_EQ(EnergySystem::calculate_pool_state(pool_small), EnergyPoolState::Collapse);

    // Larger consumption: consumed=2000, generated=0 => surplus=-2000
    // collapse_threshold = 2000 * 0.50 = 1000.0
    // surplus(-2000) <= -1000 => Collapse
    PerPlayerEnergyPool pool_large = make_pool(0, 2000);
    ASSERT_EQ(EnergySystem::calculate_pool_state(pool_large), EnergyPoolState::Collapse);
}

TEST(small_deficit_is_not_collapse) {
    // generated=990, consumed=1000 => surplus=-10
    // collapse_threshold = 1000 * 0.50 = 500.0
    // -500 < surplus(-10) < 0 => Deficit (not Collapse)
    PerPlayerEnergyPool pool = make_pool(990, 1000);
    ASSERT_EQ(EnergySystem::calculate_pool_state(pool), EnergyPoolState::Deficit);
}

// =============================================================================
// Task 4: Zero-value edge cases
// =============================================================================

TEST(zero_generation_zero_consumption_is_healthy) {
    // surplus=0, buffer_threshold=0
    // 0 >= 0 => Healthy
    PerPlayerEnergyPool pool = make_pool(0, 0);
    ASSERT_EQ(EnergySystem::calculate_pool_state(pool), EnergyPoolState::Healthy);
}

TEST(zero_generation_with_consumption_is_collapse) {
    // generated=0, consumed=100 => surplus=-100
    // collapse_threshold = 100 * 0.50 = 50
    // surplus(-100) <= -50 => Collapse
    PerPlayerEnergyPool pool = make_pool(0, 100);
    ASSERT_EQ(EnergySystem::calculate_pool_state(pool), EnergyPoolState::Collapse);
}

TEST(generation_only_no_consumption_is_healthy) {
    // generated=500, consumed=0 => surplus=500
    // buffer_threshold = 500 * 0.10 = 50
    // surplus(500) >= 50 => Healthy
    PerPlayerEnergyPool pool = make_pool(500, 0);
    ASSERT_EQ(EnergySystem::calculate_pool_state(pool), EnergyPoolState::Healthy);
}

TEST(very_small_generation_with_tiny_consumption) {
    // generated=1, consumed=0 => surplus=1
    // buffer_threshold = 1 * 0.10 = 0.1
    // surplus(1.0) >= 0.1 => Healthy
    PerPlayerEnergyPool pool = make_pool(1, 0);
    ASSERT_EQ(EnergySystem::calculate_pool_state(pool), EnergyPoolState::Healthy);
}

// =============================================================================
// Task 5: State machine uses configurable thresholds correctly
// =============================================================================

TEST(state_machine_full_cycle_healthy_to_collapse_and_back) {
    EnergySystem sys(64, 64);

    // Start: Healthy (large surplus)
    auto& pool = sys.get_pool_mut(0);
    pool.total_generated = 1000;
    pool.total_consumed = 500;
    pool.surplus = 500;
    pool.state = EnergySystem::calculate_pool_state(pool);
    ASSERT_EQ(pool.state, EnergyPoolState::Healthy);

    // Transition to Marginal (reduce surplus below buffer)
    pool.total_consumed = 950;
    pool.surplus = 50;
    pool.state = EnergySystem::calculate_pool_state(pool);
    ASSERT_EQ(pool.state, EnergyPoolState::Marginal);

    // Transition to Deficit (surplus goes negative but above collapse)
    pool.total_consumed = 1100;
    pool.surplus = -100;
    pool.state = EnergySystem::calculate_pool_state(pool);
    ASSERT_EQ(pool.state, EnergyPoolState::Deficit);

    // Transition to Collapse (deficit exceeds 50% of consumed)
    pool.total_consumed = 3000;
    pool.surplus = -2000;
    pool.state = EnergySystem::calculate_pool_state(pool);
    ASSERT_EQ(pool.state, EnergyPoolState::Collapse);

    // Recovery back to Healthy
    pool.total_consumed = 500;
    pool.surplus = 500;
    pool.state = EnergySystem::calculate_pool_state(pool);
    ASSERT_EQ(pool.state, EnergyPoolState::Healthy);
}

TEST(detect_transitions_emits_events_at_threshold_boundaries) {
    EnergySystem sys(64, 64);
    auto& pool = sys.get_pool_mut(0);

    // Start Healthy
    pool.state = EnergyPoolState::Healthy;
    pool.previous_state = EnergyPoolState::Healthy;

    // Push to Deficit
    pool.state = EnergyPoolState::Deficit;
    pool.surplus = -10;
    pool.consumer_count = 5;
    sys.clear_transition_events();
    sys.detect_pool_state_transitions(0);
    ASSERT_EQ(pool.previous_state, EnergyPoolState::Deficit);

    // Verify deficit began event was emitted
    ASSERT(sys.get_deficit_began_events().size() == 1);
    ASSERT(sys.get_collapse_began_events().size() == 0);

    // Push to Collapse
    pool.state = EnergyPoolState::Collapse;
    pool.surplus = -500;
    sys.clear_transition_events();
    sys.detect_pool_state_transitions(0);
    ASSERT_EQ(pool.previous_state, EnergyPoolState::Collapse);

    // Verify collapse began event was emitted
    ASSERT(sys.get_collapse_began_events().size() == 1);

    // Recover to Healthy
    pool.state = EnergyPoolState::Healthy;
    pool.surplus = 500;
    sys.clear_transition_events();
    sys.detect_pool_state_transitions(0);
    ASSERT_EQ(pool.previous_state, EnergyPoolState::Healthy);

    // Verify both deficit ended and collapse ended events
    ASSERT(sys.get_deficit_ended_events().size() == 1);
    ASSERT(sys.get_collapse_ended_events().size() == 1);
}

TEST(threshold_consistency_across_all_players) {
    // Verify each player uses the same threshold constants
    EnergySystem sys(64, 64);

    for (uint8_t player = 0; player < 4; ++player) {
        auto& pool = sys.get_pool_mut(player);

        // Set identical values for all players
        pool.total_generated = 1000;
        pool.total_consumed = 901;
        pool.surplus = 99;
        pool.state = EnergySystem::calculate_pool_state(pool);

        // All should be Marginal (99 < buffer_threshold of 100)
        ASSERT_EQ(pool.state, EnergyPoolState::Marginal);
    }
}

TEST(boundary_between_deficit_and_marginal) {
    // surplus = -1 (just below zero) => Deficit
    PerPlayerEnergyPool pool_deficit = make_pool(999, 1000);
    ASSERT_EQ(EnergySystem::calculate_pool_state(pool_deficit), EnergyPoolState::Deficit);

    // surplus = 0 (exactly zero) => Marginal
    PerPlayerEnergyPool pool_marginal = make_pool(1000, 1000);
    ASSERT_EQ(EnergySystem::calculate_pool_state(pool_marginal), EnergyPoolState::Marginal);
}

TEST(large_values_threshold_accuracy) {
    // Test with large values to verify no integer overflow issues
    // generated=100000, consumed=90000 => surplus=10000
    // buffer_threshold = 100000 * 0.10 = 10000.0
    // surplus(10000) >= 10000 => Healthy (exact boundary)
    PerPlayerEnergyPool pool = make_pool(100000, 90000);
    ASSERT_EQ(EnergySystem::calculate_pool_state(pool), EnergyPoolState::Healthy);

    // generated=100000, consumed=90001 => surplus=9999
    // buffer_threshold = 10000.0
    // surplus(9999) < 10000 => Marginal
    PerPlayerEnergyPool pool2 = make_pool(100000, 90001);
    ASSERT_EQ(EnergySystem::calculate_pool_state(pool2), EnergyPoolState::Marginal);
}

// =============================================================================
// Main Entry Point
// =============================================================================

int main() {
    printf("=== Threshold Balance Unit Tests (Ticket 5-039) ===\n\n");

    // Threshold constant values
    RUN_TEST(buffer_threshold_equals_ten_percent);
    RUN_TEST(collapse_threshold_equals_fifty_percent);
    RUN_TEST(buffer_threshold_is_constexpr);
    RUN_TEST(collapse_threshold_is_constexpr);

    // Buffer threshold edge cases (Healthy <-> Marginal)
    RUN_TEST(exact_buffer_boundary_is_healthy);
    RUN_TEST(one_below_buffer_boundary_is_marginal);
    RUN_TEST(one_above_buffer_boundary_is_healthy);
    RUN_TEST(zero_surplus_is_marginal);
    RUN_TEST(buffer_threshold_scales_with_generation);

    // Collapse threshold edge cases (Deficit <-> Collapse)
    RUN_TEST(exact_collapse_boundary_is_collapse);
    RUN_TEST(one_above_collapse_boundary_is_deficit);
    RUN_TEST(one_below_collapse_boundary_is_collapse);
    RUN_TEST(collapse_threshold_scales_with_consumption);
    RUN_TEST(small_deficit_is_not_collapse);

    // Zero-value edge cases
    RUN_TEST(zero_generation_zero_consumption_is_healthy);
    RUN_TEST(zero_generation_with_consumption_is_collapse);
    RUN_TEST(generation_only_no_consumption_is_healthy);
    RUN_TEST(very_small_generation_with_tiny_consumption);

    // State machine uses configurable thresholds correctly
    RUN_TEST(state_machine_full_cycle_healthy_to_collapse_and_back);
    RUN_TEST(detect_transitions_emits_events_at_threshold_boundaries);
    RUN_TEST(threshold_consistency_across_all_players);
    RUN_TEST(boundary_between_deficit_and_marginal);
    RUN_TEST(large_values_threshold_accuracy);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
