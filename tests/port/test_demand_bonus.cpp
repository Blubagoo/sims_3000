/**
 * @file test_demand_bonus.cpp
 * @brief Unit tests for global demand bonus calculation (Epic 8, Ticket E8-016)
 *
 * Tests cover:
 * - Aero ports boost Exchange demand
 * - Aqua ports boost Fabrication demand
 * - Port size thresholds (Small/Medium/Large)
 * - Total bonus capped at +30
 * - Non-operational ports do not contribute
 * - Owner filtering
 * - Other zone types return 0
 * - Edge cases: no ports, zero capacity
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
// Port Size Bonus Tests
// =============================================================================

void test_port_size_bonus_zero() {
    printf("Testing port size bonus with zero capacity...\n");
    assert(approx_eq(get_port_size_bonus(0), 0.0f));
    printf("  PASS: capacity 0 -> bonus 0\n");
}

void test_port_size_bonus_small() {
    printf("Testing port size bonus for Small ports (1-499)...\n");
    assert(approx_eq(get_port_size_bonus(1), 5.0f));
    assert(approx_eq(get_port_size_bonus(100), 5.0f));
    assert(approx_eq(get_port_size_bonus(499), 5.0f));
    printf("  PASS: Small ports get +5\n");
}

void test_port_size_bonus_medium() {
    printf("Testing port size bonus for Medium ports (500-1999)...\n");
    assert(approx_eq(get_port_size_bonus(500), 10.0f));
    assert(approx_eq(get_port_size_bonus(1000), 10.0f));
    assert(approx_eq(get_port_size_bonus(1999), 10.0f));
    printf("  PASS: Medium ports get +10\n");
}

void test_port_size_bonus_large() {
    printf("Testing port size bonus for Large ports (>= 2000)...\n");
    assert(approx_eq(get_port_size_bonus(2000), 15.0f));
    assert(approx_eq(get_port_size_bonus(3000), 15.0f));
    assert(approx_eq(get_port_size_bonus(5000), 15.0f));
    printf("  PASS: Large ports get +15\n");
}

// =============================================================================
// Aero Port -> Exchange Demand Tests
// =============================================================================

void test_aero_boosts_exchange() {
    printf("Testing aero port boosts Exchange demand...\n");

    std::vector<PortData> ports;
    ports.push_back({PortType::Aero, 600, true, 1});  // Medium: +10

    float bonus = calculate_global_demand_bonus(1, 1, ports);  // zone_type=1 (Exchange)
    assert(approx_eq(bonus, 10.0f));

    printf("  PASS: 1 medium aero port -> Exchange bonus = %.1f\n", bonus);
}

void test_multiple_aero_ports() {
    printf("Testing multiple aero ports stack...\n");

    std::vector<PortData> ports;
    ports.push_back({PortType::Aero, 200, true, 1});   // Small: +5
    ports.push_back({PortType::Aero, 1000, true, 1});  // Medium: +10

    float bonus = calculate_global_demand_bonus(1, 1, ports);
    assert(approx_eq(bonus, 15.0f));

    printf("  PASS: Small(+5) + Medium(+10) = %.1f\n", bonus);
}

// =============================================================================
// Aqua Port -> Fabrication Demand Tests
// =============================================================================

void test_aqua_boosts_fabrication() {
    printf("Testing aqua port boosts Fabrication demand...\n");

    std::vector<PortData> ports;
    ports.push_back({PortType::Aqua, 2500, true, 1});  // Large: +15

    float bonus = calculate_global_demand_bonus(2, 1, ports);  // zone_type=2 (Fabrication)
    assert(approx_eq(bonus, 15.0f));

    printf("  PASS: 1 large aqua port -> Fabrication bonus = %.1f\n", bonus);
}

void test_multiple_aqua_ports() {
    printf("Testing multiple aqua ports stack...\n");

    std::vector<PortData> ports;
    ports.push_back({PortType::Aqua, 100, true, 1});   // Small: +5
    ports.push_back({PortType::Aqua, 700, true, 1});   // Medium: +10
    ports.push_back({PortType::Aqua, 300, true, 1});   // Small: +5

    float bonus = calculate_global_demand_bonus(2, 1, ports);
    assert(approx_eq(bonus, 20.0f));

    printf("  PASS: Small(+5) + Medium(+10) + Small(+5) = %.1f\n", bonus);
}

// =============================================================================
// Cross-type: Aero does NOT boost Fabrication, Aqua does NOT boost Exchange
// =============================================================================

void test_aero_does_not_boost_fabrication() {
    printf("Testing aero port does NOT boost Fabrication...\n");

    std::vector<PortData> ports;
    ports.push_back({PortType::Aero, 2000, true, 1});  // Large aero

    float bonus = calculate_global_demand_bonus(2, 1, ports);  // Fabrication
    assert(approx_eq(bonus, 0.0f));

    printf("  PASS: Aero port -> Fabrication bonus = %.1f (expected 0)\n", bonus);
}

void test_aqua_does_not_boost_exchange() {
    printf("Testing aqua port does NOT boost Exchange...\n");

    std::vector<PortData> ports;
    ports.push_back({PortType::Aqua, 2000, true, 1});  // Large aqua

    float bonus = calculate_global_demand_bonus(1, 1, ports);  // Exchange
    assert(approx_eq(bonus, 0.0f));

    printf("  PASS: Aqua port -> Exchange bonus = %.1f (expected 0)\n", bonus);
}

// =============================================================================
// Cap at +30 Tests
// =============================================================================

void test_bonus_capped_at_30() {
    printf("Testing total bonus capped at +30...\n");

    std::vector<PortData> ports;
    // 3 large aero ports: 3 * 15 = 45, should cap at 30
    ports.push_back({PortType::Aero, 2500, true, 1});
    ports.push_back({PortType::Aero, 3000, true, 1});
    ports.push_back({PortType::Aero, 2000, true, 1});

    float bonus = calculate_global_demand_bonus(1, 1, ports);
    assert(approx_eq(bonus, 30.0f));

    printf("  PASS: 3 large aero ports (45 raw) -> capped at %.1f\n", bonus);
}

void test_bonus_exactly_30() {
    printf("Testing total bonus exactly at +30...\n");

    std::vector<PortData> ports;
    // 2 large = 30 exactly
    ports.push_back({PortType::Aqua, 2000, true, 1});
    ports.push_back({PortType::Aqua, 3000, true, 1});

    float bonus = calculate_global_demand_bonus(2, 1, ports);
    assert(approx_eq(bonus, 30.0f));

    printf("  PASS: 2 large aqua ports = %.1f (exactly 30)\n", bonus);
}

void test_bonus_under_30() {
    printf("Testing total bonus under +30 is not capped...\n");

    std::vector<PortData> ports;
    // 1 large + 1 small = 20
    ports.push_back({PortType::Aero, 2000, true, 1});
    ports.push_back({PortType::Aero, 100, true, 1});

    float bonus = calculate_global_demand_bonus(1, 1, ports);
    assert(approx_eq(bonus, 20.0f));

    printf("  PASS: Large(+15) + Small(+5) = %.1f (not capped)\n", bonus);
}

// =============================================================================
// Non-operational port tests
// =============================================================================

void test_non_operational_ports_ignored() {
    printf("Testing non-operational ports do not contribute...\n");

    std::vector<PortData> ports;
    ports.push_back({PortType::Aero, 2000, false, 1});  // Not operational
    ports.push_back({PortType::Aero, 1000, true, 1});   // Operational medium: +10

    float bonus = calculate_global_demand_bonus(1, 1, ports);
    assert(approx_eq(bonus, 10.0f));

    printf("  PASS: 1 non-op + 1 op -> bonus = %.1f (only operational counted)\n", bonus);
}

void test_all_non_operational() {
    printf("Testing all non-operational ports -> 0 bonus...\n");

    std::vector<PortData> ports;
    ports.push_back({PortType::Aero, 2000, false, 1});
    ports.push_back({PortType::Aero, 3000, false, 1});

    float bonus = calculate_global_demand_bonus(1, 1, ports);
    assert(approx_eq(bonus, 0.0f));

    printf("  PASS: All non-operational -> bonus = %.1f\n", bonus);
}

// =============================================================================
// Owner filtering tests
// =============================================================================

void test_owner_filtering() {
    printf("Testing owner filtering...\n");

    std::vector<PortData> ports;
    ports.push_back({PortType::Aero, 1000, true, 1});  // Player 1: +10
    ports.push_back({PortType::Aero, 2000, true, 2});  // Player 2: +15 (ignored for player 1)
    ports.push_back({PortType::Aero, 500, true, 1});   // Player 1: +10

    // Player 1: 10 + 10 = 20
    float bonus_p1 = calculate_global_demand_bonus(1, 1, ports);
    assert(approx_eq(bonus_p1, 20.0f));

    // Player 2: 15
    float bonus_p2 = calculate_global_demand_bonus(1, 2, ports);
    assert(approx_eq(bonus_p2, 15.0f));

    printf("  PASS: Player 1 = %.1f, Player 2 = %.1f\n", bonus_p1, bonus_p2);
}

// =============================================================================
// Other zone types return 0
// =============================================================================

void test_habitation_returns_zero() {
    printf("Testing Habitation zone type returns 0...\n");

    std::vector<PortData> ports;
    ports.push_back({PortType::Aero, 2000, true, 1});
    ports.push_back({PortType::Aqua, 2000, true, 1});

    float bonus = calculate_global_demand_bonus(0, 1, ports);  // Habitation
    assert(approx_eq(bonus, 0.0f));

    printf("  PASS: Habitation -> bonus = %.1f\n", bonus);
}

void test_invalid_zone_type_returns_zero() {
    printf("Testing invalid zone type returns 0...\n");

    std::vector<PortData> ports;
    ports.push_back({PortType::Aero, 2000, true, 1});

    float bonus = calculate_global_demand_bonus(99, 1, ports);
    assert(approx_eq(bonus, 0.0f));

    printf("  PASS: Zone type 99 -> bonus = %.1f\n", bonus);
}

// =============================================================================
// Edge cases
// =============================================================================

void test_empty_ports_vector() {
    printf("Testing empty ports vector...\n");

    std::vector<PortData> empty;
    float bonus = calculate_global_demand_bonus(1, 1, empty);
    assert(approx_eq(bonus, 0.0f));

    printf("  PASS: Empty ports -> bonus = %.1f\n", bonus);
}

void test_zero_capacity_operational() {
    printf("Testing zero capacity operational port...\n");

    std::vector<PortData> ports;
    ports.push_back({PortType::Aero, 0, true, 1});

    float bonus = calculate_global_demand_bonus(1, 1, ports);
    assert(approx_eq(bonus, 0.0f));

    printf("  PASS: Zero capacity operational -> bonus = %.1f\n", bonus);
}

// =============================================================================
// Mixed port types
// =============================================================================

void test_mixed_port_types() {
    printf("Testing mixed port types only count relevant ones...\n");

    std::vector<PortData> ports;
    ports.push_back({PortType::Aero, 1000, true, 1});  // Medium aero: +10 for Exchange
    ports.push_back({PortType::Aqua, 2000, true, 1});  // Large aqua: +15 for Fabrication
    ports.push_back({PortType::Aero, 200, true, 1});   // Small aero: +5 for Exchange
    ports.push_back({PortType::Aqua, 500, true, 1});   // Medium aqua: +10 for Fabrication

    // Exchange (zone_type=1): aero only -> 10 + 5 = 15
    float exchange_bonus = calculate_global_demand_bonus(1, 1, ports);
    assert(approx_eq(exchange_bonus, 15.0f));

    // Fabrication (zone_type=2): aqua only -> 15 + 10 = 25
    float fab_bonus = calculate_global_demand_bonus(2, 1, ports);
    assert(approx_eq(fab_bonus, 25.0f));

    printf("  PASS: Exchange = %.1f, Fabrication = %.1f\n", exchange_bonus, fab_bonus);
}

// =============================================================================
// Main
// =============================================================================

int main() {
    printf("=== Global Demand Bonus Tests (Epic 8, Ticket E8-016) ===\n\n");

    // Port size bonus
    test_port_size_bonus_zero();
    test_port_size_bonus_small();
    test_port_size_bonus_medium();
    test_port_size_bonus_large();

    // Aero -> Exchange
    test_aero_boosts_exchange();
    test_multiple_aero_ports();

    // Aqua -> Fabrication
    test_aqua_boosts_fabrication();
    test_multiple_aqua_ports();

    // Cross-type
    test_aero_does_not_boost_fabrication();
    test_aqua_does_not_boost_exchange();

    // Cap at +30
    test_bonus_capped_at_30();
    test_bonus_exactly_30();
    test_bonus_under_30();

    // Non-operational
    test_non_operational_ports_ignored();
    test_all_non_operational();

    // Owner filtering
    test_owner_filtering();

    // Other zone types
    test_habitation_returns_zero();
    test_invalid_zone_type_returns_zero();

    // Edge cases
    test_empty_ports_vector();
    test_zero_capacity_operational();

    // Mixed
    test_mixed_port_types();

    printf("\n=== All Global Demand Bonus Tests Passed ===\n");
    return 0;
}
