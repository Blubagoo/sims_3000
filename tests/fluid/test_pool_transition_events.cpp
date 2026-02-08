/**
 * @file test_pool_transition_events.cpp
 * @brief Unit tests for fluid pool state transition event emission (Ticket 6-022)
 *
 * Tests cover:
 * - Deficit began event emitted on transition to Deficit
 * - Deficit ended event emitted on recovery
 * - Collapse began event emitted
 * - Collapse ended event emitted
 * - No events when state doesn't change
 * - Events cleared at start of each tick
 * - Event field values are correct
 * - Multiple players emit independent events
 * - Accumulation across detect calls
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
#include <sims3000/fluid/FluidEvents.h>
#include <sims3000/fluid/PerPlayerFluidPool.h>
#include <entt/entt.hpp>
#include <cstdio>
#include <cstdlib>

using namespace sims3000::fluid;

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
// Helper: create an extractor entity with given current_output
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
// Helper: create a consumer entity near the extractor
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
// Deficit began event emitted on transition to Deficit
// =============================================================================

TEST(deficit_began_on_healthy_to_deficit) {
    FluidSystem sys(64, 64);
    entt::registry reg;
    sys.set_registry(&reg);

    // First tick: Healthy (extractor only, no consumers)
    sys.place_extractor(10, 10, 0);
    sys.tick(0.016f);
    ASSERT_EQ(static_cast<uint8_t>(sys.get_pool_state(0)),
              static_cast<uint8_t>(FluidPoolState::Healthy));

    // Second tick: Add heavy consumer to push into deficit
    FluidExtractorConfig config = get_default_extractor_config();
    create_consumer_near_extractor(reg, sys, 0, config.base_output + 500, 10, 11);
    sys.tick(0.016f);

    // Should have emitted FluidDeficitBeganEvent
    const auto& events = sys.get_deficit_began_events();
    ASSERT(events.size() > 0u);
    ASSERT_EQ(events[0].owner_id, 0u);
}

TEST(deficit_began_on_marginal_to_deficit) {
    FluidSystem sys(64, 64);
    entt::registry reg;
    sys.set_registry(&reg);

    // First tick: Marginal (consumer leaves < 10% surplus)
    sys.place_extractor(10, 10, 0);
    FluidExtractorConfig config = get_default_extractor_config();
    uint32_t marginal_demand = config.base_output - (config.base_output / 20);
    uint32_t consumer_eid = create_consumer_near_extractor(reg, sys, 0, marginal_demand, 10, 11);
    sys.tick(0.016f);
    ASSERT_EQ(static_cast<uint8_t>(sys.get_pool_state(0)),
              static_cast<uint8_t>(FluidPoolState::Marginal));

    // Second tick: Increase demand to push into deficit
    auto consumer_entity = static_cast<entt::entity>(consumer_eid);
    auto* fc = reg.try_get<FluidComponent>(consumer_entity);
    fc->fluid_required = config.base_output + 500;
    sys.tick(0.016f);

    const auto& events = sys.get_deficit_began_events();
    ASSERT(events.size() > 0u);
    ASSERT_EQ(events[0].owner_id, 0u);
}

// =============================================================================
// Deficit ended event emitted on recovery
// =============================================================================

TEST(deficit_ended_on_recovery) {
    FluidSystem sys(64, 64);
    entt::registry reg;
    sys.set_registry(&reg);

    // First tick: Push into deficit
    sys.place_extractor(10, 10, 0);
    FluidExtractorConfig config = get_default_extractor_config();
    uint32_t consumer_eid = create_consumer_near_extractor(reg, sys, 0,
        config.base_output + 500, 10, 11);
    sys.tick(0.016f);

    FluidPoolState after_first = sys.get_pool_state(0);
    ASSERT(after_first == FluidPoolState::Deficit ||
           after_first == FluidPoolState::Collapse);

    // Second tick: Reduce demand to recover
    auto consumer_entity = static_cast<entt::entity>(consumer_eid);
    auto* fc = reg.try_get<FluidComponent>(consumer_entity);
    fc->fluid_required = 10;
    sys.tick(0.016f);

    FluidPoolState after_second = sys.get_pool_state(0);
    if (after_second == FluidPoolState::Healthy ||
        after_second == FluidPoolState::Marginal) {
        const auto& events = sys.get_deficit_ended_events();
        ASSERT(events.size() > 0u);
        ASSERT_EQ(events[0].owner_id, 0u);
    }
}

// =============================================================================
// Collapse began event emitted
// =============================================================================

TEST(collapse_began_on_transition) {
    FluidSystem sys(64, 64);
    entt::registry reg;
    sys.set_registry(&reg);

    // First tick: Healthy
    sys.place_extractor(10, 10, 0);
    sys.tick(0.016f);
    ASSERT_EQ(static_cast<uint8_t>(sys.get_pool_state(0)),
              static_cast<uint8_t>(FluidPoolState::Healthy));

    // Second tick: Push into collapse (high demand, no reservoir)
    FluidExtractorConfig config = get_default_extractor_config();
    create_consumer_near_extractor(reg, sys, 0, config.base_output * 10, 10, 11);
    sys.tick(0.016f);

    ASSERT_EQ(static_cast<uint8_t>(sys.get_pool_state(0)),
              static_cast<uint8_t>(FluidPoolState::Collapse));

    const auto& events = sys.get_collapse_began_events();
    ASSERT(events.size() > 0u);
    ASSERT_EQ(events[0].owner_id, 0u);
}

// =============================================================================
// Collapse ended event emitted
// =============================================================================

TEST(collapse_ended_on_recovery) {
    FluidSystem sys(64, 64);
    entt::registry reg;
    sys.set_registry(&reg);

    // First tick: Healthy
    sys.place_extractor(10, 10, 0);
    sys.tick(0.016f);

    // Second tick: Push into collapse
    FluidExtractorConfig config = get_default_extractor_config();
    uint32_t consumer_eid = create_consumer_near_extractor(reg, sys, 0,
        config.base_output * 10, 10, 11);
    sys.tick(0.016f);
    ASSERT_EQ(static_cast<uint8_t>(sys.get_pool_state(0)),
              static_cast<uint8_t>(FluidPoolState::Collapse));

    // Third tick: Recover by reducing demand
    auto consumer_entity = static_cast<entt::entity>(consumer_eid);
    auto* fc = reg.try_get<FluidComponent>(consumer_entity);
    fc->fluid_required = 10;
    sys.tick(0.016f);

    const auto& events = sys.get_collapse_ended_events();
    ASSERT(events.size() > 0u);
    ASSERT_EQ(events[0].owner_id, 0u);
}

// =============================================================================
// No events when state doesn't change
// =============================================================================

TEST(no_events_on_healthy_to_healthy) {
    FluidSystem sys(64, 64);
    entt::registry reg;
    sys.set_registry(&reg);

    // First tick: Healthy
    sys.place_extractor(10, 10, 0);
    sys.tick(0.016f);
    ASSERT_EQ(static_cast<uint8_t>(sys.get_pool_state(0)),
              static_cast<uint8_t>(FluidPoolState::Healthy));

    // Second tick: Still Healthy (no consumers added)
    sys.tick(0.016f);
    ASSERT_EQ(static_cast<uint8_t>(sys.get_pool_state(0)),
              static_cast<uint8_t>(FluidPoolState::Healthy));

    // No transition events should be emitted
    ASSERT_EQ(sys.get_deficit_began_events().size(), 0u);
    ASSERT_EQ(sys.get_deficit_ended_events().size(), 0u);
    ASSERT_EQ(sys.get_collapse_began_events().size(), 0u);
    ASSERT_EQ(sys.get_collapse_ended_events().size(), 0u);
}

TEST(no_events_on_collapse_to_collapse) {
    FluidSystem sys(64, 64);
    entt::registry reg;
    sys.set_registry(&reg);

    // First tick: Healthy
    sys.place_extractor(10, 10, 0);
    sys.tick(0.016f);

    // Second tick: Collapse
    FluidExtractorConfig config = get_default_extractor_config();
    create_consumer_near_extractor(reg, sys, 0, config.base_output * 10, 10, 11);
    sys.tick(0.016f);
    ASSERT_EQ(static_cast<uint8_t>(sys.get_pool_state(0)),
              static_cast<uint8_t>(FluidPoolState::Collapse));

    // Third tick: Still Collapse (same heavy demand)
    sys.tick(0.016f);
    ASSERT_EQ(static_cast<uint8_t>(sys.get_pool_state(0)),
              static_cast<uint8_t>(FluidPoolState::Collapse));

    // On tick 3: no transition events (Collapse->Collapse is same state)
    ASSERT_EQ(sys.get_deficit_began_events().size(), 0u);
    ASSERT_EQ(sys.get_deficit_ended_events().size(), 0u);
    ASSERT_EQ(sys.get_collapse_began_events().size(), 0u);
    ASSERT_EQ(sys.get_collapse_ended_events().size(), 0u);
}

// =============================================================================
// Events cleared at start of each tick
// =============================================================================

TEST(events_cleared_at_start_of_tick) {
    FluidSystem sys(64, 64);
    entt::registry reg;
    sys.set_registry(&reg);

    // First tick: Healthy
    sys.place_extractor(10, 10, 0);
    sys.tick(0.016f);

    // Second tick: Push to collapse (generates collapse began + deficit began)
    FluidExtractorConfig config = get_default_extractor_config();
    create_consumer_near_extractor(reg, sys, 0, config.base_output * 10, 10, 11);
    sys.tick(0.016f);

    ASSERT(sys.get_collapse_began_events().size() > 0u);
    ASSERT(sys.get_deficit_began_events().size() > 0u);

    // Third tick: Still in collapse - events from tick 2 should be cleared
    sys.tick(0.016f);

    // Previous tick's deficit_began and collapse_began should be gone
    ASSERT_EQ(sys.get_deficit_began_events().size(), 0u);
    ASSERT_EQ(sys.get_collapse_began_events().size(), 0u);
}

// =============================================================================
// Direct unit-level detect_pool_state_transitions test
// =============================================================================

TEST(detect_transitions_deficit_began_direct) {
    FluidSystem sys(64, 64);

    // Manually set pool states (Fluid system doesn't have get_pool_mut,
    // but detect_pool_state_transitions is called within tick)
    // Use tick-based approach to test transitions

    entt::registry reg;
    sys.set_registry(&reg);

    // Tick 1: Healthy
    sys.place_extractor(10, 10, 0);
    sys.tick(0.016f);
    ASSERT_EQ(static_cast<uint8_t>(sys.get_pool_state(0)),
              static_cast<uint8_t>(FluidPoolState::Healthy));

    // Tick 2: Deficit (with reservoir so it stays Deficit not Collapse)
    FluidExtractorConfig config = get_default_extractor_config();

    auto res_entity = reg.create();
    uint32_t res_eid = static_cast<uint32_t>(res_entity);
    FluidReservoirComponent res{};
    res.capacity = 1000;
    res.current_level = 500;
    res.fill_rate = 50;
    res.drain_rate = 100;
    res.is_active = true;
    reg.emplace<FluidReservoirComponent>(res_entity, res);

    FluidProducerComponent prod{};
    prod.base_output = 0;
    prod.current_output = 0;
    prod.is_operational = false;
    prod.producer_type = static_cast<uint8_t>(FluidProducerType::Reservoir);
    reg.emplace<FluidProducerComponent>(res_entity, prod);

    sys.register_reservoir(res_eid, 0);
    sys.register_reservoir_position(res_eid, 0, 12, 12);

    // Consumer with very high demand
    create_consumer_near_extractor(reg, sys, 0, config.base_output + 1000, 10, 11);

    sys.tick(0.016f);

    const auto& events = sys.get_deficit_began_events();
    ASSERT(events.size() > 0u);
    ASSERT_EQ(events[0].owner_id, 0u);
    ASSERT(events[0].deficit_amount < 0);
    ASSERT(events[0].affected_consumers > 0u);
}

// =============================================================================
// Multiple players emit independent events
// =============================================================================

TEST(multiple_players_independent_events) {
    FluidSystem sys(64, 64);
    entt::registry reg;
    sys.set_registry(&reg);

    // Both players start Healthy
    sys.place_extractor(10, 10, 0);
    sys.place_extractor(40, 40, 1);
    sys.tick(0.016f);

    // Player 0: Push into collapse; Player 1: stays Healthy
    FluidExtractorConfig config = get_default_extractor_config();
    create_consumer_near_extractor(reg, sys, 0, config.base_output * 10, 10, 11);
    sys.tick(0.016f);

    // Player 0 should have deficit/collapse began
    const auto& deficit_events = sys.get_deficit_began_events();
    bool player0_deficit = false;
    for (const auto& evt : deficit_events) {
        if (evt.owner_id == 0) {
            player0_deficit = true;
        }
    }
    ASSERT(player0_deficit);

    // Player 1 should NOT have any transition events
    bool player1_deficit = false;
    for (const auto& evt : deficit_events) {
        if (evt.owner_id == 1) {
            player1_deficit = true;
        }
    }
    ASSERT(!player1_deficit);
}

// =============================================================================
// Event field validation: deficit_amount and affected_consumers
// =============================================================================

TEST(deficit_began_event_has_correct_fields) {
    FluidSystem sys(64, 64);
    entt::registry reg;
    sys.set_registry(&reg);

    // Tick 1: Healthy
    sys.place_extractor(10, 10, 0);
    sys.tick(0.016f);

    // Tick 2: Deficit with known consumer count
    FluidExtractorConfig config = get_default_extractor_config();
    create_consumer_near_extractor(reg, sys, 0, config.base_output + 500, 10, 11);
    create_consumer_near_extractor(reg, sys, 0, config.base_output + 500, 11, 10);
    sys.tick(0.016f);

    const auto& events = sys.get_deficit_began_events();
    ASSERT(events.size() > 0u);
    ASSERT_EQ(events[0].owner_id, 0u);
    // deficit_amount should be negative (surplus < 0)
    ASSERT(events[0].deficit_amount < 0);
    // affected_consumers should be the count of consumers in coverage
    ASSERT(events[0].affected_consumers > 0u);
}

TEST(collapse_began_event_has_correct_fields) {
    FluidSystem sys(64, 64);
    entt::registry reg;
    sys.set_registry(&reg);

    // Tick 1: Healthy
    sys.place_extractor(10, 10, 0);
    sys.tick(0.016f);

    // Tick 2: Collapse (no reservoir)
    FluidExtractorConfig config = get_default_extractor_config();
    create_consumer_near_extractor(reg, sys, 0, config.base_output * 10, 10, 11);
    sys.tick(0.016f);

    const auto& events = sys.get_collapse_began_events();
    ASSERT(events.size() > 0u);
    ASSERT_EQ(events[0].owner_id, 0u);
    ASSERT(events[0].deficit_amount < 0);
}

TEST(collapse_ended_event_has_correct_owner) {
    FluidSystem sys(64, 64);
    entt::registry reg;
    sys.set_registry(&reg);

    // Tick 1: Healthy
    sys.place_extractor(10, 10, 0);
    sys.tick(0.016f);

    // Tick 2: Collapse
    FluidExtractorConfig config = get_default_extractor_config();
    uint32_t consumer_eid = create_consumer_near_extractor(reg, sys, 0,
        config.base_output * 10, 10, 11);
    sys.tick(0.016f);
    ASSERT_EQ(static_cast<uint8_t>(sys.get_pool_state(0)),
              static_cast<uint8_t>(FluidPoolState::Collapse));

    // Tick 3: Recover
    auto consumer_entity = static_cast<entt::entity>(consumer_eid);
    auto* fc = reg.try_get<FluidComponent>(consumer_entity);
    fc->fluid_required = 10;
    sys.tick(0.016f);

    const auto& events = sys.get_collapse_ended_events();
    ASSERT(events.size() > 0u);
    ASSERT_EQ(events[0].owner_id, 0u);
}

// =============================================================================
// Invalid owner does not crash
// =============================================================================

TEST(invalid_owner_no_crash) {
    FluidSystem sys(64, 64);
    entt::registry reg;
    sys.set_registry(&reg);

    // distribute_fluid and emit_state_change_events with invalid owner
    // should not crash
    sys.distribute_fluid(MAX_PLAYERS);
    sys.distribute_fluid(255);
    sys.emit_state_change_events(MAX_PLAYERS);
    sys.emit_state_change_events(255);

    ASSERT_EQ(sys.get_deficit_began_events().size(), 0u);
    ASSERT_EQ(sys.get_deficit_ended_events().size(), 0u);
    ASSERT_EQ(sys.get_collapse_began_events().size(), 0u);
    ASSERT_EQ(sys.get_collapse_ended_events().size(), 0u);
}

// =============================================================================
// clear_transition_events clears all buffers
// =============================================================================

TEST(clear_transition_events_clears_all_buffers) {
    FluidSystem sys(64, 64);
    entt::registry reg;
    sys.set_registry(&reg);

    // Tick 1: Healthy
    sys.place_extractor(10, 10, 0);
    sys.tick(0.016f);

    // Tick 2: Push to collapse (generates events)
    FluidExtractorConfig config = get_default_extractor_config();
    create_consumer_near_extractor(reg, sys, 0, config.base_output * 10, 10, 11);
    sys.tick(0.016f);

    // Should have events
    ASSERT(sys.get_deficit_began_events().size() > 0u ||
           sys.get_collapse_began_events().size() > 0u);

    // Clear events
    sys.clear_transition_events();

    // All buffers should be empty
    ASSERT_EQ(sys.get_deficit_began_events().size(), 0u);
    ASSERT_EQ(sys.get_deficit_ended_events().size(), 0u);
    ASSERT_EQ(sys.get_collapse_began_events().size(), 0u);
    ASSERT_EQ(sys.get_collapse_ended_events().size(), 0u);
    ASSERT_EQ(sys.get_state_changed_events().size(), 0u);
}

// =============================================================================
// Main Entry Point
// =============================================================================

int main() {
    printf("=== Fluid Pool Transition Events Unit Tests (Ticket 6-022) ===\n\n");

    // Deficit began events
    RUN_TEST(deficit_began_on_healthy_to_deficit);
    RUN_TEST(deficit_began_on_marginal_to_deficit);

    // Deficit ended events
    RUN_TEST(deficit_ended_on_recovery);

    // Collapse began events
    RUN_TEST(collapse_began_on_transition);

    // Collapse ended events
    RUN_TEST(collapse_ended_on_recovery);

    // No events on same state
    RUN_TEST(no_events_on_healthy_to_healthy);
    RUN_TEST(no_events_on_collapse_to_collapse);

    // Events cleared at start of tick
    RUN_TEST(events_cleared_at_start_of_tick);

    // Direct transition tests
    RUN_TEST(detect_transitions_deficit_began_direct);

    // Multiple players
    RUN_TEST(multiple_players_independent_events);

    // Event field validation
    RUN_TEST(deficit_began_event_has_correct_fields);
    RUN_TEST(collapse_began_event_has_correct_fields);
    RUN_TEST(collapse_ended_event_has_correct_owner);

    // Invalid owner
    RUN_TEST(invalid_owner_no_crash);

    // Clear events
    RUN_TEST(clear_transition_events_clears_all_buffers);

    printf("\n=== Results: %d passed, %d failed ===\n",
           tests_passed, tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
