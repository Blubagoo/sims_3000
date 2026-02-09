/**
 * @file test_tribute_rate_config.cpp
 * @brief Unit tests for TributeRateConfig (E11-006)
 *
 * Tests: clamping, get/set for each zone type, average calculation,
 *        default values, edge cases (0%, 20%, >20%), event struct.
 */

#include "sims3000/economy/TributeRateConfig.h"
#include <cassert>
#include <cmath>
#include <cstdio>

using namespace sims3000::economy;

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

void test_constants() {
    printf("Testing tribute rate constants...\n");

    assert(constants::MIN_TRIBUTE_RATE == 0);
    assert(constants::MAX_TRIBUTE_RATE == 20);
    assert(constants::DEFAULT_TRIBUTE_RATE == 7);

    printf("  PASS: Constants have correct values\n");
}

// ---------------------------------------------------------------------------
// Clamping
// ---------------------------------------------------------------------------

void test_clamp_within_range() {
    printf("Testing clamp_tribute_rate within range...\n");

    assert(clamp_tribute_rate(0) == 0);
    assert(clamp_tribute_rate(7) == 7);
    assert(clamp_tribute_rate(10) == 10);
    assert(clamp_tribute_rate(20) == 20);

    printf("  PASS: Values in [0, 20] pass through unchanged\n");
}

void test_clamp_above_max() {
    printf("Testing clamp_tribute_rate above max...\n");

    assert(clamp_tribute_rate(21) == 20);
    assert(clamp_tribute_rate(50) == 20);
    assert(clamp_tribute_rate(100) == 20);
    assert(clamp_tribute_rate(255) == 20);

    printf("  PASS: Values above 20 are clamped to 20\n");
}

// ---------------------------------------------------------------------------
// Default values
// ---------------------------------------------------------------------------

void test_default_tribute_rates() {
    printf("Testing default tribute rates in TreasuryState...\n");

    TreasuryState ts;

    assert(get_tribute_rate(ts, ZoneBuildingType::Habitation) == 7);
    assert(get_tribute_rate(ts, ZoneBuildingType::Exchange) == 7);
    assert(get_tribute_rate(ts, ZoneBuildingType::Fabrication) == 7);

    printf("  PASS: All zone types default to 7%%\n");
}

// ---------------------------------------------------------------------------
// get_tribute_rate
// ---------------------------------------------------------------------------

void test_get_tribute_rate_habitation() {
    printf("Testing get_tribute_rate for Habitation...\n");

    TreasuryState ts;
    ts.tribute_rate_habitation = 12;

    assert(get_tribute_rate(ts, ZoneBuildingType::Habitation) == 12);

    printf("  PASS: Reads tribute_rate_habitation\n");
}

void test_get_tribute_rate_exchange() {
    printf("Testing get_tribute_rate for Exchange...\n");

    TreasuryState ts;
    ts.tribute_rate_exchange = 15;

    assert(get_tribute_rate(ts, ZoneBuildingType::Exchange) == 15);

    printf("  PASS: Reads tribute_rate_exchange\n");
}

void test_get_tribute_rate_fabrication() {
    printf("Testing get_tribute_rate for Fabrication...\n");

    TreasuryState ts;
    ts.tribute_rate_fabrication = 3;

    assert(get_tribute_rate(ts, ZoneBuildingType::Fabrication) == 3);

    printf("  PASS: Reads tribute_rate_fabrication\n");
}

// ---------------------------------------------------------------------------
// set_tribute_rate
// ---------------------------------------------------------------------------

void test_set_tribute_rate_habitation() {
    printf("Testing set_tribute_rate for Habitation...\n");

    TreasuryState ts;
    auto event = set_tribute_rate(ts, ZoneBuildingType::Habitation, 10, 1);

    assert(ts.tribute_rate_habitation == 10);
    assert(event.player_id == 1);
    assert(event.zone_type == ZoneBuildingType::Habitation);
    assert(event.old_rate == 7);
    assert(event.new_rate == 10);

    printf("  PASS: Sets habitation rate and returns correct event\n");
}

void test_set_tribute_rate_exchange() {
    printf("Testing set_tribute_rate for Exchange...\n");

    TreasuryState ts;
    auto event = set_tribute_rate(ts, ZoneBuildingType::Exchange, 18, 2);

    assert(ts.tribute_rate_exchange == 18);
    assert(event.player_id == 2);
    assert(event.zone_type == ZoneBuildingType::Exchange);
    assert(event.old_rate == 7);
    assert(event.new_rate == 18);

    printf("  PASS: Sets exchange rate and returns correct event\n");
}

void test_set_tribute_rate_fabrication() {
    printf("Testing set_tribute_rate for Fabrication...\n");

    TreasuryState ts;
    auto event = set_tribute_rate(ts, ZoneBuildingType::Fabrication, 5, 0);

    assert(ts.tribute_rate_fabrication == 5);
    assert(event.player_id == 0);
    assert(event.zone_type == ZoneBuildingType::Fabrication);
    assert(event.old_rate == 7);
    assert(event.new_rate == 5);

    printf("  PASS: Sets fabrication rate and returns correct event\n");
}

void test_set_tribute_rate_clamped() {
    printf("Testing set_tribute_rate clamps values above 20...\n");

    TreasuryState ts;
    auto event = set_tribute_rate(ts, ZoneBuildingType::Habitation, 50);

    assert(ts.tribute_rate_habitation == 20);
    assert(event.old_rate == 7);
    assert(event.new_rate == 20);

    printf("  PASS: Rate clamped to 20 when set to 50\n");
}

void test_set_tribute_rate_zero() {
    printf("Testing set_tribute_rate to 0...\n");

    TreasuryState ts;
    auto event = set_tribute_rate(ts, ZoneBuildingType::Exchange, 0);

    assert(ts.tribute_rate_exchange == 0);
    assert(event.old_rate == 7);
    assert(event.new_rate == 0);

    printf("  PASS: Rate set to 0 successfully\n");
}

void test_set_tribute_rate_max() {
    printf("Testing set_tribute_rate to 20...\n");

    TreasuryState ts;
    auto event = set_tribute_rate(ts, ZoneBuildingType::Fabrication, 20);

    assert(ts.tribute_rate_fabrication == 20);
    assert(event.old_rate == 7);
    assert(event.new_rate == 20);

    printf("  PASS: Rate set to 20 successfully\n");
}

void test_set_tribute_rate_no_change() {
    printf("Testing set_tribute_rate when value unchanged...\n");

    TreasuryState ts;
    // Default is 7, set to 7 again
    auto event = set_tribute_rate(ts, ZoneBuildingType::Habitation, 7);

    assert(ts.tribute_rate_habitation == 7);
    assert(event.old_rate == 7);
    assert(event.new_rate == 7);

    printf("  PASS: Event returned even when rate unchanged\n");
}

void test_set_tribute_rate_max_uint8() {
    printf("Testing set_tribute_rate with uint8_t max (255)...\n");

    TreasuryState ts;
    auto event = set_tribute_rate(ts, ZoneBuildingType::Habitation, 255);

    assert(ts.tribute_rate_habitation == 20);
    assert(event.new_rate == 20);

    printf("  PASS: uint8_t max clamped to 20\n");
}

// ---------------------------------------------------------------------------
// get_average_tribute_rate
// ---------------------------------------------------------------------------

void test_average_default() {
    printf("Testing get_average_tribute_rate with defaults...\n");

    TreasuryState ts;
    float avg = get_average_tribute_rate(ts);

    // (7 + 7 + 7) / 3 = 7.0
    assert(std::fabs(avg - 7.0f) < 0.001f);

    printf("  PASS: Average of defaults is 7.0\n");
}

void test_average_mixed_rates() {
    printf("Testing get_average_tribute_rate with mixed rates...\n");

    TreasuryState ts;
    ts.tribute_rate_habitation = 6;
    ts.tribute_rate_exchange = 9;
    ts.tribute_rate_fabrication = 12;

    float avg = get_average_tribute_rate(ts);

    // (6 + 9 + 12) / 3 = 9.0
    assert(std::fabs(avg - 9.0f) < 0.001f);

    printf("  PASS: Average of 6, 9, 12 is 9.0\n");
}

void test_average_all_zero() {
    printf("Testing get_average_tribute_rate with all zeros...\n");

    TreasuryState ts;
    ts.tribute_rate_habitation = 0;
    ts.tribute_rate_exchange = 0;
    ts.tribute_rate_fabrication = 0;

    float avg = get_average_tribute_rate(ts);

    assert(std::fabs(avg - 0.0f) < 0.001f);

    printf("  PASS: Average of 0, 0, 0 is 0.0\n");
}

void test_average_all_max() {
    printf("Testing get_average_tribute_rate with all maxed...\n");

    TreasuryState ts;
    ts.tribute_rate_habitation = 20;
    ts.tribute_rate_exchange = 20;
    ts.tribute_rate_fabrication = 20;

    float avg = get_average_tribute_rate(ts);

    // (20 + 20 + 20) / 3 = 20.0
    assert(std::fabs(avg - 20.0f) < 0.001f);

    printf("  PASS: Average of 20, 20, 20 is 20.0\n");
}

void test_average_non_integer_result() {
    printf("Testing get_average_tribute_rate with non-integer result...\n");

    TreasuryState ts;
    ts.tribute_rate_habitation = 5;
    ts.tribute_rate_exchange = 6;
    ts.tribute_rate_fabrication = 7;

    float avg = get_average_tribute_rate(ts);

    // (5 + 6 + 7) / 3 = 6.0
    assert(std::fabs(avg - 6.0f) < 0.001f);

    // Try a truly non-integer case
    ts.tribute_rate_habitation = 1;
    ts.tribute_rate_exchange = 1;
    ts.tribute_rate_fabrication = 0;

    avg = get_average_tribute_rate(ts);

    // (1 + 1 + 0) / 3 = 0.6667
    assert(std::fabs(avg - (2.0f / 3.0f)) < 0.001f);

    printf("  PASS: Non-integer average computed correctly\n");
}

// ---------------------------------------------------------------------------
// TributeRateChangedEvent struct
// ---------------------------------------------------------------------------

void test_event_struct_fields() {
    printf("Testing TributeRateChangedEvent struct...\n");

    TributeRateChangedEvent event;
    event.player_id = 3;
    event.zone_type = ZoneBuildingType::Exchange;
    event.old_rate = 7;
    event.new_rate = 15;

    assert(event.player_id == 3);
    assert(event.zone_type == ZoneBuildingType::Exchange);
    assert(event.old_rate == 7);
    assert(event.new_rate == 15);

    printf("  PASS: Event struct fields store and retrieve correctly\n");
}

// ---------------------------------------------------------------------------
// Integration: set then get
// ---------------------------------------------------------------------------

void test_set_then_get_roundtrip() {
    printf("Testing set then get roundtrip for all zone types...\n");

    TreasuryState ts;

    set_tribute_rate(ts, ZoneBuildingType::Habitation, 4);
    set_tribute_rate(ts, ZoneBuildingType::Exchange, 11);
    set_tribute_rate(ts, ZoneBuildingType::Fabrication, 19);

    assert(get_tribute_rate(ts, ZoneBuildingType::Habitation) == 4);
    assert(get_tribute_rate(ts, ZoneBuildingType::Exchange) == 11);
    assert(get_tribute_rate(ts, ZoneBuildingType::Fabrication) == 19);

    printf("  PASS: set then get returns correct values for all zones\n");
}

void test_multiple_sets_same_zone() {
    printf("Testing multiple consecutive sets on same zone...\n");

    TreasuryState ts;

    set_tribute_rate(ts, ZoneBuildingType::Habitation, 3);
    auto event = set_tribute_rate(ts, ZoneBuildingType::Habitation, 15);

    assert(ts.tribute_rate_habitation == 15);
    assert(event.old_rate == 3);
    assert(event.new_rate == 15);

    printf("  PASS: Second set captures previous set's value as old_rate\n");
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main() {
    printf("=== TributeRateConfig Unit Tests (E11-006) ===\n\n");

    // Constants
    test_constants();

    // Clamping
    test_clamp_within_range();
    test_clamp_above_max();

    // Default values
    test_default_tribute_rates();

    // get_tribute_rate per zone
    test_get_tribute_rate_habitation();
    test_get_tribute_rate_exchange();
    test_get_tribute_rate_fabrication();

    // set_tribute_rate per zone
    test_set_tribute_rate_habitation();
    test_set_tribute_rate_exchange();
    test_set_tribute_rate_fabrication();

    // Edge cases
    test_set_tribute_rate_clamped();
    test_set_tribute_rate_zero();
    test_set_tribute_rate_max();
    test_set_tribute_rate_no_change();
    test_set_tribute_rate_max_uint8();

    // Average calculation
    test_average_default();
    test_average_mixed_rates();
    test_average_all_zero();
    test_average_all_max();
    test_average_non_integer_result();

    // Event struct
    test_event_struct_fields();

    // Integration
    test_set_then_get_roundtrip();
    test_multiple_sets_same_zone();

    printf("\n=== All TributeRateConfig tests passed! ===\n");
    return 0;
}
