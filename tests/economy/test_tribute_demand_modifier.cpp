/**
 * @file test_tribute_demand_modifier.cpp
 * @brief Unit tests for TributeDemandModifier (E11-019)
 *
 * Tests: tiered formula at key rates, boundary values,
 *        zone type convenience function, edge cases.
 */

#include "sims3000/economy/TributeDemandModifier.h"
#include <cassert>
#include <cstdio>

using namespace sims3000::economy;

// ---------------------------------------------------------------------------
// Tier 1: Rate 0-3% -> +15 bonus
// ---------------------------------------------------------------------------

void test_rate_0() {
    printf("Testing calculate_tribute_demand_modifier at rate 0...\n");

    assert(calculate_tribute_demand_modifier(0) == 15);

    printf("  PASS: Rate 0 -> +15\n");
}

void test_rate_1() {
    printf("Testing calculate_tribute_demand_modifier at rate 1...\n");

    assert(calculate_tribute_demand_modifier(1) == 15);

    printf("  PASS: Rate 1 -> +15\n");
}

void test_rate_2() {
    printf("Testing calculate_tribute_demand_modifier at rate 2...\n");

    assert(calculate_tribute_demand_modifier(2) == 15);

    printf("  PASS: Rate 2 -> +15\n");
}

void test_rate_3() {
    printf("Testing calculate_tribute_demand_modifier at rate 3...\n");

    assert(calculate_tribute_demand_modifier(3) == 15);

    printf("  PASS: Rate 3 -> +15\n");
}

// ---------------------------------------------------------------------------
// Tier 2: Rate 4-7% -> neutral (0)
// ---------------------------------------------------------------------------

void test_rate_4() {
    printf("Testing calculate_tribute_demand_modifier at rate 4...\n");

    assert(calculate_tribute_demand_modifier(4) == 0);

    printf("  PASS: Rate 4 -> 0\n");
}

void test_rate_5() {
    printf("Testing calculate_tribute_demand_modifier at rate 5...\n");

    assert(calculate_tribute_demand_modifier(5) == 0);

    printf("  PASS: Rate 5 -> 0\n");
}

void test_rate_7() {
    printf("Testing calculate_tribute_demand_modifier at rate 7...\n");

    assert(calculate_tribute_demand_modifier(7) == 0);

    printf("  PASS: Rate 7 -> 0 (neutral)\n");
}

// ---------------------------------------------------------------------------
// Tier 3: Rate 8-12% -> -4 per % above 7
// ---------------------------------------------------------------------------

void test_rate_8() {
    printf("Testing calculate_tribute_demand_modifier at rate 8...\n");

    // -4 * (8 - 7) = -4
    assert(calculate_tribute_demand_modifier(8) == -4);

    printf("  PASS: Rate 8 -> -4\n");
}

void test_rate_10() {
    printf("Testing calculate_tribute_demand_modifier at rate 10...\n");

    // -4 * (10 - 7) = -12
    assert(calculate_tribute_demand_modifier(10) == -12);

    printf("  PASS: Rate 10 -> -12\n");
}

void test_rate_12() {
    printf("Testing calculate_tribute_demand_modifier at rate 12...\n");

    // -4 * (12 - 7) = -20
    assert(calculate_tribute_demand_modifier(12) == -20);

    printf("  PASS: Rate 12 -> -20\n");
}

// ---------------------------------------------------------------------------
// Tier 4: Rate 13-16% -> -20 base - 5 per % above 12
// ---------------------------------------------------------------------------

void test_rate_13() {
    printf("Testing calculate_tribute_demand_modifier at rate 13...\n");

    // -20 - 5 * (13 - 12) = -25
    assert(calculate_tribute_demand_modifier(13) == -25);

    printf("  PASS: Rate 13 -> -25\n");
}

void test_rate_14() {
    printf("Testing calculate_tribute_demand_modifier at rate 14...\n");

    // -20 - 5 * (14 - 12) = -30
    assert(calculate_tribute_demand_modifier(14) == -30);

    printf("  PASS: Rate 14 -> -30\n");
}

void test_rate_16() {
    printf("Testing calculate_tribute_demand_modifier at rate 16...\n");

    // -20 - 5 * (16 - 12) = -40
    assert(calculate_tribute_demand_modifier(16) == -40);

    printf("  PASS: Rate 16 -> -40\n");
}

// ---------------------------------------------------------------------------
// Tier 5: Rate 17-20% -> -40 base - 5 per % above 16
// ---------------------------------------------------------------------------

void test_rate_17() {
    printf("Testing calculate_tribute_demand_modifier at rate 17...\n");

    // -40 - 5 * (17 - 16) = -45
    assert(calculate_tribute_demand_modifier(17) == -45);

    printf("  PASS: Rate 17 -> -45\n");
}

void test_rate_18() {
    printf("Testing calculate_tribute_demand_modifier at rate 18...\n");

    // -40 - 5 * (18 - 16) = -50
    assert(calculate_tribute_demand_modifier(18) == -50);

    printf("  PASS: Rate 18 -> -50\n");
}

void test_rate_20() {
    printf("Testing calculate_tribute_demand_modifier at rate 20...\n");

    // -40 - 5 * (20 - 16) = -60
    assert(calculate_tribute_demand_modifier(20) == -60);

    printf("  PASS: Rate 20 -> -60\n");
}

// ---------------------------------------------------------------------------
// Zone type convenience function
// ---------------------------------------------------------------------------

void test_zone_habitation() {
    printf("Testing get_zone_tribute_modifier for habitation...\n");

    TreasuryState ts;
    ts.tribute_rate_habitation = 0;

    assert(get_zone_tribute_modifier(ts, 0) == 15);

    printf("  PASS: Habitation zone uses tribute_rate_habitation\n");
}

void test_zone_exchange() {
    printf("Testing get_zone_tribute_modifier for exchange...\n");

    TreasuryState ts;
    ts.tribute_rate_exchange = 10;

    // -4 * (10 - 7) = -12
    assert(get_zone_tribute_modifier(ts, 1) == -12);

    printf("  PASS: Exchange zone uses tribute_rate_exchange\n");
}

void test_zone_fabrication() {
    printf("Testing get_zone_tribute_modifier for fabrication...\n");

    TreasuryState ts;
    ts.tribute_rate_fabrication = 20;

    // -40 - 5 * (20 - 16) = -60
    assert(get_zone_tribute_modifier(ts, 2) == -60);

    printf("  PASS: Fabrication zone uses tribute_rate_fabrication\n");
}

void test_zone_unknown() {
    printf("Testing get_zone_tribute_modifier for unknown zone type...\n");

    TreasuryState ts;
    assert(get_zone_tribute_modifier(ts, 3) == 0);
    assert(get_zone_tribute_modifier(ts, 255) == 0);

    printf("  PASS: Unknown zone types return 0\n");
}

// ---------------------------------------------------------------------------
// Each zone type with default rate (7%)
// ---------------------------------------------------------------------------

void test_default_rates_neutral() {
    printf("Testing all zones with default rate (7%%) are neutral...\n");

    TreasuryState ts; // defaults: all 7%

    assert(get_zone_tribute_modifier(ts, 0) == 0);
    assert(get_zone_tribute_modifier(ts, 1) == 0);
    assert(get_zone_tribute_modifier(ts, 2) == 0);

    printf("  PASS: Default 7%% gives 0 modifier for all zones\n");
}

// ---------------------------------------------------------------------------
// Boundary transitions
// ---------------------------------------------------------------------------

void test_boundary_tier1_to_tier2() {
    printf("Testing boundary between tier 1 and tier 2 (3 -> 4)...\n");

    assert(calculate_tribute_demand_modifier(3) == 15);  // tier 1
    assert(calculate_tribute_demand_modifier(4) == 0);   // tier 2

    printf("  PASS: Clean transition from +15 to 0 at rate 3->4\n");
}

void test_boundary_tier2_to_tier3() {
    printf("Testing boundary between tier 2 and tier 3 (7 -> 8)...\n");

    assert(calculate_tribute_demand_modifier(7) == 0);   // tier 2
    assert(calculate_tribute_demand_modifier(8) == -4);  // tier 3

    printf("  PASS: Clean transition from 0 to -4 at rate 7->8\n");
}

void test_boundary_tier3_to_tier4() {
    printf("Testing boundary between tier 3 and tier 4 (12 -> 13)...\n");

    assert(calculate_tribute_demand_modifier(12) == -20); // tier 3
    assert(calculate_tribute_demand_modifier(13) == -25); // tier 4

    printf("  PASS: Clean transition from -20 to -25 at rate 12->13\n");
}

void test_boundary_tier4_to_tier5() {
    printf("Testing boundary between tier 4 and tier 5 (16 -> 17)...\n");

    assert(calculate_tribute_demand_modifier(16) == -40); // tier 4
    assert(calculate_tribute_demand_modifier(17) == -45); // tier 5

    printf("  PASS: Clean transition from -40 to -45 at rate 16->17\n");
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main() {
    printf("=== TributeDemandModifier Unit Tests (E11-019) ===\n\n");

    // Tier 1: 0-3% -> +15
    test_rate_0();
    test_rate_1();
    test_rate_2();
    test_rate_3();

    // Tier 2: 4-7% -> 0
    test_rate_4();
    test_rate_5();
    test_rate_7();

    // Tier 3: 8-12% -> -4 per above 7
    test_rate_8();
    test_rate_10();
    test_rate_12();

    // Tier 4: 13-16% -> -20 base -5 per above 12
    test_rate_13();
    test_rate_14();
    test_rate_16();

    // Tier 5: 17-20% -> -40 base -5 per above 16
    test_rate_17();
    test_rate_18();
    test_rate_20();

    // Zone type convenience
    test_zone_habitation();
    test_zone_exchange();
    test_zone_fabrication();
    test_zone_unknown();

    // Default rates
    test_default_rates_neutral();

    // Boundary transitions
    test_boundary_tier1_to_tier2();
    test_boundary_tier2_to_tier3();
    test_boundary_tier3_to_tier4();
    test_boundary_tier4_to_tier5();

    printf("\n=== All TributeDemandModifier tests passed! ===\n");
    return 0;
}
