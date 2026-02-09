/**
 * @file test_birth_rate.cpp
 * @brief Tests for birth rate calculation (Ticket E10-015)
 *
 * Validates:
 * - Default population: verify base birth rate
 * - High harmony/health/housing: verify increased rate
 * - Zero population: zero births
 * - Zero housing: zero births
 * - Overcrowded (housing << pop): minimal births
 * - Minimum 1 birth when pop > 0 and housing available
 */

#include <cassert>
#include <cmath>
#include <cstdio>

#include "sims3000/population/BirthRateCalculation.h"

using namespace sims3000::population;

// --------------------------------------------------------------------------
// Helper: float approximate equality
// --------------------------------------------------------------------------
static bool approx(float a, float b, float epsilon = 0.001f) {
    return std::fabs(a - b) < epsilon;
}

// --------------------------------------------------------------------------
// Test: Default population produces expected base birth rate
// --------------------------------------------------------------------------
static void test_default_population_birth_rate() {
    PopulationData pop{};
    pop.total_beings = 1000;
    pop.harmony_index = 50;
    pop.health_index = 50;

    // Available housing matches population
    uint32_t available_housing = 1000;

    BirthRateResult result = calculate_birth_rate(pop, available_housing);

    // harmony_modifier = lerp(0.5, 1.5, 0.5) = 1.0
    assert(approx(result.harmony_modifier, 1.0f) && "Default harmony modifier should be 1.0");

    // health_modifier = lerp(0.7, 1.2, 0.5) = 0.95
    assert(approx(result.health_modifier, 0.95f) && "Default health modifier should be 0.95");

    // housing_modifier = lerp(0.1, 1.0, min(1.0, 1000/1000)) = lerp(0.1, 1.0, 1.0) = 1.0
    assert(approx(result.housing_modifier, 1.0f) && "Housing modifier with matching capacity should be 1.0");

    // effective_rate = 0.015 * 1.0 * 0.95 * 1.0 = 0.01425
    assert(approx(result.effective_rate, 0.01425f) && "Effective rate should be ~0.01425");

    // births = round(1000 * 0.01425) = round(14.25) = 14
    assert(result.births == 14 && "Should produce 14 births for 1000 population at default rates");

    std::printf("  PASS: Default population birth rate\n");
}

// --------------------------------------------------------------------------
// Test: High harmony/health/housing increases birth rate
// --------------------------------------------------------------------------
static void test_high_modifiers_increase_rate() {
    PopulationData pop{};
    pop.total_beings = 1000;
    pop.harmony_index = 100;  // Max harmony
    pop.health_index = 100;   // Max health

    uint32_t available_housing = 2000;  // Plenty of housing

    BirthRateResult result = calculate_birth_rate(pop, available_housing);

    // harmony_modifier = lerp(0.5, 1.5, 1.0) = 1.5
    assert(approx(result.harmony_modifier, 1.5f) && "Max harmony modifier should be 1.5");

    // health_modifier = lerp(0.7, 1.2, 1.0) = 1.2
    assert(approx(result.health_modifier, 1.2f) && "Max health modifier should be 1.2");

    // housing_modifier = lerp(0.1, 1.0, 1.0) = 1.0 (clamped ratio)
    assert(approx(result.housing_modifier, 1.0f) && "Housing modifier capped at 1.0");

    // effective_rate = 0.015 * 1.5 * 1.2 * 1.0 = 0.027
    assert(approx(result.effective_rate, 0.027f) && "High modifier effective rate should be ~0.027");

    // births = round(1000 * 0.027) = 27
    assert(result.births == 27 && "Should produce 27 births with high modifiers");

    std::printf("  PASS: High harmony/health/housing increases birth rate\n");
}

// --------------------------------------------------------------------------
// Test: Zero population produces zero births
// --------------------------------------------------------------------------
static void test_zero_population() {
    PopulationData pop{};
    pop.total_beings = 0;

    BirthRateResult result = calculate_birth_rate(pop, 100);

    assert(result.births == 0 && "Zero population should produce zero births");
    assert(result.effective_rate == 0.0f && "Effective rate should be 0 with zero population");

    std::printf("  PASS: Zero population produces zero births\n");
}

// --------------------------------------------------------------------------
// Test: Zero housing produces zero births
// --------------------------------------------------------------------------
static void test_zero_housing() {
    PopulationData pop{};
    pop.total_beings = 1000;
    pop.harmony_index = 50;
    pop.health_index = 50;

    BirthRateResult result = calculate_birth_rate(pop, 0);

    assert(result.births == 0 && "Zero housing should produce zero births");
    assert(result.effective_rate == 0.0f && "Effective rate should be 0 with zero housing");

    std::printf("  PASS: Zero housing produces zero births\n");
}

// --------------------------------------------------------------------------
// Test: Overcrowded city (housing << population) produces minimal births
// --------------------------------------------------------------------------
static void test_overcrowded() {
    PopulationData pop{};
    pop.total_beings = 10000;
    pop.harmony_index = 50;
    pop.health_index = 50;

    // Very little housing relative to population
    uint32_t available_housing = 100;  // 1% housing ratio

    BirthRateResult result = calculate_birth_rate(pop, available_housing);

    // housing_ratio = min(1.0, 100/10000) = 0.01
    // housing_modifier = lerp(0.1, 1.0, 0.01) = 0.1 + 0.9 * 0.01 = 0.109
    assert(approx(result.housing_modifier, 0.109f) && "Overcrowded housing modifier should be ~0.109");

    // Births should still be at least 1 (minimum rule)
    assert(result.births >= 1 && "Should still produce at least 1 birth");

    // But significantly fewer than with adequate housing
    PopulationData pop2 = pop;
    BirthRateResult result_adequate = calculate_birth_rate(pop2, 10000);
    assert(result.births < result_adequate.births && "Overcrowded should produce fewer births");

    std::printf("  PASS: Overcrowded city produces minimal births\n");
}

// --------------------------------------------------------------------------
// Test: Minimum 1 birth when population > 0 and housing available
// --------------------------------------------------------------------------
static void test_minimum_one_birth() {
    PopulationData pop{};
    pop.total_beings = 1;       // Tiny population
    pop.harmony_index = 0;      // Worst harmony
    pop.health_index = 0;       // Worst health

    uint32_t available_housing = 1;  // Minimal housing

    BirthRateResult result = calculate_birth_rate(pop, available_housing);

    // Even with terrible conditions, minimum 1 birth
    assert(result.births >= 1 && "Should produce at least 1 birth when pop > 0 and housing > 0");

    std::printf("  PASS: Minimum 1 birth when pop > 0 and housing available\n");
}

// --------------------------------------------------------------------------
// Test: Low harmony reduces birth rate
// --------------------------------------------------------------------------
static void test_low_harmony() {
    PopulationData pop{};
    pop.total_beings = 1000;
    pop.harmony_index = 0;      // Minimum harmony
    pop.health_index = 50;

    uint32_t available_housing = 1000;

    BirthRateResult result = calculate_birth_rate(pop, available_housing);

    // harmony_modifier = lerp(0.5, 1.5, 0.0) = 0.5
    assert(approx(result.harmony_modifier, 0.5f) && "Zero harmony modifier should be 0.5");

    // effective_rate = 0.015 * 0.5 * 0.95 * 1.0 = 0.007125
    assert(approx(result.effective_rate, 0.007125f) && "Low harmony effective rate should be ~0.007125");

    std::printf("  PASS: Low harmony reduces birth rate\n");
}

// --------------------------------------------------------------------------
// Test: Large population scaling
// --------------------------------------------------------------------------
static void test_large_population() {
    PopulationData pop{};
    pop.total_beings = 100000;
    pop.harmony_index = 50;
    pop.health_index = 50;

    uint32_t available_housing = 100000;

    BirthRateResult result = calculate_birth_rate(pop, available_housing);

    // births = round(100000 * 0.01425) = 1425
    assert(result.births == 1425 && "Large population should scale births correctly");

    std::printf("  PASS: Large population scaling\n");
}

// --------------------------------------------------------------------------
// Main
// --------------------------------------------------------------------------
int main() {
    std::printf("=== Birth Rate Calculation Tests (E10-015) ===\n");

    test_default_population_birth_rate();
    test_high_modifiers_increase_rate();
    test_zero_population();
    test_zero_housing();
    test_overcrowded();
    test_minimum_one_birth();
    test_low_harmony();
    test_large_population();

    std::printf("All birth rate calculation tests passed.\n");
    return 0;
}
