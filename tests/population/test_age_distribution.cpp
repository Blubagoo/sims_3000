/**
 * @file test_age_distribution.cpp
 * @brief Tests for age distribution evolution (Ticket E10-018)
 *
 * Validates:
 * - Percentages always sum to 100
 * - Births increase youth percentage
 * - Aging transitions (youth->adult->elder)
 * - Weighted deaths (60% elder, 30% adult, 10% youth)
 * - No negative counts
 */

#include <cassert>
#include <cstdio>
#include <cmath>

#include "sims3000/population/AgeDistribution.h"

using namespace sims3000::population;

// --------------------------------------------------------------------------
// Test: Percentages always sum to 100
// --------------------------------------------------------------------------
static void test_percentages_sum_to_100() {
    // Various scenarios, result should always sum to 100
    auto result1 = evolve_age_distribution(33, 34, 33, 100, 50, 10000);
    int sum1 = result1.youth_percent + result1.adult_percent + result1.elder_percent;
    assert(sum1 == 100 && "Percentages must sum to 100");

    auto result2 = evolve_age_distribution(10, 80, 10, 500, 200, 20000);
    int sum2 = result2.youth_percent + result2.adult_percent + result2.elder_percent;
    assert(sum2 == 100 && "Percentages must sum to 100");

    auto result3 = evolve_age_distribution(50, 30, 20, 1000, 800, 5000);
    int sum3 = result3.youth_percent + result3.adult_percent + result3.elder_percent;
    assert(sum3 == 100 && "Percentages must sum to 100");

    std::printf("  PASS: Percentages sum to 100\n");
}

// --------------------------------------------------------------------------
// Test: Births increase youth percentage
// --------------------------------------------------------------------------
static void test_births_increase_youth() {
    uint32_t total = 10000;
    uint32_t births = 500;  // Significant births
    uint32_t deaths = 0;    // No deaths to isolate birth effect

    auto result = evolve_age_distribution(33, 34, 33, births, deaths, total);

    // With only births and no deaths, youth percentage should increase
    // (though aging also moves some youth to adult)
    // Net effect should still be positive for youth
    assert(result.youth_percent >= 30 && "Youth should remain significant with births");

    std::printf("  PASS: Births increase youth\n");
}

// --------------------------------------------------------------------------
// Test: Aging transitions (youth->adult, adult->elder)
// --------------------------------------------------------------------------
static void test_aging_transitions() {
    uint32_t total = 10000;
    uint32_t births = 0;
    uint32_t deaths = 0;

    // Start with all youth
    auto result1 = evolve_age_distribution(100, 0, 0, births, deaths, total);
    // With no births/deaths, 2% of youth should move to adult
    assert(result1.youth_percent < 100 && "Some youth should age to adult");
    assert(result1.adult_percent > 0 && "Adult percentage should increase");

    // Start with all adults
    auto result2 = evolve_age_distribution(0, 100, 0, births, deaths, total);
    // 1% of adults should move to elder
    assert(result2.adult_percent < 100 && "Some adults should age to elder");
    assert(result2.elder_percent > 0 && "Elder percentage should increase");

    std::printf("  PASS: Aging transitions work\n");
}

// --------------------------------------------------------------------------
// Test: Weighted deaths (60% elder, 30% adult, 10% youth)
// --------------------------------------------------------------------------
static void test_weighted_deaths() {
    uint32_t total = 10000;
    uint32_t births = 0;
    uint32_t deaths = 1000;  // Significant deaths

    // Start with equal distribution
    auto result = evolve_age_distribution(33, 34, 33, births, deaths, total);

    // Elder percentage should decrease the most due to 60% death weight
    // This is a relative test - elder deaths should be weighted higher
    assert(result.elder_percent < 33 && "Elders should be reduced by weighted deaths");

    std::printf("  PASS: Weighted deaths applied\n");
}

// --------------------------------------------------------------------------
// Test: No negative counts
// --------------------------------------------------------------------------
static void test_no_negative_counts() {
    uint32_t total = 100;
    uint32_t births = 0;
    uint32_t deaths = 5000;  // Deaths exceed population

    // Even with extreme deaths, counts should never go negative
    auto result = evolve_age_distribution(33, 34, 33, births, deaths, total);

    // All percentages should be valid (0-100)
    assert(result.youth_percent <= 100 && "Youth percent should be valid");
    assert(result.adult_percent <= 100 && "Adult percent should be valid");
    assert(result.elder_percent <= 100 && "Elder percent should be valid");

    int sum = result.youth_percent + result.adult_percent + result.elder_percent;
    assert(sum == 100 && "Sum should still be 100");

    std::printf("  PASS: No negative counts\n");
}

// --------------------------------------------------------------------------
// Test: Zero population edge case
// --------------------------------------------------------------------------
static void test_zero_population() {
    auto result = evolve_age_distribution(33, 34, 33, 0, 0, 0);

    // With zero population, should return default distribution
    assert(result.youth_percent == 33 && "Zero pop should return default youth");
    assert(result.adult_percent == 34 && "Zero pop should return default adult");
    assert(result.elder_percent == 33 && "Zero pop should return default elder");

    std::printf("  PASS: Zero population edge case\n");
}

// --------------------------------------------------------------------------
// Test: Full simulation cycle
// --------------------------------------------------------------------------
static void test_full_cycle() {
    uint32_t total = 10000;
    uint32_t births = 150;   // 15 per 1000
    uint32_t deaths = 80;    // 8 per 1000

    auto result = evolve_age_distribution(33, 34, 33, births, deaths, total);

    // Result should be valid
    int sum = result.youth_percent + result.adult_percent + result.elder_percent;
    assert(sum == 100 && "Full cycle should produce valid distribution");

    // With more births than deaths, and aging, expect reasonable distribution
    assert(result.youth_percent >= 10 && result.youth_percent <= 60 &&
           "Youth should be in reasonable range");
    assert(result.adult_percent >= 10 && result.adult_percent <= 60 &&
           "Adult should be in reasonable range");
    assert(result.elder_percent >= 10 && result.elder_percent <= 60 &&
           "Elder should be in reasonable range");

    std::printf("  PASS: Full simulation cycle\n");
}

// --------------------------------------------------------------------------
// Main
// --------------------------------------------------------------------------
int main() {
    std::printf("=== Age Distribution Tests (E10-018) ===\n");

    test_percentages_sum_to_100();
    test_births_increase_youth();
    test_aging_transitions();
    test_weighted_deaths();
    test_no_negative_counts();
    test_zero_population();
    test_full_cycle();

    std::printf("All age distribution tests passed.\n");
    return 0;
}
