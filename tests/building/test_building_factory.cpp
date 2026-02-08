/**
 * @file test_building_factory.cpp
 * @brief Tests for BuildingFactory (ticket 4-025)
 */

#include <gtest/gtest.h>
#include <sims3000/building/BuildingFactory.h>
#include <sims3000/building/BuildingGrid.h>
#include <sims3000/zone/ZoneSystem.h>

using namespace sims3000::building;
using namespace sims3000::zone;

namespace {

// Helper to create a test template
BuildingTemplate make_test_template(uint32_t id = 1, uint8_t fw = 1, uint8_t fh = 1) {
    BuildingTemplate templ;
    templ.template_id = id;
    templ.name = "TestBuilding";
    templ.zone_type = ZoneBuildingType::Habitation;
    templ.density = DensityLevel::Low;
    templ.footprint_w = fw;
    templ.footprint_h = fh;
    templ.construction_ticks = 100;
    templ.construction_cost = 500;
    templ.base_capacity = 20;
    templ.color_accent_count = 4;
    return templ;
}

// Helper to create a test selection result
TemplateSelectionResult make_test_selection(uint32_t template_id = 1,
                                            uint8_t rotation = 2,
                                            uint8_t color_accent = 3) {
    TemplateSelectionResult sel;
    sel.template_id = template_id;
    sel.rotation = rotation;
    sel.color_accent_index = color_accent;
    return sel;
}

class BuildingFactoryTest : public ::testing::Test {
protected:
    void SetUp() override {
        grid.initialize(128, 128);
        zone_system = std::make_unique<ZoneSystem>(nullptr, nullptr, 128);
        factory = std::make_unique<BuildingFactory>(&grid, zone_system.get());
    }

    BuildingGrid grid;
    std::unique_ptr<ZoneSystem> zone_system;
    std::unique_ptr<BuildingFactory> factory;
};

TEST_F(BuildingFactoryTest, SpawnBuildingCreatesEntityWithCorrectComponents) {
    auto templ = make_test_template();
    auto selection = make_test_selection();

    uint32_t entity_id = factory->spawn_building(templ, selection, 5, 10, 1, 100);

    EXPECT_NE(entity_id, 0u);

    const BuildingEntity* entity = factory->get_entity(entity_id);
    ASSERT_NE(entity, nullptr);

    // Check BuildingComponent fields
    EXPECT_EQ(entity->building.template_id, 1u);
    EXPECT_EQ(entity->building.getZoneBuildingType(), ZoneBuildingType::Habitation);
    EXPECT_EQ(entity->building.getDensityLevel(), DensityLevel::Low);
    EXPECT_EQ(entity->building.getBuildingState(), BuildingState::Materializing);
    EXPECT_EQ(entity->building.level, 1);
    EXPECT_EQ(entity->building.health, 255);
    EXPECT_EQ(entity->building.capacity, 20);
    EXPECT_EQ(entity->building.current_occupancy, 0);
    EXPECT_EQ(entity->building.footprint_w, 1);
    EXPECT_EQ(entity->building.footprint_h, 1);
    EXPECT_EQ(entity->building.rotation, 2);
    EXPECT_EQ(entity->building.color_accent_index, 3);
    EXPECT_EQ(entity->building.state_changed_tick, 100u);

    // Check positional data
    EXPECT_EQ(entity->grid_x, 5);
    EXPECT_EQ(entity->grid_y, 10);
    EXPECT_EQ(entity->owner_id, 1);
}

TEST_F(BuildingFactoryTest, SpawnBuildingInitializesConstructionComponent) {
    auto templ = make_test_template();
    auto selection = make_test_selection();

    uint32_t entity_id = factory->spawn_building(templ, selection, 5, 10, 1, 100);

    const BuildingEntity* entity = factory->get_entity(entity_id);
    ASSERT_NE(entity, nullptr);

    EXPECT_TRUE(entity->has_construction);
    EXPECT_EQ(entity->construction.ticks_total, 100);
    EXPECT_EQ(entity->construction.construction_cost, 500u);
    EXPECT_EQ(entity->construction.ticks_elapsed, 0);
    EXPECT_FALSE(entity->construction.isComplete());
    EXPECT_FALSE(entity->has_debris);
}

TEST_F(BuildingFactoryTest, SpawnBuildingRegistersInGrid) {
    auto templ = make_test_template();
    auto selection = make_test_selection();

    uint32_t entity_id = factory->spawn_building(templ, selection, 5, 10, 1, 100);

    EXPECT_EQ(grid.get_building_at(5, 10), entity_id);
    EXPECT_TRUE(grid.is_tile_occupied(5, 10));
}

TEST_F(BuildingFactoryTest, SpawnBuildingRegistersMultiTileFootprint) {
    auto templ = make_test_template(1, 2, 3); // 2x3 footprint
    auto selection = make_test_selection();

    uint32_t entity_id = factory->spawn_building(templ, selection, 10, 20, 0, 50);

    // Check all tiles in 2x3 footprint
    for (int32_t dy = 0; dy < 3; ++dy) {
        for (int32_t dx = 0; dx < 2; ++dx) {
            EXPECT_EQ(grid.get_building_at(10 + dx, 20 + dy), entity_id)
                << "Tile (" << (10 + dx) << ", " << (20 + dy) << ") should be occupied";
        }
    }

    // Check adjacent tiles are NOT occupied
    EXPECT_EQ(grid.get_building_at(9, 20), sims3000::building::INVALID_ENTITY);
    EXPECT_EQ(grid.get_building_at(12, 20), sims3000::building::INVALID_ENTITY);
}

TEST_F(BuildingFactoryTest, SpawnBuildingSetsZoneStateToOccupied) {
    // Place a zone first so set_zone_state can work
    zone_system->place_zone(5, 10, ZoneType::Habitation, ZoneDensity::LowDensity, 0, 100);

    auto templ = make_test_template();
    auto selection = make_test_selection();

    factory->spawn_building(templ, selection, 5, 10, 0, 100);

    ZoneState state;
    bool found = zone_system->get_zone_state(5, 10, state);
    if (found) {
        EXPECT_EQ(state, ZoneState::Occupied);
    }
}

TEST_F(BuildingFactoryTest, SpawnBuildingGeneratesUniqueEntityIDs) {
    auto templ = make_test_template();
    auto selection = make_test_selection();

    uint32_t id1 = factory->spawn_building(templ, selection, 5, 10, 0, 100);
    uint32_t id2 = factory->spawn_building(templ, selection, 6, 10, 0, 101);
    uint32_t id3 = factory->spawn_building(templ, selection, 7, 10, 0, 102);

    EXPECT_NE(id1, id2);
    EXPECT_NE(id2, id3);
    EXPECT_NE(id1, id3);
    EXPECT_NE(id1, 0u);
    EXPECT_NE(id2, 0u);
    EXPECT_NE(id3, 0u);
}

TEST_F(BuildingFactoryTest, GetEntityReturnsNullptrForInvalidId) {
    EXPECT_EQ(factory->get_entity(999), nullptr);
    EXPECT_EQ(factory->get_entity_mut(999), nullptr);
}

TEST_F(BuildingFactoryTest, GetEntitiesReturnsAllEntities) {
    auto templ = make_test_template();
    auto selection = make_test_selection();

    factory->spawn_building(templ, selection, 5, 10, 0, 100);
    factory->spawn_building(templ, selection, 6, 10, 0, 101);
    factory->spawn_building(templ, selection, 7, 10, 0, 102);

    EXPECT_EQ(factory->get_entities().size(), 3u);
}

TEST_F(BuildingFactoryTest, RemoveEntityWorks) {
    auto templ = make_test_template();
    auto selection = make_test_selection();

    uint32_t id = factory->spawn_building(templ, selection, 5, 10, 0, 100);
    EXPECT_EQ(factory->get_entities().size(), 1u);

    bool removed = factory->remove_entity(id);
    EXPECT_TRUE(removed);
    EXPECT_EQ(factory->get_entities().size(), 0u);
    EXPECT_EQ(factory->get_entity(id), nullptr);
}

TEST_F(BuildingFactoryTest, RemoveEntityReturnsFalseForInvalidId) {
    EXPECT_FALSE(factory->remove_entity(999));
}

TEST_F(BuildingFactoryTest, SpawnBuildingWithDifferentZoneTypes) {
    BuildingTemplate templ_exchange;
    templ_exchange.template_id = 2;
    templ_exchange.zone_type = ZoneBuildingType::Exchange;
    templ_exchange.density = DensityLevel::High;
    templ_exchange.footprint_w = 1;
    templ_exchange.footprint_h = 1;
    templ_exchange.construction_ticks = 50;
    templ_exchange.construction_cost = 300;
    templ_exchange.base_capacity = 15;

    auto selection = make_test_selection(2, 1, 0);

    uint32_t entity_id = factory->spawn_building(templ_exchange, selection, 20, 20, 2, 200);

    const BuildingEntity* entity = factory->get_entity(entity_id);
    ASSERT_NE(entity, nullptr);
    EXPECT_EQ(entity->building.getZoneBuildingType(), ZoneBuildingType::Exchange);
    EXPECT_EQ(entity->building.getDensityLevel(), DensityLevel::High);
    EXPECT_EQ(entity->building.rotation, 1);
    EXPECT_EQ(entity->building.color_accent_index, 0);
    EXPECT_EQ(entity->owner_id, 2);
}

TEST_F(BuildingFactoryTest, MutableEntityCanBeModified) {
    auto templ = make_test_template();
    auto selection = make_test_selection();

    uint32_t id = factory->spawn_building(templ, selection, 5, 10, 0, 100);

    BuildingEntity* entity = factory->get_entity_mut(id);
    ASSERT_NE(entity, nullptr);

    entity->building.setBuildingState(BuildingState::Active);
    entity->has_construction = false;

    const BuildingEntity* const_entity = factory->get_entity(id);
    EXPECT_EQ(const_entity->building.getBuildingState(), BuildingState::Active);
    EXPECT_FALSE(const_entity->has_construction);
}

} // anonymous namespace
