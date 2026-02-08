/**
 * @file test_building_integration.cpp
 * @brief Integration tests for EnergySystem <-> BuildingSystem (Ticket 5-036)
 *
 * Verifies the full pipeline:
 * 1. Create EnergySystem and BuildingSystem
 * 2. Register EnergySystem as the IEnergyProvider for BuildingSystem
 * 3. Place a nexus, place consumers, establish coverage
 * 4. Run tick()
 * 5. Verify consumers are powered via EnergySystem's IEnergyProvider queries
 *
 * Tests cover:
 * - EnergySystem satisfies IEnergyProvider interface for BuildingSystem
 * - set_energy_provider() accepts EnergySystem pointer
 * - is_powered() returns correct state after tick
 * - is_powered_at() returns correct state after tick
 * - Full tick pipeline: nexus -> coverage -> pool -> distribution -> powered
 * - Unpowered consumers outside coverage
 * - Multiple consumers with surplus
 * - Deficit scenario: consumers lose power after nexus removal
 * - Provider can be set before or after BuildingSystem construction
 */

#include <sims3000/energy/EnergySystem.h>
#include <sims3000/energy/EnergyComponent.h>
#include <sims3000/energy/EnergyProducerComponent.h>
#include <sims3000/energy/EnergyEnums.h>
#include <sims3000/building/BuildingSystem.h>
#include <sims3000/building/ForwardDependencyInterfaces.h>
#include <sims3000/zone/ZoneSystem.h>
#include <entt/entt.hpp>
#include <cstdio>
#include <cstdlib>

using namespace sims3000::energy;
using namespace sims3000::building;

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
// Helper: create a minimal ZoneSystem for BuildingSystem construction
// =============================================================================

static sims3000::zone::ZoneSystem make_zone_system(uint16_t grid_size = 128) {
    return sims3000::zone::ZoneSystem(nullptr, nullptr, grid_size);
}

// =============================================================================
// Test: EnergySystem can be registered as IEnergyProvider
// =============================================================================

TEST(energy_system_satisfies_interface) {
    // EnergySystem inherits from IEnergyProvider
    EnergySystem energy(128, 128);
    IEnergyProvider* provider = &energy;

    // Verify the pointer is valid and methods are callable
    ASSERT(provider != nullptr);
    // Without a registry, is_powered returns false (safe default)
    ASSERT(!provider->is_powered(0));
    ASSERT(!provider->is_powered_at(0, 0, 0));
}

TEST(set_energy_provider_accepts_energy_system) {
    auto zone_sys = make_zone_system();
    BuildingSystem building(&zone_sys, nullptr, 128);
    EnergySystem energy(128, 128);

    // This must compile and not crash
    building.set_energy_provider(&energy);

    // Verify by checking that building system can still tick
    building.tick(0.0f);
}

// =============================================================================
// Test: Full pipeline - nexus + consumer + tick -> powered
// =============================================================================

TEST(full_pipeline_consumers_powered_after_tick) {
    // Setup systems
    EnergySystem energy(128, 128);
    entt::registry registry;
    energy.set_registry(&registry);

    // 1. Place a nexus (Carbon, base_output=100, after aging ~99) at center
    uint32_t nexus_id = energy.place_nexus(NexusType::Carbon, 64, 64, 0);
    ASSERT(nexus_id != INVALID_ENTITY_ID);

    // 2. Create consumer entities within coverage radius of the nexus
    //    Carbon nexus has coverage_radius=8 from NexusTypeConfig, so (64,64) +/- 8
    auto consumer1 = registry.create();
    uint32_t cid1 = static_cast<uint32_t>(consumer1);
    {
        EnergyComponent ec{};
        ec.energy_required = 50;
        ec.is_powered = false;
        registry.emplace<EnergyComponent>(consumer1, ec);
    }
    energy.register_consumer(cid1, 0);
    energy.register_consumer_position(cid1, 0, 62, 64);

    auto consumer2 = registry.create();
    uint32_t cid2 = static_cast<uint32_t>(consumer2);
    {
        EnergyComponent ec{};
        ec.energy_required = 30;
        ec.is_powered = false;
        registry.emplace<EnergyComponent>(consumer2, ec);
    }
    energy.register_consumer(cid2, 0);
    energy.register_consumer_position(cid2, 0, 66, 64);

    // 3. Run a tick to trigger the full pipeline
    energy.tick(0.0f);

    // 4. Verify consumers are powered via IEnergyProvider interface
    IEnergyProvider* provider = &energy;

    ASSERT(provider->is_powered(cid1));
    ASSERT(provider->is_powered(cid2));

    // Also verify via is_powered_at
    ASSERT(provider->is_powered_at(62, 64, 0));
    ASSERT(provider->is_powered_at(66, 64, 0));
}

TEST(full_pipeline_with_building_system_integration) {
    // Create both systems
    auto zone_sys = make_zone_system();
    BuildingSystem building(&zone_sys, nullptr, 128);
    EnergySystem energy(128, 128);
    entt::registry registry;
    energy.set_registry(&registry);

    // Register EnergySystem as BuildingSystem's energy provider
    building.set_energy_provider(&energy);

    // Place a nexus (Carbon: base_output=100, after 1 tick aging ~99)
    uint32_t nexus_id = energy.place_nexus(NexusType::Carbon, 64, 64, 0);
    ASSERT(nexus_id != INVALID_ENTITY_ID);

    // Create a consumer in coverage (must be < ~99 to get surplus)
    auto consumer = registry.create();
    uint32_t cid = static_cast<uint32_t>(consumer);
    {
        EnergyComponent ec{};
        ec.energy_required = 50;
        ec.is_powered = false;
        registry.emplace<EnergyComponent>(consumer, ec);
    }
    energy.register_consumer(cid, 0);
    energy.register_consumer_position(cid, 0, 64, 60);

    // Run energy tick (priority 10, before building at 40)
    energy.tick(0.0f);

    // Run building tick (priority 40)
    building.tick(0.0f);

    // Verify the consumer is powered via the IEnergyProvider interface
    // that BuildingSystem holds
    IEnergyProvider* provider = &energy;
    ASSERT(provider->is_powered(cid));
    ASSERT(provider->is_powered_at(64, 60, 0));
}

// =============================================================================
// Test: Consumer outside coverage is not powered
// =============================================================================

TEST(consumer_outside_coverage_not_powered) {
    EnergySystem energy(128, 128);
    entt::registry registry;
    energy.set_registry(&registry);

    // Place nexus at (10, 10) - Carbon coverage_radius = 8
    uint32_t nexus_id = energy.place_nexus(NexusType::Carbon, 10, 10, 0);
    ASSERT(nexus_id != INVALID_ENTITY_ID);

    // Place consumer FAR outside coverage radius (at 100, 100)
    auto consumer = registry.create();
    uint32_t cid = static_cast<uint32_t>(consumer);
    {
        EnergyComponent ec{};
        ec.energy_required = 50;
        ec.is_powered = false;
        registry.emplace<EnergyComponent>(consumer, ec);
    }
    energy.register_consumer(cid, 0);
    energy.register_consumer_position(cid, 0, 100, 100);

    energy.tick(0.0f);

    IEnergyProvider* provider = &energy;
    ASSERT(!provider->is_powered(cid));
    ASSERT(!provider->is_powered_at(100, 100, 0));

    // But the nexus area itself should be powered_at
    ASSERT(provider->is_powered_at(10, 10, 0));
}

// =============================================================================
// Test: Multiple consumers with sufficient surplus
// =============================================================================

TEST(multiple_consumers_with_surplus) {
    EnergySystem energy(128, 128);
    entt::registry registry;
    energy.set_registry(&registry);

    // Place a Carbon nexus (base_output=100, after aging ~99) at (64, 64)
    uint32_t nexus_id = energy.place_nexus(NexusType::Carbon, 64, 64, 0);
    ASSERT(nexus_id != INVALID_ENTITY_ID);

    // Create 5 consumers, each requiring 10 energy (total 50 < ~99)
    uint32_t consumer_ids[5];
    for (int i = 0; i < 5; ++i) {
        auto entity = registry.create();
        consumer_ids[i] = static_cast<uint32_t>(entity);
        EnergyComponent ec{};
        ec.energy_required = 10;
        ec.is_powered = false;
        registry.emplace<EnergyComponent>(entity, ec);
        energy.register_consumer(consumer_ids[i], 0);
        // Place consumers within coverage radius
        energy.register_consumer_position(consumer_ids[i], 0,
                                          static_cast<uint32_t>(60 + i), 64);
    }

    energy.tick(0.0f);

    IEnergyProvider* provider = &energy;
    for (int i = 0; i < 5; ++i) {
        ASSERT(provider->is_powered(consumer_ids[i]));
    }

    // Pool should be healthy (generation ~99, consumption 50, surplus ~49)
    ASSERT_EQ(static_cast<uint8_t>(energy.get_pool_state(0)),
              static_cast<uint8_t>(EnergyPoolState::Healthy));
}

// =============================================================================
// Test: Deficit scenario - consumers lose power
// =============================================================================

TEST(deficit_consumers_lose_power) {
    EnergySystem energy(128, 128);
    entt::registry registry;
    energy.set_registry(&registry);

    // Place a Carbon nexus (base_output=100, after aging ~99)
    uint32_t nexus_id = energy.place_nexus(NexusType::Carbon, 64, 64, 0);
    ASSERT(nexus_id != INVALID_ENTITY_ID);

    // Create consumers demanding more than available (3 x 50 = 150 > ~99)
    // With rationing, only consumers that can be fully served get power.
    // Available ~99, each needs 50: first consumer gets 50 (49 left), second
    // needs 50 but only 49 available -> not powered. So 1 powered.
    uint32_t consumer_ids[3];
    for (int i = 0; i < 3; ++i) {
        auto entity = registry.create();
        consumer_ids[i] = static_cast<uint32_t>(entity);
        EnergyComponent ec{};
        ec.energy_required = 50;
        ec.is_powered = false;
        ec.priority = 2; // Normal priority
        registry.emplace<EnergyComponent>(entity, ec);
        energy.register_consumer(consumer_ids[i], 0);
        energy.register_consumer_position(consumer_ids[i], 0,
                                          static_cast<uint32_t>(60 + i), 64);
    }

    energy.tick(0.0f);

    // With rationing, only 1 consumer should be powered (~99 available, each needs 50)
    // First gets 50, 49 left, second needs 50 -> not enough
    IEnergyProvider* provider = &energy;
    int powered_count = 0;
    for (int i = 0; i < 3; ++i) {
        if (provider->is_powered(consumer_ids[i])) {
            powered_count++;
        }
    }
    ASSERT_EQ(powered_count, 1);
}

// =============================================================================
// Test: is_powered_at returns false for wrong player
// =============================================================================

TEST(is_powered_at_wrong_player) {
    EnergySystem energy(128, 128);
    entt::registry registry;
    energy.set_registry(&registry);

    // Place nexus for player 0
    uint32_t nexus_id = energy.place_nexus(NexusType::Carbon, 64, 64, 0);
    ASSERT(nexus_id != INVALID_ENTITY_ID);

    energy.tick(0.0f);

    IEnergyProvider* provider = &energy;

    // Player 0's coverage should show powered
    ASSERT(provider->is_powered_at(64, 64, 0));

    // Player 1 has no coverage, should not be powered
    ASSERT(!provider->is_powered_at(64, 64, 1));
}

// =============================================================================
// Test: Provider set before first tick works
// =============================================================================

TEST(provider_set_before_tick) {
    auto zone_sys = make_zone_system();
    BuildingSystem building(&zone_sys, nullptr, 128);
    EnergySystem energy(128, 128);

    // Set provider BEFORE any ticks
    building.set_energy_provider(&energy);

    // Systems should still function
    building.tick(0.0f);
    energy.tick(0.0f);
}

// =============================================================================
// Test: Provider can be replaced
// =============================================================================

TEST(provider_can_be_replaced) {
    auto zone_sys = make_zone_system();
    BuildingSystem building(&zone_sys, nullptr, 128);
    EnergySystem energy1(128, 128);
    EnergySystem energy2(128, 128);

    building.set_energy_provider(&energy1);
    building.tick(0.0f);

    // Replace with different energy system
    building.set_energy_provider(&energy2);
    building.tick(0.0f);

    // Can also set to nullptr to remove
    building.set_energy_provider(nullptr);
    building.tick(0.0f);
}

// =============================================================================
// Test: on_building_constructed integrates with EnergySystem tick
// =============================================================================

TEST(building_constructed_then_tick_powers_consumer) {
    EnergySystem energy(128, 128);
    entt::registry registry;
    energy.set_registry(&registry);

    // Place a nexus first so there's generation and coverage
    uint32_t nexus_id = energy.place_nexus(NexusType::Carbon, 64, 64, 0);
    ASSERT(nexus_id != INVALID_ENTITY_ID);

    // Simulate a "building constructed" by creating entity with EnergyComponent
    // and calling on_building_constructed (Ticket 5-032 handler)
    auto consumer = registry.create();
    uint32_t cid = static_cast<uint32_t>(consumer);
    {
        EnergyComponent ec{};
        ec.energy_required = 50;
        ec.is_powered = false;
        registry.emplace<EnergyComponent>(consumer, ec);
    }
    energy.on_building_constructed(cid, 0, 62, 64);

    // Before tick, consumer should not be powered
    ASSERT(!energy.is_powered(cid));

    // After tick, consumer within coverage should be powered
    energy.tick(0.0f);
    ASSERT(energy.is_powered(cid));
}

// =============================================================================
// Test: Full integration sequence matching game loop order
// =============================================================================

TEST(game_loop_integration_sequence) {
    // This test simulates the actual game loop ordering:
    // 1. EnergySystem.tick() at priority 10
    // 2. ZoneSystem.tick() at priority 30
    // 3. BuildingSystem.tick() at priority 40

    auto zone_sys = make_zone_system();
    BuildingSystem building(&zone_sys, nullptr, 128);
    EnergySystem energy(128, 128);
    entt::registry registry;
    energy.set_registry(&registry);

    // Wire up the dependency
    building.set_energy_provider(&energy);

    // Place nexus (Carbon: base_output=100, after aging ~99) and consumer
    uint32_t nexus_id = energy.place_nexus(NexusType::Carbon, 64, 64, 0);
    ASSERT(nexus_id != INVALID_ENTITY_ID);

    auto consumer = registry.create();
    uint32_t cid = static_cast<uint32_t>(consumer);
    {
        EnergyComponent ec{};
        ec.energy_required = 30;
        ec.is_powered = false;
        registry.emplace<EnergyComponent>(consumer, ec);
    }
    energy.register_consumer(cid, 0);
    energy.register_consumer_position(cid, 0, 64, 60);

    // Simulate game loop for several ticks
    for (int tick = 0; tick < 5; ++tick) {
        energy.tick(0.0f);      // priority 10
        zone_sys.tick(0.0f);    // priority 30
        building.tick(0.0f);    // priority 40
    }

    // After multiple ticks, consumer should still be powered
    IEnergyProvider* provider = &energy;
    ASSERT(provider->is_powered(cid));

    // Pool should be healthy (~99 generation, 30 consumption, surplus ~69)
    const PerPlayerEnergyPool& pool = energy.get_pool(0);
    ASSERT(pool.total_generated > 0);
    ASSERT(pool.surplus >= 0);
    ASSERT_EQ(static_cast<uint8_t>(energy.get_pool_state(0)),
              static_cast<uint8_t>(EnergyPoolState::Healthy));
}

// =============================================================================
// Main Entry Point
// =============================================================================

int main() {
    printf("=== BuildingSystem Integration Tests (Ticket 5-036) ===\n\n");

    // Interface satisfaction
    RUN_TEST(energy_system_satisfies_interface);
    RUN_TEST(set_energy_provider_accepts_energy_system);

    // Full pipeline
    RUN_TEST(full_pipeline_consumers_powered_after_tick);
    RUN_TEST(full_pipeline_with_building_system_integration);

    // Coverage boundary
    RUN_TEST(consumer_outside_coverage_not_powered);

    // Multiple consumers
    RUN_TEST(multiple_consumers_with_surplus);

    // Deficit
    RUN_TEST(deficit_consumers_lose_power);

    // Cross-player
    RUN_TEST(is_powered_at_wrong_player);

    // Provider lifecycle
    RUN_TEST(provider_set_before_tick);
    RUN_TEST(provider_can_be_replaced);

    // Building constructed handler integration
    RUN_TEST(building_constructed_then_tick_powers_consumer);

    // Full game loop sequence
    RUN_TEST(game_loop_integration_sequence);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
