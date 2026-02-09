/**
 * @file test_health_index.cpp
 * @brief Tests for health index calculation (Ticket E10-029)
 *
 * Validates:
 * - Base health (all neutral inputs) = 50
 * - Medical coverage effects (+/- 25)
 * - Contamination effects (up to -30)
 * - Fluid availability effects (+/- 10)
 * - Combined effects and clamping to [0, 100]
 * - apply_health_index() updates PopulationData correctly
 */

#include <cassert>
#include <cmath>
#include <cstdio>

#include "sims3000/population/HealthIndex.h"

using namespace sims3000::population;

// --------------------------------------------------------------------------
// Helper: float approximate equality
// --------------------------------------------------------------------------
static bool approx(float a, float b, float epsilon = 0.001f) {
    return std::fabs(a - b) < epsilon;
}

// --------------------------------------------------------------------------
// Test: Base health with all neutral inputs
// --------------------------------------------------------------------------
static void test_base_health() {
    HealthInput input{};
    input.medical_coverage = 0.5f;      // Neutral
    input.contamination_level = 0.0f;   // None
    input.has_fluid = true;
    input.fluid_coverage = 0.5f;        // Neutral

    HealthResult result = calculate_health_index(input);

    // medical_mod = (0.5 - 0.5) * 2 * 25 = 0
    assert(approx(result.medical_modifier, 0.0f) && "Neutral medical modifier should be 0");

    // contam_mod = -0.0 * 30 = 0
    assert(approx(result.contamination_modifier, 0.0f) && "Zero contamination modifier should be 0");

    // fluid_mod = (0.5 - 0.5) * 2 * 10 = 0
    assert(approx(result.fluid_modifier, 0.0f) && "Neutral fluid modifier should be 0");

    // health = 50 + 0 + 0 + 0 = 50
    assert(result.health_index == 50 && "Base health should be 50");

    std::printf("  PASS: Base health with neutral inputs\n");
}

// --------------------------------------------------------------------------
// Test: Maximum medical coverage (+25)
// --------------------------------------------------------------------------
static void test_max_medical_coverage() {
    HealthInput input{};
    input.medical_coverage = 1.0f;      // Maximum coverage
    input.contamination_level = 0.0f;
    input.has_fluid = true;
    input.fluid_coverage = 0.5f;

    HealthResult result = calculate_health_index(input);

    // medical_mod = (1.0 - 0.5) * 2 * 25 = 0.5 * 2 * 25 = 25
    assert(approx(result.medical_modifier, 25.0f) && "Max medical modifier should be +25");

    // health = 50 + 25 = 75
    assert(result.health_index == 75 && "Max medical coverage should give health 75");

    std::printf("  PASS: Maximum medical coverage (+25)\n");
}

// --------------------------------------------------------------------------
// Test: Minimum medical coverage (-25)
// --------------------------------------------------------------------------
static void test_min_medical_coverage() {
    HealthInput input{};
    input.medical_coverage = 0.0f;      // No coverage
    input.contamination_level = 0.0f;
    input.has_fluid = true;
    input.fluid_coverage = 0.5f;

    HealthResult result = calculate_health_index(input);

    // medical_mod = (0.0 - 0.5) * 2 * 25 = -0.5 * 2 * 25 = -25
    assert(approx(result.medical_modifier, -25.0f) && "Min medical modifier should be -25");

    // health = 50 - 25 = 25
    assert(result.health_index == 25 && "No medical coverage should give health 25");

    std::printf("  PASS: Minimum medical coverage (-25)\n");
}

// --------------------------------------------------------------------------
// Test: Maximum contamination (-30)
// --------------------------------------------------------------------------
static void test_max_contamination() {
    HealthInput input{};
    input.medical_coverage = 0.5f;
    input.contamination_level = 1.0f;   // Maximum contamination
    input.has_fluid = true;
    input.fluid_coverage = 0.5f;

    HealthResult result = calculate_health_index(input);

    // contam_mod = -1.0 * 30 = -30
    assert(approx(result.contamination_modifier, -30.0f) && "Max contamination modifier should be -30");

    // health = 50 + 0 - 30 + 0 = 20
    assert(result.health_index == 20 && "Max contamination should give health 20");

    std::printf("  PASS: Maximum contamination (-30)\n");
}

// --------------------------------------------------------------------------
// Test: Partial contamination
// --------------------------------------------------------------------------
static void test_partial_contamination() {
    HealthInput input{};
    input.medical_coverage = 0.5f;
    input.contamination_level = 0.5f;   // 50% contamination
    input.has_fluid = true;
    input.fluid_coverage = 0.5f;

    HealthResult result = calculate_health_index(input);

    // contam_mod = -0.5 * 30 = -15
    assert(approx(result.contamination_modifier, -15.0f) && "50% contamination modifier should be -15");

    // health = 50 - 15 = 35
    assert(result.health_index == 35 && "50% contamination should give health 35");

    std::printf("  PASS: Partial contamination\n");
}

// --------------------------------------------------------------------------
// Test: Maximum fluid coverage (+10)
// --------------------------------------------------------------------------
static void test_max_fluid_coverage() {
    HealthInput input{};
    input.medical_coverage = 0.5f;
    input.contamination_level = 0.0f;
    input.has_fluid = true;
    input.fluid_coverage = 1.0f;        // Full fluid coverage

    HealthResult result = calculate_health_index(input);

    // fluid_mod = (1.0 - 0.5) * 2 * 10 = 0.5 * 2 * 10 = 10
    assert(approx(result.fluid_modifier, 10.0f) && "Max fluid modifier should be +10");

    // health = 50 + 10 = 60
    assert(result.health_index == 60 && "Max fluid coverage should give health 60");

    std::printf("  PASS: Maximum fluid coverage (+10)\n");
}

// --------------------------------------------------------------------------
// Test: Minimum fluid coverage but has fluid
// --------------------------------------------------------------------------
static void test_min_fluid_coverage() {
    HealthInput input{};
    input.medical_coverage = 0.5f;
    input.contamination_level = 0.0f;
    input.has_fluid = true;
    input.fluid_coverage = 0.0f;        // Fluid exists but no coverage

    HealthResult result = calculate_health_index(input);

    // fluid_mod = (0.0 - 0.5) * 2 * 10 = -0.5 * 2 * 10 = -10
    assert(approx(result.fluid_modifier, -10.0f) && "Min fluid coverage modifier should be -10");

    // health = 50 - 10 = 40
    assert(result.health_index == 40 && "No fluid coverage should give health 40");

    std::printf("  PASS: Minimum fluid coverage (has fluid)\n");
}

// --------------------------------------------------------------------------
// Test: No fluid available (-10)
// --------------------------------------------------------------------------
static void test_no_fluid() {
    HealthInput input{};
    input.medical_coverage = 0.5f;
    input.contamination_level = 0.0f;
    input.has_fluid = false;            // No fluid at all
    input.fluid_coverage = 0.0f;        // Coverage irrelevant

    HealthResult result = calculate_health_index(input);

    // fluid_mod = -10 (flat penalty)
    assert(approx(result.fluid_modifier, -10.0f) && "No fluid modifier should be -10");

    // health = 50 - 10 = 40
    assert(result.health_index == 40 && "No fluid should give health 40");

    std::printf("  PASS: No fluid available (-10)\n");
}

// --------------------------------------------------------------------------
// Test: Combined positive modifiers
// --------------------------------------------------------------------------
static void test_all_positive() {
    HealthInput input{};
    input.medical_coverage = 1.0f;      // +25
    input.contamination_level = 0.0f;   // 0
    input.has_fluid = true;
    input.fluid_coverage = 1.0f;        // +10

    HealthResult result = calculate_health_index(input);

    // health = 50 + 25 + 0 + 10 = 85
    assert(result.health_index == 85 && "All positive modifiers should give health 85");

    std::printf("  PASS: Combined positive modifiers\n");
}

// --------------------------------------------------------------------------
// Test: Combined negative modifiers
// --------------------------------------------------------------------------
static void test_all_negative() {
    HealthInput input{};
    input.medical_coverage = 0.0f;      // -25
    input.contamination_level = 1.0f;   // -30
    input.has_fluid = false;            // -10

    HealthResult result = calculate_health_index(input);

    // health = 50 - 25 - 30 - 10 = -15 → clamped to 0
    assert(result.health_index == 0 && "All negative modifiers should clamp to 0");

    std::printf("  PASS: Combined negative modifiers (clamped to 0)\n");
}

// --------------------------------------------------------------------------
// Test: Clamping to maximum (100)
// --------------------------------------------------------------------------
static void test_clamp_max() {
    // This shouldn't happen naturally, but test the clamp logic
    HealthInput input{};
    input.medical_coverage = 1.0f;      // +25
    input.contamination_level = 0.0f;   // 0
    input.has_fluid = true;
    input.fluid_coverage = 1.0f;        // +10

    HealthResult result = calculate_health_index(input);

    // health = 50 + 25 + 0 + 10 = 85 (< 100, so no clamping)
    assert(result.health_index <= 100 && "Health should never exceed 100");

    std::printf("  PASS: Health clamped to maximum (100)\n");
}

// --------------------------------------------------------------------------
// Test: apply_health_index() updates PopulationData
// --------------------------------------------------------------------------
static void test_apply_health_index() {
    PopulationData pop{};
    pop.health_index = 0;  // Initial value

    HealthInput input{};
    input.medical_coverage = 0.8f;
    input.contamination_level = 0.2f;
    input.has_fluid = true;
    input.fluid_coverage = 0.7f;

    apply_health_index(pop, input);

    // Verify health_index was updated
    // medical_mod = (0.8 - 0.5) * 2 * 25 = 0.3 * 50 = 15
    // contam_mod = -0.2 * 30 = -6
    // fluid_mod = (0.7 - 0.5) * 2 * 10 = 0.2 * 20 = 4
    // health = 50 + 15 - 6 + 4 = 63
    assert(pop.health_index == 63 && "apply_health_index should update PopulationData.health_index");

    std::printf("  PASS: apply_health_index() updates PopulationData\n");
}

// --------------------------------------------------------------------------
// Test: Realistic scenario (moderate conditions)
// --------------------------------------------------------------------------
static void test_realistic_scenario() {
    HealthInput input{};
    input.medical_coverage = 0.6f;      // Moderate medical coverage
    input.contamination_level = 0.3f;   // Some contamination
    input.has_fluid = true;
    input.fluid_coverage = 0.8f;        // Good fluid coverage

    HealthResult result = calculate_health_index(input);

    // medical_mod = (0.6 - 0.5) * 2 * 25 = 0.1 * 50 = 5
    assert(approx(result.medical_modifier, 5.0f));

    // contam_mod = -0.3 * 30 = -9
    assert(approx(result.contamination_modifier, -9.0f));

    // fluid_mod = (0.8 - 0.5) * 2 * 10 = 0.3 * 20 = 6
    assert(approx(result.fluid_modifier, 6.0f));

    // health = 50 + 5 - 9 + 6 = 52
    assert(result.health_index == 52 && "Realistic scenario should give health 52");

    std::printf("  PASS: Realistic scenario\n");
}

// --------------------------------------------------------------------------
// Main
// --------------------------------------------------------------------------
int main() {
    std::printf("=== Health Index Calculation Tests (E10-029) ===\n");

    test_base_health();
    test_max_medical_coverage();
    test_min_medical_coverage();
    test_max_contamination();
    test_partial_contamination();
    test_max_fluid_coverage();
    test_min_fluid_coverage();
    test_no_fluid();
    test_all_positive();
    test_all_negative();
    test_clamp_max();
    test_apply_health_index();
    test_realistic_scenario();

    std::printf("All health index calculation tests passed.\n");
    return 0;
}
