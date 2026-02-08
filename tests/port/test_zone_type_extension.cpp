/**
 * @file test_zone_type_extension.cpp
 * @brief Unit tests for ZoneType extension with port zones (Epic 8, Ticket E8-031)
 *
 * Tests cover:
 * - AeroPort and AquaPort enum values (4 and 5)
 * - ZONE_TYPE_COUNT updated to 6
 * - BASE_ZONE_TYPE_COUNT remains 3
 * - Intentional gap at value 3
 * - is_port_zone_type() helper function
 * - ZoneComponent stores and retrieves port zone types
 * - ZoneCounts includes port zone fields
 * - Zone overlay color constants for port zones
 * - ZoneSystem accepts port zone placement and tracks counts
 * - ZoneSystem returns 0 demand for port zones (stub)
 * - ZoneSystem redesignation works with port zone types
 * - ZoneSystem removal works with port zone types
 */

#include <sims3000/zone/ZoneTypes.h>
#include <sims3000/zone/ZoneSystem.h>
#include <cassert>
#include <cstdio>

using namespace sims3000::zone;

// ============================================================================
// ZoneType Enum Extension Tests
// ============================================================================

void test_port_zone_type_enum_values() {
    printf("Testing port ZoneType enum values...\n");

    // Existing values must not change
    assert(static_cast<std::uint8_t>(ZoneType::Habitation) == 0);
    assert(static_cast<std::uint8_t>(ZoneType::Exchange) == 1);
    assert(static_cast<std::uint8_t>(ZoneType::Fabrication) == 2);

    // New port zone values
    assert(static_cast<std::uint8_t>(ZoneType::AeroPort) == 4);
    assert(static_cast<std::uint8_t>(ZoneType::AquaPort) == 5);

    printf("  PASS: Port ZoneType enum values correct (AeroPort=4, AquaPort=5)\n");
}

void test_zone_type_count_updated() {
    printf("Testing ZONE_TYPE_COUNT updated...\n");

    assert(ZONE_TYPE_COUNT == 6);
    assert(BASE_ZONE_TYPE_COUNT == 3);

    printf("  PASS: ZONE_TYPE_COUNT == 6, BASE_ZONE_TYPE_COUNT == 3\n");
}

void test_intentional_gap_at_value_3() {
    printf("Testing intentional gap at value 3...\n");

    // Value 3 is not assigned to any ZoneType enum member.
    // Existing values: 0, 1, 2, 4, 5
    // This test ensures the gap is maintained.
    assert(static_cast<std::uint8_t>(ZoneType::Fabrication) == 2);
    assert(static_cast<std::uint8_t>(ZoneType::AeroPort) == 4);

    // The difference between Fabrication and AeroPort should be 2 (gap of 1)
    assert(static_cast<std::uint8_t>(ZoneType::AeroPort) -
           static_cast<std::uint8_t>(ZoneType::Fabrication) == 2);

    printf("  PASS: Intentional gap at value 3 is maintained\n");
}

void test_is_port_zone_type() {
    printf("Testing is_port_zone_type()...\n");

    // Base zone types should return false
    assert(!is_port_zone_type(ZoneType::Habitation));
    assert(!is_port_zone_type(ZoneType::Exchange));
    assert(!is_port_zone_type(ZoneType::Fabrication));

    // Port zone types should return true
    assert(is_port_zone_type(ZoneType::AeroPort));
    assert(is_port_zone_type(ZoneType::AquaPort));

    printf("  PASS: is_port_zone_type() correctly identifies port zone types\n");
}

// ============================================================================
// ZoneComponent with Port Zone Types
// ============================================================================

void test_zone_component_port_types() {
    printf("Testing ZoneComponent with port zone types...\n");

    ZoneComponent zc = {};

    // Test AeroPort
    zc.setZoneType(ZoneType::AeroPort);
    assert(zc.getZoneType() == ZoneType::AeroPort);
    assert(zc.zone_type == 4);

    // Test AquaPort
    zc.setZoneType(ZoneType::AquaPort);
    assert(zc.getZoneType() == ZoneType::AquaPort);
    assert(zc.zone_type == 5);

    // ZoneComponent size must still be 4 bytes
    assert(sizeof(ZoneComponent) == 4);

    printf("  PASS: ZoneComponent stores and retrieves port zone types correctly\n");
}

// ============================================================================
// ZoneCounts with Port Zone Fields
// ============================================================================

void test_zone_counts_port_fields() {
    printf("Testing ZoneCounts port zone fields...\n");

    ZoneCounts counts;

    // New fields should be zero-initialized
    assert(counts.aeroport_total == 0);
    assert(counts.aquaport_total == 0);

    // Set and verify
    counts.aeroport_total = 10;
    counts.aquaport_total = 5;
    assert(counts.aeroport_total == 10);
    assert(counts.aquaport_total == 5);

    // Existing fields still work
    assert(counts.habitation_total == 0);
    assert(counts.exchange_total == 0);
    assert(counts.fabrication_total == 0);

    printf("  PASS: ZoneCounts port zone fields work correctly\n");
}

// ============================================================================
// Zone Overlay Color Constants
// ============================================================================

void test_port_zone_overlay_colors() {
    printf("Testing port zone overlay color constants...\n");

    // AeroPort: sky blue (#44aaff)
    assert(ZONE_COLOR_AEROPORT_R == 68);
    assert(ZONE_COLOR_AEROPORT_G == 170);
    assert(ZONE_COLOR_AEROPORT_B == 255);

    // AquaPort: deep ocean blue (#0066cc)
    assert(ZONE_COLOR_AQUAPORT_R == 0);
    assert(ZONE_COLOR_AQUAPORT_G == 102);
    assert(ZONE_COLOR_AQUAPORT_B == 204);

    // Ensure existing zone colors are defined
    assert(ZONE_COLOR_HABITATION_R == 0);
    assert(ZONE_COLOR_HABITATION_G == 170);
    assert(ZONE_COLOR_HABITATION_B == 170);

    assert(ZONE_COLOR_EXCHANGE_R == 255);
    assert(ZONE_COLOR_EXCHANGE_G == 170);
    assert(ZONE_COLOR_EXCHANGE_B == 0);

    assert(ZONE_COLOR_FABRICATION_R == 255);
    assert(ZONE_COLOR_FABRICATION_G == 0);
    assert(ZONE_COLOR_FABRICATION_B == 170);

    // Overlay alpha
    assert(ZONE_OVERLAY_ALPHA == 38);

    // AeroPort and AquaPort colors must be distinct from each other
    assert(ZONE_COLOR_AEROPORT_R != ZONE_COLOR_AQUAPORT_R ||
           ZONE_COLOR_AEROPORT_G != ZONE_COLOR_AQUAPORT_G ||
           ZONE_COLOR_AEROPORT_B != ZONE_COLOR_AQUAPORT_B);

    // Port colors must be distinct from all base zone colors
    // (Check at least one channel differs)
    // AeroPort vs Habitation
    assert(ZONE_COLOR_AEROPORT_R != ZONE_COLOR_HABITATION_R ||
           ZONE_COLOR_AEROPORT_G != ZONE_COLOR_HABITATION_G ||
           ZONE_COLOR_AEROPORT_B != ZONE_COLOR_HABITATION_B);

    // AeroPort vs Exchange
    assert(ZONE_COLOR_AEROPORT_R != ZONE_COLOR_EXCHANGE_R ||
           ZONE_COLOR_AEROPORT_G != ZONE_COLOR_EXCHANGE_G ||
           ZONE_COLOR_AEROPORT_B != ZONE_COLOR_EXCHANGE_B);

    // AeroPort vs Fabrication
    assert(ZONE_COLOR_AEROPORT_R != ZONE_COLOR_FABRICATION_R ||
           ZONE_COLOR_AEROPORT_G != ZONE_COLOR_FABRICATION_G ||
           ZONE_COLOR_AEROPORT_B != ZONE_COLOR_FABRICATION_B);

    printf("  PASS: Port zone overlay color constants are defined and distinct\n");
}

// ============================================================================
// ZoneSystem Port Zone Placement Tests
// ============================================================================

void test_zone_system_place_aeroport() {
    printf("Testing ZoneSystem placement of AeroPort zone...\n");

    ZoneSystem system(nullptr, nullptr, 128);

    // Place an AeroPort zone
    bool placed = system.place_zone(10, 10, ZoneType::AeroPort,
                                     ZoneDensity::LowDensity, 0, 100);
    assert(placed);

    // Verify zone type is set
    ZoneType type;
    bool found = system.get_zone_type(10, 10, type);
    assert(found);
    assert(type == ZoneType::AeroPort);

    // Verify is_zoned
    assert(system.is_zoned(10, 10));

    // Verify zone count
    assert(system.get_zone_count(0, ZoneType::AeroPort) == 1);
    assert(system.get_zone_count(0, ZoneType::Habitation) == 0);

    printf("  PASS: ZoneSystem places AeroPort zones correctly\n");
}

void test_zone_system_place_aquaport() {
    printf("Testing ZoneSystem placement of AquaPort zone...\n");

    ZoneSystem system(nullptr, nullptr, 128);

    // Place an AquaPort zone
    bool placed = system.place_zone(20, 20, ZoneType::AquaPort,
                                     ZoneDensity::HighDensity, 0, 200);
    assert(placed);

    // Verify zone type
    ZoneType type;
    bool found = system.get_zone_type(20, 20, type);
    assert(found);
    assert(type == ZoneType::AquaPort);

    // Verify zone count
    assert(system.get_zone_count(0, ZoneType::AquaPort) == 1);

    // Verify density
    ZoneDensity density;
    bool found_d = system.get_zone_density(20, 20, density);
    assert(found_d);
    assert(density == ZoneDensity::HighDensity);

    printf("  PASS: ZoneSystem places AquaPort zones correctly\n");
}

void test_zone_system_port_zone_counts() {
    printf("Testing ZoneSystem port zone counts...\n");

    ZoneSystem system(nullptr, nullptr, 128);

    // Place multiple port zones
    system.place_zone(0, 0, ZoneType::AeroPort, ZoneDensity::LowDensity, 0, 1);
    system.place_zone(1, 0, ZoneType::AeroPort, ZoneDensity::LowDensity, 0, 2);
    system.place_zone(2, 0, ZoneType::AquaPort, ZoneDensity::LowDensity, 0, 3);

    // Also place base zone types
    system.place_zone(3, 0, ZoneType::Habitation, ZoneDensity::LowDensity, 0, 4);

    const ZoneCounts& counts = system.get_zone_counts(0);
    assert(counts.aeroport_total == 2);
    assert(counts.aquaport_total == 1);
    assert(counts.habitation_total == 1);
    assert(counts.total == 4);

    printf("  PASS: ZoneSystem tracks port zone counts correctly\n");
}

// ============================================================================
// ZoneSystem Port Zone Demand Tests
// ============================================================================

void test_zone_system_port_demand_returns_zero() {
    printf("Testing ZoneSystem port zone demand returns 0 (stub)...\n");

    ZoneSystem system(nullptr, nullptr, 128);

    // Port zone demand is handled by PortSystem (future), not ZoneSystem
    // ZoneSystem should return 0 for port zone demand
    assert(system.get_demand_for_type(ZoneType::AeroPort, 0) == 0);
    assert(system.get_demand_for_type(ZoneType::AquaPort, 0) == 0);

    // Verify base zone demand still works (not zero by default due to base pressures)
    // Just verify it doesn't crash
    system.get_demand_for_type(ZoneType::Habitation, 0);
    system.get_demand_for_type(ZoneType::Exchange, 0);
    system.get_demand_for_type(ZoneType::Fabrication, 0);

    printf("  PASS: Port zone demand returns 0 (stub for future PortSystem)\n");
}

// ============================================================================
// ZoneSystem Port Zone Removal Tests
// ============================================================================

void test_zone_system_remove_port_zone() {
    printf("Testing ZoneSystem removal of port zones...\n");

    ZoneSystem system(nullptr, nullptr, 128);

    // Place and then remove a port zone
    system.place_zone(5, 5, ZoneType::AeroPort, ZoneDensity::LowDensity, 0, 10);
    assert(system.get_zone_count(0, ZoneType::AeroPort) == 1);

    DezoneResult result = system.remove_zones(5, 5, 1, 1, 0);
    assert(result.any_removed);
    assert(result.removed_count == 1);
    assert(system.get_zone_count(0, ZoneType::AeroPort) == 0);
    assert(!system.is_zoned(5, 5));

    printf("  PASS: ZoneSystem removes port zones correctly\n");
}

// ============================================================================
// ZoneSystem Port Zone Redesignation Tests
// ============================================================================

void test_zone_system_redesignate_to_port_zone() {
    printf("Testing ZoneSystem redesignation to port zone type...\n");

    ZoneSystem system(nullptr, nullptr, 128);

    // Place a base zone
    system.place_zone(10, 10, ZoneType::Habitation, ZoneDensity::LowDensity, 0, 1);
    assert(system.get_zone_count(0, ZoneType::Habitation) == 1);

    // Redesignate to AeroPort
    RedesignateResult result = system.redesignate_zone(10, 10,
        ZoneType::AeroPort, ZoneDensity::LowDensity, 0);
    assert(result.success);

    // Verify type changed
    ZoneType type;
    system.get_zone_type(10, 10, type);
    assert(type == ZoneType::AeroPort);
    assert(system.get_zone_count(0, ZoneType::AeroPort) == 1);
    assert(system.get_zone_count(0, ZoneType::Habitation) == 0);

    printf("  PASS: ZoneSystem redesignates to port zone type correctly\n");
}

void test_zone_system_redesignate_from_port_zone() {
    printf("Testing ZoneSystem redesignation from port zone to base zone...\n");

    ZoneSystem system(nullptr, nullptr, 128);

    // Place a port zone
    system.place_zone(15, 15, ZoneType::AquaPort, ZoneDensity::LowDensity, 0, 1);
    assert(system.get_zone_count(0, ZoneType::AquaPort) == 1);

    // Redesignate to base zone type
    RedesignateResult result = system.redesignate_zone(15, 15,
        ZoneType::Exchange, ZoneDensity::HighDensity, 0);
    assert(result.success);

    // Verify type and density changed
    ZoneType type;
    system.get_zone_type(15, 15, type);
    assert(type == ZoneType::Exchange);
    assert(system.get_zone_count(0, ZoneType::AquaPort) == 0);
    assert(system.get_zone_count(0, ZoneType::Exchange) == 1);

    printf("  PASS: ZoneSystem redesignates from port zone to base zone correctly\n");
}

void test_zone_system_redesignate_between_port_zones() {
    printf("Testing ZoneSystem redesignation between port zone types...\n");

    ZoneSystem system(nullptr, nullptr, 128);

    // Place AeroPort
    system.place_zone(20, 20, ZoneType::AeroPort, ZoneDensity::LowDensity, 0, 1);

    // Redesignate to AquaPort
    RedesignateResult result = system.redesignate_zone(20, 20,
        ZoneType::AquaPort, ZoneDensity::LowDensity, 0);
    assert(result.success);

    ZoneType type;
    system.get_zone_type(20, 20, type);
    assert(type == ZoneType::AquaPort);
    assert(system.get_zone_count(0, ZoneType::AeroPort) == 0);
    assert(system.get_zone_count(0, ZoneType::AquaPort) == 1);

    printf("  PASS: ZoneSystem redesignates between port zone types correctly\n");
}

// ============================================================================
// ZoneSystem Port Zone with place_zones (batch placement)
// ============================================================================

void test_zone_system_batch_place_port_zones() {
    printf("Testing ZoneSystem batch placement of port zones...\n");

    ZoneSystem system(nullptr, nullptr, 128);

    ZonePlacementRequest request;
    request.x = 0;
    request.y = 0;
    request.width = 3;
    request.height = 2;
    request.zone_type = ZoneType::AeroPort;
    request.density = ZoneDensity::LowDensity;
    request.player_id = 0;

    ZonePlacementResult result = system.place_zones(request);
    assert(result.any_placed);
    assert(result.placed_count == 6);
    assert(system.get_zone_count(0, ZoneType::AeroPort) == 6);

    // Verify each cell
    for (int y = 0; y < 2; ++y) {
        for (int x = 0; x < 3; ++x) {
            ZoneType type;
            bool found = system.get_zone_type(x, y, type);
            assert(found);
            assert(type == ZoneType::AeroPort);
        }
    }

    printf("  PASS: ZoneSystem batch placement of port zones works correctly\n");
}

// ============================================================================
// IZoneQueryable Interface with Port Zone Types
// ============================================================================

void test_izone_queryable_port_types() {
    printf("Testing IZoneQueryable interface with port zone types...\n");

    ZoneSystem system(nullptr, nullptr, 128);
    IZoneQueryable* queryable = &system;

    // Place port zones
    system.place_zone(5, 5, ZoneType::AeroPort, ZoneDensity::LowDensity, 0, 1);
    system.place_zone(6, 5, ZoneType::AquaPort, ZoneDensity::HighDensity, 0, 2);

    // Query through interface
    ZoneType type;
    assert(queryable->get_zone_type_at(5, 5, type));
    assert(type == ZoneType::AeroPort);

    assert(queryable->get_zone_type_at(6, 5, type));
    assert(type == ZoneType::AquaPort);

    assert(queryable->is_zoned_at(5, 5));
    assert(queryable->get_zone_count_for(0, ZoneType::AeroPort) == 1);
    assert(queryable->get_zone_count_for(0, ZoneType::AquaPort) == 1);

    printf("  PASS: IZoneQueryable interface works with port zone types\n");
}

// ============================================================================
// Main
// ============================================================================

int main() {
    printf("=== Zone Type Extension for Ports Tests (Epic 8, Ticket E8-031) ===\n\n");

    // Enum and constant tests
    test_port_zone_type_enum_values();
    test_zone_type_count_updated();
    test_intentional_gap_at_value_3();
    test_is_port_zone_type();

    // ZoneComponent tests
    test_zone_component_port_types();

    // ZoneCounts tests
    test_zone_counts_port_fields();

    // Color constants tests
    test_port_zone_overlay_colors();

    // ZoneSystem placement tests
    test_zone_system_place_aeroport();
    test_zone_system_place_aquaport();
    test_zone_system_port_zone_counts();

    // ZoneSystem demand tests
    test_zone_system_port_demand_returns_zero();

    // ZoneSystem removal tests
    test_zone_system_remove_port_zone();

    // ZoneSystem redesignation tests
    test_zone_system_redesignate_to_port_zone();
    test_zone_system_redesignate_from_port_zone();
    test_zone_system_redesignate_between_port_zones();

    // Batch placement tests
    test_zone_system_batch_place_port_zones();

    // IZoneQueryable interface tests
    test_izone_queryable_port_types();

    printf("\n=== All Zone Type Extension for Ports Tests Passed ===\n");
    return 0;
}
