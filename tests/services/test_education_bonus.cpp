/**
 * @file test_education_bonus.cpp
 * @brief Unit tests for EducationBonus utility functions
 *        (Epic 9, Ticket E9-042)
 *
 * Tests cover:
 * - calculate_education_land_value_multiplier at key coverage levels (0%, 50%, 100%)
 * - EDUCATION_LAND_VALUE_BONUS constant is 0.1
 * - Clamping of negative and >1.0 inputs
 */

#include <sims3000/services/EducationBonus.h>
#include <cassert>
#include <cmath>
#include <cstdio>

using namespace sims3000::services;

// Helper for float comparison
static bool float_eq(float a, float b, float eps = 0.001f) {
    return std::fabs(a - b) < eps;
}

// =============================================================================
// Constants tests
// =============================================================================

void test_constants() {
    printf("Testing EducationBonus constants...\n");

    assert(float_eq(EDUCATION_LAND_VALUE_BONUS, 0.1f));

    printf("  PASS: EDUCATION_LAND_VALUE_BONUS == 0.1\n");
}

// =============================================================================
// calculate_education_land_value_multiplier tests
// =============================================================================

void test_zero_coverage() {
    printf("Testing calculate_education_land_value_multiplier at 0%% coverage...\n");

    float result = calculate_education_land_value_multiplier(0.0f);
    assert(float_eq(result, 1.0f));

    printf("  PASS: 0%% coverage -> multiplier 1.0\n");
}

void test_fifty_percent_coverage() {
    printf("Testing calculate_education_land_value_multiplier at 50%% coverage...\n");

    float result = calculate_education_land_value_multiplier(0.5f);
    assert(float_eq(result, 1.05f));

    printf("  PASS: 50%% coverage -> multiplier 1.05\n");
}

void test_full_coverage() {
    printf("Testing calculate_education_land_value_multiplier at 100%% coverage...\n");

    float result = calculate_education_land_value_multiplier(1.0f);
    assert(float_eq(result, 1.1f));

    printf("  PASS: 100%% coverage -> multiplier 1.1\n");
}

void test_quarter_coverage() {
    printf("Testing calculate_education_land_value_multiplier at 25%% coverage...\n");

    float result = calculate_education_land_value_multiplier(0.25f);
    // 1.0 + (0.25 * 0.1) = 1.025
    assert(float_eq(result, 1.025f));

    printf("  PASS: 25%% coverage -> multiplier 1.025\n");
}

// =============================================================================
// Clamping tests
// =============================================================================

void test_negative_coverage_clamped() {
    printf("Testing calculate_education_land_value_multiplier with negative input...\n");

    float result = calculate_education_land_value_multiplier(-0.5f);
    assert(float_eq(result, 1.0f));

    result = calculate_education_land_value_multiplier(-100.0f);
    assert(float_eq(result, 1.0f));

    printf("  PASS: Negative coverage clamped to 0 -> multiplier 1.0\n");
}

void test_over_one_coverage_clamped() {
    printf("Testing calculate_education_land_value_multiplier with >1.0 input...\n");

    float result = calculate_education_land_value_multiplier(1.5f);
    assert(float_eq(result, 1.1f));

    result = calculate_education_land_value_multiplier(10.0f);
    assert(float_eq(result, 1.1f));

    printf("  PASS: Coverage >1.0 clamped to 1.0 -> multiplier 1.1\n");
}

// =============================================================================
// Main
// =============================================================================

int main() {
    printf("=== EducationBonus Unit Tests (Epic 9, Ticket E9-042) ===\n\n");

    test_constants();
    test_zero_coverage();
    test_fifty_percent_coverage();
    test_full_coverage();
    test_quarter_coverage();
    test_negative_coverage_clamped();
    test_over_one_coverage_clamped();

    printf("\n=== All EducationBonus Tests Passed ===\n");
    return 0;
}
