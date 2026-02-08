/**
 * @file test_terrain_bonus.cpp
 * @brief Unit tests for terrain efficiency bonus (Ticket 5-024)
 *
 * Tests cover:
 * - get_terrain_efficiency_bonus returns 1.0f with no terrain interface
 * - get_terrain_efficiency_bonus returns 1.0f for non-Wind types on Ridge
 * - get_terrain_efficiency_bonus returns 1.2f for Wind on Ridge terrain
 * - get_terrain_efficiency_bonus returns 1.0f for Wind on non-Ridge terrain
 * - update_all_nexus_outputs applies terrain bonus to Wind on Ridge
 * - update_all_nexus_outputs does not apply bonus to non-Wind types
 */

#include <sims3000/energy/EnergySystem.h>
#include <sims3000/energy/EnergyProducerComponent.h>
#include <sims3000/energy/EnergyEnums.h>
#include <sims3000/energy/NexusTypeConfig.h>
#include <sims3000/terrain/ITerrainQueryable.h>
#include <sims3000/terrain/TerrainTypes.h>
#include <entt/entt.hpp>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <vector>

using namespace sims3000::energy;
using namespace sims3000::terrain;

// Test result tracking
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) void test_##name()
#define RUN_TEST(name) do { \
    printf("Running %s...", #name); \
    test_##name(); \
    printf(" PASSED\n"); \
    tests_passed++; \
} while(0)

#define ASSERT(condition) do { \
    if (!(condition)) { \
        printf("\n  FAILED: %s (line %d)\n", #condition, __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

#define ASSERT_EQ(a, b) do { \
    if ((a) != (b)) { \
        printf("\n  FAILED: %s == %s (line %d)\n", #a, #b, __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

// =============================================================================
// Mock ITerrainQueryable for testing
// =============================================================================

class MockTerrain : public ITerrainQueryable {
public:
    MockTerrain() : m_width(64), m_height(64) {
        // Default all tiles to Substrate
        m_types.resize(m_width * m_height, TerrainType::Substrate);
        m_elevations.resize(m_width * m_height, 0);
    }

    void set_terrain_type(int32_t x, int32_t y, TerrainType type) {
        if (x >= 0 && x < static_cast<int32_t>(m_width) &&
            y >= 0 && y < static_cast<int32_t>(m_height)) {
            m_types[y * m_width + x] = type;
        }
    }

    void set_elevation(int32_t x, int32_t y, uint8_t elev) {
        if (x >= 0 && x < static_cast<int32_t>(m_width) &&
            y >= 0 && y < static_cast<int32_t>(m_height)) {
            m_elevations[y * m_width + x] = elev;
        }
    }

    // ITerrainQueryable interface
    TerrainType get_terrain_type(int32_t x, int32_t y) const override {
        if (x < 0 || x >= static_cast<int32_t>(m_width) ||
            y < 0 || y >= static_cast<int32_t>(m_height)) {
            return TerrainType::Substrate;
        }
        return m_types[y * m_width + x];
    }

    uint8_t get_elevation(int32_t x, int32_t y) const override {
        if (x < 0 || x >= static_cast<int32_t>(m_width) ||
            y < 0 || y >= static_cast<int32_t>(m_height)) {
            return 0;
        }
        return m_elevations[y * m_width + x];
    }

    bool is_buildable(int32_t, int32_t) const override { return true; }
    uint8_t get_slope(int32_t, int32_t, int32_t, int32_t) const override { return 0; }
    float get_average_elevation(int32_t, int32_t, uint32_t) const override { return 0.0f; }
    uint32_t get_water_distance(int32_t, int32_t) const override { return 255; }
    float get_value_bonus(int32_t, int32_t) const override { return 0.0f; }
    float get_harmony_bonus(int32_t, int32_t) const override { return 0.0f; }
    int32_t get_build_cost_modifier(int32_t, int32_t) const override { return 100; }
    uint32_t get_contamination_output(int32_t, int32_t) const override { return 0; }
    uint32_t get_map_width() const override { return m_width; }
    uint32_t get_map_height() const override { return m_height; }
    uint8_t get_sea_level() const override { return 8; }
    void get_tiles_in_rect(const GridRect&, std::vector<TerrainComponent>&) const override {}
    uint32_t get_buildable_tiles_in_rect(const GridRect&) const override { return 0; }
    uint32_t count_terrain_type_in_rect(const GridRect&, TerrainType) const override { return 0; }

private:
    uint32_t m_width;
    uint32_t m_height;
    std::vector<TerrainType> m_types;
    std::vector<uint8_t> m_elevations;
};

// =============================================================================
// get_terrain_efficiency_bonus - No terrain interface
// =============================================================================

TEST(bonus_returns_1_without_terrain) {
    EnergySystem sys(64, 64, nullptr);  // No terrain
    float bonus = sys.get_terrain_efficiency_bonus(NexusType::Wind, 10, 10);
    ASSERT(bonus == 1.0f);
}

TEST(bonus_returns_1_without_terrain_for_carbon) {
    EnergySystem sys(64, 64, nullptr);
    float bonus = sys.get_terrain_efficiency_bonus(NexusType::Carbon, 10, 10);
    ASSERT(bonus == 1.0f);
}

// =============================================================================
// get_terrain_efficiency_bonus - Non-Wind types on Ridge
// =============================================================================

TEST(bonus_returns_1_for_carbon_on_ridge) {
    MockTerrain terrain;
    terrain.set_terrain_type(10, 10, TerrainType::Ridge);
    EnergySystem sys(64, 64, &terrain);

    float bonus = sys.get_terrain_efficiency_bonus(NexusType::Carbon, 10, 10);
    ASSERT(bonus == 1.0f);
}

TEST(bonus_returns_1_for_nuclear_on_ridge) {
    MockTerrain terrain;
    terrain.set_terrain_type(10, 10, TerrainType::Ridge);
    EnergySystem sys(64, 64, &terrain);

    float bonus = sys.get_terrain_efficiency_bonus(NexusType::Nuclear, 10, 10);
    ASSERT(bonus == 1.0f);
}

TEST(bonus_returns_1_for_solar_on_ridge) {
    MockTerrain terrain;
    terrain.set_terrain_type(10, 10, TerrainType::Ridge);
    EnergySystem sys(64, 64, &terrain);

    float bonus = sys.get_terrain_efficiency_bonus(NexusType::Solar, 10, 10);
    ASSERT(bonus == 1.0f);
}

TEST(bonus_returns_1_for_petrochemical_on_ridge) {
    MockTerrain terrain;
    terrain.set_terrain_type(10, 10, TerrainType::Ridge);
    EnergySystem sys(64, 64, &terrain);

    float bonus = sys.get_terrain_efficiency_bonus(NexusType::Petrochemical, 10, 10);
    ASSERT(bonus == 1.0f);
}

TEST(bonus_returns_1_for_gaseous_on_ridge) {
    MockTerrain terrain;
    terrain.set_terrain_type(10, 10, TerrainType::Ridge);
    EnergySystem sys(64, 64, &terrain);

    float bonus = sys.get_terrain_efficiency_bonus(NexusType::Gaseous, 10, 10);
    ASSERT(bonus == 1.0f);
}

// =============================================================================
// get_terrain_efficiency_bonus - Wind on Ridge
// =============================================================================

TEST(bonus_returns_1_2_for_wind_on_ridge) {
    MockTerrain terrain;
    terrain.set_terrain_type(10, 10, TerrainType::Ridge);
    EnergySystem sys(64, 64, &terrain);

    float bonus = sys.get_terrain_efficiency_bonus(NexusType::Wind, 10, 10);
    ASSERT(bonus == 1.2f);
}

// =============================================================================
// get_terrain_efficiency_bonus - Wind on non-Ridge
// =============================================================================

TEST(bonus_returns_1_for_wind_on_substrate) {
    MockTerrain terrain;
    // Default is Substrate
    EnergySystem sys(64, 64, &terrain);

    float bonus = sys.get_terrain_efficiency_bonus(NexusType::Wind, 10, 10);
    ASSERT(bonus == 1.0f);
}

TEST(bonus_returns_1_for_wind_on_spore_flats) {
    MockTerrain terrain;
    terrain.set_terrain_type(10, 10, TerrainType::SporeFlats);
    EnergySystem sys(64, 64, &terrain);

    float bonus = sys.get_terrain_efficiency_bonus(NexusType::Wind, 10, 10);
    ASSERT(bonus == 1.0f);
}

TEST(bonus_returns_1_for_wind_on_ember_crust) {
    MockTerrain terrain;
    terrain.set_terrain_type(10, 10, TerrainType::EmberCrust);
    EnergySystem sys(64, 64, &terrain);

    float bonus = sys.get_terrain_efficiency_bonus(NexusType::Wind, 10, 10);
    ASSERT(bonus == 1.0f);
}

// =============================================================================
// update_all_nexus_outputs - terrain bonus integration
// =============================================================================

TEST(update_all_applies_terrain_bonus_wind_on_ridge) {
    MockTerrain terrain;
    terrain.set_terrain_type(5, 5, TerrainType::Ridge);

    entt::registry reg;
    EnergySystem sys(64, 64, &terrain);
    sys.set_registry(&reg);

    // Create a Wind nexus at position (5, 5) - on Ridge terrain
    auto entity = reg.create();
    auto& comp = reg.emplace<EnergyProducerComponent>(entity);
    comp.base_output = 1000;
    comp.efficiency = 1.0f;
    comp.age_factor = 1.0f;
    comp.is_online = true;
    comp.nexus_type = static_cast<uint8_t>(NexusType::Wind);

    uint32_t eid = static_cast<uint32_t>(entity);
    sys.register_nexus(eid, 0);
    sys.register_nexus_position(eid, 0, 5, 5);

    sys.update_all_nexus_outputs(0);

    // Wind base: 1000 * 1.0 * 1.0 * 0.75 (weather) = 750
    // With ridge bonus: 750 * 1.2 = 900
    ASSERT_EQ(comp.current_output, 900u);
}

TEST(update_all_no_terrain_bonus_wind_on_substrate) {
    MockTerrain terrain;
    // Position (5, 5) is default Substrate

    entt::registry reg;
    EnergySystem sys(64, 64, &terrain);
    sys.set_registry(&reg);

    auto entity = reg.create();
    auto& comp = reg.emplace<EnergyProducerComponent>(entity);
    comp.base_output = 1000;
    comp.efficiency = 1.0f;
    comp.age_factor = 1.0f;
    comp.is_online = true;
    comp.nexus_type = static_cast<uint8_t>(NexusType::Wind);

    uint32_t eid = static_cast<uint32_t>(entity);
    sys.register_nexus(eid, 0);
    sys.register_nexus_position(eid, 0, 5, 5);

    sys.update_all_nexus_outputs(0);

    // Wind base: 1000 * 1.0 * 1.0 * 0.75 (weather) = 750
    // No ridge bonus: stays 750
    ASSERT_EQ(comp.current_output, 750u);
}

TEST(update_all_no_terrain_bonus_carbon_on_ridge) {
    MockTerrain terrain;
    terrain.set_terrain_type(5, 5, TerrainType::Ridge);

    entt::registry reg;
    EnergySystem sys(64, 64, &terrain);
    sys.set_registry(&reg);

    auto entity = reg.create();
    auto& comp = reg.emplace<EnergyProducerComponent>(entity);
    comp.base_output = 1000;
    comp.efficiency = 1.0f;
    comp.age_factor = 1.0f;
    comp.is_online = true;
    comp.nexus_type = static_cast<uint8_t>(NexusType::Carbon);

    uint32_t eid = static_cast<uint32_t>(entity);
    sys.register_nexus(eid, 0);
    sys.register_nexus_position(eid, 0, 5, 5);

    sys.update_all_nexus_outputs(0);

    // Carbon: 1000 * 1.0 * 1.0 = 1000 (no weather factor, no ridge bonus)
    ASSERT_EQ(comp.current_output, 1000u);
}

TEST(update_all_no_bonus_when_offline) {
    MockTerrain terrain;
    terrain.set_terrain_type(5, 5, TerrainType::Ridge);

    entt::registry reg;
    EnergySystem sys(64, 64, &terrain);
    sys.set_registry(&reg);

    auto entity = reg.create();
    auto& comp = reg.emplace<EnergyProducerComponent>(entity);
    comp.base_output = 1000;
    comp.efficiency = 1.0f;
    comp.age_factor = 1.0f;
    comp.is_online = false;  // Offline
    comp.nexus_type = static_cast<uint8_t>(NexusType::Wind);

    uint32_t eid = static_cast<uint32_t>(entity);
    sys.register_nexus(eid, 0);
    sys.register_nexus_position(eid, 0, 5, 5);

    sys.update_all_nexus_outputs(0);

    // Offline: current_output = 0, no bonus applied
    ASSERT_EQ(comp.current_output, 0u);
}

TEST(update_all_no_bonus_without_terrain) {
    entt::registry reg;
    EnergySystem sys(64, 64, nullptr);  // No terrain
    sys.set_registry(&reg);

    auto entity = reg.create();
    auto& comp = reg.emplace<EnergyProducerComponent>(entity);
    comp.base_output = 1000;
    comp.efficiency = 1.0f;
    comp.age_factor = 1.0f;
    comp.is_online = true;
    comp.nexus_type = static_cast<uint8_t>(NexusType::Wind);

    uint32_t eid = static_cast<uint32_t>(entity);
    sys.register_nexus(eid, 0);
    sys.register_nexus_position(eid, 0, 5, 5);

    sys.update_all_nexus_outputs(0);

    // Wind: 1000 * 0.75 = 750, no terrain bonus (nullptr terrain)
    ASSERT_EQ(comp.current_output, 750u);
}

TEST(update_all_terrain_bonus_with_reduced_efficiency) {
    MockTerrain terrain;
    terrain.set_terrain_type(5, 5, TerrainType::Ridge);

    entt::registry reg;
    EnergySystem sys(64, 64, &terrain);
    sys.set_registry(&reg);

    auto entity = reg.create();
    auto& comp = reg.emplace<EnergyProducerComponent>(entity);
    comp.base_output = 1000;
    comp.efficiency = 0.8f;
    comp.age_factor = 1.0f;
    comp.is_online = true;
    comp.nexus_type = static_cast<uint8_t>(NexusType::Wind);

    uint32_t eid = static_cast<uint32_t>(entity);
    sys.register_nexus(eid, 0);
    sys.register_nexus_position(eid, 0, 5, 5);

    sys.update_all_nexus_outputs(0);

    // Wind: 1000 * 0.8 * 1.0 * 0.75 = 600
    // With ridge bonus: 600 * 1.2 = 720
    ASSERT_EQ(comp.current_output, 720u);
}

// =============================================================================
// Main Entry Point
// =============================================================================

int main() {
    printf("=== Terrain Bonus Efficiency Unit Tests (Ticket 5-024) ===\n\n");

    // No terrain interface
    RUN_TEST(bonus_returns_1_without_terrain);
    RUN_TEST(bonus_returns_1_without_terrain_for_carbon);

    // Non-Wind types on Ridge
    RUN_TEST(bonus_returns_1_for_carbon_on_ridge);
    RUN_TEST(bonus_returns_1_for_nuclear_on_ridge);
    RUN_TEST(bonus_returns_1_for_solar_on_ridge);
    RUN_TEST(bonus_returns_1_for_petrochemical_on_ridge);
    RUN_TEST(bonus_returns_1_for_gaseous_on_ridge);

    // Wind on Ridge
    RUN_TEST(bonus_returns_1_2_for_wind_on_ridge);

    // Wind on non-Ridge
    RUN_TEST(bonus_returns_1_for_wind_on_substrate);
    RUN_TEST(bonus_returns_1_for_wind_on_spore_flats);
    RUN_TEST(bonus_returns_1_for_wind_on_ember_crust);

    // Integration: update_all_nexus_outputs with terrain bonus
    RUN_TEST(update_all_applies_terrain_bonus_wind_on_ridge);
    RUN_TEST(update_all_no_terrain_bonus_wind_on_substrate);
    RUN_TEST(update_all_no_terrain_bonus_carbon_on_ridge);
    RUN_TEST(update_all_no_bonus_when_offline);
    RUN_TEST(update_all_no_bonus_without_terrain);
    RUN_TEST(update_all_terrain_bonus_with_reduced_efficiency);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
