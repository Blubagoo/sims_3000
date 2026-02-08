/**
 * @file test_subterra_placement.cpp
 * @brief Unit tests for subterra placement rules (Epic 7, Ticket E7-044)
 *
 * Tests:
 * - Enhanced can_build_subterra_at(x, y, require_adjacent):
 *   - Negative coordinates rejected
 *   - Out of bounds rejected
 *   - Already occupied rejected
 *   - First placement allowed on empty grid (require_adjacent=true)
 *   - Subsequent placement requires adjacent subterra (N/S/E/W)
 *   - require_adjacent=false skips adjacency check
 *   - Diagonal neighbors do not satisfy adjacency requirement
 */

#include <sims3000/transport/SubterraLayerManager.h>
#include <cassert>
#include <cstdio>
#include <cstdlib>

using namespace sims3000::transport;

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
        printf("\n  FAILED: %s == %s (line %d, got %d vs %d)\n", \
               #a, #b, __LINE__, (int)(a), (int)(b)); \
        tests_failed++; \
        return; \
    } \
} while(0)

// ============================================================================
// Negative coordinates
// ============================================================================

TEST(negative_x_rejected) {
    SubterraLayerManager mgr(16, 16);
    ASSERT(mgr.can_build_subterra_at(-1, 5, true) == false);
    ASSERT(mgr.can_build_subterra_at(-1, 5, false) == false);
}

TEST(negative_y_rejected) {
    SubterraLayerManager mgr(16, 16);
    ASSERT(mgr.can_build_subterra_at(5, -1, true) == false);
    ASSERT(mgr.can_build_subterra_at(5, -1, false) == false);
}

TEST(negative_both_rejected) {
    SubterraLayerManager mgr(16, 16);
    ASSERT(mgr.can_build_subterra_at(-3, -7, true) == false);
}

// ============================================================================
// Out of bounds
// ============================================================================

TEST(out_of_bounds_x_rejected) {
    SubterraLayerManager mgr(16, 16);
    ASSERT(mgr.can_build_subterra_at(16, 5, true) == false);
    ASSERT(mgr.can_build_subterra_at(100, 5, false) == false);
}

TEST(out_of_bounds_y_rejected) {
    SubterraLayerManager mgr(16, 16);
    ASSERT(mgr.can_build_subterra_at(5, 16, true) == false);
    ASSERT(mgr.can_build_subterra_at(5, 100, false) == false);
}

// ============================================================================
// Already occupied
// ============================================================================

TEST(occupied_cell_rejected) {
    SubterraLayerManager mgr(16, 16);
    mgr.set_subterra(5, 5, 42);

    ASSERT(mgr.can_build_subterra_at(5, 5, true) == false);
    ASSERT(mgr.can_build_subterra_at(5, 5, false) == false);
}

// ============================================================================
// First placement on empty grid
// ============================================================================

TEST(first_placement_empty_grid_allowed) {
    SubterraLayerManager mgr(16, 16);

    // Grid is empty, first placement should be allowed even with require_adjacent
    ASSERT(mgr.can_build_subterra_at(8, 8, true) == true);
}

TEST(first_placement_at_corner_allowed) {
    SubterraLayerManager mgr(16, 16);

    ASSERT(mgr.can_build_subterra_at(0, 0, true) == true);
    ASSERT(mgr.can_build_subterra_at(15, 15, true) == true);
}

// ============================================================================
// Adjacency requirement
// ============================================================================

TEST(adjacent_north_accepted) {
    SubterraLayerManager mgr(16, 16);
    mgr.set_subterra(5, 4, 1);  // North neighbor

    ASSERT(mgr.can_build_subterra_at(5, 5, true) == true);
}

TEST(adjacent_south_accepted) {
    SubterraLayerManager mgr(16, 16);
    mgr.set_subterra(5, 6, 1);  // South neighbor

    ASSERT(mgr.can_build_subterra_at(5, 5, true) == true);
}

TEST(adjacent_east_accepted) {
    SubterraLayerManager mgr(16, 16);
    mgr.set_subterra(6, 5, 1);  // East neighbor

    ASSERT(mgr.can_build_subterra_at(5, 5, true) == true);
}

TEST(adjacent_west_accepted) {
    SubterraLayerManager mgr(16, 16);
    mgr.set_subterra(4, 5, 1);  // West neighbor

    ASSERT(mgr.can_build_subterra_at(5, 5, true) == true);
}

TEST(diagonal_only_rejected) {
    SubterraLayerManager mgr(16, 16);
    // Only diagonal neighbors
    mgr.set_subterra(4, 4, 1);  // NW
    mgr.set_subterra(6, 4, 2);  // NE
    mgr.set_subterra(4, 6, 3);  // SW
    mgr.set_subterra(6, 6, 4);  // SE

    // No cardinal neighbor - should be rejected
    ASSERT(mgr.can_build_subterra_at(5, 5, true) == false);
}

TEST(no_adjacent_non_empty_grid_rejected) {
    SubterraLayerManager mgr(16, 16);
    // Place something far away so grid is not empty
    mgr.set_subterra(0, 0, 99);

    // (10, 10) has no adjacent subterra
    ASSERT(mgr.can_build_subterra_at(10, 10, true) == false);
}

// ============================================================================
// require_adjacent = false
// ============================================================================

TEST(require_adjacent_false_skips_check) {
    SubterraLayerManager mgr(16, 16);
    // Place something far away
    mgr.set_subterra(0, 0, 99);

    // (10, 10) has no adjacent subterra, but require_adjacent=false
    ASSERT(mgr.can_build_subterra_at(10, 10, false) == true);
}

TEST(require_adjacent_false_still_checks_bounds) {
    SubterraLayerManager mgr(16, 16);

    ASSERT(mgr.can_build_subterra_at(-1, 5, false) == false);
    ASSERT(mgr.can_build_subterra_at(16, 5, false) == false);
}

TEST(require_adjacent_false_still_checks_occupied) {
    SubterraLayerManager mgr(16, 16);
    mgr.set_subterra(5, 5, 42);

    ASSERT(mgr.can_build_subterra_at(5, 5, false) == false);
}

// ============================================================================
// Edge cases: building chain
// ============================================================================

TEST(chain_building_works) {
    SubterraLayerManager mgr(16, 16);

    // First placement
    ASSERT(mgr.can_build_subterra_at(5, 5, true) == true);
    mgr.set_subterra(5, 5, 1);

    // Second placement adjacent
    ASSERT(mgr.can_build_subterra_at(6, 5, true) == true);
    mgr.set_subterra(6, 5, 2);

    // Third placement adjacent to second
    ASSERT(mgr.can_build_subterra_at(7, 5, true) == true);
    mgr.set_subterra(7, 5, 3);

    // Non-adjacent placement should fail
    ASSERT(mgr.can_build_subterra_at(10, 10, true) == false);
}

// ============================================================================
// Zero-dimension grid
// ============================================================================

TEST(zero_dimension_grid_rejects_all) {
    SubterraLayerManager mgr(0, 0);

    ASSERT(mgr.can_build_subterra_at(0, 0, true) == false);
    ASSERT(mgr.can_build_subterra_at(0, 0, false) == false);
}

// ============================================================================
// Boundary cells with adjacency
// ============================================================================

TEST(corner_cell_with_adjacent) {
    SubterraLayerManager mgr(16, 16);
    mgr.set_subterra(1, 0, 1);  // East neighbor of (0,0)

    ASSERT(mgr.can_build_subterra_at(0, 0, true) == true);
}

TEST(edge_cell_with_adjacent) {
    SubterraLayerManager mgr(16, 16);
    mgr.set_subterra(15, 14, 1);  // North neighbor of (15,15)

    ASSERT(mgr.can_build_subterra_at(15, 15, true) == true);
}

// ============================================================================
// Main
// ============================================================================

int main() {
    printf("=== Subterra Placement Tests (Ticket E7-044) ===\n\n");

    // Negative coordinates
    RUN_TEST(negative_x_rejected);
    RUN_TEST(negative_y_rejected);
    RUN_TEST(negative_both_rejected);

    // Out of bounds
    RUN_TEST(out_of_bounds_x_rejected);
    RUN_TEST(out_of_bounds_y_rejected);

    // Occupied
    RUN_TEST(occupied_cell_rejected);

    // First placement
    RUN_TEST(first_placement_empty_grid_allowed);
    RUN_TEST(first_placement_at_corner_allowed);

    // Adjacency
    RUN_TEST(adjacent_north_accepted);
    RUN_TEST(adjacent_south_accepted);
    RUN_TEST(adjacent_east_accepted);
    RUN_TEST(adjacent_west_accepted);
    RUN_TEST(diagonal_only_rejected);
    RUN_TEST(no_adjacent_non_empty_grid_rejected);

    // require_adjacent = false
    RUN_TEST(require_adjacent_false_skips_check);
    RUN_TEST(require_adjacent_false_still_checks_bounds);
    RUN_TEST(require_adjacent_false_still_checks_occupied);

    // Chain building
    RUN_TEST(chain_building_works);

    // Zero dimension
    RUN_TEST(zero_dimension_grid_rejects_all);

    // Boundary
    RUN_TEST(corner_cell_with_adjacent);
    RUN_TEST(edge_cell_with_adjacent);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
