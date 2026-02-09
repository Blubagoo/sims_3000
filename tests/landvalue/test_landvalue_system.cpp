/**
 * @file test_landvalue_system.cpp
 * @brief Tests for LandValueSystem skeleton (E10-100)
 *
 * Validates:
 * - Construction with grid dimensions
 * - ISimulatable interface (getPriority, getName)
 * - Grid access (get_grid returns correct dimensions)
 * - get_land_value() returns float value
 * - tick() resets grid values (all become 128 neutral after tick since stubs are empty)
 * - tick() runs without crash
 */

#include <cassert>
#include <cstdio>
#include <cstring>
#include <cmath>

#include "sims3000/landvalue/LandValueSystem.h"
#include "sims3000/core/ISimulationTime.h"

using namespace sims3000;
using namespace sims3000::landvalue;

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
// Test: LandValueSystem creation with grid dimensions
// --------------------------------------------------------------------------
static void test_creation() {
    LandValueSystem system(64, 64);
    // Should construct without crash
    std::printf("  PASS: LandValueSystem creation (64x64)\n");
}

// --------------------------------------------------------------------------
// Test: Creation with various grid sizes
// --------------------------------------------------------------------------
static void test_creation_various_sizes() {
    LandValueSystem small_system(128, 128);
    LandValueSystem medium_system(256, 256);
    LandValueSystem large_system(512, 512);
    // All should construct without crash
    std::printf("  PASS: LandValueSystem creation with various sizes\n");
}

// --------------------------------------------------------------------------
// Test: getPriority returns 85
// --------------------------------------------------------------------------
static void test_get_priority() {
    LandValueSystem system(64, 64);
    assert(system.getPriority() == 85 && "Priority should be 85");
    std::printf("  PASS: getPriority returns 85\n");
}

// --------------------------------------------------------------------------
// Test: getName returns "LandValueSystem"
// --------------------------------------------------------------------------
static void test_get_name() {
    LandValueSystem system(64, 64);
    assert(std::strcmp(system.getName(), "LandValueSystem") == 0 &&
           "getName should return 'LandValueSystem'");
    std::printf("  PASS: getName returns 'LandValueSystem'\n");
}

// --------------------------------------------------------------------------
// Test: get_grid() returns grid with correct dimensions
// --------------------------------------------------------------------------
static void test_get_grid_dimensions() {
    LandValueSystem system(128, 64);
    const LandValueGrid& grid = system.get_grid();
    assert(grid.get_width() == 128 && "Grid width should be 128");
    assert(grid.get_height() == 64 && "Grid height should be 64");
    std::printf("  PASS: get_grid returns grid with correct dimensions\n");
}

// --------------------------------------------------------------------------
// Test: get_grid_mut() returns mutable grid reference
// --------------------------------------------------------------------------
static void test_get_grid_mut() {
    LandValueSystem system(64, 64);
    LandValueGrid& grid = system.get_grid_mut();
    // Should be able to modify grid through mutable reference
    grid.set_value(0, 0, 200);
    assert(system.get_grid().get_value(0, 0) == 200 &&
           "Mutable grid modification should be visible via const accessor");
    std::printf("  PASS: get_grid_mut returns mutable grid reference\n");
}

// --------------------------------------------------------------------------
// Test: get_land_value() returns float value
// --------------------------------------------------------------------------
static void test_get_land_value_returns_float() {
    LandValueSystem system(64, 64);

    // Default value is 128 (neutral)
    float val = system.get_land_value(0, 0);
    assert(std::fabs(val - 128.0f) < 0.001f && "Default land value should be 128.0");

    // Set a specific value and verify float conversion
    system.get_grid_mut().set_value(5, 5, 200);
    float val2 = system.get_land_value(5, 5);
    assert(std::fabs(val2 - 200.0f) < 0.001f && "Land value should be 200.0");

    // Test zero value
    system.get_grid_mut().set_value(10, 10, 0);
    float val3 = system.get_land_value(10, 10);
    assert(std::fabs(val3 - 0.0f) < 0.001f && "Land value should be 0.0");

    // Test max value
    system.get_grid_mut().set_value(15, 15, 255);
    float val4 = system.get_land_value(15, 15);
    assert(std::fabs(val4 - 255.0f) < 0.001f && "Land value should be 255.0");

    std::printf("  PASS: get_land_value returns float value\n");
}

// --------------------------------------------------------------------------
// Test: get_land_value() for out-of-bounds returns 0.0
// --------------------------------------------------------------------------
static void test_get_land_value_out_of_bounds() {
    LandValueSystem system(64, 64);

    float val = system.get_land_value(100, 100);
    assert(std::fabs(val - 0.0f) < 0.001f &&
           "Out-of-bounds land value should be 0.0");

    std::printf("  PASS: get_land_value out-of-bounds returns 0.0\n");
}

// --------------------------------------------------------------------------
// Test: tick() resets grid values (all become 128 neutral)
// --------------------------------------------------------------------------
static void test_tick_resets_values() {
    LandValueSystem system(64, 64);
    MockSimulationTime time(0);

    // Modify some values away from neutral
    system.get_grid_mut().set_value(0, 0, 50);
    system.get_grid_mut().set_value(10, 10, 200);
    system.get_grid_mut().set_value(63, 63, 0);

    assert(system.get_grid().get_value(0, 0) == 50 && "Pre-tick value should be 50");
    assert(system.get_grid().get_value(10, 10) == 200 && "Pre-tick value should be 200");

    // After tick, reset_values() sets everything back to 128
    // (stub phases don't modify values)
    system.tick(time);

    assert(system.get_grid().get_value(0, 0) == 128 &&
           "After tick, value should be reset to 128 (neutral)");
    assert(system.get_grid().get_value(10, 10) == 128 &&
           "After tick, value should be reset to 128 (neutral)");
    assert(system.get_grid().get_value(63, 63) == 128 &&
           "After tick, value should be reset to 128 (neutral)");

    // Verify via get_land_value too
    assert(std::fabs(system.get_land_value(0, 0) - 128.0f) < 0.001f &&
           "After tick, land value should be 128.0");

    std::printf("  PASS: tick() resets grid values to 128 (neutral)\n");
}

// --------------------------------------------------------------------------
// Test: tick() runs without crash
// --------------------------------------------------------------------------
static void test_tick_no_crash() {
    LandValueSystem system(128, 128);
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
// Test: terrain_bonus is preserved across tick reset
// --------------------------------------------------------------------------
static void test_terrain_bonus_preserved() {
    LandValueSystem system(64, 64);
    MockSimulationTime time(0);

    // Set a terrain bonus
    system.get_grid_mut().set_terrain_bonus(5, 5, 30);
    assert(system.get_grid().get_terrain_bonus(5, 5) == 30 &&
           "Terrain bonus should be 30");

    // tick() resets total_value but NOT terrain_bonus
    system.tick(time);

    assert(system.get_grid().get_terrain_bonus(5, 5) == 30 &&
           "Terrain bonus should be preserved after tick");
    assert(system.get_grid().get_value(5, 5) == 128 &&
           "Total value should be reset to 128 after tick");

    std::printf("  PASS: terrain_bonus preserved across tick reset\n");
}

// --------------------------------------------------------------------------
// Test: ISimulatable polymorphism
// --------------------------------------------------------------------------
static void test_isimulatable_polymorphism() {
    LandValueSystem system(64, 64);
    ISimulatable* base = &system;

    assert(base->getPriority() == 85 && "Polymorphic getPriority should be 85");
    assert(std::strcmp(base->getName(), "LandValueSystem") == 0 &&
           "Polymorphic getName should return 'LandValueSystem'");

    MockSimulationTime time(0);
    base->tick(time);  // Should not crash

    std::printf("  PASS: ISimulatable polymorphism works\n");
}

// --------------------------------------------------------------------------
// Main
// --------------------------------------------------------------------------
int main() {
    std::printf("=== LandValueSystem Tests (E10-100) ===\n");

    test_creation();
    test_creation_various_sizes();
    test_get_priority();
    test_get_name();
    test_get_grid_dimensions();
    test_get_grid_mut();
    test_get_land_value_returns_float();
    test_get_land_value_out_of_bounds();
    test_tick_resets_values();
    test_tick_no_crash();
    test_terrain_bonus_preserved();
    test_isimulatable_polymorphism();

    std::printf("All LandValueSystem tests passed.\n");
    return 0;
}
