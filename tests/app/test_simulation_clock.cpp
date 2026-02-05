/**
 * @file test_simulation_clock.cpp
 * @brief Unit tests for SimulationClock.
 */

#include "sims3000/app/SimulationClock.h"
#include <cassert>
#include <cstdio>
#include <cmath>

using namespace sims3000;

void test_initial_state() {
    printf("Testing initial state...\n");

    SimulationClock clock;

    assert(clock.getCurrentTick() == 0);
    assert(std::abs(clock.getTickDelta() - 0.05f) < 0.0001f);
    assert(clock.getInterpolation() >= 0.0f);
    assert(clock.getTotalTime() == 0.0);
    assert(!clock.isPaused());

    printf("  PASS: Initial state correct\n");
}

void test_tick_accumulation() {
    printf("Testing tick accumulation...\n");

    SimulationClock clock;

    // Less than one tick
    int ticks = clock.accumulate(0.03f);  // 30ms
    assert(ticks == 0);
    assert(clock.getCurrentTick() == 0);

    // Accumulate to one tick
    ticks = clock.accumulate(0.03f);  // 60ms total
    assert(ticks == 1);

    // Advance the tick
    clock.advanceTick();
    assert(clock.getCurrentTick() == 1);

    // Multiple ticks at once
    ticks = clock.accumulate(0.15f);  // 150ms
    assert(ticks == 3);

    for (int i = 0; i < 3; ++i) {
        clock.advanceTick();
    }
    assert(clock.getCurrentTick() == 4);

    printf("  PASS: Tick accumulation works correctly\n");
}

void test_interpolation() {
    printf("Testing interpolation...\n");

    SimulationClock clock;

    // At tick boundary
    clock.accumulate(0.05f);  // Exactly one tick
    clock.advanceTick();
    // Interpolation should be close to 0 after consuming the tick
    // (accumulator is now empty)
    assert(clock.getInterpolation() >= 0.0f && clock.getInterpolation() <= 1.0f);

    // Halfway through tick
    clock.accumulate(0.025f);  // 25ms = half a tick
    float interp = clock.getInterpolation();
    assert(interp > 0.4f && interp < 0.6f);

    printf("  PASS: Interpolation works correctly\n");
}

void test_pause() {
    printf("Testing pause functionality...\n");

    SimulationClock clock;

    // Accumulate some time (0.1f = 100ms = 2 ticks worth)
    int ticks = clock.accumulate(0.1f);
    assert(ticks == 2);  // Should have 2 ticks

    // Reset and pause
    clock.reset();
    clock.setPaused(true);
    assert(clock.isPaused());

    // Should return 0 ticks when paused
    ticks = clock.accumulate(0.5f);
    assert(ticks == 0);

    // Unpause
    clock.setPaused(false);
    assert(!clock.isPaused());

    // Now should accumulate normally
    ticks = clock.accumulate(0.1f);
    assert(ticks == 2);

    printf("  PASS: Pause works correctly\n");
}

void test_reset() {
    printf("Testing reset...\n");

    SimulationClock clock;

    // Advance clock
    for (int i = 0; i < 10; ++i) {
        clock.accumulate(0.05f);
        clock.advanceTick();
    }
    assert(clock.getCurrentTick() == 10);

    // Reset
    clock.reset();
    assert(clock.getCurrentTick() == 0);
    assert(clock.getTotalTime() == 0.0);
    assert(!clock.isPaused());

    printf("  PASS: Reset works correctly\n");
}

void test_accumulator_cap() {
    printf("Testing accumulator cap...\n");

    SimulationClock clock;

    // Simulate a long pause (e.g., breakpoint in debugger)
    int ticks = clock.accumulate(10.0f);  // 10 seconds

    // Should be capped to prevent spiral of death
    // MAX_ACCUMULATOR is 0.25s = 5 ticks max
    assert(ticks <= 5);

    printf("  PASS: Accumulator cap works correctly\n");
}

void test_total_time() {
    printf("Testing total time calculation...\n");

    SimulationClock clock;

    // 20 ticks = 1 second
    for (int i = 0; i < 20; ++i) {
        clock.accumulate(0.05f);
        clock.advanceTick();
    }

    double totalTime = clock.getTotalTime();
    assert(std::abs(totalTime - 1.0) < 0.001);

    printf("  PASS: Total time calculation correct\n");
}

int main() {
    printf("=== SimulationClock Unit Tests ===\n\n");

    test_initial_state();
    test_tick_accumulation();
    test_interpolation();
    test_pause();
    test_reset();
    test_accumulator_cap();
    test_total_time();

    printf("\n=== All tests passed! ===\n");
    return 0;
}
