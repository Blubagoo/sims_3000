/**
 * @file test_fabrication_demand.cpp
 * @brief Unit tests for fabrication demand formula (E10-045)
 */

#include "sims3000/demand/FabricationDemand.h"
#include <cassert>
#include <cstdio>

using namespace sims3000::demand;

void test_under_served_fabrication_positive_demand() {
    printf("Testing under-served fabrication gives positive demand...\n");

    FabricationInputs inputs;
    inputs.total_beings = 500;
    inputs.fabrication_jobs = 10;       // target = 500/5 = 100, ratio = 0.1
    inputs.labor_force = 300;
    inputs.employed_laborers = 100;     // surplus = 200, ratio = 200/300 = 0.67
    inputs.has_external_connectivity = true;
    inputs.congestion_level = 0.0f;

    auto result = calculate_fabrication_demand(inputs);

    // population_factor = 20 * (1.0 - 0.1) = 18
    assert(result.factors.population_factor > 0);
    assert(result.demand > 0);
    printf("  demand = %d, population_factor = %d\n",
           result.demand, result.factors.population_factor);

    printf("  PASS: under-served fabrication yields positive demand\n");
}

void test_over_served_fabrication() {
    printf("Testing over-served fabrication (too many jobs)...\n");

    FabricationInputs inputs;
    inputs.total_beings = 100;
    inputs.fabrication_jobs = 200;      // target = 100/5 = 20, ratio = 10.0
    inputs.labor_force = 50;
    inputs.employed_laborers = 50;      // surplus = 0
    inputs.has_external_connectivity = false;
    inputs.congestion_level = 50.0f;

    auto result = calculate_fabrication_demand(inputs);

    // population_factor = 20 * (1.0 - 10.0) = -180 -> clamped to -15
    assert(result.factors.population_factor == -15);
    printf("  population_factor = %d (expected -15)\n", result.factors.population_factor);

    printf("  PASS: over-served fabrication yields clamped negative population factor\n");
}

void test_labor_surplus_increases_demand() {
    printf("Testing labor surplus increases demand...\n");

    FabricationInputs inputs;
    inputs.total_beings = 500;
    inputs.fabrication_jobs = 100;      // target = 100, ratio = 1.0 -> pop = 0
    inputs.has_external_connectivity = true;
    inputs.congestion_level = 0.0f;

    // Large surplus
    inputs.labor_force = 1000;
    inputs.employed_laborers = 200;     // surplus = 800, ratio = 0.8
    auto r1 = calculate_fabrication_demand(inputs);

    // Small surplus
    inputs.labor_force = 1000;
    inputs.employed_laborers = 900;     // surplus = 100, ratio = 0.1
    auto r2 = calculate_fabrication_demand(inputs);

    assert(r1.factors.employment_factor > r2.factors.employment_factor);
    printf("  large surplus emp_factor = %d, small surplus emp_factor = %d\n",
           r1.factors.employment_factor, r2.factors.employment_factor);

    printf("  PASS: labor surplus increases employment factor\n");
}

void test_no_external_connectivity_negative_transport() {
    printf("Testing no external connectivity gives negative transport factor...\n");

    FabricationInputs inputs;
    inputs.total_beings = 500;
    inputs.fabrication_jobs = 100;
    inputs.labor_force = 300;
    inputs.employed_laborers = 200;
    inputs.congestion_level = 0.0f;

    inputs.has_external_connectivity = false;
    auto result = calculate_fabrication_demand(inputs);

    // connectivity_base = -10, congestion = 0 -> transport = -10
    assert(result.factors.transport_factor == -10);
    printf("  transport_factor = %d (expected -10)\n", result.factors.transport_factor);

    printf("  PASS: no connectivity yields negative transport factor\n");
}

void test_has_external_connectivity_positive_bonus() {
    printf("Testing external connectivity gives positive transport bonus...\n");

    FabricationInputs inputs;
    inputs.total_beings = 500;
    inputs.fabrication_jobs = 100;
    inputs.labor_force = 300;
    inputs.employed_laborers = 200;
    inputs.congestion_level = 0.0f;

    inputs.has_external_connectivity = true;
    auto result = calculate_fabrication_demand(inputs);

    // connectivity_base = +5, congestion = 0 -> transport = +5
    assert(result.factors.transport_factor == 5);
    printf("  transport_factor = %d (expected 5)\n", result.factors.transport_factor);

    printf("  PASS: connectivity yields positive transport bonus\n");
}

void test_congestion_reduces_transport() {
    printf("Testing congestion reduces transport factor...\n");

    FabricationInputs inputs;
    inputs.total_beings = 500;
    inputs.fabrication_jobs = 100;
    inputs.labor_force = 300;
    inputs.employed_laborers = 200;
    inputs.has_external_connectivity = true;

    inputs.congestion_level = 100.0f;
    auto result = calculate_fabrication_demand(inputs);

    // connectivity_base = +5, congestion = -100/10 = -10 -> transport = -5
    assert(result.factors.transport_factor == -5);
    printf("  transport_factor = %d (expected -5)\n", result.factors.transport_factor);

    printf("  PASS: congestion reduces transport factor\n");
}

void test_contamination_factor_zero() {
    printf("Testing contamination factor is always 0...\n");

    FabricationInputs inputs;
    inputs.total_beings = 500;
    inputs.fabrication_jobs = 100;
    inputs.labor_force = 300;
    inputs.employed_laborers = 200;
    inputs.has_external_connectivity = true;
    inputs.congestion_level = 50.0f;

    auto result = calculate_fabrication_demand(inputs);
    assert(result.factors.contamination_factor == 0);
    printf("  contamination_factor = %d (expected 0)\n", result.factors.contamination_factor);

    printf("  PASS: fabrication is contamination-tolerant\n");
}

void test_demand_clamped() {
    printf("Testing demand is clamped to [-100, +100]...\n");

    // Push high
    {
        FabricationInputs inputs;
        inputs.total_beings = 10000;
        inputs.fabrication_jobs = 0;
        inputs.labor_force = 10000;
        inputs.employed_laborers = 0;
        inputs.has_external_connectivity = true;
        inputs.congestion_level = 0.0f;

        auto result = calculate_fabrication_demand(inputs);
        printf("  high demand = %d\n", result.demand);
        assert(result.demand >= -100 && result.demand <= 100);
    }

    // Push low
    {
        FabricationInputs inputs;
        inputs.total_beings = 10;
        inputs.fabrication_jobs = 10000;
        inputs.labor_force = 10;
        inputs.employed_laborers = 10000;
        inputs.has_external_connectivity = false;
        inputs.congestion_level = 100.0f;

        auto result = calculate_fabrication_demand(inputs);
        printf("  low demand = %d\n", result.demand);
        assert(result.demand >= -100 && result.demand <= 100);
    }

    printf("  PASS: demand clamped to valid range\n");
}

int main() {
    printf("=== Fabrication Demand Unit Tests (E10-045) ===\n\n");

    test_under_served_fabrication_positive_demand();
    test_over_served_fabrication();
    test_labor_surplus_increases_demand();
    test_no_external_connectivity_negative_transport();
    test_has_external_connectivity_positive_bonus();
    test_congestion_reduces_transport();
    test_contamination_factor_zero();
    test_demand_clamped();

    printf("\n=== All tests passed! ===\n");
    return 0;
}
