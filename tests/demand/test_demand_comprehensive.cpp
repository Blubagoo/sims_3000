/**
 * @file test_demand_comprehensive.cpp
 * @brief Comprehensive integration tests for demand formula system (E10-122)
 *
 * Tests all demand calculation modules:
 * - HabitationDemand (5 factors)
 * - ExchangeDemand (4 factors)
 * - FabricationDemand (4 factors)
 * - DemandCap (3 factors)
 * - DemandFactorsUI (factor analysis)
 * - Full demand cycle integration
 */

#include <sims3000/demand/HabitationDemand.h>
#include <sims3000/demand/ExchangeDemand.h>
#include <sims3000/demand/FabricationDemand.h>
#include <sims3000/demand/DemandCap.h>
#include <sims3000/demand/DemandFactorsUI.h>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cstring>

using namespace sims3000::demand;

// Test counter
int g_test_count = 0;
int g_passed = 0;

#define TEST_ASSERT(condition, message) \
    do { \
        g_test_count++; \
        if (!(condition)) { \
            printf("FAIL [Test %d]: %s\n", g_test_count, message); \
            exit(1); \
        } else { \
            g_passed++; \
            printf("PASS [Test %d]: %s\n", g_test_count, message); \
        } \
    } while (0)

// Helper to check if value is in range
bool in_range(int8_t val, int8_t min, int8_t max) {
    return val >= min && val <= max;
}

// ============================================================================
// HABITATION DEMAND TESTS (5 factors)
// ============================================================================

void test_habitation_high_occupancy() {
    HabitationInputs inputs;
    inputs.total_beings = 950;
    inputs.housing_capacity = 1000;  // 0.95 occupancy
    inputs.labor_force = 500;
    inputs.total_jobs = 600;  // More jobs than labor
    inputs.service_coverage = 50.0f;
    inputs.tribute_rate = 7.0f;
    inputs.contamination_level = 0.0f;

    auto result = calculate_habitation_demand(inputs);
    TEST_ASSERT(result.demand > 0, "High occupancy produces positive demand");
    TEST_ASSERT(result.factors.population_factor > 0, "Population factor positive for high occupancy");
}

void test_habitation_low_occupancy() {
    HabitationInputs inputs;
    inputs.total_beings = 400;
    inputs.housing_capacity = 1000;  // 0.4 occupancy
    inputs.labor_force = 200;
    inputs.total_jobs = 250;
    inputs.service_coverage = 50.0f;
    inputs.tribute_rate = 7.0f;
    inputs.contamination_level = 0.0f;

    auto result = calculate_habitation_demand(inputs);
    TEST_ASSERT(result.factors.population_factor < 0, "Population factor negative for low occupancy");
}

void test_habitation_employment_bonus() {
    HabitationInputs inputs;
    inputs.total_beings = 500;
    inputs.housing_capacity = 1000;
    inputs.labor_force = 400;
    inputs.total_jobs = 600;  // Jobs > labor
    inputs.service_coverage = 50.0f;
    inputs.tribute_rate = 7.0f;
    inputs.contamination_level = 0.0f;

    auto result = calculate_habitation_demand(inputs);
    TEST_ASSERT(result.factors.employment_factor > 0, "Employment factor positive when jobs > labor");
}

void test_habitation_service_coverage() {
    HabitationInputs inputs;
    inputs.total_beings = 500;
    inputs.housing_capacity = 1000;
    inputs.labor_force = 400;
    inputs.total_jobs = 500;
    inputs.service_coverage = 80.0f;  // High coverage
    inputs.tribute_rate = 7.0f;
    inputs.contamination_level = 0.0f;

    auto result = calculate_habitation_demand(inputs);
    TEST_ASSERT(result.factors.services_factor > 0, "Services factor positive for high coverage");
}

void test_habitation_tribute_impact() {
    HabitationInputs inputs;
    inputs.total_beings = 500;
    inputs.housing_capacity = 1000;
    inputs.labor_force = 400;
    inputs.total_jobs = 500;
    inputs.service_coverage = 50.0f;
    inputs.tribute_rate = 15.0f;  // High tax
    inputs.contamination_level = 0.0f;

    auto result = calculate_habitation_demand(inputs);
    TEST_ASSERT(result.factors.tribute_factor < 0, "Tribute factor negative for high tax rate");
}

void test_habitation_contamination_penalty() {
    HabitationInputs inputs;
    inputs.total_beings = 500;
    inputs.housing_capacity = 1000;
    inputs.labor_force = 400;
    inputs.total_jobs = 500;
    inputs.service_coverage = 50.0f;
    inputs.tribute_rate = 7.0f;
    inputs.contamination_level = 80.0f;  // High contamination

    auto result = calculate_habitation_demand(inputs);
    TEST_ASSERT(result.factors.contamination_factor < 0, "Contamination factor negative for high pollution");
}

void test_habitation_bounds() {
    HabitationInputs inputs;
    inputs.total_beings = 1000;
    inputs.housing_capacity = 1000;
    inputs.labor_force = 800;
    inputs.total_jobs = 1200;
    inputs.service_coverage = 100.0f;
    inputs.tribute_rate = 0.0f;
    inputs.contamination_level = 0.0f;

    auto result = calculate_habitation_demand(inputs);
    TEST_ASSERT(in_range(result.demand, -100, 100), "Habitation demand clamped to [-100, +100]");
}

void test_habitation_zero_population() {
    HabitationInputs inputs;
    inputs.total_beings = 0;
    inputs.housing_capacity = 1000;
    inputs.labor_force = 0;
    inputs.total_jobs = 500;
    inputs.service_coverage = 50.0f;
    inputs.tribute_rate = 7.0f;
    inputs.contamination_level = 0.0f;

    auto result = calculate_habitation_demand(inputs);
    TEST_ASSERT(result.demand != 0, "Zero population produces demand response");
}

// ============================================================================
// EXCHANGE DEMAND TESTS (4 factors)
// ============================================================================

void test_exchange_underserved_population() {
    ExchangeInputs inputs;
    inputs.total_beings = 10000;
    inputs.exchange_jobs = 500;  // Low coverage ratio
    inputs.unemployment_rate = 5;
    inputs.congestion_level = 10.0f;
    inputs.tribute_rate = 7.0f;

    auto result = calculate_exchange_demand(inputs);
    TEST_ASSERT(result.demand > 0, "Underserved population produces positive demand");
    TEST_ASSERT(result.factors.population_factor > 0, "Population factor positive for underserved");
}

void test_exchange_unemployment_impact() {
    ExchangeInputs inputs;
    inputs.total_beings = 5000;
    inputs.exchange_jobs = 1000;
    inputs.unemployment_rate = 60;  // Very high unemployment (above 50% crossover)
    inputs.congestion_level = 10.0f;
    inputs.tribute_rate = 7.0f;

    auto result = calculate_exchange_demand(inputs);
    TEST_ASSERT(result.factors.employment_factor < 0, "Employment factor negative for very high unemployment");
}

void test_exchange_congestion_penalty() {
    ExchangeInputs inputs;
    inputs.total_beings = 5000;
    inputs.exchange_jobs = 1000;
    inputs.unemployment_rate = 5;
    inputs.congestion_level = 80.0f;  // High congestion
    inputs.tribute_rate = 7.0f;

    auto result = calculate_exchange_demand(inputs);
    TEST_ASSERT(result.factors.transport_factor < 0, "Transport factor negative for high congestion");
}

void test_exchange_tribute_factor() {
    ExchangeInputs inputs;
    inputs.total_beings = 5000;
    inputs.exchange_jobs = 1000;
    inputs.unemployment_rate = 5;
    inputs.congestion_level = 10.0f;
    inputs.tribute_rate = 20.0f;  // High tax

    auto result = calculate_exchange_demand(inputs);
    TEST_ASSERT(result.factors.tribute_factor < 0, "Tribute factor negative for high tax");
}

void test_exchange_bounds() {
    ExchangeInputs inputs;
    inputs.total_beings = 50000;
    inputs.exchange_jobs = 100;
    inputs.unemployment_rate = 0;
    inputs.congestion_level = 0.0f;
    inputs.tribute_rate = 0.0f;

    auto result = calculate_exchange_demand(inputs);
    TEST_ASSERT(in_range(result.demand, -100, 100), "Exchange demand clamped to [-100, +100]");
}

// ============================================================================
// FABRICATION DEMAND TESTS (4 factors)
// ============================================================================

void test_fabrication_underserved() {
    FabricationInputs inputs;
    inputs.total_beings = 10000;
    inputs.fabrication_jobs = 200;  // Low coverage
    inputs.labor_force = 5000;
    inputs.employed_laborers = 3000;
    inputs.has_external_connectivity = true;
    inputs.congestion_level = 10.0f;

    auto result = calculate_fabrication_demand(inputs);
    TEST_ASSERT(result.demand > 0, "Underserved fabrication produces positive demand");
    TEST_ASSERT(result.factors.population_factor > 0, "Population factor positive for underserved");
}

void test_fabrication_labor_surplus() {
    FabricationInputs inputs;
    inputs.total_beings = 5000;
    inputs.fabrication_jobs = 1000;
    inputs.labor_force = 3000;
    inputs.employed_laborers = 1500;  // Labor surplus
    inputs.has_external_connectivity = false;
    inputs.congestion_level = 10.0f;

    auto result = calculate_fabrication_demand(inputs);
    TEST_ASSERT(result.factors.employment_factor > 0, "Employment factor positive for labor surplus");
}

void test_fabrication_external_connectivity() {
    FabricationInputs inputs;
    inputs.total_beings = 5000;
    inputs.fabrication_jobs = 1000;
    inputs.labor_force = 3000;
    inputs.employed_laborers = 2000;
    inputs.has_external_connectivity = true;  // Has connectivity
    inputs.congestion_level = 10.0f;

    auto result = calculate_fabrication_demand(inputs);
    TEST_ASSERT(result.factors.transport_factor > 0, "Transport factor positive with external connectivity");
}

void test_fabrication_no_connectivity() {
    FabricationInputs inputs;
    inputs.total_beings = 5000;
    inputs.fabrication_jobs = 1000;
    inputs.labor_force = 3000;
    inputs.employed_laborers = 2000;
    inputs.has_external_connectivity = false;  // No connectivity
    inputs.congestion_level = 10.0f;

    auto result = calculate_fabrication_demand(inputs);
    TEST_ASSERT(result.factors.transport_factor < 0, "Transport factor negative without external connectivity");
}

void test_fabrication_contamination_tolerance() {
    FabricationInputs inputs;
    inputs.total_beings = 5000;
    inputs.fabrication_jobs = 1000;
    inputs.labor_force = 3000;
    inputs.employed_laborers = 2000;
    inputs.has_external_connectivity = true;
    inputs.congestion_level = 10.0f;

    auto result = calculate_fabrication_demand(inputs);
    TEST_ASSERT(result.factors.contamination_factor == 0, "Fabrication ignores contamination");
}

void test_fabrication_bounds() {
    FabricationInputs inputs;
    inputs.total_beings = 50000;
    inputs.fabrication_jobs = 50;
    inputs.labor_force = 30000;
    inputs.employed_laborers = 5000;
    inputs.has_external_connectivity = true;
    inputs.congestion_level = 0.0f;

    auto result = calculate_fabrication_demand(inputs);
    TEST_ASSERT(in_range(result.demand, -100, 100), "Fabrication demand clamped to [-100, +100]");
}

// ============================================================================
// DEMAND CAP TESTS
// ============================================================================

void test_cap_full_infrastructure() {
    DemandCapInputs inputs;
    inputs.housing_capacity = 1000;
    inputs.exchange_jobs = 500;
    inputs.fabrication_jobs = 300;
    inputs.energy_factor = 1.0f;
    inputs.fluid_factor = 1.0f;
    inputs.transport_factor = 1.0f;

    auto result = calculate_demand_caps(inputs);
    TEST_ASSERT(result.habitation_cap == 1000, "Full infrastructure = full habitation cap");
    TEST_ASSERT(result.exchange_cap == 500, "Full infrastructure = full exchange cap");
    TEST_ASSERT(result.fabrication_cap == 300, "Full infrastructure = full fabrication cap");
}

void test_cap_energy_limitation() {
    DemandCapInputs inputs;
    inputs.housing_capacity = 1000;
    inputs.exchange_jobs = 500;
    inputs.fabrication_jobs = 300;
    inputs.energy_factor = 0.5f;  // 50% powered
    inputs.fluid_factor = 1.0f;
    inputs.transport_factor = 1.0f;

    auto result = calculate_demand_caps(inputs);
    TEST_ASSERT(result.habitation_cap == 500, "50% energy = 50% habitation cap");
}

void test_cap_fluid_limitation() {
    DemandCapInputs inputs;
    inputs.housing_capacity = 1000;
    inputs.exchange_jobs = 500;
    inputs.fabrication_jobs = 300;
    inputs.energy_factor = 1.0f;
    inputs.fluid_factor = 0.3f;  // 30% watered
    inputs.transport_factor = 1.0f;

    auto result = calculate_demand_caps(inputs);
    TEST_ASSERT(result.habitation_cap == 300, "30% fluid = 30% habitation cap");
}

void test_cap_transport_limitation() {
    DemandCapInputs inputs;
    inputs.housing_capacity = 1000;
    inputs.exchange_jobs = 500;
    inputs.fabrication_jobs = 300;
    inputs.energy_factor = 1.0f;
    inputs.fluid_factor = 1.0f;
    inputs.transport_factor = 0.6f;  // 60% transport quality

    auto result = calculate_demand_caps(inputs);
    TEST_ASSERT(result.exchange_cap == 300, "60% transport = 60% exchange cap");
    TEST_ASSERT(result.fabrication_cap == 180, "60% transport = 60% fabrication cap");
}

void test_cap_zero_infrastructure() {
    DemandCapInputs inputs;
    inputs.housing_capacity = 1000;
    inputs.exchange_jobs = 500;
    inputs.fabrication_jobs = 300;
    inputs.energy_factor = 0.0f;
    inputs.fluid_factor = 0.0f;
    inputs.transport_factor = 0.0f;

    auto result = calculate_demand_caps(inputs);
    TEST_ASSERT(result.habitation_cap == 0, "No infrastructure = zero habitation cap");
    TEST_ASSERT(result.exchange_cap == 0, "No infrastructure = zero exchange cap");
    TEST_ASSERT(result.fabrication_cap == 0, "No infrastructure = zero fabrication cap");
}

// ============================================================================
// DEMAND FACTORS UI TESTS
// ============================================================================

void test_ui_get_factors() {
    DemandData data;
    data.habitation_factors.population_factor = 10;
    data.exchange_factors.employment_factor = -15;
    data.fabrication_factors.transport_factor = 20;

    auto& hab_factors = get_demand_factors(data, ZONE_HABITATION);
    auto& exc_factors = get_demand_factors(data, ZONE_EXCHANGE);
    auto& fab_factors = get_demand_factors(data, ZONE_FABRICATION);

    TEST_ASSERT(hab_factors.population_factor == 10, "Get habitation factors");
    TEST_ASSERT(exc_factors.employment_factor == -15, "Get exchange factors");
    TEST_ASSERT(fab_factors.transport_factor == 20, "Get fabrication factors");
}

void test_ui_dominant_factor() {
    DemandFactors factors;
    factors.population_factor = 5;
    factors.employment_factor = -20;  // Dominant
    factors.services_factor = 3;
    factors.tribute_factor = -10;
    factors.transport_factor = 0;
    factors.contamination_factor = 0;

    const char* dominant = get_dominant_factor_name(factors);
    TEST_ASSERT(strcmp(dominant, "employment") == 0, "Identify dominant factor");
}

void test_ui_demand_description() {
    TEST_ASSERT(strcmp(get_demand_description(80), "Strong Growth") == 0, "Strong growth description");
    TEST_ASSERT(strcmp(get_demand_description(30), "Growth") == 0, "Growth description");
    TEST_ASSERT(strcmp(get_demand_description(15), "Weak Growth") == 0, "Weak growth description");
    TEST_ASSERT(strcmp(get_demand_description(0), "Stagnant") == 0, "Stagnant description");
    TEST_ASSERT(strcmp(get_demand_description(-80), "Strong Decline") == 0, "Strong decline description");
}

void test_ui_sum_factors() {
    DemandFactors factors;
    factors.population_factor = 10;
    factors.employment_factor = 15;
    factors.services_factor = -5;
    factors.tribute_factor = -10;
    factors.transport_factor = 20;
    factors.contamination_factor = -10;

    int16_t sum = sum_factors(factors);
    TEST_ASSERT(sum == 20, "Sum all factors correctly");
}

void test_ui_bottleneck_detection() {
    DemandFactors factors;
    factors.population_factor = 10;
    factors.employment_factor = 5;
    factors.services_factor = -30;  // Largest negative
    factors.tribute_factor = -10;
    factors.transport_factor = 0;
    factors.contamination_factor = -5;

    TEST_ASSERT(is_bottlenecked_by(factors, "services"), "Detect services bottleneck");
    TEST_ASSERT(!is_bottlenecked_by(factors, "tribute"), "Reject non-dominant bottleneck");
}

// ============================================================================
// FULL DEMAND CYCLE TEST
// ============================================================================

void test_full_demand_cycle() {
    // Setup scenario: small growing city
    HabitationInputs hab_in;
    hab_in.total_beings = 8000;
    hab_in.housing_capacity = 10000;
    hab_in.labor_force = 5000;
    hab_in.total_jobs = 6000;
    hab_in.service_coverage = 60.0f;
    hab_in.tribute_rate = 7.0f;
    hab_in.contamination_level = 20.0f;

    ExchangeInputs exc_in;
    exc_in.total_beings = 8000;
    exc_in.exchange_jobs = 1500;
    exc_in.unemployment_rate = 10;
    exc_in.congestion_level = 25.0f;
    exc_in.tribute_rate = 7.0f;

    FabricationInputs fab_in;
    fab_in.total_beings = 8000;
    fab_in.fabrication_jobs = 800;
    fab_in.labor_force = 5000;
    fab_in.employed_laborers = 4500;
    fab_in.has_external_connectivity = true;
    fab_in.congestion_level = 25.0f;

    DemandCapInputs cap_in;
    cap_in.housing_capacity = 10000;
    cap_in.exchange_jobs = 3000;
    cap_in.fabrication_jobs = 2000;
    cap_in.energy_factor = 0.8f;
    cap_in.fluid_factor = 0.7f;
    cap_in.transport_factor = 0.75f;

    // Calculate all demands and caps
    auto hab_result = calculate_habitation_demand(hab_in);
    auto exc_result = calculate_exchange_demand(exc_in);
    auto fab_result = calculate_fabrication_demand(fab_in);
    auto cap_result = calculate_demand_caps(cap_in);

    // Verify demands are in valid range
    TEST_ASSERT(in_range(hab_result.demand, -100, 100), "Full cycle: habitation demand valid");
    TEST_ASSERT(in_range(exc_result.demand, -100, 100), "Full cycle: exchange demand valid");
    TEST_ASSERT(in_range(fab_result.demand, -100, 100), "Full cycle: fabrication demand valid");

    // Verify caps are calculated
    TEST_ASSERT(cap_result.habitation_cap > 0 && cap_result.habitation_cap <= 10000,
                "Full cycle: habitation cap valid");
    TEST_ASSERT(cap_result.exchange_cap > 0 && cap_result.exchange_cap <= 3000,
                "Full cycle: exchange cap valid");
    TEST_ASSERT(cap_result.fabrication_cap > 0 && cap_result.fabrication_cap <= 2000,
                "Full cycle: fabrication cap valid");

    // Build DemandData for UI queries
    DemandData data;
    data.habitation_demand = hab_result.demand;
    data.exchange_demand = exc_result.demand;
    data.fabrication_demand = fab_result.demand;
    data.habitation_cap = cap_result.habitation_cap;
    data.exchange_cap = cap_result.exchange_cap;
    data.fabrication_cap = cap_result.fabrication_cap;
    data.habitation_factors = hab_result.factors;
    data.exchange_factors = exc_result.factors;
    data.fabrication_factors = fab_result.factors;

    // Verify UI queries work
    const char* hab_desc = get_demand_description(data.habitation_demand);
    TEST_ASSERT(hab_desc != nullptr && strlen(hab_desc) > 0, "Full cycle: UI description valid");

    const char* hab_dominant = get_dominant_factor_name(data.habitation_factors);
    TEST_ASSERT(hab_dominant != nullptr && strlen(hab_dominant) > 0, "Full cycle: dominant factor valid");
}

// ============================================================================
// MAIN
// ============================================================================

int main() {
    printf("=== Comprehensive Demand Formula Tests (E10-122) ===\n\n");

    printf("--- Habitation Demand Tests ---\n");
    test_habitation_high_occupancy();
    test_habitation_low_occupancy();
    test_habitation_employment_bonus();
    test_habitation_service_coverage();
    test_habitation_tribute_impact();
    test_habitation_contamination_penalty();
    test_habitation_bounds();
    test_habitation_zero_population();

    printf("\n--- Exchange Demand Tests ---\n");
    test_exchange_underserved_population();
    test_exchange_unemployment_impact();
    test_exchange_congestion_penalty();
    test_exchange_tribute_factor();
    test_exchange_bounds();

    printf("\n--- Fabrication Demand Tests ---\n");
    test_fabrication_underserved();
    test_fabrication_labor_surplus();
    test_fabrication_external_connectivity();
    test_fabrication_no_connectivity();
    test_fabrication_contamination_tolerance();
    test_fabrication_bounds();

    printf("\n--- Demand Cap Tests ---\n");
    test_cap_full_infrastructure();
    test_cap_energy_limitation();
    test_cap_fluid_limitation();
    test_cap_transport_limitation();
    test_cap_zero_infrastructure();

    printf("\n--- Demand Factors UI Tests ---\n");
    test_ui_get_factors();
    test_ui_dominant_factor();
    test_ui_demand_description();
    test_ui_sum_factors();
    test_ui_bottleneck_detection();

    printf("\n--- Full Demand Cycle Test ---\n");
    test_full_demand_cycle();

    printf("\n=== All %d tests passed ===\n", g_test_count);
    return 0;
}
