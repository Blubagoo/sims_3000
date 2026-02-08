/**
 * @file test_port_development.cpp
 * @brief Unit tests for port development level calculation (Epic 8, Ticket E8-012)
 *
 * Tests cover:
 * - calculate_development_level threshold boundaries
 * - update_development_level level transitions and event emission
 * - Level name strings
 * - Edge cases (zero capacity, max capacity, same level)
 */

#include <sims3000/port/PortDevelopment.h>
#include <sims3000/port/PortZoneComponent.h>
#include <sims3000/port/PortEvents.h>
#include <cassert>
#include <cstdio>
#include <vector>

using namespace sims3000::port;

// =============================================================================
// calculate_development_level tests
// =============================================================================

void test_level_0_undeveloped() {
    printf("Testing level 0 (Undeveloped) at capacity 0...\n");

    assert(calculate_development_level(0) == 0);

    printf("  PASS: capacity 0 -> level 0\n");
}

void test_level_0_below_basic() {
    printf("Testing level 0 below Basic threshold...\n");

    assert(calculate_development_level(1) == 0);
    assert(calculate_development_level(50) == 0);
    assert(calculate_development_level(99) == 0);

    printf("  PASS: capacities 1, 50, 99 -> level 0\n");
}

void test_level_1_basic_threshold() {
    printf("Testing level 1 (Basic) at threshold 100...\n");

    assert(calculate_development_level(100) == 1);
    assert(calculate_development_level(101) == 1);
    assert(calculate_development_level(499) == 1);

    printf("  PASS: capacities 100, 101, 499 -> level 1\n");
}

void test_level_2_standard_threshold() {
    printf("Testing level 2 (Standard) at threshold 500...\n");

    assert(calculate_development_level(500) == 2);
    assert(calculate_development_level(501) == 2);
    assert(calculate_development_level(1999) == 2);

    printf("  PASS: capacities 500, 501, 1999 -> level 2\n");
}

void test_level_3_major_threshold() {
    printf("Testing level 3 (Major) at threshold 2000...\n");

    assert(calculate_development_level(2000) == 3);
    assert(calculate_development_level(2001) == 3);
    assert(calculate_development_level(4999) == 3);

    printf("  PASS: capacities 2000, 2001, 4999 -> level 3\n");
}

void test_level_4_international_threshold() {
    printf("Testing level 4 (International) at threshold 5000...\n");

    assert(calculate_development_level(5000) == 4);
    assert(calculate_development_level(5001) == 4);
    assert(calculate_development_level(10000) == 4);
    assert(calculate_development_level(65535) == 4);  // uint16_t max

    printf("  PASS: capacities 5000, 5001, 10000, 65535 -> level 4\n");
}

void test_exact_boundary_values() {
    printf("Testing exact boundary values...\n");

    // Each threshold boundary: value-1 should be previous level
    assert(calculate_development_level(99) == 0);
    assert(calculate_development_level(100) == 1);

    assert(calculate_development_level(499) == 1);
    assert(calculate_development_level(500) == 2);

    assert(calculate_development_level(1999) == 2);
    assert(calculate_development_level(2000) == 3);

    assert(calculate_development_level(4999) == 3);
    assert(calculate_development_level(5000) == 4);

    printf("  PASS: All boundary transitions correct\n");
}

// =============================================================================
// development_level_name tests
// =============================================================================

void test_level_names() {
    printf("Testing development level names...\n");

    assert(strcmp(development_level_name(0), "Undeveloped") == 0);
    assert(strcmp(development_level_name(1), "Basic") == 0);
    assert(strcmp(development_level_name(2), "Standard") == 0);
    assert(strcmp(development_level_name(3), "Major") == 0);
    assert(strcmp(development_level_name(4), "International") == 0);
    assert(strcmp(development_level_name(5), "Unknown") == 0);
    assert(strcmp(development_level_name(255), "Unknown") == 0);

    printf("  PASS: All level names correct\n");
}

// =============================================================================
// update_development_level tests
// =============================================================================

void test_update_level_upgrade_emits_event() {
    printf("Testing update_development_level upgrade emits event...\n");

    PortZoneComponent zone;
    zone.zone_level = 0;  // Start at Undeveloped

    std::vector<PortUpgradedEvent> events;
    uint32_t entity_id = 42;

    // Upgrade from 0 -> 1 (capacity 100)
    bool changed = update_development_level(zone, 100, events, entity_id);

    assert(changed == true);
    assert(zone.zone_level == 1);
    assert(events.size() == 1);
    assert(events[0].port == 42);
    assert(events[0].old_level == 0);
    assert(events[0].new_level == 1);

    printf("  PASS: Upgrade 0->1 emitted event correctly\n");
}

void test_update_level_no_change_no_event() {
    printf("Testing update_development_level no change emits no event...\n");

    PortZoneComponent zone;
    zone.zone_level = 2;  // Already Standard

    std::vector<PortUpgradedEvent> events;
    uint32_t entity_id = 99;

    // Capacity 500 corresponds to level 2, same as current
    bool changed = update_development_level(zone, 500, events, entity_id);

    assert(changed == false);
    assert(zone.zone_level == 2);
    assert(events.empty());

    printf("  PASS: No change, no event emitted\n");
}

void test_update_level_downgrade_emits_event() {
    printf("Testing update_development_level downgrade emits event...\n");

    PortZoneComponent zone;
    zone.zone_level = 3;  // Major

    std::vector<PortUpgradedEvent> events;
    uint32_t entity_id = 7;

    // Capacity 200 corresponds to level 1 (Basic), downgrade from 3
    bool changed = update_development_level(zone, 200, events, entity_id);

    assert(changed == true);
    assert(zone.zone_level == 1);
    assert(events.size() == 1);
    assert(events[0].port == 7);
    assert(events[0].old_level == 3);
    assert(events[0].new_level == 1);

    printf("  PASS: Downgrade 3->1 emitted event correctly\n");
}

void test_update_level_multiple_upgrades() {
    printf("Testing multiple sequential upgrades...\n");

    PortZoneComponent zone;
    zone.zone_level = 0;

    std::vector<PortUpgradedEvent> events;
    uint32_t entity_id = 10;

    // Upgrade 0 -> 1
    update_development_level(zone, 100, events, entity_id);
    assert(zone.zone_level == 1);

    // Upgrade 1 -> 2
    update_development_level(zone, 500, events, entity_id);
    assert(zone.zone_level == 2);

    // Upgrade 2 -> 4 (skip 3)
    update_development_level(zone, 5000, events, entity_id);
    assert(zone.zone_level == 4);

    assert(events.size() == 3);
    assert(events[0].old_level == 0 && events[0].new_level == 1);
    assert(events[1].old_level == 1 && events[1].new_level == 2);
    assert(events[2].old_level == 2 && events[2].new_level == 4);

    printf("  PASS: 3 sequential upgrades produced 3 events\n");
}

void test_update_level_zero_capacity() {
    printf("Testing update with zero capacity...\n");

    PortZoneComponent zone;
    zone.zone_level = 2;

    std::vector<PortUpgradedEvent> events;
    uint32_t entity_id = 55;

    // Capacity 0 -> level 0
    bool changed = update_development_level(zone, 0, events, entity_id);

    assert(changed == true);
    assert(zone.zone_level == 0);
    assert(events.size() == 1);
    assert(events[0].old_level == 2);
    assert(events[0].new_level == 0);

    printf("  PASS: Zero capacity downgraded to level 0\n");
}

void test_update_level_max_capacity() {
    printf("Testing update with maximum uint16_t capacity...\n");

    PortZoneComponent zone;
    zone.zone_level = 0;

    std::vector<PortUpgradedEvent> events;
    uint32_t entity_id = 1;

    bool changed = update_development_level(zone, 65535, events, entity_id);

    assert(changed == true);
    assert(zone.zone_level == 4);
    assert(events.size() == 1);

    printf("  PASS: Max capacity -> level 4\n");
}

void test_update_preserves_other_fields() {
    printf("Testing update preserves other PortZoneComponent fields...\n");

    PortZoneComponent zone;
    zone.port_type = PortType::Aqua;
    zone.zone_level = 1;
    zone.has_runway = false;
    zone.has_dock = true;
    zone.runway_length = 0;
    zone.dock_count = 5;
    zone.zone_tiles = 200;

    std::vector<PortUpgradedEvent> events;

    update_development_level(zone, 2000, events, 100);

    // zone_level should change
    assert(zone.zone_level == 3);

    // Everything else should be preserved
    assert(zone.port_type == PortType::Aqua);
    assert(zone.has_runway == false);
    assert(zone.has_dock == true);
    assert(zone.runway_length == 0);
    assert(zone.dock_count == 5);
    assert(zone.zone_tiles == 200);

    printf("  PASS: Other fields preserved after level update\n");
}

// =============================================================================
// Constants verification
// =============================================================================

void test_threshold_constants() {
    printf("Testing threshold constants...\n");

    assert(DEVELOPMENT_THRESHOLDS[0] == 0);
    assert(DEVELOPMENT_THRESHOLDS[1] == 100);
    assert(DEVELOPMENT_THRESHOLDS[2] == 500);
    assert(DEVELOPMENT_THRESHOLDS[3] == 2000);
    assert(DEVELOPMENT_THRESHOLDS[4] == 5000);
    assert(DEVELOPMENT_LEVEL_COUNT == 5);
    assert(MAX_DEVELOPMENT_LEVEL == 4);

    printf("  PASS: All threshold constants correct\n");
}

// =============================================================================
// Main
// =============================================================================

int main() {
    printf("=== Port Development Level Tests (Epic 8, Ticket E8-012) ===\n\n");

    // calculate_development_level tests
    test_level_0_undeveloped();
    test_level_0_below_basic();
    test_level_1_basic_threshold();
    test_level_2_standard_threshold();
    test_level_3_major_threshold();
    test_level_4_international_threshold();
    test_exact_boundary_values();

    // Level name tests
    test_level_names();

    // update_development_level tests
    test_update_level_upgrade_emits_event();
    test_update_level_no_change_no_event();
    test_update_level_downgrade_emits_event();
    test_update_level_multiple_upgrades();
    test_update_level_zero_capacity();
    test_update_level_max_capacity();
    test_update_preserves_other_fields();

    // Constants tests
    test_threshold_constants();

    printf("\n=== All Port Development Level Tests Passed ===\n");
    return 0;
}
