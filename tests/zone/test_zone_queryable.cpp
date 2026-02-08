/**
 * @file test_zone_queryable.cpp
 * @brief Tests for IZoneQueryable interface implementation (Ticket 4-035)
 *
 * Tests:
 * - get_zone_type_at delegates to ZoneSystem
 * - get_zone_density_at delegates to ZoneSystem
 * - is_zoned_at delegates to ZoneSystem
 * - get_zone_count_for delegates to ZoneSystem
 * - get_designated_zones returns correct positions
 * - get_designated_zones filters by player_id and type
 * - get_designated_zones excludes Occupied and Stalled zones
 * - get_demand_for returns demand as float
 * - Interface pointer polymorphism works
 */

#include <gtest/gtest.h>
#include <sims3000/zone/ZoneSystem.h>
#include <sims3000/zone/IZoneQueryable.h>

namespace sims3000 {
namespace zone {
namespace {

// ============================================================================
// Test Fixture
// ============================================================================

class ZoneQueryableTest : public ::testing::Test {
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
        static std::uint32_t next_id = 200;
        if (entity_id == 0) entity_id = next_id++;
        system->place_zone(x, y, type, density, player_id, entity_id);
        return entity_id;
    }

    /// Get the system as IZoneQueryable* for interface testing
    IZoneQueryable* queryable() { return system.get(); }

    std::unique_ptr<ZoneSystem> system;
};

// ============================================================================
// get_zone_type_at
// ============================================================================

TEST_F(ZoneQueryableTest, GetZoneTypeAt_ReturnsType) {
    place_at(10, 10, ZoneType::Exchange);

    ZoneType type;
    EXPECT_TRUE(queryable()->get_zone_type_at(10, 10, type));
    EXPECT_EQ(type, ZoneType::Exchange);
}

TEST_F(ZoneQueryableTest, GetZoneTypeAt_NoZone) {
    ZoneType type;
    EXPECT_FALSE(queryable()->get_zone_type_at(10, 10, type));
}

// ============================================================================
// get_zone_density_at
// ============================================================================

TEST_F(ZoneQueryableTest, GetZoneDensityAt_ReturnsDensity) {
    place_at(10, 10, ZoneType::Habitation, ZoneDensity::HighDensity);

    ZoneDensity density;
    EXPECT_TRUE(queryable()->get_zone_density_at(10, 10, density));
    EXPECT_EQ(density, ZoneDensity::HighDensity);
}

TEST_F(ZoneQueryableTest, GetZoneDensityAt_NoZone) {
    ZoneDensity density;
    EXPECT_FALSE(queryable()->get_zone_density_at(10, 10, density));
}

// ============================================================================
// is_zoned_at
// ============================================================================

TEST_F(ZoneQueryableTest, IsZonedAt_True) {
    place_at(10, 10);
    EXPECT_TRUE(queryable()->is_zoned_at(10, 10));
}

TEST_F(ZoneQueryableTest, IsZonedAt_False) {
    EXPECT_FALSE(queryable()->is_zoned_at(10, 10));
}

// ============================================================================
// get_zone_count_for
// ============================================================================

TEST_F(ZoneQueryableTest, GetZoneCountFor_ReturnsCount) {
    place_at(10, 10, ZoneType::Habitation, ZoneDensity::LowDensity, 0);
    place_at(11, 10, ZoneType::Habitation, ZoneDensity::LowDensity, 0);
    place_at(12, 10, ZoneType::Exchange, ZoneDensity::LowDensity, 0);

    EXPECT_EQ(queryable()->get_zone_count_for(0, ZoneType::Habitation), 2u);
    EXPECT_EQ(queryable()->get_zone_count_for(0, ZoneType::Exchange), 1u);
    EXPECT_EQ(queryable()->get_zone_count_for(0, ZoneType::Fabrication), 0u);
}

TEST_F(ZoneQueryableTest, GetZoneCountFor_DifferentPlayers) {
    place_at(10, 10, ZoneType::Habitation, ZoneDensity::LowDensity, 0);
    place_at(11, 10, ZoneType::Habitation, ZoneDensity::LowDensity, 1);

    EXPECT_EQ(queryable()->get_zone_count_for(0, ZoneType::Habitation), 1u);
    EXPECT_EQ(queryable()->get_zone_count_for(1, ZoneType::Habitation), 1u);
}

// ============================================================================
// get_designated_zones
// ============================================================================

TEST_F(ZoneQueryableTest, GetDesignatedZones_ReturnsDesignatedOnly) {
    place_at(10, 10, ZoneType::Habitation, ZoneDensity::LowDensity, 0);
    place_at(11, 10, ZoneType::Habitation, ZoneDensity::LowDensity, 0);
    place_at(12, 10, ZoneType::Habitation, ZoneDensity::LowDensity, 0);

    // Make one occupied and one stalled
    system->set_zone_state(11, 10, ZoneState::Occupied);
    system->set_zone_state(12, 10, ZoneState::Stalled);

    auto positions = queryable()->get_designated_zones(0, ZoneType::Habitation);
    ASSERT_EQ(positions.size(), 1u);
    EXPECT_EQ(positions[0].x, 10);
    EXPECT_EQ(positions[0].y, 10);
}

TEST_F(ZoneQueryableTest, GetDesignatedZones_FiltersByType) {
    place_at(10, 10, ZoneType::Habitation, ZoneDensity::LowDensity, 0);
    place_at(11, 10, ZoneType::Exchange, ZoneDensity::LowDensity, 0);
    place_at(12, 10, ZoneType::Fabrication, ZoneDensity::LowDensity, 0);

    auto hab_positions = queryable()->get_designated_zones(0, ZoneType::Habitation);
    ASSERT_EQ(hab_positions.size(), 1u);
    EXPECT_EQ(hab_positions[0].x, 10);

    auto exc_positions = queryable()->get_designated_zones(0, ZoneType::Exchange);
    ASSERT_EQ(exc_positions.size(), 1u);
    EXPECT_EQ(exc_positions[0].x, 11);
}

TEST_F(ZoneQueryableTest, GetDesignatedZones_FiltersByPlayer) {
    place_at(10, 10, ZoneType::Habitation, ZoneDensity::LowDensity, 0);
    place_at(11, 10, ZoneType::Habitation, ZoneDensity::LowDensity, 1);

    auto p0_positions = queryable()->get_designated_zones(0, ZoneType::Habitation);
    ASSERT_EQ(p0_positions.size(), 1u);
    EXPECT_EQ(p0_positions[0].x, 10);

    auto p1_positions = queryable()->get_designated_zones(1, ZoneType::Habitation);
    ASSERT_EQ(p1_positions.size(), 1u);
    EXPECT_EQ(p1_positions[0].x, 11);
}

TEST_F(ZoneQueryableTest, GetDesignatedZones_EmptyResult) {
    auto positions = queryable()->get_designated_zones(0, ZoneType::Habitation);
    EXPECT_TRUE(positions.empty());
}

// ============================================================================
// get_demand_for
// ============================================================================

TEST_F(ZoneQueryableTest, GetDemandFor_ReturnsAsFloat) {
    // Default demand with no zones should be non-zero (base pressure from DemandConfig)
    // Just verify it returns a float representation
    float demand = queryable()->get_demand_for(ZoneType::Habitation, 0);
    // The demand is calculated internally, we just verify it returns a float
    // and matches the integer version
    std::int8_t int_demand = system->get_demand_for_type(ZoneType::Habitation, 0);
    EXPECT_FLOAT_EQ(demand, static_cast<float>(int_demand));
}

TEST_F(ZoneQueryableTest, GetDemandFor_InvalidPlayer) {
    float demand = queryable()->get_demand_for(ZoneType::Habitation, 255);
    EXPECT_FLOAT_EQ(demand, 0.0f);
}

// ============================================================================
// Interface polymorphism
// ============================================================================

TEST_F(ZoneQueryableTest, InterfacePointerWorks) {
    // Verify ZoneSystem can be used through IZoneQueryable pointer
    IZoneQueryable* iface = system.get();

    place_at(5, 5, ZoneType::Fabrication, ZoneDensity::HighDensity, 2);

    EXPECT_TRUE(iface->is_zoned_at(5, 5));

    ZoneType type;
    EXPECT_TRUE(iface->get_zone_type_at(5, 5, type));
    EXPECT_EQ(type, ZoneType::Fabrication);

    ZoneDensity density;
    EXPECT_TRUE(iface->get_zone_density_at(5, 5, density));
    EXPECT_EQ(density, ZoneDensity::HighDensity);

    EXPECT_EQ(iface->get_zone_count_for(2, ZoneType::Fabrication), 1u);

    auto positions = iface->get_designated_zones(2, ZoneType::Fabrication);
    ASSERT_EQ(positions.size(), 1u);
    EXPECT_EQ(positions[0].x, 5);
    EXPECT_EQ(positions[0].y, 5);
}

} // anonymous namespace
} // namespace zone
} // namespace sims3000
