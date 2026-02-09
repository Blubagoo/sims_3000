/**
 * @file test_demand_cap.cpp
 * @brief Unit tests for demand cap calculation (E10-046)
 */

#include "sims3000/demand/DemandCap.h"
#include <cassert>
#include <cstdio>

using namespace sims3000::demand;

void test_full_infrastructure_equals_raw_capacity() {
    printf("Testing full infrastructure: caps equal raw capacity...\n");

    DemandCapInputs inputs;
    inputs.housing_capacity = 1000;
    inputs.exchange_jobs = 500;
    inputs.fabrication_jobs = 300;
    inputs.energy_factor = 1.0f;
    inputs.fluid_factor = 1.0f;
    inputs.transport_factor = 1.0f;

    auto result = calculate_demand_caps(inputs);

    assert(result.habitation_cap == 1000);
    assert(result.exchange_cap == 500);
    assert(result.fabrication_cap == 300);

    printf("  habitation_cap = %u (expected 1000)\n", result.habitation_cap);
    printf("  exchange_cap = %u (expected 500)\n", result.exchange_cap);
    printf("  fabrication_cap = %u (expected 300)\n", result.fabrication_cap);

    printf("  PASS: full infrastructure yields raw capacity\n");
}

void test_partial_energy_reduces_habitation() {
    printf("Testing partial energy reduces habitation cap...\n");

    DemandCapInputs inputs;
    inputs.housing_capacity = 1000;
    inputs.exchange_jobs = 500;
    inputs.fabrication_jobs = 300;
    inputs.energy_factor = 0.5f;     // 50% powered
    inputs.fluid_factor = 1.0f;
    inputs.transport_factor = 1.0f;

    auto result = calculate_demand_caps(inputs);

    // habitation_cap = 1000 * 0.5 * 1.0 = 500
    assert(result.habitation_cap == 500);
    // exchange/fabrication unaffected by energy
    assert(result.exchange_cap == 500);
    assert(result.fabrication_cap == 300);

    printf("  habitation_cap = %u (expected 500)\n", result.habitation_cap);

    printf("  PASS: partial energy reduces habitation cap\n");
}

void test_partial_fluid_reduces_habitation() {
    printf("Testing partial fluid reduces habitation cap...\n");

    DemandCapInputs inputs;
    inputs.housing_capacity = 1000;
    inputs.exchange_jobs = 500;
    inputs.fabrication_jobs = 300;
    inputs.energy_factor = 1.0f;
    inputs.fluid_factor = 0.75f;     // 75% watered
    inputs.transport_factor = 1.0f;

    auto result = calculate_demand_caps(inputs);

    // habitation_cap = 1000 * 1.0 * 0.75 = 750
    assert(result.habitation_cap == 750);

    printf("  habitation_cap = %u (expected 750)\n", result.habitation_cap);

    printf("  PASS: partial fluid reduces habitation cap\n");
}

void test_both_energy_and_fluid_partial() {
    printf("Testing both energy and fluid partial...\n");

    DemandCapInputs inputs;
    inputs.housing_capacity = 1000;
    inputs.exchange_jobs = 500;
    inputs.fabrication_jobs = 300;
    inputs.energy_factor = 0.5f;
    inputs.fluid_factor = 0.5f;
    inputs.transport_factor = 1.0f;

    auto result = calculate_demand_caps(inputs);

    // habitation_cap = 1000 * 0.5 * 0.5 = 250
    assert(result.habitation_cap == 250);

    printf("  habitation_cap = %u (expected 250)\n", result.habitation_cap);

    printf("  PASS: both partial factors compound\n");
}

void test_high_congestion_reduces_caps() {
    printf("Testing high congestion (low transport_factor) reduces caps...\n");

    DemandCapInputs inputs;
    inputs.housing_capacity = 1000;
    inputs.exchange_jobs = 500;
    inputs.fabrication_jobs = 300;
    inputs.energy_factor = 1.0f;
    inputs.fluid_factor = 1.0f;
    inputs.transport_factor = 0.3f;   // 70% congestion

    auto result = calculate_demand_caps(inputs);

    // exchange_cap = 500 * 0.3 = 150
    assert(result.exchange_cap == 150);
    // fabrication_cap = 300 * 0.3 = 90
    assert(result.fabrication_cap == 90);
    // habitation unaffected by transport
    assert(result.habitation_cap == 1000);

    printf("  exchange_cap = %u (expected 150)\n", result.exchange_cap);
    printf("  fabrication_cap = %u (expected 90)\n", result.fabrication_cap);

    printf("  PASS: high congestion reduces exchange and fabrication caps\n");
}

void test_zero_capacity() {
    printf("Testing zero capacity...\n");

    DemandCapInputs inputs;
    inputs.housing_capacity = 0;
    inputs.exchange_jobs = 0;
    inputs.fabrication_jobs = 0;
    inputs.energy_factor = 1.0f;
    inputs.fluid_factor = 1.0f;
    inputs.transport_factor = 1.0f;

    auto result = calculate_demand_caps(inputs);

    assert(result.habitation_cap == 0);
    assert(result.exchange_cap == 0);
    assert(result.fabrication_cap == 0);

    printf("  All caps = 0\n");

    printf("  PASS: zero capacity yields zero caps\n");
}

void test_zero_factors() {
    printf("Testing zero infrastructure factors...\n");

    DemandCapInputs inputs;
    inputs.housing_capacity = 1000;
    inputs.exchange_jobs = 500;
    inputs.fabrication_jobs = 300;
    inputs.energy_factor = 0.0f;
    inputs.fluid_factor = 0.0f;
    inputs.transport_factor = 0.0f;

    auto result = calculate_demand_caps(inputs);

    assert(result.habitation_cap == 0);
    assert(result.exchange_cap == 0);
    assert(result.fabrication_cap == 0);

    printf("  All caps = 0 when factors are zero\n");

    printf("  PASS: zero factors yield zero caps\n");
}

int main() {
    printf("=== Demand Cap Unit Tests (E10-046) ===\n\n");

    test_full_infrastructure_equals_raw_capacity();
    test_partial_energy_reduces_habitation();
    test_partial_fluid_reduces_habitation();
    test_both_energy_and_fluid_partial();
    test_high_congestion_reduces_caps();
    test_zero_capacity();
    test_zero_factors();

    printf("\n=== All tests passed! ===\n");
    return 0;
}
