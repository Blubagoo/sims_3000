/**
 * @file test_fluid_system.cpp
 * @brief Unit tests for FluidSystem class skeleton (Ticket 6-009)
 *
 * Tests cover:
 * - Construction with various map sizes
 * - set_registry() wiring
 * - Place extractor and verify entity created
 * - Place conduit and verify entity created
 * - Remove conduit and verify entity destroyed
 * - Coverage dirty flag set on placement
 * - Pool query returns default state
 * - has_fluid returns correct default
 * - get_priority() returns 20
 * - Registration and unregistration of extractors, reservoirs, consumers
 * - Position registration
 * - Event emission on placement
 */

#include <sims3000/fluid/FluidSystem.h>
#include <sims3000/fluid/FluidComponent.h>
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
// Construction Tests
// =============================================================================

TEST(construction_128x128) {
    FluidSystem sys(128, 128);
    ASSERT_EQ(sys.get_map_width(), 128u);
    ASSERT_EQ(sys.get_map_height(), 128u);
}

TEST(construction_256x256) {
    FluidSystem sys(256, 256);
    ASSERT_EQ(sys.get_map_width(), 256u);
    ASSERT_EQ(sys.get_map_height(), 256u);
}

TEST(construction_512x512) {
    FluidSystem sys(512, 512);
    ASSERT_EQ(sys.get_map_width(), 512u);
    ASSERT_EQ(sys.get_map_height(), 512u);
}

TEST(construction_with_nullptr_terrain) {
    FluidSystem sys(128, 128, nullptr);
    ASSERT_EQ(sys.get_map_width(), 128u);
    ASSERT_EQ(sys.get_map_height(), 128u);
}

TEST(construction_non_square) {
    FluidSystem sys(64, 32);
    ASSERT_EQ(sys.get_map_width(), 64u);
    ASSERT_EQ(sys.get_map_height(), 32u);
}

// =============================================================================
// Priority Tests
// =============================================================================

TEST(get_priority_returns_20) {
    FluidSystem sys(128, 128);
    ASSERT_EQ(sys.get_priority(), 20);
}

// =============================================================================
// Registry Wiring Tests
// =============================================================================

TEST(set_registry_wiring) {
    FluidSystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    // Verify placement works with registry set
    uint32_t eid = sys.place_extractor(5, 5, 0);
    ASSERT(eid != INVALID_ENTITY_ID);
}

TEST(set_registry_nullptr_prevents_placement) {
    FluidSystem sys(128, 128);
    // No registry set - placement should fail
    uint32_t eid = sys.place_extractor(5, 5, 0);
    ASSERT_EQ(eid, INVALID_ENTITY_ID);
}

// =============================================================================
// Pool Query Tests
// =============================================================================

TEST(pool_query_returns_default_state) {
    FluidSystem sys(128, 128);
    for (uint8_t i = 0; i < MAX_PLAYERS; ++i) {
        const PerPlayerFluidPool& pool = sys.get_pool(i);
        ASSERT_EQ(pool.total_generated, 0u);
        ASSERT_EQ(pool.total_consumed, 0u);
        ASSERT_EQ(pool.surplus, 0);
        ASSERT_EQ(pool.state, FluidPoolState::Healthy);
    }
}

TEST(pool_state_returns_healthy_default) {
    FluidSystem sys(128, 128);
    for (uint8_t i = 0; i < MAX_PLAYERS; ++i) {
        ASSERT_EQ(sys.get_pool_state(i), FluidPoolState::Healthy);
    }
}

TEST(pool_state_out_of_bounds_returns_healthy) {
    FluidSystem sys(128, 128);
    ASSERT_EQ(sys.get_pool_state(MAX_PLAYERS), FluidPoolState::Healthy);
    ASSERT_EQ(sys.get_pool_state(255), FluidPoolState::Healthy);
}

// =============================================================================
// has_fluid Tests
// =============================================================================

TEST(has_fluid_returns_false_no_registry) {
    FluidSystem sys(128, 128);
    ASSERT(!sys.has_fluid(0));
    ASSERT(!sys.has_fluid(42));
}

TEST(has_fluid_returns_false_invalid_entity) {
    FluidSystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);
    ASSERT(!sys.has_fluid(9999));
}

TEST(has_fluid_returns_false_no_component) {
    FluidSystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    auto entity = registry.create();
    uint32_t eid = static_cast<uint32_t>(entity);
    ASSERT(!sys.has_fluid(eid));
}

TEST(has_fluid_returns_component_value) {
    FluidSystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    auto entity = registry.create();
    uint32_t eid = static_cast<uint32_t>(entity);

    FluidComponent fc{};
    fc.has_fluid = false;
    registry.emplace<FluidComponent>(entity, fc);
    ASSERT(!sys.has_fluid(eid));

    // Set has_fluid to true
    auto& comp = registry.get<FluidComponent>(entity);
    comp.has_fluid = true;
    ASSERT(sys.has_fluid(eid));
}

// =============================================================================
// has_fluid_at Tests
// =============================================================================

TEST(has_fluid_at_returns_false_no_coverage) {
    FluidSystem sys(128, 128);
    ASSERT(!sys.has_fluid_at(5, 5, 0));
}

TEST(has_fluid_at_returns_false_invalid_player) {
    FluidSystem sys(128, 128);
    ASSERT(!sys.has_fluid_at(5, 5, MAX_PLAYERS));
    ASSERT(!sys.has_fluid_at(5, 5, 255));
}

// =============================================================================
// Place Extractor Tests
// =============================================================================

TEST(place_extractor_creates_entity) {
    FluidSystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    uint32_t eid = sys.place_extractor(10, 20, 0);
    ASSERT(eid != INVALID_ENTITY_ID);

    // Verify entity exists in registry
    auto entity = static_cast<entt::entity>(eid);
    ASSERT(registry.valid(entity));

    // Verify entity has FluidProducerComponent
    ASSERT(registry.all_of<FluidProducerComponent>(entity));

    // Verify producer type is Extractor
    const auto& prod = registry.get<FluidProducerComponent>(entity);
    ASSERT_EQ(prod.producer_type, static_cast<uint8_t>(FluidProducerType::Extractor));
}

TEST(place_extractor_registers_entity) {
    FluidSystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    ASSERT_EQ(sys.get_extractor_count(0), 0u);
    sys.place_extractor(10, 20, 0);
    ASSERT_EQ(sys.get_extractor_count(0), 1u);
}

TEST(place_extractor_emits_event) {
    FluidSystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    uint32_t eid = sys.place_extractor(10, 20, 0);
    const auto& events = sys.get_extractor_placed_events();
    ASSERT_EQ(events.size(), static_cast<size_t>(1));
    ASSERT_EQ(events[0].entity_id, eid);
    ASSERT_EQ(events[0].owner_id, static_cast<uint8_t>(0));
    ASSERT_EQ(events[0].grid_x, 10u);
    ASSERT_EQ(events[0].grid_y, 20u);
}

TEST(place_extractor_out_of_bounds_fails) {
    FluidSystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    ASSERT_EQ(sys.place_extractor(128, 0, 0), INVALID_ENTITY_ID);
    ASSERT_EQ(sys.place_extractor(0, 128, 0), INVALID_ENTITY_ID);
    ASSERT_EQ(sys.place_extractor(200, 200, 0), INVALID_ENTITY_ID);
}

TEST(place_extractor_invalid_owner_fails) {
    FluidSystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    ASSERT_EQ(sys.place_extractor(5, 5, MAX_PLAYERS), INVALID_ENTITY_ID);
}

// =============================================================================
// Place Conduit Tests
// =============================================================================

TEST(place_conduit_creates_entity) {
    FluidSystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    uint32_t eid = sys.place_conduit(15, 25, 0);
    ASSERT(eid != INVALID_ENTITY_ID);

    // Verify entity exists in registry
    auto entity = static_cast<entt::entity>(eid);
    ASSERT(registry.valid(entity));

    // Verify entity has FluidConduitComponent
    ASSERT(registry.all_of<FluidConduitComponent>(entity));
}

TEST(place_conduit_registers_position) {
    FluidSystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    ASSERT_EQ(sys.get_conduit_position_count(0), 0u);
    sys.place_conduit(15, 25, 0);
    ASSERT_EQ(sys.get_conduit_position_count(0), 1u);
}

TEST(place_conduit_emits_event) {
    FluidSystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    uint32_t eid = sys.place_conduit(15, 25, 1);
    const auto& events = sys.get_conduit_placed_events();
    ASSERT_EQ(events.size(), static_cast<size_t>(1));
    ASSERT_EQ(events[0].entity_id, eid);
    ASSERT_EQ(events[0].owner_id, static_cast<uint8_t>(1));
    ASSERT_EQ(events[0].grid_x, 15u);
    ASSERT_EQ(events[0].grid_y, 25u);
}

// =============================================================================
// Remove Conduit Tests
// =============================================================================

TEST(remove_conduit_destroys_entity) {
    FluidSystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    uint32_t eid = sys.place_conduit(15, 25, 0);
    ASSERT(eid != INVALID_ENTITY_ID);
    ASSERT_EQ(sys.get_conduit_position_count(0), 1u);

    // Remove conduit
    bool result = sys.remove_conduit(eid, 0, 15, 25);
    ASSERT(result);

    // Verify entity is destroyed
    auto entity = static_cast<entt::entity>(eid);
    ASSERT(!registry.valid(entity));

    // Verify position unregistered
    ASSERT_EQ(sys.get_conduit_position_count(0), 0u);
}

TEST(remove_conduit_emits_event) {
    FluidSystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    uint32_t eid = sys.place_conduit(15, 25, 0);
    // Clear placement events first
    sys.clear_transition_events();

    bool result = sys.remove_conduit(eid, 0, 15, 25);
    ASSERT(result);

    const auto& events = sys.get_conduit_removed_events();
    ASSERT_EQ(events.size(), static_cast<size_t>(1));
    ASSERT_EQ(events[0].entity_id, eid);
    ASSERT_EQ(events[0].owner_id, static_cast<uint8_t>(0));
}

TEST(remove_conduit_no_registry_fails) {
    FluidSystem sys(128, 128);
    ASSERT(!sys.remove_conduit(0, 0, 0, 0));
}

TEST(remove_conduit_invalid_entity_fails) {
    FluidSystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);
    ASSERT(!sys.remove_conduit(9999, 0, 0, 0));
}

TEST(remove_conduit_wrong_component_fails) {
    FluidSystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    // Create entity without FluidConduitComponent
    auto entity = registry.create();
    uint32_t eid = static_cast<uint32_t>(entity);
    ASSERT(!sys.remove_conduit(eid, 0, 0, 0));
}

// =============================================================================
// Coverage Dirty Flag Tests
// =============================================================================

TEST(coverage_dirty_on_extractor_placement) {
    FluidSystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    ASSERT(!sys.is_coverage_dirty(0));
    sys.place_extractor(10, 20, 0);
    ASSERT(sys.is_coverage_dirty(0));
    ASSERT(!sys.is_coverage_dirty(1));
}

TEST(coverage_dirty_on_conduit_placement) {
    FluidSystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    ASSERT(!sys.is_coverage_dirty(0));
    sys.place_conduit(10, 20, 0);
    ASSERT(sys.is_coverage_dirty(0));
}

TEST(coverage_dirty_on_conduit_removal) {
    FluidSystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    uint32_t eid = sys.place_conduit(10, 20, 0);
    // Reset dirty flag manually to test removal
    // (place_conduit already sets it, so we need to verify removal sets it again)
    // We can't reset it directly, but we can verify it stays true
    ASSERT(sys.is_coverage_dirty(0));

    sys.remove_conduit(eid, 0, 10, 20);
    ASSERT(sys.is_coverage_dirty(0));
}

TEST(coverage_dirty_out_of_bounds_returns_false) {
    FluidSystem sys(128, 128);
    ASSERT(!sys.is_coverage_dirty(MAX_PLAYERS));
    ASSERT(!sys.is_coverage_dirty(255));
}

// =============================================================================
// Registration Tests
// =============================================================================

TEST(register_extractor_increments_count) {
    FluidSystem sys(128, 128);
    ASSERT_EQ(sys.get_extractor_count(0), 0u);

    sys.register_extractor(100, 0);
    ASSERT_EQ(sys.get_extractor_count(0), 1u);

    sys.register_extractor(101, 0);
    ASSERT_EQ(sys.get_extractor_count(0), 2u);
}

TEST(unregister_extractor_decrements_count) {
    FluidSystem sys(128, 128);
    sys.register_extractor(100, 0);
    sys.register_extractor(101, 0);
    ASSERT_EQ(sys.get_extractor_count(0), 2u);

    sys.unregister_extractor(100, 0);
    ASSERT_EQ(sys.get_extractor_count(0), 1u);
}

TEST(register_reservoir_increments_count) {
    FluidSystem sys(128, 128);
    ASSERT_EQ(sys.get_reservoir_count(0), 0u);

    sys.register_reservoir(200, 0);
    ASSERT_EQ(sys.get_reservoir_count(0), 1u);
}

TEST(register_consumer_increments_count) {
    FluidSystem sys(128, 128);
    ASSERT_EQ(sys.get_consumer_count(0), 0u);

    sys.register_consumer(300, 0);
    ASSERT_EQ(sys.get_consumer_count(0), 1u);
}

TEST(register_out_of_bounds_owner_ignored) {
    FluidSystem sys(128, 128);
    sys.register_extractor(100, MAX_PLAYERS);
    sys.register_reservoir(200, MAX_PLAYERS);
    sys.register_consumer(300, MAX_PLAYERS);

    // All counts should still be 0 for valid owners
    for (uint8_t i = 0; i < MAX_PLAYERS; ++i) {
        ASSERT_EQ(sys.get_extractor_count(i), 0u);
        ASSERT_EQ(sys.get_reservoir_count(i), 0u);
        ASSERT_EQ(sys.get_consumer_count(i), 0u);
    }
}

// =============================================================================
// Per-Player Isolation Tests
// =============================================================================

TEST(registrations_isolated_per_player) {
    FluidSystem sys(128, 128);

    sys.register_extractor(100, 0);
    sys.register_extractor(101, 1);
    sys.register_reservoir(200, 2);
    sys.register_consumer(300, 3);

    ASSERT_EQ(sys.get_extractor_count(0), 1u);
    ASSERT_EQ(sys.get_extractor_count(1), 1u);
    ASSERT_EQ(sys.get_extractor_count(2), 0u);
    ASSERT_EQ(sys.get_reservoir_count(2), 1u);
    ASSERT_EQ(sys.get_consumer_count(3), 1u);
}

// =============================================================================
// Place Reservoir Tests
// =============================================================================

TEST(place_reservoir_creates_entity) {
    FluidSystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    uint32_t eid = sys.place_reservoir(30, 40, 0);
    ASSERT(eid != INVALID_ENTITY_ID);

    auto entity = static_cast<entt::entity>(eid);
    ASSERT(registry.valid(entity));
    ASSERT(registry.all_of<FluidReservoirComponent>(entity));
    ASSERT(registry.all_of<FluidProducerComponent>(entity));
}

TEST(place_reservoir_registers_entity) {
    FluidSystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    ASSERT_EQ(sys.get_reservoir_count(0), 0u);
    sys.place_reservoir(30, 40, 0);
    ASSERT_EQ(sys.get_reservoir_count(0), 1u);
}

// =============================================================================
// Tick Test
// =============================================================================

TEST(tick_runs_without_crash) {
    FluidSystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    // Place some entities
    sys.place_extractor(10, 10, 0);
    sys.place_conduit(11, 10, 0);
    sys.place_reservoir(12, 10, 0);

    // Tick should not crash even though pipeline phases are stubs
    sys.tick(0.016f);
}

TEST(tick_clears_events) {
    FluidSystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    sys.place_extractor(10, 10, 0);
    ASSERT_EQ(sys.get_extractor_placed_events().size(), static_cast<size_t>(1));

    // Tick should clear events
    sys.tick(0.016f);
    ASSERT_EQ(sys.get_extractor_placed_events().size(), static_cast<size_t>(0));
}

// =============================================================================
// set_energy_provider Test
// =============================================================================

TEST(set_energy_provider_accepts_nullptr) {
    FluidSystem sys(128, 128);
    sys.set_energy_provider(nullptr);
    // Should not crash
}

// =============================================================================
// Main
// =============================================================================

int main() {
    printf("=== FluidSystem Unit Tests (Ticket 6-009) ===\n\n");

    // Construction tests
    RUN_TEST(construction_128x128);
    RUN_TEST(construction_256x256);
    RUN_TEST(construction_512x512);
    RUN_TEST(construction_with_nullptr_terrain);
    RUN_TEST(construction_non_square);

    // Priority test
    RUN_TEST(get_priority_returns_20);

    // Registry wiring tests
    RUN_TEST(set_registry_wiring);
    RUN_TEST(set_registry_nullptr_prevents_placement);

    // Pool query tests
    RUN_TEST(pool_query_returns_default_state);
    RUN_TEST(pool_state_returns_healthy_default);
    RUN_TEST(pool_state_out_of_bounds_returns_healthy);

    // has_fluid tests
    RUN_TEST(has_fluid_returns_false_no_registry);
    RUN_TEST(has_fluid_returns_false_invalid_entity);
    RUN_TEST(has_fluid_returns_false_no_component);
    RUN_TEST(has_fluid_returns_component_value);

    // has_fluid_at tests
    RUN_TEST(has_fluid_at_returns_false_no_coverage);
    RUN_TEST(has_fluid_at_returns_false_invalid_player);

    // Place extractor tests
    RUN_TEST(place_extractor_creates_entity);
    RUN_TEST(place_extractor_registers_entity);
    RUN_TEST(place_extractor_emits_event);
    RUN_TEST(place_extractor_out_of_bounds_fails);
    RUN_TEST(place_extractor_invalid_owner_fails);

    // Place conduit tests
    RUN_TEST(place_conduit_creates_entity);
    RUN_TEST(place_conduit_registers_position);
    RUN_TEST(place_conduit_emits_event);

    // Remove conduit tests
    RUN_TEST(remove_conduit_destroys_entity);
    RUN_TEST(remove_conduit_emits_event);
    RUN_TEST(remove_conduit_no_registry_fails);
    RUN_TEST(remove_conduit_invalid_entity_fails);
    RUN_TEST(remove_conduit_wrong_component_fails);

    // Coverage dirty flag tests
    RUN_TEST(coverage_dirty_on_extractor_placement);
    RUN_TEST(coverage_dirty_on_conduit_placement);
    RUN_TEST(coverage_dirty_on_conduit_removal);
    RUN_TEST(coverage_dirty_out_of_bounds_returns_false);

    // Registration tests
    RUN_TEST(register_extractor_increments_count);
    RUN_TEST(unregister_extractor_decrements_count);
    RUN_TEST(register_reservoir_increments_count);
    RUN_TEST(register_consumer_increments_count);
    RUN_TEST(register_out_of_bounds_owner_ignored);

    // Per-player isolation test
    RUN_TEST(registrations_isolated_per_player);

    // Place reservoir tests
    RUN_TEST(place_reservoir_creates_entity);
    RUN_TEST(place_reservoir_registers_entity);

    // Tick tests
    RUN_TEST(tick_runs_without_crash);
    RUN_TEST(tick_clears_events);

    // Energy provider test
    RUN_TEST(set_energy_provider_accepts_nullptr);

    printf("\n=== Results: %d passed, %d failed ===\n",
           tests_passed, tests_failed);

    return tests_failed > 0 ? 1 : 0;
}
