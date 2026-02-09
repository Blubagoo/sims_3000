/**
 * @file test_exchange_demand.cpp
 * @brief Unit tests for exchange demand formula (E10-044)
 */

#include "sims3000/demand/ExchangeDemand.h"
#include <cassert>
#include <cstdio>

using namespace sims3000::demand;

void test_under_served_exchange_positive_demand() {
    printf("Testing under-served exchange (low jobs) gives positive demand...\n");

    ExchangeInputs inputs;
    inputs.total_beings = 300;
    inputs.exchange_jobs = 10;        // target = 300/3 = 100, ratio = 10/100 = 0.1
    inputs.unemployment_rate = 30;    // (50-30)/3 = 6.67 -> 6
    inputs.congestion_level = 0.0f;
    inputs.tribute_rate = 7.0f;

    auto result = calculate_exchange_demand(inputs);

    // population_factor = 30 * (1.0 - 0.1) = 27 -> clamped to 27
    assert(result.factors.population_factor > 0);
    assert(result.demand > 0);
    printf("  demand = %d, population_factor = %d\n",
           result.demand, result.factors.population_factor);

    printf("  PASS: under-served exchange yields positive demand\n");
}

void test_over_served_exchange_negative_demand() {
    printf("Testing over-served exchange (too many jobs) gives negative demand...\n");

    ExchangeInputs inputs;
    inputs.total_beings = 300;
    inputs.exchange_jobs = 500;       // target = 100, ratio = 5.0
    inputs.unemployment_rate = 80;    // (50-80)/3 = -10
    inputs.congestion_level = 50.0f;  // -50/5 = -10
    inputs.tribute_rate = 12.0f;      // (7-12)*2 = -10

    auto result = calculate_exchange_demand(inputs);

    // population_factor = 30 * (1.0 - 5.0) = -120 -> clamped to -20
    assert(result.factors.population_factor < 0);
    assert(result.demand < 0);
    printf("  demand = %d, population_factor = %d\n",
           result.demand, result.factors.population_factor);

    printf("  PASS: over-served exchange yields negative demand\n");
}

void test_high_unemployment_negative_employment() {
    printf("Testing high unemployment gives negative employment factor...\n");

    ExchangeInputs inputs;
    inputs.total_beings = 300;
    inputs.exchange_jobs = 100;       // target = 100, ratio = 1.0
    inputs.unemployment_rate = 95;    // (50-95)/3 = -15
    inputs.congestion_level = 0.0f;
    inputs.tribute_rate = 7.0f;

    auto result = calculate_exchange_demand(inputs);

    assert(result.factors.employment_factor == -15);
    printf("  employment_factor = %d (expected -15)\n", result.factors.employment_factor);

    printf("  PASS: high unemployment yields negative employment factor\n");
}

void test_low_unemployment_positive_employment() {
    printf("Testing low unemployment gives positive employment factor...\n");

    ExchangeInputs inputs;
    inputs.total_beings = 300;
    inputs.exchange_jobs = 100;
    inputs.unemployment_rate = 5;     // (50-5)/3 = 15
    inputs.congestion_level = 0.0f;
    inputs.tribute_rate = 7.0f;

    auto result = calculate_exchange_demand(inputs);

    assert(result.factors.employment_factor == 15);
    printf("  employment_factor = %d (expected 15)\n", result.factors.employment_factor);

    printf("  PASS: low unemployment yields positive employment factor\n");
}

void test_congestion_negative_transport() {
    printf("Testing congestion gives negative transport factor...\n");

    ExchangeInputs inputs;
    inputs.total_beings = 300;
    inputs.exchange_jobs = 100;
    inputs.unemployment_rate = 50;
    inputs.tribute_rate = 7.0f;

    inputs.congestion_level = 100.0f;  // -100/5 = -20
    auto result = calculate_exchange_demand(inputs);
    assert(result.factors.transport_factor == -20);
    printf("  congestion=100: transport_factor = %d (expected -20)\n",
           result.factors.transport_factor);

    inputs.congestion_level = 0.0f;    // 0
    auto r2 = calculate_exchange_demand(inputs);
    assert(r2.factors.transport_factor == 0);
    printf("  congestion=0: transport_factor = %d (expected 0)\n",
           r2.factors.transport_factor);

    printf("  PASS: congestion affects transport factor correctly\n");
}

void test_tribute_factor() {
    printf("Testing tribute factor...\n");

    ExchangeInputs inputs;
    inputs.total_beings = 300;
    inputs.exchange_jobs = 100;
    inputs.unemployment_rate = 50;
    inputs.congestion_level = 0.0f;

    // Low tribute -> positive
    inputs.tribute_rate = 0.0f;  // (7-0)*2 = 14 -> clamped to 10
    auto r1 = calculate_exchange_demand(inputs);
    assert(r1.factors.tribute_factor == 10);
    printf("  tribute=0%%: tribute_factor = %d (expected 10)\n", r1.factors.tribute_factor);

    // High tribute -> negative
    inputs.tribute_rate = 15.0f;  // (7-15)*2 = -16 -> clamped to -10
    auto r2 = calculate_exchange_demand(inputs);
    assert(r2.factors.tribute_factor == -10);
    printf("  tribute=15%%: tribute_factor = %d (expected -10)\n", r2.factors.tribute_factor);

    printf("  PASS: tribute factor responds to tribute rate\n");
}

void test_demand_clamped() {
    printf("Testing demand clamped to [-100, +100]...\n");

    // Push high
    {
        ExchangeInputs inputs;
        inputs.total_beings = 1000;
        inputs.exchange_jobs = 0;
        inputs.unemployment_rate = 0;
        inputs.congestion_level = 0.0f;
        inputs.tribute_rate = 0.0f;

        auto result = calculate_exchange_demand(inputs);
        printf("  high demand = %d\n", result.demand);
        assert(result.demand <= 100 && result.demand >= -100);
    }

    // Push low
    {
        ExchangeInputs inputs;
        inputs.total_beings = 10;
        inputs.exchange_jobs = 10000;
        inputs.unemployment_rate = 100;
        inputs.congestion_level = 100.0f;
        inputs.tribute_rate = 20.0f;

        auto result = calculate_exchange_demand(inputs);
        printf("  low demand = %d\n", result.demand);
        assert(result.demand <= 100 && result.demand >= -100);
    }

    printf("  PASS: demand clamped to valid range\n");
}

void test_zero_beings() {
    printf("Testing zero beings...\n");

    ExchangeInputs inputs;
    inputs.total_beings = 0;
    inputs.exchange_jobs = 0;
    inputs.unemployment_rate = 0;
    inputs.congestion_level = 0.0f;
    inputs.tribute_rate = 7.0f;

    auto result = calculate_exchange_demand(inputs);
    // target_exchange_jobs = 0/3 = 0, exchange_ratio = 0/1 = 0
    // population_factor = 30 * (1.0 - 0.0) = 30
    printf("  demand = %d\n", result.demand);
    assert(result.demand >= -100 && result.demand <= 100);

    printf("  PASS: zero beings handled without crash\n");
}

int main() {
    printf("=== Exchange Demand Unit Tests (E10-044) ===\n\n");

    test_under_served_exchange_positive_demand();
    test_over_served_exchange_negative_demand();
    test_high_unemployment_negative_employment();
    test_low_unemployment_positive_employment();
    test_congestion_negative_transport();
    test_tribute_factor();
    test_demand_clamped();
    test_zero_beings();

    printf("\n=== All tests passed! ===\n");
    return 0;
}
