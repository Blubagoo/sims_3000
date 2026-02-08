/**
 * @file test_state_change_events.cpp
 * @brief Unit tests for EnergyStateChangedEvent emission (Ticket 5-020)
 *
 * Tests cover:
 * - emit_state_change_events() detects powered->unpowered transition
 * - emit_state_change_events() detects unpowered->powered transition
 * - No event emitted when state doesn't change
 * - Multiple consumers with mixed transitions
 * - Event buffer is cleared at start of tick
 * - tick() integration: events emitted after distribution
 * - get_state_change_events() returns correct events
 * - Edge cases: no consumers, no registry, invalid owner
 */

#include <sims3000/energy/EnergySystem.h>
#include <sims3000/energy/EnergyComponent.h>
#include <sims3000/energy/EnergyProducerComponent.h>
#include <sims3000/energy/EnergyEnums.h>
#include <sims3000/energy/EnergyEvents.h>
#include <sims3000/energy/EnergyPriorities.h>
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
// Helper: create nexus with position (for tick tests)
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
// Helper: create consumer with manual coverage
// =============================================================================

static uint32_t create_consumer(entt::registry& reg, EnergySystem& sys,
                                 uint8_t owner, uint32_t x, uint32_t y,
                                 uint32_t energy_required) {
    auto entity = reg.create();
    uint32_t eid = static_cast<uint32_t>(entity);

    EnergyComponent ec{};
    ec.energy_required = energy_required;
    reg.emplace<EnergyComponent>(entity, ec);

    sys.register_consumer(eid, owner);
    sys.register_consumer_position(eid, owner, x, y);
    set_coverage(sys, x, y, owner);
    return eid;
}

// =============================================================================
// Helper: create consumer without coverage (for tick tests)
// =============================================================================

static uint32_t create_consumer_no_coverage(entt::registry& reg, EnergySystem& sys,
                                             uint8_t owner, uint32_t x, uint32_t y,
                                             uint32_t energy_required) {
    auto entity = reg.create();
    uint32_t eid = static_cast<uint32_t>(entity);

    EnergyComponent ec{};
    ec.energy_required = energy_required;
    reg.emplace<EnergyComponent>(entity, ec);

    sys.register_consumer(eid, owner);
    sys.register_consumer_position(eid, owner, x, y);
    return eid;
}

// =============================================================================
// Test: Unpowered to powered transition emits event
// =============================================================================

TEST(unpowered_to_powered_emits_event) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    create_nexus(reg, sys, 0, 1000, true);
    uint32_t c1 = create_consumer(reg, sys, 0, 5, 5, 100);

    // Consumer starts unpowered (default)
    auto e1 = static_cast<entt::entity>(c1);
    auto* ec1 = reg.try_get<EnergyComponent>(e1);
    ASSERT(!ec1->is_powered);

    // Snapshot previous state (all unpowered)
    sys.snapshot_power_states(0);

    // Distribute: should power consumer
    sys.update_all_nexus_outputs(0);
    sys.calculate_pool(0);
    sys.distribute_energy(0);

    ASSERT(ec1->is_powered);

    // Emit events - should detect unpowered->powered
    // Clear the events buffer first (mimicking tick behavior)
    // We access the mutable vector indirectly via emit_state_change_events
    sys.emit_state_change_events(0);

    const auto& events = sys.get_state_change_events();
    ASSERT_EQ(events.size(), 1u);
    ASSERT_EQ(events[0].entity_id, c1);
    ASSERT_EQ(events[0].owner_id, 0u);
    ASSERT_EQ(events[0].was_powered, false);
    ASSERT_EQ(events[0].is_powered, true);
}

// =============================================================================
// Test: Powered to unpowered transition emits event
// =============================================================================

TEST(powered_to_unpowered_emits_event) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    create_nexus(reg, sys, 0, 1000, true);
    uint32_t c1 = create_consumer(reg, sys, 0, 5, 5, 100);

    // First: power the consumer
    sys.update_all_nexus_outputs(0);
    sys.calculate_pool(0);
    sys.distribute_energy(0);

    auto e1 = static_cast<entt::entity>(c1);
    auto* ec1 = reg.try_get<EnergyComponent>(e1);
    ASSERT(ec1->is_powered);

    // Snapshot powered state
    sys.snapshot_power_states(0);

    // Create deficit by increasing demand
    ec1->energy_required = 5000;
    sys.calculate_pool(0);
    sys.distribute_energy(0);

    ASSERT(!ec1->is_powered);

    // Emit events
    sys.emit_state_change_events(0);

    const auto& events = sys.get_state_change_events();
    ASSERT_EQ(events.size(), 1u);
    ASSERT_EQ(events[0].entity_id, c1);
    ASSERT_EQ(events[0].owner_id, 0u);
    ASSERT_EQ(events[0].was_powered, true);
    ASSERT_EQ(events[0].is_powered, false);
}

// =============================================================================
// Test: No event when state doesn't change
// =============================================================================

TEST(no_event_when_state_unchanged) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    create_nexus(reg, sys, 0, 1000, true);
    uint32_t c1 = create_consumer(reg, sys, 0, 5, 5, 100);

    // Power the consumer
    sys.update_all_nexus_outputs(0);
    sys.calculate_pool(0);
    sys.distribute_energy(0);

    auto e1 = static_cast<entt::entity>(c1);
    auto* ec1 = reg.try_get<EnergyComponent>(e1);
    ASSERT(ec1->is_powered);

    // Snapshot powered state
    sys.snapshot_power_states(0);

    // Distribute again - still powered
    sys.distribute_energy(0);
    ASSERT(ec1->is_powered);

    // Emit events - should be empty
    sys.emit_state_change_events(0);

    const auto& events = sys.get_state_change_events();
    ASSERT_EQ(events.size(), 0u);
}

// =============================================================================
// Test: Multiple consumers with mixed transitions
// =============================================================================

TEST(mixed_transitions_multiple_consumers) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    create_nexus(reg, sys, 0, 1000, true);

    // c1 starts unpowered, will become powered
    uint32_t c1 = create_consumer(reg, sys, 0, 1, 1, 100);

    // c2 starts powered (we'll set it manually), will stay powered
    uint32_t c2 = create_consumer(reg, sys, 0, 2, 2, 200);
    auto e2 = static_cast<entt::entity>(c2);
    auto* ec2 = reg.try_get<EnergyComponent>(e2);
    ec2->is_powered = true;
    ec2->energy_received = 200;

    // c3 starts powered, will become unpowered (moved outside coverage)
    uint32_t c3 = create_consumer(reg, sys, 0, 3, 3, 300);
    auto e3 = static_cast<entt::entity>(c3);
    auto* ec3 = reg.try_get<EnergyComponent>(e3);
    ec3->is_powered = true;
    ec3->energy_received = 300;

    // Snapshot: c1=unpowered, c2=powered, c3=powered
    sys.snapshot_power_states(0);

    // Remove coverage for c3 to make it lose power
    sys.get_coverage_grid_mut().set(3, 3, 0); // clear coverage

    sys.update_all_nexus_outputs(0);
    sys.calculate_pool(0);
    sys.distribute_energy(0);

    // c1: unpowered->powered (transition)
    // c2: powered->powered (no transition)
    // c3: powered->unpowered (transition, lost coverage)

    sys.emit_state_change_events(0);

    const auto& events = sys.get_state_change_events();
    ASSERT_EQ(events.size(), 2u);

    // Find events by entity_id
    bool found_c1 = false, found_c3 = false;
    for (const auto& evt : events) {
        if (evt.entity_id == c1) {
            found_c1 = true;
            ASSERT_EQ(evt.was_powered, false);
            ASSERT_EQ(evt.is_powered, true);
        }
        if (evt.entity_id == c3) {
            found_c3 = true;
            ASSERT_EQ(evt.was_powered, true);
            ASSERT_EQ(evt.is_powered, false);
        }
    }
    ASSERT(found_c1);
    ASSERT(found_c3);
}

// =============================================================================
// Test: tick() integration - events emitted after distribution
// =============================================================================

TEST(tick_emits_state_change_events) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    // Nexus at (10,10) with base_output 1000
    create_nexus_at(reg, sys, 0, 1000, 10, 10, true);

    // Consumer at (12,10) - within coverage radius 8
    uint32_t c1 = create_consumer_no_coverage(reg, sys, 0, 12, 10, 100);

    // First tick: consumer goes from unpowered (default) to powered
    sys.tick(0.05f);

    auto e1 = static_cast<entt::entity>(c1);
    auto* ec1 = reg.try_get<EnergyComponent>(e1);
    ASSERT(ec1->is_powered);

    const auto& events = sys.get_state_change_events();
    ASSERT_EQ(events.size(), 1u);
    ASSERT_EQ(events[0].entity_id, c1);
    ASSERT_EQ(events[0].was_powered, false);
    ASSERT_EQ(events[0].is_powered, true);
}

// =============================================================================
// Test: tick() clears events between ticks
// =============================================================================

TEST(tick_clears_events_between_ticks) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    create_nexus_at(reg, sys, 0, 1000, 10, 10, true);
    uint32_t c1 = create_consumer_no_coverage(reg, sys, 0, 12, 10, 100);

    // First tick: unpowered -> powered (1 event)
    sys.tick(0.05f);
    ASSERT_EQ(sys.get_state_change_events().size(), 1u);

    // Second tick: powered -> powered (no change, 0 events)
    sys.tick(0.05f);
    ASSERT_EQ(sys.get_state_change_events().size(), 0u);

    (void)c1;
}

// =============================================================================
// Test: tick() powered -> unpowered transition
// =============================================================================

TEST(tick_powered_to_unpowered_event) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    create_nexus_at(reg, sys, 0, 1000, 10, 10, true);
    uint32_t c1 = create_consumer_no_coverage(reg, sys, 0, 12, 10, 100);

    // First tick: power it
    sys.tick(0.05f);

    auto e1 = static_cast<entt::entity>(c1);
    auto* ec1 = reg.try_get<EnergyComponent>(e1);
    ASSERT(ec1->is_powered);

    // Increase demand to cause deficit
    ec1->energy_required = 5000;

    // Second tick: should detect powered -> unpowered
    sys.tick(0.05f);

    ASSERT(!ec1->is_powered);

    const auto& events = sys.get_state_change_events();
    ASSERT_EQ(events.size(), 1u);
    ASSERT_EQ(events[0].entity_id, c1);
    ASSERT_EQ(events[0].was_powered, true);
    ASSERT_EQ(events[0].is_powered, false);
}

// =============================================================================
// Test: No consumers - no crash, no events
// =============================================================================

TEST(no_consumers_no_events) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    sys.snapshot_power_states(0);
    sys.emit_state_change_events(0);

    ASSERT_EQ(sys.get_state_change_events().size(), 0u);
}

// =============================================================================
// Test: No registry - no crash
// =============================================================================

TEST(no_registry_no_crash) {
    EnergySystem sys(64, 64);

    sys.snapshot_power_states(0);
    sys.emit_state_change_events(0);

    ASSERT_EQ(sys.get_state_change_events().size(), 0u);
}

// =============================================================================
// Test: Invalid owner - no crash
// =============================================================================

TEST(invalid_owner_no_crash) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    sys.snapshot_power_states(MAX_PLAYERS);
    sys.emit_state_change_events(MAX_PLAYERS);
    sys.snapshot_power_states(255);
    sys.emit_state_change_events(255);

    ASSERT_EQ(sys.get_state_change_events().size(), 0u);
}

// =============================================================================
// Test: New consumer (not in prev snapshot) emits event if powered
// =============================================================================

TEST(new_consumer_emits_event_if_powered) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    create_nexus(reg, sys, 0, 1000, true);

    // Snapshot with no consumers
    sys.snapshot_power_states(0);

    // Add consumer and power it
    uint32_t c1 = create_consumer(reg, sys, 0, 5, 5, 100);
    sys.update_all_nexus_outputs(0);
    sys.calculate_pool(0);
    sys.distribute_energy(0);

    auto e1 = static_cast<entt::entity>(c1);
    auto* ec1 = reg.try_get<EnergyComponent>(e1);
    ASSERT(ec1->is_powered);

    // Emit events - new consumer defaults to was_powered=false in snapshot
    sys.emit_state_change_events(0);

    const auto& events = sys.get_state_change_events();
    ASSERT_EQ(events.size(), 1u);
    ASSERT_EQ(events[0].entity_id, c1);
    ASSERT_EQ(events[0].was_powered, false);
    ASSERT_EQ(events[0].is_powered, true);
}

// =============================================================================
// Test: Multi-player event isolation
// =============================================================================

TEST(multi_player_event_isolation) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    // Player 0: will transition unpowered -> powered
    create_nexus(reg, sys, 0, 1000, true);
    uint32_t c0 = create_consumer(reg, sys, 0, 1, 1, 100);

    // Player 1: no consumers, no events
    create_nexus(reg, sys, 1, 1000, true);

    // Snapshot both players
    sys.snapshot_power_states(0);
    sys.snapshot_power_states(1);

    // Distribute
    sys.update_all_nexus_outputs(0);
    sys.update_all_nexus_outputs(1);
    sys.calculate_pool(0);
    sys.calculate_pool(1);
    sys.distribute_energy(0);
    sys.distribute_energy(1);

    // Emit events for both
    sys.emit_state_change_events(0);
    sys.emit_state_change_events(1);

    const auto& events = sys.get_state_change_events();
    // Only player 0's consumer should have an event
    ASSERT_EQ(events.size(), 1u);
    ASSERT_EQ(events[0].entity_id, c0);
    ASSERT_EQ(events[0].owner_id, 0u);
}

// =============================================================================
// Main Entry Point
// =============================================================================

int main() {
    printf("=== EnergyStateChangedEvent Unit Tests (Ticket 5-020) ===\n\n");

    // Transition detection
    RUN_TEST(unpowered_to_powered_emits_event);
    RUN_TEST(powered_to_unpowered_emits_event);
    RUN_TEST(no_event_when_state_unchanged);
    RUN_TEST(mixed_transitions_multiple_consumers);

    // tick() integration
    RUN_TEST(tick_emits_state_change_events);
    RUN_TEST(tick_clears_events_between_ticks);
    RUN_TEST(tick_powered_to_unpowered_event);

    // Edge cases
    RUN_TEST(no_consumers_no_events);
    RUN_TEST(no_registry_no_crash);
    RUN_TEST(invalid_owner_no_crash);
    RUN_TEST(new_consumer_emits_event_if_powered);

    // Multi-player
    RUN_TEST(multi_player_event_isolation);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
