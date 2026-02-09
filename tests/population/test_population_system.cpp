/**
 * @file test_population_system.cpp
 * @brief Tests for PopulationSystem skeleton (E10-014)
 *
 * Validates:
 * - Construction and ISimulatable interface
 * - Player management (add/remove/has)
 * - Data access (get_population, get_employment)
 * - tick() runs without crash
 * - Frequency gating of sub-phases
 */

#include <cassert>
#include <cstdio>
#include <cstring>

#include "sims3000/population/PopulationSystem.h"
#include "sims3000/core/ISimulationTime.h"

using namespace sims3000;
using namespace sims3000::population;

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
// Test: PopulationSystem creation
// --------------------------------------------------------------------------
static void test_creation() {
    PopulationSystem system;
    // Should construct without crash
    std::printf("  PASS: PopulationSystem creation\n");
}

// --------------------------------------------------------------------------
// Test: getPriority returns 50
// --------------------------------------------------------------------------
static void test_get_priority() {
    PopulationSystem system;
    assert(system.getPriority() == 50 && "Priority should be 50");
    std::printf("  PASS: getPriority returns 50\n");
}

// --------------------------------------------------------------------------
// Test: getName returns "PopulationSystem"
// --------------------------------------------------------------------------
static void test_get_name() {
    PopulationSystem system;
    assert(std::strcmp(system.getName(), "PopulationSystem") == 0 &&
           "getName should return 'PopulationSystem'");
    std::printf("  PASS: getName returns 'PopulationSystem'\n");
}

// --------------------------------------------------------------------------
// Test: add_player / remove_player / has_player
// --------------------------------------------------------------------------
static void test_player_management() {
    PopulationSystem system;

    // Initially no players
    assert(!system.has_player(0) && "Player 0 should not exist initially");
    assert(!system.has_player(1) && "Player 1 should not exist initially");

    // Add player 0
    system.add_player(0);
    assert(system.has_player(0) && "Player 0 should exist after add");
    assert(!system.has_player(1) && "Player 1 should still not exist");

    // Add player 1
    system.add_player(1);
    assert(system.has_player(0) && "Player 0 should still exist");
    assert(system.has_player(1) && "Player 1 should exist after add");

    // Remove player 0
    system.remove_player(0);
    assert(!system.has_player(0) && "Player 0 should not exist after remove");
    assert(system.has_player(1) && "Player 1 should still exist");

    // Out-of-range player IDs
    assert(!system.has_player(4) && "Player 4 (out of range) should not exist");
    assert(!system.has_player(255) && "Player 255 (out of range) should not exist");

    // Adding out-of-range should not crash
    system.add_player(5);
    assert(!system.has_player(5) && "Player 5 (out of range) should not exist");

    // Removing out-of-range should not crash
    system.remove_player(5);

    std::printf("  PASS: add_player / remove_player / has_player\n");
}

// --------------------------------------------------------------------------
// Test: get_population returns default data for added player
// --------------------------------------------------------------------------
static void test_get_population_default() {
    PopulationSystem system;
    system.add_player(0);

    const PopulationData& pop = system.get_population(0);
    assert(pop.total_beings == 0 && "Default total_beings should be 0");
    assert(pop.max_capacity == 0 && "Default max_capacity should be 0");
    assert(pop.youth_percent == 33 && "Default youth_percent should be 33");
    assert(pop.adult_percent == 34 && "Default adult_percent should be 34");
    assert(pop.elder_percent == 33 && "Default elder_percent should be 33");

    std::printf("  PASS: get_population returns default data\n");
}

// --------------------------------------------------------------------------
// Test: get_employment returns default data for added player
// --------------------------------------------------------------------------
static void test_get_employment_default() {
    PopulationSystem system;
    system.add_player(0);

    const EmploymentData& emp = system.get_employment(0);
    assert(emp.working_age_beings == 0 && "Default working_age_beings should be 0");
    assert(emp.labor_force == 0 && "Default labor_force should be 0");
    assert(emp.employed_laborers == 0 && "Default employed_laborers should be 0");
    assert(emp.total_jobs == 0 && "Default total_jobs should be 0");
    assert(emp.labor_participation == 65 && "Default labor_participation should be 65");

    std::printf("  PASS: get_employment returns default data\n");
}

// --------------------------------------------------------------------------
// Test: get_population for invalid player returns empty/default
// --------------------------------------------------------------------------
static void test_get_population_invalid_player() {
    PopulationSystem system;

    // Not-added player
    const PopulationData& pop = system.get_population(0);
    assert(pop.total_beings == 0 && "Invalid player population should be default");

    // Out-of-range player
    const PopulationData& pop2 = system.get_population(10);
    assert(pop2.total_beings == 0 && "Out-of-range player population should be default");

    std::printf("  PASS: get_population invalid player returns default\n");
}

// --------------------------------------------------------------------------
// Test: get_employment for invalid player returns empty/default
// --------------------------------------------------------------------------
static void test_get_employment_invalid_player() {
    PopulationSystem system;

    const EmploymentData& emp = system.get_employment(0);
    assert(emp.working_age_beings == 0 && "Invalid player employment should be default");

    const EmploymentData& emp2 = system.get_employment(10);
    assert(emp2.working_age_beings == 0 && "Out-of-range player employment should be default");

    std::printf("  PASS: get_employment invalid player returns default\n");
}

// --------------------------------------------------------------------------
// Test: mutable access works
// --------------------------------------------------------------------------
static void test_mutable_access() {
    PopulationSystem system;
    system.add_player(0);

    // Modify population data
    PopulationData& pop = system.get_population_mut(0);
    pop.total_beings = 1000;
    assert(system.get_population(0).total_beings == 1000 &&
           "Mutable modification should be visible via const accessor");

    // Modify employment data
    EmploymentData& emp = system.get_employment_mut(0);
    emp.total_jobs = 500;
    assert(system.get_employment(0).total_jobs == 500 &&
           "Mutable modification should be visible via const accessor");

    std::printf("  PASS: mutable access works\n");
}

// --------------------------------------------------------------------------
// Test: tick() runs without crash (stub phase methods)
// --------------------------------------------------------------------------
static void test_tick_no_crash() {
    PopulationSystem system;
    system.add_player(0);
    system.add_player(1);

    MockSimulationTime time(0);

    // Run several ticks
    for (SimulationTick t = 0; t < 200; ++t) {
        time.set_tick(t);
        system.tick(time);
    }

    // No crash = success
    std::printf("  PASS: tick() runs without crash (200 ticks)\n");
}

// --------------------------------------------------------------------------
// Test: tick() with no active players does not crash
// --------------------------------------------------------------------------
static void test_tick_no_players() {
    PopulationSystem system;
    MockSimulationTime time(0);

    for (SimulationTick t = 0; t < 10; ++t) {
        time.set_tick(t);
        system.tick(time);
    }

    std::printf("  PASS: tick() with no players does not crash\n");
}

// --------------------------------------------------------------------------
// Test: Frequency gating constants are correct
// --------------------------------------------------------------------------
static void test_frequency_gating_constants() {
    assert(PopulationSystem::DEMOGRAPHIC_CYCLE_TICKS == 100 &&
           "Demographics should run every 100 ticks");
    assert(PopulationSystem::MIGRATION_CYCLE_TICKS == 20 &&
           "Migration should run every 20 ticks");
    assert(PopulationSystem::EMPLOYMENT_CYCLE_TICKS == 1 &&
           "Employment should run every tick");

    std::printf("  PASS: Frequency gating constants\n");
}

// --------------------------------------------------------------------------
// Test: Frequency gating behavior (demographics at tick%100==0, migration at tick%20==0)
// --------------------------------------------------------------------------
static void test_frequency_gating_behavior() {
    // Since the stub methods are empty, we cannot directly observe whether
    // they were called. However, we verify that tick() executes correctly
    // at the critical boundary ticks where phase transitions occur.

    PopulationSystem system;
    system.add_player(0);
    MockSimulationTime time;

    // Tick 0: demographics + migration + employment
    time.set_tick(0);
    system.tick(time);

    // Tick 1: only employment
    time.set_tick(1);
    system.tick(time);

    // Tick 19: only employment
    time.set_tick(19);
    system.tick(time);

    // Tick 20: migration + employment (20 % 20 == 0)
    time.set_tick(20);
    system.tick(time);

    // Tick 99: only employment
    time.set_tick(99);
    system.tick(time);

    // Tick 100: demographics + migration + employment (100 % 100 == 0 && 100 % 20 == 0)
    time.set_tick(100);
    system.tick(time);

    // Tick 200: demographics + migration + employment
    time.set_tick(200);
    system.tick(time);

    // Verify modular arithmetic holds for the constants
    assert(0 % PopulationSystem::DEMOGRAPHIC_CYCLE_TICKS == 0);
    assert(100 % PopulationSystem::DEMOGRAPHIC_CYCLE_TICKS == 0);
    assert(200 % PopulationSystem::DEMOGRAPHIC_CYCLE_TICKS == 0);
    assert(50 % PopulationSystem::DEMOGRAPHIC_CYCLE_TICKS != 0);

    assert(0 % PopulationSystem::MIGRATION_CYCLE_TICKS == 0);
    assert(20 % PopulationSystem::MIGRATION_CYCLE_TICKS == 0);
    assert(40 % PopulationSystem::MIGRATION_CYCLE_TICKS == 0);
    assert(15 % PopulationSystem::MIGRATION_CYCLE_TICKS != 0);

    std::printf("  PASS: Frequency gating behavior\n");
}

// --------------------------------------------------------------------------
// Test: remove and re-add player resets data
// --------------------------------------------------------------------------
static void test_remove_readd_resets_data() {
    PopulationSystem system;
    system.add_player(0);

    // Modify data
    system.get_population_mut(0).total_beings = 5000;
    system.get_employment_mut(0).total_jobs = 2000;
    assert(system.get_population(0).total_beings == 5000);

    // Remove player
    system.remove_player(0);
    assert(!system.has_player(0));

    // Re-add player: data should be reset to defaults
    system.add_player(0);
    assert(system.has_player(0));
    assert(system.get_population(0).total_beings == 0 && "Re-added player should have reset population");
    assert(system.get_employment(0).total_jobs == 0 && "Re-added player should have reset employment");

    std::printf("  PASS: remove and re-add player resets data\n");
}

// --------------------------------------------------------------------------
// Main
// --------------------------------------------------------------------------
int main() {
    std::printf("=== PopulationSystem Tests (E10-014) ===\n");

    test_creation();
    test_get_priority();
    test_get_name();
    test_player_management();
    test_get_population_default();
    test_get_employment_default();
    test_get_population_invalid_player();
    test_get_employment_invalid_player();
    test_mutable_access();
    test_tick_no_crash();
    test_tick_no_players();
    test_frequency_gating_constants();
    test_frequency_gating_behavior();
    test_remove_readd_resets_data();

    std::printf("All PopulationSystem tests passed.\n");
    return 0;
}
