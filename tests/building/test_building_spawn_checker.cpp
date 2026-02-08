/**
 * @file test_building_spawn_checker.cpp
 * @brief Tests for BuildingSpawnChecker (Ticket 4-024)
 */

#include <gtest/gtest.h>
#include <sims3000/building/BuildingSpawnChecker.h>
#include <sims3000/zone/ZoneSystem.h>
#include <sims3000/building/BuildingGrid.h>
#include <sims3000/terrain/ITerrainQueryable.h>
#include <sims3000/building/ForwardDependencyInterfaces.h>
#include <sims3000/building/ForwardDependencyStubs.h>

using namespace sims3000::zone;
using namespace sims3000::building;
using namespace sims3000::terrain;

// =============================================================================
// MockTerrainQueryable for BuildingSpawnChecker tests
// =============================================================================

class SpawnMockTerrain : public ITerrainQueryable {
public:
    SpawnMockTerrain()
        : m_buildable(true)
        , m_value_bonus(50.0f)
    {}

    void set_buildable(bool b) { m_buildable = b; }
    void set_value_bonus(float v) { m_value_bonus = v; }

    TerrainType get_terrain_type(std::int32_t, std::int32_t) const override {
        return TerrainType::Substrate;
    }

    std::uint8_t get_elevation(std::int32_t, std::int32_t) const override {
        return 10;
    }

    bool is_buildable(std::int32_t, std::int32_t) const override {
        return m_buildable;
    }

    std::uint8_t get_slope(std::int32_t, std::int32_t,
                            std::int32_t, std::int32_t) const override {
        return 0;
    }

    float get_average_elevation(std::int32_t, std::int32_t,
                                 std::uint32_t) const override {
        return 10.0f;
    }

    std::uint32_t get_water_distance(std::int32_t, std::int32_t) const override {
        return 255;
    }

    float get_value_bonus(std::int32_t, std::int32_t) const override {
        return m_value_bonus;
    }

    float get_harmony_bonus(std::int32_t, std::int32_t) const override {
        return 0.0f;
    }

    std::int32_t get_build_cost_modifier(std::int32_t, std::int32_t) const override {
        return 100;
    }

    std::uint32_t get_contamination_output(std::int32_t, std::int32_t) const override {
        return 0;
    }

    std::uint32_t get_map_width() const override { return 128; }
    std::uint32_t get_map_height() const override { return 128; }
    std::uint8_t get_sea_level() const override { return 8; }

    void get_tiles_in_rect(const GridRect&,
                            std::vector<TerrainComponent>& out) const override {
        out.clear();
    }

    std::uint32_t get_buildable_tiles_in_rect(const GridRect&) const override {
        return 0;
    }

    std::uint32_t count_terrain_type_in_rect(const GridRect&,
                                              TerrainType) const override {
        return 0;
    }

private:
    bool m_buildable;
    float m_value_bonus;
};

// =============================================================================
// Test Fixture
// =============================================================================

class BuildingSpawnCheckerTest : public ::testing::Test {
protected:
    void SetUp() override {
        mock_terrain = std::make_unique<SpawnMockTerrain>();
        zone_system = std::make_unique<ZoneSystem>(nullptr, nullptr, 128);
        building_grid = std::make_unique<BuildingGrid>(128, 128);
        stub_transport = std::make_unique<StubTransportProvider>();
        stub_energy = std::make_unique<StubEnergyProvider>();
        stub_fluid = std::make_unique<StubFluidProvider>();
    }

    /// Setup a zone at (x,y) with positive demand for spawn checks
    void setup_zone_with_demand(std::int32_t x, std::int32_t y,
                                 ZoneType type = ZoneType::Habitation,
                                 std::uint32_t entity_id = 1) {
        zone_system->place_zone(x, y, type, ZoneDensity::LowDensity, 0, entity_id);
        // Set demand config so demand > 0 for this zone type
        DemandConfig demand_cfg;
        demand_cfg.habitation_base = 50;
        demand_cfg.exchange_base = 50;
        demand_cfg.fabrication_base = 50;
        zone_system->set_demand_config(demand_cfg);
        // Tick once to update demand values
        zone_system->tick(0.016f);
    }

    BuildingSpawnChecker make_checker() {
        return BuildingSpawnChecker(
            zone_system.get(),
            building_grid.get(),
            mock_terrain.get(),
            stub_transport.get(),
            stub_energy.get(),
            stub_fluid.get()
        );
    }

    std::unique_ptr<SpawnMockTerrain> mock_terrain;
    std::unique_ptr<ZoneSystem> zone_system;
    std::unique_ptr<BuildingGrid> building_grid;
    std::unique_ptr<StubTransportProvider> stub_transport;
    std::unique_ptr<StubEnergyProvider> stub_energy;
    std::unique_ptr<StubFluidProvider> stub_fluid;
};

// =============================================================================
// Tests
// =============================================================================

TEST_F(BuildingSpawnCheckerTest, SuccessfulSpawnCheck) {
    setup_zone_with_demand(10, 10);
    auto checker = make_checker();

    EXPECT_TRUE(checker.can_spawn_building(10, 10, 0));
}

TEST_F(BuildingSpawnCheckerTest, ZoneMissingRejection) {
    // No zone placed at (10, 10)
    auto checker = make_checker();

    EXPECT_FALSE(checker.can_spawn_building(10, 10, 0));
}

TEST_F(BuildingSpawnCheckerTest, ZoneNotDesignatedRejection) {
    setup_zone_with_demand(10, 10);
    // Set zone to Occupied state
    zone_system->set_zone_state(10, 10, ZoneState::Occupied);

    auto checker = make_checker();
    EXPECT_FALSE(checker.can_spawn_building(10, 10, 0));
}

TEST_F(BuildingSpawnCheckerTest, DemandZeroRejection) {
    zone_system->place_zone(10, 10, ZoneType::Habitation, ZoneDensity::LowDensity, 0, 1);
    // Don't set up positive demand - demand is 0 by default after construction
    // Actually, the default DemandConfig has non-zero base values, so we need to
    // set them to zero to test zero demand
    DemandConfig demand_cfg;
    demand_cfg.habitation_base = 0;
    demand_cfg.population_hab_factor = 0;
    demand_cfg.population_exc_factor = 0;
    demand_cfg.population_fab_factor = 0;
    demand_cfg.employment_factor = 0;
    demand_cfg.utility_factor = 0;
    demand_cfg.tribute_factor = 0;
    zone_system->set_demand_config(demand_cfg);
    zone_system->tick(0.016f);

    auto checker = make_checker();
    EXPECT_FALSE(checker.can_spawn_building(10, 10, 0));
}

TEST_F(BuildingSpawnCheckerTest, DemandNegativeRejection) {
    zone_system->place_zone(10, 10, ZoneType::Habitation, ZoneDensity::LowDensity, 0, 1);
    DemandConfig demand_cfg;
    demand_cfg.habitation_base = -50;
    demand_cfg.population_hab_factor = 0;
    demand_cfg.population_exc_factor = 0;
    demand_cfg.population_fab_factor = 0;
    demand_cfg.employment_factor = 0;
    demand_cfg.utility_factor = 0;
    demand_cfg.tribute_factor = 0;
    zone_system->set_demand_config(demand_cfg);
    zone_system->tick(0.016f);

    auto checker = make_checker();
    EXPECT_FALSE(checker.can_spawn_building(10, 10, 0));
}

TEST_F(BuildingSpawnCheckerTest, BuildingGridOccupiedRejection) {
    setup_zone_with_demand(10, 10);
    building_grid->set_building_at(10, 10, 99); // Mark as occupied

    auto checker = make_checker();
    EXPECT_FALSE(checker.can_spawn_building(10, 10, 0));
}

TEST_F(BuildingSpawnCheckerTest, TerrainNotBuildableRejection) {
    setup_zone_with_demand(10, 10);
    mock_terrain->set_buildable(false);

    auto checker = make_checker();
    EXPECT_FALSE(checker.can_spawn_building(10, 10, 0));
}

TEST_F(BuildingSpawnCheckerTest, TransportNotAccessibleRejection) {
    setup_zone_with_demand(10, 10);
    stub_transport->set_debug_restrictive(true);

    auto checker = make_checker();
    EXPECT_FALSE(checker.can_spawn_building(10, 10, 0));
}

TEST_F(BuildingSpawnCheckerTest, EnergyNotPoweredRejection) {
    setup_zone_with_demand(10, 10);
    stub_energy->set_debug_restrictive(true);

    auto checker = make_checker();
    EXPECT_FALSE(checker.can_spawn_building(10, 10, 0));
}

TEST_F(BuildingSpawnCheckerTest, FluidNotAvailableRejection) {
    setup_zone_with_demand(10, 10);
    stub_fluid->set_debug_restrictive(true);

    auto checker = make_checker();
    EXPECT_FALSE(checker.can_spawn_building(10, 10, 0));
}

TEST_F(BuildingSpawnCheckerTest, MultiTileFootprintAllValid) {
    // Setup a 2x2 footprint with zones and positive demand
    std::uint32_t eid = 1;
    for (int dy = 0; dy < 2; ++dy) {
        for (int dx = 0; dx < 2; ++dx) {
            setup_zone_with_demand(10 + dx, 10 + dy, ZoneType::Habitation, eid++);
        }
    }

    auto checker = make_checker();
    EXPECT_TRUE(checker.can_spawn_footprint(10, 10, 2, 2, 0));
}

TEST_F(BuildingSpawnCheckerTest, MultiTileFootprintPartialInvalid) {
    // Setup only 3 of 4 tiles in a 2x2 footprint
    std::uint32_t eid = 1;
    setup_zone_with_demand(10, 10, ZoneType::Habitation, eid++);
    setup_zone_with_demand(11, 10, ZoneType::Habitation, eid++);
    setup_zone_with_demand(10, 11, ZoneType::Habitation, eid++);
    // (11, 11) has no zone

    auto checker = make_checker();
    EXPECT_FALSE(checker.can_spawn_footprint(10, 10, 2, 2, 0));
}

TEST_F(BuildingSpawnCheckerTest, NullTerrainSkipsBuildabilityCheck) {
    setup_zone_with_demand(10, 10);

    // Create checker with null terrain
    BuildingSpawnChecker checker(
        zone_system.get(),
        building_grid.get(),
        nullptr,  // null terrain
        stub_transport.get(),
        stub_energy.get(),
        stub_fluid.get()
    );

    EXPECT_TRUE(checker.can_spawn_building(10, 10, 0));
}

TEST_F(BuildingSpawnCheckerTest, NullStubsDefaultToPermissive) {
    setup_zone_with_demand(10, 10);

    // Create checker with null stubs
    BuildingSpawnChecker checker(
        zone_system.get(),
        building_grid.get(),
        mock_terrain.get(),
        nullptr,  // null transport
        nullptr,  // null energy
        nullptr   // null fluid
    );

    EXPECT_TRUE(checker.can_spawn_building(10, 10, 0));
}

TEST_F(BuildingSpawnCheckerTest, SingleTileFootprint) {
    setup_zone_with_demand(10, 10);

    auto checker = make_checker();
    EXPECT_TRUE(checker.can_spawn_footprint(10, 10, 1, 1, 0));
}

TEST_F(BuildingSpawnCheckerTest, FootprintWithOccupiedTile) {
    // Setup 2x1 footprint, occupy one tile
    std::uint32_t eid = 1;
    setup_zone_with_demand(10, 10, ZoneType::Habitation, eid++);
    setup_zone_with_demand(11, 10, ZoneType::Habitation, eid++);
    building_grid->set_building_at(11, 10, 99);

    auto checker = make_checker();
    EXPECT_FALSE(checker.can_spawn_footprint(10, 10, 2, 1, 0));
}
