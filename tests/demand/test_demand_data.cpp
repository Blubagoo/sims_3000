/**
 * @file test_demand_data.cpp
 * @brief Unit tests for DemandData and DemandFactors (E10-040)
 */

#include "sims3000/demand/DemandData.h"
#include <cassert>
#include <cstdio>

using namespace sims3000::demand;

void test_demand_factors_defaults() {
    printf("Testing DemandFactors default values...\n");

    DemandFactors factors;
    assert(factors.population_factor == 0);
    assert(factors.employment_factor == 0);
    assert(factors.services_factor == 0);
    assert(factors.tribute_factor == 0);
    assert(factors.transport_factor == 0);
    assert(factors.contamination_factor == 0);

    printf("  PASS: DemandFactors defaults are all zero\n");
}

void test_demand_factors_ranges() {
    printf("Testing DemandFactors value ranges...\n");

    DemandFactors factors;
    factors.population_factor = 100;
    factors.employment_factor = -100;
    factors.services_factor = 50;
    factors.tribute_factor = -50;
    factors.transport_factor = 127;
    factors.contamination_factor = -128;

    assert(factors.population_factor == 100);
    assert(factors.employment_factor == -100);
    assert(factors.services_factor == 50);
    assert(factors.tribute_factor == -50);
    assert(factors.transport_factor == 127);
    assert(factors.contamination_factor == -128);

    printf("  PASS: DemandFactors handles int8_t range\n");
}

void test_demand_data_defaults() {
    printf("Testing DemandData default values...\n");

    DemandData data;

    // Raw demand values default to 0
    assert(data.habitation_demand == 0);
    assert(data.exchange_demand == 0);
    assert(data.fabrication_demand == 0);

    // Caps default to 0
    assert(data.habitation_cap == 0);
    assert(data.exchange_cap == 0);
    assert(data.fabrication_cap == 0);

    // Factor breakdowns default to all zeros
    assert(data.habitation_factors.population_factor == 0);
    assert(data.exchange_factors.population_factor == 0);
    assert(data.fabrication_factors.population_factor == 0);

    printf("  PASS: DemandData defaults are all zero\n");
}

void test_demand_data_size() {
    printf("Testing DemandData size...\n");

    // DemandFactors: 6 x int8_t = 6 bytes
    printf("  DemandFactors size: %zu bytes\n", sizeof(DemandFactors));
    assert(sizeof(DemandFactors) == 6);

    // DemandData target: ~40 bytes
    // 3 x int8_t (demands) + padding + 3 x uint32_t (caps) + 3 x DemandFactors
    // = 3 + padding + 12 + 18 = ~33-40 depending on padding
    printf("  DemandData size: %zu bytes\n", sizeof(DemandData));
    // Verify it's in a reasonable range (compiler padding may vary)
    assert(sizeof(DemandData) <= 48);

    printf("  PASS: DemandData size is reasonable\n");
}

void test_demand_data_mutation() {
    printf("Testing DemandData mutation...\n");

    DemandData data;
    data.habitation_demand = 75;
    data.exchange_demand = -30;
    data.fabrication_demand = 100;

    data.habitation_cap = 1000;
    data.exchange_cap = 500;
    data.fabrication_cap = 2000;

    data.habitation_factors.population_factor = 50;
    data.habitation_factors.employment_factor = 25;
    data.exchange_factors.services_factor = -10;
    data.fabrication_factors.contamination_factor = -40;

    assert(data.habitation_demand == 75);
    assert(data.exchange_demand == -30);
    assert(data.fabrication_demand == 100);
    assert(data.habitation_cap == 1000);
    assert(data.exchange_cap == 500);
    assert(data.fabrication_cap == 2000);
    assert(data.habitation_factors.population_factor == 50);
    assert(data.habitation_factors.employment_factor == 25);
    assert(data.exchange_factors.services_factor == -10);
    assert(data.fabrication_factors.contamination_factor == -40);

    printf("  PASS: DemandData mutation works correctly\n");
}

int main() {
    printf("=== DemandData Unit Tests (E10-040) ===\n\n");

    test_demand_factors_defaults();
    test_demand_factors_ranges();
    test_demand_data_defaults();
    test_demand_data_size();
    test_demand_data_mutation();

    printf("\n=== All tests passed! ===\n");
    return 0;
}
