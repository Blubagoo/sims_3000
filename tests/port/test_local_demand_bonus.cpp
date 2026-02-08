/**
 * @file test_local_demand_bonus.cpp
 * @brief Unit tests for local (radius-based) demand bonus calculation (Epic 8, Ticket E8-017)
 *
 * Tests cover:
 * - Aero ports boost Habitation demand within 20-tile Manhattan radius (+5%)
 * - Aqua ports boost Exchange demand within 25-tile Manhattan radius (+10%)
 * - Manhattan distance calculation correctness
 * - Non-operational ports do not contribute
 * - Owner filtering
 * - Multiple port stacking
 * - Combined (global + local) bonus capped at +30
 * - Edge cases: no ports, out of range, exactly on boundary
 */

#include <sims3000/port/DemandBonus.h>
#include <sims3000/port/PortTypes.h>
#include <cassert>
#include <cstdio>
#include <cmath>
#include <vector>

using namespace sims3000::port;

// =============================================================================
// Helper: float comparison
// =============================================================================

static bool approx_eq(float a, float b, float eps = 0.01f) {
    return std::fabs(a - b) < eps;
}

// =============================================================================
// Aero Port -> Habitation Local Bonus Tests
// =============================================================================

void test_aero_boosts_habitation_within_radius() {
    printf("Testing aero port boosts Habitation within 20-tile radius...\n");

    std::vector<PortData> ports;
    ports.push_back({PortType::Aero, 600, true, 1, 50, 50});

    // Query at distance 10 (within radius 20)
    float bonus = calculate_local_demand_bonus(0, 55, 55, 1, ports);  // zone_type=0 (Habitation)
    assert(approx_eq(bonus, 5.0f));

    printf("  PASS: Aero port at (50,50), query at (55,55) -> bonus = %.1f\n", bonus);
}

void test_aero_no_bonus_outside_radius() {
    printf("Testing aero port no bonus outside 20-tile radius...\n");

    std::vector<PortData> ports;
    ports.push_back({PortType::Aero, 600, true, 1, 50, 50});

    // Query at Manhattan distance 21 (just outside radius 20)
    float bonus = calculate_local_demand_bonus(0, 60, 61, 1, ports);  // dist = 10+11=21
    assert(approx_eq(bonus, 0.0f));

    printf("  PASS: Aero port at (50,50), query at (60,61) dist=21 -> bonus = %.1f\n", bonus);
}

void test_aero_bonus_at_exact_radius() {
    printf("Testing aero port bonus at exactly 20-tile Manhattan distance...\n");

    std::vector<PortData> ports;
    ports.push_back({PortType::Aero, 600, true, 1, 50, 50});

    // Query at Manhattan distance exactly 20
    float bonus = calculate_local_demand_bonus(0, 60, 60, 1, ports);  // dist = 10+10=20
    assert(approx_eq(bonus, 5.0f));

    printf("  PASS: Aero port at (50,50), query at (60,60) dist=20 -> bonus = %.1f\n", bonus);
}

void test_aero_no_bonus_at_21() {
    printf("Testing aero port no bonus at distance 21...\n");

    std::vector<PortData> ports;
    ports.push_back({PortType::Aero, 600, true, 1, 0, 0});

    // Query at Manhattan distance exactly 21
    float bonus = calculate_local_demand_bonus(0, 21, 0, 1, ports);  // dist = 21
    assert(approx_eq(bonus, 0.0f));

    printf("  PASS: Aero port at (0,0), query at (21,0) dist=21 -> bonus = %.1f\n", bonus);
}

void test_aero_same_position() {
    printf("Testing aero port bonus at same position (distance 0)...\n");

    std::vector<PortData> ports;
    ports.push_back({PortType::Aero, 600, true, 1, 50, 50});

    float bonus = calculate_local_demand_bonus(0, 50, 50, 1, ports);  // dist = 0
    assert(approx_eq(bonus, 5.0f));

    printf("  PASS: Same position -> bonus = %.1f\n", bonus);
}

// =============================================================================
// Aqua Port -> Exchange Local Bonus Tests
// =============================================================================

void test_aqua_boosts_exchange_within_radius() {
    printf("Testing aqua port boosts Exchange within 25-tile radius...\n");

    std::vector<PortData> ports;
    ports.push_back({PortType::Aqua, 800, true, 1, 30, 30});

    // Query at distance 15 (within radius 25)
    float bonus = calculate_local_demand_bonus(1, 40, 35, 1, ports);  // zone_type=1 (Exchange), dist=10+5=15
    assert(approx_eq(bonus, 10.0f));

    printf("  PASS: Aqua port at (30,30), query at (40,35) -> bonus = %.1f\n", bonus);
}

void test_aqua_no_bonus_outside_radius() {
    printf("Testing aqua port no bonus outside 25-tile radius...\n");

    std::vector<PortData> ports;
    ports.push_back({PortType::Aqua, 800, true, 1, 30, 30});

    // Query at Manhattan distance 26
    float bonus = calculate_local_demand_bonus(1, 43, 43, 1, ports);  // dist = 13+13=26
    assert(approx_eq(bonus, 0.0f));

    printf("  PASS: Aqua port at (30,30), query at (43,43) dist=26 -> bonus = %.1f\n", bonus);
}

void test_aqua_bonus_at_exact_radius() {
    printf("Testing aqua port bonus at exactly 25-tile Manhattan distance...\n");

    std::vector<PortData> ports;
    ports.push_back({PortType::Aqua, 800, true, 1, 0, 0});

    // Query at Manhattan distance exactly 25
    float bonus = calculate_local_demand_bonus(1, 25, 0, 1, ports);  // dist = 25
    assert(approx_eq(bonus, 10.0f));

    printf("  PASS: Aqua port at (0,0), query at (25,0) dist=25 -> bonus = %.1f\n", bonus);
}

void test_aqua_no_bonus_at_26() {
    printf("Testing aqua port no bonus at distance 26...\n");

    std::vector<PortData> ports;
    ports.push_back({PortType::Aqua, 800, true, 1, 0, 0});

    // Query at Manhattan distance exactly 26
    float bonus = calculate_local_demand_bonus(1, 13, 13, 1, ports);  // dist = 26
    assert(approx_eq(bonus, 0.0f));

    printf("  PASS: Aqua port at (0,0), query at (13,13) dist=26 -> bonus = %.1f\n", bonus);
}

// =============================================================================
// Cross-type: Aero does NOT locally boost Exchange, Aqua does NOT locally boost Habitation
// =============================================================================

void test_aero_no_local_bonus_for_exchange() {
    printf("Testing aero port does NOT provide local bonus to Exchange...\n");

    std::vector<PortData> ports;
    ports.push_back({PortType::Aero, 600, true, 1, 50, 50});

    float bonus = calculate_local_demand_bonus(1, 50, 50, 1, ports);  // Exchange at same position
    assert(approx_eq(bonus, 0.0f));

    printf("  PASS: Aero port -> Exchange local bonus = %.1f\n", bonus);
}

void test_aqua_no_local_bonus_for_habitation() {
    printf("Testing aqua port does NOT provide local bonus to Habitation...\n");

    std::vector<PortData> ports;
    ports.push_back({PortType::Aqua, 800, true, 1, 50, 50});

    float bonus = calculate_local_demand_bonus(0, 50, 50, 1, ports);  // Habitation at same position
    assert(approx_eq(bonus, 0.0f));

    printf("  PASS: Aqua port -> Habitation local bonus = %.1f\n", bonus);
}

void test_no_local_bonus_for_fabrication() {
    printf("Testing no local bonus for Fabrication zones...\n");

    std::vector<PortData> ports;
    ports.push_back({PortType::Aero, 600, true, 1, 50, 50});
    ports.push_back({PortType::Aqua, 800, true, 1, 50, 50});

    float bonus = calculate_local_demand_bonus(2, 50, 50, 1, ports);  // Fabrication
    assert(approx_eq(bonus, 0.0f));

    printf("  PASS: Fabrication -> local bonus = %.1f\n", bonus);
}

// =============================================================================
// Non-operational port tests
// =============================================================================

void test_non_operational_no_local_bonus() {
    printf("Testing non-operational ports provide no local bonus...\n");

    std::vector<PortData> ports;
    ports.push_back({PortType::Aero, 600, false, 1, 50, 50});  // Not operational

    float bonus = calculate_local_demand_bonus(0, 50, 50, 1, ports);
    assert(approx_eq(bonus, 0.0f));

    printf("  PASS: Non-operational aero at same position -> bonus = %.1f\n", bonus);
}

void test_mixed_operational_local_bonus() {
    printf("Testing mix of operational and non-operational ports...\n");

    std::vector<PortData> ports;
    ports.push_back({PortType::Aero, 600, false, 1, 50, 50});  // Not operational
    ports.push_back({PortType::Aero, 600, true, 1, 52, 52});   // Operational, in range

    float bonus = calculate_local_demand_bonus(0, 50, 50, 1, ports);
    assert(approx_eq(bonus, 5.0f));  // Only the operational one

    printf("  PASS: 1 non-op + 1 op -> local bonus = %.1f\n", bonus);
}

// =============================================================================
// Owner filtering tests
// =============================================================================

void test_local_bonus_owner_filtering() {
    printf("Testing local bonus owner filtering...\n");

    std::vector<PortData> ports;
    ports.push_back({PortType::Aero, 600, true, 1, 50, 50});  // Player 1
    ports.push_back({PortType::Aero, 600, true, 2, 52, 52});  // Player 2

    // Player 1 query
    float bonus_p1 = calculate_local_demand_bonus(0, 50, 50, 1, ports);
    assert(approx_eq(bonus_p1, 5.0f));

    // Player 2 query
    float bonus_p2 = calculate_local_demand_bonus(0, 50, 50, 2, ports);
    assert(approx_eq(bonus_p2, 5.0f));

    printf("  PASS: Player 1 = %.1f, Player 2 = %.1f\n", bonus_p1, bonus_p2);
}

// =============================================================================
// Multiple port stacking tests
// =============================================================================

void test_multiple_aero_ports_stack_locally() {
    printf("Testing multiple aero ports stack local bonus...\n");

    std::vector<PortData> ports;
    ports.push_back({PortType::Aero, 600, true, 1, 50, 50});
    ports.push_back({PortType::Aero, 600, true, 1, 55, 55});
    ports.push_back({PortType::Aero, 600, true, 1, 48, 48});

    // All within radius 20 of query point (50,50)
    float bonus = calculate_local_demand_bonus(0, 50, 50, 1, ports);
    assert(approx_eq(bonus, 15.0f));  // 3 * 5.0 = 15.0

    printf("  PASS: 3 aero ports in range -> local bonus = %.1f\n", bonus);
}

void test_multiple_aqua_ports_stack_locally() {
    printf("Testing multiple aqua ports stack local bonus...\n");

    std::vector<PortData> ports;
    ports.push_back({PortType::Aqua, 800, true, 1, 50, 50});
    ports.push_back({PortType::Aqua, 800, true, 1, 55, 55});

    float bonus = calculate_local_demand_bonus(1, 50, 50, 1, ports);
    assert(approx_eq(bonus, 20.0f));  // 2 * 10.0 = 20.0

    printf("  PASS: 2 aqua ports in range -> local bonus = %.1f\n", bonus);
}

void test_only_in_range_ports_contribute() {
    printf("Testing only in-range ports contribute to local bonus...\n");

    std::vector<PortData> ports;
    ports.push_back({PortType::Aero, 600, true, 1, 50, 50});   // In range
    ports.push_back({PortType::Aero, 600, true, 1, 100, 100}); // Out of range

    float bonus = calculate_local_demand_bonus(0, 50, 50, 1, ports);
    assert(approx_eq(bonus, 5.0f));  // Only 1 in range

    printf("  PASS: 1 in range + 1 out of range -> bonus = %.1f\n", bonus);
}

// =============================================================================
// Manhattan distance verification
// =============================================================================

void test_manhattan_distance_asymmetric() {
    printf("Testing Manhattan distance with asymmetric offsets...\n");

    std::vector<PortData> ports;
    ports.push_back({PortType::Aero, 600, true, 1, 50, 50});

    // dist = |50-70| + |50-50| = 20 (exactly at boundary)
    float bonus1 = calculate_local_demand_bonus(0, 70, 50, 1, ports);
    assert(approx_eq(bonus1, 5.0f));

    // dist = |50-50| + |50-70| = 20 (exactly at boundary, other axis)
    float bonus2 = calculate_local_demand_bonus(0, 50, 70, 1, ports);
    assert(approx_eq(bonus2, 5.0f));

    // dist = |50-65| + |50-56| = 15+6 = 21 (just outside)
    float bonus3 = calculate_local_demand_bonus(0, 65, 56, 1, ports);
    assert(approx_eq(bonus3, 0.0f));

    printf("  PASS: Asymmetric Manhattan distance checks correct\n");
}

void test_manhattan_distance_negative_coords() {
    printf("Testing Manhattan distance with negative coordinates...\n");

    std::vector<PortData> ports;
    ports.push_back({PortType::Aqua, 800, true, 1, -10, -10});

    // dist = |-10-(-5)| + |-10-(-5)| = 5+5=10 (within 25)
    float bonus = calculate_local_demand_bonus(1, -5, -5, 1, ports);
    assert(approx_eq(bonus, 10.0f));

    printf("  PASS: Negative coords -> bonus = %.1f\n", bonus);
}

// =============================================================================
// Combined bonus (global + local) cap tests
// =============================================================================

void test_combined_bonus_under_cap() {
    printf("Testing combined bonus under cap...\n");

    std::vector<PortData> ports;
    // Aero port: global bonus to Exchange (+10), local bonus to Habitation (+5)
    ports.push_back({PortType::Aero, 600, true, 1, 50, 50});

    // Habitation at (50,50): global=0 (aero->exchange globally), local=5 (aero->habitation locally)
    float combined = calculate_combined_demand_bonus(0, 50, 50, 1, ports);
    assert(approx_eq(combined, 5.0f));

    printf("  PASS: Combined Habitation bonus = %.1f (under cap)\n", combined);
}

void test_combined_bonus_capped_at_30() {
    printf("Testing combined (global + local) bonus capped at +30...\n");

    std::vector<PortData> ports;
    // 2 large aqua ports: global bonus to Fabrication = 2*15=30 (capped),
    //                     also local bonus to Exchange = 2*10=20
    // For Exchange query: global=0 (aqua->fabrication globally), local=20
    // But let's create a scenario where both contribute to same zone:
    // Exchange (1): global from Aero, local from Aqua
    ports.push_back({PortType::Aero, 2000, true, 1, 50, 50});  // Global Exchange: +15
    ports.push_back({PortType::Aero, 2000, true, 1, 52, 52});  // Global Exchange: +15 -> total global=30
    ports.push_back({PortType::Aqua, 800, true, 1, 50, 50});   // Local Exchange: +10

    // Exchange at (50,50): global=30 (capped), local=10, combined should cap at 30
    float combined = calculate_combined_demand_bonus(1, 50, 50, 1, ports);
    assert(approx_eq(combined, 30.0f));

    printf("  PASS: Combined Exchange bonus = %.1f (capped at 30)\n", combined);
}

void test_combined_bonus_global_plus_local_exceeds_cap() {
    printf("Testing combined global+local exceeds cap -> capped at 30...\n");

    std::vector<PortData> ports;
    // For Habitation: global=0 (no port type provides global habitation bonus)
    //   local from Aero: 7 ports * +5 = +35
    for (int i = 0; i < 7; ++i) {
        ports.push_back({PortType::Aero, 600, true, 1, 50 + i, 50});
    }

    // All within radius 20 of (50,50)
    float combined = calculate_combined_demand_bonus(0, 50, 50, 1, ports);
    assert(approx_eq(combined, 30.0f));  // raw local=35, capped to 30

    printf("  PASS: 7 aero ports local=35 -> combined capped at %.1f\n", combined);
}

void test_combined_global_and_local_both_contribute() {
    printf("Testing combined where both global and local contribute...\n");

    // Exchange (1): global from Aero ports, local from Aqua ports
    std::vector<PortData> ports;
    ports.push_back({PortType::Aero, 600, true, 1, 100, 100});  // Global Exchange: +10 (any position)
    ports.push_back({PortType::Aqua, 800, true, 1, 50, 50});    // Local Exchange: +10 (within range)

    float combined = calculate_combined_demand_bonus(1, 50, 50, 1, ports);
    assert(approx_eq(combined, 20.0f));  // global=10 + local=10 = 20

    printf("  PASS: Global(10) + Local(10) = %.1f\n", combined);
}

// =============================================================================
// Edge cases
// =============================================================================

void test_empty_ports_local() {
    printf("Testing empty ports vector for local bonus...\n");

    std::vector<PortData> empty;
    float bonus = calculate_local_demand_bonus(0, 50, 50, 1, empty);
    assert(approx_eq(bonus, 0.0f));

    printf("  PASS: Empty ports -> local bonus = %.1f\n", bonus);
}

void test_invalid_zone_type_local() {
    printf("Testing invalid zone type returns 0 for local bonus...\n");

    std::vector<PortData> ports;
    ports.push_back({PortType::Aero, 600, true, 1, 50, 50});

    float bonus = calculate_local_demand_bonus(99, 50, 50, 1, ports);
    assert(approx_eq(bonus, 0.0f));

    printf("  PASS: Zone type 99 -> local bonus = %.1f\n", bonus);
}

void test_local_bonus_does_not_depend_on_capacity() {
    printf("Testing local bonus is flat (not capacity-dependent)...\n");

    std::vector<PortData> ports;
    // Small, medium, and large aero ports all give same local bonus (+5)
    ports.push_back({PortType::Aero, 100, true, 1, 50, 50});   // Small

    float bonus_small = calculate_local_demand_bonus(0, 50, 50, 1, ports);
    assert(approx_eq(bonus_small, 5.0f));

    ports.clear();
    ports.push_back({PortType::Aero, 1000, true, 1, 50, 50});  // Medium

    float bonus_medium = calculate_local_demand_bonus(0, 50, 50, 1, ports);
    assert(approx_eq(bonus_medium, 5.0f));

    ports.clear();
    ports.push_back({PortType::Aero, 3000, true, 1, 50, 50});  // Large

    float bonus_large = calculate_local_demand_bonus(0, 50, 50, 1, ports);
    assert(approx_eq(bonus_large, 5.0f));

    printf("  PASS: All capacities give same local bonus = %.1f\n", bonus_small);
}

// =============================================================================
// Main
// =============================================================================

int main() {
    printf("=== Local Demand Bonus Tests (Epic 8, Ticket E8-017) ===\n\n");

    // Aero -> Habitation local
    test_aero_boosts_habitation_within_radius();
    test_aero_no_bonus_outside_radius();
    test_aero_bonus_at_exact_radius();
    test_aero_no_bonus_at_21();
    test_aero_same_position();

    // Aqua -> Exchange local
    test_aqua_boosts_exchange_within_radius();
    test_aqua_no_bonus_outside_radius();
    test_aqua_bonus_at_exact_radius();
    test_aqua_no_bonus_at_26();

    // Cross-type
    test_aero_no_local_bonus_for_exchange();
    test_aqua_no_local_bonus_for_habitation();
    test_no_local_bonus_for_fabrication();

    // Non-operational
    test_non_operational_no_local_bonus();
    test_mixed_operational_local_bonus();

    // Owner filtering
    test_local_bonus_owner_filtering();

    // Stacking
    test_multiple_aero_ports_stack_locally();
    test_multiple_aqua_ports_stack_locally();
    test_only_in_range_ports_contribute();

    // Manhattan distance
    test_manhattan_distance_asymmetric();
    test_manhattan_distance_negative_coords();

    // Combined bonus (global + local)
    test_combined_bonus_under_cap();
    test_combined_bonus_capped_at_30();
    test_combined_bonus_global_plus_local_exceeds_cap();
    test_combined_global_and_local_both_contribute();

    // Edge cases
    test_empty_ports_local();
    test_invalid_zone_type_local();
    test_local_bonus_does_not_depend_on_capacity();

    printf("\n=== All Local Demand Bonus Tests Passed ===\n");
    return 0;
}
