/**
 * @file test_building_event_handler.cpp
 * @brief Unit tests for BuildingConstructedEvent handler (Ticket 5-032)
 *
 * Tests cover:
 * - Consumer registration when entity has EnergyComponent
 * - Consumer position registration when entity has EnergyComponent
 * - Nexus registration when entity has EnergyProducerComponent
 * - Nexus position registration when entity has EnergyProducerComponent
 * - Coverage dirty flag set when nexus is registered
 * - Entity with both components registers as both consumer and nexus
 * - Entity with neither component does nothing
 * - No-op with null registry
 * - No-op with invalid owner
 * - No-op with invalid entity
 * - Multiple buildings for different players
 */

#include <sims3000/energy/EnergySystem.h>
#include <sims3000/energy/EnergyComponent.h>
#include <sims3000/energy/EnergyProducerComponent.h>
#include <sims3000/energy/EnergyConduitComponent.h>
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
// Consumer registration
// =============================================================================

TEST(registers_consumer_when_entity_has_energy_component) {
    EnergySystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    // Create an entity with EnergyComponent (consumer)
    auto entity = registry.create();
    uint32_t eid = static_cast<uint32_t>(entity);
    EnergyComponent ec{};
    ec.energy_required = 50;
    registry.emplace<EnergyComponent>(entity, ec);

    ASSERT_EQ(sys.get_consumer_count(0), 0u);
    ASSERT_EQ(sys.get_consumer_position_count(0), 0u);

    sys.on_building_constructed(eid, 0, 20, 30);

    // Should be registered as consumer
    ASSERT_EQ(sys.get_consumer_count(0), 1u);
    ASSERT_EQ(sys.get_consumer_position_count(0), 1u);
}

TEST(consumer_position_is_correct) {
    EnergySystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    auto entity = registry.create();
    uint32_t eid = static_cast<uint32_t>(entity);
    EnergyComponent ec{};
    ec.energy_required = 100;
    registry.emplace<EnergyComponent>(entity, ec);

    sys.on_building_constructed(eid, 0, 25, 35);

    // Place a nexus nearby so coverage includes the consumer position
    auto nexus = registry.create();
    EnergyProducerComponent prod{};
    prod.base_output = 500;
    prod.current_output = 500;
    prod.nexus_type = static_cast<uint8_t>(NexusType::Carbon);
    prod.is_online = true;
    registry.emplace<EnergyProducerComponent>(nexus, prod);
    sys.register_nexus(static_cast<uint32_t>(nexus), 0);
    sys.register_nexus_position(static_cast<uint32_t>(nexus), 0, 25, 35);

    // Recalculate coverage so consumer is in coverage
    sys.recalculate_coverage(0);

    // aggregate_consumption should find the consumer at its registered position
    uint32_t consumption = sys.aggregate_consumption(0);
    ASSERT_EQ(consumption, 100u);
}

// =============================================================================
// Nexus registration
// =============================================================================

TEST(registers_nexus_when_entity_has_producer_component) {
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

    ASSERT_EQ(sys.get_nexus_count(0), 0u);
    ASSERT_EQ(sys.get_nexus_position_count(0), 0u);

    sys.on_building_constructed(eid, 0, 40, 50);

    // Should be registered as nexus
    ASSERT_EQ(sys.get_nexus_count(0), 1u);
    ASSERT_EQ(sys.get_nexus_position_count(0), 1u);
}

TEST(nexus_registration_marks_coverage_dirty) {
    EnergySystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    // Clear dirty flag first by recalculating (even with no nexuses)
    sys.recalculate_coverage(0);
    ASSERT(!sys.is_coverage_dirty(0));

    auto entity = registry.create();
    uint32_t eid = static_cast<uint32_t>(entity);
    EnergyProducerComponent prod{};
    prod.base_output = 200;
    prod.nexus_type = static_cast<uint8_t>(NexusType::Wind);
    prod.is_online = true;
    registry.emplace<EnergyProducerComponent>(entity, prod);

    sys.on_building_constructed(eid, 0, 30, 30);

    // Coverage should be dirty after nexus registration
    ASSERT(sys.is_coverage_dirty(0));
}

TEST(nexus_generation_available_after_registration) {
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

    // get_total_generation should now include this nexus
    uint32_t gen = sys.get_total_generation(0);
    ASSERT_EQ(gen, 400u);
}

// =============================================================================
// Entity with both consumer and producer components
// =============================================================================

TEST(entity_with_both_components_registers_as_both) {
    EnergySystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    auto entity = registry.create();
    uint32_t eid = static_cast<uint32_t>(entity);

    // Add both EnergyComponent and EnergyProducerComponent
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

    // Should be both consumer and nexus
    ASSERT_EQ(sys.get_consumer_count(0), 1u);
    ASSERT_EQ(sys.get_consumer_position_count(0), 1u);
    ASSERT_EQ(sys.get_nexus_count(0), 1u);
    ASSERT_EQ(sys.get_nexus_position_count(0), 1u);
}

// =============================================================================
// Entity with neither component
// =============================================================================

TEST(entity_with_no_energy_components_does_nothing) {
    EnergySystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    // Create entity with no energy components
    auto entity = registry.create();
    uint32_t eid = static_cast<uint32_t>(entity);

    sys.on_building_constructed(eid, 0, 50, 50);

    // Nothing should be registered
    ASSERT_EQ(sys.get_consumer_count(0), 0u);
    ASSERT_EQ(sys.get_consumer_position_count(0), 0u);
    ASSERT_EQ(sys.get_nexus_count(0), 0u);
    ASSERT_EQ(sys.get_nexus_position_count(0), 0u);
}

// =============================================================================
// No-op cases
// =============================================================================

TEST(noop_with_null_registry) {
    EnergySystem sys(128, 128);
    // No registry set
    sys.on_building_constructed(42, 0, 10, 10);
    // Should not crash, no registrations
    ASSERT_EQ(sys.get_consumer_count(0), 0u);
    ASSERT_EQ(sys.get_nexus_count(0), 0u);
}

TEST(noop_for_invalid_owner) {
    EnergySystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    auto entity = registry.create();
    uint32_t eid = static_cast<uint32_t>(entity);
    EnergyComponent ec{};
    ec.energy_required = 50;
    registry.emplace<EnergyComponent>(entity, ec);

    // Invalid owner (>= MAX_PLAYERS)
    sys.on_building_constructed(eid, MAX_PLAYERS, 10, 10);
    sys.on_building_constructed(eid, 255, 10, 10);

    // Nothing should be registered for any player
    for (uint8_t i = 0; i < MAX_PLAYERS; ++i) {
        ASSERT_EQ(sys.get_consumer_count(i), 0u);
    }
}

TEST(noop_for_invalid_entity) {
    EnergySystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    // INVALID_ENTITY_ID should not crash
    sys.on_building_constructed(INVALID_ENTITY_ID, 0, 10, 10);
    ASSERT_EQ(sys.get_consumer_count(0), 0u);
    ASSERT_EQ(sys.get_nexus_count(0), 0u);
}

TEST(noop_for_destroyed_entity) {
    EnergySystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    auto entity = registry.create();
    uint32_t eid = static_cast<uint32_t>(entity);
    registry.destroy(entity);

    // Entity no longer valid
    sys.on_building_constructed(eid, 0, 10, 10);
    ASSERT_EQ(sys.get_consumer_count(0), 0u);
    ASSERT_EQ(sys.get_nexus_count(0), 0u);
}

// =============================================================================
// Multiple buildings, different players
// =============================================================================

TEST(multiple_buildings_different_players) {
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

    // Player 2: both
    auto e2 = registry.create();
    uint32_t eid2 = static_cast<uint32_t>(e2);
    EnergyComponent ec2{};
    ec2.energy_required = 25;
    registry.emplace<EnergyComponent>(e2, ec2);
    EnergyProducerComponent prod2{};
    prod2.base_output = 100;
    prod2.current_output = 100;
    prod2.nexus_type = static_cast<uint8_t>(NexusType::Solar);
    prod2.is_online = true;
    registry.emplace<EnergyProducerComponent>(e2, prod2);
    sys.on_building_constructed(eid2, 2, 30, 30);

    // Verify per-player counts
    ASSERT_EQ(sys.get_consumer_count(0), 1u);
    ASSERT_EQ(sys.get_nexus_count(0), 0u);

    ASSERT_EQ(sys.get_consumer_count(1), 0u);
    ASSERT_EQ(sys.get_nexus_count(1), 1u);

    ASSERT_EQ(sys.get_consumer_count(2), 1u);
    ASSERT_EQ(sys.get_nexus_count(2), 1u);

    // Player 3 untouched
    ASSERT_EQ(sys.get_consumer_count(3), 0u);
    ASSERT_EQ(sys.get_nexus_count(3), 0u);
}

// =============================================================================
// Consumer is_powered not immediately set (deferred to next tick)
// =============================================================================

TEST(consumer_not_immediately_powered) {
    EnergySystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    // Create a consumer building
    auto entity = registry.create();
    uint32_t eid = static_cast<uint32_t>(entity);
    EnergyComponent ec{};
    ec.energy_required = 50;
    ec.is_powered = false;
    registry.emplace<EnergyComponent>(entity, ec);

    sys.on_building_constructed(eid, 0, 20, 20);

    // Consumer should NOT be immediately powered (power comes from distribution phase)
    const auto* comp = registry.try_get<EnergyComponent>(entity);
    ASSERT(comp != nullptr);
    ASSERT(!comp->is_powered);
}

// =============================================================================
// Multiple consumers for same player
// =============================================================================

TEST(multiple_consumers_same_player) {
    EnergySystem sys(128, 128);
    entt::registry registry;
    sys.set_registry(&registry);

    for (int i = 0; i < 5; ++i) {
        auto entity = registry.create();
        uint32_t eid = static_cast<uint32_t>(entity);
        EnergyComponent ec{};
        ec.energy_required = 10 * (i + 1);
        registry.emplace<EnergyComponent>(entity, ec);
        sys.on_building_constructed(eid, 0, i * 10, i * 10);
    }

    ASSERT_EQ(sys.get_consumer_count(0), 5u);
    ASSERT_EQ(sys.get_consumer_position_count(0), 5u);
}

// =============================================================================
// Main Entry Point
// =============================================================================

int main() {
    printf("=== BuildingConstructedEvent Handler Unit Tests (Ticket 5-032) ===\n\n");

    // Consumer registration
    RUN_TEST(registers_consumer_when_entity_has_energy_component);
    RUN_TEST(consumer_position_is_correct);

    // Nexus registration
    RUN_TEST(registers_nexus_when_entity_has_producer_component);
    RUN_TEST(nexus_registration_marks_coverage_dirty);
    RUN_TEST(nexus_generation_available_after_registration);

    // Both components
    RUN_TEST(entity_with_both_components_registers_as_both);

    // Neither component
    RUN_TEST(entity_with_no_energy_components_does_nothing);

    // No-op cases
    RUN_TEST(noop_with_null_registry);
    RUN_TEST(noop_for_invalid_owner);
    RUN_TEST(noop_for_invalid_entity);
    RUN_TEST(noop_for_destroyed_entity);

    // Multiple buildings
    RUN_TEST(multiple_buildings_different_players);

    // Deferred power
    RUN_TEST(consumer_not_immediately_powered);
    RUN_TEST(multiple_consumers_same_player);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
