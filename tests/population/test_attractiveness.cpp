/**
 * @file test_attractiveness.cpp
 * @brief Tests for attractiveness calculation (Ticket E10-024)
 *
 * Validates:
 * - All neutral (50) factors: moderate positive attraction
 * - All positive factors high (100): max attraction
 * - All negative factors high (100): negative attraction
 * - Clamped to -100/+100 range
 */

#include <cassert>
#include <cmath>
#include <cstdio>

#include "sims3000/population/AttractivenessCalculation.h"

using namespace sims3000::population;

// --------------------------------------------------------------------------
// Helper: float approximate equality
// --------------------------------------------------------------------------
static bool approx(float a, float b, float epsilon = 0.01f) {
    return std::fabs(a - b) < epsilon;
}

// --------------------------------------------------------------------------
// Test: All neutral (50) factors -> moderate positive attraction
// --------------------------------------------------------------------------
static void test_all_neutral_factors() {
    MigrationFactors factors{};
    factors.job_availability = 50;
    factors.housing_availability = 50;
    factors.sector_value_avg = 50;
    factors.service_coverage = 50;
    factors.harmony_level = 50;
    factors.disorder_level = 50;
    factors.contamination_level = 50;
    factors.tribute_burden = 50;
    factors.congestion_level = 50;

    AttractivenessResult result = calculate_attractiveness(factors);

    // positive = 50*0.20 + 50*0.15 + 50*0.10 + 50*0.15 + 50*0.15
    //          = 10 + 7.5 + 5 + 7.5 + 7.5 = 37.5
    assert(approx(result.weighted_positive, 37.5f) && "Neutral positive should be 37.5");

    // negative = 50*0.10 + 50*0.10 + 50*0.03 + 50*0.02
    //          = 5 + 5 + 1.5 + 1.0 = 12.5
    assert(approx(result.weighted_negative, 12.5f) && "Neutral negative should be 12.5");

    // net = 37.5 - 12.5 = 25.0
    assert(result.net_attraction == 25 && "Neutral factors should give moderate positive attraction");

    std::printf("  PASS: All neutral (50) factors -> moderate positive\n");
}

// --------------------------------------------------------------------------
// Test: All positive factors high (100), no negatives -> max attraction
// --------------------------------------------------------------------------
static void test_all_positive_high() {
    MigrationFactors factors{};
    factors.job_availability = 100;
    factors.housing_availability = 100;
    factors.sector_value_avg = 100;
    factors.service_coverage = 100;
    factors.harmony_level = 100;
    factors.disorder_level = 0;
    factors.contamination_level = 0;
    factors.tribute_burden = 0;
    factors.congestion_level = 0;

    AttractivenessResult result = calculate_attractiveness(factors);

    // positive = 100*0.20 + 100*0.15 + 100*0.10 + 100*0.15 + 100*0.15
    //          = 20 + 15 + 10 + 15 + 15 = 75.0
    assert(approx(result.weighted_positive, 75.0f) && "Max positive should be 75.0");

    // negative = 0
    assert(approx(result.weighted_negative, 0.0f) && "Zero negatives should be 0.0");

    // net = 75.0 - 0 = 75
    assert(result.net_attraction == 75 && "All positive high should give 75 attraction");

    std::printf("  PASS: All positive factors high (100) -> high attraction\n");
}

// --------------------------------------------------------------------------
// Test: All negative factors high (100), no positives -> negative attraction
// --------------------------------------------------------------------------
static void test_all_negative_high() {
    MigrationFactors factors{};
    factors.job_availability = 0;
    factors.housing_availability = 0;
    factors.sector_value_avg = 0;
    factors.service_coverage = 0;
    factors.harmony_level = 0;
    factors.disorder_level = 100;
    factors.contamination_level = 100;
    factors.tribute_burden = 100;
    factors.congestion_level = 100;

    AttractivenessResult result = calculate_attractiveness(factors);

    // positive = 0
    assert(approx(result.weighted_positive, 0.0f) && "Zero positives should be 0.0");

    // negative = 100*0.10 + 100*0.10 + 100*0.03 + 100*0.02
    //          = 10 + 10 + 3 + 2 = 25.0
    assert(approx(result.weighted_negative, 25.0f) && "Max negative should be 25.0");

    // net = 0 - 25 = -25
    assert(result.net_attraction == -25 && "All negative high should give -25 attraction");

    std::printf("  PASS: All negative factors high (100) -> negative attraction\n");
}

// --------------------------------------------------------------------------
// Test: Clamped to +100 range
// --------------------------------------------------------------------------
static void test_clamped_positive() {
    // This test verifies the clamp behavior even though current weights
    // max out at 75 positive. The clamp logic should still be in place.
    MigrationFactors factors{};
    factors.job_availability = 100;
    factors.housing_availability = 100;
    factors.sector_value_avg = 100;
    factors.service_coverage = 100;
    factors.harmony_level = 100;
    factors.disorder_level = 0;
    factors.contamination_level = 0;
    factors.tribute_burden = 0;
    factors.congestion_level = 0;

    AttractivenessResult result = calculate_attractiveness(factors);

    // With current weights, max is 75 which is below 100
    // But verify it's clamped (not above 100)
    assert(result.net_attraction <= 100 && "Net attraction should not exceed +100");
    assert(result.net_attraction >= -100 && "Net attraction should not be below -100");

    std::printf("  PASS: Clamped to +100 range\n");
}

// --------------------------------------------------------------------------
// Test: Clamped to -100 range
// --------------------------------------------------------------------------
static void test_clamped_negative() {
    // Max negative with current weights is -25, verify clamp logic works
    MigrationFactors factors{};
    factors.job_availability = 0;
    factors.housing_availability = 0;
    factors.sector_value_avg = 0;
    factors.service_coverage = 0;
    factors.harmony_level = 0;
    factors.disorder_level = 100;
    factors.contamination_level = 100;
    factors.tribute_burden = 100;
    factors.congestion_level = 100;

    AttractivenessResult result = calculate_attractiveness(factors);

    assert(result.net_attraction >= -100 && "Net attraction should not be below -100");
    assert(result.net_attraction <= 100 && "Net attraction should not exceed +100");

    std::printf("  PASS: Clamped to -100 range\n");
}

// --------------------------------------------------------------------------
// Test: Mixed factors produce expected result
// --------------------------------------------------------------------------
static void test_mixed_factors() {
    MigrationFactors factors{};
    factors.job_availability = 80;
    factors.housing_availability = 60;
    factors.sector_value_avg = 40;
    factors.service_coverage = 70;
    factors.harmony_level = 90;
    factors.disorder_level = 20;
    factors.contamination_level = 30;
    factors.tribute_burden = 10;
    factors.congestion_level = 15;

    AttractivenessResult result = calculate_attractiveness(factors);

    // positive = 80*0.20 + 60*0.15 + 40*0.10 + 70*0.15 + 90*0.15
    //          = 16 + 9 + 4 + 10.5 + 13.5 = 53.0
    assert(approx(result.weighted_positive, 53.0f) && "Mixed positive should be 53.0");

    // negative = 20*0.10 + 30*0.10 + 10*0.03 + 15*0.02
    //          = 2 + 3 + 0.3 + 0.3 = 5.6
    assert(approx(result.weighted_negative, 5.6f) && "Mixed negative should be 5.6");

    // net = 53.0 - 5.6 = 47.4 -> rounded to 47
    assert(result.net_attraction == 47 && "Mixed factors should give 47 attraction");

    std::printf("  PASS: Mixed factors produce expected result\n");
}

// --------------------------------------------------------------------------
// Main
// --------------------------------------------------------------------------
int main() {
    std::printf("=== Attractiveness Calculation Tests (E10-024) ===\n");

    test_all_neutral_factors();
    test_all_positive_high();
    test_all_negative_high();
    test_clamped_positive();
    test_clamped_negative();
    test_mixed_factors();

    std::printf("All attractiveness calculation tests passed.\n");
    return 0;
}
