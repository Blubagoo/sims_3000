/**
 * @file test_pool_calculation.cpp
 * @brief Unit tests for pool calculation (Ticket 5-012)
 *
 * Tests cover:
 * - calculate_pool() populates PerPlayerEnergyPool correctly
 * - total_generated = SUM(nexus.current_output for online nexuses)
 * - total_consumed = SUM(consumer.energy_required for consumers in coverage)
 * - surplus = total_generated - total_consumed (can be negative)
 * - nexus_count and consumer_count updated
 * - tick() phase 3 calls calculate_pool() for each overseer
 * - Scenarios: healthy, marginal, deficit, collapse
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
// Helper: create and register a nexus with given base_output for a player.
// Does NOT register a position (so BFS won't seed from it).
// Used by calculate_pool() unit tests that bypass tick().
// =============================================================================

static uint32_t create_nexus(entt::registry& reg, EnergySystem& sys,
                              uint8_t owner, uint32_t base_output,
                              bool is_online = true) {
    auto entity = reg.create();
    uint32_t eid = static_cast<uint32_t>(entity);

    EnergyProducerComponent producer{};
    producer.base_output = base_output;
    producer.current_output = 0;
    producer.efficiency = 1.0f;
    producer.age_factor = 1.0f;
    producer.nexus_type = static_cast<uint8_t>(NexusType::Carbon);
    producer.is_online = is_online;
    reg.emplace<EnergyProducerComponent>(entity, producer);

    sys.register_nexus(eid, owner);
    return eid;
}

// =============================================================================
// Helper: create and register a nexus WITH a grid position.
// This allows BFS coverage to work properly during tick().
// Carbon nexus has coverage_radius = 8 tiles.
// =============================================================================

static uint32_t create_nexus_at(entt::registry& reg, EnergySystem& sys,
                                 uint8_t owner, uint32_t base_output,
                                 uint32_t x, uint32_t y,
                                 bool is_online = true) {
    uint32_t eid = create_nexus(reg, sys, owner, base_output, is_online);
    sys.register_nexus_position(eid, owner, x, y);
    return eid;
}

// =============================================================================
// Helper: create and register a consumer at a position with given energy_required.
// Also sets manual coverage at that position (for non-tick tests).
// =============================================================================

static uint32_t create_consumer(entt::registry& reg, EnergySystem& sys,
                                 uint8_t owner, uint32_t x, uint32_t y,
                                 uint32_t energy_required) {
    auto entity = reg.create();
    uint32_t eid = static_cast<uint32_t>(entity);

    EnergyComponent ec{};
    ec.energy_required = energy_required;
    reg.emplace<EnergyComponent>(entity, ec);

    sys.register_consumer(eid, owner);
    sys.register_consumer_position(eid, owner, x, y);
    set_coverage(sys, x, y, owner);
    return eid;
}

// =============================================================================
// Helper: create and register a consumer WITHOUT setting manual coverage.
// Used in tick tests where BFS will provide coverage.
// =============================================================================

static uint32_t create_consumer_no_coverage(entt::registry& reg, EnergySystem& sys,
                                             uint8_t owner, uint32_t x, uint32_t y,
                                             uint32_t energy_required) {
    auto entity = reg.create();
    uint32_t eid = static_cast<uint32_t>(entity);

    EnergyComponent ec{};
    ec.energy_required = energy_required;
    reg.emplace<EnergyComponent>(entity, ec);

    sys.register_consumer(eid, owner);
    sys.register_consumer_position(eid, owner, x, y);
    return eid;
}

// =============================================================================
// calculate_pool basic behavior
// =============================================================================

TEST(calculate_pool_empty_player) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    sys.calculate_pool(0);

    const auto& pool = sys.get_pool(0);
    ASSERT_EQ(pool.total_generated, 0u);
    ASSERT_EQ(pool.total_consumed, 0u);
    ASSERT_EQ(pool.surplus, 0);
    ASSERT_EQ(pool.nexus_count, 0u);
    ASSERT_EQ(pool.consumer_count, 0u);
}

TEST(calculate_pool_invalid_owner_no_crash) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    // Should not crash
    sys.calculate_pool(MAX_PLAYERS);
    sys.calculate_pool(255);
}

TEST(calculate_pool_sets_total_generated) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    // Create two online Carbon nexuses (base_output 500 each)
    create_nexus(reg, sys, 0, 500, true);
    create_nexus(reg, sys, 0, 300, true);

    // Must update outputs before calculate_pool can sum them
    sys.update_all_nexus_outputs(0);
    sys.calculate_pool(0);

    // Carbon nexus: current_output = base_output * 1.0 * 1.0 = base_output
    ASSERT_EQ(sys.get_pool(0).total_generated, 800u);
}

TEST(calculate_pool_sets_total_consumed) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    create_consumer(reg, sys, 0, 5, 5, 100);
    create_consumer(reg, sys, 0, 10, 10, 200);

    sys.calculate_pool(0);

    ASSERT_EQ(sys.get_pool(0).total_consumed, 300u);
}

TEST(calculate_pool_sets_surplus) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    create_nexus(reg, sys, 0, 1000, true);
    create_consumer(reg, sys, 0, 5, 5, 300);

    sys.update_all_nexus_outputs(0);
    sys.calculate_pool(0);

    // surplus = 1000 - 300 = 700
    ASSERT_EQ(sys.get_pool(0).surplus, 700);
}

TEST(calculate_pool_sets_nexus_count) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    create_nexus(reg, sys, 0, 100, true);
    create_nexus(reg, sys, 0, 200, true);
    create_nexus(reg, sys, 0, 300, true);

    sys.calculate_pool(0);

    ASSERT_EQ(sys.get_pool(0).nexus_count, 3u);
}

TEST(calculate_pool_sets_consumer_count) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    create_consumer(reg, sys, 0, 5, 5, 100);
    create_consumer(reg, sys, 0, 10, 10, 200);

    sys.calculate_pool(0);

    ASSERT_EQ(sys.get_pool(0).consumer_count, 2u);
}

// =============================================================================
// Healthy scenario: surplus >= 0 (generation >= consumption)
// =============================================================================

TEST(healthy_scenario_generation_exceeds_consumption) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    // 2 nexuses producing 500 each = 1000 total
    create_nexus(reg, sys, 0, 500, true);
    create_nexus(reg, sys, 0, 500, true);

    // 3 consumers using 100 each = 300 total
    create_consumer(reg, sys, 0, 5, 5, 100);
    create_consumer(reg, sys, 0, 10, 10, 100);
    create_consumer(reg, sys, 0, 15, 15, 100);

    sys.update_all_nexus_outputs(0);
    sys.calculate_pool(0);

    const auto& pool = sys.get_pool(0);
    ASSERT_EQ(pool.total_generated, 1000u);
    ASSERT_EQ(pool.total_consumed, 300u);
    ASSERT_EQ(pool.surplus, 700);
    ASSERT_EQ(pool.nexus_count, 2u);
    ASSERT_EQ(pool.consumer_count, 3u);
    ASSERT(pool.surplus > 0);
}

TEST(healthy_scenario_exact_balance) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    create_nexus(reg, sys, 0, 500, true);
    create_consumer(reg, sys, 0, 5, 5, 500);

    sys.update_all_nexus_outputs(0);
    sys.calculate_pool(0);

    const auto& pool = sys.get_pool(0);
    ASSERT_EQ(pool.total_generated, 500u);
    ASSERT_EQ(pool.total_consumed, 500u);
    ASSERT_EQ(pool.surplus, 0);
}

// =============================================================================
// Marginal scenario: small surplus (close to zero)
// =============================================================================

TEST(marginal_scenario_small_surplus) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    create_nexus(reg, sys, 0, 1000, true);
    create_consumer(reg, sys, 0, 5, 5, 990);

    sys.update_all_nexus_outputs(0);
    sys.calculate_pool(0);

    const auto& pool = sys.get_pool(0);
    ASSERT_EQ(pool.total_generated, 1000u);
    ASSERT_EQ(pool.total_consumed, 990u);
    ASSERT_EQ(pool.surplus, 10);
    ASSERT(pool.surplus > 0);
}

// =============================================================================
// Deficit scenario: consumption exceeds generation (negative surplus)
// =============================================================================

TEST(deficit_scenario_negative_surplus) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    create_nexus(reg, sys, 0, 500, true);
    create_consumer(reg, sys, 0, 5, 5, 300);
    create_consumer(reg, sys, 0, 10, 10, 400);

    sys.update_all_nexus_outputs(0);
    sys.calculate_pool(0);

    const auto& pool = sys.get_pool(0);
    ASSERT_EQ(pool.total_generated, 500u);
    ASSERT_EQ(pool.total_consumed, 700u);
    ASSERT_EQ(pool.surplus, -200);
    ASSERT(pool.surplus < 0);
}

TEST(deficit_scenario_large_deficit) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    create_nexus(reg, sys, 0, 100, true);

    // Many heavy consumers
    create_consumer(reg, sys, 0, 1, 1, 1000);
    create_consumer(reg, sys, 0, 2, 2, 1000);
    create_consumer(reg, sys, 0, 3, 3, 1000);

    sys.update_all_nexus_outputs(0);
    sys.calculate_pool(0);

    const auto& pool = sys.get_pool(0);
    ASSERT_EQ(pool.total_generated, 100u);
    ASSERT_EQ(pool.total_consumed, 3000u);
    ASSERT_EQ(pool.surplus, -2900);
}

// =============================================================================
// Collapse scenario: no generation at all (all nexuses offline)
// =============================================================================

TEST(collapse_scenario_no_generation) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    // All nexuses offline
    create_nexus(reg, sys, 0, 500, false);
    create_nexus(reg, sys, 0, 500, false);

    create_consumer(reg, sys, 0, 5, 5, 300);
    create_consumer(reg, sys, 0, 10, 10, 400);

    sys.update_all_nexus_outputs(0);
    sys.calculate_pool(0);

    const auto& pool = sys.get_pool(0);
    ASSERT_EQ(pool.total_generated, 0u);
    ASSERT_EQ(pool.total_consumed, 700u);
    ASSERT_EQ(pool.surplus, -700);
    ASSERT_EQ(pool.nexus_count, 2u);
    ASSERT_EQ(pool.consumer_count, 2u);
}

TEST(collapse_scenario_zero_generation_zero_consumption) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    // No nexuses, no consumers
    sys.calculate_pool(0);

    const auto& pool = sys.get_pool(0);
    ASSERT_EQ(pool.total_generated, 0u);
    ASSERT_EQ(pool.total_consumed, 0u);
    ASSERT_EQ(pool.surplus, 0);
    ASSERT_EQ(pool.nexus_count, 0u);
    ASSERT_EQ(pool.consumer_count, 0u);
}

// =============================================================================
// Offline nexuses do not contribute to generation
// =============================================================================

TEST(offline_nexus_not_counted_in_generation) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    create_nexus(reg, sys, 0, 500, true);
    create_nexus(reg, sys, 0, 500, false);  // offline

    sys.update_all_nexus_outputs(0);
    sys.calculate_pool(0);

    const auto& pool = sys.get_pool(0);
    // Only the online nexus contributes
    ASSERT_EQ(pool.total_generated, 500u);
    // Both nexuses are registered
    ASSERT_EQ(pool.nexus_count, 2u);
}

// =============================================================================
// Consumers outside coverage are not counted
// =============================================================================

TEST(consumer_outside_coverage_not_counted) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    create_nexus(reg, sys, 0, 1000, true);

    // Create consumer manually without coverage
    auto entity = reg.create();
    uint32_t eid = static_cast<uint32_t>(entity);
    EnergyComponent ec{};
    ec.energy_required = 500;
    reg.emplace<EnergyComponent>(entity, ec);
    sys.register_consumer(eid, 0);
    sys.register_consumer_position(eid, 0, 50, 50);
    // Do NOT set coverage at (50, 50)

    // Also create a consumer IN coverage
    create_consumer(reg, sys, 0, 5, 5, 200);

    sys.update_all_nexus_outputs(0);
    sys.calculate_pool(0);

    const auto& pool = sys.get_pool(0);
    // Only the in-coverage consumer is counted
    ASSERT_EQ(pool.total_consumed, 200u);
    // Both consumers are registered
    ASSERT_EQ(pool.consumer_count, 2u);
    ASSERT_EQ(pool.surplus, 800);
}

// =============================================================================
// tick() integration: calculate_pool is called for each player
//
// These tests use create_nexus_at() to register nexus positions so that
// BFS coverage works during tick(). Consumers are placed within the
// Carbon nexus coverage_radius of 8 tiles.
//
// Note: tick() ages nexuses (Ticket 5-022) before computing outputs,
// so total_generated will be slightly less than base_output after 1+ ticks.
// Tests verify the surplus formula rather than exact base_output values.
// =============================================================================

TEST(tick_calls_calculate_pool_for_all_players) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    // Player 0: nexus at (10,10), consumer at (12,10) - within radius 8
    create_nexus_at(reg, sys, 0, 500, 10, 10, true);
    create_consumer_no_coverage(reg, sys, 0, 12, 10, 200);

    // Player 1: nexus at (40,40), consumers at (42,40) and (40,42) - within radius 8
    create_nexus_at(reg, sys, 1, 1000, 40, 40, true);
    create_consumer_no_coverage(reg, sys, 1, 42, 40, 300);
    create_consumer_no_coverage(reg, sys, 1, 40, 42, 400);

    sys.tick(0.05f);

    // Player 0 pool
    const auto& pool0 = sys.get_pool(0);
    ASSERT(pool0.total_generated > 0u);
    ASSERT(pool0.total_generated <= 500u);
    ASSERT_EQ(pool0.total_consumed, 200u);
    ASSERT_EQ(pool0.surplus,
              static_cast<int32_t>(pool0.total_generated) -
              static_cast<int32_t>(pool0.total_consumed));
    ASSERT(pool0.surplus > 0);
    ASSERT_EQ(pool0.nexus_count, 1u);
    ASSERT_EQ(pool0.consumer_count, 1u);

    // Player 1 pool
    const auto& pool1 = sys.get_pool(1);
    ASSERT(pool1.total_generated > 0u);
    ASSERT(pool1.total_generated <= 1000u);
    ASSERT_EQ(pool1.total_consumed, 700u);
    ASSERT_EQ(pool1.surplus,
              static_cast<int32_t>(pool1.total_generated) -
              static_cast<int32_t>(pool1.total_consumed));
    ASSERT_EQ(pool1.nexus_count, 1u);
    ASSERT_EQ(pool1.consumer_count, 2u);

    // Player 2 (empty)
    const auto& pool2 = sys.get_pool(2);
    ASSERT_EQ(pool2.total_generated, 0u);
    ASSERT_EQ(pool2.total_consumed, 0u);
    ASSERT_EQ(pool2.surplus, 0);
    ASSERT_EQ(pool2.nexus_count, 0u);
    ASSERT_EQ(pool2.consumer_count, 0u);
}

TEST(tick_updates_pool_each_tick) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    // Nexus at (10,10), consumer at (12,10) - within radius 8
    create_nexus_at(reg, sys, 0, 1000, 10, 10, true);
    uint32_t consumer_eid = create_consumer_no_coverage(reg, sys, 0, 12, 10, 200);

    sys.tick(0.05f);

    // Verify surplus = total_generated - total_consumed and it's positive
    const auto& pool = sys.get_pool(0);
    ASSERT_EQ(pool.surplus,
              static_cast<int32_t>(pool.total_generated) -
              static_cast<int32_t>(pool.total_consumed));
    ASSERT(pool.surplus > 0);
    ASSERT_EQ(pool.total_consumed, 200u);

    // Increase consumption close to generation
    auto consumer_entity = static_cast<entt::entity>(consumer_eid);
    auto* ec = reg.try_get<EnergyComponent>(consumer_entity);
    ec->energy_required = 900;

    sys.tick(0.05f);
    // After aging, total_generated < 1000, so surplus < 100
    // But should still be positive (aging is minimal after 2 ticks)
    ASSERT(sys.get_pool(0).surplus > 0);
    ASSERT_EQ(sys.get_pool(0).total_consumed, 900u);

    // Push into deficit: consumption >> generation
    ec->energy_required = 1500;
    sys.tick(0.05f);
    ASSERT(sys.get_pool(0).surplus < 0);
    ASSERT_EQ(sys.get_pool(0).total_consumed, 1500u);
}

TEST(tick_no_registry_does_not_crash) {
    EnergySystem sys(64, 64);
    // No registry set
    sys.register_nexus(42, 0);
    sys.register_consumer(43, 0);

    sys.tick(0.05f);

    // Pool should be zeroed (no registry -> 0 generation, 0 consumption)
    const auto& pool = sys.get_pool(0);
    ASSERT_EQ(pool.total_generated, 0u);
    ASSERT_EQ(pool.total_consumed, 0u);
    ASSERT_EQ(pool.surplus, 0);
    // nexus_count and consumer_count still reflect registered entities
    ASSERT_EQ(pool.nexus_count, 1u);
    ASSERT_EQ(pool.consumer_count, 1u);
}

// =============================================================================
// Multi-player isolation (via tick)
// =============================================================================

TEST(multi_player_pool_isolation) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    // Player 0: surplus (1000 base gen, 200 consumption)
    // Nexus at (10,10), consumer at (12,10) - within radius 8
    create_nexus_at(reg, sys, 0, 1000, 10, 10, true);
    create_consumer_no_coverage(reg, sys, 0, 12, 10, 200);

    // Player 1: deficit (200 base gen, 800 consumption)
    // Nexus at (40,40), consumer at (42,40) - within radius 8
    create_nexus_at(reg, sys, 1, 200, 40, 40, true);
    create_consumer_no_coverage(reg, sys, 1, 42, 40, 800);

    // Player 2: no activity

    // Player 3: generation only (5000 base gen, 0 consumption)
    // Nexus at (10,50)
    create_nexus_at(reg, sys, 3, 5000, 10, 50, true);

    sys.tick(0.05f);

    // Player 0: positive surplus (gen ~1000 > consumption 200)
    ASSERT(sys.get_pool(0).surplus > 0);
    ASSERT_EQ(sys.get_pool(0).total_consumed, 200u);
    ASSERT_EQ(sys.get_pool(0).surplus,
              static_cast<int32_t>(sys.get_pool(0).total_generated) -
              static_cast<int32_t>(sys.get_pool(0).total_consumed));

    // Player 1: negative surplus (gen ~200 < consumption 800)
    ASSERT(sys.get_pool(1).surplus < 0);
    ASSERT_EQ(sys.get_pool(1).total_consumed, 800u);

    // Player 2: surplus = 0 - 0 = 0
    ASSERT_EQ(sys.get_pool(2).surplus, 0);

    // Player 3: positive surplus (gen ~5000, consumption 0)
    ASSERT(sys.get_pool(3).surplus > 0);
    ASSERT_EQ(sys.get_pool(3).total_consumed, 0u);
    ASSERT_EQ(sys.get_pool(3).surplus,
              static_cast<int32_t>(sys.get_pool(3).total_generated));
}

// =============================================================================
// Main Entry Point
// =============================================================================

int main() {
    printf("=== Pool Calculation Unit Tests (Ticket 5-012) ===\n\n");

    // Basic calculate_pool behavior
    RUN_TEST(calculate_pool_empty_player);
    RUN_TEST(calculate_pool_invalid_owner_no_crash);
    RUN_TEST(calculate_pool_sets_total_generated);
    RUN_TEST(calculate_pool_sets_total_consumed);
    RUN_TEST(calculate_pool_sets_surplus);
    RUN_TEST(calculate_pool_sets_nexus_count);
    RUN_TEST(calculate_pool_sets_consumer_count);

    // Healthy scenario
    RUN_TEST(healthy_scenario_generation_exceeds_consumption);
    RUN_TEST(healthy_scenario_exact_balance);

    // Marginal scenario
    RUN_TEST(marginal_scenario_small_surplus);

    // Deficit scenario
    RUN_TEST(deficit_scenario_negative_surplus);
    RUN_TEST(deficit_scenario_large_deficit);

    // Collapse scenario
    RUN_TEST(collapse_scenario_no_generation);
    RUN_TEST(collapse_scenario_zero_generation_zero_consumption);

    // Edge cases
    RUN_TEST(offline_nexus_not_counted_in_generation);
    RUN_TEST(consumer_outside_coverage_not_counted);

    // tick() integration
    RUN_TEST(tick_calls_calculate_pool_for_all_players);
    RUN_TEST(tick_updates_pool_each_tick);
    RUN_TEST(tick_no_registry_does_not_crash);

    // Multi-player isolation
    RUN_TEST(multi_player_pool_isolation);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
