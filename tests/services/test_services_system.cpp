/**
 * @file test_services_system.cpp
 * @brief Unit tests for ServicesSystem class (Epic 9, Ticket E9-003)
 *
 * Tests cover:
 * - System priority is 55
 * - System name is "ServicesSystem"
 * - Init/cleanup lifecycle
 * - Tick doesn't crash (empty stub)
 * - Double init/cleanup safety
 * - Destructor cleanup
 */

#include <sims3000/services/ServicesSystem.h>
#include <cassert>
#include <cstdio>
#include <cstring>

using namespace sims3000::services;

// =============================================================================
// Mock ISimulationTime for tick() testing
// =============================================================================

class MockSimulationTime : public sims3000::ISimulationTime {
public:
    sims3000::SimulationTick getCurrentTick() const override { return m_tick; }
    float getTickDelta() const override { return 0.05f; }
    float getInterpolation() const override { return 0.0f; }
    double getTotalTime() const override { return static_cast<double>(m_tick) * 0.05; }

    sims3000::SimulationTick m_tick = 0;
};

// =============================================================================
// Priority tests
// =============================================================================

void test_priority_value() {
    printf("Testing ServicesSystem priority is 55...\n");

    ServicesSystem system;
    assert(system.getPriority() == 55);
    assert(ServicesSystem::TICK_PRIORITY == 55);

    printf("  PASS: Priority is 55\n");
}

// =============================================================================
// Name tests
// =============================================================================

void test_system_name() {
    printf("Testing ServicesSystem name...\n");

    ServicesSystem system;
    assert(std::strcmp(system.getName(), "ServicesSystem") == 0);

    printf("  PASS: Name is \"ServicesSystem\"\n");
}

// =============================================================================
// Lifecycle tests
// =============================================================================

void test_init_sets_dimensions() {
    printf("Testing init sets map dimensions...\n");

    ServicesSystem system;
    assert(!system.isInitialized());
    assert(system.getMapWidth() == 0);
    assert(system.getMapHeight() == 0);

    system.init(128, 256);
    assert(system.isInitialized());
    assert(system.getMapWidth() == 128);
    assert(system.getMapHeight() == 256);

    printf("  PASS: Init sets dimensions correctly\n");
}

void test_cleanup_resets_state() {
    printf("Testing cleanup resets state...\n");

    ServicesSystem system;
    system.init(64, 64);
    assert(system.isInitialized());

    system.cleanup();
    assert(!system.isInitialized());
    assert(system.getMapWidth() == 0);
    assert(system.getMapHeight() == 0);

    printf("  PASS: Cleanup resets state correctly\n");
}

void test_double_init() {
    printf("Testing double init replaces state...\n");

    ServicesSystem system;
    system.init(64, 64);
    assert(system.getMapWidth() == 64);

    system.init(128, 128);
    assert(system.isInitialized());
    assert(system.getMapWidth() == 128);
    assert(system.getMapHeight() == 128);

    printf("  PASS: Double init works correctly\n");
}

void test_double_cleanup() {
    printf("Testing double cleanup is safe...\n");

    ServicesSystem system;
    system.init(64, 64);
    system.cleanup();
    system.cleanup();  // Should not crash

    assert(!system.isInitialized());

    printf("  PASS: Double cleanup is safe\n");
}

void test_destructor_cleanup() {
    printf("Testing destructor cleans up initialized system...\n");

    {
        ServicesSystem system;
        system.init(64, 64);
        // Destructor runs here - should not crash
    }

    printf("  PASS: Destructor cleanup works correctly\n");
}

void test_destructor_uninitialized() {
    printf("Testing destructor on uninitialized system...\n");

    {
        ServicesSystem system;
        // Destructor runs here without init - should not crash
    }

    printf("  PASS: Destructor on uninitialized system works correctly\n");
}

// =============================================================================
// Tick tests
// =============================================================================

void test_tick_stub_uninitialized() {
    printf("Testing tick on uninitialized system doesn't crash...\n");

    ServicesSystem system;
    MockSimulationTime time;

    // Should not crash even when not initialized
    system.tick(time);

    printf("  PASS: Tick on uninitialized system doesn't crash\n");
}

void test_tick_stub_initialized() {
    printf("Testing tick on initialized system doesn't crash...\n");

    ServicesSystem system;
    system.init(128, 128);

    MockSimulationTime time;
    time.m_tick = 1;

    // Should not crash
    system.tick(time);

    printf("  PASS: Tick on initialized system doesn't crash\n");
}

void test_tick_multiple_calls() {
    printf("Testing multiple tick calls...\n");

    ServicesSystem system;
    system.init(64, 64);

    MockSimulationTime time;
    for (uint32_t i = 0; i < 100; ++i) {
        time.m_tick = i;
        system.tick(time);
    }

    printf("  PASS: Multiple tick calls work correctly\n");
}

// =============================================================================
// Constants tests
// =============================================================================

void test_max_players() {
    printf("Testing MAX_PLAYERS constant...\n");

    assert(ServicesSystem::MAX_PLAYERS == 4);

    printf("  PASS: MAX_PLAYERS is 4\n");
}

// =============================================================================
// ISimulatable interface tests
// =============================================================================

void test_isimulatable_interface() {
    printf("Testing ISimulatable interface conformance...\n");

    ServicesSystem system;

    // Test through base pointer
    sims3000::ISimulatable* base = &system;
    assert(base->getPriority() == 55);
    assert(std::strcmp(base->getName(), "ServicesSystem") == 0);

    MockSimulationTime time;
    base->tick(time);  // Should not crash

    printf("  PASS: ISimulatable interface conformance correct\n");
}

// =============================================================================
// Main
// =============================================================================

int main() {
    printf("=== ServicesSystem Unit Tests (Epic 9, Ticket E9-003) ===\n\n");

    test_priority_value();
    test_system_name();
    test_init_sets_dimensions();
    test_cleanup_resets_state();
    test_double_init();
    test_double_cleanup();
    test_destructor_cleanup();
    test_destructor_uninitialized();
    test_tick_stub_uninitialized();
    test_tick_stub_initialized();
    test_tick_multiple_calls();
    test_max_players();
    test_isimulatable_interface();

    printf("\n=== All ServicesSystem Tests Passed ===\n");
    return 0;
}
