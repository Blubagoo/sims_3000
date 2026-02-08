/**
 * @file test_fluid_serialization.cpp
 * @brief Tests for fluid serialization/deserialization
 *        (Epic 6, tickets 6-036, 6-037)
 *
 * Tests cover:
 * - FluidComponent round-trip serialization (memcpy path)
 * - Compact fluid-state bit packing (pack/unpack)
 * - FluidPoolSyncMessage round-trip
 * - FluidPoolSyncMessage serialized size is 22 bytes
 * - Buffer overflow protection
 * - Version byte validation
 */

#include <sims3000/fluid/FluidSerialization.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

using namespace sims3000::fluid;

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
// FluidComponent Serialization Tests (Ticket 6-036)
// ============================================================================

TEST(fluid_component_round_trip) {
    FluidComponent original;
    original.fluid_required = 500;
    original.fluid_received = 450;
    original.has_fluid = true;

    std::vector<uint8_t> buffer;
    serialize_fluid_component(original, buffer);

    // Should be version byte + 12 bytes component = 13 bytes
    ASSERT_EQ(buffer.size(), 13u);
    ASSERT_EQ(buffer[0], FLUID_SERIALIZATION_VERSION);

    FluidComponent deserialized;
    size_t consumed = deserialize_fluid_component(buffer.data(), buffer.size(), deserialized);

    ASSERT_EQ(consumed, 13u);
    ASSERT_EQ(deserialized.fluid_required, 500u);
    ASSERT_EQ(deserialized.fluid_received, 450u);
    ASSERT_EQ(deserialized.has_fluid, true);
}

TEST(fluid_component_default_values) {
    FluidComponent original;

    std::vector<uint8_t> buffer;
    serialize_fluid_component(original, buffer);

    FluidComponent deserialized;
    deserialize_fluid_component(buffer.data(), buffer.size(), deserialized);

    ASSERT_EQ(deserialized.fluid_required, 0u);
    ASSERT_EQ(deserialized.fluid_received, 0u);
    ASSERT_EQ(deserialized.has_fluid, false);
}

TEST(fluid_component_max_values) {
    FluidComponent original;
    original.fluid_required = UINT32_MAX;
    original.fluid_received = UINT32_MAX;
    original.has_fluid = true;

    std::vector<uint8_t> buffer;
    serialize_fluid_component(original, buffer);

    FluidComponent deserialized;
    deserialize_fluid_component(buffer.data(), buffer.size(), deserialized);

    ASSERT_EQ(deserialized.fluid_required, UINT32_MAX);
    ASSERT_EQ(deserialized.fluid_received, UINT32_MAX);
    ASSERT_EQ(deserialized.has_fluid, true);
}

TEST(fluid_component_buffer_too_small) {
    uint8_t small_buf[5] = {};
    FluidComponent comp;
    ASSERT_THROW(deserialize_fluid_component(small_buf, 5, comp), std::runtime_error);
}

TEST(fluid_component_version_validation) {
    FluidComponent original;
    original.fluid_required = 100;

    std::vector<uint8_t> buffer;
    serialize_fluid_component(original, buffer);

    // Corrupt version byte
    buffer[0] = 99;

    FluidComponent deserialized;
    ASSERT_THROW(deserialize_fluid_component(buffer.data(), buffer.size(), deserialized), std::runtime_error);
}

TEST(fluid_component_multiple_in_buffer) {
    FluidComponent comp1;
    comp1.fluid_required = 100;
    comp1.has_fluid = true;

    FluidComponent comp2;
    comp2.fluid_required = 200;
    comp2.has_fluid = false;

    std::vector<uint8_t> buffer;
    serialize_fluid_component(comp1, buffer);
    serialize_fluid_component(comp2, buffer);

    ASSERT_EQ(buffer.size(), 26u); // 13 + 13

    FluidComponent out1, out2;
    size_t consumed1 = deserialize_fluid_component(buffer.data(), buffer.size(), out1);
    ASSERT_EQ(consumed1, 13u);

    size_t consumed2 = deserialize_fluid_component(buffer.data() + consumed1,
                                                     buffer.size() - consumed1, out2);
    ASSERT_EQ(consumed2, 13u);

    ASSERT_EQ(out1.fluid_required, 100u);
    ASSERT_EQ(out1.has_fluid, true);
    ASSERT_EQ(out2.fluid_required, 200u);
    ASSERT_EQ(out2.has_fluid, false);
}

// ============================================================================
// Fluid States Bit Packing Tests (Ticket 6-036)
// ============================================================================

TEST(fluid_states_round_trip) {
    bool states[8] = { true, false, true, true, false, false, true, false };

    std::vector<uint8_t> buffer;
    pack_fluid_states(states, 8, buffer);

    // 4 bytes count + 1 byte packed = 5 bytes
    ASSERT_EQ(buffer.size(), 5u);

    bool result[8] = {};
    size_t consumed = unpack_fluid_states(buffer.data(), buffer.size(), result, 8);
    ASSERT_EQ(consumed, 5u);

    for (int i = 0; i < 8; ++i) {
        ASSERT_EQ(result[i], states[i]);
    }
}

TEST(fluid_states_partial_byte) {
    bool states[5] = { true, true, false, true, false };

    std::vector<uint8_t> buffer;
    pack_fluid_states(states, 5, buffer);

    ASSERT_EQ(buffer.size(), 5u);

    bool result[5] = {};
    size_t consumed = unpack_fluid_states(buffer.data(), buffer.size(), result, 5);
    ASSERT_EQ(consumed, 5u);

    for (int i = 0; i < 5; ++i) {
        ASSERT_EQ(result[i], states[i]);
    }
}

TEST(fluid_states_multiple_bytes) {
    bool states[16];
    for (int i = 0; i < 16; ++i) {
        states[i] = (i % 3 == 0);
    }

    std::vector<uint8_t> buffer;
    pack_fluid_states(states, 16, buffer);

    ASSERT_EQ(buffer.size(), 6u);

    bool result[16] = {};
    size_t consumed = unpack_fluid_states(buffer.data(), buffer.size(), result, 16);
    ASSERT_EQ(consumed, 6u);

    for (int i = 0; i < 16; ++i) {
        ASSERT_EQ(result[i], states[i]);
    }
}

TEST(fluid_states_all_true) {
    bool states[8];
    for (int i = 0; i < 8; ++i) states[i] = true;

    std::vector<uint8_t> buffer;
    pack_fluid_states(states, 8, buffer);

    ASSERT_EQ(buffer[4], 0xFF);

    bool result[8] = {};
    unpack_fluid_states(buffer.data(), buffer.size(), result, 8);

    for (int i = 0; i < 8; ++i) {
        ASSERT(result[i]);
    }
}

TEST(fluid_states_all_false) {
    bool states[8] = {};

    std::vector<uint8_t> buffer;
    pack_fluid_states(states, 8, buffer);

    ASSERT_EQ(buffer[4], 0x00);

    bool result[8] = {};
    unpack_fluid_states(buffer.data(), buffer.size(), result, 8);

    for (int i = 0; i < 8; ++i) {
        ASSERT(!result[i]);
    }
}

TEST(fluid_states_zero_count) {
    std::vector<uint8_t> buffer;
    pack_fluid_states(nullptr, 0, buffer);

    ASSERT_EQ(buffer.size(), 4u);

    size_t consumed = unpack_fluid_states(buffer.data(), buffer.size(), nullptr, 0);
    ASSERT_EQ(consumed, 4u);
}

TEST(fluid_states_buffer_too_small) {
    uint8_t small_buf[2] = {};
    bool states[8] = {};
    ASSERT_THROW(unpack_fluid_states(small_buf, 2, states, 8), std::runtime_error);
}

TEST(fluid_states_count_exceeds_max) {
    std::vector<uint8_t> buffer;
    bool states[100] = {};
    pack_fluid_states(states, 100, buffer);

    bool result[8] = {};
    ASSERT_THROW(unpack_fluid_states(buffer.data(), buffer.size(), result, 8), std::runtime_error);
}

TEST(fluid_states_bit_ordering) {
    bool states[8] = { true, false, false, false, false, false, false, false };

    std::vector<uint8_t> buffer;
    pack_fluid_states(states, 8, buffer);

    ASSERT_EQ(buffer[4], 0x01);

    bool states2[8] = { false, false, false, false, false, false, false, true };
    buffer.clear();
    pack_fluid_states(states2, 8, buffer);

    ASSERT_EQ(buffer[4], 0x80);
}

// ============================================================================
// FluidPoolSyncMessage Tests (Ticket 6-037)
// ============================================================================

TEST(pool_sync_message_size) {
    ASSERT_EQ(FLUID_POOL_SYNC_MESSAGE_SIZE, 22u);
}

TEST(pool_sync_round_trip) {
    FluidPoolSyncMessage original;
    original.owner = 2;
    original.state = static_cast<uint8_t>(FluidPoolState::Deficit);
    original.total_generated = 5000;
    original.total_consumed = 7000;
    original.surplus = -2000;
    original.reservoir_stored = 800;
    original.reservoir_capacity = 1000;

    std::vector<uint8_t> buffer;
    serialize_pool_sync(original, buffer);

    ASSERT_EQ(buffer.size(), 22u);

    FluidPoolSyncMessage deserialized;
    size_t consumed = deserialize_pool_sync(buffer.data(), buffer.size(), deserialized);

    ASSERT_EQ(consumed, 22u);
    ASSERT_EQ(deserialized.owner, 2u);
    ASSERT_EQ(deserialized.state, static_cast<uint8_t>(FluidPoolState::Deficit));
    ASSERT_EQ(deserialized.total_generated, 5000u);
    ASSERT_EQ(deserialized.total_consumed, 7000u);
    ASSERT_EQ(deserialized.surplus, -2000);
    ASSERT_EQ(deserialized.reservoir_stored, 800u);
    ASSERT_EQ(deserialized.reservoir_capacity, 1000u);
}

TEST(pool_sync_negative_surplus) {
    FluidPoolSyncMessage original;
    original.surplus = -1;

    std::vector<uint8_t> buffer;
    serialize_pool_sync(original, buffer);

    FluidPoolSyncMessage deserialized;
    deserialize_pool_sync(buffer.data(), buffer.size(), deserialized);

    ASSERT_EQ(deserialized.surplus, -1);
}

TEST(pool_sync_max_values) {
    FluidPoolSyncMessage original;
    original.owner = 255;
    original.state = static_cast<uint8_t>(FluidPoolState::Collapse);
    original.total_generated = UINT32_MAX;
    original.total_consumed = UINT32_MAX;
    original.surplus = INT32_MIN;
    original.reservoir_stored = UINT32_MAX;
    original.reservoir_capacity = UINT32_MAX;

    std::vector<uint8_t> buffer;
    serialize_pool_sync(original, buffer);

    FluidPoolSyncMessage deserialized;
    deserialize_pool_sync(buffer.data(), buffer.size(), deserialized);

    ASSERT_EQ(deserialized.owner, 255u);
    ASSERT_EQ(deserialized.state, static_cast<uint8_t>(FluidPoolState::Collapse));
    ASSERT_EQ(deserialized.total_generated, UINT32_MAX);
    ASSERT_EQ(deserialized.total_consumed, UINT32_MAX);
    ASSERT_EQ(deserialized.surplus, INT32_MIN);
    ASSERT_EQ(deserialized.reservoir_stored, UINT32_MAX);
    ASSERT_EQ(deserialized.reservoir_capacity, UINT32_MAX);
}

TEST(pool_sync_default_values) {
    FluidPoolSyncMessage original;

    std::vector<uint8_t> buffer;
    serialize_pool_sync(original, buffer);

    FluidPoolSyncMessage deserialized;
    deserialize_pool_sync(buffer.data(), buffer.size(), deserialized);

    ASSERT_EQ(deserialized.owner, 0u);
    ASSERT_EQ(deserialized.state, 0u);
    ASSERT_EQ(deserialized.total_generated, 0u);
    ASSERT_EQ(deserialized.total_consumed, 0u);
    ASSERT_EQ(deserialized.surplus, 0);
    ASSERT_EQ(deserialized.reservoir_stored, 0u);
    ASSERT_EQ(deserialized.reservoir_capacity, 0u);
}

TEST(pool_sync_buffer_too_small) {
    uint8_t small_buf[10] = {};
    FluidPoolSyncMessage msg;
    ASSERT_THROW(deserialize_pool_sync(small_buf, 10, msg), std::runtime_error);
}

TEST(pool_sync_little_endian_encoding) {
    FluidPoolSyncMessage msg;
    msg.total_generated = 0x12345678;

    std::vector<uint8_t> buffer;
    serialize_pool_sync(msg, buffer);

    // total_generated starts at offset 2 (after owner, state)
    ASSERT_EQ(buffer[2], 0x78); // LSB
    ASSERT_EQ(buffer[3], 0x56);
    ASSERT_EQ(buffer[4], 0x34);
    ASSERT_EQ(buffer[5], 0x12); // MSB
}

TEST(pool_sync_all_pool_states) {
    FluidPoolState all_states[] = {
        FluidPoolState::Healthy,
        FluidPoolState::Marginal,
        FluidPoolState::Deficit,
        FluidPoolState::Collapse
    };

    for (auto pool_state : all_states) {
        FluidPoolSyncMessage original;
        original.state = static_cast<uint8_t>(pool_state);

        std::vector<uint8_t> buffer;
        serialize_pool_sync(original, buffer);

        FluidPoolSyncMessage deserialized;
        deserialize_pool_sync(buffer.data(), buffer.size(), deserialized);

        ASSERT_EQ(deserialized.state, static_cast<uint8_t>(pool_state));
    }
}

TEST(pool_sync_reservoir_fields) {
    FluidPoolSyncMessage original;
    original.owner = 1;
    original.reservoir_stored = 750;
    original.reservoir_capacity = 2000;

    std::vector<uint8_t> buffer;
    serialize_pool_sync(original, buffer);

    FluidPoolSyncMessage deserialized;
    deserialize_pool_sync(buffer.data(), buffer.size(), deserialized);

    ASSERT_EQ(deserialized.reservoir_stored, 750u);
    ASSERT_EQ(deserialized.reservoir_capacity, 2000u);
}

// ============================================================================
// Main
// ============================================================================

int main() {
    printf("=== FluidSerialization Unit Tests (Tickets 6-036, 6-037) ===\n\n");

    // FluidComponent serialization
    RUN_TEST(fluid_component_round_trip);
    RUN_TEST(fluid_component_default_values);
    RUN_TEST(fluid_component_max_values);
    RUN_TEST(fluid_component_buffer_too_small);
    RUN_TEST(fluid_component_version_validation);
    RUN_TEST(fluid_component_multiple_in_buffer);

    // Fluid states bit packing
    RUN_TEST(fluid_states_round_trip);
    RUN_TEST(fluid_states_partial_byte);
    RUN_TEST(fluid_states_multiple_bytes);
    RUN_TEST(fluid_states_all_true);
    RUN_TEST(fluid_states_all_false);
    RUN_TEST(fluid_states_zero_count);
    RUN_TEST(fluid_states_buffer_too_small);
    RUN_TEST(fluid_states_count_exceeds_max);
    RUN_TEST(fluid_states_bit_ordering);

    // FluidPoolSyncMessage
    RUN_TEST(pool_sync_message_size);
    RUN_TEST(pool_sync_round_trip);
    RUN_TEST(pool_sync_negative_surplus);
    RUN_TEST(pool_sync_max_values);
    RUN_TEST(pool_sync_default_values);
    RUN_TEST(pool_sync_buffer_too_small);
    RUN_TEST(pool_sync_little_endian_encoding);
    RUN_TEST(pool_sync_all_pool_states);
    RUN_TEST(pool_sync_reservoir_fields);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
