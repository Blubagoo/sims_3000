/**
 * @file test_conduit_connection.cpp
 * @brief Unit tests for conduit connection detection during BFS (Ticket 5-028)
 *
 * Tests cover:
 * - Connected conduit has is_connected = true after BFS
 * - Isolated conduit remains is_connected = false
 * - BFS resets is_connected before traversal
 * - Chain of conduits all marked connected
 * - Conduit connected then disconnected after nexus removal
 */

#include <sims3000/energy/EnergySystem.h>
#include <sims3000/energy/EnergyConduitComponent.h>
#include <sims3000/energy/EnergyProducerComponent.h>
#include <sims3000/energy/EnergyEnums.h>
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
// Connected conduit tests
// =============================================================================

TEST(connected_conduit_is_marked_true) {
    EnergySystem sys(128, 128);
    entt::registry registry;

    // Create nexus (Wind, radius 4)
    auto nexus_ent = registry.create();
    EnergyProducerComponent producer{};
    producer.nexus_type = static_cast<uint8_t>(NexusType::Wind);
    producer.is_online = true;
    registry.emplace<EnergyProducerComponent>(nexus_ent, producer);
    uint32_t nexus_id = static_cast<uint32_t>(nexus_ent);

    // Create conduit adjacent to nexus
    auto cond_ent = registry.create();
    EnergyConduitComponent conduit{};
    conduit.coverage_radius = 2;
    conduit.is_connected = false;
    registry.emplace<EnergyConduitComponent>(cond_ent, conduit);
    uint32_t cond_id = static_cast<uint32_t>(cond_ent);

    sys.set_registry(&registry);
    sys.register_nexus(nexus_id, 0);
    sys.register_nexus_position(nexus_id, 0, 50, 50);
    sys.register_conduit_position(cond_id, 0, 51, 50);

    sys.recalculate_coverage(0);

    // Conduit should be marked connected
    const auto& c = registry.get<EnergyConduitComponent>(cond_ent);
    ASSERT(c.is_connected);
}

TEST(isolated_conduit_remains_disconnected) {
    EnergySystem sys(128, 128);
    entt::registry registry;

    // Create nexus at (20, 50)
    auto nexus_ent = registry.create();
    EnergyProducerComponent producer{};
    producer.nexus_type = static_cast<uint8_t>(NexusType::Wind);
    producer.is_online = true;
    registry.emplace<EnergyProducerComponent>(nexus_ent, producer);
    uint32_t nexus_id = static_cast<uint32_t>(nexus_ent);

    // Create conduit far from nexus (isolated)
    auto cond_ent = registry.create();
    EnergyConduitComponent conduit{};
    conduit.coverage_radius = 2;
    conduit.is_connected = false;
    registry.emplace<EnergyConduitComponent>(cond_ent, conduit);
    uint32_t cond_id = static_cast<uint32_t>(cond_ent);

    sys.set_registry(&registry);
    sys.register_nexus(nexus_id, 0);
    sys.register_nexus_position(nexus_id, 0, 20, 50);
    sys.register_conduit_position(cond_id, 0, 100, 100); // far away

    sys.recalculate_coverage(0);

    // Isolated conduit should remain disconnected
    const auto& c = registry.get<EnergyConduitComponent>(cond_ent);
    ASSERT(!c.is_connected);
}

TEST(bfs_resets_is_connected_before_traversal) {
    EnergySystem sys(128, 128);
    entt::registry registry;

    // Create nexus
    auto nexus_ent = registry.create();
    EnergyProducerComponent producer{};
    producer.nexus_type = static_cast<uint8_t>(NexusType::Wind);
    producer.is_online = true;
    registry.emplace<EnergyProducerComponent>(nexus_ent, producer);
    uint32_t nexus_id = static_cast<uint32_t>(nexus_ent);

    // Create conduit adjacent
    auto cond_ent = registry.create();
    EnergyConduitComponent conduit{};
    conduit.coverage_radius = 2;
    conduit.is_connected = true; // pre-set to true
    registry.emplace<EnergyConduitComponent>(cond_ent, conduit);
    uint32_t cond_id = static_cast<uint32_t>(cond_ent);

    sys.set_registry(&registry);
    sys.register_nexus(nexus_id, 0);
    sys.register_nexus_position(nexus_id, 0, 50, 50);
    sys.register_conduit_position(cond_id, 0, 51, 50);

    // First recalculate - conduit connected
    sys.recalculate_coverage(0);
    ASSERT(registry.get<EnergyConduitComponent>(cond_ent).is_connected);

    // Remove nexus - conduit should become disconnected
    sys.unregister_nexus(nexus_id, 0);
    sys.unregister_nexus_position(nexus_id, 0, 50, 50);
    sys.recalculate_coverage(0);

    // After recalculate with no nexus, conduit should be reset to disconnected
    ASSERT(!registry.get<EnergyConduitComponent>(cond_ent).is_connected);
}

TEST(chain_of_conduits_all_connected) {
    EnergySystem sys(128, 128);
    entt::registry registry;

    // Create nexus (Wind, radius 4)
    auto nexus_ent = registry.create();
    EnergyProducerComponent producer{};
    producer.nexus_type = static_cast<uint8_t>(NexusType::Wind);
    producer.is_online = true;
    registry.emplace<EnergyProducerComponent>(nexus_ent, producer);
    uint32_t nexus_id = static_cast<uint32_t>(nexus_ent);

    // Create chain of 5 conduits
    entt::entity cond_ents[5];
    uint32_t cond_ids[5];
    for (int i = 0; i < 5; ++i) {
        cond_ents[i] = registry.create();
        EnergyConduitComponent c{};
        c.coverage_radius = 1;
        c.is_connected = false;
        registry.emplace<EnergyConduitComponent>(cond_ents[i], c);
        cond_ids[i] = static_cast<uint32_t>(cond_ents[i]);
    }

    sys.set_registry(&registry);
    sys.register_nexus(nexus_id, 0);
    sys.register_nexus_position(nexus_id, 0, 50, 50);

    // Chain: (51,50), (52,50), (53,50), (54,50), (55,50)
    for (int i = 0; i < 5; ++i) {
        sys.register_conduit_position(cond_ids[i], 0, 51 + i, 50);
    }

    sys.recalculate_coverage(0);

    // All conduits should be connected
    for (int i = 0; i < 5; ++i) {
        ASSERT(registry.get<EnergyConduitComponent>(cond_ents[i]).is_connected);
    }
}

TEST(conduit_with_gap_partially_connected) {
    EnergySystem sys(128, 128);
    entt::registry registry;

    // Create nexus (Wind, radius 4)
    auto nexus_ent = registry.create();
    EnergyProducerComponent producer{};
    producer.nexus_type = static_cast<uint8_t>(NexusType::Wind);
    producer.is_online = true;
    registry.emplace<EnergyProducerComponent>(nexus_ent, producer);
    uint32_t nexus_id = static_cast<uint32_t>(nexus_ent);

    // Connected conduit at (21,50) - adjacent to nexus
    auto c1_ent = registry.create();
    EnergyConduitComponent c1{};
    c1.coverage_radius = 1;
    c1.is_connected = false;
    registry.emplace<EnergyConduitComponent>(c1_ent, c1);
    uint32_t c1_id = static_cast<uint32_t>(c1_ent);

    // Isolated conduit at (80,50) - far from nexus, gap
    auto c2_ent = registry.create();
    EnergyConduitComponent c2{};
    c2.coverage_radius = 1;
    c2.is_connected = false;
    registry.emplace<EnergyConduitComponent>(c2_ent, c2);
    uint32_t c2_id = static_cast<uint32_t>(c2_ent);

    sys.set_registry(&registry);
    sys.register_nexus(nexus_id, 0);
    sys.register_nexus_position(nexus_id, 0, 20, 50);
    sys.register_conduit_position(c1_id, 0, 21, 50);
    sys.register_conduit_position(c2_id, 0, 80, 50); // far away

    sys.recalculate_coverage(0);

    // c1 should be connected (adjacent to nexus)
    ASSERT(registry.get<EnergyConduitComponent>(c1_ent).is_connected);
    // c2 should NOT be connected (isolated)
    ASSERT(!registry.get<EnergyConduitComponent>(c2_ent).is_connected);
}

TEST(no_registry_does_not_crash) {
    EnergySystem sys(128, 128);
    // No registry set
    sys.register_nexus(100, 0);
    sys.register_nexus_position(100, 0, 50, 50);
    sys.register_conduit_position(200, 0, 51, 50);

    // Should not crash
    sys.recalculate_coverage(0);
}

TEST(per_player_connection_isolation) {
    EnergySystem sys(128, 128);
    entt::registry registry;

    // Player 0 nexus at (20,50)
    auto n0_ent = registry.create();
    EnergyProducerComponent p0{};
    p0.nexus_type = static_cast<uint8_t>(NexusType::Wind);
    p0.is_online = true;
    registry.emplace<EnergyProducerComponent>(n0_ent, p0);
    uint32_t n0_id = static_cast<uint32_t>(n0_ent);

    // Player 0 conduit at (21,50)
    auto c0_ent = registry.create();
    EnergyConduitComponent c0{};
    c0.coverage_radius = 2;
    c0.is_connected = false;
    registry.emplace<EnergyConduitComponent>(c0_ent, c0);
    uint32_t c0_id = static_cast<uint32_t>(c0_ent);

    // Player 1 conduit at (80,80) - no nexus for player 1
    auto c1_ent = registry.create();
    EnergyConduitComponent c1{};
    c1.coverage_radius = 2;
    c1.is_connected = false;
    registry.emplace<EnergyConduitComponent>(c1_ent, c1);
    uint32_t c1_id = static_cast<uint32_t>(c1_ent);

    sys.set_registry(&registry);
    sys.register_nexus(n0_id, 0);
    sys.register_nexus_position(n0_id, 0, 20, 50);
    sys.register_conduit_position(c0_id, 0, 21, 50);
    sys.register_conduit_position(c1_id, 1, 80, 80);

    sys.recalculate_coverage(0);
    sys.recalculate_coverage(1);

    // Player 0 conduit: connected
    ASSERT(registry.get<EnergyConduitComponent>(c0_ent).is_connected);
    // Player 1 conduit: no nexus -> disconnected
    ASSERT(!registry.get<EnergyConduitComponent>(c1_ent).is_connected);
}

// =============================================================================
// Main Entry Point
// =============================================================================

int main() {
    printf("=== Conduit Connection Detection Unit Tests (Ticket 5-028) ===\n\n");

    RUN_TEST(connected_conduit_is_marked_true);
    RUN_TEST(isolated_conduit_remains_disconnected);
    RUN_TEST(bfs_resets_is_connected_before_traversal);
    RUN_TEST(chain_of_conduits_all_connected);
    RUN_TEST(conduit_with_gap_partially_connected);
    RUN_TEST(no_registry_does_not_crash);
    RUN_TEST(per_player_connection_isolation);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
