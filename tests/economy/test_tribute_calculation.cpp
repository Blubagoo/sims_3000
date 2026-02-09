/**
 * @file test_tribute_calculation.cpp
 * @brief Unit tests for TributeCalculation engine (E11-007)
 *
 * Tests: base tribute values per zone/density, per-building formula with
 *        various occupancy/sector/rate/modifier combinations, edge cases
 *        (zero capacity, zero rate), and aggregate function.
 */

#include "sims3000/economy/TributeCalculation.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <vector>

using namespace sims3000::economy;

// Helper: float approximately equal
static bool approx(float a, float b, float eps = 0.001f) {
    return std::fabs(a - b) < eps;
}

// ---------------------------------------------------------------------------
// Base tribute value constants
// ---------------------------------------------------------------------------

void test_base_tribute_habitation_low() {
    printf("Testing base tribute: Habitation low density...\n");
    assert(get_base_tribute_value(ZoneBuildingType::Habitation, 0) == 50);
    printf("  PASS: Habitation low = 50\n");
}

void test_base_tribute_habitation_high() {
    printf("Testing base tribute: Habitation high density...\n");
    assert(get_base_tribute_value(ZoneBuildingType::Habitation, 1) == 200);
    printf("  PASS: Habitation high = 200\n");
}

void test_base_tribute_exchange_low() {
    printf("Testing base tribute: Exchange low density...\n");
    assert(get_base_tribute_value(ZoneBuildingType::Exchange, 0) == 100);
    printf("  PASS: Exchange low = 100\n");
}

void test_base_tribute_exchange_high() {
    printf("Testing base tribute: Exchange high density...\n");
    assert(get_base_tribute_value(ZoneBuildingType::Exchange, 1) == 400);
    printf("  PASS: Exchange high = 400\n");
}

void test_base_tribute_fabrication_low() {
    printf("Testing base tribute: Fabrication low density...\n");
    assert(get_base_tribute_value(ZoneBuildingType::Fabrication, 0) == 75);
    printf("  PASS: Fabrication low = 75\n");
}

void test_base_tribute_fabrication_high() {
    printf("Testing base tribute: Fabrication high density...\n");
    assert(get_base_tribute_value(ZoneBuildingType::Fabrication, 1) == 300);
    printf("  PASS: Fabrication high = 300\n");
}

// ---------------------------------------------------------------------------
// Full occupancy
// ---------------------------------------------------------------------------

void test_full_occupancy() {
    printf("Testing full occupancy (100%%)...\n");

    TributeInput input;
    input.base_value = 100;
    input.current_occupancy = 100;
    input.capacity = 100;
    input.sector_value = 128;       // mid-range
    input.tribute_rate = 7;
    input.tribute_modifier = 1.0f;

    TributeResult r = calculate_building_tribute(input);

    assert(approx(r.occupancy_factor, 1.0f));
    assert(r.tribute_amount > 0);

    printf("  PASS: Full occupancy -> occupancy_factor = 1.0, tribute > 0\n");
}

// ---------------------------------------------------------------------------
// Half occupancy
// ---------------------------------------------------------------------------

void test_half_occupancy() {
    printf("Testing half occupancy (50%%)...\n");

    TributeInput input;
    input.base_value = 100;
    input.current_occupancy = 50;
    input.capacity = 100;
    input.sector_value = 128;
    input.tribute_rate = 7;
    input.tribute_modifier = 1.0f;

    TributeResult r = calculate_building_tribute(input);

    assert(approx(r.occupancy_factor, 0.5f));

    // Compare with full occupancy -- should be roughly half
    TributeInput full_input = input;
    full_input.current_occupancy = 100;
    TributeResult full_r = calculate_building_tribute(full_input);

    // Half occupancy should yield roughly half tribute
    // (integer truncation may cause slight difference)
    assert(r.tribute_amount <= full_r.tribute_amount);

    printf("  PASS: Half occupancy -> occupancy_factor = 0.5\n");
}

// ---------------------------------------------------------------------------
// Zero occupancy
// ---------------------------------------------------------------------------

void test_zero_occupancy() {
    printf("Testing zero occupancy...\n");

    TributeInput input;
    input.base_value = 200;
    input.current_occupancy = 0;
    input.capacity = 100;
    input.sector_value = 255;
    input.tribute_rate = 20;
    input.tribute_modifier = 2.0f;

    TributeResult r = calculate_building_tribute(input);

    assert(approx(r.occupancy_factor, 0.0f));
    assert(r.tribute_amount == 0);

    printf("  PASS: Zero occupancy -> tribute = 0\n");
}

// ---------------------------------------------------------------------------
// Zero capacity edge case
// ---------------------------------------------------------------------------

void test_zero_capacity() {
    printf("Testing zero capacity edge case...\n");

    TributeInput input;
    input.base_value = 100;
    input.current_occupancy = 50;   // occupants but no capacity
    input.capacity = 0;
    input.sector_value = 128;
    input.tribute_rate = 7;
    input.tribute_modifier = 1.0f;

    TributeResult r = calculate_building_tribute(input);

    assert(approx(r.occupancy_factor, 0.0f));
    assert(r.tribute_amount == 0);

    printf("  PASS: Zero capacity -> occupancy_factor = 0, tribute = 0\n");
}

// ---------------------------------------------------------------------------
// Sector value: low (0), mid (128), high (255)
// ---------------------------------------------------------------------------

void test_sector_value_low() {
    printf("Testing sector value = 0 (lowest)...\n");

    TributeInput input;
    input.base_value = 100;
    input.current_occupancy = 100;
    input.capacity = 100;
    input.sector_value = 0;
    input.tribute_rate = 10;
    input.tribute_modifier = 1.0f;

    TributeResult r = calculate_building_tribute(input);

    // value_factor = 0.5 + (0/255.0)*1.5 = 0.5
    assert(approx(r.value_factor, 0.5f));

    printf("  PASS: Sector value 0 -> value_factor = 0.5\n");
}

void test_sector_value_mid() {
    printf("Testing sector value = 128 (mid)...\n");

    TributeInput input;
    input.base_value = 100;
    input.current_occupancy = 100;
    input.capacity = 100;
    input.sector_value = 128;
    input.tribute_rate = 10;
    input.tribute_modifier = 1.0f;

    TributeResult r = calculate_building_tribute(input);

    // value_factor = 0.5 + (128/255.0)*1.5 ~= 0.5 + 0.7529 = 1.2529
    float expected = 0.5f + (128.0f / 255.0f) * 1.5f;
    assert(approx(r.value_factor, expected));

    printf("  PASS: Sector value 128 -> value_factor ~= %.4f\n", expected);
}

void test_sector_value_high() {
    printf("Testing sector value = 255 (highest)...\n");

    TributeInput input;
    input.base_value = 100;
    input.current_occupancy = 100;
    input.capacity = 100;
    input.sector_value = 255;
    input.tribute_rate = 10;
    input.tribute_modifier = 1.0f;

    TributeResult r = calculate_building_tribute(input);

    // value_factor = 0.5 + (255/255.0)*1.5 = 0.5 + 1.5 = 2.0
    assert(approx(r.value_factor, 2.0f));

    printf("  PASS: Sector value 255 -> value_factor = 2.0\n");
}

// ---------------------------------------------------------------------------
// Tribute rate: 0%, 7%, 20%
// ---------------------------------------------------------------------------

void test_tribute_rate_zero() {
    printf("Testing 0%% tribute rate...\n");

    TributeInput input;
    input.base_value = 100;
    input.current_occupancy = 100;
    input.capacity = 100;
    input.sector_value = 128;
    input.tribute_rate = 0;
    input.tribute_modifier = 1.0f;

    TributeResult r = calculate_building_tribute(input);

    assert(approx(r.rate_factor, 0.0f));
    assert(r.tribute_amount == 0);

    printf("  PASS: 0%% rate -> rate_factor = 0.0, tribute = 0\n");
}

void test_tribute_rate_seven() {
    printf("Testing 7%% tribute rate...\n");

    TributeInput input;
    input.base_value = 100;
    input.current_occupancy = 100;
    input.capacity = 100;
    input.sector_value = 128;
    input.tribute_rate = 7;
    input.tribute_modifier = 1.0f;

    TributeResult r = calculate_building_tribute(input);

    assert(approx(r.rate_factor, 0.07f));
    assert(r.tribute_amount > 0);

    printf("  PASS: 7%% rate -> rate_factor = 0.07\n");
}

void test_tribute_rate_twenty() {
    printf("Testing 20%% tribute rate...\n");

    TributeInput input;
    input.base_value = 100;
    input.current_occupancy = 100;
    input.capacity = 100;
    input.sector_value = 128;
    input.tribute_rate = 20;
    input.tribute_modifier = 1.0f;

    TributeResult r = calculate_building_tribute(input);

    assert(approx(r.rate_factor, 0.2f));
    assert(r.tribute_amount > 0);

    printf("  PASS: 20%% rate -> rate_factor = 0.2\n");
}

// ---------------------------------------------------------------------------
// Tribute modifier effects
// ---------------------------------------------------------------------------

void test_tribute_modifier_half() {
    printf("Testing tribute_modifier = 0.5...\n");

    TributeInput base_input;
    base_input.base_value = 100;
    base_input.current_occupancy = 100;
    base_input.capacity = 100;
    base_input.sector_value = 128;
    base_input.tribute_rate = 10;
    base_input.tribute_modifier = 1.0f;

    TributeInput half_input = base_input;
    half_input.tribute_modifier = 0.5f;

    TributeResult base_r = calculate_building_tribute(base_input);
    TributeResult half_r = calculate_building_tribute(half_input);

    // With int truncation, half modifier should yield roughly half
    // Allow +/- 1 for truncation
    assert(half_r.tribute_amount <= base_r.tribute_amount);
    assert(half_r.tribute_amount >= (base_r.tribute_amount / 2) - 1);
    assert(half_r.tribute_amount <= (base_r.tribute_amount / 2) + 1);

    printf("  PASS: 0.5x modifier yields ~half tribute\n");
}

void test_tribute_modifier_one() {
    printf("Testing tribute_modifier = 1.0 (default)...\n");

    TributeInput input;
    input.base_value = 100;
    input.current_occupancy = 100;
    input.capacity = 100;
    input.sector_value = 255;
    input.tribute_rate = 10;
    input.tribute_modifier = 1.0f;

    TributeResult r = calculate_building_tribute(input);

    // 100 * 1.0 * 2.0 * 0.1 * 1.0 = 20
    assert(r.tribute_amount == 20);

    printf("  PASS: 1.0x modifier, known inputs -> tribute = 20\n");
}

void test_tribute_modifier_double() {
    printf("Testing tribute_modifier = 2.0...\n");

    TributeInput base_input;
    base_input.base_value = 100;
    base_input.current_occupancy = 100;
    base_input.capacity = 100;
    base_input.sector_value = 255;
    base_input.tribute_rate = 10;
    base_input.tribute_modifier = 1.0f;

    TributeInput double_input = base_input;
    double_input.tribute_modifier = 2.0f;

    TributeResult base_r = calculate_building_tribute(base_input);
    TributeResult double_r = calculate_building_tribute(double_input);

    // 2x modifier should yield exactly 2x tribute
    assert(double_r.tribute_amount == base_r.tribute_amount * 2);

    printf("  PASS: 2.0x modifier yields 2x tribute\n");
}

// ---------------------------------------------------------------------------
// Known exact calculation
// ---------------------------------------------------------------------------

void test_exact_formula() {
    printf("Testing exact formula calculation...\n");

    TributeInput input;
    input.base_value = 200;
    input.current_occupancy = 80;
    input.capacity = 100;
    input.sector_value = 255;       // value_factor = 2.0
    input.tribute_rate = 10;        // rate_factor = 0.1
    input.tribute_modifier = 1.5f;

    TributeResult r = calculate_building_tribute(input);

    // occupancy_factor = 80/100 = 0.8
    // value_factor = 2.0
    // rate_factor = 0.1
    // amount = 200 * 0.8 * 2.0 * 0.1 * 1.5 = 48.0
    assert(approx(r.occupancy_factor, 0.8f));
    assert(approx(r.value_factor, 2.0f));
    assert(approx(r.rate_factor, 0.1f));
    assert(r.tribute_amount == 48);

    printf("  PASS: Exact calculation matches expected value (48)\n");
}

// ---------------------------------------------------------------------------
// Aggregate function
// ---------------------------------------------------------------------------

void test_aggregate_empty() {
    printf("Testing aggregate with empty input...\n");

    std::vector<std::pair<ZoneBuildingType, int64_t>> results;
    AggregateTributeResult agg = aggregate_tribute(results);

    assert(agg.habitation_total == 0);
    assert(agg.exchange_total == 0);
    assert(agg.fabrication_total == 0);
    assert(agg.grand_total == 0);
    assert(agg.buildings_counted == 0);

    printf("  PASS: Empty input -> all zeros\n");
}

void test_aggregate_single_zone() {
    printf("Testing aggregate with single zone type...\n");

    std::vector<std::pair<ZoneBuildingType, int64_t>> results;
    results.push_back({ZoneBuildingType::Habitation, 100});
    results.push_back({ZoneBuildingType::Habitation, 200});
    results.push_back({ZoneBuildingType::Habitation, 50});

    AggregateTributeResult agg = aggregate_tribute(results);

    assert(agg.habitation_total == 350);
    assert(agg.exchange_total == 0);
    assert(agg.fabrication_total == 0);
    assert(agg.grand_total == 350);
    assert(agg.buildings_counted == 3);

    printf("  PASS: Single zone type aggregated correctly\n");
}

void test_aggregate_mixed_zones() {
    printf("Testing aggregate with mixed zone types...\n");

    std::vector<std::pair<ZoneBuildingType, int64_t>> results;
    results.push_back({ZoneBuildingType::Habitation, 100});
    results.push_back({ZoneBuildingType::Exchange, 200});
    results.push_back({ZoneBuildingType::Fabrication, 150});
    results.push_back({ZoneBuildingType::Habitation, 50});
    results.push_back({ZoneBuildingType::Exchange, 300});

    AggregateTributeResult agg = aggregate_tribute(results);

    assert(agg.habitation_total == 150);    // 100 + 50
    assert(agg.exchange_total == 500);      // 200 + 300
    assert(agg.fabrication_total == 150);   // 150
    assert(agg.grand_total == 800);         // 150 + 500 + 150
    assert(agg.buildings_counted == 5);

    printf("  PASS: Mixed zone types aggregated correctly\n");
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main() {
    printf("=== TributeCalculation Unit Tests (E11-007) ===\n\n");

    // Base tribute values (6 tests)
    test_base_tribute_habitation_low();
    test_base_tribute_habitation_high();
    test_base_tribute_exchange_low();
    test_base_tribute_exchange_high();
    test_base_tribute_fabrication_low();
    test_base_tribute_fabrication_high();

    // Occupancy tests (3 tests)
    test_full_occupancy();
    test_half_occupancy();
    test_zero_occupancy();

    // Edge case (1 test)
    test_zero_capacity();

    // Sector value tests (3 tests)
    test_sector_value_low();
    test_sector_value_mid();
    test_sector_value_high();

    // Tribute rate tests (3 tests)
    test_tribute_rate_zero();
    test_tribute_rate_seven();
    test_tribute_rate_twenty();

    // Modifier tests (3 tests)
    test_tribute_modifier_half();
    test_tribute_modifier_one();
    test_tribute_modifier_double();

    // Exact formula (1 test)
    test_exact_formula();

    // Aggregate tests (3 tests)
    test_aggregate_empty();
    test_aggregate_single_zone();
    test_aggregate_mixed_zones();

    printf("\n=== All TributeCalculation tests passed! (20 tests) ===\n");
    return 0;
}
