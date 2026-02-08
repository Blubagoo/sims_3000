/**
 * @file test_building_decon_handler.cpp
 * @brief Unit tests for FluidSystem::on_building_deconstructed (Ticket 6-035)
 *
 * Tests cover:
 * - Deconstruct consumer reduces count
 * - Deconstruct extractor reduces count and sets coverage dirty
 * - Deconstruct reservoir reduces count and sets coverage dirty
 * - Deconstruct non-fluid entity is no-op
 * - Entity that was both consumer and extractor unregisters both
 * - Bounds check: invalid owner (>= MAX_PLAYERS) does nothing
 * - Double deconstruction does not crash
 * - Deconstruct without registry does not crash
 * - Coverage not dirty after consumer deconstruct (only producers affect coverage)
 * - Multiple deconstructions across different players
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
// Consumer deconstruction
// =============================================================================

TEST(deconstruct_consumer_reduces_count) {
    FluidSystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    // Create and register a consumer via on_building_constructed
    auto entity = registry.create();
    uint32_t eid = static_cast<uint32_t>(entity);
    FluidComponent fc{};
    fc.fluid_required = 50;
    registry.emplace<FluidComponent>(entity, fc);

    sys.on_building_constructed(eid, 0, 20, 30);
    ASSERT_EQ(sys.get_consumer_count(0), 1u);

    // Deconstruct
    sys.on_building_deconstructed(eid, 0, 20, 30);

    ASSERT_EQ(sys.get_consumer_count(0), 0u);
}

// =============================================================================
// Extractor deconstruction
// =============================================================================

TEST(deconstruct_extractor_reduces_count_and_sets_dirty) {
    FluidSystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    auto entity = registry.create();
    uint32_t eid = static_cast<uint32_t>(entity);
    FluidProducerComponent prod{};
    prod.base_output = 300;
    prod.producer_type = static_cast<uint8_t>(FluidProducerType::Extractor);
    registry.emplace<FluidProducerComponent>(entity, prod);

    sys.on_building_constructed(eid, 0, 40, 50);
    ASSERT_EQ(sys.get_extractor_count(0), 1u);

    // Run a tick to clear the dirty flag set during construction
    sys.tick(0.0f);
    ASSERT(!sys.is_coverage_dirty(0));

    // Deconstruct
    sys.on_building_deconstructed(eid, 0, 40, 50);

    ASSERT_EQ(sys.get_extractor_count(0), 0u);
    ASSERT(sys.is_coverage_dirty(0));
}

// =============================================================================
// Reservoir deconstruction
// =============================================================================

TEST(deconstruct_reservoir_reduces_count_and_sets_dirty) {
    FluidSystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    auto entity = registry.create();
    uint32_t eid = static_cast<uint32_t>(entity);
    FluidReservoirComponent res{};
    res.capacity = 1000;
    registry.emplace<FluidReservoirComponent>(entity, res);

    sys.on_building_constructed(eid, 0, 60, 60);
    ASSERT_EQ(sys.get_reservoir_count(0), 1u);

    // Run a tick to clear the dirty flag set during construction
    sys.tick(0.0f);
    ASSERT(!sys.is_coverage_dirty(0));

    // Deconstruct
    sys.on_building_deconstructed(eid, 0, 60, 60);

    ASSERT_EQ(sys.get_reservoir_count(0), 0u);
    ASSERT(sys.is_coverage_dirty(0));
}

// =============================================================================
// Non-fluid entity deconstruction
// =============================================================================

TEST(deconstruct_non_fluid_entity_is_noop) {
    FluidSystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    // Create entity with no fluid components, never registered
    auto entity = registry.create();
    uint32_t eid = static_cast<uint32_t>(entity);

    // Should not crash and should not change any counts
    sys.on_building_deconstructed(eid, 0, 50, 50);

    ASSERT_EQ(sys.get_consumer_count(0), 0u);
    ASSERT_EQ(sys.get_extractor_count(0), 0u);
    ASSERT_EQ(sys.get_reservoir_count(0), 0u);
}

TEST(deconstruct_entity_not_in_any_list) {
    FluidSystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    // Register one consumer
    auto e1 = registry.create();
    uint32_t eid1 = static_cast<uint32_t>(e1);
    FluidComponent fc1{};
    fc1.fluid_required = 50;
    registry.emplace<FluidComponent>(e1, fc1);
    sys.on_building_constructed(eid1, 0, 10, 10);

    // Try to deconstruct a different entity that was never registered
    auto e2 = registry.create();
    uint32_t eid2 = static_cast<uint32_t>(e2);
    sys.on_building_deconstructed(eid2, 0, 20, 20);

    // The first consumer should still be registered
    ASSERT_EQ(sys.get_consumer_count(0), 1u);
}

// =============================================================================
// Entity with both consumer and extractor
// =============================================================================

TEST(entity_with_both_components_unregisters_both) {
    FluidSystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    auto entity = registry.create();
    uint32_t eid = static_cast<uint32_t>(entity);

    FluidComponent fc{};
    fc.fluid_required = 10;
    registry.emplace<FluidComponent>(entity, fc);

    FluidProducerComponent prod{};
    prod.base_output = 200;
    prod.producer_type = static_cast<uint8_t>(FluidProducerType::Extractor);
    registry.emplace<FluidProducerComponent>(entity, prod);

    sys.on_building_constructed(eid, 0, 15, 15);
    ASSERT_EQ(sys.get_consumer_count(0), 1u);
    ASSERT_EQ(sys.get_extractor_count(0), 1u);

    sys.on_building_deconstructed(eid, 0, 15, 15);

    ASSERT_EQ(sys.get_consumer_count(0), 0u);
    ASSERT_EQ(sys.get_extractor_count(0), 0u);
}

// =============================================================================
// Invalid owner
// =============================================================================

TEST(invalid_owner_does_nothing) {
    FluidSystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    auto entity = registry.create();
    uint32_t eid = static_cast<uint32_t>(entity);
    FluidComponent fc{};
    fc.fluid_required = 50;
    registry.emplace<FluidComponent>(entity, fc);

    // Register with valid owner first
    sys.on_building_constructed(eid, 0, 10, 10);
    ASSERT_EQ(sys.get_consumer_count(0), 1u);

    // Deconstruct with invalid owner should do nothing
    sys.on_building_deconstructed(eid, MAX_PLAYERS, 10, 10);
    sys.on_building_deconstructed(eid, 255, 10, 10);

    // Consumer still registered for player 0
    ASSERT_EQ(sys.get_consumer_count(0), 1u);
}

// =============================================================================
// Double deconstruction
// =============================================================================

TEST(double_deconstruction_no_crash) {
    FluidSystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    auto entity = registry.create();
    uint32_t eid = static_cast<uint32_t>(entity);
    FluidComponent fc{};
    fc.fluid_required = 50;
    registry.emplace<FluidComponent>(entity, fc);

    sys.on_building_constructed(eid, 0, 10, 10);
    ASSERT_EQ(sys.get_consumer_count(0), 1u);

    // Deconstruct twice - second should be no-op
    sys.on_building_deconstructed(eid, 0, 10, 10);
    ASSERT_EQ(sys.get_consumer_count(0), 0u);

    sys.on_building_deconstructed(eid, 0, 10, 10);
    ASSERT_EQ(sys.get_consumer_count(0), 0u);
}

// =============================================================================
// Deconstruct without registry (no crash)
// =============================================================================

TEST(deconstruct_without_registry_no_crash) {
    FluidSystem sys(128, 128);
    // No registry set - on_building_deconstructed only checks internal lists
    sys.on_building_deconstructed(42, 0, 10, 10);
    ASSERT_EQ(sys.get_consumer_count(0), 0u);
    ASSERT_EQ(sys.get_extractor_count(0), 0u);
    ASSERT_EQ(sys.get_reservoir_count(0), 0u);
}

// =============================================================================
// Coverage not dirty after consumer deconstruction
// =============================================================================

TEST(coverage_not_dirty_after_consumer_deconstruct) {
    FluidSystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    auto entity = registry.create();
    uint32_t eid = static_cast<uint32_t>(entity);
    FluidComponent fc{};
    fc.fluid_required = 50;
    registry.emplace<FluidComponent>(entity, fc);

    sys.on_building_constructed(eid, 0, 10, 10);

    // Clear dirty flag by running a tick
    sys.tick(0.0f);
    ASSERT(!sys.is_coverage_dirty(0));

    // Deconstruct consumer - should NOT mark coverage dirty
    // (consumers don't affect coverage, only extractors/reservoirs do)
    sys.on_building_deconstructed(eid, 0, 10, 10);
    ASSERT(!sys.is_coverage_dirty(0));
}

// =============================================================================
// Multiple deconstructions different players
// =============================================================================

TEST(multiple_deconstructions_different_players) {
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

    ASSERT_EQ(sys.get_consumer_count(0), 1u);
    ASSERT_EQ(sys.get_extractor_count(1), 1u);

    // Deconstruct player 0's consumer
    sys.on_building_deconstructed(eid0, 0, 10, 10);
    ASSERT_EQ(sys.get_consumer_count(0), 0u);
    ASSERT_EQ(sys.get_extractor_count(1), 1u); // player 1 unaffected

    // Deconstruct player 1's extractor
    sys.on_building_deconstructed(eid1, 1, 20, 20);
    ASSERT_EQ(sys.get_extractor_count(1), 0u);
}

// =============================================================================
// Multiple deconstructions same player
// =============================================================================

TEST(multiple_deconstructions_same_player) {
    FluidSystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    // Create 3 consumers for player 0
    uint32_t eids[3];
    for (int i = 0; i < 3; ++i) {
        auto entity = registry.create();
        eids[i] = static_cast<uint32_t>(entity);
        FluidComponent fc{};
        fc.fluid_required = 10 * static_cast<uint32_t>(i + 1);
        registry.emplace<FluidComponent>(entity, fc);
        sys.on_building_constructed(eids[i], 0, i * 10, i * 10);
    }
    ASSERT_EQ(sys.get_consumer_count(0), 3u);

    // Deconstruct first and third
    sys.on_building_deconstructed(eids[0], 0, 0, 0);
    ASSERT_EQ(sys.get_consumer_count(0), 2u);

    sys.on_building_deconstructed(eids[2], 0, 20, 20);
    ASSERT_EQ(sys.get_consumer_count(0), 1u);

    // Second consumer still there
    sys.on_building_deconstructed(eids[1], 0, 10, 10);
    ASSERT_EQ(sys.get_consumer_count(0), 0u);
}

// =============================================================================
// Main Entry Point
// =============================================================================

int main() {
    printf("=== FluidSystem BuildingDeconstructedEvent Handler Unit Tests (Ticket 6-035) ===\n\n");

    // Consumer deconstruction
    RUN_TEST(deconstruct_consumer_reduces_count);

    // Extractor deconstruction
    RUN_TEST(deconstruct_extractor_reduces_count_and_sets_dirty);

    // Reservoir deconstruction
    RUN_TEST(deconstruct_reservoir_reduces_count_and_sets_dirty);

    // Non-fluid entity
    RUN_TEST(deconstruct_non_fluid_entity_is_noop);
    RUN_TEST(deconstruct_entity_not_in_any_list);

    // Both components
    RUN_TEST(entity_with_both_components_unregisters_both);

    // Invalid owner
    RUN_TEST(invalid_owner_does_nothing);

    // Edge cases
    RUN_TEST(double_deconstruction_no_crash);
    RUN_TEST(deconstruct_without_registry_no_crash);
    RUN_TEST(coverage_not_dirty_after_consumer_deconstruct);

    // Multiple deconstructions
    RUN_TEST(multiple_deconstructions_different_players);
    RUN_TEST(multiple_deconstructions_same_player);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
