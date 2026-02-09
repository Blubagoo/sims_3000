/**
 * @file test_disorder_generation.cpp
 * @brief Unit tests for disorder generation from buildings (Ticket E10-073).
 *
 * Tests cover:
 * - calculate_disorder_amount for each zone type (0-4)
 * - Low occupancy produces lower generation
 * - High occupancy produces higher generation
 * - Low land value increases generation
 * - High land value produces minimal increase
 * - Zone type 0 (hab_low) generates ~2-4
 * - Zone type 1 (hab_high) generates ~5-10
 * - apply_disorder_generation updates grid
 * - Invalid zone type returns 0
 */

#include <sims3000/disorder/DisorderGeneration.h>
#include <sims3000/disorder/DisorderGrid.h>
#include <cstdio>
#include <cstdlib>

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

// =============================================================================
// calculate_disorder_amount per zone type
// =============================================================================

TEST(zone_type_0_hab_low_mid_occupancy) {
    // hab_low: base=2, pop_mult=0.5, lv_mod=0.3
    // occupancy=0.5, land_value=128 (mid)
    // generation = 2 + (2 * 0.5 * 0.5) = 2 + 0.5 = 2.5
    // lv_mod = 0.3 * (1.0 - 128/255) = 0.3 * 0.498 = 0.1494
    // generation = 2.5 + 2.5 * 0.1494 = 2.5 + 0.374 = 2.874 -> 2
    DisorderSource src = { 5, 5, 0, 0.5f, 128 };
    uint8_t result = calculate_disorder_amount(src);
    ASSERT(result >= 2 && result <= 4);
}

TEST(zone_type_1_hab_high_mid_occupancy) {
    // hab_high: base=5, pop_mult=0.8, lv_mod=0.5
    // occupancy=0.5, land_value=128
    // generation = 5 + (5 * 0.8 * 0.5) = 5 + 2.0 = 7.0
    // lv_mod = 0.5 * (1.0 - 128/255) = 0.5 * 0.498 = 0.249
    // generation = 7.0 + 7.0 * 0.249 = 7.0 + 1.743 = 8.743 -> 8
    DisorderSource src = { 5, 5, 1, 0.5f, 128 };
    uint8_t result = calculate_disorder_amount(src);
    ASSERT(result >= 5 && result <= 10);
}

TEST(zone_type_2_exchange_low_mid_occupancy) {
    // exchange_low: base=3, pop_mult=0.4, lv_mod=0.2
    // occupancy=0.5, land_value=128
    // generation = 3 + (3 * 0.4 * 0.5) = 3 + 0.6 = 3.6
    // lv_mod = 0.2 * (1.0 - 128/255) = 0.2 * 0.498 = 0.0996
    // generation = 3.6 + 3.6 * 0.0996 = 3.6 + 0.359 = 3.959 -> 3
    DisorderSource src = { 5, 5, 2, 0.5f, 128 };
    uint8_t result = calculate_disorder_amount(src);
    ASSERT(result >= 3 && result <= 5);
}

TEST(zone_type_3_exchange_high_mid_occupancy) {
    // exchange_high: base=6, pop_mult=0.6, lv_mod=0.3
    // occupancy=0.5, land_value=128
    // generation = 6 + (6 * 0.6 * 0.5) = 6 + 1.8 = 7.8
    // lv_mod = 0.3 * (1.0 - 128/255) = 0.3 * 0.498 = 0.1494
    // generation = 7.8 + 7.8 * 0.1494 = 7.8 + 1.165 = 8.965 -> 8
    DisorderSource src = { 5, 5, 3, 0.5f, 128 };
    uint8_t result = calculate_disorder_amount(src);
    ASSERT(result >= 6 && result <= 12);
}

TEST(zone_type_4_fab_mid_occupancy) {
    // fab: base=1, pop_mult=0.2, lv_mod=0.1
    // occupancy=0.5, land_value=128
    // generation = 1 + (1 * 0.2 * 0.5) = 1 + 0.1 = 1.1
    // lv_mod = 0.1 * (1.0 - 128/255) = 0.1 * 0.498 = 0.0498
    // generation = 1.1 + 1.1 * 0.0498 = 1.1 + 0.055 = 1.155 -> 1
    DisorderSource src = { 5, 5, 4, 0.5f, 128 };
    uint8_t result = calculate_disorder_amount(src);
    ASSERT(result >= 1 && result <= 3);
}

// =============================================================================
// Occupancy effects
// =============================================================================

TEST(low_occupancy_lower_generation) {
    // hab_high with zero occupancy: only base
    // generation = 5 + (5 * 0.8 * 0.0) = 5.0
    // lv_mod = 0.5 * (1.0 - 128/255) = 0.249
    // generation = 5.0 + 5.0 * 0.249 = 6.245 -> 6
    DisorderSource low = { 5, 5, 1, 0.0f, 128 };
    uint8_t result_low = calculate_disorder_amount(low);

    // hab_high with full occupancy
    // generation = 5 + (5 * 0.8 * 1.0) = 9.0
    // lv_mod = 0.5 * 0.498 = 0.249
    // generation = 9.0 + 9.0 * 0.249 = 11.24 -> 11
    DisorderSource high = { 5, 5, 1, 1.0f, 128 };
    uint8_t result_high = calculate_disorder_amount(high);

    ASSERT(result_low < result_high);
}

TEST(high_occupancy_higher_generation) {
    DisorderSource low_occ = { 5, 5, 0, 0.1f, 128 };
    DisorderSource high_occ = { 5, 5, 0, 0.9f, 128 };
    uint8_t result_low = calculate_disorder_amount(low_occ);
    uint8_t result_high = calculate_disorder_amount(high_occ);
    ASSERT(result_high >= result_low);
}

// =============================================================================
// Land value effects
// =============================================================================

TEST(low_land_value_increases_generation) {
    // Low land value (0) -> maximum land value modifier
    DisorderSource low_lv = { 5, 5, 1, 0.5f, 0 };
    // High land value (255) -> minimal land value modifier
    DisorderSource high_lv = { 5, 5, 1, 0.5f, 255 };

    uint8_t result_low_lv = calculate_disorder_amount(low_lv);
    uint8_t result_high_lv = calculate_disorder_amount(high_lv);

    ASSERT(result_low_lv > result_high_lv);
}

TEST(high_land_value_minimal_increase) {
    // land_value=255 -> lv_mod = modifier * (1.0 - 255/255) = modifier * 0 = 0
    // So generation should just be base + base * pop_mult * occupancy
    DisorderSource src = { 5, 5, 1, 0.5f, 255 };
    uint8_t result = calculate_disorder_amount(src);

    // generation = 5 + (5 * 0.8 * 0.5) = 5 + 2 = 7
    // lv_mod = 0.5 * 0.0 = 0.0
    // generation = 7 + 0 = 7
    ASSERT_EQ(result, 7);
}

TEST(zero_land_value_maximum_increase) {
    // land_value=0 -> lv_mod = modifier * 1.0 = full modifier
    DisorderSource src = { 5, 5, 1, 0.5f, 0 };
    uint8_t result = calculate_disorder_amount(src);

    // generation = 5 + (5 * 0.8 * 0.5) = 7.0
    // lv_mod = 0.5 * 1.0 = 0.5
    // generation = 7.0 + 7.0 * 0.5 = 10.5 -> 10
    ASSERT(result >= 10 && result <= 11);
}

// =============================================================================
// Zone type range checks
// =============================================================================

TEST(hab_low_generates_2_to_4) {
    // Zone type 0 with full occupancy, zero land value (worst case)
    // generation = 2 + (2 * 0.5 * 1.0) = 3.0
    // lv_mod = 0.3 * 1.0 = 0.3
    // generation = 3.0 + 3.0 * 0.3 = 3.9 -> 3
    DisorderSource worst = { 5, 5, 0, 1.0f, 0 };
    uint8_t result = calculate_disorder_amount(worst);
    ASSERT(result >= 2 && result <= 5);

    // Zone type 0 with zero occupancy, max land value (best case)
    DisorderSource best = { 5, 5, 0, 0.0f, 255 };
    uint8_t result_best = calculate_disorder_amount(best);
    ASSERT(result_best >= 2 && result_best <= 4);
}

TEST(hab_high_generates_5_to_10) {
    // Zone type 1, full occupancy, zero land value (max)
    DisorderSource worst = { 5, 5, 1, 1.0f, 0 };
    uint8_t result_max = calculate_disorder_amount(worst);
    ASSERT(result_max >= 5 && result_max <= 15);

    // Zone type 1, zero occupancy, max land value (min)
    DisorderSource best = { 5, 5, 1, 0.0f, 255 };
    uint8_t result_min = calculate_disorder_amount(best);
    ASSERT(result_min >= 5 && result_min <= 10);
}

// =============================================================================
// apply_disorder_generation
// =============================================================================

TEST(apply_disorder_generation_updates_grid) {
    DisorderGrid grid(64, 64);
    ASSERT_EQ(grid.get_level(10, 10), 0);

    DisorderSource src = { 10, 10, 1, 1.0f, 0 };
    apply_disorder_generation(grid, src);

    ASSERT(grid.get_level(10, 10) > 0);
}

TEST(apply_disorder_generation_accumulates) {
    DisorderGrid grid(64, 64);

    DisorderSource src = { 10, 10, 1, 1.0f, 0 };
    apply_disorder_generation(grid, src);
    uint8_t first = grid.get_level(10, 10);

    apply_disorder_generation(grid, src);
    uint8_t second = grid.get_level(10, 10);

    ASSERT(second > first);
}

TEST(apply_disorder_generation_only_affects_target_cell) {
    DisorderGrid grid(64, 64);

    DisorderSource src = { 10, 10, 1, 1.0f, 0 };
    apply_disorder_generation(grid, src);

    ASSERT_EQ(grid.get_level(9, 10), 0);
    ASSERT_EQ(grid.get_level(11, 10), 0);
    ASSERT_EQ(grid.get_level(10, 9), 0);
    ASSERT_EQ(grid.get_level(10, 11), 0);
}

// =============================================================================
// Edge cases
// =============================================================================

TEST(invalid_zone_type_returns_zero) {
    DisorderSource src = { 5, 5, 99, 1.0f, 0 };
    uint8_t result = calculate_disorder_amount(src);
    ASSERT_EQ(result, 0);
}

TEST(zero_occupancy_still_produces_base) {
    // Even with 0 occupancy, base generation is nonzero (unless fab)
    DisorderSource src = { 5, 5, 1, 0.0f, 128 };
    uint8_t result = calculate_disorder_amount(src);
    ASSERT(result >= 5);
}

// =============================================================================
// Main
// =============================================================================

int main() {
    printf("=== Disorder Generation Tests (E10-073) ===\n\n");

    RUN_TEST(zone_type_0_hab_low_mid_occupancy);
    RUN_TEST(zone_type_1_hab_high_mid_occupancy);
    RUN_TEST(zone_type_2_exchange_low_mid_occupancy);
    RUN_TEST(zone_type_3_exchange_high_mid_occupancy);
    RUN_TEST(zone_type_4_fab_mid_occupancy);
    RUN_TEST(low_occupancy_lower_generation);
    RUN_TEST(high_occupancy_higher_generation);
    RUN_TEST(low_land_value_increases_generation);
    RUN_TEST(high_land_value_minimal_increase);
    RUN_TEST(zero_land_value_maximum_increase);
    RUN_TEST(hab_low_generates_2_to_4);
    RUN_TEST(hab_high_generates_5_to_10);
    RUN_TEST(apply_disorder_generation_updates_grid);
    RUN_TEST(apply_disorder_generation_accumulates);
    RUN_TEST(apply_disorder_generation_only_affects_target_cell);
    RUN_TEST(invalid_zone_type_returns_zero);
    RUN_TEST(zero_occupancy_still_produces_base);

    printf("\n=== Results: %d passed, %d failed ===\n",
           tests_passed, tests_failed);

    return tests_failed > 0 ? 1 : 0;
}
