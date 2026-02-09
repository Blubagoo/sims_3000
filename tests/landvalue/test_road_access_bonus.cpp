/**
 * @file test_road_access_bonus.cpp
 * @brief Unit tests for RoadAccessBonus (Ticket E10-102)
 *
 * Tests cover:
 * - On road: +20
 * - Distance 1: +15
 * - Distance 2: +10
 * - Distance 3: +5
 * - Beyond 3: +0
 * - apply_road_bonuses updates grid
 * - Values clamped to 0-255
 */

#include <sims3000/landvalue/RoadAccessBonus.h>
#include <sims3000/landvalue/LandValueGrid.h>
#include <cstdio>
#include <cstdlib>
#include <vector>

using namespace sims3000::landvalue;

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
        printf("\n  FAILED: %s == %s (%d != %d) (line %d)\n", \
               #a, #b, static_cast<int>(a), static_cast<int>(b), __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

// =============================================================================
// calculate_road_bonus Tests
// =============================================================================

TEST(on_road_bonus) {
    // Distance 0 (on road): +20
    ASSERT_EQ(calculate_road_bonus(0), static_cast<uint8_t>(20));
}

TEST(distance_1_bonus) {
    // Distance 1 (adjacent): +15
    ASSERT_EQ(calculate_road_bonus(1), static_cast<uint8_t>(15));
}

TEST(distance_2_bonus) {
    // Distance 2: +10
    ASSERT_EQ(calculate_road_bonus(2), static_cast<uint8_t>(10));
}

TEST(distance_3_bonus) {
    // Distance 3: +5
    ASSERT_EQ(calculate_road_bonus(3), static_cast<uint8_t>(5));
}

TEST(distance_4_no_bonus) {
    // Distance 4: +0
    ASSERT_EQ(calculate_road_bonus(4), static_cast<uint8_t>(0));
}

TEST(distance_10_no_bonus) {
    // Distance 10: +0
    ASSERT_EQ(calculate_road_bonus(10), static_cast<uint8_t>(0));
}

TEST(distance_255_no_bonus) {
    // Distance 255 (no road): +0
    ASSERT_EQ(calculate_road_bonus(255), static_cast<uint8_t>(0));
}

// =============================================================================
// apply_road_bonuses Tests
// =============================================================================

TEST(apply_road_bonuses_updates_grid) {
    LandValueGrid grid(16, 16);

    std::vector<RoadDistanceInfo> info;
    info.push_back({5, 5, 0});   // on road
    info.push_back({6, 6, 1});   // distance 1
    info.push_back({7, 7, 2});   // distance 2
    info.push_back({8, 8, 3});   // distance 3

    apply_road_bonuses(grid, info);

    // 128 + 20 = 148
    ASSERT_EQ(grid.get_value(5, 5), static_cast<uint8_t>(148));
    // 128 + 15 = 143
    ASSERT_EQ(grid.get_value(6, 6), static_cast<uint8_t>(143));
    // 128 + 10 = 138
    ASSERT_EQ(grid.get_value(7, 7), static_cast<uint8_t>(138));
    // 128 + 5 = 133
    ASSERT_EQ(grid.get_value(8, 8), static_cast<uint8_t>(133));
}

TEST(apply_road_bonuses_far_tile_unchanged) {
    LandValueGrid grid(16, 16);

    std::vector<RoadDistanceInfo> info;
    info.push_back({5, 5, 10});   // far from road
    info.push_back({6, 6, 255});  // no road at all

    apply_road_bonuses(grid, info);

    // Should remain at 128 (no bonus added)
    ASSERT_EQ(grid.get_value(5, 5), static_cast<uint8_t>(128));
    ASSERT_EQ(grid.get_value(6, 6), static_cast<uint8_t>(128));
}

TEST(apply_road_bonuses_clamps_to_255) {
    LandValueGrid grid(16, 16);
    // Set value near max
    grid.set_value(3, 3, 250);

    std::vector<RoadDistanceInfo> info;
    info.push_back({3, 3, 0});  // on road: +20

    apply_road_bonuses(grid, info);

    // 250 + 20 = 270, clamped to 255
    ASSERT_EQ(grid.get_value(3, 3), static_cast<uint8_t>(255));
}

TEST(apply_road_bonuses_at_max) {
    LandValueGrid grid(16, 16);
    grid.set_value(2, 2, 255);

    std::vector<RoadDistanceInfo> info;
    info.push_back({2, 2, 0});  // on road: +20

    apply_road_bonuses(grid, info);

    // 255 + 20 = 275, clamped to 255
    ASSERT_EQ(grid.get_value(2, 2), static_cast<uint8_t>(255));
}

TEST(apply_road_bonuses_from_zero) {
    LandValueGrid grid(16, 16);
    grid.set_value(1, 1, 0);

    std::vector<RoadDistanceInfo> info;
    info.push_back({1, 1, 0});  // on road: +20

    apply_road_bonuses(grid, info);

    // 0 + 20 = 20
    ASSERT_EQ(grid.get_value(1, 1), static_cast<uint8_t>(20));
}

TEST(apply_road_bonuses_empty_vector) {
    LandValueGrid grid(16, 16);
    std::vector<RoadDistanceInfo> info;

    apply_road_bonuses(grid, info);

    // Grid should remain unchanged (all 128)
    ASSERT_EQ(grid.get_value(0, 0), static_cast<uint8_t>(128));
    ASSERT_EQ(grid.get_value(8, 8), static_cast<uint8_t>(128));
}

TEST(apply_road_bonuses_out_of_bounds_ignored) {
    LandValueGrid grid(16, 16);

    std::vector<RoadDistanceInfo> info;
    info.push_back({-1, 0, 0});   // out of bounds
    info.push_back({16, 0, 0});    // out of bounds
    info.push_back({0, 16, 0});    // out of bounds

    apply_road_bonuses(grid, info);

    // Grid should remain unchanged (out-of-bounds writes are no-ops)
    ASSERT_EQ(grid.get_value(0, 0), static_cast<uint8_t>(128));
}

TEST(apply_road_bonuses_multiple_tiles_same_row) {
    LandValueGrid grid(16, 16);

    std::vector<RoadDistanceInfo> info;
    for (int32_t x = 0; x < 8; ++x) {
        info.push_back({x, 0, static_cast<uint8_t>(x)});
    }

    apply_road_bonuses(grid, info);

    // x=0: 128+20=148, x=1: 128+15=143, x=2: 128+10=138, x=3: 128+5=133
    ASSERT_EQ(grid.get_value(0, 0), static_cast<uint8_t>(148));
    ASSERT_EQ(grid.get_value(1, 0), static_cast<uint8_t>(143));
    ASSERT_EQ(grid.get_value(2, 0), static_cast<uint8_t>(138));
    ASSERT_EQ(grid.get_value(3, 0), static_cast<uint8_t>(133));
    // x=4 through x=7: no bonus, remain 128
    ASSERT_EQ(grid.get_value(4, 0), static_cast<uint8_t>(128));
    ASSERT_EQ(grid.get_value(5, 0), static_cast<uint8_t>(128));
    ASSERT_EQ(grid.get_value(6, 0), static_cast<uint8_t>(128));
    ASSERT_EQ(grid.get_value(7, 0), static_cast<uint8_t>(128));
}

// =============================================================================
// Main Entry Point
// =============================================================================

int main() {
    printf("=== RoadAccessBonus Unit Tests (Ticket E10-102) ===\n\n");

    // calculate_road_bonus tests
    RUN_TEST(on_road_bonus);
    RUN_TEST(distance_1_bonus);
    RUN_TEST(distance_2_bonus);
    RUN_TEST(distance_3_bonus);
    RUN_TEST(distance_4_no_bonus);
    RUN_TEST(distance_10_no_bonus);
    RUN_TEST(distance_255_no_bonus);

    // apply_road_bonuses tests
    RUN_TEST(apply_road_bonuses_updates_grid);
    RUN_TEST(apply_road_bonuses_far_tile_unchanged);
    RUN_TEST(apply_road_bonuses_clamps_to_255);
    RUN_TEST(apply_road_bonuses_at_max);
    RUN_TEST(apply_road_bonuses_from_zero);
    RUN_TEST(apply_road_bonuses_empty_vector);
    RUN_TEST(apply_road_bonuses_out_of_bounds_ignored);
    RUN_TEST(apply_road_bonuses_multiple_tiles_same_row);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
