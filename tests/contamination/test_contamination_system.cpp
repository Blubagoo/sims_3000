/**
 * @file test_contamination_system.cpp
 * @brief Tests for ContaminationSystem skeleton (E10-081)
 *
 * Validates:
 * - Construction with grid dimensions
 * - ISimulatable interface (getPriority, getName)
 * - Grid access (get_grid returns correct dimensions)
 * - tick() swaps buffers (data moves to previous)
 * - tick() runs without crash
 * - Stats return 0 on empty grid
 */

#include <cassert>
#include <cstdio>
#include <cstring>

#include "sims3000/contamination/ContaminationSystem.h"
#include "sims3000/core/ISimulationTime.h"

using namespace sims3000;
using namespace sims3000::contamination;

// --------------------------------------------------------------------------
// Mock ISimulationTime for testing
// --------------------------------------------------------------------------

class MockSimulationTime : public ISimulationTime {
public:
    explicit MockSimulationTime(SimulationTick tick = 0)
        : m_tick(tick) {}

    SimulationTick getCurrentTick() const override { return m_tick; }
    float getTickDelta() const override { return 0.05f; }
    float getInterpolation() const override { return 0.0f; }
    double getTotalTime() const override { return static_cast<double>(m_tick) * 0.05; }

    void set_tick(SimulationTick tick) { m_tick = tick; }

private:
    SimulationTick m_tick;
};

// --------------------------------------------------------------------------
// Test: ContaminationSystem creation with grid dimensions
// --------------------------------------------------------------------------
static void test_creation() {
    ContaminationSystem system(64, 64);
    // Should construct without crash
    std::printf("  PASS: ContaminationSystem creation (64x64)\n");
}

// --------------------------------------------------------------------------
// Test: Creation with various grid sizes
// --------------------------------------------------------------------------
static void test_creation_various_sizes() {
    ContaminationSystem small_system(128, 128);
    ContaminationSystem medium_system(256, 256);
    ContaminationSystem large_system(512, 512);
    // All should construct without crash
    std::printf("  PASS: ContaminationSystem creation with various sizes\n");
}

// --------------------------------------------------------------------------
// Test: getPriority returns 80
// --------------------------------------------------------------------------
static void test_get_priority() {
    ContaminationSystem system(64, 64);
    assert(system.getPriority() == 80 && "Priority should be 80");
    std::printf("  PASS: getPriority returns 80\n");
}

// --------------------------------------------------------------------------
// Test: getName returns "ContaminationSystem"
// --------------------------------------------------------------------------
static void test_get_name() {
    ContaminationSystem system(64, 64);
    assert(std::strcmp(system.getName(), "ContaminationSystem") == 0 &&
           "getName should return 'ContaminationSystem'");
    std::printf("  PASS: getName returns 'ContaminationSystem'\n");
}

// --------------------------------------------------------------------------
// Test: get_grid() returns grid with correct dimensions
// --------------------------------------------------------------------------
static void test_get_grid_dimensions() {
    ContaminationSystem system(128, 64);
    const ContaminationGrid& grid = system.get_grid();
    assert(grid.get_width() == 128 && "Grid width should be 128");
    assert(grid.get_height() == 64 && "Grid height should be 64");
    std::printf("  PASS: get_grid returns grid with correct dimensions\n");
}

// --------------------------------------------------------------------------
// Test: get_grid_mut() returns mutable grid reference
// --------------------------------------------------------------------------
static void test_get_grid_mut() {
    ContaminationSystem system(64, 64);
    ContaminationGrid& grid = system.get_grid_mut();
    // Should be able to modify grid through mutable reference
    grid.add_contamination(0, 0, 50, 1);
    assert(system.get_grid().get_level(0, 0) == 50 &&
           "Mutable grid modification should be visible via const accessor");
    std::printf("  PASS: get_grid_mut returns mutable grid reference\n");
}

// --------------------------------------------------------------------------
// Test: tick() swaps buffers (data moves to previous)
// --------------------------------------------------------------------------
static void test_tick_swaps_buffers() {
    ContaminationSystem system(64, 64);
    MockSimulationTime time(0);

    // Write data into current buffer
    system.get_grid_mut().add_contamination(5, 5, 100, 1);
    assert(system.get_grid().get_level(5, 5) == 100 &&
           "Current buffer should have contamination");
    assert(system.get_grid().get_level_previous_tick(5, 5) == 0 &&
           "Previous buffer should be empty before tick");

    // Tick swaps buffers: current becomes previous, new current starts empty
    time.set_tick(1);
    system.tick(time);

    // After swap, the data should be in the previous buffer
    assert(system.get_grid().get_level_previous_tick(5, 5) == 100 &&
           "Previous buffer should contain data after swap");
    // Current buffer should be clean (was the old previous, which was empty)
    assert(system.get_grid().get_level(5, 5) == 0 &&
           "Current buffer should be empty after swap (no generate/spread stubs)");

    std::printf("  PASS: tick() swaps buffers (data moves to previous)\n");
}

// --------------------------------------------------------------------------
// Test: tick() runs without crash
// --------------------------------------------------------------------------
static void test_tick_no_crash() {
    ContaminationSystem system(128, 128);
    MockSimulationTime time(0);

    // Run several ticks
    for (SimulationTick t = 0; t < 100; ++t) {
        time.set_tick(t);
        system.tick(time);
    }

    // No crash = success
    std::printf("  PASS: tick() runs without crash (100 ticks)\n");
}

// --------------------------------------------------------------------------
// Test: Stats return 0 on empty grid
// --------------------------------------------------------------------------
static void test_stats_empty_grid() {
    ContaminationSystem system(64, 64);
    MockSimulationTime time(0);

    // Run a tick so update_stats is called
    system.tick(time);

    assert(system.get_total_contamination() == 0 &&
           "Total contamination should be 0 on empty grid");
    assert(system.get_toxic_tiles() == 0 &&
           "Toxic tiles should be 0 on empty grid");
    assert(system.get_toxic_tiles(1) == 0 &&
           "Toxic tiles (threshold=1) should be 0 on empty grid");

    std::printf("  PASS: stats return 0 on empty grid\n");
}

// --------------------------------------------------------------------------
// Test: Stats reflect grid data after manual contamination
// --------------------------------------------------------------------------
static void test_stats_after_contamination() {
    ContaminationSystem system(64, 64);
    MockSimulationTime time(0);

    // Add some contamination manually
    system.get_grid_mut().add_contamination(0, 0, 200, 1);
    system.get_grid_mut().add_contamination(1, 0, 150, 1);
    system.get_grid_mut().add_contamination(2, 0, 50, 2);

    // update_stats is called inside tick, but tick also swaps buffers first.
    // So we need to call update_stats directly on the grid to test current data.
    system.get_grid_mut().update_stats();

    assert(system.get_total_contamination() == 400 &&
           "Total contamination should be 200+150+50=400");
    assert(system.get_toxic_tiles(128) == 2 &&
           "Toxic tiles (threshold=128) should be 2 (200 and 150)");

    std::printf("  PASS: stats reflect grid data after manual contamination\n");
}

// --------------------------------------------------------------------------
// Test: ISimulatable polymorphism
// --------------------------------------------------------------------------
static void test_isimulatable_polymorphism() {
    ContaminationSystem system(64, 64);
    ISimulatable* base = &system;

    assert(base->getPriority() == 80 && "Polymorphic getPriority should be 80");
    assert(std::strcmp(base->getName(), "ContaminationSystem") == 0 &&
           "Polymorphic getName should return 'ContaminationSystem'");

    MockSimulationTime time(0);
    base->tick(time);  // Should not crash

    std::printf("  PASS: ISimulatable polymorphism works\n");
}

// --------------------------------------------------------------------------
// Main
// --------------------------------------------------------------------------
int main() {
    std::printf("=== ContaminationSystem Tests (E10-081) ===\n");

    test_creation();
    test_creation_various_sizes();
    test_get_priority();
    test_get_name();
    test_get_grid_dimensions();
    test_get_grid_mut();
    test_tick_swaps_buffers();
    test_tick_no_crash();
    test_stats_empty_grid();
    test_stats_after_contamination();
    test_isimulatable_polymorphism();

    std::printf("All ContaminationSystem tests passed.\n");
    return 0;
}
