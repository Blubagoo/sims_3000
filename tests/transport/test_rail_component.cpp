/**
 * @file test_rail_component.cpp
 * @brief Unit tests for RailComponent (Epic 7, Ticket E7-030)
 */

#include <sims3000/transport/RailComponent.h>
#include <cassert>
#include <cstdio>

using namespace sims3000::transport;

void test_rail_component_size() {
    printf("Testing RailComponent size...\n");

    assert(sizeof(RailComponent) == 12);

    printf("  PASS: RailComponent is 12 bytes\n");
}

void test_rail_trivially_copyable() {
    printf("Testing RailComponent is trivially copyable...\n");

    assert(std::is_trivially_copyable<RailComponent>::value);

    printf("  PASS: RailComponent is trivially copyable\n");
}

void test_rail_default_initialization() {
    printf("Testing default initialization...\n");

    RailComponent rail{};
    assert(rail.type == RailType::SurfaceRail);
    assert(rail.capacity == 500);
    assert(rail.current_load == 0);
    assert(rail.connection_mask == 0);
    assert(rail.is_terminal_adjacent == false);
    assert(rail.is_powered == false);
    assert(rail.is_active == false);
    assert(rail.rail_network_id == 0);
    assert(rail.health == 255);

    printf("  PASS: Default initialization works correctly\n");
}

void test_rail_custom_values() {
    printf("Testing custom value assignment...\n");

    RailComponent rail{};
    rail.type = RailType::ElevatedRail;
    rail.capacity = 1000;
    rail.current_load = 250;
    rail.connection_mask = 0x0F;
    rail.is_terminal_adjacent = true;
    rail.is_powered = true;
    rail.is_active = true;
    rail.rail_network_id = 42;
    rail.health = 200;

    assert(rail.type == RailType::ElevatedRail);
    assert(rail.capacity == 1000);
    assert(rail.current_load == 250);
    assert(rail.connection_mask == 0x0F);
    assert(rail.is_terminal_adjacent == true);
    assert(rail.is_powered == true);
    assert(rail.is_active == true);
    assert(rail.rail_network_id == 42);
    assert(rail.health == 200);

    printf("  PASS: Custom value assignment works correctly\n");
}

void test_rail_type_enum() {
    printf("Testing RailType enum values...\n");

    assert(static_cast<uint8_t>(RailType::SurfaceRail) == 0);
    assert(static_cast<uint8_t>(RailType::ElevatedRail) == 1);
    assert(static_cast<uint8_t>(RailType::SubterraRail) == 2);
    assert(sizeof(RailType) == 1);

    printf("  PASS: RailType enum values are correct\n");
}

void test_rail_copy() {
    printf("Testing copy semantics...\n");

    RailComponent original{};
    original.type = RailType::SubterraRail;
    original.capacity = 750;
    original.current_load = 100;
    original.connection_mask = 0xFF;
    original.is_terminal_adjacent = true;
    original.is_powered = true;
    original.is_active = true;
    original.rail_network_id = 99;
    original.health = 128;

    RailComponent copy = original;
    assert(copy.type == RailType::SubterraRail);
    assert(copy.capacity == 750);
    assert(copy.current_load == 100);
    assert(copy.connection_mask == 0xFF);
    assert(copy.is_terminal_adjacent == true);
    assert(copy.is_powered == true);
    assert(copy.is_active == true);
    assert(copy.rail_network_id == 99);
    assert(copy.health == 128);

    printf("  PASS: Copy semantics work correctly\n");
}

void test_rail_all_types() {
    printf("Testing all rail types can be assigned...\n");

    RailComponent rail{};

    rail.type = RailType::SurfaceRail;
    assert(rail.type == RailType::SurfaceRail);

    rail.type = RailType::ElevatedRail;
    assert(rail.type == RailType::ElevatedRail);

    rail.type = RailType::SubterraRail;
    assert(rail.type == RailType::SubterraRail);

    printf("  PASS: All rail types can be assigned\n");
}

int main() {
    printf("=== RailComponent Unit Tests (Epic 7, Ticket E7-030) ===\n\n");

    test_rail_component_size();
    test_rail_trivially_copyable();
    test_rail_default_initialization();
    test_rail_custom_values();
    test_rail_type_enum();
    test_rail_copy();
    test_rail_all_types();

    printf("\n=== All RailComponent Tests Passed ===\n");
    return 0;
}
