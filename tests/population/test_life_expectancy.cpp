/**
 * @file test_life_expectancy.cpp
 * @brief Tests for life expectancy calculation (Ticket E10-028)
 *
 * Validates:
 * - Default conditions: verify base life expectancy
 * - Optimal conditions: verify maximum modifiers
 * - Poor conditions: verify minimum modifiers
 * - Contamination impact (inverted)
 * - Disorder impact (inverted)
 * - Clamping to 30-120 range
 * - Individual modifier calculations
 */

#include <cassert>
#include <cmath>
#include <cstdio>

#include "sims3000/population/LifeExpectancy.h"

using namespace sims3000::population;

// --------------------------------------------------------------------------
// Helper: float approximate equality
// --------------------------------------------------------------------------
static bool approx(float a, float b, float epsilon = 0.001f) {
    return std::fabs(a - b) < epsilon;
}

// --------------------------------------------------------------------------
// Test: Default conditions (all indices at 50)
// --------------------------------------------------------------------------
static void test_default_conditions() {
    LifeExpectancyInput input{};
    input.health_index = 50;
    input.contamination_level = 50;
    input.disorder_level = 50;
    input.education_index = 50;
    input.harmony_index = 50;

    LifeExpectancyResult result = calculate_life_expectancy(input);

    // health_modifier = lerp(0.7, 1.3, 0.5) = 1.0
    assert(approx(result.health_modifier, 1.0f) && "Default health modifier should be 1.0");

    // contamination_modifier = lerp(1.0, 0.6, 0.5) = 0.8
    assert(approx(result.contamination_modifier, 0.8f) && "Default contamination modifier should be 0.8");

    // disorder_modifier = lerp(1.0, 0.9, 0.5) = 0.95
    assert(approx(result.disorder_modifier, 0.95f) && "Default disorder modifier should be 0.95");

    // education_modifier = lerp(0.95, 1.1, 0.5) = 1.025
    assert(approx(result.education_modifier, 1.025f) && "Default education modifier should be 1.025");

    // harmony_modifier = lerp(0.9, 1.1, 0.5) = 1.0
    assert(approx(result.harmony_modifier, 1.0f) && "Default harmony modifier should be 1.0");

    // combined = 1.0 * 0.8 * 0.95 * 1.025 * 1.0 = 0.779
    // life_expectancy = 75 * 0.779 = 58.425
    assert(approx(result.life_expectancy, 58.425f, 0.01f) && "Default life expectancy should be ~58.4");

    std::printf("  PASS: Default conditions\n");
}

// --------------------------------------------------------------------------
// Test: Optimal conditions (all positive indices at 100, negative at 0)
// --------------------------------------------------------------------------
static void test_optimal_conditions() {
    LifeExpectancyInput input{};
    input.health_index = 100;          // Max health
    input.contamination_level = 0;     // Min contamination (optimal)
    input.disorder_level = 0;          // Min disorder (optimal)
    input.education_index = 100;       // Max education
    input.harmony_index = 100;         // Max harmony

    LifeExpectancyResult result = calculate_life_expectancy(input);

    // health_modifier = lerp(0.7, 1.3, 1.0) = 1.3
    assert(approx(result.health_modifier, 1.3f) && "Optimal health modifier should be 1.3");

    // contamination_modifier = lerp(1.0, 0.6, 0.0) = 1.0 (no contamination)
    assert(approx(result.contamination_modifier, 1.0f) && "Optimal contamination modifier should be 1.0");

    // disorder_modifier = lerp(1.0, 0.9, 0.0) = 1.0 (no disorder)
    assert(approx(result.disorder_modifier, 1.0f) && "Optimal disorder modifier should be 1.0");

    // education_modifier = lerp(0.95, 1.1, 1.0) = 1.1
    assert(approx(result.education_modifier, 1.1f) && "Optimal education modifier should be 1.1");

    // harmony_modifier = lerp(0.9, 1.1, 1.0) = 1.1
    assert(approx(result.harmony_modifier, 1.1f) && "Optimal harmony modifier should be 1.1");

    // combined = 1.3 * 1.0 * 1.0 * 1.1 * 1.1 = 1.573
    // life_expectancy = 75 * 1.573 = 117.975, clamped to 120
    assert(approx(result.life_expectancy, 117.975f, 0.01f) && "Optimal life expectancy should be ~118");

    std::printf("  PASS: Optimal conditions\n");
}

// --------------------------------------------------------------------------
// Test: Poor conditions (all indices worst case)
// --------------------------------------------------------------------------
static void test_poor_conditions() {
    LifeExpectancyInput input{};
    input.health_index = 0;            // Min health
    input.contamination_level = 100;   // Max contamination (worst)
    input.disorder_level = 100;        // Max disorder (worst)
    input.education_index = 0;         // Min education
    input.harmony_index = 0;           // Min harmony

    LifeExpectancyResult result = calculate_life_expectancy(input);

    // health_modifier = lerp(0.7, 1.3, 0.0) = 0.7
    assert(approx(result.health_modifier, 0.7f) && "Poor health modifier should be 0.7");

    // contamination_modifier = lerp(1.0, 0.6, 1.0) = 0.6 (max contamination)
    assert(approx(result.contamination_modifier, 0.6f) && "Poor contamination modifier should be 0.6");

    // disorder_modifier = lerp(1.0, 0.9, 1.0) = 0.9 (max disorder)
    assert(approx(result.disorder_modifier, 0.9f) && "Poor disorder modifier should be 0.9");

    // education_modifier = lerp(0.95, 1.1, 0.0) = 0.95
    assert(approx(result.education_modifier, 0.95f) && "Poor education modifier should be 0.95");

    // harmony_modifier = lerp(0.9, 1.1, 0.0) = 0.9
    assert(approx(result.harmony_modifier, 0.9f) && "Poor harmony modifier should be 0.9");

    // combined = 0.7 * 0.6 * 0.9 * 0.95 * 0.9 = 0.32319
    // life_expectancy = 75 * 0.32319 = 24.239, clamped to 30
    assert(approx(result.life_expectancy, 30.0f) && "Poor conditions should clamp to minimum 30");

    std::printf("  PASS: Poor conditions\n");
}

// --------------------------------------------------------------------------
// Test: Contamination is inverted (high contamination reduces expectancy)
// --------------------------------------------------------------------------
static void test_contamination_inverted() {
    LifeExpectancyInput input{};
    input.health_index = 50;
    input.contamination_level = 0;  // No contamination
    input.disorder_level = 50;
    input.education_index = 50;
    input.harmony_index = 50;

    LifeExpectancyResult result_clean = calculate_life_expectancy(input);

    input.contamination_level = 100;  // Max contamination
    LifeExpectancyResult result_dirty = calculate_life_expectancy(input);

    // Higher contamination should reduce life expectancy
    assert(result_clean.life_expectancy > result_dirty.life_expectancy
        && "Clean city should have higher life expectancy than contaminated");

    // Clean should have modifier 1.0, dirty should have modifier 0.6
    assert(approx(result_clean.contamination_modifier, 1.0f) && "Clean modifier should be 1.0");
    assert(approx(result_dirty.contamination_modifier, 0.6f) && "Dirty modifier should be 0.6");

    std::printf("  PASS: Contamination is inverted\n");
}

// --------------------------------------------------------------------------
// Test: Disorder is inverted (high disorder reduces expectancy)
// --------------------------------------------------------------------------
static void test_disorder_inverted() {
    LifeExpectancyInput input{};
    input.health_index = 50;
    input.contamination_level = 50;
    input.disorder_level = 0;  // No disorder
    input.education_index = 50;
    input.harmony_index = 50;

    LifeExpectancyResult result_peaceful = calculate_life_expectancy(input);

    input.disorder_level = 100;  // Max disorder
    LifeExpectancyResult result_chaotic = calculate_life_expectancy(input);

    // Higher disorder should reduce life expectancy
    assert(result_peaceful.life_expectancy > result_chaotic.life_expectancy
        && "Peaceful city should have higher life expectancy than chaotic");

    // Peaceful should have modifier 1.0, chaotic should have modifier 0.9
    assert(approx(result_peaceful.disorder_modifier, 1.0f) && "Peaceful modifier should be 1.0");
    assert(approx(result_chaotic.disorder_modifier, 0.9f) && "Chaotic modifier should be 0.9");

    std::printf("  PASS: Disorder is inverted\n");
}

// --------------------------------------------------------------------------
// Test: Maximum clamping
// --------------------------------------------------------------------------
static void test_maximum_clamping() {
    // Create conditions that would exceed 120
    LifeExpectancyInput input{};
    input.health_index = 100;
    input.contamination_level = 0;
    input.disorder_level = 0;
    input.education_index = 100;
    input.harmony_index = 100;

    LifeExpectancyResult result = calculate_life_expectancy(input);

    // Should not exceed 120
    assert(result.life_expectancy <= MAX_LIFE_EXPECTANCY && "Life expectancy should not exceed 120");

    std::printf("  PASS: Maximum clamping\n");
}

// --------------------------------------------------------------------------
// Test: Minimum clamping
// --------------------------------------------------------------------------
static void test_minimum_clamping() {
    // Worst possible conditions
    LifeExpectancyInput input{};
    input.health_index = 0;
    input.contamination_level = 100;
    input.disorder_level = 100;
    input.education_index = 0;
    input.harmony_index = 0;

    LifeExpectancyResult result = calculate_life_expectancy(input);

    // Should not go below 30
    assert(result.life_expectancy >= MIN_LIFE_EXPECTANCY && "Life expectancy should not go below 30");
    assert(approx(result.life_expectancy, 30.0f) && "Should clamp to exactly 30");

    std::printf("  PASS: Minimum clamping\n");
}

// --------------------------------------------------------------------------
// Test: Health has strongest impact
// --------------------------------------------------------------------------
static void test_health_impact() {
    LifeExpectancyInput input{};
    input.health_index = 0;
    input.contamination_level = 50;
    input.disorder_level = 50;
    input.education_index = 50;
    input.harmony_index = 50;

    LifeExpectancyResult result_poor = calculate_life_expectancy(input);

    input.health_index = 100;
    LifeExpectancyResult result_good = calculate_life_expectancy(input);

    // Health range is 0.7-1.3 (0.6 spread), largest of all modifiers
    float health_impact = result_good.life_expectancy - result_poor.life_expectancy;
    assert(health_impact > 20.0f && "Health should have significant impact on life expectancy");

    std::printf("  PASS: Health has strong impact\n");
}

// --------------------------------------------------------------------------
// Main
// --------------------------------------------------------------------------
int main() {
    std::printf("=== Life Expectancy Calculation Tests (E10-028) ===\n");

    test_default_conditions();
    test_optimal_conditions();
    test_poor_conditions();
    test_contamination_inverted();
    test_disorder_inverted();
    test_maximum_clamping();
    test_minimum_clamping();
    test_health_impact();

    std::printf("All life expectancy calculation tests passed.\n");
    return 0;
}
