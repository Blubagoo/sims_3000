/**
 * @file test_simulation_speed.cpp
 * @brief Tests for simulation speed control (Ticket E10-002)
 *
 * Verifies:
 * - Default speed is Normal
 * - set_speed/get_speed round-trip
 * - Speed multipliers: Paused=0, Normal=1, Fast=2, Fastest=3
 * - When paused: update(1.0f) produces no ticks
 * - When Fast: ticks accumulate 2x faster
 * - When Fastest: ticks accumulate 3x faster
 * - is_paused() reflects state
 */

#include "sims3000/sim/SimulationCore.h"
#include "sims3000/sim/SimulationSpeed.h"
#include <cassert>
#include <cmath>
#include <cstdio>

using namespace sims3000;
using namespace sims3000::sim;

// =========================================================================
// Test: Default speed is Normal
// =========================================================================

void test_default_speed() {
    printf("Testing default speed is Normal...\n");

    SimulationCore core;
    assert(core.get_speed() == SimulationSpeed::Normal);
    assert(!core.is_paused());
    assert(core.get_speed_multiplier() == 1.0f);

    printf("  PASS: Default speed is Normal\n");
}

// =========================================================================
// Test: set_speed/get_speed round-trip
// =========================================================================

void test_speed_round_trip() {
    printf("Testing set_speed/get_speed round-trip...\n");

    SimulationCore core;

    core.set_speed(SimulationSpeed::Paused);
    assert(core.get_speed() == SimulationSpeed::Paused);

    core.set_speed(SimulationSpeed::Normal);
    assert(core.get_speed() == SimulationSpeed::Normal);

    core.set_speed(SimulationSpeed::Fast);
    assert(core.get_speed() == SimulationSpeed::Fast);

    core.set_speed(SimulationSpeed::Fastest);
    assert(core.get_speed() == SimulationSpeed::Fastest);

    printf("  PASS: set_speed/get_speed round-trip works\n");
}

// =========================================================================
// Test: Speed multipliers
// =========================================================================

void test_speed_multipliers() {
    printf("Testing speed multipliers...\n");

    SimulationCore core;

    core.set_speed(SimulationSpeed::Paused);
    assert(core.get_speed_multiplier() == 0.0f);

    core.set_speed(SimulationSpeed::Normal);
    assert(core.get_speed_multiplier() == 1.0f);

    core.set_speed(SimulationSpeed::Fast);
    assert(core.get_speed_multiplier() == 2.0f);

    core.set_speed(SimulationSpeed::Fastest);
    assert(core.get_speed_multiplier() == 3.0f);

    printf("  PASS: Speed multipliers are correct\n");
}

// =========================================================================
// Test: Paused produces no ticks
// =========================================================================

void test_paused_no_ticks() {
    printf("Testing paused produces no ticks...\n");

    SimulationCore core;
    core.set_speed(SimulationSpeed::Paused);

    // Even a large delta should produce no ticks when paused
    core.update(1.0f);
    assert(core.getCurrentTick() == 0);

    core.update(10.0f);
    assert(core.getCurrentTick() == 0);

    printf("  PASS: Paused produces no ticks\n");
}

// =========================================================================
// Test: Fast speed accumulates 2x faster
// =========================================================================

void test_fast_speed() {
    printf("Testing Fast speed accumulates 2x faster...\n");

    SimulationCore core;
    core.set_speed(SimulationSpeed::Fast);

    // At Normal speed, 0.05s = 1 tick.
    // At Fast (2x), 0.05s * 2 = 0.1s effective = 2 ticks.
    core.update(0.05f);
    assert(core.getCurrentTick() == 2);

    // 0.1s * 2 = 0.2s effective = 4 more ticks (total 6)
    core.update(0.1f);
    assert(core.getCurrentTick() == 6);

    printf("  PASS: Fast speed accumulates 2x\n");
}

// =========================================================================
// Test: Fastest speed accumulates 3x faster
// =========================================================================

void test_fastest_speed() {
    printf("Testing Fastest speed accumulates 3x faster...\n");

    SimulationCore core;
    core.set_speed(SimulationSpeed::Fastest);

    // At Fastest (3x), 0.05s * 3 = 0.15s effective = 3 ticks.
    core.update(0.05f);
    assert(core.getCurrentTick() == 3);

    // 0.1s * 3 = 0.3s effective = 6 more ticks (total 9)
    core.update(0.1f);
    assert(core.getCurrentTick() == 9);

    printf("  PASS: Fastest speed accumulates 3x\n");
}

// =========================================================================
// Test: is_paused reflects state
// =========================================================================

void test_is_paused() {
    printf("Testing is_paused reflects state...\n");

    SimulationCore core;

    assert(!core.is_paused());

    core.set_speed(SimulationSpeed::Paused);
    assert(core.is_paused());

    core.set_speed(SimulationSpeed::Normal);
    assert(!core.is_paused());

    core.set_speed(SimulationSpeed::Fast);
    assert(!core.is_paused());

    core.set_speed(SimulationSpeed::Fastest);
    assert(!core.is_paused());

    printf("  PASS: is_paused reflects state correctly\n");
}

// =========================================================================
// Test: Speed change mid-simulation
// =========================================================================

void test_speed_change_mid_simulation() {
    printf("Testing speed change mid-simulation...\n");

    SimulationCore core;

    // Normal speed: 0.05s = 1 tick
    core.update(0.05f);
    assert(core.getCurrentTick() == 1);

    // Switch to Fast: 0.05s * 2 = 0.1s effective = 2 ticks (total 3)
    core.set_speed(SimulationSpeed::Fast);
    core.update(0.05f);
    assert(core.getCurrentTick() == 3);

    // Pause: no more ticks
    core.set_speed(SimulationSpeed::Paused);
    core.update(1.0f);
    assert(core.getCurrentTick() == 3);

    // Resume at normal: 0.05s = 1 tick (total 4)
    core.set_speed(SimulationSpeed::Normal);
    core.update(0.05f);
    assert(core.getCurrentTick() == 4);

    printf("  PASS: Speed change mid-simulation works\n");
}

// =========================================================================
// Main
// =========================================================================

int main() {
    printf("=== Simulation Speed Tests (E10-002) ===\n\n");

    test_default_speed();
    test_speed_round_trip();
    test_speed_multipliers();
    test_paused_no_ticks();
    test_fast_speed();
    test_fastest_speed();
    test_is_paused();
    test_speed_change_mid_simulation();

    printf("\n=== All Simulation Speed tests passed ===\n");
    return 0;
}
