/**
 * @file test_port_capacity.cpp
 * @brief Unit tests for port capacity calculation (Epic 8, Ticket E8-010)
 *
 * Tests cover:
 * - Aero capacity scales with zone size and runway
 * - Aqua capacity scales with zone size, docks, and rail
 * - Maximum capacity capped per port type
 * - Edge cases (zero tiles, disconnected, etc.)
 */

#include <sims3000/port/PortCapacity.h>
#include <sims3000/port/PortZoneComponent.h>
#include <sims3000/port/PortTypes.h>
#include <cassert>
#include <cstdio>
#include <cmath>

using namespace sims3000::port;

// =============================================================================
// Helper: float comparison with tolerance
// =============================================================================

static bool approx_eq(float a, float b, float eps = 0.01f) {
    return std::fabs(a - b) < eps;
}

// =============================================================================
// Aero Port Capacity Tests
// =============================================================================

void test_aero_basic_capacity_with_runway_and_access() {
    printf("Testing aero basic capacity with runway and access...\n");

    PortZoneComponent zone;
    zone.port_type = PortType::Aero;
    zone.zone_tiles = 36;  // minimum 6x6
    zone.has_runway = true;

    // base = 36 * 10 = 360
    // runway_bonus = 1.5
    // access_bonus = 1.0
    // capacity = 360 * 1.5 * 1.0 = 540
    uint16_t cap = calculate_aero_capacity(zone, true);
    assert(cap == 540);

    printf("  PASS: Aero capacity = %u (expected 540)\n", cap);
}

void test_aero_capacity_without_runway() {
    printf("Testing aero capacity without runway...\n");

    PortZoneComponent zone;
    zone.port_type = PortType::Aero;
    zone.zone_tiles = 36;
    zone.has_runway = false;

    // base = 36 * 10 = 360
    // runway_bonus = 0.5
    // access_bonus = 1.0
    // capacity = 360 * 0.5 * 1.0 = 180
    uint16_t cap = calculate_aero_capacity(zone, true);
    assert(cap == 180);

    printf("  PASS: Aero capacity without runway = %u (expected 180)\n", cap);
}

void test_aero_capacity_without_access() {
    printf("Testing aero capacity without pathway access...\n");

    PortZoneComponent zone;
    zone.port_type = PortType::Aero;
    zone.zone_tiles = 100;
    zone.has_runway = true;

    // access_bonus = 0.0, so capacity = 0
    uint16_t cap = calculate_aero_capacity(zone, false);
    assert(cap == 0);

    printf("  PASS: Aero capacity without access = %u (expected 0)\n", cap);
}

void test_aero_capacity_scales_with_zone_size() {
    printf("Testing aero capacity scales with zone size...\n");

    PortZoneComponent zone;
    zone.port_type = PortType::Aero;
    zone.has_runway = true;

    // 36 tiles: 36 * 10 * 1.5 * 1.0 = 540
    zone.zone_tiles = 36;
    uint16_t cap_small = calculate_aero_capacity(zone, true);

    // 100 tiles: 100 * 10 * 1.5 * 1.0 = 1500
    zone.zone_tiles = 100;
    uint16_t cap_large = calculate_aero_capacity(zone, true);

    assert(cap_small == 540);
    assert(cap_large == 1500);
    assert(cap_large > cap_small);

    printf("  PASS: Capacity scales: %u (36 tiles) < %u (100 tiles)\n",
           cap_small, cap_large);
}

void test_aero_capacity_cap() {
    printf("Testing aero capacity capped at 2500...\n");

    PortZoneComponent zone;
    zone.port_type = PortType::Aero;
    zone.zone_tiles = 500;  // Very large zone
    zone.has_runway = true;

    // base = 500 * 10 = 5000
    // runway_bonus = 1.5
    // raw = 5000 * 1.5 * 1.0 = 7500 -> capped to 2500
    uint16_t cap = calculate_aero_capacity(zone, true);
    assert(cap == AERO_PORT_MAX_CAPACITY);
    assert(cap == 2500);

    printf("  PASS: Aero capacity capped at %u\n", cap);
}

void test_aero_zero_tiles() {
    printf("Testing aero capacity with zero tiles...\n");

    PortZoneComponent zone;
    zone.port_type = PortType::Aero;
    zone.zone_tiles = 0;
    zone.has_runway = true;

    uint16_t cap = calculate_aero_capacity(zone, true);
    assert(cap == 0);

    printf("  PASS: Aero capacity with zero tiles = %u\n", cap);
}

// =============================================================================
// Aqua Port Capacity Tests
// =============================================================================

void test_aqua_basic_capacity() {
    printf("Testing aqua basic capacity...\n");

    PortZoneComponent zone;
    zone.port_type = PortType::Aqua;
    zone.zone_tiles = 32;  // minimum
    zone.dock_count = 4;
    zone.has_dock = true;

    // base = 32 * 15 = 480
    // dock_bonus = 1.0 + (4 * 0.2) = 1.8
    // water_access = 1.0 (adjacent_water >= 4)
    // rail_bonus = 1.0 (no rail)
    // capacity = 480 * 1.8 * 1.0 * 1.0 = 864
    uint16_t cap = calculate_aqua_capacity(zone, 4, false);
    assert(cap == 864);

    printf("  PASS: Aqua capacity = %u (expected 864)\n", cap);
}

void test_aqua_capacity_with_rail() {
    printf("Testing aqua capacity with rail connection...\n");

    PortZoneComponent zone;
    zone.port_type = PortType::Aqua;
    zone.zone_tiles = 32;
    zone.dock_count = 4;

    // base = 32 * 15 = 480
    // dock_bonus = 1.0 + (4 * 0.2) = 1.8
    // water_access = 1.0
    // rail_bonus = 1.5
    // capacity = 480 * 1.8 * 1.0 * 1.5 = 1296
    uint16_t cap = calculate_aqua_capacity(zone, 4, true);
    assert(cap == 1296);

    printf("  PASS: Aqua capacity with rail = %u (expected 1296)\n", cap);
}

void test_aqua_capacity_low_water_access() {
    printf("Testing aqua capacity with low water access...\n");

    PortZoneComponent zone;
    zone.port_type = PortType::Aqua;
    zone.zone_tiles = 32;
    zone.dock_count = 4;

    // adjacent_water = 3 (< 4), water_access = 0.5
    // base = 480, dock_bonus = 1.8, water = 0.5, rail = 1.0
    // capacity = 480 * 1.8 * 0.5 * 1.0 = 432
    uint16_t cap = calculate_aqua_capacity(zone, 3, false);
    assert(cap == 432);

    printf("  PASS: Aqua capacity with low water = %u (expected 432)\n", cap);
}

void test_aqua_capacity_scales_with_docks() {
    printf("Testing aqua capacity scales with dock count...\n");

    PortZoneComponent zone;
    zone.port_type = PortType::Aqua;
    zone.zone_tiles = 32;

    // 0 docks: dock_bonus = 1.0
    zone.dock_count = 0;
    uint16_t cap0 = calculate_aqua_capacity(zone, 4, false);

    // 5 docks: dock_bonus = 1.0 + (5 * 0.2) = 2.0
    zone.dock_count = 5;
    uint16_t cap5 = calculate_aqua_capacity(zone, 4, false);

    // 10 docks: dock_bonus = 1.0 + (10 * 0.2) = 3.0
    zone.dock_count = 10;
    uint16_t cap10 = calculate_aqua_capacity(zone, 4, false);

    // 0 docks: 480 * 1.0 = 480
    // 5 docks: 480 * 2.0 = 960
    // 10 docks: 480 * 3.0 = 1440
    assert(cap0 == 480);
    assert(cap5 == 960);
    assert(cap10 == 1440);
    assert(cap0 < cap5 && cap5 < cap10);

    printf("  PASS: Capacity scales with docks: %u < %u < %u\n", cap0, cap5, cap10);
}

void test_aqua_capacity_scales_with_zone_size() {
    printf("Testing aqua capacity scales with zone size...\n");

    PortZoneComponent zone;
    zone.port_type = PortType::Aqua;
    zone.dock_count = 4;

    zone.zone_tiles = 32;
    uint16_t cap_small = calculate_aqua_capacity(zone, 4, false);

    zone.zone_tiles = 100;
    uint16_t cap_large = calculate_aqua_capacity(zone, 4, false);

    assert(cap_large > cap_small);

    printf("  PASS: Capacity scales: %u (32 tiles) < %u (100 tiles)\n",
           cap_small, cap_large);
}

void test_aqua_capacity_cap() {
    printf("Testing aqua capacity capped at 5000...\n");

    PortZoneComponent zone;
    zone.port_type = PortType::Aqua;
    zone.zone_tiles = 500;
    zone.dock_count = 20;  // dock_bonus = 1.0 + 4.0 = 5.0

    // base = 500 * 15 = 7500
    // dock_bonus = 5.0
    // water = 1.0
    // rail = 1.5
    // raw = 7500 * 5.0 * 1.0 * 1.5 = 56250 -> capped to 5000
    uint16_t cap = calculate_aqua_capacity(zone, 10, true);
    assert(cap == AQUA_PORT_MAX_CAPACITY);
    assert(cap == 5000);

    printf("  PASS: Aqua capacity capped at %u\n", cap);
}

void test_aqua_zero_tiles() {
    printf("Testing aqua capacity with zero tiles...\n");

    PortZoneComponent zone;
    zone.port_type = PortType::Aqua;
    zone.zone_tiles = 0;
    zone.dock_count = 4;

    uint16_t cap = calculate_aqua_capacity(zone, 4, false);
    assert(cap == 0);

    printf("  PASS: Aqua capacity with zero tiles = %u\n", cap);
}

// =============================================================================
// Generic dispatch and utility tests
// =============================================================================

void test_calculate_port_capacity_dispatch_aero() {
    printf("Testing calculate_port_capacity dispatches to aero...\n");

    PortZoneComponent zone;
    zone.port_type = PortType::Aero;
    zone.zone_tiles = 36;
    zone.has_runway = true;

    uint16_t cap = calculate_port_capacity(zone, true, 0, false);
    uint16_t expected = calculate_aero_capacity(zone, true);
    assert(cap == expected);

    printf("  PASS: Dispatch to aero: %u == %u\n", cap, expected);
}

void test_calculate_port_capacity_dispatch_aqua() {
    printf("Testing calculate_port_capacity dispatches to aqua...\n");

    PortZoneComponent zone;
    zone.port_type = PortType::Aqua;
    zone.zone_tiles = 32;
    zone.dock_count = 4;

    uint16_t cap = calculate_port_capacity(zone, true, 5, true);
    uint16_t expected = calculate_aqua_capacity(zone, 5, true);
    assert(cap == expected);

    printf("  PASS: Dispatch to aqua: %u == %u\n", cap, expected);
}

void test_get_max_capacity() {
    printf("Testing get_max_capacity...\n");

    assert(get_max_capacity(PortType::Aero) == 2500);
    assert(get_max_capacity(PortType::Aqua) == 5000);

    printf("  PASS: Aero max = 2500, Aqua max = 5000\n");
}

// =============================================================================
// Main
// =============================================================================

int main() {
    printf("=== Port Capacity Calculation Tests (Epic 8, Ticket E8-010) ===\n\n");

    // Aero tests
    test_aero_basic_capacity_with_runway_and_access();
    test_aero_capacity_without_runway();
    test_aero_capacity_without_access();
    test_aero_capacity_scales_with_zone_size();
    test_aero_capacity_cap();
    test_aero_zero_tiles();

    // Aqua tests
    test_aqua_basic_capacity();
    test_aqua_capacity_with_rail();
    test_aqua_capacity_low_water_access();
    test_aqua_capacity_scales_with_docks();
    test_aqua_capacity_scales_with_zone_size();
    test_aqua_capacity_cap();
    test_aqua_zero_tiles();

    // Dispatch and utility tests
    test_calculate_port_capacity_dispatch_aero();
    test_calculate_port_capacity_dispatch_aqua();
    test_get_max_capacity();

    printf("\n=== All Port Capacity Tests Passed ===\n");
    return 0;
}
