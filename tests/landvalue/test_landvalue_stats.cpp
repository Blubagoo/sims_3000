/**
 * @file test_landvalue_stats.cpp
 * @brief Tests for land value statistics interface (Ticket E10-106)
 *
 * Validates:
 * - get_landvalue_stat() computes correct statistics
 * - Average, max, min values
 * - High-value and low-value tile counts
 * - get_landvalue_stat_name() returns correct names
 * - is_valid_landvalue_stat() validates stat IDs correctly
 * - Invalid stat IDs return 0.0f and "Unknown"
 * - Edge cases (empty grid, uniform values)
 */

#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "sims3000/landvalue/LandValueStats.h"

using namespace sims3000::landvalue;

// --------------------------------------------------------------------------
// Helper: float approximate equality
// --------------------------------------------------------------------------
static bool approx(float a, float b, float epsilon = 0.001f) {
    return std::fabs(a - b) < epsilon;
}

// --------------------------------------------------------------------------
// Test: Average land value calculation
// --------------------------------------------------------------------------
static void test_average_land_value() {
    LandValueGrid grid(4, 4);  // 4x4 grid

    // Set values: 100, 150, 200, 128, ...
    grid.set_value(0, 0, 100);
    grid.set_value(1, 0, 150);
    grid.set_value(2, 0, 200);
    grid.set_value(3, 0, 128);
    // Rest are default 128

    // Calculate expected average
    // Total: 100 + 150 + 200 + 128 + 12 * 128 = 578 + 1536 = 2114
    // Average: 2114 / 16 = 132.125
    float avg = get_landvalue_stat(grid, STAT_AVERAGE_LAND_VALUE);
    assert(approx(avg, 132.125f) && "Average land value should be 132.125");

    std::printf("  PASS: Average land value calculation\n");
}

// --------------------------------------------------------------------------
// Test: Maximum land value
// --------------------------------------------------------------------------
static void test_max_land_value() {
    LandValueGrid grid(4, 4);

    // Default is 128, set one to 255
    grid.set_value(2, 2, 255);

    float max_val = get_landvalue_stat(grid, STAT_MAX_LAND_VALUE);
    assert(approx(max_val, 255.0f) && "Max land value should be 255");

    std::printf("  PASS: Maximum land value\n");
}

// --------------------------------------------------------------------------
// Test: Minimum land value
// --------------------------------------------------------------------------
static void test_min_land_value() {
    LandValueGrid grid(4, 4);

    // Default is 128, set one to 0
    grid.set_value(1, 3, 0);

    float min_val = get_landvalue_stat(grid, STAT_MIN_LAND_VALUE);
    assert(approx(min_val, 0.0f) && "Min land value should be 0");

    std::printf("  PASS: Minimum land value\n");
}

// --------------------------------------------------------------------------
// Test: High-value tiles count (> 192)
// --------------------------------------------------------------------------
static void test_high_value_tiles() {
    LandValueGrid grid(4, 4);

    // Set some tiles above 192
    grid.set_value(0, 0, 193);
    grid.set_value(1, 0, 200);
    grid.set_value(2, 2, 255);
    grid.set_value(3, 3, 192);  // Not > 192, so doesn't count

    float count = get_landvalue_stat(grid, STAT_HIGH_VALUE_TILES);
    assert(approx(count, 3.0f) && "Should have 3 high-value tiles");

    std::printf("  PASS: High-value tiles count\n");
}

// --------------------------------------------------------------------------
// Test: Low-value tiles count (< 64)
// --------------------------------------------------------------------------
static void test_low_value_tiles() {
    LandValueGrid grid(4, 4);

    // Set some tiles below 64
    grid.set_value(0, 0, 0);
    grid.set_value(1, 1, 32);
    grid.set_value(2, 2, 63);
    grid.set_value(3, 3, 64);  // Not < 64, so doesn't count

    float count = get_landvalue_stat(grid, STAT_LOW_VALUE_TILES);
    assert(approx(count, 3.0f) && "Should have 3 low-value tiles");

    std::printf("  PASS: Low-value tiles count\n");
}

// --------------------------------------------------------------------------
// Test: Uniform grid (all same value)
// --------------------------------------------------------------------------
static void test_uniform_grid() {
    LandValueGrid grid(4, 4);

    // All tiles default to 128
    float avg = get_landvalue_stat(grid, STAT_AVERAGE_LAND_VALUE);
    float max_val = get_landvalue_stat(grid, STAT_MAX_LAND_VALUE);
    float min_val = get_landvalue_stat(grid, STAT_MIN_LAND_VALUE);

    assert(approx(avg, 128.0f) && "Uniform grid average should be 128");
    assert(approx(max_val, 128.0f) && "Uniform grid max should be 128");
    assert(approx(min_val, 128.0f) && "Uniform grid min should be 128");

    float high_count = get_landvalue_stat(grid, STAT_HIGH_VALUE_TILES);
    float low_count = get_landvalue_stat(grid, STAT_LOW_VALUE_TILES);

    assert(approx(high_count, 0.0f) && "No high-value tiles in uniform 128 grid");
    assert(approx(low_count, 0.0f) && "No low-value tiles in uniform 128 grid");

    std::printf("  PASS: Uniform grid statistics\n");
}

// --------------------------------------------------------------------------
// Test: get_landvalue_stat() with invalid stat ID
// --------------------------------------------------------------------------
static void test_invalid_stat_id() {
    LandValueGrid grid(4, 4);

    // Invalid stat IDs should return 0.0f
    assert(get_landvalue_stat(grid, 0) == 0.0f);
    assert(get_landvalue_stat(grid, 999) == 0.0f);
    assert(get_landvalue_stat(grid, 599) == 0.0f);  // One below valid range
    assert(get_landvalue_stat(grid, 605) == 0.0f);  // One above valid range

    std::printf("  PASS: Invalid stat IDs return 0.0f\n");
}

// --------------------------------------------------------------------------
// Test: get_landvalue_stat_name() returns correct names
// --------------------------------------------------------------------------
static void test_stat_names() {
    assert(std::strcmp(get_landvalue_stat_name(STAT_AVERAGE_LAND_VALUE), "Average Land Value") == 0);
    assert(std::strcmp(get_landvalue_stat_name(STAT_MAX_LAND_VALUE), "Maximum Land Value") == 0);
    assert(std::strcmp(get_landvalue_stat_name(STAT_MIN_LAND_VALUE), "Minimum Land Value") == 0);
    assert(std::strcmp(get_landvalue_stat_name(STAT_HIGH_VALUE_TILES), "High Value Tiles") == 0);
    assert(std::strcmp(get_landvalue_stat_name(STAT_LOW_VALUE_TILES), "Low Value Tiles") == 0);

    std::printf("  PASS: Stat names are correct\n");
}

// --------------------------------------------------------------------------
// Test: get_landvalue_stat_name() with invalid ID
// --------------------------------------------------------------------------
static void test_invalid_stat_name() {
    assert(std::strcmp(get_landvalue_stat_name(0), "Unknown") == 0);
    assert(std::strcmp(get_landvalue_stat_name(999), "Unknown") == 0);

    std::printf("  PASS: Invalid stat IDs return 'Unknown'\n");
}

// --------------------------------------------------------------------------
// Test: is_valid_landvalue_stat() validates correctly
// --------------------------------------------------------------------------
static void test_is_valid_stat() {
    // Valid range: 600-604
    assert(is_valid_landvalue_stat(STAT_AVERAGE_LAND_VALUE) == true);
    assert(is_valid_landvalue_stat(STAT_MAX_LAND_VALUE) == true);
    assert(is_valid_landvalue_stat(STAT_MIN_LAND_VALUE) == true);
    assert(is_valid_landvalue_stat(STAT_HIGH_VALUE_TILES) == true);
    assert(is_valid_landvalue_stat(STAT_LOW_VALUE_TILES) == true);

    // Invalid IDs
    assert(is_valid_landvalue_stat(0) == false);
    assert(is_valid_landvalue_stat(599) == false);
    assert(is_valid_landvalue_stat(605) == false);
    assert(is_valid_landvalue_stat(999) == false);

    std::printf("  PASS: is_valid_landvalue_stat() validates correctly\n");
}

// --------------------------------------------------------------------------
// Test: All stat ID constants are unique
// --------------------------------------------------------------------------
static void test_stat_id_uniqueness() {
    // Ensure no duplicate stat IDs
    uint16_t ids[] = {
        STAT_AVERAGE_LAND_VALUE,
        STAT_MAX_LAND_VALUE,
        STAT_MIN_LAND_VALUE,
        STAT_HIGH_VALUE_TILES,
        STAT_LOW_VALUE_TILES
    };

    constexpr size_t count = sizeof(ids) / sizeof(ids[0]);
    for (size_t i = 0; i < count; ++i) {
        for (size_t j = i + 1; j < count; ++j) {
            assert(ids[i] != ids[j] && "Stat IDs must be unique");
        }
    }

    std::printf("  PASS: All stat IDs are unique\n");
}

// --------------------------------------------------------------------------
// Test: Stat ID range is sequential
// --------------------------------------------------------------------------
static void test_stat_id_range() {
    // Verify stat IDs are sequential 600-604
    assert(STAT_AVERAGE_LAND_VALUE == 600);
    assert(STAT_MAX_LAND_VALUE == 601);
    assert(STAT_MIN_LAND_VALUE == 602);
    assert(STAT_HIGH_VALUE_TILES == 603);
    assert(STAT_LOW_VALUE_TILES == 604);

    std::printf("  PASS: Stat IDs are in expected sequential range\n");
}

// --------------------------------------------------------------------------
// Test: Large grid performance (basic sanity check)
// --------------------------------------------------------------------------
static void test_large_grid() {
    LandValueGrid grid(64, 64);  // 4096 tiles

    // Set a few distinct values
    grid.set_value(0, 0, 0);
    grid.set_value(63, 63, 255);

    float avg = get_landvalue_stat(grid, STAT_AVERAGE_LAND_VALUE);
    float max_val = get_landvalue_stat(grid, STAT_MAX_LAND_VALUE);
    float min_val = get_landvalue_stat(grid, STAT_MIN_LAND_VALUE);

    // Most tiles are 128, with one 0 and one 255
    // Average should be close to 128
    assert(avg > 127.0f && avg < 129.0f && "Large grid average should be ~128");
    assert(approx(max_val, 255.0f) && "Max should be 255");
    assert(approx(min_val, 0.0f) && "Min should be 0");

    std::printf("  PASS: Large grid statistics\n");
}

// --------------------------------------------------------------------------
// Test: Threshold constants
// --------------------------------------------------------------------------
static void test_threshold_constants() {
    assert(HIGH_VALUE_THRESHOLD == 192);
    assert(LOW_VALUE_THRESHOLD == 64);

    std::printf("  PASS: Threshold constants are correct\n");
}

// --------------------------------------------------------------------------
// Main
// --------------------------------------------------------------------------
int main() {
    std::printf("=== Land Value Statistics Tests (E10-106) ===\n");

    test_average_land_value();
    test_max_land_value();
    test_min_land_value();
    test_high_value_tiles();
    test_low_value_tiles();
    test_uniform_grid();
    test_invalid_stat_id();
    test_stat_names();
    test_invalid_stat_name();
    test_is_valid_stat();
    test_stat_id_uniqueness();
    test_stat_id_range();
    test_large_grid();
    test_threshold_constants();

    std::printf("All land value statistics tests passed.\n");
    return 0;
}
