/**
 * @file test_rail_system.cpp
 * @brief Unit tests for RailSystem (Epic 7, Ticket E7-032)
 *
 * Tests:
 * - Construction with map dimensions
 * - Priority = 47 (after TransportSystem at 45)
 * - Rail placement and removal
 * - Terminal placement and removal
 * - Power state queries
 * - Terminal active state queries
 * - Terminal coverage radius queries
 * - Per-player rail/terminal counts
 * - Tick execution (stub phases)
 * - Ownership enforcement
 * - Bounds checking
 */

#include <sims3000/transport/RailSystem.h>
#include <cassert>
#include <cstdio>

using namespace sims3000::transport;

void test_construction() {
    printf("Testing RailSystem construction...\n");

    RailSystem system(64, 64);

    // Should start with zero rails and terminals for all players
    for (uint8_t p = 0; p < RailSystem::MAX_PLAYERS; ++p) {
        assert(system.get_rail_count(p) == 0);
        assert(system.get_terminal_count(p) == 0);
    }

    printf("  PASS: Construction initializes empty state\n");
}

void test_priority() {
    printf("Testing RailSystem priority...\n");

    RailSystem system(64, 64);

    assert(system.get_priority() == 47);
    assert(RailSystem::TICK_PRIORITY == 47);

    printf("  PASS: Priority is 47\n");
}

void test_max_players() {
    printf("Testing MAX_PLAYERS constant...\n");

    assert(RailSystem::MAX_PLAYERS == 4);

    printf("  PASS: MAX_PLAYERS is 4\n");
}

void test_place_rail() {
    printf("Testing rail placement...\n");

    RailSystem system(64, 64);

    uint32_t id = system.place_rail(10, 20, RailType::SurfaceRail, 0);
    assert(id != 0);
    assert(system.get_rail_count(0) == 1);

    printf("  PASS: Rail placed successfully\n");
}

void test_place_rail_unique_ids() {
    printf("Testing rail placement returns unique IDs...\n");

    RailSystem system(64, 64);

    uint32_t id1 = system.place_rail(10, 20, RailType::SurfaceRail, 0);
    uint32_t id2 = system.place_rail(11, 20, RailType::ElevatedRail, 0);
    uint32_t id3 = system.place_rail(12, 20, RailType::SubterraRail, 1);

    assert(id1 != 0);
    assert(id2 != 0);
    assert(id3 != 0);
    assert(id1 != id2);
    assert(id2 != id3);
    assert(id1 != id3);

    printf("  PASS: Unique IDs assigned\n");
}

void test_place_rail_per_player() {
    printf("Testing per-player rail counting...\n");

    RailSystem system(64, 64);

    system.place_rail(10, 10, RailType::SurfaceRail, 0);
    system.place_rail(11, 10, RailType::SurfaceRail, 0);
    system.place_rail(20, 20, RailType::SurfaceRail, 1);

    assert(system.get_rail_count(0) == 2);
    assert(system.get_rail_count(1) == 1);
    assert(system.get_rail_count(2) == 0);
    assert(system.get_rail_count(3) == 0);

    printf("  PASS: Per-player rail counts correct\n");
}

void test_place_rail_invalid_owner() {
    printf("Testing rail placement with invalid owner...\n");

    RailSystem system(64, 64);

    uint32_t id = system.place_rail(10, 10, RailType::SurfaceRail, 5);  // Invalid owner
    assert(id == 0);

    printf("  PASS: Invalid owner returns 0\n");
}

void test_place_rail_out_of_bounds() {
    printf("Testing rail placement out of bounds...\n");

    RailSystem system(64, 64);

    // Negative coordinates
    assert(system.place_rail(-1, 10, RailType::SurfaceRail, 0) == 0);
    assert(system.place_rail(10, -1, RailType::SurfaceRail, 0) == 0);

    // Beyond map dimensions
    assert(system.place_rail(64, 10, RailType::SurfaceRail, 0) == 0);
    assert(system.place_rail(10, 64, RailType::SurfaceRail, 0) == 0);

    // Edge of map (should succeed)
    assert(system.place_rail(63, 63, RailType::SurfaceRail, 0) != 0);

    printf("  PASS: Out-of-bounds placements rejected\n");
}

void test_remove_rail() {
    printf("Testing rail removal...\n");

    RailSystem system(64, 64);

    uint32_t id = system.place_rail(10, 20, RailType::SurfaceRail, 0);
    assert(system.get_rail_count(0) == 1);

    bool removed = system.remove_rail(id, 0);
    assert(removed == true);
    assert(system.get_rail_count(0) == 0);

    printf("  PASS: Rail removed successfully\n");
}

void test_remove_rail_wrong_owner() {
    printf("Testing rail removal with wrong owner...\n");

    RailSystem system(64, 64);

    uint32_t id = system.place_rail(10, 20, RailType::SurfaceRail, 0);

    // Try to remove with wrong owner
    bool removed = system.remove_rail(id, 1);
    assert(removed == false);
    assert(system.get_rail_count(0) == 1);  // Still there

    printf("  PASS: Wrong owner cannot remove rail\n");
}

void test_remove_rail_nonexistent() {
    printf("Testing removal of nonexistent rail...\n");

    RailSystem system(64, 64);

    bool removed = system.remove_rail(999, 0);
    assert(removed == false);

    printf("  PASS: Nonexistent rail removal returns false\n");
}

void test_place_terminal() {
    printf("Testing terminal placement...\n");

    RailSystem system(64, 64);

    // Must place adjacent rail first (E7-034 validation)
    system.place_rail(10, 20, RailType::SurfaceRail, 0);
    uint32_t id = system.place_terminal(11, 20, TerminalType::SurfaceStation, 0);
    assert(id != 0);
    assert(system.get_terminal_count(0) == 1);

    printf("  PASS: Terminal placed successfully\n");
}

void test_place_terminal_types() {
    printf("Testing terminal placement with all types...\n");

    RailSystem system(64, 64);

    // Place adjacent rails first (E7-034 validation)
    system.place_rail(10, 10, RailType::SurfaceRail, 0);
    system.place_rail(20, 20, RailType::SurfaceRail, 0);
    system.place_rail(30, 30, RailType::SurfaceRail, 0);

    uint32_t id1 = system.place_terminal(11, 10, TerminalType::SurfaceStation, 0);
    uint32_t id2 = system.place_terminal(21, 20, TerminalType::SubterraStation, 0);
    uint32_t id3 = system.place_terminal(31, 30, TerminalType::IntermodalHub, 0);

    assert(id1 != 0);
    assert(id2 != 0);
    assert(id3 != 0);
    assert(system.get_terminal_count(0) == 3);

    printf("  PASS: All terminal types placed\n");
}

void test_place_terminal_invalid_owner() {
    printf("Testing terminal placement with invalid owner...\n");

    RailSystem system(64, 64);

    // Place rail so position would be valid for a valid owner
    system.place_rail(10, 10, RailType::SurfaceRail, 0);
    uint32_t id = system.place_terminal(11, 10, TerminalType::SurfaceStation, 5);
    assert(id == 0);

    printf("  PASS: Invalid owner returns 0\n");
}

void test_place_terminal_out_of_bounds() {
    printf("Testing terminal placement out of bounds...\n");

    RailSystem system(64, 64);

    assert(system.place_terminal(-1, 10, TerminalType::SurfaceStation, 0) == 0);
    assert(system.place_terminal(10, -1, TerminalType::SurfaceStation, 0) == 0);
    assert(system.place_terminal(64, 10, TerminalType::SurfaceStation, 0) == 0);
    assert(system.place_terminal(10, 64, TerminalType::SurfaceStation, 0) == 0);

    printf("  PASS: Out-of-bounds terminal placements rejected\n");
}

void test_remove_terminal() {
    printf("Testing terminal removal...\n");

    RailSystem system(64, 64);

    system.place_rail(10, 20, RailType::SurfaceRail, 0);
    uint32_t id = system.place_terminal(11, 20, TerminalType::SurfaceStation, 0);
    assert(system.get_terminal_count(0) == 1);

    bool removed = system.remove_terminal(id, 0);
    assert(removed == true);
    assert(system.get_terminal_count(0) == 0);

    printf("  PASS: Terminal removed successfully\n");
}

void test_remove_terminal_wrong_owner() {
    printf("Testing terminal removal with wrong owner...\n");

    RailSystem system(64, 64);

    system.place_rail(10, 20, RailType::SurfaceRail, 0);
    uint32_t id = system.place_terminal(11, 20, TerminalType::SurfaceStation, 0);

    bool removed = system.remove_terminal(id, 1);
    assert(removed == false);
    assert(system.get_terminal_count(0) == 1);

    printf("  PASS: Wrong owner cannot remove terminal\n");
}

void test_is_rail_powered_default() {
    printf("Testing rail powered state (default stub)...\n");

    RailSystem system(64, 64);

    uint32_t id = system.place_rail(10, 20, RailType::SurfaceRail, 0);

    // After tick, rail should be powered (stub: all powered)
    system.tick(0.0f);
    assert(system.is_rail_powered(id) == true);

    printf("  PASS: Rail is powered by default (stub)\n");
}

void test_is_rail_powered_nonexistent() {
    printf("Testing powered query for nonexistent rail...\n");

    RailSystem system(64, 64);

    assert(system.is_rail_powered(999) == false);

    printf("  PASS: Nonexistent rail returns not powered\n");
}

void test_is_terminal_active_after_tick() {
    printf("Testing terminal active state after tick...\n");

    RailSystem system(64, 64);

    // Place adjacent rail (required for terminal placement and activation)
    system.place_rail(10, 20, RailType::SurfaceRail, 0);
    uint32_t id = system.place_terminal(11, 20, TerminalType::SurfaceStation, 0);
    assert(id != 0);

    // Before tick, terminal is not active
    // (it starts inactive and gets activated by tick)
    system.tick(0.0f);

    // After tick, terminal should be active (powered via fallback + adjacent rail)
    assert(system.is_terminal_active(id) == true);

    printf("  PASS: Terminal is active after tick\n");
}

void test_is_terminal_active_nonexistent() {
    printf("Testing active query for nonexistent terminal...\n");

    RailSystem system(64, 64);

    assert(system.is_terminal_active(999) == false);

    printf("  PASS: Nonexistent terminal returns not active\n");
}

void test_terminal_coverage_radius() {
    printf("Testing terminal coverage radius query...\n");

    RailSystem system(64, 64);

    system.place_rail(10, 20, RailType::SurfaceRail, 0);
    uint32_t id = system.place_terminal(11, 20, TerminalType::SurfaceStation, 0);
    assert(id != 0);

    // Default coverage radius is 8
    uint8_t radius = system.get_terminal_coverage_radius(id);
    assert(radius == 8);

    printf("  PASS: Terminal coverage radius is 8\n");
}

void test_terminal_coverage_radius_nonexistent() {
    printf("Testing coverage radius for nonexistent terminal...\n");

    RailSystem system(64, 64);

    uint8_t radius = system.get_terminal_coverage_radius(999);
    assert(radius == 0);

    printf("  PASS: Nonexistent terminal returns 0 radius\n");
}

void test_get_counts_invalid_owner() {
    printf("Testing count queries with invalid owner...\n");

    RailSystem system(64, 64);

    assert(system.get_rail_count(5) == 0);
    assert(system.get_terminal_count(5) == 0);

    printf("  PASS: Invalid owner returns 0 counts\n");
}

void test_tick_runs_phases() {
    printf("Testing tick executes all phases...\n");

    RailSystem system(64, 64);

    // Place some rails and a terminal adjacent to a rail
    system.place_rail(10, 10, RailType::SurfaceRail, 0);
    system.place_rail(11, 10, RailType::SurfaceRail, 0);
    system.place_terminal(10, 11, TerminalType::SurfaceStation, 0);

    // Tick should not crash
    system.tick(1.0f / 60.0f);
    system.tick(1.0f / 60.0f);
    system.tick(1.0f / 60.0f);

    // Verify state is consistent after multiple ticks
    assert(system.get_rail_count(0) == 2);
    assert(system.get_terminal_count(0) == 1);

    printf("  PASS: Tick runs without errors\n");
}

void test_set_energy_provider() {
    printf("Testing set_energy_provider...\n");

    RailSystem system(64, 64);

    // Should not crash with nullptr
    system.set_energy_provider(nullptr);

    // Should still work after setting provider back to nullptr
    system.place_rail(10, 10, RailType::SurfaceRail, 0);
    system.tick(0.0f);

    printf("  PASS: Energy provider can be set\n");
}

void test_mixed_entity_ids() {
    printf("Testing mixed rail and terminal entity IDs...\n");

    RailSystem system(64, 64);

    // Rails and terminals share the same ID space
    uint32_t rail_id = system.place_rail(10, 10, RailType::SurfaceRail, 0);
    // Place rail adjacent to terminal position for E7-034 validation
    system.place_rail(20, 20, RailType::SurfaceRail, 0);
    uint32_t term_id = system.place_terminal(21, 20, TerminalType::SurfaceStation, 0);

    assert(rail_id != 0);
    assert(term_id != 0);
    assert(rail_id != term_id);

    printf("  PASS: Rails and terminals have unique IDs\n");
}

void test_multiple_players() {
    printf("Testing multiple player operations...\n");

    RailSystem system(64, 64);

    // Each player places rails and terminals (terminal adjacent to rail)
    for (uint8_t p = 0; p < RailSystem::MAX_PLAYERS; ++p) {
        system.place_rail(p * 10, 10, RailType::SurfaceRail, p);
        system.place_terminal(p * 10 + 1, 10, TerminalType::SurfaceStation, p);
    }

    // Verify per-player counts
    for (uint8_t p = 0; p < RailSystem::MAX_PLAYERS; ++p) {
        assert(system.get_rail_count(p) == 1);
        assert(system.get_terminal_count(p) == 1);
    }

    // Tick should handle all players
    system.tick(0.0f);

    printf("  PASS: Multiple players work correctly\n");
}

int main() {
    printf("=== RailSystem Unit Tests (Epic 7, Ticket E7-032) ===\n\n");

    test_construction();
    test_priority();
    test_max_players();
    test_place_rail();
    test_place_rail_unique_ids();
    test_place_rail_per_player();
    test_place_rail_invalid_owner();
    test_place_rail_out_of_bounds();
    test_remove_rail();
    test_remove_rail_wrong_owner();
    test_remove_rail_nonexistent();
    test_place_terminal();
    test_place_terminal_types();
    test_place_terminal_invalid_owner();
    test_place_terminal_out_of_bounds();
    test_remove_terminal();
    test_remove_terminal_wrong_owner();
    test_is_rail_powered_default();
    test_is_rail_powered_nonexistent();
    test_is_terminal_active_after_tick();
    test_is_terminal_active_nonexistent();
    test_terminal_coverage_radius();
    test_terminal_coverage_radius_nonexistent();
    test_get_counts_invalid_owner();
    test_tick_runs_phases();
    test_set_energy_provider();
    test_mixed_entity_ids();
    test_multiple_players();

    printf("\n=== All RailSystem Tests Passed ===\n");
    return 0;
}
