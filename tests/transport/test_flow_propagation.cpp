/**
 * @file test_flow_propagation.cpp
 * @brief Unit tests for FlowPropagation diffusion model (Epic 7, Ticket E7-014)
 *
 * Tests:
 * - Empty flow map (no-op)
 * - Single tile with no neighbors (no spread)
 * - Linear chain: flow spreads along connected pathway
 * - Junction: flow splits equally among neighbors
 * - Spread rate configuration
 * - Flow conservation (approximate - integer rounding)
 * - Disconnected segments don't share flow
 * - Zero flow tiles don't spread
 * - Multiple source tiles
 */

#include <sims3000/transport/FlowPropagation.h>
#include <sims3000/transport/PathwayGrid.h>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <unordered_map>

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

// Position packing (same convention as FlowDistribution)
static uint64_t pack_pos(int32_t x, int32_t y) {
    uint64_t ux = static_cast<uint64_t>(static_cast<uint32_t>(x));
    uint64_t uy = static_cast<uint64_t>(static_cast<uint32_t>(y));
    return (uy << 32) | ux;
}

// ============================================================================
// Empty flow map
// ============================================================================

TEST(empty_flow_map_noop) {
    PathwayGrid grid(16, 16);
    grid.set_pathway(5, 5, 1);

    std::unordered_map<uint64_t, uint32_t> flow_map;
    FlowPropagation prop;
    prop.propagate(flow_map, grid);

    ASSERT(flow_map.empty());
}

// ============================================================================
// Single tile with no neighbors
// ============================================================================

TEST(isolated_tile_no_spread) {
    PathwayGrid grid(16, 16);
    grid.set_pathway(5, 5, 1);  // Single isolated pathway tile

    std::unordered_map<uint64_t, uint32_t> flow_map;
    flow_map[pack_pos(5, 5)] = 1000;

    FlowPropagation prop;
    prop.propagate(flow_map, grid);

    // No neighbors to spread to - flow stays
    ASSERT_EQ(flow_map[pack_pos(5, 5)], 1000u);
}

// ============================================================================
// Linear chain: flow spreads along pathway
// ============================================================================

TEST(linear_chain_spread) {
    PathwayGrid grid(16, 16);
    // Create a horizontal line: (3,5) - (4,5) - (5,5) - (6,5) - (7,5)
    for (int x = 3; x <= 7; ++x) {
        grid.set_pathway(x, 5, 1);
    }

    // Flow only at center tile (5,5)
    std::unordered_map<uint64_t, uint32_t> flow_map;
    flow_map[pack_pos(5, 5)] = 1000;

    FlowPropagation prop;
    FlowPropagationConfig config;
    config.spread_rate = 0.20f;  // 20%
    prop.propagate(flow_map, grid, config);

    // Center has 2 neighbors (4,5) and (6,5)
    // Spread total = 1000 * 0.20 = 200
    // Per neighbor = 200 / 2 = 100
    // Center: 1000 - 200 = 800
    ASSERT_EQ(flow_map[pack_pos(5, 5)], 800u);
    ASSERT_EQ(flow_map[pack_pos(4, 5)], 100u);
    ASSERT_EQ(flow_map[pack_pos(6, 5)], 100u);
}

// ============================================================================
// Junction: flow splits equally
// ============================================================================

TEST(junction_equal_split) {
    PathwayGrid grid(16, 16);
    // Cross-shaped junction at (5,5)
    grid.set_pathway(5, 5, 1);  // Center
    grid.set_pathway(5, 4, 2);  // North
    grid.set_pathway(5, 6, 3);  // South
    grid.set_pathway(6, 5, 4);  // East
    grid.set_pathway(4, 5, 5);  // West

    std::unordered_map<uint64_t, uint32_t> flow_map;
    flow_map[pack_pos(5, 5)] = 1000;

    FlowPropagation prop;
    FlowPropagationConfig config;
    config.spread_rate = 0.20f;
    prop.propagate(flow_map, grid, config);

    // 4 neighbors: spread = 200, per neighbor = 50
    ASSERT_EQ(flow_map[pack_pos(5, 5)], 800u);
    ASSERT_EQ(flow_map[pack_pos(5, 4)], 50u);  // North
    ASSERT_EQ(flow_map[pack_pos(5, 6)], 50u);  // South
    ASSERT_EQ(flow_map[pack_pos(6, 5)], 50u);  // East
    ASSERT_EQ(flow_map[pack_pos(4, 5)], 50u);  // West
}

// ============================================================================
// Single neighbor only
// ============================================================================

TEST(single_neighbor_all_spread_to_one) {
    PathwayGrid grid(16, 16);
    grid.set_pathway(0, 0, 1);  // Corner - at most 2 neighbors if placed
    grid.set_pathway(1, 0, 2);  // Only neighbor to the east

    std::unordered_map<uint64_t, uint32_t> flow_map;
    flow_map[pack_pos(0, 0)] = 500;

    FlowPropagation prop;
    FlowPropagationConfig config;
    config.spread_rate = 0.20f;
    prop.propagate(flow_map, grid, config);

    // 1 neighbor: spread = 100, per neighbor = 100
    ASSERT_EQ(flow_map[pack_pos(0, 0)], 400u);
    ASSERT_EQ(flow_map[pack_pos(1, 0)], 100u);
}

// ============================================================================
// Spread rate configuration
// ============================================================================

TEST(custom_spread_rate) {
    PathwayGrid grid(16, 16);
    grid.set_pathway(5, 5, 1);
    grid.set_pathway(6, 5, 2);

    std::unordered_map<uint64_t, uint32_t> flow_map;
    flow_map[pack_pos(5, 5)] = 1000;

    FlowPropagation prop;
    FlowPropagationConfig config;
    config.spread_rate = 0.50f;  // 50% spread
    prop.propagate(flow_map, grid, config);

    // 1 neighbor: spread = 500
    ASSERT_EQ(flow_map[pack_pos(5, 5)], 500u);
    ASSERT_EQ(flow_map[pack_pos(6, 5)], 500u);
}

TEST(zero_spread_rate) {
    PathwayGrid grid(16, 16);
    grid.set_pathway(5, 5, 1);
    grid.set_pathway(6, 5, 2);

    std::unordered_map<uint64_t, uint32_t> flow_map;
    flow_map[pack_pos(5, 5)] = 1000;

    FlowPropagation prop;
    FlowPropagationConfig config;
    config.spread_rate = 0.0f;  // No spread
    prop.propagate(flow_map, grid, config);

    // Nothing should change
    ASSERT_EQ(flow_map[pack_pos(5, 5)], 1000u);
    ASSERT(flow_map.find(pack_pos(6, 5)) == flow_map.end());
}

// ============================================================================
// Disconnected segments don't share flow
// ============================================================================

TEST(disconnected_segments_no_sharing) {
    PathwayGrid grid(32, 32);
    // Segment A: (2,2) - (3,2)
    grid.set_pathway(2, 2, 1);
    grid.set_pathway(3, 2, 2);
    // Segment B: (20,20) - (21,20)
    grid.set_pathway(20, 20, 3);
    grid.set_pathway(21, 20, 4);

    std::unordered_map<uint64_t, uint32_t> flow_map;
    flow_map[pack_pos(2, 2)] = 1000;

    FlowPropagation prop;
    prop.propagate(flow_map, grid);

    // Flow only spreads to (3,2), not to segment B
    ASSERT(flow_map[pack_pos(2, 2)] < 1000u);
    ASSERT(flow_map[pack_pos(3, 2)] > 0u);
    ASSERT(flow_map.find(pack_pos(20, 20)) == flow_map.end());
    ASSERT(flow_map.find(pack_pos(21, 20)) == flow_map.end());
}

// ============================================================================
// Zero flow tiles don't spread
// ============================================================================

TEST(zero_flow_no_spread) {
    PathwayGrid grid(16, 16);
    grid.set_pathway(5, 5, 1);
    grid.set_pathway(6, 5, 2);

    std::unordered_map<uint64_t, uint32_t> flow_map;
    flow_map[pack_pos(5, 5)] = 0;

    FlowPropagation prop;
    prop.propagate(flow_map, grid);

    ASSERT_EQ(flow_map[pack_pos(5, 5)], 0u);
    ASSERT(flow_map.find(pack_pos(6, 5)) == flow_map.end());
}

// ============================================================================
// Multiple source tiles
// ============================================================================

TEST(multiple_sources_both_spread) {
    PathwayGrid grid(16, 16);
    // Chain: (3,5) - (4,5) - (5,5)
    grid.set_pathway(3, 5, 1);
    grid.set_pathway(4, 5, 2);
    grid.set_pathway(5, 5, 3);

    std::unordered_map<uint64_t, uint32_t> flow_map;
    flow_map[pack_pos(3, 5)] = 500;
    flow_map[pack_pos(5, 5)] = 500;

    FlowPropagation prop;
    FlowPropagationConfig config;
    config.spread_rate = 0.20f;
    prop.propagate(flow_map, grid, config);

    // (3,5) has 1 neighbor (4,5): spreads 100
    // (5,5) has 1 neighbor (4,5): spreads 100
    // Both spread to (4,5) which gets 100+100 = 200
    ASSERT_EQ(flow_map[pack_pos(3, 5)], 400u);
    ASSERT_EQ(flow_map[pack_pos(5, 5)], 400u);
    ASSERT_EQ(flow_map[pack_pos(4, 5)], 200u);
}

// ============================================================================
// Flow at non-pathway position (should be skipped)
// ============================================================================

TEST(flow_at_non_pathway_skipped) {
    PathwayGrid grid(16, 16);
    grid.set_pathway(5, 5, 1);
    grid.set_pathway(6, 5, 2);

    // Flow at (10, 10) which has no pathway
    std::unordered_map<uint64_t, uint32_t> flow_map;
    flow_map[pack_pos(10, 10)] = 500;

    FlowPropagation prop;
    prop.propagate(flow_map, grid);

    // No neighbors found -> nothing changes
    ASSERT_EQ(flow_map[pack_pos(10, 10)], 500u);
}

// ============================================================================
// Small flow amount with integer rounding
// ============================================================================

TEST(small_flow_integer_rounding) {
    PathwayGrid grid(16, 16);
    grid.set_pathway(5, 5, 1);
    grid.set_pathway(5, 4, 2);
    grid.set_pathway(5, 6, 3);
    grid.set_pathway(6, 5, 4);
    grid.set_pathway(4, 5, 5);

    // Flow = 3, spread_rate = 0.20, 4 neighbors
    // spread_total = floor(3 * 0.20) = 0 -> no spread
    std::unordered_map<uint64_t, uint32_t> flow_map;
    flow_map[pack_pos(5, 5)] = 3;

    FlowPropagation prop;
    FlowPropagationConfig config;
    config.spread_rate = 0.20f;
    prop.propagate(flow_map, grid, config);

    // Spread amount rounds to 0, so no change
    ASSERT_EQ(flow_map[pack_pos(5, 5)], 3u);
}

// ============================================================================
// Main
// ============================================================================

int main() {
    printf("=== FlowPropagation Unit Tests (Ticket E7-014) ===\n\n");

    RUN_TEST(empty_flow_map_noop);
    RUN_TEST(isolated_tile_no_spread);
    RUN_TEST(linear_chain_spread);
    RUN_TEST(junction_equal_split);
    RUN_TEST(single_neighbor_all_spread_to_one);
    RUN_TEST(custom_spread_rate);
    RUN_TEST(zero_spread_rate);
    RUN_TEST(disconnected_segments_no_sharing);
    RUN_TEST(zero_flow_no_spread);
    RUN_TEST(multiple_sources_both_spread);
    RUN_TEST(flow_at_non_pathway_skipped);
    RUN_TEST(small_flow_integer_rounding);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
