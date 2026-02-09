/**
 * @file test_simulation_events.cpp
 * @brief Tests for simulation events (Ticket E10-005)
 *
 * Verifies:
 * - TickStartEvent struct has tick_number and delta_time
 * - TickCompleteEvent struct has tick_number and delta_time
 * - PhaseChangedEvent struct has cycle, new_phase, old_phase
 * - CycleChangedEvent struct has new_cycle, old_cycle
 * - After ticking, get_last_tick_start/complete return correct values
 */

#include "sims3000/sim/SimulationCore.h"
#include "sims3000/sim/SimulationEvents.h"
#include <cassert>
#include <cmath>
#include <cstdio>

using namespace sims3000;
using namespace sims3000::sim;

// =========================================================================
// Test: TickStartEvent struct fields
// =========================================================================

void test_tick_start_event_struct() {
    printf("Testing TickStartEvent struct fields...\n");

    TickStartEvent event{42, 0.05f};
    assert(event.tick_number == 42);
    assert(event.delta_time == 0.05f);

    // Default-initialized
    TickStartEvent event2{};
    assert(event2.tick_number == 0);
    assert(event2.delta_time == 0.0f);

    printf("  PASS: TickStartEvent struct has correct fields\n");
}

// =========================================================================
// Test: TickCompleteEvent struct fields
// =========================================================================

void test_tick_complete_event_struct() {
    printf("Testing TickCompleteEvent struct fields...\n");

    TickCompleteEvent event{99, 0.05f};
    assert(event.tick_number == 99);
    assert(event.delta_time == 0.05f);

    // Default-initialized
    TickCompleteEvent event2{};
    assert(event2.tick_number == 0);
    assert(event2.delta_time == 0.0f);

    printf("  PASS: TickCompleteEvent struct has correct fields\n");
}

// =========================================================================
// Test: PhaseChangedEvent struct fields
// =========================================================================

void test_phase_changed_event_struct() {
    printf("Testing PhaseChangedEvent struct fields...\n");

    PhaseChangedEvent event{3, 2, 1};
    assert(event.cycle == 3);
    assert(event.new_phase == 2);
    assert(event.old_phase == 1);

    printf("  PASS: PhaseChangedEvent struct has correct fields\n");
}

// =========================================================================
// Test: CycleChangedEvent struct fields
// =========================================================================

void test_cycle_changed_event_struct() {
    printf("Testing CycleChangedEvent struct fields...\n");

    CycleChangedEvent event{5, 4};
    assert(event.new_cycle == 5);
    assert(event.old_cycle == 4);

    printf("  PASS: CycleChangedEvent struct has correct fields\n");
}

// =========================================================================
// Test: Initial event state (no ticks yet)
// =========================================================================

void test_initial_event_state() {
    printf("Testing initial event state before any ticks...\n");

    SimulationCore core;

    TickStartEvent start = core.get_last_tick_start();
    assert(start.tick_number == 0);
    assert(start.delta_time == 0.0f);

    TickCompleteEvent complete = core.get_last_tick_complete();
    assert(complete.tick_number == 0);
    assert(complete.delta_time == 0.0f);

    printf("  PASS: Initial events are zero-initialized\n");
}

// =========================================================================
// Test: After one tick, events reflect tick 1
// =========================================================================

void test_events_after_one_tick() {
    printf("Testing events after one tick...\n");

    SimulationCore core;
    core.update(SIMULATION_TICK_DELTA);  // triggers tick 1

    TickStartEvent start = core.get_last_tick_start();
    assert(start.tick_number == 1);
    assert(start.delta_time == SIMULATION_TICK_DELTA);

    TickCompleteEvent complete = core.get_last_tick_complete();
    assert(complete.tick_number == 1);
    assert(complete.delta_time == SIMULATION_TICK_DELTA);

    printf("  PASS: Events reflect tick 1 after one tick\n");
}

// =========================================================================
// Test: After multiple ticks, events reflect the last tick
// =========================================================================

void test_events_after_multiple_ticks() {
    printf("Testing events after multiple ticks...\n");

    SimulationCore core;

    // 0.15s = 3 ticks at Normal speed
    core.update(0.15f);
    assert(core.getCurrentTick() == 3);

    TickStartEvent start = core.get_last_tick_start();
    assert(start.tick_number == 3);
    assert(start.delta_time == SIMULATION_TICK_DELTA);

    TickCompleteEvent complete = core.get_last_tick_complete();
    assert(complete.tick_number == 3);
    assert(complete.delta_time == SIMULATION_TICK_DELTA);

    printf("  PASS: Events reflect last tick after multiple ticks\n");
}

// =========================================================================
// Test: Events not updated when no tick fires
// =========================================================================

void test_events_no_update_on_sub_tick() {
    printf("Testing events not updated when no tick fires...\n");

    SimulationCore core;

    // Fire one tick
    core.update(SIMULATION_TICK_DELTA);
    assert(core.get_last_tick_start().tick_number == 1);

    // Sub-tick update - no new tick fires
    core.update(0.01f);
    assert(core.getCurrentTick() == 1);  // still tick 1

    // Events should still show tick 1
    assert(core.get_last_tick_start().tick_number == 1);
    assert(core.get_last_tick_complete().tick_number == 1);

    printf("  PASS: Events not updated on sub-tick update\n");
}

// =========================================================================
// Test: Events update sequentially through ticks
// =========================================================================

void test_events_sequential() {
    printf("Testing events update sequentially...\n");

    SimulationCore core;

    core.update(SIMULATION_TICK_DELTA);
    assert(core.get_last_tick_start().tick_number == 1);
    assert(core.get_last_tick_complete().tick_number == 1);

    core.update(SIMULATION_TICK_DELTA);
    assert(core.get_last_tick_start().tick_number == 2);
    assert(core.get_last_tick_complete().tick_number == 2);

    core.update(SIMULATION_TICK_DELTA);
    assert(core.get_last_tick_start().tick_number == 3);
    assert(core.get_last_tick_complete().tick_number == 3);

    printf("  PASS: Events update sequentially\n");
}

// =========================================================================
// Main
// =========================================================================

int main() {
    printf("=== Simulation Events Tests (E10-005) ===\n\n");

    test_tick_start_event_struct();
    test_tick_complete_event_struct();
    test_phase_changed_event_struct();
    test_cycle_changed_event_struct();
    test_initial_event_state();
    test_events_after_one_tick();
    test_events_after_multiple_ticks();
    test_events_no_update_on_sub_tick();
    test_events_sequential();

    printf("\n=== All Simulation Events tests passed ===\n");
    return 0;
}
