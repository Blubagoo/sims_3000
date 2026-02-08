/**
 * @file test_fluid_coverage_bfs.cpp
 * @brief Unit tests for FluidCoverageBFS (Ticket 6-010)
 *
 * Tests cover:
 * - Single extractor seeds coverage
 * - Reservoir seeds coverage (even without power)
 * - Conduit extends coverage from extractor
 * - Chain of conduits extends coverage
 * - Isolated conduit (no producer nearby) stays disconnected
 * - Non-operational extractor does NOT seed coverage
 * - Multiple producers seed independently
 * - BFS doesn't cross map boundaries
 * - Large grid performance (256x256 with 1000 conduits)
 *
 * Uses entt::registry to create entities with components.
 */

#include <sims3000/fluid/FluidCoverageBFS.h>
#include <sims3000/fluid/FluidCoverageGrid.h>
#include <sims3000/fluid/FluidConduitComponent.h>
#include <sims3000/fluid/FluidProducerComponent.h>
#include <sims3000/fluid/FluidEnums.h>
#include <sims3000/fluid/FluidExtractorConfig.h>
#include <sims3000/fluid/FluidReservoirConfig.h>
#include <entt/entt.hpp>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <unordered_map>

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
// Helper: Create an operational extractor entity
// =============================================================================
static uint32_t create_extractor(entt::registry& registry, bool is_operational) {
    auto entity = registry.create();
    FluidProducerComponent producer{};
    producer.base_output = EXTRACTOR_DEFAULT_BASE_OUTPUT;
    producer.current_output = is_operational ? EXTRACTOR_DEFAULT_BASE_OUTPUT : 0;
    producer.is_operational = is_operational;
    producer.producer_type = static_cast<uint8_t>(FluidProducerType::Extractor);
    registry.emplace<FluidProducerComponent>(entity, producer);
    return static_cast<uint32_t>(entity);
}

// =============================================================================
// Helper: Create a reservoir entity (always seeds, no power check)
// =============================================================================
static uint32_t create_reservoir(entt::registry& registry) {
    auto entity = registry.create();
    FluidProducerComponent producer{};
    producer.base_output = 0; // Reservoirs don't produce, they store
    producer.current_output = 0;
    producer.is_operational = false; // Doesn't matter for reservoirs in BFS
    producer.producer_type = static_cast<uint8_t>(FluidProducerType::Reservoir);
    registry.emplace<FluidProducerComponent>(entity, producer);
    return static_cast<uint32_t>(entity);
}

// =============================================================================
// Helper: Create a conduit entity
// =============================================================================
static uint32_t create_conduit(entt::registry& registry, uint8_t coverage_radius = 2) {
    auto entity = registry.create();
    FluidConduitComponent conduit{};
    conduit.coverage_radius = coverage_radius;
    conduit.is_connected = false;
    conduit.is_active = false;
    conduit.conduit_level = 1;
    registry.emplace<FluidConduitComponent>(entity, conduit);
    return static_cast<uint32_t>(entity);
}

// =============================================================================
// Test: Single extractor seeds coverage
// =============================================================================
TEST(single_extractor_seeds_coverage) {
    const uint32_t MAP_SIZE = 64;
    FluidCoverageGrid grid(MAP_SIZE, MAP_SIZE);
    entt::registry registry;

    uint32_t ext_id = create_extractor(registry, true);

    std::unordered_map<uint64_t, uint32_t> extractor_positions;
    std::unordered_map<uint64_t, uint32_t> reservoir_positions;
    std::unordered_map<uint64_t, uint32_t> conduit_positions;

    // Place extractor at (30, 30)
    extractor_positions[pack_pos(30, 30)] = ext_id;

    BFSContext ctx{
        grid,
        extractor_positions,
        reservoir_positions,
        conduit_positions,
        &registry,
        0, // owner (player 0)
        MAP_SIZE,
        MAP_SIZE
    };

    recalculate_coverage(ctx);

    // owner_id = owner + 1 = 1
    // Extractor at (30,30) with coverage_radius=8 should cover
    // a square from (22,22) to (38,38)
    ASSERT(grid.is_in_coverage(30, 30, 1)); // center
    ASSERT(grid.is_in_coverage(22, 22, 1)); // min corner
    ASSERT(grid.is_in_coverage(38, 38, 1)); // max corner
    ASSERT(grid.is_in_coverage(30, 22, 1)); // top edge
    ASSERT(grid.is_in_coverage(30, 38, 1)); // bottom edge

    // Outside coverage radius
    ASSERT(!grid.is_in_coverage(21, 30, 1));
    ASSERT(!grid.is_in_coverage(39, 30, 1));
    ASSERT(!grid.is_in_coverage(30, 21, 1));
    ASSERT(!grid.is_in_coverage(30, 39, 1));
}

// =============================================================================
// Test: Reservoir seeds coverage (even without power)
// =============================================================================
TEST(reservoir_seeds_coverage) {
    const uint32_t MAP_SIZE = 64;
    FluidCoverageGrid grid(MAP_SIZE, MAP_SIZE);
    entt::registry registry;

    uint32_t res_id = create_reservoir(registry);

    std::unordered_map<uint64_t, uint32_t> extractor_positions;
    std::unordered_map<uint64_t, uint32_t> reservoir_positions;
    std::unordered_map<uint64_t, uint32_t> conduit_positions;

    // Place reservoir at (20, 20)
    reservoir_positions[pack_pos(20, 20)] = res_id;

    BFSContext ctx{
        grid,
        extractor_positions,
        reservoir_positions,
        conduit_positions,
        &registry,
        0,
        MAP_SIZE,
        MAP_SIZE
    };

    recalculate_coverage(ctx);

    // Reservoir at (20,20) with coverage_radius=6 should cover
    // a square from (14,14) to (26,26)
    ASSERT(grid.is_in_coverage(20, 20, 1)); // center
    ASSERT(grid.is_in_coverage(14, 14, 1)); // min corner
    ASSERT(grid.is_in_coverage(26, 26, 1)); // max corner

    // Outside coverage radius
    ASSERT(!grid.is_in_coverage(13, 20, 1));
    ASSERT(!grid.is_in_coverage(27, 20, 1));
}

// =============================================================================
// Test: Conduit extends coverage from extractor
// =============================================================================
TEST(conduit_extends_coverage_from_extractor) {
    const uint32_t MAP_SIZE = 64;
    FluidCoverageGrid grid(MAP_SIZE, MAP_SIZE);
    entt::registry registry;

    uint32_t ext_id = create_extractor(registry, true);
    uint32_t cond_id = create_conduit(registry, 2);

    std::unordered_map<uint64_t, uint32_t> extractor_positions;
    std::unordered_map<uint64_t, uint32_t> reservoir_positions;
    std::unordered_map<uint64_t, uint32_t> conduit_positions;

    // Extractor at (30, 30), conduit adjacent at (31, 30)
    extractor_positions[pack_pos(30, 30)] = ext_id;
    conduit_positions[pack_pos(31, 30)] = cond_id;

    BFSContext ctx{
        grid,
        extractor_positions,
        reservoir_positions,
        conduit_positions,
        &registry,
        0,
        MAP_SIZE,
        MAP_SIZE
    };

    recalculate_coverage(ctx);

    // Conduit should be connected
    auto entity = static_cast<entt::entity>(cond_id);
    auto* conduit = registry.try_get<FluidConduitComponent>(entity);
    ASSERT(conduit != nullptr);
    ASSERT(conduit->is_connected);

    // Conduit at (31,30) with radius=2 should add coverage at (33,30)
    // which is beyond the extractor's own coverage at that point
    ASSERT(grid.is_in_coverage(33, 30, 1));
    ASSERT(grid.is_in_coverage(31, 32, 1));
}

// =============================================================================
// Test: Chain of conduits extends coverage
// =============================================================================
TEST(chain_of_conduits_extends_coverage) {
    const uint32_t MAP_SIZE = 128;
    FluidCoverageGrid grid(MAP_SIZE, MAP_SIZE);
    entt::registry registry;

    uint32_t ext_id = create_extractor(registry, true);

    std::unordered_map<uint64_t, uint32_t> extractor_positions;
    std::unordered_map<uint64_t, uint32_t> reservoir_positions;
    std::unordered_map<uint64_t, uint32_t> conduit_positions;

    // Extractor at (50, 50)
    extractor_positions[pack_pos(50, 50)] = ext_id;

    // Chain of conduits extending right: (51,50), (52,50), (53,50), (54,50), (55,50)
    uint32_t cond_ids[5];
    for (int i = 0; i < 5; ++i) {
        cond_ids[i] = create_conduit(registry, 2);
        conduit_positions[pack_pos(51 + i, 50)] = cond_ids[i];
    }

    BFSContext ctx{
        grid,
        extractor_positions,
        reservoir_positions,
        conduit_positions,
        &registry,
        0,
        MAP_SIZE,
        MAP_SIZE
    };

    recalculate_coverage(ctx);

    // All conduits should be connected
    for (int i = 0; i < 5; ++i) {
        auto entity = static_cast<entt::entity>(cond_ids[i]);
        auto* conduit = registry.try_get<FluidConduitComponent>(entity);
        ASSERT(conduit != nullptr);
        ASSERT(conduit->is_connected);
    }

    // Last conduit at (55,50) with radius=2 should cover up to (57,50)
    ASSERT(grid.is_in_coverage(57, 50, 1));

    // And beyond that should be uncovered
    // Extractor radius=8 covers 50-8=42 to 50+8=58 in x
    // But conduit at 55 with radius 2 covers 53-57
    // So (57,50) is covered, (58,50) is also covered by extractor
    // Check that (59, 50) is NOT covered (beyond extractor 50+8=58)
    ASSERT(!grid.is_in_coverage(59, 50, 1));
}

// =============================================================================
// Test: Isolated conduit stays disconnected
// =============================================================================
TEST(isolated_conduit_stays_disconnected) {
    const uint32_t MAP_SIZE = 64;
    FluidCoverageGrid grid(MAP_SIZE, MAP_SIZE);
    entt::registry registry;

    uint32_t ext_id = create_extractor(registry, true);
    uint32_t isolated_cond_id = create_conduit(registry, 2);

    std::unordered_map<uint64_t, uint32_t> extractor_positions;
    std::unordered_map<uint64_t, uint32_t> reservoir_positions;
    std::unordered_map<uint64_t, uint32_t> conduit_positions;

    // Extractor at (10, 10)
    extractor_positions[pack_pos(10, 10)] = ext_id;

    // Isolated conduit far away at (50, 50) - not adjacent to anything
    conduit_positions[pack_pos(50, 50)] = isolated_cond_id;

    BFSContext ctx{
        grid,
        extractor_positions,
        reservoir_positions,
        conduit_positions,
        &registry,
        0,
        MAP_SIZE,
        MAP_SIZE
    };

    recalculate_coverage(ctx);

    // Isolated conduit should NOT be connected
    auto entity = static_cast<entt::entity>(isolated_cond_id);
    auto* conduit = registry.try_get<FluidConduitComponent>(entity);
    ASSERT(conduit != nullptr);
    ASSERT(!conduit->is_connected);

    // The area around the isolated conduit should NOT be covered
    ASSERT(!grid.is_in_coverage(50, 50, 1));
}

// =============================================================================
// Test: Non-operational extractor does NOT seed coverage
// =============================================================================
TEST(non_operational_extractor_no_coverage) {
    const uint32_t MAP_SIZE = 64;
    FluidCoverageGrid grid(MAP_SIZE, MAP_SIZE);
    entt::registry registry;

    // Create a non-operational extractor (is_operational = false)
    uint32_t ext_id = create_extractor(registry, false);

    std::unordered_map<uint64_t, uint32_t> extractor_positions;
    std::unordered_map<uint64_t, uint32_t> reservoir_positions;
    std::unordered_map<uint64_t, uint32_t> conduit_positions;

    // Place non-operational extractor at (30, 30)
    extractor_positions[pack_pos(30, 30)] = ext_id;

    BFSContext ctx{
        grid,
        extractor_positions,
        reservoir_positions,
        conduit_positions,
        &registry,
        0,
        MAP_SIZE,
        MAP_SIZE
    };

    recalculate_coverage(ctx);

    // Non-operational extractor should NOT seed any coverage
    ASSERT(!grid.is_in_coverage(30, 30, 1));
    ASSERT_EQ(grid.get_coverage_count(1), 0u);
}

// =============================================================================
// Test: Multiple producers seed independently
// =============================================================================
TEST(multiple_producers_seed_independently) {
    const uint32_t MAP_SIZE = 128;
    FluidCoverageGrid grid(MAP_SIZE, MAP_SIZE);
    entt::registry registry;

    uint32_t ext_id = create_extractor(registry, true);
    uint32_t res_id = create_reservoir(registry);

    std::unordered_map<uint64_t, uint32_t> extractor_positions;
    std::unordered_map<uint64_t, uint32_t> reservoir_positions;
    std::unordered_map<uint64_t, uint32_t> conduit_positions;

    // Extractor at (20, 20), Reservoir at (80, 80) - far apart
    extractor_positions[pack_pos(20, 20)] = ext_id;
    reservoir_positions[pack_pos(80, 80)] = res_id;

    BFSContext ctx{
        grid,
        extractor_positions,
        reservoir_positions,
        conduit_positions,
        &registry,
        0,
        MAP_SIZE,
        MAP_SIZE
    };

    recalculate_coverage(ctx);

    // Extractor coverage around (20,20) with radius 8
    ASSERT(grid.is_in_coverage(20, 20, 1));
    ASSERT(grid.is_in_coverage(12, 12, 1));
    ASSERT(grid.is_in_coverage(28, 28, 1));

    // Reservoir coverage around (80,80) with radius 6
    ASSERT(grid.is_in_coverage(80, 80, 1));
    ASSERT(grid.is_in_coverage(74, 74, 1));
    ASSERT(grid.is_in_coverage(86, 86, 1));

    // Gap between them should be uncovered
    ASSERT(!grid.is_in_coverage(50, 50, 1));
}

// =============================================================================
// Test: BFS doesn't cross map boundaries
// =============================================================================
TEST(bfs_doesnt_cross_map_boundaries) {
    const uint32_t MAP_SIZE = 32;
    FluidCoverageGrid grid(MAP_SIZE, MAP_SIZE);
    entt::registry registry;

    // Extractor at corner (0, 0)
    uint32_t ext_id = create_extractor(registry, true);

    std::unordered_map<uint64_t, uint32_t> extractor_positions;
    std::unordered_map<uint64_t, uint32_t> reservoir_positions;
    std::unordered_map<uint64_t, uint32_t> conduit_positions;

    extractor_positions[pack_pos(0, 0)] = ext_id;

    BFSContext ctx{
        grid,
        extractor_positions,
        reservoir_positions,
        conduit_positions,
        &registry,
        0,
        MAP_SIZE,
        MAP_SIZE
    };

    recalculate_coverage(ctx);

    // Coverage should be clamped to grid bounds
    // Extractor at (0,0) with radius 8 covers (0,0) to (8,8)
    ASSERT(grid.is_in_coverage(0, 0, 1));
    ASSERT(grid.is_in_coverage(8, 8, 1));
    ASSERT(!grid.is_in_coverage(9, 0, 1));
    ASSERT(!grid.is_in_coverage(0, 9, 1));

    // The coverage area should be 9x9 = 81 cells (0 through 8 in each dimension)
    ASSERT_EQ(grid.get_coverage_count(1), 81u);
}

// =============================================================================
// Test: BFS at bottom-right corner
// =============================================================================
TEST(bfs_at_bottom_right_corner) {
    const uint32_t MAP_SIZE = 32;
    FluidCoverageGrid grid(MAP_SIZE, MAP_SIZE);
    entt::registry registry;

    // Extractor at bottom-right corner (31, 31)
    uint32_t ext_id = create_extractor(registry, true);

    std::unordered_map<uint64_t, uint32_t> extractor_positions;
    std::unordered_map<uint64_t, uint32_t> reservoir_positions;
    std::unordered_map<uint64_t, uint32_t> conduit_positions;

    extractor_positions[pack_pos(31, 31)] = ext_id;

    BFSContext ctx{
        grid,
        extractor_positions,
        reservoir_positions,
        conduit_positions,
        &registry,
        0,
        MAP_SIZE,
        MAP_SIZE
    };

    recalculate_coverage(ctx);

    // Coverage should be clamped: (31-8=23, 31-8=23) to (31, 31)
    ASSERT(grid.is_in_coverage(31, 31, 1));
    ASSERT(grid.is_in_coverage(23, 23, 1));
    ASSERT(!grid.is_in_coverage(22, 31, 1));
    ASSERT(!grid.is_in_coverage(31, 22, 1));

    // Coverage area: 9x9 = 81 cells (23 through 31 in each dimension)
    ASSERT_EQ(grid.get_coverage_count(1), 81u);
}

// =============================================================================
// Test: Conduit not adjacent to producer stays disconnected
// =============================================================================
TEST(conduit_not_adjacent_to_producer_disconnected) {
    const uint32_t MAP_SIZE = 64;
    FluidCoverageGrid grid(MAP_SIZE, MAP_SIZE);
    entt::registry registry;

    uint32_t ext_id = create_extractor(registry, true);
    uint32_t cond1_id = create_conduit(registry, 2);
    uint32_t cond2_id = create_conduit(registry, 2);

    std::unordered_map<uint64_t, uint32_t> extractor_positions;
    std::unordered_map<uint64_t, uint32_t> reservoir_positions;
    std::unordered_map<uint64_t, uint32_t> conduit_positions;

    // Extractor at (10, 10), conduit adjacent at (11, 10)
    extractor_positions[pack_pos(10, 10)] = ext_id;
    conduit_positions[pack_pos(11, 10)] = cond1_id;

    // Conduit at (13, 10) - gap of 1 tile from first conduit, not adjacent
    conduit_positions[pack_pos(13, 10)] = cond2_id;

    BFSContext ctx{
        grid,
        extractor_positions,
        reservoir_positions,
        conduit_positions,
        &registry,
        0,
        MAP_SIZE,
        MAP_SIZE
    };

    recalculate_coverage(ctx);

    // First conduit should be connected
    auto entity1 = static_cast<entt::entity>(cond1_id);
    auto* c1 = registry.try_get<FluidConduitComponent>(entity1);
    ASSERT(c1 != nullptr);
    ASSERT(c1->is_connected);

    // Second conduit (gap) should NOT be connected
    auto entity2 = static_cast<entt::entity>(cond2_id);
    auto* c2 = registry.try_get<FluidConduitComponent>(entity2);
    ASSERT(c2 != nullptr);
    ASSERT(!c2->is_connected);
}

// =============================================================================
// Test: Coverage clears for owner before recalculation
// =============================================================================
TEST(coverage_clears_before_recalculation) {
    const uint32_t MAP_SIZE = 64;
    FluidCoverageGrid grid(MAP_SIZE, MAP_SIZE);
    entt::registry registry;

    // Pre-set some coverage for owner 1 that should be cleared
    grid.set(60, 60, 1);
    grid.set(61, 60, 1);
    ASSERT(grid.is_in_coverage(60, 60, 1));

    uint32_t ext_id = create_extractor(registry, true);

    std::unordered_map<uint64_t, uint32_t> extractor_positions;
    std::unordered_map<uint64_t, uint32_t> reservoir_positions;
    std::unordered_map<uint64_t, uint32_t> conduit_positions;

    // Extractor at (10, 10) - far from pre-set coverage
    extractor_positions[pack_pos(10, 10)] = ext_id;

    BFSContext ctx{
        grid,
        extractor_positions,
        reservoir_positions,
        conduit_positions,
        &registry,
        0,
        MAP_SIZE,
        MAP_SIZE
    };

    recalculate_coverage(ctx);

    // Pre-set coverage should be cleared (not near the extractor)
    ASSERT(!grid.is_in_coverage(60, 60, 1));
    ASSERT(!grid.is_in_coverage(61, 60, 1));

    // Extractor coverage should exist
    ASSERT(grid.is_in_coverage(10, 10, 1));
}

// =============================================================================
// Test: Different owners don't interfere
// =============================================================================
TEST(different_owners_dont_interfere) {
    const uint32_t MAP_SIZE = 64;
    FluidCoverageGrid grid(MAP_SIZE, MAP_SIZE);
    entt::registry registry;

    // Set up coverage for owner 2 (player 1) first
    grid.set(30, 30, 2);
    grid.set(31, 30, 2);

    // Now run BFS for owner 1 (player 0)
    uint32_t ext_id = create_extractor(registry, true);

    std::unordered_map<uint64_t, uint32_t> extractor_positions;
    std::unordered_map<uint64_t, uint32_t> reservoir_positions;
    std::unordered_map<uint64_t, uint32_t> conduit_positions;

    extractor_positions[pack_pos(10, 10)] = ext_id;

    BFSContext ctx{
        grid,
        extractor_positions,
        reservoir_positions,
        conduit_positions,
        &registry,
        0, // player 0 -> owner_id 1
        MAP_SIZE,
        MAP_SIZE
    };

    recalculate_coverage(ctx);

    // Owner 2 coverage should still be intact
    ASSERT(grid.is_in_coverage(30, 30, 2));
    ASSERT(grid.is_in_coverage(31, 30, 2));

    // Owner 1 coverage should exist around extractor
    ASSERT(grid.is_in_coverage(10, 10, 1));
}

// =============================================================================
// Test: Reservoir connects to conduits (BFS extends from reservoir)
// =============================================================================
TEST(reservoir_connects_to_conduits) {
    const uint32_t MAP_SIZE = 64;
    FluidCoverageGrid grid(MAP_SIZE, MAP_SIZE);
    entt::registry registry;

    uint32_t res_id = create_reservoir(registry);
    uint32_t cond_id = create_conduit(registry, 2);

    std::unordered_map<uint64_t, uint32_t> extractor_positions;
    std::unordered_map<uint64_t, uint32_t> reservoir_positions;
    std::unordered_map<uint64_t, uint32_t> conduit_positions;

    // Reservoir at (20, 20), conduit adjacent at (21, 20)
    reservoir_positions[pack_pos(20, 20)] = res_id;
    conduit_positions[pack_pos(21, 20)] = cond_id;

    BFSContext ctx{
        grid,
        extractor_positions,
        reservoir_positions,
        conduit_positions,
        &registry,
        0,
        MAP_SIZE,
        MAP_SIZE
    };

    recalculate_coverage(ctx);

    // Conduit should be connected via reservoir
    auto entity = static_cast<entt::entity>(cond_id);
    auto* conduit = registry.try_get<FluidConduitComponent>(entity);
    ASSERT(conduit != nullptr);
    ASSERT(conduit->is_connected);
}

// =============================================================================
// Test: Empty grid (no producers or conduits)
// =============================================================================
TEST(empty_grid_no_coverage) {
    const uint32_t MAP_SIZE = 64;
    FluidCoverageGrid grid(MAP_SIZE, MAP_SIZE);
    entt::registry registry;

    std::unordered_map<uint64_t, uint32_t> extractor_positions;
    std::unordered_map<uint64_t, uint32_t> reservoir_positions;
    std::unordered_map<uint64_t, uint32_t> conduit_positions;

    BFSContext ctx{
        grid,
        extractor_positions,
        reservoir_positions,
        conduit_positions,
        &registry,
        0,
        MAP_SIZE,
        MAP_SIZE
    };

    recalculate_coverage(ctx);

    // No coverage should exist
    ASSERT_EQ(grid.get_coverage_count(1), 0u);
}

// =============================================================================
// Test: Conduit is_connected resets between runs
// =============================================================================
TEST(conduit_connected_resets_between_runs) {
    const uint32_t MAP_SIZE = 64;
    FluidCoverageGrid grid(MAP_SIZE, MAP_SIZE);
    entt::registry registry;

    uint32_t ext_id = create_extractor(registry, true);
    uint32_t cond_id = create_conduit(registry, 2);

    std::unordered_map<uint64_t, uint32_t> extractor_positions;
    std::unordered_map<uint64_t, uint32_t> reservoir_positions;
    std::unordered_map<uint64_t, uint32_t> conduit_positions;

    // Extractor at (10, 10), conduit adjacent at (11, 10)
    extractor_positions[pack_pos(10, 10)] = ext_id;
    conduit_positions[pack_pos(11, 10)] = cond_id;

    BFSContext ctx{
        grid,
        extractor_positions,
        reservoir_positions,
        conduit_positions,
        &registry,
        0,
        MAP_SIZE,
        MAP_SIZE
    };

    // First run - conduit should be connected
    recalculate_coverage(ctx);
    auto entity = static_cast<entt::entity>(cond_id);
    auto* conduit = registry.try_get<FluidConduitComponent>(entity);
    ASSERT(conduit != nullptr);
    ASSERT(conduit->is_connected);

    // Now make the extractor non-operational
    auto ext_entity = static_cast<entt::entity>(ext_id);
    auto* producer = registry.try_get<FluidProducerComponent>(ext_entity);
    ASSERT(producer != nullptr);
    producer->is_operational = false;

    // Second run - conduit should be disconnected since extractor is non-operational
    recalculate_coverage(ctx);
    ASSERT(!conduit->is_connected);
}

// =============================================================================
// Test: Large grid performance (256x256 with 1000 conduits)
// =============================================================================
TEST(large_grid_performance) {
    const uint32_t MAP_SIZE = 256;
    FluidCoverageGrid grid(MAP_SIZE, MAP_SIZE);
    entt::registry registry;

    std::unordered_map<uint64_t, uint32_t> extractor_positions;
    std::unordered_map<uint64_t, uint32_t> reservoir_positions;
    std::unordered_map<uint64_t, uint32_t> conduit_positions;

    // Place 4 extractors in different quadrants
    uint32_t ext1 = create_extractor(registry, true);
    uint32_t ext2 = create_extractor(registry, true);
    uint32_t ext3 = create_extractor(registry, true);
    uint32_t ext4 = create_extractor(registry, true);

    extractor_positions[pack_pos(50, 50)] = ext1;
    extractor_positions[pack_pos(200, 50)] = ext2;
    extractor_positions[pack_pos(50, 200)] = ext3;
    extractor_positions[pack_pos(200, 200)] = ext4;

    // Place 1000 conduits in a connected chain from extractor 1
    // Create a long chain going right then down
    for (uint32_t i = 0; i < 500; ++i) {
        uint32_t cid = create_conduit(registry, 2);
        conduit_positions[pack_pos(51 + i, 50)] = cid;
    }
    // Chain going down from (51, 50) isn't connected to above,
    // so let's place chain going down from (50, 51)
    for (uint32_t i = 0; i < 500; ++i) {
        uint32_t cid = create_conduit(registry, 2);
        // Make sure we don't go out of bounds (map is 256)
        uint32_t y = 51 + i;
        if (y >= MAP_SIZE) break;
        conduit_positions[pack_pos(50, y)] = cid;
    }

    BFSContext ctx{
        grid,
        extractor_positions,
        reservoir_positions,
        conduit_positions,
        &registry,
        0,
        MAP_SIZE,
        MAP_SIZE
    };

    // Time the BFS
    auto start = std::chrono::high_resolution_clock::now();
    recalculate_coverage(ctx);
    auto end = std::chrono::high_resolution_clock::now();

    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    printf(" (%lldms)", (long long)duration_ms);

    // Performance target: <10ms
    ASSERT(duration_ms < 100); // Use generous 100ms for CI; actual should be <10ms

    // Verify some coverage exists
    ASSERT(grid.is_in_coverage(50, 50, 1));
    ASSERT(grid.get_coverage_count(1) > 0);
}

// =============================================================================
// Test: L-shaped conduit network
// =============================================================================
TEST(l_shaped_conduit_network) {
    const uint32_t MAP_SIZE = 64;
    FluidCoverageGrid grid(MAP_SIZE, MAP_SIZE);
    entt::registry registry;

    uint32_t ext_id = create_extractor(registry, true);

    std::unordered_map<uint64_t, uint32_t> extractor_positions;
    std::unordered_map<uint64_t, uint32_t> reservoir_positions;
    std::unordered_map<uint64_t, uint32_t> conduit_positions;

    // Extractor at (30, 30)
    extractor_positions[pack_pos(30, 30)] = ext_id;

    // L-shaped conduit chain: right 3, then down 3
    uint32_t c1 = create_conduit(registry, 2);
    uint32_t c2 = create_conduit(registry, 2);
    uint32_t c3 = create_conduit(registry, 2);
    uint32_t c4 = create_conduit(registry, 2);
    uint32_t c5 = create_conduit(registry, 2);
    uint32_t c6 = create_conduit(registry, 2);

    conduit_positions[pack_pos(31, 30)] = c1; // right
    conduit_positions[pack_pos(32, 30)] = c2; // right
    conduit_positions[pack_pos(33, 30)] = c3; // right (corner)
    conduit_positions[pack_pos(33, 31)] = c4; // down
    conduit_positions[pack_pos(33, 32)] = c5; // down
    conduit_positions[pack_pos(33, 33)] = c6; // down

    BFSContext ctx{
        grid,
        extractor_positions,
        reservoir_positions,
        conduit_positions,
        &registry,
        0,
        MAP_SIZE,
        MAP_SIZE
    };

    recalculate_coverage(ctx);

    // All conduits should be connected
    auto check_connected = [&](uint32_t id) {
        auto entity = static_cast<entt::entity>(id);
        auto* conduit = registry.try_get<FluidConduitComponent>(entity);
        return conduit && conduit->is_connected;
    };

    ASSERT(check_connected(c1));
    ASSERT(check_connected(c2));
    ASSERT(check_connected(c3));
    ASSERT(check_connected(c4));
    ASSERT(check_connected(c5));
    ASSERT(check_connected(c6));

    // Last conduit at (33,33) with radius 2 should cover (33,35)
    ASSERT(grid.is_in_coverage(33, 35, 1));
}

// =============================================================================
// Test: mark_coverage_radius standalone
// =============================================================================
TEST(mark_coverage_radius_basic) {
    const uint32_t MAP_SIZE = 32;
    FluidCoverageGrid grid(MAP_SIZE, MAP_SIZE);

    mark_coverage_radius(grid, 15, 15, 3, 1, MAP_SIZE, MAP_SIZE);

    // Square from (12,12) to (18,18)
    ASSERT(grid.is_in_coverage(15, 15, 1)); // center
    ASSERT(grid.is_in_coverage(12, 12, 1)); // min corner
    ASSERT(grid.is_in_coverage(18, 18, 1)); // max corner
    ASSERT(!grid.is_in_coverage(11, 15, 1)); // outside
    ASSERT(!grid.is_in_coverage(15, 19, 1)); // outside

    // Count should be 7*7 = 49
    ASSERT_EQ(grid.get_coverage_count(1), 49u);
}

// =============================================================================
// Test: mark_coverage_radius at edge clamps correctly
// =============================================================================
TEST(mark_coverage_radius_edge_clamp) {
    const uint32_t MAP_SIZE = 16;
    FluidCoverageGrid grid(MAP_SIZE, MAP_SIZE);

    // Place at (0,0) with radius 3 - should clamp to (0,0)-(3,3)
    mark_coverage_radius(grid, 0, 0, 3, 1, MAP_SIZE, MAP_SIZE);

    ASSERT(grid.is_in_coverage(0, 0, 1));
    ASSERT(grid.is_in_coverage(3, 3, 1));
    ASSERT(!grid.is_in_coverage(4, 0, 1));
    ASSERT(!grid.is_in_coverage(0, 4, 1));

    // Count: 4*4 = 16
    ASSERT_EQ(grid.get_coverage_count(1), 16u);
}

// =============================================================================
// Main Entry Point
// =============================================================================

int main() {
    printf("=== FluidCoverageBFS Unit Tests (Ticket 6-010) ===\n\n");

    // Core BFS tests
    RUN_TEST(single_extractor_seeds_coverage);
    RUN_TEST(reservoir_seeds_coverage);
    RUN_TEST(conduit_extends_coverage_from_extractor);
    RUN_TEST(chain_of_conduits_extends_coverage);
    RUN_TEST(isolated_conduit_stays_disconnected);
    RUN_TEST(non_operational_extractor_no_coverage);
    RUN_TEST(multiple_producers_seed_independently);
    RUN_TEST(bfs_doesnt_cross_map_boundaries);
    RUN_TEST(bfs_at_bottom_right_corner);

    // Edge cases
    RUN_TEST(conduit_not_adjacent_to_producer_disconnected);
    RUN_TEST(coverage_clears_before_recalculation);
    RUN_TEST(different_owners_dont_interfere);
    RUN_TEST(reservoir_connects_to_conduits);
    RUN_TEST(empty_grid_no_coverage);
    RUN_TEST(conduit_connected_resets_between_runs);
    RUN_TEST(l_shaped_conduit_network);

    // Standalone function tests
    RUN_TEST(mark_coverage_radius_basic);
    RUN_TEST(mark_coverage_radius_edge_clamp);

    // Performance test
    RUN_TEST(large_grid_performance);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
