/**
 * @file test_coverage_dirty.cpp
 * @brief Unit tests for coverage dirty flag tracking and boundary enforcement (Tickets 5-015, 5-016)
 *
 * Tests cover:
 * - Event handlers set dirty flags for correct owner
 * - tick() recalculates coverage only when dirty
 * - tick() clears dirty flag after recalculation
 * - can_extend_coverage_to() stub always returns true
 * - Boundary enforcement check point in BFS
 */

#include <sims3000/energy/EnergySystem.h>
#include <sims3000/energy/EnergyEvents.h>
#include <sims3000/energy/EnergyProducerComponent.h>
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
// Event handler dirty flag tests (Ticket 5-015)
// =============================================================================

TEST(on_conduit_placed_sets_dirty) {
    EnergySystem sys(64, 64);
    sys.recalculate_coverage(0);
    ASSERT(!sys.is_coverage_dirty(0));

    ConduitPlacedEvent event(100, 0, 10, 10);
    sys.on_conduit_placed(event);
    ASSERT(sys.is_coverage_dirty(0));
    ASSERT(!sys.is_coverage_dirty(1));
}

TEST(on_conduit_removed_sets_dirty) {
    EnergySystem sys(64, 64);
    sys.recalculate_coverage(0);
    ASSERT(!sys.is_coverage_dirty(0));

    ConduitRemovedEvent event(100, 0, 10, 10);
    sys.on_conduit_removed(event);
    ASSERT(sys.is_coverage_dirty(0));
}

TEST(on_nexus_placed_sets_dirty) {
    EnergySystem sys(64, 64);
    sys.recalculate_coverage(1);
    ASSERT(!sys.is_coverage_dirty(1));

    NexusPlacedEvent event(200, 1, 0, 20, 20);
    sys.on_nexus_placed(event);
    ASSERT(sys.is_coverage_dirty(1));
    ASSERT(!sys.is_coverage_dirty(0));
}

TEST(on_nexus_removed_sets_dirty) {
    EnergySystem sys(64, 64);
    sys.recalculate_coverage(2);
    ASSERT(!sys.is_coverage_dirty(2));

    NexusRemovedEvent event(200, 2, 20, 20);
    sys.on_nexus_removed(event);
    ASSERT(sys.is_coverage_dirty(2));
}

TEST(events_only_dirty_owning_player) {
    EnergySystem sys(64, 64);
    for (uint8_t i = 0; i < MAX_PLAYERS; ++i) {
        sys.recalculate_coverage(i);
    }

    // Place conduit for player 2
    ConduitPlacedEvent event(100, 2, 5, 5);
    sys.on_conduit_placed(event);

    ASSERT(!sys.is_coverage_dirty(0));
    ASSERT(!sys.is_coverage_dirty(1));
    ASSERT(sys.is_coverage_dirty(2));
    ASSERT(!sys.is_coverage_dirty(3));
}

TEST(multiple_events_same_player_stays_dirty) {
    EnergySystem sys(64, 64);
    sys.recalculate_coverage(0);

    ConduitPlacedEvent e1(100, 0, 5, 5);
    ConduitPlacedEvent e2(101, 0, 6, 6);
    NexusPlacedEvent e3(200, 0, 0, 10, 10);
    sys.on_conduit_placed(e1);
    sys.on_conduit_placed(e2);
    sys.on_nexus_placed(e3);

    ASSERT(sys.is_coverage_dirty(0));
}

// =============================================================================
// tick() dirty flag integration tests (Ticket 5-015)
// =============================================================================

TEST(tick_recalculates_when_dirty) {
    EnergySystem sys(128, 128);

    // Register a nexus and mark dirty
    sys.register_nexus(100, 0);
    sys.register_nexus_position(100, 0, 50, 50);
    sys.mark_coverage_dirty(0);
    ASSERT(sys.is_coverage_dirty(0));

    // No coverage yet
    ASSERT_EQ(sys.get_coverage_count(1), 0u);

    // tick should recalculate and clear dirty
    sys.tick(0.05f);

    ASSERT(!sys.is_coverage_dirty(0));
    // Coverage should exist now (default radius 8 -> 17x17 = 289)
    ASSERT_EQ(sys.get_coverage_count(1), 17u * 17u);
}

TEST(tick_skips_recalculation_when_not_dirty) {
    EnergySystem sys(128, 128);

    // Register a nexus and recalculate manually
    sys.register_nexus(100, 0);
    sys.register_nexus_position(100, 0, 50, 50);
    sys.recalculate_coverage(0);
    ASSERT(!sys.is_coverage_dirty(0));
    uint32_t count_before = sys.get_coverage_count(1);

    // tick with no dirty flag should not change anything
    sys.tick(0.05f);

    ASSERT(!sys.is_coverage_dirty(0));
    ASSERT_EQ(sys.get_coverage_count(1), count_before);
}

TEST(tick_clears_dirty_after_recalculation) {
    EnergySystem sys(128, 128);
    sys.mark_coverage_dirty(0);
    sys.mark_coverage_dirty(1);
    ASSERT(sys.is_coverage_dirty(0));
    ASSERT(sys.is_coverage_dirty(1));

    sys.tick(0.05f);

    ASSERT(!sys.is_coverage_dirty(0));
    ASSERT(!sys.is_coverage_dirty(1));
}

TEST(tick_recalculates_only_dirty_players) {
    EnergySystem sys(128, 128);

    // Player 0: has nexus, dirty
    sys.register_nexus(100, 0);
    sys.register_nexus_position(100, 0, 30, 30);
    sys.mark_coverage_dirty(0);

    // Player 1: has nexus, NOT dirty (already calculated)
    sys.register_nexus(200, 1);
    sys.register_nexus_position(200, 1, 80, 80);
    sys.recalculate_coverage(1);
    ASSERT(!sys.is_coverage_dirty(1));

    uint32_t p1_count_before = sys.get_coverage_count(2);

    sys.tick(0.05f);

    // Player 0 should now have coverage
    ASSERT(sys.get_coverage_count(1) > 0);
    // Player 1 coverage should be unchanged
    ASSERT_EQ(sys.get_coverage_count(2), p1_count_before);
}

// =============================================================================
// can_extend_coverage_to tests (Ticket 5-016)
// =============================================================================

TEST(can_extend_coverage_to_always_returns_true) {
    EnergySystem sys(64, 64);
    // Stub implementation always returns true
    ASSERT(sys.can_extend_coverage_to(0, 0, 0));
    ASSERT(sys.can_extend_coverage_to(32, 32, 1));
    ASSERT(sys.can_extend_coverage_to(63, 63, 2));
    ASSERT(sys.can_extend_coverage_to(0, 63, 3));
}

TEST(can_extend_coverage_to_out_of_bounds) {
    EnergySystem sys(64, 64);
    // Even out of bounds, stub returns true (boundary check is separate from grid bounds)
    ASSERT(sys.can_extend_coverage_to(100, 100, 0));
}

TEST(boundary_check_integrated_in_bfs) {
    // Verify that BFS uses can_extend_coverage_to check
    // Since stub always returns true, coverage should work normally
    EnergySystem sys(128, 128);

    sys.register_nexus(100, 0);
    sys.register_nexus_position(100, 0, 50, 50);
    sys.register_conduit_position(101, 0, 51, 50);

    sys.recalculate_coverage(0);

    // Both nexus and conduit coverage should exist
    ASSERT_EQ(sys.get_coverage_at(50, 50), 1);
    ASSERT_EQ(sys.get_coverage_at(51, 50), 1);
}

// =============================================================================
// Main Entry Point
// =============================================================================

int main() {
    printf("=== Coverage Dirty Flag & Boundary Tests (Tickets 5-015, 5-016) ===\n\n");

    // Event handler tests
    RUN_TEST(on_conduit_placed_sets_dirty);
    RUN_TEST(on_conduit_removed_sets_dirty);
    RUN_TEST(on_nexus_placed_sets_dirty);
    RUN_TEST(on_nexus_removed_sets_dirty);
    RUN_TEST(events_only_dirty_owning_player);
    RUN_TEST(multiple_events_same_player_stays_dirty);

    // tick() integration
    RUN_TEST(tick_recalculates_when_dirty);
    RUN_TEST(tick_skips_recalculation_when_not_dirty);
    RUN_TEST(tick_clears_dirty_after_recalculation);
    RUN_TEST(tick_recalculates_only_dirty_players);

    // Boundary enforcement
    RUN_TEST(can_extend_coverage_to_always_returns_true);
    RUN_TEST(can_extend_coverage_to_out_of_bounds);
    RUN_TEST(boundary_check_integrated_in_bfs);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
