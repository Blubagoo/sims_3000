/**
 * @file test_consumer_aggregation.cpp
 * @brief Unit tests for consumer registration and requirement aggregation
 *        (Ticket 6-016)
 *
 * Tests cover:
 * - Register consumer, verify count
 * - Unregister consumer, verify count
 * - Aggregate consumption with all consumers in coverage
 * - Aggregate consumption with mixed coverage (some in, some out)
 * - Consumer outside coverage contributes 0
 * - Multiple players isolated
 *
 * Uses printf test pattern consistent with other fluid tests.
 */

#include <sims3000/fluid/FluidSystem.h>
#include <sims3000/fluid/FluidComponent.h>
#include <sims3000/fluid/FluidExtractorConfig.h>
#include <entt/entt.hpp>
#include <cstdio>
#include <cstdlib>

using namespace sims3000::fluid;

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
// 6-016: Consumer Registration Tests
// =============================================================================

TEST(register_consumer_verify_count) {
    FluidSystem sys(128, 128);
    ASSERT_EQ(sys.get_consumer_count(0), 0u);

    sys.register_consumer(100, 0);
    ASSERT_EQ(sys.get_consumer_count(0), 1u);

    sys.register_consumer(101, 0);
    ASSERT_EQ(sys.get_consumer_count(0), 2u);

    sys.register_consumer(102, 0);
    ASSERT_EQ(sys.get_consumer_count(0), 3u);
}

TEST(unregister_consumer_verify_count) {
    FluidSystem sys(128, 128);
    sys.register_consumer(100, 0);
    sys.register_consumer(101, 0);
    sys.register_consumer(102, 0);
    ASSERT_EQ(sys.get_consumer_count(0), 3u);

    sys.unregister_consumer(101, 0);
    ASSERT_EQ(sys.get_consumer_count(0), 2u);

    sys.unregister_consumer(100, 0);
    ASSERT_EQ(sys.get_consumer_count(0), 1u);

    // Unregistering non-existent entity does nothing
    sys.unregister_consumer(999, 0);
    ASSERT_EQ(sys.get_consumer_count(0), 1u);

    sys.unregister_consumer(102, 0);
    ASSERT_EQ(sys.get_consumer_count(0), 0u);
}

// =============================================================================
// 6-016: Aggregate Consumption Tests
// =============================================================================

TEST(aggregate_all_consumers_in_coverage) {
    // Place an extractor to create coverage, then place consumers in the
    // coverage area. All consumers should contribute to total_consumed.
    FluidSystem sys(32, 32);
    entt::registry registry;
    sys.set_registry(&registry);

    // Place extractor at (5,5) for player 0 - creates coverage around it
    uint32_t ext_id = sys.place_extractor(5, 5, 0);
    ASSERT(ext_id != INVALID_ENTITY_ID);

    // Create consumer entities with FluidComponent
    auto c1 = registry.create();
    uint32_t c1_id = static_cast<uint32_t>(c1);
    FluidComponent fc1{};
    fc1.fluid_required = 10;
    registry.emplace<FluidComponent>(c1, fc1);

    auto c2 = registry.create();
    uint32_t c2_id = static_cast<uint32_t>(c2);
    FluidComponent fc2{};
    fc2.fluid_required = 20;
    registry.emplace<FluidComponent>(c2, fc2);

    // Register consumers and their positions (near extractor = in coverage)
    sys.register_consumer(c1_id, 0);
    sys.register_consumer_position(c1_id, 0, 5, 6);

    sys.register_consumer(c2_id, 0);
    sys.register_consumer_position(c2_id, 0, 6, 5);

    // Tick to compute coverage (BFS from extractor) and aggregate consumption
    sys.tick(0.016f);

    const PerPlayerFluidPool& pool = sys.get_pool(0);
    // Both consumers should be in coverage (adjacent to extractor)
    ASSERT_EQ(pool.total_consumed, 30u);  // 10 + 20
    ASSERT_EQ(pool.consumer_count, 2u);
}

TEST(aggregate_mixed_coverage) {
    // Some consumers in coverage, some outside. Only in-coverage contribute.
    FluidSystem sys(32, 32);
    entt::registry registry;
    sys.set_registry(&registry);

    // Place extractor at (5,5) for player 0
    uint32_t ext_id = sys.place_extractor(5, 5, 0);
    ASSERT(ext_id != INVALID_ENTITY_ID);

    // Consumer 1: near extractor (in coverage)
    auto c1 = registry.create();
    uint32_t c1_id = static_cast<uint32_t>(c1);
    FluidComponent fc1{};
    fc1.fluid_required = 15;
    registry.emplace<FluidComponent>(c1, fc1);
    sys.register_consumer(c1_id, 0);
    sys.register_consumer_position(c1_id, 0, 5, 6);

    // Consumer 2: far from extractor (outside coverage)
    auto c2 = registry.create();
    uint32_t c2_id = static_cast<uint32_t>(c2);
    FluidComponent fc2{};
    fc2.fluid_required = 25;
    registry.emplace<FluidComponent>(c2, fc2);
    sys.register_consumer(c2_id, 0);
    sys.register_consumer_position(c2_id, 0, 30, 30);

    // Tick
    sys.tick(0.016f);

    const PerPlayerFluidPool& pool = sys.get_pool(0);
    // Only consumer 1 is in coverage (near extractor)
    // Consumer 2 at (30,30) is far from extractor at (5,5), outside coverage
    ASSERT_EQ(pool.total_consumed, 15u);
    ASSERT_EQ(pool.consumer_count, 1u);
}

TEST(consumer_outside_coverage_contributes_zero) {
    // All consumers are outside coverage area => total_consumed = 0
    FluidSystem sys(64, 64);
    entt::registry registry;
    sys.set_registry(&registry);

    // Place extractor at (5,5) for player 0
    sys.place_extractor(5, 5, 0);

    // Consumer far away from extractor (well outside coverage radius)
    auto c1 = registry.create();
    uint32_t c1_id = static_cast<uint32_t>(c1);
    FluidComponent fc1{};
    fc1.fluid_required = 50;
    registry.emplace<FluidComponent>(c1, fc1);
    sys.register_consumer(c1_id, 0);
    sys.register_consumer_position(c1_id, 0, 60, 60);

    sys.tick(0.016f);

    const PerPlayerFluidPool& pool = sys.get_pool(0);
    ASSERT_EQ(pool.total_consumed, 0u);
    ASSERT_EQ(pool.consumer_count, 0u);
}

TEST(multiple_players_isolated) {
    // Consumers for player 0 and player 1 are completely isolated.
    FluidSystem sys(64, 64);
    entt::registry registry;
    sys.set_registry(&registry);

    // Player 0: extractor at (5,5)
    sys.place_extractor(5, 5, 0);

    // Player 1: extractor at (50,50)
    sys.place_extractor(50, 50, 1);

    // Player 0 consumer near player 0 extractor
    auto c0 = registry.create();
    uint32_t c0_id = static_cast<uint32_t>(c0);
    FluidComponent fc0{};
    fc0.fluid_required = 10;
    registry.emplace<FluidComponent>(c0, fc0);
    sys.register_consumer(c0_id, 0);
    sys.register_consumer_position(c0_id, 0, 5, 6);

    // Player 1 consumer near player 1 extractor
    auto c1 = registry.create();
    uint32_t c1_id = static_cast<uint32_t>(c1);
    FluidComponent fc1{};
    fc1.fluid_required = 30;
    registry.emplace<FluidComponent>(c1, fc1);
    sys.register_consumer(c1_id, 1);
    sys.register_consumer_position(c1_id, 1, 50, 51);

    sys.tick(0.016f);

    const PerPlayerFluidPool& pool0 = sys.get_pool(0);
    const PerPlayerFluidPool& pool1 = sys.get_pool(1);

    // Player 0 sees only its consumer
    ASSERT_EQ(pool0.total_consumed, 10u);
    ASSERT_EQ(pool0.consumer_count, 1u);

    // Player 1 sees only its consumer
    ASSERT_EQ(pool1.total_consumed, 30u);
    ASSERT_EQ(pool1.consumer_count, 1u);
}

TEST(register_consumer_position_tracks_position) {
    FluidSystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    // Create a consumer entity
    auto c1 = registry.create();
    uint32_t c1_id = static_cast<uint32_t>(c1);
    FluidComponent fc1{};
    fc1.fluid_required = 5;
    registry.emplace<FluidComponent>(c1, fc1);

    sys.register_consumer(c1_id, 0);
    sys.register_consumer_position(c1_id, 0, 10, 20);

    // Verify consumer count
    ASSERT_EQ(sys.get_consumer_count(0), 1u);
}

TEST(aggregate_no_consumers_zero) {
    // No consumers registered => total_consumed should be 0
    FluidSystem sys(32, 32);
    entt::registry registry;
    sys.set_registry(&registry);

    sys.place_extractor(5, 5, 0);
    sys.tick(0.016f);

    const PerPlayerFluidPool& pool = sys.get_pool(0);
    ASSERT_EQ(pool.total_consumed, 0u);
    ASSERT_EQ(pool.consumer_count, 0u);
}

// =============================================================================
// Main
// =============================================================================

int main() {
    printf("=== Consumer Registration & Aggregation Tests (Ticket 6-016) ===\n\n");

    // Consumer registration
    RUN_TEST(register_consumer_verify_count);
    RUN_TEST(unregister_consumer_verify_count);

    // Aggregate consumption
    RUN_TEST(aggregate_all_consumers_in_coverage);
    RUN_TEST(aggregate_mixed_coverage);
    RUN_TEST(consumer_outside_coverage_contributes_zero);
    RUN_TEST(multiple_players_isolated);
    RUN_TEST(register_consumer_position_tracks_position);
    RUN_TEST(aggregate_no_consumers_zero);

    printf("\n=== Results: %d passed, %d failed ===\n",
           tests_passed, tests_failed);

    return tests_failed > 0 ? 1 : 0;
}
