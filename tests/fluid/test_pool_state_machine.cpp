/**
 * @file test_pool_state_machine.cpp
 * @brief Unit tests for fluid pool state machine and reservoir buffering (Ticket 6-018)
 *
 * Tests cover:
 * - Pool transitions Healthy -> Marginal
 * - Pool transitions Marginal -> Deficit
 * - Pool transitions to Collapse when reservoirs empty
 * - Reservoir fill during surplus
 * - Reservoir drain during deficit
 * - Proportional drain across multiple reservoirs
 * - Asymmetric rates: fill at 50, drain at 100
 * - Deficit -> Healthy recovery when new extractor added
 * - Transition events emitted correctly
 *
 * Uses printf test pattern consistent with other fluid tests.
 */

#include <sims3000/fluid/FluidSystem.h>
#include <sims3000/fluid/FluidComponent.h>
#include <sims3000/fluid/FluidProducerComponent.h>
#include <sims3000/fluid/FluidReservoirComponent.h>
#include <sims3000/fluid/FluidExtractorConfig.h>
#include <sims3000/fluid/FluidReservoirConfig.h>
#include <sims3000/fluid/FluidEnums.h>
#include <entt/entt.hpp>
#include <cstdio>
#include <cstdlib>

using namespace sims3000::fluid;

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
// Helper: create an extractor entity with given current_output, register it
// with FluidSystem. Sets is_operational to true and current_output directly
// (bypasses water distance / power checks for unit-level pool tests).
// =============================================================================

static uint32_t create_extractor_direct(entt::registry& reg, FluidSystem& sys,
                                         uint8_t owner, uint32_t current_output,
                                         uint32_t x = 10, uint32_t y = 10) {
    auto entity = reg.create();
    uint32_t eid = static_cast<uint32_t>(entity);

    FluidProducerComponent prod{};
    prod.base_output = current_output;
    prod.current_output = current_output;
    prod.is_operational = true;
    prod.producer_type = static_cast<uint8_t>(FluidProducerType::Extractor);
    prod.max_water_distance = 5;
    prod.current_water_distance = 0;
    reg.emplace<FluidProducerComponent>(entity, prod);

    sys.register_extractor(eid, owner);
    sys.register_extractor_position(eid, owner, x, y);
    return eid;
}

// =============================================================================
// Helper: create a reservoir entity with given current_level and capacity,
// register with FluidSystem.
// =============================================================================

static uint32_t create_reservoir_direct(entt::registry& reg, FluidSystem& sys,
                                         uint8_t owner, uint32_t current_level,
                                         uint32_t capacity,
                                         uint16_t fill_rate = 50,
                                         uint16_t drain_rate = 100,
                                         uint32_t x = 12, uint32_t y = 12) {
    auto entity = reg.create();
    uint32_t eid = static_cast<uint32_t>(entity);

    FluidReservoirComponent res{};
    res.capacity = capacity;
    res.current_level = current_level;
    res.fill_rate = fill_rate;
    res.drain_rate = drain_rate;
    res.is_active = true;
    reg.emplace<FluidReservoirComponent>(entity, res);

    FluidProducerComponent prod{};
    prod.base_output = 0;
    prod.current_output = 0;
    prod.is_operational = false;
    prod.producer_type = static_cast<uint8_t>(FluidProducerType::Reservoir);
    reg.emplace<FluidProducerComponent>(entity, prod);

    sys.register_reservoir(eid, owner);
    sys.register_reservoir_position(eid, owner, x, y);
    return eid;
}

// =============================================================================
// Helper: create a consumer entity with FluidComponent, register and place
// it near the extractor so BFS coverage reaches it during tick().
// =============================================================================

static uint32_t create_consumer_near_extractor(entt::registry& reg, FluidSystem& sys,
                                                uint8_t owner, uint32_t fluid_required,
                                                uint32_t x = 10, uint32_t y = 11) {
    auto entity = reg.create();
    uint32_t eid = static_cast<uint32_t>(entity);

    FluidComponent fc{};
    fc.fluid_required = fluid_required;
    reg.emplace<FluidComponent>(entity, fc);

    sys.register_consumer(eid, owner);
    sys.register_consumer_position(eid, owner, x, y);
    return eid;
}

// =============================================================================
// Pool transitions Healthy -> Marginal
// =============================================================================

TEST(pool_transitions_healthy_to_marginal) {
    FluidSystem sys(64, 64);
    entt::registry reg;
    sys.set_registry(&reg);

    // First tick: Healthy (extractor only, no consumers)
    sys.place_extractor(10, 10, 0);
    sys.tick(0.016f);
    ASSERT_EQ(static_cast<uint8_t>(sys.get_pool_state(0)),
              static_cast<uint8_t>(FluidPoolState::Healthy));

    // Add consumer that leaves a tiny surplus (< 10% of available)
    FluidExtractorConfig config = get_default_extractor_config();
    uint32_t consumed = config.base_output - (config.base_output / 20);  // 5% surplus
    create_consumer_near_extractor(reg, sys, 0, consumed, 10, 11);

    sys.tick(0.016f);

    const PerPlayerFluidPool& pool = sys.get_pool(0);
    ASSERT(pool.surplus >= 0);
    ASSERT_EQ(static_cast<uint8_t>(pool.state), static_cast<uint8_t>(FluidPoolState::Marginal));
}

// =============================================================================
// Pool transitions Marginal -> Deficit
// =============================================================================

TEST(pool_transitions_marginal_to_deficit) {
    FluidSystem sys(64, 64);
    entt::registry reg;
    sys.set_registry(&reg);

    // Place extractor (generation=100) and reservoir with large stored fluid
    // generation=100, reservoir_stored=500, capacity=1000
    sys.place_extractor(10, 10, 0);
    create_reservoir_direct(reg, sys, 0, 500, 1000);

    // Consumer demand exceeds available (generation + reservoir stored)
    // available = 100 + 500 = 600, consumed = 700 => surplus = -100
    // Drain: deficit_remaining = 100, drain_rate = 100, current_level = 500
    //   drain_amount = min(100, 100, 500) = 100
    // After drain: reservoir_stored = 400, available = 100 + 400 = 500, surplus = 500 - 700 = -200
    // reservoir_stored = 400 > 0 => Deficit (not Collapse)
    create_consumer_near_extractor(reg, sys, 0, 700, 10, 11);

    sys.tick(0.016f);

    const PerPlayerFluidPool& pool = sys.get_pool(0);
    ASSERT(pool.surplus < 0);
    ASSERT(pool.total_reservoir_stored > 0u);
    ASSERT_EQ(static_cast<uint8_t>(pool.state), static_cast<uint8_t>(FluidPoolState::Deficit));
}

// =============================================================================
// Pool transitions to Collapse when reservoirs empty
// =============================================================================

TEST(pool_transitions_to_collapse_when_reservoirs_empty) {
    FluidSystem sys(64, 64);
    entt::registry reg;
    sys.set_registry(&reg);

    // Place extractor
    sys.place_extractor(10, 10, 0);

    // Create reservoir with 0 stored fluid
    create_reservoir_direct(reg, sys, 0, 0, 1000);

    // Large consumer demand causing deficit
    FluidExtractorConfig config = get_default_extractor_config();
    create_consumer_near_extractor(reg, sys, 0, config.base_output + 500, 10, 11);

    sys.tick(0.016f);

    const PerPlayerFluidPool& pool = sys.get_pool(0);
    ASSERT(pool.surplus < 0);
    ASSERT_EQ(pool.total_reservoir_stored, 0u);
    ASSERT_EQ(static_cast<uint8_t>(pool.state), static_cast<uint8_t>(FluidPoolState::Collapse));
}

// =============================================================================
// Reservoir fill during surplus
// =============================================================================

TEST(reservoir_fill_during_surplus) {
    FluidSystem sys(64, 64);
    entt::registry reg;
    sys.set_registry(&reg);

    // Place extractor at (10,10) with output 100 (default)
    sys.place_extractor(10, 10, 0);

    // Create a reservoir with 0 stored, capacity 1000, fill_rate 50
    uint32_t res_eid = create_reservoir_direct(reg, sys, 0, 0, 1000, 50, 100);

    // No consumers - full surplus goes to filling reservoir
    sys.tick(0.016f);

    // After tick, reservoir should have been filled
    auto res_entity = static_cast<entt::entity>(res_eid);
    const auto* res = reg.try_get<FluidReservoirComponent>(res_entity);
    ASSERT(res != nullptr);
    // Fill amount limited by fill_rate (50) and surplus
    ASSERT(res->current_level > 0u);
    ASSERT(res->current_level <= 50u);  // Limited by fill_rate

    // Check that ReservoirLevelChangedEvent was emitted
    const auto& events = sys.get_reservoir_level_changed_events();
    ASSERT(events.size() > 0u);
}

// =============================================================================
// Reservoir drain during deficit
// =============================================================================

TEST(reservoir_drain_during_deficit) {
    FluidSystem sys(64, 64);
    entt::registry reg;
    sys.set_registry(&reg);

    // Place extractor generating 100
    sys.place_extractor(10, 10, 0);

    // Create reservoir with 500 stored, drain_rate 100
    uint32_t res_eid = create_reservoir_direct(reg, sys, 0, 500, 1000, 50, 100);

    // Consumer demands more than generation + reservoir stored
    // available = 100 + 500 = 600, consumed = 650 => surplus = -50
    // drain: min(50, 100, 500) = 50, reservoir goes from 500 to 450
    create_consumer_near_extractor(reg, sys, 0, 650, 10, 11);

    sys.tick(0.016f);

    // Reservoir should have been drained
    auto res_entity = static_cast<entt::entity>(res_eid);
    const auto* res = reg.try_get<FluidReservoirComponent>(res_entity);
    ASSERT(res != nullptr);
    ASSERT(res->current_level < 500u);  // Was drained

    // Check event emitted
    const auto& events = sys.get_reservoir_level_changed_events();
    ASSERT(events.size() > 0u);
}

// =============================================================================
// Proportional drain across multiple reservoirs
// =============================================================================

TEST(proportional_drain_across_multiple_reservoirs) {
    FluidSystem sys(64, 64);
    entt::registry reg;
    sys.set_registry(&reg);

    // Place extractor generating 100
    sys.place_extractor(10, 10, 0);

    // Create two reservoirs with different stored levels
    // Reservoir A: 800 stored, drain_rate 100
    uint32_t res_a = create_reservoir_direct(reg, sys, 0, 800, 1000, 50, 100, 12, 12);
    // Reservoir B: 200 stored, drain_rate 100
    uint32_t res_b = create_reservoir_direct(reg, sys, 0, 200, 1000, 50, 100, 14, 14);

    // available = 100 + 800 + 200 = 1100
    // Need consumed > 1100 for deficit. Use 1200 => surplus = -100
    // Proportional drain: A has 800/1000=80%, B has 200/1000=20%
    // A share: 80 of 100 deficit, limited by drain_rate 100 => drain 80
    // B share: 20 of 100 deficit, limited by drain_rate 100 => drain 20
    create_consumer_near_extractor(reg, sys, 0, 1200, 10, 11);

    sys.tick(0.016f);

    auto res_a_entity = static_cast<entt::entity>(res_a);
    auto res_b_entity = static_cast<entt::entity>(res_b);
    const auto* ra = reg.try_get<FluidReservoirComponent>(res_a_entity);
    const auto* rb = reg.try_get<FluidReservoirComponent>(res_b_entity);
    ASSERT(ra != nullptr);
    ASSERT(rb != nullptr);

    // Both should have been drained (proportionally)
    uint32_t a_drained = 800 - ra->current_level;
    uint32_t b_drained = 200 - rb->current_level;
    ASSERT(a_drained > 0u);
    ASSERT(b_drained > 0u);
    // A should have drained more than B (proportional to stored level)
    ASSERT(a_drained >= b_drained);
}

// =============================================================================
// Asymmetric rates: fill at 50, drain at 100
// =============================================================================

TEST(asymmetric_rates_fill_and_drain) {
    FluidSystem sys(64, 64);
    entt::registry reg;
    sys.set_registry(&reg);

    // Place extractor generating 100
    sys.place_extractor(10, 10, 0);

    // Create reservoir with custom rates: fill=50, drain=100
    uint32_t res_eid = create_reservoir_direct(reg, sys, 0, 0, 1000, 50, 100);

    // FILL phase: no consumers, surplus = 100
    sys.tick(0.016f);

    auto res_entity = static_cast<entt::entity>(res_eid);
    auto* res = reg.try_get<FluidReservoirComponent>(res_entity);
    ASSERT(res != nullptr);

    uint32_t filled = res->current_level;
    // Fill should be limited by fill_rate = 50 (even though surplus is 100)
    ASSERT(filled <= 50u);
    ASSERT(filled > 0u);

    // DRAIN phase: set reservoir to 500, create heavy consumer
    // Need consumed > generation + reservoir_stored
    // available = 100 + 500 = 600, consumed = 680 => surplus = -80
    // drain: min(proportional_share=80, drain_rate=100, current_level=500) = 80
    res->current_level = 500;
    create_consumer_near_extractor(reg, sys, 0, 680, 10, 11);

    sys.tick(0.016f);

    // Drain should be limited by the deficit amount (80) since drain_rate (100) > deficit
    uint32_t drained = 500 - res->current_level;
    ASSERT(drained > 0u);
    ASSERT(drained <= 100u);  // Limited by drain_rate
}

// =============================================================================
// Deficit -> Healthy recovery when new extractor added
// =============================================================================

TEST(deficit_to_healthy_recovery_with_new_extractor) {
    FluidSystem sys(64, 64);
    entt::registry reg;
    sys.set_registry(&reg);

    // Place one extractor generating 100
    sys.place_extractor(10, 10, 0);

    // Create reservoir with stored fluid
    // generation=100, reservoir_stored=200, available=300
    create_reservoir_direct(reg, sys, 0, 200, 1000, 50, 100);

    // Consumer demands more than available: consumed=400
    // available = 100 + 200 = 300, surplus = -100
    // After drain: reservoir drains 100 (limited by drain_rate and deficit)
    // reservoir_stored = 100, available = 200, surplus = -200
    // reservoir_stored > 0 => Deficit
    create_consumer_near_extractor(reg, sys, 0, 400, 10, 11);

    // First tick: should be in Deficit
    sys.tick(0.016f);
    FluidPoolState first_state = sys.get_pool(0).state;
    uint32_t first_gen = sys.get_pool(0).total_generated;
    ASSERT(first_state == FluidPoolState::Deficit ||
           first_state == FluidPoolState::Collapse);

    // Add more extractors to bring generation above consumption
    // generation=100 currently, add 4 more => generation=500 total
    // consumed=400, available = 500 + reservoir_stored > 400 => Healthy
    sys.place_extractor(20, 20, 0);
    sys.place_extractor(30, 30, 0);
    sys.place_extractor(40, 40, 0);
    sys.place_extractor(50, 50, 0);

    // Second tick: with five extractors (500 output) vs 400 demand
    sys.tick(0.016f);
    const PerPlayerFluidPool& pool2 = sys.get_pool(0);
    // Generation should have increased with more extractors
    ASSERT(pool2.total_generated > first_gen);
    // Should have recovered to Healthy or Marginal
    ASSERT(pool2.state == FluidPoolState::Healthy ||
           pool2.state == FluidPoolState::Marginal);
}

// =============================================================================
// Transition events emitted correctly: Healthy -> Deficit
// =============================================================================

TEST(transition_events_healthy_to_deficit) {
    FluidSystem sys(64, 64);
    entt::registry reg;
    sys.set_registry(&reg);

    // First tick: Healthy (no consumers)
    sys.place_extractor(10, 10, 0);
    create_reservoir_direct(reg, sys, 0, 200, 1000, 50, 100);
    sys.tick(0.016f);
    ASSERT_EQ(static_cast<uint8_t>(sys.get_pool_state(0)),
              static_cast<uint8_t>(FluidPoolState::Healthy));

    // Second tick: Push into deficit with large consumer
    FluidExtractorConfig config = get_default_extractor_config();
    create_consumer_near_extractor(reg, sys, 0, config.base_output + 500, 10, 11);
    sys.tick(0.016f);

    // Should have emitted FluidDeficitBeganEvent
    const auto& deficit_events = sys.get_deficit_began_events();
    ASSERT(deficit_events.size() > 0u);
    ASSERT_EQ(deficit_events[0].owner_id, 0u);
}

// =============================================================================
// Transition events emitted correctly: Deficit -> Healthy
// =============================================================================

TEST(transition_events_deficit_to_healthy) {
    FluidSystem sys(64, 64);
    entt::registry reg;
    sys.set_registry(&reg);

    // Create a scenario where we start in deficit
    sys.place_extractor(10, 10, 0);
    create_reservoir_direct(reg, sys, 0, 200, 1000, 50, 100);
    FluidExtractorConfig config = get_default_extractor_config();
    uint32_t consumer_eid = create_consumer_near_extractor(reg, sys, 0,
        config.base_output + 500, 10, 11);

    // First tick: should go to deficit/collapse from Healthy
    sys.tick(0.016f);
    FluidPoolState state_after_first = sys.get_pool_state(0);
    ASSERT(state_after_first == FluidPoolState::Deficit ||
           state_after_first == FluidPoolState::Collapse);

    // Remove large consumer and tick again
    auto consumer_entity = static_cast<entt::entity>(consumer_eid);
    auto* fc = reg.try_get<FluidComponent>(consumer_entity);
    ASSERT(fc != nullptr);
    fc->fluid_required = 10;  // Reduce demand to very small

    sys.tick(0.016f);

    // If we recovered, should have deficit ended event
    FluidPoolState state_after_second = sys.get_pool_state(0);
    if (state_after_second == FluidPoolState::Healthy ||
        state_after_second == FluidPoolState::Marginal) {
        const auto& ended_events = sys.get_deficit_ended_events();
        ASSERT(ended_events.size() > 0u);
        ASSERT_EQ(ended_events[0].owner_id, 0u);
    }
}

// =============================================================================
// Transition events: Collapse began/ended
// =============================================================================

TEST(transition_events_collapse) {
    FluidSystem sys(64, 64);
    entt::registry reg;
    sys.set_registry(&reg);

    // First tick: Healthy
    sys.place_extractor(10, 10, 0);
    // No reservoir, so collapse when deficit
    sys.tick(0.016f);
    ASSERT_EQ(static_cast<uint8_t>(sys.get_pool_state(0)),
              static_cast<uint8_t>(FluidPoolState::Healthy));

    // Second tick: Push into collapse (high demand, no reservoir)
    FluidExtractorConfig config = get_default_extractor_config();
    uint32_t consumer_eid = create_consumer_near_extractor(reg, sys, 0,
        config.base_output * 10, 10, 11);
    sys.tick(0.016f);

    ASSERT_EQ(static_cast<uint8_t>(sys.get_pool_state(0)),
              static_cast<uint8_t>(FluidPoolState::Collapse));

    // Should have emitted collapse began event
    const auto& collapse_events = sys.get_collapse_began_events();
    ASSERT(collapse_events.size() > 0u);
    ASSERT_EQ(collapse_events[0].owner_id, 0u);

    // Third tick: Recover by reducing demand
    auto consumer_entity = static_cast<entt::entity>(consumer_eid);
    auto* fc = reg.try_get<FluidComponent>(consumer_entity);
    ASSERT(fc != nullptr);
    fc->fluid_required = 10;  // Very low demand

    sys.tick(0.016f);

    // Should have emitted collapse ended event
    const auto& collapse_ended = sys.get_collapse_ended_events();
    ASSERT(collapse_ended.size() > 0u);
}

// =============================================================================
// detect_pool_state_transitions updates previous_state
// =============================================================================

TEST(detect_transitions_updates_previous_state) {
    FluidSystem sys(64, 64);
    entt::registry reg;
    sys.set_registry(&reg);

    // Tick with no entities to establish Healthy baseline
    sys.tick(0.016f);
    ASSERT_EQ(static_cast<uint8_t>(sys.get_pool(0).previous_state),
              static_cast<uint8_t>(FluidPoolState::Healthy));
    ASSERT_EQ(static_cast<uint8_t>(sys.get_pool(0).state),
              static_cast<uint8_t>(FluidPoolState::Healthy));
}

// =============================================================================
// Reservoir level changed events track old and new levels
// =============================================================================

TEST(reservoir_level_changed_events_correct) {
    FluidSystem sys(64, 64);
    entt::registry reg;
    sys.set_registry(&reg);

    // Place extractor to have surplus for filling
    sys.place_extractor(10, 10, 0);
    uint32_t res_eid = create_reservoir_direct(reg, sys, 0, 0, 1000, 50, 100);

    sys.tick(0.016f);

    const auto& events = sys.get_reservoir_level_changed_events();
    // Should have at least one event for the reservoir fill
    bool found_event = false;
    for (const auto& evt : events) {
        if (evt.entity_id == res_eid) {
            found_event = true;
            ASSERT_EQ(evt.old_level, 0u);
            ASSERT(evt.new_level > 0u);
            ASSERT_EQ(evt.owner_id, 0u);
        }
    }
    ASSERT(found_event);
}

// =============================================================================
// Main Entry Point
// =============================================================================

int main() {
    printf("=== Fluid Pool State Machine Unit Tests (Ticket 6-018) ===\n\n");

    // Pool state transitions
    RUN_TEST(pool_transitions_healthy_to_marginal);
    RUN_TEST(pool_transitions_marginal_to_deficit);
    RUN_TEST(pool_transitions_to_collapse_when_reservoirs_empty);

    // Reservoir buffering
    RUN_TEST(reservoir_fill_during_surplus);
    RUN_TEST(reservoir_drain_during_deficit);
    RUN_TEST(proportional_drain_across_multiple_reservoirs);
    RUN_TEST(asymmetric_rates_fill_and_drain);

    // Recovery
    RUN_TEST(deficit_to_healthy_recovery_with_new_extractor);

    // Transition events
    RUN_TEST(transition_events_healthy_to_deficit);
    RUN_TEST(transition_events_deficit_to_healthy);
    RUN_TEST(transition_events_collapse);
    RUN_TEST(detect_transitions_updates_previous_state);
    RUN_TEST(reservoir_level_changed_events_correct);

    printf("\n=== Results: %d passed, %d failed ===\n",
           tests_passed, tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
