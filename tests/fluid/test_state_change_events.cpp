/**
 * @file test_state_change_events.cpp
 * @brief Unit tests for FluidStateChangedEvent emission (Ticket 6-021)
 *
 * Tests cover:
 * - Consumer gains fluid: FluidStateChangedEvent emitted with had_fluid=false, has_fluid=true
 * - Consumer loses fluid: event emitted with had_fluid=true, has_fluid=false
 * - No change: no event emitted
 * - Events cleared at start of each tick
 * - Multiple consumers changing state simultaneously
 * - Different players have independent events
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
// Helper: create an extractor entity with given current_output, register it
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
// Helper: create a consumer entity near the extractor for BFS coverage
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
// Consumer gains fluid: FluidStateChangedEvent emitted
// =============================================================================

TEST(consumer_gains_fluid_emits_event) {
    FluidSystem sys(64, 64);
    entt::registry reg;
    sys.set_registry(&reg);

    // Place extractor generating 100
    sys.place_extractor(10, 10, 0);

    // Consumer near extractor with small demand
    uint32_t c1 = create_consumer_near_extractor(reg, sys, 0, 10, 10, 11);

    // First tick: consumer starts with has_fluid=false, should gain fluid
    sys.tick(0.016f);

    const auto& events = sys.get_state_changed_events();
    ASSERT_EQ(events.size(), 1u);
    ASSERT_EQ(events[0].entity_id, c1);
    ASSERT_EQ(events[0].owner_id, 0u);
    ASSERT_EQ(events[0].had_fluid, false);
    ASSERT_EQ(events[0].has_fluid, true);
}

// =============================================================================
// Consumer loses fluid: event emitted with had_fluid=true, has_fluid=false
// =============================================================================

TEST(consumer_loses_fluid_emits_event) {
    FluidSystem sys(64, 64);
    entt::registry reg;
    sys.set_registry(&reg);

    // Place extractor generating 100
    sys.place_extractor(10, 10, 0);

    // Consumer near extractor with small demand
    uint32_t c1 = create_consumer_near_extractor(reg, sys, 0, 10, 10, 11);

    // First tick: consumer gains fluid (false -> true)
    sys.tick(0.016f);

    const auto& events_t1 = sys.get_state_changed_events();
    ASSERT_EQ(events_t1.size(), 1u);
    ASSERT_EQ(events_t1[0].has_fluid, true);

    // Now add a massive consumer to create deficit
    // (total demand >> supply, no reservoirs)
    create_consumer_near_extractor(reg, sys, 0, 5000, 11, 10);

    // Second tick: c1 should lose fluid (true -> false)
    sys.tick(0.016f);

    const auto& events_t2 = sys.get_state_changed_events();
    // Both consumers change state: c1 (true->false), c2 was already false in snapshot
    // but c2 was just created and never had fluid, so it stays false->false = no event
    // c1: true->false = 1 event

    bool found_c1_loss = false;
    for (const auto& evt : events_t2) {
        if (evt.entity_id == c1) {
            ASSERT_EQ(evt.had_fluid, true);
            ASSERT_EQ(evt.has_fluid, false);
            ASSERT_EQ(evt.owner_id, 0u);
            found_c1_loss = true;
        }
    }
    ASSERT(found_c1_loss);
}

// =============================================================================
// No change: no event emitted
// =============================================================================

TEST(no_change_no_event) {
    FluidSystem sys(64, 64);
    entt::registry reg;
    sys.set_registry(&reg);

    // Place extractor generating 100
    sys.place_extractor(10, 10, 0);

    // Consumer near extractor with small demand
    create_consumer_near_extractor(reg, sys, 0, 10, 10, 11);

    // First tick: consumer gains fluid (false -> true) = 1 event
    sys.tick(0.016f);
    ASSERT_EQ(sys.get_state_changed_events().size(), 1u);

    // Second tick: consumer still has fluid (true -> true) = 0 events
    sys.tick(0.016f);
    ASSERT_EQ(sys.get_state_changed_events().size(), 0u);

    // Third tick: still no change
    sys.tick(0.016f);
    ASSERT_EQ(sys.get_state_changed_events().size(), 0u);
}

// =============================================================================
// Events cleared at start of each tick
// =============================================================================

TEST(events_cleared_each_tick) {
    FluidSystem sys(64, 64);
    entt::registry reg;
    sys.set_registry(&reg);

    // Place extractor generating 100
    sys.place_extractor(10, 10, 0);

    // Consumer near extractor
    create_consumer_near_extractor(reg, sys, 0, 10, 10, 11);

    // First tick: consumer gains fluid -> 1 event
    sys.tick(0.016f);
    ASSERT_EQ(sys.get_state_changed_events().size(), 1u);

    // Second tick: no state change -> events should be cleared to 0
    sys.tick(0.016f);
    ASSERT_EQ(sys.get_state_changed_events().size(), 0u);
}

// =============================================================================
// Multiple consumers changing state simultaneously
// =============================================================================

TEST(multiple_consumers_change_simultaneously) {
    FluidSystem sys(64, 64);
    entt::registry reg;
    sys.set_registry(&reg);

    // Place extractor generating 100
    sys.place_extractor(10, 10, 0);

    // Create 3 consumers near extractor, total demand = 30 (within surplus)
    uint32_t c1 = create_consumer_near_extractor(reg, sys, 0, 10, 10, 11);
    uint32_t c2 = create_consumer_near_extractor(reg, sys, 0, 10, 11, 10);
    uint32_t c3 = create_consumer_near_extractor(reg, sys, 0, 10, 11, 11);

    // First tick: all 3 consumers gain fluid (false -> true) = 3 events
    sys.tick(0.016f);
    const auto& events = sys.get_state_changed_events();
    ASSERT_EQ(events.size(), 3u);

    // All events should show had_fluid=false, has_fluid=true
    for (const auto& evt : events) {
        ASSERT_EQ(evt.had_fluid, false);
        ASSERT_EQ(evt.has_fluid, true);
        ASSERT_EQ(evt.owner_id, 0u);
    }

    // Verify all 3 entity IDs are present in the events
    bool found_c1 = false, found_c2 = false, found_c3 = false;
    for (const auto& evt : events) {
        if (evt.entity_id == c1) found_c1 = true;
        if (evt.entity_id == c2) found_c2 = true;
        if (evt.entity_id == c3) found_c3 = true;
    }
    ASSERT(found_c1);
    ASSERT(found_c2);
    ASSERT(found_c3);
}

// =============================================================================
// Multiple consumers lose fluid simultaneously
// =============================================================================

TEST(multiple_consumers_lose_fluid_simultaneously) {
    FluidSystem sys(64, 64);
    entt::registry reg;
    sys.set_registry(&reg);

    // Place extractor generating 100
    sys.place_extractor(10, 10, 0);

    // Create 3 consumers near extractor, total demand = 30 (within surplus)
    uint32_t c1 = create_consumer_near_extractor(reg, sys, 0, 10, 10, 11);
    uint32_t c2 = create_consumer_near_extractor(reg, sys, 0, 10, 11, 10);
    uint32_t c3 = create_consumer_near_extractor(reg, sys, 0, 10, 11, 11);

    // First tick: all gain fluid
    sys.tick(0.016f);
    ASSERT_EQ(sys.get_state_changed_events().size(), 3u);

    // Now add a massive consumer to create deficit
    create_consumer_near_extractor(reg, sys, 0, 5000, 12, 10);

    // Second tick: c1, c2, c3 all lose fluid (true -> false)
    // c4 was false in snapshot and stays false (no event for c4)
    sys.tick(0.016f);
    const auto& events = sys.get_state_changed_events();

    // At least c1, c2, c3 should have events (true -> false)
    uint32_t loss_count = 0;
    bool found_c1 = false, found_c2 = false, found_c3 = false;
    for (const auto& evt : events) {
        if (evt.had_fluid == true && evt.has_fluid == false) {
            ++loss_count;
            if (evt.entity_id == c1) found_c1 = true;
            if (evt.entity_id == c2) found_c2 = true;
            if (evt.entity_id == c3) found_c3 = true;
        }
    }
    ASSERT(loss_count >= 3u);
    ASSERT(found_c1);
    ASSERT(found_c2);
    ASSERT(found_c3);
}

// =============================================================================
// Different players have independent events
// =============================================================================

TEST(different_players_independent_events) {
    FluidSystem sys(64, 64);
    entt::registry reg;
    sys.set_registry(&reg);

    // Player 0: extractor at (10,10), consumer nearby
    sys.place_extractor(10, 10, 0);
    uint32_t c0 = create_consumer_near_extractor(reg, sys, 0, 10, 10, 11);

    // Player 1: extractor at (40,40), consumer nearby
    sys.place_extractor(40, 40, 1);
    uint32_t c1 = create_consumer_near_extractor(reg, sys, 1, 10, 40, 41);

    // First tick: both consumers gain fluid
    sys.tick(0.016f);

    const auto& events = sys.get_state_changed_events();
    ASSERT_EQ(events.size(), 2u);

    // Find events for each player
    bool found_p0 = false, found_p1 = false;
    for (const auto& evt : events) {
        if (evt.entity_id == c0 && evt.owner_id == 0) {
            ASSERT_EQ(evt.had_fluid, false);
            ASSERT_EQ(evt.has_fluid, true);
            found_p0 = true;
        }
        if (evt.entity_id == c1 && evt.owner_id == 1) {
            ASSERT_EQ(evt.had_fluid, false);
            ASSERT_EQ(evt.has_fluid, true);
            found_p1 = true;
        }
    }
    ASSERT(found_p0);
    ASSERT(found_p1);
}

// =============================================================================
// Player 0 loses fluid but player 1 keeps it (independent events)
// =============================================================================

TEST(one_player_loses_other_keeps) {
    FluidSystem sys(64, 64);
    entt::registry reg;
    sys.set_registry(&reg);

    // Player 0: extractor at (10,10), consumer nearby
    sys.place_extractor(10, 10, 0);
    uint32_t c0 = create_consumer_near_extractor(reg, sys, 0, 10, 10, 11);

    // Player 1: extractor at (40,40), consumer nearby
    sys.place_extractor(40, 40, 1);
    uint32_t c1 = create_consumer_near_extractor(reg, sys, 1, 10, 40, 41);

    // First tick: both gain fluid
    sys.tick(0.016f);
    ASSERT_EQ(sys.get_state_changed_events().size(), 2u);

    // Now add massive consumer only for player 0 to create deficit
    create_consumer_near_extractor(reg, sys, 0, 5000, 11, 10);

    // Second tick: player 0 consumer c0 should lose fluid, player 1 unaffected
    sys.tick(0.016f);

    const auto& events = sys.get_state_changed_events();

    // c0 should have an event (true -> false)
    bool found_c0_loss = false;
    for (const auto& evt : events) {
        if (evt.entity_id == c0 && evt.owner_id == 0) {
            ASSERT_EQ(evt.had_fluid, true);
            ASSERT_EQ(evt.has_fluid, false);
            found_c0_loss = true;
        }
    }
    ASSERT(found_c0_loss);

    // c1 should NOT have any event (still has fluid, no change)
    for (const auto& evt : events) {
        if (evt.entity_id == c1) {
            // c1 should not appear because its state didn't change
            printf("\n  FAILED: unexpected event for player 1 consumer (line %d)\n", __LINE__);
            tests_failed++;
            return;
        }
    }
}

// =============================================================================
// No consumers = no events
// =============================================================================

TEST(no_consumers_no_events) {
    FluidSystem sys(64, 64);
    entt::registry reg;
    sys.set_registry(&reg);

    sys.place_extractor(10, 10, 0);

    sys.tick(0.016f);
    ASSERT_EQ(sys.get_state_changed_events().size(), 0u);
}

// =============================================================================
// Consumer outside coverage never generates events
// =============================================================================

TEST(consumer_outside_coverage_no_events) {
    FluidSystem sys(64, 64);
    entt::registry reg;
    sys.set_registry(&reg);

    // Place extractor at (10,10)
    sys.place_extractor(10, 10, 0);

    // Consumer far from extractor (outside BFS coverage)
    create_consumer_near_extractor(reg, sys, 0, 10, 50, 50);

    // First tick: consumer starts false, stays false (outside coverage)
    sys.tick(0.016f);
    ASSERT_EQ(sys.get_state_changed_events().size(), 0u);

    // Second tick: still false
    sys.tick(0.016f);
    ASSERT_EQ(sys.get_state_changed_events().size(), 0u);
}

// =============================================================================
// Event fields are correctly populated
// =============================================================================

TEST(event_fields_correct) {
    FluidSystem sys(64, 64);
    entt::registry reg;
    sys.set_registry(&reg);

    sys.place_extractor(10, 10, 0);
    uint32_t c1 = create_consumer_near_extractor(reg, sys, 0, 10, 10, 11);

    sys.tick(0.016f);

    const auto& events = sys.get_state_changed_events();
    ASSERT_EQ(events.size(), 1u);

    const FluidStateChangedEvent& evt = events[0];
    ASSERT_EQ(evt.entity_id, c1);
    ASSERT_EQ(evt.owner_id, 0u);
    ASSERT_EQ(evt.had_fluid, false);
    ASSERT_EQ(evt.has_fluid, true);
}

// =============================================================================
// Main Entry Point
// =============================================================================

int main() {
    printf("=== Fluid State Change Events Unit Tests (Ticket 6-021) ===\n\n");

    // Consumer gains fluid
    RUN_TEST(consumer_gains_fluid_emits_event);

    // Consumer loses fluid
    RUN_TEST(consumer_loses_fluid_emits_event);

    // No change = no event
    RUN_TEST(no_change_no_event);

    // Events cleared each tick
    RUN_TEST(events_cleared_each_tick);

    // Multiple consumers changing simultaneously
    RUN_TEST(multiple_consumers_change_simultaneously);
    RUN_TEST(multiple_consumers_lose_fluid_simultaneously);

    // Different players have independent events
    RUN_TEST(different_players_independent_events);
    RUN_TEST(one_player_loses_other_keeps);

    // Edge cases
    RUN_TEST(no_consumers_no_events);
    RUN_TEST(consumer_outside_coverage_no_events);
    RUN_TEST(event_fields_correct);

    printf("\n=== Results: %d passed, %d failed ===\n",
           tests_passed, tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
