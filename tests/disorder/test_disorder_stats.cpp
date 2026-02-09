/**
 * @file test_disorder_stats.cpp
 * @brief Unit tests for DisorderStats (Ticket E10-077)
 *
 * Tests cover:
 * - get_disorder_stat: total_disorder, average_disorder, high_disorder_tiles, max_disorder
 * - get_disorder_stat_name: all stat names
 * - is_valid_disorder_stat: valid and invalid IDs
 * - get_disorder_at: direct position query
 */

#include <sims3000/disorder/DisorderStats.h>
#include <sims3000/disorder/DisorderGrid.h>
#include <cstdio>
#include <cstdlib>
#include <cmath>

using namespace sims3000::disorder;

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

#define ASSERT_FLOAT_EQ(a, b) do { \
    if (std::fabs((a) - (b)) > 0.001f) { \
        printf("\n  FAILED: %s == %s (line %d, got %f vs %f)\n", #a, #b, __LINE__, (float)(a), (float)(b)); \
        tests_failed++; \
        return; \
    } \
} while(0)

// =============================================================================
// get_disorder_stat Tests
// =============================================================================

TEST(stat_total_disorder_empty) {
    DisorderGrid grid(64, 64);
    grid.update_stats();
    float total = get_disorder_stat(grid, STAT_TOTAL_DISORDER);
    ASSERT_FLOAT_EQ(total, 0.0f);
}

TEST(stat_total_disorder_with_data) {
    DisorderGrid grid(4, 4);
    grid.set_level(0, 0, 100);
    grid.set_level(1, 0, 50);
    grid.set_level(2, 0, 25);
    grid.update_stats();
    float total = get_disorder_stat(grid, STAT_TOTAL_DISORDER);
    ASSERT_FLOAT_EQ(total, 175.0f);
}

TEST(stat_average_disorder_empty) {
    DisorderGrid grid(64, 64);
    grid.update_stats();
    float avg = get_disorder_stat(grid, STAT_AVERAGE_DISORDER);
    ASSERT_FLOAT_EQ(avg, 0.0f);
}

TEST(stat_average_disorder_uniform) {
    DisorderGrid grid(4, 4);
    for (int32_t y = 0; y < 4; ++y) {
        for (int32_t x = 0; x < 4; ++x) {
            grid.set_level(x, y, 100);
        }
    }
    grid.update_stats();
    float avg = get_disorder_stat(grid, STAT_AVERAGE_DISORDER);
    ASSERT_FLOAT_EQ(avg, 100.0f);
}

TEST(stat_average_disorder_mixed) {
    DisorderGrid grid(4, 4);
    grid.set_level(0, 0, 200);
    grid.set_level(1, 0, 100);
    // 2 tiles * 150 average = 300 / 16 tiles = 18.75
    grid.update_stats();
    float avg = get_disorder_stat(grid, STAT_AVERAGE_DISORDER);
    ASSERT_FLOAT_EQ(avg, 18.75f);
}

TEST(stat_high_disorder_tiles_none) {
    DisorderGrid grid(8, 8);
    grid.set_level(0, 0, 100);
    grid.set_level(1, 0, 127); // Below threshold
    grid.update_stats();
    float high = get_disorder_stat(grid, STAT_HIGH_DISORDER_TILES);
    ASSERT_FLOAT_EQ(high, 0.0f);
}

TEST(stat_high_disorder_tiles_some) {
    DisorderGrid grid(8, 8);
    grid.set_level(0, 0, 128); // At threshold
    grid.set_level(1, 0, 200); // Above threshold
    grid.set_level(2, 0, 255); // Max
    grid.update_stats();
    float high = get_disorder_stat(grid, STAT_HIGH_DISORDER_TILES);
    ASSERT_FLOAT_EQ(high, 3.0f);
}

TEST(stat_max_disorder_empty) {
    DisorderGrid grid(64, 64);
    float max_val = get_disorder_stat(grid, STAT_MAX_DISORDER);
    ASSERT_FLOAT_EQ(max_val, 0.0f);
}

TEST(stat_max_disorder_single) {
    DisorderGrid grid(64, 64);
    grid.set_level(10, 10, 200);
    float max_val = get_disorder_stat(grid, STAT_MAX_DISORDER);
    ASSERT_FLOAT_EQ(max_val, 200.0f);
}

TEST(stat_max_disorder_multiple) {
    DisorderGrid grid(8, 8);
    grid.set_level(0, 0, 100);
    grid.set_level(1, 0, 255);
    grid.set_level(2, 0, 200);
    grid.set_level(3, 0, 50);
    float max_val = get_disorder_stat(grid, STAT_MAX_DISORDER);
    ASSERT_FLOAT_EQ(max_val, 255.0f);
}

TEST(stat_invalid_id_returns_zero) {
    DisorderGrid grid(64, 64);
    float val = get_disorder_stat(grid, 9999);
    ASSERT_FLOAT_EQ(val, 0.0f);
}

// =============================================================================
// get_disorder_stat_name Tests
// =============================================================================

TEST(stat_name_total_disorder) {
    const char* name = get_disorder_stat_name(STAT_TOTAL_DISORDER);
    ASSERT(name != nullptr);
    // Just verify it's not "Unknown"
    ASSERT(name[0] == 'T'); // "Total Disorder"
}

TEST(stat_name_average_disorder) {
    const char* name = get_disorder_stat_name(STAT_AVERAGE_DISORDER);
    ASSERT(name != nullptr);
    ASSERT(name[0] == 'A'); // "Average Disorder"
}

TEST(stat_name_high_disorder_tiles) {
    const char* name = get_disorder_stat_name(STAT_HIGH_DISORDER_TILES);
    ASSERT(name != nullptr);
    ASSERT(name[0] == 'H'); // "High Disorder Tiles"
}

TEST(stat_name_max_disorder) {
    const char* name = get_disorder_stat_name(STAT_MAX_DISORDER);
    ASSERT(name != nullptr);
    ASSERT(name[0] == 'M'); // "Max Disorder"
}

TEST(stat_name_invalid_returns_unknown) {
    const char* name = get_disorder_stat_name(9999);
    ASSERT(name != nullptr);
    ASSERT(name[0] == 'U'); // "Unknown"
}

// =============================================================================
// is_valid_disorder_stat Tests
// =============================================================================

TEST(is_valid_all_valid_stats) {
    ASSERT(is_valid_disorder_stat(STAT_TOTAL_DISORDER));
    ASSERT(is_valid_disorder_stat(STAT_AVERAGE_DISORDER));
    ASSERT(is_valid_disorder_stat(STAT_HIGH_DISORDER_TILES));
    ASSERT(is_valid_disorder_stat(STAT_MAX_DISORDER));
}

TEST(is_valid_invalid_stats) {
    ASSERT(!is_valid_disorder_stat(0));
    ASSERT(!is_valid_disorder_stat(9999));
    ASSERT(!is_valid_disorder_stat(399)); // Just before valid range
    ASSERT(!is_valid_disorder_stat(404)); // Just after valid range
}

// =============================================================================
// get_disorder_at Tests
// =============================================================================

TEST(get_disorder_at_basic) {
    DisorderGrid grid(64, 64);
    grid.set_level(10, 20, 150);
    uint8_t level = get_disorder_at(grid, 10, 20);
    ASSERT_EQ(level, 150);
}

TEST(get_disorder_at_multiple_positions) {
    DisorderGrid grid(64, 64);
    grid.set_level(0, 0, 100);
    grid.set_level(10, 10, 200);
    grid.set_level(63, 63, 255);
    ASSERT_EQ(get_disorder_at(grid, 0, 0), 100);
    ASSERT_EQ(get_disorder_at(grid, 10, 10), 200);
    ASSERT_EQ(get_disorder_at(grid, 63, 63), 255);
}

TEST(get_disorder_at_empty_cell) {
    DisorderGrid grid(64, 64);
    uint8_t level = get_disorder_at(grid, 10, 10);
    ASSERT_EQ(level, 0);
}

TEST(get_disorder_at_out_of_bounds) {
    DisorderGrid grid(64, 64);
    grid.set_level(0, 0, 100);
    ASSERT_EQ(get_disorder_at(grid, 64, 0), 0);
    ASSERT_EQ(get_disorder_at(grid, 0, 64), 0);
    ASSERT_EQ(get_disorder_at(grid, -1, 0), 0);
    ASSERT_EQ(get_disorder_at(grid, 0, -1), 0);
}

// =============================================================================
// Main Entry Point
// =============================================================================

int main() {
    printf("=== DisorderStats Unit Tests (Ticket E10-077) ===\n\n");

    // get_disorder_stat tests
    RUN_TEST(stat_total_disorder_empty);
    RUN_TEST(stat_total_disorder_with_data);
    RUN_TEST(stat_average_disorder_empty);
    RUN_TEST(stat_average_disorder_uniform);
    RUN_TEST(stat_average_disorder_mixed);
    RUN_TEST(stat_high_disorder_tiles_none);
    RUN_TEST(stat_high_disorder_tiles_some);
    RUN_TEST(stat_max_disorder_empty);
    RUN_TEST(stat_max_disorder_single);
    RUN_TEST(stat_max_disorder_multiple);
    RUN_TEST(stat_invalid_id_returns_zero);

    // get_disorder_stat_name tests
    RUN_TEST(stat_name_total_disorder);
    RUN_TEST(stat_name_average_disorder);
    RUN_TEST(stat_name_high_disorder_tiles);
    RUN_TEST(stat_name_max_disorder);
    RUN_TEST(stat_name_invalid_returns_unknown);

    // is_valid_disorder_stat tests
    RUN_TEST(is_valid_all_valid_stats);
    RUN_TEST(is_valid_invalid_stats);

    // get_disorder_at tests
    RUN_TEST(get_disorder_at_basic);
    RUN_TEST(get_disorder_at_multiple_positions);
    RUN_TEST(get_disorder_at_empty_cell);
    RUN_TEST(get_disorder_at_out_of_bounds);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
