/**
 * @file test_traffic_component.cpp
 * @brief Unit tests for TrafficComponent (Epic 7, Ticket E7-003)
 *
 * Tests cover:
 * - Size verification (16 bytes)
 * - Trivially copyable for serialization
 * - Default initialization values
 * - Field assignment and read-back
 * - Copy semantics
 */

#include <sims3000/transport/TrafficComponent.h>
#include <cassert>
#include <cstdio>

using namespace sims3000::transport;

void test_traffic_component_size() {
    printf("Testing TrafficComponent size...\n");

    assert(sizeof(TrafficComponent) == 16);

    printf("  PASS: TrafficComponent is 16 bytes\n");
}

void test_traffic_component_trivially_copyable() {
    printf("Testing TrafficComponent is trivially copyable...\n");

    assert(std::is_trivially_copyable<TrafficComponent>::value);

    printf("  PASS: TrafficComponent is trivially copyable\n");
}

void test_traffic_component_default_initialization() {
    printf("Testing default initialization...\n");

    TrafficComponent tc{};
    assert(tc.flow_current == 0);
    assert(tc.flow_previous == 0);
    assert(tc.flow_sources == 0);
    assert(tc.congestion_level == 0);
    assert(tc.flow_blockage_ticks == 0);
    assert(tc.contamination_rate == 0);
    assert(tc.padding[0] == 0);
    assert(tc.padding[1] == 0);
    assert(tc.padding[2] == 0);

    printf("  PASS: Default initialization works correctly\n");
}

void test_traffic_component_field_assignment() {
    printf("Testing field assignment...\n");

    TrafficComponent tc{};

    tc.flow_current = 1000;
    assert(tc.flow_current == 1000);

    tc.flow_previous = 950;
    assert(tc.flow_previous == 950);

    tc.flow_sources = 12;
    assert(tc.flow_sources == 12);

    tc.congestion_level = 200;
    assert(tc.congestion_level == 200);

    tc.flow_blockage_ticks = 5;
    assert(tc.flow_blockage_ticks == 5);

    tc.contamination_rate = 30;
    assert(tc.contamination_rate == 30);

    printf("  PASS: Field assignment works correctly\n");
}

void test_traffic_component_max_values() {
    printf("Testing maximum field values...\n");

    TrafficComponent tc{};

    // uint32_t max
    tc.flow_current = 0xFFFFFFFF;
    assert(tc.flow_current == 0xFFFFFFFF);

    tc.flow_previous = 0xFFFFFFFF;
    assert(tc.flow_previous == 0xFFFFFFFF);

    // uint16_t max
    tc.flow_sources = 0xFFFF;
    assert(tc.flow_sources == 0xFFFF);

    // uint8_t max
    tc.congestion_level = 255;
    assert(tc.congestion_level == 255);

    tc.flow_blockage_ticks = 255;
    assert(tc.flow_blockage_ticks == 255);

    tc.contamination_rate = 255;
    assert(tc.contamination_rate == 255);

    printf("  PASS: Maximum field values correct\n");
}

void test_traffic_component_copy() {
    printf("Testing copy semantics...\n");

    TrafficComponent original{};
    original.flow_current = 500;
    original.flow_previous = 480;
    original.flow_sources = 8;
    original.congestion_level = 150;
    original.flow_blockage_ticks = 3;
    original.contamination_rate = 20;

    TrafficComponent copy = original;
    assert(copy.flow_current == 500);
    assert(copy.flow_previous == 480);
    assert(copy.flow_sources == 8);
    assert(copy.congestion_level == 150);
    assert(copy.flow_blockage_ticks == 3);
    assert(copy.contamination_rate == 20);

    printf("  PASS: Copy semantics work correctly\n");
}

void test_traffic_component_default_constructible() {
    printf("Testing default constructible...\n");

    assert(std::is_default_constructible<TrafficComponent>::value);

    printf("  PASS: TrafficComponent is default constructible\n");
}

int main() {
    printf("=== TrafficComponent Unit Tests (Epic 7, Ticket E7-003) ===\n\n");

    test_traffic_component_size();
    test_traffic_component_trivially_copyable();
    test_traffic_component_default_initialization();
    test_traffic_component_field_assignment();
    test_traffic_component_max_values();
    test_traffic_component_copy();
    test_traffic_component_default_constructible();

    printf("\n=== All TrafficComponent Tests Passed ===\n");
    return 0;
}
