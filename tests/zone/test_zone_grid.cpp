/**
 * @file test_zone_grid.cpp
 * @brief Unit tests for ZoneGrid sparse spatial index (Ticket 4-006)
 *
 * Tests cover:
 * - ZoneGrid initialization (128x128, 256x256, 512x512)
 * - Memory budget verification
 * - Bounds checking
 * - Zone placement and removal
 * - Zone query operations
 * - Overlap prevention
 * - Edge-of-map sectors
 */

#include <sims3000/zone/ZoneGrid.h>
#include <cstdio>
#include <cstdlib>

using namespace sims3000::zone;

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
// ZoneGrid Initialization Tests
// =============================================================================

TEST(zone_grid_default_constructor) {
    ZoneGrid grid;
    ASSERT(grid.empty());
    ASSERT_EQ(grid.getWidth(), 0);
    ASSERT_EQ(grid.getHeight(), 0);
}

TEST(zone_grid_parameterized_constructor_128) {
    ZoneGrid grid(128, 128);
    ASSERT(!grid.empty());
    ASSERT_EQ(grid.getWidth(), 128);
    ASSERT_EQ(grid.getHeight(), 128);
    ASSERT_EQ(grid.cell_count(), 16384);
    ASSERT_EQ(grid.memory_bytes(), 65536); // 16384 * 4 bytes
}

TEST(zone_grid_parameterized_constructor_256) {
    ZoneGrid grid(256, 256);
    ASSERT(!grid.empty());
    ASSERT_EQ(grid.getWidth(), 256);
    ASSERT_EQ(grid.getHeight(), 256);
    ASSERT_EQ(grid.cell_count(), 65536);
    ASSERT_EQ(grid.memory_bytes(), 262144); // 65536 * 4 bytes
}

TEST(zone_grid_parameterized_constructor_512) {
    ZoneGrid grid(512, 512);
    ASSERT(!grid.empty());
    ASSERT_EQ(grid.getWidth(), 512);
    ASSERT_EQ(grid.getHeight(), 512);
    ASSERT_EQ(grid.cell_count(), 262144);
    ASSERT_EQ(grid.memory_bytes(), 1048576); // 262144 * 4 bytes (1MB)
}

TEST(zone_grid_initialize) {
    ZoneGrid grid;
    grid.initialize(256, 256);
    ASSERT(!grid.empty());
    ASSERT_EQ(grid.getWidth(), 256);
    ASSERT_EQ(grid.getHeight(), 256);
}

// =============================================================================
// ZoneGrid Bounds Checking Tests
// =============================================================================

TEST(zone_grid_in_bounds) {
    ZoneGrid grid(128, 128);

    // Valid coordinates
    ASSERT(grid.in_bounds(0, 0));
    ASSERT(grid.in_bounds(127, 127));
    ASSERT(grid.in_bounds(64, 64));

    // Invalid coordinates
    ASSERT(!grid.in_bounds(-1, 0));
    ASSERT(!grid.in_bounds(0, -1));
    ASSERT(!grid.in_bounds(128, 0));
    ASSERT(!grid.in_bounds(0, 128));
    ASSERT(!grid.in_bounds(128, 128));
}

// =============================================================================
// ZoneGrid Placement Tests
// =============================================================================

TEST(zone_grid_place_zone_success) {
    ZoneGrid grid(128, 128);

    // Place zone at (10, 20)
    ASSERT(grid.place_zone(10, 20, 1234));
    ASSERT_EQ(grid.get_zone_at(10, 20), 1234);
    ASSERT(grid.has_zone_at(10, 20));
}

TEST(zone_grid_place_zone_out_of_bounds) {
    ZoneGrid grid(128, 128);

    // Cannot place out of bounds
    ASSERT(!grid.place_zone(-1, 0, 1234));
    ASSERT(!grid.place_zone(0, -1, 1234));
    ASSERT(!grid.place_zone(128, 0, 1234));
    ASSERT(!grid.place_zone(0, 128, 1234));
}

TEST(zone_grid_place_zone_overlap_prevention) {
    ZoneGrid grid(128, 128);

    // Place first zone
    ASSERT(grid.place_zone(10, 20, 1234));

    // Cannot place second zone at same location
    ASSERT(!grid.place_zone(10, 20, 5678));

    // Original zone is still there
    ASSERT_EQ(grid.get_zone_at(10, 20), 1234);
}

// =============================================================================
// ZoneGrid Removal Tests
// =============================================================================

TEST(zone_grid_remove_zone_success) {
    ZoneGrid grid(128, 128);

    // Place and remove zone
    ASSERT(grid.place_zone(10, 20, 1234));
    ASSERT(grid.remove_zone(10, 20));
    ASSERT_EQ(grid.get_zone_at(10, 20), INVALID_ENTITY);
    ASSERT(!grid.has_zone_at(10, 20));
}

TEST(zone_grid_remove_zone_out_of_bounds) {
    ZoneGrid grid(128, 128);

    // Cannot remove out of bounds
    ASSERT(!grid.remove_zone(-1, 0));
    ASSERT(!grid.remove_zone(0, -1));
    ASSERT(!grid.remove_zone(128, 0));
    ASSERT(!grid.remove_zone(0, 128));
}

TEST(zone_grid_remove_zone_no_zone_present) {
    ZoneGrid grid(128, 128);

    // Cannot remove if no zone present
    ASSERT(!grid.remove_zone(10, 20));
}

// =============================================================================
// ZoneGrid Query Tests
// =============================================================================

TEST(zone_grid_get_zone_at) {
    ZoneGrid grid(128, 128);

    // Empty cell returns INVALID_ENTITY
    ASSERT_EQ(grid.get_zone_at(10, 20), INVALID_ENTITY);

    // Place zone and query
    grid.place_zone(10, 20, 9999);
    ASSERT_EQ(grid.get_zone_at(10, 20), 9999);

    // Out of bounds returns INVALID_ENTITY
    ASSERT_EQ(grid.get_zone_at(-1, 0), INVALID_ENTITY);
    ASSERT_EQ(grid.get_zone_at(128, 0), INVALID_ENTITY);
}

TEST(zone_grid_has_zone_at) {
    ZoneGrid grid(128, 128);

    // Empty cell
    ASSERT(!grid.has_zone_at(10, 20));

    // Place zone
    grid.place_zone(10, 20, 1234);
    ASSERT(grid.has_zone_at(10, 20));

    // Remove zone
    grid.remove_zone(10, 20);
    ASSERT(!grid.has_zone_at(10, 20));
}

// =============================================================================
// ZoneGrid Edge-of-Map Tests
// =============================================================================

TEST(zone_grid_edge_of_map_corners) {
    ZoneGrid grid(128, 128);

    // Top-left corner (0, 0)
    ASSERT(grid.place_zone(0, 0, 1));
    ASSERT_EQ(grid.get_zone_at(0, 0), 1);

    // Top-right corner (127, 0)
    ASSERT(grid.place_zone(127, 0, 2));
    ASSERT_EQ(grid.get_zone_at(127, 0), 2);

    // Bottom-left corner (0, 127)
    ASSERT(grid.place_zone(0, 127, 3));
    ASSERT_EQ(grid.get_zone_at(0, 127), 3);

    // Bottom-right corner (127, 127)
    ASSERT(grid.place_zone(127, 127, 4));
    ASSERT_EQ(grid.get_zone_at(127, 127), 4);
}

// =============================================================================
// ZoneGrid Clear Tests
// =============================================================================

TEST(zone_grid_clear_all) {
    ZoneGrid grid(128, 128);

    // Place multiple zones
    grid.place_zone(10, 20, 1);
    grid.place_zone(30, 40, 2);
    grid.place_zone(50, 60, 3);

    // Clear all
    grid.clear_all();

    // All zones should be removed
    ASSERT(!grid.has_zone_at(10, 20));
    ASSERT(!grid.has_zone_at(30, 40));
    ASSERT(!grid.has_zone_at(50, 60));
}

// =============================================================================
// ZoneGrid Row-Major Layout Tests
// =============================================================================

TEST(zone_grid_row_major_layout) {
    ZoneGrid grid(128, 128);

    // Row-major: index = y * width + x
    // Place zone at (5, 3) -> index = 3 * 128 + 5 = 389
    grid.place_zone(5, 3, 1234);
    ASSERT_EQ(grid.get_zone_at(5, 3), 1234);

    // Verify adjacent cells are independent
    ASSERT_EQ(grid.get_zone_at(4, 3), INVALID_ENTITY);
    ASSERT_EQ(grid.get_zone_at(6, 3), INVALID_ENTITY);
    ASSERT_EQ(grid.get_zone_at(5, 2), INVALID_ENTITY);
    ASSERT_EQ(grid.get_zone_at(5, 4), INVALID_ENTITY);
}

// =============================================================================
// Main
// =============================================================================

int main() {
    printf("Running ZoneGrid tests...\n\n");

    RUN_TEST(zone_grid_default_constructor);
    RUN_TEST(zone_grid_parameterized_constructor_128);
    RUN_TEST(zone_grid_parameterized_constructor_256);
    RUN_TEST(zone_grid_parameterized_constructor_512);
    RUN_TEST(zone_grid_initialize);
    RUN_TEST(zone_grid_in_bounds);
    RUN_TEST(zone_grid_place_zone_success);
    RUN_TEST(zone_grid_place_zone_out_of_bounds);
    RUN_TEST(zone_grid_place_zone_overlap_prevention);
    RUN_TEST(zone_grid_remove_zone_success);
    RUN_TEST(zone_grid_remove_zone_out_of_bounds);
    RUN_TEST(zone_grid_remove_zone_no_zone_present);
    RUN_TEST(zone_grid_get_zone_at);
    RUN_TEST(zone_grid_has_zone_at);
    RUN_TEST(zone_grid_edge_of_map_corners);
    RUN_TEST(zone_grid_clear_all);
    RUN_TEST(zone_grid_row_major_layout);

    printf("\n");
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_failed);

    return tests_failed > 0 ? 1 : 0;
}
