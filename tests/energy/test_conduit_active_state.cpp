/**
 * @file test_conduit_active_state.cpp
 * @brief Unit tests for conduit active state (Ticket 5-030)
 *
 * Tests cover:
 * - Conduit is_active set true when connected AND pool has generation
 * - Conduit is_active set false when disconnected
 * - Conduit is_active set false when pool has zero generation
 * - Conduit is_active set false when both disconnected and no generation
 * - Multiple conduits updated correctly per player
 * - Per-player isolation (player 0 conduits unaffected by player 1 state)
 * - No-op when registry is null
 * - No-op for invalid owner
 * - No-op when no conduit positions exist
 */

#include <sims3000/energy/EnergySystem.h>
#include <sims3000/energy/EnergyComponent.h>
#include <sims3000/energy/EnergyProducerComponent.h>
#include <sims3000/energy/EnergyConduitComponent.h>
#include <sims3000/energy/EnergyEnums.h>
#include <entt/entt.hpp>
#include <cstdio>
#include <cstdlib>

using namespace sims3000::energy;

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
// Helper: set up a nexus so pool has generation
// =============================================================================

static uint32_t setup_nexus_with_output(EnergySystem& sys, entt::registry& registry,
                                         uint8_t owner, uint32_t x, uint32_t y,
                                         uint32_t output) {
    auto entity = registry.create();
    uint32_t eid = static_cast<uint32_t>(entity);

    EnergyProducerComponent prod{};
    prod.base_output = output;
    prod.current_output = output;
    prod.efficiency = 1.0f;
    prod.age_factor = 1.0f;
    prod.nexus_type = static_cast<uint8_t>(NexusType::Carbon);
    prod.is_online = true;
    registry.emplace<EnergyProducerComponent>(entity, prod);

    sys.register_nexus(eid, owner);
    sys.register_nexus_position(eid, owner, x, y);

    return eid;
}

// =============================================================================
// Active when connected AND generation > 0
// =============================================================================

TEST(conduit_active_when_connected_and_generating) {
    EnergySystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    // Place a nexus with output so pool has generation
    setup_nexus_with_output(sys, registry, 0, 10, 10, 500);

    // Run coverage + pool calculation so pool.total_generated is set
    sys.recalculate_coverage(0);
    sys.calculate_pool(0);

    // Place a conduit adjacent to nexus (will be connected after BFS)
    uint32_t cid = sys.place_conduit(11, 10, 0);
    ASSERT(cid != INVALID_ENTITY_ID);

    // Recalculate coverage to set is_connected on the conduit
    sys.recalculate_coverage(0);
    sys.calculate_pool(0);

    // Verify pool has generation
    ASSERT(sys.get_pool(0).total_generated > 0);

    // Verify conduit is connected
    auto conduit_entity = static_cast<entt::entity>(cid);
    auto* conduit = registry.try_get<EnergyConduitComponent>(conduit_entity);
    ASSERT(conduit != nullptr);
    ASSERT(conduit->is_connected);

    // Now update active states
    sys.update_conduit_active_states(0);

    // Conduit should be active
    ASSERT(conduit->is_active);
}

// =============================================================================
// Inactive when disconnected
// =============================================================================

TEST(conduit_inactive_when_disconnected) {
    EnergySystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    // Place a nexus with output
    setup_nexus_with_output(sys, registry, 0, 10, 10, 500);

    // Place a conduit far away (not adjacent to nexus, won't be connected)
    uint32_t cid = sys.place_conduit(100, 100, 0);
    ASSERT(cid != INVALID_ENTITY_ID);

    // Recalculate coverage - conduit too far from nexus, won't be in BFS
    sys.recalculate_coverage(0);
    sys.calculate_pool(0);

    // Verify pool has generation
    ASSERT(sys.get_pool(0).total_generated > 0);

    // Verify conduit is NOT connected
    auto conduit_entity = static_cast<entt::entity>(cid);
    auto* conduit = registry.try_get<EnergyConduitComponent>(conduit_entity);
    ASSERT(conduit != nullptr);
    ASSERT(!conduit->is_connected);

    // Update active states
    sys.update_conduit_active_states(0);

    // Should be inactive: connected=false even though generation>0
    ASSERT(!conduit->is_active);
}

// =============================================================================
// Inactive when no generation
// =============================================================================

TEST(conduit_inactive_when_no_generation) {
    EnergySystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    // Place a conduit without any nexus (no generation)
    uint32_t cid = sys.place_conduit(50, 50, 0);
    ASSERT(cid != INVALID_ENTITY_ID);

    // Manually set the conduit as connected (simulate it for this test)
    auto conduit_entity = static_cast<entt::entity>(cid);
    auto* conduit = registry.try_get<EnergyConduitComponent>(conduit_entity);
    ASSERT(conduit != nullptr);
    conduit->is_connected = true;

    // Pool has zero generation (no nexuses)
    sys.calculate_pool(0);
    ASSERT_EQ(sys.get_pool(0).total_generated, 0u);

    // Update active states
    sys.update_conduit_active_states(0);

    // Should be inactive: connected=true but generation=0
    ASSERT(!conduit->is_active);
}

// =============================================================================
// Inactive when both disconnected and no generation
// =============================================================================

TEST(conduit_inactive_when_disconnected_and_no_generation) {
    EnergySystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    // Place a conduit, no nexus
    uint32_t cid = sys.place_conduit(50, 50, 0);
    ASSERT(cid != INVALID_ENTITY_ID);

    sys.calculate_pool(0);

    auto conduit_entity = static_cast<entt::entity>(cid);
    auto* conduit = registry.try_get<EnergyConduitComponent>(conduit_entity);
    ASSERT(conduit != nullptr);
    ASSERT(!conduit->is_connected);
    ASSERT_EQ(sys.get_pool(0).total_generated, 0u);

    sys.update_conduit_active_states(0);

    ASSERT(!conduit->is_active);
}

// =============================================================================
// Multiple conduits - mix of connected and disconnected
// =============================================================================

TEST(multiple_conduits_mixed_states) {
    EnergySystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    // Place a nexus
    setup_nexus_with_output(sys, registry, 0, 10, 10, 500);

    // Place adjacent conduit (will be connected)
    uint32_t cid1 = sys.place_conduit(11, 10, 0);
    // Place far conduit (won't be connected)
    uint32_t cid2 = sys.place_conduit(100, 100, 0);

    ASSERT(cid1 != INVALID_ENTITY_ID);
    ASSERT(cid2 != INVALID_ENTITY_ID);

    // Recalculate coverage and pool
    sys.recalculate_coverage(0);
    sys.calculate_pool(0);

    auto* c1 = registry.try_get<EnergyConduitComponent>(static_cast<entt::entity>(cid1));
    auto* c2 = registry.try_get<EnergyConduitComponent>(static_cast<entt::entity>(cid2));
    ASSERT(c1 != nullptr);
    ASSERT(c2 != nullptr);
    ASSERT(c1->is_connected);
    ASSERT(!c2->is_connected);

    sys.update_conduit_active_states(0);

    // Connected conduit should be active
    ASSERT(c1->is_active);
    // Disconnected conduit should not be active
    ASSERT(!c2->is_active);
}

// =============================================================================
// Per-player isolation
// =============================================================================

TEST(per_player_isolation) {
    EnergySystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    // Player 0: has nexus with output
    setup_nexus_with_output(sys, registry, 0, 10, 10, 500);
    uint32_t cid0 = sys.place_conduit(11, 10, 0);
    ASSERT(cid0 != INVALID_ENTITY_ID);

    // Player 1: no nexus (no generation)
    uint32_t cid1 = sys.place_conduit(50, 50, 1);
    ASSERT(cid1 != INVALID_ENTITY_ID);

    // Manually set player 1's conduit as connected for this test
    auto* c1 = registry.try_get<EnergyConduitComponent>(static_cast<entt::entity>(cid1));
    ASSERT(c1 != nullptr);
    c1->is_connected = true;

    // Recalculate for player 0
    sys.recalculate_coverage(0);
    sys.calculate_pool(0);
    sys.calculate_pool(1);

    // Player 0 has generation, player 1 does not
    ASSERT(sys.get_pool(0).total_generated > 0);
    ASSERT_EQ(sys.get_pool(1).total_generated, 0u);

    // Update active states for each player
    sys.update_conduit_active_states(0);
    sys.update_conduit_active_states(1);

    auto* c0 = registry.try_get<EnergyConduitComponent>(static_cast<entt::entity>(cid0));
    ASSERT(c0 != nullptr);

    // Player 0's conduit: connected + generation = active
    ASSERT(c0->is_active);
    // Player 1's conduit: connected but no generation = inactive
    ASSERT(!c1->is_active);
}

// =============================================================================
// No-op with null registry
// =============================================================================

TEST(noop_with_null_registry) {
    EnergySystem sys(128, 128);
    // No registry set - should not crash
    sys.update_conduit_active_states(0);
    // If we get here, it didn't crash
    ASSERT(true);
}

// =============================================================================
// No-op for invalid owner
// =============================================================================

TEST(noop_for_invalid_owner) {
    EnergySystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    // Invalid owner (>= MAX_PLAYERS) - should not crash
    sys.update_conduit_active_states(MAX_PLAYERS);
    sys.update_conduit_active_states(255);
    ASSERT(true);
}

// =============================================================================
// No-op when no conduit positions
// =============================================================================

TEST(noop_when_no_conduits) {
    EnergySystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    // No conduits placed - should not crash
    sys.update_conduit_active_states(0);
    ASSERT(true);
}

// =============================================================================
// Active state transitions from true to false when generation drops
// =============================================================================

TEST(active_becomes_inactive_when_nexus_goes_offline) {
    EnergySystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    // Place nexus with output
    uint32_t nid = setup_nexus_with_output(sys, registry, 0, 10, 10, 500);

    // Place adjacent conduit
    uint32_t cid = sys.place_conduit(11, 10, 0);
    ASSERT(cid != INVALID_ENTITY_ID);

    // Recalculate and calculate pool
    sys.recalculate_coverage(0);
    sys.calculate_pool(0);

    auto* conduit = registry.try_get<EnergyConduitComponent>(static_cast<entt::entity>(cid));
    ASSERT(conduit != nullptr);
    ASSERT(conduit->is_connected);

    // Update active states - should be active
    sys.update_conduit_active_states(0);
    ASSERT(conduit->is_active);

    // Now take the nexus offline
    auto* prod = registry.try_get<EnergyProducerComponent>(static_cast<entt::entity>(nid));
    ASSERT(prod != nullptr);
    prod->is_online = false;
    prod->current_output = 0;

    // Recalculate pool (generation now 0)
    sys.calculate_pool(0);
    ASSERT_EQ(sys.get_pool(0).total_generated, 0u);

    // Update active states again - should now be inactive
    sys.update_conduit_active_states(0);
    ASSERT(!conduit->is_active);
}

// =============================================================================
// Conduit with invalid entity in registry is skipped
// =============================================================================

TEST(skips_invalid_entities) {
    EnergySystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    // Place nexus and conduit
    setup_nexus_with_output(sys, registry, 0, 10, 10, 500);
    uint32_t cid = sys.place_conduit(11, 10, 0);
    ASSERT(cid != INVALID_ENTITY_ID);

    sys.recalculate_coverage(0);
    sys.calculate_pool(0);

    // Destroy the conduit entity from registry directly (simulating corruption)
    auto conduit_entity = static_cast<entt::entity>(cid);
    registry.destroy(conduit_entity);

    // Should not crash when encountering invalid entity
    sys.update_conduit_active_states(0);
    ASSERT(true);
}

// =============================================================================
// Main Entry Point
// =============================================================================

int main() {
    printf("=== Conduit Active State Unit Tests (Ticket 5-030) ===\n\n");

    // Core behavior
    RUN_TEST(conduit_active_when_connected_and_generating);
    RUN_TEST(conduit_inactive_when_disconnected);
    RUN_TEST(conduit_inactive_when_no_generation);
    RUN_TEST(conduit_inactive_when_disconnected_and_no_generation);

    // Multiple conduits
    RUN_TEST(multiple_conduits_mixed_states);

    // Per-player
    RUN_TEST(per_player_isolation);

    // Edge cases
    RUN_TEST(noop_with_null_registry);
    RUN_TEST(noop_for_invalid_owner);
    RUN_TEST(noop_when_no_conduits);

    // State transitions
    RUN_TEST(active_becomes_inactive_when_nexus_goes_offline);

    // Robustness
    RUN_TEST(skips_invalid_entities);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
