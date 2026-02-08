/**
 * @file test_per_player_energy_pool.cpp
 * @brief Unit tests for PerPlayerEnergyPool (Epic 5, Ticket 5-005)
 *
 * Tests cover:
 * - Default initialization
 * - Surplus calculation (total_generated - total_consumed)
 * - Negative surplus (deficit)
 * - State values (Healthy, Marginal, Deficit, Collapse)
 * - Trivially copyable for serialization
 */

#include <sims3000/energy/PerPlayerEnergyPool.h>
#include <sims3000/energy/EnergyEnums.h>
#include <sims3000/core/types.h>
#include <cassert>
#include <cstdio>

using namespace sims3000::energy;
using sims3000::PlayerID;

void test_pool_size() {
    printf("Testing PerPlayerEnergyPool size...\n");

    // Document actual size: 24 bytes
    assert(sizeof(PerPlayerEnergyPool) == 24);

    printf("  PASS: PerPlayerEnergyPool is 24 bytes\n");
}

void test_pool_trivially_copyable() {
    printf("Testing PerPlayerEnergyPool is trivially copyable...\n");

    assert(std::is_trivially_copyable<PerPlayerEnergyPool>::value);

    printf("  PASS: PerPlayerEnergyPool is trivially copyable\n");
}

void test_pool_default_initialization() {
    printf("Testing default initialization...\n");

    PerPlayerEnergyPool pool{};
    assert(pool.total_generated == 0);
    assert(pool.total_consumed == 0);
    assert(pool.surplus == 0);
    assert(pool.nexus_count == 0);
    assert(pool.consumer_count == 0);
    assert(pool.owner == 0);
    assert(pool.state == EnergyPoolState::Healthy);
    assert(pool.previous_state == EnergyPoolState::Healthy);
    assert(pool._padding == 0);

    printf("  PASS: Default initialization works correctly\n");
}

void test_pool_owner_assignment() {
    printf("Testing owner assignment...\n");

    PerPlayerEnergyPool pool{};

    // Assign player 1
    pool.owner = 1;
    assert(pool.owner == 1);

    // Assign player 255 (max PlayerID)
    pool.owner = 255;
    assert(pool.owner == 255);

    // Player 0 = no owner
    pool.owner = 0;
    assert(pool.owner == 0);

    printf("  PASS: Owner assignment works correctly\n");
}

void test_pool_surplus_positive() {
    printf("Testing positive surplus calculation...\n");

    PerPlayerEnergyPool pool{};
    pool.total_generated = 1000;
    pool.total_consumed = 600;
    pool.surplus = static_cast<int32_t>(pool.total_generated) -
                   static_cast<int32_t>(pool.total_consumed);

    assert(pool.surplus == 400);

    printf("  PASS: Positive surplus is correct\n");
}

void test_pool_surplus_zero() {
    printf("Testing zero surplus (balanced)...\n");

    PerPlayerEnergyPool pool{};
    pool.total_generated = 500;
    pool.total_consumed = 500;
    pool.surplus = static_cast<int32_t>(pool.total_generated) -
                   static_cast<int32_t>(pool.total_consumed);

    assert(pool.surplus == 0);

    printf("  PASS: Zero surplus is correct\n");
}

void test_pool_surplus_negative() {
    printf("Testing negative surplus (deficit)...\n");

    PerPlayerEnergyPool pool{};
    pool.total_generated = 300;
    pool.total_consumed = 800;
    pool.surplus = static_cast<int32_t>(pool.total_generated) -
                   static_cast<int32_t>(pool.total_consumed);

    assert(pool.surplus == -500);

    printf("  PASS: Negative surplus (deficit) is correct\n");
}

void test_pool_state_healthy() {
    printf("Testing Healthy state...\n");

    PerPlayerEnergyPool pool{};
    pool.state = EnergyPoolState::Healthy;
    assert(pool.state == EnergyPoolState::Healthy);
    assert(static_cast<uint8_t>(pool.state) == 0);

    printf("  PASS: Healthy state works correctly\n");
}

void test_pool_state_marginal() {
    printf("Testing Marginal state...\n");

    PerPlayerEnergyPool pool{};
    pool.state = EnergyPoolState::Marginal;
    assert(pool.state == EnergyPoolState::Marginal);
    assert(static_cast<uint8_t>(pool.state) == 1);

    printf("  PASS: Marginal state works correctly\n");
}

void test_pool_state_deficit() {
    printf("Testing Deficit state...\n");

    PerPlayerEnergyPool pool{};
    pool.state = EnergyPoolState::Deficit;
    assert(pool.state == EnergyPoolState::Deficit);
    assert(static_cast<uint8_t>(pool.state) == 2);

    printf("  PASS: Deficit state works correctly\n");
}

void test_pool_state_collapse() {
    printf("Testing Collapse state...\n");

    PerPlayerEnergyPool pool{};
    pool.state = EnergyPoolState::Collapse;
    assert(pool.state == EnergyPoolState::Collapse);
    assert(static_cast<uint8_t>(pool.state) == 3);

    printf("  PASS: Collapse state works correctly\n");
}

void test_pool_state_transition() {
    printf("Testing state transition tracking...\n");

    PerPlayerEnergyPool pool{};
    assert(pool.state == EnergyPoolState::Healthy);
    assert(pool.previous_state == EnergyPoolState::Healthy);

    // Transition to Marginal
    pool.previous_state = pool.state;
    pool.state = EnergyPoolState::Marginal;
    assert(pool.state == EnergyPoolState::Marginal);
    assert(pool.previous_state == EnergyPoolState::Healthy);

    // Transition to Deficit
    pool.previous_state = pool.state;
    pool.state = EnergyPoolState::Deficit;
    assert(pool.state == EnergyPoolState::Deficit);
    assert(pool.previous_state == EnergyPoolState::Marginal);

    printf("  PASS: State transition tracking works correctly\n");
}

void test_pool_nexus_and_consumer_counts() {
    printf("Testing nexus and consumer count tracking...\n");

    PerPlayerEnergyPool pool{};

    pool.nexus_count = 3;
    pool.consumer_count = 150;
    assert(pool.nexus_count == 3);
    assert(pool.consumer_count == 150);

    // Large city scenario
    pool.nexus_count = 20;
    pool.consumer_count = 10000;
    assert(pool.nexus_count == 20);
    assert(pool.consumer_count == 10000);

    printf("  PASS: Count tracking works correctly\n");
}

void test_pool_copy() {
    printf("Testing copy semantics...\n");

    PerPlayerEnergyPool original{};
    original.owner = 1;
    original.state = EnergyPoolState::Marginal;
    original.previous_state = EnergyPoolState::Healthy;
    original.total_generated = 2000;
    original.total_consumed = 1800;
    original.surplus = 200;
    original.nexus_count = 5;
    original.consumer_count = 300;

    PerPlayerEnergyPool copy = original;
    assert(copy.owner == 1);
    assert(copy.state == EnergyPoolState::Marginal);
    assert(copy.previous_state == EnergyPoolState::Healthy);
    assert(copy.total_generated == 2000);
    assert(copy.total_consumed == 1800);
    assert(copy.surplus == 200);
    assert(copy.nexus_count == 5);
    assert(copy.consumer_count == 300);

    printf("  PASS: Copy semantics work correctly\n");
}

int main() {
    printf("=== PerPlayerEnergyPool Unit Tests (Epic 5, Ticket 5-005) ===\n\n");

    test_pool_size();
    test_pool_trivially_copyable();
    test_pool_default_initialization();
    test_pool_owner_assignment();
    test_pool_surplus_positive();
    test_pool_surplus_zero();
    test_pool_surplus_negative();
    test_pool_state_healthy();
    test_pool_state_marginal();
    test_pool_state_deficit();
    test_pool_state_collapse();
    test_pool_state_transition();
    test_pool_nexus_and_consumer_counts();
    test_pool_copy();

    printf("\n=== All PerPlayerEnergyPool Tests Passed ===\n");
    return 0;
}
