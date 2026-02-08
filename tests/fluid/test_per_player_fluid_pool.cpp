/**
 * @file test_per_player_fluid_pool.cpp
 * @brief Unit tests for PerPlayerFluidPool (Epic 6, Ticket 6-006)
 *
 * Tests cover:
 * - Size verification (40 bytes)
 * - Trivially copyable for serialization
 * - Default initialization values
 * - Available calculation (total_generated + total_reservoir_stored)
 * - Surplus calculation (available - total_consumed)
 * - Negative surplus (deficit)
 * - State values (Healthy, Marginal, Deficit, Collapse)
 * - State transition tracking
 * - clear() method
 * - Reservoir-specific fields
 */

#include <sims3000/fluid/PerPlayerFluidPool.h>
#include <sims3000/fluid/FluidEnums.h>
#include <cassert>
#include <cstdio>

using namespace sims3000::fluid;

void test_pool_size() {
    printf("Testing PerPlayerFluidPool size...\n");

    assert(sizeof(PerPlayerFluidPool) == 40);

    printf("  PASS: PerPlayerFluidPool is 40 bytes\n");
}

void test_pool_trivially_copyable() {
    printf("Testing PerPlayerFluidPool is trivially copyable...\n");

    assert(std::is_trivially_copyable<PerPlayerFluidPool>::value);

    printf("  PASS: PerPlayerFluidPool is trivially copyable\n");
}

void test_pool_default_initialization() {
    printf("Testing default initialization...\n");

    PerPlayerFluidPool pool{};
    assert(pool.total_generated == 0);
    assert(pool.total_reservoir_stored == 0);
    assert(pool.total_reservoir_capacity == 0);
    assert(pool.available == 0);
    assert(pool.total_consumed == 0);
    assert(pool.surplus == 0);
    assert(pool.extractor_count == 0);
    assert(pool.reservoir_count == 0);
    assert(pool.consumer_count == 0);
    assert(pool.state == FluidPoolState::Healthy);
    assert(pool.previous_state == FluidPoolState::Healthy);
    assert(pool._padding[0] == 0);
    assert(pool._padding[1] == 0);

    printf("  PASS: Default initialization works correctly\n");
}

void test_pool_available_calculation() {
    printf("Testing available calculation...\n");

    PerPlayerFluidPool pool{};
    pool.total_generated = 500;
    pool.total_reservoir_stored = 300;
    pool.available = pool.total_generated + pool.total_reservoir_stored;

    assert(pool.available == 800);

    printf("  PASS: Available calculation is correct\n");
}

void test_pool_available_generation_only() {
    printf("Testing available with generation only (no reservoir)...\n");

    PerPlayerFluidPool pool{};
    pool.total_generated = 1000;
    pool.total_reservoir_stored = 0;
    pool.available = pool.total_generated + pool.total_reservoir_stored;

    assert(pool.available == 1000);

    printf("  PASS: Available with generation only is correct\n");
}

void test_pool_available_reservoir_only() {
    printf("Testing available with reservoir only (no generation)...\n");

    PerPlayerFluidPool pool{};
    pool.total_generated = 0;
    pool.total_reservoir_stored = 750;
    pool.available = pool.total_generated + pool.total_reservoir_stored;

    assert(pool.available == 750);

    printf("  PASS: Available with reservoir only is correct\n");
}

void test_pool_surplus_positive() {
    printf("Testing positive surplus calculation...\n");

    PerPlayerFluidPool pool{};
    pool.total_generated = 800;
    pool.total_reservoir_stored = 200;
    pool.available = pool.total_generated + pool.total_reservoir_stored;
    pool.total_consumed = 600;
    pool.surplus = static_cast<int32_t>(pool.available) -
                   static_cast<int32_t>(pool.total_consumed);

    assert(pool.surplus == 400);

    printf("  PASS: Positive surplus is correct\n");
}

void test_pool_surplus_zero() {
    printf("Testing zero surplus (balanced)...\n");

    PerPlayerFluidPool pool{};
    pool.total_generated = 400;
    pool.total_reservoir_stored = 100;
    pool.available = pool.total_generated + pool.total_reservoir_stored;
    pool.total_consumed = 500;
    pool.surplus = static_cast<int32_t>(pool.available) -
                   static_cast<int32_t>(pool.total_consumed);

    assert(pool.surplus == 0);

    printf("  PASS: Zero surplus is correct\n");
}

void test_pool_surplus_negative() {
    printf("Testing negative surplus (deficit)...\n");

    PerPlayerFluidPool pool{};
    pool.total_generated = 200;
    pool.total_reservoir_stored = 100;
    pool.available = pool.total_generated + pool.total_reservoir_stored;
    pool.total_consumed = 800;
    pool.surplus = static_cast<int32_t>(pool.available) -
                   static_cast<int32_t>(pool.total_consumed);

    assert(pool.surplus == -500);

    printf("  PASS: Negative surplus (deficit) is correct\n");
}

void test_pool_state_healthy() {
    printf("Testing Healthy state...\n");

    PerPlayerFluidPool pool{};
    pool.state = FluidPoolState::Healthy;
    assert(pool.state == FluidPoolState::Healthy);
    assert(static_cast<uint8_t>(pool.state) == 0);

    printf("  PASS: Healthy state works correctly\n");
}

void test_pool_state_marginal() {
    printf("Testing Marginal state...\n");

    PerPlayerFluidPool pool{};
    pool.state = FluidPoolState::Marginal;
    assert(pool.state == FluidPoolState::Marginal);
    assert(static_cast<uint8_t>(pool.state) == 1);

    printf("  PASS: Marginal state works correctly\n");
}

void test_pool_state_deficit() {
    printf("Testing Deficit state...\n");

    PerPlayerFluidPool pool{};
    pool.state = FluidPoolState::Deficit;
    assert(pool.state == FluidPoolState::Deficit);
    assert(static_cast<uint8_t>(pool.state) == 2);

    printf("  PASS: Deficit state works correctly\n");
}

void test_pool_state_collapse() {
    printf("Testing Collapse state...\n");

    PerPlayerFluidPool pool{};
    pool.state = FluidPoolState::Collapse;
    assert(pool.state == FluidPoolState::Collapse);
    assert(static_cast<uint8_t>(pool.state) == 3);

    printf("  PASS: Collapse state works correctly\n");
}

void test_pool_state_transition() {
    printf("Testing state transition tracking...\n");

    PerPlayerFluidPool pool{};
    assert(pool.state == FluidPoolState::Healthy);
    assert(pool.previous_state == FluidPoolState::Healthy);

    // Transition to Marginal
    pool.previous_state = pool.state;
    pool.state = FluidPoolState::Marginal;
    assert(pool.state == FluidPoolState::Marginal);
    assert(pool.previous_state == FluidPoolState::Healthy);

    // Transition to Deficit
    pool.previous_state = pool.state;
    pool.state = FluidPoolState::Deficit;
    assert(pool.state == FluidPoolState::Deficit);
    assert(pool.previous_state == FluidPoolState::Marginal);

    // Transition to Collapse
    pool.previous_state = pool.state;
    pool.state = FluidPoolState::Collapse;
    assert(pool.state == FluidPoolState::Collapse);
    assert(pool.previous_state == FluidPoolState::Deficit);

    printf("  PASS: State transition tracking works correctly\n");
}

void test_pool_counts() {
    printf("Testing extractor, reservoir, and consumer count tracking...\n");

    PerPlayerFluidPool pool{};

    pool.extractor_count = 5;
    pool.reservoir_count = 2;
    pool.consumer_count = 150;
    assert(pool.extractor_count == 5);
    assert(pool.reservoir_count == 2);
    assert(pool.consumer_count == 150);

    // Large city scenario
    pool.extractor_count = 50;
    pool.reservoir_count = 20;
    pool.consumer_count = 10000;
    assert(pool.extractor_count == 50);
    assert(pool.reservoir_count == 20);
    assert(pool.consumer_count == 10000);

    printf("  PASS: Count tracking works correctly\n");
}

void test_pool_reservoir_fields() {
    printf("Testing reservoir-specific fields...\n");

    PerPlayerFluidPool pool{};

    pool.total_reservoir_stored = 5000;
    pool.total_reservoir_capacity = 10000;
    assert(pool.total_reservoir_stored == 5000);
    assert(pool.total_reservoir_capacity == 10000);

    // Full reservoirs
    pool.total_reservoir_stored = 10000;
    assert(pool.total_reservoir_stored == pool.total_reservoir_capacity);

    // Empty reservoirs
    pool.total_reservoir_stored = 0;
    assert(pool.total_reservoir_stored == 0);

    printf("  PASS: Reservoir-specific fields work correctly\n");
}

void test_pool_clear() {
    printf("Testing clear() method...\n");

    PerPlayerFluidPool pool{};

    // Set all fields to non-default values
    pool.total_generated = 1000;
    pool.total_reservoir_stored = 500;
    pool.total_reservoir_capacity = 2000;
    pool.available = 1500;
    pool.total_consumed = 800;
    pool.surplus = 700;
    pool.extractor_count = 10;
    pool.reservoir_count = 5;
    pool.consumer_count = 200;
    pool.state = FluidPoolState::Deficit;
    pool.previous_state = FluidPoolState::Marginal;

    // Clear and verify all fields reset
    pool.clear();

    assert(pool.total_generated == 0);
    assert(pool.total_reservoir_stored == 0);
    assert(pool.total_reservoir_capacity == 0);
    assert(pool.available == 0);
    assert(pool.total_consumed == 0);
    assert(pool.surplus == 0);
    assert(pool.extractor_count == 0);
    assert(pool.reservoir_count == 0);
    assert(pool.consumer_count == 0);
    assert(pool.state == FluidPoolState::Healthy);
    assert(pool.previous_state == FluidPoolState::Healthy);

    printf("  PASS: clear() method works correctly\n");
}

void test_pool_copy() {
    printf("Testing copy semantics...\n");

    PerPlayerFluidPool original{};
    original.total_generated = 2000;
    original.total_reservoir_stored = 500;
    original.total_reservoir_capacity = 3000;
    original.available = 2500;
    original.total_consumed = 1800;
    original.surplus = 700;
    original.extractor_count = 8;
    original.reservoir_count = 3;
    original.consumer_count = 300;
    original.state = FluidPoolState::Marginal;
    original.previous_state = FluidPoolState::Healthy;

    PerPlayerFluidPool copy = original;
    assert(copy.total_generated == 2000);
    assert(copy.total_reservoir_stored == 500);
    assert(copy.total_reservoir_capacity == 3000);
    assert(copy.available == 2500);
    assert(copy.total_consumed == 1800);
    assert(copy.surplus == 700);
    assert(copy.extractor_count == 8);
    assert(copy.reservoir_count == 3);
    assert(copy.consumer_count == 300);
    assert(copy.state == FluidPoolState::Marginal);
    assert(copy.previous_state == FluidPoolState::Healthy);

    printf("  PASS: Copy semantics work correctly\n");
}

int main() {
    printf("=== PerPlayerFluidPool Unit Tests (Epic 6, Ticket 6-006) ===\n\n");

    test_pool_size();
    test_pool_trivially_copyable();
    test_pool_default_initialization();
    test_pool_available_calculation();
    test_pool_available_generation_only();
    test_pool_available_reservoir_only();
    test_pool_surplus_positive();
    test_pool_surplus_zero();
    test_pool_surplus_negative();
    test_pool_state_healthy();
    test_pool_state_marginal();
    test_pool_state_deficit();
    test_pool_state_collapse();
    test_pool_state_transition();
    test_pool_counts();
    test_pool_reservoir_fields();
    test_pool_clear();
    test_pool_copy();

    printf("\n=== All PerPlayerFluidPool Tests Passed ===\n");
    return 0;
}
