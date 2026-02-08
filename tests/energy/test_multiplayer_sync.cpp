/**
 * @file test_multiplayer_sync.cpp
 * @brief Multiplayer sync verification tests (Ticket 5-042)
 *
 * Since there is no actual networking layer, these tests verify DETERMINISM
 * guarantees that ensure server-client consistency:
 *
 * 1. Rationing order is deterministic: same scenario twice -> same consumers powered
 * 2. Two separate EnergySystem instances with identical state produce identical
 *    results after tick()
 * 3. Pool state transitions are deterministic
 * 4. Coverage reconstruction produces identical results
 * 5. Multiple players can see each other's pool states (get_pool works for all)
 * 6. Serialization round-trip tests (read EnergyComponent fields, write, verify)
 */

#include <sims3000/energy/EnergySystem.h>
#include <sims3000/energy/EnergyComponent.h>
#include <sims3000/energy/EnergyProducerComponent.h>
#include <sims3000/energy/EnergyConduitComponent.h>
#include <sims3000/energy/EnergyEnums.h>
#include <sims3000/energy/EnergyPriorities.h>
#include <sims3000/energy/EnergySerialization.h>
#include <sims3000/energy/PerPlayerEnergyPool.h>
#include <entt/entt.hpp>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

using namespace sims3000::energy;

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
// Helper: set up coverage at a position for an owner
// =============================================================================

static void set_coverage(EnergySystem& sys, uint32_t x, uint32_t y, uint8_t player_id) {
    uint8_t overseer_id = player_id + 1;
    sys.get_coverage_grid_mut().set(x, y, overseer_id);
}

// =============================================================================
// Helper: create and register a nexus (no position)
// =============================================================================

static uint32_t create_nexus(entt::registry& reg, EnergySystem& sys,
                              uint8_t owner, uint32_t base_output,
                              bool is_online = true) {
    auto entity = reg.create();
    uint32_t eid = static_cast<uint32_t>(entity);

    EnergyProducerComponent producer{};
    producer.base_output = base_output;
    producer.current_output = 0;
    producer.efficiency = 1.0f;
    producer.age_factor = 1.0f;
    producer.nexus_type = static_cast<uint8_t>(NexusType::Carbon);
    producer.is_online = is_online;
    reg.emplace<EnergyProducerComponent>(entity, producer);

    sys.register_nexus(eid, owner);
    return eid;
}

// =============================================================================
// Helper: create nexus with position
// =============================================================================

static uint32_t create_nexus_at(entt::registry& reg, EnergySystem& sys,
                                 uint8_t owner, uint32_t base_output,
                                 uint32_t x, uint32_t y,
                                 bool is_online = true) {
    uint32_t eid = create_nexus(reg, sys, owner, base_output, is_online);
    sys.register_nexus_position(eid, owner, x, y);
    return eid;
}

// =============================================================================
// Helper: create consumer with priority and manual coverage
// =============================================================================

static uint32_t create_consumer_with_priority(entt::registry& reg, EnergySystem& sys,
                                               uint8_t owner, uint32_t x, uint32_t y,
                                               uint32_t energy_required, uint8_t priority) {
    auto entity = reg.create();
    uint32_t eid = static_cast<uint32_t>(entity);

    EnergyComponent ec{};
    ec.energy_required = energy_required;
    ec.priority = priority;
    reg.emplace<EnergyComponent>(entity, ec);

    sys.register_consumer(eid, owner);
    sys.register_consumer_position(eid, owner, x, y);
    set_coverage(sys, x, y, owner);
    return eid;
}

// =============================================================================
// Helper: create consumer without coverage (for tick tests with BFS)
// =============================================================================

static uint32_t create_consumer_no_coverage(entt::registry& reg, EnergySystem& sys,
                                              uint8_t owner, uint32_t x, uint32_t y,
                                              uint32_t energy_required, uint8_t priority) {
    auto entity = reg.create();
    uint32_t eid = static_cast<uint32_t>(entity);

    EnergyComponent ec{};
    ec.energy_required = energy_required;
    ec.priority = priority;
    reg.emplace<EnergyComponent>(entity, ec);

    sys.register_consumer(eid, owner);
    sys.register_consumer_position(eid, owner, x, y);
    return eid;
}

// =============================================================================
// Helper: build an identical scenario in a registry+system pair
//
// Creates:
// - One nexus for player 0 with 150 base output
// - Three consumers for player 0 with priorities Critical(100), Normal(100), Low(100)
// This produces a deficit of 150 - 300 = -150, triggering rationing.
// =============================================================================

struct ScenarioEntities {
    uint32_t nexus_id;
    uint32_t consumer_critical;
    uint32_t consumer_normal;
    uint32_t consumer_low;
};

static ScenarioEntities build_deficit_scenario(entt::registry& reg, EnergySystem& sys) {
    ScenarioEntities e{};
    e.nexus_id = create_nexus(reg, sys, 0, 150, true);
    e.consumer_critical = create_consumer_with_priority(reg, sys, 0, 1, 1, 100, ENERGY_PRIORITY_CRITICAL);
    e.consumer_normal   = create_consumer_with_priority(reg, sys, 0, 2, 2, 100, ENERGY_PRIORITY_NORMAL);
    e.consumer_low      = create_consumer_with_priority(reg, sys, 0, 3, 3, 100, ENERGY_PRIORITY_LOW);
    return e;
}

// =============================================================================
// Test 1: Rationing order is deterministic across two identical runs
// =============================================================================

TEST(rationing_order_deterministic) {
    // --- Run A ---
    entt::registry reg_a;
    EnergySystem sys_a(64, 64);
    sys_a.set_registry(&reg_a);

    ScenarioEntities ea = build_deficit_scenario(reg_a, sys_a);

    sys_a.update_all_nexus_outputs(0);
    sys_a.calculate_pool(0);
    ASSERT(sys_a.get_pool(0).surplus < 0);
    sys_a.distribute_energy(0);

    bool a_crit_powered = reg_a.get<EnergyComponent>(static_cast<entt::entity>(ea.consumer_critical)).is_powered;
    bool a_norm_powered = reg_a.get<EnergyComponent>(static_cast<entt::entity>(ea.consumer_normal)).is_powered;
    bool a_low_powered  = reg_a.get<EnergyComponent>(static_cast<entt::entity>(ea.consumer_low)).is_powered;

    uint32_t a_crit_recv = reg_a.get<EnergyComponent>(static_cast<entt::entity>(ea.consumer_critical)).energy_received;
    uint32_t a_norm_recv = reg_a.get<EnergyComponent>(static_cast<entt::entity>(ea.consumer_normal)).energy_received;
    uint32_t a_low_recv  = reg_a.get<EnergyComponent>(static_cast<entt::entity>(ea.consumer_low)).energy_received;

    // --- Run B (identical setup) ---
    entt::registry reg_b;
    EnergySystem sys_b(64, 64);
    sys_b.set_registry(&reg_b);

    ScenarioEntities eb = build_deficit_scenario(reg_b, sys_b);

    sys_b.update_all_nexus_outputs(0);
    sys_b.calculate_pool(0);
    ASSERT(sys_b.get_pool(0).surplus < 0);
    sys_b.distribute_energy(0);

    bool b_crit_powered = reg_b.get<EnergyComponent>(static_cast<entt::entity>(eb.consumer_critical)).is_powered;
    bool b_norm_powered = reg_b.get<EnergyComponent>(static_cast<entt::entity>(eb.consumer_normal)).is_powered;
    bool b_low_powered  = reg_b.get<EnergyComponent>(static_cast<entt::entity>(eb.consumer_low)).is_powered;

    uint32_t b_crit_recv = reg_b.get<EnergyComponent>(static_cast<entt::entity>(eb.consumer_critical)).energy_received;
    uint32_t b_norm_recv = reg_b.get<EnergyComponent>(static_cast<entt::entity>(eb.consumer_normal)).energy_received;
    uint32_t b_low_recv  = reg_b.get<EnergyComponent>(static_cast<entt::entity>(eb.consumer_low)).energy_received;

    // Both runs must produce identical rationing decisions
    ASSERT_EQ(a_crit_powered, b_crit_powered);
    ASSERT_EQ(a_norm_powered, b_norm_powered);
    ASSERT_EQ(a_low_powered, b_low_powered);

    ASSERT_EQ(a_crit_recv, b_crit_recv);
    ASSERT_EQ(a_norm_recv, b_norm_recv);
    ASSERT_EQ(a_low_recv, b_low_recv);

    // Verify the rationing itself is correct: critical powered, others not
    ASSERT(a_crit_powered);
    ASSERT(!a_norm_powered);
    ASSERT(!a_low_powered);
}

// =============================================================================
// Test 2: Two separate EnergySystem instances produce identical tick() results
// =============================================================================

TEST(twin_systems_identical_tick_results) {
    // Build two completely independent system/registry pairs with same scenario
    entt::registry reg_a;
    EnergySystem sys_a(64, 64);
    sys_a.set_registry(&reg_a);

    entt::registry reg_b;
    EnergySystem sys_b(64, 64);
    sys_b.set_registry(&reg_b);

    // Place nexus at same position for both
    uint32_t nx_a = create_nexus_at(reg_a, sys_a, 0, 500, 10, 10, true);
    uint32_t nx_b = create_nexus_at(reg_b, sys_b, 0, 500, 10, 10, true);

    // Place consumers at same positions for both (within nexus coverage radius)
    uint32_t c1_a = create_consumer_no_coverage(reg_a, sys_a, 0, 12, 10, 100, ENERGY_PRIORITY_CRITICAL);
    uint32_t c2_a = create_consumer_no_coverage(reg_a, sys_a, 0, 13, 10, 200, ENERGY_PRIORITY_NORMAL);

    uint32_t c1_b = create_consumer_no_coverage(reg_b, sys_b, 0, 12, 10, 100, ENERGY_PRIORITY_CRITICAL);
    uint32_t c2_b = create_consumer_no_coverage(reg_b, sys_b, 0, 13, 10, 200, ENERGY_PRIORITY_NORMAL);

    // Run tick on both
    sys_a.tick(0.05f);
    sys_b.tick(0.05f);

    // Pools must match
    const auto& pool_a = sys_a.get_pool(0);
    const auto& pool_b = sys_b.get_pool(0);

    ASSERT_EQ(pool_a.total_generated, pool_b.total_generated);
    ASSERT_EQ(pool_a.total_consumed, pool_b.total_consumed);
    ASSERT_EQ(pool_a.surplus, pool_b.surplus);
    ASSERT_EQ(static_cast<uint8_t>(pool_a.state), static_cast<uint8_t>(pool_b.state));

    // Consumer states must match
    auto get_ec_a = [&](uint32_t eid) -> const EnergyComponent& {
        return reg_a.get<EnergyComponent>(static_cast<entt::entity>(eid));
    };
    auto get_ec_b = [&](uint32_t eid) -> const EnergyComponent& {
        return reg_b.get<EnergyComponent>(static_cast<entt::entity>(eid));
    };

    ASSERT_EQ(get_ec_a(c1_a).is_powered, get_ec_b(c1_b).is_powered);
    ASSERT_EQ(get_ec_a(c1_a).energy_received, get_ec_b(c1_b).energy_received);
    ASSERT_EQ(get_ec_a(c2_a).is_powered, get_ec_b(c2_b).is_powered);
    ASSERT_EQ(get_ec_a(c2_a).energy_received, get_ec_b(c2_b).energy_received);
}

// =============================================================================
// Test 3: Pool state transitions are deterministic
// =============================================================================

TEST(pool_state_transitions_deterministic) {
    // Build two identical systems that will go through state transitions
    auto run_scenario = [](EnergyPoolState& final_state,
                           int32_t& final_surplus,
                           bool& had_deficit_event,
                           bool& had_collapse_event) {
        entt::registry reg;
        EnergySystem sys(64, 64);
        sys.set_registry(&reg);

        // Nexus with low output -> deficit
        create_nexus_at(reg, sys, 0, 50, 10, 10, true);

        // Consumer demanding far more than supply -> collapse
        create_consumer_no_coverage(reg, sys, 0, 12, 10, 500, ENERGY_PRIORITY_NORMAL);

        sys.tick(0.05f);

        final_state = sys.get_pool_state(0);
        final_surplus = sys.get_pool(0).surplus;
        had_deficit_event = !sys.get_deficit_began_events().empty();
        had_collapse_event = !sys.get_collapse_began_events().empty();
    };

    EnergyPoolState state_a, state_b;
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
// Test 4: Coverage reconstruction produces identical results
// =============================================================================

TEST(coverage_reconstruction_deterministic) {
    auto build_and_recalculate = [](std::vector<uint8_t>& coverage_snapshot,
                                     uint32_t& coverage_count) {
        entt::registry reg;
        EnergySystem sys(32, 32);
        sys.set_registry(&reg);

        // Place nexus at center
        create_nexus_at(reg, sys, 0, 500, 16, 16, true);

        // Place conduits extending from nexus in a line
        for (uint32_t x = 17; x <= 24; ++x) {
            auto entity = reg.create();
            uint32_t eid = static_cast<uint32_t>(entity);
            EnergyConduitComponent conduit{};
            conduit.coverage_radius = 3;
            reg.emplace<EnergyConduitComponent>(entity, conduit);
            sys.register_conduit_position(eid, 0, x, 16);
        }

        // Mark dirty and recalculate
        sys.mark_coverage_dirty(0);
        sys.recalculate_coverage(0);

        // Snapshot coverage grid
        coverage_count = sys.get_coverage_count(1); // overseer_id = player_id + 1
        const auto& grid = sys.get_coverage_grid();
        coverage_snapshot.clear();
        for (uint32_t y = 0; y < 32; ++y) {
            for (uint32_t x = 0; x < 32; ++x) {
                coverage_snapshot.push_back(grid.get_coverage_owner(x, y));
            }
        }
    };

    std::vector<uint8_t> snapshot_a, snapshot_b;
    uint32_t count_a, count_b;

    build_and_recalculate(snapshot_a, count_a);
    build_and_recalculate(snapshot_b, count_b);

    ASSERT_EQ(count_a, count_b);
    ASSERT_EQ(snapshot_a.size(), snapshot_b.size());

    for (size_t i = 0; i < snapshot_a.size(); ++i) {
        ASSERT_EQ(snapshot_a[i], snapshot_b[i]);
    }
}

// =============================================================================
// Test 5: Multiple players can see each other's pool states
// =============================================================================

TEST(cross_player_pool_visibility) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    // Set up all 4 players with different energy scenarios
    for (uint8_t player = 0; player < MAX_PLAYERS; ++player) {
        uint32_t base_output = 100 * (player + 1); // 100, 200, 300, 400
        create_nexus(reg, sys, player, base_output, true);

        uint32_t demand = 50 * (player + 1); // 50, 100, 150, 200
        uint32_t cx = player * 10 + 1;
        uint32_t cy = 1;
        create_consumer_with_priority(reg, sys, player, cx, cy, demand, ENERGY_PRIORITY_NORMAL);
    }

    // Update outputs and pools for all players
    for (uint8_t i = 0; i < MAX_PLAYERS; ++i) {
        sys.update_all_nexus_outputs(i);
        sys.calculate_pool(i);
    }

    // Any player should be able to read any other player's pool state
    for (uint8_t viewer = 0; viewer < MAX_PLAYERS; ++viewer) {
        for (uint8_t target = 0; target < MAX_PLAYERS; ++target) {
            const PerPlayerEnergyPool& pool = sys.get_pool(target);

            // Verify pool has correct owner
            ASSERT_EQ(pool.owner, target);

            // Verify pool has expected generation
            uint32_t expected_gen = 100 * (target + 1);
            ASSERT_EQ(pool.total_generated, expected_gen);

            // Verify pool state is accessible (no crash)
            EnergyPoolState state = sys.get_pool_state(target);
            // With surplus >= 0, state should be Healthy or Marginal
            ASSERT(pool.surplus >= 0);
            (void)state; // used for access verification
        }
    }
}

// =============================================================================
// Test 5b: get_pool returns consistent data for all players simultaneously
// =============================================================================

TEST(all_players_pool_consistent_after_tick) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    // Player 0: healthy surplus
    create_nexus_at(reg, sys, 0, 1000, 5, 5, true);
    create_consumer_no_coverage(reg, sys, 0, 6, 5, 100, ENERGY_PRIORITY_NORMAL);

    // Player 1: deficit
    create_nexus_at(reg, sys, 1, 50, 20, 5, true);
    create_consumer_no_coverage(reg, sys, 1, 21, 5, 300, ENERGY_PRIORITY_NORMAL);

    // Player 2: no generation (collapse territory)
    create_consumer_no_coverage(reg, sys, 2, 40, 5, 200, ENERGY_PRIORITY_NORMAL);

    // Player 3: no consumers (idle)
    create_nexus_at(reg, sys, 3, 500, 50, 5, true);

    sys.tick(0.05f);

    // Read all pools -- each should reflect its own state independently
    const auto& p0 = sys.get_pool(0);
    const auto& p1 = sys.get_pool(1);
    const auto& p2 = sys.get_pool(2);
    const auto& p3 = sys.get_pool(3);

    // Player 0: should have surplus
    ASSERT(p0.total_generated > 0);
    ASSERT(p0.surplus >= 0);

    // Player 1: may have deficit depending on coverage
    ASSERT(p1.total_generated > 0);

    // Player 2: no generation
    ASSERT_EQ(p2.total_generated, 0u);

    // Player 3: no consumers, so surplus = total_generated
    ASSERT(p3.total_generated > 0);
    ASSERT_EQ(p3.total_consumed, 0u);
    ASSERT(p3.surplus >= 0);
}

// =============================================================================
// Test 6a: EnergyComponent serialization round-trip preserves all fields
// =============================================================================

TEST(energy_component_serialization_round_trip) {
    // Create component with specific values
    EnergyComponent original{};
    original.energy_required = 250;
    original.energy_received = 200;
    original.is_powered = true;
    original.priority = ENERGY_PRIORITY_IMPORTANT;
    original.grid_id = 2;

    // Serialize
    std::vector<uint8_t> buffer;
    serialize_energy_component(original, buffer);

    // Deserialize
    EnergyComponent deserialized{};
    size_t consumed = deserialize_energy_component(buffer.data(), buffer.size(), deserialized);

    ASSERT(consumed > 0);
    ASSERT_EQ(deserialized.energy_required, original.energy_required);
    ASSERT_EQ(deserialized.energy_received, original.energy_received);
    ASSERT_EQ(deserialized.is_powered, original.is_powered);
    ASSERT_EQ(deserialized.priority, original.priority);
    ASSERT_EQ(deserialized.grid_id, original.grid_id);
}

// =============================================================================
// Test 6b: EnergyPoolSyncMessage round-trip preserves pool state
// =============================================================================

TEST(pool_sync_message_round_trip) {
    // Build a pool with known values
    PerPlayerEnergyPool pool{};
    pool.owner = 2;
    pool.state = EnergyPoolState::Deficit;
    pool.total_generated = 1000;
    pool.total_consumed = 3000;
    pool.surplus = -2000;
    pool.nexus_count = 5;
    pool.consumer_count = 25;

    // Create sync message from pool
    EnergyPoolSyncMessage msg = create_pool_sync_message(pool);

    // Serialize
    std::vector<uint8_t> buffer;
    serialize_pool_sync(msg, buffer);

    // Deserialize
    EnergyPoolSyncMessage deserialized{};
    size_t consumed = deserialize_pool_sync(buffer.data(), buffer.size(), deserialized);

    ASSERT(consumed > 0);
    ASSERT_EQ(deserialized.owner, pool.owner);
    ASSERT_EQ(deserialized.state, static_cast<uint8_t>(pool.state));
    ASSERT_EQ(deserialized.total_generated, pool.total_generated);
    ASSERT_EQ(deserialized.total_consumed, pool.total_consumed);
    ASSERT_EQ(deserialized.surplus, pool.surplus);
}

// =============================================================================
// Test 6c: Power states bit-pack serialization round-trip
// =============================================================================

TEST(power_states_serialization_round_trip) {
    // Simulate a set of consumer power states
    bool states[12] = { true, false, true, true, false, true,
                        false, false, true, false, true, true };

    std::vector<uint8_t> buffer;
    serialize_power_states(states, 12, buffer);

    bool restored[12] = {};
    size_t consumed = deserialize_power_states(buffer.data(), buffer.size(), restored, 12);

    ASSERT(consumed > 0);
    for (int i = 0; i < 12; ++i) {
        ASSERT_EQ(restored[i], states[i]);
    }
}

// =============================================================================
// Test 6d: Serialization of EnergyComponent from live registry entity
// =============================================================================

TEST(serialization_from_live_entity) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    // Create a consumer, run distribution to set is_powered
    create_nexus(reg, sys, 0, 500, true);
    uint32_t cid = create_consumer_with_priority(reg, sys, 0, 1, 1, 100, ENERGY_PRIORITY_CRITICAL);

    sys.update_all_nexus_outputs(0);
    sys.calculate_pool(0);
    sys.distribute_energy(0);

    // Read component from entity
    auto entity = static_cast<entt::entity>(cid);
    const EnergyComponent& live_comp = reg.get<EnergyComponent>(entity);

    // Serialize the live component
    std::vector<uint8_t> buffer;
    serialize_energy_component(live_comp, buffer);

    // Deserialize into fresh component
    EnergyComponent deserialized{};
    deserialize_energy_component(buffer.data(), buffer.size(), deserialized);

    // All fields must match the live entity
    ASSERT_EQ(deserialized.energy_required, live_comp.energy_required);
    ASSERT_EQ(deserialized.energy_received, live_comp.energy_received);
    ASSERT_EQ(deserialized.is_powered, live_comp.is_powered);
    ASSERT_EQ(deserialized.priority, live_comp.priority);
    ASSERT_EQ(deserialized.grid_id, live_comp.grid_id);
}

// =============================================================================
// Test: Multiple ticks produce deterministic results
// =============================================================================

TEST(multiple_ticks_deterministic) {
    auto run_n_ticks = [](uint32_t n, int32_t& final_surplus,
                          EnergyPoolState& final_state,
                          uint32_t& final_gen) {
        entt::registry reg;
        EnergySystem sys(64, 64);
        sys.set_registry(&reg);

        create_nexus_at(reg, sys, 0, 300, 10, 10, true);
        create_consumer_no_coverage(reg, sys, 0, 12, 10, 150, ENERGY_PRIORITY_CRITICAL);
        create_consumer_no_coverage(reg, sys, 0, 13, 10, 100, ENERGY_PRIORITY_NORMAL);

        for (uint32_t i = 0; i < n; ++i) {
            sys.tick(0.05f);
        }

        final_surplus = sys.get_pool(0).surplus;
        final_state = sys.get_pool_state(0);
        final_gen = sys.get_pool(0).total_generated;
    };

    int32_t surplus_a, surplus_b;
    EnergyPoolState state_a, state_b;
    uint32_t gen_a, gen_b;

    // Run 10 ticks on two independent instances
    run_n_ticks(10, surplus_a, state_a, gen_a);
    run_n_ticks(10, surplus_b, state_b, gen_b);

    ASSERT_EQ(surplus_a, surplus_b);
    ASSERT_EQ(static_cast<uint8_t>(state_a), static_cast<uint8_t>(state_b));
    ASSERT_EQ(gen_a, gen_b);
}

// =============================================================================
// Test: Rationing with entity ID tie-breaking is deterministic
// =============================================================================

TEST(rationing_tiebreak_deterministic) {
    // Two runs with consumers at same priority -- entity_id ordering must match
    auto run_tiebreak = [](bool& first_powered, bool& second_powered) {
        entt::registry reg;
        EnergySystem sys(64, 64);
        sys.set_registry(&reg);

        create_nexus(reg, sys, 0, 150, true);

        // Both consumers have NORMAL priority, each needs 100
        uint32_t c1 = create_consumer_with_priority(reg, sys, 0, 1, 1, 100, ENERGY_PRIORITY_NORMAL);
        uint32_t c2 = create_consumer_with_priority(reg, sys, 0, 2, 2, 100, ENERGY_PRIORITY_NORMAL);

        sys.update_all_nexus_outputs(0);
        sys.calculate_pool(0);
        sys.distribute_energy(0);

        first_powered = reg.get<EnergyComponent>(static_cast<entt::entity>(c1)).is_powered;
        second_powered = reg.get<EnergyComponent>(static_cast<entt::entity>(c2)).is_powered;
    };

    bool a_first, a_second;
    bool b_first, b_second;

    run_tiebreak(a_first, a_second);
    run_tiebreak(b_first, b_second);

    ASSERT_EQ(a_first, b_first);
    ASSERT_EQ(a_second, b_second);

    // Lower entity_id should be powered (deterministic tie-break)
    ASSERT(a_first);
    ASSERT(!a_second);
}

// =============================================================================
// Test: Pool state calculation is deterministic (static method)
// =============================================================================

TEST(pool_state_calculation_deterministic) {
    // Same pool values must always yield same state
    PerPlayerEnergyPool pool{};
    pool.total_generated = 1000;
    pool.total_consumed = 800;
    pool.surplus = 200;

    EnergyPoolState state1 = EnergySystem::calculate_pool_state(pool);
    EnergyPoolState state2 = EnergySystem::calculate_pool_state(pool);

    ASSERT_EQ(static_cast<uint8_t>(state1), static_cast<uint8_t>(state2));

    // With large surplus relative to generation, should be Healthy
    ASSERT_EQ(static_cast<uint8_t>(state1), static_cast<uint8_t>(EnergyPoolState::Healthy));

    // Now test Deficit state
    PerPlayerEnergyPool deficit_pool{};
    deficit_pool.total_generated = 100;
    deficit_pool.total_consumed = 200;
    deficit_pool.surplus = -100;

    EnergyPoolState ds1 = EnergySystem::calculate_pool_state(deficit_pool);
    EnergyPoolState ds2 = EnergySystem::calculate_pool_state(deficit_pool);

    ASSERT_EQ(static_cast<uint8_t>(ds1), static_cast<uint8_t>(ds2));
}

// =============================================================================
// Test: Coverage grid after clear and rebuild is deterministic
// =============================================================================

TEST(coverage_clear_rebuild_deterministic) {
    entt::registry reg;
    EnergySystem sys(32, 32);
    sys.set_registry(&reg);

    // Place nexus and conduits
    create_nexus_at(reg, sys, 0, 500, 16, 16, true);
    for (uint32_t x = 17; x <= 20; ++x) {
        auto entity = reg.create();
        uint32_t eid = static_cast<uint32_t>(entity);
        EnergyConduitComponent conduit{};
        conduit.coverage_radius = 3;
        reg.emplace<EnergyConduitComponent>(entity, conduit);
        sys.register_conduit_position(eid, 0, x, 16);
    }

    // First calculation
    sys.recalculate_coverage(0);
    uint32_t count_first = sys.get_coverage_count(1);

    // Clear and rebuild
    sys.get_coverage_grid_mut().clear_all_for_owner(1);
    ASSERT_EQ(sys.get_coverage_count(1), 0u);

    sys.recalculate_coverage(0);
    uint32_t count_second = sys.get_coverage_count(1);

    ASSERT_EQ(count_first, count_second);
    ASSERT(count_first > 0);
}

// =============================================================================
// Test: Nexus aging is deterministic (same ticks_since_built -> same age_factor)
// =============================================================================

TEST(nexus_aging_deterministic) {
    EnergyProducerComponent comp_a{};
    comp_a.base_output = 500;
    comp_a.efficiency = 1.0f;
    comp_a.age_factor = 1.0f;
    comp_a.ticks_since_built = 0;
    comp_a.nexus_type = static_cast<uint8_t>(NexusType::Carbon);
    comp_a.is_online = true;

    EnergyProducerComponent comp_b = comp_a; // identical copy

    // Age both 100 ticks
    for (int i = 0; i < 100; ++i) {
        EnergySystem::update_nexus_aging(comp_a);
        EnergySystem::update_nexus_aging(comp_b);
    }

    ASSERT_EQ(comp_a.ticks_since_built, comp_b.ticks_since_built);
    ASSERT_EQ(comp_a.age_factor, comp_b.age_factor);

    // Age factor should have decreased from 1.0
    ASSERT(comp_a.age_factor < 1.0f);
    ASSERT(comp_a.age_factor > 0.0f);
}

// =============================================================================
// Main Entry Point
// =============================================================================

int main() {
    printf("=== Multiplayer Sync Verification Tests (Ticket 5-042) ===\n\n");

    // Determinism: rationing
    RUN_TEST(rationing_order_deterministic);
    RUN_TEST(rationing_tiebreak_deterministic);

    // Determinism: twin system tick
    RUN_TEST(twin_systems_identical_tick_results);

    // Determinism: pool state
    RUN_TEST(pool_state_transitions_deterministic);
    RUN_TEST(pool_state_calculation_deterministic);

    // Determinism: coverage
    RUN_TEST(coverage_reconstruction_deterministic);
    RUN_TEST(coverage_clear_rebuild_deterministic);

    // Determinism: nexus aging
    RUN_TEST(nexus_aging_deterministic);

    // Determinism: multiple ticks
    RUN_TEST(multiple_ticks_deterministic);

    // Cross-player visibility
    RUN_TEST(cross_player_pool_visibility);
    RUN_TEST(all_players_pool_consistent_after_tick);

    // Serialization round-trips
    RUN_TEST(energy_component_serialization_round_trip);
    RUN_TEST(pool_sync_message_round_trip);
    RUN_TEST(power_states_serialization_round_trip);
    RUN_TEST(serialization_from_live_entity);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
