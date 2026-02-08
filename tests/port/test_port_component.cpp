/**
 * @file test_port_component.cpp
 * @brief Unit tests for PortComponent (Epic 8, Ticket E8-002)
 *
 * Tests cover:
 * - PortComponent size assertion (16 bytes)
 * - Trivially copyable check
 * - Default initialization
 * - Custom value assignment
 * - Port type assignment (Aero, Aqua)
 * - Infrastructure level range (0-3)
 * - Connection flags bitmask
 * - Copy semantics
 * - Capacity limits
 */

#include <sims3000/port/PortComponent.h>
#include <cassert>
#include <cstdio>
#include <cstring>

using namespace sims3000::port;

void test_port_component_size() {
    printf("Testing PortComponent size...\n");

    assert(sizeof(PortComponent) == 16);

    printf("  PASS: PortComponent is 16 bytes\n");
}

void test_port_trivially_copyable() {
    printf("Testing PortComponent is trivially copyable...\n");

    assert(std::is_trivially_copyable<PortComponent>::value);

    printf("  PASS: PortComponent is trivially copyable\n");
}

void test_port_default_initialization() {
    printf("Testing default initialization...\n");

    PortComponent port{};
    assert(port.port_type == PortType::Aero);
    assert(port.capacity == 0);
    assert(port.max_capacity == 0);
    assert(port.utilization == 0);
    assert(port.infrastructure_level == 0);
    assert(port.is_operational == false);
    assert(port.is_connected_to_edge == false);
    assert(port.demand_bonus_radius == 0);
    assert(port.connection_flags == 0);
    assert(port.padding[0] == 0);
    assert(port.padding[1] == 0);
    assert(port.padding[2] == 0);
    assert(port.padding[3] == 0);

    printf("  PASS: Default initialization works correctly\n");
}

void test_port_custom_values() {
    printf("Testing custom value assignment...\n");

    PortComponent port{};
    port.port_type = PortType::Aqua;
    port.capacity = 3000;
    port.max_capacity = 5000;
    port.utilization = 75;
    port.infrastructure_level = 2;
    port.is_operational = true;
    port.is_connected_to_edge = true;
    port.demand_bonus_radius = 10;
    port.connection_flags = 0x03; // Pathway + Rail

    assert(port.port_type == PortType::Aqua);
    assert(port.capacity == 3000);
    assert(port.max_capacity == 5000);
    assert(port.utilization == 75);
    assert(port.infrastructure_level == 2);
    assert(port.is_operational == true);
    assert(port.is_connected_to_edge == true);
    assert(port.demand_bonus_radius == 10);
    assert(port.connection_flags == 0x03);

    printf("  PASS: Custom value assignment works correctly\n");
}

void test_port_types() {
    printf("Testing all port types can be assigned...\n");

    PortComponent port{};

    port.port_type = PortType::Aero;
    assert(port.port_type == PortType::Aero);
    assert(static_cast<uint8_t>(port.port_type) == 0);

    port.port_type = PortType::Aqua;
    assert(port.port_type == PortType::Aqua);
    assert(static_cast<uint8_t>(port.port_type) == 1);

    printf("  PASS: All port types can be assigned\n");
}

void test_port_infrastructure_levels() {
    printf("Testing infrastructure levels 0-3...\n");

    PortComponent port{};

    for (uint8_t level = 0; level <= 3; ++level) {
        port.infrastructure_level = level;
        assert(port.infrastructure_level == level);
    }

    printf("  PASS: Infrastructure levels 0-3 work correctly\n");
}

void test_port_connection_flags() {
    printf("Testing connection flags bitmask...\n");

    PortComponent port{};

    // Pathway bit (1)
    port.connection_flags = 1;
    assert((port.connection_flags & 1) != 0);   // Pathway set
    assert((port.connection_flags & 2) == 0);   // Rail not set
    assert((port.connection_flags & 4) == 0);   // Energy not set
    assert((port.connection_flags & 8) == 0);   // Fluid not set

    // Rail bit (2)
    port.connection_flags = 2;
    assert((port.connection_flags & 1) == 0);
    assert((port.connection_flags & 2) != 0);

    // Energy bit (4)
    port.connection_flags = 4;
    assert((port.connection_flags & 4) != 0);

    // Fluid bit (8)
    port.connection_flags = 8;
    assert((port.connection_flags & 8) != 0);

    // Combine all
    port.connection_flags = 1 | 2 | 4 | 8;
    assert(port.connection_flags == 0x0F);

    printf("  PASS: Connection flags bitmask works correctly\n");
}

void test_port_capacity_max() {
    printf("Testing capacity limits...\n");

    PortComponent port{};
    port.capacity = 5000;
    port.max_capacity = 5000;
    assert(port.capacity == 5000);
    assert(port.max_capacity == 5000);

    // uint16_t max
    port.capacity = UINT16_MAX;
    port.max_capacity = UINT16_MAX;
    assert(port.capacity == UINT16_MAX);
    assert(port.max_capacity == UINT16_MAX);

    printf("  PASS: Capacity limits work correctly\n");
}

void test_port_utilization_range() {
    printf("Testing utilization range 0-100...\n");

    PortComponent port{};

    port.utilization = 0;
    assert(port.utilization == 0);

    port.utilization = 50;
    assert(port.utilization == 50);

    port.utilization = 100;
    assert(port.utilization == 100);

    // uint8_t allows 0-255, but semantically 0-100
    port.utilization = 255;
    assert(port.utilization == 255);

    printf("  PASS: Utilization range works correctly\n");
}

void test_port_copy() {
    printf("Testing copy semantics...\n");

    PortComponent original{};
    original.port_type = PortType::Aqua;
    original.capacity = 2500;
    original.max_capacity = 4000;
    original.utilization = 80;
    original.infrastructure_level = 3;
    original.is_operational = true;
    original.is_connected_to_edge = true;
    original.demand_bonus_radius = 15;
    original.connection_flags = 0x07;

    PortComponent copy = original;
    assert(copy.port_type == PortType::Aqua);
    assert(copy.capacity == 2500);
    assert(copy.max_capacity == 4000);
    assert(copy.utilization == 80);
    assert(copy.infrastructure_level == 3);
    assert(copy.is_operational == true);
    assert(copy.is_connected_to_edge == true);
    assert(copy.demand_bonus_radius == 15);
    assert(copy.connection_flags == 0x07);

    printf("  PASS: Copy semantics work correctly\n");
}

void test_port_memcpy_safe() {
    printf("Testing memcpy safety...\n");

    PortComponent original{};
    original.port_type = PortType::Aqua;
    original.capacity = 1234;
    original.max_capacity = 5000;
    original.utilization = 99;
    original.infrastructure_level = 3;
    original.is_operational = true;
    original.is_connected_to_edge = false;
    original.demand_bonus_radius = 20;
    original.connection_flags = 0x0F;

    PortComponent copy{};
    std::memcpy(&copy, &original, sizeof(PortComponent));

    assert(copy.port_type == PortType::Aqua);
    assert(copy.capacity == 1234);
    assert(copy.max_capacity == 5000);
    assert(copy.utilization == 99);
    assert(copy.infrastructure_level == 3);
    assert(copy.is_operational == true);
    assert(copy.is_connected_to_edge == false);
    assert(copy.demand_bonus_radius == 20);
    assert(copy.connection_flags == 0x0F);

    printf("  PASS: memcpy safety works correctly\n");
}

int main() {
    printf("=== PortComponent Unit Tests (Epic 8, Ticket E8-002) ===\n\n");

    test_port_component_size();
    test_port_trivially_copyable();
    test_port_default_initialization();
    test_port_custom_values();
    test_port_types();
    test_port_infrastructure_levels();
    test_port_connection_flags();
    test_port_capacity_max();
    test_port_utilization_range();
    test_port_copy();
    test_port_memcpy_safe();

    printf("\n=== All PortComponent Tests Passed ===\n");
    return 0;
}
