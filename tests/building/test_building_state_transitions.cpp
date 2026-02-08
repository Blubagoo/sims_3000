/**
 * @file test_building_state_transitions.cpp
 * @brief Tests for BuildingStateTransitionSystem (ticket 4-028)
 */

#include <gtest/gtest.h>
#include <sims3000/building/BuildingStateTransitionSystem.h>
#include <sims3000/building/BuildingFactory.h>
#include <sims3000/building/BuildingGrid.h>
#include <sims3000/building/ForwardDependencyStubs.h>
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
TemplateSelectionResult make_test_selection(uint32_t template_id = 1) {
    TemplateSelectionResult sel;
    sel.template_id = template_id;
    sel.rotation = 0;
    sel.color_accent_index = 0;
    return sel;
}

class BuildingStateTransitionTest : public ::testing::Test {
protected:
    void SetUp() override {
        building_grid.initialize(128, 128);
        zone_system = std::make_unique<ZoneSystem>(nullptr, nullptr, 128);
        factory = std::make_unique<BuildingFactory>(&building_grid, zone_system.get());

        stub_energy = std::make_unique<StubEnergyProvider>();
        stub_fluid = std::make_unique<StubFluidProvider>();
        stub_transport = std::make_unique<StubTransportProvider>();

        system = std::make_unique<BuildingStateTransitionSystem>(
            factory.get(), &building_grid,
            stub_energy.get(), stub_fluid.get(), stub_transport.get()
        );
    }

    // Helper to spawn an Active building (bypass Materializing)
    uint32_t spawn_active_building(int32_t x = 5, int32_t y = 5, uint8_t owner = 0) {
        auto templ = make_test_template();
        auto sel = make_test_selection();
        uint32_t eid = factory->spawn_building(templ, sel, x, y, owner, 0);

        // Transition to Active (skip Materializing)
        auto* entity = factory->get_entity_mut(eid);
        entity->building.setBuildingState(BuildingState::Active);
        entity->building.state_changed_tick = 0;
        entity->has_construction = false;
        return eid;
    }

    BuildingGrid building_grid;
    std::unique_ptr<ZoneSystem> zone_system;
    std::unique_ptr<BuildingFactory> factory;
    std::unique_ptr<StubEnergyProvider> stub_energy;
    std::unique_ptr<StubFluidProvider> stub_fluid;
    std::unique_ptr<StubTransportProvider> stub_transport;
    std::unique_ptr<BuildingStateTransitionSystem> system;
};

// =========================================================================
// Basic Construction
// =========================================================================

TEST_F(BuildingStateTransitionTest, ConstructionWithValidDependencies) {
    const auto& config = system->get_config();
    EXPECT_EQ(config.service_grace_period, 100u);
    EXPECT_EQ(config.abandon_timer_ticks, 200u);
    EXPECT_EQ(config.derelict_timer_ticks, 500u);
}

TEST_F(BuildingStateTransitionTest, SetConfig) {
    StateTransitionConfig config;
    config.service_grace_period = 50;
    config.abandon_timer_ticks = 100;
    config.derelict_timer_ticks = 200;
    system->set_config(config);

    const auto& result = system->get_config();
    EXPECT_EQ(result.service_grace_period, 50u);
    EXPECT_EQ(result.abandon_timer_ticks, 100u);
    EXPECT_EQ(result.derelict_timer_ticks, 200u);
}

// =========================================================================
// Active Stays Active With Services
// =========================================================================

TEST_F(BuildingStateTransitionTest, ActiveStaysActiveWithAllServices) {
    uint32_t eid = spawn_active_building();

    // All stubs are permissive by default
    for (uint32_t tick = 1; tick <= 200; ++tick) {
        system->tick(tick);
    }

    const auto* entity = factory->get_entity(eid);
    ASSERT_NE(entity, nullptr);
    EXPECT_EQ(entity->building.getBuildingState(), BuildingState::Active);
    EXPECT_TRUE(system->get_pending_abandoned_events().empty());
}

// =========================================================================
// Grace Period Tracking
// =========================================================================

TEST_F(BuildingStateTransitionTest, GracePeriodDoesNotTransitionEarly) {
    uint32_t eid = spawn_active_building();

    // Use short grace period for testing
    StateTransitionConfig config;
    config.service_grace_period = 10;
    config.abandon_timer_ticks = 50;
    config.derelict_timer_ticks = 100;
    system->set_config(config);

    // Cut energy
    stub_energy->set_debug_restrictive(true);

    // Tick 9 times - should still be Active (need > 10 ticks without service)
    for (uint32_t tick = 1; tick <= 10; ++tick) {
        system->tick(tick);
    }

    const auto* entity = factory->get_entity(eid);
    ASSERT_NE(entity, nullptr);
    EXPECT_EQ(entity->building.getBuildingState(), BuildingState::Active);
}

TEST_F(BuildingStateTransitionTest, GracePeriodResetsWhenServiceRestored) {
    uint32_t eid = spawn_active_building();

    StateTransitionConfig config;
    config.service_grace_period = 5;
    config.abandon_timer_ticks = 50;
    config.derelict_timer_ticks = 100;
    system->set_config(config);

    // Cut energy for 4 ticks
    stub_energy->set_debug_restrictive(true);
    for (uint32_t tick = 1; tick <= 4; ++tick) {
        system->tick(tick);
    }

    // Restore energy
    stub_energy->set_debug_restrictive(false);
    system->tick(5);

    // Cut again for 4 ticks - should still be Active since grace reset
    stub_energy->set_debug_restrictive(true);
    for (uint32_t tick = 6; tick <= 9; ++tick) {
        system->tick(tick);
    }

    const auto* entity = factory->get_entity(eid);
    ASSERT_NE(entity, nullptr);
    EXPECT_EQ(entity->building.getBuildingState(), BuildingState::Active);
}

// =========================================================================
// Active -> Abandoned After Grace
// =========================================================================

TEST_F(BuildingStateTransitionTest, ActiveToAbandonedAfterGracePeriod) {
    uint32_t eid = spawn_active_building();

    StateTransitionConfig config;
    config.service_grace_period = 5;
    config.abandon_timer_ticks = 50;
    config.derelict_timer_ticks = 100;
    system->set_config(config);

    // Cut energy
    stub_energy->set_debug_restrictive(true);

    // Tick enough to exceed grace period (>5 ticks)
    for (uint32_t tick = 1; tick <= 7; ++tick) {
        system->tick(tick);
    }

    const auto* entity = factory->get_entity(eid);
    ASSERT_NE(entity, nullptr);
    EXPECT_EQ(entity->building.getBuildingState(), BuildingState::Abandoned);
}

TEST_F(BuildingStateTransitionTest, AbandonedEventEmittedOnTransition) {
    uint32_t eid = spawn_active_building(5, 5, 2);

    StateTransitionConfig config;
    config.service_grace_period = 3;
    config.abandon_timer_ticks = 50;
    system->set_config(config);

    stub_fluid->set_debug_restrictive(true);

    for (uint32_t tick = 1; tick <= 5; ++tick) {
        system->tick(tick);
    }

    const auto& events = system->get_pending_abandoned_events();
    ASSERT_EQ(events.size(), 1u);
    EXPECT_EQ(events[0].entity_id, eid);
    EXPECT_EQ(events[0].owner_id, 2u);
    EXPECT_EQ(events[0].grid_x, 5);
    EXPECT_EQ(events[0].grid_y, 5);
}

// =========================================================================
// Abandoned -> Active On Restore
// =========================================================================

TEST_F(BuildingStateTransitionTest, AbandonedToActiveOnServiceRestore) {
    uint32_t eid = spawn_active_building();

    StateTransitionConfig config;
    config.service_grace_period = 3;
    config.abandon_timer_ticks = 50;
    config.derelict_timer_ticks = 100;
    system->set_config(config);

    // Cut energy to force Abandoned
    stub_energy->set_debug_restrictive(true);
    for (uint32_t tick = 1; tick <= 5; ++tick) {
        system->tick(tick);
    }

    EXPECT_EQ(factory->get_entity(eid)->building.getBuildingState(), BuildingState::Abandoned);
    system->clear_all_pending_events();

    // Restore energy
    stub_energy->set_debug_restrictive(false);
    system->tick(6);

    const auto* entity = factory->get_entity(eid);
    EXPECT_EQ(entity->building.getBuildingState(), BuildingState::Active);

    const auto& restored_events = system->get_pending_restored_events();
    ASSERT_EQ(restored_events.size(), 1u);
    EXPECT_EQ(restored_events[0].entity_id, eid);
}

// =========================================================================
// Abandoned -> Derelict On Timer
// =========================================================================

TEST_F(BuildingStateTransitionTest, AbandonedToDerelictOnTimerExpiry) {
    uint32_t eid = spawn_active_building();

    StateTransitionConfig config;
    config.service_grace_period = 2;
    config.abandon_timer_ticks = 10;
    config.derelict_timer_ticks = 100;
    system->set_config(config);

    // Cut all services
    stub_energy->set_debug_restrictive(true);
    stub_fluid->set_debug_restrictive(true);
    stub_transport->set_debug_restrictive(true);

    // Exceed grace period -> Abandoned
    for (uint32_t tick = 1; tick <= 4; ++tick) {
        system->tick(tick);
    }
    EXPECT_EQ(factory->get_entity(eid)->building.getBuildingState(), BuildingState::Abandoned);
    system->clear_all_pending_events();

    // Now tick through abandon timer (10 ticks)
    // The abandon_timer is set to 10 and decremented each tick while abandoned
    for (uint32_t tick = 5; tick <= 15; ++tick) {
        system->tick(tick);
    }

    const auto* entity = factory->get_entity(eid);
    ASSERT_NE(entity, nullptr);
    EXPECT_EQ(entity->building.getBuildingState(), BuildingState::Derelict);

    const auto& derelict_events = system->get_pending_derelict_events();
    ASSERT_GE(derelict_events.size(), 1u);
    EXPECT_EQ(derelict_events[0].entity_id, eid);
}

// =========================================================================
// Derelict -> Deconstructed On Timer
// =========================================================================

TEST_F(BuildingStateTransitionTest, DerelictToDeconstructedOnTimerExpiry) {
    uint32_t eid = spawn_active_building();

    StateTransitionConfig config;
    config.service_grace_period = 2;
    config.abandon_timer_ticks = 5;
    config.derelict_timer_ticks = 10;
    system->set_config(config);

    // Cut all services
    stub_energy->set_debug_restrictive(true);
    stub_fluid->set_debug_restrictive(true);
    stub_transport->set_debug_restrictive(true);

    // Grace period -> Abandoned (tick 1-3)
    for (uint32_t tick = 1; tick <= 4; ++tick) {
        system->tick(tick);
    }
    EXPECT_EQ(factory->get_entity(eid)->building.getBuildingState(), BuildingState::Abandoned);

    // Abandon timer -> Derelict (tick 5-9)
    for (uint32_t tick = 5; tick <= 10; ++tick) {
        system->tick(tick);
    }
    EXPECT_EQ(factory->get_entity(eid)->building.getBuildingState(), BuildingState::Derelict);
    system->clear_all_pending_events();

    // Remember what tick the derelict transition happened
    uint32_t derelict_tick = factory->get_entity(eid)->building.state_changed_tick;

    // Derelict timer -> Deconstructed
    // Need derelict_timer_ticks (10) ticks after state_changed_tick
    for (uint32_t tick = derelict_tick + 1; tick <= derelict_tick + 11; ++tick) {
        system->tick(tick);
    }

    const auto* entity = factory->get_entity(eid);
    ASSERT_NE(entity, nullptr);
    EXPECT_EQ(entity->building.getBuildingState(), BuildingState::Deconstructed);
    EXPECT_TRUE(entity->has_debris);
    EXPECT_EQ(entity->debris.original_template_id, 1u);

    const auto& decon_events = system->get_pending_deconstructed_events();
    ASSERT_GE(decon_events.size(), 1u);
    EXPECT_EQ(decon_events[0].entity_id, eid);
    EXPECT_FALSE(decon_events[0].was_player_initiated);
}

// =========================================================================
// Full Lifecycle
// =========================================================================

TEST_F(BuildingStateTransitionTest, FullLifecycleActiveToDeconstructed) {
    uint32_t eid = spawn_active_building();

    StateTransitionConfig config;
    config.service_grace_period = 2;
    config.abandon_timer_ticks = 3;
    config.derelict_timer_ticks = 4;
    system->set_config(config);

    // Cut services
    stub_energy->set_debug_restrictive(true);

    // Phase 1: Grace period (3 ticks to exceed grace of 2)
    system->tick(1);
    system->tick(2);
    system->tick(3);
    EXPECT_EQ(factory->get_entity(eid)->building.getBuildingState(), BuildingState::Abandoned);

    // Phase 2: Abandon timer (3 ticks)
    system->tick(4);
    system->tick(5);
    system->tick(6);
    EXPECT_EQ(factory->get_entity(eid)->building.getBuildingState(), BuildingState::Derelict);

    uint32_t derelict_tick = factory->get_entity(eid)->building.state_changed_tick;

    // Phase 3: Derelict timer (4 ticks after state_changed_tick)
    for (uint32_t t = derelict_tick + 1; t <= derelict_tick + 5; ++t) {
        system->tick(t);
    }
    EXPECT_EQ(factory->get_entity(eid)->building.getBuildingState(), BuildingState::Deconstructed);
}

// =========================================================================
// Grid Cleared On Deconstructed
// =========================================================================

TEST_F(BuildingStateTransitionTest, GridClearedOnDeconstructed) {
    uint32_t eid = spawn_active_building(10, 10);

    StateTransitionConfig config;
    config.service_grace_period = 1;
    config.abandon_timer_ticks = 1;
    config.derelict_timer_ticks = 1;
    system->set_config(config);

    // Verify building is in grid
    EXPECT_NE(building_grid.get_building_at(10, 10), sims3000::building::INVALID_ENTITY);

    // Cut services
    stub_energy->set_debug_restrictive(true);

    // Grace -> Abandoned (tick 1-2)
    system->tick(1);
    system->tick(2);

    // Abandon -> Derelict (tick 3)
    system->tick(3);

    uint32_t derelict_tick = factory->get_entity(eid)->building.state_changed_tick;

    // Derelict -> Deconstructed (tick after derelict_timer_ticks)
    system->tick(derelict_tick + 1);
    system->tick(derelict_tick + 2);

    // Grid should be cleared
    EXPECT_EQ(building_grid.get_building_at(10, 10), sims3000::building::INVALID_ENTITY);
}

// =========================================================================
// Events For Each Transition
// =========================================================================

TEST_F(BuildingStateTransitionTest, EventsEmittedForEachTransition) {
    spawn_active_building(5, 5, 1);

    StateTransitionConfig config;
    config.service_grace_period = 1;
    config.abandon_timer_ticks = 2;
    config.derelict_timer_ticks = 2;
    system->set_config(config);

    stub_energy->set_debug_restrictive(true);

    // Grace -> Abandoned
    system->tick(1);
    system->tick(2);
    EXPECT_EQ(system->get_pending_abandoned_events().size(), 1u);

    // Abandon timer -> Derelict
    system->tick(3);
    system->tick(4);
    EXPECT_GE(system->get_pending_derelict_events().size(), 1u);

    // Derelict -> Deconstructed
    // Find state_changed_tick
    auto& entities = factory->get_entities_mut();
    uint32_t derelict_tick = entities[0].building.state_changed_tick;
    for (uint32_t t = derelict_tick + 1; t <= derelict_tick + 3; ++t) {
        system->tick(t);
    }
    EXPECT_GE(system->get_pending_deconstructed_events().size(), 1u);
}

TEST_F(BuildingStateTransitionTest, ClearAllPendingEvents) {
    spawn_active_building();

    StateTransitionConfig config;
    config.service_grace_period = 1;
    system->set_config(config);

    stub_energy->set_debug_restrictive(true);
    system->tick(1);
    system->tick(2);

    EXPECT_FALSE(system->get_pending_abandoned_events().empty());

    system->clear_all_pending_events();

    EXPECT_TRUE(system->get_pending_abandoned_events().empty());
    EXPECT_TRUE(system->get_pending_restored_events().empty());
    EXPECT_TRUE(system->get_pending_derelict_events().empty());
    EXPECT_TRUE(system->get_pending_deconstructed_events().empty());
}

// =========================================================================
// Configurable Timers
// =========================================================================

TEST_F(BuildingStateTransitionTest, LongerGracePeriodDelaysAbandonment) {
    uint32_t eid = spawn_active_building();

    StateTransitionConfig config;
    config.service_grace_period = 50;
    config.abandon_timer_ticks = 50;
    system->set_config(config);

    stub_energy->set_debug_restrictive(true);

    // 30 ticks should not be enough
    for (uint32_t tick = 1; tick <= 30; ++tick) {
        system->tick(tick);
    }

    EXPECT_EQ(factory->get_entity(eid)->building.getBuildingState(), BuildingState::Active);
}

// =========================================================================
// Stub Restrictive Mode For Testing
// =========================================================================

TEST_F(BuildingStateTransitionTest, FluidLossAlsoTriggersAbandonment) {
    uint32_t eid = spawn_active_building();

    StateTransitionConfig config;
    config.service_grace_period = 3;
    system->set_config(config);

    // Only fluid is restrictive
    stub_fluid->set_debug_restrictive(true);

    for (uint32_t tick = 1; tick <= 5; ++tick) {
        system->tick(tick);
    }

    EXPECT_EQ(factory->get_entity(eid)->building.getBuildingState(), BuildingState::Abandoned);
}

TEST_F(BuildingStateTransitionTest, TransportLossTriggersAbandonment) {
    uint32_t eid = spawn_active_building();

    StateTransitionConfig config;
    config.service_grace_period = 3;
    system->set_config(config);

    // Only transport is restrictive
    stub_transport->set_debug_restrictive(true);

    for (uint32_t tick = 1; tick <= 5; ++tick) {
        system->tick(tick);
    }

    EXPECT_EQ(factory->get_entity(eid)->building.getBuildingState(), BuildingState::Abandoned);
}

// =========================================================================
// Materializing Buildings Are Ignored
// =========================================================================

TEST_F(BuildingStateTransitionTest, MaterializingBuildingsNotAffected) {
    auto templ = make_test_template();
    auto sel = make_test_selection();
    uint32_t eid = factory->spawn_building(templ, sel, 5, 5, 0, 0);
    // Building starts as Materializing

    StateTransitionConfig config;
    config.service_grace_period = 1;
    system->set_config(config);

    stub_energy->set_debug_restrictive(true);

    for (uint32_t tick = 1; tick <= 10; ++tick) {
        system->tick(tick);
    }

    // Should still be Materializing (not affected by state transition system)
    EXPECT_EQ(factory->get_entity(eid)->building.getBuildingState(), BuildingState::Materializing);
}

// =========================================================================
// Multi-tile Footprint Grid Clear
// =========================================================================

TEST_F(BuildingStateTransitionTest, MultiTileFootprintClearedOnDeconstructed) {
    // Create 2x2 building
    auto templ = make_test_template(1, 2, 2);
    auto sel = make_test_selection();
    uint32_t eid = factory->spawn_building(templ, sel, 10, 10, 0, 0);

    // Set to Active
    auto* entity = factory->get_entity_mut(eid);
    entity->building.setBuildingState(BuildingState::Active);
    entity->has_construction = false;

    StateTransitionConfig config;
    config.service_grace_period = 1;
    config.abandon_timer_ticks = 1;
    config.derelict_timer_ticks = 1;
    system->set_config(config);

    // Verify all 4 tiles occupied
    EXPECT_NE(building_grid.get_building_at(10, 10), sims3000::building::INVALID_ENTITY);
    EXPECT_NE(building_grid.get_building_at(11, 10), sims3000::building::INVALID_ENTITY);
    EXPECT_NE(building_grid.get_building_at(10, 11), sims3000::building::INVALID_ENTITY);
    EXPECT_NE(building_grid.get_building_at(11, 11), sims3000::building::INVALID_ENTITY);

    stub_energy->set_debug_restrictive(true);

    // Grace -> Abandoned -> Derelict -> Deconstructed
    system->tick(1);
    system->tick(2); // Abandoned
    system->tick(3); // Derelict

    uint32_t derelict_tick = factory->get_entity(eid)->building.state_changed_tick;
    system->tick(derelict_tick + 1);
    system->tick(derelict_tick + 2); // Deconstructed

    // All 4 tiles should be cleared
    EXPECT_EQ(building_grid.get_building_at(10, 10), sims3000::building::INVALID_ENTITY);
    EXPECT_EQ(building_grid.get_building_at(11, 10), sims3000::building::INVALID_ENTITY);
    EXPECT_EQ(building_grid.get_building_at(10, 11), sims3000::building::INVALID_ENTITY);
    EXPECT_EQ(building_grid.get_building_at(11, 11), sims3000::building::INVALID_ENTITY);
}

} // anonymous namespace
