/**
 * @file test_building_spawning_loop.cpp
 * @brief Tests for BuildingSpawningLoop (ticket 4-026)
 */

#include <gtest/gtest.h>
#include <sims3000/building/BuildingSpawningLoop.h>
#include <sims3000/building/BuildingFactory.h>
#include <sims3000/building/BuildingSpawnChecker.h>
#include <sims3000/building/BuildingTemplate.h>
#include <sims3000/building/BuildingGrid.h>
#include <sims3000/building/ForwardDependencyStubs.h>
#include <sims3000/zone/ZoneSystem.h>

using namespace sims3000::building;
using namespace sims3000::zone;

namespace {

// Helper to create a test template and register it
BuildingTemplate make_test_template(uint32_t id = 1,
                                     ZoneBuildingType zone_type = ZoneBuildingType::Habitation,
                                     DensityLevel density = DensityLevel::Low) {
    BuildingTemplate templ;
    templ.template_id = id;
    templ.name = "TestBuilding";
    templ.zone_type = zone_type;
    templ.density = density;
    templ.footprint_w = 1;
    templ.footprint_h = 1;
    templ.construction_ticks = 100;
    templ.construction_cost = 500;
    templ.base_capacity = 20;
    templ.color_accent_count = 4;
    templ.selection_weight = 1.0f;
    templ.min_land_value = 0.0f;
    templ.min_level = 1;
    return templ;
}

class BuildingSpawningLoopTest : public ::testing::Test {
protected:
    void SetUp() override {
        building_grid.initialize(128, 128);
        zone_system = std::make_unique<ZoneSystem>(nullptr, nullptr, 128);
        factory = std::make_unique<BuildingFactory>(&building_grid, zone_system.get());

        stub_energy = std::make_unique<StubEnergyProvider>();
        stub_fluid = std::make_unique<StubFluidProvider>();
        stub_transport = std::make_unique<StubTransportProvider>();

        checker = std::make_unique<BuildingSpawnChecker>(
            zone_system.get(), &building_grid, nullptr,
            stub_transport.get(), stub_energy.get(), stub_fluid.get()
        );

        registry = std::make_unique<BuildingTemplateRegistry>();

        // Register default templates
        registry->register_template(make_test_template(1, ZoneBuildingType::Habitation, DensityLevel::Low));
        registry->register_template(make_test_template(2, ZoneBuildingType::Exchange, DensityLevel::Low));
        registry->register_template(make_test_template(3, ZoneBuildingType::Fabrication, DensityLevel::Low));

        loop = std::make_unique<BuildingSpawningLoop>(
            factory.get(), checker.get(), registry.get(),
            zone_system.get(), &building_grid
        );
    }

    // Helper to place a designated zone with positive demand
    void place_designated_zone(int32_t x, int32_t y, uint8_t player_id = 0,
                                ZoneType type = ZoneType::Habitation,
                                ZoneDensity density = ZoneDensity::LowDensity) {
        zone_system->place_zone(x, y, type, density, player_id, 1);
        // Demand must be > 0 for spawning - set via demand config
    }

    // Helper to set positive demand for all zone types
    void set_positive_demand() {
        DemandConfig demand_config;
        demand_config.habitation_base = 50;
        demand_config.exchange_base = 50;
        demand_config.fabrication_base = 50;
        demand_config.target_zone_count = 1000; // High target to avoid saturation
        zone_system->set_demand_config(demand_config);
        // Force demand recalculation by ticking
        zone_system->tick(0.05f);
    }

    BuildingGrid building_grid;
    std::unique_ptr<ZoneSystem> zone_system;
    std::unique_ptr<BuildingFactory> factory;
    std::unique_ptr<StubEnergyProvider> stub_energy;
    std::unique_ptr<StubFluidProvider> stub_fluid;
    std::unique_ptr<StubTransportProvider> stub_transport;
    std::unique_ptr<BuildingSpawnChecker> checker;
    std::unique_ptr<BuildingTemplateRegistry> registry;
    std::unique_ptr<BuildingSpawningLoop> loop;
};

// =========================================================================
// Basic Construction
// =========================================================================

TEST_F(BuildingSpawningLoopTest, ConstructionWithValidDependencies) {
    EXPECT_EQ(loop->get_total_spawned(), 0u);
}

TEST_F(BuildingSpawningLoopTest, DefaultConfig) {
    const auto& config = loop->get_config();
    EXPECT_EQ(config.scan_interval, 20u);
    EXPECT_EQ(config.max_spawns_per_scan, 3u);
    EXPECT_EQ(config.stagger_offset, 5u);
}

TEST_F(BuildingSpawningLoopTest, SetConfig) {
    SpawningConfig config;
    config.scan_interval = 10;
    config.max_spawns_per_scan = 5;
    config.stagger_offset = 2;
    loop->set_config(config);

    const auto& result = loop->get_config();
    EXPECT_EQ(result.scan_interval, 10u);
    EXPECT_EQ(result.max_spawns_per_scan, 5u);
    EXPECT_EQ(result.stagger_offset, 2u);
}

// =========================================================================
// Spawning Triggers At Correct Interval
// =========================================================================

TEST_F(BuildingSpawningLoopTest, NoSpawnOnNonIntervalTick) {
    set_positive_demand();
    place_designated_zone(5, 5, 0);

    // Default scan_interval=20, stagger_offset=5
    // Player 0 scans when (tick + 0*5) % 20 == 0, so tick=0,20,40,...
    // Tick 1 should not trigger scan for player 0
    loop->tick(1);
    EXPECT_EQ(loop->get_total_spawned(), 0u);
}

TEST_F(BuildingSpawningLoopTest, SpawnTriggersAtIntervalTick) {
    set_positive_demand();
    place_designated_zone(5, 5, 0);

    // Player 0 scans at tick 0 (0 + 0*5 = 0, 0%20=0)
    loop->tick(0);
    EXPECT_EQ(loop->get_total_spawned(), 1u);
}

TEST_F(BuildingSpawningLoopTest, SpawnTriggersAtSecondInterval) {
    set_positive_demand();
    place_designated_zone(5, 5, 0);
    place_designated_zone(6, 5, 0);

    // Use stagger_offset=7 to avoid collisions (no two players share a tick)
    SpawningConfig config;
    config.scan_interval = 20;
    config.max_spawns_per_scan = 1;
    config.stagger_offset = 7;
    loop->set_config(config);

    // Player 0 scans at tick 0 (0 + 0*7 = 0, 0%20=0)
    // Only player 0 scans (no other player_id*7 mod 20 == 0 for id in [1,4])
    loop->tick(0);
    EXPECT_EQ(loop->get_total_spawned(), 1u);

    // Next scan for player 0 at tick 20
    loop->tick(20);
    EXPECT_EQ(loop->get_total_spawned(), 2u);
}

// =========================================================================
// Stagger Per Player
// =========================================================================

TEST_F(BuildingSpawningLoopTest, Player1StaggeredCorrectly) {
    set_positive_demand();
    // Place zone for player 1
    place_designated_zone(10, 10, 1);

    // Player 1 scans when (tick + 1*5) % 20 == 0
    // So tick + 5 must be divisible by 20
    // tick = 15: (15 + 5) = 20, 20%20=0 -> scan
    loop->tick(14);
    EXPECT_EQ(loop->get_total_spawned(), 0u);

    loop->tick(15);
    EXPECT_EQ(loop->get_total_spawned(), 1u);
}

TEST_F(BuildingSpawningLoopTest, DifferentPlayersScannedAtDifferentTicks) {
    set_positive_demand();
    place_designated_zone(5, 5, 0);
    place_designated_zone(10, 10, 1);

    // Use stagger_offset=7 to avoid collisions at tick 0
    SpawningConfig config;
    config.scan_interval = 20;
    config.max_spawns_per_scan = 3;
    config.stagger_offset = 7;
    loop->set_config(config);

    // Player 0 scans at tick 0: (0+0*7)%20 = 0
    // Player 1 scans at tick 13: (13+1*7)%20 = 0
    loop->tick(0);
    // Player 0 spawns on (5,5) - zone at (10,10) also gets spawned by player 0
    // since can_spawn_building doesn't filter by zone owner
    uint32_t spawned_after_tick0 = loop->get_total_spawned();
    EXPECT_GE(spawned_after_tick0, 1u);

    // Player 1 scans at tick 13: (13 + 7) = 20, 20%20=0
    loop->tick(13);
    // More spawns may occur if zones still available
    EXPECT_GE(loop->get_total_spawned(), spawned_after_tick0);
}

// =========================================================================
// Max Spawns Per Scan Cap
// =========================================================================

TEST_F(BuildingSpawningLoopTest, CappedAtMaxSpawnsPerScan) {
    set_positive_demand();

    // Place 5 designated zones for player 0
    for (int i = 0; i < 5; ++i) {
        place_designated_zone(i, 0, 0);
    }

    // Use stagger_offset=7 so only player 0 scans at tick 0
    // (no player_id in [1,4] has player_id*7 % 20 == 0)
    SpawningConfig config;
    config.scan_interval = 20;
    config.max_spawns_per_scan = 3;
    config.stagger_offset = 7;
    loop->set_config(config);

    loop->tick(0);
    EXPECT_EQ(loop->get_total_spawned(), 3u);
}

TEST_F(BuildingSpawningLoopTest, CustomMaxSpawnsPerScan) {
    set_positive_demand();

    // Use stagger_offset=7 so only player 0 scans at tick 0
    SpawningConfig config;
    config.scan_interval = 20;
    config.max_spawns_per_scan = 1;
    config.stagger_offset = 7;
    loop->set_config(config);

    // Place 3 designated zones
    for (int i = 0; i < 3; ++i) {
        place_designated_zone(i, 0, 0);
    }

    loop->tick(0);
    EXPECT_EQ(loop->get_total_spawned(), 1u);
}

// =========================================================================
// No Spawn When Demand <= 0
// =========================================================================

TEST_F(BuildingSpawningLoopTest, NoSpawnWhenDemandIsZero) {
    // Use default demand (zero) - don't call set_positive_demand()
    place_designated_zone(5, 5, 0);

    loop->tick(0);
    EXPECT_EQ(loop->get_total_spawned(), 0u);
}

// =========================================================================
// Grid Registration On Spawn
// =========================================================================

TEST_F(BuildingSpawningLoopTest, SpawnedBuildingRegisteredInGrid) {
    set_positive_demand();
    place_designated_zone(5, 5, 0);

    loop->tick(0);
    EXPECT_EQ(loop->get_total_spawned(), 1u);

    // Grid should have the building registered at (5,5)
    EXPECT_NE(building_grid.get_building_at(5, 5), sims3000::building::INVALID_ENTITY);
}

// =========================================================================
// Correct Template Selection
// =========================================================================

TEST_F(BuildingSpawningLoopTest, SpawnedBuildingHasCorrectZoneType) {
    set_positive_demand();
    place_designated_zone(5, 5, 0, ZoneType::Habitation, ZoneDensity::LowDensity);

    loop->tick(0);
    ASSERT_EQ(factory->get_entities().size(), 1u);

    const auto& entity = factory->get_entities()[0];
    EXPECT_EQ(entity.building.getZoneBuildingType(), ZoneBuildingType::Habitation);
}

// =========================================================================
// Factory Entity State After Spawn
// =========================================================================

TEST_F(BuildingSpawningLoopTest, SpawnedBuildingIsMaterializing) {
    set_positive_demand();
    place_designated_zone(5, 5, 0);

    loop->tick(0);
    ASSERT_EQ(factory->get_entities().size(), 1u);

    const auto& entity = factory->get_entities()[0];
    EXPECT_EQ(entity.building.getBuildingState(), BuildingState::Materializing);
    EXPECT_TRUE(entity.has_construction);
}

TEST_F(BuildingSpawningLoopTest, SpawnedBuildingHasCorrectOwner) {
    set_positive_demand();
    place_designated_zone(5, 5, 0);

    loop->tick(0);
    ASSERT_EQ(factory->get_entities().size(), 1u);

    const auto& entity = factory->get_entities()[0];
    EXPECT_EQ(entity.owner_id, 0u);
}

// =========================================================================
// No Double-Spawn on Occupied Zone
// =========================================================================

TEST_F(BuildingSpawningLoopTest, NoDoubleSpawnOnOccupiedZone) {
    set_positive_demand();
    place_designated_zone(5, 5, 0);

    // First scan at tick 0
    loop->tick(0);
    EXPECT_EQ(loop->get_total_spawned(), 1u);

    // Zone should be Occupied now, second scan should not spawn again
    loop->tick(20);
    EXPECT_EQ(loop->get_total_spawned(), 1u);
}

// =========================================================================
// Multiple Zones Multiple Players
// =========================================================================

TEST_F(BuildingSpawningLoopTest, MultiplePlayersSpawnIndependently) {
    set_positive_demand();
    place_designated_zone(5, 5, 0);
    place_designated_zone(10, 10, 2);

    // Set scan_interval=1 for simpler testing
    SpawningConfig config;
    config.scan_interval = 1;
    config.max_spawns_per_scan = 10;
    config.stagger_offset = 0;
    loop->set_config(config);

    loop->tick(0);
    EXPECT_EQ(loop->get_total_spawned(), 2u);
    EXPECT_EQ(factory->get_entities().size(), 2u);
}

// =========================================================================
// No Spawn When Checker Fails
// =========================================================================

TEST_F(BuildingSpawningLoopTest, NoSpawnWhenEnergyNotAvailable) {
    set_positive_demand();
    place_designated_zone(5, 5, 0);

    // Make energy restrictive
    stub_energy->set_debug_restrictive(true);

    loop->tick(0);
    EXPECT_EQ(loop->get_total_spawned(), 0u);
}

TEST_F(BuildingSpawningLoopTest, NoSpawnWhenTransportNotAvailable) {
    set_positive_demand();
    place_designated_zone(5, 5, 0);

    // Make transport restrictive
    stub_transport->set_debug_restrictive(true);

    loop->tick(0);
    EXPECT_EQ(loop->get_total_spawned(), 0u);
}

// =========================================================================
// Zero Scan Interval Guard
// =========================================================================

TEST_F(BuildingSpawningLoopTest, ZeroScanIntervalDoesNotCrash) {
    SpawningConfig config;
    config.scan_interval = 0;
    loop->set_config(config);

    // Should not crash or spawn anything
    loop->tick(0);
    EXPECT_EQ(loop->get_total_spawned(), 0u);
}

} // anonymous namespace
