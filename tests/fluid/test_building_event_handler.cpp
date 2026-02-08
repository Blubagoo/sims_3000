/**
 * @file test_building_event_handler.cpp
 * @brief Unit tests for FluidSystem::on_building_constructed (Ticket 6-034)
 *
 * Tests cover:
 * - Construct building with FluidComponent -> registered as consumer
 * - Construct building with FluidProducerComponent -> registered as extractor
 * - Construct building with FluidReservoirComponent -> registered as reservoir
 * - Entity with multiple fluid components registers for each role
 * - Entity with no fluid components does nothing
 * - No-op with null registry
 * - No-op with invalid owner (>= MAX_PLAYERS)
 * - No-op with invalid entity (not valid in registry)
 * - No-op with negative coordinates (bounds validation)
 * - No-op with out-of-bounds coordinates
 * - Multiple buildings for different players
 */

#include <sims3000/fluid/FluidSystem.h>
#include <sims3000/fluid/FluidComponent.h>
#include <sims3000/fluid/FluidProducerComponent.h>
#include <sims3000/fluid/FluidReservoirComponent.h>
#include <sims3000/fluid/FluidEnums.h>
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
// Consumer registration (FluidComponent)
// =============================================================================

TEST(registers_consumer_when_entity_has_fluid_component) {
    FluidSystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    auto entity = registry.create();
    uint32_t eid = static_cast<uint32_t>(entity);
    FluidComponent fc{};
    fc.fluid_required = 50;
    registry.emplace<FluidComponent>(entity, fc);

    ASSERT_EQ(sys.get_consumer_count(0), 0u);

    sys.on_building_constructed(eid, 0, 20, 30);

    ASSERT_EQ(sys.get_consumer_count(0), 1u);
}

// =============================================================================
// Extractor registration (FluidProducerComponent)
// =============================================================================

TEST(registers_extractor_when_entity_has_producer_component) {
    FluidSystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    auto entity = registry.create();
    uint32_t eid = static_cast<uint32_t>(entity);
    FluidProducerComponent prod{};
    prod.base_output = 300;
    prod.current_output = 300;
    prod.producer_type = static_cast<uint8_t>(FluidProducerType::Extractor);
    prod.is_operational = true;
    registry.emplace<FluidProducerComponent>(entity, prod);

    ASSERT_EQ(sys.get_extractor_count(0), 0u);

    sys.on_building_constructed(eid, 0, 40, 50);

    ASSERT_EQ(sys.get_extractor_count(0), 1u);
}

TEST(extractor_registration_marks_coverage_dirty) {
    FluidSystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    // Coverage should not be dirty initially
    ASSERT(!sys.is_coverage_dirty(0));

    auto entity = registry.create();
    uint32_t eid = static_cast<uint32_t>(entity);
    FluidProducerComponent prod{};
    prod.base_output = 200;
    prod.producer_type = static_cast<uint8_t>(FluidProducerType::Extractor);
    registry.emplace<FluidProducerComponent>(entity, prod);

    sys.on_building_constructed(eid, 0, 30, 30);

    // Coverage should be dirty after extractor registration
    ASSERT(sys.is_coverage_dirty(0));
}

// =============================================================================
// Reservoir registration (FluidReservoirComponent)
// =============================================================================

TEST(registers_reservoir_when_entity_has_reservoir_component) {
    FluidSystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    auto entity = registry.create();
    uint32_t eid = static_cast<uint32_t>(entity);
    FluidReservoirComponent res{};
    res.capacity = 1000;
    res.current_level = 0;
    registry.emplace<FluidReservoirComponent>(entity, res);

    ASSERT_EQ(sys.get_reservoir_count(0), 0u);

    sys.on_building_constructed(eid, 0, 60, 60);

    ASSERT_EQ(sys.get_reservoir_count(0), 1u);
}

TEST(reservoir_registration_marks_coverage_dirty) {
    FluidSystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    ASSERT(!sys.is_coverage_dirty(0));

    auto entity = registry.create();
    uint32_t eid = static_cast<uint32_t>(entity);
    FluidReservoirComponent res{};
    res.capacity = 1000;
    registry.emplace<FluidReservoirComponent>(entity, res);

    sys.on_building_constructed(eid, 0, 50, 50);

    ASSERT(sys.is_coverage_dirty(0));
}

// =============================================================================
// Entity with multiple fluid components
// =============================================================================

TEST(entity_with_consumer_and_producer_registers_as_both) {
    FluidSystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    auto entity = registry.create();
    uint32_t eid = static_cast<uint32_t>(entity);

    // Add both FluidComponent and FluidProducerComponent
    FluidComponent fc{};
    fc.fluid_required = 10;
    registry.emplace<FluidComponent>(entity, fc);

    FluidProducerComponent prod{};
    prod.base_output = 200;
    prod.producer_type = static_cast<uint8_t>(FluidProducerType::Extractor);
    registry.emplace<FluidProducerComponent>(entity, prod);

    sys.on_building_constructed(eid, 0, 15, 15);

    // Should be both consumer and extractor
    ASSERT_EQ(sys.get_consumer_count(0), 1u);
    ASSERT_EQ(sys.get_extractor_count(0), 1u);
}

TEST(entity_with_producer_and_reservoir_registers_as_both) {
    FluidSystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    auto entity = registry.create();
    uint32_t eid = static_cast<uint32_t>(entity);

    // Reservoir entities typically have both FluidProducerComponent and FluidReservoirComponent
    FluidProducerComponent prod{};
    prod.base_output = 0;
    prod.producer_type = static_cast<uint8_t>(FluidProducerType::Reservoir);
    registry.emplace<FluidProducerComponent>(entity, prod);

    FluidReservoirComponent res{};
    res.capacity = 1000;
    registry.emplace<FluidReservoirComponent>(entity, res);

    sys.on_building_constructed(eid, 0, 25, 25);

    // Should be both extractor (from producer component) and reservoir
    ASSERT_EQ(sys.get_extractor_count(0), 1u);
    ASSERT_EQ(sys.get_reservoir_count(0), 1u);
}

// =============================================================================
// Entity with no fluid components
// =============================================================================

TEST(entity_with_no_fluid_components_does_nothing) {
    FluidSystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    auto entity = registry.create();
    uint32_t eid = static_cast<uint32_t>(entity);

    sys.on_building_constructed(eid, 0, 50, 50);

    ASSERT_EQ(sys.get_consumer_count(0), 0u);
    ASSERT_EQ(sys.get_extractor_count(0), 0u);
    ASSERT_EQ(sys.get_reservoir_count(0), 0u);
}

// =============================================================================
// No-op cases
// =============================================================================

TEST(noop_with_null_registry) {
    FluidSystem sys(128, 128);
    // No registry set
    sys.on_building_constructed(42, 0, 10, 10);
    ASSERT_EQ(sys.get_consumer_count(0), 0u);
    ASSERT_EQ(sys.get_extractor_count(0), 0u);
    ASSERT_EQ(sys.get_reservoir_count(0), 0u);
}

TEST(noop_for_invalid_owner) {
    FluidSystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    auto entity = registry.create();
    uint32_t eid = static_cast<uint32_t>(entity);
    FluidComponent fc{};
    fc.fluid_required = 50;
    registry.emplace<FluidComponent>(entity, fc);

    // Invalid owner (>= MAX_PLAYERS)
    sys.on_building_constructed(eid, MAX_PLAYERS, 10, 10);
    sys.on_building_constructed(eid, 255, 10, 10);

    // Nothing should be registered for any player
    for (uint8_t i = 0; i < MAX_PLAYERS; ++i) {
        ASSERT_EQ(sys.get_consumer_count(i), 0u);
    }
}

TEST(noop_for_invalid_entity) {
    FluidSystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    // INVALID_ENTITY_ID should not crash
    sys.on_building_constructed(INVALID_ENTITY_ID, 0, 10, 10);
    ASSERT_EQ(sys.get_consumer_count(0), 0u);
    ASSERT_EQ(sys.get_extractor_count(0), 0u);
    ASSERT_EQ(sys.get_reservoir_count(0), 0u);
}

TEST(noop_for_destroyed_entity) {
    FluidSystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    auto entity = registry.create();
    uint32_t eid = static_cast<uint32_t>(entity);
    registry.destroy(entity);

    // Entity no longer valid
    sys.on_building_constructed(eid, 0, 10, 10);
    ASSERT_EQ(sys.get_consumer_count(0), 0u);
    ASSERT_EQ(sys.get_extractor_count(0), 0u);
}

TEST(noop_for_negative_coordinates) {
    FluidSystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    auto entity = registry.create();
    uint32_t eid = static_cast<uint32_t>(entity);
    FluidComponent fc{};
    fc.fluid_required = 50;
    registry.emplace<FluidComponent>(entity, fc);

    // Negative coordinates should be rejected
    sys.on_building_constructed(eid, 0, -1, 10);
    ASSERT_EQ(sys.get_consumer_count(0), 0u);

    sys.on_building_constructed(eid, 0, 10, -1);
    ASSERT_EQ(sys.get_consumer_count(0), 0u);
}

TEST(noop_for_out_of_bounds_coordinates) {
    FluidSystem sys(64, 64);
    entt::registry registry;
    sys.set_registry(&registry);

    auto entity = registry.create();
    uint32_t eid = static_cast<uint32_t>(entity);
    FluidComponent fc{};
    fc.fluid_required = 50;
    registry.emplace<FluidComponent>(entity, fc);

    // Coordinates at or beyond map bounds
    sys.on_building_constructed(eid, 0, 64, 10);
    ASSERT_EQ(sys.get_consumer_count(0), 0u);

    sys.on_building_constructed(eid, 0, 10, 64);
    ASSERT_EQ(sys.get_consumer_count(0), 0u);
}

// =============================================================================
// Multiple buildings, different players
// =============================================================================

TEST(multiple_buildings_different_players) {
    FluidSystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    // Player 0: consumer
    auto e0 = registry.create();
    uint32_t eid0 = static_cast<uint32_t>(e0);
    FluidComponent fc0{};
    fc0.fluid_required = 50;
    registry.emplace<FluidComponent>(e0, fc0);
    sys.on_building_constructed(eid0, 0, 10, 10);

    // Player 1: extractor
    auto e1 = registry.create();
    uint32_t eid1 = static_cast<uint32_t>(e1);
    FluidProducerComponent prod1{};
    prod1.base_output = 300;
    prod1.producer_type = static_cast<uint8_t>(FluidProducerType::Extractor);
    registry.emplace<FluidProducerComponent>(e1, prod1);
    sys.on_building_constructed(eid1, 1, 20, 20);

    // Player 2: reservoir
    auto e2 = registry.create();
    uint32_t eid2 = static_cast<uint32_t>(e2);
    FluidReservoirComponent res2{};
    res2.capacity = 1000;
    registry.emplace<FluidReservoirComponent>(e2, res2);
    sys.on_building_constructed(eid2, 2, 30, 30);

    // Verify per-player counts
    ASSERT_EQ(sys.get_consumer_count(0), 1u);
    ASSERT_EQ(sys.get_extractor_count(0), 0u);
    ASSERT_EQ(sys.get_reservoir_count(0), 0u);

    ASSERT_EQ(sys.get_consumer_count(1), 0u);
    ASSERT_EQ(sys.get_extractor_count(1), 1u);
    ASSERT_EQ(sys.get_reservoir_count(1), 0u);

    ASSERT_EQ(sys.get_consumer_count(2), 0u);
    ASSERT_EQ(sys.get_extractor_count(2), 0u);
    ASSERT_EQ(sys.get_reservoir_count(2), 1u);

    // Player 3 untouched
    ASSERT_EQ(sys.get_consumer_count(3), 0u);
    ASSERT_EQ(sys.get_extractor_count(3), 0u);
    ASSERT_EQ(sys.get_reservoir_count(3), 0u);
}

// =============================================================================
// Multiple consumers for same player
// =============================================================================

TEST(multiple_consumers_same_player) {
    FluidSystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    for (int i = 0; i < 5; ++i) {
        auto entity = registry.create();
        uint32_t eid = static_cast<uint32_t>(entity);
        FluidComponent fc{};
        fc.fluid_required = 10 * static_cast<uint32_t>(i + 1);
        registry.emplace<FluidComponent>(entity, fc);
        sys.on_building_constructed(eid, 0, i * 10, i * 10);
    }

    ASSERT_EQ(sys.get_consumer_count(0), 5u);
}

// =============================================================================
// Main Entry Point
// =============================================================================

int main() {
    printf("=== FluidSystem BuildingConstructedEvent Handler Unit Tests (Ticket 6-034) ===\n\n");

    // Consumer registration
    RUN_TEST(registers_consumer_when_entity_has_fluid_component);

    // Extractor registration
    RUN_TEST(registers_extractor_when_entity_has_producer_component);
    RUN_TEST(extractor_registration_marks_coverage_dirty);

    // Reservoir registration
    RUN_TEST(registers_reservoir_when_entity_has_reservoir_component);
    RUN_TEST(reservoir_registration_marks_coverage_dirty);

    // Multiple components
    RUN_TEST(entity_with_consumer_and_producer_registers_as_both);
    RUN_TEST(entity_with_producer_and_reservoir_registers_as_both);

    // No fluid components
    RUN_TEST(entity_with_no_fluid_components_does_nothing);

    // No-op cases
    RUN_TEST(noop_with_null_registry);
    RUN_TEST(noop_for_invalid_owner);
    RUN_TEST(noop_for_invalid_entity);
    RUN_TEST(noop_for_destroyed_entity);
    RUN_TEST(noop_for_negative_coordinates);
    RUN_TEST(noop_for_out_of_bounds_coordinates);

    // Multiple buildings
    RUN_TEST(multiple_buildings_different_players);
    RUN_TEST(multiple_consumers_same_player);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
