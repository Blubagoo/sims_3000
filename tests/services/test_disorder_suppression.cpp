/**
 * @file test_disorder_suppression.cpp
 * @brief Unit tests for DisorderSuppression utility functions
 *        (Epic 9, Ticket E9-040)
 *
 * Tests cover:
 * - calculate_disorder_suppression at key coverage levels (0%, 50%, 100%)
 * - ENFORCER_SUPPRESSION_FACTOR constant is 0.7
 * - Clamping of negative and >1.0 inputs
 */

#include <sims3000/services/DisorderSuppression.h>
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
    printf("Testing DisorderSuppression constants...\n");

    assert(float_eq(ENFORCER_SUPPRESSION_FACTOR, 0.7f));

    printf("  PASS: ENFORCER_SUPPRESSION_FACTOR == 0.7\n");
}

// =============================================================================
// calculate_disorder_suppression tests
// =============================================================================

void test_zero_coverage() {
    printf("Testing calculate_disorder_suppression at 0%% coverage...\n");

    float result = calculate_disorder_suppression(0.0f);
    assert(float_eq(result, 1.0f));

    printf("  PASS: 0%% coverage -> multiplier 1.0 (no suppression)\n");
}

void test_fifty_percent_coverage() {
    printf("Testing calculate_disorder_suppression at 50%% coverage...\n");

    float result = calculate_disorder_suppression(0.5f);
    assert(float_eq(result, 0.65f));

    printf("  PASS: 50%% coverage -> multiplier 0.65\n");
}

void test_full_coverage() {
    printf("Testing calculate_disorder_suppression at 100%% coverage...\n");

    float result = calculate_disorder_suppression(1.0f);
    assert(float_eq(result, 0.3f));

    printf("  PASS: 100%% coverage -> multiplier 0.3\n");
}

void test_quarter_coverage() {
    printf("Testing calculate_disorder_suppression at 25%% coverage...\n");

    float result = calculate_disorder_suppression(0.25f);
    // 1.0 - (0.25 * 0.7) = 1.0 - 0.175 = 0.825
    assert(float_eq(result, 0.825f));

    printf("  PASS: 25%% coverage -> multiplier 0.825\n");
}

// =============================================================================
// Clamping tests
// =============================================================================

void test_negative_coverage_clamped() {
    printf("Testing calculate_disorder_suppression with negative input...\n");

    float result = calculate_disorder_suppression(-0.5f);
    assert(float_eq(result, 1.0f));

    result = calculate_disorder_suppression(-100.0f);
    assert(float_eq(result, 1.0f));

    printf("  PASS: Negative coverage clamped to 0 -> multiplier 1.0\n");
}

void test_over_one_coverage_clamped() {
    printf("Testing calculate_disorder_suppression with >1.0 input...\n");

    float result = calculate_disorder_suppression(1.5f);
    assert(float_eq(result, 0.3f));

    result = calculate_disorder_suppression(10.0f);
    assert(float_eq(result, 0.3f));

    printf("  PASS: Coverage >1.0 clamped to 1.0 -> multiplier 0.3\n");
}

// =============================================================================
// Main
// =============================================================================

int main() {
    printf("=== DisorderSuppression Unit Tests (Epic 9, Ticket E9-040) ===\n\n");

    test_constants();
    test_zero_coverage();
    test_fifty_percent_coverage();
    test_full_coverage();
    test_quarter_coverage();
    test_negative_coverage_clamped();
    test_over_one_coverage_clamped();

    printf("\n=== All DisorderSuppression Tests Passed ===\n");
    return 0;
}
