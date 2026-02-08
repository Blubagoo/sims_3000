/**
 * @file test_port_validation_comprehensive.cpp
 * @brief Comprehensive unit tests for port zone validation (Epic 8, Ticket E8-036)
 *
 * Edge-case and regression tests for PortZoneValidation, PortCapacity:
 *
 * Aero port size validation edge cases:
 * - Exactly 35 tiles (one below minimum)
 * - Exactly 36 tiles in non-square shapes
 * - Very wide zones (2xN) with area >= 36
 * - Very tall zones (Nx2) with area >= 36
 * - Single-tile dimension zones
 *
 * Aero port runway detection edge cases:
 * - Zone exactly 6 wide x 2 tall (36 tiles from 6x6, runway barely fits)
 * - Runway at exact zone boundary
 * - Runway only available vertically (wide zone can't fit horizontal)
 * - Multiple flat strips of varying sizes
 * - All elevations different (no valid runway)
 * - Single flat row spanning full width (too narrow for 2-wide runway)
 * - Runway at corner positions
 *
 * Aqua port water adjacency edge cases:
 * - Zone at map coordinate (0,0) with water on south/east only
 * - Water on all four edges
 * - Mixed water types across edges
 * - Water on north edge only (requires zone.y > 0)
 * - Zone at origin with no north/west water possible
 * - Single-column zone
 *
 * Aqua port dock requirement edge cases:
 * - Exactly 3 dock tiles (one below minimum)
 * - Water tiles not immediately adjacent (gap between zone and water)
 * - Dock tiles from multiple edges summing to exactly 4
 * - Large zone with water only on one corner-adjacent tile
 *
 * Capacity calculation boundary values:
 * - Aero capacity at exact cap boundary (zone_tiles that produce exactly 2500)
 * - Aqua capacity at exact cap boundary
 * - Capacity with maximum possible dock count (uint8_t max)
 * - Capacity with 1 tile (minimum non-zero)
 * - Combined multiplier overflow edge cases
 */

#include <sims3000/port/PortZoneValidation.h>
#include <sims3000/port/PortCapacity.h>
#include <sims3000/port/PortZoneComponent.h>
#include <sims3000/port/PortTypes.h>
#include <sims3000/terrain/ITerrainQueryable.h>
#include <sims3000/building/ForwardDependencyInterfaces.h>
#include <sims3000/terrain/TerrainTypes.h>
#include <sims3000/terrain/TerrainEvents.h>
#include <cassert>
#include <cstdio>
#include <cstdint>
#include <cmath>
#include <vector>
#include <unordered_map>

using namespace sims3000::port;
using namespace sims3000::terrain;
using namespace sims3000::building;

// =============================================================================
// Mock ITerrainQueryable
// =============================================================================

class MockTerrain : public ITerrainQueryable {
public:
    MockTerrain()
        : m_default_elevation(10)
        , m_default_type(TerrainType::Substrate)
        , m_map_width(128)
        , m_map_height(128)
    {}

    void set_elevation(int32_t x, int32_t y, uint8_t elev) {
        m_elevations[key(x, y)] = elev;
    }

    void set_default_elevation(uint8_t elev) { m_default_elevation = elev; }

    void set_terrain_type(int32_t x, int32_t y, TerrainType type) {
        m_types[key(x, y)] = type;
    }

    void set_default_terrain_type(TerrainType type) { m_default_type = type; }

    TerrainType get_terrain_type(int32_t x, int32_t y) const override {
        auto it = m_types.find(key(x, y));
        return it != m_types.end() ? it->second : m_default_type;
    }

    uint8_t get_elevation(int32_t x, int32_t y) const override {
        auto it = m_elevations.find(key(x, y));
        return it != m_elevations.end() ? it->second : m_default_elevation;
    }

    bool is_buildable(int32_t, int32_t) const override { return true; }
    uint8_t get_slope(int32_t, int32_t, int32_t, int32_t) const override { return 0; }
    float get_average_elevation(int32_t, int32_t, uint32_t) const override { return 10.0f; }
    uint32_t get_water_distance(int32_t, int32_t) const override { return 255; }
    float get_value_bonus(int32_t, int32_t) const override { return 0.0f; }
    float get_harmony_bonus(int32_t, int32_t) const override { return 0.0f; }
    int32_t get_build_cost_modifier(int32_t, int32_t) const override { return 100; }
    uint32_t get_contamination_output(int32_t, int32_t) const override { return 0; }
    uint32_t get_map_width() const override { return m_map_width; }
    uint32_t get_map_height() const override { return m_map_height; }
    uint8_t get_sea_level() const override { return 8; }

    void get_tiles_in_rect(const GridRect&,
                            std::vector<TerrainComponent>& out) const override {
        out.clear();
    }

    uint32_t get_buildable_tiles_in_rect(const GridRect&) const override { return 0; }
    uint32_t count_terrain_type_in_rect(const GridRect&, TerrainType) const override { return 0; }

private:
    static uint64_t key(int32_t x, int32_t y) {
        return (static_cast<uint64_t>(static_cast<uint32_t>(x)) << 32) |
               static_cast<uint64_t>(static_cast<uint32_t>(y));
    }

    uint8_t m_default_elevation;
    TerrainType m_default_type;
    uint32_t m_map_width;
    uint32_t m_map_height;
    std::unordered_map<uint64_t, uint8_t> m_elevations;
    std::unordered_map<uint64_t, TerrainType> m_types;
};

// =============================================================================
// Mock ITransportProvider
// =============================================================================

class MockTransport : public ITransportProvider {
public:
    MockTransport() : m_accessible(true) {}

    void set_accessible(bool accessible) { m_accessible = accessible; }

    void set_accessible_at(uint32_t x, uint32_t y, bool accessible) {
        uint64_t k = (static_cast<uint64_t>(x) << 32) | static_cast<uint64_t>(y);
        m_tile_accessibility[k] = accessible;
    }

    bool is_road_accessible_at(uint32_t x, uint32_t y, uint32_t) const override {
        uint64_t k = (static_cast<uint64_t>(x) << 32) | static_cast<uint64_t>(y);
        auto it = m_tile_accessibility.find(k);
        if (it != m_tile_accessibility.end()) {
            return it->second;
        }
        return m_accessible;
    }

    uint32_t get_nearest_road_distance(uint32_t, uint32_t) const override {
        return m_accessible ? 1 : 255;
    }

private:
    bool m_accessible;
    std::unordered_map<uint64_t, bool> m_tile_accessibility;
};

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

static bool approx_eq(float a, float b, float eps = 0.01f) {
    return std::fabs(a - b) < eps;
}

/// Place water tiles along the south edge of the zone (outside boundary)
static void place_water_south(MockTerrain& terrain, const GridRect& zone,
                               int16_t count,
                               TerrainType water_type = TerrainType::StillBasin) {
    int16_t y = zone.bottom();
    for (int16_t i = 0; i < count && i < static_cast<int16_t>(zone.width); ++i) {
        terrain.set_terrain_type(zone.x + i, y, water_type);
    }
}

/// Place water tiles along the north edge of the zone (outside boundary)
static void place_water_north(MockTerrain& terrain, const GridRect& zone,
                               int16_t count,
                               TerrainType water_type = TerrainType::StillBasin) {
    if (zone.y <= 0) return;  // Cannot place water above row 0
    int16_t y = zone.y - 1;
    for (int16_t i = 0; i < count && i < static_cast<int16_t>(zone.width); ++i) {
        terrain.set_terrain_type(zone.x + i, y, water_type);
    }
}

/// Place water tiles along the east edge of the zone (outside boundary)
static void place_water_east(MockTerrain& terrain, const GridRect& zone,
                              int16_t count,
                              TerrainType water_type = TerrainType::StillBasin) {
    int16_t x = zone.right();
    for (int16_t i = 0; i < count && i < static_cast<int16_t>(zone.height); ++i) {
        terrain.set_terrain_type(x, zone.y + i, water_type);
    }
}

/// Place water tiles along the west edge of the zone (outside boundary)
static void place_water_west(MockTerrain& terrain, const GridRect& zone,
                              int16_t count,
                              TerrainType water_type = TerrainType::StillBasin) {
    if (zone.x <= 0) return;  // Cannot place water left of column 0
    int16_t x = zone.x - 1;
    for (int16_t i = 0; i < count && i < static_cast<int16_t>(zone.height); ++i) {
        terrain.set_terrain_type(x, zone.y + i, water_type);
    }
}

// =============================================================================
// AERO PORT SIZE VALIDATION EDGE CASES
// =============================================================================

static void test_aero_exactly_35_tiles_rejected() {
    TEST("aero: exactly 35 tiles (one below minimum) rejected");
    MockTerrain terrain;
    terrain.set_default_elevation(10);
    MockTransport transport;

    // 5x7 = 35 tiles
    GridRect zone;
    zone.x = 0; zone.y = 0; zone.width = 5; zone.height = 7;
    assert(!validate_aero_port_zone(zone, terrain, transport));
    PASS();
}

static void test_aero_36_tiles_non_square_9x4() {
    TEST("aero: 9x4 = 36 tiles (non-square, flat) accepted");
    MockTerrain terrain;
    terrain.set_default_elevation(10);
    MockTransport transport;

    // 9x4 = 36 tiles, width >= 6 and height >= 2, so horizontal runway fits
    GridRect zone;
    zone.x = 0; zone.y = 0; zone.width = 9; zone.height = 4;
    assert(validate_aero_port_zone(zone, terrain, transport));
    PASS();
}

static void test_aero_36_tiles_non_square_4x9() {
    TEST("aero: 4x9 = 36 tiles (non-square, tall) accepted with vertical runway");
    MockTerrain terrain;
    terrain.set_default_elevation(10);
    MockTransport transport;

    // 4x9: height >= 6, width >= 2 so vertical runway fits
    GridRect zone;
    zone.x = 0; zone.y = 0; zone.width = 4; zone.height = 9;
    assert(validate_aero_port_zone(zone, terrain, transport));
    PASS();
}

static void test_aero_36_tiles_12x3() {
    TEST("aero: 12x3 = 36 tiles, height < runway width for horizontal, width >= 6 but height < 2 for vert");
    MockTerrain terrain;
    terrain.set_default_elevation(10);
    MockTransport transport;

    // 12x3: horizontal: width(12)>=6 and height(3)>=2 -> fits
    GridRect zone;
    zone.x = 0; zone.y = 0; zone.width = 12; zone.height = 3;
    assert(validate_aero_port_zone(zone, terrain, transport));
    PASS();
}

static void test_aero_very_wide_2x18() {
    TEST("aero: 2x18 = 36 tiles, runway needs 6 long and 2 wide");
    MockTerrain terrain;
    terrain.set_default_elevation(10);
    MockTransport transport;

    // 2x18: horizontal: width(2)<6 fail. vertical: height(18)>=6 and width(2)>=2 -> fits
    GridRect zone;
    zone.x = 0; zone.y = 0; zone.width = 2; zone.height = 18;
    assert(validate_aero_port_zone(zone, terrain, transport));
    PASS();
}

static void test_aero_18x2() {
    TEST("aero: 18x2 = 36 tiles, horizontal runway fits (width>=6, height>=2)");
    MockTerrain terrain;
    terrain.set_default_elevation(10);
    MockTransport transport;

    GridRect zone;
    zone.x = 0; zone.y = 0; zone.width = 18; zone.height = 2;
    assert(validate_aero_port_zone(zone, terrain, transport));
    PASS();
}

static void test_aero_1x36_rejected_too_narrow() {
    TEST("aero: 1x36 = 36 tiles but width=1, cannot fit 2-wide runway");
    MockTerrain terrain;
    terrain.set_default_elevation(10);
    MockTransport transport;

    // Width=1 is too narrow for any runway orientation
    // Horizontal: width(1)<6. Vertical: width(1)<2.
    GridRect zone;
    zone.x = 0; zone.y = 0; zone.width = 1; zone.height = 36;
    assert(!validate_aero_port_zone(zone, terrain, transport));
    PASS();
}

static void test_aero_36x1_rejected_too_short() {
    TEST("aero: 36x1 = 36 tiles but height=1, cannot fit 2-wide runway");
    MockTerrain terrain;
    terrain.set_default_elevation(10);
    MockTransport transport;

    // Horizontal: width(36)>=6 but height(1)<2. Vertical: height(1)<6.
    GridRect zone;
    zone.x = 0; zone.y = 0; zone.width = 36; zone.height = 1;
    assert(!validate_aero_port_zone(zone, terrain, transport));
    PASS();
}

// =============================================================================
// AERO PORT RUNWAY DETECTION EDGE CASES
// =============================================================================

static void test_aero_runway_barely_fits_exact_6x2_at_corner() {
    TEST("aero: runway barely fits (6x2) at zone origin");
    MockTerrain terrain;
    // Most terrain is uneven, but a 6x2 strip at (0,0)-(5,1) is flat
    for (int16_t y = 0; y < 8; ++y) {
        for (int16_t x = 0; x < 8; ++x) {
            terrain.set_elevation(x, y, static_cast<uint8_t>((x * 7 + y * 13) % 20 + 5));
        }
    }
    // Create flat 6x2 strip at top-left
    for (int16_t y = 0; y < 2; ++y) {
        for (int16_t x = 0; x < 6; ++x) {
            terrain.set_elevation(x, y, 10);
        }
    }
    MockTransport transport;

    GridRect zone;
    zone.x = 0; zone.y = 0; zone.width = 8; zone.height = 8;
    assert(validate_aero_port_zone(zone, terrain, transport));
    PASS();
}

static void test_aero_runway_barely_fits_at_bottom_right() {
    TEST("aero: runway barely fits (6x2) at bottom-right corner of zone");
    MockTerrain terrain;
    // All terrain varied
    for (int16_t y = 0; y < 8; ++y) {
        for (int16_t x = 0; x < 8; ++x) {
            terrain.set_elevation(x, y, static_cast<uint8_t>((x * 3 + y * 7) % 10 + 5));
        }
    }
    // Flat 6x2 strip at bottom-right: x=2..7, y=6..7
    for (int16_t y = 6; y <= 7; ++y) {
        for (int16_t x = 2; x <= 7; ++x) {
            terrain.set_elevation(x, y, 15);
        }
    }
    MockTransport transport;

    GridRect zone;
    zone.x = 0; zone.y = 0; zone.width = 8; zone.height = 8;
    assert(validate_aero_port_zone(zone, terrain, transport));
    PASS();
}

static void test_aero_runway_5x2_does_not_fit() {
    TEST("aero: 5x2 flat strip is not long enough for runway");
    MockTerrain terrain;
    // All varied
    for (int16_t y = 0; y < 8; ++y) {
        for (int16_t x = 0; x < 8; ++x) {
            terrain.set_elevation(x, y, static_cast<uint8_t>(x + y * 2));
        }
    }
    // Flat 5x2 strip (not 6)
    for (int16_t y = 3; y <= 4; ++y) {
        for (int16_t x = 1; x <= 5; ++x) {
            terrain.set_elevation(x, y, 10);
        }
    }
    MockTransport transport;

    GridRect zone;
    zone.x = 0; zone.y = 0; zone.width = 8; zone.height = 8;
    // No valid 6x2 or 2x6 flat area exists
    assert(!validate_aero_port_zone(zone, terrain, transport));
    PASS();
}

static void test_aero_vertical_runway_only() {
    TEST("aero: only vertical runway fits (zone is 3 wide, 12 tall)");
    MockTerrain terrain;
    // Make horizontal impossible: width=3 < 6, so horizontal won't fit
    // Vertical: height(12) >= 6, width(3) >= 2
    terrain.set_default_elevation(10);
    MockTransport transport;

    GridRect zone;
    zone.x = 0; zone.y = 0; zone.width = 3; zone.height = 12;
    assert(validate_aero_port_zone(zone, terrain, transport));
    PASS();
}

static void test_aero_all_elevations_different() {
    TEST("aero: all unique elevations - no valid runway anywhere");
    MockTerrain terrain;
    // Give every tile a unique elevation
    uint8_t elev = 0;
    for (int16_t y = 0; y < 6; ++y) {
        for (int16_t x = 0; x < 6; ++x) {
            terrain.set_elevation(x, y, elev++);
        }
    }
    MockTransport transport;

    GridRect zone;
    zone.x = 0; zone.y = 0; zone.width = 6; zone.height = 6;
    assert(!validate_aero_port_zone(zone, terrain, transport));
    PASS();
}

static void test_aero_single_flat_row_too_narrow() {
    TEST("aero: single flat row (6x1) is too narrow for 2-wide runway");
    MockTerrain terrain;
    // All varied, then one flat row
    for (int16_t y = 0; y < 6; ++y) {
        for (int16_t x = 0; x < 6; ++x) {
            terrain.set_elevation(x, y, static_cast<uint8_t>((x + y) % 2 == 0 ? 10 : 15));
        }
    }
    // Flat row at y=3 (6 tiles wide, but only 1 row high)
    for (int16_t x = 0; x < 6; ++x) {
        terrain.set_elevation(x, 3, 20);
    }
    // But the adjacent rows are not flat with elevation 20
    MockTransport transport;

    GridRect zone;
    zone.x = 0; zone.y = 0; zone.width = 6; zone.height = 6;
    // No 6x2 or 2x6 flat area exists - still checkerboard except row 3
    assert(!validate_aero_port_zone(zone, terrain, transport));
    PASS();
}

static void test_aero_two_adjacent_flat_rows_form_runway() {
    TEST("aero: two adjacent flat rows form valid 6x2 runway");
    MockTerrain terrain;
    // Checkerboard pattern
    for (int16_t y = 0; y < 6; ++y) {
        for (int16_t x = 0; x < 6; ++x) {
            terrain.set_elevation(x, y, static_cast<uint8_t>((x + y) % 2 == 0 ? 10 : 15));
        }
    }
    // Override rows 3 and 4 to be flat at elevation 20
    for (int16_t x = 0; x < 6; ++x) {
        terrain.set_elevation(x, 3, 20);
        terrain.set_elevation(x, 4, 20);
    }
    MockTransport transport;

    GridRect zone;
    zone.x = 0; zone.y = 0; zone.width = 6; zone.height = 6;
    assert(validate_aero_port_zone(zone, terrain, transport));
    PASS();
}

static void test_aero_flat_strip_at_elevation_zero() {
    TEST("aero: flat runway at elevation 0 is valid");
    MockTerrain terrain;
    // All tiles at different elevations
    for (int16_t y = 0; y < 6; ++y) {
        for (int16_t x = 0; x < 6; ++x) {
            terrain.set_elevation(x, y, static_cast<uint8_t>(x + y + 5));
        }
    }
    // Create flat 6x2 strip at elevation 0
    for (int16_t y = 0; y < 2; ++y) {
        for (int16_t x = 0; x < 6; ++x) {
            terrain.set_elevation(x, y, 0);
        }
    }
    MockTransport transport;

    GridRect zone;
    zone.x = 0; zone.y = 0; zone.width = 6; zone.height = 6;
    assert(validate_aero_port_zone(zone, terrain, transport));
    PASS();
}

static void test_aero_flat_strip_at_elevation_255() {
    TEST("aero: flat runway at elevation 255 (max uint8_t) is valid");
    MockTerrain terrain;
    for (int16_t y = 0; y < 6; ++y) {
        for (int16_t x = 0; x < 6; ++x) {
            terrain.set_elevation(x, y, static_cast<uint8_t>(x + y + 1));
        }
    }
    // Flat 6x2 at elevation 255
    for (int16_t y = 0; y < 2; ++y) {
        for (int16_t x = 0; x < 6; ++x) {
            terrain.set_elevation(x, y, 255);
        }
    }
    MockTransport transport;

    GridRect zone;
    zone.x = 0; zone.y = 0; zone.width = 6; zone.height = 6;
    assert(validate_aero_port_zone(zone, terrain, transport));
    PASS();
}

static void test_aero_multiple_flat_strips_only_one_valid() {
    TEST("aero: multiple flat strips, only one is 6 tiles long");
    MockTerrain terrain;
    for (int16_t y = 0; y < 8; ++y) {
        for (int16_t x = 0; x < 8; ++x) {
            terrain.set_elevation(x, y, static_cast<uint8_t>(x * 3 + y * 5));
        }
    }
    // 4x2 flat strip at y=1,2 x=0..3 (too short)
    for (int16_t y = 1; y <= 2; ++y) {
        for (int16_t x = 0; x <= 3; ++x) {
            terrain.set_elevation(x, y, 10);
        }
    }
    // 6x2 flat strip at y=5,6 x=1..6 (valid)
    for (int16_t y = 5; y <= 6; ++y) {
        for (int16_t x = 1; x <= 6; ++x) {
            terrain.set_elevation(x, y, 20);
        }
    }
    MockTransport transport;

    GridRect zone;
    zone.x = 0; zone.y = 0; zone.width = 8; zone.height = 8;
    assert(validate_aero_port_zone(zone, terrain, transport));
    PASS();
}

// =============================================================================
// AQUA PORT WATER ADJACENCY EDGE CASES
// =============================================================================

static void test_aqua_zone_at_origin_water_south_only() {
    TEST("aqua: zone at (0,0), water on south edge only");
    MockTerrain terrain;
    MockTransport transport;

    // Zone at origin: no north or west water possible at the boundary
    GridRect zone;
    zone.x = 0; zone.y = 0; zone.width = 8; zone.height = 4;
    place_water_south(terrain, zone, 4);
    assert(validate_aqua_port_zone(zone, terrain, transport));
    PASS();
}

static void test_aqua_water_on_all_four_edges() {
    TEST("aqua: water on all four edges");
    MockTerrain terrain;
    MockTransport transport;

    GridRect zone;
    zone.x = 10; zone.y = 10; zone.width = 8; zone.height = 4;
    // 1 tile per edge = 4 total
    place_water_north(terrain, zone, 1);
    place_water_south(terrain, zone, 1);
    place_water_east(terrain, zone, 1);
    place_water_west(terrain, zone, 1);
    assert(validate_aqua_port_zone(zone, terrain, transport));
    PASS();
}

static void test_aqua_mixed_water_types_across_edges() {
    TEST("aqua: different water types on different edges");
    MockTerrain terrain;
    MockTransport transport;

    GridRect zone;
    zone.x = 10; zone.y = 10; zone.width = 8; zone.height = 4;
    place_water_south(terrain, zone, 1, TerrainType::DeepVoid);
    place_water_south(terrain, zone, 1, TerrainType::DeepVoid);  // overwriting same tile
    place_water_east(terrain, zone, 1, TerrainType::FlowChannel);
    place_water_north(terrain, zone, 1, TerrainType::StillBasin);
    place_water_west(terrain, zone, 1, TerrainType::DeepVoid);
    assert(validate_aqua_port_zone(zone, terrain, transport));
    PASS();
}

static void test_aqua_water_on_north_edge_only() {
    TEST("aqua: water on north edge only (zone not at origin)");
    MockTerrain terrain;
    MockTransport transport;

    GridRect zone;
    zone.x = 10; zone.y = 10; zone.width = 8; zone.height = 4;
    place_water_north(terrain, zone, 4);
    assert(validate_aqua_port_zone(zone, terrain, transport));
    PASS();
}

static void test_aqua_zone_at_origin_no_north_west_water() {
    TEST("aqua: zone at (0,0), cannot have north or west water");
    MockTerrain terrain;
    MockTransport transport;

    GridRect zone;
    zone.x = 0; zone.y = 0; zone.width = 8; zone.height = 4;
    // Only south and east edges can have water
    // Place 2 on south, 2 on east = 4
    place_water_south(terrain, zone, 2);
    place_water_east(terrain, zone, 2);
    assert(validate_aqua_port_zone(zone, terrain, transport));
    PASS();
}

static void test_aqua_zone_at_origin_no_water_anywhere() {
    TEST("aqua: zone at (0,0), no water -> rejected");
    MockTerrain terrain;
    MockTransport transport;

    GridRect zone;
    zone.x = 0; zone.y = 0; zone.width = 8; zone.height = 4;
    // No water placed
    assert(!validate_aqua_port_zone(zone, terrain, transport));
    PASS();
}

static void test_aqua_single_column_zone() {
    TEST("aqua: single-column zone (1x32), water on east provides 4 dock tiles");
    MockTerrain terrain;
    MockTransport transport;

    GridRect zone;
    zone.x = 5; zone.y = 5; zone.width = 1; zone.height = 32;
    place_water_east(terrain, zone, 4);
    assert(validate_aqua_port_zone(zone, terrain, transport));
    PASS();
}

static void test_aqua_single_row_zone() {
    TEST("aqua: single-row zone (32x1), water on south provides dock tiles");
    MockTerrain terrain;
    MockTransport transport;

    GridRect zone;
    zone.x = 5; zone.y = 5; zone.width = 32; zone.height = 1;
    place_water_south(terrain, zone, 4);
    assert(validate_aqua_port_zone(zone, terrain, transport));
    PASS();
}

// =============================================================================
// AQUA PORT DOCK REQUIREMENT EDGE CASES
// =============================================================================

static void test_aqua_exactly_3_dock_tiles_rejected() {
    TEST("aqua: exactly 3 dock tiles (one below minimum) rejected");
    MockTerrain terrain;
    MockTransport transport;

    GridRect zone;
    zone.x = 10; zone.y = 10; zone.width = 8; zone.height = 4;
    place_water_south(terrain, zone, 3);
    assert(!validate_aqua_port_zone(zone, terrain, transport));
    PASS();
}

static void test_aqua_dock_tiles_from_multiple_edges_sum_to_4() {
    TEST("aqua: dock tiles from 3 edges (1+1+2) sum to 4");
    MockTerrain terrain;
    MockTransport transport;

    GridRect zone;
    zone.x = 10; zone.y = 10; zone.width = 8; zone.height = 4;
    place_water_north(terrain, zone, 1);
    place_water_east(terrain, zone, 1);
    place_water_south(terrain, zone, 2);
    assert(validate_aqua_port_zone(zone, terrain, transport));
    PASS();
}

static void test_aqua_large_zone_minimal_water() {
    TEST("aqua: large zone (16x16) with exactly 4 dock tiles on one edge");
    MockTerrain terrain;
    MockTransport transport;

    GridRect zone;
    zone.x = 10; zone.y = 10; zone.width = 16; zone.height = 16;
    place_water_south(terrain, zone, 4);
    assert(validate_aqua_port_zone(zone, terrain, transport));
    PASS();
}

static void test_aqua_water_not_immediately_adjacent_rejected() {
    TEST("aqua: water tiles 2 rows below zone (not adjacent) rejected");
    MockTerrain terrain;
    MockTransport transport;

    GridRect zone;
    zone.x = 10; zone.y = 10; zone.width = 8; zone.height = 4;
    // Place water 2 rows below zone bottom instead of immediately adjacent
    int16_t gap_y = zone.bottom() + 1;
    for (int16_t i = 0; i < 8; ++i) {
        terrain.set_terrain_type(zone.x + i, gap_y, TerrainType::StillBasin);
    }
    // No immediately adjacent water
    assert(!validate_aqua_port_zone(zone, terrain, transport));
    PASS();
}

static void test_aqua_all_perimeter_water() {
    TEST("aqua: water surrounding entire perimeter");
    MockTerrain terrain;
    MockTransport transport;

    GridRect zone;
    zone.x = 10; zone.y = 10; zone.width = 8; zone.height = 4;
    place_water_north(terrain, zone, 8);
    place_water_south(terrain, zone, 8);
    place_water_east(terrain, zone, 4);
    place_water_west(terrain, zone, 4);
    assert(validate_aqua_port_zone(zone, terrain, transport));
    PASS();
}

static void test_aqua_water_inside_zone_doesnt_count() {
    TEST("aqua: water inside zone boundary does not count as dock tiles");
    MockTerrain terrain;
    MockTransport transport;

    GridRect zone;
    zone.x = 10; zone.y = 10; zone.width = 8; zone.height = 4;
    // Place water inside the zone (should not count as dock tiles)
    for (int16_t y = 10; y < 14; ++y) {
        for (int16_t x = 10; x < 18; ++x) {
            terrain.set_terrain_type(x, y, TerrainType::StillBasin);
        }
    }
    // No water outside the zone boundary
    assert(!validate_aqua_port_zone(zone, terrain, transport));
    PASS();
}

// =============================================================================
// AERO PORT PATHWAY EDGE CASES
// =============================================================================

static void test_aero_pathway_on_single_perimeter_tile_only() {
    TEST("aero: pathway accessible on only one perimeter tile");
    MockTerrain terrain;
    terrain.set_default_elevation(10);
    MockTransport transport;
    transport.set_accessible(false);
    // Only bottom-right corner has road access
    transport.set_accessible_at(15, 15, true);

    GridRect zone;
    zone.x = 10; zone.y = 10; zone.width = 6; zone.height = 6;
    assert(validate_aero_port_zone(zone, terrain, transport));
    PASS();
}

static void test_aqua_pathway_on_single_perimeter_tile_only() {
    TEST("aqua: pathway accessible on only one perimeter tile");
    MockTerrain terrain;
    MockTransport transport;
    transport.set_accessible(false);
    transport.set_accessible_at(10, 10, true);

    GridRect zone;
    zone.x = 10; zone.y = 10; zone.width = 8; zone.height = 4;
    place_water_south(terrain, zone, 4);
    assert(validate_aqua_port_zone(zone, terrain, transport));
    PASS();
}

// =============================================================================
// CAPACITY CALCULATION BOUNDARY VALUES
// =============================================================================

static void test_aero_capacity_exactly_at_cap_boundary() {
    TEST("aero capacity: zone_tiles that produce exactly 2500");
    PortZoneComponent zone;
    zone.port_type = PortType::Aero;
    zone.has_runway = true;

    // cap = zone_tiles * 10 * 1.5 * 1.0 = zone_tiles * 15
    // 2500 / 15 = 166.67, so 167 tiles -> 167*15=2505 > 2500 -> capped
    // 166 tiles -> 166*15=2490 < 2500 -> not capped
    zone.zone_tiles = 166;
    uint16_t cap_below = calculate_aero_capacity(zone, true);
    assert(cap_below == 2490);

    zone.zone_tiles = 167;
    uint16_t cap_at = calculate_aero_capacity(zone, true);
    assert(cap_at == 2500);  // capped

    zone.zone_tiles = 200;
    uint16_t cap_above = calculate_aero_capacity(zone, true);
    assert(cap_above == 2500);  // still capped

    PASS();
}

static void test_aqua_capacity_exactly_at_cap_boundary() {
    TEST("aqua capacity: zone configuration that hits exactly 5000");
    PortZoneComponent zone;
    zone.port_type = PortType::Aqua;
    zone.dock_count = 0;  // dock_bonus = 1.0

    // cap = zone_tiles * 15 * 1.0 * 1.0 * 1.0 = zone_tiles * 15
    // 5000 / 15 = 333.33
    zone.zone_tiles = 333;
    uint16_t cap_below = calculate_aqua_capacity(zone, 4, false);
    assert(cap_below == 4995);

    zone.zone_tiles = 334;
    uint16_t cap_at = calculate_aqua_capacity(zone, 4, false);
    assert(cap_at == 5000);  // capped at 5000 (raw = 5010)

    PASS();
}

static void test_aero_capacity_1_tile() {
    TEST("aero capacity: 1 tile with runway");
    PortZoneComponent zone;
    zone.port_type = PortType::Aero;
    zone.zone_tiles = 1;
    zone.has_runway = true;

    // 1 * 10 * 1.5 * 1.0 = 15
    uint16_t cap = calculate_aero_capacity(zone, true);
    assert(cap == 15);
    PASS();
}

static void test_aqua_capacity_1_tile() {
    TEST("aqua capacity: 1 tile, 0 docks, full water, no rail");
    PortZoneComponent zone;
    zone.port_type = PortType::Aqua;
    zone.zone_tiles = 1;
    zone.dock_count = 0;

    // 1 * 15 * 1.0 * 1.0 * 1.0 = 15
    uint16_t cap = calculate_aqua_capacity(zone, 4, false);
    assert(cap == 15);
    PASS();
}

static void test_aqua_capacity_max_dock_count() {
    TEST("aqua capacity: maximum dock_count (255)");
    PortZoneComponent zone;
    zone.port_type = PortType::Aqua;
    zone.zone_tiles = 32;
    zone.dock_count = 255;

    // dock_bonus = 1.0 + (255 * 0.2) = 52.0
    // base = 32 * 15 = 480
    // raw = 480 * 52.0 * 1.0 * 1.0 = 24960 -> capped to 5000
    uint16_t cap = calculate_aqua_capacity(zone, 4, false);
    assert(cap == AQUA_PORT_MAX_CAPACITY);
    assert(cap == 5000);
    PASS();
}

static void test_aero_capacity_no_runway_no_access() {
    TEST("aero capacity: no runway AND no access = 0");
    PortZoneComponent zone;
    zone.port_type = PortType::Aero;
    zone.zone_tiles = 100;
    zone.has_runway = false;

    // 100 * 10 * 0.5 * 0.0 = 0
    uint16_t cap = calculate_aero_capacity(zone, false);
    assert(cap == 0);
    PASS();
}

static void test_aqua_capacity_low_water_access_with_rail() {
    TEST("aqua capacity: partial water access with rail");
    PortZoneComponent zone;
    zone.port_type = PortType::Aqua;
    zone.zone_tiles = 64;
    zone.dock_count = 2;

    // base = 64 * 15 = 960
    // dock_bonus = 1.0 + (2 * 0.2) = 1.4
    // water_access = 0.5 (adjacent_water=3 < 4)
    // rail_bonus = 1.5
    // raw = 960 * 1.4 * 0.5 * 1.5 = 1008
    uint16_t cap = calculate_aqua_capacity(zone, 3, true);
    assert(cap == 1008);
    PASS();
}

static void test_aqua_capacity_water_access_boundary_3_vs_4() {
    TEST("aqua capacity: water access boundary (3 vs 4 adjacent water)");
    PortZoneComponent zone;
    zone.port_type = PortType::Aqua;
    zone.zone_tiles = 32;
    zone.dock_count = 0;

    // 3 water tiles -> water_access = 0.5
    // 32 * 15 * 1.0 * 0.5 * 1.0 = 240
    uint16_t cap3 = calculate_aqua_capacity(zone, 3, false);
    assert(cap3 == 240);

    // 4 water tiles -> water_access = 1.0
    // 32 * 15 * 1.0 * 1.0 * 1.0 = 480
    uint16_t cap4 = calculate_aqua_capacity(zone, 4, false);
    assert(cap4 == 480);

    assert(cap4 == cap3 * 2);  // Full access is exactly double partial
    PASS();
}

static void test_aero_capacity_max_tiles_uint16() {
    TEST("aero capacity: max zone_tiles (65535) always capped");
    PortZoneComponent zone;
    zone.port_type = PortType::Aero;
    zone.zone_tiles = 65535;
    zone.has_runway = true;

    // 65535 * 10 * 1.5 = 983025 -> capped to 2500
    uint16_t cap = calculate_aero_capacity(zone, true);
    assert(cap == AERO_PORT_MAX_CAPACITY);
    PASS();
}

static void test_aqua_capacity_max_tiles_uint16() {
    TEST("aqua capacity: max zone_tiles (65535) always capped");
    PortZoneComponent zone;
    zone.port_type = PortType::Aqua;
    zone.zone_tiles = 65535;
    zone.dock_count = 10;

    // huge number -> capped to 5000
    uint16_t cap = calculate_aqua_capacity(zone, 100, true);
    assert(cap == AQUA_PORT_MAX_CAPACITY);
    PASS();
}

static void test_port_capacity_dispatch_unknown_type() {
    TEST("capacity dispatch: unknown port_type returns 0");
    PortZoneComponent zone;
    zone.port_type = static_cast<PortType>(99);
    zone.zone_tiles = 100;

    uint16_t cap = calculate_port_capacity(zone, true, 10, true);
    assert(cap == 0);
    PASS();
}

static void test_get_max_capacity_values() {
    TEST("get_max_capacity returns correct values for both types");
    assert(get_max_capacity(PortType::Aero) == 2500);
    assert(get_max_capacity(PortType::Aqua) == 5000);
    PASS();
}

static void test_aero_capacity_without_runway_scaling() {
    TEST("aero capacity: without runway, scales at 0.5x");
    PortZoneComponent zone;
    zone.port_type = PortType::Aero;
    zone.has_runway = false;

    zone.zone_tiles = 50;
    uint16_t cap_no_runway = calculate_aero_capacity(zone, true);
    // 50 * 10 * 0.5 * 1.0 = 250
    assert(cap_no_runway == 250);

    zone.has_runway = true;
    uint16_t cap_with_runway = calculate_aero_capacity(zone, true);
    // 50 * 10 * 1.5 * 1.0 = 750
    assert(cap_with_runway == 750);

    // Runway triples the effective capacity
    assert(cap_with_runway == cap_no_runway * 3);
    PASS();
}

// =============================================================================
// REGRESSION TESTS
// =============================================================================

static void test_aero_regression_zone_at_map_edge() {
    TEST("regression: aero zone near map edge validates correctly");
    MockTerrain terrain;
    terrain.set_default_elevation(10);
    MockTransport transport;

    // Zone near the edge of 128x128 map
    GridRect zone;
    zone.x = 120; zone.y = 120; zone.width = 8; zone.height = 8;
    assert(validate_aero_port_zone(zone, terrain, transport));
    PASS();
}

static void test_aqua_regression_zone_at_map_edge() {
    TEST("regression: aqua zone near map edge validates correctly");
    MockTerrain terrain;
    MockTransport transport;

    GridRect zone;
    zone.x = 120; zone.y = 120; zone.width = 8; zone.height = 4;
    // Water outside the map boundary (row 124)
    place_water_south(terrain, zone, 4);
    assert(validate_aqua_port_zone(zone, terrain, transport));
    PASS();
}

static void test_aero_regression_large_zone_with_isolated_flat_patch() {
    TEST("regression: large zone with small isolated flat patch");
    MockTerrain terrain;
    // Fill large zone with varied terrain
    for (int16_t y = 0; y < 20; ++y) {
        for (int16_t x = 0; x < 20; ++x) {
            terrain.set_elevation(x, y, static_cast<uint8_t>((x * 11 + y * 17) % 50));
        }
    }
    // Insert a 6x2 flat island at (7,9)
    for (int16_t y = 9; y <= 10; ++y) {
        for (int16_t x = 7; x <= 12; ++x) {
            terrain.set_elevation(x, y, 42);
        }
    }
    MockTransport transport;

    GridRect zone;
    zone.x = 0; zone.y = 0; zone.width = 20; zone.height = 20;
    assert(validate_aero_port_zone(zone, terrain, transport));
    PASS();
}

static void test_aero_regression_overlapping_flat_patches() {
    TEST("regression: overlapping flat patches at different elevations");
    MockTerrain terrain;
    for (int16_t y = 0; y < 10; ++y) {
        for (int16_t x = 0; x < 10; ++x) {
            terrain.set_elevation(x, y, static_cast<uint8_t>((x + y) * 3));
        }
    }
    // Patch 1: 4x2 at elevation 10 (too short for runway)
    for (int16_t y = 2; y <= 3; ++y) {
        for (int16_t x = 0; x <= 3; ++x) {
            terrain.set_elevation(x, y, 10);
        }
    }
    // Patch 2: 6x2 at elevation 20 (valid runway)
    for (int16_t y = 5; y <= 6; ++y) {
        for (int16_t x = 2; x <= 7; ++x) {
            terrain.set_elevation(x, y, 20);
        }
    }
    MockTransport transport;

    GridRect zone;
    zone.x = 0; zone.y = 0; zone.width = 10; zone.height = 10;
    assert(validate_aero_port_zone(zone, terrain, transport));
    PASS();
}

// =============================================================================
// Main
// =============================================================================

int main() {
    printf("=== Comprehensive Port Validation Tests (E8-036) ===\n\n");

    printf("--- Aero Port Size Validation Edge Cases ---\n");
    test_aero_exactly_35_tiles_rejected();
    test_aero_36_tiles_non_square_9x4();
    test_aero_36_tiles_non_square_4x9();
    test_aero_36_tiles_12x3();
    test_aero_very_wide_2x18();
    test_aero_18x2();
    test_aero_1x36_rejected_too_narrow();
    test_aero_36x1_rejected_too_short();

    printf("\n--- Aero Port Runway Detection Edge Cases ---\n");
    test_aero_runway_barely_fits_exact_6x2_at_corner();
    test_aero_runway_barely_fits_at_bottom_right();
    test_aero_runway_5x2_does_not_fit();
    test_aero_vertical_runway_only();
    test_aero_all_elevations_different();
    test_aero_single_flat_row_too_narrow();
    test_aero_two_adjacent_flat_rows_form_runway();
    test_aero_flat_strip_at_elevation_zero();
    test_aero_flat_strip_at_elevation_255();
    test_aero_multiple_flat_strips_only_one_valid();

    printf("\n--- Aqua Port Water Adjacency Edge Cases ---\n");
    test_aqua_zone_at_origin_water_south_only();
    test_aqua_water_on_all_four_edges();
    test_aqua_mixed_water_types_across_edges();
    test_aqua_water_on_north_edge_only();
    test_aqua_zone_at_origin_no_north_west_water();
    test_aqua_zone_at_origin_no_water_anywhere();
    test_aqua_single_column_zone();
    test_aqua_single_row_zone();

    printf("\n--- Aqua Port Dock Requirement Edge Cases ---\n");
    test_aqua_exactly_3_dock_tiles_rejected();
    test_aqua_dock_tiles_from_multiple_edges_sum_to_4();
    test_aqua_large_zone_minimal_water();
    test_aqua_water_not_immediately_adjacent_rejected();
    test_aqua_all_perimeter_water();
    test_aqua_water_inside_zone_doesnt_count();

    printf("\n--- Pathway Accessibility Edge Cases ---\n");
    test_aero_pathway_on_single_perimeter_tile_only();
    test_aqua_pathway_on_single_perimeter_tile_only();

    printf("\n--- Capacity Calculation Boundary Values ---\n");
    test_aero_capacity_exactly_at_cap_boundary();
    test_aqua_capacity_exactly_at_cap_boundary();
    test_aero_capacity_1_tile();
    test_aqua_capacity_1_tile();
    test_aqua_capacity_max_dock_count();
    test_aero_capacity_no_runway_no_access();
    test_aqua_capacity_low_water_access_with_rail();
    test_aqua_capacity_water_access_boundary_3_vs_4();
    test_aero_capacity_max_tiles_uint16();
    test_aqua_capacity_max_tiles_uint16();
    test_port_capacity_dispatch_unknown_type();
    test_get_max_capacity_values();
    test_aero_capacity_without_runway_scaling();

    printf("\n--- Regression Tests ---\n");
    test_aero_regression_zone_at_map_edge();
    test_aqua_regression_zone_at_map_edge();
    test_aero_regression_large_zone_with_isolated_flat_patch();
    test_aero_regression_overlapping_flat_patches();

    printf("\n=== Results: %d/%d passed ===\n", tests_passed, tests_total);
    return (tests_passed == tests_total) ? 0 : 1;
}
