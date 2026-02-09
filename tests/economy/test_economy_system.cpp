/**
 * @file test_economy_system.cpp
 * @brief Unit tests for EconomySystem skeleton (E11-004)
 *
 * Validates:
 * - Construction and ISimulatable interface (priority, name)
 * - Treasury default values (starting balance = 20000)
 * - Player activation/deactivation
 * - tick() runs without crash
 * - Budget cycle frequency gating
 */

#include <cassert>
#include <cstdio>
#include <cstring>

#include "sims3000/economy/EconomySystem.h"
#include "sims3000/core/ISimulationTime.h"

using namespace sims3000;
using namespace sims3000::economy;

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
// Test: EconomySystem creation
// --------------------------------------------------------------------------
static void test_creation() {
    EconomySystem system;
    // Should construct without crash
    std::printf("  PASS: EconomySystem creation\n");
}

// --------------------------------------------------------------------------
// Test: getPriority returns 60
// --------------------------------------------------------------------------
static void test_get_priority() {
    EconomySystem system;
    assert(system.getPriority() == 60 && "Priority should be 60");
    std::printf("  PASS: getPriority returns 60\n");
}

// --------------------------------------------------------------------------
// Test: getName returns "EconomySystem"
// --------------------------------------------------------------------------
static void test_get_name() {
    EconomySystem system;
    assert(std::strcmp(system.getName(), "EconomySystem") == 0 &&
           "getName should return 'EconomySystem'");
    std::printf("  PASS: getName returns 'EconomySystem'\n");
}

// --------------------------------------------------------------------------
// Test: Treasury defaults (starting balance = 20000)
// --------------------------------------------------------------------------
static void test_treasury_defaults() {
    EconomySystem system;
    system.activate_player(0);

    const TreasuryState& t = system.get_treasury(0);
    assert(t.balance == 20000 && "Starting balance should be 20000");
    assert(t.last_income == 0 && "Last income should be 0");
    assert(t.last_expense == 0 && "Last expense should be 0");
    assert(t.tribute_rate_habitation == 7 && "Default habitation tribute rate should be 7");
    assert(t.tribute_rate_exchange == 7 && "Default exchange tribute rate should be 7");
    assert(t.tribute_rate_fabrication == 7 && "Default fabrication tribute rate should be 7");
    assert(t.funding_enforcer == 100 && "Default enforcer funding should be 100");
    assert(t.funding_hazard_response == 100 && "Default hazard response funding should be 100");
    assert(t.funding_medical == 100 && "Default medical funding should be 100");
    assert(t.funding_education == 100 && "Default education funding should be 100");
    assert(t.active_bonds.empty() && "No bonds by default");

    std::printf("  PASS: treasury defaults are correct\n");
}

// --------------------------------------------------------------------------
// Test: Player activation and deactivation
// --------------------------------------------------------------------------
static void test_player_activation() {
    EconomySystem system;

    // Initially no players active
    assert(!system.is_player_active(0) && "Player 0 should start inactive");
    assert(!system.is_player_active(1) && "Player 1 should start inactive");

    // Activate player 0
    system.activate_player(0);
    assert(system.is_player_active(0) && "Player 0 should be active after activation");
    assert(!system.is_player_active(1) && "Player 1 should still be inactive");

    // Activate player 3
    system.activate_player(3);
    assert(system.is_player_active(3) && "Player 3 should be active");

    // Invalid player_id should return false
    assert(!system.is_player_active(4) && "Player 4 is out of range");
    assert(!system.is_player_active(255) && "Player 255 is out of range");

    // Activate out-of-range should not crash
    system.activate_player(5);
    system.activate_player(255);

    std::printf("  PASS: player activation works correctly\n");
}

// --------------------------------------------------------------------------
// Test: tick() doesn't crash (no active players)
// --------------------------------------------------------------------------
static void test_tick_no_crash_empty() {
    EconomySystem system;
    MockSimulationTime time(0);

    // Tick with no active players should not crash
    system.tick(time);
    time.set_tick(1);
    system.tick(time);
    time.set_tick(100);
    system.tick(time);

    std::printf("  PASS: tick() doesn't crash with no active players\n");
}

// --------------------------------------------------------------------------
// Test: tick() doesn't crash (with active players)
// --------------------------------------------------------------------------
static void test_tick_no_crash_active() {
    EconomySystem system;
    system.activate_player(0);
    system.activate_player(2);

    MockSimulationTime time(0);

    // Run a variety of ticks including budget cycle tick
    for (SimulationTick t = 0; t <= 400; ++t) {
        time.set_tick(t);
        system.tick(time);
    }

    // Balance should still be 20000: no cached income/expenses means zero net change
    assert(system.get_treasury(0).balance == 20000 && "Balance unchanged (zero income/expenses)");

    std::printf("  PASS: tick() doesn't crash with active players\n");
}

// --------------------------------------------------------------------------
// Test: Budget cycle frequency gating
// --------------------------------------------------------------------------
static void test_budget_cycle_frequency() {
    EconomySystem system;
    system.activate_player(0);

    MockSimulationTime time(0);

    // Tick 0 should NOT trigger budget cycle (current_tick > 0 guard)
    time.set_tick(0);
    system.tick(time);

    // Tick 199 should not trigger
    time.set_tick(199);
    system.tick(time);

    // Tick 200 should trigger budget cycle (stub, so no observable effect)
    time.set_tick(200);
    system.tick(time);

    // Tick 400 should trigger again
    time.set_tick(400);
    system.tick(time);

    std::printf("  PASS: budget cycle frequency gating\n");
}

// --------------------------------------------------------------------------
// Test: Multiple treasuries are independent
// --------------------------------------------------------------------------
static void test_treasury_independence() {
    EconomySystem system;
    system.activate_player(0);
    system.activate_player(1);

    // Modify player 0's treasury
    system.get_treasury(0).balance = 50000;

    // Player 1's treasury should be unchanged
    assert(system.get_treasury(1).balance == 20000 && "Player 1 balance should be independent");
    assert(system.get_treasury(0).balance == 50000 && "Player 0 balance should be 50000");

    std::printf("  PASS: treasuries are independent per player\n");
}

// --------------------------------------------------------------------------
// Test: BUDGET_CYCLE_TICKS constant
// --------------------------------------------------------------------------
static void test_budget_cycle_constant() {
    assert(EconomySystem::BUDGET_CYCLE_TICKS == 200 && "Budget cycle should be 200 ticks");
    std::printf("  PASS: BUDGET_CYCLE_TICKS is 200\n");
}

// --------------------------------------------------------------------------
// Test: MAX_PLAYERS constant
// --------------------------------------------------------------------------
static void test_max_players_constant() {
    assert(EconomySystem::MAX_PLAYERS == 4 && "MAX_PLAYERS should be 4");
    std::printf("  PASS: MAX_PLAYERS is 4\n");
}

// --------------------------------------------------------------------------
// Main
// --------------------------------------------------------------------------
int main() {
    std::printf("=== EconomySystem Unit Tests (E11-004) ===\n\n");

    test_creation();
    test_get_priority();
    test_get_name();
    test_treasury_defaults();
    test_player_activation();
    test_tick_no_crash_empty();
    test_tick_no_crash_active();
    test_budget_cycle_frequency();
    test_treasury_independence();
    test_budget_cycle_constant();
    test_max_players_constant();

    std::printf("\n=== All EconomySystem tests passed! ===\n");
    return 0;
}
