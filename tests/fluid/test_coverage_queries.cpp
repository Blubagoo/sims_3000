/**
 * @file test_coverage_queries.cpp
 * @brief Unit tests for FluidSystem coverage queries (Ticket 6-013)
 *        and dirty flag tracking (Ticket 6-011)
 *
 * Tests cover:
 * - is_in_coverage returns false on empty grid
 * - After recalculate, is_in_coverage returns true for covered tiles
 * - get_coverage_at returns correct owner
 * - get_coverage_count returns correct count
 * - Dirty flag set on conduit placement
 * - Dirty flag set on extractor placement
 * - Dirty flag cleared after tick()
 * - Different players have independent dirty flags
 *
 * Uses printf test pattern matching existing fluid tests.
 */

#include <sims3000/fluid/FluidSystem.h>
#include <sims3000/fluid/FluidComponent.h>
#include <sims3000/fluid/FluidProducerComponent.h>
#include <sims3000/fluid/FluidEnums.h>
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
// is_in_coverage Tests
// =============================================================================

TEST(is_in_coverage_returns_false_on_empty_grid) {
    FluidSystem sys(64, 64);
    // No extractors, conduits, or reservoirs placed - grid is empty
    // owner=1 is overseer_id for player 0
    ASSERT(!sys.is_in_coverage(0, 0, 1));
    ASSERT(!sys.is_in_coverage(32, 32, 1));
    ASSERT(!sys.is_in_coverage(63, 63, 1));
    // All 4 players
    for (uint8_t owner_id = 1; owner_id <= MAX_PLAYERS; ++owner_id) {
        ASSERT(!sys.is_in_coverage(10, 10, owner_id));
    }
}

TEST(is_in_coverage_returns_true_after_recalculate) {
    // Place a reservoir (seeds coverage without power check) and run tick
    // to trigger BFS recalculation via dirty flag mechanism.
    FluidSystem sys(64, 64);
    entt::registry registry;
    sys.set_registry(&registry);

    // Place a reservoir at (10, 10) for player 0
    uint32_t eid = sys.place_reservoir(10, 10, 0);
    ASSERT(eid != INVALID_ENTITY_ID);

    // Dirty flag should be set
    ASSERT(sys.is_coverage_dirty(0));

    // Tick triggers Phase 4: recalculate_coverage if dirty
    sys.tick(0.016f);

    // After tick, dirty flag should be cleared
    ASSERT(!sys.is_coverage_dirty(0));

    // The reservoir position itself should be in coverage (overseer_id = player_id + 1 = 1)
    ASSERT(sys.is_in_coverage(10, 10, 1));
}

TEST(is_in_coverage_covers_radius_around_reservoir) {
    FluidSystem sys(64, 64);
    entt::registry registry;
    sys.set_registry(&registry);

    // Place a reservoir at (10, 10) for player 0
    sys.place_reservoir(10, 10, 0);
    sys.tick(0.016f);

    // Reservoir has a coverage radius; tiles within radius should be covered
    // Check the center tile
    ASSERT(sys.is_in_coverage(10, 10, 1));
    // Check adjacent tiles (within default reservoir coverage radius)
    ASSERT(sys.is_in_coverage(10, 11, 1));
    ASSERT(sys.is_in_coverage(11, 10, 1));
    ASSERT(sys.is_in_coverage(9, 10, 1));
    ASSERT(sys.is_in_coverage(10, 9, 1));
}

// =============================================================================
// get_coverage_at Tests
// =============================================================================

TEST(get_coverage_at_returns_zero_on_empty_grid) {
    FluidSystem sys(64, 64);
    ASSERT_EQ(sys.get_coverage_at(0, 0), static_cast<uint8_t>(0));
    ASSERT_EQ(sys.get_coverage_at(32, 32), static_cast<uint8_t>(0));
}

TEST(get_coverage_at_returns_correct_owner) {
    FluidSystem sys(64, 64);
    entt::registry registry;
    sys.set_registry(&registry);

    // Player 0 places reservoir at (10, 10)
    sys.place_reservoir(10, 10, 0);
    sys.tick(0.016f);

    // overseer_id for player 0 is 1
    ASSERT_EQ(sys.get_coverage_at(10, 10), static_cast<uint8_t>(1));
}

TEST(get_coverage_at_returns_correct_owner_player1) {
    FluidSystem sys(64, 64);
    entt::registry registry;
    sys.set_registry(&registry);

    // Player 1 places reservoir at (30, 30)
    sys.place_reservoir(30, 30, 1);
    sys.tick(0.016f);

    // overseer_id for player 1 is 2
    ASSERT_EQ(sys.get_coverage_at(30, 30), static_cast<uint8_t>(2));

    // Uncovered tile should still be 0
    ASSERT_EQ(sys.get_coverage_at(0, 0), static_cast<uint8_t>(0));
}

// =============================================================================
// get_coverage_count Tests
// =============================================================================

TEST(get_coverage_count_returns_zero_on_empty_grid) {
    FluidSystem sys(64, 64);
    // owner_id 1-4 are valid player overseer IDs (player_id + 1)
    // owner_id 0 means "uncovered" in the grid, so get_coverage_count(0)
    // would count all uncovered cells (the entire grid).
    for (uint8_t owner_id = 1; owner_id <= MAX_PLAYERS; ++owner_id) {
        ASSERT_EQ(sys.get_coverage_count(owner_id), 0u);
    }
}

TEST(get_coverage_count_returns_correct_count) {
    FluidSystem sys(64, 64);
    entt::registry registry;
    sys.set_registry(&registry);

    // Place a reservoir for player 0
    sys.place_reservoir(10, 10, 0);
    sys.tick(0.016f);

    // After recalculation, coverage count for overseer_id=1 should be > 0
    uint32_t count = sys.get_coverage_count(1);
    ASSERT(count > 0);

    // Coverage count for other owners should be 0
    ASSERT_EQ(sys.get_coverage_count(2), 0u);
    ASSERT_EQ(sys.get_coverage_count(3), 0u);
    ASSERT_EQ(sys.get_coverage_count(4), 0u);
}

// =============================================================================
// Dirty flag set on conduit placement
// =============================================================================

TEST(dirty_flag_set_on_conduit_placement) {
    FluidSystem sys(64, 64);
    entt::registry registry;
    sys.set_registry(&registry);

    ASSERT(!sys.is_coverage_dirty(0));
    sys.place_conduit(5, 5, 0);
    ASSERT(sys.is_coverage_dirty(0));
}

// =============================================================================
// Dirty flag set on extractor placement
// =============================================================================

TEST(dirty_flag_set_on_extractor_placement) {
    FluidSystem sys(64, 64);
    entt::registry registry;
    sys.set_registry(&registry);

    ASSERT(!sys.is_coverage_dirty(0));
    sys.place_extractor(5, 5, 0);
    ASSERT(sys.is_coverage_dirty(0));
}

// =============================================================================
// Dirty flag cleared after tick
// =============================================================================

TEST(dirty_flag_cleared_after_tick) {
    FluidSystem sys(64, 64);
    entt::registry registry;
    sys.set_registry(&registry);

    sys.place_extractor(5, 5, 0);
    ASSERT(sys.is_coverage_dirty(0));

    sys.tick(0.016f);
    ASSERT(!sys.is_coverage_dirty(0));
}

// =============================================================================
// Independent dirty flags per player
// =============================================================================

TEST(different_players_have_independent_dirty_flags) {
    FluidSystem sys(64, 64);
    entt::registry registry;
    sys.set_registry(&registry);

    // All start clean
    for (uint8_t i = 0; i < MAX_PLAYERS; ++i) {
        ASSERT(!sys.is_coverage_dirty(i));
    }

    // Dirty player 0 only
    sys.place_conduit(5, 5, 0);
    ASSERT(sys.is_coverage_dirty(0));
    ASSERT(!sys.is_coverage_dirty(1));
    ASSERT(!sys.is_coverage_dirty(2));
    ASSERT(!sys.is_coverage_dirty(3));

    // Dirty player 2
    sys.place_conduit(15, 15, 2);
    ASSERT(sys.is_coverage_dirty(0));
    ASSERT(!sys.is_coverage_dirty(1));
    ASSERT(sys.is_coverage_dirty(2));
    ASSERT(!sys.is_coverage_dirty(3));

    // Tick clears all dirty flags
    sys.tick(0.016f);
    for (uint8_t i = 0; i < MAX_PLAYERS; ++i) {
        ASSERT(!sys.is_coverage_dirty(i));
    }

    // Dirty player 1 only
    sys.place_extractor(20, 20, 1);
    ASSERT(!sys.is_coverage_dirty(0));
    ASSERT(sys.is_coverage_dirty(1));
    ASSERT(!sys.is_coverage_dirty(2));
    ASSERT(!sys.is_coverage_dirty(3));
}

// =============================================================================
// Main
// =============================================================================

int main() {
    printf("=== FluidSystem Coverage Query Tests (Tickets 6-013, 6-011) ===\n\n");

    // is_in_coverage tests
    RUN_TEST(is_in_coverage_returns_false_on_empty_grid);
    RUN_TEST(is_in_coverage_returns_true_after_recalculate);
    RUN_TEST(is_in_coverage_covers_radius_around_reservoir);

    // get_coverage_at tests
    RUN_TEST(get_coverage_at_returns_zero_on_empty_grid);
    RUN_TEST(get_coverage_at_returns_correct_owner);
    RUN_TEST(get_coverage_at_returns_correct_owner_player1);

    // get_coverage_count tests
    RUN_TEST(get_coverage_count_returns_zero_on_empty_grid);
    RUN_TEST(get_coverage_count_returns_correct_count);

    // Dirty flag tests
    RUN_TEST(dirty_flag_set_on_conduit_placement);
    RUN_TEST(dirty_flag_set_on_extractor_placement);
    RUN_TEST(dirty_flag_cleared_after_tick);
    RUN_TEST(different_players_have_independent_dirty_flags);

    printf("\n=== Results: %d passed, %d failed ===\n",
           tests_passed, tests_failed);

    return tests_failed > 0 ? 1 : 0;
}
