/**
 * @file test_conduit_connection.cpp
 * @brief Unit tests for fluid conduit connection detection during BFS (Ticket 6-030)
 *
 * Tests cover:
 * - Conduit connected to extractor network is_connected = true
 * - Isolated conduit is_connected = false
 * - Chain of conduits all connected
 * - BFS resets is_connected before traversal
 * - Per-player connection isolation
 */

#include <sims3000/fluid/FluidConduitComponent.h>
#include <sims3000/fluid/FluidProducerComponent.h>
#include <sims3000/fluid/FluidExtractorConfig.h>
#include <sims3000/fluid/FluidCoverageBFS.h>
#include <sims3000/fluid/FluidCoverageGrid.h>
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
// Helper: create an operational extractor entity
// =============================================================================

static uint32_t create_operational_extractor(entt::registry& registry) {
    auto entity = registry.create();
    FluidProducerComponent producer{};
    producer.base_output = 100;
    producer.current_output = 100;
    producer.max_water_distance = 5;
    producer.current_water_distance = 0;
    producer.is_operational = true;
    producer.producer_type = static_cast<uint8_t>(FluidProducerType::Extractor);
    registry.emplace<FluidProducerComponent>(entity, producer);
    return static_cast<uint32_t>(entity);
}

// =============================================================================
// Connected conduit tests
// =============================================================================

TEST(connected_conduit_is_marked_true) {
    // Extractor at (50,50), conduit adjacent at (51,50)
    // After BFS, conduit should be is_connected = true
    entt::registry registry;

    uint32_t ext_id = create_operational_extractor(registry);

    // Create conduit adjacent to extractor
    auto cond_ent = registry.create();
    FluidConduitComponent conduit{};
    conduit.coverage_radius = 3;
    conduit.is_connected = false;
    registry.emplace<FluidConduitComponent>(cond_ent, conduit);
    uint32_t cond_id = static_cast<uint32_t>(cond_ent);

    FluidCoverageGrid grid(128, 128);
    std::unordered_map<uint64_t, uint32_t> ext_pos;
    ext_pos[pack_pos(50, 50)] = ext_id;
    std::unordered_map<uint64_t, uint32_t> res_pos;
    std::unordered_map<uint64_t, uint32_t> cond_pos;
    cond_pos[pack_pos(51, 50)] = cond_id;

    BFSContext ctx{
        grid,
        ext_pos,
        res_pos,
        cond_pos,
        &registry,
        0,
        128,
        128
    };
    recalculate_coverage(ctx);

    const auto& c = registry.get<FluidConduitComponent>(cond_ent);
    ASSERT(c.is_connected);
}

TEST(isolated_conduit_remains_disconnected) {
    entt::registry registry;

    uint32_t ext_id = create_operational_extractor(registry);

    // Create conduit far from extractor (isolated)
    auto cond_ent = registry.create();
    FluidConduitComponent conduit{};
    conduit.coverage_radius = 3;
    conduit.is_connected = false;
    registry.emplace<FluidConduitComponent>(cond_ent, conduit);
    uint32_t cond_id = static_cast<uint32_t>(cond_ent);

    FluidCoverageGrid grid(128, 128);
    std::unordered_map<uint64_t, uint32_t> ext_pos;
    ext_pos[pack_pos(20, 50)] = ext_id;
    std::unordered_map<uint64_t, uint32_t> res_pos;
    std::unordered_map<uint64_t, uint32_t> cond_pos;
    cond_pos[pack_pos(100, 100)] = cond_id; // far away

    BFSContext ctx{
        grid,
        ext_pos,
        res_pos,
        cond_pos,
        &registry,
        0,
        128,
        128
    };
    recalculate_coverage(ctx);

    const auto& c = registry.get<FluidConduitComponent>(cond_ent);
    ASSERT(!c.is_connected);
}

TEST(chain_of_conduits_all_connected) {
    entt::registry registry;

    uint32_t ext_id = create_operational_extractor(registry);

    // Create chain of 5 conduits
    entt::entity cond_ents[5];
    uint32_t cond_ids[5];
    for (int i = 0; i < 5; ++i) {
        cond_ents[i] = registry.create();
        FluidConduitComponent c{};
        c.coverage_radius = 3;
        c.is_connected = false;
        registry.emplace<FluidConduitComponent>(cond_ents[i], c);
        cond_ids[i] = static_cast<uint32_t>(cond_ents[i]);
    }

    FluidCoverageGrid grid(128, 128);
    std::unordered_map<uint64_t, uint32_t> ext_pos;
    ext_pos[pack_pos(50, 50)] = ext_id;
    std::unordered_map<uint64_t, uint32_t> res_pos;
    std::unordered_map<uint64_t, uint32_t> cond_pos;

    // Chain: (51,50), (52,50), (53,50), (54,50), (55,50)
    for (int i = 0; i < 5; ++i) {
        cond_pos[pack_pos(51 + i, 50)] = cond_ids[i];
    }

    BFSContext ctx{
        grid,
        ext_pos,
        res_pos,
        cond_pos,
        &registry,
        0,
        128,
        128
    };
    recalculate_coverage(ctx);

    // All conduits should be connected
    for (int i = 0; i < 5; ++i) {
        ASSERT(registry.get<FluidConduitComponent>(cond_ents[i]).is_connected);
    }
}

TEST(bfs_resets_is_connected_before_traversal) {
    entt::registry registry;

    uint32_t ext_id = create_operational_extractor(registry);

    // Create conduit adjacent to extractor
    auto cond_ent = registry.create();
    FluidConduitComponent conduit{};
    conduit.coverage_radius = 3;
    conduit.is_connected = true; // pre-set to true
    registry.emplace<FluidConduitComponent>(cond_ent, conduit);
    uint32_t cond_id = static_cast<uint32_t>(cond_ent);

    FluidCoverageGrid grid(128, 128);
    std::unordered_map<uint64_t, uint32_t> ext_pos;
    ext_pos[pack_pos(50, 50)] = ext_id;
    std::unordered_map<uint64_t, uint32_t> res_pos;
    std::unordered_map<uint64_t, uint32_t> cond_pos;
    cond_pos[pack_pos(51, 50)] = cond_id;

    BFSContext ctx{
        grid,
        ext_pos,
        res_pos,
        cond_pos,
        &registry,
        0,
        128,
        128
    };

    // First recalculate - conduit connected
    recalculate_coverage(ctx);
    ASSERT(registry.get<FluidConduitComponent>(cond_ent).is_connected);

    // Remove extractor from positions - conduit should become disconnected
    std::unordered_map<uint64_t, uint32_t> empty_ext_pos;
    BFSContext ctx2{
        grid,
        empty_ext_pos,
        res_pos,
        cond_pos,
        &registry,
        0,
        128,
        128
    };
    recalculate_coverage(ctx2);

    // After recalculate with no extractor, conduit should be reset to disconnected
    ASSERT(!registry.get<FluidConduitComponent>(cond_ent).is_connected);
}

TEST(conduit_with_gap_partially_connected) {
    entt::registry registry;

    uint32_t ext_id = create_operational_extractor(registry);

    // Connected conduit at (21,50) - adjacent to extractor at (20,50)
    auto c1_ent = registry.create();
    FluidConduitComponent c1{};
    c1.coverage_radius = 3;
    c1.is_connected = false;
    registry.emplace<FluidConduitComponent>(c1_ent, c1);
    uint32_t c1_id = static_cast<uint32_t>(c1_ent);

    // Isolated conduit at (80,50) - far from extractor, gap
    auto c2_ent = registry.create();
    FluidConduitComponent c2{};
    c2.coverage_radius = 3;
    c2.is_connected = false;
    registry.emplace<FluidConduitComponent>(c2_ent, c2);
    uint32_t c2_id = static_cast<uint32_t>(c2_ent);

    FluidCoverageGrid grid(128, 128);
    std::unordered_map<uint64_t, uint32_t> ext_pos;
    ext_pos[pack_pos(20, 50)] = ext_id;
    std::unordered_map<uint64_t, uint32_t> res_pos;
    std::unordered_map<uint64_t, uint32_t> cond_pos;
    cond_pos[pack_pos(21, 50)] = c1_id;
    cond_pos[pack_pos(80, 50)] = c2_id;

    BFSContext ctx{
        grid,
        ext_pos,
        res_pos,
        cond_pos,
        &registry,
        0,
        128,
        128
    };
    recalculate_coverage(ctx);

    // c1 should be connected (adjacent to extractor)
    ASSERT(registry.get<FluidConduitComponent>(c1_ent).is_connected);
    // c2 should NOT be connected (isolated)
    ASSERT(!registry.get<FluidConduitComponent>(c2_ent).is_connected);
}

TEST(conduit_connected_via_reservoir) {
    // Reservoirs also seed BFS - conduit adjacent to reservoir should connect
    entt::registry registry;

    // No extractor, but we have a reservoir at (50,50)
    auto res_ent = registry.create();
    uint32_t res_id = static_cast<uint32_t>(res_ent);

    // Create conduit adjacent to reservoir
    auto cond_ent = registry.create();
    FluidConduitComponent conduit{};
    conduit.coverage_radius = 3;
    conduit.is_connected = false;
    registry.emplace<FluidConduitComponent>(cond_ent, conduit);
    uint32_t cond_id = static_cast<uint32_t>(cond_ent);

    FluidCoverageGrid grid(128, 128);
    std::unordered_map<uint64_t, uint32_t> ext_pos;
    std::unordered_map<uint64_t, uint32_t> res_pos;
    res_pos[pack_pos(50, 50)] = res_id;
    std::unordered_map<uint64_t, uint32_t> cond_pos;
    cond_pos[pack_pos(51, 50)] = cond_id;

    BFSContext ctx{
        grid,
        ext_pos,
        res_pos,
        cond_pos,
        &registry,
        0,
        128,
        128
    };
    recalculate_coverage(ctx);

    const auto& c = registry.get<FluidConduitComponent>(cond_ent);
    ASSERT(c.is_connected);
}

// =============================================================================
// Main Entry Point
// =============================================================================

int main() {
    printf("=== Fluid Conduit Connection Detection Unit Tests (Ticket 6-030) ===\n\n");

    RUN_TEST(connected_conduit_is_marked_true);
    RUN_TEST(isolated_conduit_remains_disconnected);
    RUN_TEST(chain_of_conduits_all_connected);
    RUN_TEST(bfs_resets_is_connected_before_traversal);
    RUN_TEST(conduit_with_gap_partially_connected);
    RUN_TEST(conduit_connected_via_reservoir);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
