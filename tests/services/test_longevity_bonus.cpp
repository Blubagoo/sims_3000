/**
 * @file test_longevity_bonus.cpp
 * @brief Unit tests for LongevityBonus utility functions
 *        (Epic 9, Ticket E9-041)
 *
 * Tests cover:
 * - calculate_longevity at key coverage levels (0%, 50%, 100%)
 * - MEDICAL_BASE_LONGEVITY and MEDICAL_MAX_LONGEVITY_BONUS constants
 * - Clamping of negative and >1.0 inputs
 */

#include <sims3000/services/LongevityBonus.h>
#include <cassert>
#include <cstdio>

using namespace sims3000::services;

// =============================================================================
// Constants tests
// =============================================================================

void test_constants() {
    printf("Testing LongevityBonus constants...\n");

    assert(MEDICAL_BASE_LONGEVITY == 60);
    assert(MEDICAL_MAX_LONGEVITY_BONUS == 40);

    printf("  PASS: MEDICAL_BASE_LONGEVITY == 60, MEDICAL_MAX_LONGEVITY_BONUS == 40\n");
}

// =============================================================================
// calculate_longevity tests
// =============================================================================

void test_zero_coverage() {
    printf("Testing calculate_longevity at 0%% coverage...\n");

    uint32_t result = calculate_longevity(0.0f);
    assert(result == 60);

    printf("  PASS: 0%% coverage -> 60 cycles\n");
}

void test_fifty_percent_coverage() {
    printf("Testing calculate_longevity at 50%% coverage...\n");

    uint32_t result = calculate_longevity(0.5f);
    assert(result == 80);

    printf("  PASS: 50%% coverage -> 80 cycles\n");
}

void test_full_coverage() {
    printf("Testing calculate_longevity at 100%% coverage...\n");

    uint32_t result = calculate_longevity(1.0f);
    assert(result == 100);

    printf("  PASS: 100%% coverage -> 100 cycles\n");
}

void test_quarter_coverage() {
    printf("Testing calculate_longevity at 25%% coverage...\n");

    uint32_t result = calculate_longevity(0.25f);
    // 60 + (0.25 * 40) = 60 + 10 = 70
    assert(result == 70);

    printf("  PASS: 25%% coverage -> 70 cycles\n");
}

// =============================================================================
// Clamping tests
// =============================================================================

void test_negative_coverage_clamped() {
    printf("Testing calculate_longevity with negative input...\n");

    uint32_t result = calculate_longevity(-0.5f);
    assert(result == 60);

    result = calculate_longevity(-100.0f);
    assert(result == 60);

    printf("  PASS: Negative coverage clamped to 0 -> 60 cycles\n");
}

void test_over_one_coverage_clamped() {
    printf("Testing calculate_longevity with >1.0 input...\n");

    uint32_t result = calculate_longevity(1.5f);
    assert(result == 100);

    result = calculate_longevity(10.0f);
    assert(result == 100);

    printf("  PASS: Coverage >1.0 clamped to 1.0 -> 100 cycles\n");
}

// =============================================================================
// Main
// =============================================================================

int main() {
    printf("=== LongevityBonus Unit Tests (Epic 9, Ticket E9-041) ===\n\n");

    test_constants();
    test_zero_coverage();
    test_fifty_percent_coverage();
    test_full_coverage();
    test_quarter_coverage();
    test_negative_coverage_clamped();
    test_over_one_coverage_clamped();

    printf("\n=== All LongevityBonus Tests Passed ===\n");
    return 0;
}
