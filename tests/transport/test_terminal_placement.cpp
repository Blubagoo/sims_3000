/**
 * @file test_terminal_placement.cpp
 * @brief Unit tests for terminal placement validation and activation (Epic 7, Ticket E7-034)
 *
 * Tests:
 * - can_place_terminal: bounds check, occupied check, adjacent rail check
 * - place_terminal integration with can_place_terminal
 * - check_terminal_activation: power + adjacent rail
 * - Terminal activation during tick
 */

#include <sims3000/transport/RailSystem.h>
#include <sims3000/building/ForwardDependencyInterfaces.h>
#include <cassert>
#include <cstdio>
#include <cstdint>

using namespace sims3000::transport;
using namespace sims3000::building;

// =============================================================================
// Mock energy provider for testing
// =============================================================================

class MockEnergyProvider : public IEnergyProvider {
public:
    bool default_powered = true;

    struct PoweredPos {
        uint32_t x, y, player_id;
    };

    static constexpr int MAX_POWERED = 32;
    PoweredPos powered_positions[MAX_POWERED] = {};
    int powered_count = 0;
    bool use_position_list = false;

    bool is_powered(uint32_t /*entity_id*/) const override {
        return default_powered;
    }

    bool is_powered_at(uint32_t x, uint32_t y, uint32_t player_id) const override {
        if (!use_position_list) return default_powered;
        for (int i = 0; i < powered_count; ++i) {
            if (powered_positions[i].x == x &&
                powered_positions[i].y == y &&
                powered_positions[i].player_id == player_id) {
                return true;
            }
        }
        return false;
    }

    void add_powered_position(uint32_t x, uint32_t y, uint32_t player_id) {
        assert(powered_count < MAX_POWERED);
        powered_positions[powered_count++] = {x, y, player_id};
    }
};

// =============================================================================
// can_place_terminal tests
// =============================================================================

void test_can_place_terminal_valid() {
    printf("Testing can_place_terminal with valid placement...\n");

    RailSystem system(64, 64);

    // Place a rail first
    system.place_rail(10, 10, RailType::SurfaceRail, 0);

    // Terminal adjacent to rail (east)
    assert(system.can_place_terminal(11, 10, 0) == true);

    // Terminal adjacent to rail (west)
    assert(system.can_place_terminal(9, 10, 0) == true);

    // Terminal adjacent to rail (north)
    assert(system.can_place_terminal(10, 9, 0) == true);

    // Terminal adjacent to rail (south)
    assert(system.can_place_terminal(10, 11, 0) == true);

    printf("  PASS: Valid placements accepted in all cardinal directions\n");
}

void test_can_place_terminal_out_of_bounds() {
    printf("Testing can_place_terminal out of bounds...\n");

    RailSystem system(64, 64);

    // Even if we had rail nearby, out of bounds should fail
    system.place_rail(0, 0, RailType::SurfaceRail, 0);

    assert(system.can_place_terminal(-1, 0, 0) == false);
    assert(system.can_place_terminal(0, -1, 0) == false);
    assert(system.can_place_terminal(64, 0, 0) == false);
    assert(system.can_place_terminal(0, 64, 0) == false);

    printf("  PASS: Out-of-bounds placements rejected\n");
}

void test_can_place_terminal_invalid_owner() {
    printf("Testing can_place_terminal with invalid owner...\n");

    RailSystem system(64, 64);
    system.place_rail(10, 10, RailType::SurfaceRail, 0);

    assert(system.can_place_terminal(11, 10, 5) == false);
    assert(system.can_place_terminal(11, 10, 255) == false);

    printf("  PASS: Invalid owner rejected\n");
}

void test_can_place_terminal_occupied() {
    printf("Testing can_place_terminal at occupied position...\n");

    RailSystem system(64, 64);

    // Place rail at (10,10), terminal at (11,10)
    system.place_rail(10, 10, RailType::SurfaceRail, 0);
    uint32_t term_id = system.place_terminal(11, 10, TerminalType::SurfaceStation, 0);
    assert(term_id != 0);

    // Another terminal at same position should fail (same player)
    assert(system.can_place_terminal(11, 10, 0) == false);

    // Another terminal at same position should fail (different player)
    assert(system.can_place_terminal(11, 10, 1) == false);

    printf("  PASS: Occupied position rejected\n");
}

void test_can_place_terminal_no_adjacent_rail() {
    printf("Testing can_place_terminal with no adjacent rail...\n");

    RailSystem system(64, 64);

    // No rails placed at all
    assert(system.can_place_terminal(10, 10, 0) == false);

    // Rail placed but not adjacent (diagonal doesn't count)
    system.place_rail(20, 20, RailType::SurfaceRail, 0);
    assert(system.can_place_terminal(21, 21, 0) == false);  // Diagonal
    assert(system.can_place_terminal(22, 20, 0) == false);  // 2 tiles away

    printf("  PASS: No adjacent rail rejected\n");
}

void test_can_place_terminal_cross_player_rail() {
    printf("Testing can_place_terminal with cross-player adjacent rail...\n");

    RailSystem system(64, 64);

    // Player 1 places a rail
    system.place_rail(10, 10, RailType::SurfaceRail, 1);

    // Player 0 should be able to place terminal adjacent to player 1's rail
    assert(system.can_place_terminal(11, 10, 0) == true);

    printf("  PASS: Cross-player adjacent rail accepted\n");
}

void test_can_place_terminal_edge_of_map() {
    printf("Testing can_place_terminal at map edges...\n");

    RailSystem system(64, 64);

    // Place rail at (0,1) so terminal at (0,0) has adjacent rail to south
    system.place_rail(0, 1, RailType::SurfaceRail, 0);
    assert(system.can_place_terminal(0, 0, 0) == true);

    // Place rail at (62,63) so terminal at (63,63) has adjacent rail to west
    system.place_rail(62, 63, RailType::SurfaceRail, 0);
    assert(system.can_place_terminal(63, 63, 0) == true);

    printf("  PASS: Edge-of-map placements work\n");
}

// =============================================================================
// place_terminal integration tests (uses can_place_terminal validation)
// =============================================================================

void test_place_terminal_requires_adjacent_rail() {
    printf("Testing place_terminal enforces adjacent rail requirement...\n");

    RailSystem system(64, 64);

    // No rail placed - terminal placement should fail
    uint32_t id = system.place_terminal(10, 10, TerminalType::SurfaceStation, 0);
    assert(id == 0);
    assert(system.get_terminal_count(0) == 0);

    // Place rail, then terminal adjacent should succeed
    system.place_rail(10, 10, RailType::SurfaceRail, 0);
    id = system.place_terminal(11, 10, TerminalType::SurfaceStation, 0);
    assert(id != 0);
    assert(system.get_terminal_count(0) == 1);

    printf("  PASS: place_terminal enforces adjacent rail\n");
}

void test_place_terminal_rejects_duplicate() {
    printf("Testing place_terminal rejects duplicate position...\n");

    RailSystem system(64, 64);

    system.place_rail(10, 10, RailType::SurfaceRail, 0);
    uint32_t id1 = system.place_terminal(11, 10, TerminalType::SurfaceStation, 0);
    assert(id1 != 0);

    // Second terminal at same position should fail
    uint32_t id2 = system.place_terminal(11, 10, TerminalType::SubterraStation, 0);
    assert(id2 == 0);

    printf("  PASS: Duplicate position rejected\n");
}

// =============================================================================
// Terminal activation tests
// =============================================================================

void test_terminal_activation_powered_and_adjacent_rail() {
    printf("Testing terminal activation: powered + adjacent rail...\n");

    RailSystem system(64, 64);

    // Place rail then terminal
    system.place_rail(10, 10, RailType::SurfaceRail, 0);
    uint32_t term_id = system.place_terminal(11, 10, TerminalType::SurfaceStation, 0);
    assert(term_id != 0);

    // No energy provider = all powered fallback
    system.tick(0.0f);

    // Terminal should be active (powered + adjacent rail)
    assert(system.is_terminal_active(term_id) == true);

    printf("  PASS: Terminal active when powered with adjacent rail\n");
}

void test_terminal_activation_not_powered() {
    printf("Testing terminal activation: not powered...\n");

    RailSystem system(64, 64);
    MockEnergyProvider provider;
    provider.default_powered = false;
    provider.use_position_list = true;
    // No positions powered

    system.set_energy_provider(&provider);

    system.place_rail(10, 10, RailType::SurfaceRail, 0);
    uint32_t term_id = system.place_terminal(11, 10, TerminalType::SurfaceStation, 0);
    assert(term_id != 0);

    system.tick(0.0f);

    // Terminal should NOT be active (not powered)
    assert(system.is_terminal_active(term_id) == false);

    printf("  PASS: Unpowered terminal is not active\n");
}

void test_terminal_activation_rail_removed() {
    printf("Testing terminal activation: adjacent rail removed...\n");

    RailSystem system(64, 64);

    // Place rail and terminal
    uint32_t rail_id = system.place_rail(10, 10, RailType::SurfaceRail, 0);
    uint32_t term_id = system.place_terminal(11, 10, TerminalType::SurfaceStation, 0);
    assert(term_id != 0);

    system.tick(0.0f);
    assert(system.is_terminal_active(term_id) == true);

    // Remove the adjacent rail
    bool removed = system.remove_rail(rail_id, 0);
    assert(removed == true);

    system.tick(0.0f);

    // Terminal should NOT be active (no adjacent rail anymore)
    assert(system.is_terminal_active(term_id) == false);

    printf("  PASS: Terminal inactive after adjacent rail removed\n");
}

void test_terminal_activation_power_toggled() {
    printf("Testing terminal activation: power toggled on/off...\n");

    RailSystem system(64, 64);
    MockEnergyProvider provider;
    provider.use_position_list = true;
    provider.add_powered_position(10, 10, 0);  // Rail position
    provider.add_powered_position(11, 10, 0);  // Terminal position

    system.set_energy_provider(&provider);

    system.place_rail(10, 10, RailType::SurfaceRail, 0);
    uint32_t term_id = system.place_terminal(11, 10, TerminalType::SurfaceStation, 0);
    assert(term_id != 0);

    system.tick(0.0f);
    assert(system.is_terminal_active(term_id) == true);

    // Remove power from terminal position
    provider.powered_count = 0;
    provider.add_powered_position(10, 10, 0);  // Only rail powered, not terminal

    system.tick(0.0f);
    assert(system.is_terminal_active(term_id) == false);

    // Restore power
    provider.add_powered_position(11, 10, 0);

    system.tick(0.0f);
    assert(system.is_terminal_active(term_id) == true);

    printf("  PASS: Terminal responds to power toggle\n");
}

void test_terminal_activation_multiple_terminals() {
    printf("Testing terminal activation: multiple terminals mixed states...\n");

    RailSystem system(64, 64);
    MockEnergyProvider provider;
    provider.use_position_list = true;
    provider.add_powered_position(10, 10, 0);  // Rail
    provider.add_powered_position(11, 10, 0);  // Terminal 1 (powered)
    // Terminal 2 at (9, 10) is NOT powered

    system.set_energy_provider(&provider);

    system.place_rail(10, 10, RailType::SurfaceRail, 0);
    uint32_t term1 = system.place_terminal(11, 10, TerminalType::SurfaceStation, 0);
    uint32_t term2 = system.place_terminal(9, 10, TerminalType::SurfaceStation, 0);
    assert(term1 != 0);
    assert(term2 != 0);

    system.tick(0.0f);

    assert(system.is_terminal_active(term1) == true);
    assert(system.is_terminal_active(term2) == false);

    printf("  PASS: Multiple terminals have correct mixed activation\n");
}

void test_terminal_all_types_activation() {
    printf("Testing terminal activation: all terminal types...\n");

    RailSystem system(64, 64);

    // Place rails at three positions
    system.place_rail(10, 10, RailType::SurfaceRail, 0);
    system.place_rail(20, 10, RailType::ElevatedRail, 0);
    system.place_rail(30, 10, RailType::SubterraRail, 0);

    uint32_t t1 = system.place_terminal(11, 10, TerminalType::SurfaceStation, 0);
    uint32_t t2 = system.place_terminal(21, 10, TerminalType::SubterraStation, 0);
    uint32_t t3 = system.place_terminal(31, 10, TerminalType::IntermodalHub, 0);

    assert(t1 != 0);
    assert(t2 != 0);
    assert(t3 != 0);

    // No provider = all powered
    system.tick(0.0f);

    assert(system.is_terminal_active(t1) == true);
    assert(system.is_terminal_active(t2) == true);
    assert(system.is_terminal_active(t3) == true);

    printf("  PASS: All terminal types activate correctly\n");
}

// =============================================================================
// Main
// =============================================================================

int main() {
    printf("=== Terminal Placement Tests (Epic 7, Ticket E7-034) ===\n\n");

    // can_place_terminal tests
    test_can_place_terminal_valid();
    test_can_place_terminal_out_of_bounds();
    test_can_place_terminal_invalid_owner();
    test_can_place_terminal_occupied();
    test_can_place_terminal_no_adjacent_rail();
    test_can_place_terminal_cross_player_rail();
    test_can_place_terminal_edge_of_map();

    // place_terminal integration
    test_place_terminal_requires_adjacent_rail();
    test_place_terminal_rejects_duplicate();

    // Terminal activation
    test_terminal_activation_powered_and_adjacent_rail();
    test_terminal_activation_not_powered();
    test_terminal_activation_rail_removed();
    test_terminal_activation_power_toggled();
    test_terminal_activation_multiple_terminals();
    test_terminal_all_types_activation();

    printf("\n=== All Terminal Placement Tests Passed ===\n");
    return 0;
}
