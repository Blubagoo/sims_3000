/**
 * @file test_linear_falloff.cpp
 * @brief Exhaustive unit tests for linear distance falloff model (Epic 9, Ticket E9-021)
 *
 * Validates the calculate_falloff() function from CoverageCalculation.h
 * against the specification:
 *   falloff = 1.0 - (distance / radius)
 *   strength = max_effectiveness * falloff
 *
 * Tests cover:
 * - Exact formula verification at multiple points
 * - Distance 0: 100% strength
 * - Distance == radius: 0% strength
 * - Distance > radius: 0% strength
 * - Linearity: halfway distance = half strength
 * - Multiple effectiveness values (0, 128/255, 255/255)
 * - Multiple radii (1, 5, 8, 12, 16, 20)
 * - Negative distance handling (absolute value)
 * - Zero and negative radius edge cases
 */

#include <sims3000/services/CoverageCalculation.h>
#include <cassert>
#include <cstdio>
#include <cmath>

using namespace sims3000::services;

static constexpr float EPSILON = 0.001f;

static bool approx_eq(float a, float b) {
    return std::fabs(a - b) < EPSILON;
}

// =============================================================================
// Exact formula verification
// =============================================================================

void test_exact_formula() {
    printf("Testing exact formula: strength = effectiveness * (1.0 - distance/radius)...\n");

    // Manually compute expected values and compare
    struct TestCase {
        float effectiveness;
        int distance;
        int radius;
        float expected;
    };

    TestCase cases[] = {
        // Full effectiveness, various distances with radius 8
        { 1.0f,  0,  8, 1.0f },                    // 1.0 * (1 - 0/8)   = 1.0
        { 1.0f,  1,  8, 1.0f - 1.0f/8.0f },        // 1.0 * (1 - 1/8)   = 0.875
        { 1.0f,  2,  8, 1.0f - 2.0f/8.0f },        // 1.0 * (1 - 2/8)   = 0.75
        { 1.0f,  3,  8, 1.0f - 3.0f/8.0f },        // 1.0 * (1 - 3/8)   = 0.625
        { 1.0f,  4,  8, 1.0f - 4.0f/8.0f },        // 1.0 * (1 - 4/8)   = 0.5
        { 1.0f,  5,  8, 1.0f - 5.0f/8.0f },        // 1.0 * (1 - 5/8)   = 0.375
        { 1.0f,  6,  8, 1.0f - 6.0f/8.0f },        // 1.0 * (1 - 6/8)   = 0.25
        { 1.0f,  7,  8, 1.0f - 7.0f/8.0f },        // 1.0 * (1 - 7/8)   = 0.125
        { 1.0f,  8,  8, 0.0f },                     // At edge -> 0

        // Half effectiveness
        { 0.5f,  0,  8, 0.5f },
        { 0.5f,  4,  8, 0.25f },

        // Quarter effectiveness
        { 0.25f, 0, 10, 0.25f },
        { 0.25f, 5, 10, 0.25f * 0.5f },             // 0.125
    };

    int num_cases = sizeof(cases) / sizeof(cases[0]);
    for (int i = 0; i < num_cases; ++i) {
        float result = calculate_falloff(cases[i].effectiveness, cases[i].distance, cases[i].radius);
        assert(approx_eq(result, cases[i].expected));
    }

    printf("  PASS: All %d exact formula cases verified\n", num_cases);
}

// =============================================================================
// Distance 0: Full effectiveness
// =============================================================================

void test_distance_zero_full_strength() {
    printf("Testing distance 0 returns full effectiveness...\n");

    // With various radii
    int radii[] = { 1, 5, 8, 12, 16, 20 };
    int num_radii = sizeof(radii) / sizeof(radii[0]);

    for (int i = 0; i < num_radii; ++i) {
        float result = calculate_falloff(1.0f, 0, radii[i]);
        assert(approx_eq(result, 1.0f));
    }

    // With various effectiveness values
    float effs[] = { 0.0f, 0.1f, 0.5f, 0.75f, 1.0f };
    int num_effs = sizeof(effs) / sizeof(effs[0]);

    for (int i = 0; i < num_effs; ++i) {
        float result = calculate_falloff(effs[i], 0, 8);
        assert(approx_eq(result, effs[i]));
    }

    printf("  PASS: Distance 0 always returns full effectiveness\n");
}

// =============================================================================
// Distance == radius: Zero strength
// =============================================================================

void test_distance_equals_radius() {
    printf("Testing distance == radius returns 0...\n");

    int radii[] = { 1, 5, 8, 12, 16, 20 };
    int num_radii = sizeof(radii) / sizeof(radii[0]);

    for (int i = 0; i < num_radii; ++i) {
        float result = calculate_falloff(1.0f, radii[i], radii[i]);
        assert(result == 0.0f);
    }

    // Also with partial effectiveness
    for (int i = 0; i < num_radii; ++i) {
        float result = calculate_falloff(0.5f, radii[i], radii[i]);
        assert(result == 0.0f);
    }

    printf("  PASS: Distance == radius returns 0 for all radii\n");
}

// =============================================================================
// Distance > radius: Zero strength
// =============================================================================

void test_distance_beyond_radius() {
    printf("Testing distance > radius returns 0...\n");

    int radii[] = { 1, 5, 8, 12, 16, 20 };
    int num_radii = sizeof(radii) / sizeof(radii[0]);

    for (int i = 0; i < num_radii; ++i) {
        // Just beyond
        float result1 = calculate_falloff(1.0f, radii[i] + 1, radii[i]);
        assert(result1 == 0.0f);

        // Well beyond
        float result2 = calculate_falloff(1.0f, radii[i] * 2, radii[i]);
        assert(result2 == 0.0f);

        // Extremely far
        float result3 = calculate_falloff(1.0f, 1000, radii[i]);
        assert(result3 == 0.0f);
    }

    printf("  PASS: Distance beyond radius always returns 0\n");
}

// =============================================================================
// Linearity: halfway = half strength
// =============================================================================

void test_linearity() {
    printf("Testing linearity: halfway distance = half strength...\n");

    // For each even radius, verify that distance = radius/2 gives exactly half
    int even_radii[] = { 2, 4, 8, 12, 16, 20 };
    int num_radii = sizeof(even_radii) / sizeof(even_radii[0]);

    for (int i = 0; i < num_radii; ++i) {
        int r = even_radii[i];
        int half_dist = r / 2;

        float full = calculate_falloff(1.0f, 0, r);
        float half = calculate_falloff(1.0f, half_dist, r);

        assert(approx_eq(half, full * 0.5f));
    }

    // Verify quarter point: distance = radius * 3/4 gives 1/4 strength
    for (int i = 0; i < num_radii; ++i) {
        int r = even_radii[i];
        // Only test if 3/4 * radius is an integer
        if ((r * 3) % 4 == 0) {
            int three_quarter_dist = (r * 3) / 4;
            float result = calculate_falloff(1.0f, three_quarter_dist, r);
            assert(approx_eq(result, 0.25f));
        }
    }

    // Verify linearity property: f(d1) - f(d2) is proportional to d2 - d1
    // For radius 12: f(3) - f(6) should equal f(6) - f(9) (each is 3 units apart)
    {
        float f3 = calculate_falloff(1.0f, 3, 12);
        float f6 = calculate_falloff(1.0f, 6, 12);
        float f9 = calculate_falloff(1.0f, 9, 12);

        float delta1 = f3 - f6;
        float delta2 = f6 - f9;
        assert(approx_eq(delta1, delta2));
    }

    printf("  PASS: Linear falloff is verified at multiple points\n");
}

// =============================================================================
// Different effectiveness values: 0, 128/255, 255/255
// =============================================================================

void test_effectiveness_zero() {
    printf("Testing effectiveness = 0 always returns 0...\n");

    for (int d = 0; d <= 20; ++d) {
        float result = calculate_falloff(0.0f, d, 20);
        assert(result == 0.0f);
    }

    printf("  PASS: Zero effectiveness always returns 0\n");
}

void test_effectiveness_half() {
    printf("Testing effectiveness = 128/255 (~0.502)...\n");

    float eff = 128.0f / 255.0f;

    // At distance 0: should return eff
    float at_0 = calculate_falloff(eff, 0, 8);
    assert(approx_eq(at_0, eff));

    // At distance 4 (half of radius 8): should return eff * 0.5
    float at_half = calculate_falloff(eff, 4, 8);
    assert(approx_eq(at_half, eff * 0.5f));

    // At distance 8 (edge): should return 0
    float at_edge = calculate_falloff(eff, 8, 8);
    assert(at_edge == 0.0f);

    printf("  PASS: Half effectiveness scales correctly\n");
}

void test_effectiveness_full() {
    printf("Testing effectiveness = 255/255 (1.0)...\n");

    float eff = 255.0f / 255.0f;  // 1.0

    // At distance 0: returns 1.0
    float at_0 = calculate_falloff(eff, 0, 8);
    assert(approx_eq(at_0, 1.0f));

    // At distance 2: returns 0.75
    float at_2 = calculate_falloff(eff, 2, 8);
    assert(approx_eq(at_2, 0.75f));

    printf("  PASS: Full effectiveness returns expected values\n");
}

// =============================================================================
// Different radii: 1, 5, 8, 12, 16, 20
// =============================================================================

void test_radius_1() {
    printf("Testing radius = 1...\n");

    // Distance 0: full strength
    float at_0 = calculate_falloff(1.0f, 0, 1);
    assert(approx_eq(at_0, 1.0f));

    // Distance 1: edge, should be 0
    float at_1 = calculate_falloff(1.0f, 1, 1);
    assert(at_1 == 0.0f);

    // Distance 2: beyond, should be 0
    float at_2 = calculate_falloff(1.0f, 2, 1);
    assert(at_2 == 0.0f);

    printf("  PASS: Radius 1 has coverage only at center\n");
}

void test_radius_5() {
    printf("Testing radius = 5...\n");

    // Verify all integer distances 0-5
    float expected[] = { 1.0f, 0.8f, 0.6f, 0.4f, 0.2f, 0.0f };

    for (int d = 0; d <= 5; ++d) {
        float result = calculate_falloff(1.0f, d, 5);
        assert(approx_eq(result, expected[d]));
    }

    printf("  PASS: Radius 5 falloff values correct\n");
}

void test_radius_8() {
    printf("Testing radius = 8...\n");

    // Verify all integer distances 0-8
    for (int d = 0; d <= 8; ++d) {
        float expected = (d < 8) ? (1.0f - static_cast<float>(d) / 8.0f) : 0.0f;
        float result = calculate_falloff(1.0f, d, 8);
        assert(approx_eq(result, expected));
    }

    printf("  PASS: Radius 8 falloff values correct\n");
}

void test_radius_12() {
    printf("Testing radius = 12...\n");

    for (int d = 0; d <= 12; ++d) {
        float expected = (d < 12) ? (1.0f - static_cast<float>(d) / 12.0f) : 0.0f;
        float result = calculate_falloff(1.0f, d, 12);
        assert(approx_eq(result, expected));
    }

    printf("  PASS: Radius 12 falloff values correct\n");
}

void test_radius_16() {
    printf("Testing radius = 16...\n");

    for (int d = 0; d <= 16; ++d) {
        float expected = (d < 16) ? (1.0f - static_cast<float>(d) / 16.0f) : 0.0f;
        float result = calculate_falloff(1.0f, d, 16);
        assert(approx_eq(result, expected));
    }

    printf("  PASS: Radius 16 falloff values correct\n");
}

void test_radius_20() {
    printf("Testing radius = 20...\n");

    for (int d = 0; d <= 20; ++d) {
        float expected = (d < 20) ? (1.0f - static_cast<float>(d) / 20.0f) : 0.0f;
        float result = calculate_falloff(1.0f, d, 20);
        assert(approx_eq(result, expected));
    }

    printf("  PASS: Radius 20 falloff values correct\n");
}

// =============================================================================
// Negative distance handling
// =============================================================================

void test_negative_distance() {
    printf("Testing negative distance is treated as absolute value...\n");

    int radii[] = { 1, 5, 8, 12, 16, 20 };
    int num_radii = sizeof(radii) / sizeof(radii[0]);

    for (int i = 0; i < num_radii; ++i) {
        int r = radii[i];
        // For each distance 1..r-1, verify negative equals positive
        for (int d = 1; d < r; ++d) {
            float pos = calculate_falloff(1.0f, d, r);
            float neg = calculate_falloff(1.0f, -d, r);
            assert(approx_eq(pos, neg));
        }
    }

    // Negative distance at edge: should still be 0
    assert(calculate_falloff(1.0f, -8, 8) == 0.0f);

    // Negative distance beyond: should still be 0
    assert(calculate_falloff(1.0f, -10, 8) == 0.0f);

    printf("  PASS: Negative distances handled correctly\n");
}

// =============================================================================
// Zero radius edge case
// =============================================================================

void test_zero_radius() {
    printf("Testing zero radius always returns 0...\n");

    assert(calculate_falloff(1.0f, 0, 0) == 0.0f);
    assert(calculate_falloff(1.0f, 1, 0) == 0.0f);
    assert(calculate_falloff(0.5f, 0, 0) == 0.0f);

    printf("  PASS: Zero radius returns 0\n");
}

// =============================================================================
// Negative radius edge case
// =============================================================================

void test_negative_radius() {
    printf("Testing negative radius always returns 0...\n");

    assert(calculate_falloff(1.0f, 0, -1) == 0.0f);
    assert(calculate_falloff(1.0f, 0, -8) == 0.0f);
    assert(calculate_falloff(1.0f, 5, -8) == 0.0f);

    printf("  PASS: Negative radius returns 0\n");
}

// =============================================================================
// Monotonic decrease verification
// =============================================================================

void test_monotonic_decrease() {
    printf("Testing falloff is monotonically decreasing with distance...\n");

    int radii[] = { 5, 8, 12, 16, 20 };
    int num_radii = sizeof(radii) / sizeof(radii[0]);

    for (int i = 0; i < num_radii; ++i) {
        int r = radii[i];
        float prev = calculate_falloff(1.0f, 0, r);
        for (int d = 1; d <= r; ++d) {
            float curr = calculate_falloff(1.0f, d, r);
            assert(curr <= prev);
            prev = curr;
        }
    }

    printf("  PASS: Falloff is monotonically decreasing\n");
}

// =============================================================================
// Main
// =============================================================================

int main() {
    printf("=== Linear Distance Falloff Unit Tests (Epic 9, Ticket E9-021) ===\n\n");

    // Exact formula
    test_exact_formula();

    // Boundary conditions
    test_distance_zero_full_strength();
    test_distance_equals_radius();
    test_distance_beyond_radius();

    // Linearity
    test_linearity();

    // Effectiveness values
    test_effectiveness_zero();
    test_effectiveness_half();
    test_effectiveness_full();

    // Different radii
    test_radius_1();
    test_radius_5();
    test_radius_8();
    test_radius_12();
    test_radius_16();
    test_radius_20();

    // Edge cases
    test_negative_distance();
    test_zero_radius();
    test_negative_radius();

    // Properties
    test_monotonic_decrease();

    printf("\n=== All Linear Falloff Tests Passed ===\n");
    return 0;
}
