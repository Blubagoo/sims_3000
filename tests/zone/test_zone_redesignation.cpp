/**
 * @file test_zone_redesignation.cpp
 * @brief Tests for zone redesignation (Ticket 4-014)
 *
 * Tests:
 * - Redesignate designated zone (type change)
 * - Redesignate designated zone (density change)
 * - Redesignate designated zone (both type and density change)
 * - Redesignate stalled zone
 * - Redesignate occupied zone with type change (emits DemolitionRequestEvent)
 * - Redesignate occupied zone with density-only change (CCR-005, direct update)
 * - Same type and density returns SameTypeAndDensity
 * - No zone at position returns NoZoneAtPosition
 * - Wrong owner returns NotOwned
 * - ZoneCounts updated correctly
 * - Multiple redesignations
 */

#include <gtest/gtest.h>
#include <sims3000/zone/ZoneSystem.h>

namespace sims3000 {
namespace zone {
namespace {

// ============================================================================
// Test Fixture
// ============================================================================

class ZoneRedesignationTest : public ::testing::Test {
protected:
    void SetUp() override {
        system = std::make_unique<ZoneSystem>(nullptr, nullptr, 128);
    }

    /// Helper to place a zone and return the entity_id
    std::uint32_t place_at(std::int32_t x, std::int32_t y,
                            ZoneType type = ZoneType::Habitation,
                            ZoneDensity density = ZoneDensity::LowDensity,
                            std::uint8_t player_id = 0,
                            std::uint32_t entity_id = 0) {
        static std::uint32_t next_id = 100;
        if (entity_id == 0) entity_id = next_id++;
        system->place_zone(x, y, type, density, player_id, entity_id);
        return entity_id;
    }

    std::unique_ptr<ZoneSystem> system;
};

// ============================================================================
// No zone at position
// ============================================================================

TEST_F(ZoneRedesignationTest, NoZoneAtPosition) {
    auto result = system->redesignate_zone(10, 10, ZoneType::Exchange, ZoneDensity::HighDensity, 0);
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.reason, RedesignateResult::Reason::NoZoneAtPosition);
    EXPECT_FALSE(result.demolition_requested);
}

// ============================================================================
// Wrong owner
// ============================================================================

TEST_F(ZoneRedesignationTest, WrongOwner) {
    place_at(10, 10, ZoneType::Habitation, ZoneDensity::LowDensity, 0);

    auto result = system->redesignate_zone(10, 10, ZoneType::Exchange, ZoneDensity::HighDensity, 1);
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.reason, RedesignateResult::Reason::NotOwned);
    EXPECT_FALSE(result.demolition_requested);
}

// ============================================================================
// Same type and density
// ============================================================================

TEST_F(ZoneRedesignationTest, SameTypeAndDensity) {
    place_at(10, 10, ZoneType::Habitation, ZoneDensity::LowDensity, 0);

    auto result = system->redesignate_zone(10, 10, ZoneType::Habitation, ZoneDensity::LowDensity, 0);
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.reason, RedesignateResult::Reason::SameTypeAndDensity);
    EXPECT_FALSE(result.demolition_requested);
}

// ============================================================================
// Redesignate Designated zone - type change
// ============================================================================

TEST_F(ZoneRedesignationTest, DesignatedZone_TypeChange) {
    place_at(10, 10, ZoneType::Habitation, ZoneDensity::LowDensity, 0);

    auto result = system->redesignate_zone(10, 10, ZoneType::Exchange, ZoneDensity::LowDensity, 0);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.reason, RedesignateResult::Reason::Ok);
    EXPECT_FALSE(result.demolition_requested);

    // Verify the zone was updated
    ZoneType type;
    ASSERT_TRUE(system->get_zone_type(10, 10, type));
    EXPECT_EQ(type, ZoneType::Exchange);
}

// ============================================================================
// Redesignate Designated zone - density change
// ============================================================================

TEST_F(ZoneRedesignationTest, DesignatedZone_DensityChange) {
    place_at(10, 10, ZoneType::Habitation, ZoneDensity::LowDensity, 0);

    auto result = system->redesignate_zone(10, 10, ZoneType::Habitation, ZoneDensity::HighDensity, 0);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.reason, RedesignateResult::Reason::Ok);

    ZoneDensity density;
    ASSERT_TRUE(system->get_zone_density(10, 10, density));
    EXPECT_EQ(density, ZoneDensity::HighDensity);
}

// ============================================================================
// Redesignate Designated zone - both type and density change
// ============================================================================

TEST_F(ZoneRedesignationTest, DesignatedZone_BothChange) {
    place_at(10, 10, ZoneType::Habitation, ZoneDensity::LowDensity, 0);

    auto result = system->redesignate_zone(10, 10, ZoneType::Fabrication, ZoneDensity::HighDensity, 0);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.reason, RedesignateResult::Reason::Ok);

    ZoneType type;
    ZoneDensity density;
    ASSERT_TRUE(system->get_zone_type(10, 10, type));
    ASSERT_TRUE(system->get_zone_density(10, 10, density));
    EXPECT_EQ(type, ZoneType::Fabrication);
    EXPECT_EQ(density, ZoneDensity::HighDensity);
}

// ============================================================================
// Redesignate Stalled zone
// ============================================================================

TEST_F(ZoneRedesignationTest, StalledZone_TypeChange) {
    place_at(10, 10, ZoneType::Habitation, ZoneDensity::LowDensity, 0);
    ASSERT_TRUE(system->set_zone_state(10, 10, ZoneState::Stalled));

    auto result = system->redesignate_zone(10, 10, ZoneType::Exchange, ZoneDensity::HighDensity, 0);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.reason, RedesignateResult::Reason::Ok);

    ZoneType type;
    ZoneDensity density;
    ASSERT_TRUE(system->get_zone_type(10, 10, type));
    ASSERT_TRUE(system->get_zone_density(10, 10, density));
    EXPECT_EQ(type, ZoneType::Exchange);
    EXPECT_EQ(density, ZoneDensity::HighDensity);
}

// ============================================================================
// Redesignate Occupied zone - type change (emits DemolitionRequestEvent)
// ============================================================================

TEST_F(ZoneRedesignationTest, OccupiedZone_TypeChange_EmitsDemolition) {
    std::uint32_t eid = place_at(10, 10, ZoneType::Habitation, ZoneDensity::LowDensity, 0);
    ASSERT_TRUE(system->set_zone_state(10, 10, ZoneState::Occupied));

    system->clear_pending_demolition_events();
    auto result = system->redesignate_zone(10, 10, ZoneType::Exchange, ZoneDensity::LowDensity, 0);

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.reason, RedesignateResult::Reason::OccupiedRequiresDemolition);
    EXPECT_TRUE(result.demolition_requested);

    // DemolitionRequestEvent should be emitted
    const auto& events = system->get_pending_demolition_events();
    ASSERT_EQ(events.size(), 1u);
    EXPECT_EQ(events[0].grid_x, 10);
    EXPECT_EQ(events[0].grid_y, 10);
    EXPECT_EQ(events[0].requesting_entity_id, eid);

    // Zone should NOT be modified yet
    ZoneType type;
    ASSERT_TRUE(system->get_zone_type(10, 10, type));
    EXPECT_EQ(type, ZoneType::Habitation);
}

// ============================================================================
// Redesignate Occupied zone - density-only change (CCR-005)
// ============================================================================

TEST_F(ZoneRedesignationTest, OccupiedZone_DensityOnlyChange_DirectUpdate) {
    place_at(10, 10, ZoneType::Habitation, ZoneDensity::LowDensity, 0);
    ASSERT_TRUE(system->set_zone_state(10, 10, ZoneState::Occupied));

    system->clear_pending_demolition_events();
    auto result = system->redesignate_zone(10, 10, ZoneType::Habitation, ZoneDensity::HighDensity, 0);

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.reason, RedesignateResult::Reason::Ok);
    EXPECT_FALSE(result.demolition_requested);

    // No demolition event should be emitted
    EXPECT_EQ(system->get_pending_demolition_events().size(), 0u);

    // Density should be updated
    ZoneDensity density;
    ASSERT_TRUE(system->get_zone_density(10, 10, density));
    EXPECT_EQ(density, ZoneDensity::HighDensity);
}

// ============================================================================
// ZoneCounts updated correctly - type change
// ============================================================================

TEST_F(ZoneRedesignationTest, ZoneCountsUpdated_TypeChange) {
    place_at(10, 10, ZoneType::Habitation, ZoneDensity::LowDensity, 0);

    EXPECT_EQ(system->get_zone_count(0, ZoneType::Habitation), 1u);
    EXPECT_EQ(system->get_zone_count(0, ZoneType::Exchange), 0u);

    system->redesignate_zone(10, 10, ZoneType::Exchange, ZoneDensity::LowDensity, 0);

    EXPECT_EQ(system->get_zone_count(0, ZoneType::Habitation), 0u);
    EXPECT_EQ(system->get_zone_count(0, ZoneType::Exchange), 1u);

    // Total should not change
    EXPECT_EQ(system->get_zone_counts(0).total, 1u);
}

// ============================================================================
// ZoneCounts updated correctly - density change
// ============================================================================

TEST_F(ZoneRedesignationTest, ZoneCountsUpdated_DensityChange) {
    place_at(10, 10, ZoneType::Habitation, ZoneDensity::LowDensity, 0);

    EXPECT_EQ(system->get_zone_counts(0).low_density_total, 1u);
    EXPECT_EQ(system->get_zone_counts(0).high_density_total, 0u);

    system->redesignate_zone(10, 10, ZoneType::Habitation, ZoneDensity::HighDensity, 0);

    EXPECT_EQ(system->get_zone_counts(0).low_density_total, 0u);
    EXPECT_EQ(system->get_zone_counts(0).high_density_total, 1u);
}

// ============================================================================
// ZoneCounts updated - occupied density-only change
// ============================================================================

TEST_F(ZoneRedesignationTest, ZoneCountsUpdated_OccupiedDensityChange) {
    place_at(10, 10, ZoneType::Habitation, ZoneDensity::LowDensity, 0);
    system->set_zone_state(10, 10, ZoneState::Occupied);

    EXPECT_EQ(system->get_zone_counts(0).low_density_total, 1u);
    EXPECT_EQ(system->get_zone_counts(0).high_density_total, 0u);

    system->redesignate_zone(10, 10, ZoneType::Habitation, ZoneDensity::HighDensity, 0);

    EXPECT_EQ(system->get_zone_counts(0).low_density_total, 0u);
    EXPECT_EQ(system->get_zone_counts(0).high_density_total, 1u);
}

// ============================================================================
// Multiple redesignations
// ============================================================================

TEST_F(ZoneRedesignationTest, MultipleRedesignations) {
    place_at(10, 10, ZoneType::Habitation, ZoneDensity::LowDensity, 0);

    // First redesignation: Habitation -> Exchange
    auto r1 = system->redesignate_zone(10, 10, ZoneType::Exchange, ZoneDensity::LowDensity, 0);
    EXPECT_TRUE(r1.success);

    // Second redesignation: Exchange -> Fabrication
    auto r2 = system->redesignate_zone(10, 10, ZoneType::Fabrication, ZoneDensity::HighDensity, 0);
    EXPECT_TRUE(r2.success);

    ZoneType type;
    ZoneDensity density;
    ASSERT_TRUE(system->get_zone_type(10, 10, type));
    ASSERT_TRUE(system->get_zone_density(10, 10, density));
    EXPECT_EQ(type, ZoneType::Fabrication);
    EXPECT_EQ(density, ZoneDensity::HighDensity);

    EXPECT_EQ(system->get_zone_count(0, ZoneType::Habitation), 0u);
    EXPECT_EQ(system->get_zone_count(0, ZoneType::Exchange), 0u);
    EXPECT_EQ(system->get_zone_count(0, ZoneType::Fabrication), 1u);
}

// ============================================================================
// Occupied type change does not modify zone
// ============================================================================

TEST_F(ZoneRedesignationTest, OccupiedTypeChange_DoesNotModifyZoneCounts) {
    place_at(10, 10, ZoneType::Habitation, ZoneDensity::LowDensity, 0);
    system->set_zone_state(10, 10, ZoneState::Occupied);

    std::uint32_t hab_before = system->get_zone_count(0, ZoneType::Habitation);
    std::uint32_t total_before = system->get_zone_counts(0).total;

    system->redesignate_zone(10, 10, ZoneType::Exchange, ZoneDensity::LowDensity, 0);

    // Counts should NOT change for occupied zones with type change
    EXPECT_EQ(system->get_zone_count(0, ZoneType::Habitation), hab_before);
    EXPECT_EQ(system->get_zone_counts(0).total, total_before);
}

} // anonymous namespace
} // namespace zone
} // namespace sims3000
