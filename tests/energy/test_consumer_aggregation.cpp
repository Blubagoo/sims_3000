/**
 * @file test_consumer_aggregation.cpp
 * @brief Unit tests for consumer registration and requirement aggregation (Ticket 5-011)
 *
 * Tests cover:
 * - register_consumer_position / unregister_consumer_position
 * - get_consumer_position_count
 * - aggregate_consumption with all consumers in coverage
 * - aggregate_consumption with no consumers in coverage
 * - aggregate_consumption with mixed coverage (some in, some out)
 * - aggregate_consumption with no registry returns 0
 * - aggregate_consumption with invalid owner returns 0
 * - aggregate_consumption ignores entities without EnergyComponent
 * - tick() integration sets pool.total_consumed
 * - Multi-player isolation
 */

#include <sims3000/energy/EnergySystem.h>
#include <sims3000/energy/EnergyComponent.h>
#include <sims3000/energy/EnergyProducerComponent.h>
#include <sims3000/energy/EnergyEnums.h>
#include <entt/entt.hpp>
#include <cstdio>
#include <cstdlib>

using namespace sims3000::energy;

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

// =============================================================================
// Helper: set up coverage at a position for an owner
// Coverage grid uses overseer_id (1-based): overseer_id = player_id + 1
// =============================================================================

static void set_coverage(EnergySystem& sys, uint32_t x, uint32_t y, uint8_t player_id) {
    uint8_t overseer_id = player_id + 1;
    sys.get_coverage_grid_mut().set(x, y, overseer_id);
}

// =============================================================================
// register_consumer_position / unregister_consumer_position
// =============================================================================

TEST(register_consumer_position_increments_count) {
    EnergySystem sys(64, 64);
    ASSERT_EQ(sys.get_consumer_position_count(0), 0u);

    sys.register_consumer_position(100, 0, 5, 5);
    ASSERT_EQ(sys.get_consumer_position_count(0), 1u);

    sys.register_consumer_position(101, 0, 10, 10);
    ASSERT_EQ(sys.get_consumer_position_count(0), 2u);
}

TEST(unregister_consumer_position_decrements_count) {
    EnergySystem sys(64, 64);

    sys.register_consumer_position(100, 0, 5, 5);
    sys.register_consumer_position(101, 0, 10, 10);
    ASSERT_EQ(sys.get_consumer_position_count(0), 2u);

    sys.unregister_consumer_position(100, 0, 5, 5);
    ASSERT_EQ(sys.get_consumer_position_count(0), 1u);

    sys.unregister_consumer_position(101, 0, 10, 10);
    ASSERT_EQ(sys.get_consumer_position_count(0), 0u);
}

TEST(register_consumer_position_invalid_owner_is_noop) {
    EnergySystem sys(64, 64);

    sys.register_consumer_position(100, MAX_PLAYERS, 5, 5);
    ASSERT_EQ(sys.get_consumer_position_count(MAX_PLAYERS), 0u);

    sys.register_consumer_position(101, 255, 5, 5);
    ASSERT_EQ(sys.get_consumer_position_count(255), 0u);
}

TEST(unregister_consumer_position_invalid_owner_is_noop) {
    EnergySystem sys(64, 64);

    // Should not crash
    sys.unregister_consumer_position(100, MAX_PLAYERS, 5, 5);
    sys.unregister_consumer_position(101, 255, 5, 5);
}

TEST(consumer_positions_per_player_isolation) {
    EnergySystem sys(64, 64);

    sys.register_consumer_position(100, 0, 5, 5);
    sys.register_consumer_position(200, 1, 10, 10);
    sys.register_consumer_position(201, 1, 11, 11);

    ASSERT_EQ(sys.get_consumer_position_count(0), 1u);
    ASSERT_EQ(sys.get_consumer_position_count(1), 2u);
    ASSERT_EQ(sys.get_consumer_position_count(2), 0u);
    ASSERT_EQ(sys.get_consumer_position_count(3), 0u);
}

// =============================================================================
// aggregate_consumption - Basic scenarios
// =============================================================================

TEST(aggregate_no_registry_returns_zero) {
    EnergySystem sys(64, 64);
    // No registry set
    sys.register_consumer_position(100, 0, 5, 5);
    set_coverage(sys, 5, 5, 0);

    ASSERT_EQ(sys.aggregate_consumption(0), 0u);
}

TEST(aggregate_no_consumers_returns_zero) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    ASSERT_EQ(sys.aggregate_consumption(0), 0u);
}

TEST(aggregate_invalid_owner_returns_zero) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    ASSERT_EQ(sys.aggregate_consumption(MAX_PLAYERS), 0u);
    ASSERT_EQ(sys.aggregate_consumption(255), 0u);
}

TEST(aggregate_all_consumers_in_coverage) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    // Create 3 consumer entities with EnergyComponent
    auto e1 = reg.create();
    auto& ec1 = reg.emplace<EnergyComponent>(e1);
    ec1.energy_required = 100;

    auto e2 = reg.create();
    auto& ec2 = reg.emplace<EnergyComponent>(e2);
    ec2.energy_required = 200;

    auto e3 = reg.create();
    auto& ec3 = reg.emplace<EnergyComponent>(e3);
    ec3.energy_required = 300;

    // Register consumer positions
    sys.register_consumer_position(static_cast<uint32_t>(e1), 0, 5, 5);
    sys.register_consumer_position(static_cast<uint32_t>(e2), 0, 10, 10);
    sys.register_consumer_position(static_cast<uint32_t>(e3), 0, 15, 15);

    // Set all positions in coverage for player 0
    set_coverage(sys, 5, 5, 0);
    set_coverage(sys, 10, 10, 0);
    set_coverage(sys, 15, 15, 0);

    // 100 + 200 + 300 = 600
    ASSERT_EQ(sys.aggregate_consumption(0), 600u);
}

TEST(aggregate_no_consumers_in_coverage) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    auto e1 = reg.create();
    auto& ec1 = reg.emplace<EnergyComponent>(e1);
    ec1.energy_required = 100;

    auto e2 = reg.create();
    auto& ec2 = reg.emplace<EnergyComponent>(e2);
    ec2.energy_required = 200;

    // Register consumer positions but do NOT set coverage
    sys.register_consumer_position(static_cast<uint32_t>(e1), 0, 5, 5);
    sys.register_consumer_position(static_cast<uint32_t>(e2), 0, 10, 10);

    // No coverage set -> all out of coverage
    ASSERT_EQ(sys.aggregate_consumption(0), 0u);
}

TEST(aggregate_mixed_coverage) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    auto e1 = reg.create();
    auto& ec1 = reg.emplace<EnergyComponent>(e1);
    ec1.energy_required = 100;

    auto e2 = reg.create();
    auto& ec2 = reg.emplace<EnergyComponent>(e2);
    ec2.energy_required = 200;

    auto e3 = reg.create();
    auto& ec3 = reg.emplace<EnergyComponent>(e3);
    ec3.energy_required = 300;

    sys.register_consumer_position(static_cast<uint32_t>(e1), 0, 5, 5);
    sys.register_consumer_position(static_cast<uint32_t>(e2), 0, 10, 10);
    sys.register_consumer_position(static_cast<uint32_t>(e3), 0, 15, 15);

    // Only e1 and e3 are in coverage
    set_coverage(sys, 5, 5, 0);
    // e2 at (10,10) is NOT in coverage
    set_coverage(sys, 15, 15, 0);

    // Only e1 (100) + e3 (300) = 400
    ASSERT_EQ(sys.aggregate_consumption(0), 400u);
}

TEST(aggregate_consumer_wrong_player_coverage_not_counted) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    auto e1 = reg.create();
    auto& ec1 = reg.emplace<EnergyComponent>(e1);
    ec1.energy_required = 500;

    // Consumer registered for player 0
    sys.register_consumer_position(static_cast<uint32_t>(e1), 0, 5, 5);

    // But coverage is for player 1 (overseer_id = 2)
    set_coverage(sys, 5, 5, 1);

    // Player 0 consumer is NOT in player 0 coverage
    ASSERT_EQ(sys.aggregate_consumption(0), 0u);
}

// =============================================================================
// aggregate_consumption - Edge cases
// =============================================================================

TEST(aggregate_entity_without_energy_component_skipped) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    // Create entity WITHOUT EnergyComponent
    auto e1 = reg.create();
    // No EnergyComponent added

    // Create entity WITH EnergyComponent
    auto e2 = reg.create();
    auto& ec2 = reg.emplace<EnergyComponent>(e2);
    ec2.energy_required = 250;

    sys.register_consumer_position(static_cast<uint32_t>(e1), 0, 5, 5);
    sys.register_consumer_position(static_cast<uint32_t>(e2), 0, 10, 10);

    set_coverage(sys, 5, 5, 0);
    set_coverage(sys, 10, 10, 0);

    // Only e2 contributes: 250
    ASSERT_EQ(sys.aggregate_consumption(0), 250u);
}

TEST(aggregate_destroyed_entity_skipped) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    auto e1 = reg.create();
    auto& ec1 = reg.emplace<EnergyComponent>(e1);
    ec1.energy_required = 100;

    auto e2 = reg.create();
    auto& ec2 = reg.emplace<EnergyComponent>(e2);
    ec2.energy_required = 200;

    sys.register_consumer_position(static_cast<uint32_t>(e1), 0, 5, 5);
    sys.register_consumer_position(static_cast<uint32_t>(e2), 0, 10, 10);

    set_coverage(sys, 5, 5, 0);
    set_coverage(sys, 10, 10, 0);

    // Destroy e1 in registry (but position still registered)
    reg.destroy(e1);

    // Only e2 contributes: 200
    ASSERT_EQ(sys.aggregate_consumption(0), 200u);
}

TEST(aggregate_consumer_with_zero_required) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    auto e1 = reg.create();
    auto& ec1 = reg.emplace<EnergyComponent>(e1);
    ec1.energy_required = 0;  // Zero demand

    auto e2 = reg.create();
    auto& ec2 = reg.emplace<EnergyComponent>(e2);
    ec2.energy_required = 300;

    sys.register_consumer_position(static_cast<uint32_t>(e1), 0, 5, 5);
    sys.register_consumer_position(static_cast<uint32_t>(e2), 0, 10, 10);

    set_coverage(sys, 5, 5, 0);
    set_coverage(sys, 10, 10, 0);

    // 0 + 300 = 300
    ASSERT_EQ(sys.aggregate_consumption(0), 300u);
}

TEST(aggregate_after_unregister_position) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    auto e1 = reg.create();
    auto& ec1 = reg.emplace<EnergyComponent>(e1);
    ec1.energy_required = 100;

    auto e2 = reg.create();
    auto& ec2 = reg.emplace<EnergyComponent>(e2);
    ec2.energy_required = 200;

    sys.register_consumer_position(static_cast<uint32_t>(e1), 0, 5, 5);
    sys.register_consumer_position(static_cast<uint32_t>(e2), 0, 10, 10);

    set_coverage(sys, 5, 5, 0);
    set_coverage(sys, 10, 10, 0);

    // Both in coverage: 100 + 200 = 300
    ASSERT_EQ(sys.aggregate_consumption(0), 300u);

    // Unregister e1 position
    sys.unregister_consumer_position(static_cast<uint32_t>(e1), 0, 5, 5);

    // Only e2 remains: 200
    ASSERT_EQ(sys.aggregate_consumption(0), 200u);
}

// =============================================================================
// Multi-player isolation
// =============================================================================

TEST(aggregate_multi_player_isolation) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    // Player 0 consumers
    auto e0a = reg.create();
    auto& ec0a = reg.emplace<EnergyComponent>(e0a);
    ec0a.energy_required = 100;

    auto e0b = reg.create();
    auto& ec0b = reg.emplace<EnergyComponent>(e0b);
    ec0b.energy_required = 150;

    sys.register_consumer_position(static_cast<uint32_t>(e0a), 0, 5, 5);
    sys.register_consumer_position(static_cast<uint32_t>(e0b), 0, 6, 6);

    // Player 1 consumers
    auto e1a = reg.create();
    auto& ec1a = reg.emplace<EnergyComponent>(e1a);
    ec1a.energy_required = 500;

    sys.register_consumer_position(static_cast<uint32_t>(e1a), 1, 30, 30);

    // Set coverage for each player
    set_coverage(sys, 5, 5, 0);
    set_coverage(sys, 6, 6, 0);
    set_coverage(sys, 30, 30, 1);

    // Player 0: 100 + 150 = 250
    ASSERT_EQ(sys.aggregate_consumption(0), 250u);
    // Player 1: 500
    ASSERT_EQ(sys.aggregate_consumption(1), 500u);
    // Player 2: no consumers
    ASSERT_EQ(sys.aggregate_consumption(2), 0u);
}

// =============================================================================
// tick() integration - sets pool.total_consumed
// =============================================================================

TEST(tick_sets_pool_total_consumed) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    // Create consumers for player 0
    auto e1 = reg.create();
    auto& ec1 = reg.emplace<EnergyComponent>(e1);
    ec1.energy_required = 100;

    auto e2 = reg.create();
    auto& ec2 = reg.emplace<EnergyComponent>(e2);
    ec2.energy_required = 200;

    sys.register_consumer(static_cast<uint32_t>(e1), 0);
    sys.register_consumer(static_cast<uint32_t>(e2), 0);
    sys.register_consumer_position(static_cast<uint32_t>(e1), 0, 5, 5);
    sys.register_consumer_position(static_cast<uint32_t>(e2), 0, 10, 10);

    // Set both in coverage
    set_coverage(sys, 5, 5, 0);
    set_coverage(sys, 10, 10, 0);

    // Initial pool should have 0 total_consumed
    ASSERT_EQ(sys.get_pool(0).total_consumed, 0u);

    // Run tick
    sys.tick(0.05f);

    // Pool should now reflect aggregated consumption
    ASSERT_EQ(sys.get_pool(0).total_consumed, 300u);
}

TEST(tick_total_consumed_only_in_coverage) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    auto e1 = reg.create();
    auto& ec1 = reg.emplace<EnergyComponent>(e1);
    ec1.energy_required = 100;

    auto e2 = reg.create();
    auto& ec2 = reg.emplace<EnergyComponent>(e2);
    ec2.energy_required = 200;

    sys.register_consumer(static_cast<uint32_t>(e1), 0);
    sys.register_consumer(static_cast<uint32_t>(e2), 0);
    sys.register_consumer_position(static_cast<uint32_t>(e1), 0, 5, 5);
    sys.register_consumer_position(static_cast<uint32_t>(e2), 0, 10, 10);

    // Only e1 is in coverage
    set_coverage(sys, 5, 5, 0);

    sys.tick(0.05f);

    // Only e1's 100 should be counted
    ASSERT_EQ(sys.get_pool(0).total_consumed, 100u);
}

TEST(tick_total_consumed_updates_each_tick) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    auto e1 = reg.create();
    auto& ec1 = reg.emplace<EnergyComponent>(e1);
    ec1.energy_required = 100;

    sys.register_consumer(static_cast<uint32_t>(e1), 0);
    sys.register_consumer_position(static_cast<uint32_t>(e1), 0, 5, 5);
    set_coverage(sys, 5, 5, 0);

    sys.tick(0.05f);
    ASSERT_EQ(sys.get_pool(0).total_consumed, 100u);

    // Change energy_required and tick again
    ec1.energy_required = 500;
    sys.tick(0.05f);
    ASSERT_EQ(sys.get_pool(0).total_consumed, 500u);
}

TEST(tick_total_consumed_multi_player) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    // Player 0 consumer
    auto e0 = reg.create();
    auto& ec0 = reg.emplace<EnergyComponent>(e0);
    ec0.energy_required = 100;
    sys.register_consumer(static_cast<uint32_t>(e0), 0);
    sys.register_consumer_position(static_cast<uint32_t>(e0), 0, 5, 5);
    set_coverage(sys, 5, 5, 0);

    // Player 1 consumer
    auto e1 = reg.create();
    auto& ec1 = reg.emplace<EnergyComponent>(e1);
    ec1.energy_required = 700;
    sys.register_consumer(static_cast<uint32_t>(e1), 1);
    sys.register_consumer_position(static_cast<uint32_t>(e1), 1, 30, 30);
    set_coverage(sys, 30, 30, 1);

    sys.tick(0.05f);

    ASSERT_EQ(sys.get_pool(0).total_consumed, 100u);
    ASSERT_EQ(sys.get_pool(1).total_consumed, 700u);
    ASSERT_EQ(sys.get_pool(2).total_consumed, 0u);
    ASSERT_EQ(sys.get_pool(3).total_consumed, 0u);
}

TEST(tick_no_registry_does_not_crash_consumption) {
    EnergySystem sys(64, 64);
    // No registry set
    sys.register_consumer(42, 0);
    sys.tick(0.05f);

    // total_consumed should remain 0
    ASSERT_EQ(sys.get_pool(0).total_consumed, 0u);
}

// =============================================================================
// Main Entry Point
// =============================================================================

int main() {
    printf("=== Consumer Aggregation Unit Tests (Ticket 5-011) ===\n\n");

    // Position registration
    RUN_TEST(register_consumer_position_increments_count);
    RUN_TEST(unregister_consumer_position_decrements_count);
    RUN_TEST(register_consumer_position_invalid_owner_is_noop);
    RUN_TEST(unregister_consumer_position_invalid_owner_is_noop);
    RUN_TEST(consumer_positions_per_player_isolation);

    // Aggregation basic
    RUN_TEST(aggregate_no_registry_returns_zero);
    RUN_TEST(aggregate_no_consumers_returns_zero);
    RUN_TEST(aggregate_invalid_owner_returns_zero);
    RUN_TEST(aggregate_all_consumers_in_coverage);
    RUN_TEST(aggregate_no_consumers_in_coverage);
    RUN_TEST(aggregate_mixed_coverage);
    RUN_TEST(aggregate_consumer_wrong_player_coverage_not_counted);

    // Aggregation edge cases
    RUN_TEST(aggregate_entity_without_energy_component_skipped);
    RUN_TEST(aggregate_destroyed_entity_skipped);
    RUN_TEST(aggregate_consumer_with_zero_required);
    RUN_TEST(aggregate_after_unregister_position);

    // Multi-player
    RUN_TEST(aggregate_multi_player_isolation);

    // tick() integration
    RUN_TEST(tick_sets_pool_total_consumed);
    RUN_TEST(tick_total_consumed_only_in_coverage);
    RUN_TEST(tick_total_consumed_updates_each_tick);
    RUN_TEST(tick_total_consumed_multi_player);
    RUN_TEST(tick_no_registry_does_not_crash_consumption);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
