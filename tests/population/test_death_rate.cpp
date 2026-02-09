/**
 * @file test_death_rate.cpp
 * @brief Tests for death rate calculation (Ticket E10-016)
 *
 * Validates:
 * - Default population: verify base death rate
 * - High contamination: increased deaths
 * - Low services: increased deaths
 * - High elder percent: increased deaths
 * - Deaths capped at 5% of population
 * - Zero population: zero deaths
 */

#include <cassert>
#include <cmath>
#include <cstdio>

#include "sims3000/population/DeathRateCalculation.h"

using namespace sims3000::population;

// --------------------------------------------------------------------------
// Helper: float approximate equality
// --------------------------------------------------------------------------
static bool approx(float a, float b, float epsilon = 0.001f) {
    return std::fabs(a - b) < epsilon;
}

// --------------------------------------------------------------------------
// Test: Default population produces expected base death rate
// --------------------------------------------------------------------------
static void test_default_population_death_rate() {
    PopulationData pop{};
    pop.total_beings = 1000;
    pop.health_index = 50;
    pop.elder_percent = 33;

    float contamination = 0.0f;    // No contamination
    float service_coverage = 50.0f; // Average services

    DeathRateResult result = calculate_death_rate(pop, contamination, service_coverage);

    // health_modifier = lerp(0.5, 1.5, 1.0 - 0.5) = lerp(0.5, 1.5, 0.5) = 1.0
    assert(approx(result.health_modifier, 1.0f) && "Default health modifier should be 1.0");

    // contamination_modifier = lerp(1.0, 2.0, 0.0) = 1.0
    assert(approx(result.contamination_modifier, 1.0f) && "Zero contamination modifier should be 1.0");

    // services_modifier = lerp(0.7, 1.3, 1.0 - 0.5) = lerp(0.7, 1.3, 0.5) = 1.0
    assert(approx(result.services_modifier, 1.0f) && "Default services modifier should be 1.0");

    // age_modifier = lerp(0.5, 2.0, 33/100) = lerp(0.5, 2.0, 0.33) = 0.5 + 1.5 * 0.33 = 0.995
    assert(approx(result.age_modifier, 0.995f) && "Default age modifier should be ~0.995");

    // effective_rate = 0.008 * 1.0 * 1.0 * 1.0 * 0.995 = ~0.00796
    assert(approx(result.effective_rate, 0.00796f) && "Effective rate should be ~0.00796");

    // deaths = round(1000 * 0.00796) = round(7.96) = 8
    assert(result.deaths == 8 && "Should produce ~8 deaths for 1000 population at default rates");

    std::printf("  PASS: Default population death rate\n");
}

// --------------------------------------------------------------------------
// Test: High contamination increases death rate
// --------------------------------------------------------------------------
static void test_high_contamination() {
    PopulationData pop{};
    pop.total_beings = 1000;
    pop.health_index = 50;
    pop.elder_percent = 33;

    float contamination = 100.0f;   // Maximum contamination
    float service_coverage = 50.0f;

    DeathRateResult result = calculate_death_rate(pop, contamination, service_coverage);

    // contamination_modifier = lerp(1.0, 2.0, 1.0) = 2.0
    assert(approx(result.contamination_modifier, 2.0f) && "Max contamination modifier should be 2.0");

    // Deaths should be significantly higher than default
    DeathRateResult default_result = calculate_death_rate(pop, 0.0f, service_coverage);
    assert(result.deaths > default_result.deaths && "High contamination should increase deaths");

    std::printf("  PASS: High contamination increases death rate\n");
}

// --------------------------------------------------------------------------
// Test: Low services increases death rate
// --------------------------------------------------------------------------
static void test_low_services() {
    PopulationData pop{};
    pop.total_beings = 1000;
    pop.health_index = 50;
    pop.elder_percent = 33;

    float contamination = 0.0f;
    float service_coverage = 0.0f;  // No service coverage

    DeathRateResult result = calculate_death_rate(pop, contamination, service_coverage);

    // services_modifier = lerp(0.7, 1.3, 1.0 - 0.0) = lerp(0.7, 1.3, 1.0) = 1.3
    assert(approx(result.services_modifier, 1.3f) && "Zero coverage services modifier should be 1.3");

    // Compare with full coverage
    DeathRateResult full_svc = calculate_death_rate(pop, contamination, 100.0f);
    // services_modifier at full coverage = lerp(0.7, 1.3, 0.0) = 0.7
    assert(approx(full_svc.services_modifier, 0.7f) && "Full coverage services modifier should be 0.7");

    assert(result.deaths > full_svc.deaths && "Low services should increase deaths");

    std::printf("  PASS: Low services increases death rate\n");
}

// --------------------------------------------------------------------------
// Test: High elder percent increases death rate
// --------------------------------------------------------------------------
static void test_high_elder_percent() {
    PopulationData pop_young{};
    pop_young.total_beings = 1000;
    pop_young.health_index = 50;
    pop_young.elder_percent = 5;  // Young population

    PopulationData pop_old{};
    pop_old.total_beings = 1000;
    pop_old.health_index = 50;
    pop_old.elder_percent = 80;  // Elderly population

    DeathRateResult result_young = calculate_death_rate(pop_young, 0.0f, 50.0f);
    DeathRateResult result_old = calculate_death_rate(pop_old, 0.0f, 50.0f);

    // age_modifier for young = lerp(0.5, 2.0, 0.05) = 0.575
    assert(approx(result_young.age_modifier, 0.575f) && "Young age modifier should be ~0.575");

    // age_modifier for old = lerp(0.5, 2.0, 0.80) = 1.7
    assert(approx(result_old.age_modifier, 1.7f) && "Old age modifier should be ~1.7");

    assert(result_old.deaths > result_young.deaths && "High elder percent should increase deaths");

    std::printf("  PASS: High elder percent increases death rate\n");
}

// --------------------------------------------------------------------------
// Test: Deaths capped at 5% of population
// --------------------------------------------------------------------------
static void test_death_cap() {
    PopulationData pop{};
    pop.total_beings = 1000;
    pop.health_index = 0;       // Terrible health (max health modifier)
    pop.elder_percent = 100;    // All elders (max age modifier)

    float contamination = 100.0f;   // Max contamination
    float service_coverage = 0.0f;  // No services

    DeathRateResult result = calculate_death_rate(pop, contamination, service_coverage);

    // Cap = 5% of 1000 = 50
    uint32_t cap = static_cast<uint32_t>(std::round(
        constants::MAX_DEATH_PERCENT * static_cast<float>(pop.total_beings)
    ));
    assert(cap == 50 && "Death cap should be 50 for 1000 population");
    assert(result.deaths <= cap && "Deaths should be capped at 5% of population");
    assert(result.deaths == cap && "With worst conditions, deaths should hit the cap");

    std::printf("  PASS: Deaths capped at 5%% of population\n");
}

// --------------------------------------------------------------------------
// Test: Zero population produces zero deaths
// --------------------------------------------------------------------------
static void test_zero_population() {
    PopulationData pop{};
    pop.total_beings = 0;

    DeathRateResult result = calculate_death_rate(pop, 50.0f, 50.0f);

    assert(result.deaths == 0 && "Zero population should produce zero deaths");
    assert(result.effective_rate == 0.0f && "Effective rate should be 0 with zero population");

    std::printf("  PASS: Zero population produces zero deaths\n");
}

// --------------------------------------------------------------------------
// Test: Good health reduces death rate
// --------------------------------------------------------------------------
static void test_good_health() {
    PopulationData pop{};
    pop.total_beings = 1000;
    pop.health_index = 100;     // Perfect health
    pop.elder_percent = 33;

    DeathRateResult result = calculate_death_rate(pop, 0.0f, 50.0f);

    // health_modifier = lerp(0.5, 1.5, 1.0 - 1.0) = lerp(0.5, 1.5, 0.0) = 0.5
    assert(approx(result.health_modifier, 0.5f) && "Perfect health modifier should be 0.5");

    // Compare with worst health
    PopulationData pop_sick = pop;
    pop_sick.health_index = 0;
    DeathRateResult result_sick = calculate_death_rate(pop_sick, 0.0f, 50.0f);
    // health_modifier = lerp(0.5, 1.5, 1.0) = 1.5
    assert(approx(result_sick.health_modifier, 1.5f) && "Worst health modifier should be 1.5");

    assert(result.deaths < result_sick.deaths && "Good health should reduce deaths");

    std::printf("  PASS: Good health reduces death rate\n");
}

// --------------------------------------------------------------------------
// Test: Large population scaling with cap
// --------------------------------------------------------------------------
static void test_large_population_cap() {
    PopulationData pop{};
    pop.total_beings = 100000;
    pop.health_index = 0;
    pop.elder_percent = 100;

    DeathRateResult result = calculate_death_rate(pop, 100.0f, 0.0f);

    // Cap = 5% of 100000 = 5000
    assert(result.deaths <= 5000 && "Deaths should be capped at 5000 for 100k population");

    std::printf("  PASS: Large population scaling with cap\n");
}

// --------------------------------------------------------------------------
// Main
// --------------------------------------------------------------------------
int main() {
    std::printf("=== Death Rate Calculation Tests (E10-016) ===\n");

    test_default_population_death_rate();
    test_high_contamination();
    test_low_services();
    test_high_elder_percent();
    test_death_cap();
    test_zero_population();
    test_good_health();
    test_large_population_cap();

    std::printf("All death rate calculation tests passed.\n");
    return 0;
}
