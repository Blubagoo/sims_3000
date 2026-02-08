/**
 * @file test_conduit_removal.cpp
 * @brief Unit tests for fluid conduit removal (Ticket 6-031)
 *
 * Tests cover:
 * - Remove conduit returns true on success
 * - Remove conduit destroys entity
 * - Remove conduit sets coverage dirty
 * - Remove conduit emits event
 * - Remove invalid entity returns false
 * - Double removal returns false second time
 *
 * Uses printf test pattern consistent with other fluid tests.
 */

#include <sims3000/fluid/FluidSystem.h>
#include <sims3000/fluid/FluidConduitComponent.h>
#include <sims3000/fluid/FluidEvents.h>
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
// 6-031: Remove conduit returns true on success
// =============================================================================

TEST(remove_conduit_returns_true_on_success) {
    FluidSystem sys(128, 128);
    entt::registry reg;
    sys.set_registry(&reg);

    // Place a conduit first
    uint32_t eid = sys.place_conduit(64, 64, 0);
    ASSERT(eid != INVALID_ENTITY_ID);

    // Remove it
    bool result = sys.remove_conduit(eid, 0, 64, 64);
    ASSERT(result);
}

// =============================================================================
// 6-031: Remove conduit destroys entity
// =============================================================================

TEST(remove_conduit_destroys_entity) {
    FluidSystem sys(128, 128);
    entt::registry reg;
    sys.set_registry(&reg);

    uint32_t eid = sys.place_conduit(64, 64, 0);
    ASSERT(eid != INVALID_ENTITY_ID);

    auto entity = static_cast<entt::entity>(eid);
    ASSERT(reg.valid(entity));

    sys.remove_conduit(eid, 0, 64, 64);

    // Entity should no longer be valid
    ASSERT(!reg.valid(entity));
}

// =============================================================================
// 6-031: Remove conduit sets coverage dirty
// =============================================================================

TEST(remove_conduit_sets_coverage_dirty) {
    FluidSystem sys(128, 128);
    entt::registry reg;
    sys.set_registry(&reg);

    uint32_t eid = sys.place_conduit(64, 64, 0);

    // Place an extractor so we can trigger BFS coverage recalc to clear dirty
    sys.place_extractor(32, 32, 0);

    // Run tick to clear dirty flags via BFS recalc
    sys.tick(0.016f);

    // Dirty should now be false after tick recalculated coverage
    ASSERT(!sys.is_coverage_dirty(0));

    // Now remove the conduit
    sys.remove_conduit(eid, 0, 64, 64);

    // Coverage should be dirty again
    ASSERT(sys.is_coverage_dirty(0));
}

// =============================================================================
// 6-031: Remove conduit emits FluidConduitRemovedEvent
// =============================================================================

TEST(remove_conduit_emits_event) {
    FluidSystem sys(128, 128);
    entt::registry reg;
    sys.set_registry(&reg);

    uint32_t eid = sys.place_conduit(64, 64, 0);
    ASSERT(eid != INVALID_ENTITY_ID);

    // Clear events from placement
    sys.clear_transition_events();

    // Remove the conduit
    sys.remove_conduit(eid, 0, 64, 64);

    // Should have emitted a FluidConduitRemovedEvent
    const auto& events = sys.get_conduit_removed_events();
    ASSERT_EQ(events.size(), static_cast<size_t>(1));
    ASSERT_EQ(events[0].entity_id, eid);
    ASSERT_EQ(events[0].owner_id, static_cast<uint8_t>(0));
    ASSERT_EQ(events[0].grid_x, 64u);
    ASSERT_EQ(events[0].grid_y, 64u);
}

// =============================================================================
// 6-031: Remove invalid entity returns false
// =============================================================================

TEST(remove_invalid_entity_returns_false) {
    FluidSystem sys(128, 128);
    entt::registry reg;
    sys.set_registry(&reg);

    // Try to remove an entity that doesn't exist
    bool result = sys.remove_conduit(INVALID_ENTITY_ID, 0, 64, 64);
    ASSERT(!result);
}

TEST(remove_nonexistent_entity_returns_false) {
    FluidSystem sys(128, 128);
    entt::registry reg;
    sys.set_registry(&reg);

    // Create and destroy an entity to get a recycled ID
    auto e = reg.create();
    uint32_t eid = static_cast<uint32_t>(e);
    reg.destroy(e);

    // Entity no longer valid
    bool result = sys.remove_conduit(eid, 0, 64, 64);
    ASSERT(!result);
}

TEST(remove_entity_without_conduit_component_returns_false) {
    FluidSystem sys(128, 128);
    entt::registry reg;
    sys.set_registry(&reg);

    // Create an entity without FluidConduitComponent (just a plain entity)
    auto entity = reg.create();
    uint32_t eid = static_cast<uint32_t>(entity);

    bool result = sys.remove_conduit(eid, 0, 64, 64);
    ASSERT(!result);

    // Entity should still be valid (not destroyed on failure)
    ASSERT(reg.valid(entity));
}

TEST(remove_conduit_returns_false_without_registry) {
    FluidSystem sys(128, 128);
    // No registry set
    bool result = sys.remove_conduit(42, 0, 64, 64);
    ASSERT(!result);
}

TEST(remove_conduit_returns_false_for_invalid_owner) {
    FluidSystem sys(128, 128);
    entt::registry reg;
    sys.set_registry(&reg);

    uint32_t eid = sys.place_conduit(64, 64, 0);
    ASSERT(eid != INVALID_ENTITY_ID);

    // Invalid owner (>= MAX_PLAYERS)
    bool result = sys.remove_conduit(eid, MAX_PLAYERS, 64, 64);
    ASSERT(!result);

    // Entity should still be valid
    auto entity = static_cast<entt::entity>(eid);
    ASSERT(reg.valid(entity));
}

// =============================================================================
// 6-031: Double removal returns false second time
// =============================================================================

TEST(double_removal_returns_false_second_time) {
    FluidSystem sys(128, 128);
    entt::registry reg;
    sys.set_registry(&reg);

    uint32_t eid = sys.place_conduit(64, 64, 0);
    ASSERT(eid != INVALID_ENTITY_ID);

    // First removal succeeds
    ASSERT(sys.remove_conduit(eid, 0, 64, 64));

    // Second removal fails (entity destroyed)
    ASSERT(!sys.remove_conduit(eid, 0, 64, 64));
}

// =============================================================================
// 6-031: Remove conduit unregisters position
// =============================================================================

TEST(remove_conduit_unregisters_position) {
    FluidSystem sys(128, 128);
    entt::registry reg;
    sys.set_registry(&reg);

    uint32_t eid = sys.place_conduit(64, 64, 0);
    ASSERT_EQ(sys.get_conduit_position_count(0), 1u);

    sys.remove_conduit(eid, 0, 64, 64);

    // Position should be unregistered
    ASSERT_EQ(sys.get_conduit_position_count(0), 0u);
}

// =============================================================================
// 6-031: Multiple conduit removal
// =============================================================================

TEST(remove_multiple_conduits) {
    FluidSystem sys(128, 128);
    entt::registry reg;
    sys.set_registry(&reg);

    uint32_t eid1 = sys.place_conduit(10, 10, 0);
    uint32_t eid2 = sys.place_conduit(20, 20, 0);
    uint32_t eid3 = sys.place_conduit(30, 30, 0);

    ASSERT_EQ(sys.get_conduit_position_count(0), 3u);

    ASSERT(sys.remove_conduit(eid1, 0, 10, 10));
    ASSERT_EQ(sys.get_conduit_position_count(0), 2u);

    ASSERT(sys.remove_conduit(eid2, 0, 20, 20));
    ASSERT_EQ(sys.get_conduit_position_count(0), 1u);

    ASSERT(sys.remove_conduit(eid3, 0, 30, 30));
    ASSERT_EQ(sys.get_conduit_position_count(0), 0u);
}

// =============================================================================
// 6-031: Per-player isolation
// =============================================================================

TEST(remove_conduit_only_dirties_owner_coverage) {
    FluidSystem sys(128, 128);
    entt::registry reg;
    sys.set_registry(&reg);

    // Place conduits for player 0 and player 1
    uint32_t eid0 = sys.place_conduit(10, 10, 0);
    sys.place_conduit(20, 20, 1);

    // Place extractors so BFS can run and clear dirty flags
    sys.place_extractor(5, 5, 0);
    sys.place_extractor(15, 15, 1);

    // Tick to clear dirty flags
    sys.tick(0.016f);
    ASSERT(!sys.is_coverage_dirty(0));
    ASSERT(!sys.is_coverage_dirty(1));

    // Remove player 0's conduit
    sys.remove_conduit(eid0, 0, 10, 10);

    // Player 0's coverage should be dirty
    ASSERT(sys.is_coverage_dirty(0));

    // Player 1's coverage should NOT be dirty
    ASSERT(!sys.is_coverage_dirty(1));
}

// =============================================================================
// Main Entry Point
// =============================================================================

int main() {
    printf("=== Fluid Conduit Removal Unit Tests (Ticket 6-031) ===\n\n");

    // Successful removal
    RUN_TEST(remove_conduit_returns_true_on_success);
    RUN_TEST(remove_conduit_destroys_entity);
    RUN_TEST(remove_conduit_sets_coverage_dirty);
    RUN_TEST(remove_conduit_emits_event);

    // Failure cases
    RUN_TEST(remove_invalid_entity_returns_false);
    RUN_TEST(remove_nonexistent_entity_returns_false);
    RUN_TEST(remove_entity_without_conduit_component_returns_false);
    RUN_TEST(remove_conduit_returns_false_without_registry);
    RUN_TEST(remove_conduit_returns_false_for_invalid_owner);

    // Double removal
    RUN_TEST(double_removal_returns_false_second_time);

    // Position tracking
    RUN_TEST(remove_conduit_unregisters_position);

    // Multiple removals
    RUN_TEST(remove_multiple_conduits);

    // Per-player isolation
    RUN_TEST(remove_conduit_only_dirties_owner_coverage);

    printf("\n=== Results: %d passed, %d failed ===\n",
           tests_passed, tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
