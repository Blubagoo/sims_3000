/**
 * @file test_zone_serialization.cpp
 * @brief Tests for zone data serialization/deserialization (Ticket 4-041)
 */

#include <sims3000/zone/ZoneSerialization.h>
#include <gtest/gtest.h>

using namespace sims3000::zone;

// ============================================================================
// ZoneComponent Serialization Tests
// ============================================================================

TEST(ZoneSerializationTest, ZoneComponentRoundTrip) {
    ZoneComponent original;
    original.zone_type = static_cast<std::uint8_t>(ZoneType::Exchange);
    original.density = static_cast<std::uint8_t>(ZoneDensity::HighDensity);
    original.state = static_cast<std::uint8_t>(ZoneState::Occupied);
    original.desirability = 200;

    std::vector<std::uint8_t> buffer;
    serialize_zone_component(original, buffer);

    ZoneComponent deserialized = deserialize_zone_component(buffer.data(), buffer.size());

    EXPECT_EQ(deserialized.zone_type, original.zone_type);
    EXPECT_EQ(deserialized.density, original.density);
    EXPECT_EQ(deserialized.state, original.state);
    EXPECT_EQ(deserialized.desirability, original.desirability);
}

TEST(ZoneSerializationTest, ZoneComponentVersionByte) {
    ZoneComponent comp;
    comp.zone_type = 0;
    comp.density = 0;
    comp.state = 0;
    comp.desirability = 0;

    std::vector<std::uint8_t> buffer;
    serialize_zone_component(comp, buffer);

    // First byte should be version
    EXPECT_EQ(buffer[0], ZONE_SERIALIZATION_VERSION);
    // Total size = version(1) + 4 fields = 5 bytes
    EXPECT_EQ(buffer.size(), 5u);
}

TEST(ZoneSerializationTest, ZoneComponentAllZoneTypes) {
    for (std::uint8_t zt = 0; zt < ZONE_TYPE_COUNT; ++zt) {
        ZoneComponent original;
        original.zone_type = zt;
        original.density = 0;
        original.state = 0;
        original.desirability = 128;

        std::vector<std::uint8_t> buffer;
        serialize_zone_component(original, buffer);
        ZoneComponent deserialized = deserialize_zone_component(buffer.data(), buffer.size());

        EXPECT_EQ(deserialized.zone_type, zt);
        EXPECT_EQ(deserialized.desirability, 128);
    }
}

// ============================================================================
// ZoneGrid Serialization Tests
// ============================================================================

TEST(ZoneSerializationTest, EmptyGridSerialization) {
    ZoneGrid grid;  // Default constructor = empty grid

    std::vector<std::uint8_t> buffer;
    serialize_zone_grid(grid, buffer);

    // Should have version(1) + width(2) + height(2) + cell_count(4) = 9
    EXPECT_EQ(buffer.size(), 9u);
    EXPECT_EQ(buffer[0], ZONE_SERIALIZATION_VERSION);

    ZoneGrid deserialized = deserialize_zone_grid(buffer.data(), buffer.size());
    EXPECT_EQ(deserialized.getWidth(), 0u);
    EXPECT_EQ(deserialized.getHeight(), 0u);
    EXPECT_TRUE(deserialized.empty());
}

TEST(ZoneSerializationTest, GridWithZonesRoundTrip) {
    ZoneGrid original(128, 128);
    original.place_zone(0, 0, 100);
    original.place_zone(10, 20, 200);
    original.place_zone(127, 127, 300);

    std::vector<std::uint8_t> buffer;
    serialize_zone_grid(original, buffer);

    ZoneGrid deserialized = deserialize_zone_grid(buffer.data(), buffer.size());

    EXPECT_EQ(deserialized.getWidth(), 128u);
    EXPECT_EQ(deserialized.getHeight(), 128u);
    EXPECT_EQ(deserialized.get_zone_at(0, 0), 100u);
    EXPECT_EQ(deserialized.get_zone_at(10, 20), 200u);
    EXPECT_EQ(deserialized.get_zone_at(127, 127), 300u);

    // Empty cells should remain empty
    EXPECT_EQ(deserialized.get_zone_at(1, 1), INVALID_ENTITY);
    EXPECT_EQ(deserialized.get_zone_at(50, 50), INVALID_ENTITY);
}

TEST(ZoneSerializationTest, GridVersionByte) {
    ZoneGrid grid(128, 128);

    std::vector<std::uint8_t> buffer;
    serialize_zone_grid(grid, buffer);

    EXPECT_EQ(buffer[0], ZONE_SERIALIZATION_VERSION);
}

TEST(ZoneSerializationTest, GridLargerSize) {
    ZoneGrid original(256, 256);
    // Place zones in a pattern
    for (int i = 0; i < 100; ++i) {
        original.place_zone(i, i, static_cast<std::uint32_t>(i + 1));
    }

    std::vector<std::uint8_t> buffer;
    serialize_zone_grid(original, buffer);

    ZoneGrid deserialized = deserialize_zone_grid(buffer.data(), buffer.size());

    EXPECT_EQ(deserialized.getWidth(), 256u);
    EXPECT_EQ(deserialized.getHeight(), 256u);

    for (int i = 0; i < 100; ++i) {
        EXPECT_EQ(deserialized.get_zone_at(i, i), static_cast<std::uint32_t>(i + 1));
    }
}

// ============================================================================
// ZoneCounts Serialization Tests
// ============================================================================

TEST(ZoneSerializationTest, ZoneCountsRoundTrip) {
    ZoneCounts original;
    original.habitation_total = 500;
    original.exchange_total = 300;
    original.fabrication_total = 200;
    original.aeroport_total = 50;
    original.aquaport_total = 25;
    original.low_density_total = 600;
    original.high_density_total = 400;
    original.designated_total = 100;
    original.occupied_total = 800;
    original.stalled_total = 100;
    original.total = 1000;

    std::vector<std::uint8_t> buffer;
    serialize_zone_counts(original, buffer);

    ZoneCounts deserialized = deserialize_zone_counts(buffer.data(), buffer.size());

    EXPECT_EQ(deserialized.habitation_total, 500u);
    EXPECT_EQ(deserialized.exchange_total, 300u);
    EXPECT_EQ(deserialized.fabrication_total, 200u);
    EXPECT_EQ(deserialized.aeroport_total, 50u);
    EXPECT_EQ(deserialized.aquaport_total, 25u);
    EXPECT_EQ(deserialized.low_density_total, 600u);
    EXPECT_EQ(deserialized.high_density_total, 400u);
    EXPECT_EQ(deserialized.designated_total, 100u);
    EXPECT_EQ(deserialized.occupied_total, 800u);
    EXPECT_EQ(deserialized.stalled_total, 100u);
    EXPECT_EQ(deserialized.total, 1000u);
}

TEST(ZoneSerializationTest, ZoneCountsVersionByte) {
    ZoneCounts counts;
    std::vector<std::uint8_t> buffer;
    serialize_zone_counts(counts, buffer);

    EXPECT_EQ(buffer[0], ZONE_SERIALIZATION_VERSION);
    // Total: version(1) + 11 * uint32(4) = 45 bytes (includes aeroport/aquaport counts)
    EXPECT_EQ(buffer.size(), 45u);
}

// ============================================================================
// ZoneDemandData Serialization Tests
// ============================================================================

TEST(ZoneSerializationTest, ZoneDemandDataRoundTrip) {
    ZoneDemandData original;
    original.habitation_demand = 75;
    original.exchange_demand = -50;
    original.fabrication_demand = 100;

    std::vector<std::uint8_t> buffer;
    serialize_zone_demand_data(original, buffer);

    ZoneDemandData deserialized = deserialize_zone_demand_data(buffer.data(), buffer.size());

    EXPECT_EQ(deserialized.habitation_demand, 75);
    EXPECT_EQ(deserialized.exchange_demand, -50);
    EXPECT_EQ(deserialized.fabrication_demand, 100);
}

TEST(ZoneSerializationTest, ZoneDemandDataNegativeValues) {
    ZoneDemandData original;
    original.habitation_demand = -100;
    original.exchange_demand = -100;
    original.fabrication_demand = -100;

    std::vector<std::uint8_t> buffer;
    serialize_zone_demand_data(original, buffer);

    ZoneDemandData deserialized = deserialize_zone_demand_data(buffer.data(), buffer.size());

    EXPECT_EQ(deserialized.habitation_demand, -100);
    EXPECT_EQ(deserialized.exchange_demand, -100);
    EXPECT_EQ(deserialized.fabrication_demand, -100);
}

TEST(ZoneSerializationTest, ZoneDemandDataVersionByte) {
    ZoneDemandData demand;
    std::vector<std::uint8_t> buffer;
    serialize_zone_demand_data(demand, buffer);

    EXPECT_EQ(buffer[0], ZONE_SERIALIZATION_VERSION);
    // Total: version(1) + 3 * int8(1) = 4 bytes
    EXPECT_EQ(buffer.size(), 4u);
}

// ============================================================================
// Error Handling Tests
// ============================================================================

TEST(ZoneSerializationTest, ZoneComponentTooSmallBuffer) {
    std::uint8_t small_buf[2] = {1, 0};
    EXPECT_THROW(deserialize_zone_component(small_buf, 2), std::runtime_error);
}

TEST(ZoneSerializationTest, ZoneGridTooSmallBuffer) {
    std::uint8_t small_buf[4] = {1, 0, 0, 0};
    EXPECT_THROW(deserialize_zone_grid(small_buf, 4), std::runtime_error);
}

TEST(ZoneSerializationTest, ZoneCountsTooSmallBuffer) {
    std::uint8_t small_buf[10] = {};
    // Minimum required: 45 bytes (version + 11 uint32 fields including port counts)
    EXPECT_THROW(deserialize_zone_counts(small_buf, 10), std::runtime_error);
}

TEST(ZoneSerializationTest, ZoneDemandDataTooSmallBuffer) {
    std::uint8_t small_buf[2] = {1, 0};
    EXPECT_THROW(deserialize_zone_demand_data(small_buf, 2), std::runtime_error);
}
