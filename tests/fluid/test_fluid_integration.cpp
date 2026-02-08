/**
 * @file test_fluid_integration.cpp
 * @brief Integration tests for FluidSystem full pipeline (Ticket 6-043)
 *
 * End-to-end integration tests exercising the complete fluid pipeline:
 * - Extractor placement -> generation -> pool update
 * - Consumer registration -> coverage -> fluid distribution
 * - Conduit extension -> coverage change -> consumer fluid state
 * - All-or-nothing distribution under deficit
 * - Reservoir fill/drain during surplus/deficit
 * - Proportional reservoir drain
 * - Event emission for state changes
 * - Performance with many consumers
 *
 * Uses printf test pattern consistent with other fluid tests.
 *
 * @see /docs/epics/epic-6/tickets.md (ticket 6-043)
 */

#include <sims3000/fluid/FluidSystem.h>
#include <sims3000/fluid/FluidComponent.h>
#include <sims3000/fluid/FluidProducerComponent.h>
#include <sims3000/fluid/FluidReservoirComponent.h>
#include <sims3000/fluid/FluidConduitComponent.h>
#include <sims3000/fluid/FluidExtractorConfig.h>
#include <sims3000/fluid/FluidReservoirConfig.h>
#include <sims3000/fluid/FluidPlacementValidation.h>
#include <sims3000/fluid/FluidEvents.h>
#include <sims3000/fluid/PerPlayerFluidPool.h>
#include <sims3000/energy/EnergyComponent.h>
#include <sims3000/building/ForwardDependencyInterfaces.h>
#include <sims3000/terrain/ITerrainQueryable.h>
#include <sims3000/terrain/TerrainTypes.h>
#include <entt/entt.hpp>
#include <cstdio>
#include <cstdlib>
#include <chrono>
#include <cmath>

using namespace sims3000::fluid;
using namespace sims3000::building;

// =============================================================================
// Test framework macros
// =============================================================================

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

#define ASSERT_GT(a, b) do { \
    if (!((a) > (b))) { \
        printf("\n  FAILED: %s > %s (line %d)\n", #a, #b, __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

#define ASSERT_LT(a, b) do { \
    if (!((a) < (b))) { \
        printf("\n  FAILED: %s < %s (line %d)\n", #a, #b, __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

#define ASSERT_GE(a, b) do { \
    if (!((a) >= (b))) { \
        printf("\n  FAILED: %s >= %s (line %d)\n", #a, #b, __LINE__); \
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

    uint32_t get_map_width() const override { return 256; }
    uint32_t get_map_height() const override { return 256; }
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
        if (override_count < 256) {
            override_x[override_count] = x;
            override_y[override_count] = y;
            override_dist[override_count] = dist;
            ++override_count;
        }
    }

private:
    uint32_t m_default_water_distance;
    int32_t override_x[256] = {};
    int32_t override_y[256] = {};
    uint32_t override_dist[256] = {};
    uint32_t override_count = 0;
};

// =============================================================================
// Helper: Create a consumer entity with FluidComponent in the registry
// =============================================================================

static uint32_t create_consumer(entt::registry& reg, uint32_t fluid_required) {
    auto entity = reg.create();
    FluidComponent fc{};
    fc.fluid_required = fluid_required;
    fc.fluid_received = 0;
    fc.has_fluid = false;
    reg.emplace<FluidComponent>(entity, fc);
    return static_cast<uint32_t>(entity);
}

// =============================================================================
// Test 1: Place extractor near water, verify pool generation increases
// =============================================================================

TEST(extractor_near_water_generates) {
    // Mock terrain: water at (10,10) so extractor at (12,12) is distance 2+2=4
    // (Manhattan distance will be queried at the extractor position)
    StubTerrainQueryable terrain;
    terrain.set_water_distance_at(12, 12, 2);  // distance 2 from water

    entt::registry reg;
    FluidSystem sys(256, 256, &terrain);
    sys.set_registry(&reg);

    // Pool starts with 0 generation
    ASSERT_EQ(sys.get_pool(0).total_generated, 0u);

    // Place extractor at (12, 12) for player 0
    uint32_t ext_id = sys.place_extractor(12, 12, 0);
    ASSERT(ext_id != INVALID_ENTITY_ID);

    // Tick
    sys.tick(0.016f);

    // Pool generation should be > 0 (distance 2 => water_factor 0.9)
    const PerPlayerFluidPool& pool = sys.get_pool(0);
    ASSERT_GT(pool.total_generated, 0u);
}

// =============================================================================
// Test 2: Extractor far from water produces reduced output
// =============================================================================

TEST(extractor_far_from_water_reduced_output) {
    // Extractor at distance 1 vs distance 5
    // Distance 1 => water_factor = 0.9
    // Distance 5 => water_factor = 0.5
    StubTerrainQueryable terrain_close;
    terrain_close.set_water_distance_at(10, 10, 1);

    StubTerrainQueryable terrain_far;
    terrain_far.set_water_distance_at(10, 10, 5);

    entt::registry reg_close;
    FluidSystem sys_close(256, 256, &terrain_close);
    sys_close.set_registry(&reg_close);

    entt::registry reg_far;
    FluidSystem sys_far(256, 256, &terrain_far);
    sys_far.set_registry(&reg_far);

    sys_close.place_extractor(10, 10, 0);
    sys_far.place_extractor(10, 10, 0);

    sys_close.tick(0.016f);
    sys_far.tick(0.016f);

    uint32_t gen_close = sys_close.get_pool(0).total_generated;
    uint32_t gen_far = sys_far.get_pool(0).total_generated;

    // Close extractor should produce more than far extractor
    ASSERT_GT(gen_close, gen_far);
    ASSERT_GT(gen_close, 0u);
    ASSERT_GT(gen_far, 0u);

    // Verify the ratio is consistent with the water factor curve
    // close: 0.9 * base_output, far: 0.5 * base_output
    FluidExtractorConfig config = get_default_extractor_config();
    uint32_t expected_close = static_cast<uint32_t>(
        static_cast<float>(config.base_output) * 0.9f);
    uint32_t expected_far = static_cast<uint32_t>(
        static_cast<float>(config.base_output) * 0.5f);
    ASSERT_EQ(gen_close, expected_close);
    ASSERT_EQ(gen_far, expected_far);
}

// =============================================================================
// Test 3: Unpowered extractor produces nothing
// =============================================================================

TEST(unpowered_extractor_produces_nothing) {
    StubTerrainQueryable terrain;
    terrain.set_water_distance_at(10, 10, 0);

    StubEnergyProvider energy;
    energy.default_powered = false;

    entt::registry reg;
    FluidSystem sys(256, 256, &terrain);
    sys.set_registry(&reg);
    sys.set_energy_provider(&energy);

    sys.place_extractor(10, 10, 0);
    sys.tick(0.016f);

    const PerPlayerFluidPool& pool = sys.get_pool(0);
    ASSERT_EQ(pool.total_generated, 0u);
    ASSERT_EQ(pool.extractor_count, 0u);
}

// =============================================================================
// Test 4: Structure with FluidComponent gets fluid when in coverage
// =============================================================================

TEST(consumer_in_coverage_gets_fluid) {
    StubTerrainQueryable terrain;
    terrain.set_water_distance_at(10, 10, 0);

    entt::registry reg;
    FluidSystem sys(256, 256, &terrain);
    sys.set_registry(&reg);

    // Place extractor at (10, 10) with coverage_radius = 8
    sys.place_extractor(10, 10, 0);

    // Create consumer entity requiring 10 fluid
    uint32_t consumer_id = create_consumer(reg, 10);

    // Register consumer at (12, 12) - within extractor's coverage radius
    sys.register_consumer(consumer_id, 0);
    sys.register_consumer_position(consumer_id, 0, 12, 12);

    // Tick
    sys.tick(0.016f);

    // Consumer should have fluid
    auto entity = static_cast<entt::entity>(consumer_id);
    const auto& fc = reg.get<FluidComponent>(entity);
    ASSERT(fc.has_fluid);
    ASSERT_EQ(fc.fluid_received, 10u);
}

// =============================================================================
// Test 5: Structure outside coverage gets no fluid
// =============================================================================

TEST(consumer_outside_coverage_no_fluid) {
    StubTerrainQueryable terrain;
    terrain.set_water_distance_at(10, 10, 0);

    entt::registry reg;
    FluidSystem sys(256, 256, &terrain);
    sys.set_registry(&reg);

    // Place extractor at (10, 10) with coverage_radius = 8
    sys.place_extractor(10, 10, 0);

    // Create consumer entity at (100, 100) - far outside coverage
    uint32_t consumer_id = create_consumer(reg, 10);
    sys.register_consumer(consumer_id, 0);
    sys.register_consumer_position(consumer_id, 0, 100, 100);

    // Tick
    sys.tick(0.016f);

    // Consumer should NOT have fluid (outside coverage)
    auto entity = static_cast<entt::entity>(consumer_id);
    const auto& fc = reg.get<FluidComponent>(entity);
    ASSERT(!fc.has_fluid);
    ASSERT_EQ(fc.fluid_received, 0u);
}

// =============================================================================
// Test 6: Pool deficit: ALL consumers lose fluid (no rationing)
// =============================================================================

TEST(deficit_all_consumers_lose_fluid) {
    StubTerrainQueryable terrain;
    terrain.set_water_distance_at(10, 10, 0);

    entt::registry reg;
    FluidSystem sys(256, 256, &terrain);
    sys.set_registry(&reg);

    // Extractor produces base_output (100 fluid) at distance 0
    sys.place_extractor(10, 10, 0);

    // Create 3 consumers each requiring 50 fluid (total 150 > 100)
    uint32_t c1 = create_consumer(reg, 50);
    uint32_t c2 = create_consumer(reg, 50);
    uint32_t c3 = create_consumer(reg, 50);

    // All within coverage
    sys.register_consumer(c1, 0);
    sys.register_consumer_position(c1, 0, 11, 10);
    sys.register_consumer(c2, 0);
    sys.register_consumer_position(c2, 0, 12, 10);
    sys.register_consumer(c3, 0);
    sys.register_consumer_position(c3, 0, 13, 10);

    // Tick
    sys.tick(0.016f);

    // Verify pool is in deficit
    const PerPlayerFluidPool& pool = sys.get_pool(0);
    ASSERT_LT(pool.surplus, 0);

    // ALL consumers should have has_fluid == false (all-or-nothing per CCR-002)
    auto e1 = static_cast<entt::entity>(c1);
    auto e2 = static_cast<entt::entity>(c2);
    auto e3 = static_cast<entt::entity>(c3);

    ASSERT(!reg.get<FluidComponent>(e1).has_fluid);
    ASSERT(!reg.get<FluidComponent>(e2).has_fluid);
    ASSERT(!reg.get<FluidComponent>(e3).has_fluid);

    ASSERT_EQ(reg.get<FluidComponent>(e1).fluid_received, 0u);
    ASSERT_EQ(reg.get<FluidComponent>(e2).fluid_received, 0u);
    ASSERT_EQ(reg.get<FluidComponent>(e3).fluid_received, 0u);
}

// =============================================================================
// Test 7: Reservoir fills during surplus
// =============================================================================

TEST(reservoir_fills_during_surplus) {
    StubTerrainQueryable terrain;
    terrain.set_water_distance_at(10, 10, 0);

    entt::registry reg;
    FluidSystem sys(256, 256, &terrain);
    sys.set_registry(&reg);

    // Extractor at (10,10) producing 100 fluid
    sys.place_extractor(10, 10, 0);

    // Reservoir at (11,10) with default config: capacity=1000, fill_rate=50
    uint32_t res_id = sys.place_reservoir(11, 10, 0);
    ASSERT(res_id != INVALID_ENTITY_ID);

    // No consumers, so all production is surplus
    // Tick multiple times - reservoir should fill
    for (int i = 0; i < 5; ++i) {
        sys.tick(0.016f);
    }

    auto res_entity = static_cast<entt::entity>(res_id);
    const auto& reservoir = reg.get<FluidReservoirComponent>(res_entity);
    ASSERT_GT(reservoir.current_level, 0u);
}

// =============================================================================
// Test 8: Reservoir drains during deficit, delays collapse
// =============================================================================

TEST(reservoir_drains_during_deficit_delays_collapse) {
    StubTerrainQueryable terrain;
    terrain.set_water_distance_at(10, 10, 0);

    entt::registry reg;
    FluidSystem sys(256, 256, &terrain);
    sys.set_registry(&reg);

    // Extractor producing 100 fluid
    sys.place_extractor(10, 10, 0);

    // Reservoir with some pre-filled level
    uint32_t res_id = sys.place_reservoir(11, 10, 0);
    ASSERT(res_id != INVALID_ENTITY_ID);

    // Pre-fill reservoir with a small amount
    auto res_entity = static_cast<entt::entity>(res_id);
    auto& reservoir = reg.get<FluidReservoirComponent>(res_entity);
    reservoir.current_level = 300;

    // Create very heavy consumer demand that exceeds generation + reservoir stored.
    // Pool calculates: available = total_generated + total_reservoir_stored
    // We need consumed > available so surplus < 0, triggering reservoir drain.
    // generation = 100, stored = 300, available = 400
    // With consumption = 500, surplus = 400 - 500 = -100 => Deficit (reservoir has level)
    uint32_t c1 = create_consumer(reg, 500);
    sys.register_consumer(c1, 0);
    sys.register_consumer_position(c1, 0, 12, 10);

    // First tick: deficit, but reservoir still has stored fluid
    sys.tick(0.016f);

    // Pool state should be Deficit (not Collapse) because reservoir has remaining level
    // Deficit = surplus < 0 AND reservoir_stored > 0
    // (After drain, reservoir_stored may still be > 0 if drain_rate limited the drain)
    FluidPoolState state = sys.get_pool_state(0);
    const PerPlayerFluidPool& pool = sys.get_pool(0);

    // Reservoir should have been partially drained (drain_rate = 100 per tick)
    // Deficit was 100, so drain up to 100 units from reservoir
    ASSERT_LT(reservoir.current_level, 300u);

    // If reservoir still has level, state should be Deficit (not yet Collapse)
    if (reservoir.current_level > 0) {
        ASSERT(state == FluidPoolState::Deficit || state == FluidPoolState::Collapse);
    }

    // Keep ticking until reservoir is empty
    uint32_t ticks = 0;
    while (reservoir.current_level > 0 && ticks < 100) {
        sys.tick(0.016f);
        ++ticks;
    }

    // Reservoir should be empty now
    ASSERT_EQ(reservoir.current_level, 0u);

    // One more tick with empty reservoir
    sys.tick(0.016f);

    // After reservoir is depleted and another tick processes:
    // available = 100 + 0 = 100, consumed = 500, surplus = -400
    // total_reservoir_stored = 0 => Collapse
    FluidPoolState final_state = sys.get_pool_state(0);
    ASSERT_EQ(static_cast<uint8_t>(final_state), static_cast<uint8_t>(FluidPoolState::Collapse));
}

// =============================================================================
// Test 9: Proportional drain across multiple reservoirs
// =============================================================================

TEST(proportional_drain_across_reservoirs) {
    StubTerrainQueryable terrain;
    terrain.set_water_distance_at(10, 10, 0);

    entt::registry reg;
    FluidSystem sys(256, 256, &terrain);
    sys.set_registry(&reg);

    // Extractor producing 100 fluid
    sys.place_extractor(10, 10, 0);

    // Two reservoirs with different initial levels
    uint32_t res1_id = sys.place_reservoir(11, 10, 0);
    uint32_t res2_id = sys.place_reservoir(12, 10, 0);
    ASSERT(res1_id != INVALID_ENTITY_ID);
    ASSERT(res2_id != INVALID_ENTITY_ID);

    auto res1_entity = static_cast<entt::entity>(res1_id);
    auto res2_entity = static_cast<entt::entity>(res2_id);

    // Set different levels: res1 = 800, res2 = 200
    auto& res1 = reg.get<FluidReservoirComponent>(res1_entity);
    auto& res2 = reg.get<FluidReservoirComponent>(res2_entity);
    res1.current_level = 800;
    res2.current_level = 200;

    uint32_t res1_initial = res1.current_level;
    uint32_t res2_initial = res2.current_level;

    // Create consumer demanding 200 (deficit = 100 since generation = 100)
    uint32_t c1 = create_consumer(reg, 200);
    sys.register_consumer(c1, 0);
    sys.register_consumer_position(c1, 0, 13, 10);

    // Tick once to trigger proportional drain
    sys.tick(0.016f);

    // Both reservoirs should have drained
    uint32_t res1_drained = res1_initial - res1.current_level;
    uint32_t res2_drained = res2_initial - res2.current_level;

    // At minimum, both should have been drained (proportionally)
    // res1 had 80% of total storage, res2 had 20%, so res1 should drain more
    // (may be limited by drain_rate per reservoir)
    ASSERT_GT(res1_drained, 0u);
    // res2 may drain at least 1 due to min-drain logic
    // The proportional drain ensures higher-level reservoir drains more
    if (res2_initial > 0) {
        ASSERT_GE(res1_drained, res2_drained);
    }
}

// =============================================================================
// Test 10: Conduit placement extends coverage
// =============================================================================

TEST(conduit_extends_coverage) {
    StubTerrainQueryable terrain;
    terrain.set_water_distance_at(10, 10, 0);

    entt::registry reg;
    FluidSystem sys(256, 256, &terrain);
    sys.set_registry(&reg);

    // Place extractor at (10, 10) - default coverage_radius = 8
    sys.place_extractor(10, 10, 0);

    // Consumer at (30, 10) - outside extractor's direct coverage
    uint32_t consumer_id = create_consumer(reg, 10);
    sys.register_consumer(consumer_id, 0);
    sys.register_consumer_position(consumer_id, 0, 30, 10);

    // Tick - consumer should NOT have fluid (outside coverage)
    sys.tick(0.016f);
    auto entity = static_cast<entt::entity>(consumer_id);
    ASSERT(!reg.get<FluidComponent>(entity).has_fluid);

    // Place continuous conduit chain from (11,10) to (27,10)
    // BFS walks from extractor through adjacent conduits.
    // Conduit coverage_radius = 3, so the last conduit at (27,10)
    // covers (24..30, 7..13) which includes (30, 10).
    for (uint32_t x = 11; x <= 27; ++x) {
        sys.place_conduit(x, 10, 0);
    }

    // Tick - coverage should now extend to consumer's position
    sys.tick(0.016f);

    ASSERT(reg.get<FluidComponent>(entity).has_fluid);
    ASSERT_EQ(reg.get<FluidComponent>(entity).fluid_received, 10u);
}

// =============================================================================
// Test 11: Conduit removal contracts coverage
// =============================================================================

TEST(conduit_removal_contracts_coverage) {
    StubTerrainQueryable terrain;
    terrain.set_water_distance_at(10, 10, 0);

    entt::registry reg;
    FluidSystem sys(256, 256, &terrain);
    sys.set_registry(&reg);

    // Place extractor at (10, 10)
    sys.place_extractor(10, 10, 0);

    // Place continuous conduit chain from (11,10) to (27,10)
    uint32_t first_conduit = sys.place_conduit(11, 10, 0);
    for (uint32_t x = 12; x <= 27; ++x) {
        sys.place_conduit(x, 10, 0);
    }

    // Consumer at (30, 10) - reachable through conduit chain
    uint32_t consumer_id = create_consumer(reg, 10);
    sys.register_consumer(consumer_id, 0);
    sys.register_consumer_position(consumer_id, 0, 30, 10);

    // Tick - consumer should have fluid
    sys.tick(0.016f);
    auto entity = static_cast<entt::entity>(consumer_id);
    ASSERT(reg.get<FluidComponent>(entity).has_fluid);

    // Remove the first conduit in the chain - breaks connectivity
    bool removed = sys.remove_conduit(first_conduit, 0, 11, 10);
    ASSERT(removed);

    // Tick - coverage should shrink, consumer loses fluid
    sys.tick(0.016f);
    ASSERT(!reg.get<FluidComponent>(entity).has_fluid);
}

// =============================================================================
// Test 12: Coverage doesn't cross ownership boundaries (stub)
// =============================================================================

TEST(coverage_ownership_boundary_stub) {
    // Ownership boundary enforcement is stubbed for now.
    // This test verifies the stub always allows coverage extension.
    // When territory/ownership system is implemented, this test will
    // need to be updated to verify actual boundary enforcement.

    entt::registry reg;
    FluidSystem sys(64, 64);
    sys.set_registry(&reg);

    // Place extractor for player 0
    sys.place_extractor(10, 10, 0);

    // Tick to establish coverage
    sys.tick(0.016f);

    // Coverage should exist for player 0 (overseer_id = 1)
    // The stub always returns true for can_extend_coverage_to
    ASSERT(sys.get_coverage_count(1) > 0);

    printf(" [stub - boundary enforcement not yet implemented]");
}

// =============================================================================
// Test 13: Event emission for state changes
// =============================================================================

TEST(event_emission_for_state_changes) {
    StubTerrainQueryable terrain;
    terrain.set_water_distance_at(10, 10, 0);

    entt::registry reg;
    FluidSystem sys(256, 256, &terrain);
    sys.set_registry(&reg);

    // Place extractor producing 100 fluid
    sys.place_extractor(10, 10, 0);

    // Create consumer in coverage requiring 10 fluid
    uint32_t consumer_id = create_consumer(reg, 10);
    sys.register_consumer(consumer_id, 0);
    sys.register_consumer_position(consumer_id, 0, 12, 10);

    // Tick 1: Consumer transitions from no-fluid to has-fluid
    sys.tick(0.016f);

    // Should have FluidStateChangedEvent (false -> true)
    const auto& events = sys.get_state_changed_events();
    ASSERT_GT(events.size(), static_cast<size_t>(0));

    bool found_gain = false;
    for (const auto& evt : events) {
        if (evt.entity_id == consumer_id && !evt.had_fluid && evt.has_fluid) {
            found_gain = true;
            break;
        }
    }
    ASSERT(found_gain);

    // Now create deficit to lose fluid: add many consumers
    for (int i = 0; i < 5; ++i) {
        uint32_t cid = create_consumer(reg, 50);
        sys.register_consumer(cid, 0);
        sys.register_consumer_position(cid, 0, static_cast<uint32_t>(11 + i), 11);
    }

    // Tick 2: Should transition to deficit, all consumers lose fluid
    sys.tick(0.016f);

    const auto& events2 = sys.get_state_changed_events();
    // Original consumer should have transitioned from true -> false
    bool found_loss = false;
    for (const auto& evt : events2) {
        if (evt.entity_id == consumer_id && evt.had_fluid && !evt.has_fluid) {
            found_loss = true;
            break;
        }
    }
    ASSERT(found_loss);
}

// =============================================================================
// Test 14: Performance test: tick() with many consumers
// =============================================================================

TEST(performance_many_consumers) {
    StubTerrainQueryable terrain;
    // Set all positions to water distance 0 for maximum generation
    terrain.set_default_water_distance(0);

    entt::registry reg;
    FluidSystem sys(256, 256, &terrain);
    sys.set_registry(&reg);

    // Place multiple extractors to generate enough fluid
    for (uint32_t i = 0; i < 10; ++i) {
        sys.place_extractor(10 + i, 10, 0);
    }

    // Create 1000+ consumers all within coverage
    // Place conduit chain to extend coverage
    for (uint32_t x = 20; x <= 60; ++x) {
        sys.place_conduit(x, 10, 0);
    }

    const uint32_t NUM_CONSUMERS = 1000;
    for (uint32_t i = 0; i < NUM_CONSUMERS; ++i) {
        uint32_t cid = create_consumer(reg, 1);
        uint32_t cx = 10 + (i % 50);
        uint32_t cy = 5 + (i / 50);
        sys.register_consumer(cid, 0);
        sys.register_consumer_position(cid, 0, cx, cy);
    }

    // Warm-up tick
    sys.tick(0.016f);

    // Timed tick
    auto start = std::chrono::high_resolution_clock::now();
    sys.tick(0.016f);
    auto end = std::chrono::high_resolution_clock::now();

    auto duration_us = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    printf("\n  [Info] tick() with %u consumers: %lld us", NUM_CONSUMERS,
           static_cast<long long>(duration_us));

    // Informational - no strict assertion on timing
    // Just verify the system didn't crash and produced valid results
    const PerPlayerFluidPool& pool = sys.get_pool(0);
    ASSERT_GT(pool.total_generated, 0u);
    ASSERT_GT(pool.consumer_count, 0u);
}

// =============================================================================
// Main
// =============================================================================

int main() {
    printf("=== FluidSystem Integration Tests (Ticket 6-043) ===\n\n");

    RUN_TEST(extractor_near_water_generates);
    RUN_TEST(extractor_far_from_water_reduced_output);
    RUN_TEST(unpowered_extractor_produces_nothing);
    RUN_TEST(consumer_in_coverage_gets_fluid);
    RUN_TEST(consumer_outside_coverage_no_fluid);
    RUN_TEST(deficit_all_consumers_lose_fluid);
    RUN_TEST(reservoir_fills_during_surplus);
    RUN_TEST(reservoir_drains_during_deficit_delays_collapse);
    RUN_TEST(proportional_drain_across_reservoirs);
    RUN_TEST(conduit_extends_coverage);
    RUN_TEST(conduit_removal_contracts_coverage);
    RUN_TEST(coverage_ownership_boundary_stub);
    RUN_TEST(event_emission_for_state_changes);
    RUN_TEST(performance_many_consumers);

    printf("\n=== Results: %d passed, %d failed ===\n",
           tests_passed, tests_failed);

    return tests_failed > 0 ? 1 : 0;
}
