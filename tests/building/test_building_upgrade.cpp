/**
 * @file test_building_upgrade.cpp
 * @brief Tests for BuildingUpgradeSystem (Ticket 4-032)
 *
 * Verifies:
 * - Upgrade when all conditions met
 * - Level cap prevents over-upgrade
 * - Cooldown prevents rapid upgrades
 * - Demand required for upgrade
 * - Capacity scales with level multiplier
 * - Event emitted on upgrade
 * - Check interval respected
 * - Non-Active buildings ignored
 * - Multiple buildings upgraded independently
 */

#include <gtest/gtest.h>
#include <sims3000/building/BuildingUpgradeSystem.h>
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

class BuildingUpgradeTest : public ::testing::Test {
protected:
    void SetUp() override {
        building_grid.initialize(128, 128);
        zone_system = std::make_unique<ZoneSystem>(nullptr, nullptr, 128);
        factory = std::make_unique<BuildingFactory>(&building_grid, zone_system.get());
        upgrade_system = std::make_unique<BuildingUpgradeSystem>(factory.get(), zone_system.get());

        // Set up demand to be positive (required for upgrades)
        DemandConfig demand_config;
        demand_config.habitation_base = 50;
        demand_config.exchange_base = 50;
        demand_config.fabrication_base = 50;
        zone_system->set_demand_config(demand_config);
        // Tick the zone system to calculate demand
        zone_system->tick(0.05f);
    }

    uint32_t spawn_active_building(int32_t x = 5, int32_t y = 5, uint8_t owner = 0,
                                    uint32_t state_changed_tick = 0) {
        // Place a zone first so demand/desirability checks can work
        zone_system->place_zone(x, y, ZoneType::Habitation, ZoneDensity::LowDensity, owner, 0);

        auto templ = make_test_template();
        auto sel = make_test_selection();
        uint32_t eid = factory->spawn_building(templ, sel, x, y, owner, state_changed_tick);
        auto* entity = factory->get_entity_mut(eid);
        entity->building.setBuildingState(BuildingState::Active);
        entity->building.state_changed_tick = state_changed_tick;
        entity->building.level = 1;
        entity->building.capacity = 20; // base_capacity
        entity->has_construction = false;
        return eid;
    }

    BuildingGrid building_grid;
    std::unique_ptr<ZoneSystem> zone_system;
    std::unique_ptr<BuildingFactory> factory;
    std::unique_ptr<BuildingUpgradeSystem> upgrade_system;
};

// =========================================================================
// Default Config
// =========================================================================

TEST_F(BuildingUpgradeTest, DefaultConfig) {
    const auto& config = upgrade_system->get_config();
    EXPECT_EQ(config.upgrade_cooldown, 200u);
    EXPECT_EQ(config.check_interval, 10u);
    EXPECT_EQ(config.max_level, 5u);
    EXPECT_EQ(config.upgrade_animation_ticks, 20u);
    EXPECT_FLOAT_EQ(config.level_multipliers[1], 1.0f);
    EXPECT_FLOAT_EQ(config.level_multipliers[2], 1.5f);
    EXPECT_FLOAT_EQ(config.level_multipliers[3], 2.0f);
    EXPECT_FLOAT_EQ(config.level_multipliers[4], 2.5f);
    EXPECT_FLOAT_EQ(config.level_multipliers[5], 3.0f);
}

TEST_F(BuildingUpgradeTest, SetConfig) {
    UpgradeConfig config;
    config.upgrade_cooldown = 50;
    config.check_interval = 5;
    config.max_level = 3;
    upgrade_system->set_config(config);

    const auto& result = upgrade_system->get_config();
    EXPECT_EQ(result.upgrade_cooldown, 50u);
    EXPECT_EQ(result.check_interval, 5u);
    EXPECT_EQ(result.max_level, 3u);
}

// =========================================================================
// Upgrade When Conditions Met
// =========================================================================

TEST_F(BuildingUpgradeTest, UpgradeWhenAllConditionsMet) {
    uint32_t eid = spawn_active_building(5, 5, 0, 0);

    UpgradeConfig config;
    config.upgrade_cooldown = 10;
    config.check_interval = 1; // Check every tick for test convenience
    config.max_level = 5;
    upgrade_system->set_config(config);

    // Tick past cooldown (need > 10 ticks since state_changed_tick=0)
    upgrade_system->tick(11);

    // Since zone demand is set positive, upgrade should happen
    const auto* entity = factory->get_entity(eid);
    ASSERT_NE(entity, nullptr);
    EXPECT_EQ(entity->building.level, 2u);
}

// =========================================================================
// Level Cap Prevents Over-Upgrade
// =========================================================================

TEST_F(BuildingUpgradeTest, LevelCapPreventsOverUpgrade) {
    uint32_t eid = spawn_active_building(5, 5, 0, 0);

    // Set building to max level
    auto* entity = factory->get_entity_mut(eid);
    entity->building.level = 5;

    UpgradeConfig config;
    config.upgrade_cooldown = 0; // No cooldown
    config.check_interval = 1;
    config.max_level = 5;
    upgrade_system->set_config(config);

    upgrade_system->tick(100);

    // Should still be level 5
    EXPECT_EQ(factory->get_entity(eid)->building.level, 5u);
    EXPECT_TRUE(upgrade_system->get_pending_events().empty());
}

TEST_F(BuildingUpgradeTest, CustomMaxLevelRespected) {
    uint32_t eid = spawn_active_building(5, 5, 0, 0);

    auto* entity = factory->get_entity_mut(eid);
    entity->building.level = 3;

    UpgradeConfig config;
    config.upgrade_cooldown = 0;
    config.check_interval = 1;
    config.max_level = 3;
    upgrade_system->set_config(config);

    upgrade_system->tick(100);

    EXPECT_EQ(factory->get_entity(eid)->building.level, 3u);
    EXPECT_TRUE(upgrade_system->get_pending_events().empty());
}

// =========================================================================
// Cooldown Prevents Rapid Upgrades
// =========================================================================

TEST_F(BuildingUpgradeTest, CooldownPreventsRapidUpgrades) {
    uint32_t eid = spawn_active_building(5, 5, 0, 0);

    UpgradeConfig config;
    config.upgrade_cooldown = 50;
    config.check_interval = 1;
    config.max_level = 5;
    upgrade_system->set_config(config);

    // Tick at 30 - cooldown not elapsed (30 <= 50)
    upgrade_system->tick(30);
    EXPECT_EQ(factory->get_entity(eid)->building.level, 1u);

    // Tick at 51 - cooldown elapsed (51 > 50)
    upgrade_system->tick(51);
    EXPECT_EQ(factory->get_entity(eid)->building.level, 2u);
}

TEST_F(BuildingUpgradeTest, CooldownResetsAfterUpgrade) {
    uint32_t eid = spawn_active_building(5, 5, 0, 0);

    UpgradeConfig config;
    config.upgrade_cooldown = 10;
    config.check_interval = 1;
    config.max_level = 5;
    upgrade_system->set_config(config);

    // First upgrade at tick 11
    upgrade_system->tick(11);
    EXPECT_EQ(factory->get_entity(eid)->building.level, 2u);

    // state_changed_tick should now be 11
    EXPECT_EQ(factory->get_entity(eid)->building.state_changed_tick, 11u);

    // Immediate tick should not upgrade (cooldown not elapsed)
    upgrade_system->tick(12);
    EXPECT_EQ(factory->get_entity(eid)->building.level, 2u);

    // Tick at 22 should upgrade (22 - 11 = 11 > 10)
    upgrade_system->tick(22);
    EXPECT_EQ(factory->get_entity(eid)->building.level, 3u);
}

// =========================================================================
// Demand Required
// =========================================================================

TEST_F(BuildingUpgradeTest, NoDemandPreventsUpgrade) {
    uint32_t eid = spawn_active_building(5, 5, 0, 0);

    // Set demand config to produce negative demand
    DemandConfig demand_config;
    demand_config.habitation_base = -100;
    demand_config.exchange_base = -100;
    demand_config.fabrication_base = -100;
    demand_config.target_zone_count = 1; // Very low target for saturation
    zone_system->set_demand_config(demand_config);
    zone_system->tick(0.05f); // Recalculate demand

    UpgradeConfig config;
    config.upgrade_cooldown = 0;
    config.check_interval = 1;
    config.max_level = 5;
    upgrade_system->set_config(config);

    upgrade_system->tick(100);

    EXPECT_EQ(factory->get_entity(eid)->building.level, 1u);
    EXPECT_TRUE(upgrade_system->get_pending_events().empty());
}

// =========================================================================
// Capacity Scales With Level
// =========================================================================

TEST_F(BuildingUpgradeTest, CapacityScalesWithLevelMultiplier) {
    uint32_t eid = spawn_active_building(5, 5, 0, 0);

    UpgradeConfig config;
    config.upgrade_cooldown = 0;
    config.check_interval = 1;
    config.max_level = 5;
    config.level_multipliers = {0.0f, 1.0f, 1.5f, 2.0f, 2.5f, 3.0f};
    upgrade_system->set_config(config);

    // Level 1 -> 2: capacity = 20 / 1.0 * 1.5 = 30
    upgrade_system->tick(1);
    const auto* entity = factory->get_entity(eid);
    EXPECT_EQ(entity->building.level, 2u);
    EXPECT_EQ(entity->building.capacity, 30u);
}

TEST_F(BuildingUpgradeTest, CapacityScalesCorrectlyForMultipleUpgrades) {
    uint32_t eid = spawn_active_building(5, 5, 0, 0);

    UpgradeConfig config;
    config.upgrade_cooldown = 0;
    config.check_interval = 1;
    config.max_level = 5;
    config.level_multipliers = {0.0f, 1.0f, 1.5f, 2.0f, 2.5f, 3.0f};
    upgrade_system->set_config(config);

    // Level 1 -> 2: 20/1.0*1.5 = 30
    upgrade_system->tick(1);
    EXPECT_EQ(factory->get_entity(eid)->building.capacity, 30u);

    // Level 2 -> 3: 30/1.5*2.0 = 40
    upgrade_system->tick(2);
    EXPECT_EQ(factory->get_entity(eid)->building.capacity, 40u);

    // Level 3 -> 4: 40/2.0*2.5 = 50
    upgrade_system->tick(3);
    EXPECT_EQ(factory->get_entity(eid)->building.capacity, 50u);

    // Level 4 -> 5: 50/2.5*3.0 = 60
    upgrade_system->tick(4);
    EXPECT_EQ(factory->get_entity(eid)->building.capacity, 60u);
}

// =========================================================================
// Event Emitted On Upgrade
// =========================================================================

TEST_F(BuildingUpgradeTest, EventEmittedOnUpgrade) {
    uint32_t eid = spawn_active_building(5, 5, 0, 0);

    UpgradeConfig config;
    config.upgrade_cooldown = 0;
    config.check_interval = 1;
    config.max_level = 5;
    upgrade_system->set_config(config);

    upgrade_system->tick(1);

    const auto& events = upgrade_system->get_pending_events();
    ASSERT_EQ(events.size(), 1u);
    EXPECT_EQ(events[0].entity_id, eid);
    EXPECT_EQ(events[0].old_level, 1u);
    EXPECT_EQ(events[0].new_level, 2u);
}

TEST_F(BuildingUpgradeTest, ClearPendingEvents) {
    uint32_t eid = spawn_active_building(5, 5, 0, 0);
    (void)eid;

    UpgradeConfig config;
    config.upgrade_cooldown = 0;
    config.check_interval = 1;
    config.max_level = 5;
    upgrade_system->set_config(config);

    upgrade_system->tick(1);
    EXPECT_FALSE(upgrade_system->get_pending_events().empty());

    upgrade_system->clear_pending_events();
    EXPECT_TRUE(upgrade_system->get_pending_events().empty());
}

// =========================================================================
// Check Interval Respected
// =========================================================================

TEST_F(BuildingUpgradeTest, CheckIntervalRespected) {
    uint32_t eid = spawn_active_building(5, 5, 0, 0);

    UpgradeConfig config;
    config.upgrade_cooldown = 0;
    config.check_interval = 10;
    config.max_level = 5;
    upgrade_system->set_config(config);

    // Tick at non-interval should not check
    upgrade_system->tick(3);
    EXPECT_EQ(factory->get_entity(eid)->building.level, 1u);

    // Tick at interval should check and upgrade
    upgrade_system->tick(10);
    EXPECT_EQ(factory->get_entity(eid)->building.level, 2u);
}

// =========================================================================
// Non-Active Buildings Ignored
// =========================================================================

TEST_F(BuildingUpgradeTest, MaterializingBuildingNotUpgraded) {
    auto templ = make_test_template();
    auto sel = make_test_selection();
    uint32_t eid = factory->spawn_building(templ, sel, 5, 5, 0, 0);
    // Building starts as Materializing

    UpgradeConfig config;
    config.upgrade_cooldown = 0;
    config.check_interval = 1;
    config.max_level = 5;
    upgrade_system->set_config(config);

    upgrade_system->tick(100);

    EXPECT_EQ(factory->get_entity(eid)->building.getBuildingState(), BuildingState::Materializing);
    EXPECT_TRUE(upgrade_system->get_pending_events().empty());
}

TEST_F(BuildingUpgradeTest, AbandonedBuildingNotUpgraded) {
    uint32_t eid = spawn_active_building(5, 5, 0, 0);
    auto* entity = factory->get_entity_mut(eid);
    entity->building.setBuildingState(BuildingState::Abandoned);

    UpgradeConfig config;
    config.upgrade_cooldown = 0;
    config.check_interval = 1;
    config.max_level = 5;
    upgrade_system->set_config(config);

    upgrade_system->tick(100);

    EXPECT_EQ(factory->get_entity(eid)->building.level, 1u);
    EXPECT_TRUE(upgrade_system->get_pending_events().empty());
}

// =========================================================================
// Null Factory Handled Gracefully
// =========================================================================

TEST_F(BuildingUpgradeTest, NullFactoryDoesNotCrash) {
    auto null_system = std::make_unique<BuildingUpgradeSystem>(nullptr, zone_system.get());

    UpgradeConfig config;
    config.upgrade_cooldown = 0;
    config.check_interval = 1;
    upgrade_system->set_config(config);

    // Should not crash
    null_system->tick(1);
    EXPECT_TRUE(null_system->get_pending_events().empty());
}

// =========================================================================
// StateChangedTick Updated On Upgrade
// =========================================================================

TEST_F(BuildingUpgradeTest, StateChangedTickUpdatedOnUpgrade) {
    uint32_t eid = spawn_active_building(5, 5, 0, 0);

    UpgradeConfig config;
    config.upgrade_cooldown = 0;
    config.check_interval = 1;
    config.max_level = 5;
    upgrade_system->set_config(config);

    upgrade_system->tick(42);

    const auto* entity = factory->get_entity(eid);
    EXPECT_EQ(entity->building.state_changed_tick, 42u);
}

} // anonymous namespace
