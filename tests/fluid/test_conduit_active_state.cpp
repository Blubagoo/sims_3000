/**
 * @file test_conduit_active_state.cpp
 * @brief Unit tests for fluid conduit active state (Ticket 6-032)
 *
 * Tests cover:
 * - Connected conduit with generation: is_active = true
 * - Connected conduit with no generation: is_active = false
 * - Disconnected conduit: is_active = false
 * - Active state updates each tick
 *
 * Uses printf test pattern consistent with other fluid tests.
 */

#include <sims3000/fluid/FluidSystem.h>
#include <sims3000/fluid/FluidConduitComponent.h>
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
// Connected conduit with generation: is_active = true
// =============================================================================

TEST(connected_conduit_with_generation_is_active) {
    FluidSystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    // Place an extractor at (10,10) - provides generation
    uint32_t ext_id = sys.place_extractor(10, 10, 0);
    ASSERT(ext_id != INVALID_ENTITY_ID);

    // Place a conduit adjacent to extractor - will be connected after BFS
    uint32_t cid = sys.place_conduit(11, 10, 0);
    ASSERT(cid != INVALID_ENTITY_ID);

    // Run a tick to trigger BFS coverage, pool calculation, and conduit active update
    sys.tick(0.016f);

    // Verify pool has generation
    ASSERT(sys.get_pool(0).total_generated > 0);

    // Verify conduit is connected (BFS reached it from extractor)
    auto conduit_entity = static_cast<entt::entity>(cid);
    const auto* conduit = registry.try_get<FluidConduitComponent>(conduit_entity);
    ASSERT(conduit != nullptr);
    ASSERT(conduit->is_connected);

    // Conduit should be active: is_connected=true AND total_generated>0
    ASSERT(conduit->is_active);
}

// =============================================================================
// Connected conduit with no generation: is_active = false
// =============================================================================

TEST(connected_conduit_with_no_generation_is_inactive) {
    FluidSystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    // Place a conduit without any extractor (no generation)
    uint32_t cid = sys.place_conduit(50, 50, 0);
    ASSERT(cid != INVALID_ENTITY_ID);

    // Manually set the conduit as connected (simulate for this test)
    auto conduit_entity = static_cast<entt::entity>(cid);
    auto* conduit = registry.try_get<FluidConduitComponent>(conduit_entity);
    ASSERT(conduit != nullptr);
    conduit->is_connected = true;

    // Run a tick - pool will have zero generation
    sys.tick(0.016f);

    // Pool has zero generation (no extractors)
    ASSERT_EQ(sys.get_pool(0).total_generated, 0u);

    // Conduit should be inactive: connected=true but generation=0
    // Note: tick() resets is_connected via BFS, but since no extractor,
    // the conduit won't be reached by BFS and is_connected will be reset to false.
    // So is_active will be false either way.
    ASSERT(!conduit->is_active);
}

// =============================================================================
// Disconnected conduit: is_active = false
// =============================================================================

TEST(disconnected_conduit_is_inactive) {
    FluidSystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    // Place an extractor at (10,10) - provides generation
    sys.place_extractor(10, 10, 0);

    // Place a conduit far away (not adjacent to extractor, won't be connected)
    uint32_t cid = sys.place_conduit(100, 100, 0);
    ASSERT(cid != INVALID_ENTITY_ID);

    // Run a tick
    sys.tick(0.016f);

    // Verify pool has generation
    ASSERT(sys.get_pool(0).total_generated > 0);

    // Conduit is not connected (too far from extractor)
    auto conduit_entity = static_cast<entt::entity>(cid);
    const auto* conduit = registry.try_get<FluidConduitComponent>(conduit_entity);
    ASSERT(conduit != nullptr);
    ASSERT(!conduit->is_connected);

    // Conduit should be inactive: connected=false
    ASSERT(!conduit->is_active);
}

// =============================================================================
// Active state updates each tick
// =============================================================================

TEST(active_state_updates_each_tick) {
    FluidSystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    // Place extractor and adjacent conduit
    uint32_t ext_id = sys.place_extractor(10, 10, 0);
    uint32_t cid = sys.place_conduit(11, 10, 0);
    ASSERT(ext_id != INVALID_ENTITY_ID);
    ASSERT(cid != INVALID_ENTITY_ID);

    // First tick: conduit should become active
    sys.tick(0.016f);

    auto conduit_entity = static_cast<entt::entity>(cid);
    auto* conduit = registry.try_get<FluidConduitComponent>(conduit_entity);
    ASSERT(conduit != nullptr);
    ASSERT(conduit->is_connected);
    ASSERT(conduit->is_active);

    // Now take extractor offline (set current_output to 0 and is_operational to false)
    auto ext_entity = static_cast<entt::entity>(ext_id);
    auto* prod = registry.try_get<FluidProducerComponent>(ext_entity);
    ASSERT(prod != nullptr);
    prod->current_output = 0;
    prod->is_operational = false;
    prod->base_output = 0;

    // Second tick: generation drops to 0, conduit should become inactive
    sys.tick(0.016f);

    ASSERT_EQ(sys.get_pool(0).total_generated, 0u);
    // Conduit is still connected (BFS traverses from extractor position)
    // but generation is 0 so is_active = false
    ASSERT(!conduit->is_active);

    // Restore extractor
    prod->base_output = 100;
    prod->current_output = 100;
    prod->is_operational = true;

    // Third tick: conduit should become active again
    sys.tick(0.016f);

    ASSERT(sys.get_pool(0).total_generated > 0);
    ASSERT(conduit->is_active);
}

// =============================================================================
// No-op with null registry does not crash
// =============================================================================

TEST(noop_with_null_registry) {
    FluidSystem sys(128, 128);
    // No registry set - should not crash
    sys.tick(0.016f);
    ASSERT(true);
}

// =============================================================================
// No-op when no conduit positions exist
// =============================================================================

TEST(noop_when_no_conduits) {
    FluidSystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    // No conduits placed - should not crash
    sys.tick(0.016f);
    ASSERT(true);
}

// =============================================================================
// Per-player isolation: player 0 conduits unaffected by player 1 state
// =============================================================================

TEST(per_player_isolation) {
    FluidSystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    // Player 0: extractor + conduit (will be active)
    sys.place_extractor(10, 10, 0);
    uint32_t cid0 = sys.place_conduit(11, 10, 0);
    ASSERT(cid0 != INVALID_ENTITY_ID);

    // Player 1: conduit only, no extractor (no generation)
    uint32_t cid1 = sys.place_conduit(50, 50, 1);
    ASSERT(cid1 != INVALID_ENTITY_ID);

    sys.tick(0.016f);

    // Player 0's conduit should be active
    auto* c0 = registry.try_get<FluidConduitComponent>(static_cast<entt::entity>(cid0));
    ASSERT(c0 != nullptr);
    ASSERT(c0->is_active);

    // Player 1's conduit should be inactive (no generation)
    auto* c1 = registry.try_get<FluidConduitComponent>(static_cast<entt::entity>(cid1));
    ASSERT(c1 != nullptr);
    ASSERT(!c1->is_active);
}

// =============================================================================
// Skips invalid entities gracefully
// =============================================================================

TEST(skips_invalid_entities) {
    FluidSystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    // Place extractor and conduit
    sys.place_extractor(10, 10, 0);
    uint32_t cid = sys.place_conduit(11, 10, 0);
    ASSERT(cid != INVALID_ENTITY_ID);

    // Destroy the conduit entity from registry directly
    auto conduit_entity = static_cast<entt::entity>(cid);
    registry.destroy(conduit_entity);

    // Should not crash when encountering invalid entity
    sys.tick(0.016f);
    ASSERT(true);
}

// =============================================================================
// Main Entry Point
// =============================================================================

int main() {
    printf("=== Fluid Conduit Active State Unit Tests (Ticket 6-032) ===\n\n");

    // Core behavior
    RUN_TEST(connected_conduit_with_generation_is_active);
    RUN_TEST(connected_conduit_with_no_generation_is_inactive);
    RUN_TEST(disconnected_conduit_is_inactive);

    // State updates across ticks
    RUN_TEST(active_state_updates_each_tick);

    // Edge cases
    RUN_TEST(noop_with_null_registry);
    RUN_TEST(noop_when_no_conduits);

    // Per-player isolation
    RUN_TEST(per_player_isolation);

    // Robustness
    RUN_TEST(skips_invalid_entities);

    printf("\n=== Results: %d passed, %d failed ===\n",
           tests_passed, tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
