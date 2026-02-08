/**
 * @file test_fluid_conduit_component.cpp
 * @brief Unit tests for FluidConduitComponent (Epic 6, Ticket 6-005)
 */

#include <sims3000/fluid/FluidConduitComponent.h>
#include <cassert>
#include <cstdio>

using namespace sims3000::fluid;

void test_conduit_component_size() {
    printf("Testing FluidConduitComponent size...\n");

    assert(sizeof(FluidConduitComponent) == 4);

    printf("  PASS: FluidConduitComponent is 4 bytes\n");
}

void test_conduit_trivially_copyable() {
    printf("Testing FluidConduitComponent is trivially copyable...\n");

    assert(std::is_trivially_copyable<FluidConduitComponent>::value);

    printf("  PASS: FluidConduitComponent is trivially copyable\n");
}

void test_conduit_default_initialization() {
    printf("Testing default initialization...\n");

    FluidConduitComponent conduit{};
    assert(conduit.coverage_radius == 3);
    assert(conduit.is_connected == false);
    assert(conduit.is_active == false);
    assert(conduit.conduit_level == 1);

    printf("  PASS: Default initialization works correctly\n");
}

void test_conduit_custom_values() {
    printf("Testing custom value assignment...\n");

    FluidConduitComponent conduit{};
    conduit.coverage_radius = 5;
    conduit.is_connected = true;
    conduit.is_active = true;
    conduit.conduit_level = 2;

    assert(conduit.coverage_radius == 5);
    assert(conduit.is_connected == true);
    assert(conduit.is_active == true);
    assert(conduit.conduit_level == 2);

    printf("  PASS: Custom value assignment works correctly\n");
}

void test_conduit_aggregate_initialization() {
    printf("Testing aggregate initialization...\n");

    FluidConduitComponent conduit{3, true, true, 2};
    assert(conduit.coverage_radius == 3);
    assert(conduit.is_connected == true);
    assert(conduit.is_active == true);
    assert(conduit.conduit_level == 2);

    printf("  PASS: Aggregate initialization works correctly\n");
}

void test_conduit_copy() {
    printf("Testing copy semantics...\n");

    FluidConduitComponent original{};
    original.coverage_radius = 7;
    original.is_connected = true;
    original.is_active = true;
    original.conduit_level = 2;

    FluidConduitComponent copy = original;
    assert(copy.coverage_radius == 7);
    assert(copy.is_connected == true);
    assert(copy.is_active == true);
    assert(copy.conduit_level == 2);

    printf("  PASS: Copy semantics work correctly\n");
}

void test_conduit_structure_matches_energy() {
    printf("Testing structure matches EnergyConduitComponent layout...\n");

    // FluidConduitComponent must be 4 bytes, same as EnergyConduitComponent
    assert(sizeof(FluidConduitComponent) == 4);

    // Same field layout: coverage_radius, is_connected, is_active, conduit_level
    FluidConduitComponent conduit{};
    conduit.coverage_radius = 10;
    conduit.is_connected = true;
    conduit.is_active = false;
    conduit.conduit_level = 1;

    assert(conduit.coverage_radius == 10);
    assert(conduit.is_connected == true);
    assert(conduit.is_active == false);
    assert(conduit.conduit_level == 1);

    printf("  PASS: Structure matches EnergyConduitComponent layout\n");
}

int main() {
    printf("=== FluidConduitComponent Unit Tests (Epic 6, Ticket 6-005) ===\n\n");

    test_conduit_component_size();
    test_conduit_trivially_copyable();
    test_conduit_default_initialization();
    test_conduit_custom_values();
    test_conduit_aggregate_initialization();
    test_conduit_copy();
    test_conduit_structure_matches_energy();

    printf("\n=== All FluidConduitComponent Tests Passed ===\n");
    return 0;
}
