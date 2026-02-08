/**
 * @file test_multiplayer_sync.cpp
 * @brief Multiplayer sync verification tests for FluidSystem (Ticket 6-044)
 *
 * Since there is no actual networking layer, these tests verify DETERMINISM
 * guarantees that ensure server-client consistency:
 *
 * 1. has_fluid state serialization round-trip
 * 2. All-or-nothing distribution is consistent across twin systems
 * 3. FluidPoolSyncMessage serialization round-trip
 * 4. Reservoir levels included in pool sync round-trip
 * 5. Coverage reconstruction produces identical results
 * 6. Rival fluid states visible (all players' pools accessible)
 * 7. Compact bit packing round-trip for has_fluid
 *
 * Uses printf test pattern consistent with other fluid tests.
 *
 * @see /docs/epics/epic-6/tickets.md (ticket 6-044)
 */

#include <sims3000/fluid/FluidSystem.h>
#include <sims3000/fluid/FluidComponent.h>
#include <sims3000/fluid/FluidProducerComponent.h>
#include <sims3000/fluid/FluidReservoirComponent.h>
#include <sims3000/fluid/FluidConduitComponent.h>
#include <sims3000/fluid/FluidExtractorConfig.h>
#include <sims3000/fluid/FluidReservoirConfig.h>
#include <sims3000/fluid/FluidSerialization.h>
#include <sims3000/fluid/FluidEvents.h>
#include <sims3000/fluid/PerPlayerFluidPool.h>
#include <sims3000/energy/EnergyComponent.h>
#include <sims3000/building/ForwardDependencyInterfaces.h>
#include <sims3000/terrain/ITerrainQueryable.h>
#include <sims3000/terrain/TerrainTypes.h>
#include <entt/entt.hpp>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

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

// =============================================================================
// Stub TerrainQueryable for testing
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

    uint32_t get_water_distance(int32_t /*x*/, int32_t /*y*/) const override {
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

    void set_default_water_distance(uint32_t dist) {
        m_default_water_distance = dist;
    }

private:
    uint32_t m_default_water_distance;
};

// =============================================================================
// Helper: Create a consumer entity with FluidComponent
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
// Helper: Build identical scenario in two separate systems
// =============================================================================

struct SyncScenario {
    entt::registry reg;
    FluidSystem sys;
    StubTerrainQueryable terrain;
    uint32_t extractor_id;
    uint32_t consumer1;
    uint32_t consumer2;
    uint32_t consumer3;

    SyncScenario()
        : sys(128, 128, &terrain)
    {
        terrain.set_default_water_distance(0);
        sys.set_registry(&reg);
    }
};

static void build_deficit_scenario(SyncScenario& s) {
    // Extractor producing 100 fluid (distance 0)
    s.extractor_id = s.sys.place_extractor(10, 10, 0);

    // Three consumers each requiring 50 (total 150 > 100 generation = deficit)
    s.consumer1 = create_consumer(s.reg, 50);
    s.sys.register_consumer(s.consumer1, 0);
    s.sys.register_consumer_position(s.consumer1, 0, 11, 10);

    s.consumer2 = create_consumer(s.reg, 50);
    s.sys.register_consumer(s.consumer2, 0);
    s.sys.register_consumer_position(s.consumer2, 0, 12, 10);

    s.consumer3 = create_consumer(s.reg, 50);
    s.sys.register_consumer(s.consumer3, 0);
    s.sys.register_consumer_position(s.consumer3, 0, 13, 10);
}

static void build_surplus_scenario(SyncScenario& s) {
    // Extractor producing 100 fluid
    s.extractor_id = s.sys.place_extractor(10, 10, 0);

    // One consumer requiring 10 (well within surplus)
    s.consumer1 = create_consumer(s.reg, 10);
    s.sys.register_consumer(s.consumer1, 0);
    s.sys.register_consumer_position(s.consumer1, 0, 12, 10);
}

// =============================================================================
// Test 1: has_fluid state consistency (serialization round-trip)
// =============================================================================

TEST(has_fluid_serialization_round_trip) {
    // Create a FluidComponent with specific values
    FluidComponent original{};
    original.fluid_required = 100;
    original.fluid_received = 100;
    original.has_fluid = true;

    // Serialize
    std::vector<uint8_t> buffer;
    serialize_fluid_component(original, buffer);

    // Deserialize
    FluidComponent deserialized{};
    size_t consumed = deserialize_fluid_component(buffer.data(), buffer.size(), deserialized);

    ASSERT(consumed > 0);
    ASSERT_EQ(deserialized.fluid_required, original.fluid_required);
    ASSERT_EQ(deserialized.fluid_received, original.fluid_received);
    ASSERT_EQ(deserialized.has_fluid, original.has_fluid);

    // Also test with has_fluid = false
    FluidComponent no_fluid{};
    no_fluid.fluid_required = 50;
    no_fluid.fluid_received = 0;
    no_fluid.has_fluid = false;

    buffer.clear();
    serialize_fluid_component(no_fluid, buffer);

    FluidComponent no_fluid_deser{};
    consumed = deserialize_fluid_component(buffer.data(), buffer.size(), no_fluid_deser);

    ASSERT(consumed > 0);
    ASSERT_EQ(no_fluid_deser.fluid_required, no_fluid.fluid_required);
    ASSERT_EQ(no_fluid_deser.fluid_received, no_fluid.fluid_received);
    ASSERT_EQ(no_fluid_deser.has_fluid, no_fluid.has_fluid);
}

// =============================================================================
// Test 2: All-or-nothing distribution is consistent across twin systems
// =============================================================================

TEST(all_or_nothing_distribution_consistent) {
    // Two identical systems with same inputs should produce identical has_fluid results

    // --- System A ---
    SyncScenario a;
    build_deficit_scenario(a);
    a.sys.tick(0.016f);

    bool a_c1_fluid = a.reg.get<FluidComponent>(static_cast<entt::entity>(a.consumer1)).has_fluid;
    bool a_c2_fluid = a.reg.get<FluidComponent>(static_cast<entt::entity>(a.consumer2)).has_fluid;
    bool a_c3_fluid = a.reg.get<FluidComponent>(static_cast<entt::entity>(a.consumer3)).has_fluid;
    uint32_t a_c1_recv = a.reg.get<FluidComponent>(static_cast<entt::entity>(a.consumer1)).fluid_received;

    // --- System B (identical setup) ---
    SyncScenario b;
    build_deficit_scenario(b);
    b.sys.tick(0.016f);

    bool b_c1_fluid = b.reg.get<FluidComponent>(static_cast<entt::entity>(b.consumer1)).has_fluid;
    bool b_c2_fluid = b.reg.get<FluidComponent>(static_cast<entt::entity>(b.consumer2)).has_fluid;
    bool b_c3_fluid = b.reg.get<FluidComponent>(static_cast<entt::entity>(b.consumer3)).has_fluid;
    uint32_t b_c1_recv = b.reg.get<FluidComponent>(static_cast<entt::entity>(b.consumer1)).fluid_received;

    // Both runs must produce identical distribution decisions
    ASSERT_EQ(a_c1_fluid, b_c1_fluid);
    ASSERT_EQ(a_c2_fluid, b_c2_fluid);
    ASSERT_EQ(a_c3_fluid, b_c3_fluid);
    ASSERT_EQ(a_c1_recv, b_c1_recv);

    // Verify all-or-nothing: in deficit, all consumers should have no fluid
    ASSERT(!a_c1_fluid);
    ASSERT(!a_c2_fluid);
    ASSERT(!a_c3_fluid);

    // Also test surplus scenario
    SyncScenario c;
    build_surplus_scenario(c);
    c.sys.tick(0.016f);

    SyncScenario d;
    build_surplus_scenario(d);
    d.sys.tick(0.016f);

    bool c_fluid = c.reg.get<FluidComponent>(static_cast<entt::entity>(c.consumer1)).has_fluid;
    bool d_fluid = d.reg.get<FluidComponent>(static_cast<entt::entity>(d.consumer1)).has_fluid;

    ASSERT_EQ(c_fluid, d_fluid);
    ASSERT(c_fluid); // Should have fluid in surplus
}

// =============================================================================
// Test 3: Pool state sync (FluidPoolSyncMessage round-trip)
// =============================================================================

TEST(pool_sync_message_round_trip) {
    FluidPoolSyncMessage msg{};
    msg.owner = 2;
    msg.state = static_cast<uint8_t>(FluidPoolState::Deficit);
    msg.total_generated = 500;
    msg.total_consumed = 1200;
    msg.surplus = -700;
    msg.reservoir_stored = 300;
    msg.reservoir_capacity = 2000;

    // Serialize
    std::vector<uint8_t> buffer;
    serialize_pool_sync(msg, buffer);

    // Verify serialized size
    ASSERT_EQ(buffer.size(), FLUID_POOL_SYNC_MESSAGE_SIZE);

    // Deserialize
    FluidPoolSyncMessage deserialized{};
    size_t consumed = deserialize_pool_sync(buffer.data(), buffer.size(), deserialized);

    ASSERT_EQ(consumed, FLUID_POOL_SYNC_MESSAGE_SIZE);
    ASSERT_EQ(deserialized.owner, msg.owner);
    ASSERT_EQ(deserialized.state, msg.state);
    ASSERT_EQ(deserialized.total_generated, msg.total_generated);
    ASSERT_EQ(deserialized.total_consumed, msg.total_consumed);
    ASSERT_EQ(deserialized.surplus, msg.surplus);
    ASSERT_EQ(deserialized.reservoir_stored, msg.reservoir_stored);
    ASSERT_EQ(deserialized.reservoir_capacity, msg.reservoir_capacity);
}

// =============================================================================
// Test 4: Reservoir levels included in pool sync round-trip
// =============================================================================

TEST(reservoir_levels_sync_round_trip) {
    // Build pool sync message from a running system that has reservoirs
    StubTerrainQueryable terrain;
    terrain.set_default_water_distance(0);

    entt::registry reg;
    FluidSystem sys(128, 128, &terrain);
    sys.set_registry(&reg);

    // Extractor + reservoir
    sys.place_extractor(10, 10, 0);
    uint32_t res_id = sys.place_reservoir(11, 10, 0);

    // Pre-fill reservoir
    auto res_entity = static_cast<entt::entity>(res_id);
    auto& reservoir = reg.get<FluidReservoirComponent>(res_entity);
    reservoir.current_level = 450;

    // Tick to update pool
    sys.tick(0.016f);

    const PerPlayerFluidPool& pool = sys.get_pool(0);

    // Build sync message from pool state
    FluidPoolSyncMessage msg{};
    msg.owner = 0;
    msg.state = static_cast<uint8_t>(pool.state);
    msg.total_generated = pool.total_generated;
    msg.total_consumed = pool.total_consumed;
    msg.surplus = pool.surplus;
    msg.reservoir_stored = pool.total_reservoir_stored;
    msg.reservoir_capacity = pool.total_reservoir_capacity;

    // Serialize
    std::vector<uint8_t> buffer;
    serialize_pool_sync(msg, buffer);

    // Deserialize
    FluidPoolSyncMessage deserialized{};
    deserialize_pool_sync(buffer.data(), buffer.size(), deserialized);

    // Verify reservoir fields survived round-trip
    ASSERT_EQ(deserialized.reservoir_stored, msg.reservoir_stored);
    ASSERT_EQ(deserialized.reservoir_capacity, msg.reservoir_capacity);
    ASSERT_GT(deserialized.reservoir_stored, 0u);
    ASSERT_GT(deserialized.reservoir_capacity, 0u);
}

// =============================================================================
// Test 5: Coverage reconstruction matches across twin systems
// =============================================================================

TEST(coverage_reconstruction_matches) {
    // Two identical systems should compute the same coverage grid
    auto build_and_tick = [](std::vector<uint8_t>& coverage_snapshot,
                              uint32_t& coverage_count) {
        StubTerrainQueryable terrain;
        terrain.set_default_water_distance(0);

        entt::registry reg;
        FluidSystem sys(64, 64, &terrain);
        sys.set_registry(&reg);

        // Place extractor at center
        sys.place_extractor(20, 20, 0);

        // Place conduit chain extending from extractor
        for (uint32_t x = 21; x <= 30; ++x) {
            sys.place_conduit(x, 20, 0);
        }

        // Tick to calculate coverage via BFS
        sys.tick(0.016f);

        // Snapshot coverage grid: check coverage count for overseer_id = 1 (player 0)
        coverage_count = sys.get_coverage_count(1);

        // Capture each tile's coverage state
        coverage_snapshot.clear();
        for (uint32_t y = 0; y < 64; ++y) {
            for (uint32_t x = 0; x < 64; ++x) {
                coverage_snapshot.push_back(sys.get_coverage_at(x, y));
            }
        }
    };

    std::vector<uint8_t> snapshot_a, snapshot_b;
    uint32_t count_a, count_b;

    build_and_tick(snapshot_a, count_a);
    build_and_tick(snapshot_b, count_b);

    ASSERT_EQ(count_a, count_b);
    ASSERT_GT(count_a, 0u);
    ASSERT_EQ(snapshot_a.size(), snapshot_b.size());

    for (size_t i = 0; i < snapshot_a.size(); ++i) {
        ASSERT_EQ(snapshot_a[i], snapshot_b[i]);
    }
}

// =============================================================================
// Test 6: Rival fluid states visible (all players' pools accessible)
// =============================================================================

TEST(rival_fluid_states_visible) {
    StubTerrainQueryable terrain;
    terrain.set_default_water_distance(0);

    entt::registry reg;
    FluidSystem sys(128, 128, &terrain);
    sys.set_registry(&reg);

    // Set up all 4 players with different fluid scenarios
    // Player 0: extractor + small consumer => surplus
    sys.place_extractor(10, 10, 0);
    uint32_t p0_consumer = create_consumer(reg, 10);
    sys.register_consumer(p0_consumer, 0);
    sys.register_consumer_position(p0_consumer, 0, 11, 10);

    // Player 1: extractor + heavy consumer => deficit
    sys.place_extractor(30, 30, 1);
    uint32_t p1_consumer = create_consumer(reg, 200);
    sys.register_consumer(p1_consumer, 1);
    sys.register_consumer_position(p1_consumer, 1, 31, 30);

    // Player 2: no extractor, just consumer => collapse
    uint32_t p2_consumer = create_consumer(reg, 50);
    sys.register_consumer(p2_consumer, 2);
    sys.register_consumer_position(p2_consumer, 2, 50, 50);

    // Player 3: extractor, no consumers => healthy
    sys.place_extractor(70, 70, 3);

    // Tick
    sys.tick(0.016f);

    // All players' pools should be accessible
    const PerPlayerFluidPool& p0 = sys.get_pool(0);
    const PerPlayerFluidPool& p1 = sys.get_pool(1);
    const PerPlayerFluidPool& p2 = sys.get_pool(2);
    const PerPlayerFluidPool& p3 = sys.get_pool(3);

    // Player 0: has generation
    ASSERT_GT(p0.total_generated, 0u);

    // Player 1: has generation
    ASSERT_GT(p1.total_generated, 0u);

    // Player 2: no generation
    ASSERT_EQ(p2.total_generated, 0u);

    // Player 3: has generation, no consumers
    ASSERT_GT(p3.total_generated, 0u);
    ASSERT_EQ(p3.total_consumed, 0u);

    // Build pool sync messages for all players (simulating network sync)
    for (uint8_t owner = 0; owner < MAX_PLAYERS; ++owner) {
        const PerPlayerFluidPool& pool = sys.get_pool(owner);

        FluidPoolSyncMessage msg{};
        msg.owner = owner;
        msg.state = static_cast<uint8_t>(pool.state);
        msg.total_generated = pool.total_generated;
        msg.total_consumed = pool.total_consumed;
        msg.surplus = pool.surplus;
        msg.reservoir_stored = pool.total_reservoir_stored;
        msg.reservoir_capacity = pool.total_reservoir_capacity;

        std::vector<uint8_t> buffer;
        serialize_pool_sync(msg, buffer);

        FluidPoolSyncMessage deserialized{};
        deserialize_pool_sync(buffer.data(), buffer.size(), deserialized);

        // Verify round-trip for each player
        ASSERT_EQ(deserialized.owner, owner);
        ASSERT_EQ(deserialized.total_generated, pool.total_generated);
        ASSERT_EQ(deserialized.total_consumed, pool.total_consumed);
        ASSERT_EQ(deserialized.surplus, pool.surplus);
    }
}

// =============================================================================
// Test 7: Compact bit packing round-trip
// =============================================================================

TEST(compact_bit_packing_round_trip) {
    // Pack has_fluid for many entities, unpack, verify all match
    const uint32_t NUM_ENTITIES = 37; // not a multiple of 8 to test padding
    bool states[37] = {
        true, false, true, true, false, false, true, false,  // byte 0
        true, true, false, false, true, false, true, true,   // byte 1
        false, true, true, false, false, true, false, true,  // byte 2
        true, false, false, true, true, true, false, false,  // byte 3
        true, true, false, true, false                       // byte 4 (partial)
    };

    std::vector<uint8_t> buffer;
    pack_fluid_states(states, NUM_ENTITIES, buffer);

    // Expected size: 4 (count) + ceil(37/8) = 4 + 5 = 9 bytes
    ASSERT_EQ(buffer.size(), static_cast<size_t>(9));

    bool restored[37] = {};
    size_t consumed = unpack_fluid_states(buffer.data(), buffer.size(), restored, NUM_ENTITIES);

    ASSERT(consumed > 0);
    for (uint32_t i = 0; i < NUM_ENTITIES; ++i) {
        ASSERT_EQ(restored[i], states[i]);
    }
}

// =============================================================================
// Test: Compact bit packing with large entity count
// =============================================================================

TEST(bit_packing_large_count) {
    const uint32_t NUM_ENTITIES = 256;
    bool states[256] = {};

    // Alternate pattern
    for (uint32_t i = 0; i < NUM_ENTITIES; ++i) {
        states[i] = (i % 3 == 0); // every 3rd entity has fluid
    }

    std::vector<uint8_t> buffer;
    pack_fluid_states(states, NUM_ENTITIES, buffer);

    bool restored[256] = {};
    size_t consumed = unpack_fluid_states(buffer.data(), buffer.size(), restored, NUM_ENTITIES);

    ASSERT(consumed > 0);
    for (uint32_t i = 0; i < NUM_ENTITIES; ++i) {
        ASSERT_EQ(restored[i], states[i]);
    }
}

// =============================================================================
// Test: Twin systems produce identical pool state after tick
// =============================================================================

TEST(twin_systems_identical_tick_results) {
    // Two completely independent system/registry pairs with same scenario
    SyncScenario a;
    build_surplus_scenario(a);
    a.sys.tick(0.016f);

    SyncScenario b;
    build_surplus_scenario(b);
    b.sys.tick(0.016f);

    // Pools must match
    const PerPlayerFluidPool& pool_a = a.sys.get_pool(0);
    const PerPlayerFluidPool& pool_b = b.sys.get_pool(0);

    ASSERT_EQ(pool_a.total_generated, pool_b.total_generated);
    ASSERT_EQ(pool_a.total_consumed, pool_b.total_consumed);
    ASSERT_EQ(pool_a.surplus, pool_b.surplus);
    ASSERT_EQ(static_cast<uint8_t>(pool_a.state), static_cast<uint8_t>(pool_b.state));
    ASSERT_EQ(pool_a.extractor_count, pool_b.extractor_count);
    ASSERT_EQ(pool_a.consumer_count, pool_b.consumer_count);
}

// =============================================================================
// Test: Pool state transitions are deterministic
// =============================================================================

TEST(pool_state_transitions_deterministic) {
    auto run_scenario = [](FluidPoolState& final_state,
                           int32_t& final_surplus,
                           bool& had_deficit_event,
                           bool& had_collapse_event) {
        StubTerrainQueryable terrain;
        terrain.set_default_water_distance(0);

        entt::registry reg;
        FluidSystem sys(128, 128, &terrain);
        sys.set_registry(&reg);

        // Extractor with low output -> deficit
        sys.place_extractor(10, 10, 0);

        // Consumer demanding far more than supply
        uint32_t cid = create_consumer(reg, 500);
        sys.register_consumer(cid, 0);
        sys.register_consumer_position(cid, 0, 12, 10);

        sys.tick(0.016f);

        final_state = sys.get_pool_state(0);
        final_surplus = sys.get_pool(0).surplus;
        had_deficit_event = !sys.get_deficit_began_events().empty();
        had_collapse_event = !sys.get_collapse_began_events().empty();
    };

    FluidPoolState state_a, state_b;
    int32_t surplus_a, surplus_b;
    bool deficit_a, deficit_b;
    bool collapse_a, collapse_b;

    run_scenario(state_a, surplus_a, deficit_a, collapse_a);
    run_scenario(state_b, surplus_b, deficit_b, collapse_b);

    ASSERT_EQ(static_cast<uint8_t>(state_a), static_cast<uint8_t>(state_b));
    ASSERT_EQ(surplus_a, surplus_b);
    ASSERT_EQ(deficit_a, deficit_b);
    ASSERT_EQ(collapse_a, collapse_b);
}

// =============================================================================
// Test: Multiple ticks produce deterministic results
// =============================================================================

TEST(multiple_ticks_deterministic) {
    auto run_n_ticks = [](uint32_t n, int32_t& final_surplus,
                          FluidPoolState& final_state,
                          uint32_t& final_gen) {
        StubTerrainQueryable terrain;
        terrain.set_default_water_distance(0);

        entt::registry reg;
        FluidSystem sys(128, 128, &terrain);
        sys.set_registry(&reg);

        sys.place_extractor(10, 10, 0);

        uint32_t c1 = create_consumer(reg, 30);
        sys.register_consumer(c1, 0);
        sys.register_consumer_position(c1, 0, 12, 10);

        uint32_t c2 = create_consumer(reg, 20);
        sys.register_consumer(c2, 0);
        sys.register_consumer_position(c2, 0, 13, 10);

        for (uint32_t i = 0; i < n; ++i) {
            sys.tick(0.016f);
        }

        final_surplus = sys.get_pool(0).surplus;
        final_state = sys.get_pool_state(0);
        final_gen = sys.get_pool(0).total_generated;
    };

    int32_t surplus_a, surplus_b;
    FluidPoolState state_a, state_b;
    uint32_t gen_a, gen_b;

    run_n_ticks(10, surplus_a, state_a, gen_a);
    run_n_ticks(10, surplus_b, state_b, gen_b);

    ASSERT_EQ(surplus_a, surplus_b);
    ASSERT_EQ(static_cast<uint8_t>(state_a), static_cast<uint8_t>(state_b));
    ASSERT_EQ(gen_a, gen_b);
}

// =============================================================================
// Test: Serialization from live entity
// =============================================================================

TEST(serialization_from_live_entity) {
    StubTerrainQueryable terrain;
    terrain.set_default_water_distance(0);

    entt::registry reg;
    FluidSystem sys(128, 128, &terrain);
    sys.set_registry(&reg);

    // Place extractor, register consumer
    sys.place_extractor(10, 10, 0);
    uint32_t cid = create_consumer(reg, 10);
    sys.register_consumer(cid, 0);
    sys.register_consumer_position(cid, 0, 12, 10);

    // Tick to distribute fluid
    sys.tick(0.016f);

    // Read component from live entity
    auto entity = static_cast<entt::entity>(cid);
    const FluidComponent& live_comp = reg.get<FluidComponent>(entity);

    // Serialize
    std::vector<uint8_t> buffer;
    serialize_fluid_component(live_comp, buffer);

    // Deserialize
    FluidComponent deserialized{};
    deserialize_fluid_component(buffer.data(), buffer.size(), deserialized);

    // All fields must match
    ASSERT_EQ(deserialized.fluid_required, live_comp.fluid_required);
    ASSERT_EQ(deserialized.fluid_received, live_comp.fluid_received);
    ASSERT_EQ(deserialized.has_fluid, live_comp.has_fluid);
}

// =============================================================================
// Main
// =============================================================================

int main() {
    printf("=== FluidSystem Multiplayer Sync Tests (Ticket 6-044) ===\n\n");

    // Serialization round-trips
    RUN_TEST(has_fluid_serialization_round_trip);
    RUN_TEST(pool_sync_message_round_trip);
    RUN_TEST(reservoir_levels_sync_round_trip);
    RUN_TEST(compact_bit_packing_round_trip);
    RUN_TEST(bit_packing_large_count);
    RUN_TEST(serialization_from_live_entity);

    // Determinism: twin system distribution
    RUN_TEST(all_or_nothing_distribution_consistent);
    RUN_TEST(twin_systems_identical_tick_results);

    // Determinism: pool state
    RUN_TEST(pool_state_transitions_deterministic);
    RUN_TEST(multiple_ticks_deterministic);

    // Determinism: coverage
    RUN_TEST(coverage_reconstruction_matches);

    // Cross-player visibility
    RUN_TEST(rival_fluid_states_visible);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
