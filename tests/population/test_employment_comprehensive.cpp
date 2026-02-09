/**
 * @file test_employment_comprehensive.cpp
 * @brief Comprehensive integration tests for employment (Ticket E10-121)
 *
 * Tests labor pool calculation, job market aggregation, employment matching,
 * occupancy distribution, and unemployment effects together.
 *
 * Validates:
 * - Labor pool: base participation, harmony/education bonuses
 * - Job market: aggregation from building capacities
 * - Employment matching: proportional distribution, zero cases
 * - Occupancy distribution: proportional fill, state transitions
 * - Unemployment effects: full employment bonus, penalty scaling, cap
 * - Full employment cycle integration
 */

#include <cassert>
#include <cmath>
#include <cstdio>
#include <vector>

#include "sims3000/population/LaborPoolCalculation.h"
#include "sims3000/population/JobMarketAggregation.h"
#include "sims3000/population/EmploymentMatching.h"
#include "sims3000/population/OccupancyDistribution.h"
#include "sims3000/population/UnemploymentEffects.h"
#include "sims3000/population/PopulationData.h"
#include "sims3000/population/EmploymentData.h"

using namespace sims3000::population;

// Helper: float approximate equality
static bool approx(float a, float b, float epsilon = 0.01f) {
    return std::fabs(a - b) < epsilon;
}

// --------------------------------------------------------------------------
// Labor Pool Tests
// --------------------------------------------------------------------------

static void test_labor_pool_base_participation() {
    PopulationData pop{};
    pop.total_beings = 10000;
    pop.adult_percent = 60; // 6000 working-age
    pop.harmony_index = 50; // Neutral
    pop.education_index = 50; // Neutral

    EmploymentData emp{};

    LaborPoolResult result = calculate_labor_pool(pop, emp);

    assert(result.working_age_beings == 6000 && "Working-age should be adult_percent of total");
    assert(approx(result.labor_participation_rate, constants::BASE_LABOR_PARTICIPATION)
           && "Base participation should be 65% with neutral indices");
    assert(result.labor_force == static_cast<uint32_t>(6000 * constants::BASE_LABOR_PARTICIPATION)
           && "Labor force should be participation rate * working-age");

    printf("[PASS] Labor pool base participation\n");
}

static void test_labor_pool_harmony_bonus() {
    PopulationData pop{};
    pop.total_beings = 10000;
    pop.adult_percent = 60;
    pop.harmony_index = 100; // Max harmony
    pop.education_index = 50;

    EmploymentData emp{};

    LaborPoolResult result = calculate_labor_pool(pop, emp);

    // Should have harmony bonus
    float expected_rate = constants::BASE_LABOR_PARTICIPATION + constants::HARMONY_PARTICIPATION_BONUS;
    assert(approx(result.labor_participation_rate, expected_rate)
           && "Max harmony should add 10% participation bonus");
    assert(result.labor_force > 3900 && "Labor force should be increased by harmony bonus");

    printf("[PASS] Labor pool harmony bonus\n");
}

static void test_labor_pool_education_bonus() {
    PopulationData pop{};
    pop.total_beings = 10000;
    pop.adult_percent = 60;
    pop.harmony_index = 50;
    pop.education_index = 100; // Max education

    EmploymentData emp{};

    LaborPoolResult result = calculate_labor_pool(pop, emp);

    // Should have education bonus
    float expected_rate = constants::BASE_LABOR_PARTICIPATION + constants::EDUCATION_PARTICIPATION_BONUS;
    assert(approx(result.labor_participation_rate, expected_rate)
           && "Max education should add 10% participation bonus");
    assert(result.labor_force > 3900 && "Labor force should be increased by education bonus");

    printf("[PASS] Labor pool education bonus\n");
}

static void test_labor_pool_combined_bonuses() {
    PopulationData pop{};
    pop.total_beings = 10000;
    pop.adult_percent = 60;
    pop.harmony_index = 100; // Max harmony
    pop.education_index = 100; // Max education

    EmploymentData emp{};

    LaborPoolResult result = calculate_labor_pool(pop, emp);

    // Should have both bonuses, but clamped at 1.0
    assert(result.labor_participation_rate <= 1.0f && "Participation rate should be clamped at 100%");
    assert(result.labor_force <= result.working_age_beings
           && "Labor force cannot exceed working-age population");

    printf("[PASS] Labor pool combined bonuses (clamped)\n");
}

// --------------------------------------------------------------------------
// Job Market Tests
// --------------------------------------------------------------------------

static void test_job_market_aggregation() {
    uint32_t exchange_capacity = 1000;
    uint32_t fabrication_capacity = 2000;

    JobMarketResult result = aggregate_job_market(exchange_capacity, fabrication_capacity);

    assert(result.exchange_jobs == 1000 && "Exchange jobs should match capacity");
    assert(result.fabrication_jobs == 2000 && "Fabrication jobs should match capacity");
    assert(result.total_jobs == 3000 && "Total jobs should be sum of sectors");

    printf("[PASS] Job market aggregation\n");
}

static void test_job_market_zero_jobs() {
    JobMarketResult result = aggregate_job_market(0, 0);

    assert(result.exchange_jobs == 0 && "Should handle zero exchange jobs");
    assert(result.fabrication_jobs == 0 && "Should handle zero fabrication jobs");
    assert(result.total_jobs == 0 && "Total should be zero");

    printf("[PASS] Job market with zero jobs\n");
}

// --------------------------------------------------------------------------
// Employment Matching Tests
// --------------------------------------------------------------------------

static void test_employment_matching_proportional_distribution() {
    uint32_t labor_force = 3000;
    uint32_t exchange_jobs = 1000;
    uint32_t fabrication_jobs = 2000;

    EmploymentMatchingResult result = match_employment(labor_force, exchange_jobs, fabrication_jobs);

    assert(result.employed_laborers == 3000 && "All workers should be employed when jobs >= labor");
    assert(result.unemployed == 0 && "Should have zero unemployment");
    assert(result.unemployment_rate == 0 && "Unemployment rate should be 0%");

    // Check proportional distribution: 1:2 ratio
    float ratio = static_cast<float>(result.exchange_employed) / static_cast<float>(result.fabrication_employed);
    assert(approx(ratio, 0.5f, 0.1f) && "Employment should be distributed proportionally");

    printf("[PASS] Employment matching proportional distribution\n");
}

static void test_employment_matching_labor_shortage() {
    uint32_t labor_force = 1000;
    uint32_t exchange_jobs = 2000;
    uint32_t fabrication_jobs = 3000;

    EmploymentMatchingResult result = match_employment(labor_force, exchange_jobs, fabrication_jobs);

    assert(result.employed_laborers == 1000 && "All available workers should be employed");
    assert(result.unemployed == 0 && "Should have zero unemployment with job surplus");
    assert(result.unemployment_rate == 0 && "Unemployment rate should be 0%");
    assert(result.exchange_employed + result.fabrication_employed == 1000
           && "Total employed should equal labor force");

    printf("[PASS] Employment matching with labor shortage\n");
}

static void test_employment_matching_job_shortage() {
    uint32_t labor_force = 5000;
    uint32_t exchange_jobs = 800;
    uint32_t fabrication_jobs = 1200;

    EmploymentMatchingResult result = match_employment(labor_force, exchange_jobs, fabrication_jobs);

    assert(result.employed_laborers == 2000 && "Only job count should be filled");
    assert(result.unemployed == 3000 && "Remaining workers should be unemployed");
    assert(result.unemployment_rate == 60 && "Unemployment rate should be 60%");

    printf("[PASS] Employment matching with job shortage\n");
}

static void test_employment_matching_zero_jobs() {
    uint32_t labor_force = 1000;

    EmploymentMatchingResult result = match_employment(labor_force, 0, 0);

    assert(result.employed_laborers == 0 && "No one should be employed with no jobs");
    assert(result.unemployed == 1000 && "All workers should be unemployed");
    assert(result.unemployment_rate == 100 && "Unemployment rate should be 100%");
    assert(result.exchange_employed == 0 && "Exchange employment should be zero");
    assert(result.fabrication_employed == 0 && "Fabrication employment should be zero");

    printf("[PASS] Employment matching with zero jobs\n");
}

static void test_employment_matching_zero_labor() {
    EmploymentMatchingResult result = match_employment(0, 1000, 2000);

    assert(result.employed_laborers == 0 && "No one to employ");
    assert(result.unemployed == 0 && "No unemployed");
    assert(result.unemployment_rate == 0 && "Unemployment rate should be 0% (no labor force)");

    printf("[PASS] Employment matching with zero labor force\n");
}

// --------------------------------------------------------------------------
// Occupancy Distribution Tests
// --------------------------------------------------------------------------

static void test_occupancy_distribution_proportional_fill() {
    std::vector<BuildingOccupancyInput> buildings = {
        {1, 100, 0}, // Habitation, capacity 100
        {2, 200, 0}, // Habitation, capacity 200
        {3, 150, 1}, // Exchange (should be filtered out)
    };

    uint32_t total_beings = 150;

    std::vector<OccupancyResult> results = distribute_occupancy(total_beings, buildings);

    assert(results.size() == 2 && "Should only return habitation buildings");

    // Total capacity = 300, population = 150, so 50% fill
    uint32_t total_occupancy = 0;
    for (const auto& res : results) {
        total_occupancy += res.occupancy;
    }
    assert(total_occupancy == 150 && "Total occupancy should equal population");

    // Check proportional distribution
    for (const auto& res : results) {
        if (res.building_id == 1) {
            assert(res.occupancy == 50 && "Building 1 should have 1/3 of population");
            assert(res.state == 2 && "50% occupancy should be NormalOccupied (state 2)");
        } else if (res.building_id == 2) {
            assert(res.occupancy == 100 && "Building 2 should have 2/3 of population");
            assert(res.state == 2 && "50% occupancy should be NormalOccupied (state 2)");
        }
    }

    printf("[PASS] Occupancy distribution proportional fill\n");
}

static void test_occupancy_distribution_state_empty() {
    std::vector<BuildingOccupancyInput> buildings = {
        {1, 100, 0},
    };

    std::vector<OccupancyResult> results = distribute_occupancy(0, buildings);

    assert(results.size() == 1 && "Should return one result");
    assert(results[0].occupancy == 0 && "Should have zero occupancy");
    assert(results[0].state == 0 && "Should be Empty state");

    printf("[PASS] Occupancy distribution state: Empty\n");
}

static void test_occupancy_distribution_state_underoccupied() {
    std::vector<BuildingOccupancyInput> buildings = {
        {1, 100, 0},
    };

    std::vector<OccupancyResult> results = distribute_occupancy(30, buildings);

    assert(results[0].occupancy == 30 && "Should have 30 occupancy");
    assert(results[0].state == 1 && "30% occupancy should be UnderOccupied (state 1)");

    printf("[PASS] Occupancy distribution state: UnderOccupied\n");
}

static void test_occupancy_distribution_state_normal() {
    std::vector<BuildingOccupancyInput> buildings = {
        {1, 100, 0},
    };

    std::vector<OccupancyResult> results = distribute_occupancy(70, buildings);

    assert(results[0].occupancy == 70 && "Should have 70 occupancy");
    assert(results[0].state == 2 && "70% occupancy should be NormalOccupied (state 2)");

    printf("[PASS] Occupancy distribution state: NormalOccupied\n");
}

static void test_occupancy_distribution_state_fully_occupied() {
    std::vector<BuildingOccupancyInput> buildings = {
        {1, 100, 0},
    };

    std::vector<OccupancyResult> results = distribute_occupancy(95, buildings);

    assert(results[0].occupancy == 95 && "Should have 95 occupancy");
    assert(results[0].state == 3 && "95% occupancy should be FullyOccupied (state 3)");

    printf("[PASS] Occupancy distribution state: FullyOccupied\n");
}

static void test_occupancy_distribution_state_overcrowded() {
    std::vector<BuildingOccupancyInput> buildings = {
        {1, 100, 0},
    };

    // More population than capacity
    std::vector<OccupancyResult> results = distribute_occupancy(150, buildings);

    assert(results[0].occupancy == 150 && "Should have 150 occupancy (overcrowded)");
    assert(results[0].state == 4 && "150% occupancy should be Overcrowded (state 4)");

    printf("[PASS] Occupancy distribution state: Overcrowded\n");
}

// --------------------------------------------------------------------------
// Unemployment Effects Tests
// --------------------------------------------------------------------------

static void test_unemployment_effects_full_employment_bonus() {
    float unemployment_rate = 1.5f; // Very low unemployment

    UnemploymentEffectResult result = calculate_unemployment_effect(unemployment_rate);

    assert(result.is_full_employment && "Should be considered full employment at <= 2%");
    assert(approx(result.harmony_modifier, FULL_EMPLOYMENT_BONUS)
           && "Should apply full employment bonus");

    printf("[PASS] Unemployment effects: full employment bonus\n");
}

static void test_unemployment_effects_penalty_scaling() {
    float unemployment_rate_low = 5.0f;
    float unemployment_rate_high = 15.0f;

    UnemploymentEffectResult result_low = calculate_unemployment_effect(unemployment_rate_low);
    UnemploymentEffectResult result_high = calculate_unemployment_effect(unemployment_rate_high);

    assert(!result_low.is_full_employment && "5% should not be full employment");
    assert(!result_high.is_full_employment && "15% should not be full employment");

    assert(result_low.harmony_modifier < 0.0f && "Should have negative harmony modifier");
    assert(result_high.harmony_modifier < result_low.harmony_modifier
           && "Higher unemployment should have larger penalty");

    printf("[PASS] Unemployment effects: penalty scaling\n");
}

static void test_unemployment_effects_penalty_cap() {
    float unemployment_rate = 80.0f; // Very high unemployment

    UnemploymentEffectResult result = calculate_unemployment_effect(unemployment_rate);

    assert(result.harmony_modifier >= -MAX_UNEMPLOYMENT_PENALTY
           && "Penalty should be capped at -30");

    printf("[PASS] Unemployment effects: penalty cap\n");
}

static void test_unemployment_effects_apply_to_population() {
    PopulationData pop{};
    pop.harmony_index = 50;

    // Full employment: should increase harmony
    apply_unemployment_effect(pop, 1.0f);
    assert(pop.harmony_index > 50 && "Full employment should increase harmony");

    // High unemployment: should decrease harmony
    pop.harmony_index = 50;
    apply_unemployment_effect(pop, 20.0f);
    assert(pop.harmony_index < 50 && "High unemployment should decrease harmony");

    // Test clamping at 0
    pop.harmony_index = 10;
    apply_unemployment_effect(pop, 80.0f);
    assert(pop.harmony_index >= 0 && pop.harmony_index <= 100
           && "Harmony should be clamped to [0, 100]");

    printf("[PASS] Unemployment effects: apply to population\n");
}

// --------------------------------------------------------------------------
// Full Employment Cycle Tests
// --------------------------------------------------------------------------

static void test_full_employment_cycle_balanced() {
    // Setup population
    PopulationData pop{};
    pop.total_beings = 10000;
    pop.adult_percent = 60; // 6000 working-age
    pop.harmony_index = 60;
    pop.education_index = 60;

    EmploymentData emp{};

    // Step 1: Calculate labor pool
    LaborPoolResult labor_result = calculate_labor_pool(pop, emp);

    // Step 2: Aggregate job market (balanced jobs)
    JobMarketResult job_result = aggregate_job_market(2000, 2000); // 4000 jobs total

    // Step 3: Match employment
    EmploymentMatchingResult match_result = match_employment(
        labor_result.labor_force,
        job_result.exchange_jobs,
        job_result.fabrication_jobs
    );

    // Step 4: Apply unemployment effects
    apply_unemployment_effect(pop, match_result.unemployment_rate);

    // Validate full cycle
    assert(labor_result.labor_force > 0 && "Should have labor force");
    assert(match_result.employed_laborers > 0 && "Should have employed workers");
    assert(match_result.unemployed >= 0 && "Should have unemployment count");

    printf("[PASS] Full employment cycle: balanced economy\n");
}

static void test_full_employment_cycle_job_shortage() {
    PopulationData pop{};
    pop.total_beings = 10000;
    pop.adult_percent = 60;
    pop.harmony_index = 60;
    pop.education_index = 60;

    EmploymentData emp{};

    // Labor pool
    LaborPoolResult labor_result = calculate_labor_pool(pop, emp);

    // Job market with shortage
    JobMarketResult job_result = aggregate_job_market(500, 500); // Only 1000 jobs

    // Match employment
    EmploymentMatchingResult match_result = match_employment(
        labor_result.labor_force,
        job_result.exchange_jobs,
        job_result.fabrication_jobs
    );

    // Should have high unemployment
    assert(match_result.unemployment_rate > 50 && "Should have high unemployment with job shortage");
    assert(match_result.unemployed > match_result.employed_laborers
           && "More unemployed than employed");

    // Apply effects (should hurt harmony)
    uint8_t initial_harmony = pop.harmony_index;
    apply_unemployment_effect(pop, match_result.unemployment_rate);
    assert(pop.harmony_index < initial_harmony && "High unemployment should reduce harmony");

    printf("[PASS] Full employment cycle: job shortage scenario\n");
}

static void test_full_employment_cycle_labor_shortage() {
    PopulationData pop{};
    pop.total_beings = 10000;
    pop.adult_percent = 30; // Only 3000 working-age (small workforce)
    pop.harmony_index = 60;
    pop.education_index = 40; // Low education

    EmploymentData emp{};

    // Labor pool (small due to low adult population)
    LaborPoolResult labor_result = calculate_labor_pool(pop, emp);

    // Job market with plenty of jobs
    JobMarketResult job_result = aggregate_job_market(3000, 3000); // 6000 jobs

    // Match employment
    EmploymentMatchingResult match_result = match_employment(
        labor_result.labor_force,
        job_result.exchange_jobs,
        job_result.fabrication_jobs
    );

    // Should have full employment (or very low unemployment)
    assert(match_result.unemployment_rate <= 5 && "Should have very low unemployment with labor shortage");

    // Apply effects (should boost harmony)
    uint8_t initial_harmony = pop.harmony_index;
    apply_unemployment_effect(pop, match_result.unemployment_rate);
    assert(pop.harmony_index >= initial_harmony && "Low unemployment should not reduce harmony");

    printf("[PASS] Full employment cycle: labor shortage scenario\n");
}

static void test_full_employment_cycle_with_occupancy() {
    PopulationData pop{};
    pop.total_beings = 5000;
    pop.adult_percent = 60;
    pop.harmony_index = 60;
    pop.education_index = 60;

    EmploymentData emp{};

    // Buildings for occupancy
    std::vector<BuildingOccupancyInput> buildings = {
        {1, 2000, 0}, // Habitation
        {2, 3000, 0}, // Habitation
        {3, 1000, 1}, // Exchange
        {4, 1500, 2}, // Fabrication
    };

    // Full cycle
    LaborPoolResult labor_result = calculate_labor_pool(pop, emp);
    JobMarketResult job_result = aggregate_job_market(1000, 1500);
    EmploymentMatchingResult match_result = match_employment(
        labor_result.labor_force,
        job_result.exchange_jobs,
        job_result.fabrication_jobs
    );
    std::vector<OccupancyResult> occupancy_results = distribute_occupancy(pop.total_beings, buildings);

    // Validate occupancy
    assert(occupancy_results.size() == 2 && "Should have 2 habitation buildings");
    uint32_t total_occupancy = 0;
    for (const auto& res : occupancy_results) {
        total_occupancy += res.occupancy;
    }
    assert(total_occupancy == pop.total_beings && "Total occupancy should match population");

    printf("[PASS] Full employment cycle: with occupancy distribution\n");
}

static void test_full_employment_cycle_zero_population() {
    PopulationData pop{};
    pop.total_beings = 0;
    pop.adult_percent = 60;

    EmploymentData emp{};

    LaborPoolResult labor_result = calculate_labor_pool(pop, emp);
    JobMarketResult job_result = aggregate_job_market(1000, 1000);
    EmploymentMatchingResult match_result = match_employment(
        labor_result.labor_force,
        job_result.exchange_jobs,
        job_result.fabrication_jobs
    );

    assert(labor_result.labor_force == 0 && "Zero population should have zero labor force");
    assert(match_result.employed_laborers == 0 && "No one to employ");
    assert(match_result.unemployment_rate == 0 && "Unemployment rate should be 0% (no labor force)");

    printf("[PASS] Full employment cycle: zero population\n");
}

// --------------------------------------------------------------------------
// Main
// --------------------------------------------------------------------------

int main() {
    printf("=== Employment Comprehensive Tests (E10-121) ===\n\n");

    printf("-- Labor Pool Tests --\n");
    test_labor_pool_base_participation();
    test_labor_pool_harmony_bonus();
    test_labor_pool_education_bonus();
    test_labor_pool_combined_bonuses();

    printf("\n-- Job Market Tests --\n");
    test_job_market_aggregation();
    test_job_market_zero_jobs();

    printf("\n-- Employment Matching Tests --\n");
    test_employment_matching_proportional_distribution();
    test_employment_matching_labor_shortage();
    test_employment_matching_job_shortage();
    test_employment_matching_zero_jobs();
    test_employment_matching_zero_labor();

    printf("\n-- Occupancy Distribution Tests --\n");
    test_occupancy_distribution_proportional_fill();
    test_occupancy_distribution_state_empty();
    test_occupancy_distribution_state_underoccupied();
    test_occupancy_distribution_state_normal();
    test_occupancy_distribution_state_fully_occupied();
    test_occupancy_distribution_state_overcrowded();

    printf("\n-- Unemployment Effects Tests --\n");
    test_unemployment_effects_full_employment_bonus();
    test_unemployment_effects_penalty_scaling();
    test_unemployment_effects_penalty_cap();
    test_unemployment_effects_apply_to_population();

    printf("\n-- Full Employment Cycle Tests --\n");
    test_full_employment_cycle_balanced();
    test_full_employment_cycle_job_shortage();
    test_full_employment_cycle_labor_shortage();
    test_full_employment_cycle_with_occupancy();
    test_full_employment_cycle_zero_population();

    printf("\n=== All Employment Comprehensive Tests Passed ===\n");
    return 0;
}
