/**
 * @file test_road_component.cpp
 * @brief Unit tests for RoadComponent (Epic 7, Ticket E7-002)
 */

#include <sims3000/transport/RoadComponent.h>
#include <cassert>
#include <cstdio>

using namespace sims3000::transport;

void test_road_component_size() {
    printf("Testing RoadComponent size...\n");

    assert(sizeof(RoadComponent) == 16);

    printf("  PASS: RoadComponent is 16 bytes\n");
}

void test_road_trivially_copyable() {
    printf("Testing RoadComponent is trivially copyable...\n");

    assert(std::is_trivially_copyable<RoadComponent>::value);

    printf("  PASS: RoadComponent is trivially copyable\n");
}

void test_road_default_initialization() {
    printf("Testing default initialization...\n");

    RoadComponent road{};
    assert(road.type == PathwayType::BasicPathway);
    assert(road.direction == PathwayDirection::Bidirectional);
    assert(road.base_capacity == 100);
    assert(road.current_capacity == 100);
    assert(road.health == 255);
    assert(road.decay_rate == 1);
    assert(road.connection_mask == 0);
    assert(road.is_junction == false);
    assert(road.network_id == 0);
    assert(road.last_maintained_tick == 0);

    printf("  PASS: Default initialization works correctly\n");
}

void test_road_custom_values() {
    printf("Testing custom value assignment...\n");

    RoadComponent road{};
    road.type = PathwayType::TransitCorridor;
    road.direction = PathwayDirection::OneWayNorth;
    road.base_capacity = 500;
    road.current_capacity = 400;
    road.health = 200;
    road.decay_rate = 3;
    road.connection_mask = 0x0F;  // All directions connected
    road.is_junction = true;
    road.network_id = 42;
    road.last_maintained_tick = 1000;

    assert(road.type == PathwayType::TransitCorridor);
    assert(road.direction == PathwayDirection::OneWayNorth);
    assert(road.base_capacity == 500);
    assert(road.current_capacity == 400);
    assert(road.health == 200);
    assert(road.decay_rate == 3);
    assert(road.connection_mask == 0x0F);
    assert(road.is_junction == true);
    assert(road.network_id == 42);
    assert(road.last_maintained_tick == 1000);

    printf("  PASS: Custom value assignment works correctly\n");
}

void test_road_connection_mask_bits() {
    printf("Testing connection mask bits...\n");

    RoadComponent road{};

    // North bit (1)
    road.connection_mask = 1;
    assert((road.connection_mask & 1) != 0);   // N set
    assert((road.connection_mask & 2) == 0);   // S not set
    assert((road.connection_mask & 4) == 0);   // E not set
    assert((road.connection_mask & 8) == 0);   // W not set

    // South bit (2)
    road.connection_mask = 2;
    assert((road.connection_mask & 1) == 0);
    assert((road.connection_mask & 2) != 0);

    // East bit (4)
    road.connection_mask = 4;
    assert((road.connection_mask & 4) != 0);

    // West bit (8)
    road.connection_mask = 8;
    assert((road.connection_mask & 8) != 0);

    // Combine N+S+E+W
    road.connection_mask = 1 | 2 | 4 | 8;
    assert(road.connection_mask == 0x0F);

    printf("  PASS: Connection mask bits work correctly\n");
}

void test_road_pathway_types() {
    printf("Testing all pathway types can be assigned...\n");

    RoadComponent road{};

    road.type = PathwayType::BasicPathway;
    assert(road.type == PathwayType::BasicPathway);

    road.type = PathwayType::TransitCorridor;
    assert(road.type == PathwayType::TransitCorridor);

    road.type = PathwayType::Pedestrian;
    assert(road.type == PathwayType::Pedestrian);

    road.type = PathwayType::Bridge;
    assert(road.type == PathwayType::Bridge);

    road.type = PathwayType::Tunnel;
    assert(road.type == PathwayType::Tunnel);

    printf("  PASS: All pathway types can be assigned\n");
}

void test_road_direction_modes() {
    printf("Testing all direction modes can be assigned...\n");

    RoadComponent road{};

    road.direction = PathwayDirection::Bidirectional;
    assert(road.direction == PathwayDirection::Bidirectional);

    road.direction = PathwayDirection::OneWayNorth;
    assert(road.direction == PathwayDirection::OneWayNorth);

    road.direction = PathwayDirection::OneWaySouth;
    assert(road.direction == PathwayDirection::OneWaySouth);

    road.direction = PathwayDirection::OneWayEast;
    assert(road.direction == PathwayDirection::OneWayEast);

    road.direction = PathwayDirection::OneWayWest;
    assert(road.direction == PathwayDirection::OneWayWest);

    printf("  PASS: All direction modes can be assigned\n");
}

void test_road_copy() {
    printf("Testing copy semantics...\n");

    RoadComponent original{};
    original.type = PathwayType::Bridge;
    original.direction = PathwayDirection::OneWayEast;
    original.base_capacity = 300;
    original.current_capacity = 250;
    original.health = 128;
    original.decay_rate = 5;
    original.connection_mask = 0x05;  // N+E
    original.is_junction = true;
    original.network_id = 99;
    original.last_maintained_tick = 5000;

    RoadComponent copy = original;
    assert(copy.type == PathwayType::Bridge);
    assert(copy.direction == PathwayDirection::OneWayEast);
    assert(copy.base_capacity == 300);
    assert(copy.current_capacity == 250);
    assert(copy.health == 128);
    assert(copy.decay_rate == 5);
    assert(copy.connection_mask == 0x05);
    assert(copy.is_junction == true);
    assert(copy.network_id == 99);
    assert(copy.last_maintained_tick == 5000);

    printf("  PASS: Copy semantics work correctly\n");
}

void test_road_health_decay() {
    printf("Testing health and decay rate...\n");

    RoadComponent road{};
    assert(road.health == 255);  // pristine
    assert(road.decay_rate == 1);

    // Simulate decay
    road.health = static_cast<uint8_t>(road.health - road.decay_rate);
    assert(road.health == 254);

    // High decay rate
    road.decay_rate = 10;
    road.health = static_cast<uint8_t>(road.health - road.decay_rate);
    assert(road.health == 244);

    // Zero health (destroyed)
    road.health = 0;
    assert(road.health == 0);

    printf("  PASS: Health and decay rate work correctly\n");
}

int main() {
    printf("=== RoadComponent Unit Tests (Epic 7, Ticket E7-002) ===\n\n");

    test_road_component_size();
    test_road_trivially_copyable();
    test_road_default_initialization();
    test_road_custom_values();
    test_road_connection_mask_bits();
    test_road_pathway_types();
    test_road_direction_modes();
    test_road_copy();
    test_road_health_decay();

    printf("\n=== All RoadComponent Tests Passed ===\n");
    return 0;
}
