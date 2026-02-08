/**
 * @file test_zone_server_handler.cpp
 * @brief Tests for ZoneServerHandler (Ticket 4-039)
 *
 * Verifies:
 * - Placement via handler delegates to ZoneSystem
 * - Dezone via handler delegates to ZoneSystem
 * - Redesignation via handler delegates to ZoneSystem
 * - Invalid player ID rejection
 * - Handler response contains correct counts
 * - Invalid area dimensions rejected
 * - Null zone system handled gracefully
 */

#include <gtest/gtest.h>
#include <sims3000/zone/ZoneServerHandler.h>
#include <sims3000/zone/ZoneSystem.h>
#include <sims3000/zone/ZoneNetworkMessages.h>

using namespace sims3000::zone;

namespace {

class ZoneServerHandlerTest : public ::testing::Test {
protected:
    void SetUp() override {
        zone_system = std::make_unique<ZoneSystem>(nullptr, nullptr, 128);
        handler = std::make_unique<ZoneServerHandler>(zone_system.get());
    }

    std::unique_ptr<ZoneSystem> zone_system;
    std::unique_ptr<ZoneServerHandler> handler;
};

// =========================================================================
// Placement Via Handler
// =========================================================================

TEST_F(ZoneServerHandlerTest, PlacementRequestSucceeds) {
    ZonePlacementRequestMsg msg;
    msg.x = 10;
    msg.y = 10;
    msg.width = 3;
    msg.height = 3;
    msg.zone_type = static_cast<uint8_t>(ZoneType::Habitation);
    msg.density = static_cast<uint8_t>(ZoneDensity::LowDensity);

    auto response = handler->handle_placement_request(msg, 0);

    EXPECT_TRUE(response.success);
    EXPECT_EQ(response.placed_count, 9u); // 3x3 = 9 zones
    EXPECT_TRUE(response.rejection_reason.empty());
}

TEST_F(ZoneServerHandlerTest, PlacementRequestSingleTile) {
    ZonePlacementRequestMsg msg;
    msg.x = 50;
    msg.y = 50;
    msg.width = 1;
    msg.height = 1;
    msg.zone_type = static_cast<uint8_t>(ZoneType::Exchange);
    msg.density = static_cast<uint8_t>(ZoneDensity::HighDensity);

    auto response = handler->handle_placement_request(msg, 1);

    EXPECT_TRUE(response.success);
    EXPECT_EQ(response.placed_count, 1u);
}

TEST_F(ZoneServerHandlerTest, PlacementVerifiesZoneInSystem) {
    ZonePlacementRequestMsg msg;
    msg.x = 20;
    msg.y = 20;
    msg.width = 2;
    msg.height = 2;
    msg.zone_type = static_cast<uint8_t>(ZoneType::Fabrication);
    msg.density = static_cast<uint8_t>(ZoneDensity::LowDensity);

    handler->handle_placement_request(msg, 0);

    // Verify zones are actually in the zone system
    EXPECT_TRUE(zone_system->is_zoned(20, 20));
    EXPECT_TRUE(zone_system->is_zoned(21, 20));
    EXPECT_TRUE(zone_system->is_zoned(20, 21));
    EXPECT_TRUE(zone_system->is_zoned(21, 21));
}

// =========================================================================
// Dezone Via Handler
// =========================================================================

TEST_F(ZoneServerHandlerTest, DezoneRequestSucceeds) {
    // First place some zones
    ZonePlacementRequestMsg placement_msg;
    placement_msg.x = 30;
    placement_msg.y = 30;
    placement_msg.width = 3;
    placement_msg.height = 3;
    placement_msg.zone_type = static_cast<uint8_t>(ZoneType::Habitation);
    placement_msg.density = static_cast<uint8_t>(ZoneDensity::LowDensity);
    handler->handle_placement_request(placement_msg, 0);

    // Now dezone them
    DezoneRequestMsg dezone_msg;
    dezone_msg.x = 30;
    dezone_msg.y = 30;
    dezone_msg.width = 3;
    dezone_msg.height = 3;

    auto response = handler->handle_dezone_request(dezone_msg, 0);

    EXPECT_TRUE(response.success);
    EXPECT_EQ(response.removed_count, 9u);
}

TEST_F(ZoneServerHandlerTest, DezoneEmptyAreaReturnsFailure) {
    DezoneRequestMsg msg;
    msg.x = 50;
    msg.y = 50;
    msg.width = 2;
    msg.height = 2;

    auto response = handler->handle_dezone_request(msg, 0);

    EXPECT_FALSE(response.success);
    EXPECT_EQ(response.removed_count, 0u);
}

// =========================================================================
// Redesignation Via Handler
// =========================================================================

TEST_F(ZoneServerHandlerTest, RedesignateRequestSucceeds) {
    // Place a zone first
    zone_system->place_zone(40, 40, ZoneType::Habitation, ZoneDensity::LowDensity, 0, 1);

    RedesignateRequestMsg msg;
    msg.x = 40;
    msg.y = 40;
    msg.new_zone_type = static_cast<uint8_t>(ZoneType::Exchange);
    msg.new_density = static_cast<uint8_t>(ZoneDensity::LowDensity);

    auto response = handler->handle_redesignate_request(msg, 0);

    EXPECT_TRUE(response.success);
    EXPECT_TRUE(response.rejection_reason.empty());

    // Verify type changed
    ZoneType zone_type;
    zone_system->get_zone_type(40, 40, zone_type);
    EXPECT_EQ(zone_type, ZoneType::Exchange);
}

TEST_F(ZoneServerHandlerTest, RedesignateNoZoneAtPosition) {
    RedesignateRequestMsg msg;
    msg.x = 99;
    msg.y = 99;
    msg.new_zone_type = static_cast<uint8_t>(ZoneType::Exchange);
    msg.new_density = static_cast<uint8_t>(ZoneDensity::LowDensity);

    auto response = handler->handle_redesignate_request(msg, 0);

    EXPECT_FALSE(response.success);
    EXPECT_FALSE(response.rejection_reason.empty());
}

// =========================================================================
// Invalid Player ID Rejection
// =========================================================================

TEST_F(ZoneServerHandlerTest, PlacementRejectsInvalidPlayerId) {
    ZonePlacementRequestMsg msg;
    msg.x = 10;
    msg.y = 10;
    msg.width = 1;
    msg.height = 1;
    msg.zone_type = static_cast<uint8_t>(ZoneType::Habitation);
    msg.density = static_cast<uint8_t>(ZoneDensity::LowDensity);

    auto response = handler->handle_placement_request(msg, MAX_OVERSEERS); // Invalid

    EXPECT_FALSE(response.success);
    EXPECT_EQ(response.rejection_reason, "Invalid player ID");
}

TEST_F(ZoneServerHandlerTest, DezoneRejectsInvalidPlayerId) {
    DezoneRequestMsg msg;
    msg.x = 10;
    msg.y = 10;
    msg.width = 1;
    msg.height = 1;

    auto response = handler->handle_dezone_request(msg, 255); // Invalid

    EXPECT_FALSE(response.success);
    EXPECT_EQ(response.rejection_reason, "Invalid player ID");
}

TEST_F(ZoneServerHandlerTest, RedesignateRejectsInvalidPlayerId) {
    RedesignateRequestMsg msg;
    msg.x = 10;
    msg.y = 10;
    msg.new_zone_type = static_cast<uint8_t>(ZoneType::Exchange);
    msg.new_density = static_cast<uint8_t>(ZoneDensity::LowDensity);

    auto response = handler->handle_redesignate_request(msg, MAX_OVERSEERS);

    EXPECT_FALSE(response.success);
    EXPECT_EQ(response.rejection_reason, "Invalid player ID");
}

// =========================================================================
// Invalid Area Dimensions Rejected
// =========================================================================

TEST_F(ZoneServerHandlerTest, PlacementRejectsZeroWidth) {
    ZonePlacementRequestMsg msg;
    msg.x = 10;
    msg.y = 10;
    msg.width = 0;
    msg.height = 1;
    msg.zone_type = static_cast<uint8_t>(ZoneType::Habitation);
    msg.density = static_cast<uint8_t>(ZoneDensity::LowDensity);

    auto response = handler->handle_placement_request(msg, 0);

    EXPECT_FALSE(response.success);
    EXPECT_EQ(response.rejection_reason, "Invalid area dimensions");
}

TEST_F(ZoneServerHandlerTest, DezoneRejectsNegativeHeight) {
    DezoneRequestMsg msg;
    msg.x = 10;
    msg.y = 10;
    msg.width = 1;
    msg.height = -1;

    auto response = handler->handle_dezone_request(msg, 0);

    EXPECT_FALSE(response.success);
    EXPECT_EQ(response.rejection_reason, "Invalid area dimensions");
}

// =========================================================================
// Null Zone System
// =========================================================================

TEST_F(ZoneServerHandlerTest, NullZoneSystemHandledGracefully) {
    auto null_handler = std::make_unique<ZoneServerHandler>(nullptr);

    ZonePlacementRequestMsg placement_msg;
    placement_msg.x = 10;
    placement_msg.y = 10;
    placement_msg.width = 1;
    placement_msg.height = 1;
    placement_msg.zone_type = 0;
    placement_msg.density = 0;

    auto response = null_handler->handle_placement_request(placement_msg, 0);
    EXPECT_FALSE(response.success);
    EXPECT_EQ(response.rejection_reason, "Zone system unavailable");

    DezoneRequestMsg dezone_msg;
    dezone_msg.x = 10;
    dezone_msg.y = 10;
    dezone_msg.width = 1;
    dezone_msg.height = 1;

    response = null_handler->handle_dezone_request(dezone_msg, 0);
    EXPECT_FALSE(response.success);

    RedesignateRequestMsg redesig_msg;
    redesig_msg.x = 10;
    redesig_msg.y = 10;
    redesig_msg.new_zone_type = 0;
    redesig_msg.new_density = 0;

    response = null_handler->handle_redesignate_request(redesig_msg, 0);
    EXPECT_FALSE(response.success);
}

// =========================================================================
// Max Valid Player ID Accepted
// =========================================================================

TEST_F(ZoneServerHandlerTest, MaxValidPlayerIdAccepted) {
    ZonePlacementRequestMsg msg;
    msg.x = 10;
    msg.y = 10;
    msg.width = 1;
    msg.height = 1;
    msg.zone_type = static_cast<uint8_t>(ZoneType::Habitation);
    msg.density = static_cast<uint8_t>(ZoneDensity::LowDensity);

    // MAX_OVERSEERS - 1 is the last valid player ID
    auto response = handler->handle_placement_request(msg, MAX_OVERSEERS - 1);

    EXPECT_TRUE(response.success);
    EXPECT_EQ(response.placed_count, 1u);
}

} // anonymous namespace
