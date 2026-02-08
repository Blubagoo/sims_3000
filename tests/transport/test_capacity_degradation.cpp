/**
 * @file test_capacity_degradation.cpp
 * @brief Unit tests for CapacityDegradation (Epic 7, Ticket E7-026)
 *
 * Tests:
 * - Capacity scales linearly with health
 * - Zero capacity at zero health
 * - Full capacity at health=255
 * - Intermediate health values
 * - Edge cases: zero base_capacity, small values
 */

#include <sims3000/transport/CapacityDegradation.h>
#include <cassert>
#include <cstdio>

using namespace sims3000::transport;

void test_full_health_full_capacity() {
    printf("Testing full health gives full capacity...\n");

    RoadComponent road{};
    road.base_capacity = 1000;
    road.health = 255;

    update_capacity_from_health(road);

    assert(road.current_capacity == 1000);

    printf("  PASS: health=255 -> current_capacity == base_capacity\n");
}

void test_zero_health_zero_capacity() {
    printf("Testing zero health gives zero capacity...\n");

    RoadComponent road{};
    road.base_capacity = 1000;
    road.health = 0;

    update_capacity_from_health(road);

    assert(road.current_capacity == 0);

    printf("  PASS: health=0 -> current_capacity == 0\n");
}

void test_half_health_half_capacity() {
    printf("Testing half health gives approximately half capacity...\n");

    RoadComponent road{};
    road.base_capacity = 1000;
    road.health = 128;  // ~50.2% of 255

    update_capacity_from_health(road);

    // (1000 * 128) / 255 = 501 (integer division)
    uint16_t expected = static_cast<uint16_t>((1000u * 128u) / 255u);
    assert(road.current_capacity == expected);

    printf("  PASS: health=128 -> current_capacity == %u\n", road.current_capacity);
}

void test_quarter_health() {
    printf("Testing quarter health...\n");

    RoadComponent road{};
    road.base_capacity = 1000;
    road.health = 64;  // ~25.1% of 255

    update_capacity_from_health(road);

    uint16_t expected = static_cast<uint16_t>((1000u * 64u) / 255u);
    assert(road.current_capacity == expected);

    printf("  PASS: health=64 -> current_capacity == %u\n", road.current_capacity);
}

void test_three_quarter_health() {
    printf("Testing three-quarter health...\n");

    RoadComponent road{};
    road.base_capacity = 1000;
    road.health = 192;  // ~75.3% of 255

    update_capacity_from_health(road);

    uint16_t expected = static_cast<uint16_t>((1000u * 192u) / 255u);
    assert(road.current_capacity == expected);

    printf("  PASS: health=192 -> current_capacity == %u\n", road.current_capacity);
}

void test_zero_base_capacity() {
    printf("Testing zero base capacity...\n");

    RoadComponent road{};
    road.base_capacity = 0;
    road.health = 255;

    update_capacity_from_health(road);

    assert(road.current_capacity == 0);

    printf("  PASS: base_capacity=0 -> current_capacity == 0 regardless of health\n");
}

void test_health_one() {
    printf("Testing health=1 (minimal)...\n");

    RoadComponent road{};
    road.base_capacity = 1000;
    road.health = 1;

    update_capacity_from_health(road);

    // (1000 * 1) / 255 = 3 (integer division)
    uint16_t expected = static_cast<uint16_t>((1000u * 1u) / 255u);
    assert(road.current_capacity == expected);

    printf("  PASS: health=1 -> current_capacity == %u\n", road.current_capacity);
}

void test_health_254() {
    printf("Testing health=254 (nearly full)...\n");

    RoadComponent road{};
    road.base_capacity = 1000;
    road.health = 254;

    update_capacity_from_health(road);

    // (1000 * 254) / 255 = 996
    uint16_t expected = static_cast<uint16_t>((1000u * 254u) / 255u);
    assert(road.current_capacity == expected);

    printf("  PASS: health=254 -> current_capacity == %u\n", road.current_capacity);
}

void test_small_base_capacity() {
    printf("Testing small base capacity...\n");

    RoadComponent road{};
    road.base_capacity = 10;
    road.health = 128;

    update_capacity_from_health(road);

    uint16_t expected = static_cast<uint16_t>((10u * 128u) / 255u);
    assert(road.current_capacity == expected);

    printf("  PASS: base_capacity=10, health=128 -> current_capacity == %u\n", road.current_capacity);
}

void test_max_base_capacity() {
    printf("Testing max base capacity (65535)...\n");

    RoadComponent road{};
    road.base_capacity = 65535;
    road.health = 255;

    update_capacity_from_health(road);

    assert(road.current_capacity == 65535);

    printf("  PASS: base_capacity=65535, health=255 -> current_capacity == 65535\n");
}

void test_linear_scaling_monotonic() {
    printf("Testing capacity is monotonically increasing with health...\n");

    RoadComponent road{};
    road.base_capacity = 500;

    uint16_t prev_capacity = 0;
    for (int h = 0; h <= 255; ++h) {
        road.health = static_cast<uint8_t>(h);
        update_capacity_from_health(road);

        assert(road.current_capacity >= prev_capacity);
        prev_capacity = road.current_capacity;
    }

    printf("  PASS: Capacity is monotonically non-decreasing with health\n");
}

int main() {
    printf("=== CapacityDegradation Unit Tests (Epic 7, Ticket E7-026) ===\n\n");

    test_full_health_full_capacity();
    test_zero_health_zero_capacity();
    test_half_health_half_capacity();
    test_quarter_health();
    test_three_quarter_health();
    test_zero_base_capacity();
    test_health_one();
    test_health_254();
    test_small_base_capacity();
    test_max_base_capacity();
    test_linear_scaling_monotonic();

    printf("\n=== All CapacityDegradation Tests Passed ===\n");
    return 0;
}
