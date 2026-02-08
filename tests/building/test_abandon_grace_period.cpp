/**
 * @file test_abandon_grace_period.cpp
 * @brief Tests for per-service abandon grace period logic (Ticket 4-029)
 *
 * Verifies:
 * - Per-service configurable grace periods
 * - Intermittent outage does not cause abandon (flickering protection)
 * - Sustained outage causes abandon at threshold
 * - Immediate transport abandon (grace = 0)
 * - Counter reset on service restore
 * - Per-service independence
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

BuildingTemplate make_test_template(uint32_t id = 1) {
    BuildingTemplate templ;
    templ.template_id = id;
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

TemplateSelectionResult make_test_selection(uint32_t template_id = 1) {
    TemplateSelectionResult sel;
    sel.template_id = template_id;
    sel.rotation = 0;
    sel.color_accent_index = 0;
    return sel;
}

class AbandonGracePeriodTest : public ::testing::Test {
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

    uint32_t spawn_active_building(int32_t x = 5, int32_t y = 5, uint8_t owner = 0) {
        auto templ = make_test_template();
        auto sel = make_test_selection();
        uint32_t eid = factory->spawn_building(templ, sel, x, y, owner, 0);
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
// Per-service grace period config defaults
// =========================================================================

TEST_F(AbandonGracePeriodTest, DefaultConfigHasPerServiceGracePeriods) {
    const auto& config = system->get_config();
    // Default per-service periods use USE_LEGACY sentinel
    EXPECT_EQ(config.energy_grace_period, StateTransitionConfig::USE_LEGACY);
    EXPECT_EQ(config.fluid_grace_period, StateTransitionConfig::USE_LEGACY);
    EXPECT_EQ(config.transport_grace_period, StateTransitionConfig::USE_LEGACY);
    // Effective values resolve to service_grace_period (100)
    EXPECT_EQ(config.get_energy_grace(), 100u);
    EXPECT_EQ(config.get_fluid_grace(), 100u);
    EXPECT_EQ(config.get_transport_grace(), 100u);
}

TEST_F(AbandonGracePeriodTest, SetPerServiceGracePeriods) {
    StateTransitionConfig config;
    config.energy_grace_period = 50;
    config.fluid_grace_period = 75;
    config.transport_grace_period = 10;
    system->set_config(config);

    const auto& result = system->get_config();
    EXPECT_EQ(result.energy_grace_period, 50u);
    EXPECT_EQ(result.fluid_grace_period, 75u);
    EXPECT_EQ(result.transport_grace_period, 10u);
}

// =========================================================================
// Intermittent power outage does NOT cause abandon (flickering protection)
// =========================================================================

TEST_F(AbandonGracePeriodTest, IntermittentPowerOutageDoesNotCauseAbandon) {
    uint32_t eid = spawn_active_building();

    StateTransitionConfig config;
    config.energy_grace_period = 10;
    config.fluid_grace_period = 10;
    config.transport_grace_period = 0;
    config.abandon_timer_ticks = 50;
    system->set_config(config);

    // Simulate flickering: lose power for 5 ticks, restore, repeat
    // Each outage is under the 10-tick grace period
    for (int cycle = 0; cycle < 5; ++cycle) {
        // Power off for 5 ticks
        stub_energy->set_debug_restrictive(true);
        for (uint32_t t = 0; t < 5; ++t) {
            system->tick(cycle * 10 + t + 1);
        }
        // Power on for 5 ticks
        stub_energy->set_debug_restrictive(false);
        for (uint32_t t = 5; t < 10; ++t) {
            system->tick(cycle * 10 + t + 1);
        }
    }

    const auto* entity = factory->get_entity(eid);
    ASSERT_NE(entity, nullptr);
    EXPECT_EQ(entity->building.getBuildingState(), BuildingState::Active);
    EXPECT_TRUE(system->get_pending_abandoned_events().empty());
}

// =========================================================================
// Sustained outage causes abandon at threshold
// =========================================================================

TEST_F(AbandonGracePeriodTest, SustainedEnergyOutageCausesAbandon) {
    uint32_t eid = spawn_active_building();

    StateTransitionConfig config;
    config.energy_grace_period = 10;
    config.fluid_grace_period = 100;
    config.transport_grace_period = 100;
    config.abandon_timer_ticks = 50;
    system->set_config(config);

    // Cut energy permanently
    stub_energy->set_debug_restrictive(true);

    // Tick 10 times - should still be Active (need > 10 ticks)
    for (uint32_t tick = 1; tick <= 10; ++tick) {
        system->tick(tick);
    }
    EXPECT_EQ(factory->get_entity(eid)->building.getBuildingState(), BuildingState::Active);

    // Tick 11th time - should now be Abandoned
    system->tick(11);
    EXPECT_EQ(factory->get_entity(eid)->building.getBuildingState(), BuildingState::Abandoned);
}

TEST_F(AbandonGracePeriodTest, SustainedFluidOutageCausesAbandon) {
    uint32_t eid = spawn_active_building();

    StateTransitionConfig config;
    config.energy_grace_period = 100;
    config.fluid_grace_period = 5;
    config.transport_grace_period = 100;
    config.abandon_timer_ticks = 50;
    system->set_config(config);

    // Cut fluid permanently
    stub_fluid->set_debug_restrictive(true);

    // 5 ticks - should be Active
    for (uint32_t tick = 1; tick <= 5; ++tick) {
        system->tick(tick);
    }
    EXPECT_EQ(factory->get_entity(eid)->building.getBuildingState(), BuildingState::Active);

    // 6th tick - should be Abandoned
    system->tick(6);
    EXPECT_EQ(factory->get_entity(eid)->building.getBuildingState(), BuildingState::Abandoned);
}

// =========================================================================
// Immediate transport abandon (grace = 0)
// =========================================================================

TEST_F(AbandonGracePeriodTest, ImmediateTransportAbandonWithZeroGrace) {
    uint32_t eid = spawn_active_building();

    StateTransitionConfig config;
    config.energy_grace_period = 100;
    config.fluid_grace_period = 100;
    config.transport_grace_period = 0;  // Immediate
    config.abandon_timer_ticks = 50;
    system->set_config(config);

    // Cut transport
    stub_transport->set_debug_restrictive(true);

    // Just one tick should cause abandon (>0 = 1 > 0)
    system->tick(1);

    const auto* entity = factory->get_entity(eid);
    ASSERT_NE(entity, nullptr);
    EXPECT_EQ(entity->building.getBuildingState(), BuildingState::Abandoned);
}

TEST_F(AbandonGracePeriodTest, TransportWithNonZeroGraceDelaysAbandon) {
    uint32_t eid = spawn_active_building();

    StateTransitionConfig config;
    config.energy_grace_period = 100;
    config.fluid_grace_period = 100;
    config.transport_grace_period = 5;
    config.abandon_timer_ticks = 50;
    system->set_config(config);

    stub_transport->set_debug_restrictive(true);

    // 5 ticks - should still be Active
    for (uint32_t tick = 1; tick <= 5; ++tick) {
        system->tick(tick);
    }
    EXPECT_EQ(factory->get_entity(eid)->building.getBuildingState(), BuildingState::Active);

    // 6th tick - should be Abandoned
    system->tick(6);
    EXPECT_EQ(factory->get_entity(eid)->building.getBuildingState(), BuildingState::Abandoned);
}

// =========================================================================
// Counter reset on service restore
// =========================================================================

TEST_F(AbandonGracePeriodTest, EnergyGraceCounterResetsOnRestore) {
    uint32_t eid = spawn_active_building();

    StateTransitionConfig config;
    config.energy_grace_period = 10;
    config.fluid_grace_period = 100;
    config.transport_grace_period = 100;
    config.abandon_timer_ticks = 50;
    system->set_config(config);

    // Cut energy for 8 ticks (close to but under 10 grace)
    stub_energy->set_debug_restrictive(true);
    for (uint32_t tick = 1; tick <= 8; ++tick) {
        system->tick(tick);
    }
    EXPECT_EQ(factory->get_entity(eid)->building.getBuildingState(), BuildingState::Active);

    // Restore energy - counter should reset
    stub_energy->set_debug_restrictive(false);
    system->tick(9);

    // Cut energy again for 8 ticks - should still be Active (reset happened)
    stub_energy->set_debug_restrictive(true);
    for (uint32_t tick = 10; tick <= 17; ++tick) {
        system->tick(tick);
    }
    EXPECT_EQ(factory->get_entity(eid)->building.getBuildingState(), BuildingState::Active);
}

// =========================================================================
// Per-service independence
// =========================================================================

TEST_F(AbandonGracePeriodTest, PerServiceGracePeriodsAreIndependent) {
    uint32_t eid = spawn_active_building();

    StateTransitionConfig config;
    config.energy_grace_period = 20;
    config.fluid_grace_period = 5;
    config.transport_grace_period = 100;
    config.abandon_timer_ticks = 50;
    system->set_config(config);

    // Cut both energy and fluid
    stub_energy->set_debug_restrictive(true);
    stub_fluid->set_debug_restrictive(true);

    // After 5 ticks, fluid grace exceeded but energy grace not yet
    for (uint32_t tick = 1; tick <= 5; ++tick) {
        system->tick(tick);
    }
    // Still Active - fluid is at 5, needs >5
    EXPECT_EQ(factory->get_entity(eid)->building.getBuildingState(), BuildingState::Active);

    // 6th tick - fluid grace exceeded (6 > 5)
    system->tick(6);
    EXPECT_EQ(factory->get_entity(eid)->building.getBuildingState(), BuildingState::Abandoned);
}

TEST_F(AbandonGracePeriodTest, EnergyLossOnlyUsesEnergyGracePeriod) {
    uint32_t eid = spawn_active_building();

    StateTransitionConfig config;
    config.energy_grace_period = 3;
    config.fluid_grace_period = 100;
    config.transport_grace_period = 100;
    config.abandon_timer_ticks = 50;
    system->set_config(config);

    // Only energy is cut
    stub_energy->set_debug_restrictive(true);

    // 3 ticks - still Active
    for (uint32_t tick = 1; tick <= 3; ++tick) {
        system->tick(tick);
    }
    EXPECT_EQ(factory->get_entity(eid)->building.getBuildingState(), BuildingState::Active);

    // 4th tick - energy grace exceeded
    system->tick(4);
    EXPECT_EQ(factory->get_entity(eid)->building.getBuildingState(), BuildingState::Abandoned);
}

// =========================================================================
// Abandoned event contains correct data
// =========================================================================

TEST_F(AbandonGracePeriodTest, AbandonedEventEmittedWithCorrectData) {
    uint32_t eid = spawn_active_building(10, 15, 3);

    StateTransitionConfig config;
    config.energy_grace_period = 2;
    config.fluid_grace_period = 100;
    config.transport_grace_period = 100;
    config.abandon_timer_ticks = 50;
    system->set_config(config);

    stub_energy->set_debug_restrictive(true);

    for (uint32_t tick = 1; tick <= 4; ++tick) {
        system->tick(tick);
    }

    const auto& events = system->get_pending_abandoned_events();
    ASSERT_EQ(events.size(), 1u);
    EXPECT_EQ(events[0].entity_id, eid);
    EXPECT_EQ(events[0].owner_id, 3u);
    EXPECT_EQ(events[0].grid_x, 10);
    EXPECT_EQ(events[0].grid_y, 15);
}

// =========================================================================
// Brief loss + restore does NOT trigger abandon (flicker protection)
// =========================================================================

TEST_F(AbandonGracePeriodTest, BriefLossAndRestoreDoesNotTriggerAbandon) {
    uint32_t eid = spawn_active_building();

    StateTransitionConfig config;
    config.energy_grace_period = 5;
    config.fluid_grace_period = 5;
    config.transport_grace_period = 100;
    config.abandon_timer_ticks = 50;
    system->set_config(config);

    // Brief 2-tick loss, restore, repeat many times
    for (int cycle = 0; cycle < 20; ++cycle) {
        stub_energy->set_debug_restrictive(true);
        system->tick(cycle * 3 + 1);
        system->tick(cycle * 3 + 2);
        stub_energy->set_debug_restrictive(false);
        system->tick(cycle * 3 + 3);
    }

    const auto* entity = factory->get_entity(eid);
    ASSERT_NE(entity, nullptr);
    EXPECT_EQ(entity->building.getBuildingState(), BuildingState::Active);
}

// =========================================================================
// Multiple buildings independently tracked
// =========================================================================

TEST_F(AbandonGracePeriodTest, MultipleBuildingsTrackedIndependently) {
    uint32_t eid1 = spawn_active_building(5, 5);
    uint32_t eid2 = spawn_active_building(10, 10);

    StateTransitionConfig config;
    config.energy_grace_period = 5;
    config.fluid_grace_period = 100;
    config.transport_grace_period = 100;
    config.abandon_timer_ticks = 50;
    system->set_config(config);

    // Both lose energy
    stub_energy->set_debug_restrictive(true);
    for (uint32_t tick = 1; tick <= 3; ++tick) {
        system->tick(tick);
    }

    // Both still Active
    EXPECT_EQ(factory->get_entity(eid1)->building.getBuildingState(), BuildingState::Active);
    EXPECT_EQ(factory->get_entity(eid2)->building.getBuildingState(), BuildingState::Active);

    // Continue to exceed grace
    for (uint32_t tick = 4; tick <= 7; ++tick) {
        system->tick(tick);
    }

    // Both should be Abandoned
    EXPECT_EQ(factory->get_entity(eid1)->building.getBuildingState(), BuildingState::Abandoned);
    EXPECT_EQ(factory->get_entity(eid2)->building.getBuildingState(), BuildingState::Abandoned);
}

} // anonymous namespace
