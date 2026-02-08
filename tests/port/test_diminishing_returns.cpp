/**
 * @file test_diminishing_returns.cpp
 * @brief Unit tests for multiple port diminishing returns (Epic 8, Ticket E8-035)
 *
 * Tests cover:
 * - Diminishing returns multiplier per port index
 * - apply_diminishing_returns with various base bonuses
 * - Second same-type port gives 50% bonus
 * - Third same-type port gives 25% bonus
 * - Fourth+ ports get floor multiplier (12.5%)
 * - Global demand bonus with diminishing returns
 * - Mixed port types (only same-type diminishes)
 * - Owner filtering with diminishing returns
 * - Edge cases: no ports, single port, non-operational ports
 */

#include <sims3000/port/DiminishingReturns.h>
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
// Diminishing Multiplier Tests
// =============================================================================

void test_first_port_full_multiplier() {
    printf("Testing 1st port gets full multiplier (1.0)...\n");
    assert(approx_eq(get_diminishing_multiplier(0), 1.0f));
    printf("  PASS: Index 0 -> multiplier = 1.0\n");
}

void test_second_port_half_multiplier() {
    printf("Testing 2nd port gets half multiplier (0.5)...\n");
    assert(approx_eq(get_diminishing_multiplier(1), 0.5f));
    printf("  PASS: Index 1 -> multiplier = 0.5\n");
}

void test_third_port_quarter_multiplier() {
    printf("Testing 3rd port gets quarter multiplier (0.25)...\n");
    assert(approx_eq(get_diminishing_multiplier(2), 0.25f));
    printf("  PASS: Index 2 -> multiplier = 0.25\n");
}

void test_fourth_port_floor_multiplier() {
    printf("Testing 4th port gets floor multiplier (0.125)...\n");
    assert(approx_eq(get_diminishing_multiplier(3), 0.125f));
    printf("  PASS: Index 3 -> multiplier = 0.125\n");
}

void test_fifth_port_still_floor() {
    printf("Testing 5th port still gets floor multiplier...\n");
    assert(approx_eq(get_diminishing_multiplier(4), 0.125f));
    printf("  PASS: Index 4 -> multiplier = 0.125 (floor)\n");
}

// =============================================================================
// apply_diminishing_returns Tests
// =============================================================================

void test_apply_diminishing_first() {
    printf("Testing apply_diminishing_returns for 1st port...\n");
    float result = apply_diminishing_returns(10.0f, 0);
    assert(approx_eq(result, 10.0f));
    printf("  PASS: base=10, index=0 -> %0.1f\n", result);
}

void test_apply_diminishing_second() {
    printf("Testing apply_diminishing_returns for 2nd port...\n");
    float result = apply_diminishing_returns(10.0f, 1);
    assert(approx_eq(result, 5.0f));
    printf("  PASS: base=10, index=1 -> %0.1f\n", result);
}

void test_apply_diminishing_third() {
    printf("Testing apply_diminishing_returns for 3rd port...\n");
    float result = apply_diminishing_returns(10.0f, 2);
    assert(approx_eq(result, 2.5f));
    printf("  PASS: base=10, index=2 -> %0.1f\n", result);
}

void test_apply_diminishing_fourth() {
    printf("Testing apply_diminishing_returns for 4th port...\n");
    float result = apply_diminishing_returns(10.0f, 3);
    assert(approx_eq(result, 1.25f));
    printf("  PASS: base=10, index=3 -> %0.2f\n", result);
}

void test_apply_diminishing_with_large_bonus() {
    printf("Testing apply_diminishing_returns with Large bonus (15.0)...\n");
    assert(approx_eq(apply_diminishing_returns(15.0f, 0), 15.0f));
    assert(approx_eq(apply_diminishing_returns(15.0f, 1), 7.5f));
    assert(approx_eq(apply_diminishing_returns(15.0f, 2), 3.75f));
    assert(approx_eq(apply_diminishing_returns(15.0f, 3), 1.875f));
    printf("  PASS: Large bonus diminishing verified\n");
}

// =============================================================================
// Global Demand Bonus with Diminishing Returns Tests
// =============================================================================

void test_single_aero_port_no_diminishing() {
    printf("Testing single aero port (no diminishing effect)...\n");
    std::vector<PortData> ports;
    ports.push_back({PortType::Aero, 600, true, 1});  // Medium: base +10

    float bonus = calculate_global_demand_bonus_with_diminishing(1, 1, ports);
    assert(approx_eq(bonus, 10.0f));  // 10 * 1.0
    printf("  PASS: Single medium aero -> %.1f\n", bonus);
}

void test_two_same_type_ports_diminishing() {
    printf("Testing two same-type ports with diminishing...\n");
    std::vector<PortData> ports;
    ports.push_back({PortType::Aero, 600, true, 1});   // Medium: 10 * 1.0 = 10.0
    ports.push_back({PortType::Aero, 600, true, 1});   // Medium: 10 * 0.5 = 5.0

    float bonus = calculate_global_demand_bonus_with_diminishing(1, 1, ports);
    assert(approx_eq(bonus, 15.0f));  // 10 + 5
    printf("  PASS: Two medium aero -> %.1f (10 + 5)\n", bonus);
}

void test_three_same_type_ports_diminishing() {
    printf("Testing three same-type ports with diminishing...\n");
    std::vector<PortData> ports;
    ports.push_back({PortType::Aero, 600, true, 1});   // Medium: 10 * 1.0 = 10.0
    ports.push_back({PortType::Aero, 600, true, 1});   // Medium: 10 * 0.5 = 5.0
    ports.push_back({PortType::Aero, 600, true, 1});   // Medium: 10 * 0.25 = 2.5

    float bonus = calculate_global_demand_bonus_with_diminishing(1, 1, ports);
    assert(approx_eq(bonus, 17.5f));  // 10 + 5 + 2.5
    printf("  PASS: Three medium aero -> %.1f (10 + 5 + 2.5)\n", bonus);
}

void test_four_same_type_ports_diminishing() {
    printf("Testing four same-type ports with diminishing (floor)...\n");
    std::vector<PortData> ports;
    ports.push_back({PortType::Aero, 2000, true, 1});  // Large: 15 * 1.0 = 15.0
    ports.push_back({PortType::Aero, 2000, true, 1});  // Large: 15 * 0.5 = 7.5
    ports.push_back({PortType::Aero, 2000, true, 1});  // Large: 15 * 0.25 = 3.75
    ports.push_back({PortType::Aero, 2000, true, 1});  // Large: 15 * 0.125 = 1.875

    float bonus = calculate_global_demand_bonus_with_diminishing(1, 1, ports);
    assert(approx_eq(bonus, 28.125f));  // 15 + 7.5 + 3.75 + 1.875
    printf("  PASS: Four large aero -> %.3f\n", bonus);
}

void test_diminishing_still_capped_at_30() {
    printf("Testing diminishing bonus still capped at 30...\n");
    std::vector<PortData> ports;
    // Many large ports to try to exceed cap
    for (int i = 0; i < 10; i++) {
        ports.push_back({PortType::Aero, 2000, true, 1});
    }

    float bonus = calculate_global_demand_bonus_with_diminishing(1, 1, ports);
    assert(bonus <= 30.0f);
    printf("  PASS: 10 large aero ports -> %.1f (capped at 30)\n", bonus);
}

// =============================================================================
// Mixed Port Types (only same-type diminishes)
// =============================================================================

void test_different_types_no_cross_diminishing() {
    printf("Testing different types don't cross-diminish...\n");
    std::vector<PortData> ports;
    // Aero and Aqua don't interfere with each other's indexing
    ports.push_back({PortType::Aero, 600, true, 1});   // Aero #1: 10 * 1.0 = 10.0
    ports.push_back({PortType::Aqua, 600, true, 1});   // Aqua (ignored for Exchange)
    ports.push_back({PortType::Aero, 600, true, 1});   // Aero #2: 10 * 0.5 = 5.0

    float exchange_bonus = calculate_global_demand_bonus_with_diminishing(1, 1, ports);
    assert(approx_eq(exchange_bonus, 15.0f));  // 10 + 5 (aqua ignored)

    // For Fabrication, only aqua ports count
    float fab_bonus = calculate_global_demand_bonus_with_diminishing(2, 1, ports);
    assert(approx_eq(fab_bonus, 10.0f));  // 10 * 1.0 (only 1 aqua)

    printf("  PASS: Exchange=%.1f (aero only), Fabrication=%.1f (aqua only)\n",
           exchange_bonus, fab_bonus);
}

// =============================================================================
// Non-operational Ports Skipped in Indexing
// =============================================================================

void test_non_operational_skipped_in_index() {
    printf("Testing non-operational ports don't count toward index...\n");
    std::vector<PortData> ports;
    ports.push_back({PortType::Aero, 600, true, 1});    // Aero #1: 10 * 1.0 = 10.0
    ports.push_back({PortType::Aero, 600, false, 1});   // Non-operational (skipped)
    ports.push_back({PortType::Aero, 600, true, 1});    // Aero #2: 10 * 0.5 = 5.0

    float bonus = calculate_global_demand_bonus_with_diminishing(1, 1, ports);
    assert(approx_eq(bonus, 15.0f));  // 10 + 5 (non-op skipped)
    printf("  PASS: Non-operational skipped -> %.1f\n", bonus);
}

// =============================================================================
// Owner Filtering with Diminishing Returns
// =============================================================================

void test_owner_filtering_with_diminishing() {
    printf("Testing owner filtering with diminishing returns...\n");
    std::vector<PortData> ports;
    ports.push_back({PortType::Aero, 600, true, 1});   // Player 1, Aero #1: 10 * 1.0
    ports.push_back({PortType::Aero, 600, true, 2});   // Player 2 (ignored for P1)
    ports.push_back({PortType::Aero, 600, true, 1});   // Player 1, Aero #2: 10 * 0.5

    float p1_bonus = calculate_global_demand_bonus_with_diminishing(1, 1, ports);
    assert(approx_eq(p1_bonus, 15.0f));  // 10 + 5

    float p2_bonus = calculate_global_demand_bonus_with_diminishing(1, 2, ports);
    assert(approx_eq(p2_bonus, 10.0f));  // 10 * 1.0 (only 1 port)

    printf("  PASS: P1=%.1f, P2=%.1f\n", p1_bonus, p2_bonus);
}

// =============================================================================
// Edge Cases
// =============================================================================

void test_empty_ports_diminishing() {
    printf("Testing diminishing with empty ports...\n");
    std::vector<PortData> empty;
    float bonus = calculate_global_demand_bonus_with_diminishing(1, 1, empty);
    assert(approx_eq(bonus, 0.0f));
    printf("  PASS: Empty ports -> %.1f\n", bonus);
}

void test_other_zone_type_returns_zero() {
    printf("Testing other zone type returns 0 with diminishing...\n");
    std::vector<PortData> ports;
    ports.push_back({PortType::Aero, 2000, true, 1});

    float bonus = calculate_global_demand_bonus_with_diminishing(0, 1, ports);  // Habitation
    assert(approx_eq(bonus, 0.0f));
    printf("  PASS: Zone type 0 -> %.1f\n", bonus);
}

void test_different_sized_ports_diminishing() {
    printf("Testing different sized ports with diminishing...\n");
    std::vector<PortData> ports;
    ports.push_back({PortType::Aqua, 2500, true, 1});  // Large:  15 * 1.0 = 15.0
    ports.push_back({PortType::Aqua, 800, true, 1});   // Medium: 10 * 0.5 = 5.0
    ports.push_back({PortType::Aqua, 100, true, 1});   // Small:  5 * 0.25 = 1.25

    float bonus = calculate_global_demand_bonus_with_diminishing(2, 1, ports);
    assert(approx_eq(bonus, 21.25f));  // 15 + 5 + 1.25
    printf("  PASS: Large+Medium+Small aqua -> %.2f\n", bonus);
}

void test_encourages_diversity() {
    printf("Testing diversity strategy is more effective...\n");

    // Strategy 1: Three aero ports for Exchange
    std::vector<PortData> all_aero;
    all_aero.push_back({PortType::Aero, 2000, true, 1});  // 15 * 1.0 = 15.0
    all_aero.push_back({PortType::Aero, 2000, true, 1});  // 15 * 0.5 = 7.5
    all_aero.push_back({PortType::Aero, 2000, true, 1});  // 15 * 0.25 = 3.75
    float three_aero = calculate_global_demand_bonus_with_diminishing(1, 1, all_aero);

    // Strategy 2: One aero port (no diminishing)
    std::vector<PortData> one_aero;
    one_aero.push_back({PortType::Aero, 2000, true, 1});  // 15 * 1.0 = 15.0
    float single_aero = calculate_global_demand_bonus_with_diminishing(1, 1, one_aero);

    // Three ports of same type give 26.25, not 45 (without diminishing)
    assert(approx_eq(three_aero, 26.25f));
    assert(approx_eq(single_aero, 15.0f));

    // The second and third ports together only add 11.25 (not 30 more)
    float marginal = three_aero - single_aero;
    assert(approx_eq(marginal, 11.25f));

    printf("  PASS: 3 aero=%.2f, 1 aero=%.1f, marginal=%.2f\n",
           three_aero, single_aero, marginal);
}

// =============================================================================
// Main
// =============================================================================

int main() {
    printf("=== Multiple Port Diminishing Returns Tests (Epic 8, Ticket E8-035) ===\n\n");

    // Multiplier values
    test_first_port_full_multiplier();
    test_second_port_half_multiplier();
    test_third_port_quarter_multiplier();
    test_fourth_port_floor_multiplier();
    test_fifth_port_still_floor();

    // apply_diminishing_returns
    test_apply_diminishing_first();
    test_apply_diminishing_second();
    test_apply_diminishing_third();
    test_apply_diminishing_fourth();
    test_apply_diminishing_with_large_bonus();

    // Global demand bonus with diminishing
    test_single_aero_port_no_diminishing();
    test_two_same_type_ports_diminishing();
    test_three_same_type_ports_diminishing();
    test_four_same_type_ports_diminishing();
    test_diminishing_still_capped_at_30();

    // Mixed types
    test_different_types_no_cross_diminishing();

    // Non-operational
    test_non_operational_skipped_in_index();

    // Owner filtering
    test_owner_filtering_with_diminishing();

    // Edge cases
    test_empty_ports_diminishing();
    test_other_zone_type_returns_zero();
    test_different_sized_ports_diminishing();
    test_encourages_diversity();

    printf("\n=== All Multiple Port Diminishing Returns Tests Passed ===\n");
    return 0;
}
