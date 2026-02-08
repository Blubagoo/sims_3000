/**
 * @file test_pool_transition_events.cpp
 * @brief Unit tests for pool state transition event emission (Ticket 5-021)
 *
 * Tests cover:
 * - EnergyDeficitBeganEvent emitted on Healthy->Deficit transition
 * - EnergyDeficitBeganEvent emitted on Marginal->Deficit transition
 * - EnergyDeficitBeganEvent emitted on Healthy->Collapse transition
 * - EnergyDeficitEndedEvent emitted on Deficit->Healthy transition
 * - EnergyDeficitEndedEvent emitted on Collapse->Marginal transition
 * - GridCollapseBeganEvent emitted on Healthy->Collapse transition
 * - GridCollapseBeganEvent emitted on Deficit->Collapse transition
 * - GridCollapseEndedEvent emitted on Collapse->Healthy transition
 * - GridCollapseEndedEvent emitted on Collapse->Deficit transition
 * - No events on same-state transition (Healthy->Healthy)
 * - Event field values are correct (owner, deficit_amount, surplus, etc.)
 * - clear_transition_events() clears all buffers
 * - tick() clears events at start of each tick
 * - Multiple players emit independent events
 * - Deficit->Deficit (no change) emits nothing
 * - Collapse->Collapse (no change) emits nothing
 */

#include <sims3000/energy/EnergySystem.h>
#include <sims3000/energy/EnergyComponent.h>
#include <sims3000/energy/EnergyProducerComponent.h>
#include <sims3000/energy/EnergyEnums.h>
#include <sims3000/energy/EnergyEvents.h>
#include <sims3000/energy/PerPlayerEnergyPool.h>
#include <entt/entt.hpp>
#include <cstdio>
#include <cstdlib>

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
// EnergyDeficitBeganEvent emission
// =============================================================================

TEST(deficit_began_on_healthy_to_deficit) {
    EnergySystem sys(64, 64);

    auto& pool = sys.get_pool_mut(0);
    pool.previous_state = EnergyPoolState::Healthy;
    pool.state = EnergyPoolState::Deficit;
    pool.surplus = -100;
    pool.consumer_count = 5;

    sys.detect_pool_state_transitions(0);

    const auto& events = sys.get_deficit_began_events();
    ASSERT_EQ(events.size(), 1u);
    ASSERT_EQ(events[0].owner_id, 0u);
    ASSERT_EQ(events[0].deficit_amount, -100);
    ASSERT_EQ(events[0].affected_consumers, 5u);
}

TEST(deficit_began_on_marginal_to_deficit) {
    EnergySystem sys(64, 64);

    auto& pool = sys.get_pool_mut(0);
    pool.previous_state = EnergyPoolState::Marginal;
    pool.state = EnergyPoolState::Deficit;
    pool.surplus = -50;
    pool.consumer_count = 3;

    sys.detect_pool_state_transitions(0);

    const auto& events = sys.get_deficit_began_events();
    ASSERT_EQ(events.size(), 1u);
    ASSERT_EQ(events[0].owner_id, 0u);
    ASSERT_EQ(events[0].deficit_amount, -50);
    ASSERT_EQ(events[0].affected_consumers, 3u);
}

TEST(deficit_began_on_healthy_to_collapse) {
    EnergySystem sys(64, 64);

    auto& pool = sys.get_pool_mut(0);
    pool.previous_state = EnergyPoolState::Healthy;
    pool.state = EnergyPoolState::Collapse;
    pool.surplus = -900;
    pool.consumer_count = 10;

    sys.detect_pool_state_transitions(0);

    // Healthy->Collapse emits both deficit began AND collapse began
    const auto& deficit_events = sys.get_deficit_began_events();
    ASSERT_EQ(deficit_events.size(), 1u);
    ASSERT_EQ(deficit_events[0].owner_id, 0u);

    const auto& collapse_events = sys.get_collapse_began_events();
    ASSERT_EQ(collapse_events.size(), 1u);
}

TEST(deficit_began_on_marginal_to_collapse) {
    EnergySystem sys(64, 64);

    auto& pool = sys.get_pool_mut(0);
    pool.previous_state = EnergyPoolState::Marginal;
    pool.state = EnergyPoolState::Collapse;
    pool.surplus = -800;
    pool.consumer_count = 8;

    sys.detect_pool_state_transitions(0);

    const auto& deficit_events = sys.get_deficit_began_events();
    ASSERT_EQ(deficit_events.size(), 1u);

    const auto& collapse_events = sys.get_collapse_began_events();
    ASSERT_EQ(collapse_events.size(), 1u);
}

// =============================================================================
// EnergyDeficitEndedEvent emission
// =============================================================================

TEST(deficit_ended_on_deficit_to_healthy) {
    EnergySystem sys(64, 64);

    auto& pool = sys.get_pool_mut(0);
    pool.previous_state = EnergyPoolState::Deficit;
    pool.state = EnergyPoolState::Healthy;
    pool.surplus = 500;

    sys.detect_pool_state_transitions(0);

    const auto& events = sys.get_deficit_ended_events();
    ASSERT_EQ(events.size(), 1u);
    ASSERT_EQ(events[0].owner_id, 0u);
    ASSERT_EQ(events[0].surplus_amount, 500);
}

TEST(deficit_ended_on_deficit_to_marginal) {
    EnergySystem sys(64, 64);

    auto& pool = sys.get_pool_mut(0);
    pool.previous_state = EnergyPoolState::Deficit;
    pool.state = EnergyPoolState::Marginal;
    pool.surplus = 10;

    sys.detect_pool_state_transitions(0);

    const auto& events = sys.get_deficit_ended_events();
    ASSERT_EQ(events.size(), 1u);
    ASSERT_EQ(events[0].owner_id, 0u);
    ASSERT_EQ(events[0].surplus_amount, 10);
}

TEST(deficit_ended_on_collapse_to_healthy) {
    EnergySystem sys(64, 64);

    auto& pool = sys.get_pool_mut(0);
    pool.previous_state = EnergyPoolState::Collapse;
    pool.state = EnergyPoolState::Healthy;
    pool.surplus = 1000;

    sys.detect_pool_state_transitions(0);

    // Collapse->Healthy emits both deficit ended AND collapse ended
    const auto& deficit_events = sys.get_deficit_ended_events();
    ASSERT_EQ(deficit_events.size(), 1u);

    const auto& collapse_events = sys.get_collapse_ended_events();
    ASSERT_EQ(collapse_events.size(), 1u);
}

TEST(deficit_ended_on_collapse_to_marginal) {
    EnergySystem sys(64, 64);

    auto& pool = sys.get_pool_mut(0);
    pool.previous_state = EnergyPoolState::Collapse;
    pool.state = EnergyPoolState::Marginal;
    pool.surplus = 5;

    sys.detect_pool_state_transitions(0);

    const auto& deficit_events = sys.get_deficit_ended_events();
    ASSERT_EQ(deficit_events.size(), 1u);

    const auto& collapse_events = sys.get_collapse_ended_events();
    ASSERT_EQ(collapse_events.size(), 1u);
}

// =============================================================================
// GridCollapseBeganEvent emission
// =============================================================================

TEST(collapse_began_on_deficit_to_collapse) {
    EnergySystem sys(64, 64);

    auto& pool = sys.get_pool_mut(0);
    pool.previous_state = EnergyPoolState::Deficit;
    pool.state = EnergyPoolState::Collapse;
    pool.surplus = -600;

    sys.detect_pool_state_transitions(0);

    const auto& events = sys.get_collapse_began_events();
    ASSERT_EQ(events.size(), 1u);
    ASSERT_EQ(events[0].owner_id, 0u);
    ASSERT_EQ(events[0].deficit_amount, -600);

    // Deficit->Collapse should NOT emit deficit began (already in deficit)
    const auto& deficit_events = sys.get_deficit_began_events();
    ASSERT_EQ(deficit_events.size(), 0u);
}

// =============================================================================
// GridCollapseEndedEvent emission
// =============================================================================

TEST(collapse_ended_on_collapse_to_deficit) {
    EnergySystem sys(64, 64);

    auto& pool = sys.get_pool_mut(0);
    pool.previous_state = EnergyPoolState::Collapse;
    pool.state = EnergyPoolState::Deficit;
    pool.surplus = -50;

    sys.detect_pool_state_transitions(0);

    const auto& events = sys.get_collapse_ended_events();
    ASSERT_EQ(events.size(), 1u);
    ASSERT_EQ(events[0].owner_id, 0u);

    // Collapse->Deficit should NOT emit deficit ended (still in deficit)
    const auto& deficit_events = sys.get_deficit_ended_events();
    ASSERT_EQ(deficit_events.size(), 0u);
}

// =============================================================================
// No events on same-state
// =============================================================================

TEST(no_events_on_healthy_to_healthy) {
    EnergySystem sys(64, 64);

    auto& pool = sys.get_pool_mut(0);
    pool.previous_state = EnergyPoolState::Healthy;
    pool.state = EnergyPoolState::Healthy;
    pool.surplus = 500;

    sys.detect_pool_state_transitions(0);

    ASSERT_EQ(sys.get_deficit_began_events().size(), 0u);
    ASSERT_EQ(sys.get_deficit_ended_events().size(), 0u);
    ASSERT_EQ(sys.get_collapse_began_events().size(), 0u);
    ASSERT_EQ(sys.get_collapse_ended_events().size(), 0u);
}

TEST(no_events_on_deficit_to_deficit) {
    EnergySystem sys(64, 64);

    auto& pool = sys.get_pool_mut(0);
    pool.previous_state = EnergyPoolState::Deficit;
    pool.state = EnergyPoolState::Deficit;
    pool.surplus = -100;

    sys.detect_pool_state_transitions(0);

    ASSERT_EQ(sys.get_deficit_began_events().size(), 0u);
    ASSERT_EQ(sys.get_deficit_ended_events().size(), 0u);
    ASSERT_EQ(sys.get_collapse_began_events().size(), 0u);
    ASSERT_EQ(sys.get_collapse_ended_events().size(), 0u);
}

TEST(no_events_on_collapse_to_collapse) {
    EnergySystem sys(64, 64);

    auto& pool = sys.get_pool_mut(0);
    pool.previous_state = EnergyPoolState::Collapse;
    pool.state = EnergyPoolState::Collapse;
    pool.surplus = -900;

    sys.detect_pool_state_transitions(0);

    ASSERT_EQ(sys.get_deficit_began_events().size(), 0u);
    ASSERT_EQ(sys.get_deficit_ended_events().size(), 0u);
    ASSERT_EQ(sys.get_collapse_began_events().size(), 0u);
    ASSERT_EQ(sys.get_collapse_ended_events().size(), 0u);
}

TEST(no_events_on_marginal_to_marginal) {
    EnergySystem sys(64, 64);

    auto& pool = sys.get_pool_mut(0);
    pool.previous_state = EnergyPoolState::Marginal;
    pool.state = EnergyPoolState::Marginal;
    pool.surplus = 10;

    sys.detect_pool_state_transitions(0);

    ASSERT_EQ(sys.get_deficit_began_events().size(), 0u);
    ASSERT_EQ(sys.get_deficit_ended_events().size(), 0u);
    ASSERT_EQ(sys.get_collapse_began_events().size(), 0u);
    ASSERT_EQ(sys.get_collapse_ended_events().size(), 0u);
}

// =============================================================================
// clear_transition_events()
// =============================================================================

TEST(clear_transition_events_clears_all_buffers) {
    EnergySystem sys(64, 64);

    // Generate events in all four buffers
    auto& pool0 = sys.get_pool_mut(0);
    pool0.previous_state = EnergyPoolState::Healthy;
    pool0.state = EnergyPoolState::Collapse;
    pool0.surplus = -900;
    pool0.consumer_count = 10;
    sys.detect_pool_state_transitions(0);

    auto& pool1 = sys.get_pool_mut(1);
    pool1.previous_state = EnergyPoolState::Collapse;
    pool1.state = EnergyPoolState::Healthy;
    pool1.surplus = 500;
    sys.detect_pool_state_transitions(1);

    // Verify events were emitted
    ASSERT(sys.get_deficit_began_events().size() > 0u);
    ASSERT(sys.get_deficit_ended_events().size() > 0u);
    ASSERT(sys.get_collapse_began_events().size() > 0u);
    ASSERT(sys.get_collapse_ended_events().size() > 0u);

    sys.clear_transition_events();

    ASSERT_EQ(sys.get_deficit_began_events().size(), 0u);
    ASSERT_EQ(sys.get_deficit_ended_events().size(), 0u);
    ASSERT_EQ(sys.get_collapse_began_events().size(), 0u);
    ASSERT_EQ(sys.get_collapse_ended_events().size(), 0u);
}

// =============================================================================
// tick() clears events at start
// =============================================================================

TEST(tick_clears_events_at_start) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    // Manually inject a deficit began event
    auto& pool = sys.get_pool_mut(0);
    pool.previous_state = EnergyPoolState::Healthy;
    pool.state = EnergyPoolState::Deficit;
    pool.surplus = -100;
    pool.consumer_count = 2;
    sys.detect_pool_state_transitions(0);
    ASSERT_EQ(sys.get_deficit_began_events().size(), 1u);

    // Now tick - events should be cleared at the start, then new events
    // may or may not be emitted depending on state
    sys.tick(0.05f);

    // After tick with no nexuses/consumers, pool goes to Healthy (0/0)
    // The previous_state was set to Deficit by detect above, then tick
    // recalculates pool. Since pool was reset, Healthy state means
    // Deficit->Healthy transition => deficit_ended should be emitted
    // But the deficit_began from before tick should be gone (cleared)
    // We can check: if deficit_began is empty, the clear worked
    // (new deficit_began would only be emitted on Healthy->Deficit)
    ASSERT_EQ(sys.get_deficit_began_events().size(), 0u);
}

// =============================================================================
// Multiple players emit independent events
// =============================================================================

TEST(multiple_players_independent_events) {
    EnergySystem sys(64, 64);

    // Player 0: Healthy -> Deficit
    auto& pool0 = sys.get_pool_mut(0);
    pool0.previous_state = EnergyPoolState::Healthy;
    pool0.state = EnergyPoolState::Deficit;
    pool0.surplus = -100;
    pool0.consumer_count = 5;

    // Player 1: Deficit -> Healthy
    auto& pool1 = sys.get_pool_mut(1);
    pool1.previous_state = EnergyPoolState::Deficit;
    pool1.state = EnergyPoolState::Healthy;
    pool1.surplus = 200;

    sys.detect_pool_state_transitions(0);
    sys.detect_pool_state_transitions(1);

    // Player 0 should have deficit began
    const auto& began = sys.get_deficit_began_events();
    ASSERT_EQ(began.size(), 1u);
    ASSERT_EQ(began[0].owner_id, 0u);

    // Player 1 should have deficit ended
    const auto& ended = sys.get_deficit_ended_events();
    ASSERT_EQ(ended.size(), 1u);
    ASSERT_EQ(ended[0].owner_id, 1u);
}

// =============================================================================
// Invalid owner does not crash
// =============================================================================

TEST(invalid_owner_no_crash) {
    EnergySystem sys(64, 64);

    sys.detect_pool_state_transitions(MAX_PLAYERS);
    sys.detect_pool_state_transitions(255);

    ASSERT_EQ(sys.get_deficit_began_events().size(), 0u);
    ASSERT_EQ(sys.get_deficit_ended_events().size(), 0u);
    ASSERT_EQ(sys.get_collapse_began_events().size(), 0u);
    ASSERT_EQ(sys.get_collapse_ended_events().size(), 0u);
}

// =============================================================================
// Event field validation
// =============================================================================

TEST(deficit_began_event_has_correct_fields) {
    EnergySystem sys(64, 64);

    auto& pool = sys.get_pool_mut(2);
    pool.previous_state = EnergyPoolState::Healthy;
    pool.state = EnergyPoolState::Deficit;
    pool.surplus = -250;
    pool.consumer_count = 42;

    sys.detect_pool_state_transitions(2);

    const auto& events = sys.get_deficit_began_events();
    ASSERT_EQ(events.size(), 1u);
    ASSERT_EQ(events[0].owner_id, 2u);
    ASSERT_EQ(events[0].deficit_amount, -250);
    ASSERT_EQ(events[0].affected_consumers, 42u);
}

TEST(collapse_began_event_has_correct_fields) {
    EnergySystem sys(64, 64);

    auto& pool = sys.get_pool_mut(3);
    pool.previous_state = EnergyPoolState::Deficit;
    pool.state = EnergyPoolState::Collapse;
    pool.surplus = -750;

    sys.detect_pool_state_transitions(3);

    const auto& events = sys.get_collapse_began_events();
    ASSERT_EQ(events.size(), 1u);
    ASSERT_EQ(events[0].owner_id, 3u);
    ASSERT_EQ(events[0].deficit_amount, -750);
}

TEST(deficit_ended_event_has_correct_surplus) {
    EnergySystem sys(64, 64);

    auto& pool = sys.get_pool_mut(1);
    pool.previous_state = EnergyPoolState::Deficit;
    pool.state = EnergyPoolState::Marginal;
    pool.surplus = 42;

    sys.detect_pool_state_transitions(1);

    const auto& events = sys.get_deficit_ended_events();
    ASSERT_EQ(events.size(), 1u);
    ASSERT_EQ(events[0].owner_id, 1u);
    ASSERT_EQ(events[0].surplus_amount, 42);
}

TEST(collapse_ended_event_has_correct_owner) {
    EnergySystem sys(64, 64);

    auto& pool = sys.get_pool_mut(2);
    pool.previous_state = EnergyPoolState::Collapse;
    pool.state = EnergyPoolState::Deficit;
    pool.surplus = -30;

    sys.detect_pool_state_transitions(2);

    const auto& events = sys.get_collapse_ended_events();
    ASSERT_EQ(events.size(), 1u);
    ASSERT_EQ(events[0].owner_id, 2u);
}

// =============================================================================
// Accumulation: multiple transitions accumulate in buffer
// =============================================================================

TEST(events_accumulate_across_detect_calls) {
    EnergySystem sys(64, 64);

    // Player 0: Healthy -> Deficit
    auto& pool0 = sys.get_pool_mut(0);
    pool0.previous_state = EnergyPoolState::Healthy;
    pool0.state = EnergyPoolState::Deficit;
    pool0.surplus = -100;
    pool0.consumer_count = 5;
    sys.detect_pool_state_transitions(0);

    // Player 1: Marginal -> Deficit
    auto& pool1 = sys.get_pool_mut(1);
    pool1.previous_state = EnergyPoolState::Marginal;
    pool1.state = EnergyPoolState::Deficit;
    pool1.surplus = -200;
    pool1.consumer_count = 8;
    sys.detect_pool_state_transitions(1);

    // Should have 2 deficit began events
    const auto& events = sys.get_deficit_began_events();
    ASSERT_EQ(events.size(), 2u);
    ASSERT_EQ(events[0].owner_id, 0u);
    ASSERT_EQ(events[1].owner_id, 1u);
}

// =============================================================================
// Main Entry Point
// =============================================================================

int main() {
    printf("=== Pool State Transition Events Unit Tests (Ticket 5-021) ===\n\n");

    // Deficit began events
    RUN_TEST(deficit_began_on_healthy_to_deficit);
    RUN_TEST(deficit_began_on_marginal_to_deficit);
    RUN_TEST(deficit_began_on_healthy_to_collapse);
    RUN_TEST(deficit_began_on_marginal_to_collapse);

    // Deficit ended events
    RUN_TEST(deficit_ended_on_deficit_to_healthy);
    RUN_TEST(deficit_ended_on_deficit_to_marginal);
    RUN_TEST(deficit_ended_on_collapse_to_healthy);
    RUN_TEST(deficit_ended_on_collapse_to_marginal);

    // Collapse began events
    RUN_TEST(collapse_began_on_deficit_to_collapse);

    // Collapse ended events
    RUN_TEST(collapse_ended_on_collapse_to_deficit);

    // No events on same state
    RUN_TEST(no_events_on_healthy_to_healthy);
    RUN_TEST(no_events_on_deficit_to_deficit);
    RUN_TEST(no_events_on_collapse_to_collapse);
    RUN_TEST(no_events_on_marginal_to_marginal);

    // Clear and tick
    RUN_TEST(clear_transition_events_clears_all_buffers);
    RUN_TEST(tick_clears_events_at_start);

    // Multiple players
    RUN_TEST(multiple_players_independent_events);

    // Invalid owner
    RUN_TEST(invalid_owner_no_crash);

    // Event field validation
    RUN_TEST(deficit_began_event_has_correct_fields);
    RUN_TEST(collapse_began_event_has_correct_fields);
    RUN_TEST(deficit_ended_event_has_correct_surplus);
    RUN_TEST(collapse_ended_event_has_correct_owner);

    // Accumulation
    RUN_TEST(events_accumulate_across_detect_calls);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
