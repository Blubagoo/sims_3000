/**
 * @file test_conduit_removal.cpp
 * @brief Unit tests for conduit removal (Ticket 5-029)
 *
 * Tests cover:
 * - remove_conduit() validates entity exists
 * - remove_conduit() validates entity has EnergyConduitComponent
 * - remove_conduit() unregisters conduit position
 * - remove_conduit() emits ConduitRemovedEvent (sets coverage dirty)
 * - remove_conduit() destroys entity from registry
 * - remove_conduit() returns true on success, false on failure
 * - remove_conduit() returns false without registry
 * - remove_conduit() returns false for invalid entity
 * - remove_conduit() returns false for entity without conduit component
 * - Coverage dirty flag set after removal
 */

#include <sims3000/energy/EnergySystem.h>
#include <sims3000/energy/EnergyConduitComponent.h>
#include <sims3000/energy/EnergyProducerComponent.h>
#include <sims3000/energy/EnergyEvents.h>
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
// Successful removal
// =============================================================================

TEST(remove_conduit_returns_true_on_success) {
    EnergySystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    // Place a conduit first
    uint32_t eid = sys.place_conduit(64, 64, 0);
    ASSERT(eid != INVALID_ENTITY_ID);

    // Remove it
    bool result = sys.remove_conduit(eid, 0, 64, 64);
    ASSERT(result);
}

TEST(remove_conduit_destroys_entity) {
    EnergySystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    uint32_t eid = sys.place_conduit(64, 64, 0);
    ASSERT(eid != INVALID_ENTITY_ID);

    auto entity = static_cast<entt::entity>(eid);
    ASSERT(registry.valid(entity));

    sys.remove_conduit(eid, 0, 64, 64);

    // Entity should no longer be valid
    ASSERT(!registry.valid(entity));
}

TEST(remove_conduit_unregisters_position) {
    EnergySystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    uint32_t eid = sys.place_conduit(64, 64, 0);
    ASSERT_EQ(sys.get_conduit_position_count(0), 1u);

    sys.remove_conduit(eid, 0, 64, 64);

    // Position should be unregistered
    ASSERT_EQ(sys.get_conduit_position_count(0), 0u);
}

TEST(remove_conduit_sets_coverage_dirty) {
    EnergySystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    uint32_t eid = sys.place_conduit(64, 64, 0);

    // Clear dirty flag (simulate tick processing)
    // We need to manually clear it since place_conduit sets it
    // Use recalculate_coverage to clear it, or just check it's dirty after removal
    // Actually, let's register a nexus position and recalculate to clear the flag
    auto nexus = registry.create();
    auto& prod = registry.emplace<EnergyProducerComponent>(nexus);
    prod.nexus_type = static_cast<uint8_t>(NexusType::Carbon);
    prod.base_output = 100;
    sys.register_nexus(static_cast<uint32_t>(nexus), 0);
    sys.register_nexus_position(static_cast<uint32_t>(nexus), 0, 32, 32);
    sys.recalculate_coverage(0);

    // Dirty should now be false after recalculation
    ASSERT(!sys.is_coverage_dirty(0));

    // Now remove the conduit
    sys.remove_conduit(eid, 0, 64, 64);

    // Coverage should be dirty again
    ASSERT(sys.is_coverage_dirty(0));
}

// =============================================================================
// Failure cases
// =============================================================================

TEST(remove_conduit_returns_false_without_registry) {
    EnergySystem sys(128, 128);
    // No registry set
    bool result = sys.remove_conduit(42, 0, 64, 64);
    ASSERT(!result);
}

TEST(remove_conduit_returns_false_for_invalid_entity) {
    EnergySystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    // Entity INVALID_ENTITY_ID doesn't exist
    bool result = sys.remove_conduit(INVALID_ENTITY_ID, 0, 64, 64);
    ASSERT(!result);
}

TEST(remove_conduit_returns_false_for_nonexistent_entity) {
    EnergySystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    // Create and destroy an entity to get a recycled ID
    auto e = registry.create();
    uint32_t eid = static_cast<uint32_t>(e);
    registry.destroy(e);

    // Entity no longer valid
    bool result = sys.remove_conduit(eid, 0, 64, 64);
    ASSERT(!result);
}

TEST(remove_conduit_returns_false_for_entity_without_conduit_component) {
    EnergySystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    // Create an entity with only EnergyProducerComponent (not conduit)
    auto entity = registry.create();
    auto& prod = registry.emplace<EnergyProducerComponent>(entity);
    prod.nexus_type = static_cast<uint8_t>(NexusType::Carbon);

    uint32_t eid = static_cast<uint32_t>(entity);
    bool result = sys.remove_conduit(eid, 0, 64, 64);
    ASSERT(!result);

    // Entity should still be valid (not destroyed on failure)
    ASSERT(registry.valid(entity));
}

TEST(remove_conduit_returns_false_for_invalid_owner) {
    EnergySystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    uint32_t eid = sys.place_conduit(64, 64, 0);
    ASSERT(eid != INVALID_ENTITY_ID);

    // Invalid owner (>= MAX_PLAYERS)
    bool result = sys.remove_conduit(eid, MAX_PLAYERS, 64, 64);
    ASSERT(!result);

    // Entity should still be valid
    auto entity = static_cast<entt::entity>(eid);
    ASSERT(registry.valid(entity));
}

// =============================================================================
// Multiple conduit removal
// =============================================================================

TEST(remove_multiple_conduits) {
    EnergySystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    uint32_t eid1 = sys.place_conduit(10, 10, 0);
    uint32_t eid2 = sys.place_conduit(20, 20, 0);
    uint32_t eid3 = sys.place_conduit(30, 30, 0);

    ASSERT_EQ(sys.get_conduit_position_count(0), 3u);

    // Remove first
    ASSERT(sys.remove_conduit(eid1, 0, 10, 10));
    ASSERT_EQ(sys.get_conduit_position_count(0), 2u);

    // Remove second
    ASSERT(sys.remove_conduit(eid2, 0, 20, 20));
    ASSERT_EQ(sys.get_conduit_position_count(0), 1u);

    // Remove third
    ASSERT(sys.remove_conduit(eid3, 0, 30, 30));
    ASSERT_EQ(sys.get_conduit_position_count(0), 0u);
}

TEST(remove_conduit_different_players) {
    EnergySystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    uint32_t eid0 = sys.place_conduit(10, 10, 0);
    uint32_t eid1 = sys.place_conduit(20, 20, 1);
    uint32_t eid2 = sys.place_conduit(30, 30, 2);

    ASSERT_EQ(sys.get_conduit_position_count(0), 1u);
    ASSERT_EQ(sys.get_conduit_position_count(1), 1u);
    ASSERT_EQ(sys.get_conduit_position_count(2), 1u);

    // Remove player 1's conduit
    ASSERT(sys.remove_conduit(eid1, 1, 20, 20));
    ASSERT_EQ(sys.get_conduit_position_count(0), 1u);
    ASSERT_EQ(sys.get_conduit_position_count(1), 0u);
    ASSERT_EQ(sys.get_conduit_position_count(2), 1u);

    // Player 0 and 2 entities should still be valid
    ASSERT(registry.valid(static_cast<entt::entity>(eid0)));
    ASSERT(registry.valid(static_cast<entt::entity>(eid2)));
}

// =============================================================================
// Double removal prevention
// =============================================================================

TEST(remove_conduit_twice_fails_second_time) {
    EnergySystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    uint32_t eid = sys.place_conduit(64, 64, 0);
    ASSERT(eid != INVALID_ENTITY_ID);

    // First removal succeeds
    ASSERT(sys.remove_conduit(eid, 0, 64, 64));

    // Second removal fails (entity destroyed)
    ASSERT(!sys.remove_conduit(eid, 0, 64, 64));
}

// =============================================================================
// Coverage dirty flag per-player isolation
// =============================================================================

TEST(remove_conduit_only_dirties_owner_coverage) {
    EnergySystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    // Place conduits for player 0 and player 1
    uint32_t eid0 = sys.place_conduit(10, 10, 0);
    sys.place_conduit(20, 20, 1);

    // Clear dirty flags by creating nexuses and recalculating
    auto n0 = registry.create();
    auto& p0 = registry.emplace<EnergyProducerComponent>(n0);
    p0.nexus_type = static_cast<uint8_t>(NexusType::Carbon);
    p0.base_output = 100;
    sys.register_nexus(static_cast<uint32_t>(n0), 0);
    sys.register_nexus_position(static_cast<uint32_t>(n0), 0, 5, 5);

    auto n1 = registry.create();
    auto& p1 = registry.emplace<EnergyProducerComponent>(n1);
    p1.nexus_type = static_cast<uint8_t>(NexusType::Carbon);
    p1.base_output = 100;
    sys.register_nexus(static_cast<uint32_t>(n1), 1);
    sys.register_nexus_position(static_cast<uint32_t>(n1), 1, 15, 15);

    sys.recalculate_coverage(0);
    sys.recalculate_coverage(1);

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
    printf("=== Conduit Removal Unit Tests (Ticket 5-029) ===\n\n");

    // Successful removal
    RUN_TEST(remove_conduit_returns_true_on_success);
    RUN_TEST(remove_conduit_destroys_entity);
    RUN_TEST(remove_conduit_unregisters_position);
    RUN_TEST(remove_conduit_sets_coverage_dirty);

    // Failure cases
    RUN_TEST(remove_conduit_returns_false_without_registry);
    RUN_TEST(remove_conduit_returns_false_for_invalid_entity);
    RUN_TEST(remove_conduit_returns_false_for_nonexistent_entity);
    RUN_TEST(remove_conduit_returns_false_for_entity_without_conduit_component);
    RUN_TEST(remove_conduit_returns_false_for_invalid_owner);

    // Multiple removals
    RUN_TEST(remove_multiple_conduits);
    RUN_TEST(remove_conduit_different_players);

    // Double removal
    RUN_TEST(remove_conduit_twice_fails_second_time);

    // Per-player isolation
    RUN_TEST(remove_conduit_only_dirties_owner_coverage);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
