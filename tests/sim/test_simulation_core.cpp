/**
 * @file test_simulation_core.cpp
 * @brief Tests for SimulationCore tick scheduler (Ticket E10-001)
 *
 * Verifies:
 * - SimulationCore creation with zero initial state
 * - Register/unregister systems at runtime
 * - Accumulator pattern: multiple small updates accumulate to trigger tick
 * - Systems execute in priority order (lower = earlier)
 * - ISimulationTime: getCurrentTick, getTickDelta, getTotalTime
 * - Interpolation between ticks
 * - Multiple ticks fire when delta is large
 * - No ticks fire when accumulated time is below threshold
 */

#include "sims3000/sim/SimulationCore.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

using namespace sims3000;
using namespace sims3000::sim;

// =========================================================================
// Test helper: Mock ISimulatable system
// =========================================================================

class MockSystem : public ISimulatable {
public:
    MockSystem(int priority, const char* name)
        : m_priority(priority), m_name(name) {}

    void tick(const ISimulationTime& time) override {
        m_tick_count++;
        m_last_tick = time.getCurrentTick();
        if (s_execution_log) {
            s_execution_log->push_back(m_name);
        }
    }

    int getPriority() const override { return m_priority; }
    const char* getName() const override { return m_name; }

    int m_tick_count = 0;
    SimulationTick m_last_tick = 0;

    // Shared execution log for ordering tests
    static std::vector<const char*>* s_execution_log;

private:
    int m_priority;
    const char* m_name;
};

std::vector<const char*>* MockSystem::s_execution_log = nullptr;

// =========================================================================
// Test: Initial state
// =========================================================================

void test_initial_state() {
    printf("Testing SimulationCore initial state...\n");

    SimulationCore core;

    assert(core.system_count() == 0);
    assert(core.getCurrentTick() == 0);
    assert(core.getTickDelta() == SIMULATION_TICK_DELTA);
    assert(core.getTotalTime() == 0.0);
    assert(core.getInterpolation() == 0.0f);

    printf("  PASS: Initial state correct\n");
}

// =========================================================================
// Test: Register and unregister systems
// =========================================================================

void test_register_unregister() {
    printf("Testing register/unregister systems...\n");

    SimulationCore core;
    MockSystem sys1(10, "System1");
    MockSystem sys2(20, "System2");

    core.register_system(&sys1);
    assert(core.system_count() == 1);

    core.register_system(&sys2);
    assert(core.system_count() == 2);

    core.unregister_system(&sys1);
    assert(core.system_count() == 1);

    core.unregister_system(&sys2);
    assert(core.system_count() == 0);

    // Unregistering a system not in the list is a no-op
    core.unregister_system(&sys1);
    assert(core.system_count() == 0);

    // Registering nullptr is a no-op
    core.register_system(nullptr);
    assert(core.system_count() == 0);

    printf("  PASS: Register/unregister works correctly\n");
}

// =========================================================================
// Test: Accumulator pattern - no tick below threshold
// =========================================================================

void test_accumulator_no_tick() {
    printf("Testing accumulator - no tick below threshold...\n");

    SimulationCore core;
    MockSystem sys(10, "Sys");
    core.register_system(&sys);

    // 20ms is below 50ms threshold
    core.update(0.02f);
    assert(sys.m_tick_count == 0);
    assert(core.getCurrentTick() == 0);

    // Another 20ms (total 40ms) still below threshold
    core.update(0.02f);
    assert(sys.m_tick_count == 0);
    assert(core.getCurrentTick() == 0);

    printf("  PASS: No tick fires below threshold\n");
}

// =========================================================================
// Test: Accumulator pattern - tick fires at threshold
// =========================================================================

void test_accumulator_tick_fires() {
    printf("Testing accumulator - tick fires at threshold...\n");

    SimulationCore core;
    MockSystem sys(10, "Sys");
    core.register_system(&sys);

    // Exactly 50ms should trigger one tick
    core.update(0.05f);
    assert(sys.m_tick_count == 1);
    assert(core.getCurrentTick() == 1);

    // Another 50ms
    core.update(0.05f);
    assert(sys.m_tick_count == 2);
    assert(core.getCurrentTick() == 2);

    printf("  PASS: Tick fires at threshold\n");
}

// =========================================================================
// Test: Accumulator pattern - multiple ticks on large delta
// =========================================================================

void test_accumulator_multiple_ticks() {
    printf("Testing accumulator - multiple ticks on large delta...\n");

    SimulationCore core;
    MockSystem sys(10, "Sys");
    core.register_system(&sys);

    // 150ms should trigger 3 ticks (150ms / 50ms = 3)
    core.update(0.15f);
    assert(sys.m_tick_count == 3);
    assert(core.getCurrentTick() == 3);

    printf("  PASS: Multiple ticks fire on large delta\n");
}

// =========================================================================
// Test: Accumulator pattern - accumulates across updates
// =========================================================================

void test_accumulator_carries_over() {
    printf("Testing accumulator - carries over remainder...\n");

    SimulationCore core;
    MockSystem sys(10, "Sys");
    core.register_system(&sys);

    // 25ms - no tick
    core.update(0.025f);
    assert(sys.m_tick_count == 0);

    // 25ms more (total 50ms) - should trigger 1 tick, 0ms remainder
    core.update(0.025f);
    assert(sys.m_tick_count == 1);
    assert(core.getCurrentTick() == 1);

    // 25ms - no tick (remainder from above ~0 + 25ms = 25ms < 50ms)
    core.update(0.025f);
    assert(sys.m_tick_count == 1);

    // 25ms more (total 25ms + 25ms = 50ms) - should trigger 1 tick
    core.update(0.025f);
    assert(sys.m_tick_count == 2);
    assert(core.getCurrentTick() == 2);

    printf("  PASS: Accumulator carries over remainder\n");
}

// =========================================================================
// Test: Systems execute in priority order
// =========================================================================

void test_priority_order() {
    printf("Testing systems execute in priority order...\n");

    SimulationCore core;
    MockSystem sys_high(50, "HighPriority");
    MockSystem sys_low(10, "LowPriority");
    MockSystem sys_mid(30, "MidPriority");

    // Register out of order
    core.register_system(&sys_high);
    core.register_system(&sys_low);
    core.register_system(&sys_mid);

    std::vector<const char*> log;
    MockSystem::s_execution_log = &log;

    core.update(0.05f);

    assert(log.size() == 3);
    assert(std::string(log[0]) == "LowPriority");    // priority 10 first
    assert(std::string(log[1]) == "MidPriority");     // priority 30 second
    assert(std::string(log[2]) == "HighPriority");    // priority 50 third

    MockSystem::s_execution_log = nullptr;
    printf("  PASS: Systems execute in priority order\n");
}

// =========================================================================
// Test: ISimulationTime - getTickDelta always returns constant
// =========================================================================

void test_tick_delta_constant() {
    printf("Testing getTickDelta returns constant...\n");

    SimulationCore core;

    assert(core.getTickDelta() == SIMULATION_TICK_DELTA);
    assert(core.getTickDelta() == 0.05f);

    // After some ticks, still constant
    core.update(0.2f);
    assert(core.getTickDelta() == SIMULATION_TICK_DELTA);

    printf("  PASS: getTickDelta always returns 0.05f\n");
}

// =========================================================================
// Test: ISimulationTime - getCurrentTick increments correctly
// =========================================================================

void test_get_current_tick() {
    printf("Testing getCurrentTick increments...\n");

    SimulationCore core;
    assert(core.getCurrentTick() == 0);

    core.update(0.05f);
    assert(core.getCurrentTick() == 1);

    core.update(0.1f);
    assert(core.getCurrentTick() == 3);

    core.update(0.25f);
    assert(core.getCurrentTick() == 8);

    printf("  PASS: getCurrentTick increments correctly\n");
}

// =========================================================================
// Test: ISimulationTime - getTotalTime
// =========================================================================

void test_get_total_time() {
    printf("Testing getTotalTime...\n");

    SimulationCore core;
    assert(core.getTotalTime() == 0.0);

    core.update(0.05f);  // 1 tick
    double expected = 1.0 * SIMULATION_TICK_DELTA;
    assert(std::abs(core.getTotalTime() - expected) < 1e-9);

    core.update(0.1f);  // 2 more ticks (total 3)
    expected = 3.0 * SIMULATION_TICK_DELTA;
    assert(std::abs(core.getTotalTime() - expected) < 1e-9);

    printf("  PASS: getTotalTime matches tick count\n");
}

// =========================================================================
// Test: Interpolation between ticks
// =========================================================================

void test_interpolation() {
    printf("Testing interpolation between ticks...\n");

    SimulationCore core;

    // No time accumulated - interpolation should be 0
    assert(core.getInterpolation() == 0.0f);

    // 25ms = half a tick
    core.update(0.025f);
    float interp = core.getInterpolation();
    assert(std::abs(interp - 0.5f) < 0.01f);

    // Another 12.5ms (total 37.5ms = 75% of a tick)
    core.update(0.0125f);
    interp = core.getInterpolation();
    assert(std::abs(interp - 0.75f) < 0.01f);

    // Another 12.5ms (total 50ms = tick fires, remainder 0)
    core.update(0.0125f);
    assert(core.getCurrentTick() == 1);
    interp = core.getInterpolation();
    assert(std::abs(interp - 0.0f) < 0.01f);

    // 60ms = 1 tick fires, 10ms remainder (20% of next tick)
    core.update(0.06f);
    assert(core.getCurrentTick() == 2);
    interp = core.getInterpolation();
    assert(std::abs(interp - 0.2f) < 0.01f);

    printf("  PASS: Interpolation computed correctly\n");
}

// =========================================================================
// Test: System receives correct tick info during tick
// =========================================================================

void test_system_receives_time() {
    printf("Testing system receives correct ISimulationTime...\n");

    SimulationCore core;

    SimulationTick received_tick = 0;
    float received_delta = 0.0f;

    class TimeCapture : public ISimulatable {
    public:
        TimeCapture(SimulationTick& tick, float& delta)
            : m_tick(tick), m_delta(delta) {}

        void tick(const ISimulationTime& time) override {
            m_tick = time.getCurrentTick();
            m_delta = time.getTickDelta();
        }

        int getPriority() const override { return 0; }
        const char* getName() const override { return "TimeCapture"; }

    private:
        SimulationTick& m_tick;
        float& m_delta;
    };

    TimeCapture capture(received_tick, received_delta);
    core.register_system(&capture);

    core.update(0.05f);
    assert(received_tick == 1);
    assert(received_delta == SIMULATION_TICK_DELTA);

    core.update(0.05f);
    assert(received_tick == 2);

    printf("  PASS: System receives correct time info\n");
}

// =========================================================================
// Test: Empty core update does not crash
// =========================================================================

void test_empty_update() {
    printf("Testing empty core update...\n");

    SimulationCore core;
    core.update(0.05f);
    assert(core.getCurrentTick() == 1);

    core.update(0.1f);
    assert(core.getCurrentTick() == 3);

    printf("  PASS: Empty core updates without crash\n");
}

// =========================================================================
// Main
// =========================================================================

int main() {
    printf("=== SimulationCore Tests (E10-001) ===\n\n");

    test_initial_state();
    test_register_unregister();
    test_accumulator_no_tick();
    test_accumulator_tick_fires();
    test_accumulator_multiple_ticks();
    test_accumulator_carries_over();
    test_priority_order();
    test_tick_delta_constant();
    test_get_current_tick();
    test_get_total_time();
    test_interpolation();
    test_system_receives_time();
    test_empty_update();

    printf("\n=== All SimulationCore tests passed ===\n");
    return 0;
}
