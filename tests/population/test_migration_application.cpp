/**
 * @file test_migration_application.cpp
 * @brief Tests for migration application (Ticket E10-027)
 *
 * Validates:
 * - apply_migration() combines in/out correctly
 * - total_beings updated correctly
 * - net_migration and growth_rate updated
 * - MigrationEvent contains correct values
 * - Housing cap is respected
 * - Population never goes below 0
 */

#include <cassert>
#include <cstdio>

#include "sims3000/population/MigrationApplication.h"

using namespace sims3000::population;

// --------------------------------------------------------------------------
// Test: Basic migration application with positive net migration
// --------------------------------------------------------------------------
static void test_positive_net_migration() {
    printf("Testing positive net migration...\n");

    // Setup: city with moderate population, attractive conditions
    PopulationData data;
    data.total_beings = 1000;
    data.max_capacity = 2000;
    data.natural_growth = 10;  // From births/deaths

    MigrationFactors factors;
    factors.net_attraction = 50;  // Positive attraction
    factors.disorder_level = 0;
    factors.contamination_level = 0;
    factors.job_availability = 80;
    factors.harmony_level = 70;

    uint32_t available_housing = data.max_capacity - data.total_beings;  // 1000

    // Apply migration
    MigrationEvent event = apply_migration(data, factors, available_housing);

    // Should have migrants coming in (positive attraction)
    assert(event.migrants_in > 0 && "Positive attraction should bring migrants in");

    // Net migration should be positive (more in than out)
    assert(event.net_migration > 0 && "Net migration should be positive");

    // Population should increase
    assert(data.total_beings > 1000 && "Population should increase");

    // net_migration field should match event
    assert(data.net_migration == event.net_migration && "net_migration field should match event");

    // growth_rate should include natural + migration
    assert(data.growth_rate == static_cast<float>(data.natural_growth + data.net_migration) &&
           "growth_rate should be natural_growth + net_migration");

    printf("  PASS: Positive net migration\n");
}

// --------------------------------------------------------------------------
// Test: Negative net migration (high desperation)
// --------------------------------------------------------------------------
static void test_negative_net_migration() {
    printf("Testing negative net migration...\n");

    PopulationData data;
    data.total_beings = 5000;
    data.max_capacity = 10000;
    data.natural_growth = 20;

    // Very bad conditions -> people leave
    MigrationFactors factors;
    factors.net_attraction = -60;  // Very unattractive
    factors.disorder_level = 80;   // High disorder
    factors.contamination_level = 70;  // High contamination
    factors.job_availability = 20;  // Low jobs
    factors.harmony_level = 25;     // Low harmony

    uint32_t available_housing = data.max_capacity - data.total_beings;

    MigrationEvent event = apply_migration(data, factors, available_housing);

    // Very negative attraction -> no one comes in
    assert(event.migrants_in == 0 && "Very negative attraction should block migration in");

    // High desperation -> people leave
    assert(event.migrants_out > 0 && "High desperation should cause migration out");

    // Net migration should be negative
    assert(event.net_migration < 0 && "Net migration should be negative");

    // Population should decrease
    assert(data.total_beings < 5000 && "Population should decrease");

    printf("  PASS: Negative net migration\n");
}

// --------------------------------------------------------------------------
// Test: Migration capped by housing
// --------------------------------------------------------------------------
static void test_housing_cap() {
    printf("Testing housing cap...\n");

    PopulationData data;
    data.total_beings = 1000;
    data.max_capacity = 1010;  // Only 10 units available
    data.natural_growth = 0;

    // Very attractive city
    MigrationFactors factors;
    factors.net_attraction = 100;
    factors.disorder_level = 0;
    factors.contamination_level = 0;
    factors.job_availability = 100;
    factors.harmony_level = 100;

    uint32_t available_housing = data.max_capacity - data.total_beings;  // Only 10

    MigrationEvent event = apply_migration(data, factors, available_housing);

    // Should be capped by available housing
    assert(event.migrants_in <= 10 && "Migration should be capped by available housing");

    // Population should not exceed capacity
    assert(data.total_beings <= data.max_capacity && "Population should not exceed capacity");

    printf("  PASS: Housing cap respected\n");
}

// --------------------------------------------------------------------------
// Test: Population never goes below 0
// --------------------------------------------------------------------------
static void test_population_floor() {
    printf("Testing population floor...\n");

    PopulationData data;
    data.total_beings = 50;  // Small population
    data.max_capacity = 1000;
    data.natural_growth = 0;

    // Extreme desperation
    MigrationFactors factors;
    factors.net_attraction = -100;
    factors.disorder_level = 100;
    factors.contamination_level = 100;
    factors.job_availability = 0;
    factors.harmony_level = 0;

    uint32_t available_housing = data.max_capacity - data.total_beings;

    MigrationEvent event = apply_migration(data, factors, available_housing);

    // Population should never go negative
    assert(data.total_beings >= 0 && "Population should never go negative");

    // migrants_out should be clamped to population
    assert(event.migrants_out <= 50 && "migrants_out should not exceed initial population");

    printf("  PASS: Population floor (>= 0)\n");
}

// --------------------------------------------------------------------------
// Test: Zero housing available
// --------------------------------------------------------------------------
static void test_zero_housing() {
    printf("Testing zero housing available...\n");

    PopulationData data;
    data.total_beings = 1000;
    data.max_capacity = 1000;  // At capacity
    data.natural_growth = 5;

    MigrationFactors factors;
    factors.net_attraction = 80;  // Very attractive
    factors.disorder_level = 0;
    factors.contamination_level = 0;
    factors.job_availability = 90;
    factors.harmony_level = 90;

    uint32_t available_housing = 0;  // No housing available

    MigrationEvent event = apply_migration(data, factors, available_housing);

    // No housing -> no one can move in
    assert(event.migrants_in == 0 && "No housing should block migration in");

    // Population at capacity should remain the same or decrease slightly
    assert(data.total_beings <= 1000 && "Population should not exceed capacity");

    printf("  PASS: Zero housing available\n");
}

// --------------------------------------------------------------------------
// Test: Event values match population updates
// --------------------------------------------------------------------------
static void test_event_consistency() {
    printf("Testing event consistency...\n");

    PopulationData data;
    data.total_beings = 2000;
    data.max_capacity = 5000;
    data.natural_growth = 15;

    MigrationFactors factors;
    factors.net_attraction = 30;
    factors.disorder_level = 20;
    factors.contamination_level = 10;
    factors.job_availability = 60;
    factors.harmony_level = 65;

    uint32_t initial_population = data.total_beings;
    uint32_t available_housing = data.max_capacity - data.total_beings;

    MigrationEvent event = apply_migration(data, factors, available_housing);

    // Check population change matches event
    int32_t expected_population_change = static_cast<int32_t>(event.migrants_in) -
                                         static_cast<int32_t>(event.migrants_out);
    int32_t actual_population_change = static_cast<int32_t>(data.total_beings) -
                                       static_cast<int32_t>(initial_population);

    assert(actual_population_change == expected_population_change &&
           "Population change should match event in - out");

    // Check net_migration matches
    assert(event.net_migration == expected_population_change &&
           "Event net_migration should equal in - out");

    // Check data.net_migration matches event
    assert(data.net_migration == event.net_migration &&
           "data.net_migration should match event");

    printf("  PASS: Event consistency\n");
}

// --------------------------------------------------------------------------
// Test: Growth rate calculation
// --------------------------------------------------------------------------
static void test_growth_rate() {
    printf("Testing growth rate calculation...\n");

    PopulationData data;
    data.total_beings = 3000;
    data.max_capacity = 8000;
    data.natural_growth = 25;  // From births/deaths

    MigrationFactors factors;
    factors.net_attraction = 40;
    factors.disorder_level = 15;
    factors.contamination_level = 10;
    factors.job_availability = 70;
    factors.harmony_level = 75;

    uint32_t available_housing = data.max_capacity - data.total_beings;

    MigrationEvent event = apply_migration(data, factors, available_housing);

    // growth_rate should be natural_growth + net_migration
    float expected_growth_rate = static_cast<float>(data.natural_growth + data.net_migration);

    assert(data.growth_rate == expected_growth_rate &&
           "growth_rate should equal natural_growth + net_migration");

    printf("  PASS: Growth rate calculation\n");
}

// --------------------------------------------------------------------------
// Main
// --------------------------------------------------------------------------
int main() {
    printf("=== Migration Application Tests (E10-027) ===\n\n");

    test_positive_net_migration();
    test_negative_net_migration();
    test_housing_cap();
    test_population_floor();
    test_zero_housing();
    test_event_consistency();
    test_growth_rate();

    printf("\n=== All Tests Passed ===\n");
    return 0;
}
