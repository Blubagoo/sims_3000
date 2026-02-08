/**
 * @file test_flow_distribution.cpp
 * @brief Unit tests for FlowDistribution (Epic 7, Ticket E7-013)
 *
 * Tests:
 * - Position packing/unpacking
 * - Single building -> single pathway distribution
 * - Multiple buildings -> single pathway
 * - Multiple buildings -> multiple pathways
 * - Building out of range (skipped)
 * - Cross-ownership flow (CCR-002)
 * - Empty sources
 * - Building on pathway tile
 * - Max distance boundary
 */

#include <sims3000/transport/FlowDistribution.h>
#include <sims3000/transport/PathwayGrid.h>
#include <sims3000/transport/ProximityCache.h>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <unordered_map>
#include <vector>

using namespace sims3000::transport;

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
        printf("\n  FAILED: %s == %s (line %d, got %lld vs %lld)\n", \
               #a, #b, __LINE__, (long long)(a), (long long)(b)); \
        tests_failed++; \
        return; \
    } \
} while(0)

// ============================================================================
// Position packing tests
// ============================================================================

TEST(pack_unpack_positive) {
    uint64_t key = FlowDistribution::pack_position(10, 20);
    ASSERT_EQ(FlowDistribution::unpack_x(key), 10);
    ASSERT_EQ(FlowDistribution::unpack_y(key), 20);
}

TEST(pack_unpack_zero) {
    uint64_t key = FlowDistribution::pack_position(0, 0);
    ASSERT_EQ(FlowDistribution::unpack_x(key), 0);
    ASSERT_EQ(FlowDistribution::unpack_y(key), 0);
}

TEST(pack_unpack_large) {
    uint64_t key = FlowDistribution::pack_position(511, 511);
    ASSERT_EQ(FlowDistribution::unpack_x(key), 511);
    ASSERT_EQ(FlowDistribution::unpack_y(key), 511);
}

TEST(pack_different_positions_differ) {
    uint64_t k1 = FlowDistribution::pack_position(5, 10);
    uint64_t k2 = FlowDistribution::pack_position(10, 5);
    ASSERT(k1 != k2);
}

// ============================================================================
// Empty sources
// ============================================================================

TEST(empty_sources_returns_zero) {
    PathwayGrid grid(16, 16);
    ProximityCache cache(16, 16);
    grid.set_pathway(8, 8, 1);
    cache.mark_dirty();
    cache.rebuild_if_dirty(grid);

    std::vector<BuildingTrafficSource> sources;
    std::unordered_map<uint64_t, uint32_t> accum;
    FlowDistribution dist;

    uint32_t count = dist.distribute_flow(sources, grid, cache, accum);
    ASSERT_EQ(count, 0u);
    ASSERT(accum.empty());
}

// ============================================================================
// Single building -> single pathway
// ============================================================================

TEST(single_building_adjacent_pathway) {
    PathwayGrid grid(16, 16);
    ProximityCache cache(16, 16);

    // Pathway at (5, 5)
    grid.set_pathway(5, 5, 1);
    cache.mark_dirty();
    cache.rebuild_if_dirty(grid);

    // Building at (6, 5) - distance 1
    std::vector<BuildingTrafficSource> sources;
    sources.push_back({6, 5, 100, 0});

    std::unordered_map<uint64_t, uint32_t> accum;
    FlowDistribution dist;

    uint32_t count = dist.distribute_flow(sources, grid, cache, accum);
    ASSERT_EQ(count, 1u);

    uint64_t key = FlowDistribution::pack_position(5, 5);
    ASSERT(accum.count(key) > 0);
    ASSERT_EQ(accum[key], 100u);
}

TEST(single_building_on_pathway) {
    PathwayGrid grid(16, 16);
    ProximityCache cache(16, 16);

    // Pathway at (5, 5)
    grid.set_pathway(5, 5, 1);
    cache.mark_dirty();
    cache.rebuild_if_dirty(grid);

    // Building at same position as pathway (distance 0)
    std::vector<BuildingTrafficSource> sources;
    sources.push_back({5, 5, 50, 0});

    std::unordered_map<uint64_t, uint32_t> accum;
    FlowDistribution dist;

    uint32_t count = dist.distribute_flow(sources, grid, cache, accum);
    ASSERT_EQ(count, 1u);

    uint64_t key = FlowDistribution::pack_position(5, 5);
    ASSERT_EQ(accum[key], 50u);
}

// ============================================================================
// Building out of range
// ============================================================================

TEST(building_out_of_range_skipped) {
    PathwayGrid grid(32, 32);
    ProximityCache cache(32, 32);

    // Pathway at (5, 5)
    grid.set_pathway(5, 5, 1);
    cache.mark_dirty();
    cache.rebuild_if_dirty(grid);

    // Building at (20, 20) - way too far (Manhattan distance 30)
    std::vector<BuildingTrafficSource> sources;
    sources.push_back({20, 20, 100, 0});

    std::unordered_map<uint64_t, uint32_t> accum;
    FlowDistribution dist;

    uint32_t count = dist.distribute_flow(sources, grid, cache, accum, 3);
    ASSERT_EQ(count, 0u);
    ASSERT(accum.empty());
}

TEST(building_at_exact_max_distance) {
    PathwayGrid grid(16, 16);
    ProximityCache cache(16, 16);

    // Pathway at (5, 5)
    grid.set_pathway(5, 5, 1);
    cache.mark_dirty();
    cache.rebuild_if_dirty(grid);

    // Building at (8, 5) - Manhattan distance 3 (exactly max_distance)
    std::vector<BuildingTrafficSource> sources;
    sources.push_back({8, 5, 75, 0});

    std::unordered_map<uint64_t, uint32_t> accum;
    FlowDistribution dist;

    uint32_t count = dist.distribute_flow(sources, grid, cache, accum, 3);
    ASSERT_EQ(count, 1u);

    uint64_t key = FlowDistribution::pack_position(5, 5);
    ASSERT_EQ(accum[key], 75u);
}

TEST(building_just_beyond_max_distance) {
    PathwayGrid grid(16, 16);
    ProximityCache cache(16, 16);

    // Pathway at (5, 5)
    grid.set_pathway(5, 5, 1);
    cache.mark_dirty();
    cache.rebuild_if_dirty(grid);

    // Building at (9, 5) - Manhattan distance 4 (beyond max_distance=3)
    std::vector<BuildingTrafficSource> sources;
    sources.push_back({9, 5, 75, 0});

    std::unordered_map<uint64_t, uint32_t> accum;
    FlowDistribution dist;

    uint32_t count = dist.distribute_flow(sources, grid, cache, accum, 3);
    ASSERT_EQ(count, 0u);
    ASSERT(accum.empty());
}

// ============================================================================
// Multiple buildings -> single pathway
// ============================================================================

TEST(multiple_buildings_same_pathway) {
    PathwayGrid grid(16, 16);
    ProximityCache cache(16, 16);

    // Pathway at (5, 5)
    grid.set_pathway(5, 5, 1);
    cache.mark_dirty();
    cache.rebuild_if_dirty(grid);

    // Two buildings near the same pathway
    std::vector<BuildingTrafficSource> sources;
    sources.push_back({6, 5, 100, 0});
    sources.push_back({4, 5, 50, 1});

    std::unordered_map<uint64_t, uint32_t> accum;
    FlowDistribution dist;

    uint32_t count = dist.distribute_flow(sources, grid, cache, accum);
    ASSERT_EQ(count, 2u);

    uint64_t key = FlowDistribution::pack_position(5, 5);
    ASSERT_EQ(accum[key], 150u);  // 100 + 50
}

// ============================================================================
// Multiple buildings -> multiple pathways
// ============================================================================

TEST(buildings_distribute_to_nearest_pathway) {
    PathwayGrid grid(16, 16);
    ProximityCache cache(16, 16);

    // Two pathways
    grid.set_pathway(2, 5, 1);   // Pathway A
    grid.set_pathway(10, 5, 2);  // Pathway B
    cache.mark_dirty();
    cache.rebuild_if_dirty(grid);

    // Building near pathway A
    std::vector<BuildingTrafficSource> sources;
    sources.push_back({3, 5, 80, 0});   // dist 1 to A, dist 7 to B
    sources.push_back({9, 5, 120, 1});  // dist 7 to A, dist 1 to B

    std::unordered_map<uint64_t, uint32_t> accum;
    FlowDistribution dist;

    uint32_t count = dist.distribute_flow(sources, grid, cache, accum, 3);
    ASSERT_EQ(count, 2u);

    uint64_t keyA = FlowDistribution::pack_position(2, 5);
    uint64_t keyB = FlowDistribution::pack_position(10, 5);

    ASSERT_EQ(accum[keyA], 80u);
    ASSERT_EQ(accum[keyB], 120u);
}

// ============================================================================
// Cross-ownership (CCR-002)
// ============================================================================

TEST(cross_ownership_flow_distribution) {
    PathwayGrid grid(16, 16);
    ProximityCache cache(16, 16);

    // Pathway owned by player 0
    grid.set_pathway(5, 5, 1);  // entity_id = 1 (owner irrelevant in grid)
    cache.mark_dirty();
    cache.rebuild_if_dirty(grid);

    // Building owned by player 1 - should still distribute to player 0's pathway
    std::vector<BuildingTrafficSource> sources;
    sources.push_back({6, 5, 200, 1});

    std::unordered_map<uint64_t, uint32_t> accum;
    FlowDistribution dist;

    uint32_t count = dist.distribute_flow(sources, grid, cache, accum);
    ASSERT_EQ(count, 1u);

    uint64_t key = FlowDistribution::pack_position(5, 5);
    ASSERT_EQ(accum[key], 200u);
}

// ============================================================================
// Mixed connected and disconnected buildings
// ============================================================================

TEST(mixed_connected_disconnected) {
    PathwayGrid grid(32, 32);
    ProximityCache cache(32, 32);

    // Pathway at (5, 5)
    grid.set_pathway(5, 5, 1);
    cache.mark_dirty();
    cache.rebuild_if_dirty(grid);

    std::vector<BuildingTrafficSource> sources;
    sources.push_back({6, 5, 100, 0});   // Connected (dist 1)
    sources.push_back({25, 25, 200, 0}); // Disconnected (dist 40)
    sources.push_back({4, 5, 50, 1});    // Connected (dist 1)

    std::unordered_map<uint64_t, uint32_t> accum;
    FlowDistribution dist;

    uint32_t count = dist.distribute_flow(sources, grid, cache, accum, 3);
    ASSERT_EQ(count, 2u);  // Only 2 connected

    uint64_t key = FlowDistribution::pack_position(5, 5);
    ASSERT_EQ(accum[key], 150u);  // 100 + 50
}

// ============================================================================
// Zero flow amount
// ============================================================================

TEST(zero_flow_still_counted) {
    PathwayGrid grid(16, 16);
    ProximityCache cache(16, 16);

    grid.set_pathway(5, 5, 1);
    cache.mark_dirty();
    cache.rebuild_if_dirty(grid);

    // Building with 0 flow - should still count as connected
    std::vector<BuildingTrafficSource> sources;
    sources.push_back({6, 5, 0, 0});

    std::unordered_map<uint64_t, uint32_t> accum;
    FlowDistribution dist;

    uint32_t count = dist.distribute_flow(sources, grid, cache, accum);
    ASSERT_EQ(count, 1u);

    // Accumulator should have the key but with 0 flow
    uint64_t key = FlowDistribution::pack_position(5, 5);
    ASSERT(accum.count(key) > 0);
    ASSERT_EQ(accum[key], 0u);
}

// ============================================================================
// Custom max_distance
// ============================================================================

TEST(custom_max_distance_1) {
    PathwayGrid grid(16, 16);
    ProximityCache cache(16, 16);

    grid.set_pathway(5, 5, 1);
    cache.mark_dirty();
    cache.rebuild_if_dirty(grid);

    // Building at distance 2 with max_distance 1 - should fail
    std::vector<BuildingTrafficSource> sources;
    sources.push_back({7, 5, 100, 0});

    std::unordered_map<uint64_t, uint32_t> accum;
    FlowDistribution dist;

    uint32_t count = dist.distribute_flow(sources, grid, cache, accum, 1);
    ASSERT_EQ(count, 0u);

    // Building at distance 1 with max_distance 1 - should succeed
    sources.clear();
    sources.push_back({6, 5, 100, 0});
    accum.clear();

    count = dist.distribute_flow(sources, grid, cache, accum, 1);
    ASSERT_EQ(count, 1u);
}

// ============================================================================
// No pathways at all
// ============================================================================

TEST(no_pathways_all_skipped) {
    PathwayGrid grid(16, 16);
    ProximityCache cache(16, 16);
    cache.rebuild_if_dirty(grid);  // No pathways

    std::vector<BuildingTrafficSource> sources;
    sources.push_back({5, 5, 100, 0});
    sources.push_back({8, 8, 200, 1});

    std::unordered_map<uint64_t, uint32_t> accum;
    FlowDistribution dist;

    uint32_t count = dist.distribute_flow(sources, grid, cache, accum);
    ASSERT_EQ(count, 0u);
    ASSERT(accum.empty());
}

// ============================================================================
// Accumulator adds to existing values
// ============================================================================

TEST(accumulator_adds_to_existing) {
    PathwayGrid grid(16, 16);
    ProximityCache cache(16, 16);

    grid.set_pathway(5, 5, 1);
    cache.mark_dirty();
    cache.rebuild_if_dirty(grid);

    std::unordered_map<uint64_t, uint32_t> accum;
    // Pre-populate accumulator
    uint64_t key = FlowDistribution::pack_position(5, 5);
    accum[key] = 1000;

    std::vector<BuildingTrafficSource> sources;
    sources.push_back({6, 5, 50, 0});

    FlowDistribution dist;
    dist.distribute_flow(sources, grid, cache, accum);

    ASSERT_EQ(accum[key], 1050u);  // 1000 + 50
}

// ============================================================================
// Main
// ============================================================================

int main() {
    printf("=== FlowDistribution Unit Tests (Ticket E7-013) ===\n\n");

    // Position packing
    RUN_TEST(pack_unpack_positive);
    RUN_TEST(pack_unpack_zero);
    RUN_TEST(pack_unpack_large);
    RUN_TEST(pack_different_positions_differ);

    // Empty sources
    RUN_TEST(empty_sources_returns_zero);

    // Single building
    RUN_TEST(single_building_adjacent_pathway);
    RUN_TEST(single_building_on_pathway);

    // Out of range
    RUN_TEST(building_out_of_range_skipped);
    RUN_TEST(building_at_exact_max_distance);
    RUN_TEST(building_just_beyond_max_distance);

    // Multiple buildings
    RUN_TEST(multiple_buildings_same_pathway);
    RUN_TEST(buildings_distribute_to_nearest_pathway);

    // Cross-ownership
    RUN_TEST(cross_ownership_flow_distribution);

    // Mixed
    RUN_TEST(mixed_connected_disconnected);

    // Zero flow
    RUN_TEST(zero_flow_still_counted);

    // Custom max distance
    RUN_TEST(custom_max_distance_1);

    // No pathways
    RUN_TEST(no_pathways_all_skipped);

    // Accumulator
    RUN_TEST(accumulator_adds_to_existing);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
