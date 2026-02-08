/**
 * @file test_energy_integration.cpp
 * @brief Integration tests for EnergySystem (Ticket 5-041)
 *
 * Tests end-to-end scenarios involving multiple subsystems working together:
 * - Nexus placement -> generation -> pool update
 * - Consumer registration -> coverage -> power distribution
 * - Conduit extension -> coverage change -> consumer power state
 * - Priority-based rationing under deficit
 * - Multi-player isolation
 * - Building constructed/deconstructed event handlers
 * - Nexus offline toggle and aging degradation
 *
 * @see /docs/epics/epic-5/tickets.md (ticket 5-041)
 */

#include <sims3000/energy/EnergySystem.h>
#include <sims3000/energy/EnergyComponent.h>
#include <sims3000/energy/EnergyProducerComponent.h>
#include <sims3000/energy/EnergyConduitComponent.h>
#include <sims3000/energy/EnergyEnums.h>
#include <sims3000/energy/EnergyPriorities.h>
#include <sims3000/energy/NexusTypeConfig.h>
#include <entt/entt.hpp>
#include <cstdio>
#include <cstdlib>

using namespace sims3000::energy;

// =============================================================================
// Test framework macros
// =============================================================================

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

#define ASSERT_GT(a, b) do { \
    if (!((a) > (b))) { \
        printf("\n  FAILED: %s > %s (line %d)\n", #a, #b, __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

#define ASSERT_LT(a, b) do { \
    if (!((a) < (b))) { \
        printf("\n  FAILED: %s < %s (line %d)\n", #a, #b, __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

// =============================================================================
// Helper: Create a consumer entity with EnergyComponent in the registry
// =============================================================================

static uint32_t create_consumer(entt::registry& reg, uint32_t energy_required,
                                uint8_t priority = ENERGY_PRIORITY_NORMAL) {
    auto entity = reg.create();
    EnergyComponent ec{};
    ec.energy_required = energy_required;
    ec.priority = priority;
    reg.emplace<EnergyComponent>(entity, ec);
    return static_cast<uint32_t>(entity);
}

// =============================================================================
// Test 1: Place nexus, verify pool generation increases
// =============================================================================

TEST(nexus_placement_increases_generation) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    // Pool starts with 0 generation
    ASSERT_EQ(sys.get_pool(0).total_generated, 0u);

    // Place a Carbon nexus at (10, 10) for player 0
    uint32_t nexus_id = sys.place_nexus(NexusType::Carbon, 10, 10, 0);
    ASSERT(nexus_id != INVALID_ENTITY_ID);

    // Tick so the system updates outputs, coverage, and pool
    sys.tick(0.05f);

    // Pool generation should be > 0 now (Carbon base_output=100)
    const auto& pool = sys.get_pool(0);
    ASSERT_GT(pool.total_generated, 0u);
    ASSERT_EQ(pool.nexus_count, 1u);
}

// =============================================================================
// Test 2: Place consumer in coverage, verify pool consumption increases
// =============================================================================

TEST(consumer_in_coverage_increases_consumption) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    // Place a nexus at (10, 10) for player 0 (coverage_radius=8 for Carbon)
    uint32_t nexus_id = sys.place_nexus(NexusType::Carbon, 10, 10, 0);
    ASSERT(nexus_id != INVALID_ENTITY_ID);

    // Create a consumer entity that requires 20 energy
    uint32_t consumer_id = create_consumer(reg, 20);

    // Register consumer at position (12, 12) - within coverage radius of nexus at (10,10)
    sys.register_consumer(consumer_id, 0);
    sys.register_consumer_position(consumer_id, 0, 12, 12);

    // Tick
    sys.tick(0.05f);

    // Pool should show consumption
    const auto& pool = sys.get_pool(0);
    ASSERT_GT(pool.total_consumed, 0u);
    ASSERT_EQ(pool.total_consumed, 20u);
}

// =============================================================================
// Test 3: Consumer in coverage + pool surplus -> is_powered = true after tick()
// =============================================================================

TEST(consumer_powered_with_surplus) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    // Place a Carbon nexus (base_output=100) at (10, 10)
    sys.place_nexus(NexusType::Carbon, 10, 10, 0);

    // Create a consumer that requires 20 energy (well within surplus)
    uint32_t consumer_id = create_consumer(reg, 20);
    sys.register_consumer(consumer_id, 0);
    sys.register_consumer_position(consumer_id, 0, 12, 12);

    // Before tick: consumer should not be powered
    ASSERT(!sys.is_powered(consumer_id));

    // Tick
    sys.tick(0.05f);

    // After tick: consumer should be powered
    ASSERT(sys.is_powered(consumer_id));

    // Verify energy_received
    ASSERT_EQ(sys.get_energy_received(consumer_id), 20u);
}

// =============================================================================
// Test 4: Consumer outside coverage -> is_powered = false
// =============================================================================

TEST(consumer_outside_coverage_not_powered) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    // Place a Carbon nexus at (10, 10) with coverage_radius=8
    sys.place_nexus(NexusType::Carbon, 10, 10, 0);

    // Create a consumer far outside coverage at (50, 50)
    uint32_t consumer_id = create_consumer(reg, 10);
    sys.register_consumer(consumer_id, 0);
    sys.register_consumer_position(consumer_id, 0, 50, 50);

    // Tick
    sys.tick(0.05f);

    // Consumer should NOT be powered (outside coverage)
    ASSERT(!sys.is_powered(consumer_id));
    ASSERT_EQ(sys.get_energy_received(consumer_id), 0u);
}

// =============================================================================
// Test 5: Pool deficit -> priority rationing (critical powered first)
// =============================================================================

TEST(deficit_priority_rationing) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    // Place a Carbon nexus at (10, 10) - base_output=100
    // After aging tick 1, output will be close to 100 (very slight decay)
    sys.place_nexus(NexusType::Carbon, 10, 10, 0);

    // Tick once to compute initial output
    sys.tick(0.05f);

    uint32_t generation = sys.get_pool(0).total_generated;
    // generation should be ~99 (100 * 1.0 * age_factor_after_1_tick)
    ASSERT_GT(generation, 0u);

    // Create critical priority consumer requiring 40 energy (within nexus coverage)
    uint32_t critical_id = create_consumer(reg, 40, ENERGY_PRIORITY_CRITICAL);
    sys.register_consumer(critical_id, 0);
    sys.register_consumer_position(critical_id, 0, 11, 10);

    // Create low priority consumer requiring 40 energy (within nexus coverage)
    uint32_t low_id = create_consumer(reg, 40, ENERGY_PRIORITY_LOW);
    sys.register_consumer(low_id, 0);
    sys.register_consumer_position(low_id, 0, 12, 10);

    // Create another low priority consumer requiring 40 energy (total demand=120 > ~99 generation)
    uint32_t low2_id = create_consumer(reg, 40, ENERGY_PRIORITY_LOW);
    sys.register_consumer(low2_id, 0);
    sys.register_consumer_position(low2_id, 0, 13, 10);

    // Tick to trigger distribution with deficit
    sys.tick(0.05f);

    // Critical consumer should be powered (allocated first)
    ASSERT(sys.is_powered(critical_id));
    ASSERT_EQ(sys.get_energy_received(critical_id), 40u);

    // At least one low-priority consumer should be powered too (40+40=80 < ~99)
    // But with total demand 120 > ~99, the third consumer should be unpowered
    // Note: the two low-priority consumers are sorted by entity_id,
    // so the one with lower entity_id gets powered first
    bool low_powered = sys.is_powered(low_id);
    bool low2_powered = sys.is_powered(low2_id);

    // With ~99 available: critical(40) + first_low(40) = 80 < 99, so first_low gets powered
    // Then 99-80 = ~19 < 40 required by second_low, so second_low is unpowered
    ASSERT(low_powered || low2_powered);  // At least one of them is powered
    ASSERT(!(low_powered && low2_powered));  // But not both (deficit)
}

// =============================================================================
// Test 6: Place conduit to extend coverage -> consumer becomes powered
// =============================================================================

TEST(conduit_extends_coverage_to_consumer) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    // Place a Carbon nexus at (10, 10) coverage_radius=8 -> covers (2..18, 2..18)
    sys.place_nexus(NexusType::Carbon, 10, 10, 0);

    // Create a consumer at (25, 10) - outside nexus coverage
    uint32_t consumer_id = create_consumer(reg, 10);
    sys.register_consumer(consumer_id, 0);
    sys.register_consumer_position(consumer_id, 0, 25, 10);

    // Tick - consumer should NOT be powered (out of coverage)
    sys.tick(0.05f);
    ASSERT(!sys.is_powered(consumer_id));

    // BFS walks tile-by-tile checking 4-adjacent neighbors for conduits.
    // We need a continuous chain of conduits from nexus adjacency outward.
    // Place conduits at x=11,12,...,22 along y=10.
    // Nexus at (10,10) -> BFS finds conduit at (11,10) -> (12,10) -> ... -> (22,10)
    // Conduit at (22,10) has coverage_radius=3, covering (19..25, 7..13)
    // -> includes consumer at (25,10)
    for (uint32_t x = 11; x <= 22; ++x) {
        sys.place_conduit(x, 10, 0);
    }

    // Tick - coverage should now extend to consumer's position
    sys.tick(0.05f);

    // Consumer should now be powered
    ASSERT(sys.is_powered(consumer_id));
    ASSERT_EQ(sys.get_energy_received(consumer_id), 10u);
}

// =============================================================================
// Test 7: Remove conduit -> consumer loses coverage and power
// =============================================================================

TEST(remove_conduit_loses_coverage) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    // Place nexus at (10, 10)
    sys.place_nexus(NexusType::Carbon, 10, 10, 0);

    // Place continuous conduit chain from x=11..22 along y=10
    // Store the first conduit entity for removal
    uint32_t c_first = sys.place_conduit(11, 10, 0);
    for (uint32_t x = 12; x <= 22; ++x) {
        sys.place_conduit(x, 10, 0);
    }

    // Place consumer at (25, 10) - reachable through conduit chain
    // Last conduit at (22,10) has coverage_radius=3 -> covers (19..25, 7..13)
    uint32_t consumer_id = create_consumer(reg, 10);
    sys.register_consumer(consumer_id, 0);
    sys.register_consumer_position(consumer_id, 0, 25, 10);

    // Tick - consumer should be powered
    sys.tick(0.05f);
    ASSERT(sys.is_powered(consumer_id));

    // Remove the first conduit in the chain (x=11) - breaks connectivity
    // Without (11,10), BFS can't reach (12,10) and beyond from nexus at (10,10)
    bool removed = sys.remove_conduit(c_first, 0, 11, 10);
    ASSERT(removed);

    // Tick - coverage should shrink, consumer loses power
    sys.tick(0.05f);
    ASSERT(!sys.is_powered(consumer_id));
}

// =============================================================================
// Test 8: Set nexus offline -> generation drops to 0
// =============================================================================

TEST(nexus_offline_zero_generation) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    // Place nexus
    uint32_t nexus_id = sys.place_nexus(NexusType::Carbon, 10, 10, 0);
    ASSERT(nexus_id != INVALID_ENTITY_ID);

    // Tick to get initial generation
    sys.tick(0.05f);
    ASSERT_GT(sys.get_pool(0).total_generated, 0u);

    // Set nexus offline by modifying the component directly
    auto entity = static_cast<entt::entity>(nexus_id);
    auto* producer = reg.try_get<EnergyProducerComponent>(entity);
    ASSERT(producer != nullptr);
    producer->is_online = false;

    // Tick again
    sys.tick(0.05f);

    // Generation should be 0
    ASSERT_EQ(sys.get_pool(0).total_generated, 0u);
}

// =============================================================================
// Test 9: Nexus aging over many ticks reduces output
// =============================================================================

TEST(nexus_aging_reduces_output) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    // Place a Carbon nexus
    uint32_t nexus_id = sys.place_nexus(NexusType::Carbon, 10, 10, 0);
    ASSERT(nexus_id != INVALID_ENTITY_ID);

    // Tick once to get initial output
    sys.tick(0.05f);
    uint32_t initial_gen = sys.get_pool(0).total_generated;
    ASSERT_GT(initial_gen, 0u);

    // Advance aging significantly by setting ticks_since_built
    auto entity = static_cast<entt::entity>(nexus_id);
    auto* producer = reg.try_get<EnergyProducerComponent>(entity);
    ASSERT(producer != nullptr);

    // Manually age the nexus to 10000 ticks
    producer->ticks_since_built = 10000;

    // Tick to recalculate with new age
    sys.tick(0.05f);
    uint32_t aged_gen = sys.get_pool(0).total_generated;

    // Output should be lower after aging
    ASSERT_LT(aged_gen, initial_gen);
    // But still above 0 (aging floor prevents going to 0)
    ASSERT_GT(aged_gen, 0u);
}

// =============================================================================
// Test 10: Multi-player isolation
// =============================================================================

TEST(multiplayer_isolation) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    // Player 0: Place nexus with small output, and heavy consumers -> deficit
    sys.place_nexus(NexusType::Carbon, 10, 10, 0);  // ~100 output

    uint32_t p0_consumer = create_consumer(reg, 200);  // demand exceeds supply
    sys.register_consumer(p0_consumer, 0);
    sys.register_consumer_position(p0_consumer, 0, 12, 10);

    // Player 1: Place nexus with excess capacity, one small consumer -> surplus
    sys.place_nexus(NexusType::Carbon, 40, 40, 1);  // ~100 output

    uint32_t p1_consumer = create_consumer(reg, 10);  // small demand
    sys.register_consumer(p1_consumer, 1);
    sys.register_consumer_position(p1_consumer, 1, 42, 40);

    // Tick
    sys.tick(0.05f);

    // Player 0 is in deficit - consumer should NOT be powered (200 > ~99 generation)
    ASSERT(!sys.is_powered(p0_consumer));

    // Player 1 has surplus - consumer should be powered
    ASSERT(sys.is_powered(p1_consumer));

    // Verify pools are independent
    const auto& pool0 = sys.get_pool(0);
    const auto& pool1 = sys.get_pool(1);
    ASSERT_LT(pool0.surplus, 0);  // Player 0 in deficit
    ASSERT_GT(pool1.surplus, 0);  // Player 1 has surplus
}

// =============================================================================
// Test 11: Building constructed event registers consumer, next tick powers it
// =============================================================================

TEST(building_constructed_registers_consumer) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    // Place a nexus to provide power at (10, 10)
    sys.place_nexus(NexusType::Carbon, 10, 10, 0);

    // Tick once to establish coverage and generation
    sys.tick(0.05f);

    // Simulate a building being constructed:
    // Create an entity with EnergyComponent (consumer) in the registry
    auto entity = reg.create();
    uint32_t eid = static_cast<uint32_t>(entity);
    EnergyComponent ec{};
    ec.energy_required = 15;
    ec.priority = ENERGY_PRIORITY_NORMAL;
    reg.emplace<EnergyComponent>(entity, ec);

    // Fire the building constructed event handler
    // Position (12, 12) is within nexus coverage
    sys.on_building_constructed(eid, 0, 12, 12);

    // Verify consumer was registered
    ASSERT_EQ(sys.get_consumer_count(0), 1u);
    ASSERT_EQ(sys.get_consumer_position_count(0), 1u);

    // Tick - distribution should power the consumer
    sys.tick(0.05f);

    ASSERT(sys.is_powered(eid));
    ASSERT_EQ(sys.get_energy_received(eid), 15u);
}

// =============================================================================
// Test 12: Building deconstructed event removes consumer from pool
// =============================================================================

TEST(building_deconstructed_removes_consumer) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    // Place a nexus
    sys.place_nexus(NexusType::Carbon, 10, 10, 0);

    // Create a consumer via building constructed event
    auto entity = reg.create();
    uint32_t eid = static_cast<uint32_t>(entity);
    EnergyComponent ec{};
    ec.energy_required = 15;
    ec.priority = ENERGY_PRIORITY_NORMAL;
    reg.emplace<EnergyComponent>(entity, ec);
    sys.on_building_constructed(eid, 0, 12, 12);

    // Tick - consumer is registered and powered
    sys.tick(0.05f);
    ASSERT(sys.is_powered(eid));
    ASSERT_EQ(sys.get_consumer_count(0), 1u);

    // Fire building deconstructed event
    sys.on_building_deconstructed(eid, 0, 12, 12);

    // Consumer should be unregistered
    ASSERT_EQ(sys.get_consumer_count(0), 0u);
    ASSERT_EQ(sys.get_consumer_position_count(0), 0u);

    // Tick - pool should have 0 consumption now
    sys.tick(0.05f);
    ASSERT_EQ(sys.get_pool(0).total_consumed, 0u);
}

// =============================================================================
// Main
// =============================================================================

int main() {
    printf("=== EnergySystem Integration Tests (Ticket 5-041) ===\n\n");

    RUN_TEST(nexus_placement_increases_generation);
    RUN_TEST(consumer_in_coverage_increases_consumption);
    RUN_TEST(consumer_powered_with_surplus);
    RUN_TEST(consumer_outside_coverage_not_powered);
    RUN_TEST(deficit_priority_rationing);
    RUN_TEST(conduit_extends_coverage_to_consumer);
    RUN_TEST(remove_conduit_loses_coverage);
    RUN_TEST(nexus_offline_zero_generation);
    RUN_TEST(nexus_aging_reduces_output);
    RUN_TEST(multiplayer_isolation);
    RUN_TEST(building_constructed_registers_consumer);
    RUN_TEST(building_deconstructed_removes_consumer);

    printf("\n=== Results: %d passed, %d failed ===\n",
           tests_passed, tests_failed);

    return tests_failed > 0 ? 1 : 0;
}
