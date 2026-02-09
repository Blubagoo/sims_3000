/**
 * @file test_natural_growth.cpp
 * @brief Tests for natural growth application (Ticket E10-017)
 *
 * Validates:
 * - Normal growth: births > deaths -> positive growth
 * - Deaths > births -> negative growth (pop decreases)
 * - Population never goes negative
 * - Zero population -> no change
 */

#include <cassert>
#include <cmath>
#include <cstdio>

#include "sims3000/population/NaturalGrowth.h"

using namespace sims3000::population;

// --------------------------------------------------------------------------
// Test: Normal growth (births > deaths) -> positive growth
// --------------------------------------------------------------------------
static void test_normal_positive_growth() {
    PopulationData pop{};
    pop.total_beings = 1000;
    pop.harmony_index = 50;
    pop.health_index = 50;
    pop.elder_percent = 33;

    uint32_t available_housing = 1000;
    float contamination = 0.0f;     // Clean city
    float service_coverage = 100.0f; // Full coverage

    NaturalGrowthResult result = apply_natural_growth(pop, available_housing,
                                                       contamination, service_coverage);

    // With good conditions, births should exceed deaths
    assert(result.births > 0 && "Should have births");
    assert(result.deaths >= 0 && "Deaths should be non-negative");
    assert(result.natural_growth > 0 && "Births should exceed deaths in good conditions");
    assert(result.new_total_beings > 1000 && "Population should increase");
    assert(pop.total_beings == result.new_total_beings && "PopulationData should be updated");
    assert(pop.natural_growth == result.natural_growth && "natural_growth field should be updated");

    std::printf("  PASS: Normal positive growth\n");
}

// --------------------------------------------------------------------------
// Test: Deaths > births -> negative growth (population decreases)
// --------------------------------------------------------------------------
static void test_negative_growth() {
    PopulationData pop{};
    pop.total_beings = 1000;
    pop.harmony_index = 0;       // Terrible harmony -> low births
    pop.health_index = 0;        // Terrible health -> more deaths
    pop.elder_percent = 100;     // All elderly -> high deaths

    // Very little housing, high contamination, no services
    uint32_t available_housing = 10;
    float contamination = 100.0f;
    float service_coverage = 0.0f;

    NaturalGrowthResult result = apply_natural_growth(pop, available_housing,
                                                       contamination, service_coverage);

    // Terrible conditions: deaths should exceed births
    assert(result.deaths > result.births && "Deaths should exceed births in terrible conditions");
    assert(result.natural_growth < 0 && "Natural growth should be negative");
    assert(result.new_total_beings < 1000 && "Population should decrease");
    assert(pop.total_beings == result.new_total_beings && "PopulationData should be updated");

    std::printf("  PASS: Negative growth (deaths > births)\n");
}

// --------------------------------------------------------------------------
// Test: Population never goes negative
// --------------------------------------------------------------------------
static void test_population_never_negative() {
    PopulationData pop{};
    pop.total_beings = 5;        // Very small population
    pop.harmony_index = 0;
    pop.health_index = 0;
    pop.elder_percent = 100;     // All elderly

    // Terrible conditions to maximize deaths
    uint32_t available_housing = 1;
    float contamination = 100.0f;
    float service_coverage = 0.0f;

    NaturalGrowthResult result = apply_natural_growth(pop, available_housing,
                                                       contamination, service_coverage);

    // Population must never go below zero regardless of deaths
    assert(result.new_total_beings >= 0 && "Population should never be negative");
    assert(pop.total_beings >= 0 && "PopulationData total_beings should never be negative");

    std::printf("  PASS: Population never goes negative\n");
}

// --------------------------------------------------------------------------
// Test: Zero population -> no change
// --------------------------------------------------------------------------
static void test_zero_population_no_change() {
    PopulationData pop{};
    pop.total_beings = 0;

    uint32_t available_housing = 100;
    float contamination = 50.0f;
    float service_coverage = 50.0f;

    NaturalGrowthResult result = apply_natural_growth(pop, available_housing,
                                                       contamination, service_coverage);

    assert(result.births == 0 && "Zero population should have zero births");
    assert(result.deaths == 0 && "Zero population should have zero deaths");
    assert(result.natural_growth == 0 && "Zero population should have zero growth");
    assert(result.new_total_beings == 0 && "Zero population should remain zero");
    assert(pop.total_beings == 0 && "PopulationData should remain at zero");

    std::printf("  PASS: Zero population -> no change\n");
}

// --------------------------------------------------------------------------
// Test: birth_rate_per_1000 and death_rate_per_1000 are updated
// --------------------------------------------------------------------------
static void test_rates_updated() {
    PopulationData pop{};
    pop.total_beings = 1000;
    pop.harmony_index = 50;
    pop.health_index = 50;
    pop.elder_percent = 33;
    pop.birth_rate_per_1000 = 0;
    pop.death_rate_per_1000 = 0;

    uint32_t available_housing = 1000;
    float contamination = 0.0f;
    float service_coverage = 100.0f;

    apply_natural_growth(pop, available_housing, contamination, service_coverage);

    assert(pop.birth_rate_per_1000 > 0 && "birth_rate_per_1000 should be updated");
    assert(pop.death_rate_per_1000 > 0 && "death_rate_per_1000 should be updated");

    std::printf("  PASS: Rates updated in PopulationData\n");
}

// --------------------------------------------------------------------------
// Test: natural_growth = births - deaths identity holds
// --------------------------------------------------------------------------
static void test_growth_identity() {
    PopulationData pop{};
    pop.total_beings = 5000;
    pop.harmony_index = 75;
    pop.health_index = 60;
    pop.elder_percent = 20;

    uint32_t available_housing = 3000;
    float contamination = 30.0f;
    float service_coverage = 70.0f;

    NaturalGrowthResult result = apply_natural_growth(pop, available_housing,
                                                       contamination, service_coverage);

    int32_t expected_growth = static_cast<int32_t>(result.births) - static_cast<int32_t>(result.deaths);
    assert(result.natural_growth == expected_growth && "natural_growth should equal births - deaths");

    std::printf("  PASS: Growth identity (births - deaths)\n");
}

// --------------------------------------------------------------------------
// Main
// --------------------------------------------------------------------------
int main() {
    std::printf("=== Natural Growth Tests (E10-017) ===\n");

    test_normal_positive_growth();
    test_negative_growth();
    test_population_never_negative();
    test_zero_population_no_change();
    test_rates_updated();
    test_growth_identity();

    std::printf("All natural growth tests passed.\n");
    return 0;
}
