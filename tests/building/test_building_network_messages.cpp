/**
 * @file test_building_network_messages.cpp
 * @brief Tests for building network message serialization (Ticket 4-040)
 *
 * Tests round-trip serialization for all eight message types:
 * - DemolishRequestMessage (2 tests)
 * - ClearDebrisMessage (2 tests)
 * - BuildingSpawnedMessage (2 tests)
 * - BuildingStateChangedMessage (2 tests)
 * - BuildingUpgradedMessage (2 tests)
 * - ConstructionProgressMessage (2 tests)
 * - BuildingDemolishedMessage (2 tests)
 * - DebrisClearedMessage (2 tests)
 *
 * Each message type has:
 * - Round-trip serialization with non-trivial values
 * - Invalid/truncated data deserialization failure
 */

#include <gtest/gtest.h>
#include <sims3000/building/BuildingNetworkMessages.h>
#include <sims3000/building/BuildingTypes.h>

namespace sims3000 {
namespace building {
namespace {

// ============================================================================
// DemolishRequestMessage tests
// ============================================================================

TEST(BuildingNetworkMessagesTest, DemolishRequest_RoundTrip) {
    DemolishRequestMessage msg;
    msg.entity_id = 42;

    auto data = msg.serialize();
    EXPECT_EQ(data.size(), 5u);

    DemolishRequestMessage out;
    EXPECT_TRUE(DemolishRequestMessage::deserialize(data, out));
    EXPECT_EQ(out.entity_id, 42u);
    EXPECT_EQ(out.version, 1);
}

TEST(BuildingNetworkMessagesTest, DemolishRequest_DeserializeInvalidData) {
    // Too short - only 3 bytes, needs 5
    std::vector<std::uint8_t> data = {1, 0, 0};
    DemolishRequestMessage out;
    EXPECT_FALSE(DemolishRequestMessage::deserialize(data, out));

    // Empty data
    std::vector<std::uint8_t> empty;
    EXPECT_FALSE(DemolishRequestMessage::deserialize(empty, out));
}

// ============================================================================
// ClearDebrisMessage tests
// ============================================================================

TEST(BuildingNetworkMessagesTest, ClearDebris_RoundTrip) {
    ClearDebrisMessage msg;
    msg.entity_id = 12345;

    auto data = msg.serialize();
    EXPECT_EQ(data.size(), 5u);

    ClearDebrisMessage out;
    EXPECT_TRUE(ClearDebrisMessage::deserialize(data, out));
    EXPECT_EQ(out.entity_id, 12345u);
    EXPECT_EQ(out.version, 1);
}

TEST(BuildingNetworkMessagesTest, ClearDebris_DeserializeInvalidData) {
    // Too short - only 2 bytes, needs 5
    std::vector<std::uint8_t> data = {1, 0};
    ClearDebrisMessage out;
    EXPECT_FALSE(ClearDebrisMessage::deserialize(data, out));

    // Empty data
    std::vector<std::uint8_t> empty;
    EXPECT_FALSE(ClearDebrisMessage::deserialize(empty, out));
}

// ============================================================================
// BuildingSpawnedMessage tests
// ============================================================================

TEST(BuildingNetworkMessagesTest, BuildingSpawned_RoundTrip) {
    BuildingSpawnedMessage msg;
    msg.entity_id = 100;
    msg.grid_x = -50;
    msg.grid_y = 200;
    msg.template_id = 7;
    msg.owner_id = 3;
    msg.rotation = 2;
    msg.color_accent_index = 5;

    auto data = msg.serialize();
    EXPECT_EQ(data.size(), 20u);

    BuildingSpawnedMessage out;
    EXPECT_TRUE(BuildingSpawnedMessage::deserialize(data, out));
    EXPECT_EQ(out.entity_id, 100u);
    EXPECT_EQ(out.grid_x, -50);
    EXPECT_EQ(out.grid_y, 200);
    EXPECT_EQ(out.template_id, 7u);
    EXPECT_EQ(out.owner_id, 3);
    EXPECT_EQ(out.rotation, 2);
    EXPECT_EQ(out.color_accent_index, 5);
    EXPECT_EQ(out.version, 1);
}

TEST(BuildingNetworkMessagesTest, BuildingSpawned_DeserializeTooShort) {
    // Only 10 bytes, needs 20
    std::vector<std::uint8_t> data = {1, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    BuildingSpawnedMessage out;
    EXPECT_FALSE(BuildingSpawnedMessage::deserialize(data, out));
}

// ============================================================================
// BuildingStateChangedMessage tests
// ============================================================================

TEST(BuildingNetworkMessagesTest, BuildingStateChanged_RoundTrip) {
    BuildingStateChangedMessage msg;
    msg.entity_id = 999;
    msg.new_state = static_cast<std::uint8_t>(BuildingState::Abandoned);

    auto data = msg.serialize();
    EXPECT_EQ(data.size(), 6u);

    BuildingStateChangedMessage out;
    EXPECT_TRUE(BuildingStateChangedMessage::deserialize(data, out));
    EXPECT_EQ(out.entity_id, 999u);
    EXPECT_EQ(out.new_state, static_cast<std::uint8_t>(BuildingState::Abandoned));
    EXPECT_EQ(out.version, 1);
}

TEST(BuildingNetworkMessagesTest, BuildingStateChanged_VersionCheck) {
    // Default version should be 1
    BuildingStateChangedMessage msg;
    EXPECT_EQ(msg.version, 1);

    // Serialize and verify version is first byte
    auto data = msg.serialize();
    EXPECT_EQ(data[0], 1);

    // Deserialize with version preserved
    BuildingStateChangedMessage out;
    EXPECT_TRUE(BuildingStateChangedMessage::deserialize(data, out));
    EXPECT_EQ(out.version, 1);

    // Truncated data fails
    std::vector<std::uint8_t> short_data = {1, 0, 0};
    EXPECT_FALSE(BuildingStateChangedMessage::deserialize(short_data, out));
}

// ============================================================================
// BuildingUpgradedMessage tests
// ============================================================================

TEST(BuildingNetworkMessagesTest, BuildingUpgraded_RoundTrip) {
    BuildingUpgradedMessage msg;
    msg.entity_id = 500;
    msg.new_level = 3;
    msg.new_template_id = 42;

    auto data = msg.serialize();
    EXPECT_EQ(data.size(), 10u);

    BuildingUpgradedMessage out;
    EXPECT_TRUE(BuildingUpgradedMessage::deserialize(data, out));
    EXPECT_EQ(out.entity_id, 500u);
    EXPECT_EQ(out.new_level, 3);
    EXPECT_EQ(out.new_template_id, 42u);
    EXPECT_EQ(out.version, 1);
}

TEST(BuildingNetworkMessagesTest, BuildingUpgraded_AllFieldsPreserved) {
    BuildingUpgradedMessage msg;
    msg.entity_id = 0xDEADBEEF;
    msg.new_level = 255;
    msg.new_template_id = 0xCAFEBABE;

    auto data = msg.serialize();

    BuildingUpgradedMessage out;
    EXPECT_TRUE(BuildingUpgradedMessage::deserialize(data, out));
    EXPECT_EQ(out.entity_id, 0xDEADBEEFu);
    EXPECT_EQ(out.new_level, 255);
    EXPECT_EQ(out.new_template_id, 0xCAFEBABEu);
    EXPECT_EQ(out.version, 1);
}

// ============================================================================
// ConstructionProgressMessage tests
// ============================================================================

TEST(BuildingNetworkMessagesTest, ConstructionProgress_RoundTrip) {
    ConstructionProgressMessage msg;
    msg.entity_id = 77;
    msg.ticks_elapsed = 50;
    msg.ticks_total = 100;

    auto data = msg.serialize();
    EXPECT_EQ(data.size(), 9u);

    ConstructionProgressMessage out;
    EXPECT_TRUE(ConstructionProgressMessage::deserialize(data, out));
    EXPECT_EQ(out.entity_id, 77u);
    EXPECT_EQ(out.ticks_elapsed, 50);
    EXPECT_EQ(out.ticks_total, 100);
    EXPECT_EQ(out.version, 1);
}

TEST(BuildingNetworkMessagesTest, ConstructionProgress_BoundaryValues) {
    ConstructionProgressMessage msg;
    msg.entity_id = 0xFFFFFFFF;
    msg.ticks_elapsed = 0xFFFF;
    msg.ticks_total = 0xFFFF;

    auto data = msg.serialize();

    ConstructionProgressMessage out;
    EXPECT_TRUE(ConstructionProgressMessage::deserialize(data, out));
    EXPECT_EQ(out.entity_id, 0xFFFFFFFFu);
    EXPECT_EQ(out.ticks_elapsed, 0xFFFF);
    EXPECT_EQ(out.ticks_total, 0xFFFF);

    // Zero values
    ConstructionProgressMessage msg2;
    msg2.entity_id = 0;
    msg2.ticks_elapsed = 0;
    msg2.ticks_total = 0;

    auto data2 = msg2.serialize();

    ConstructionProgressMessage out2;
    EXPECT_TRUE(ConstructionProgressMessage::deserialize(data2, out2));
    EXPECT_EQ(out2.entity_id, 0u);
    EXPECT_EQ(out2.ticks_elapsed, 0);
    EXPECT_EQ(out2.ticks_total, 0);
}

// ============================================================================
// BuildingDemolishedMessage tests
// ============================================================================

TEST(BuildingNetworkMessagesTest, BuildingDemolished_RoundTrip) {
    BuildingDemolishedMessage msg;
    msg.entity_id = 2048;

    auto data = msg.serialize();
    EXPECT_EQ(data.size(), 5u);

    BuildingDemolishedMessage out;
    EXPECT_TRUE(BuildingDemolishedMessage::deserialize(data, out));
    EXPECT_EQ(out.entity_id, 2048u);
    EXPECT_EQ(out.version, 1);
}

TEST(BuildingNetworkMessagesTest, BuildingDemolished_DeserializeEmpty) {
    std::vector<std::uint8_t> empty;
    BuildingDemolishedMessage out;
    EXPECT_FALSE(BuildingDemolishedMessage::deserialize(empty, out));

    // Single byte is also too short
    std::vector<std::uint8_t> one_byte = {1};
    EXPECT_FALSE(BuildingDemolishedMessage::deserialize(one_byte, out));
}

// ============================================================================
// DebrisClearedMessage tests
// ============================================================================

TEST(BuildingNetworkMessagesTest, DebrisCleared_RoundTrip) {
    DebrisClearedMessage msg;
    msg.entity_id = 300;
    msg.grid_x = 10;
    msg.grid_y = 20;

    auto data = msg.serialize();
    EXPECT_EQ(data.size(), 13u);

    DebrisClearedMessage out;
    EXPECT_TRUE(DebrisClearedMessage::deserialize(data, out));
    EXPECT_EQ(out.entity_id, 300u);
    EXPECT_EQ(out.grid_x, 10);
    EXPECT_EQ(out.grid_y, 20);
    EXPECT_EQ(out.version, 1);
}

TEST(BuildingNetworkMessagesTest, DebrisCleared_AllFieldsPreserved) {
    DebrisClearedMessage msg;
    msg.entity_id = 0xABCD1234;
    msg.grid_x = -1000;
    msg.grid_y = 2000;

    auto data = msg.serialize();

    DebrisClearedMessage out;
    EXPECT_TRUE(DebrisClearedMessage::deserialize(data, out));
    EXPECT_EQ(out.entity_id, 0xABCD1234u);
    EXPECT_EQ(out.grid_x, -1000);
    EXPECT_EQ(out.grid_y, 2000);
    EXPECT_EQ(out.version, 1);

    // Truncated data should fail
    std::vector<std::uint8_t> short_data = {1, 0, 0, 0, 0};
    EXPECT_FALSE(DebrisClearedMessage::deserialize(short_data, out));
}

} // anonymous namespace
} // namespace building
} // namespace sims3000
