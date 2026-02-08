/**
 * @file test_transport_provider_impl.cpp
 * @brief Unit tests for TransportProviderImpl (Epic 7, Tickets E7-017/E7-018)
 *
 * Tests cover:
 * - E7-017: is_road_accessible_at via ProximityCache (O(1), 3-tile default)
 * - E7-018: get_nearest_road_distance (0 for pathway, 255 for no pathway, correct Manhattan)
 * - Extended methods: is_connected_to_network, are_connected, get_network_id_at
 * - Stub methods: get_congestion_at (0.0f), get_traffic_volume_at (0)
 * - Null pointer safety (no data sources configured)
 * - ITransportProvider polymorphism
 */

#include <sims3000/transport/TransportProviderImpl.h>
#include <sims3000/transport/PathwayGrid.h>
#include <sims3000/transport/ProximityCache.h>
#include <sims3000/transport/NetworkGraph.h>
#include <sims3000/building/ForwardDependencyInterfaces.h>
#include <cstdio>
#include <cstdlib>

using namespace sims3000::transport;
using namespace sims3000::building;

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
        printf("\n  FAILED: %s == %s (line %d, got %d vs %d)\n", #a, #b, __LINE__, (int)(a), (int)(b)); \
        tests_failed++; \
        return; \
    } \
} while(0)

#define ASSERT_FLOAT_EQ(a, b) do { \
    float diff = (a) - (b); \
    if (diff < -0.001f || diff > 0.001f) { \
        printf("\n  FAILED: %s == %s (line %d)\n", #a, #b, __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

// ============================================================================
// Helper: set up a fully configured TransportProviderImpl
// ============================================================================

struct TestFixture {
    PathwayGrid grid;
    ProximityCache cache;
    NetworkGraph graph;
    TransportProviderImpl provider;

    TestFixture(uint32_t w, uint32_t h)
        : grid(w, h), cache(w, h)
    {
        provider.set_proximity_cache(&cache);
        provider.set_pathway_grid(&grid);
        provider.set_network_graph(&graph);
    }

    void rebuild() {
        cache.mark_dirty();
        cache.rebuild_if_dirty(grid);
        graph.rebuild_from_grid(grid);
    }
};

// ============================================================================
// E7-017: is_road_accessible_at tests
// ============================================================================

TEST(accessible_on_pathway_tile) {
    TestFixture f(16, 16);
    f.grid.set_pathway(5, 5, 1);
    f.rebuild();

    // On the pathway tile itself, distance = 0 <= 3
    ASSERT(f.provider.is_road_accessible_at(5, 5, 3));
}

TEST(accessible_within_3_tiles) {
    TestFixture f(16, 16);
    f.grid.set_pathway(5, 5, 1);
    f.rebuild();

    // Distance 1
    ASSERT(f.provider.is_road_accessible_at(6, 5, 3));
    ASSERT(f.provider.is_road_accessible_at(4, 5, 3));
    ASSERT(f.provider.is_road_accessible_at(5, 6, 3));
    ASSERT(f.provider.is_road_accessible_at(5, 4, 3));

    // Distance 2
    ASSERT(f.provider.is_road_accessible_at(7, 5, 3));
    ASSERT(f.provider.is_road_accessible_at(6, 6, 3));

    // Distance 3
    ASSERT(f.provider.is_road_accessible_at(8, 5, 3));
    ASSERT(f.provider.is_road_accessible_at(7, 6, 3));
}

TEST(not_accessible_beyond_3_tiles) {
    TestFixture f(16, 16);
    f.grid.set_pathway(5, 5, 1);
    f.rebuild();

    // Distance 4
    ASSERT(!f.provider.is_road_accessible_at(9, 5, 3));
    // Distance 5
    ASSERT(!f.provider.is_road_accessible_at(10, 5, 3));
}

TEST(accessible_custom_max_distance) {
    TestFixture f(16, 16);
    f.grid.set_pathway(5, 5, 1);
    f.rebuild();

    // max_distance = 1: only adjacent
    ASSERT(f.provider.is_road_accessible_at(5, 5, 1));
    ASSERT(f.provider.is_road_accessible_at(6, 5, 1));
    ASSERT(!f.provider.is_road_accessible_at(7, 5, 1));

    // max_distance = 0: only the tile itself
    ASSERT(f.provider.is_road_accessible_at(5, 5, 0));
    ASSERT(!f.provider.is_road_accessible_at(6, 5, 0));
}

TEST(not_accessible_no_pathways) {
    TestFixture f(16, 16);
    f.rebuild();

    // No pathways: distance is 255 everywhere, so max_distance < 255 fails
    ASSERT(!f.provider.is_road_accessible_at(5, 5, 3));
    ASSERT(!f.provider.is_road_accessible_at(0, 0, 254));
    // Note: max_distance=255 would pass since 255 <= 255, but that's an edge case;
    // in practice max_distance is always small (e.g. 3 for building spawn rule)
}

// ============================================================================
// E7-018: get_nearest_road_distance tests
// ============================================================================

TEST(distance_zero_on_pathway) {
    TestFixture f(16, 16);
    f.grid.set_pathway(5, 5, 1);
    f.rebuild();

    ASSERT_EQ(f.provider.get_nearest_road_distance(5, 5), 0u);
}

TEST(distance_correct_manhattan) {
    TestFixture f(16, 16);
    f.grid.set_pathway(5, 5, 1);
    f.rebuild();

    ASSERT_EQ(f.provider.get_nearest_road_distance(6, 5), 1u);
    ASSERT_EQ(f.provider.get_nearest_road_distance(7, 5), 2u);
    ASSERT_EQ(f.provider.get_nearest_road_distance(8, 5), 3u);
    ASSERT_EQ(f.provider.get_nearest_road_distance(6, 6), 2u);  // diagonal = Manhattan 2
    ASSERT_EQ(f.provider.get_nearest_road_distance(5, 10), 5u);
}

TEST(distance_255_no_pathway) {
    TestFixture f(16, 16);
    f.rebuild();  // No pathways placed

    ASSERT_EQ(f.provider.get_nearest_road_distance(5, 5), 255u);
    ASSERT_EQ(f.provider.get_nearest_road_distance(0, 0), 255u);
}

TEST(distance_255_far_away) {
    // 512x1 grid, pathway at (0,0)
    TestFixture f(512, 1);
    f.grid.set_pathway(0, 0, 1);
    f.rebuild();

    ASSERT_EQ(f.provider.get_nearest_road_distance(0, 0), 0u);
    ASSERT_EQ(f.provider.get_nearest_road_distance(254, 0), 254u);
    ASSERT_EQ(f.provider.get_nearest_road_distance(255, 0), 255u);
    ASSERT_EQ(f.provider.get_nearest_road_distance(400, 0), 255u);
}

TEST(distance_multi_source) {
    TestFixture f(16, 16);
    f.grid.set_pathway(2, 2, 1);
    f.grid.set_pathway(12, 12, 2);
    f.rebuild();

    ASSERT_EQ(f.provider.get_nearest_road_distance(2, 2), 0u);
    ASSERT_EQ(f.provider.get_nearest_road_distance(12, 12), 0u);
    // (7,7): dist from (2,2) = 10, dist from (12,12) = 10 -> min = 10
    ASSERT_EQ(f.provider.get_nearest_road_distance(7, 7), 10u);
}

// ============================================================================
// Extended methods: connectivity
// ============================================================================

TEST(is_connected_to_network_with_pathway) {
    TestFixture f(16, 16);
    f.grid.set_pathway(5, 5, 1);
    f.grid.set_pathway(6, 5, 2);
    f.rebuild();

    ASSERT(f.provider.is_connected_to_network(5, 5));
    ASSERT(f.provider.is_connected_to_network(6, 5));
}

TEST(is_connected_to_network_without_pathway) {
    TestFixture f(16, 16);
    f.grid.set_pathway(5, 5, 1);
    f.rebuild();

    ASSERT(!f.provider.is_connected_to_network(6, 5));
    ASSERT(!f.provider.is_connected_to_network(0, 0));
}

TEST(are_connected_same_network) {
    TestFixture f(16, 16);
    f.grid.set_pathway(2, 2, 1);
    f.grid.set_pathway(3, 2, 2);
    f.grid.set_pathway(4, 2, 3);
    f.rebuild();

    ASSERT(f.provider.are_connected(2, 2, 4, 2));
    ASSERT(f.provider.are_connected(2, 2, 3, 2));
}

TEST(are_connected_different_networks) {
    TestFixture f(16, 16);
    // Network A
    f.grid.set_pathway(0, 0, 1);
    f.grid.set_pathway(1, 0, 2);
    // Network B (not adjacent)
    f.grid.set_pathway(10, 10, 3);
    f.grid.set_pathway(11, 10, 4);
    f.rebuild();

    ASSERT(!f.provider.are_connected(0, 0, 10, 10));
    ASSERT(!f.provider.are_connected(1, 0, 11, 10));
}

TEST(are_connected_no_pathway) {
    TestFixture f(16, 16);
    f.grid.set_pathway(5, 5, 1);
    f.rebuild();

    // One has pathway, other doesn't
    ASSERT(!f.provider.are_connected(5, 5, 6, 5));
    // Neither has pathway
    ASSERT(!f.provider.are_connected(0, 0, 1, 1));
}

TEST(get_network_id_at_with_pathway) {
    TestFixture f(16, 16);
    f.grid.set_pathway(5, 5, 1);
    f.grid.set_pathway(6, 5, 2);
    f.rebuild();

    uint16_t id = f.provider.get_network_id_at(5, 5);
    ASSERT(id != 0);
    ASSERT_EQ(f.provider.get_network_id_at(6, 5), id);
}

TEST(get_network_id_at_without_pathway) {
    TestFixture f(16, 16);
    f.rebuild();

    ASSERT_EQ(f.provider.get_network_id_at(5, 5), 0);
}

// ============================================================================
// Stub methods
// ============================================================================

TEST(congestion_returns_zero) {
    TestFixture f(16, 16);
    f.grid.set_pathway(5, 5, 1);
    f.rebuild();

    ASSERT_FLOAT_EQ(f.provider.get_congestion_at(5, 5), 0.0f);
    ASSERT_FLOAT_EQ(f.provider.get_congestion_at(0, 0), 0.0f);
}

TEST(traffic_volume_returns_zero) {
    TestFixture f(16, 16);
    f.grid.set_pathway(5, 5, 1);
    f.rebuild();

    ASSERT_EQ(f.provider.get_traffic_volume_at(5, 5), 0u);
    ASSERT_EQ(f.provider.get_traffic_volume_at(0, 0), 0u);
}

TEST(is_road_accessible_entity_returns_true) {
    TestFixture f(16, 16);

    // Stub: always returns true regardless of entity
    ASSERT(f.provider.is_road_accessible(0));
    ASSERT(f.provider.is_road_accessible(12345));
}

// ============================================================================
// Null pointer safety
// ============================================================================

TEST(null_cache_returns_defaults) {
    TransportProviderImpl provider;
    // No data sources configured

    // No cache = permissive (E7-019: allows graceful transition from stub)
    ASSERT(provider.is_road_accessible_at(5, 5, 3));
    ASSERT_EQ(provider.get_nearest_road_distance(5, 5), 255u);
}

TEST(null_grid_graph_returns_defaults) {
    TransportProviderImpl provider;
    // No data sources configured

    ASSERT(!provider.is_connected_to_network(5, 5));
    ASSERT(!provider.are_connected(0, 0, 1, 0));
    ASSERT_EQ(provider.get_network_id_at(5, 5), 0);
}

// ============================================================================
// Polymorphism: ITransportProvider interface
// ============================================================================

TEST(polymorphic_usage) {
    TestFixture f(16, 16);
    f.grid.set_pathway(5, 5, 1);
    f.rebuild();

    // Use through ITransportProvider pointer
    ITransportProvider* iface = &f.provider;

    ASSERT(iface->is_road_accessible_at(5, 5, 3));
    ASSERT_EQ(iface->get_nearest_road_distance(5, 5), 0u);
    ASSERT(iface->is_connected_to_network(5, 5));
    ASSERT_EQ(iface->get_network_id_at(5, 5), f.provider.get_network_id_at(5, 5));
}

// ============================================================================
// Main
// ============================================================================

int main() {
    printf("=== TransportProviderImpl Unit Tests (Tickets E7-017/E7-018) ===\n\n");

    // E7-017: is_road_accessible_at
    RUN_TEST(accessible_on_pathway_tile);
    RUN_TEST(accessible_within_3_tiles);
    RUN_TEST(not_accessible_beyond_3_tiles);
    RUN_TEST(accessible_custom_max_distance);
    RUN_TEST(not_accessible_no_pathways);

    // E7-018: get_nearest_road_distance
    RUN_TEST(distance_zero_on_pathway);
    RUN_TEST(distance_correct_manhattan);
    RUN_TEST(distance_255_no_pathway);
    RUN_TEST(distance_255_far_away);
    RUN_TEST(distance_multi_source);

    // Extended methods: connectivity
    RUN_TEST(is_connected_to_network_with_pathway);
    RUN_TEST(is_connected_to_network_without_pathway);
    RUN_TEST(are_connected_same_network);
    RUN_TEST(are_connected_different_networks);
    RUN_TEST(are_connected_no_pathway);
    RUN_TEST(get_network_id_at_with_pathway);
    RUN_TEST(get_network_id_at_without_pathway);

    // Stub methods
    RUN_TEST(congestion_returns_zero);
    RUN_TEST(traffic_volume_returns_zero);
    RUN_TEST(is_road_accessible_entity_returns_true);

    // Null pointer safety
    RUN_TEST(null_cache_returns_defaults);
    RUN_TEST(null_grid_graph_returns_defaults);

    // Polymorphism
    RUN_TEST(polymorphic_usage);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
