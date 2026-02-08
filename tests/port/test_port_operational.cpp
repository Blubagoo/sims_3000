/**
 * @file test_port_operational.cpp
 * @brief Unit tests for port operational status check (Epic 8, Ticket E8-011)
 *
 * Tests cover:
 * - Operational when all conditions met
 * - Non-operational when zone validation fails
 * - Non-operational when infrastructure missing
 * - Non-operational when pathway disconnected
 * - Non-operational when capacity is zero
 * - PortOperationalEvent emitted on state change
 * - No event when status unchanged
 */

#include <sims3000/port/PortOperational.h>
#include <sims3000/port/PortComponent.h>
#include <sims3000/port/PortZoneComponent.h>
#include <sims3000/port/PortZoneValidation.h>
#include <sims3000/port/PortEvents.h>
#include <sims3000/terrain/ITerrainQueryable.h>
#include <sims3000/terrain/TerrainTypes.h>
#include <sims3000/terrain/TerrainEvents.h>
#include <sims3000/building/ForwardDependencyInterfaces.h>
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

class MockTerrainForOps : public ITerrainQueryable {
public:
    MockTerrainForOps()
        : m_default_elevation(10)
        , m_default_type(TerrainType::Substrate)
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
    uint32_t get_map_width() const override { return 128; }
    uint32_t get_map_height() const override { return 128; }
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
    std::unordered_map<uint64_t, uint8_t> m_elevations;
    std::unordered_map<uint64_t, TerrainType> m_types;
};

// =============================================================================
// Mock ITransportProvider
// =============================================================================

class MockTransportForOps : public ITransportProvider {
public:
    MockTransportForOps() : m_accessible(true) {}

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
// Helper: create a valid aero port setup
// =============================================================================

static void setup_valid_aero(PortComponent& port, PortZoneComponent& zone) {
    port.port_type = PortType::Aero;
    port.capacity = 540;  // non-zero
    port.is_operational = false;

    zone.port_type = PortType::Aero;
    zone.zone_tiles = 36;
    zone.has_runway = true;
    zone.runway_length = 6;
    // Set runway_area to represent a 6x6 zone at (0,0)
    zone.runway_area.x = 0;
    zone.runway_area.y = 0;
    zone.runway_area.width = 6;
    zone.runway_area.height = 6;
}

// =============================================================================
// Helper: create a valid aqua port setup
// =============================================================================

static void setup_valid_aqua(PortComponent& port, PortZoneComponent& zone,
                              MockTerrainForOps& terrain) {
    port.port_type = PortType::Aqua;
    port.capacity = 864;  // non-zero
    port.is_operational = false;

    zone.port_type = PortType::Aqua;
    zone.zone_tiles = 32;
    zone.has_dock = true;
    zone.dock_count = 4;
    // Set runway_area to represent an 8x4 zone at (0,0)
    zone.runway_area.x = 0;
    zone.runway_area.y = 0;
    zone.runway_area.width = 8;
    zone.runway_area.height = 4;

    // Set water tiles adjacent to zone bottom edge (y=4)
    for (int16_t x = 0; x < 8; ++x) {
        terrain.set_terrain_type(x, 4, TerrainType::DeepVoid);
    }
}

// =============================================================================
// Tests
// =============================================================================

void test_aero_fully_operational() {
    printf("Testing aero port fully operational...\n");

    MockTerrainForOps terrain;
    terrain.set_default_elevation(10);
    MockTransportForOps transport;
    transport.set_accessible(true);

    PortComponent port;
    PortZoneComponent zone;
    setup_valid_aero(port, zone);

    OperationalCheckResult result = check_port_operational(port, zone, terrain, transport);
    assert(result.zone_valid == true);
    assert(result.infrastructure_met == true);
    assert(result.pathway_connected == true);
    assert(result.has_capacity == true);
    assert(result.is_operational() == true);

    printf("  PASS: Aero port is operational when all conditions met\n");
}

void test_aero_not_operational_no_runway() {
    printf("Testing aero port not operational without runway...\n");

    MockTerrainForOps terrain;
    terrain.set_default_elevation(10);
    MockTransportForOps transport;
    transport.set_accessible(true);

    PortComponent port;
    PortZoneComponent zone;
    setup_valid_aero(port, zone);
    zone.has_runway = false;

    OperationalCheckResult result = check_port_operational(port, zone, terrain, transport);
    assert(result.infrastructure_met == false);
    assert(result.is_operational() == false);

    printf("  PASS: Aero port not operational without runway\n");
}

void test_aero_not_operational_no_pathway() {
    printf("Testing aero port not operational without pathway...\n");

    MockTerrainForOps terrain;
    terrain.set_default_elevation(10);
    MockTransportForOps transport;
    transport.set_accessible(false);

    PortComponent port;
    PortZoneComponent zone;
    setup_valid_aero(port, zone);

    OperationalCheckResult result = check_port_operational(port, zone, terrain, transport);
    assert(result.pathway_connected == false);
    assert(result.is_operational() == false);

    printf("  PASS: Aero port not operational without pathway\n");
}

void test_aero_not_operational_zero_capacity() {
    printf("Testing aero port not operational with zero capacity...\n");

    MockTerrainForOps terrain;
    terrain.set_default_elevation(10);
    MockTransportForOps transport;
    transport.set_accessible(true);

    PortComponent port;
    PortZoneComponent zone;
    setup_valid_aero(port, zone);
    port.capacity = 0;

    OperationalCheckResult result = check_port_operational(port, zone, terrain, transport);
    assert(result.has_capacity == false);
    assert(result.is_operational() == false);

    printf("  PASS: Aero port not operational with zero capacity\n");
}

void test_aero_not_operational_empty_zone() {
    printf("Testing aero port not operational with empty zone rect...\n");

    MockTerrainForOps terrain;
    MockTransportForOps transport;
    transport.set_accessible(true);

    PortComponent port;
    port.port_type = PortType::Aero;
    port.capacity = 100;

    PortZoneComponent zone;
    zone.port_type = PortType::Aero;
    zone.has_runway = true;
    // Empty zone rect
    zone.runway_area.x = 0;
    zone.runway_area.y = 0;
    zone.runway_area.width = 0;
    zone.runway_area.height = 0;

    OperationalCheckResult result = check_port_operational(port, zone, terrain, transport);
    assert(result.is_operational() == false);

    printf("  PASS: Empty zone rect results in non-operational\n");
}

void test_aqua_fully_operational() {
    printf("Testing aqua port fully operational...\n");

    MockTerrainForOps terrain;
    terrain.set_default_elevation(10);
    MockTransportForOps transport;
    transport.set_accessible(true);

    PortComponent port;
    PortZoneComponent zone;
    setup_valid_aqua(port, zone, terrain);

    OperationalCheckResult result = check_port_operational(port, zone, terrain, transport);
    assert(result.zone_valid == true);
    assert(result.infrastructure_met == true);
    assert(result.pathway_connected == true);
    assert(result.has_capacity == true);
    assert(result.is_operational() == true);

    printf("  PASS: Aqua port is operational when all conditions met\n");
}

void test_aqua_not_operational_no_dock() {
    printf("Testing aqua port not operational without dock...\n");

    MockTerrainForOps terrain;
    terrain.set_default_elevation(10);
    MockTransportForOps transport;
    transport.set_accessible(true);

    PortComponent port;
    PortZoneComponent zone;
    setup_valid_aqua(port, zone, terrain);
    zone.has_dock = false;

    OperationalCheckResult result = check_port_operational(port, zone, terrain, transport);
    assert(result.infrastructure_met == false);
    assert(result.is_operational() == false);

    printf("  PASS: Aqua port not operational without dock\n");
}

// =============================================================================
// Event emission tests
// =============================================================================

void test_event_emitted_on_become_operational() {
    printf("Testing PortOperationalEvent emitted when becoming operational...\n");

    MockTerrainForOps terrain;
    terrain.set_default_elevation(10);
    MockTransportForOps transport;
    transport.set_accessible(true);

    PortComponent port;
    PortZoneComponent zone;
    setup_valid_aero(port, zone);
    port.is_operational = false;  // Start non-operational

    std::vector<PortOperationalEvent> events;
    bool changed = update_port_operational_status(
        port, zone, terrain, transport, 42, 1, events);

    assert(changed == true);
    assert(port.is_operational == true);
    assert(events.size() == 1);
    assert(events[0].port == 42);
    assert(events[0].is_operational == true);
    assert(events[0].owner == 1);

    printf("  PASS: Event emitted on becoming operational\n");
}

void test_event_emitted_on_become_non_operational() {
    printf("Testing PortOperationalEvent emitted when becoming non-operational...\n");

    MockTerrainForOps terrain;
    terrain.set_default_elevation(10);
    MockTransportForOps transport;
    transport.set_accessible(false);  // No pathway access

    PortComponent port;
    PortZoneComponent zone;
    setup_valid_aero(port, zone);
    port.is_operational = true;  // Start operational

    std::vector<PortOperationalEvent> events;
    bool changed = update_port_operational_status(
        port, zone, terrain, transport, 99, 2, events);

    assert(changed == true);
    assert(port.is_operational == false);
    assert(events.size() == 1);
    assert(events[0].port == 99);
    assert(events[0].is_operational == false);
    assert(events[0].owner == 2);

    printf("  PASS: Event emitted on becoming non-operational\n");
}

void test_no_event_when_status_unchanged() {
    printf("Testing no event when status unchanged...\n");

    MockTerrainForOps terrain;
    terrain.set_default_elevation(10);
    MockTransportForOps transport;
    transport.set_accessible(true);

    PortComponent port;
    PortZoneComponent zone;
    setup_valid_aero(port, zone);
    port.is_operational = true;  // Already operational

    std::vector<PortOperationalEvent> events;
    bool changed = update_port_operational_status(
        port, zone, terrain, transport, 42, 1, events);

    assert(changed == false);
    assert(port.is_operational == true);
    assert(events.size() == 0);

    printf("  PASS: No event when status unchanged\n");
}

void test_no_event_when_stays_non_operational() {
    printf("Testing no event when stays non-operational...\n");

    MockTerrainForOps terrain;
    terrain.set_default_elevation(10);
    MockTransportForOps transport;
    transport.set_accessible(false);

    PortComponent port;
    PortZoneComponent zone;
    setup_valid_aero(port, zone);
    port.is_operational = false;  // Already non-operational

    std::vector<PortOperationalEvent> events;
    bool changed = update_port_operational_status(
        port, zone, terrain, transport, 42, 1, events);

    assert(changed == false);
    assert(port.is_operational == false);
    assert(events.size() == 0);

    printf("  PASS: No event when stays non-operational\n");
}

void test_non_operational_no_demand_or_trade() {
    printf("Testing non-operational port provides no capacity...\n");

    MockTerrainForOps terrain;
    terrain.set_default_elevation(10);
    MockTransportForOps transport;
    transport.set_accessible(false);  // Disconnected

    PortComponent port;
    PortZoneComponent zone;
    setup_valid_aero(port, zone);

    OperationalCheckResult result = check_port_operational(port, zone, terrain, transport);
    assert(result.is_operational() == false);

    // A non-operational port should not provide demand bonus or trade income.
    // This is enforced by the system checking is_operational before applying bonuses.
    // We verify the port is correctly marked non-operational.
    port.is_operational = result.is_operational();
    assert(port.is_operational == false);

    printf("  PASS: Non-operational port correctly flagged\n");
}

// =============================================================================
// Main
// =============================================================================

int main() {
    printf("=== Port Operational Status Tests (Epic 8, Ticket E8-011) ===\n\n");

    // Operational check tests
    test_aero_fully_operational();
    test_aero_not_operational_no_runway();
    test_aero_not_operational_no_pathway();
    test_aero_not_operational_zero_capacity();
    test_aero_not_operational_empty_zone();
    test_aqua_fully_operational();
    test_aqua_not_operational_no_dock();

    // Event emission tests
    test_event_emitted_on_become_operational();
    test_event_emitted_on_become_non_operational();
    test_no_event_when_status_unchanged();
    test_no_event_when_stays_non_operational();
    test_non_operational_no_demand_or_trade();

    printf("\n=== All Port Operational Status Tests Passed ===\n");
    return 0;
}
