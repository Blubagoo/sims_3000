/**
 * @file test_aero_port_validation.cpp
 * @brief Unit tests for aero port zone validation (Epic 8, Ticket E8-008)
 *
 * Tests cover:
 * - Minimum zone size validation (36 tiles)
 * - Runway detection (6 tiles long, 2 tiles wide minimum)
 * - Terrain flatness for runway area
 * - Pathway accessibility (3-tile rule)
 * - Edge cases (exact minimum, non-flat terrain, no road access)
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

class MockTerrainForAero : public ITerrainQueryable {
public:
    MockTerrainForAero()
        : m_default_elevation(10)
        , m_default_type(TerrainType::Substrate)
        , m_map_width(128)
        , m_map_height(128)
    {}

    /// Set elevation for a specific tile
    void set_elevation(int32_t x, int32_t y, uint8_t elev) {
        m_elevations[key(x, y)] = elev;
    }

    /// Set default elevation for all tiles
    void set_default_elevation(uint8_t elev) { m_default_elevation = elev; }

    /// Set terrain type for a specific tile
    void set_terrain_type(int32_t x, int32_t y, TerrainType type) {
        m_types[key(x, y)] = type;
    }

    /// Set default terrain type for all tiles
    void set_default_terrain_type(TerrainType type) { m_default_type = type; }

    // ITerrainQueryable interface implementation
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

class MockTransportForAero : public ITransportProvider {
public:
    MockTransportForAero() : m_accessible(true) {}

    /// Set whether all tiles have road access
    void set_accessible(bool accessible) { m_accessible = accessible; }

    /// Set road accessibility for a specific tile
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
// Tests
// =============================================================================

void test_aero_rejects_zone_too_small() {
    printf("Testing aero port rejects zone smaller than 36 tiles...\n");

    MockTerrainForAero terrain;
    MockTransportForAero transport;

    // 5x5 = 25 tiles (too small)
    GridRect zone;
    zone.x = 0; zone.y = 0; zone.width = 5; zone.height = 5;
    assert(!validate_aero_port_zone(zone, terrain, transport));

    // 5x7 = 35 tiles (still too small)
    zone.width = 5; zone.height = 7;
    assert(!validate_aero_port_zone(zone, terrain, transport));

    // 4x8 = 32 tiles (too small)
    zone.width = 4; zone.height = 8;
    assert(!validate_aero_port_zone(zone, terrain, transport));

    printf("  PASS: Zones smaller than 36 tiles rejected\n");
}

void test_aero_accepts_minimum_zone_size() {
    printf("Testing aero port accepts minimum zone size (6x6 = 36)...\n");

    MockTerrainForAero terrain;
    terrain.set_default_elevation(10);  // Flat terrain
    MockTransportForAero transport;
    transport.set_accessible(true);

    // 6x6 = 36 tiles (exact minimum)
    GridRect zone;
    zone.x = 0; zone.y = 0; zone.width = 6; zone.height = 6;
    assert(validate_aero_port_zone(zone, terrain, transport));

    printf("  PASS: Minimum zone size (6x6) accepted\n");
}

void test_aero_accepts_larger_zone() {
    printf("Testing aero port accepts larger zone...\n");

    MockTerrainForAero terrain;
    terrain.set_default_elevation(10);
    MockTransportForAero transport;
    transport.set_accessible(true);

    // 10x10 = 100 tiles
    GridRect zone;
    zone.x = 5; zone.y = 5; zone.width = 10; zone.height = 10;
    assert(validate_aero_port_zone(zone, terrain, transport));

    printf("  PASS: Larger zone accepted\n");
}

void test_aero_detects_horizontal_runway() {
    printf("Testing aero port detects valid horizontal runway...\n");

    MockTerrainForAero terrain;
    terrain.set_default_elevation(10);
    MockTransportForAero transport;
    transport.set_accessible(true);

    // 8x4 zone - wide enough for horizontal runway (6x2)
    GridRect zone;
    zone.x = 0; zone.y = 0; zone.width = 8; zone.height = 5;
    assert(validate_aero_port_zone(zone, terrain, transport));

    printf("  PASS: Horizontal runway detected\n");
}

void test_aero_detects_vertical_runway() {
    printf("Testing aero port detects valid vertical runway...\n");

    MockTerrainForAero terrain;
    terrain.set_default_elevation(10);
    MockTransportForAero transport;
    transport.set_accessible(true);

    // 3x12 zone - tall enough for vertical runway (2x6) but total area = 36
    GridRect zone;
    zone.x = 0; zone.y = 0; zone.width = 3; zone.height = 12;
    assert(validate_aero_port_zone(zone, terrain, transport));

    printf("  PASS: Vertical runway detected\n");
}

void test_aero_rejects_non_flat_runway() {
    printf("Testing aero port rejects non-flat runway area...\n");

    MockTerrainForAero terrain;
    // Set varying elevations so no 6x2 or 2x6 area is flat
    for (int16_t y = 0; y < 6; ++y) {
        for (int16_t x = 0; x < 6; ++x) {
            // Checkerboard elevations: alternating 10 and 15
            terrain.set_elevation(x, y, static_cast<uint8_t>((x + y) % 2 == 0 ? 10 : 15));
        }
    }
    MockTransportForAero transport;
    transport.set_accessible(true);

    // 6x6 zone
    GridRect zone;
    zone.x = 0; zone.y = 0; zone.width = 6; zone.height = 6;
    assert(!validate_aero_port_zone(zone, terrain, transport));

    printf("  PASS: Non-flat runway rejected\n");
}

void test_aero_accepts_partially_flat_zone() {
    printf("Testing aero port accepts zone with partial flatness...\n");

    MockTerrainForAero terrain;
    // Make most terrain varied but have one flat 6x2 strip
    for (int16_t y = 0; y < 8; ++y) {
        for (int16_t x = 0; x < 8; ++x) {
            terrain.set_elevation(x, y, static_cast<uint8_t>(x + y));
        }
    }
    // Create a flat 6x2 strip at y=3,4 x=1..6
    for (int16_t y = 3; y <= 4; ++y) {
        for (int16_t x = 1; x <= 6; ++x) {
            terrain.set_elevation(x, y, 10);
        }
    }

    MockTransportForAero transport;
    transport.set_accessible(true);

    GridRect zone;
    zone.x = 0; zone.y = 0; zone.width = 8; zone.height = 8;
    assert(validate_aero_port_zone(zone, terrain, transport));

    printf("  PASS: Zone with partial flat area for runway accepted\n");
}

void test_aero_rejects_no_pathway_access() {
    printf("Testing aero port rejects zone without pathway access...\n");

    MockTerrainForAero terrain;
    terrain.set_default_elevation(10);
    MockTransportForAero transport;
    transport.set_accessible(false);  // No roads anywhere

    GridRect zone;
    zone.x = 10; zone.y = 10; zone.width = 6; zone.height = 6;
    assert(!validate_aero_port_zone(zone, terrain, transport));

    printf("  PASS: Zone without pathway access rejected\n");
}

void test_aero_accepts_with_pathway_access() {
    printf("Testing aero port accepts zone with pathway access...\n");

    MockTerrainForAero terrain;
    terrain.set_default_elevation(10);
    MockTransportForAero transport;
    transport.set_accessible(false);
    // Place road near one perimeter tile
    transport.set_accessible_at(10, 10, true);

    GridRect zone;
    zone.x = 10; zone.y = 10; zone.width = 6; zone.height = 6;
    assert(validate_aero_port_zone(zone, terrain, transport));

    printf("  PASS: Zone with pathway access accepted\n");
}

void test_aero_rejects_empty_zone() {
    printf("Testing aero port rejects empty zone...\n");

    MockTerrainForAero terrain;
    MockTransportForAero transport;

    GridRect zone;
    zone.x = 0; zone.y = 0; zone.width = 0; zone.height = 0;
    assert(!validate_aero_port_zone(zone, terrain, transport));

    printf("  PASS: Empty zone rejected\n");
}

void test_aero_rejects_narrow_zone_no_runway() {
    printf("Testing aero port rejects narrow zone that cannot fit runway...\n");

    MockTerrainForAero terrain;
    terrain.set_default_elevation(10);
    MockTransportForAero transport;
    transport.set_accessible(true);

    // 1x36 zone - enough tiles but only 1 wide (cannot fit 2-wide runway)
    GridRect zone;
    zone.x = 0; zone.y = 0; zone.width = 1; zone.height = 36;
    assert(!validate_aero_port_zone(zone, terrain, transport));

    printf("  PASS: Narrow zone without runway space rejected\n");
}

void test_aero_runway_at_different_elevation() {
    printf("Testing aero port runway at non-zero elevation...\n");

    MockTerrainForAero terrain;
    terrain.set_default_elevation(20);  // Higher elevation, but still flat
    MockTransportForAero transport;
    transport.set_accessible(true);

    GridRect zone;
    zone.x = 0; zone.y = 0; zone.width = 6; zone.height = 6;
    assert(validate_aero_port_zone(zone, terrain, transport));

    printf("  PASS: Runway at different elevation accepted (as long as flat)\n");
}

void test_aero_zone_with_offset_position() {
    printf("Testing aero port zone at offset position...\n");

    MockTerrainForAero terrain;
    terrain.set_default_elevation(10);
    MockTransportForAero transport;
    transport.set_accessible(true);

    // Zone at offset position (50, 50)
    GridRect zone;
    zone.x = 50; zone.y = 50; zone.width = 8; zone.height = 8;
    assert(validate_aero_port_zone(zone, terrain, transport));

    printf("  PASS: Zone at offset position validated correctly\n");
}

// =============================================================================
// Main
// =============================================================================

int main() {
    printf("=== Aero Port Zone Validation Tests (Epic 8, Ticket E8-008) ===\n\n");

    test_aero_rejects_zone_too_small();
    test_aero_accepts_minimum_zone_size();
    test_aero_accepts_larger_zone();
    test_aero_detects_horizontal_runway();
    test_aero_detects_vertical_runway();
    test_aero_rejects_non_flat_runway();
    test_aero_accepts_partially_flat_zone();
    test_aero_rejects_no_pathway_access();
    test_aero_accepts_with_pathway_access();
    test_aero_rejects_empty_zone();
    test_aero_rejects_narrow_zone_no_runway();
    test_aero_runway_at_different_elevation();
    test_aero_zone_with_offset_position();

    printf("\n=== All Aero Port Validation Tests Passed ===\n");
    return 0;
}
