/**
 * @file test_extractor_registration.cpp
 * @brief Unit tests for extractor/reservoir registration and output calculation
 *        (Tickets 6-014, 6-015)
 *
 * Tests cover:
 * - Register extractor, verify count increases
 * - Unregister extractor, verify count decreases
 * - Output calculation with powered extractor
 * - Output calculation with unpowered extractor (output = 0)
 * - Output at various water distances (efficiency curve)
 * - Register reservoir, verify count
 * - Reservoir totals aggregation
 *
 * Uses printf test pattern consistent with other fluid tests.
 */

#include <sims3000/fluid/FluidSystem.h>
#include <sims3000/fluid/FluidComponent.h>
#include <sims3000/fluid/FluidExtractorConfig.h>
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

#define ASSERT_NEAR(a, b, eps) do { \
    if (std::fabs(static_cast<double>(a) - static_cast<double>(b)) > static_cast<double>(eps)) { \
        printf("\n  FAILED: %s ~= %s (line %d, got %f vs %f)\n", \
               #a, #b, __LINE__, static_cast<double>(a), static_cast<double>(b)); \
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
        // Check if entity is in our powered set
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
        // Check per-tile overrides
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
// 6-014: Extractor Registration Tests
// =============================================================================

TEST(register_extractor_increases_count) {
    FluidSystem sys(128, 128);
    ASSERT_EQ(sys.get_extractor_count(0), 0u);

    sys.register_extractor(100, 0);
    ASSERT_EQ(sys.get_extractor_count(0), 1u);

    sys.register_extractor(101, 0);
    ASSERT_EQ(sys.get_extractor_count(0), 2u);

    sys.register_extractor(200, 1);
    ASSERT_EQ(sys.get_extractor_count(1), 1u);
    // Player 0 still has 2
    ASSERT_EQ(sys.get_extractor_count(0), 2u);
}

TEST(unregister_extractor_decreases_count) {
    FluidSystem sys(128, 128);
    sys.register_extractor(100, 0);
    sys.register_extractor(101, 0);
    sys.register_extractor(102, 0);
    ASSERT_EQ(sys.get_extractor_count(0), 3u);

    sys.unregister_extractor(101, 0);
    ASSERT_EQ(sys.get_extractor_count(0), 2u);

    sys.unregister_extractor(100, 0);
    ASSERT_EQ(sys.get_extractor_count(0), 1u);

    // Unregistering non-existent entity does nothing
    sys.unregister_extractor(999, 0);
    ASSERT_EQ(sys.get_extractor_count(0), 1u);
}

// =============================================================================
// 6-014: Extractor Output Calculation Tests
// =============================================================================

TEST(output_powered_extractor_at_water) {
    // Powered extractor at distance 0 from water => full output
    StubTerrainQueryable terrain;
    terrain.set_default_water_distance(0);

    FluidSystem sys(128, 128, &terrain);
    entt::registry registry;
    sys.set_registry(&registry);

    // No energy provider => assume powered
    uint32_t eid = sys.place_extractor(10, 10, 0);
    ASSERT(eid != INVALID_ENTITY_ID);

    // Tick to run update_extractor_outputs
    sys.tick(0.016f);

    // Check producer component
    auto entity = static_cast<entt::entity>(eid);
    const auto& prod = registry.get<FluidProducerComponent>(entity);

    // water_distance=0 => water_factor=1.0, powered => output = base_output * 1.0 * 1.0
    FluidExtractorConfig config = get_default_extractor_config();
    ASSERT_EQ(prod.current_output, config.base_output);
    ASSERT(prod.is_operational);

    // Pool should have generation
    const PerPlayerFluidPool& pool = sys.get_pool(0);
    ASSERT_EQ(pool.total_generated, config.base_output);
    ASSERT_EQ(pool.extractor_count, 1u);
}

TEST(output_unpowered_extractor_zero) {
    // Unpowered extractor => output = 0
    StubTerrainQueryable terrain;
    terrain.set_default_water_distance(0);

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

    // Unpowered => output = 0
    ASSERT_EQ(prod.current_output, 0u);
    ASSERT(!prod.is_operational);

    // Pool should have zero generation
    const PerPlayerFluidPool& pool = sys.get_pool(0);
    ASSERT_EQ(pool.total_generated, 0u);
    ASSERT_EQ(pool.extractor_count, 0u);
}

TEST(output_powered_extractor_becomes_unpowered) {
    // Test that a powered extractor that becomes unpowered produces 0 output
    StubTerrainQueryable terrain;
    terrain.set_default_water_distance(0);

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

    // Now unpower it
    energy.default_powered = false;
    sys.tick(0.016f);

    ASSERT_EQ(prod.current_output, 0u);
    ASSERT(!prod.is_operational);
}

TEST(output_at_water_distance_0) {
    // Distance 0 => water_factor = 1.0 => output = base_output
    StubTerrainQueryable terrain;
    terrain.set_water_distance_at(10, 10, 0);

    FluidSystem sys(128, 128, &terrain);
    entt::registry registry;
    sys.set_registry(&registry);

    uint32_t eid = sys.place_extractor(10, 10, 0);
    sys.tick(0.016f);

    auto entity = static_cast<entt::entity>(eid);
    const auto& prod = registry.get<FluidProducerComponent>(entity);

    FluidExtractorConfig config = get_default_extractor_config();
    // water_factor(0) = 1.0
    uint32_t expected = static_cast<uint32_t>(static_cast<float>(config.base_output) * 1.0f);
    ASSERT_EQ(prod.current_output, expected);
    ASSERT(prod.is_operational);
}

TEST(output_at_water_distance_1) {
    // Distance 1 => water_factor = 0.9 => output = base_output * 0.9
    StubTerrainQueryable terrain;
    terrain.set_water_distance_at(10, 10, 1);

    FluidSystem sys(128, 128, &terrain);
    entt::registry registry;
    sys.set_registry(&registry);

    uint32_t eid = sys.place_extractor(10, 10, 0);
    sys.tick(0.016f);

    auto entity = static_cast<entt::entity>(eid);
    const auto& prod = registry.get<FluidProducerComponent>(entity);

    FluidExtractorConfig config = get_default_extractor_config();
    uint32_t expected = static_cast<uint32_t>(static_cast<float>(config.base_output) * 0.9f);
    ASSERT_EQ(prod.current_output, expected);
    ASSERT(prod.is_operational);
}

TEST(output_at_water_distance_3) {
    // Distance 3 => water_factor = 0.7 => output = base_output * 0.7
    StubTerrainQueryable terrain;
    terrain.set_water_distance_at(10, 10, 3);

    FluidSystem sys(128, 128, &terrain);
    entt::registry registry;
    sys.set_registry(&registry);

    uint32_t eid = sys.place_extractor(10, 10, 0);
    sys.tick(0.016f);

    auto entity = static_cast<entt::entity>(eid);
    const auto& prod = registry.get<FluidProducerComponent>(entity);

    FluidExtractorConfig config = get_default_extractor_config();
    uint32_t expected = static_cast<uint32_t>(static_cast<float>(config.base_output) * 0.7f);
    ASSERT_EQ(prod.current_output, expected);
    ASSERT(prod.is_operational);
}

TEST(output_at_water_distance_5) {
    // Distance 5 => water_factor = 0.5 => output = base_output * 0.5
    // Distance 5 = max_operational_distance => still operational
    StubTerrainQueryable terrain;
    terrain.set_water_distance_at(10, 10, 5);

    FluidSystem sys(128, 128, &terrain);
    entt::registry registry;
    sys.set_registry(&registry);

    uint32_t eid = sys.place_extractor(10, 10, 0);
    sys.tick(0.016f);

    auto entity = static_cast<entt::entity>(eid);
    const auto& prod = registry.get<FluidProducerComponent>(entity);

    FluidExtractorConfig config = get_default_extractor_config();
    uint32_t expected = static_cast<uint32_t>(static_cast<float>(config.base_output) * 0.5f);
    ASSERT_EQ(prod.current_output, expected);
    ASSERT(prod.is_operational);
}

TEST(output_at_water_distance_7) {
    // Distance 7 => water_factor = 0.3 => output = base_output * 0.3
    // Distance 7 > max_operational_distance (5) => NOT operational
    StubTerrainQueryable terrain;
    terrain.set_water_distance_at(10, 10, 7);

    FluidSystem sys(128, 128, &terrain);
    entt::registry registry;
    sys.set_registry(&registry);

    uint32_t eid = sys.place_extractor(10, 10, 0);
    sys.tick(0.016f);

    auto entity = static_cast<entt::entity>(eid);
    const auto& prod = registry.get<FluidProducerComponent>(entity);

    FluidExtractorConfig config = get_default_extractor_config();
    uint32_t expected = static_cast<uint32_t>(static_cast<float>(config.base_output) * 0.3f);
    ASSERT_EQ(prod.current_output, expected);
    // Distance 7 > max_operational_distance (5) => not operational
    ASSERT(!prod.is_operational);
}

TEST(output_at_water_distance_9) {
    // Distance 9 => water_factor = 0.0 => output = 0
    StubTerrainQueryable terrain;
    terrain.set_water_distance_at(10, 10, 9);

    FluidSystem sys(128, 128, &terrain);
    entt::registry registry;
    sys.set_registry(&registry);

    uint32_t eid = sys.place_extractor(10, 10, 0);
    sys.tick(0.016f);

    auto entity = static_cast<entt::entity>(eid);
    const auto& prod = registry.get<FluidProducerComponent>(entity);

    ASSERT_EQ(prod.current_output, 0u);
    ASSERT(!prod.is_operational);
}

TEST(output_no_terrain_assumes_distance_zero) {
    // No terrain => water distance defaults to 0 => full output
    FluidSystem sys(128, 128, nullptr);
    entt::registry registry;
    sys.set_registry(&registry);

    uint32_t eid = sys.place_extractor(10, 10, 0);
    sys.tick(0.016f);

    auto entity = static_cast<entt::entity>(eid);
    const auto& prod = registry.get<FluidProducerComponent>(entity);

    FluidExtractorConfig config = get_default_extractor_config();
    ASSERT_EQ(prod.current_output, config.base_output);
    ASSERT(prod.is_operational);
}

TEST(output_no_energy_provider_assumes_powered) {
    // No energy provider set => assume powered
    StubTerrainQueryable terrain;
    terrain.set_default_water_distance(0);

    FluidSystem sys(128, 128, &terrain);
    entt::registry registry;
    sys.set_registry(&registry);
    // Do NOT set energy provider

    uint32_t eid = sys.place_extractor(10, 10, 0);
    sys.tick(0.016f);

    auto entity = static_cast<entt::entity>(eid);
    const auto& prod = registry.get<FluidProducerComponent>(entity);

    FluidExtractorConfig config = get_default_extractor_config();
    ASSERT_EQ(prod.current_output, config.base_output);
    ASSERT(prod.is_operational);
}

TEST(output_multiple_extractors_sum) {
    // Two extractors for same player => pool.total_generated is sum
    StubTerrainQueryable terrain;
    terrain.set_water_distance_at(10, 10, 0);  // distance 0 => factor 1.0
    terrain.set_water_distance_at(20, 20, 2);  // distance 2 => factor 0.9

    FluidSystem sys(128, 128, &terrain);
    entt::registry registry;
    sys.set_registry(&registry);

    uint32_t eid1 = sys.place_extractor(10, 10, 0);
    uint32_t eid2 = sys.place_extractor(20, 20, 0);
    ASSERT(eid1 != INVALID_ENTITY_ID);
    ASSERT(eid2 != INVALID_ENTITY_ID);

    sys.tick(0.016f);

    FluidExtractorConfig config = get_default_extractor_config();
    uint32_t expected1 = static_cast<uint32_t>(static_cast<float>(config.base_output) * 1.0f);
    uint32_t expected2 = static_cast<uint32_t>(static_cast<float>(config.base_output) * 0.9f);

    const PerPlayerFluidPool& pool = sys.get_pool(0);
    ASSERT_EQ(pool.total_generated, expected1 + expected2);
    ASSERT_EQ(pool.extractor_count, 2u);
}

// =============================================================================
// 6-015: Reservoir Registration Tests
// =============================================================================

TEST(register_reservoir_increases_count) {
    FluidSystem sys(128, 128);
    ASSERT_EQ(sys.get_reservoir_count(0), 0u);

    sys.register_reservoir(200, 0);
    ASSERT_EQ(sys.get_reservoir_count(0), 1u);

    sys.register_reservoir(201, 0);
    ASSERT_EQ(sys.get_reservoir_count(0), 2u);

    // Different player
    sys.register_reservoir(300, 1);
    ASSERT_EQ(sys.get_reservoir_count(1), 1u);
    ASSERT_EQ(sys.get_reservoir_count(0), 2u);
}

TEST(unregister_reservoir_decreases_count) {
    FluidSystem sys(128, 128);
    sys.register_reservoir(200, 0);
    sys.register_reservoir(201, 0);
    ASSERT_EQ(sys.get_reservoir_count(0), 2u);

    sys.unregister_reservoir(200, 0);
    ASSERT_EQ(sys.get_reservoir_count(0), 1u);

    // Unregister non-existent does nothing
    sys.unregister_reservoir(999, 0);
    ASSERT_EQ(sys.get_reservoir_count(0), 1u);
}

// =============================================================================
// 6-015: Reservoir Totals Aggregation Tests
// =============================================================================

TEST(reservoir_totals_aggregation) {
    FluidSystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    // Place two reservoirs
    uint32_t eid1 = sys.place_reservoir(10, 10, 0);
    uint32_t eid2 = sys.place_reservoir(20, 20, 0);
    ASSERT(eid1 != INVALID_ENTITY_ID);
    ASSERT(eid2 != INVALID_ENTITY_ID);

    // Set current levels on the reservoir components
    auto entity1 = static_cast<entt::entity>(eid1);
    auto entity2 = static_cast<entt::entity>(eid2);
    auto& res1 = registry.get<FluidReservoirComponent>(entity1);
    auto& res2 = registry.get<FluidReservoirComponent>(entity2);

    res1.current_level = 300;
    res2.current_level = 500;

    // Tick to run update_reservoir_totals
    sys.tick(0.016f);

    const PerPlayerFluidPool& pool = sys.get_pool(0);
    // After tick, reservoir buffering fills each reservoir by up to fill_rate (50)
    // since surplus > 0 (stored counts as available). 300+50=350, 500+50=550
    ASSERT_EQ(pool.total_reservoir_stored, 900u);  // (300+50) + (500+50)
    ASSERT_EQ(pool.total_reservoir_capacity, res1.capacity + res2.capacity);
    ASSERT_EQ(pool.reservoir_count, 2u);
}

TEST(reservoir_totals_empty) {
    // No reservoirs => all zeros
    FluidSystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    sys.tick(0.016f);

    const PerPlayerFluidPool& pool = sys.get_pool(0);
    ASSERT_EQ(pool.total_reservoir_stored, 0u);
    ASSERT_EQ(pool.total_reservoir_capacity, 0u);
    ASSERT_EQ(pool.reservoir_count, 0u);
}

TEST(reservoir_totals_per_player_isolation) {
    // Reservoirs for different players don't cross-contaminate
    FluidSystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    uint32_t eid_p0 = sys.place_reservoir(10, 10, 0);
    uint32_t eid_p1 = sys.place_reservoir(20, 20, 1);

    auto entity_p0 = static_cast<entt::entity>(eid_p0);
    auto entity_p1 = static_cast<entt::entity>(eid_p1);
    auto& res_p0 = registry.get<FluidReservoirComponent>(entity_p0);
    auto& res_p1 = registry.get<FluidReservoirComponent>(entity_p1);

    res_p0.current_level = 100;
    res_p1.current_level = 700;

    sys.tick(0.016f);

    const PerPlayerFluidPool& pool0 = sys.get_pool(0);
    const PerPlayerFluidPool& pool1 = sys.get_pool(1);

    // Reservoir buffering fills each by up to fill_rate (50): 100+50=150, 700+50=750
    ASSERT_EQ(pool0.total_reservoir_stored, 150u);
    ASSERT_EQ(pool0.reservoir_count, 1u);
    ASSERT_EQ(pool1.total_reservoir_stored, 750u);
    ASSERT_EQ(pool1.reservoir_count, 1u);
}

TEST(reservoir_totals_update_on_subsequent_ticks) {
    // Reservoir levels change between ticks
    FluidSystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    uint32_t eid = sys.place_reservoir(10, 10, 0);
    auto entity = static_cast<entt::entity>(eid);
    auto& res = registry.get<FluidReservoirComponent>(entity);

    res.current_level = 200;
    sys.tick(0.016f);
    // Reservoir buffering fills by up to fill_rate (50): 200+50=250
    ASSERT_EQ(sys.get_pool(0).total_reservoir_stored, 250u);

    // Change level and tick again
    res.current_level = 450;
    sys.tick(0.016f);
    // Reservoir buffering fills by up to fill_rate (50): 450+50=500
    ASSERT_EQ(sys.get_pool(0).total_reservoir_stored, 500u);
}

// =============================================================================
// Main
// =============================================================================

int main() {
    printf("=== Extractor/Reservoir Registration Tests (Tickets 6-014, 6-015) ===\n\n");

    // 6-014: Extractor registration
    RUN_TEST(register_extractor_increases_count);
    RUN_TEST(unregister_extractor_decreases_count);

    // 6-014: Extractor output calculation
    RUN_TEST(output_powered_extractor_at_water);
    RUN_TEST(output_unpowered_extractor_zero);
    RUN_TEST(output_powered_extractor_becomes_unpowered);

    // 6-014: Output at various water distances (efficiency curve)
    RUN_TEST(output_at_water_distance_0);
    RUN_TEST(output_at_water_distance_1);
    RUN_TEST(output_at_water_distance_3);
    RUN_TEST(output_at_water_distance_5);
    RUN_TEST(output_at_water_distance_7);
    RUN_TEST(output_at_water_distance_9);
    RUN_TEST(output_no_terrain_assumes_distance_zero);
    RUN_TEST(output_no_energy_provider_assumes_powered);
    RUN_TEST(output_multiple_extractors_sum);

    // 6-015: Reservoir registration
    RUN_TEST(register_reservoir_increases_count);
    RUN_TEST(unregister_reservoir_decreases_count);

    // 6-015: Reservoir totals aggregation
    RUN_TEST(reservoir_totals_aggregation);
    RUN_TEST(reservoir_totals_empty);
    RUN_TEST(reservoir_totals_per_player_isolation);
    RUN_TEST(reservoir_totals_update_on_subsequent_ticks);

    printf("\n=== Results: %d passed, %d failed ===\n",
           tests_passed, tests_failed);

    return tests_failed > 0 ? 1 : 0;
}
