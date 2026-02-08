/**
 * @file test_building_decon_handler.cpp
 * @brief Unit tests for BuildingDeconstructedEvent handler (Ticket 5-033)
 *
 * Tests cover:
 * - Consumer unregistration when entity was a consumer
 * - Consumer position unregistration when entity was a consumer
 * - Nexus unregistration when entity was a producer
 * - Nexus position unregistration when entity was a producer
 * - Coverage dirty flag set when nexus is unregistered
 * - Entity that was both consumer and producer unregisters both
 * - Entity that was neither consumer nor producer does nothing
 * - Bounds check: invalid owner (>= MAX_PLAYERS) does nothing
 * - Entity not registered does nothing (no crash)
 * - Multiple deconstructed buildings for same player
 * - Multiple deconstructed buildings for different players
 * - Generation decreases after nexus deconstruction
 */

#include <sims3000/energy/EnergySystem.h>
#include <sims3000/energy/EnergyComponent.h>
#include <sims3000/energy/EnergyProducerComponent.h>
#include <sims3000/energy/EnergyEnums.h>
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
// Consumer unregistration
// =============================================================================

TEST(unregisters_consumer_on_deconstruct) {
    EnergySystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    // Create and register a consumer via on_building_constructed
    auto entity = registry.create();
    uint32_t eid = static_cast<uint32_t>(entity);
    EnergyComponent ec{};
    ec.energy_required = 50;
    registry.emplace<EnergyComponent>(entity, ec);

    sys.on_building_constructed(eid, 0, 20, 30);
    ASSERT_EQ(sys.get_consumer_count(0), 1u);
    ASSERT_EQ(sys.get_consumer_position_count(0), 1u);

    // Deconstruct
    sys.on_building_deconstructed(eid, 0, 20, 30);

    ASSERT_EQ(sys.get_consumer_count(0), 0u);
    ASSERT_EQ(sys.get_consumer_position_count(0), 0u);
}

TEST(consumer_no_longer_in_aggregation_after_deconstruct) {
    EnergySystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    // Create consumer
    auto entity = registry.create();
    uint32_t eid = static_cast<uint32_t>(entity);
    EnergyComponent ec{};
    ec.energy_required = 100;
    registry.emplace<EnergyComponent>(entity, ec);

    sys.on_building_constructed(eid, 0, 10, 10);

    // Set up a nexus with coverage over the consumer position
    auto nexus = registry.create();
    EnergyProducerComponent prod{};
    prod.base_output = 500;
    prod.current_output = 500;
    prod.nexus_type = static_cast<uint8_t>(NexusType::Carbon);
    prod.is_online = true;
    registry.emplace<EnergyProducerComponent>(nexus, prod);
    sys.register_nexus(static_cast<uint32_t>(nexus), 0);
    sys.register_nexus_position(static_cast<uint32_t>(nexus), 0, 10, 10);
    sys.recalculate_coverage(0);

    // Verify consumption before deconstruct
    uint32_t consumption_before = sys.aggregate_consumption(0);
    ASSERT_EQ(consumption_before, 100u);

    // Deconstruct consumer
    sys.on_building_deconstructed(eid, 0, 10, 10);

    // Consumption should now be 0
    uint32_t consumption_after = sys.aggregate_consumption(0);
    ASSERT_EQ(consumption_after, 0u);
}

// =============================================================================
// Nexus unregistration
// =============================================================================

TEST(unregisters_nexus_on_deconstruct) {
    EnergySystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    auto entity = registry.create();
    uint32_t eid = static_cast<uint32_t>(entity);
    EnergyProducerComponent prod{};
    prod.base_output = 300;
    prod.current_output = 300;
    prod.nexus_type = static_cast<uint8_t>(NexusType::Solar);
    prod.is_online = true;
    registry.emplace<EnergyProducerComponent>(entity, prod);

    sys.on_building_constructed(eid, 0, 40, 50);
    ASSERT_EQ(sys.get_nexus_count(0), 1u);
    ASSERT_EQ(sys.get_nexus_position_count(0), 1u);

    // Deconstruct
    sys.on_building_deconstructed(eid, 0, 40, 50);

    ASSERT_EQ(sys.get_nexus_count(0), 0u);
    ASSERT_EQ(sys.get_nexus_position_count(0), 0u);
}

TEST(nexus_deconstruction_marks_coverage_dirty) {
    EnergySystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    auto entity = registry.create();
    uint32_t eid = static_cast<uint32_t>(entity);
    EnergyProducerComponent prod{};
    prod.base_output = 200;
    prod.nexus_type = static_cast<uint8_t>(NexusType::Wind);
    prod.is_online = true;
    registry.emplace<EnergyProducerComponent>(entity, prod);

    sys.on_building_constructed(eid, 0, 30, 30);

    // Clear dirty flag
    sys.recalculate_coverage(0);
    ASSERT(!sys.is_coverage_dirty(0));

    // Deconstruct
    sys.on_building_deconstructed(eid, 0, 30, 30);

    // Coverage should be dirty after nexus deconstruction
    ASSERT(sys.is_coverage_dirty(0));
}

TEST(nexus_generation_removed_after_deconstruct) {
    EnergySystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    auto entity = registry.create();
    uint32_t eid = static_cast<uint32_t>(entity);
    EnergyProducerComponent prod{};
    prod.base_output = 400;
    prod.current_output = 400;
    prod.nexus_type = static_cast<uint8_t>(NexusType::Nuclear);
    prod.is_online = true;
    registry.emplace<EnergyProducerComponent>(entity, prod);

    sys.on_building_constructed(eid, 0, 60, 60);
    ASSERT_EQ(sys.get_total_generation(0), 400u);

    // Deconstruct
    sys.on_building_deconstructed(eid, 0, 60, 60);

    // Generation should be 0 since nexus is unregistered
    ASSERT_EQ(sys.get_total_generation(0), 0u);
}

// =============================================================================
// Entity with both consumer and producer
// =============================================================================

TEST(entity_with_both_components_unregisters_both) {
    EnergySystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    auto entity = registry.create();
    uint32_t eid = static_cast<uint32_t>(entity);

    EnergyComponent ec{};
    ec.energy_required = 10;
    registry.emplace<EnergyComponent>(entity, ec);

    EnergyProducerComponent prod{};
    prod.base_output = 200;
    prod.current_output = 200;
    prod.nexus_type = static_cast<uint8_t>(NexusType::Carbon);
    prod.is_online = true;
    registry.emplace<EnergyProducerComponent>(entity, prod);

    sys.on_building_constructed(eid, 0, 15, 15);
    ASSERT_EQ(sys.get_consumer_count(0), 1u);
    ASSERT_EQ(sys.get_nexus_count(0), 1u);

    sys.on_building_deconstructed(eid, 0, 15, 15);

    ASSERT_EQ(sys.get_consumer_count(0), 0u);
    ASSERT_EQ(sys.get_consumer_position_count(0), 0u);
    ASSERT_EQ(sys.get_nexus_count(0), 0u);
    ASSERT_EQ(sys.get_nexus_position_count(0), 0u);
}

// =============================================================================
// Entity not registered does nothing
// =============================================================================

TEST(unregistered_entity_does_nothing) {
    EnergySystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    // Create entity but don't register it
    auto entity = registry.create();
    uint32_t eid = static_cast<uint32_t>(entity);

    // Should not crash and should not change any counts
    sys.on_building_deconstructed(eid, 0, 50, 50);

    ASSERT_EQ(sys.get_consumer_count(0), 0u);
    ASSERT_EQ(sys.get_nexus_count(0), 0u);
}

TEST(deconstruct_entity_not_in_any_list) {
    EnergySystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    // Register one consumer
    auto e1 = registry.create();
    uint32_t eid1 = static_cast<uint32_t>(e1);
    EnergyComponent ec1{};
    ec1.energy_required = 50;
    registry.emplace<EnergyComponent>(e1, ec1);
    sys.on_building_constructed(eid1, 0, 10, 10);

    // Try to deconstruct a different entity that was never registered
    auto e2 = registry.create();
    uint32_t eid2 = static_cast<uint32_t>(e2);
    sys.on_building_deconstructed(eid2, 0, 20, 20);

    // The first consumer should still be registered
    ASSERT_EQ(sys.get_consumer_count(0), 1u);
}

// =============================================================================
// Bounds check: invalid owner
// =============================================================================

TEST(invalid_owner_does_nothing) {
    EnergySystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    auto entity = registry.create();
    uint32_t eid = static_cast<uint32_t>(entity);
    EnergyComponent ec{};
    ec.energy_required = 50;
    registry.emplace<EnergyComponent>(entity, ec);

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
// Multiple deconstructed buildings same player
// =============================================================================

TEST(multiple_deconstructions_same_player) {
    EnergySystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    // Create 3 consumers for player 0
    uint32_t eids[3];
    for (int i = 0; i < 3; ++i) {
        auto entity = registry.create();
        eids[i] = static_cast<uint32_t>(entity);
        EnergyComponent ec{};
        ec.energy_required = 10 * (i + 1);
        registry.emplace<EnergyComponent>(entity, ec);
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
// Multiple deconstructed buildings different players
// =============================================================================

TEST(multiple_deconstructions_different_players) {
    EnergySystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    // Player 0: consumer
    auto e0 = registry.create();
    uint32_t eid0 = static_cast<uint32_t>(e0);
    EnergyComponent ec0{};
    ec0.energy_required = 50;
    registry.emplace<EnergyComponent>(e0, ec0);
    sys.on_building_constructed(eid0, 0, 10, 10);

    // Player 1: nexus
    auto e1 = registry.create();
    uint32_t eid1 = static_cast<uint32_t>(e1);
    EnergyProducerComponent prod1{};
    prod1.base_output = 300;
    prod1.current_output = 300;
    prod1.nexus_type = static_cast<uint8_t>(NexusType::Carbon);
    prod1.is_online = true;
    registry.emplace<EnergyProducerComponent>(e1, prod1);
    sys.on_building_constructed(eid1, 1, 20, 20);

    ASSERT_EQ(sys.get_consumer_count(0), 1u);
    ASSERT_EQ(sys.get_nexus_count(1), 1u);

    // Deconstruct player 0's consumer
    sys.on_building_deconstructed(eid0, 0, 10, 10);
    ASSERT_EQ(sys.get_consumer_count(0), 0u);
    ASSERT_EQ(sys.get_nexus_count(1), 1u); // player 1 unaffected

    // Deconstruct player 1's nexus
    sys.on_building_deconstructed(eid1, 1, 20, 20);
    ASSERT_EQ(sys.get_nexus_count(1), 0u);
}

// =============================================================================
// Double deconstruction does not crash
// =============================================================================

TEST(double_deconstruction_no_crash) {
    EnergySystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    auto entity = registry.create();
    uint32_t eid = static_cast<uint32_t>(entity);
    EnergyComponent ec{};
    ec.energy_required = 50;
    registry.emplace<EnergyComponent>(entity, ec);

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
    EnergySystem sys(128, 128);
    // No registry set - but on_building_deconstructed doesn't need registry
    // It only checks internal lists
    sys.on_building_deconstructed(42, 0, 10, 10);
    ASSERT_EQ(sys.get_consumer_count(0), 0u);
    ASSERT_EQ(sys.get_nexus_count(0), 0u);
}

// =============================================================================
// Coverage not dirty after consumer deconstruction
// =============================================================================

TEST(coverage_not_dirty_after_consumer_deconstruct) {
    EnergySystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    auto entity = registry.create();
    uint32_t eid = static_cast<uint32_t>(entity);
    EnergyComponent ec{};
    ec.energy_required = 50;
    registry.emplace<EnergyComponent>(entity, ec);

    sys.on_building_constructed(eid, 0, 10, 10);

    // Clear dirty flag
    sys.recalculate_coverage(0);
    ASSERT(!sys.is_coverage_dirty(0));

    // Deconstruct consumer - should NOT mark coverage dirty
    // (consumers don't affect coverage, only nexuses do)
    sys.on_building_deconstructed(eid, 0, 10, 10);
    ASSERT(!sys.is_coverage_dirty(0));
}

// =============================================================================
// Main Entry Point
// =============================================================================

int main() {
    printf("=== BuildingDeconstructedEvent Handler Unit Tests (Ticket 5-033) ===\n\n");

    // Consumer unregistration
    RUN_TEST(unregisters_consumer_on_deconstruct);
    RUN_TEST(consumer_no_longer_in_aggregation_after_deconstruct);

    // Nexus unregistration
    RUN_TEST(unregisters_nexus_on_deconstruct);
    RUN_TEST(nexus_deconstruction_marks_coverage_dirty);
    RUN_TEST(nexus_generation_removed_after_deconstruct);

    // Both components
    RUN_TEST(entity_with_both_components_unregisters_both);

    // Unregistered entity
    RUN_TEST(unregistered_entity_does_nothing);
    RUN_TEST(deconstruct_entity_not_in_any_list);

    // Invalid owner
    RUN_TEST(invalid_owner_does_nothing);

    // Multiple deconstructions
    RUN_TEST(multiple_deconstructions_same_player);
    RUN_TEST(multiple_deconstructions_different_players);

    // Edge cases
    RUN_TEST(double_deconstruction_no_crash);
    RUN_TEST(deconstruct_without_registry_no_crash);
    RUN_TEST(coverage_not_dirty_after_consumer_deconstruct);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
