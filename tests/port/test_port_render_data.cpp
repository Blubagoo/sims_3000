/**
 * @file test_port_render_data.cpp
 * @brief Unit tests for port zone rendering support (Epic 8, Ticket E8-030)
 *
 * Tests cover:
 * - PortRenderData struct defaults
 * - RenderingSystem can query port visual state via get_port_render_data()
 * - Different visuals for development levels 0-4
 * - Aero ports show runway outline data
 * - Aqua ports show dock structures
 * - Boundary flags based on map edge proximity
 * - Operational status reflected in render data
 * - Port zone management (set/get)
 */

#include <sims3000/port/PortSystem.h>
#include <sims3000/port/PortRenderData.h>
#include <sims3000/port/PortZoneComponent.h>
#include <sims3000/port/DemandBonus.h>
#include <cassert>
#include <cstdio>

using namespace sims3000::port;

// =============================================================================
// Helpers
// =============================================================================

static int tests_passed = 0;
static int tests_total = 0;

#define TEST(name) \
    do { \
        tests_total++; \
        printf("  TEST: %s ... ", name); \
    } while(0)

#define PASS() \
    do { \
        tests_passed++; \
        printf("PASS\n"); \
    } while(0)

// =============================================================================
// PortRenderData Struct Tests
// =============================================================================

static void test_render_data_defaults() {
    TEST("PortRenderData: default values");
    PortRenderData rd;
    assert(rd.x == 0);
    assert(rd.y == 0);
    assert(rd.width == 0);
    assert(rd.height == 0);
    assert(rd.port_type == PortType::Aero);
    assert(rd.zone_level == 0);
    assert(rd.is_operational == false);
    assert(rd.boundary_flags == 0);
    assert(rd.runway_x == 0);
    assert(rd.runway_y == 0);
    assert(rd.runway_w == 0);
    assert(rd.runway_h == 0);
    assert(rd.dock_count == 0);
    PASS();
}

static void test_boundary_flag_constants() {
    TEST("boundary flags: correct bitmask values");
    assert(BOUNDARY_NORTH == 1);
    assert(BOUNDARY_SOUTH == 2);
    assert(BOUNDARY_EAST == 4);
    assert(BOUNDARY_WEST == 8);
    PASS();
}

// =============================================================================
// Port Zone Management Tests
// =============================================================================

static void test_set_get_port_zone() {
    TEST("set_port_zone/get_port_zone: round-trip");
    PortSystem sys(100, 100);

    PortZoneComponent zone;
    zone.port_type = PortType::Aero;
    zone.zone_level = 3;
    zone.has_runway = true;
    zone.runway_length = 10;
    zone.zone_tiles = 50;
    zone.runway_area = {5, 10, 20, 3};

    sys.set_port_zone(1, 10, 20, zone);

    PortZoneComponent out;
    bool found = sys.get_port_zone(1, 10, 20, out);
    assert(found);
    assert(out.zone_level == 3);
    assert(out.has_runway == true);
    assert(out.runway_length == 10);
    assert(out.zone_tiles == 50);
    assert(out.runway_area.x == 5);
    assert(out.runway_area.width == 20);
    PASS();
}

static void test_get_port_zone_not_found() {
    TEST("get_port_zone: returns false if not set");
    PortSystem sys(100, 100);
    PortZoneComponent out;
    bool found = sys.get_port_zone(1, 99, 99, out);
    assert(!found);
    PASS();
}

static void test_set_port_zone_update() {
    TEST("set_port_zone: update existing entry");
    PortSystem sys(100, 100);

    PortZoneComponent zone;
    zone.zone_level = 1;
    sys.set_port_zone(1, 10, 20, zone);

    zone.zone_level = 4;
    sys.set_port_zone(1, 10, 20, zone);

    PortZoneComponent out;
    bool found = sys.get_port_zone(1, 10, 20, out);
    assert(found);
    assert(out.zone_level == 4);
    PASS();
}

// =============================================================================
// get_port_render_data() Tests
// =============================================================================

static void test_render_data_empty() {
    TEST("get_port_render_data: empty system returns empty vector");
    PortSystem sys(100, 100);
    auto result = sys.get_port_render_data(1);
    assert(result.empty());
    PASS();
}

static void test_render_data_basic_port() {
    TEST("get_port_render_data: basic port data");
    PortSystem sys(100, 100);

    PortData port = {PortType::Aero, 1000, true, 1, 50, 50};
    sys.add_port(port);

    auto result = sys.get_port_render_data(1);
    assert(result.size() == 1);
    assert(result[0].x == 50);
    assert(result[0].y == 50);
    assert(result[0].port_type == PortType::Aero);
    assert(result[0].is_operational == true);
    PASS();
}

static void test_render_data_filters_by_owner() {
    TEST("get_port_render_data: filters by owner");
    PortSystem sys(100, 100);

    PortData p1 = {PortType::Aero, 1000, true, 1, 10, 10};
    PortData p2 = {PortType::Aqua, 2000, true, 2, 20, 20};
    sys.add_port(p1);
    sys.add_port(p2);

    auto result1 = sys.get_port_render_data(1);
    auto result2 = sys.get_port_render_data(2);

    assert(result1.size() == 1);
    assert(result1[0].port_type == PortType::Aero);
    assert(result2.size() == 1);
    assert(result2[0].port_type == PortType::Aqua);
    PASS();
}

static void test_render_data_operational_status() {
    TEST("get_port_render_data: reflects operational status");
    PortSystem sys(100, 100);

    PortData port_on = {PortType::Aero, 1000, true, 1, 10, 10};
    PortData port_off = {PortType::Aqua, 500, false, 1, 30, 30};
    sys.add_port(port_on);
    sys.add_port(port_off);

    auto result = sys.get_port_render_data(1);
    assert(result.size() == 2);
    assert(result[0].is_operational == true);
    assert(result[1].is_operational == false);
    PASS();
}

// =============================================================================
// Development Level Tests
// =============================================================================

static void test_render_data_zone_level() {
    TEST("get_port_render_data: zone level from PortZoneComponent");
    PortSystem sys(100, 100);

    PortData port = {PortType::Aero, 1000, true, 1, 10, 10};
    sys.add_port(port);

    PortZoneComponent zone;
    zone.port_type = PortType::Aero;
    zone.zone_level = 3;
    zone.zone_tiles = 25;
    sys.set_port_zone(1, 10, 10, zone);

    auto result = sys.get_port_render_data(1);
    assert(result.size() == 1);
    assert(result[0].zone_level == 3);
    PASS();
}

static void test_render_data_all_zone_levels() {
    TEST("get_port_render_data: different visuals for levels 0-4");
    PortSystem sys(100, 100);

    for (uint8_t level = 0; level <= 4; ++level) {
        PortData port = {PortType::Aero, 1000, true, 1,
                         static_cast<int32_t>(level * 10), 10};
        sys.add_port(port);

        PortZoneComponent zone;
        zone.zone_level = level;
        zone.zone_tiles = 10;
        sys.set_port_zone(1, level * 10, 10, zone);
    }

    auto result = sys.get_port_render_data(1);
    assert(result.size() == 5);
    for (uint8_t level = 0; level <= 4; ++level) {
        assert(result[level].zone_level == level);
    }
    PASS();
}

// =============================================================================
// Aero Port Runway Tests
// =============================================================================

static void test_render_data_aero_runway() {
    TEST("get_port_render_data: aero port shows runway outline");
    PortSystem sys(100, 100);

    PortData port = {PortType::Aero, 2000, true, 1, 10, 10};
    sys.add_port(port);

    PortZoneComponent zone;
    zone.port_type = PortType::Aero;
    zone.zone_level = 2;
    zone.has_runway = true;
    zone.runway_length = 15;
    zone.zone_tiles = 40;
    zone.runway_area = {12, 15, 15, 2};
    sys.set_port_zone(1, 10, 10, zone);

    auto result = sys.get_port_render_data(1);
    assert(result.size() == 1);
    assert(result[0].runway_x == 12);
    assert(result[0].runway_y == 15);
    assert(result[0].runway_w == 15);
    assert(result[0].runway_h == 2);
    assert(result[0].dock_count == 0);
    PASS();
}

// =============================================================================
// Aqua Port Dock Tests
// =============================================================================

static void test_render_data_aqua_docks() {
    TEST("get_port_render_data: aqua port shows dock structures");
    PortSystem sys(100, 100);

    PortData port = {PortType::Aqua, 1500, true, 1, 20, 20};
    sys.add_port(port);

    PortZoneComponent zone;
    zone.port_type = PortType::Aqua;
    zone.zone_level = 2;
    zone.has_dock = true;
    zone.dock_count = 4;
    zone.zone_tiles = 30;
    sys.set_port_zone(1, 20, 20, zone);

    auto result = sys.get_port_render_data(1);
    assert(result.size() == 1);
    assert(result[0].dock_count == 4);
    assert(result[0].runway_w == 0);
    assert(result[0].runway_h == 0);
    PASS();
}

// =============================================================================
// Boundary Flag Tests
// =============================================================================

static void test_render_data_boundary_north() {
    TEST("get_port_render_data: north boundary flag at y=0");
    PortSystem sys(100, 100);

    PortData port = {PortType::Aero, 500, true, 1, 50, 0};
    sys.add_port(port);

    auto result = sys.get_port_render_data(1);
    assert(result.size() == 1);
    assert((result[0].boundary_flags & BOUNDARY_NORTH) != 0);
    assert((result[0].boundary_flags & BOUNDARY_SOUTH) == 0);
    PASS();
}

static void test_render_data_boundary_south() {
    TEST("get_port_render_data: south boundary flag at y=map_height-1");
    PortSystem sys(100, 100);

    PortData port = {PortType::Aero, 500, true, 1, 50, 99};
    sys.add_port(port);

    auto result = sys.get_port_render_data(1);
    assert(result.size() == 1);
    assert((result[0].boundary_flags & BOUNDARY_SOUTH) != 0);
    assert((result[0].boundary_flags & BOUNDARY_NORTH) == 0);
    PASS();
}

static void test_render_data_boundary_east() {
    TEST("get_port_render_data: east boundary flag at x=map_width-1");
    PortSystem sys(100, 100);

    PortData port = {PortType::Aqua, 500, true, 1, 99, 50};
    sys.add_port(port);

    auto result = sys.get_port_render_data(1);
    assert(result.size() == 1);
    assert((result[0].boundary_flags & BOUNDARY_EAST) != 0);
    PASS();
}

static void test_render_data_boundary_west() {
    TEST("get_port_render_data: west boundary flag at x=0");
    PortSystem sys(100, 100);

    PortData port = {PortType::Aqua, 500, true, 1, 0, 50};
    sys.add_port(port);

    auto result = sys.get_port_render_data(1);
    assert(result.size() == 1);
    assert((result[0].boundary_flags & BOUNDARY_WEST) != 0);
    PASS();
}

static void test_render_data_boundary_corner() {
    TEST("get_port_render_data: corner has two boundary flags");
    PortSystem sys(100, 100);

    PortData port = {PortType::Aero, 500, true, 1, 0, 0};
    sys.add_port(port);

    auto result = sys.get_port_render_data(1);
    assert(result.size() == 1);
    assert((result[0].boundary_flags & BOUNDARY_NORTH) != 0);
    assert((result[0].boundary_flags & BOUNDARY_WEST) != 0);
    assert((result[0].boundary_flags & BOUNDARY_SOUTH) == 0);
    assert((result[0].boundary_flags & BOUNDARY_EAST) == 0);
    PASS();
}

static void test_render_data_no_boundary_interior() {
    TEST("get_port_render_data: interior port has no boundary flags");
    PortSystem sys(100, 100);

    PortData port = {PortType::Aero, 500, true, 1, 50, 50};
    sys.add_port(port);

    auto result = sys.get_port_render_data(1);
    assert(result.size() == 1);
    assert(result[0].boundary_flags == 0);
    PASS();
}

// =============================================================================
// No Zone Data Fallback Tests
// =============================================================================

static void test_render_data_no_zone_defaults() {
    TEST("get_port_render_data: no zone data uses minimal defaults");
    PortSystem sys(100, 100);

    PortData port = {PortType::Aero, 1000, true, 1, 50, 50};
    sys.add_port(port);
    // Intentionally do not set zone data

    auto result = sys.get_port_render_data(1);
    assert(result.size() == 1);
    assert(result[0].zone_level == 0);
    assert(result[0].width == 1);
    assert(result[0].height == 1);
    assert(result[0].dock_count == 0);
    PASS();
}

// =============================================================================
// Main
// =============================================================================

int main() {
    printf("=== Port Render Data Tests (E8-030) ===\n");

    // Struct defaults
    test_render_data_defaults();
    test_boundary_flag_constants();

    // Zone management
    test_set_get_port_zone();
    test_get_port_zone_not_found();
    test_set_port_zone_update();

    // Basic render data queries
    test_render_data_empty();
    test_render_data_basic_port();
    test_render_data_filters_by_owner();
    test_render_data_operational_status();

    // Development levels
    test_render_data_zone_level();
    test_render_data_all_zone_levels();

    // Aero runway
    test_render_data_aero_runway();

    // Aqua docks
    test_render_data_aqua_docks();

    // Boundary flags
    test_render_data_boundary_north();
    test_render_data_boundary_south();
    test_render_data_boundary_east();
    test_render_data_boundary_west();
    test_render_data_boundary_corner();
    test_render_data_no_boundary_interior();

    // Fallback defaults
    test_render_data_no_zone_defaults();

    printf("\n=== Results: %d/%d passed ===\n", tests_passed, tests_total);
    return (tests_passed == tests_total) ? 0 : 1;
}
