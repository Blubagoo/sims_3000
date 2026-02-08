/**
 * @file test_energy_serialization.cpp
 * @brief Tests for energy serialization/deserialization
 *        (Epic 5, tickets 5-034, 5-035)
 *
 * Tests cover:
 * - EnergyComponent round-trip serialization (memcpy path)
 * - Power states bit packing (serialize/deserialize)
 * - EnergyPoolSyncMessage round-trip
 * - EnergyPoolSyncMessage size is 16 bytes
 * - create_pool_sync_message helper
 * - Error handling for undersized buffers
 */

#include <sims3000/energy/EnergySerialization.h>
#include <sims3000/energy/EnergyPriorities.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

using namespace sims3000::energy;

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
// EnergyComponent Serialization Tests (Ticket 5-034)
// ============================================================================

TEST(energy_component_round_trip) {
    EnergyComponent original;
    original.energy_required = 500;
    original.energy_received = 450;
    original.is_powered = true;
    original.priority = ENERGY_PRIORITY_CRITICAL;
    original.grid_id = 3;
    original._padding = 0;

    std::vector<uint8_t> buffer;
    serialize_energy_component(original, buffer);

    // Should be version byte + 12 bytes component = 13 bytes
    ASSERT_EQ(buffer.size(), 13u);
    ASSERT_EQ(buffer[0], ENERGY_SERIALIZATION_VERSION);

    EnergyComponent deserialized;
    size_t consumed = deserialize_energy_component(buffer.data(), buffer.size(), deserialized);

    ASSERT_EQ(consumed, 13u);
    ASSERT_EQ(deserialized.energy_required, 500u);
    ASSERT_EQ(deserialized.energy_received, 450u);
    ASSERT_EQ(deserialized.is_powered, true);
    ASSERT_EQ(deserialized.priority, ENERGY_PRIORITY_CRITICAL);
    ASSERT_EQ(deserialized.grid_id, 3u);
}

TEST(energy_component_default_values) {
    EnergyComponent original;

    std::vector<uint8_t> buffer;
    serialize_energy_component(original, buffer);

    EnergyComponent deserialized;
    deserialize_energy_component(buffer.data(), buffer.size(), deserialized);

    ASSERT_EQ(deserialized.energy_required, 0u);
    ASSERT_EQ(deserialized.energy_received, 0u);
    ASSERT_EQ(deserialized.is_powered, false);
    ASSERT_EQ(deserialized.priority, ENERGY_PRIORITY_DEFAULT);
    ASSERT_EQ(deserialized.grid_id, 0u);
}

TEST(energy_component_max_values) {
    EnergyComponent original;
    original.energy_required = UINT32_MAX;
    original.energy_received = UINT32_MAX;
    original.is_powered = true;
    original.priority = 255;
    original.grid_id = 255;

    std::vector<uint8_t> buffer;
    serialize_energy_component(original, buffer);

    EnergyComponent deserialized;
    deserialize_energy_component(buffer.data(), buffer.size(), deserialized);

    ASSERT_EQ(deserialized.energy_required, UINT32_MAX);
    ASSERT_EQ(deserialized.energy_received, UINT32_MAX);
    ASSERT_EQ(deserialized.is_powered, true);
    ASSERT_EQ(deserialized.priority, 255u);
    ASSERT_EQ(deserialized.grid_id, 255u);
}

TEST(energy_component_buffer_too_small) {
    uint8_t small_buf[5] = {};
    EnergyComponent comp;
    ASSERT_THROW(deserialize_energy_component(small_buf, 5, comp), std::runtime_error);
}

TEST(energy_component_multiple_in_buffer) {
    EnergyComponent comp1;
    comp1.energy_required = 100;
    comp1.priority = ENERGY_PRIORITY_CRITICAL;

    EnergyComponent comp2;
    comp2.energy_required = 200;
    comp2.priority = ENERGY_PRIORITY_LOW;

    std::vector<uint8_t> buffer;
    serialize_energy_component(comp1, buffer);
    serialize_energy_component(comp2, buffer);

    ASSERT_EQ(buffer.size(), 26u); // 13 + 13

    EnergyComponent out1, out2;
    size_t consumed1 = deserialize_energy_component(buffer.data(), buffer.size(), out1);
    ASSERT_EQ(consumed1, 13u);

    size_t consumed2 = deserialize_energy_component(buffer.data() + consumed1,
                                                     buffer.size() - consumed1, out2);
    ASSERT_EQ(consumed2, 13u);

    ASSERT_EQ(out1.energy_required, 100u);
    ASSERT_EQ(out1.priority, ENERGY_PRIORITY_CRITICAL);
    ASSERT_EQ(out2.energy_required, 200u);
    ASSERT_EQ(out2.priority, ENERGY_PRIORITY_LOW);
}

// ============================================================================
// Power States Bit Packing Tests (Ticket 5-034)
// ============================================================================

TEST(power_states_round_trip) {
    bool states[8] = { true, false, true, true, false, false, true, false };

    std::vector<uint8_t> buffer;
    serialize_power_states(states, 8, buffer);

    // 4 bytes count + 1 byte packed = 5 bytes
    ASSERT_EQ(buffer.size(), 5u);

    bool result[8] = {};
    size_t consumed = deserialize_power_states(buffer.data(), buffer.size(), result, 8);
    ASSERT_EQ(consumed, 5u);

    for (int i = 0; i < 8; ++i) {
        ASSERT_EQ(result[i], states[i]);
    }
}

TEST(power_states_partial_byte) {
    bool states[5] = { true, true, false, true, false };

    std::vector<uint8_t> buffer;
    serialize_power_states(states, 5, buffer);

    ASSERT_EQ(buffer.size(), 5u);

    bool result[5] = {};
    size_t consumed = deserialize_power_states(buffer.data(), buffer.size(), result, 5);
    ASSERT_EQ(consumed, 5u);

    for (int i = 0; i < 5; ++i) {
        ASSERT_EQ(result[i], states[i]);
    }
}

TEST(power_states_multiple_bytes) {
    bool states[16];
    for (int i = 0; i < 16; ++i) {
        states[i] = (i % 3 == 0);
    }

    std::vector<uint8_t> buffer;
    serialize_power_states(states, 16, buffer);

    ASSERT_EQ(buffer.size(), 6u);

    bool result[16] = {};
    size_t consumed = deserialize_power_states(buffer.data(), buffer.size(), result, 16);
    ASSERT_EQ(consumed, 6u);

    for (int i = 0; i < 16; ++i) {
        ASSERT_EQ(result[i], states[i]);
    }
}

TEST(power_states_all_true) {
    bool states[8];
    for (int i = 0; i < 8; ++i) states[i] = true;

    std::vector<uint8_t> buffer;
    serialize_power_states(states, 8, buffer);

    ASSERT_EQ(buffer[4], 0xFF);

    bool result[8] = {};
    deserialize_power_states(buffer.data(), buffer.size(), result, 8);

    for (int i = 0; i < 8; ++i) {
        ASSERT(result[i]);
    }
}

TEST(power_states_all_false) {
    bool states[8] = {};

    std::vector<uint8_t> buffer;
    serialize_power_states(states, 8, buffer);

    ASSERT_EQ(buffer[4], 0x00);

    bool result[8] = {};
    deserialize_power_states(buffer.data(), buffer.size(), result, 8);

    for (int i = 0; i < 8; ++i) {
        ASSERT(!result[i]);
    }
}

TEST(power_states_zero_count) {
    std::vector<uint8_t> buffer;
    serialize_power_states(nullptr, 0, buffer);

    ASSERT_EQ(buffer.size(), 4u);

    size_t consumed = deserialize_power_states(buffer.data(), buffer.size(), nullptr, 0);
    ASSERT_EQ(consumed, 4u);
}

TEST(power_states_buffer_too_small) {
    uint8_t small_buf[2] = {};
    bool states[8] = {};
    ASSERT_THROW(deserialize_power_states(small_buf, 2, states, 8), std::runtime_error);
}

TEST(power_states_count_exceeds_max) {
    std::vector<uint8_t> buffer;
    bool states[100] = {};
    serialize_power_states(states, 100, buffer);

    bool result[8] = {};
    ASSERT_THROW(deserialize_power_states(buffer.data(), buffer.size(), result, 8), std::runtime_error);
}

TEST(power_states_bit_ordering) {
    bool states[8] = { true, false, false, false, false, false, false, false };

    std::vector<uint8_t> buffer;
    serialize_power_states(states, 8, buffer);

    ASSERT_EQ(buffer[4], 0x01);

    bool states2[8] = { false, false, false, false, false, false, false, true };
    buffer.clear();
    serialize_power_states(states2, 8, buffer);

    ASSERT_EQ(buffer[4], 0x80);
}

// ============================================================================
// EnergyPoolSyncMessage Tests (Ticket 5-035)
// ============================================================================

TEST(pool_sync_message_size) {
    ASSERT_EQ(sizeof(EnergyPoolSyncMessage), 16u);
}

TEST(pool_sync_round_trip) {
    EnergyPoolSyncMessage original;
    original.owner = 2;
    original.state = static_cast<uint8_t>(EnergyPoolState::Deficit);
    original._padding[0] = 0;
    original._padding[1] = 0;
    original.total_generated = 5000;
    original.total_consumed = 7000;
    original.surplus = -2000;

    std::vector<uint8_t> buffer;
    serialize_pool_sync(original, buffer);

    ASSERT_EQ(buffer.size(), 16u);

    EnergyPoolSyncMessage deserialized;
    size_t consumed = deserialize_pool_sync(buffer.data(), buffer.size(), deserialized);

    ASSERT_EQ(consumed, 16u);
    ASSERT_EQ(deserialized.owner, 2u);
    ASSERT_EQ(deserialized.state, static_cast<uint8_t>(EnergyPoolState::Deficit));
    ASSERT_EQ(deserialized.total_generated, 5000u);
    ASSERT_EQ(deserialized.total_consumed, 7000u);
    ASSERT_EQ(deserialized.surplus, -2000);
}

TEST(pool_sync_negative_surplus) {
    EnergyPoolSyncMessage original;
    original.surplus = -1;

    std::vector<uint8_t> buffer;
    serialize_pool_sync(original, buffer);

    EnergyPoolSyncMessage deserialized;
    deserialize_pool_sync(buffer.data(), buffer.size(), deserialized);

    ASSERT_EQ(deserialized.surplus, -1);
}

TEST(pool_sync_max_values) {
    EnergyPoolSyncMessage original;
    original.owner = 255;
    original.state = static_cast<uint8_t>(EnergyPoolState::Collapse);
    original.total_generated = UINT32_MAX;
    original.total_consumed = UINT32_MAX;
    original.surplus = INT32_MIN;

    std::vector<uint8_t> buffer;
    serialize_pool_sync(original, buffer);

    EnergyPoolSyncMessage deserialized;
    deserialize_pool_sync(buffer.data(), buffer.size(), deserialized);

    ASSERT_EQ(deserialized.owner, 255u);
    ASSERT_EQ(deserialized.state, static_cast<uint8_t>(EnergyPoolState::Collapse));
    ASSERT_EQ(deserialized.total_generated, UINT32_MAX);
    ASSERT_EQ(deserialized.total_consumed, UINT32_MAX);
    ASSERT_EQ(deserialized.surplus, INT32_MIN);
}

TEST(pool_sync_default_values) {
    EnergyPoolSyncMessage original;

    std::vector<uint8_t> buffer;
    serialize_pool_sync(original, buffer);

    EnergyPoolSyncMessage deserialized;
    deserialize_pool_sync(buffer.data(), buffer.size(), deserialized);

    ASSERT_EQ(deserialized.owner, 0u);
    ASSERT_EQ(deserialized.state, 0u);
    ASSERT_EQ(deserialized.total_generated, 0u);
    ASSERT_EQ(deserialized.total_consumed, 0u);
    ASSERT_EQ(deserialized.surplus, 0);
}

TEST(pool_sync_buffer_too_small) {
    uint8_t small_buf[10] = {};
    EnergyPoolSyncMessage msg;
    ASSERT_THROW(deserialize_pool_sync(small_buf, 10, msg), std::runtime_error);
}

TEST(pool_sync_little_endian_encoding) {
    EnergyPoolSyncMessage msg;
    msg.total_generated = 0x12345678;

    std::vector<uint8_t> buffer;
    serialize_pool_sync(msg, buffer);

    // total_generated starts at offset 4 (after owner, state, 2 padding bytes)
    ASSERT_EQ(buffer[4], 0x78); // LSB
    ASSERT_EQ(buffer[5], 0x56);
    ASSERT_EQ(buffer[6], 0x34);
    ASSERT_EQ(buffer[7], 0x12); // MSB
}

// ============================================================================
// create_pool_sync_message Helper Tests (Ticket 5-035)
// ============================================================================

TEST(create_pool_sync_message_basic) {
    PerPlayerEnergyPool pool;
    pool.owner = 1;
    pool.state = EnergyPoolState::Marginal;
    pool.total_generated = 3000;
    pool.total_consumed = 2800;
    pool.surplus = 200;
    pool.nexus_count = 5;
    pool.consumer_count = 50;

    EnergyPoolSyncMessage msg = create_pool_sync_message(pool);

    ASSERT_EQ(msg.owner, 1u);
    ASSERT_EQ(msg.state, static_cast<uint8_t>(EnergyPoolState::Marginal));
    ASSERT_EQ(msg.total_generated, 3000u);
    ASSERT_EQ(msg.total_consumed, 2800u);
    ASSERT_EQ(msg.surplus, 200);
    ASSERT_EQ(msg._padding[0], 0u);
    ASSERT_EQ(msg._padding[1], 0u);
}

TEST(create_pool_sync_message_deficit) {
    PerPlayerEnergyPool pool;
    pool.owner = 3;
    pool.state = EnergyPoolState::Deficit;
    pool.total_generated = 1000;
    pool.total_consumed = 5000;
    pool.surplus = -4000;

    EnergyPoolSyncMessage msg = create_pool_sync_message(pool);

    ASSERT_EQ(msg.owner, 3u);
    ASSERT_EQ(msg.state, static_cast<uint8_t>(EnergyPoolState::Deficit));
    ASSERT_EQ(msg.total_generated, 1000u);
    ASSERT_EQ(msg.total_consumed, 5000u);
    ASSERT_EQ(msg.surplus, -4000);
}

TEST(create_pool_sync_message_round_trip) {
    PerPlayerEnergyPool pool;
    pool.owner = 7;
    pool.state = EnergyPoolState::Healthy;
    pool.total_generated = 10000;
    pool.total_consumed = 8000;
    pool.surplus = 2000;

    EnergyPoolSyncMessage msg = create_pool_sync_message(pool);

    std::vector<uint8_t> buffer;
    serialize_pool_sync(msg, buffer);

    EnergyPoolSyncMessage deserialized;
    deserialize_pool_sync(buffer.data(), buffer.size(), deserialized);

    ASSERT_EQ(deserialized.owner, pool.owner);
    ASSERT_EQ(deserialized.state, static_cast<uint8_t>(pool.state));
    ASSERT_EQ(deserialized.total_generated, pool.total_generated);
    ASSERT_EQ(deserialized.total_consumed, pool.total_consumed);
    ASSERT_EQ(deserialized.surplus, pool.surplus);
}

// ============================================================================
// Main
// ============================================================================

int main() {
    printf("=== EnergySerialization Unit Tests (Tickets 5-034, 5-035) ===\n\n");

    // EnergyComponent serialization
    RUN_TEST(energy_component_round_trip);
    RUN_TEST(energy_component_default_values);
    RUN_TEST(energy_component_max_values);
    RUN_TEST(energy_component_buffer_too_small);
    RUN_TEST(energy_component_multiple_in_buffer);

    // Power states bit packing
    RUN_TEST(power_states_round_trip);
    RUN_TEST(power_states_partial_byte);
    RUN_TEST(power_states_multiple_bytes);
    RUN_TEST(power_states_all_true);
    RUN_TEST(power_states_all_false);
    RUN_TEST(power_states_zero_count);
    RUN_TEST(power_states_buffer_too_small);
    RUN_TEST(power_states_count_exceeds_max);
    RUN_TEST(power_states_bit_ordering);

    // EnergyPoolSyncMessage
    RUN_TEST(pool_sync_message_size);
    RUN_TEST(pool_sync_round_trip);
    RUN_TEST(pool_sync_negative_surplus);
    RUN_TEST(pool_sync_max_values);
    RUN_TEST(pool_sync_default_values);
    RUN_TEST(pool_sync_buffer_too_small);
    RUN_TEST(pool_sync_little_endian_encoding);

    // create_pool_sync_message helper
    RUN_TEST(create_pool_sync_message_basic);
    RUN_TEST(create_pool_sync_message_deficit);
    RUN_TEST(create_pool_sync_message_round_trip);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
