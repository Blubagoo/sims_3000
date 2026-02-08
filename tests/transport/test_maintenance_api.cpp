/**
 * @file test_maintenance_api.cpp
 * @brief Unit tests for MaintenanceAPI (Epic 7, Ticket E7-027)
 *
 * Tests:
 * - Health restoration capped at 255
 * - Capacity recalculated after maintenance
 * - last_maintained_tick updated
 * - Edge cases: zero restoration, already max health, overflow
 */

#include <sims3000/transport/MaintenanceAPI.h>
#include <cassert>
#include <cstdio>

using namespace sims3000::transport;

void test_basic_maintenance() {
    printf("Testing basic maintenance restores health...\n");

    RoadComponent road{};
    road.base_capacity = 1000;
    road.health = 100;
    road.current_capacity = 0;  // Intentionally wrong, should be recalculated
    road.last_maintained_tick = 0;

    apply_maintenance(road, 50, 500);

    assert(road.health == 150);
    // Capacity should be recalculated: (1000 * 150) / 255 = 588
    uint16_t expected_cap = static_cast<uint16_t>((1000u * 150u) / 255u);
    assert(road.current_capacity == expected_cap);
    assert(road.last_maintained_tick == 500);

    printf("  PASS: Health=150, capacity=%u, tick=500\n", road.current_capacity);
}

void test_maintenance_caps_at_255() {
    printf("Testing maintenance caps health at 255...\n");

    RoadComponent road{};
    road.base_capacity = 1000;
    road.health = 200;
    road.last_maintained_tick = 0;

    apply_maintenance(road, 100, 1000);

    assert(road.health == 255);  // 200 + 100 = 300, capped at 255
    assert(road.current_capacity == 1000);  // Full capacity at max health
    assert(road.last_maintained_tick == 1000);

    printf("  PASS: Health capped at 255\n");
}

void test_maintenance_max_health_max_restore() {
    printf("Testing maintenance with max health and max restore...\n");

    RoadComponent road{};
    road.base_capacity = 500;
    road.health = 255;
    road.last_maintained_tick = 0;

    apply_maintenance(road, 255, 9999);

    assert(road.health == 255);  // 255 + 255 = 510, capped at 255
    assert(road.current_capacity == 500);
    assert(road.last_maintained_tick == 9999);

    printf("  PASS: Already max health stays at 255\n");
}

void test_maintenance_zero_restore() {
    printf("Testing maintenance with zero restoration...\n");

    RoadComponent road{};
    road.base_capacity = 1000;
    road.health = 100;
    road.last_maintained_tick = 0;

    apply_maintenance(road, 0, 2000);

    assert(road.health == 100);  // No change
    // Capacity still recalculated: (1000 * 100) / 255 = 392
    uint16_t expected_cap = static_cast<uint16_t>((1000u * 100u) / 255u);
    assert(road.current_capacity == expected_cap);
    assert(road.last_maintained_tick == 2000);

    printf("  PASS: Zero restoration keeps health, updates tick\n");
}

void test_maintenance_zero_health_full_restore() {
    printf("Testing maintenance from zero health...\n");

    RoadComponent road{};
    road.base_capacity = 1000;
    road.health = 0;
    road.current_capacity = 0;
    road.last_maintained_tick = 0;

    apply_maintenance(road, 255, 3000);

    assert(road.health == 255);
    assert(road.current_capacity == 1000);
    assert(road.last_maintained_tick == 3000);

    printf("  PASS: Full restore from zero health\n");
}

void test_maintenance_updates_tick() {
    printf("Testing last_maintained_tick is updated...\n");

    RoadComponent road{};
    road.base_capacity = 100;
    road.health = 200;
    road.last_maintained_tick = 100;

    apply_maintenance(road, 10, 500);
    assert(road.last_maintained_tick == 500);

    apply_maintenance(road, 10, 1000);
    assert(road.last_maintained_tick == 1000);

    apply_maintenance(road, 10, 999999);
    assert(road.last_maintained_tick == 999999);

    printf("  PASS: last_maintained_tick updated correctly on each call\n");
}

void test_maintenance_capacity_recalculated() {
    printf("Testing capacity is recalculated after maintenance...\n");

    RoadComponent road{};
    road.base_capacity = 500;
    road.health = 50;
    road.current_capacity = 0;  // Wrong value, should be fixed

    apply_maintenance(road, 100, 4000);

    // Health is now 150
    assert(road.health == 150);
    // Expected capacity: (500 * 150) / 255 = 294
    uint16_t expected_cap = static_cast<uint16_t>((500u * 150u) / 255u);
    assert(road.current_capacity == expected_cap);

    printf("  PASS: Capacity recalculated to %u\n", road.current_capacity);
}

void test_maintenance_small_restore() {
    printf("Testing small incremental maintenance...\n");

    RoadComponent road{};
    road.base_capacity = 1000;
    road.health = 100;
    road.last_maintained_tick = 0;

    apply_maintenance(road, 1, 100);
    assert(road.health == 101);

    apply_maintenance(road, 1, 200);
    assert(road.health == 102);

    apply_maintenance(road, 1, 300);
    assert(road.health == 103);

    // Verify capacity tracks health
    uint16_t expected_cap = static_cast<uint16_t>((1000u * 103u) / 255u);
    assert(road.current_capacity == expected_cap);
    assert(road.last_maintained_tick == 300);

    printf("  PASS: Incremental maintenance works correctly\n");
}

void test_maintenance_boundary_254_to_255() {
    printf("Testing maintenance at boundary 254->255...\n");

    RoadComponent road{};
    road.base_capacity = 1000;
    road.health = 254;
    road.last_maintained_tick = 0;

    apply_maintenance(road, 1, 5000);

    assert(road.health == 255);
    assert(road.current_capacity == 1000);
    assert(road.last_maintained_tick == 5000);

    printf("  PASS: 254 + 1 = 255 (full health)\n");
}

int main() {
    printf("=== MaintenanceAPI Unit Tests (Epic 7, Ticket E7-027) ===\n\n");

    test_basic_maintenance();
    test_maintenance_caps_at_255();
    test_maintenance_max_health_max_restore();
    test_maintenance_zero_restore();
    test_maintenance_zero_health_full_restore();
    test_maintenance_updates_tick();
    test_maintenance_capacity_recalculated();
    test_maintenance_small_restore();
    test_maintenance_boundary_254_to_255();

    printf("\n=== All MaintenanceAPI Tests Passed ===\n");
    return 0;
}
