/**
 * @file test_habitation_demand.cpp
 * @brief Unit tests for habitation demand formula (E10-043)
 */

#include "sims3000/demand/HabitationDemand.h"
#include <cassert>
#include <cstdio>

using namespace sims3000::demand;

void test_default_inputs_near_zero() {
    printf("Testing default inputs produce near-zero demand...\n");

    HabitationInputs inputs;
    // All defaults: 0 beings, 0 capacity, 0 labor, 0 jobs, 0 coverage, 7% tribute, 0 contamination
    auto result = calculate_habitation_demand(inputs);

    // occupancy_ratio = 0/1 = 0 -> < 0.5 -> population_factor = -10
    // jobs=0, labor=0 -> jobs >= labor -> employment_factor = +20
    // service_coverage=0 -> (0-50)/5 = -10 -> services_factor = -10
    // tribute_rate=7 -> (7-7)*3 = 0 -> tribute_factor = 0
    // contamination=0 -> 0/5 = 0 -> contamination_factor = 0
    // sum = -10 + 20 + (-10) + 0 + 0 = 0
    printf("  demand = %d\n", result.demand);
    assert(result.demand >= -10 && result.demand <= 10);

    printf("  PASS: default inputs produce near-zero demand\n");
}

void test_high_occupancy_positive_population() {
    printf("Testing high occupancy (>90%%) gives positive population factor...\n");

    HabitationInputs inputs;
    inputs.total_beings = 950;
    inputs.housing_capacity = 1000;
    inputs.labor_force = 500;
    inputs.total_jobs = 500;
    inputs.service_coverage = 50.0f;
    inputs.tribute_rate = 7.0f;

    auto result = calculate_habitation_demand(inputs);

    // occupancy = 0.95 -> population_factor = +30
    assert(result.factors.population_factor == 30);
    printf("  population_factor = %d (expected 30)\n", result.factors.population_factor);

    printf("  PASS: high occupancy yields positive population factor\n");
}

void test_low_occupancy_negative_population() {
    printf("Testing low occupancy (<50%%) gives negative population factor...\n");

    HabitationInputs inputs;
    inputs.total_beings = 100;
    inputs.housing_capacity = 1000;
    inputs.labor_force = 50;
    inputs.total_jobs = 50;
    inputs.service_coverage = 50.0f;
    inputs.tribute_rate = 7.0f;

    auto result = calculate_habitation_demand(inputs);

    // occupancy = 0.1 -> population_factor = -10
    assert(result.factors.population_factor == -10);
    printf("  population_factor = %d (expected -10)\n", result.factors.population_factor);

    printf("  PASS: low occupancy yields negative population factor\n");
}

void test_jobs_greater_than_labor() {
    printf("Testing jobs > labor gives positive employment factor...\n");

    HabitationInputs inputs;
    inputs.total_beings = 500;
    inputs.housing_capacity = 500;
    inputs.labor_force = 300;
    inputs.total_jobs = 500;
    inputs.service_coverage = 50.0f;
    inputs.tribute_rate = 7.0f;

    auto result = calculate_habitation_demand(inputs);

    // total_jobs > labor_force -> employment_factor = +20
    assert(result.factors.employment_factor == 20);
    printf("  employment_factor = %d (expected 20)\n", result.factors.employment_factor);

    printf("  PASS: jobs > labor yields positive employment factor\n");
}

void test_labor_much_greater_than_jobs() {
    printf("Testing labor >> jobs gives negative employment factor...\n");

    HabitationInputs inputs;
    inputs.total_beings = 500;
    inputs.housing_capacity = 500;
    inputs.labor_force = 1000;
    inputs.total_jobs = 100;
    inputs.service_coverage = 50.0f;
    inputs.tribute_rate = 7.0f;

    auto result = calculate_habitation_demand(inputs);

    // labor_force(1000) > total_jobs*2 (200) -> employment_factor = -15
    assert(result.factors.employment_factor == -15);
    printf("  employment_factor = %d (expected -15)\n", result.factors.employment_factor);

    printf("  PASS: labor >> jobs yields negative employment factor\n");
}

void test_demand_clamped_to_range() {
    printf("Testing demand is clamped to [-100, +100]...\n");

    // Try to push demand very high
    {
        HabitationInputs inputs;
        inputs.total_beings = 1000;
        inputs.housing_capacity = 100;    // Very high occupancy -> +30
        inputs.labor_force = 100;
        inputs.total_jobs = 1000;          // Jobs >> labor -> +20
        inputs.service_coverage = 100.0f;  // (100-50)/5 = +10
        inputs.tribute_rate = 0.0f;        // (7-0)*3 = +21, clamped to +15
        inputs.contamination_level = 0.0f; // 0

        auto result = calculate_habitation_demand(inputs);
        printf("  high demand = %d\n", result.demand);
        assert(result.demand <= 100);
        assert(result.demand >= -100);
    }

    // Try to push demand very low
    {
        HabitationInputs inputs;
        inputs.total_beings = 10;
        inputs.housing_capacity = 10000;   // Very low occupancy -> -10
        inputs.labor_force = 10000;
        inputs.total_jobs = 100;            // labor >> jobs -> -15
        inputs.service_coverage = 0.0f;     // (0-50)/5 = -10
        inputs.tribute_rate = 20.0f;        // (7-20)*3 = -39, clamped to -15
        inputs.contamination_level = 100.0f; // -100/5 = -20

        auto result = calculate_habitation_demand(inputs);
        printf("  low demand = %d\n", result.demand);
        assert(result.demand >= -100);
        assert(result.demand <= 100);
    }

    printf("  PASS: demand values are clamped to [-100, +100]\n");
}

void test_services_factor() {
    printf("Testing services factor...\n");

    HabitationInputs inputs;
    inputs.total_beings = 500;
    inputs.housing_capacity = 700;
    inputs.labor_force = 300;
    inputs.total_jobs = 300;
    inputs.tribute_rate = 7.0f;

    // High coverage
    inputs.service_coverage = 100.0f;
    auto r1 = calculate_habitation_demand(inputs);
    assert(r1.factors.services_factor == 10);
    printf("  coverage=100: services_factor = %d (expected 10)\n", r1.factors.services_factor);

    // Low coverage
    inputs.service_coverage = 0.0f;
    auto r2 = calculate_habitation_demand(inputs);
    assert(r2.factors.services_factor == -10);
    printf("  coverage=0: services_factor = %d (expected -10)\n", r2.factors.services_factor);

    printf("  PASS: services factor responds to coverage\n");
}

void test_contamination_factor() {
    printf("Testing contamination factor...\n");

    HabitationInputs inputs;
    inputs.total_beings = 500;
    inputs.housing_capacity = 700;
    inputs.labor_force = 300;
    inputs.total_jobs = 300;
    inputs.service_coverage = 50.0f;
    inputs.tribute_rate = 7.0f;

    // High contamination
    inputs.contamination_level = 100.0f;
    auto result = calculate_habitation_demand(inputs);
    assert(result.factors.contamination_factor == -20);
    printf("  contamination=100: contamination_factor = %d (expected -20)\n",
           result.factors.contamination_factor);

    // No contamination
    inputs.contamination_level = 0.0f;
    auto r2 = calculate_habitation_demand(inputs);
    assert(r2.factors.contamination_factor == 0);
    printf("  contamination=0: contamination_factor = %d (expected 0)\n",
           r2.factors.contamination_factor);

    printf("  PASS: contamination factor responds to contamination level\n");
}

void test_tribute_factor() {
    printf("Testing tribute factor...\n");

    HabitationInputs inputs;
    inputs.total_beings = 500;
    inputs.housing_capacity = 700;
    inputs.labor_force = 300;
    inputs.total_jobs = 300;
    inputs.service_coverage = 50.0f;

    // Low tribute (positive demand)
    inputs.tribute_rate = 0.0f;
    auto r1 = calculate_habitation_demand(inputs);
    assert(r1.factors.tribute_factor > 0);
    printf("  tribute=0%%: tribute_factor = %d (expected positive)\n",
           r1.factors.tribute_factor);

    // High tribute (negative demand)
    inputs.tribute_rate = 15.0f;
    auto r2 = calculate_habitation_demand(inputs);
    assert(r2.factors.tribute_factor < 0);
    printf("  tribute=15%%: tribute_factor = %d (expected negative)\n",
           r2.factors.tribute_factor);

    printf("  PASS: tribute factor responds to tribute rate\n");
}

int main() {
    printf("=== Habitation Demand Unit Tests (E10-043) ===\n\n");

    test_default_inputs_near_zero();
    test_high_occupancy_positive_population();
    test_low_occupancy_negative_population();
    test_jobs_greater_than_labor();
    test_labor_much_greater_than_jobs();
    test_demand_clamped_to_range();
    test_services_factor();
    test_contamination_factor();
    test_tribute_factor();

    printf("\n=== All tests passed! ===\n");
    return 0;
}
