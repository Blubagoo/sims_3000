/**
 * @file test_building_queryable.cpp
 * @brief Tests for IBuildingQueryable implementation in BuildingSystem (Ticket 4-036)
 *
 * Verifies:
 * - get_building_at returns 0 for empty tile
 * - get_building_at returns entity_id for occupied tile
 * - is_tile_occupied true/false
 * - is_footprint_available true/false
 * - get_buildings_in_rect returns correct entities
 * - get_buildings_by_owner filters correctly
 * - get_building_count returns correct total
 * - get_building_count_by_state filters by state
 * - get_building_state returns correct state
 * - get_building_state returns empty optional for invalid id
 * - get_total_capacity sums correctly
 * - get_total_occupancy sums correctly
 * - Empty system returns zeros/empty
 * - Multiple buildings with different owners
 * - Multiple buildings with different states
 */

#include <gtest/gtest.h>
#include <sims3000/building/BuildingSystem.h>
#include <sims3000/building/IBuildingQueryable.h>
#include <sims3000/zone/ZoneSystem.h>

using namespace sims3000::building;
using namespace sims3000::zone;

namespace {

BuildingTemplate make_test_template(uint32_t id = 1,
                                     ZoneBuildingType type = ZoneBuildingType::Habitation,
                                     uint8_t fw = 1, uint8_t fh = 1) {
    BuildingTemplate templ;
    templ.template_id = id;
    templ.name = "TestBuilding";
    templ.zone_type = type;
    templ.density = DensityLevel::Low;
    templ.footprint_w = fw;
    templ.footprint_h = fh;
    templ.construction_ticks = 100;
    templ.construction_cost = 500;
    templ.base_capacity = 20;
    templ.color_accent_count = 4;
    return templ;
}

TemplateSelectionResult make_test_selection(uint32_t template_id = 1) {
    TemplateSelectionResult sel;
    sel.template_id = template_id;
    sel.rotation = 0;
    sel.color_accent_index = 0;
    return sel;
}

class BuildingQueryableTest : public ::testing::Test {
protected:
    void SetUp() override {
        zone_system = std::make_unique<ZoneSystem>(nullptr, nullptr, 128);
        building_system = std::make_unique<BuildingSystem>(zone_system.get(), nullptr, 128);
    }

    uint32_t spawn_building(int32_t x, int32_t y, uint8_t owner = 0,
                            ZoneBuildingType type = ZoneBuildingType::Habitation,
                            uint8_t fw = 1, uint8_t fh = 1) {
        // Place zone first
        ZoneType zt = static_cast<ZoneType>(static_cast<uint8_t>(type));
        zone_system->place_zone(x, y, zt, ZoneDensity::LowDensity, owner, 0);

        auto templ = make_test_template(1, type, fw, fh);
        auto sel = make_test_selection(1);
        return building_system->get_factory().spawn_building(templ, sel, x, y, owner, 0);
    }

    void set_building_active(uint32_t eid, uint16_t capacity = 20, uint16_t occupancy = 0) {
        auto* entity = building_system->get_factory().get_entity_mut(eid);
        entity->building.setBuildingState(BuildingState::Active);
        entity->building.capacity = capacity;
        entity->building.current_occupancy = occupancy;
        entity->has_construction = false;
    }

    std::unique_ptr<ZoneSystem> zone_system;
    std::unique_ptr<BuildingSystem> building_system;
};

// =========================================================================
// get_building_at
// =========================================================================

TEST_F(BuildingQueryableTest, GetBuildingAtReturnsZeroForEmptyTile) {
    EXPECT_EQ(building_system->get_building_at(10, 10), 0u);
}

TEST_F(BuildingQueryableTest, GetBuildingAtReturnsEntityIdForOccupiedTile) {
    uint32_t eid = spawn_building(5, 5);
    EXPECT_EQ(building_system->get_building_at(5, 5), eid);
}

// =========================================================================
// is_tile_occupied
// =========================================================================

TEST_F(BuildingQueryableTest, IsTileOccupiedFalseForEmptyTile) {
    EXPECT_FALSE(building_system->is_tile_occupied(10, 10));
}

TEST_F(BuildingQueryableTest, IsTileOccupiedTrueForOccupiedTile) {
    spawn_building(5, 5);
    EXPECT_TRUE(building_system->is_tile_occupied(5, 5));
}

// =========================================================================
// is_footprint_available
// =========================================================================

TEST_F(BuildingQueryableTest, IsFootprintAvailableTrueForEmptyArea) {
    EXPECT_TRUE(building_system->is_footprint_available(10, 10, 2, 2));
}

TEST_F(BuildingQueryableTest, IsFootprintAvailableFalseForOccupiedArea) {
    spawn_building(10, 10);
    EXPECT_FALSE(building_system->is_footprint_available(10, 10, 2, 2));
}

// =========================================================================
// get_buildings_in_rect
// =========================================================================

TEST_F(BuildingQueryableTest, GetBuildingsInRectReturnsCorrectEntities) {
    uint32_t eid1 = spawn_building(5, 5);
    uint32_t eid2 = spawn_building(6, 6);
    // Building outside rect
    spawn_building(20, 20);

    auto result = building_system->get_buildings_in_rect(4, 4, 4, 4);
    EXPECT_EQ(result.size(), 2u);

    // Check both entities are present
    bool found1 = false, found2 = false;
    for (uint32_t id : result) {
        if (id == eid1) found1 = true;
        if (id == eid2) found2 = true;
    }
    EXPECT_TRUE(found1);
    EXPECT_TRUE(found2);
}

TEST_F(BuildingQueryableTest, GetBuildingsInRectReturnsEmptyForEmptyArea) {
    auto result = building_system->get_buildings_in_rect(50, 50, 5, 5);
    EXPECT_TRUE(result.empty());
}

// =========================================================================
// get_buildings_by_owner
// =========================================================================

TEST_F(BuildingQueryableTest, GetBuildingsByOwnerFiltersCorrectly) {
    uint32_t eid1 = spawn_building(5, 5, 0);
    uint32_t eid2 = spawn_building(10, 10, 0);
    spawn_building(15, 15, 1);

    auto result = building_system->get_buildings_by_owner(0);
    EXPECT_EQ(result.size(), 2u);

    bool found1 = false, found2 = false;
    for (uint32_t id : result) {
        if (id == eid1) found1 = true;
        if (id == eid2) found2 = true;
    }
    EXPECT_TRUE(found1);
    EXPECT_TRUE(found2);
}

// =========================================================================
// get_building_count
// =========================================================================

TEST_F(BuildingQueryableTest, GetBuildingCountReturnsCorrectTotal) {
    spawn_building(5, 5);
    spawn_building(10, 10);
    spawn_building(15, 15);
    EXPECT_EQ(building_system->get_building_count(), 3u);
}

// =========================================================================
// get_building_count_by_state
// =========================================================================

TEST_F(BuildingQueryableTest, GetBuildingCountByStateFiltersByState) {
    uint32_t eid1 = spawn_building(5, 5);
    uint32_t eid2 = spawn_building(10, 10);
    uint32_t eid3 = spawn_building(15, 15);

    set_building_active(eid1);
    set_building_active(eid2);
    // eid3 remains Materializing

    EXPECT_EQ(building_system->get_building_count_by_state(BuildingState::Active), 2u);
    EXPECT_EQ(building_system->get_building_count_by_state(BuildingState::Materializing), 1u);
    (void)eid3;
}

// =========================================================================
// get_building_state
// =========================================================================

TEST_F(BuildingQueryableTest, GetBuildingStateReturnsCorrectState) {
    uint32_t eid = spawn_building(5, 5);
    set_building_active(eid);

    auto state = building_system->get_building_state(eid);
    ASSERT_TRUE(state.has_value());
    EXPECT_EQ(state.value(), BuildingState::Active);
}

TEST_F(BuildingQueryableTest, GetBuildingStateReturnsEmptyOptionalForInvalidId) {
    auto state = building_system->get_building_state(9999);
    EXPECT_FALSE(state.has_value());
}

// =========================================================================
// get_total_capacity
// =========================================================================

TEST_F(BuildingQueryableTest, GetTotalCapacitySumsCorrectly) {
    uint32_t eid1 = spawn_building(5, 5, 0, ZoneBuildingType::Habitation);
    uint32_t eid2 = spawn_building(10, 10, 0, ZoneBuildingType::Habitation);

    set_building_active(eid1, 20);
    set_building_active(eid2, 30);

    EXPECT_EQ(building_system->get_total_capacity(ZoneBuildingType::Habitation, 0), 50u);
}

// =========================================================================
// get_total_occupancy
// =========================================================================

TEST_F(BuildingQueryableTest, GetTotalOccupancySumsCorrectly) {
    uint32_t eid1 = spawn_building(5, 5, 0, ZoneBuildingType::Habitation);
    uint32_t eid2 = spawn_building(10, 10, 0, ZoneBuildingType::Habitation);

    set_building_active(eid1, 20, 10);
    set_building_active(eid2, 30, 15);

    EXPECT_EQ(building_system->get_total_occupancy(ZoneBuildingType::Habitation, 0), 25u);
}

// =========================================================================
// Empty System
// =========================================================================

TEST_F(BuildingQueryableTest, EmptySystemReturnsZerosAndEmpty) {
    EXPECT_EQ(building_system->get_building_count(), 0u);
    EXPECT_EQ(building_system->get_building_count_by_state(BuildingState::Active), 0u);
    EXPECT_EQ(building_system->get_building_at(0, 0), 0u);
    EXPECT_FALSE(building_system->is_tile_occupied(0, 0));
    EXPECT_TRUE(building_system->get_buildings_in_rect(0, 0, 10, 10).empty());
    EXPECT_TRUE(building_system->get_buildings_by_owner(0).empty());
    EXPECT_EQ(building_system->get_total_capacity(ZoneBuildingType::Habitation, 0), 0u);
    EXPECT_EQ(building_system->get_total_occupancy(ZoneBuildingType::Habitation, 0), 0u);
}

// =========================================================================
// Multiple Buildings With Different Owners
// =========================================================================

TEST_F(BuildingQueryableTest, MultipleBuildingsWithDifferentOwners) {
    uint32_t eid0a = spawn_building(5, 5, 0);
    uint32_t eid0b = spawn_building(10, 10, 0);
    uint32_t eid1a = spawn_building(15, 15, 1);

    set_building_active(eid0a, 20, 5);
    set_building_active(eid0b, 30, 10);
    set_building_active(eid1a, 40, 20);

    // Owner 0 queries
    auto owner0 = building_system->get_buildings_by_owner(0);
    EXPECT_EQ(owner0.size(), 2u);
    EXPECT_EQ(building_system->get_total_capacity(ZoneBuildingType::Habitation, 0), 50u);
    EXPECT_EQ(building_system->get_total_occupancy(ZoneBuildingType::Habitation, 0), 15u);

    // Owner 1 queries
    auto owner1 = building_system->get_buildings_by_owner(1);
    EXPECT_EQ(owner1.size(), 1u);
    EXPECT_EQ(building_system->get_total_capacity(ZoneBuildingType::Habitation, 1), 40u);
    EXPECT_EQ(building_system->get_total_occupancy(ZoneBuildingType::Habitation, 1), 20u);
}

// =========================================================================
// Multiple Buildings With Different States
// =========================================================================

TEST_F(BuildingQueryableTest, MultipleBuildingsWithDifferentStates) {
    uint32_t eid1 = spawn_building(5, 5);
    uint32_t eid2 = spawn_building(10, 10);
    uint32_t eid3 = spawn_building(15, 15);

    set_building_active(eid1);

    auto* entity2 = building_system->get_factory().get_entity_mut(eid2);
    entity2->building.setBuildingState(BuildingState::Abandoned);

    // eid3 remains Materializing
    (void)eid3;

    EXPECT_EQ(building_system->get_building_count_by_state(BuildingState::Active), 1u);
    EXPECT_EQ(building_system->get_building_count_by_state(BuildingState::Abandoned), 1u);
    EXPECT_EQ(building_system->get_building_count_by_state(BuildingState::Materializing), 1u);
    EXPECT_EQ(building_system->get_building_count(), 3u);

    // Verify individual states
    EXPECT_EQ(building_system->get_building_state(eid1).value(), BuildingState::Active);
    EXPECT_EQ(building_system->get_building_state(eid2).value(), BuildingState::Abandoned);
    EXPECT_EQ(building_system->get_building_state(eid3).value(), BuildingState::Materializing);
}

} // anonymous namespace
