/**
 * @file test_transport_serialization.cpp
 * @brief Tests for transport component serialization/deserialization
 *        (Epic 7, tickets E7-036, E7-037)
 *
 * Tests cover:
 * - RoadComponent round-trip serialization (field-by-field LE)
 * - RoadComponent serialized size is 17 bytes
 * - TrafficComponent round-trip serialization (field-by-field LE)
 * - TrafficComponent serialized size is 14 bytes (padding skipped)
 * - Buffer overflow protection
 * - Version byte validation
 * - Little-endian encoding verification
 * - Default values round-trip
 * - Max values round-trip
 * - Multiple components in buffer
 */

#include <sims3000/transport/TransportSerialization.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

using namespace sims3000::transport;

// Test result tracking
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) void test_##name()
#define RUN_TEST(name) do { \
    printf("Running %s...", #name); \
    test_##name(); \
    printf(" PASSED\n"); \
    tests_passed++; \
} while(0)

#define ASSERT(condition) do { \
    if (!(condition)) { \
        printf("\n  FAILED: %s (line %d)\n", #condition, __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

#define ASSERT_EQ(a, b) do { \
    if ((a) != (b)) { \
        printf("\n  FAILED: %s == %s (line %d)\n", #a, #b, __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

#define ASSERT_THROW(expr, exception_type) do { \
    bool caught = false; \
    try { expr; } catch (const exception_type&) { caught = true; } \
    if (!caught) { \
        printf("\n  FAILED: expected exception at line %d\n", __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

// ============================================================================
// RoadComponent Serialization Tests (Ticket E7-036)
// ============================================================================

TEST(road_component_serialized_size) {
    ASSERT_EQ(ROAD_COMPONENT_SERIALIZED_SIZE, 17u);
}

TEST(road_component_round_trip_defaults) {
    RoadComponent original;

    std::vector<uint8_t> buffer;
    serialize_road_component(original, buffer);

    ASSERT_EQ(buffer.size(), ROAD_COMPONENT_SERIALIZED_SIZE);
    ASSERT_EQ(buffer[0], TRANSPORT_SERIALIZATION_VERSION);

    RoadComponent deserialized;
    size_t consumed = deserialize_road_component(buffer.data(), buffer.size(), deserialized);

    ASSERT_EQ(consumed, ROAD_COMPONENT_SERIALIZED_SIZE);
    ASSERT_EQ(static_cast<uint8_t>(deserialized.type), static_cast<uint8_t>(PathwayType::BasicPathway));
    ASSERT_EQ(static_cast<uint8_t>(deserialized.direction), static_cast<uint8_t>(PathwayDirection::Bidirectional));
    ASSERT_EQ(deserialized.base_capacity, 100u);
    ASSERT_EQ(deserialized.current_capacity, 100u);
    ASSERT_EQ(deserialized.health, 255u);
    ASSERT_EQ(deserialized.decay_rate, 1u);
    ASSERT_EQ(deserialized.connection_mask, 0u);
    ASSERT_EQ(deserialized.is_junction, false);
    ASSERT_EQ(deserialized.network_id, 0u);
    ASSERT_EQ(deserialized.last_maintained_tick, 0u);
}

TEST(road_component_round_trip_custom_values) {
    RoadComponent original;
    original.type = PathwayType::TransitCorridor;
    original.direction = PathwayDirection::OneWayEast;
    original.base_capacity = 500;
    original.current_capacity = 450;
    original.health = 200;
    original.decay_rate = 5;
    original.connection_mask = 0x0F;  // All directions
    original.is_junction = true;
    original.network_id = 42;
    original.last_maintained_tick = 123456;

    std::vector<uint8_t> buffer;
    serialize_road_component(original, buffer);

    ASSERT_EQ(buffer.size(), ROAD_COMPONENT_SERIALIZED_SIZE);

    RoadComponent deserialized;
    size_t consumed = deserialize_road_component(buffer.data(), buffer.size(), deserialized);

    ASSERT_EQ(consumed, ROAD_COMPONENT_SERIALIZED_SIZE);
    ASSERT_EQ(static_cast<uint8_t>(deserialized.type), static_cast<uint8_t>(PathwayType::TransitCorridor));
    ASSERT_EQ(static_cast<uint8_t>(deserialized.direction), static_cast<uint8_t>(PathwayDirection::OneWayEast));
    ASSERT_EQ(deserialized.base_capacity, 500u);
    ASSERT_EQ(deserialized.current_capacity, 450u);
    ASSERT_EQ(deserialized.health, 200u);
    ASSERT_EQ(deserialized.decay_rate, 5u);
    ASSERT_EQ(deserialized.connection_mask, 0x0Fu);
    ASSERT_EQ(deserialized.is_junction, true);
    ASSERT_EQ(deserialized.network_id, 42u);
    ASSERT_EQ(deserialized.last_maintained_tick, 123456u);
}

TEST(road_component_max_values) {
    RoadComponent original;
    original.type = PathwayType::Tunnel;
    original.direction = PathwayDirection::OneWayWest;
    original.base_capacity = UINT16_MAX;
    original.current_capacity = UINT16_MAX;
    original.health = 255;
    original.decay_rate = 255;
    original.connection_mask = 255;
    original.is_junction = true;
    original.network_id = UINT16_MAX;
    original.last_maintained_tick = UINT32_MAX;

    std::vector<uint8_t> buffer;
    serialize_road_component(original, buffer);

    RoadComponent deserialized;
    deserialize_road_component(buffer.data(), buffer.size(), deserialized);

    ASSERT_EQ(deserialized.base_capacity, UINT16_MAX);
    ASSERT_EQ(deserialized.current_capacity, UINT16_MAX);
    ASSERT_EQ(deserialized.health, 255u);
    ASSERT_EQ(deserialized.decay_rate, 255u);
    ASSERT_EQ(deserialized.connection_mask, 255u);
    ASSERT_EQ(deserialized.is_junction, true);
    ASSERT_EQ(deserialized.network_id, UINT16_MAX);
    ASSERT_EQ(deserialized.last_maintained_tick, UINT32_MAX);
}

TEST(road_component_all_pathway_types) {
    PathwayType types[] = {
        PathwayType::BasicPathway,
        PathwayType::TransitCorridor,
        PathwayType::Pedestrian,
        PathwayType::Bridge,
        PathwayType::Tunnel
    };

    for (auto type : types) {
        RoadComponent original;
        original.type = type;

        std::vector<uint8_t> buffer;
        serialize_road_component(original, buffer);

        RoadComponent deserialized;
        deserialize_road_component(buffer.data(), buffer.size(), deserialized);

        ASSERT_EQ(static_cast<uint8_t>(deserialized.type), static_cast<uint8_t>(type));
    }
}

TEST(road_component_all_directions) {
    PathwayDirection dirs[] = {
        PathwayDirection::Bidirectional,
        PathwayDirection::OneWayNorth,
        PathwayDirection::OneWaySouth,
        PathwayDirection::OneWayEast,
        PathwayDirection::OneWayWest
    };

    for (auto dir : dirs) {
        RoadComponent original;
        original.direction = dir;

        std::vector<uint8_t> buffer;
        serialize_road_component(original, buffer);

        RoadComponent deserialized;
        deserialize_road_component(buffer.data(), buffer.size(), deserialized);

        ASSERT_EQ(static_cast<uint8_t>(deserialized.direction), static_cast<uint8_t>(dir));
    }
}

TEST(road_component_buffer_too_small) {
    uint8_t small_buf[10] = {};
    RoadComponent comp;
    ASSERT_THROW(deserialize_road_component(small_buf, 10, comp), std::runtime_error);
}

TEST(road_component_version_validation) {
    RoadComponent original;
    original.base_capacity = 500;

    std::vector<uint8_t> buffer;
    serialize_road_component(original, buffer);

    // Corrupt version byte
    buffer[0] = 99;

    RoadComponent deserialized;
    ASSERT_THROW(deserialize_road_component(buffer.data(), buffer.size(), deserialized), std::runtime_error);
}

TEST(road_component_little_endian_encoding) {
    RoadComponent original;
    original.base_capacity = 0x1234;
    original.last_maintained_tick = 0xAABBCCDD;

    std::vector<uint8_t> buffer;
    serialize_road_component(original, buffer);

    // base_capacity starts at offset 3 (1 version + 1 type + 1 direction)
    ASSERT_EQ(buffer[3], 0x34); // LSB
    ASSERT_EQ(buffer[4], 0x12); // MSB

    // last_maintained_tick starts at offset 13 (1+1+1+2+2+1+1+1+1+2 = 13)
    ASSERT_EQ(buffer[13], 0xDD); // LSB
    ASSERT_EQ(buffer[14], 0xCC);
    ASSERT_EQ(buffer[15], 0xBB);
    ASSERT_EQ(buffer[16], 0xAA); // MSB
}

TEST(road_component_multiple_in_buffer) {
    RoadComponent comp1;
    comp1.base_capacity = 100;
    comp1.is_junction = true;

    RoadComponent comp2;
    comp2.base_capacity = 200;
    comp2.is_junction = false;

    std::vector<uint8_t> buffer;
    serialize_road_component(comp1, buffer);
    serialize_road_component(comp2, buffer);

    ASSERT_EQ(buffer.size(), 34u); // 17 + 17

    RoadComponent out1, out2;
    size_t consumed1 = deserialize_road_component(buffer.data(), buffer.size(), out1);
    ASSERT_EQ(consumed1, 17u);

    size_t consumed2 = deserialize_road_component(buffer.data() + consumed1,
                                                    buffer.size() - consumed1, out2);
    ASSERT_EQ(consumed2, 17u);

    ASSERT_EQ(out1.base_capacity, 100u);
    ASSERT_EQ(out1.is_junction, true);
    ASSERT_EQ(out2.base_capacity, 200u);
    ASSERT_EQ(out2.is_junction, false);
}

// ============================================================================
// TrafficComponent Serialization Tests (Ticket E7-037)
// ============================================================================

TEST(traffic_component_serialized_size) {
    ASSERT_EQ(TRAFFIC_COMPONENT_SERIALIZED_SIZE, 14u);
}

TEST(traffic_component_round_trip_defaults) {
    TrafficComponent original;

    std::vector<uint8_t> buffer;
    serialize_traffic_component(original, buffer);

    ASSERT_EQ(buffer.size(), TRAFFIC_COMPONENT_SERIALIZED_SIZE);
    ASSERT_EQ(buffer[0], TRANSPORT_SERIALIZATION_VERSION);

    TrafficComponent deserialized;
    size_t consumed = deserialize_traffic_component(buffer.data(), buffer.size(), deserialized);

    ASSERT_EQ(consumed, TRAFFIC_COMPONENT_SERIALIZED_SIZE);
    ASSERT_EQ(deserialized.flow_current, 0u);
    ASSERT_EQ(deserialized.flow_previous, 0u);
    ASSERT_EQ(deserialized.flow_sources, 0u);
    ASSERT_EQ(deserialized.congestion_level, 0u);
    ASSERT_EQ(deserialized.flow_blockage_ticks, 0u);
    ASSERT_EQ(deserialized.contamination_rate, 0u);
}

TEST(traffic_component_round_trip_custom_values) {
    TrafficComponent original;
    original.flow_current = 5000;
    original.flow_previous = 4800;
    original.flow_sources = 12;
    original.congestion_level = 180;
    original.flow_blockage_ticks = 5;
    original.contamination_rate = 30;

    std::vector<uint8_t> buffer;
    serialize_traffic_component(original, buffer);

    ASSERT_EQ(buffer.size(), TRAFFIC_COMPONENT_SERIALIZED_SIZE);

    TrafficComponent deserialized;
    size_t consumed = deserialize_traffic_component(buffer.data(), buffer.size(), deserialized);

    ASSERT_EQ(consumed, TRAFFIC_COMPONENT_SERIALIZED_SIZE);
    ASSERT_EQ(deserialized.flow_current, 5000u);
    ASSERT_EQ(deserialized.flow_previous, 4800u);
    ASSERT_EQ(deserialized.flow_sources, 12u);
    ASSERT_EQ(deserialized.congestion_level, 180u);
    ASSERT_EQ(deserialized.flow_blockage_ticks, 5u);
    ASSERT_EQ(deserialized.contamination_rate, 30u);
}

TEST(traffic_component_max_values) {
    TrafficComponent original;
    original.flow_current = UINT32_MAX;
    original.flow_previous = UINT32_MAX;
    original.flow_sources = UINT16_MAX;
    original.congestion_level = 255;
    original.flow_blockage_ticks = 255;
    original.contamination_rate = 255;

    std::vector<uint8_t> buffer;
    serialize_traffic_component(original, buffer);

    TrafficComponent deserialized;
    deserialize_traffic_component(buffer.data(), buffer.size(), deserialized);

    ASSERT_EQ(deserialized.flow_current, UINT32_MAX);
    ASSERT_EQ(deserialized.flow_previous, UINT32_MAX);
    ASSERT_EQ(deserialized.flow_sources, UINT16_MAX);
    ASSERT_EQ(deserialized.congestion_level, 255u);
    ASSERT_EQ(deserialized.flow_blockage_ticks, 255u);
    ASSERT_EQ(deserialized.contamination_rate, 255u);
}

TEST(traffic_component_padding_zeroed) {
    TrafficComponent original;
    original.flow_current = 100;
    original.padding[0] = 0xAA;
    original.padding[1] = 0xBB;
    original.padding[2] = 0xCC;

    std::vector<uint8_t> buffer;
    serialize_traffic_component(original, buffer);

    // Padding should NOT be in the buffer (14 bytes, not 17)
    ASSERT_EQ(buffer.size(), 14u);

    TrafficComponent deserialized;
    deserialize_traffic_component(buffer.data(), buffer.size(), deserialized);

    // Padding should be zeroed on deserialization
    ASSERT_EQ(deserialized.padding[0], 0u);
    ASSERT_EQ(deserialized.padding[1], 0u);
    ASSERT_EQ(deserialized.padding[2], 0u);

    // Actual data preserved
    ASSERT_EQ(deserialized.flow_current, 100u);
}

TEST(traffic_component_buffer_too_small) {
    uint8_t small_buf[8] = {};
    TrafficComponent comp;
    ASSERT_THROW(deserialize_traffic_component(small_buf, 8, comp), std::runtime_error);
}

TEST(traffic_component_version_validation) {
    TrafficComponent original;
    original.flow_current = 500;

    std::vector<uint8_t> buffer;
    serialize_traffic_component(original, buffer);

    // Corrupt version byte
    buffer[0] = 99;

    TrafficComponent deserialized;
    ASSERT_THROW(deserialize_traffic_component(buffer.data(), buffer.size(), deserialized), std::runtime_error);
}

TEST(traffic_component_little_endian_encoding) {
    TrafficComponent original;
    original.flow_current = 0x12345678;

    std::vector<uint8_t> buffer;
    serialize_traffic_component(original, buffer);

    // flow_current starts at offset 1 (after version byte)
    ASSERT_EQ(buffer[1], 0x78); // LSB
    ASSERT_EQ(buffer[2], 0x56);
    ASSERT_EQ(buffer[3], 0x34);
    ASSERT_EQ(buffer[4], 0x12); // MSB
}

TEST(traffic_component_multiple_in_buffer) {
    TrafficComponent comp1;
    comp1.flow_current = 100;
    comp1.congestion_level = 50;

    TrafficComponent comp2;
    comp2.flow_current = 200;
    comp2.congestion_level = 100;

    std::vector<uint8_t> buffer;
    serialize_traffic_component(comp1, buffer);
    serialize_traffic_component(comp2, buffer);

    ASSERT_EQ(buffer.size(), 28u); // 14 + 14

    TrafficComponent out1, out2;
    size_t consumed1 = deserialize_traffic_component(buffer.data(), buffer.size(), out1);
    ASSERT_EQ(consumed1, 14u);

    size_t consumed2 = deserialize_traffic_component(buffer.data() + consumed1,
                                                       buffer.size() - consumed1, out2);
    ASSERT_EQ(consumed2, 14u);

    ASSERT_EQ(out1.flow_current, 100u);
    ASSERT_EQ(out1.congestion_level, 50u);
    ASSERT_EQ(out2.flow_current, 200u);
    ASSERT_EQ(out2.congestion_level, 100u);
}

// ============================================================================
// Cross-component test
// ============================================================================

TEST(road_and_traffic_in_same_buffer) {
    RoadComponent road;
    road.base_capacity = 500;
    road.is_junction = true;

    TrafficComponent traffic;
    traffic.flow_current = 300;
    traffic.congestion_level = 120;

    std::vector<uint8_t> buffer;
    serialize_road_component(road, buffer);
    serialize_traffic_component(traffic, buffer);

    ASSERT_EQ(buffer.size(), 31u); // 17 + 14

    RoadComponent road_out;
    size_t consumed1 = deserialize_road_component(buffer.data(), buffer.size(), road_out);
    ASSERT_EQ(consumed1, 17u);

    TrafficComponent traffic_out;
    size_t consumed2 = deserialize_traffic_component(buffer.data() + consumed1,
                                                       buffer.size() - consumed1, traffic_out);
    ASSERT_EQ(consumed2, 14u);

    ASSERT_EQ(road_out.base_capacity, 500u);
    ASSERT_EQ(road_out.is_junction, true);
    ASSERT_EQ(traffic_out.flow_current, 300u);
    ASSERT_EQ(traffic_out.congestion_level, 120u);
}

// ============================================================================
// Main
// ============================================================================

int main() {
    printf("=== TransportSerialization Unit Tests (Tickets E7-036, E7-037) ===\n\n");

    // RoadComponent serialization
    RUN_TEST(road_component_serialized_size);
    RUN_TEST(road_component_round_trip_defaults);
    RUN_TEST(road_component_round_trip_custom_values);
    RUN_TEST(road_component_max_values);
    RUN_TEST(road_component_all_pathway_types);
    RUN_TEST(road_component_all_directions);
    RUN_TEST(road_component_buffer_too_small);
    RUN_TEST(road_component_version_validation);
    RUN_TEST(road_component_little_endian_encoding);
    RUN_TEST(road_component_multiple_in_buffer);

    // TrafficComponent serialization
    RUN_TEST(traffic_component_serialized_size);
    RUN_TEST(traffic_component_round_trip_defaults);
    RUN_TEST(traffic_component_round_trip_custom_values);
    RUN_TEST(traffic_component_max_values);
    RUN_TEST(traffic_component_padding_zeroed);
    RUN_TEST(traffic_component_buffer_too_small);
    RUN_TEST(traffic_component_version_validation);
    RUN_TEST(traffic_component_little_endian_encoding);
    RUN_TEST(traffic_component_multiple_in_buffer);

    // Cross-component
    RUN_TEST(road_and_traffic_in_same_buffer);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
