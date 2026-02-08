/**
 * @file test_coverage_queries.cpp
 * @brief Unit tests for coverage query methods (Ticket 5-017)
 *
 * Tests cover:
 * - is_in_coverage(x, y, owner): O(1) query via CoverageGrid
 * - get_coverage_at(x, y): returns covering owner or 0
 * - get_coverage_count(owner): count of covered cells for owner
 * - Const and thread-safe query semantics
 * - Out-of-bounds behavior
 * - Multi-player coverage queries
 */

#include <sims3000/energy/EnergySystem.h>
#include <sims3000/energy/EnergyConduitComponent.h>
#include <sims3000/energy/EnergyProducerComponent.h>
#include <sims3000/energy/NexusTypeConfig.h>
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
// is_in_coverage Tests
// =============================================================================

TEST(is_in_coverage_uncovered_cell_returns_false) {
    EnergySystem sys(64, 64);
    // Fresh grid - nothing covered
    ASSERT(!sys.is_in_coverage(10, 10, 1));
    ASSERT(!sys.is_in_coverage(0, 0, 1));
    ASSERT(!sys.is_in_coverage(63, 63, 1));
}

TEST(is_in_coverage_covered_cell_returns_true) {
    EnergySystem sys(64, 64);
    // Manually set coverage via the grid
    sys.get_coverage_grid_mut().set(10, 10, 1);

    ASSERT(sys.is_in_coverage(10, 10, 1));
}

TEST(is_in_coverage_wrong_owner_returns_false) {
    EnergySystem sys(64, 64);
    sys.get_coverage_grid_mut().set(10, 10, 1);

    // Cell is covered by owner 1, not owner 2
    ASSERT(!sys.is_in_coverage(10, 10, 2));
    ASSERT(!sys.is_in_coverage(10, 10, 3));
}

TEST(is_in_coverage_out_of_bounds_returns_false) {
    EnergySystem sys(64, 64);
    // Out of bounds should return false, not crash
    ASSERT(!sys.is_in_coverage(64, 0, 1));
    ASSERT(!sys.is_in_coverage(0, 64, 1));
    ASSERT(!sys.is_in_coverage(100, 100, 1));
    ASSERT(!sys.is_in_coverage(999, 999, 1));
}

TEST(is_in_coverage_after_bfs_recalculate) {
    EnergySystem sys(128, 128);
    // Register a nexus and recalculate coverage
    sys.register_nexus(100, 0);
    sys.register_nexus_position(100, 0, 50, 50);
    sys.recalculate_coverage(0);

    // owner_id in grid = player_id + 1 = 1
    // Default nexus radius = 8, so center should be covered
    ASSERT(sys.is_in_coverage(50, 50, 1));
    // Edge of radius
    ASSERT(sys.is_in_coverage(42, 42, 1));
    ASSERT(sys.is_in_coverage(58, 58, 1));
    // Just outside
    ASSERT(!sys.is_in_coverage(41, 50, 1));
    ASSERT(!sys.is_in_coverage(59, 50, 1));
}

TEST(is_in_coverage_const_method) {
    EnergySystem sys(64, 64);
    sys.get_coverage_grid_mut().set(5, 5, 2);

    // Call through a const reference to verify const-correctness
    const EnergySystem& const_sys = sys;
    ASSERT(const_sys.is_in_coverage(5, 5, 2));
    ASSERT(!const_sys.is_in_coverage(5, 5, 1));
}

// =============================================================================
// get_coverage_at Tests
// =============================================================================

TEST(get_coverage_at_uncovered_returns_zero) {
    EnergySystem sys(64, 64);
    ASSERT_EQ(sys.get_coverage_at(10, 10), 0);
    ASSERT_EQ(sys.get_coverage_at(0, 0), 0);
}

TEST(get_coverage_at_covered_returns_owner) {
    EnergySystem sys(64, 64);
    sys.get_coverage_grid_mut().set(10, 10, 3);
    ASSERT_EQ(sys.get_coverage_at(10, 10), 3);
}

TEST(get_coverage_at_multiple_owners) {
    EnergySystem sys(64, 64);
    sys.get_coverage_grid_mut().set(10, 10, 1);
    sys.get_coverage_grid_mut().set(20, 20, 2);
    sys.get_coverage_grid_mut().set(30, 30, 3);
    sys.get_coverage_grid_mut().set(40, 40, 4);

    ASSERT_EQ(sys.get_coverage_at(10, 10), 1);
    ASSERT_EQ(sys.get_coverage_at(20, 20), 2);
    ASSERT_EQ(sys.get_coverage_at(30, 30), 3);
    ASSERT_EQ(sys.get_coverage_at(40, 40), 4);
}

TEST(get_coverage_at_out_of_bounds_returns_zero) {
    EnergySystem sys(64, 64);
    ASSERT_EQ(sys.get_coverage_at(64, 0), 0);
    ASSERT_EQ(sys.get_coverage_at(0, 64), 0);
    ASSERT_EQ(sys.get_coverage_at(200, 200), 0);
}

TEST(get_coverage_at_after_bfs_recalculate) {
    EnergySystem sys(128, 128);
    // Player 0 nexus
    sys.register_nexus(100, 0);
    sys.register_nexus_position(100, 0, 30, 30);
    sys.recalculate_coverage(0);

    // Player 1 nexus
    sys.register_nexus(200, 1);
    sys.register_nexus_position(200, 1, 80, 80);
    sys.recalculate_coverage(1);

    // owner_id = player_id + 1
    ASSERT_EQ(sys.get_coverage_at(30, 30), 1);  // player 0 -> owner_id 1
    ASSERT_EQ(sys.get_coverage_at(80, 80), 2);  // player 1 -> owner_id 2
    ASSERT_EQ(sys.get_coverage_at(60, 60), 0);  // gap between - uncovered
}

TEST(get_coverage_at_const_method) {
    EnergySystem sys(64, 64);
    sys.get_coverage_grid_mut().set(5, 5, 2);

    const EnergySystem& const_sys = sys;
    ASSERT_EQ(const_sys.get_coverage_at(5, 5), 2);
    ASSERT_EQ(const_sys.get_coverage_at(0, 0), 0);
}

// =============================================================================
// get_coverage_count Tests
// =============================================================================

TEST(get_coverage_count_empty_grid_returns_zero) {
    EnergySystem sys(64, 64);
    ASSERT_EQ(sys.get_coverage_count(1), 0u);
    ASSERT_EQ(sys.get_coverage_count(2), 0u);
    ASSERT_EQ(sys.get_coverage_count(3), 0u);
    ASSERT_EQ(sys.get_coverage_count(4), 0u);
}

TEST(get_coverage_count_single_cell) {
    EnergySystem sys(64, 64);
    sys.get_coverage_grid_mut().set(10, 10, 1);
    ASSERT_EQ(sys.get_coverage_count(1), 1u);
    ASSERT_EQ(sys.get_coverage_count(2), 0u);
}

TEST(get_coverage_count_multiple_cells_same_owner) {
    EnergySystem sys(64, 64);
    sys.get_coverage_grid_mut().set(10, 10, 1);
    sys.get_coverage_grid_mut().set(11, 10, 1);
    sys.get_coverage_grid_mut().set(12, 10, 1);
    sys.get_coverage_grid_mut().set(13, 10, 1);
    sys.get_coverage_grid_mut().set(14, 10, 1);
    ASSERT_EQ(sys.get_coverage_count(1), 5u);
}

TEST(get_coverage_count_multiple_owners) {
    EnergySystem sys(64, 64);
    // Owner 1: 3 cells
    sys.get_coverage_grid_mut().set(10, 10, 1);
    sys.get_coverage_grid_mut().set(11, 10, 1);
    sys.get_coverage_grid_mut().set(12, 10, 1);
    // Owner 2: 2 cells
    sys.get_coverage_grid_mut().set(20, 20, 2);
    sys.get_coverage_grid_mut().set(21, 20, 2);

    ASSERT_EQ(sys.get_coverage_count(1), 3u);
    ASSERT_EQ(sys.get_coverage_count(2), 2u);
    ASSERT_EQ(sys.get_coverage_count(3), 0u);
}

TEST(get_coverage_count_after_bfs) {
    EnergySystem sys(128, 128);
    sys.register_nexus(100, 0);
    sys.register_nexus_position(100, 0, 50, 50);
    sys.recalculate_coverage(0);

    // Default nexus radius = 8 -> 17x17 = 289 cells
    ASSERT_EQ(sys.get_coverage_count(1), 17u * 17u);
}

TEST(get_coverage_count_after_coverage_cleared) {
    EnergySystem sys(128, 128);
    sys.register_nexus(100, 0);
    sys.register_nexus_position(100, 0, 50, 50);
    sys.recalculate_coverage(0);
    ASSERT_EQ(sys.get_coverage_count(1), 17u * 17u);

    // Remove nexus and recalculate - should clear coverage
    sys.unregister_nexus(100, 0);
    sys.unregister_nexus_position(100, 0, 50, 50);
    sys.recalculate_coverage(0);
    ASSERT_EQ(sys.get_coverage_count(1), 0u);
}

TEST(get_coverage_count_const_method) {
    EnergySystem sys(64, 64);
    sys.get_coverage_grid_mut().set(5, 5, 1);
    sys.get_coverage_grid_mut().set(6, 5, 1);

    const EnergySystem& const_sys = sys;
    ASSERT_EQ(const_sys.get_coverage_count(1), 2u);
}

// =============================================================================
// Integration: queries work consistently with BFS and conduits
// =============================================================================

TEST(queries_consistent_with_conduit_extended_coverage) {
    EnergySystem sys(128, 128);
    entt::registry registry;

    // Create nexus with Wind type (radius 4)
    auto nexus_ent = registry.create();
    EnergyProducerComponent producer;
    producer.nexus_type = static_cast<uint8_t>(NexusType::Wind);
    producer.is_online = true;
    registry.emplace<EnergyProducerComponent>(nexus_ent, producer);
    uint32_t nexus_id = static_cast<uint32_t>(nexus_ent);

    // Create conduit with radius 3
    auto cond_ent = registry.create();
    EnergyConduitComponent conduit;
    conduit.coverage_radius = 3;
    registry.emplace<EnergyConduitComponent>(cond_ent, conduit);
    uint32_t cond_id = static_cast<uint32_t>(cond_ent);

    sys.set_registry(&registry);

    // Nexus at (50,50), conduit at (51,50) - adjacent
    sys.register_nexus(nexus_id, 0);
    sys.register_nexus_position(nexus_id, 0, 50, 50);
    sys.register_conduit_position(cond_id, 0, 51, 50);

    sys.recalculate_coverage(0);

    // owner_id = 1
    // Nexus Wind radius 4: [46,54] x [46,54] = 81 cells
    // Conduit at (51,50) radius 3: [48,54] x [47,53] - mostly overlaps nexus

    // is_in_coverage and get_coverage_at should agree
    ASSERT(sys.is_in_coverage(50, 50, 1));
    ASSERT_EQ(sys.get_coverage_at(50, 50), 1);

    // Conduit extends coverage
    ASSERT(sys.is_in_coverage(54, 50, 1)); // nexus edge
    ASSERT_EQ(sys.get_coverage_at(54, 50), 1);

    // Coverage count should be > 0
    uint32_t count = sys.get_coverage_count(1);
    ASSERT(count > 0);

    // Outside all coverage
    ASSERT(!sys.is_in_coverage(0, 0, 1));
    ASSERT_EQ(sys.get_coverage_at(0, 0), 0);
}

TEST(queries_return_zero_for_owner_zero) {
    // Owner 0 in the grid means "uncovered" - querying for owner 0
    // should reflect the grid semantics
    EnergySystem sys(64, 64);
    sys.get_coverage_grid_mut().set(10, 10, 1);

    // is_in_coverage with owner=0 checks if grid cell equals 0
    // The cell at (10,10) is set to 1, so is_in_coverage(10,10,0) should be false
    ASSERT(!sys.is_in_coverage(10, 10, 0));

    // An uncovered cell should match owner 0
    ASSERT(sys.is_in_coverage(20, 20, 0));

    // get_coverage_count(0) counts cells with value 0
    // On a 64x64 grid with 1 cell set, that's (64*64 - 1) = 4095 cells with value 0
    ASSERT_EQ(sys.get_coverage_count(0), 64u * 64u - 1u);
}

// =============================================================================
// Main Entry Point
// =============================================================================

int main() {
    printf("=== Coverage Query Unit Tests (Ticket 5-017) ===\n\n");

    // is_in_coverage tests
    RUN_TEST(is_in_coverage_uncovered_cell_returns_false);
    RUN_TEST(is_in_coverage_covered_cell_returns_true);
    RUN_TEST(is_in_coverage_wrong_owner_returns_false);
    RUN_TEST(is_in_coverage_out_of_bounds_returns_false);
    RUN_TEST(is_in_coverage_after_bfs_recalculate);
    RUN_TEST(is_in_coverage_const_method);

    // get_coverage_at tests
    RUN_TEST(get_coverage_at_uncovered_returns_zero);
    RUN_TEST(get_coverage_at_covered_returns_owner);
    RUN_TEST(get_coverage_at_multiple_owners);
    RUN_TEST(get_coverage_at_out_of_bounds_returns_zero);
    RUN_TEST(get_coverage_at_after_bfs_recalculate);
    RUN_TEST(get_coverage_at_const_method);

    // get_coverage_count tests
    RUN_TEST(get_coverage_count_empty_grid_returns_zero);
    RUN_TEST(get_coverage_count_single_cell);
    RUN_TEST(get_coverage_count_multiple_cells_same_owner);
    RUN_TEST(get_coverage_count_multiple_owners);
    RUN_TEST(get_coverage_count_after_bfs);
    RUN_TEST(get_coverage_count_after_coverage_cleared);
    RUN_TEST(get_coverage_count_const_method);

    // Integration tests
    RUN_TEST(queries_consistent_with_conduit_extended_coverage);
    RUN_TEST(queries_return_zero_for_owner_zero);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
