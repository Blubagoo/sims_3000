/**
 * @file test_time_progression.cpp
 * @brief Tests for time progression tracking (Ticket E10-003)
 *
 * Verifies:
 * - Initial cycle = 0, phase = 0
 * - After TICKS_PER_PHASE ticks, phase increments
 * - After TICKS_PER_CYCLE ticks, cycle increments
 * - Phase wraps: 0 -> 1 -> 2 -> 3 -> 0
 * - Constants are correct: TICKS_PER_PHASE=500, PHASES_PER_CYCLE=4, TICKS_PER_CYCLE=2000
 */

#include "sims3000/sim/SimulationCore.h"
#include <cassert>
#include <cstdio>

using namespace sims3000;
using namespace sims3000::sim;

// Helper: advance the core by N ticks (at Normal speed)
static void advance_ticks(SimulationCore& core, uint64_t n) {
    for (uint64_t i = 0; i < n; ++i) {
        core.update(SIMULATION_TICK_DELTA);
    }
}

// =========================================================================
// Test: Constants are correct
// =========================================================================

void test_constants() {
    printf("Testing time progression constants...\n");

    assert(SimulationCore::TICKS_PER_PHASE == 500);
    assert(SimulationCore::PHASES_PER_CYCLE == 4);
    assert(SimulationCore::TICKS_PER_CYCLE == 2000);

    printf("  PASS: Constants are correct\n");
}

// =========================================================================
// Test: Initial state
// =========================================================================

void test_initial_state() {
    printf("Testing initial cycle/phase state...\n");

    SimulationCore core;
    assert(core.get_current_cycle() == 0);
    assert(core.get_current_phase() == 0);

    printf("  PASS: Initial cycle=0, phase=0\n");
}

// =========================================================================
// Test: Phase increments after TICKS_PER_PHASE ticks
// =========================================================================

void test_phase_increments() {
    printf("Testing phase increments after TICKS_PER_PHASE ticks...\n");

    SimulationCore core;

    // Advance to tick 499 (still phase 0)
    advance_ticks(core, 499);
    assert(core.getCurrentTick() == 499);
    assert(core.get_current_phase() == 0);

    // Tick 500 -> phase 1
    advance_ticks(core, 1);
    assert(core.getCurrentTick() == 500);
    assert(core.get_current_phase() == 1);

    printf("  PASS: Phase increments at TICKS_PER_PHASE boundary\n");
}

// =========================================================================
// Test: Phase wraps 0 -> 1 -> 2 -> 3 -> 0
// =========================================================================

void test_phase_wraps() {
    printf("Testing phase wraps 0 -> 1 -> 2 -> 3 -> 0...\n");

    SimulationCore core;

    // Phase 0: ticks 0-499
    assert(core.get_current_phase() == 0);

    // Phase 1: ticks 500-999
    advance_ticks(core, 500);
    assert(core.get_current_phase() == 1);

    // Phase 2: ticks 1000-1499
    advance_ticks(core, 500);
    assert(core.get_current_phase() == 2);

    // Phase 3: ticks 1500-1999
    advance_ticks(core, 500);
    assert(core.get_current_phase() == 3);

    // Phase 0 again: tick 2000
    advance_ticks(core, 500);
    assert(core.get_current_phase() == 0);
    assert(core.getCurrentTick() == 2000);

    printf("  PASS: Phase wraps correctly\n");
}

// =========================================================================
// Test: Cycle increments after TICKS_PER_CYCLE ticks
// =========================================================================

void test_cycle_increments() {
    printf("Testing cycle increments after TICKS_PER_CYCLE ticks...\n");

    SimulationCore core;

    // Cycle 0 at start
    assert(core.get_current_cycle() == 0);

    // Advance to tick 1999 (still cycle 0)
    advance_ticks(core, 1999);
    assert(core.get_current_cycle() == 0);

    // Tick 2000 -> cycle 1
    advance_ticks(core, 1);
    assert(core.getCurrentTick() == 2000);
    assert(core.get_current_cycle() == 1);

    // Tick 4000 -> cycle 2
    advance_ticks(core, 2000);
    assert(core.getCurrentTick() == 4000);
    assert(core.get_current_cycle() == 2);

    printf("  PASS: Cycle increments at TICKS_PER_CYCLE boundary\n");
}

// =========================================================================
// Test: Cycle and phase together
// =========================================================================

void test_cycle_and_phase_together() {
    printf("Testing cycle and phase relationship...\n");

    SimulationCore core;

    // Advance to tick 2500 = cycle 1, phase 1
    advance_ticks(core, 2500);
    assert(core.getCurrentTick() == 2500);
    assert(core.get_current_cycle() == 1);
    assert(core.get_current_phase() == 1);

    // Advance to tick 3500 = cycle 1, phase 3
    advance_ticks(core, 1000);
    assert(core.getCurrentTick() == 3500);
    assert(core.get_current_cycle() == 1);
    assert(core.get_current_phase() == 3);

    // Advance to tick 4000 = cycle 2, phase 0
    advance_ticks(core, 500);
    assert(core.getCurrentTick() == 4000);
    assert(core.get_current_cycle() == 2);
    assert(core.get_current_phase() == 0);

    printf("  PASS: Cycle and phase are consistent\n");
}

// =========================================================================
// Main
// =========================================================================

int main() {
    printf("=== Time Progression Tests (E10-003) ===\n\n");

    test_constants();
    test_initial_state();
    test_phase_increments();
    test_phase_wraps();
    test_cycle_increments();
    test_cycle_and_phase_together();

    printf("\n=== All Time Progression tests passed ===\n");
    return 0;
}
