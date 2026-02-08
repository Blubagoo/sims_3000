/**
 * @file test_port_serialization.cpp
 * @brief Tests for port component serialization/deserialization
 *        (Epic 8, ticket E8-002)
 *
 * Tests cover:
 * - PortComponent round-trip serialization (field-by-field LE)
 * - PortComponent serialized size is 12 bytes
 * - Buffer overflow protection
 * - Version byte validation
 * - Little-endian encoding verification
 * - Default values round-trip
 * - Max values round-trip
 * - Multiple components in buffer
 * - Padding not serialized
 */

#include <sims3000/port/PortSerialization.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

using namespace sims3000::port;

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
// PortComponent Serialization Tests (Ticket E8-002)
// ============================================================================

TEST(port_component_serialized_size) {
    ASSERT_EQ(PORT_COMPONENT_SERIALIZED_SIZE, 12u);
}

TEST(port_component_round_trip_defaults) {
    PortComponent original;

    std::vector<uint8_t> buffer;
    serialize_port_component(original, buffer);

    ASSERT_EQ(buffer.size(), PORT_COMPONENT_SERIALIZED_SIZE);
    ASSERT_EQ(buffer[0], PORT_SERIALIZATION_VERSION);

    PortComponent deserialized;
    size_t consumed = deserialize_port_component(buffer.data(), buffer.size(), deserialized);

    ASSERT_EQ(consumed, PORT_COMPONENT_SERIALIZED_SIZE);
    ASSERT_EQ(static_cast<uint8_t>(deserialized.port_type), static_cast<uint8_t>(PortType::Aero));
    ASSERT_EQ(deserialized.capacity, 0u);
    ASSERT_EQ(deserialized.max_capacity, 0u);
    ASSERT_EQ(deserialized.utilization, 0u);
    ASSERT_EQ(deserialized.infrastructure_level, 0u);
    ASSERT_EQ(deserialized.is_operational, false);
    ASSERT_EQ(deserialized.is_connected_to_edge, false);
    ASSERT_EQ(deserialized.demand_bonus_radius, 0u);
    ASSERT_EQ(deserialized.connection_flags, 0u);
}

TEST(port_component_round_trip_custom_values) {
    PortComponent original;
    original.port_type = PortType::Aqua;
    original.capacity = 3500;
    original.max_capacity = 5000;
    original.utilization = 85;
    original.infrastructure_level = 3;
    original.is_operational = true;
    original.is_connected_to_edge = true;
    original.demand_bonus_radius = 12;
    original.connection_flags = 0x07;

    std::vector<uint8_t> buffer;
    serialize_port_component(original, buffer);

    ASSERT_EQ(buffer.size(), PORT_COMPONENT_SERIALIZED_SIZE);

    PortComponent deserialized;
    size_t consumed = deserialize_port_component(buffer.data(), buffer.size(), deserialized);

    ASSERT_EQ(consumed, PORT_COMPONENT_SERIALIZED_SIZE);
    ASSERT_EQ(static_cast<uint8_t>(deserialized.port_type), static_cast<uint8_t>(PortType::Aqua));
    ASSERT_EQ(deserialized.capacity, 3500u);
    ASSERT_EQ(deserialized.max_capacity, 5000u);
    ASSERT_EQ(deserialized.utilization, 85u);
    ASSERT_EQ(deserialized.infrastructure_level, 3u);
    ASSERT_EQ(deserialized.is_operational, true);
    ASSERT_EQ(deserialized.is_connected_to_edge, true);
    ASSERT_EQ(deserialized.demand_bonus_radius, 12u);
    ASSERT_EQ(deserialized.connection_flags, 0x07u);
}

TEST(port_component_max_values) {
    PortComponent original;
    original.port_type = PortType::Aqua;
    original.capacity = UINT16_MAX;
    original.max_capacity = UINT16_MAX;
    original.utilization = 255;
    original.infrastructure_level = 255;
    original.is_operational = true;
    original.is_connected_to_edge = true;
    original.demand_bonus_radius = 255;
    original.connection_flags = 255;

    std::vector<uint8_t> buffer;
    serialize_port_component(original, buffer);

    PortComponent deserialized;
    deserialize_port_component(buffer.data(), buffer.size(), deserialized);

    ASSERT_EQ(deserialized.capacity, UINT16_MAX);
    ASSERT_EQ(deserialized.max_capacity, UINT16_MAX);
    ASSERT_EQ(deserialized.utilization, 255u);
    ASSERT_EQ(deserialized.infrastructure_level, 255u);
    ASSERT_EQ(deserialized.is_operational, true);
    ASSERT_EQ(deserialized.is_connected_to_edge, true);
    ASSERT_EQ(deserialized.demand_bonus_radius, 255u);
    ASSERT_EQ(deserialized.connection_flags, 255u);
}

TEST(port_component_all_port_types) {
    PortType types[] = {
        PortType::Aero,
        PortType::Aqua
    };

    for (auto type : types) {
        PortComponent original;
        original.port_type = type;

        std::vector<uint8_t> buffer;
        serialize_port_component(original, buffer);

        PortComponent deserialized;
        deserialize_port_component(buffer.data(), buffer.size(), deserialized);

        ASSERT_EQ(static_cast<uint8_t>(deserialized.port_type), static_cast<uint8_t>(type));
    }
}

TEST(port_component_buffer_too_small) {
    uint8_t small_buf[8] = {};
    PortComponent comp;
    ASSERT_THROW(deserialize_port_component(small_buf, 8, comp), std::runtime_error);
}

TEST(port_component_version_validation) {
    PortComponent original;
    original.capacity = 500;

    std::vector<uint8_t> buffer;
    serialize_port_component(original, buffer);

    // Corrupt version byte
    buffer[0] = 99;

    PortComponent deserialized;
    ASSERT_THROW(deserialize_port_component(buffer.data(), buffer.size(), deserialized), std::runtime_error);
}

TEST(port_component_little_endian_encoding) {
    PortComponent original;
    original.capacity = 0x1234;
    original.max_capacity = 0xABCD;

    std::vector<uint8_t> buffer;
    serialize_port_component(original, buffer);

    // capacity starts at offset 2 (1 version + 1 port_type)
    ASSERT_EQ(buffer[2], 0x34); // LSB
    ASSERT_EQ(buffer[3], 0x12); // MSB

    // max_capacity starts at offset 4 (1 version + 1 port_type + 2 capacity)
    ASSERT_EQ(buffer[4], 0xCD); // LSB
    ASSERT_EQ(buffer[5], 0xAB); // MSB
}

TEST(port_component_padding_not_serialized) {
    PortComponent original;
    original.capacity = 100;
    original.padding[0] = 0xAA;
    original.padding[1] = 0xBB;
    original.padding[2] = 0xCC;
    original.padding[3] = 0xDD;

    std::vector<uint8_t> buffer;
    serialize_port_component(original, buffer);

    // Padding should NOT be in the buffer (12 bytes, not 16)
    ASSERT_EQ(buffer.size(), 12u);

    PortComponent deserialized;
    deserialize_port_component(buffer.data(), buffer.size(), deserialized);

    // Padding should be zeroed on deserialization
    ASSERT_EQ(deserialized.padding[0], 0u);
    ASSERT_EQ(deserialized.padding[1], 0u);
    ASSERT_EQ(deserialized.padding[2], 0u);
    ASSERT_EQ(deserialized.padding[3], 0u);

    // Actual data preserved
    ASSERT_EQ(deserialized.capacity, 100u);
}

TEST(port_component_multiple_in_buffer) {
    PortComponent comp1;
    comp1.port_type = PortType::Aero;
    comp1.capacity = 100;
    comp1.is_operational = true;

    PortComponent comp2;
    comp2.port_type = PortType::Aqua;
    comp2.capacity = 200;
    comp2.is_operational = false;

    std::vector<uint8_t> buffer;
    serialize_port_component(comp1, buffer);
    serialize_port_component(comp2, buffer);

    ASSERT_EQ(buffer.size(), 24u); // 12 + 12

    PortComponent out1, out2;
    size_t consumed1 = deserialize_port_component(buffer.data(), buffer.size(), out1);
    ASSERT_EQ(consumed1, 12u);

    size_t consumed2 = deserialize_port_component(buffer.data() + consumed1,
                                                    buffer.size() - consumed1, out2);
    ASSERT_EQ(consumed2, 12u);

    ASSERT_EQ(static_cast<uint8_t>(out1.port_type), static_cast<uint8_t>(PortType::Aero));
    ASSERT_EQ(out1.capacity, 100u);
    ASSERT_EQ(out1.is_operational, true);
    ASSERT_EQ(static_cast<uint8_t>(out2.port_type), static_cast<uint8_t>(PortType::Aqua));
    ASSERT_EQ(out2.capacity, 200u);
    ASSERT_EQ(out2.is_operational, false);
}

TEST(port_component_bool_encoding) {
    // Test that booleans are encoded as 0/1 bytes
    PortComponent original;
    original.is_operational = true;
    original.is_connected_to_edge = false;

    std::vector<uint8_t> buffer;
    serialize_port_component(original, buffer);

    // is_operational at offset 8 (1 version + 1 port_type + 2 capacity + 2 max_capacity + 1 utilization + 1 infra = 8)
    ASSERT_EQ(buffer[8], 1u);
    // is_connected_to_edge at offset 9
    ASSERT_EQ(buffer[9], 0u);

    // Swap values
    buffer.clear();
    original.is_operational = false;
    original.is_connected_to_edge = true;
    serialize_port_component(original, buffer);

    ASSERT_EQ(buffer[8], 0u);
    ASSERT_EQ(buffer[9], 1u);
}

// ============================================================================
// Main
// ============================================================================

int main() {
    printf("=== PortSerialization Unit Tests (Epic 8, Ticket E8-002) ===\n\n");

    RUN_TEST(port_component_serialized_size);
    RUN_TEST(port_component_round_trip_defaults);
    RUN_TEST(port_component_round_trip_custom_values);
    RUN_TEST(port_component_max_values);
    RUN_TEST(port_component_all_port_types);
    RUN_TEST(port_component_buffer_too_small);
    RUN_TEST(port_component_version_validation);
    RUN_TEST(port_component_little_endian_encoding);
    RUN_TEST(port_component_padding_not_serialized);
    RUN_TEST(port_component_multiple_in_buffer);
    RUN_TEST(port_component_bool_encoding);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
