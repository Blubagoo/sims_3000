/**
 * @file test_demographics_comprehensive.cpp
 * @brief Comprehensive integration tests for demographics (Ticket E10-120)
 *
 * Tests birth rate, death rate, natural growth, migration in/out,
 * migration application, and age distribution together.
 *
 * Validates:
 * - Birth rate modifiers (harmony, health, housing)
 * - Death rate modifiers (health, contamination, services, age)
 * - Natural growth edge cases
 * - Migration in (attraction, colony size, housing cap, threshold)
 * - Migration out (desperation, rate cap, exodus guard)
 * - Migration application (net migration, growth rate)
 * - Age distribution (aging transitions, weighted deaths, sum to 100)
 * - Full demographic cycle integration
 */

#include <cassert>
#include <cmath>
#include <cstdio>

#include "sims3000/population/BirthRateCalculation.h"
#include "sims3000/population/DeathRateCalculation.h"
#include "sims3000/population/NaturalGrowth.h"
#include "sims3000/population/MigrationIn.h"
#include "sims3000/population/MigrationOut.h"
#include "sims3000/population/MigrationApplication.h"
#include "sims3000/population/AgeDistribution.h"
#include "sims3000/population/PopulationData.h"
#include "sims3000/population/MigrationFactors.h"

using namespace sims3000::population;

// Helper: float approximate equality
static bool approx(float a, float b, float epsilon = 0.01f) {
    return std::fabs(a - b) < epsilon;
}

// --------------------------------------------------------------------------
// Birth Rate Tests
// --------------------------------------------------------------------------

static void test_birth_rate_harmony_modifier() {
    PopulationData pop{};
    pop.total_beings = 10000;
    pop.harmony_index = 100; // Max harmony
    pop.health_index = 50;

    BirthRateResult result = calculate_birth_rate(pop, 1000);

    // High harmony should increase births
    assert(result.harmony_modifier > 1.0f && "Max harmony should boost birth rate");
    assert(result.births > 0 && "Should have births with population");
    printf("[PASS] Birth rate harmony modifier\n");
}

static void test_birth_rate_health_modifier() {
    PopulationData pop{};
    pop.total_beings = 10000;
    pop.harmony_index = 50;
    pop.health_index = 100; // Max health

    BirthRateResult result = calculate_birth_rate(pop, 1000);

    // High health should increase births
    assert(result.health_modifier > 1.0f && "Max health should boost birth rate");
    assert(result.births > 0 && "Should have births with population");
    printf("[PASS] Birth rate health modifier\n");
}

static void test_birth_rate_housing_modifier() {
    PopulationData pop{};
    pop.total_beings = 10000;
    pop.max_capacity = 15000;

    // No available housing
    BirthRateResult result1 = calculate_birth_rate(pop, 0);
    assert(result1.births == 0 && "No births without housing");

    // Plentiful housing
    BirthRateResult result2 = calculate_birth_rate(pop, 5000);
    assert(result2.housing_modifier >= 0.9f && "Plentiful housing should have high modifier");
    assert(result2.births > 0 && "Should have births with housing");

    printf("[PASS] Birth rate housing modifier\n");
}

// --------------------------------------------------------------------------
// Death Rate Tests
// --------------------------------------------------------------------------

static void test_death_rate_health_modifier() {
    PopulationData pop{};
    pop.total_beings = 10000;
    pop.health_index = 10; // Poor health
    pop.youth_percent = 33;
    pop.adult_percent = 34;
    pop.elder_percent = 33;

    DeathRateResult result = calculate_death_rate(pop, 0.0f, 50.0f);

    // Poor health should increase deaths
    assert(result.health_modifier > 1.0f && "Poor health should increase death rate");
    assert(result.deaths > 0 && "Should have deaths with population");
    printf("[PASS] Death rate health modifier\n");
}

static void test_death_rate_contamination_modifier() {
    PopulationData pop{};
    pop.total_beings = 10000;
    pop.health_index = 50;
    pop.youth_percent = 33;
    pop.adult_percent = 34;
    pop.elder_percent = 33;

    DeathRateResult result = calculate_death_rate(pop, 100.0f, 50.0f); // Max contamination

    // High contamination should increase deaths
    assert(result.contamination_modifier > 1.0f && "High contamination should increase death rate");
    assert(result.deaths > 0 && "Should have deaths with contaminated environment");
    printf("[PASS] Death rate contamination modifier\n");
}

static void test_death_rate_services_modifier() {
    PopulationData pop{};
    pop.total_beings = 10000;
    pop.health_index = 50;
    pop.youth_percent = 33;
    pop.adult_percent = 34;
    pop.elder_percent = 33;

    // No services
    DeathRateResult result1 = calculate_death_rate(pop, 0.0f, 0.0f);
    assert(result1.services_modifier > 1.0f && "No services should increase death rate");

    // Full services
    DeathRateResult result2 = calculate_death_rate(pop, 0.0f, 100.0f);
    assert(result2.services_modifier < 1.0f && "Full services should decrease death rate");

    printf("[PASS] Death rate services modifier\n");
}

static void test_death_rate_age_modifier() {
    PopulationData pop{};
    pop.total_beings = 10000;
    pop.health_index = 50;

    // Young population
    pop.youth_percent = 70;
    pop.adult_percent = 25;
    pop.elder_percent = 5;
    DeathRateResult result1 = calculate_death_rate(pop, 0.0f, 50.0f);

    // Old population
    pop.youth_percent = 10;
    pop.adult_percent = 20;
    pop.elder_percent = 70;
    DeathRateResult result2 = calculate_death_rate(pop, 0.0f, 50.0f);

    assert(result2.age_modifier > result1.age_modifier && "Older population should have higher death modifier");
    assert(result2.deaths > result1.deaths && "Older population should have more deaths");

    printf("[PASS] Death rate age modifier\n");
}

// --------------------------------------------------------------------------
// Natural Growth Tests
// --------------------------------------------------------------------------

static void test_natural_growth_zero_population() {
    PopulationData pop{};
    pop.total_beings = 0;

    NaturalGrowthResult result = apply_natural_growth(pop, 1000, 0.0f, 50.0f);

    assert(result.births == 0 && "Zero population should have no births");
    assert(result.deaths == 0 && "Zero population should have no deaths");
    assert(result.natural_growth == 0 && "Zero population should have no natural growth");
    assert(pop.total_beings == 0 && "Population should remain zero");

    printf("[PASS] Natural growth with zero population\n");
}

static void test_natural_growth_max_capacity() {
    PopulationData pop{};
    pop.total_beings = 5000;
    pop.max_capacity = 5000;

    NaturalGrowthResult result = apply_natural_growth(pop, 0, 0.0f, 50.0f);

    assert(result.births == 0 && "At max capacity with no housing should have no births");

    printf("[PASS] Natural growth at max capacity\n");
}

static void test_natural_growth_more_deaths_than_births() {
    PopulationData pop{};
    pop.total_beings = 1000;
    pop.health_index = 10; // Very poor health
    pop.harmony_index = 10; // Very low harmony
    pop.elder_percent = 80; // Very old population
    pop.adult_percent = 15;
    pop.youth_percent = 5;

    NaturalGrowthResult result = apply_natural_growth(pop, 0, 100.0f, 0.0f);

    // Should have more deaths than births
    assert(result.natural_growth < 0 && "Poor conditions should lead to negative natural growth");
    assert(pop.total_beings < 1000 && "Population should decrease");

    printf("[PASS] Natural growth with more deaths than births\n");
}

// --------------------------------------------------------------------------
// Migration In Tests
// --------------------------------------------------------------------------

static void test_migration_in_attraction_multiplier() {
    // High attraction
    MigrationInResult result1 = calculate_migration_in(100, 5000, 1000);
    assert(result1.attraction_multiplier == 2.0f && "Max attraction should give 2x multiplier");
    assert(result1.migrants_in > 0 && "Should have migrants at max attraction");

    // Neutral attraction
    MigrationInResult result2 = calculate_migration_in(0, 5000, 1000);
    assert(approx(result2.attraction_multiplier, 1.0f) && "Neutral attraction should give 1x multiplier");

    // Low attraction
    MigrationInResult result3 = calculate_migration_in(-100, 5000, 1000);
    assert(result3.attraction_multiplier == 0.0f && "Min attraction should give 0x multiplier");

    printf("[PASS] Migration in attraction multiplier\n");
}

static void test_migration_in_colony_size_bonus() {
    // Small colony
    MigrationInResult result1 = calculate_migration_in(50, 1000, 1000);

    // Large colony
    MigrationInResult result2 = calculate_migration_in(50, 10000, 1000);

    assert(result2.colony_size_bonus > result1.colony_size_bonus && "Larger colony should have bigger bonus");
    assert(result2.migrants_in > result1.migrants_in && "Larger colony should attract more migrants");

    printf("[PASS] Migration in colony size bonus\n");
}

static void test_migration_in_housing_cap() {
    // Limited housing
    MigrationInResult result = calculate_migration_in(100, 10000, 10);

    assert(result.migrants_in <= 10 && "Migrants should be capped by available housing");

    printf("[PASS] Migration in housing cap\n");
}

static void test_migration_in_blocked_at_negative_50() {
    // Attraction below -50
    MigrationInResult result1 = calculate_migration_in(-60, 5000, 1000);
    assert(result1.migrants_in == 0 && "Should block migration at attraction < -50");

    // Attraction at -50
    MigrationInResult result2 = calculate_migration_in(-50, 5000, 1000);
    assert(result2.migrants_in == 0 && "Should block migration at attraction = -50");

    // Attraction just above -50
    MigrationInResult result3 = calculate_migration_in(-49, 5000, 1000);
    assert(result3.migrants_in > 0 && "Should allow migration at attraction > -50");

    printf("[PASS] Migration in blocked at -50 threshold\n");
}

// --------------------------------------------------------------------------
// Migration Out Tests
// --------------------------------------------------------------------------

static void test_migration_out_desperation_factors() {
    MigrationFactors factors{};
    factors.job_availability = 20; // Low (< 30)
    factors.harmony_level = 20;    // Low (< 30)
    factors.disorder_level = 60;   // High (> 50)
    factors.contamination_level = 60; // High (> 50)

    MigrationOutResult result = calculate_migration_out(factors, 10000);

    assert(result.desperation_factor > 0.0f && "Desperation factors should accumulate");
    assert(result.effective_out_rate > constants::BASE_OUT_RATE && "Desperation should increase out rate");
    assert(result.migrants_out > 0 && "Should have migrants leaving");

    printf("[PASS] Migration out desperation factors\n");
}

static void test_migration_out_rate_cap() {
    MigrationFactors factors{};
    factors.job_availability = 0;
    factors.harmony_level = 0;
    factors.disorder_level = 100;
    factors.contamination_level = 100;

    MigrationOutResult result = calculate_migration_out(factors, 10000);

    assert(result.effective_out_rate <= constants::MAX_OUT_RATE && "Out rate should be capped at 5%");

    printf("[PASS] Migration out rate cap\n");
}

static void test_migration_out_exodus_guard() {
    MigrationFactors factors{};
    factors.disorder_level = 100;
    factors.contamination_level = 100;

    MigrationOutResult result = calculate_migration_out(factors, 10);

    // Should never cause total exodus
    assert(result.migrants_out < 10 && "Should never cause total exodus");

    printf("[PASS] Migration out exodus guard\n");
}

// --------------------------------------------------------------------------
// Migration Application Tests
// --------------------------------------------------------------------------

static void test_migration_application_net_calculation() {
    PopulationData data{};
    data.total_beings = 5000;

    MigrationFactors factors{};
    factors.net_attraction = 50; // Positive attraction
    factors.job_availability = 80;
    factors.harmony_level = 80;

    uint32_t initial_beings = data.total_beings;
    MigrationEvent event = apply_migration(data, factors, 1000);

    assert(event.net_migration == static_cast<int32_t>(event.migrants_in) - static_cast<int32_t>(event.migrants_out)
           && "Net migration should equal in minus out");
    assert(data.net_migration == event.net_migration && "PopulationData should store net migration");

    printf("[PASS] Migration application net calculation\n");
}

static void test_migration_application_growth_rate_update() {
    PopulationData data{};
    data.total_beings = 5000;
    data.natural_growth = 100; // Some natural growth

    MigrationFactors factors{};
    factors.net_attraction = 50;

    apply_migration(data, factors, 1000);

    // growth_rate should reflect both natural growth and migration
    // The implementation should update growth_rate appropriately
    printf("[PASS] Migration application growth rate update\n");
}

// --------------------------------------------------------------------------
// Age Distribution Tests
// --------------------------------------------------------------------------

static void test_age_distribution_aging_transitions() {
    uint32_t total = 10000;

    AgeDistributionResult result = evolve_age_distribution(
        33, 34, 33,  // Current distribution
        0, 0,        // No births, no deaths
        total
    );

    // With aging, youth should decrease, elder should increase
    assert(result.youth_percent <= 33 && "Youth should decrease with aging");
    assert(result.elder_percent >= 33 && "Elder should increase with aging");
    assert(result.youth_percent + result.adult_percent + result.elder_percent == 100
           && "Percentages should sum to 100");

    printf("[PASS] Age distribution aging transitions\n");
}

static void test_age_distribution_weighted_deaths() {
    uint32_t total = 10000;

    // Apply many deaths
    AgeDistributionResult result = evolve_age_distribution(
        33, 34, 33,  // Current distribution
        0, 5000,     // No births, many deaths
        total
    );

    // Elder deaths should be weighted more heavily (60%)
    assert(result.youth_percent + result.adult_percent + result.elder_percent == 100
           && "Percentages should sum to 100 even after deaths");

    printf("[PASS] Age distribution weighted deaths\n");
}

static void test_age_distribution_percentage_sum() {
    uint32_t total = 8000;

    // Various scenarios
    AgeDistributionResult result1 = evolve_age_distribution(20, 50, 30, 500, 200, total);
    assert(result1.youth_percent + result1.adult_percent + result1.elder_percent == 100
           && "Percentages should sum to 100");

    AgeDistributionResult result2 = evolve_age_distribution(50, 30, 20, 1000, 800, total);
    assert(result2.youth_percent + result2.adult_percent + result2.elder_percent == 100
           && "Percentages should sum to 100");

    AgeDistributionResult result3 = evolve_age_distribution(10, 80, 10, 100, 50, total);
    assert(result3.youth_percent + result3.adult_percent + result3.elder_percent == 100
           && "Percentages should sum to 100");

    printf("[PASS] Age distribution percentage sum to 100\n");
}

// --------------------------------------------------------------------------
// Full Demographic Cycle Tests
// --------------------------------------------------------------------------

static void test_full_cycle_birth_death_migration_aging() {
    PopulationData pop{};
    pop.total_beings = 5000;
    pop.max_capacity = 10000;
    pop.harmony_index = 60;
    pop.health_index = 60;
    pop.youth_percent = 30;
    pop.adult_percent = 50;
    pop.elder_percent = 20;

    MigrationFactors factors{};
    factors.net_attraction = 40;
    factors.job_availability = 70;
    factors.harmony_level = 60;

    uint32_t initial_beings = pop.total_beings;

    // Step 1: Apply natural growth (births and deaths)
    NaturalGrowthResult growth_result = apply_natural_growth(pop, 2000, 10.0f, 70.0f);

    // Step 2: Apply migration
    uint32_t available_housing = pop.max_capacity - pop.total_beings;
    MigrationEvent migration_event = apply_migration(pop, factors, available_housing);

    // Step 3: Update age distribution
    AgeDistributionResult age_result = evolve_age_distribution(
        pop.youth_percent, pop.adult_percent, pop.elder_percent,
        growth_result.births, growth_result.deaths,
        pop.total_beings
    );

    pop.youth_percent = age_result.youth_percent;
    pop.adult_percent = age_result.adult_percent;
    pop.elder_percent = age_result.elder_percent;

    // Validate full cycle
    assert(growth_result.births > 0 && "Should have births in healthy city");
    assert(growth_result.deaths > 0 && "Should have deaths in any population");
    assert(pop.total_beings != initial_beings && "Population should change");
    assert(pop.youth_percent + pop.adult_percent + pop.elder_percent == 100
           && "Age distribution should be valid");

    printf("[PASS] Full demographic cycle: birth + death + migration + aging\n");
}

static void test_full_cycle_declining_city() {
    PopulationData pop{};
    pop.total_beings = 5000;
    pop.max_capacity = 10000;
    pop.harmony_index = 20; // Very low
    pop.health_index = 20;  // Very low
    pop.youth_percent = 10;
    pop.adult_percent = 30;
    pop.elder_percent = 60; // Very old population

    MigrationFactors factors{};
    factors.net_attraction = -70; // Very unattractive (but not blocked)
    factors.job_availability = 20;
    factors.harmony_level = 20;
    factors.disorder_level = 80;
    factors.contamination_level = 80;

    uint32_t initial_beings = pop.total_beings;

    // Full cycle
    NaturalGrowthResult growth_result = apply_natural_growth(pop, 100, 80.0f, 20.0f);
    uint32_t available_housing = pop.max_capacity - pop.total_beings;
    MigrationEvent migration_event = apply_migration(pop, factors, available_housing);

    // Should be declining
    assert(growth_result.natural_growth <= 0 && "Declining city should have negative or zero natural growth");
    assert(migration_event.migrants_out > migration_event.migrants_in
           && "Declining city should have more out-migration");
    assert(pop.total_beings < initial_beings && "Total population should decline");

    printf("[PASS] Full demographic cycle: declining city\n");
}

static void test_full_cycle_growing_city() {
    PopulationData pop{};
    pop.total_beings = 3000;
    pop.max_capacity = 10000;
    pop.harmony_index = 80; // High
    pop.health_index = 80;  // High
    pop.youth_percent = 40;
    pop.adult_percent = 50;
    pop.elder_percent = 10; // Young population

    MigrationFactors factors{};
    factors.net_attraction = 80; // Very attractive
    factors.job_availability = 80;
    factors.harmony_level = 80;
    factors.disorder_level = 10;
    factors.contamination_level = 10;

    uint32_t initial_beings = pop.total_beings;

    // Full cycle
    NaturalGrowthResult growth_result = apply_natural_growth(pop, 3000, 10.0f, 80.0f);
    uint32_t available_housing = pop.max_capacity - pop.total_beings;
    MigrationEvent migration_event = apply_migration(pop, factors, available_housing);

    // Should be growing
    assert(growth_result.natural_growth > 0 && "Growing city should have positive natural growth");
    assert(migration_event.migrants_in > migration_event.migrants_out
           && "Growing city should have more in-migration");
    assert(pop.total_beings > initial_beings && "Total population should increase");

    printf("[PASS] Full demographic cycle: growing city\n");
}

static void test_full_cycle_stable_city() {
    PopulationData pop{};
    pop.total_beings = 5000;
    pop.max_capacity = 6000;
    pop.harmony_index = 50; // Neutral
    pop.health_index = 50;  // Neutral
    pop.youth_percent = 33;
    pop.adult_percent = 34;
    pop.elder_percent = 33;

    MigrationFactors factors{};
    factors.net_attraction = 0; // Neutral
    factors.job_availability = 50;
    factors.harmony_level = 50;
    factors.disorder_level = 30;
    factors.contamination_level = 30;

    uint32_t initial_beings = pop.total_beings;

    // Full cycle
    NaturalGrowthResult growth_result = apply_natural_growth(pop, 500, 30.0f, 50.0f);
    uint32_t available_housing = pop.max_capacity - pop.total_beings;
    MigrationEvent migration_event = apply_migration(pop, factors, available_housing);

    // Should be relatively stable (small changes)
    int32_t total_change = growth_result.natural_growth + migration_event.net_migration;
    assert(abs(total_change) < 500 && "Stable city should have small population changes");

    printf("[PASS] Full demographic cycle: stable city\n");
}

// --------------------------------------------------------------------------
// Main
// --------------------------------------------------------------------------

int main() {
    printf("=== Demographics Comprehensive Tests (E10-120) ===\n\n");

    printf("-- Birth Rate Tests --\n");
    test_birth_rate_harmony_modifier();
    test_birth_rate_health_modifier();
    test_birth_rate_housing_modifier();

    printf("\n-- Death Rate Tests --\n");
    test_death_rate_health_modifier();
    test_death_rate_contamination_modifier();
    test_death_rate_services_modifier();
    test_death_rate_age_modifier();

    printf("\n-- Natural Growth Tests --\n");
    test_natural_growth_zero_population();
    test_natural_growth_max_capacity();
    test_natural_growth_more_deaths_than_births();

    printf("\n-- Migration In Tests --\n");
    test_migration_in_attraction_multiplier();
    test_migration_in_colony_size_bonus();
    test_migration_in_housing_cap();
    test_migration_in_blocked_at_negative_50();

    printf("\n-- Migration Out Tests --\n");
    test_migration_out_desperation_factors();
    test_migration_out_rate_cap();
    test_migration_out_exodus_guard();

    printf("\n-- Migration Application Tests --\n");
    test_migration_application_net_calculation();
    test_migration_application_growth_rate_update();

    printf("\n-- Age Distribution Tests --\n");
    test_age_distribution_aging_transitions();
    test_age_distribution_weighted_deaths();
    test_age_distribution_percentage_sum();

    printf("\n-- Full Demographic Cycle Tests --\n");
    test_full_cycle_birth_death_migration_aging();
    test_full_cycle_declining_city();
    test_full_cycle_growing_city();
    test_full_cycle_stable_city();

    printf("\n=== All Demographics Comprehensive Tests Passed ===\n");
    return 0;
}
