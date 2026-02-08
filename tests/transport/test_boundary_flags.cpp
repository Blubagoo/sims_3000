/**
 * @file test_boundary_flags.cpp
 * @brief Unit tests for BoundaryFlags (Epic 7, Ticket E7-028)
 *
 * Tests:
 * - No neighbors -> no flags
 * - Same owner neighbors -> no flags
 * - Different owner on each side
 * - Multiple boundary edges
 * - Owner 0 (no pathway) does NOT trigger boundary
 * - All sides different owner
 */

#include <sims3000/transport/BoundaryFlags.h>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <functional>

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
        printf("\n  FAILED: %s == %s (line %d, got %lld vs %lld)\n", \
               #a, #b, __LINE__, (long long)(a), (long long)(b)); \
        tests_failed++; \
        return; \
    } \
} while(0)

// ============================================================================
// No neighbors (all return 0 = no pathway)
// ============================================================================

TEST(no_neighbors_no_flags) {
    auto owner_at = [](int32_t, int32_t) -> uint8_t { return 0; };

    uint8_t flags = calculate_boundary_flags(5, 5, 1, owner_at);
    ASSERT_EQ(flags, 0u);
}

// ============================================================================
// All same owner -> no flags
// ============================================================================

TEST(same_owner_no_flags) {
    auto owner_at = [](int32_t, int32_t) -> uint8_t { return 1; };

    uint8_t flags = calculate_boundary_flags(5, 5, 1, owner_at);
    ASSERT_EQ(flags, 0u);
}

// ============================================================================
// North neighbor different owner
// ============================================================================

TEST(north_boundary) {
    auto owner_at = [](int32_t x, int32_t y) -> uint8_t {
        if (x == 5 && y == 4) return 2;  // North neighbor has different owner
        return 1;  // Same owner elsewhere
    };

    uint8_t flags = calculate_boundary_flags(5, 5, 1, owner_at);
    ASSERT_EQ(flags, 1u);  // North = bit 0
}

// ============================================================================
// South neighbor different owner
// ============================================================================

TEST(south_boundary) {
    auto owner_at = [](int32_t x, int32_t y) -> uint8_t {
        if (x == 5 && y == 6) return 2;  // South neighbor has different owner
        return 1;
    };

    uint8_t flags = calculate_boundary_flags(5, 5, 1, owner_at);
    ASSERT_EQ(flags, 2u);  // South = bit 1
}

// ============================================================================
// East neighbor different owner
// ============================================================================

TEST(east_boundary) {
    auto owner_at = [](int32_t x, int32_t y) -> uint8_t {
        if (x == 6 && y == 5) return 3;  // East neighbor has different owner
        return 1;
    };

    uint8_t flags = calculate_boundary_flags(5, 5, 1, owner_at);
    ASSERT_EQ(flags, 4u);  // East = bit 2
}

// ============================================================================
// West neighbor different owner
// ============================================================================

TEST(west_boundary) {
    auto owner_at = [](int32_t x, int32_t y) -> uint8_t {
        if (x == 4 && y == 5) return 2;  // West neighbor has different owner
        return 1;
    };

    uint8_t flags = calculate_boundary_flags(5, 5, 1, owner_at);
    ASSERT_EQ(flags, 8u);  // West = bit 3
}

// ============================================================================
// Multiple boundary edges
// ============================================================================

TEST(north_and_east_boundary) {
    auto owner_at = [](int32_t x, int32_t y) -> uint8_t {
        if (x == 5 && y == 4) return 2;  // North
        if (x == 6 && y == 5) return 3;  // East
        return 1;
    };

    uint8_t flags = calculate_boundary_flags(5, 5, 1, owner_at);
    ASSERT_EQ(flags, 5u);  // N(1) + E(4) = 5
}

// ============================================================================
// All sides different owner
// ============================================================================

TEST(all_sides_boundary) {
    auto owner_at = [](int32_t, int32_t) -> uint8_t { return 2; };

    uint8_t flags = calculate_boundary_flags(5, 5, 1, owner_at);
    ASSERT_EQ(flags, 15u);  // N(1) + S(2) + E(4) + W(8) = 15
}

// ============================================================================
// Owner 0 (no pathway) does NOT trigger boundary
// ============================================================================

TEST(owner_zero_no_boundary) {
    // All neighbors have owner 0 (no pathway)
    auto owner_at = [](int32_t, int32_t) -> uint8_t { return 0; };

    uint8_t flags = calculate_boundary_flags(5, 5, 1, owner_at);
    ASSERT_EQ(flags, 0u);
}

TEST(mixed_zero_and_different_owner) {
    auto owner_at = [](int32_t x, int32_t y) -> uint8_t {
        if (x == 5 && y == 4) return 0;  // North: no pathway (no boundary)
        if (x == 5 && y == 6) return 2;  // South: different owner (boundary)
        if (x == 6 && y == 5) return 0;  // East: no pathway (no boundary)
        if (x == 4 && y == 5) return 1;  // West: same owner (no boundary)
        return 0;
    };

    uint8_t flags = calculate_boundary_flags(5, 5, 1, owner_at);
    ASSERT_EQ(flags, 2u);  // Only South
}

// ============================================================================
// PathwayRenderData struct usage
// ============================================================================

TEST(render_data_struct) {
    PathwayRenderData data;
    data.x = 10;
    data.y = 20;
    data.type = PathwayType::BasicPathway;
    data.health = 255;
    data.congestion_level = 50;
    data.owner = 1;
    data.boundary_flags = 0;

    // Calculate boundary flags
    auto owner_at = [](int32_t x, int32_t y) -> uint8_t {
        if (x == 11 && y == 20) return 2;  // East neighbor
        return 0;
    };

    data.boundary_flags = calculate_boundary_flags(data.x, data.y, data.owner, owner_at);
    ASSERT_EQ(data.boundary_flags, 4u);  // East boundary
    ASSERT_EQ(data.x, 10);
    ASSERT_EQ(data.y, 20);
}

// ============================================================================
// Self-owner matches all neighbors
// ============================================================================

TEST(owner_2_with_all_same) {
    auto owner_at = [](int32_t, int32_t) -> uint8_t { return 2; };

    uint8_t flags = calculate_boundary_flags(5, 5, 2, owner_at);
    ASSERT_EQ(flags, 0u);  // All same, no boundary
}

// ============================================================================
// Main
// ============================================================================

int main() {
    printf("=== BoundaryFlags Unit Tests (Ticket E7-028) ===\n\n");

    RUN_TEST(no_neighbors_no_flags);
    RUN_TEST(same_owner_no_flags);
    RUN_TEST(north_boundary);
    RUN_TEST(south_boundary);
    RUN_TEST(east_boundary);
    RUN_TEST(west_boundary);
    RUN_TEST(north_and_east_boundary);
    RUN_TEST(all_sides_boundary);
    RUN_TEST(owner_zero_no_boundary);
    RUN_TEST(mixed_zero_and_different_owner);
    RUN_TEST(render_data_struct);
    RUN_TEST(owner_2_with_all_same);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
