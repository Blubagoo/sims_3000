/**
 * @file test_funding_modifier.cpp
 * @brief Unit tests for FundingModifier utility functions
 *        (Epic 9, Ticket E9-024)
 *
 * Tests cover:
 * - calculate_funding_modifier at key funding levels (0%, 50%, 100%, 150%, 200%)
 * - Funding curve is linear up to cap
 * - MAX_FUNDING_MODIFIER cap at 1.15
 * - apply_funding_to_effectiveness at various inputs
 * - 0% funding = 0% effectiveness (building disabled)
 * - DEFAULT_FUNDING_PERCENT constant is 100
 */

#include <sims3000/services/FundingModifier.h>
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
    printf("Testing FundingModifier constants...\n");

    assert(float_eq(MAX_FUNDING_MODIFIER, 1.15f));
    assert(DEFAULT_FUNDING_PERCENT == 100);

    printf("  PASS: Constants are correct\n");
}

// =============================================================================
// calculate_funding_modifier tests
// =============================================================================

void test_modifier_at_zero_percent() {
    printf("Testing calculate_funding_modifier at 0%%...\n");

    float mod = calculate_funding_modifier(0);
    assert(float_eq(mod, 0.0f));

    printf("  PASS: 0%% funding -> 0.0 modifier\n");
}

void test_modifier_at_fifty_percent() {
    printf("Testing calculate_funding_modifier at 50%%...\n");

    float mod = calculate_funding_modifier(50);
    assert(float_eq(mod, 0.5f));

    printf("  PASS: 50%% funding -> 0.5 modifier\n");
}

void test_modifier_at_hundred_percent() {
    printf("Testing calculate_funding_modifier at 100%%...\n");

    float mod = calculate_funding_modifier(100);
    assert(float_eq(mod, 1.0f));

    printf("  PASS: 100%% funding -> 1.0 modifier\n");
}

void test_modifier_at_150_percent() {
    printf("Testing calculate_funding_modifier at 150%%...\n");

    float mod = calculate_funding_modifier(150);
    assert(float_eq(mod, 1.15f));

    printf("  PASS: 150%% funding -> 1.15 modifier (capped)\n");
}

void test_modifier_at_200_percent() {
    printf("Testing calculate_funding_modifier at 200%%...\n");

    float mod = calculate_funding_modifier(200);
    assert(float_eq(mod, 1.15f));

    printf("  PASS: 200%% funding -> 1.15 modifier (capped)\n");
}

void test_modifier_at_115_percent() {
    printf("Testing calculate_funding_modifier at 115%%...\n");

    float mod = calculate_funding_modifier(115);
    assert(float_eq(mod, 1.15f));

    printf("  PASS: 115%% funding -> 1.15 modifier (exactly at cap)\n");
}

void test_modifier_linear_below_cap() {
    printf("Testing calculate_funding_modifier is linear below cap...\n");

    // Check linearity: modifier = funding / 100
    for (uint8_t pct = 0; pct <= 114; ++pct) {
        float expected = static_cast<float>(pct) / 100.0f;
        float actual = calculate_funding_modifier(pct);
        assert(float_eq(actual, expected));
    }

    printf("  PASS: Modifier is linear for 0-114%%\n");
}

void test_modifier_capped_above_115() {
    printf("Testing calculate_funding_modifier is capped above 115%%...\n");

    for (uint16_t pct = 115; pct <= 255; ++pct) {
        float actual = calculate_funding_modifier(static_cast<uint8_t>(pct));
        assert(float_eq(actual, MAX_FUNDING_MODIFIER));
    }

    printf("  PASS: Modifier is capped at 1.15 for 115-255%%\n");
}

// =============================================================================
// apply_funding_to_effectiveness tests
// =============================================================================

void test_apply_zero_funding() {
    printf("Testing apply_funding_to_effectiveness with 0%% funding...\n");

    uint8_t result = apply_funding_to_effectiveness(100, 0);
    assert(result == 0);

    printf("  PASS: 0%% funding disables building (effectiveness = 0)\n");
}

void test_apply_fifty_funding() {
    printf("Testing apply_funding_to_effectiveness with 50%% funding...\n");

    uint8_t result = apply_funding_to_effectiveness(100, 50);
    assert(result == 50);

    printf("  PASS: 50%% funding -> 50 effectiveness\n");
}

void test_apply_hundred_funding() {
    printf("Testing apply_funding_to_effectiveness with 100%% funding...\n");

    uint8_t result = apply_funding_to_effectiveness(100, 100);
    assert(result == 100);

    printf("  PASS: 100%% funding -> 100 effectiveness\n");
}

void test_apply_150_funding() {
    printf("Testing apply_funding_to_effectiveness with 150%% funding...\n");

    uint8_t result = apply_funding_to_effectiveness(100, 150);
    assert(result == 115);

    printf("  PASS: 150%% funding -> 115 effectiveness (capped)\n");
}

void test_apply_200_funding() {
    printf("Testing apply_funding_to_effectiveness with 200%% funding...\n");

    uint8_t result = apply_funding_to_effectiveness(100, 200);
    assert(result == 115);

    printf("  PASS: 200%% funding -> 115 effectiveness (capped)\n");
}

void test_apply_zero_base() {
    printf("Testing apply_funding_to_effectiveness with 0 base effectiveness...\n");

    uint8_t result = apply_funding_to_effectiveness(0, 100);
    assert(result == 0);

    result = apply_funding_to_effectiveness(0, 150);
    assert(result == 0);

    printf("  PASS: 0 base effectiveness -> 0 regardless of funding\n");
}

void test_apply_partial_base() {
    printf("Testing apply_funding_to_effectiveness with partial base effectiveness...\n");

    // 50 base * 1.0 modifier (100% funding) = 50
    uint8_t result = apply_funding_to_effectiveness(50, 100);
    assert(result == 50);

    // 80 base * 0.5 modifier (50% funding) = 40
    result = apply_funding_to_effectiveness(80, 50);
    assert(result == 40);

    printf("  PASS: Partial base effectiveness scales correctly\n");
}

void test_apply_max_uint8_base() {
    printf("Testing apply_funding_to_effectiveness with max uint8 base...\n");

    // 255 base * 1.15 modifier = 293.25 -> clamped to 255
    uint8_t result = apply_funding_to_effectiveness(255, 150);
    assert(result == 255);

    printf("  PASS: Overflow clamped to 255\n");
}

// =============================================================================
// Default funding test
// =============================================================================

void test_default_funding_gives_full_effectiveness() {
    printf("Testing DEFAULT_FUNDING_PERCENT gives full effectiveness...\n");

    float mod = calculate_funding_modifier(DEFAULT_FUNDING_PERCENT);
    assert(float_eq(mod, 1.0f));

    uint8_t result = apply_funding_to_effectiveness(100, DEFAULT_FUNDING_PERCENT);
    assert(result == 100);

    printf("  PASS: Default funding (100%%) gives full effectiveness\n");
}

// =============================================================================
// Main
// =============================================================================

int main() {
    printf("=== FundingModifier Unit Tests (Epic 9, Ticket E9-024) ===\n\n");

    test_constants();
    test_modifier_at_zero_percent();
    test_modifier_at_fifty_percent();
    test_modifier_at_hundred_percent();
    test_modifier_at_150_percent();
    test_modifier_at_200_percent();
    test_modifier_at_115_percent();
    test_modifier_linear_below_cap();
    test_modifier_capped_above_115();
    test_apply_zero_funding();
    test_apply_fifty_funding();
    test_apply_hundred_funding();
    test_apply_150_funding();
    test_apply_200_funding();
    test_apply_zero_base();
    test_apply_partial_base();
    test_apply_max_uint8_base();
    test_default_funding_gives_full_effectiveness();

    printf("\n=== All FundingModifier Tests Passed ===\n");
    return 0;
}
