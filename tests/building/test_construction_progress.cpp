/**
 * @file test_construction_progress.cpp
 * @brief Tests for ConstructionProgressSystem (ticket 4-027)
 */

#include <gtest/gtest.h>
#include <sims3000/building/ConstructionProgressSystem.h>
#include <sims3000/building/BuildingFactory.h>
#include <sims3000/building/BuildingGrid.h>
#include <sims3000/zone/ZoneSystem.h>

using namespace sims3000::building;
using namespace sims3000::zone;

namespace {

// Helper to create a test template
BuildingTemplate make_test_template(uint16_t construction_ticks = 100) {
    BuildingTemplate templ;
    templ.template_id = 1;
    templ.name = "TestBuilding";
    templ.zone_type = ZoneBuildingType::Habitation;
    templ.density = DensityLevel::Low;
    templ.footprint_w = 1;
    templ.footprint_h = 1;
    templ.construction_ticks = construction_ticks;
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

class ConstructionProgressTest : public ::testing::Test {
protected:
    void SetUp() override {
        grid.initialize(128, 128);
        zone_system = std::make_unique<ZoneSystem>(nullptr, nullptr, 128);
        factory = std::make_unique<BuildingFactory>(&grid, zone_system.get());
        system = std::make_unique<ConstructionProgressSystem>(factory.get());
    }

    BuildingGrid grid;
    std::unique_ptr<ZoneSystem> zone_system;
    std::unique_ptr<BuildingFactory> factory;
    std::unique_ptr<ConstructionProgressSystem> system;
};

TEST_F(ConstructionProgressTest, TicksElapsedIncrements) {
    auto templ = make_test_template(100);
    auto selection = make_test_selection();
    uint32_t id = factory->spawn_building(templ, selection, 5, 10, 0, 0);

    // Tick once
    system->tick(1);

    const BuildingEntity* entity = factory->get_entity(id);
    ASSERT_NE(entity, nullptr);
    EXPECT_EQ(entity->construction.ticks_elapsed, 1);
    EXPECT_TRUE(entity->has_construction);
    EXPECT_EQ(entity->building.getBuildingState(), BuildingState::Materializing);
}

TEST_F(ConstructionProgressTest, MultipleTicksAdvanceProgress) {
    auto templ = make_test_template(100);
    auto selection = make_test_selection();
    uint32_t id = factory->spawn_building(templ, selection, 5, 10, 0, 0);

    // Tick 10 times
    for (uint32_t t = 1; t <= 10; ++t) {
        system->tick(t);
    }

    const BuildingEntity* entity = factory->get_entity(id);
    ASSERT_NE(entity, nullptr);
    EXPECT_EQ(entity->construction.ticks_elapsed, 10);
}

TEST_F(ConstructionProgressTest, PhaseTransitionsAt25Percent) {
    auto templ = make_test_template(100);
    auto selection = make_test_selection();
    uint32_t id = factory->spawn_building(templ, selection, 5, 10, 0, 0);

    // At 0%, should be Foundation
    const BuildingEntity* entity = factory->get_entity(id);
    EXPECT_EQ(entity->construction.getPhase(), ConstructionPhase::Foundation);

    // Tick to 25%
    for (uint32_t t = 1; t <= 25; ++t) {
        system->tick(t);
    }

    entity = factory->get_entity(id);
    EXPECT_EQ(entity->construction.getPhase(), ConstructionPhase::Framework);
}

TEST_F(ConstructionProgressTest, PhaseTransitionsAt50Percent) {
    auto templ = make_test_template(100);
    auto selection = make_test_selection();
    uint32_t id = factory->spawn_building(templ, selection, 5, 10, 0, 0);

    // Tick to 50%
    for (uint32_t t = 1; t <= 50; ++t) {
        system->tick(t);
    }

    const BuildingEntity* entity = factory->get_entity(id);
    EXPECT_EQ(entity->construction.getPhase(), ConstructionPhase::Exterior);
}

TEST_F(ConstructionProgressTest, PhaseTransitionsAt75Percent) {
    auto templ = make_test_template(100);
    auto selection = make_test_selection();
    uint32_t id = factory->spawn_building(templ, selection, 5, 10, 0, 0);

    // Tick to 75%
    for (uint32_t t = 1; t <= 75; ++t) {
        system->tick(t);
    }

    const BuildingEntity* entity = factory->get_entity(id);
    EXPECT_EQ(entity->construction.getPhase(), ConstructionPhase::Finalization);
}

TEST_F(ConstructionProgressTest, CompletionTransitionsToActive) {
    auto templ = make_test_template(10); // Short construction time
    auto selection = make_test_selection();
    uint32_t id = factory->spawn_building(templ, selection, 5, 10, 0, 0);

    // Tick until complete
    for (uint32_t t = 1; t <= 10; ++t) {
        system->tick(t);
    }

    const BuildingEntity* entity = factory->get_entity(id);
    ASSERT_NE(entity, nullptr);
    EXPECT_EQ(entity->building.getBuildingState(), BuildingState::Active);
    EXPECT_FALSE(entity->has_construction);
    EXPECT_EQ(entity->building.state_changed_tick, 10u);
}

TEST_F(ConstructionProgressTest, PausedConstructionDoesNotAdvance) {
    auto templ = make_test_template(100);
    auto selection = make_test_selection();
    uint32_t id = factory->spawn_building(templ, selection, 5, 10, 0, 0);

    // Tick once
    system->tick(1);
    const BuildingEntity* entity = factory->get_entity(id);
    EXPECT_EQ(entity->construction.ticks_elapsed, 1);

    // Pause construction
    BuildingEntity* mut_entity = factory->get_entity_mut(id);
    mut_entity->construction.setPaused(true);

    // Tick again - should not advance
    system->tick(2);
    entity = factory->get_entity(id);
    EXPECT_EQ(entity->construction.ticks_elapsed, 1); // Still 1
}

TEST_F(ConstructionProgressTest, EventEmittedOnCompletion) {
    auto templ = make_test_template(5); // Very short
    auto selection = make_test_selection();
    uint32_t id = factory->spawn_building(templ, selection, 5, 10, 1, 0);

    // No events initially
    EXPECT_TRUE(system->get_pending_constructed_events().empty());

    // Tick until complete
    for (uint32_t t = 1; t <= 5; ++t) {
        system->tick(t);
    }

    // Should have exactly one event
    EXPECT_EQ(system->get_pending_constructed_events().size(), 1u);

    const auto& event = system->get_pending_constructed_events()[0];
    EXPECT_EQ(event.entity_id, id);
    EXPECT_EQ(event.owner_id, 1);
    EXPECT_EQ(event.zone_type, ZoneBuildingType::Habitation);
    EXPECT_EQ(event.grid_x, 5);
    EXPECT_EQ(event.grid_y, 10);
    EXPECT_EQ(event.template_id, 1u);
}

TEST_F(ConstructionProgressTest, ClearPendingEvents) {
    auto templ = make_test_template(1); // Completes in 1 tick
    auto selection = make_test_selection();
    factory->spawn_building(templ, selection, 5, 10, 0, 0);

    system->tick(1);
    EXPECT_EQ(system->get_pending_constructed_events().size(), 1u);

    system->clear_pending_constructed_events();
    EXPECT_TRUE(system->get_pending_constructed_events().empty());
}

TEST_F(ConstructionProgressTest, PhaseProgressCalculation) {
    auto templ = make_test_template(100);
    auto selection = make_test_selection();
    uint32_t id = factory->spawn_building(templ, selection, 5, 10, 0, 0);

    // Tick to 12% (within Foundation phase at 0-25%)
    for (uint32_t t = 1; t <= 12; ++t) {
        system->tick(t);
    }

    const BuildingEntity* entity = factory->get_entity(id);
    EXPECT_EQ(entity->construction.getPhase(), ConstructionPhase::Foundation);
    // phase_progress should be non-zero (12% into a 25% phase = ~48% of phase)
    EXPECT_GT(entity->construction.phase_progress, 0);
}

TEST_F(ConstructionProgressTest, CompletedBuildingNotTickedAgain) {
    auto templ = make_test_template(5);
    auto selection = make_test_selection();
    uint32_t id = factory->spawn_building(templ, selection, 5, 10, 0, 0);

    // Complete construction
    for (uint32_t t = 1; t <= 5; ++t) {
        system->tick(t);
    }

    system->clear_pending_constructed_events();

    // Tick again - should not generate more events
    system->tick(6);
    system->tick(7);

    EXPECT_TRUE(system->get_pending_constructed_events().empty());

    const BuildingEntity* entity = factory->get_entity(id);
    EXPECT_EQ(entity->building.getBuildingState(), BuildingState::Active);
}

TEST_F(ConstructionProgressTest, MultipleEntitiesTickIndependently) {
    auto templ_short = make_test_template(5);
    auto templ_long = make_test_template(20);
    templ_long.template_id = 2;
    auto selection = make_test_selection();

    uint32_t id1 = factory->spawn_building(templ_short, selection, 5, 10, 0, 0);
    uint32_t id2 = factory->spawn_building(templ_long, selection, 6, 10, 0, 0);

    // Tick 5 times - first should complete, second should not
    for (uint32_t t = 1; t <= 5; ++t) {
        system->tick(t);
    }

    const BuildingEntity* entity1 = factory->get_entity(id1);
    const BuildingEntity* entity2 = factory->get_entity(id2);

    ASSERT_NE(entity1, nullptr);
    ASSERT_NE(entity2, nullptr);

    EXPECT_EQ(entity1->building.getBuildingState(), BuildingState::Active);
    EXPECT_FALSE(entity1->has_construction);

    EXPECT_EQ(entity2->building.getBuildingState(), BuildingState::Materializing);
    EXPECT_TRUE(entity2->has_construction);
    EXPECT_EQ(entity2->construction.ticks_elapsed, 5);
}

} // anonymous namespace
