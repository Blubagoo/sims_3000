/**
 * @file test_demolition_handler.cpp
 * @brief Tests for DemolitionHandler (ticket 4-030)
 */

#include <gtest/gtest.h>
#include <sims3000/building/DemolitionHandler.h>
#include <sims3000/building/BuildingFactory.h>
#include <sims3000/building/BuildingGrid.h>
#include <sims3000/building/ForwardDependencyStubs.h>
#include <sims3000/zone/ZoneSystem.h>

using namespace sims3000::building;
using namespace sims3000::zone;

namespace {

BuildingTemplate make_test_template(uint32_t id = 1, uint8_t fw = 1, uint8_t fh = 1) {
    BuildingTemplate templ;
    templ.template_id = id;
    templ.name = "TestBuilding";
    templ.zone_type = ZoneBuildingType::Habitation;
    templ.density = DensityLevel::Low;
    templ.footprint_w = fw;
    templ.footprint_h = fh;
    templ.construction_ticks = 100;
    templ.construction_cost = 1000;
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

class DemolitionHandlerTest : public ::testing::Test {
protected:
    void SetUp() override {
        grid.initialize(128, 128);
        zone_system = std::make_unique<ZoneSystem>(nullptr, nullptr, 128);
        factory = std::make_unique<BuildingFactory>(&grid, zone_system.get());
        credits = std::make_unique<StubCreditProvider>();
        handler = std::make_unique<DemolitionHandler>(
            factory.get(), &grid, credits.get(), zone_system.get());
    }

    // Helper to spawn and make a building active
    uint32_t spawn_active_building(int32_t x = 5, int32_t y = 10, uint8_t owner = 0) {
        auto templ = make_test_template();
        auto selection = make_test_selection();
        uint32_t id = factory->spawn_building(templ, selection, x, y, owner, 0);

        // Transition to Active
        BuildingEntity* entity = factory->get_entity_mut(id);
        entity->building.setBuildingState(BuildingState::Active);
        entity->has_construction = false;
        return id;
    }

    BuildingGrid grid;
    std::unique_ptr<ZoneSystem> zone_system;
    std::unique_ptr<BuildingFactory> factory;
    std::unique_ptr<StubCreditProvider> credits;
    std::unique_ptr<DemolitionHandler> handler;
};

TEST_F(DemolitionHandlerTest, SuccessfulDemolition) {
    uint32_t id = spawn_active_building(5, 10, 0);

    auto result = handler->handle_demolish(id, 0);

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.reason, DemolitionResult::Reason::Ok);

    // Check building is now Deconstructed
    const BuildingEntity* entity = factory->get_entity(id);
    ASSERT_NE(entity, nullptr);
    EXPECT_EQ(entity->building.getBuildingState(), BuildingState::Deconstructed);
    EXPECT_TRUE(entity->has_debris);
    EXPECT_FALSE(entity->has_construction);
}

TEST_F(DemolitionHandlerTest, OwnershipRejection) {
    uint32_t id = spawn_active_building(5, 10, 0); // Owner is player 0

    auto result = handler->handle_demolish(id, 1); // Player 1 tries to demolish

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.reason, DemolitionResult::Reason::NotOwned);

    // Building should still be Active
    const BuildingEntity* entity = factory->get_entity(id);
    EXPECT_EQ(entity->building.getBuildingState(), BuildingState::Active);
}

TEST_F(DemolitionHandlerTest, AlreadyDeconstructedRejection) {
    uint32_t id = spawn_active_building(5, 10, 0);

    // First demolition succeeds
    auto result1 = handler->handle_demolish(id, 0);
    EXPECT_TRUE(result1.success);

    // Second demolition fails
    auto result2 = handler->handle_demolish(id, 0);
    EXPECT_FALSE(result2.success);
    EXPECT_EQ(result2.reason, DemolitionResult::Reason::AlreadyDeconstructed);
}

TEST_F(DemolitionHandlerTest, EntityNotFoundRejection) {
    auto result = handler->handle_demolish(999, 0);

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.reason, DemolitionResult::Reason::EntityNotFound);
}

TEST_F(DemolitionHandlerTest, CostCalculationForActiveState) {
    uint32_t id = spawn_active_building(5, 10, 0);

    auto result = handler->handle_demolish(id, 0);

    // Default: construction_cost(1000) * base_ratio(0.25) * active_modifier(1.0) = 250
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.cost, 250u);
}

TEST_F(DemolitionHandlerTest, CostCalculationForMaterializingState) {
    auto templ = make_test_template();
    auto selection = make_test_selection();
    uint32_t id = factory->spawn_building(templ, selection, 5, 10, 0, 0);
    // Building stays in Materializing state (default from spawn)

    auto result = handler->handle_demolish(id, 0);

    // construction_cost(1000) * base_ratio(0.25) * materializing_modifier(0.5) = 125
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.cost, 125u);
}

TEST_F(DemolitionHandlerTest, CostCalculationForAbandonedState) {
    uint32_t id = spawn_active_building(5, 10, 0);

    // Set to Abandoned
    BuildingEntity* entity = factory->get_entity_mut(id);
    entity->building.setBuildingState(BuildingState::Abandoned);

    auto result = handler->handle_demolish(id, 0);

    // construction_cost(1000) * base_ratio(0.25) * abandoned_modifier(0.1) = 25
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.cost, 25u);
}

TEST_F(DemolitionHandlerTest, CostCalculationForDerelictState) {
    uint32_t id = spawn_active_building(5, 10, 0);

    // Set to Derelict
    BuildingEntity* entity = factory->get_entity_mut(id);
    entity->building.setBuildingState(BuildingState::Derelict);

    auto result = handler->handle_demolish(id, 0);

    // construction_cost(1000) * base_ratio(0.25) * derelict_modifier(0.0) = 0 (free)
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.cost, 0u);
}

TEST_F(DemolitionHandlerTest, GridClearedOnDemolition) {
    uint32_t id = spawn_active_building(5, 10, 0);

    // Grid should be occupied before demolition
    EXPECT_TRUE(grid.is_tile_occupied(5, 10));

    handler->handle_demolish(id, 0);

    // Grid should be clear after demolition
    EXPECT_FALSE(grid.is_tile_occupied(5, 10));
}

TEST_F(DemolitionHandlerTest, GridClearedForMultiTileFootprint) {
    auto templ = make_test_template(1, 2, 2); // 2x2 footprint
    auto selection = make_test_selection();
    uint32_t id = factory->spawn_building(templ, selection, 10, 10, 0, 0);

    // Make Active
    BuildingEntity* entity = factory->get_entity_mut(id);
    entity->building.setBuildingState(BuildingState::Active);
    entity->has_construction = false;

    // All tiles should be occupied
    EXPECT_TRUE(grid.is_tile_occupied(10, 10));
    EXPECT_TRUE(grid.is_tile_occupied(11, 10));
    EXPECT_TRUE(grid.is_tile_occupied(10, 11));
    EXPECT_TRUE(grid.is_tile_occupied(11, 11));

    handler->handle_demolish(id, 0);

    // All tiles should be clear
    EXPECT_FALSE(grid.is_tile_occupied(10, 10));
    EXPECT_FALSE(grid.is_tile_occupied(11, 10));
    EXPECT_FALSE(grid.is_tile_occupied(10, 11));
    EXPECT_FALSE(grid.is_tile_occupied(11, 11));
}

TEST_F(DemolitionHandlerTest, EventEmittedOnDemolition) {
    uint32_t id = spawn_active_building(5, 10, 1);

    EXPECT_TRUE(handler->get_pending_events().empty());

    handler->handle_demolish(id, 1);

    EXPECT_EQ(handler->get_pending_events().size(), 1u);

    const auto& event = handler->get_pending_events()[0];
    EXPECT_EQ(event.entity_id, id);
    EXPECT_EQ(event.owner_id, 1);
    EXPECT_EQ(event.grid_x, 5);
    EXPECT_EQ(event.grid_y, 10);
    EXPECT_TRUE(event.was_player_initiated);
}

TEST_F(DemolitionHandlerTest, ClearPendingEvents) {
    uint32_t id = spawn_active_building(5, 10, 0);
    handler->handle_demolish(id, 0);

    EXPECT_EQ(handler->get_pending_events().size(), 1u);
    handler->clear_pending_events();
    EXPECT_TRUE(handler->get_pending_events().empty());
}

TEST_F(DemolitionHandlerTest, DemolitionRequestFromDeZoneFlow) {
    uint32_t id = spawn_active_building(5, 10, 0);

    auto result = handler->handle_demolition_request(5, 10);

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.cost, 0u); // System-initiated = no cost
    EXPECT_EQ(result.reason, DemolitionResult::Reason::Ok);

    // Event should have was_player_initiated = false
    EXPECT_EQ(handler->get_pending_events().size(), 1u);
    EXPECT_FALSE(handler->get_pending_events()[0].was_player_initiated);
}

TEST_F(DemolitionHandlerTest, DemolitionRequestAtEmptyPosition) {
    auto result = handler->handle_demolition_request(50, 50);

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.reason, DemolitionResult::Reason::EntityNotFound);
}

TEST_F(DemolitionHandlerTest, InsufficientCreditsRejection) {
    uint32_t id = spawn_active_building(5, 10, 0);

    // Make credits restrictive (always fails)
    credits->set_debug_restrictive(true);

    auto result = handler->handle_demolish(id, 0);

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.reason, DemolitionResult::Reason::InsufficientCredits);

    // Building should still be Active
    const BuildingEntity* entity = factory->get_entity(id);
    EXPECT_EQ(entity->building.getBuildingState(), BuildingState::Active);
}

TEST_F(DemolitionHandlerTest, DebrisDataCreatedOnDemolition) {
    uint32_t id = spawn_active_building(5, 10, 0);

    handler->handle_demolish(id, 0);

    const BuildingEntity* entity = factory->get_entity(id);
    ASSERT_NE(entity, nullptr);
    EXPECT_TRUE(entity->has_debris);
    EXPECT_EQ(entity->debris.original_template_id, 1u);
    EXPECT_EQ(entity->debris.footprint_w, 1);
    EXPECT_EQ(entity->debris.footprint_h, 1);
    EXPECT_EQ(entity->debris.clear_timer, DebrisComponent::DEFAULT_CLEAR_TIMER);
}

TEST_F(DemolitionHandlerTest, CustomCostConfig) {
    DemolitionCostConfig config;
    config.base_cost_ratio = 0.5f;
    config.active_modifier = 2.0f;
    handler->set_cost_config(config);

    uint32_t id = spawn_active_building(5, 10, 0);

    auto result = handler->handle_demolish(id, 0);

    // construction_cost(1000) * base_ratio(0.5) * active_modifier(2.0) = 1000
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.cost, 1000u);
}

} // anonymous namespace
