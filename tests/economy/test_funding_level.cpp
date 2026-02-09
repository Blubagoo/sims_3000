/**
 * @file test_funding_level.cpp
 * @brief Unit tests for FundingLevel (E11-013)
 *
 * Tests: constants, clamping, get/set for each service type,
 *        effectiveness curve at key points, event struct, edge cases.
 */

#include "sims3000/economy/FundingLevel.h"
#include <cassert>
#include <cmath>
#include <cstdio>

using namespace sims3000::economy;

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

void test_constants() {
    printf("Testing funding level constants...\n");

    assert(constants::MIN_FUNDING_LEVEL == 0);
    assert(constants::MAX_FUNDING_LEVEL == 150);
    assert(constants::DEFAULT_FUNDING_LEVEL == 100);

    printf("  PASS: Constants have correct values\n");
}

// ---------------------------------------------------------------------------
// Clamping
// ---------------------------------------------------------------------------

void test_clamp_within_range() {
    printf("Testing clamp_funding_level within range...\n");

    assert(clamp_funding_level(0) == 0);
    assert(clamp_funding_level(50) == 50);
    assert(clamp_funding_level(100) == 100);
    assert(clamp_funding_level(150) == 150);

    printf("  PASS: Values in [0, 150] pass through unchanged\n");
}

void test_clamp_above_max() {
    printf("Testing clamp_funding_level above max...\n");

    assert(clamp_funding_level(151) == 150);
    assert(clamp_funding_level(200) == 150);
    assert(clamp_funding_level(255) == 150);

    printf("  PASS: Values above 150 are clamped to 150\n");
}

// ---------------------------------------------------------------------------
// Default values
// ---------------------------------------------------------------------------

void test_default_funding_levels() {
    printf("Testing default funding levels in TreasuryState...\n");

    TreasuryState ts;

    assert(get_funding_level(ts, 0) == 100); // Enforcer
    assert(get_funding_level(ts, 1) == 100); // HazardResponse
    assert(get_funding_level(ts, 2) == 100); // Medical
    assert(get_funding_level(ts, 3) == 100); // Education

    printf("  PASS: All service types default to 100%%\n");
}

// ---------------------------------------------------------------------------
// get_funding_level per service type
// ---------------------------------------------------------------------------

void test_get_funding_level_enforcer() {
    printf("Testing get_funding_level for Enforcer (0)...\n");

    TreasuryState ts;
    ts.funding_enforcer = 80;

    assert(get_funding_level(ts, 0) == 80);

    printf("  PASS: Reads funding_enforcer\n");
}

void test_get_funding_level_hazard_response() {
    printf("Testing get_funding_level for HazardResponse (1)...\n");

    TreasuryState ts;
    ts.funding_hazard_response = 120;

    assert(get_funding_level(ts, 1) == 120);

    printf("  PASS: Reads funding_hazard_response\n");
}

void test_get_funding_level_medical() {
    printf("Testing get_funding_level for Medical (2)...\n");

    TreasuryState ts;
    ts.funding_medical = 50;

    assert(get_funding_level(ts, 2) == 50);

    printf("  PASS: Reads funding_medical\n");
}

void test_get_funding_level_education() {
    printf("Testing get_funding_level for Education (3)...\n");

    TreasuryState ts;
    ts.funding_education = 140;

    assert(get_funding_level(ts, 3) == 140);

    printf("  PASS: Reads funding_education\n");
}

void test_get_funding_level_unknown_type() {
    printf("Testing get_funding_level for unknown service type...\n");

    TreasuryState ts;
    ts.funding_enforcer = 80;

    // Unknown service types return DEFAULT_FUNDING_LEVEL (100)
    assert(get_funding_level(ts, 4) == 100);
    assert(get_funding_level(ts, 255) == 100);

    printf("  PASS: Unknown types return default (100)\n");
}

// ---------------------------------------------------------------------------
// set_funding_level per service type
// ---------------------------------------------------------------------------

void test_set_funding_level_enforcer() {
    printf("Testing set_funding_level for Enforcer (0)...\n");

    TreasuryState ts;
    auto event = set_funding_level(ts, 0, 75, 1);

    assert(ts.funding_enforcer == 75);
    assert(event.player_id == 1);
    assert(event.service_type == 0);
    assert(event.old_level == 100);
    assert(event.new_level == 75);

    printf("  PASS: Sets enforcer funding and returns correct event\n");
}

void test_set_funding_level_hazard_response() {
    printf("Testing set_funding_level for HazardResponse (1)...\n");

    TreasuryState ts;
    auto event = set_funding_level(ts, 1, 130, 2);

    assert(ts.funding_hazard_response == 130);
    assert(event.player_id == 2);
    assert(event.service_type == 1);
    assert(event.old_level == 100);
    assert(event.new_level == 130);

    printf("  PASS: Sets hazard response funding and returns correct event\n");
}

void test_set_funding_level_medical() {
    printf("Testing set_funding_level for Medical (2)...\n");

    TreasuryState ts;
    auto event = set_funding_level(ts, 2, 0, 0);

    assert(ts.funding_medical == 0);
    assert(event.player_id == 0);
    assert(event.service_type == 2);
    assert(event.old_level == 100);
    assert(event.new_level == 0);

    printf("  PASS: Sets medical funding to 0 and returns correct event\n");
}

void test_set_funding_level_education() {
    printf("Testing set_funding_level for Education (3)...\n");

    TreasuryState ts;
    auto event = set_funding_level(ts, 3, 150, 3);

    assert(ts.funding_education == 150);
    assert(event.player_id == 3);
    assert(event.service_type == 3);
    assert(event.old_level == 100);
    assert(event.new_level == 150);

    printf("  PASS: Sets education funding to 150 and returns correct event\n");
}

// ---------------------------------------------------------------------------
// Edge cases
// ---------------------------------------------------------------------------

void test_set_funding_level_clamped() {
    printf("Testing set_funding_level clamps values above 150...\n");

    TreasuryState ts;
    auto event = set_funding_level(ts, 0, 200);

    assert(ts.funding_enforcer == 150);
    assert(event.old_level == 100);
    assert(event.new_level == 150);

    printf("  PASS: Level clamped to 150 when set to 200\n");
}

void test_set_funding_level_max_uint8() {
    printf("Testing set_funding_level with uint8_t max (255)...\n");

    TreasuryState ts;
    auto event = set_funding_level(ts, 1, 255);

    assert(ts.funding_hazard_response == 150);
    assert(event.new_level == 150);

    printf("  PASS: uint8_t max clamped to 150\n");
}

void test_set_funding_level_no_change() {
    printf("Testing set_funding_level when value unchanged...\n");

    TreasuryState ts;
    // Default is 100, set to 100 again
    auto event = set_funding_level(ts, 0, 100);

    assert(ts.funding_enforcer == 100);
    assert(event.old_level == 100);
    assert(event.new_level == 100);

    printf("  PASS: Event returned even when level unchanged\n");
}

// ---------------------------------------------------------------------------
// Effectiveness curve
// ---------------------------------------------------------------------------

void test_effectiveness_zero() {
    printf("Testing calculate_effectiveness at 0%%...\n");

    float eff = calculate_effectiveness(0);
    assert(std::fabs(eff - 0.0f) < 0.001f);

    printf("  PASS: 0%% -> 0.0\n");
}

void test_effectiveness_25() {
    printf("Testing calculate_effectiveness at 25%%...\n");

    float eff = calculate_effectiveness(25);
    assert(std::fabs(eff - 0.40f) < 0.001f);

    printf("  PASS: 25%% -> 0.40\n");
}

void test_effectiveness_50() {
    printf("Testing calculate_effectiveness at 50%%...\n");

    float eff = calculate_effectiveness(50);
    assert(std::fabs(eff - 0.65f) < 0.001f);

    printf("  PASS: 50%% -> 0.65\n");
}

void test_effectiveness_75() {
    printf("Testing calculate_effectiveness at 75%%...\n");

    float eff = calculate_effectiveness(75);
    assert(std::fabs(eff - 0.85f) < 0.001f);

    printf("  PASS: 75%% -> 0.85\n");
}

void test_effectiveness_100() {
    printf("Testing calculate_effectiveness at 100%%...\n");

    float eff = calculate_effectiveness(100);
    assert(std::fabs(eff - 1.0f) < 0.001f);

    printf("  PASS: 100%% -> 1.0\n");
}

void test_effectiveness_150() {
    printf("Testing calculate_effectiveness at 150%%...\n");

    float eff = calculate_effectiveness(150);
    assert(std::fabs(eff - 1.10f) < 0.001f);

    printf("  PASS: 150%% -> 1.10\n");
}

void test_effectiveness_above_150_clamped() {
    printf("Testing calculate_effectiveness above 150 (clamped)...\n");

    // Values above 150 are clamped to 150, so same as 150
    float eff = calculate_effectiveness(200);
    assert(std::fabs(eff - 1.10f) < 0.001f);

    float eff2 = calculate_effectiveness(255);
    assert(std::fabs(eff2 - 1.10f) < 0.001f);

    printf("  PASS: Values above 150 produce same result as 150 (1.10)\n");
}

void test_effectiveness_interpolation_midpoints() {
    printf("Testing calculate_effectiveness at midpoints...\n");

    // Midpoint between 0 and 25 -> (0.0 + 0.40) / 2 = 0.20
    // At level 12 (approx midpoint): 12 * 0.016 = 0.192
    // Actually at level 12.5 which we can't test. Use 13:
    // 13 * 0.016 = 0.208
    float eff12 = calculate_effectiveness(12);
    // 12 * (0.40/25) = 12 * 0.016 = 0.192
    assert(std::fabs(eff12 - 0.192f) < 0.001f);

    // Midpoint between 100 and 150 -> level 125
    // 1.0 + 25 * (0.10/50) = 1.0 + 0.05 = 1.05
    float eff125 = calculate_effectiveness(125);
    assert(std::fabs(eff125 - 1.05f) < 0.001f);

    printf("  PASS: Midpoint interpolation values correct\n");
}

void test_effectiveness_monotonic() {
    printf("Testing calculate_effectiveness is monotonically increasing...\n");

    float prev = calculate_effectiveness(0);
    for (uint8_t i = 1; i <= 150; ++i) {
        float curr = calculate_effectiveness(i);
        assert(curr >= prev);
        prev = curr;
    }

    printf("  PASS: Effectiveness is monotonically increasing from 0 to 150\n");
}

// ---------------------------------------------------------------------------
// FundingLevelChangedEvent struct
// ---------------------------------------------------------------------------

void test_event_struct_fields() {
    printf("Testing FundingLevelChangedEvent struct...\n");

    FundingLevelChangedEvent event;
    event.player_id = 3;
    event.service_type = 2;
    event.old_level = 100;
    event.new_level = 75;

    assert(event.player_id == 3);
    assert(event.service_type == 2);
    assert(event.old_level == 100);
    assert(event.new_level == 75);

    printf("  PASS: Event struct fields store and retrieve correctly\n");
}

// ---------------------------------------------------------------------------
// Integration: set then get
// ---------------------------------------------------------------------------

void test_set_then_get_roundtrip() {
    printf("Testing set then get roundtrip for all service types...\n");

    TreasuryState ts;

    set_funding_level(ts, 0, 80);   // Enforcer
    set_funding_level(ts, 1, 120);  // HazardResponse
    set_funding_level(ts, 2, 50);   // Medical
    set_funding_level(ts, 3, 140);  // Education

    assert(get_funding_level(ts, 0) == 80);
    assert(get_funding_level(ts, 1) == 120);
    assert(get_funding_level(ts, 2) == 50);
    assert(get_funding_level(ts, 3) == 140);

    printf("  PASS: set then get returns correct values for all services\n");
}

void test_multiple_sets_same_service() {
    printf("Testing multiple consecutive sets on same service...\n");

    TreasuryState ts;

    set_funding_level(ts, 0, 60);
    auto event = set_funding_level(ts, 0, 130);

    assert(ts.funding_enforcer == 130);
    assert(event.old_level == 60);
    assert(event.new_level == 130);

    printf("  PASS: Second set captures previous set's value as old_level\n");
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main() {
    printf("=== FundingLevel Unit Tests (E11-013) ===\n\n");

    // Constants
    test_constants();

    // Clamping
    test_clamp_within_range();
    test_clamp_above_max();

    // Default values
    test_default_funding_levels();

    // get_funding_level per service type
    test_get_funding_level_enforcer();
    test_get_funding_level_hazard_response();
    test_get_funding_level_medical();
    test_get_funding_level_education();
    test_get_funding_level_unknown_type();

    // set_funding_level per service type
    test_set_funding_level_enforcer();
    test_set_funding_level_hazard_response();
    test_set_funding_level_medical();
    test_set_funding_level_education();

    // Edge cases
    test_set_funding_level_clamped();
    test_set_funding_level_max_uint8();
    test_set_funding_level_no_change();

    // Effectiveness curve
    test_effectiveness_zero();
    test_effectiveness_25();
    test_effectiveness_50();
    test_effectiveness_75();
    test_effectiveness_100();
    test_effectiveness_150();
    test_effectiveness_above_150_clamped();
    test_effectiveness_interpolation_midpoints();
    test_effectiveness_monotonic();

    // Event struct
    test_event_struct_fields();

    // Integration
    test_set_then_get_roundtrip();
    test_multiple_sets_same_service();

    printf("\n=== All FundingLevel tests passed! ===\n");
    return 0;
}
