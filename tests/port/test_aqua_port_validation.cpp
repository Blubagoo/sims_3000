/**
 * @file test_aqua_port_validation.cpp
 * @brief Unit tests for aqua port zone validation (Epic 8, Ticket E8-009)
 *
 * Tests cover:
 * - Minimum zone size validation (32 tiles)
 * - Water adjacency checking (zone perimeter borders water tiles)
 * - Minimum dock tile count (4 water-adjacent perimeter tiles)
 * - Pathway accessibility
 * - Edge cases (no water, insufficient dock tiles, various water types)
 */

#include <sims3000/port/PortZoneValidation.h>
#include <sims3000/terrain/ITerrainQueryable.h>
#include <sims3000/building/ForwardDependencyInterfaces.h>
#include <sims3000/terrain/TerrainTypes.h>
#include <sims3000/terrain/TerrainEvents.h>
#include <cassert>
#include <cstdio>
#include <cstdint>
#include <vector>
#include <unordered_map>

using namespace sims3000::port;
using namespace sims3000::terrain;
using namespace sims3000::building;

// =============================================================================
// Mock ITerrainQueryable
// =============================================================================

class MockTerrainForAqua : public ITerrainQueryable {
public:
    MockTerrainForAqua()
        : m_default_elevation(10)
        , m_default_type(TerrainType::Substrate)
        , m_map_width(128)
        , m_map_height(128)
    {}

    /// Set terrain type for a specific tile
    void set_terrain_type(int32_t x, int32_t y, TerrainType type) {
        m_types[key(x, y)] = type;
    }

    /// Set default terrain type for all tiles
    void set_default_terrain_type(TerrainType type) { m_default_type = type; }

    /// Set elevation for a specific tile
    void set_elevation(int32_t x, int32_t y, uint8_t elev) {
        m_elevations[key(x, y)] = elev;
    }

    /// Set default elevation
    void set_default_elevation(uint8_t elev) { m_default_elevation = elev; }

    // ITerrainQueryable interface
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

class MockTransportForAqua : public ITransportProvider {
public:
    MockTransportForAqua() : m_accessible(true) {}

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
// Helper: place water tiles adjacent to zone
// =============================================================================

/// Place water tiles along the bottom edge of the zone (outside the zone)
static void place_water_south(MockTerrainForAqua& terrain,
                               const GridRect& zone,
                               int16_t count,
                               TerrainType water_type = TerrainType::StillBasin) {
    int16_t y = zone.bottom();  // Row just below zone
    for (int16_t i = 0; i < count && i < static_cast<int16_t>(zone.width); ++i) {
        terrain.set_terrain_type(zone.x + i, y, water_type);
    }
}

/// Place water tiles along the right edge of the zone (outside the zone)
static void place_water_east(MockTerrainForAqua& terrain,
                              const GridRect& zone,
                              int16_t count,
                              TerrainType water_type = TerrainType::StillBasin) {
    int16_t x = zone.right();  // Column just right of zone
    for (int16_t i = 0; i < count && i < static_cast<int16_t>(zone.height); ++i) {
        terrain.set_terrain_type(x, zone.y + i, water_type);
    }
}

// =============================================================================
// Tests
// =============================================================================

void test_aqua_rejects_zone_too_small() {
    printf("Testing aqua port rejects zone smaller than 32 tiles...\n");

    MockTerrainForAqua terrain;
    MockTransportForAqua transport;

    // 3x10 = 30 tiles (too small)
    GridRect zone;
    zone.x = 0; zone.y = 0; zone.width = 3; zone.height = 10;
    assert(!validate_aqua_port_zone(zone, terrain, transport));

    // 4x7 = 28 tiles (too small)
    zone.width = 4; zone.height = 7;
    assert(!validate_aqua_port_zone(zone, terrain, transport));

    // 5x6 = 30 tiles (too small)
    zone.width = 5; zone.height = 6;
    assert(!validate_aqua_port_zone(zone, terrain, transport));

    printf("  PASS: Zones smaller than 32 tiles rejected\n");
}

void test_aqua_accepts_minimum_zone_size() {
    printf("Testing aqua port accepts minimum zone size (4x8 = 32)...\n");

    MockTerrainForAqua terrain;
    MockTransportForAqua transport;
    transport.set_accessible(true);

    // 4x8 = 32 tiles (exact minimum)
    GridRect zone;
    zone.x = 0; zone.y = 0; zone.width = 4; zone.height = 8;
    // Need water adjacency - place 4 water tiles on south edge
    place_water_south(terrain, zone, 4);
    assert(validate_aqua_port_zone(zone, terrain, transport));

    printf("  PASS: Minimum zone size (4x8) accepted\n");
}

void test_aqua_accepts_larger_zone() {
    printf("Testing aqua port accepts larger zone...\n");

    MockTerrainForAqua terrain;
    MockTransportForAqua transport;
    transport.set_accessible(true);

    // 8x8 = 64 tiles
    GridRect zone;
    zone.x = 5; zone.y = 5; zone.width = 8; zone.height = 8;
    // Place water on south edge
    place_water_south(terrain, zone, 8);
    assert(validate_aqua_port_zone(zone, terrain, transport));

    printf("  PASS: Larger zone accepted\n");
}

void test_aqua_rejects_no_water_adjacency() {
    printf("Testing aqua port rejects zone with no water adjacency...\n");

    MockTerrainForAqua terrain;
    // All tiles are Substrate (no water anywhere)
    MockTransportForAqua transport;
    transport.set_accessible(true);

    GridRect zone;
    zone.x = 10; zone.y = 10; zone.width = 8; zone.height = 4;
    assert(!validate_aqua_port_zone(zone, terrain, transport));

    printf("  PASS: Zone with no water adjacency rejected\n");
}

void test_aqua_rejects_insufficient_dock_tiles() {
    printf("Testing aqua port rejects zone with fewer than 4 water-adjacent tiles...\n");

    MockTerrainForAqua terrain;
    MockTransportForAqua transport;
    transport.set_accessible(true);

    GridRect zone;
    zone.x = 0; zone.y = 0; zone.width = 8; zone.height = 4;
    // Only 3 water tiles adjacent (need 4)
    place_water_south(terrain, zone, 3);
    assert(!validate_aqua_port_zone(zone, terrain, transport));

    printf("  PASS: Zone with insufficient dock tiles (3 < 4) rejected\n");
}

void test_aqua_accepts_exactly_4_dock_tiles() {
    printf("Testing aqua port accepts zone with exactly 4 water-adjacent tiles...\n");

    MockTerrainForAqua terrain;
    MockTransportForAqua transport;
    transport.set_accessible(true);

    GridRect zone;
    zone.x = 0; zone.y = 0; zone.width = 8; zone.height = 4;
    // Exactly 4 water tiles on south edge
    place_water_south(terrain, zone, 4);
    assert(validate_aqua_port_zone(zone, terrain, transport));

    printf("  PASS: Zone with exactly 4 dock tiles accepted\n");
}

void test_aqua_detects_deep_void_water() {
    printf("Testing aqua port detects DeepVoid as water...\n");

    MockTerrainForAqua terrain;
    MockTransportForAqua transport;
    transport.set_accessible(true);

    GridRect zone;
    zone.x = 0; zone.y = 0; zone.width = 8; zone.height = 4;
    place_water_south(terrain, zone, 4, TerrainType::DeepVoid);
    assert(validate_aqua_port_zone(zone, terrain, transport));

    printf("  PASS: DeepVoid recognized as water\n");
}

void test_aqua_detects_flow_channel_water() {
    printf("Testing aqua port detects FlowChannel as water...\n");

    MockTerrainForAqua terrain;
    MockTransportForAqua transport;
    transport.set_accessible(true);

    GridRect zone;
    zone.x = 0; zone.y = 0; zone.width = 8; zone.height = 4;
    place_water_south(terrain, zone, 4, TerrainType::FlowChannel);
    assert(validate_aqua_port_zone(zone, terrain, transport));

    printf("  PASS: FlowChannel recognized as water\n");
}

void test_aqua_detects_still_basin_water() {
    printf("Testing aqua port detects StillBasin as water...\n");

    MockTerrainForAqua terrain;
    MockTransportForAqua transport;
    transport.set_accessible(true);

    GridRect zone;
    zone.x = 0; zone.y = 0; zone.width = 8; zone.height = 4;
    place_water_south(terrain, zone, 4, TerrainType::StillBasin);
    assert(validate_aqua_port_zone(zone, terrain, transport));

    printf("  PASS: StillBasin recognized as water\n");
}

void test_aqua_rejects_non_water_terrain_types() {
    printf("Testing aqua port rejects non-water terrain types...\n");

    MockTerrainForAqua terrain;
    MockTransportForAqua transport;
    transport.set_accessible(true);

    GridRect zone;
    zone.x = 0; zone.y = 0; zone.width = 8; zone.height = 4;

    // Place non-water terrain adjacent (BiolumeGrove)
    int16_t y = zone.bottom();
    for (int16_t i = 0; i < 4; ++i) {
        terrain.set_terrain_type(i, y, TerrainType::BiolumeGrove);
    }
    assert(!validate_aqua_port_zone(zone, terrain, transport));

    printf("  PASS: Non-water terrain types not counted as dock tiles\n");
}

void test_aqua_water_on_east_edge() {
    printf("Testing aqua port with water on east edge...\n");

    MockTerrainForAqua terrain;
    MockTransportForAqua transport;
    transport.set_accessible(true);

    GridRect zone;
    zone.x = 0; zone.y = 0; zone.width = 4; zone.height = 8;
    // Place water on east side (right edge)
    place_water_east(terrain, zone, 4);
    assert(validate_aqua_port_zone(zone, terrain, transport));

    printf("  PASS: Water on east edge detected\n");
}

void test_aqua_water_on_multiple_edges() {
    printf("Testing aqua port with water on multiple edges...\n");

    MockTerrainForAqua terrain;
    MockTransportForAqua transport;
    transport.set_accessible(true);

    GridRect zone;
    zone.x = 5; zone.y = 5; zone.width = 8; zone.height = 4;
    // Place 2 water tiles on south + 2 on east = 4 total
    place_water_south(terrain, zone, 2);
    place_water_east(terrain, zone, 2);
    assert(validate_aqua_port_zone(zone, terrain, transport));

    printf("  PASS: Water on multiple edges combined for dock count\n");
}

void test_aqua_rejects_no_pathway_access() {
    printf("Testing aqua port rejects zone without pathway access...\n");

    MockTerrainForAqua terrain;
    MockTransportForAqua transport;
    transport.set_accessible(false);  // No roads

    GridRect zone;
    zone.x = 10; zone.y = 10; zone.width = 8; zone.height = 4;
    place_water_south(terrain, zone, 8);
    assert(!validate_aqua_port_zone(zone, terrain, transport));

    printf("  PASS: Zone without pathway access rejected\n");
}

void test_aqua_accepts_with_pathway_access() {
    printf("Testing aqua port accepts zone with pathway access...\n");

    MockTerrainForAqua terrain;
    MockTransportForAqua transport;
    transport.set_accessible(false);
    // Place road near one perimeter tile
    transport.set_accessible_at(10, 10, true);

    GridRect zone;
    zone.x = 10; zone.y = 10; zone.width = 8; zone.height = 4;
    place_water_south(terrain, zone, 4);
    assert(validate_aqua_port_zone(zone, terrain, transport));

    printf("  PASS: Zone with pathway access accepted\n");
}

void test_aqua_rejects_empty_zone() {
    printf("Testing aqua port rejects empty zone...\n");

    MockTerrainForAqua terrain;
    MockTransportForAqua transport;

    GridRect zone;
    zone.x = 0; zone.y = 0; zone.width = 0; zone.height = 0;
    assert(!validate_aqua_port_zone(zone, terrain, transport));

    printf("  PASS: Empty zone rejected\n");
}

void test_aqua_zone_at_offset() {
    printf("Testing aqua port zone at offset position...\n");

    MockTerrainForAqua terrain;
    MockTransportForAqua transport;
    transport.set_accessible(true);

    GridRect zone;
    zone.x = 50; zone.y = 50; zone.width = 8; zone.height = 8;
    place_water_south(terrain, zone, 6);
    assert(validate_aqua_port_zone(zone, terrain, transport));

    printf("  PASS: Zone at offset position validated correctly\n");
}

// =============================================================================
// Main
// =============================================================================

int main() {
    printf("=== Aqua Port Zone Validation Tests (Epic 8, Ticket E8-009) ===\n\n");

    test_aqua_rejects_zone_too_small();
    test_aqua_accepts_minimum_zone_size();
    test_aqua_accepts_larger_zone();
    test_aqua_rejects_no_water_adjacency();
    test_aqua_rejects_insufficient_dock_tiles();
    test_aqua_accepts_exactly_4_dock_tiles();
    test_aqua_detects_deep_void_water();
    test_aqua_detects_flow_channel_water();
    test_aqua_detects_still_basin_water();
    test_aqua_rejects_non_water_terrain_types();
    test_aqua_water_on_east_edge();
    test_aqua_water_on_multiple_edges();
    test_aqua_rejects_no_pathway_access();
    test_aqua_accepts_with_pathway_access();
    test_aqua_rejects_empty_zone();
    test_aqua_zone_at_offset();

    printf("\n=== All Aqua Port Validation Tests Passed ===\n");
    return 0;
}
