/**
 * @file test_port_zone_component.cpp
 * @brief Unit tests for PortZoneComponent (Epic 8, Ticket E8-003)
 *
 * Tests cover:
 * - PortZoneComponent size assertion (16 bytes)
 * - Trivially copyable check
 * - Default initialization
 * - Custom value assignment
 * - Zone level range (0-4)
 * - Aero-specific requirements (runway)
 * - Aqua-specific requirements (dock)
 * - Runway area (GridRect) usage
 * - Copy semantics
 */

#include <sims3000/port/PortZoneComponent.h>
#include <cassert>
#include <cstdio>
#include <cstring>

using namespace sims3000::port;
using namespace sims3000::terrain;

void test_port_zone_component_size() {
    printf("Testing PortZoneComponent size...\n");

    assert(sizeof(PortZoneComponent) == 16);

    printf("  PASS: PortZoneComponent is 16 bytes\n");
}

void test_port_zone_trivially_copyable() {
    printf("Testing PortZoneComponent is trivially copyable...\n");

    assert(std::is_trivially_copyable<PortZoneComponent>::value);

    printf("  PASS: PortZoneComponent is trivially copyable\n");
}

void test_port_zone_default_initialization() {
    printf("Testing default initialization...\n");

    PortZoneComponent zone{};
    assert(zone.port_type == PortType::Aero);
    assert(zone.zone_level == 0);
    assert(zone.has_runway == false);
    assert(zone.has_dock == false);
    assert(zone.runway_length == 0);
    assert(zone.dock_count == 0);
    assert(zone.zone_tiles == 0);
    assert(zone.runway_area.x == 0);
    assert(zone.runway_area.y == 0);
    assert(zone.runway_area.width == 0);
    assert(zone.runway_area.height == 0);

    printf("  PASS: Default initialization works correctly\n");
}

void test_port_zone_custom_values() {
    printf("Testing custom value assignment...\n");

    PortZoneComponent zone{};
    zone.port_type = PortType::Aero;
    zone.zone_level = 3;
    zone.has_runway = true;
    zone.has_dock = false;
    zone.runway_length = 12;
    zone.dock_count = 0;
    zone.zone_tiles = 256;
    zone.runway_area.x = 10;
    zone.runway_area.y = 20;
    zone.runway_area.width = 12;
    zone.runway_area.height = 3;

    assert(zone.port_type == PortType::Aero);
    assert(zone.zone_level == 3);
    assert(zone.has_runway == true);
    assert(zone.has_dock == false);
    assert(zone.runway_length == 12);
    assert(zone.dock_count == 0);
    assert(zone.zone_tiles == 256);
    assert(zone.runway_area.x == 10);
    assert(zone.runway_area.y == 20);
    assert(zone.runway_area.width == 12);
    assert(zone.runway_area.height == 3);

    printf("  PASS: Custom value assignment works correctly\n");
}

void test_port_zone_levels() {
    printf("Testing zone levels 0-4...\n");

    PortZoneComponent zone{};

    for (uint8_t level = 0; level <= 4; ++level) {
        zone.zone_level = level;
        assert(zone.zone_level == level);
    }

    printf("  PASS: Zone levels 0-4 work correctly\n");
}

void test_port_zone_aero_requirements() {
    printf("Testing aero port requirements...\n");

    PortZoneComponent zone{};
    zone.port_type = PortType::Aero;
    zone.has_runway = true;
    zone.runway_length = 8;  // 8-tile runway
    zone.zone_tiles = 100;

    // Set runway area
    zone.runway_area = GridRect::fromCorners(5, 10, 13, 13);

    assert(zone.has_runway == true);
    assert(zone.runway_length == 8);
    assert(zone.runway_area.x == 5);
    assert(zone.runway_area.y == 10);
    assert(zone.runway_area.width == 8);
    assert(zone.runway_area.height == 3);

    // Aero ports don't need docks
    assert(zone.has_dock == false);
    assert(zone.dock_count == 0);

    printf("  PASS: Aero port requirements work correctly\n");
}

void test_port_zone_aqua_requirements() {
    printf("Testing aqua port requirements...\n");

    PortZoneComponent zone{};
    zone.port_type = PortType::Aqua;
    zone.has_dock = true;
    zone.dock_count = 4;
    zone.zone_tiles = 150;

    assert(zone.has_dock == true);
    assert(zone.dock_count == 4);

    // Aqua ports don't need runways
    assert(zone.has_runway == false);
    assert(zone.runway_length == 0);

    printf("  PASS: Aqua port requirements work correctly\n");
}

void test_port_zone_runway_area_gridrect() {
    printf("Testing runway_area GridRect operations...\n");

    PortZoneComponent zone{};
    zone.runway_area = GridRect::singleTile(5, 10);

    assert(zone.runway_area.x == 5);
    assert(zone.runway_area.y == 10);
    assert(zone.runway_area.width == 1);
    assert(zone.runway_area.height == 1);
    assert(!zone.runway_area.isEmpty());
    assert(zone.runway_area.contains(5, 10));
    assert(!zone.runway_area.contains(6, 10));

    // Use fromCorners
    zone.runway_area = GridRect::fromCorners(0, 0, 20, 3);
    assert(zone.runway_area.width == 20);
    assert(zone.runway_area.height == 3);
    assert(zone.runway_area.right() == 20);
    assert(zone.runway_area.bottom() == 3);
    assert(zone.runway_area.contains(0, 0));
    assert(zone.runway_area.contains(19, 2));
    assert(!zone.runway_area.contains(20, 0));

    printf("  PASS: Runway area GridRect operations work correctly\n");
}

void test_port_zone_empty_runway() {
    printf("Testing empty runway area...\n");

    PortZoneComponent zone{};
    assert(zone.runway_area.isEmpty());

    zone.runway_area.width = 10;
    zone.runway_area.height = 0;
    assert(zone.runway_area.isEmpty());

    zone.runway_area.width = 0;
    zone.runway_area.height = 3;
    assert(zone.runway_area.isEmpty());

    zone.runway_area.width = 10;
    zone.runway_area.height = 3;
    assert(!zone.runway_area.isEmpty());

    printf("  PASS: Empty runway area detection works correctly\n");
}

void test_port_zone_copy() {
    printf("Testing copy semantics...\n");

    PortZoneComponent original{};
    original.port_type = PortType::Aero;
    original.zone_level = 4;
    original.has_runway = true;
    original.has_dock = false;
    original.runway_length = 15;
    original.dock_count = 0;
    original.zone_tiles = 500;
    original.runway_area = GridRect::fromCorners(10, 20, 25, 23);

    PortZoneComponent copy = original;
    assert(copy.port_type == PortType::Aero);
    assert(copy.zone_level == 4);
    assert(copy.has_runway == true);
    assert(copy.has_dock == false);
    assert(copy.runway_length == 15);
    assert(copy.dock_count == 0);
    assert(copy.zone_tiles == 500);
    assert(copy.runway_area == original.runway_area);

    printf("  PASS: Copy semantics work correctly\n");
}

void test_port_zone_memcpy_safe() {
    printf("Testing memcpy safety...\n");

    PortZoneComponent original{};
    original.port_type = PortType::Aqua;
    original.zone_level = 2;
    original.has_dock = true;
    original.dock_count = 6;
    original.zone_tiles = 200;

    PortZoneComponent copy{};
    std::memcpy(&copy, &original, sizeof(PortZoneComponent));

    assert(copy.port_type == PortType::Aqua);
    assert(copy.zone_level == 2);
    assert(copy.has_dock == true);
    assert(copy.dock_count == 6);
    assert(copy.zone_tiles == 200);

    printf("  PASS: memcpy safety works correctly\n");
}

void test_port_zone_max_tiles() {
    printf("Testing max zone_tiles value...\n");

    PortZoneComponent zone{};
    zone.zone_tiles = UINT16_MAX;
    assert(zone.zone_tiles == UINT16_MAX);

    printf("  PASS: Max zone_tiles value works correctly\n");
}

int main() {
    printf("=== PortZoneComponent Unit Tests (Epic 8, Ticket E8-003) ===\n\n");

    test_port_zone_component_size();
    test_port_zone_trivially_copyable();
    test_port_zone_default_initialization();
    test_port_zone_custom_values();
    test_port_zone_levels();
    test_port_zone_aero_requirements();
    test_port_zone_aqua_requirements();
    test_port_zone_runway_area_gridrect();
    test_port_zone_empty_runway();
    test_port_zone_copy();
    test_port_zone_memcpy_safe();
    test_port_zone_max_tiles();

    printf("\n=== All PortZoneComponent Tests Passed ===\n");
    return 0;
}
