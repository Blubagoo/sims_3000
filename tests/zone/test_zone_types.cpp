/**
 * @file test_zone_types.cpp
 * @brief Unit tests for ZoneTypes (Epic 4, Ticket 4-001)
 *
 * Tests:
 * - ZoneType enum values and ranges
 * - ZoneDensity enum values and ranges
 * - ZoneState enum values and ranges
 * - ZoneComponent size assertion (exactly 4 bytes)
 * - ZoneComponent trivially copyable
 * - ZoneComponent enum accessors
 * - Supporting structs (ZoneDemandData, ZoneCounts, etc.)
 */

#include <sims3000/zone/ZoneTypes.h>
#include <cassert>
#include <cstdio>

using namespace sims3000::zone;

void test_zone_type_enum() {
    printf("Testing ZoneType enum...\n");

    assert(static_cast<std::uint8_t>(ZoneType::Habitation) == 0);
    assert(static_cast<std::uint8_t>(ZoneType::Exchange) == 1);
    assert(static_cast<std::uint8_t>(ZoneType::Fabrication) == 2);
    assert(ZONE_TYPE_COUNT == 3);

    printf("  PASS: ZoneType enum values correct\n");
}

void test_zone_density_enum() {
    printf("Testing ZoneDensity enum...\n");

    assert(static_cast<std::uint8_t>(ZoneDensity::LowDensity) == 0);
    assert(static_cast<std::uint8_t>(ZoneDensity::HighDensity) == 1);
    assert(ZONE_DENSITY_COUNT == 2);

    printf("  PASS: ZoneDensity enum values correct\n");
}

void test_zone_state_enum() {
    printf("Testing ZoneState enum...\n");

    assert(static_cast<std::uint8_t>(ZoneState::Designated) == 0);
    assert(static_cast<std::uint8_t>(ZoneState::Occupied) == 1);
    assert(static_cast<std::uint8_t>(ZoneState::Stalled) == 2);
    assert(ZONE_STATE_COUNT == 3);

    printf("  PASS: ZoneState enum values correct\n");
}

void test_zone_component_size() {
    printf("Testing ZoneComponent size...\n");

    // Canonical requirement: exactly 4 bytes per CCR-002
    assert(sizeof(ZoneComponent) == 4);

    printf("  PASS: ZoneComponent is exactly 4 bytes\n");
}

void test_zone_component_copyable() {
    printf("Testing ZoneComponent trivially copyable...\n");

    // Must be trivially copyable for network serialization
    assert(std::is_trivially_copyable<ZoneComponent>::value);

    printf("  PASS: ZoneComponent is trivially copyable\n");
}

void test_zone_component_accessors() {
    printf("Testing ZoneComponent accessors...\n");

    ZoneComponent zc = {};

    // Test zone type
    zc.setZoneType(ZoneType::Exchange);
    assert(zc.getZoneType() == ZoneType::Exchange);
    assert(zc.zone_type == 1);

    // Test density
    zc.setDensity(ZoneDensity::HighDensity);
    assert(zc.getDensity() == ZoneDensity::HighDensity);
    assert(zc.density == 1);

    // Test state
    zc.setState(ZoneState::Occupied);
    assert(zc.getState() == ZoneState::Occupied);
    assert(zc.state == 1);

    // Test desirability
    zc.desirability = 128;
    assert(zc.desirability == 128);

    printf("  PASS: ZoneComponent accessors work correctly\n");
}

void test_zone_demand_data() {
    printf("Testing ZoneDemandData...\n");

    ZoneDemandData demand;
    assert(demand.habitation_demand == 0);
    assert(demand.exchange_demand == 0);
    assert(demand.fabrication_demand == 0);

    // Test range
    demand.habitation_demand = 100;
    demand.exchange_demand = -100;
    demand.fabrication_demand = 0;

    assert(demand.habitation_demand == 100);
    assert(demand.exchange_demand == -100);
    assert(demand.fabrication_demand == 0);

    printf("  PASS: ZoneDemandData works correctly\n");
}

void test_zone_counts() {
    printf("Testing ZoneCounts...\n");

    ZoneCounts counts;
    assert(counts.habitation_total == 0);
    assert(counts.exchange_total == 0);
    assert(counts.fabrication_total == 0);
    assert(counts.low_density_total == 0);
    assert(counts.high_density_total == 0);
    assert(counts.designated_total == 0);
    assert(counts.occupied_total == 0);
    assert(counts.stalled_total == 0);
    assert(counts.total == 0);

    // Test increment
    counts.habitation_total = 10;
    counts.low_density_total = 5;
    counts.designated_total = 7;
    counts.total = 10;

    assert(counts.habitation_total == 10);
    assert(counts.low_density_total == 5);
    assert(counts.designated_total == 7);
    assert(counts.total == 10);

    printf("  PASS: ZoneCounts works correctly\n");
}

void test_zone_placement_request() {
    printf("Testing ZonePlacementRequest...\n");

    ZonePlacementRequest req;
    req.x = 10;
    req.y = 20;
    req.width = 5;
    req.height = 5;
    req.zone_type = ZoneType::Habitation;
    req.density = ZoneDensity::LowDensity;
    req.player_id = 1;

    assert(req.x == 10);
    assert(req.y == 20);
    assert(req.width == 5);
    assert(req.height == 5);
    assert(req.zone_type == ZoneType::Habitation);
    assert(req.density == ZoneDensity::LowDensity);
    assert(req.player_id == 1);

    printf("  PASS: ZonePlacementRequest works correctly\n");
}

void test_zone_placement_result() {
    printf("Testing ZonePlacementResult...\n");

    ZonePlacementResult result;
    assert(result.placed_count == 0);
    assert(result.skipped_count == 0);
    assert(!result.any_placed);

    // Test success state
    result.placed_count = 10;
    result.skipped_count = 2;
    result.any_placed = true;

    assert(result.placed_count == 10);
    assert(result.skipped_count == 2);
    assert(result.any_placed);

    printf("  PASS: ZonePlacementResult works correctly\n");
}

void test_dezone_result() {
    printf("Testing DezoneResult...\n");

    DezoneResult result;
    assert(result.removed_count == 0);
    assert(result.skipped_count == 0);
    assert(!result.any_removed);

    // Test success state
    result.removed_count = 5;
    result.skipped_count = 1;
    result.any_removed = true;

    assert(result.removed_count == 5);
    assert(result.skipped_count == 1);
    assert(result.any_removed);

    printf("  PASS: DezoneResult works correctly\n");
}

int main() {
    printf("=== ZoneTypes Unit Tests (Epic 4, Ticket 4-001) ===\n\n");

    test_zone_type_enum();
    test_zone_density_enum();
    test_zone_state_enum();
    test_zone_component_size();
    test_zone_component_copyable();
    test_zone_component_accessors();
    test_zone_demand_data();
    test_zone_counts();
    test_zone_placement_request();
    test_zone_placement_result();
    test_dezone_result();

    printf("\n=== All ZoneTypes Tests Passed ===\n");
    return 0;
}
