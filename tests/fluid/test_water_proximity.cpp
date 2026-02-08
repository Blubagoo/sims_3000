/**
 * @file test_water_proximity.cpp
 * @brief Unit tests for water proximity extraction efficiency and power dependency
 *        (Tickets 6-025, 6-026)
 *
 * Tests cover:
 * - Extractor at distance 0: full output
 * - Extractor at distance 3: 70% output
 * - Extractor at distance 8: 30% output
 * - Extractor at distance 9: 0 output (non-operational)
 * - Unpowered extractor: 0 output regardless of distance
 *
 * Uses printf test pattern consistent with other fluid tests.
 */

#include <sims3000/fluid/FluidSystem.h>
#include <sims3000/fluid/FluidComponent.h>
#include <sims3000/fluid/FluidExtractorConfig.h>
#include <sims3000/fluid/FluidProducerComponent.h>
#include <sims3000/energy/EnergyComponent.h>
#include <sims3000/building/ForwardDependencyInterfaces.h>
#include <sims3000/terrain/ITerrainQueryable.h>
#include <sims3000/terrain/TerrainTypes.h>
#include <entt/entt.hpp>
#include <cstdio>
#include <cstdlib>
#include <cmath>

using namespace sims3000::fluid;
using namespace sims3000::building;

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
// Stub EnergyProvider for testing power state
// =============================================================================

class StubEnergyProvider : public IEnergyProvider {
public:
    bool is_powered(uint32_t entity_id) const override {
        for (uint32_t i = 0; i < powered_count; ++i) {
            if (powered_entities[i] == entity_id) {
                return true;
            }
        }
        return default_powered;
    }

    bool is_powered_at(uint32_t /*x*/, uint32_t /*y*/, uint32_t /*player_id*/) const override {
        return default_powered;
    }

    void set_powered(uint32_t entity_id) {
        if (powered_count < 64) {
            powered_entities[powered_count++] = entity_id;
        }
    }

    bool default_powered = true;
    uint32_t powered_entities[64] = {};
    uint32_t powered_count = 0;
};

// =============================================================================
// Stub TerrainQueryable for testing water distance
// =============================================================================

class StubTerrainQueryable : public sims3000::terrain::ITerrainQueryable {
public:
    StubTerrainQueryable() : m_default_water_distance(0) {}

    sims3000::terrain::TerrainType get_terrain_type(
        int32_t /*x*/, int32_t /*y*/) const override {
        return sims3000::terrain::TerrainType::Substrate;
    }

    uint8_t get_elevation(int32_t /*x*/, int32_t /*y*/) const override {
        return 10;
    }

    bool is_buildable(int32_t /*x*/, int32_t /*y*/) const override {
        return true;
    }

    uint8_t get_slope(int32_t /*x1*/, int32_t /*y1*/,
                      int32_t /*x2*/, int32_t /*y2*/) const override {
        return 0;
    }

    float get_average_elevation(int32_t /*x*/, int32_t /*y*/,
                                uint32_t /*radius*/) const override {
        return 10.0f;
    }

    uint32_t get_water_distance(int32_t x, int32_t y) const override {
        for (uint32_t i = 0; i < override_count; ++i) {
            if (override_x[i] == x && override_y[i] == y) {
                return override_dist[i];
            }
        }
        return m_default_water_distance;
    }

    float get_value_bonus(int32_t /*x*/, int32_t /*y*/) const override {
        return 0.0f;
    }

    float get_harmony_bonus(int32_t /*x*/, int32_t /*y*/) const override {
        return 0.0f;
    }

    int32_t get_build_cost_modifier(int32_t /*x*/, int32_t /*y*/) const override {
        return 100;
    }

    uint32_t get_contamination_output(int32_t /*x*/, int32_t /*y*/) const override {
        return 0;
    }

    uint32_t get_map_width() const override { return 128; }
    uint32_t get_map_height() const override { return 128; }
    uint8_t get_sea_level() const override { return 8; }

    void get_tiles_in_rect(const sims3000::terrain::GridRect& /*rect*/,
                           std::vector<sims3000::terrain::TerrainComponent>& out_tiles) const override {
        out_tiles.clear();
    }

    uint32_t get_buildable_tiles_in_rect(const sims3000::terrain::GridRect& /*rect*/) const override {
        return 0;
    }

    uint32_t count_terrain_type_in_rect(const sims3000::terrain::GridRect& /*rect*/,
                                        sims3000::terrain::TerrainType /*type*/) const override {
        return 0;
    }

    // Configuration
    void set_default_water_distance(uint32_t dist) {
        m_default_water_distance = dist;
    }

    void set_water_distance_at(int32_t x, int32_t y, uint32_t dist) {
        if (override_count < 64) {
            override_x[override_count] = x;
            override_y[override_count] = y;
            override_dist[override_count] = dist;
            ++override_count;
        }
    }

private:
    uint32_t m_default_water_distance;
    int32_t override_x[64] = {};
    int32_t override_y[64] = {};
    uint32_t override_dist[64] = {};
    uint32_t override_count = 0;
};

// =============================================================================
// 6-025: Water Proximity Extraction Efficiency Tests
// =============================================================================

TEST(extractor_distance_0_full_output) {
    // Distance 0 => water_factor = 1.0 => output = base_output * 1.0
    StubTerrainQueryable terrain;
    terrain.set_water_distance_at(10, 10, 0);

    FluidSystem sys(128, 128, &terrain);
    entt::registry registry;
    sys.set_registry(&registry);

    uint32_t eid = sys.place_extractor(10, 10, 0);
    ASSERT(eid != INVALID_ENTITY_ID);

    sys.tick(0.016f);

    auto entity = static_cast<entt::entity>(eid);
    const auto& prod = registry.get<FluidProducerComponent>(entity);

    FluidExtractorConfig config = get_default_extractor_config();
    uint32_t expected = static_cast<uint32_t>(static_cast<float>(config.base_output) * 1.0f);
    ASSERT_EQ(prod.current_output, expected);
    ASSERT(prod.is_operational);
    ASSERT_EQ(prod.current_water_distance, static_cast<uint8_t>(0));
}

TEST(extractor_distance_3_seventy_percent) {
    // Distance 3 => water_factor = 0.7 => output = base_output * 0.7
    StubTerrainQueryable terrain;
    terrain.set_water_distance_at(10, 10, 3);

    FluidSystem sys(128, 128, &terrain);
    entt::registry registry;
    sys.set_registry(&registry);

    uint32_t eid = sys.place_extractor(10, 10, 0);
    ASSERT(eid != INVALID_ENTITY_ID);

    sys.tick(0.016f);

    auto entity = static_cast<entt::entity>(eid);
    const auto& prod = registry.get<FluidProducerComponent>(entity);

    FluidExtractorConfig config = get_default_extractor_config();
    uint32_t expected = static_cast<uint32_t>(static_cast<float>(config.base_output) * 0.7f);
    ASSERT_EQ(prod.current_output, expected);
    ASSERT(prod.is_operational);
    ASSERT_EQ(prod.current_water_distance, static_cast<uint8_t>(3));
}

TEST(extractor_distance_8_thirty_percent) {
    // Distance 8 => water_factor = 0.3 => output = base_output * 0.3
    StubTerrainQueryable terrain;
    terrain.set_water_distance_at(10, 10, 8);

    FluidSystem sys(128, 128, &terrain);
    entt::registry registry;
    sys.set_registry(&registry);

    uint32_t eid = sys.place_extractor(10, 10, 0);
    ASSERT(eid != INVALID_ENTITY_ID);

    sys.tick(0.016f);

    auto entity = static_cast<entt::entity>(eid);
    const auto& prod = registry.get<FluidProducerComponent>(entity);

    FluidExtractorConfig config = get_default_extractor_config();
    uint32_t expected = static_cast<uint32_t>(static_cast<float>(config.base_output) * 0.3f);
    ASSERT_EQ(prod.current_output, expected);
    // Distance 8 > max_operational_distance (5) => not operational
    ASSERT(!prod.is_operational);
    ASSERT_EQ(prod.current_water_distance, static_cast<uint8_t>(8));
}

TEST(extractor_distance_9_zero_output) {
    // Distance 9 => water_factor = 0.0 => output = 0, non-operational
    StubTerrainQueryable terrain;
    terrain.set_water_distance_at(10, 10, 9);

    FluidSystem sys(128, 128, &terrain);
    entt::registry registry;
    sys.set_registry(&registry);

    uint32_t eid = sys.place_extractor(10, 10, 0);
    ASSERT(eid != INVALID_ENTITY_ID);

    sys.tick(0.016f);

    auto entity = static_cast<entt::entity>(eid);
    const auto& prod = registry.get<FluidProducerComponent>(entity);

    ASSERT_EQ(prod.current_output, 0u);
    ASSERT(!prod.is_operational);
    ASSERT_EQ(prod.current_water_distance, static_cast<uint8_t>(9));

    // Pool should have zero generation
    const PerPlayerFluidPool& pool = sys.get_pool(0);
    ASSERT_EQ(pool.total_generated, 0u);
    ASSERT_EQ(pool.extractor_count, 0u);
}

// =============================================================================
// 6-026: Extractor Power Dependency Tests
// =============================================================================

TEST(unpowered_extractor_zero_output_regardless_of_distance) {
    // Even at distance 0 (best water proximity), an unpowered extractor
    // produces zero output.
    StubTerrainQueryable terrain;
    terrain.set_water_distance_at(10, 10, 0);

    StubEnergyProvider energy;
    energy.default_powered = false;

    FluidSystem sys(128, 128, &terrain);
    entt::registry registry;
    sys.set_registry(&registry);
    sys.set_energy_provider(&energy);

    uint32_t eid = sys.place_extractor(10, 10, 0);
    ASSERT(eid != INVALID_ENTITY_ID);

    sys.tick(0.016f);

    auto entity = static_cast<entt::entity>(eid);
    const auto& prod = registry.get<FluidProducerComponent>(entity);

    ASSERT_EQ(prod.current_output, 0u);
    ASSERT(!prod.is_operational);

    // Pool should reflect zero generation
    const PerPlayerFluidPool& pool = sys.get_pool(0);
    ASSERT_EQ(pool.total_generated, 0u);
    ASSERT_EQ(pool.extractor_count, 0u);
}

TEST(extractor_has_energy_component) {
    // Verify that place_extractor creates entity with EnergyComponent
    // having energy_required = 20 and priority = 2
    FluidSystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    uint32_t eid = sys.place_extractor(10, 10, 0);
    ASSERT(eid != INVALID_ENTITY_ID);

    auto entity = static_cast<entt::entity>(eid);
    ASSERT(registry.all_of<sims3000::energy::EnergyComponent>(entity));

    const auto& ec = registry.get<sims3000::energy::EnergyComponent>(entity);
    ASSERT_EQ(ec.energy_required, EXTRACTOR_DEFAULT_ENERGY_REQUIRED);
    ASSERT_EQ(ec.priority, EXTRACTOR_DEFAULT_ENERGY_PRIORITY);
}

TEST(no_energy_provider_assumes_powered) {
    // When no energy provider is set, extractors assume they are powered
    StubTerrainQueryable terrain;
    terrain.set_water_distance_at(10, 10, 0);

    FluidSystem sys(128, 128, &terrain);
    entt::registry registry;
    sys.set_registry(&registry);
    // Deliberately do NOT set energy provider

    uint32_t eid = sys.place_extractor(10, 10, 0);
    sys.tick(0.016f);

    auto entity = static_cast<entt::entity>(eid);
    const auto& prod = registry.get<FluidProducerComponent>(entity);

    FluidExtractorConfig config = get_default_extractor_config();
    ASSERT_EQ(prod.current_output, config.base_output);
    ASSERT(prod.is_operational);
}

TEST(power_loss_stops_generation) {
    // Extractor that was powered becomes unpowered => generation drops to 0
    StubTerrainQueryable terrain;
    terrain.set_water_distance_at(10, 10, 0);

    StubEnergyProvider energy;
    energy.default_powered = true;

    FluidSystem sys(128, 128, &terrain);
    entt::registry registry;
    sys.set_registry(&registry);
    sys.set_energy_provider(&energy);

    uint32_t eid = sys.place_extractor(10, 10, 0);
    sys.tick(0.016f);

    auto entity = static_cast<entt::entity>(eid);
    const auto& prod = registry.get<FluidProducerComponent>(entity);
    FluidExtractorConfig config = get_default_extractor_config();
    ASSERT_EQ(prod.current_output, config.base_output);
    ASSERT(prod.is_operational);

    // Cut power
    energy.default_powered = false;
    sys.tick(0.016f);

    ASSERT_EQ(prod.current_output, 0u);
    ASSERT(!prod.is_operational);

    const PerPlayerFluidPool& pool = sys.get_pool(0);
    ASSERT_EQ(pool.total_generated, 0u);
    ASSERT_EQ(pool.extractor_count, 0u);
}

TEST(power_restored_restores_generation) {
    // Extractor loses power then regains it => generation recovers
    StubTerrainQueryable terrain;
    terrain.set_water_distance_at(10, 10, 0);

    StubEnergyProvider energy;
    energy.default_powered = false;

    FluidSystem sys(128, 128, &terrain);
    entt::registry registry;
    sys.set_registry(&registry);
    sys.set_energy_provider(&energy);

    uint32_t eid = sys.place_extractor(10, 10, 0);
    sys.tick(0.016f);

    auto entity = static_cast<entt::entity>(eid);
    const auto& prod = registry.get<FluidProducerComponent>(entity);
    ASSERT_EQ(prod.current_output, 0u);

    // Restore power
    energy.default_powered = true;
    sys.tick(0.016f);

    FluidExtractorConfig config = get_default_extractor_config();
    ASSERT_EQ(prod.current_output, config.base_output);
    ASSERT(prod.is_operational);
}

// =============================================================================
// Main
// =============================================================================

int main() {
    printf("=== Water Proximity & Power Dependency Tests (Tickets 6-025, 6-026) ===\n\n");

    // 6-025: Water proximity extraction efficiency
    RUN_TEST(extractor_distance_0_full_output);
    RUN_TEST(extractor_distance_3_seventy_percent);
    RUN_TEST(extractor_distance_8_thirty_percent);
    RUN_TEST(extractor_distance_9_zero_output);

    // 6-026: Extractor power dependency
    RUN_TEST(unpowered_extractor_zero_output_regardless_of_distance);
    RUN_TEST(extractor_has_energy_component);
    RUN_TEST(no_energy_provider_assumes_powered);
    RUN_TEST(power_loss_stops_generation);
    RUN_TEST(power_restored_restores_generation);

    printf("\n=== Results: %d passed, %d failed ===\n",
           tests_passed, tests_failed);

    return tests_failed > 0 ? 1 : 0;
}
