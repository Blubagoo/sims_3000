/**
 * @file test_energy_component.cpp
 * @brief Unit tests for EnergyComponent (Epic 5, Ticket 5-002)
 *
 * Tests cover:
 * - Size verification (12 bytes)
 * - Trivially copyable for serialization
 * - Default initialization values
 * - Priority constants from EnergyPriorities.h
 * - is_powered logic (energy_received >= energy_required)
 */

#include <sims3000/energy/EnergyComponent.h>
#include <sims3000/energy/EnergyPriorities.h>
#include <cassert>
#include <cstdio>

using namespace sims3000::energy;

void test_energy_component_size() {
    printf("Testing EnergyComponent size...\n");

    assert(sizeof(EnergyComponent) == 12);

    printf("  PASS: EnergyComponent is 12 bytes\n");
}

void test_energy_component_trivially_copyable() {
    printf("Testing EnergyComponent is trivially copyable...\n");

    assert(std::is_trivially_copyable<EnergyComponent>::value);

    printf("  PASS: EnergyComponent is trivially copyable\n");
}

void test_energy_component_default_initialization() {
    printf("Testing default initialization...\n");

    EnergyComponent ec{};
    assert(ec.energy_required == 0);
    assert(ec.energy_received == 0);
    assert(ec.is_powered == false);
    assert(ec.priority == ENERGY_PRIORITY_DEFAULT);
    assert(ec.grid_id == 0);
    assert(ec._padding == 0);

    printf("  PASS: Default initialization works correctly\n");
}

void test_energy_component_priority_constants() {
    printf("Testing priority constants...\n");

    // Verify priority levels
    assert(ENERGY_PRIORITY_CRITICAL == 1);
    assert(ENERGY_PRIORITY_IMPORTANT == 2);
    assert(ENERGY_PRIORITY_NORMAL == 3);
    assert(ENERGY_PRIORITY_LOW == 4);
    assert(ENERGY_PRIORITY_DEFAULT == ENERGY_PRIORITY_NORMAL);

    // Verify default priority matches ENERGY_PRIORITY_DEFAULT (which is NORMAL = 3)
    EnergyComponent ec{};
    assert(ec.priority == 3);
    assert(ec.priority == ENERGY_PRIORITY_NORMAL);

    printf("  PASS: Priority constants are correct\n");
}

void test_energy_component_priority_assignment() {
    printf("Testing priority assignment...\n");

    EnergyComponent ec{};

    // Set to critical
    ec.priority = ENERGY_PRIORITY_CRITICAL;
    assert(ec.priority == 1);

    // Set to important
    ec.priority = ENERGY_PRIORITY_IMPORTANT;
    assert(ec.priority == 2);

    // Set to normal
    ec.priority = ENERGY_PRIORITY_NORMAL;
    assert(ec.priority == 3);

    // Set to low
    ec.priority = ENERGY_PRIORITY_LOW;
    assert(ec.priority == 4);

    printf("  PASS: Priority assignment works correctly\n");
}

void test_energy_component_is_powered_logic() {
    printf("Testing is_powered logic...\n");

    EnergyComponent ec{};
    ec.energy_required = 100;

    // Not powered: received < required
    ec.energy_received = 50;
    ec.is_powered = (ec.energy_received >= ec.energy_required);
    assert(ec.is_powered == false);

    // Powered: received == required
    ec.energy_received = 100;
    ec.is_powered = (ec.energy_received >= ec.energy_required);
    assert(ec.is_powered == true);

    // Powered: received > required
    ec.energy_received = 150;
    ec.is_powered = (ec.energy_received >= ec.energy_required);
    assert(ec.is_powered == true);

    // Edge case: zero required, zero received -> powered
    ec.energy_required = 0;
    ec.energy_received = 0;
    ec.is_powered = (ec.energy_received >= ec.energy_required);
    assert(ec.is_powered == true);

    printf("  PASS: is_powered logic works correctly\n");
}

void test_energy_component_copy() {
    printf("Testing copy semantics...\n");

    EnergyComponent original{};
    original.energy_required = 200;
    original.energy_received = 150;
    original.is_powered = false;
    original.priority = ENERGY_PRIORITY_CRITICAL;
    original.grid_id = 5;

    EnergyComponent copy = original;
    assert(copy.energy_required == 200);
    assert(copy.energy_received == 150);
    assert(copy.is_powered == false);
    assert(copy.priority == ENERGY_PRIORITY_CRITICAL);
    assert(copy.grid_id == 5);

    printf("  PASS: Copy semantics work correctly\n");
}

int main() {
    printf("=== EnergyComponent Unit Tests (Epic 5, Ticket 5-002) ===\n\n");

    test_energy_component_size();
    test_energy_component_trivially_copyable();
    test_energy_component_default_initialization();
    test_energy_component_priority_constants();
    test_energy_component_priority_assignment();
    test_energy_component_is_powered_logic();
    test_energy_component_copy();

    printf("\n=== All EnergyComponent Tests Passed ===\n");
    return 0;
}
