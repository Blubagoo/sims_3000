/**
 * @file test_component_serialization.cpp
 * @brief Unit tests for component network serialization (Ticket 1-007).
 *
 * Tests cover:
 * - Component type registry with unique IDs
 * - PositionComponent serialization (grid_x, grid_y, elevation)
 * - OwnershipComponent serialization (owner, state, state_changed_at)
 * - Component version byte for backward compatibility
 * - get_serialized_size() method
 * - Round-trip tests for each component type
 * - Edge cases: max values, negative values, all enum states
 */

#include "sims3000/ecs/Components.h"
#include "sims3000/net/NetworkBuffer.h"
#include <cassert>
#include <cstdio>
#include <limits>
#include <cstdint>

using namespace sims3000;

// ============================================================================
// Test helpers
// ============================================================================

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            printf("  FAIL: %s\n", msg); \
            return false; \
        } \
    } while(0)

#define TEST_ASSERT_EQ(a, b, msg) \
    do { \
        if ((a) != (b)) { \
            printf("  FAIL: %s (expected %lld, got %lld)\n", msg, (long long)(b), (long long)(a)); \
            return false; \
        } \
    } while(0)

// ============================================================================
// Component Type Registry Tests
// ============================================================================

bool test_component_type_ids_unique() {
    printf("  test_component_type_ids_unique...\n");

    // Verify all type IDs are unique and non-zero
    TEST_ASSERT_EQ(ComponentTypeID::Invalid, 0, "Invalid type ID should be 0");
    TEST_ASSERT(ComponentTypeID::Position != ComponentTypeID::Invalid, "Position ID is not Invalid");
    TEST_ASSERT(ComponentTypeID::Ownership != ComponentTypeID::Invalid, "Ownership ID is not Invalid");
    TEST_ASSERT(ComponentTypeID::Position != ComponentTypeID::Ownership, "Position != Ownership");

    // Test static methods
    TEST_ASSERT_EQ(PositionComponent::get_type_id(), ComponentTypeID::Position, "PositionComponent type ID");
    TEST_ASSERT_EQ(OwnershipComponent::get_type_id(), ComponentTypeID::Ownership, "OwnershipComponent type ID");

    printf("  PASS\n");
    return true;
}

bool test_component_type_ids_range() {
    printf("  test_component_type_ids_range...\n");

    // Verify type IDs fit in a single byte
    TEST_ASSERT(ComponentTypeID::Position <= 255, "Position ID fits in u8");
    TEST_ASSERT(ComponentTypeID::Ownership <= 255, "Ownership ID fits in u8");
    TEST_ASSERT(ComponentTypeID::Transform <= 255, "Transform ID fits in u8");
    TEST_ASSERT(ComponentTypeID::Building <= 255, "Building ID fits in u8");
    TEST_ASSERT(ComponentTypeID::Energy <= 255, "Energy ID fits in u8");
    TEST_ASSERT(ComponentTypeID::Population <= 255, "Population ID fits in u8");
    TEST_ASSERT(ComponentTypeID::Zone <= 255, "Zone ID fits in u8");
    TEST_ASSERT(ComponentTypeID::Transport <= 255, "Transport ID fits in u8");
    TEST_ASSERT(ComponentTypeID::ServiceCoverage <= 255, "ServiceCoverage ID fits in u8");
    TEST_ASSERT(ComponentTypeID::Taxable <= 255, "Taxable ID fits in u8");

    printf("  PASS\n");
    return true;
}

// ============================================================================
// PositionComponent Serialization Tests
// ============================================================================

bool test_position_basic_roundtrip() {
    printf("  test_position_basic_roundtrip...\n");

    PositionComponent pos;
    pos.pos.x = 100;
    pos.pos.y = 200;
    pos.elevation = 5;

    NetworkBuffer buffer;
    pos.serialize_net(buffer);

    TEST_ASSERT_EQ(buffer.size(), PositionComponent::get_serialized_size(), "serialized size matches");

    buffer.reset_read();
    PositionComponent result = PositionComponent::deserialize_net(buffer);

    TEST_ASSERT_EQ(result.pos.x, 100, "x roundtrip");
    TEST_ASSERT_EQ(result.pos.y, 200, "y roundtrip");
    TEST_ASSERT_EQ(result.elevation, 5, "elevation roundtrip");
    TEST_ASSERT(buffer.at_end(), "buffer fully consumed");

    printf("  PASS\n");
    return true;
}

bool test_position_negative_values() {
    printf("  test_position_negative_values...\n");

    PositionComponent pos;
    pos.pos.x = -100;
    pos.pos.y = -200;
    pos.elevation = -10;

    NetworkBuffer buffer;
    pos.serialize_net(buffer);

    buffer.reset_read();
    PositionComponent result = PositionComponent::deserialize_net(buffer);

    TEST_ASSERT_EQ(result.pos.x, -100, "negative x roundtrip");
    TEST_ASSERT_EQ(result.pos.y, -200, "negative y roundtrip");
    TEST_ASSERT_EQ(result.elevation, -10, "negative elevation roundtrip");

    printf("  PASS\n");
    return true;
}

bool test_position_max_values() {
    printf("  test_position_max_values...\n");

    PositionComponent pos;
    pos.pos.x = std::numeric_limits<std::int16_t>::max();  // 32767
    pos.pos.y = std::numeric_limits<std::int16_t>::max();
    pos.elevation = std::numeric_limits<std::int16_t>::max();

    NetworkBuffer buffer;
    pos.serialize_net(buffer);

    buffer.reset_read();
    PositionComponent result = PositionComponent::deserialize_net(buffer);

    TEST_ASSERT_EQ(result.pos.x, std::numeric_limits<std::int16_t>::max(), "max x roundtrip");
    TEST_ASSERT_EQ(result.pos.y, std::numeric_limits<std::int16_t>::max(), "max y roundtrip");
    TEST_ASSERT_EQ(result.elevation, std::numeric_limits<std::int16_t>::max(), "max elevation roundtrip");

    printf("  PASS\n");
    return true;
}

bool test_position_min_values() {
    printf("  test_position_min_values...\n");

    PositionComponent pos;
    pos.pos.x = std::numeric_limits<std::int16_t>::min();  // -32768
    pos.pos.y = std::numeric_limits<std::int16_t>::min();
    pos.elevation = std::numeric_limits<std::int16_t>::min();

    NetworkBuffer buffer;
    pos.serialize_net(buffer);

    buffer.reset_read();
    PositionComponent result = PositionComponent::deserialize_net(buffer);

    TEST_ASSERT_EQ(result.pos.x, std::numeric_limits<std::int16_t>::min(), "min x roundtrip");
    TEST_ASSERT_EQ(result.pos.y, std::numeric_limits<std::int16_t>::min(), "min y roundtrip");
    TEST_ASSERT_EQ(result.elevation, std::numeric_limits<std::int16_t>::min(), "min elevation roundtrip");

    printf("  PASS\n");
    return true;
}

bool test_position_zero_values() {
    printf("  test_position_zero_values...\n");

    PositionComponent pos;
    pos.pos.x = 0;
    pos.pos.y = 0;
    pos.elevation = 0;

    NetworkBuffer buffer;
    pos.serialize_net(buffer);

    buffer.reset_read();
    PositionComponent result = PositionComponent::deserialize_net(buffer);

    TEST_ASSERT_EQ(result.pos.x, 0, "zero x roundtrip");
    TEST_ASSERT_EQ(result.pos.y, 0, "zero y roundtrip");
    TEST_ASSERT_EQ(result.elevation, 0, "zero elevation roundtrip");

    printf("  PASS\n");
    return true;
}

bool test_position_version_byte() {
    printf("  test_position_version_byte...\n");

    PositionComponent pos;
    pos.pos.x = 1;
    pos.pos.y = 2;
    pos.elevation = 3;

    NetworkBuffer buffer;
    pos.serialize_net(buffer);

    // Verify first byte is version
    TEST_ASSERT_EQ(buffer.data()[0], ComponentVersion::Position, "first byte is version");

    printf("  PASS\n");
    return true;
}

bool test_position_get_serialized_size() {
    printf("  test_position_get_serialized_size...\n");

    // Verify constexpr size matches actual serialized size
    constexpr std::size_t expected_size = PositionComponent::get_serialized_size();
    TEST_ASSERT_EQ(expected_size, 7, "expected size is 7 bytes");

    PositionComponent pos;
    pos.pos.x = 12345;
    pos.pos.y = -12345;
    pos.elevation = 31;

    NetworkBuffer buffer;
    pos.serialize_net(buffer);

    TEST_ASSERT_EQ(buffer.size(), expected_size, "actual size matches get_serialized_size()");

    printf("  PASS\n");
    return true;
}

// ============================================================================
// OwnershipComponent Serialization Tests
// ============================================================================

bool test_ownership_basic_roundtrip() {
    printf("  test_ownership_basic_roundtrip...\n");

    OwnershipComponent ownership;
    ownership.owner = 1;
    ownership.state = OwnershipState::Owned;
    ownership.state_changed_at = 12345;

    NetworkBuffer buffer;
    ownership.serialize_net(buffer);

    TEST_ASSERT_EQ(buffer.size(), OwnershipComponent::get_serialized_size(), "serialized size matches");

    buffer.reset_read();
    OwnershipComponent result = OwnershipComponent::deserialize_net(buffer);

    TEST_ASSERT_EQ(result.owner, 1, "owner roundtrip");
    TEST_ASSERT(result.state == OwnershipState::Owned, "state roundtrip");
    TEST_ASSERT_EQ(result.state_changed_at, 12345, "state_changed_at roundtrip");
    TEST_ASSERT(buffer.at_end(), "buffer fully consumed");

    printf("  PASS\n");
    return true;
}

bool test_ownership_game_master() {
    printf("  test_ownership_game_master...\n");

    OwnershipComponent ownership;
    ownership.owner = GAME_MASTER;  // 0
    ownership.state = OwnershipState::Neutral;
    ownership.state_changed_at = 0;

    NetworkBuffer buffer;
    ownership.serialize_net(buffer);

    buffer.reset_read();
    OwnershipComponent result = OwnershipComponent::deserialize_net(buffer);

    TEST_ASSERT_EQ(result.owner, GAME_MASTER, "game master owner roundtrip");
    TEST_ASSERT(result.state == OwnershipState::Neutral, "neutral state roundtrip");
    TEST_ASSERT_EQ(result.state_changed_at, 0, "zero tick roundtrip");

    printf("  PASS\n");
    return true;
}

bool test_ownership_all_states() {
    printf("  test_ownership_all_states...\n");

    // Test all OwnershipState enum values
    OwnershipState states[] = {
        OwnershipState::Owned,
        OwnershipState::Abandoned,
        OwnershipState::Neutral,
        OwnershipState::Contested
    };

    for (OwnershipState state : states) {
        OwnershipComponent ownership;
        ownership.owner = 1;
        ownership.state = state;
        ownership.state_changed_at = 100;

        NetworkBuffer buffer;
        ownership.serialize_net(buffer);

        buffer.reset_read();
        OwnershipComponent result = OwnershipComponent::deserialize_net(buffer);

        TEST_ASSERT(result.state == state, "state enum roundtrip");
    }

    printf("  PASS\n");
    return true;
}

bool test_ownership_max_player_id() {
    printf("  test_ownership_max_player_id...\n");

    OwnershipComponent ownership;
    ownership.owner = std::numeric_limits<PlayerID>::max();  // 255
    ownership.state = OwnershipState::Owned;
    ownership.state_changed_at = 1000;

    NetworkBuffer buffer;
    ownership.serialize_net(buffer);

    buffer.reset_read();
    OwnershipComponent result = OwnershipComponent::deserialize_net(buffer);

    TEST_ASSERT_EQ(result.owner, std::numeric_limits<PlayerID>::max(), "max player id roundtrip");

    printf("  PASS\n");
    return true;
}

bool test_ownership_max_tick() {
    printf("  test_ownership_max_tick...\n");

    OwnershipComponent ownership;
    ownership.owner = 1;
    ownership.state = OwnershipState::Owned;
    ownership.state_changed_at = std::numeric_limits<SimulationTick>::max();

    NetworkBuffer buffer;
    ownership.serialize_net(buffer);

    buffer.reset_read();
    OwnershipComponent result = OwnershipComponent::deserialize_net(buffer);

    TEST_ASSERT_EQ(result.state_changed_at, std::numeric_limits<SimulationTick>::max(), "max tick roundtrip");

    printf("  PASS\n");
    return true;
}

bool test_ownership_large_tick_values() {
    printf("  test_ownership_large_tick_values...\n");

    // Test values that exercise both low and high 32-bit parts
    SimulationTick test_values[] = {
        0xFFFFFFFF,                     // Max u32 (low part only)
        0x100000000ULL,                 // First value with high part
        0x123456789ABCDEF0ULL,          // Large mixed value
        0xFFFFFFFFFFFFFFFFULL           // Max u64
    };

    for (SimulationTick tick : test_values) {
        OwnershipComponent ownership;
        ownership.owner = 1;
        ownership.state = OwnershipState::Owned;
        ownership.state_changed_at = tick;

        NetworkBuffer buffer;
        ownership.serialize_net(buffer);

        buffer.reset_read();
        OwnershipComponent result = OwnershipComponent::deserialize_net(buffer);

        TEST_ASSERT_EQ(result.state_changed_at, tick, "large tick roundtrip");
    }

    printf("  PASS\n");
    return true;
}

bool test_ownership_version_byte() {
    printf("  test_ownership_version_byte...\n");

    OwnershipComponent ownership;
    ownership.owner = 1;
    ownership.state = OwnershipState::Owned;
    ownership.state_changed_at = 100;

    NetworkBuffer buffer;
    ownership.serialize_net(buffer);

    // Verify first byte is version
    TEST_ASSERT_EQ(buffer.data()[0], ComponentVersion::Ownership, "first byte is version");

    printf("  PASS\n");
    return true;
}

bool test_ownership_get_serialized_size() {
    printf("  test_ownership_get_serialized_size...\n");

    // Verify constexpr size matches actual serialized size
    constexpr std::size_t expected_size = OwnershipComponent::get_serialized_size();
    TEST_ASSERT_EQ(expected_size, 11, "expected size is 11 bytes");

    OwnershipComponent ownership;
    ownership.owner = 255;
    ownership.state = OwnershipState::Contested;
    ownership.state_changed_at = 0xFFFFFFFFFFFFFFFFULL;

    NetworkBuffer buffer;
    ownership.serialize_net(buffer);

    TEST_ASSERT_EQ(buffer.size(), expected_size, "actual size matches get_serialized_size()");

    printf("  PASS\n");
    return true;
}

// ============================================================================
// Multiple Components in Buffer Test
// ============================================================================

bool test_multiple_components_in_buffer() {
    printf("  test_multiple_components_in_buffer...\n");

    // Write multiple components to same buffer
    PositionComponent pos1;
    pos1.pos.x = 10;
    pos1.pos.y = 20;
    pos1.elevation = 1;

    OwnershipComponent own1;
    own1.owner = 2;
    own1.state = OwnershipState::Owned;
    own1.state_changed_at = 500;

    PositionComponent pos2;
    pos2.pos.x = -10;
    pos2.pos.y = -20;
    pos2.elevation = -1;

    NetworkBuffer buffer;
    pos1.serialize_net(buffer);
    own1.serialize_net(buffer);
    pos2.serialize_net(buffer);

    std::size_t expected_total = PositionComponent::get_serialized_size() +
                                  OwnershipComponent::get_serialized_size() +
                                  PositionComponent::get_serialized_size();
    TEST_ASSERT_EQ(buffer.size(), expected_total, "total buffer size");

    // Read them back in order
    buffer.reset_read();

    PositionComponent r_pos1 = PositionComponent::deserialize_net(buffer);
    OwnershipComponent r_own1 = OwnershipComponent::deserialize_net(buffer);
    PositionComponent r_pos2 = PositionComponent::deserialize_net(buffer);

    TEST_ASSERT_EQ(r_pos1.pos.x, 10, "first position x");
    TEST_ASSERT_EQ(r_pos1.pos.y, 20, "first position y");
    TEST_ASSERT_EQ(r_own1.owner, 2, "ownership owner");
    TEST_ASSERT(r_own1.state == OwnershipState::Owned, "ownership state");
    TEST_ASSERT_EQ(r_pos2.pos.x, -10, "second position x");
    TEST_ASSERT_EQ(r_pos2.pos.y, -20, "second position y");
    TEST_ASSERT(buffer.at_end(), "buffer fully consumed");

    printf("  PASS\n");
    return true;
}

// ============================================================================
// Pre-allocation Optimization Test
// ============================================================================

bool test_preallocation_optimization() {
    printf("  test_preallocation_optimization...\n");

    // Verify that get_serialized_size() can be used for pre-allocation
    constexpr std::size_t pos_size = PositionComponent::get_serialized_size();
    constexpr std::size_t own_size = OwnershipComponent::get_serialized_size();
    constexpr std::size_t total_size = pos_size + own_size;

    NetworkBuffer buffer(total_size);  // Pre-allocate

    PositionComponent pos;
    pos.pos.x = 1;
    pos.pos.y = 2;
    pos.elevation = 3;

    OwnershipComponent own;
    own.owner = 1;
    own.state = OwnershipState::Owned;
    own.state_changed_at = 100;

    pos.serialize_net(buffer);
    own.serialize_net(buffer);

    TEST_ASSERT_EQ(buffer.size(), total_size, "buffer size matches pre-allocated");

    printf("  PASS\n");
    return true;
}

// ============================================================================
// Default Values Test (for version compatibility)
// ============================================================================

bool test_default_values() {
    printf("  test_default_values...\n");

    // Verify that default-constructed components have expected values
    PositionComponent pos;
    TEST_ASSERT_EQ(pos.pos.x, 0, "default position x");
    TEST_ASSERT_EQ(pos.pos.y, 0, "default position y");
    TEST_ASSERT_EQ(pos.elevation, 0, "default elevation");

    OwnershipComponent own;
    TEST_ASSERT_EQ(own.owner, GAME_MASTER, "default owner is GAME_MASTER");
    TEST_ASSERT(own.state == OwnershipState::Neutral, "default state is Neutral");
    TEST_ASSERT_EQ(own.state_changed_at, 0, "default state_changed_at is 0");

    printf("  PASS\n");
    return true;
}

// ============================================================================
// Main
// ============================================================================

int main() {
    printf("=== Component Serialization Unit Tests (Ticket 1-007) ===\n\n");

    int passed = 0;
    int failed = 0;

    #define RUN_TEST(test) \
        do { \
            printf("Running %s\n", #test); \
            if (test()) { passed++; } else { failed++; } \
        } while(0)

    printf("--- Component Type Registry Tests ---\n");
    RUN_TEST(test_component_type_ids_unique);
    RUN_TEST(test_component_type_ids_range);

    printf("\n--- PositionComponent Serialization Tests ---\n");
    RUN_TEST(test_position_basic_roundtrip);
    RUN_TEST(test_position_negative_values);
    RUN_TEST(test_position_max_values);
    RUN_TEST(test_position_min_values);
    RUN_TEST(test_position_zero_values);
    RUN_TEST(test_position_version_byte);
    RUN_TEST(test_position_get_serialized_size);

    printf("\n--- OwnershipComponent Serialization Tests ---\n");
    RUN_TEST(test_ownership_basic_roundtrip);
    RUN_TEST(test_ownership_game_master);
    RUN_TEST(test_ownership_all_states);
    RUN_TEST(test_ownership_max_player_id);
    RUN_TEST(test_ownership_max_tick);
    RUN_TEST(test_ownership_large_tick_values);
    RUN_TEST(test_ownership_version_byte);
    RUN_TEST(test_ownership_get_serialized_size);

    printf("\n--- Integration Tests ---\n");
    RUN_TEST(test_multiple_components_in_buffer);
    RUN_TEST(test_preallocation_optimization);
    RUN_TEST(test_default_values);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", passed);
    printf("Failed: %d\n", failed);

    if (failed > 0) {
        printf("\n=== TESTS FAILED ===\n");
        return 1;
    }

    printf("\n=== All tests passed! ===\n");
    return 0;
}
