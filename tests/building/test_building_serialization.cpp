/**
 * @file test_building_serialization.cpp
 * @brief Tests for building component serialization/deserialization (Ticket 4-042)
 */

#include <sims3000/building/BuildingSerialization.h>
#include <gtest/gtest.h>

using namespace sims3000::building;

// ============================================================================
// BuildingComponent Serialization Tests
// ============================================================================

TEST(BuildingSerializationTest, BuildingComponentRoundTrip) {
    BuildingComponent original;
    original.template_id = 1001;
    original.zone_type = static_cast<std::uint8_t>(ZoneBuildingType::Exchange);
    original.density = static_cast<std::uint8_t>(DensityLevel::High);
    original.state = static_cast<std::uint8_t>(BuildingState::Active);
    original.level = 3;
    original.health = 200;
    original.capacity = 500;
    original.current_occupancy = 350;
    original.footprint_w = 2;
    original.footprint_h = 3;
    original.state_changed_tick = 123456;
    original.abandon_timer = 100;
    original.rotation = 2;
    original.color_accent_index = 1;

    std::vector<std::uint8_t> buffer;
    serialize_building_component(original, buffer);

    BuildingComponent deserialized = deserialize_building_component(buffer.data(), buffer.size());

    EXPECT_EQ(deserialized.template_id, 1001u);
    EXPECT_EQ(deserialized.zone_type, static_cast<std::uint8_t>(ZoneBuildingType::Exchange));
    EXPECT_EQ(deserialized.density, static_cast<std::uint8_t>(DensityLevel::High));
    EXPECT_EQ(deserialized.state, static_cast<std::uint8_t>(BuildingState::Active));
    EXPECT_EQ(deserialized.level, 3u);
    EXPECT_EQ(deserialized.health, 200u);
    EXPECT_EQ(deserialized.capacity, 500u);
    EXPECT_EQ(deserialized.current_occupancy, 350u);
    EXPECT_EQ(deserialized.footprint_w, 2u);
    EXPECT_EQ(deserialized.footprint_h, 3u);
    EXPECT_EQ(deserialized.state_changed_tick, 123456u);
    EXPECT_EQ(deserialized.abandon_timer, 100u);
    EXPECT_EQ(deserialized.rotation, 2u);
    EXPECT_EQ(deserialized.color_accent_index, 1u);
}

TEST(BuildingSerializationTest, BuildingComponentVersionByte) {
    BuildingComponent comp;
    std::vector<std::uint8_t> buffer;
    serialize_building_component(comp, buffer);

    EXPECT_EQ(buffer[0], BUILDING_SERIALIZATION_VERSION);
    // Total: 28 bytes
    EXPECT_EQ(buffer.size(), 28u);
}

TEST(BuildingSerializationTest, BuildingComponentReservedBytesPreserved) {
    BuildingComponent original;
    original.template_id = 42;
    original.rotation = 3;

    std::vector<std::uint8_t> buffer;
    serialize_building_component(original, buffer);

    // Verify reserved bytes are zero
    EXPECT_EQ(buffer[24], 0u); // reserved byte 1
    EXPECT_EQ(buffer[25], 0u); // reserved byte 2
    EXPECT_EQ(buffer[26], 0u); // reserved byte 3
    EXPECT_EQ(buffer[27], 0u); // reserved byte 4

    // Corrupt reserved bytes (simulate future version data)
    buffer[24] = 0xFF;
    buffer[25] = 0xAB;
    buffer[26] = 0xCD;
    buffer[27] = 0xEF;

    // Deserialize should still work correctly (reserved bytes skipped)
    BuildingComponent deserialized = deserialize_building_component(buffer.data(), buffer.size());
    EXPECT_EQ(deserialized.template_id, 42u);
    EXPECT_EQ(deserialized.rotation, 3u);
}

TEST(BuildingSerializationTest, BuildingComponentAllStates) {
    for (std::uint8_t s = 0; s < BUILDING_STATE_COUNT; ++s) {
        BuildingComponent original;
        original.state = s;

        std::vector<std::uint8_t> buffer;
        serialize_building_component(original, buffer);
        BuildingComponent deserialized = deserialize_building_component(buffer.data(), buffer.size());

        EXPECT_EQ(deserialized.state, s);
    }
}

TEST(BuildingSerializationTest, BuildingComponentOldVersionCompatibility) {
    // Simulate an older version without reserved bytes (24 bytes)
    BuildingComponent original;
    original.template_id = 9999;
    original.capacity = 42;

    std::vector<std::uint8_t> buffer;
    serialize_building_component(original, buffer);

    // Truncate to remove reserved bytes (simulate old format)
    buffer.resize(24);

    // Should still deserialize with default values for missing fields
    BuildingComponent deserialized = deserialize_building_component(buffer.data(), buffer.size());
    EXPECT_EQ(deserialized.template_id, 9999u);
    EXPECT_EQ(deserialized.capacity, 42u);
}

// ============================================================================
// ConstructionComponent Serialization Tests
// ============================================================================

TEST(BuildingSerializationTest, ConstructionComponentRoundTrip) {
    ConstructionComponent original;
    original.ticks_total = 200;
    original.ticks_elapsed = 150;
    original.phase = static_cast<std::uint8_t>(ConstructionPhase::Exterior);
    original.phase_progress = 128;
    original.construction_cost = 5000;
    original.is_paused = 1;

    std::vector<std::uint8_t> buffer;
    serialize_construction_component(original, buffer);

    ConstructionComponent deserialized = deserialize_construction_component(buffer.data(), buffer.size());

    EXPECT_EQ(deserialized.ticks_total, 200u);
    EXPECT_EQ(deserialized.ticks_elapsed, 150u);
    EXPECT_EQ(deserialized.phase, static_cast<std::uint8_t>(ConstructionPhase::Exterior));
    EXPECT_EQ(deserialized.phase_progress, 128u);
    EXPECT_EQ(deserialized.construction_cost, 5000u);
    EXPECT_EQ(deserialized.is_paused, 1u);
}

TEST(BuildingSerializationTest, ConstructionComponentVersionByte) {
    ConstructionComponent comp;
    std::vector<std::uint8_t> buffer;
    serialize_construction_component(comp, buffer);

    EXPECT_EQ(buffer[0], BUILDING_SERIALIZATION_VERSION);
    // Total: 16 bytes
    EXPECT_EQ(buffer.size(), 16u);
}

TEST(BuildingSerializationTest, ConstructionComponentReservedBytes) {
    ConstructionComponent original(200, 5000);

    std::vector<std::uint8_t> buffer;
    serialize_construction_component(original, buffer);

    // Verify reserved bytes are at the end
    EXPECT_EQ(buffer[12], 0u);
    EXPECT_EQ(buffer[13], 0u);
    EXPECT_EQ(buffer[14], 0u);
    EXPECT_EQ(buffer[15], 0u);

    // Corrupt reserved bytes
    buffer[12] = 0xFF;
    buffer[13] = 0xFF;
    buffer[14] = 0xFF;
    buffer[15] = 0xFF;

    // Deserialize should still work
    ConstructionComponent deserialized = deserialize_construction_component(buffer.data(), buffer.size());
    EXPECT_EQ(deserialized.ticks_total, 200u);
    EXPECT_EQ(deserialized.construction_cost, 5000u);
}

TEST(BuildingSerializationTest, ConstructionComponentOldVersionCompatibility) {
    ConstructionComponent original;
    original.ticks_total = 100;
    original.construction_cost = 3000;

    std::vector<std::uint8_t> buffer;
    serialize_construction_component(original, buffer);

    // Truncate to remove reserved bytes
    buffer.resize(12);

    ConstructionComponent deserialized = deserialize_construction_component(buffer.data(), buffer.size());
    EXPECT_EQ(deserialized.ticks_total, 100u);
    EXPECT_EQ(deserialized.construction_cost, 3000u);
}

// ============================================================================
// DebrisComponent Serialization Tests
// ============================================================================

TEST(BuildingSerializationTest, DebrisComponentRoundTrip) {
    DebrisComponent original(42, 2, 3, 120);

    std::vector<std::uint8_t> buffer;
    serialize_debris_component(original, buffer);

    DebrisComponent deserialized = deserialize_debris_component(buffer.data(), buffer.size());

    EXPECT_EQ(deserialized.original_template_id, 42u);
    EXPECT_EQ(deserialized.clear_timer, 120u);
    EXPECT_EQ(deserialized.footprint_w, 2u);
    EXPECT_EQ(deserialized.footprint_h, 3u);
}

TEST(BuildingSerializationTest, DebrisComponentVersionByte) {
    DebrisComponent comp;
    std::vector<std::uint8_t> buffer;
    serialize_debris_component(comp, buffer);

    EXPECT_EQ(buffer[0], BUILDING_SERIALIZATION_VERSION);
    // Total: 13 bytes
    EXPECT_EQ(buffer.size(), 13u);
}

TEST(BuildingSerializationTest, DebrisComponentReservedBytes) {
    DebrisComponent original(1001, 1, 1, 60);

    std::vector<std::uint8_t> buffer;
    serialize_debris_component(original, buffer);

    // Reserved bytes at end
    EXPECT_EQ(buffer[9], 0u);
    EXPECT_EQ(buffer[10], 0u);
    EXPECT_EQ(buffer[11], 0u);
    EXPECT_EQ(buffer[12], 0u);

    // Corrupt reserved bytes
    buffer[9] = 0xAA;
    buffer[10] = 0xBB;
    buffer[11] = 0xCC;
    buffer[12] = 0xDD;

    DebrisComponent deserialized = deserialize_debris_component(buffer.data(), buffer.size());
    EXPECT_EQ(deserialized.original_template_id, 1001u);
    EXPECT_EQ(deserialized.clear_timer, 60u);
}

TEST(BuildingSerializationTest, DebrisComponentOldVersionCompatibility) {
    DebrisComponent original(555, 4, 4, 30);

    std::vector<std::uint8_t> buffer;
    serialize_debris_component(original, buffer);

    // Truncate to remove reserved bytes
    buffer.resize(9);

    DebrisComponent deserialized = deserialize_debris_component(buffer.data(), buffer.size());
    EXPECT_EQ(deserialized.original_template_id, 555u);
    EXPECT_EQ(deserialized.clear_timer, 30u);
    EXPECT_EQ(deserialized.footprint_w, 4u);
    EXPECT_EQ(deserialized.footprint_h, 4u);
}

// ============================================================================
// Error Handling Tests
// ============================================================================

TEST(BuildingSerializationTest, BuildingComponentTooSmallBuffer) {
    std::uint8_t small_buf[10] = {};
    EXPECT_THROW(deserialize_building_component(small_buf, 10), std::runtime_error);
}

TEST(BuildingSerializationTest, ConstructionComponentTooSmallBuffer) {
    std::uint8_t small_buf[5] = {};
    EXPECT_THROW(deserialize_construction_component(small_buf, 5), std::runtime_error);
}

TEST(BuildingSerializationTest, DebrisComponentTooSmallBuffer) {
    std::uint8_t small_buf[4] = {};
    EXPECT_THROW(deserialize_debris_component(small_buf, 4), std::runtime_error);
}

// ============================================================================
// Little-Endian Encoding Tests
// ============================================================================

TEST(BuildingSerializationTest, LittleEndianEncoding) {
    BuildingComponent comp;
    comp.template_id = 0x12345678;

    std::vector<std::uint8_t> buffer;
    serialize_building_component(comp, buffer);

    // template_id starts at offset 1 (after version byte)
    // Little-endian: least significant byte first
    EXPECT_EQ(buffer[1], 0x78); // LSB
    EXPECT_EQ(buffer[2], 0x56);
    EXPECT_EQ(buffer[3], 0x34);
    EXPECT_EQ(buffer[4], 0x12); // MSB
}
