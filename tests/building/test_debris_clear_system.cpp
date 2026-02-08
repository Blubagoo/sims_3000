/**
 * @file test_debris_clear_system.cpp
 * @brief Tests for DebrisClearSystem (ticket 4-031)
 */

#include <gtest/gtest.h>
#include <sims3000/building/DebrisClearSystem.h>
#include <sims3000/building/BuildingFactory.h>
#include <sims3000/building/BuildingGrid.h>
#include <sims3000/building/ForwardDependencyStubs.h>
#include <sims3000/zone/ZoneSystem.h>

using namespace sims3000::building;
using namespace sims3000::zone;

namespace {

BuildingTemplate make_test_template() {
    BuildingTemplate templ;
    templ.template_id = 1;
    templ.name = "TestBuilding";
    templ.zone_type = ZoneBuildingType::Habitation;
    templ.density = DensityLevel::Low;
    templ.footprint_w = 1;
    templ.footprint_h = 1;
    templ.construction_ticks = 100;
    templ.construction_cost = 500;
    templ.base_capacity = 20;
    templ.color_accent_count = 4;
    return templ;
}

TemplateSelectionResult make_test_selection() {
    TemplateSelectionResult sel;
    sel.template_id = 1;
    sel.rotation = 0;
    sel.color_accent_index = 0;
    return sel;
}

class DebrisClearSystemTest : public ::testing::Test {
protected:
    void SetUp() override {
        grid.initialize(128, 128);
        zone_system = std::make_unique<ZoneSystem>(nullptr, nullptr, 128);
        factory = std::make_unique<BuildingFactory>(&grid, zone_system.get());
        credits = std::make_unique<StubCreditProvider>();
        system = std::make_unique<DebrisClearSystem>(factory.get(), &grid, credits.get());
    }

    // Helper: spawn a building and transition it to Deconstructed with debris
    uint32_t spawn_debris_entity(int32_t x = 5, int32_t y = 10,
                                  uint16_t clear_timer = 5) {
        auto templ = make_test_template();
        auto selection = make_test_selection();
        uint32_t id = factory->spawn_building(templ, selection, x, y, 0, 0);

        BuildingEntity* entity = factory->get_entity_mut(id);
        entity->building.setBuildingState(BuildingState::Deconstructed);
        entity->has_construction = false;
        entity->has_debris = true;
        entity->debris = DebrisComponent(1, 1, 1, clear_timer);

        // Clear grid (simulates DemolitionHandler clearing footprint)
        grid.clear_footprint(x, y, 1, 1);

        return id;
    }

    BuildingGrid grid;
    std::unique_ptr<ZoneSystem> zone_system;
    std::unique_ptr<BuildingFactory> factory;
    std::unique_ptr<StubCreditProvider> credits;
    std::unique_ptr<DebrisClearSystem> system;
};

TEST_F(DebrisClearSystemTest, TimerDecrementEachTick) {
    uint32_t id = spawn_debris_entity(5, 10, 10);

    system->tick();

    const BuildingEntity* entity = factory->get_entity(id);
    ASSERT_NE(entity, nullptr);
    EXPECT_EQ(entity->debris.clear_timer, 9);
}

TEST_F(DebrisClearSystemTest, TimerDecrementsMultipleTicks) {
    uint32_t id = spawn_debris_entity(5, 10, 10);

    for (int i = 0; i < 5; ++i) {
        system->tick();
    }

    const BuildingEntity* entity = factory->get_entity(id);
    ASSERT_NE(entity, nullptr);
    EXPECT_EQ(entity->debris.clear_timer, 5);
}

TEST_F(DebrisClearSystemTest, AutoClearOnExpiry) {
    uint32_t id = spawn_debris_entity(5, 10, 3); // Timer = 3

    // Tick 3 times
    system->tick();
    system->tick();
    system->tick();

    // Entity should be removed
    EXPECT_EQ(factory->get_entity(id), nullptr);
    EXPECT_EQ(factory->get_entities().size(), 0u);
}

TEST_F(DebrisClearSystemTest, EventEmittedOnAutoClear) {
    spawn_debris_entity(5, 10, 2);

    EXPECT_TRUE(system->get_pending_events().empty());

    system->tick();
    EXPECT_TRUE(system->get_pending_events().empty()); // Not expired yet

    system->tick();
    EXPECT_EQ(system->get_pending_events().size(), 1u);

    const auto& event = system->get_pending_events()[0];
    EXPECT_EQ(event.grid_x, 5);
    EXPECT_EQ(event.grid_y, 10);
}

TEST_F(DebrisClearSystemTest, ManualClearWithCost) {
    uint32_t id = spawn_debris_entity(5, 10, 100); // Long timer

    DebrisClearConfig config;
    config.manual_clear_cost = 50;
    system->set_config(config);

    bool result = system->handle_clear_debris(id, 0);

    EXPECT_TRUE(result);

    // Entity should be removed
    EXPECT_EQ(factory->get_entity(id), nullptr);
    EXPECT_EQ(factory->get_entities().size(), 0u);
}

TEST_F(DebrisClearSystemTest, ManualClearEmitsEvent) {
    uint32_t id = spawn_debris_entity(5, 10, 100);

    system->handle_clear_debris(id, 0);

    EXPECT_EQ(system->get_pending_events().size(), 1u);
    EXPECT_EQ(system->get_pending_events()[0].grid_x, 5);
    EXPECT_EQ(system->get_pending_events()[0].grid_y, 10);
}

TEST_F(DebrisClearSystemTest, ManualClearFailsForInvalidEntity) {
    bool result = system->handle_clear_debris(999, 0);
    EXPECT_FALSE(result);
}

TEST_F(DebrisClearSystemTest, ManualClearFailsForNonDebrisEntity) {
    auto templ = make_test_template();
    auto selection = make_test_selection();
    uint32_t id = factory->spawn_building(templ, selection, 5, 10, 0, 0);

    // Building is in Materializing state, not Deconstructed
    bool result = system->handle_clear_debris(id, 0);
    EXPECT_FALSE(result);
}

TEST_F(DebrisClearSystemTest, ManualClearFailsWithInsufficientCredits) {
    uint32_t id = spawn_debris_entity(5, 10, 100);

    credits->set_debug_restrictive(true); // Credits always fail

    bool result = system->handle_clear_debris(id, 0);
    EXPECT_FALSE(result);

    // Entity should still exist
    EXPECT_NE(factory->get_entity(id), nullptr);
}

TEST_F(DebrisClearSystemTest, EntityRemovedAfterClear) {
    uint32_t id = spawn_debris_entity(5, 10, 1);

    EXPECT_EQ(factory->get_entities().size(), 1u);

    system->tick(); // Timer goes to 0, entity removed

    EXPECT_EQ(factory->get_entities().size(), 0u);
    EXPECT_EQ(factory->get_entity(id), nullptr);
}

TEST_F(DebrisClearSystemTest, ClearPendingEvents) {
    spawn_debris_entity(5, 10, 1);
    system->tick();

    EXPECT_EQ(system->get_pending_events().size(), 1u);

    system->clear_pending_events();
    EXPECT_TRUE(system->get_pending_events().empty());
}

TEST_F(DebrisClearSystemTest, NonDebrisEntitiesNotAffected) {
    // Create an Active building (not debris)
    auto templ = make_test_template();
    auto selection = make_test_selection();
    uint32_t id1 = factory->spawn_building(templ, selection, 5, 10, 0, 0);
    BuildingEntity* entity1 = factory->get_entity_mut(id1);
    entity1->building.setBuildingState(BuildingState::Active);
    entity1->has_construction = false;

    // Create a debris entity
    uint32_t id2 = spawn_debris_entity(6, 10, 1);

    EXPECT_EQ(factory->get_entities().size(), 2u);

    system->tick(); // Only debris should be affected

    EXPECT_EQ(factory->get_entities().size(), 1u);
    EXPECT_NE(factory->get_entity(id1), nullptr); // Active building still exists
    EXPECT_EQ(factory->get_entity(id2), nullptr);  // Debris removed
}

TEST_F(DebrisClearSystemTest, MultipleDebrisEntitiesCleared) {
    uint32_t id1 = spawn_debris_entity(5, 10, 2);
    uint32_t id2 = spawn_debris_entity(6, 10, 2);
    uint32_t id3 = spawn_debris_entity(7, 10, 5); // This one has longer timer

    EXPECT_EQ(factory->get_entities().size(), 3u);

    // Tick twice - first two should be cleared
    system->tick();
    system->tick();

    EXPECT_EQ(factory->get_entities().size(), 1u);
    EXPECT_EQ(factory->get_entity(id1), nullptr);
    EXPECT_EQ(factory->get_entity(id2), nullptr);
    EXPECT_NE(factory->get_entity(id3), nullptr); // Still has 3 ticks left

    EXPECT_EQ(system->get_pending_events().size(), 2u);
}

} // anonymous namespace
