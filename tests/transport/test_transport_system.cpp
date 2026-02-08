/**
 * @file test_transport_system.cpp
 * @brief Tests for TransportSystem orchestrator (Epic 7, Ticket E7-022)
 *
 * Tests:
 * - Construction and initialization
 * - Priority 45
 * - Pathway placement and removal
 * - ITransportProvider delegation
 * - Tick phases (rebuild, flow, congestion, decay)
 * - Event emission
 * - Grace period
 */

#include <sims3000/transport/TransportSystem.h>
#include <cassert>
#include <cstdio>
#include <cmath>

using namespace sims3000::transport;

// =============================================================================
// Helper
// =============================================================================

static int tests_passed = 0;
static int tests_total = 0;

#define TEST(name) \
    do { \
        tests_total++; \
        printf("  TEST: %s ... ", name); \
    } while(0)

#define PASS() \
    do { \
        tests_passed++; \
        printf("PASS\n"); \
    } while(0)

// =============================================================================
// Tests
// =============================================================================

static void test_construction() {
    TEST("Construction with map dimensions");
    TransportSystem sys(64, 64);
    assert(sys.get_pathway_count() == 0);
    assert(sys.get_priority() == 45);
    PASS();
}

static void test_priority() {
    TEST("Priority is 45");
    TransportSystem sys(32, 32);
    assert(sys.get_priority() == TransportSystem::TICK_PRIORITY);
    assert(TransportSystem::TICK_PRIORITY == 45);
    PASS();
}

static void test_place_pathway() {
    TEST("Place pathway creates entity");
    TransportSystem sys(32, 32);
    uint32_t id = sys.place_pathway(5, 5, PathwayType::BasicPathway, 0);
    assert(id != 0);
    assert(sys.get_pathway_count() == 1);
    assert(sys.has_pathway_at(5, 5));
    PASS();
}

static void test_place_pathway_out_of_bounds() {
    TEST("Place pathway out of bounds returns 0");
    TransportSystem sys(32, 32);
    uint32_t id = sys.place_pathway(100, 100, PathwayType::BasicPathway, 0);
    assert(id == 0);
    assert(sys.get_pathway_count() == 0);
    PASS();
}

static void test_place_pathway_occupied() {
    TEST("Place pathway on occupied cell returns 0");
    TransportSystem sys(32, 32);
    uint32_t id1 = sys.place_pathway(5, 5, PathwayType::BasicPathway, 0);
    assert(id1 != 0);
    uint32_t id2 = sys.place_pathway(5, 5, PathwayType::BasicPathway, 0);
    assert(id2 == 0);
    assert(sys.get_pathway_count() == 1);
    PASS();
}

static void test_place_pathway_invalid_owner() {
    TEST("Place pathway with invalid owner returns 0");
    TransportSystem sys(32, 32);
    uint32_t id = sys.place_pathway(5, 5, PathwayType::BasicPathway, 5);
    assert(id == 0);
    PASS();
}

static void test_remove_pathway() {
    TEST("Remove pathway clears grid");
    TransportSystem sys(32, 32);
    uint32_t id = sys.place_pathway(5, 5, PathwayType::BasicPathway, 0);
    assert(id != 0);
    bool removed = sys.remove_pathway(id, 5, 5, 0);
    assert(removed);
    assert(sys.get_pathway_count() == 0);
    assert(!sys.has_pathway_at(5, 5));
    PASS();
}

static void test_remove_pathway_wrong_owner() {
    TEST("Remove pathway with wrong owner fails");
    TransportSystem sys(32, 32);
    uint32_t id = sys.place_pathway(5, 5, PathwayType::BasicPathway, 0);
    bool removed = sys.remove_pathway(id, 5, 5, 1);
    assert(!removed);
    assert(sys.get_pathway_count() == 1);
    PASS();
}

static void test_remove_pathway_wrong_position() {
    TEST("Remove pathway with wrong position fails");
    TransportSystem sys(32, 32);
    uint32_t id = sys.place_pathway(5, 5, PathwayType::BasicPathway, 0);
    bool removed = sys.remove_pathway(id, 6, 6, 0);
    assert(!removed);
    assert(sys.get_pathway_count() == 1);
    PASS();
}

static void test_remove_nonexistent() {
    TEST("Remove nonexistent entity fails");
    TransportSystem sys(32, 32);
    bool removed = sys.remove_pathway(999, 5, 5, 0);
    assert(!removed);
    PASS();
}

static void test_tick_rebuilds_network() {
    TEST("Tick rebuilds network when dirty");
    TransportSystem sys(32, 32);

    // Place two adjacent pathways
    sys.place_pathway(5, 5, PathwayType::BasicPathway, 0);
    sys.place_pathway(5, 6, PathwayType::BasicPathway, 0);

    // Before tick, network not yet rebuilt
    // After tick, should be rebuilt
    sys.tick(0.05f);

    // Both should be on the same network
    assert(sys.is_connected_to_network(5, 5));
    assert(sys.is_connected_to_network(5, 6));
    assert(sys.are_connected(5, 5, 5, 6));
    PASS();
}

static void test_tick_rebuilds_proximity() {
    TEST("Tick rebuilds proximity cache");
    TransportSystem sys(32, 32);

    sys.place_pathway(10, 10, PathwayType::BasicPathway, 0);
    sys.tick(0.05f);

    // Position at (10,10) should be on a pathway (distance 0)
    assert(sys.get_nearest_road_distance(10, 10) == 0);

    // Position at (10,11) should be 1 tile away
    assert(sys.get_nearest_road_distance(10, 11) == 1);

    // Position at (10,13) should be 3 tiles away
    assert(sys.get_nearest_road_distance(10, 13) == 3);
    PASS();
}

static void test_is_road_accessible_at() {
    TEST("is_road_accessible_at via proximity cache");
    TransportSystem sys(32, 32);

    sys.place_pathway(10, 10, PathwayType::BasicPathway, 0);
    sys.tick(0.05f);

    // Within 3 tiles
    assert(sys.is_road_accessible_at(10, 10, 3));
    assert(sys.is_road_accessible_at(10, 12, 3));

    // Exactly 3 tiles away
    assert(sys.is_road_accessible_at(10, 13, 3));

    // 4 tiles away - not accessible with max_distance 3
    assert(!sys.is_road_accessible_at(10, 14, 3));
    PASS();
}

static void test_disconnected_networks() {
    TEST("Disconnected networks are not connected");
    TransportSystem sys(32, 32);

    sys.place_pathway(0, 0, PathwayType::BasicPathway, 0);
    sys.place_pathway(20, 20, PathwayType::BasicPathway, 1);
    sys.tick(0.05f);

    assert(!sys.are_connected(0, 0, 20, 20));
    assert(sys.is_connected_to_network(0, 0));
    assert(sys.is_connected_to_network(20, 20));
    PASS();
}

static void test_network_id_at() {
    TEST("get_network_id_at returns correct IDs");
    TransportSystem sys(32, 32);

    sys.place_pathway(0, 0, PathwayType::BasicPathway, 0);
    sys.place_pathway(20, 20, PathwayType::BasicPathway, 1);
    sys.tick(0.05f);

    uint16_t id1 = sys.get_network_id_at(0, 0);
    uint16_t id2 = sys.get_network_id_at(20, 20);
    uint16_t id3 = sys.get_network_id_at(15, 15);

    assert(id1 != 0);
    assert(id2 != 0);
    assert(id1 != id2);
    assert(id3 == 0);  // No pathway at (15,15)
    PASS();
}

static void test_placed_events() {
    TEST("Placed events emitted on placement");
    TransportSystem sys(32, 32);

    sys.place_pathway(5, 5, PathwayType::BasicPathway, 0);
    sys.place_pathway(6, 6, PathwayType::TransitCorridor, 1);

    const auto& events = sys.get_placed_events();
    assert(events.size() == 2);
    assert(events[0].x == 5 && events[0].y == 5);
    assert(events[0].type == PathwayType::BasicPathway);
    assert(events[0].owner == 0);
    assert(events[1].x == 6 && events[1].y == 6);
    assert(events[1].type == PathwayType::TransitCorridor);
    assert(events[1].owner == 1);
    PASS();
}

static void test_removed_events() {
    TEST("Removed events emitted on removal");
    TransportSystem sys(32, 32);

    uint32_t id = sys.place_pathway(5, 5, PathwayType::BasicPathway, 0);
    // Clear events from placement by ticking
    sys.tick(0.05f);

    sys.remove_pathway(id, 5, 5, 0);

    const auto& events = sys.get_removed_events();
    assert(events.size() == 1);
    assert(events[0].entity_id == id);
    assert(events[0].x == 5 && events[0].y == 5);
    assert(events[0].owner == 0);
    PASS();
}

static void test_events_cleared_on_tick() {
    TEST("Events cleared at tick start");
    TransportSystem sys(32, 32);

    sys.place_pathway(5, 5, PathwayType::BasicPathway, 0);
    assert(sys.get_placed_events().size() == 1);

    sys.tick(0.05f);

    // Events should be cleared at the start of tick
    assert(sys.get_placed_events().empty());
    assert(sys.get_removed_events().empty());
    PASS();
}

static void test_grace_period() {
    TEST("Grace period allows all access");
    TransportSystem sys(32, 32);

    // No pathways placed - normally should not be accessible
    sys.tick(0.05f);

    // Activate grace period
    sys.activate_grace_period(1);

    // During grace period, should be accessible
    assert(sys.is_road_accessible_at(15, 15, 3));
    PASS();
}

static void test_congestion_at() {
    TEST("get_congestion_at returns 0 for empty pathway");
    TransportSystem sys(32, 32);

    sys.place_pathway(5, 5, PathwayType::BasicPathway, 0);
    sys.tick(0.05f);

    // No traffic, congestion should be 0
    float congestion = sys.get_congestion_at(5, 5);
    assert(congestion >= 0.0f && congestion <= 0.01f);
    PASS();
}

static void test_traffic_volume_at() {
    TEST("get_traffic_volume_at returns 0 for empty pathway");
    TransportSystem sys(32, 32);

    sys.place_pathway(5, 5, PathwayType::BasicPathway, 0);
    sys.tick(0.05f);

    uint32_t volume = sys.get_traffic_volume_at(5, 5);
    assert(volume == 0);
    PASS();
}

static void test_congestion_no_pathway() {
    TEST("Congestion/volume at empty cell returns 0");
    TransportSystem sys(32, 32);
    sys.tick(0.05f);

    assert(sys.get_congestion_at(5, 5) == 0.0f);
    assert(sys.get_traffic_volume_at(5, 5) == 0);
    PASS();
}

static void test_multiple_ticks() {
    TEST("Multiple ticks execute without crash");
    TransportSystem sys(64, 64);

    // Place a network of pathways
    for (int i = 0; i < 10; ++i) {
        sys.place_pathway(10 + i, 10, PathwayType::BasicPathway, 0);
    }

    // Run many ticks
    for (int i = 0; i < 200; ++i) {
        sys.tick(0.05f);
    }

    assert(sys.get_pathway_count() == 10);
    PASS();
}

static void test_decay_runs_periodically() {
    TEST("Decay runs every 100 ticks");
    TransportSystem sys(32, 32);

    sys.place_pathway(5, 5, PathwayType::BasicPathway, 0);
    sys.tick(0.05f);

    // Run 99 ticks - decay should NOT have run yet (plus 1 from above = 100)
    for (int i = 0; i < 99; ++i) {
        sys.tick(0.05f);
    }

    // After 100 ticks total, decay should have run
    // The pathway should still be mostly healthy (decay is very slow)
    assert(sys.get_pathway_count() == 1);
    PASS();
}

static void test_unique_entity_ids() {
    TEST("Entity IDs are unique and increasing");
    TransportSystem sys(32, 32);

    uint32_t id1 = sys.place_pathway(0, 0, PathwayType::BasicPathway, 0);
    uint32_t id2 = sys.place_pathway(1, 0, PathwayType::BasicPathway, 0);
    uint32_t id3 = sys.place_pathway(2, 0, PathwayType::BasicPathway, 0);

    assert(id1 != 0);
    assert(id2 != 0);
    assert(id3 != 0);
    assert(id1 != id2);
    assert(id2 != id3);
    assert(id1 < id2);
    assert(id2 < id3);
    PASS();
}

static void test_pathway_grid_accessor() {
    TEST("get_pathway_grid returns correct grid");
    TransportSystem sys(64, 64);

    sys.place_pathway(10, 10, PathwayType::BasicPathway, 0);

    const PathwayGrid& grid = sys.get_pathway_grid();
    assert(grid.width() == 64);
    assert(grid.height() == 64);
    assert(grid.has_pathway(10, 10));
    assert(!grid.has_pathway(0, 0));
    PASS();
}

static void test_network_graph_accessor() {
    TEST("get_network_graph returns graph after tick");
    TransportSystem sys(32, 32);

    sys.place_pathway(5, 5, PathwayType::BasicPathway, 0);
    sys.place_pathway(5, 6, PathwayType::BasicPathway, 0);
    sys.tick(0.05f);

    const NetworkGraph& graph = sys.get_network_graph();
    assert(graph.node_count() == 2);
    PASS();
}

static void test_proximity_cache_accessor() {
    TEST("get_proximity_cache returns rebuilt cache");
    TransportSystem sys(32, 32);

    sys.place_pathway(10, 10, PathwayType::BasicPathway, 0);
    sys.tick(0.05f);

    const ProximityCache& cache = sys.get_proximity_cache();
    assert(cache.get_distance(10, 10) == 0);
    assert(cache.get_distance(10, 11) == 1);
    PASS();
}

static void test_cross_ownership_connectivity() {
    TEST("Cross-ownership pathways connect (CCR-002)");
    TransportSystem sys(32, 32);

    sys.place_pathway(5, 5, PathwayType::BasicPathway, 0);
    sys.place_pathway(5, 6, PathwayType::BasicPathway, 1);  // Different owner
    sys.tick(0.05f);

    // Should be on same network despite different owners
    assert(sys.are_connected(5, 5, 5, 6));
    PASS();
}

static void test_pathway_types() {
    TEST("Different pathway types can be placed");
    TransportSystem sys(32, 32);

    uint32_t id1 = sys.place_pathway(0, 0, PathwayType::BasicPathway, 0);
    uint32_t id2 = sys.place_pathway(1, 0, PathwayType::TransitCorridor, 0);
    uint32_t id3 = sys.place_pathway(2, 0, PathwayType::Pedestrian, 0);
    uint32_t id4 = sys.place_pathway(3, 0, PathwayType::Bridge, 0);
    uint32_t id5 = sys.place_pathway(4, 0, PathwayType::Tunnel, 0);

    assert(id1 != 0 && id2 != 0 && id3 != 0 && id4 != 0 && id5 != 0);
    assert(sys.get_pathway_count() == 5);
    PASS();
}

// =============================================================================
// Main
// =============================================================================

int main() {
    printf("=== TransportSystem Tests (E7-022) ===\n");

    test_construction();
    test_priority();
    test_place_pathway();
    test_place_pathway_out_of_bounds();
    test_place_pathway_occupied();
    test_place_pathway_invalid_owner();
    test_remove_pathway();
    test_remove_pathway_wrong_owner();
    test_remove_pathway_wrong_position();
    test_remove_nonexistent();
    test_tick_rebuilds_network();
    test_tick_rebuilds_proximity();
    test_is_road_accessible_at();
    test_disconnected_networks();
    test_network_id_at();
    test_placed_events();
    test_removed_events();
    test_events_cleared_on_tick();
    test_grace_period();
    test_congestion_at();
    test_traffic_volume_at();
    test_congestion_no_pathway();
    test_multiple_ticks();
    test_decay_runs_periodically();
    test_unique_entity_ids();
    test_pathway_grid_accessor();
    test_network_graph_accessor();
    test_proximity_cache_accessor();
    test_cross_ownership_connectivity();
    test_pathway_types();

    printf("\n=== Results: %d/%d passed ===\n", tests_passed, tests_total);
    return (tests_passed == tests_total) ? 0 : 1;
}
