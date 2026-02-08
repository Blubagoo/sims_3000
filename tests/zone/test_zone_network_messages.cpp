/**
 * @file test_zone_network_messages.cpp
 * @brief Tests for zone network message serialization (Ticket 4-038)
 *
 * Tests round-trip serialization for all four message types:
 * - ZonePlacementRequestMsg (3 tests)
 * - DezoneRequestMsg (3 tests)
 * - RedesignateRequestMsg (3 tests)
 * - ZoneDemandSyncMsg (3 tests)
 *
 * Each message type has:
 * - Default values round-trip
 * - Non-trivial values round-trip
 * - Truncated data fails gracefully
 */

#include <gtest/gtest.h>
#include <sims3000/zone/ZoneNetworkMessages.h>

namespace sims3000 {
namespace zone {
namespace {

// ============================================================================
// ZonePlacementRequestMsg tests
// ============================================================================

TEST(ZoneNetworkMessagesTest, PlacementRequest_DefaultRoundTrip) {
    ZonePlacementRequestMsg msg;
    auto data = msg.serialize();
    EXPECT_EQ(data.size(), 19u);

    ZonePlacementRequestMsg out;
    EXPECT_TRUE(ZonePlacementRequestMsg::deserialize(data, out));
    EXPECT_EQ(out.x, 0);
    EXPECT_EQ(out.y, 0);
    EXPECT_EQ(out.width, 0);
    EXPECT_EQ(out.height, 0);
    EXPECT_EQ(out.zone_type, 0);
    EXPECT_EQ(out.density, 0);
    EXPECT_EQ(out.version, 1);
}

TEST(ZoneNetworkMessagesTest, PlacementRequest_ValuesRoundTrip) {
    ZonePlacementRequestMsg msg;
    msg.x = 42;
    msg.y = -10;
    msg.width = 5;
    msg.height = 3;
    msg.zone_type = 2;  // Fabrication
    msg.density = 1;    // HighDensity

    auto data = msg.serialize();
    EXPECT_EQ(data.size(), 19u);

    ZonePlacementRequestMsg out;
    EXPECT_TRUE(ZonePlacementRequestMsg::deserialize(data, out));
    EXPECT_EQ(out.x, 42);
    EXPECT_EQ(out.y, -10);
    EXPECT_EQ(out.width, 5);
    EXPECT_EQ(out.height, 3);
    EXPECT_EQ(out.zone_type, 2);
    EXPECT_EQ(out.density, 1);
    EXPECT_EQ(out.version, 1);
}

TEST(ZoneNetworkMessagesTest, PlacementRequest_TruncatedDataFails) {
    std::vector<std::uint8_t> data = {1, 0, 0}; // Only 3 bytes, need 19
    ZonePlacementRequestMsg out;
    EXPECT_FALSE(ZonePlacementRequestMsg::deserialize(data, out));
}

TEST(ZoneNetworkMessagesTest, PlacementRequest_EmptyDataFails) {
    std::vector<std::uint8_t> data;
    ZonePlacementRequestMsg out;
    EXPECT_FALSE(ZonePlacementRequestMsg::deserialize(data, out));
}

// ============================================================================
// DezoneRequestMsg tests
// ============================================================================

TEST(ZoneNetworkMessagesTest, DezoneRequest_DefaultRoundTrip) {
    DezoneRequestMsg msg;
    auto data = msg.serialize();
    EXPECT_EQ(data.size(), 17u);

    DezoneRequestMsg out;
    EXPECT_TRUE(DezoneRequestMsg::deserialize(data, out));
    EXPECT_EQ(out.x, 0);
    EXPECT_EQ(out.y, 0);
    EXPECT_EQ(out.width, 0);
    EXPECT_EQ(out.height, 0);
    EXPECT_EQ(out.version, 1);
}

TEST(ZoneNetworkMessagesTest, DezoneRequest_ValuesRoundTrip) {
    DezoneRequestMsg msg;
    msg.x = 100;
    msg.y = 200;
    msg.width = 10;
    msg.height = 20;

    auto data = msg.serialize();
    EXPECT_EQ(data.size(), 17u);

    DezoneRequestMsg out;
    EXPECT_TRUE(DezoneRequestMsg::deserialize(data, out));
    EXPECT_EQ(out.x, 100);
    EXPECT_EQ(out.y, 200);
    EXPECT_EQ(out.width, 10);
    EXPECT_EQ(out.height, 20);
    EXPECT_EQ(out.version, 1);
}

TEST(ZoneNetworkMessagesTest, DezoneRequest_NegativeCoordinates) {
    DezoneRequestMsg msg;
    msg.x = -50;
    msg.y = -100;
    msg.width = 1;
    msg.height = 1;

    auto data = msg.serialize();

    DezoneRequestMsg out;
    EXPECT_TRUE(DezoneRequestMsg::deserialize(data, out));
    EXPECT_EQ(out.x, -50);
    EXPECT_EQ(out.y, -100);
}

TEST(ZoneNetworkMessagesTest, DezoneRequest_TruncatedDataFails) {
    std::vector<std::uint8_t> data = {1, 0};
    DezoneRequestMsg out;
    EXPECT_FALSE(DezoneRequestMsg::deserialize(data, out));
}

// ============================================================================
// RedesignateRequestMsg tests
// ============================================================================

TEST(ZoneNetworkMessagesTest, RedesignateRequest_DefaultRoundTrip) {
    RedesignateRequestMsg msg;
    auto data = msg.serialize();
    EXPECT_EQ(data.size(), 11u);

    RedesignateRequestMsg out;
    EXPECT_TRUE(RedesignateRequestMsg::deserialize(data, out));
    EXPECT_EQ(out.x, 0);
    EXPECT_EQ(out.y, 0);
    EXPECT_EQ(out.new_zone_type, 0);
    EXPECT_EQ(out.new_density, 0);
    EXPECT_EQ(out.version, 1);
}

TEST(ZoneNetworkMessagesTest, RedesignateRequest_ValuesRoundTrip) {
    RedesignateRequestMsg msg;
    msg.x = 75;
    msg.y = -25;
    msg.new_zone_type = 1;  // Exchange
    msg.new_density = 1;    // HighDensity

    auto data = msg.serialize();
    EXPECT_EQ(data.size(), 11u);

    RedesignateRequestMsg out;
    EXPECT_TRUE(RedesignateRequestMsg::deserialize(data, out));
    EXPECT_EQ(out.x, 75);
    EXPECT_EQ(out.y, -25);
    EXPECT_EQ(out.new_zone_type, 1);
    EXPECT_EQ(out.new_density, 1);
    EXPECT_EQ(out.version, 1);
}

TEST(ZoneNetworkMessagesTest, RedesignateRequest_TruncatedDataFails) {
    std::vector<std::uint8_t> data = {1, 0, 0, 0, 0};
    RedesignateRequestMsg out;
    EXPECT_FALSE(RedesignateRequestMsg::deserialize(data, out));
}

// ============================================================================
// ZoneDemandSyncMsg tests
// ============================================================================

TEST(ZoneNetworkMessagesTest, DemandSync_DefaultRoundTrip) {
    ZoneDemandSyncMsg msg;
    auto data = msg.serialize();
    EXPECT_EQ(data.size(), 5u);

    ZoneDemandSyncMsg out;
    EXPECT_TRUE(ZoneDemandSyncMsg::deserialize(data, out));
    EXPECT_EQ(out.player_id, 0);
    EXPECT_EQ(out.habitation_demand, 0);
    EXPECT_EQ(out.exchange_demand, 0);
    EXPECT_EQ(out.fabrication_demand, 0);
    EXPECT_EQ(out.version, 1);
}

TEST(ZoneNetworkMessagesTest, DemandSync_PositiveValues) {
    ZoneDemandSyncMsg msg;
    msg.player_id = 3;
    msg.habitation_demand = 50;
    msg.exchange_demand = 25;
    msg.fabrication_demand = 100;

    auto data = msg.serialize();
    EXPECT_EQ(data.size(), 5u);

    ZoneDemandSyncMsg out;
    EXPECT_TRUE(ZoneDemandSyncMsg::deserialize(data, out));
    EXPECT_EQ(out.player_id, 3);
    EXPECT_EQ(out.habitation_demand, 50);
    EXPECT_EQ(out.exchange_demand, 25);
    EXPECT_EQ(out.fabrication_demand, 100);
    EXPECT_EQ(out.version, 1);
}

TEST(ZoneNetworkMessagesTest, DemandSync_NegativeValues) {
    ZoneDemandSyncMsg msg;
    msg.player_id = 0;
    msg.habitation_demand = -100;
    msg.exchange_demand = -50;
    msg.fabrication_demand = -1;

    auto data = msg.serialize();

    ZoneDemandSyncMsg out;
    EXPECT_TRUE(ZoneDemandSyncMsg::deserialize(data, out));
    EXPECT_EQ(out.habitation_demand, -100);
    EXPECT_EQ(out.exchange_demand, -50);
    EXPECT_EQ(out.fabrication_demand, -1);
}

TEST(ZoneNetworkMessagesTest, DemandSync_TruncatedDataFails) {
    std::vector<std::uint8_t> data = {1, 0};
    ZoneDemandSyncMsg out;
    EXPECT_FALSE(ZoneDemandSyncMsg::deserialize(data, out));
}

// ============================================================================
// Large coordinate values
// ============================================================================

TEST(ZoneNetworkMessagesTest, PlacementRequest_LargeCoordinates) {
    ZonePlacementRequestMsg msg;
    msg.x = 2147483647;   // INT32_MAX
    msg.y = -2147483647;  // near INT32_MIN
    msg.width = 512;
    msg.height = 512;
    msg.zone_type = 2;
    msg.density = 1;

    auto data = msg.serialize();

    ZonePlacementRequestMsg out;
    EXPECT_TRUE(ZonePlacementRequestMsg::deserialize(data, out));
    EXPECT_EQ(out.x, 2147483647);
    EXPECT_EQ(out.y, -2147483647);
    EXPECT_EQ(out.width, 512);
    EXPECT_EQ(out.height, 512);
}

// ============================================================================
// Version field preserved
// ============================================================================

TEST(ZoneNetworkMessagesTest, VersionFieldPreserved) {
    // All messages should have version 1 by default
    ZonePlacementRequestMsg pm;
    EXPECT_EQ(pm.version, 1);

    DezoneRequestMsg dm;
    EXPECT_EQ(dm.version, 1);

    RedesignateRequestMsg rm;
    EXPECT_EQ(rm.version, 1);

    ZoneDemandSyncMsg sm;
    EXPECT_EQ(sm.version, 1);
}

} // anonymous namespace
} // namespace zone
} // namespace sims3000
