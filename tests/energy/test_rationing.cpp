/**
 * @file test_rationing.cpp
 * @brief Unit tests for priority-based rationing (Ticket 5-019)
 *
 * Tests cover:
 * - apply_rationing() powers critical consumers first during deficit
 * - Priority ordering: 1=Critical, 2=Important, 3=Normal, 4=Low
 * - Entity ID tie-breaking for same priority
 * - Available energy = pool.total_generated (not surplus)
 * - Consumers outside coverage always unpowered during rationing
 * - distribute_energy() calls apply_rationing() when surplus < 0
 * - Edge cases: no consumers, no registry, zero generation
 * - tick() integration with rationing
 */

#include <sims3000/energy/EnergySystem.h>
#include <sims3000/energy/EnergyComponent.h>
#include <sims3000/energy/EnergyProducerComponent.h>
#include <sims3000/energy/EnergyEnums.h>
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
// Helper: create consumer with manual coverage and priority
// =============================================================================

static uint32_t create_consumer_with_priority(entt::registry& reg, EnergySystem& sys,
                                               uint8_t owner, uint32_t x, uint32_t y,
                                               uint32_t energy_required, uint8_t priority) {
    auto entity = reg.create();
    uint32_t eid = static_cast<uint32_t>(entity);

    EnergyComponent ec{};
    ec.energy_required = energy_required;
    ec.priority = priority;
    reg.emplace<EnergyComponent>(entity, ec);

    sys.register_consumer(eid, owner);
    sys.register_consumer_position(eid, owner, x, y);
    set_coverage(sys, x, y, owner);
    return eid;
}

// =============================================================================
// Helper: create consumer with default priority and manual coverage
// =============================================================================

static uint32_t create_consumer(entt::registry& reg, EnergySystem& sys,
                                 uint8_t owner, uint32_t x, uint32_t y,
                                 uint32_t energy_required) {
    return create_consumer_with_priority(reg, sys, owner, x, y, energy_required,
                                         ENERGY_PRIORITY_DEFAULT);
}

// =============================================================================
// Helper: create consumer without coverage (for tick tests)
// =============================================================================

static uint32_t create_consumer_no_coverage_with_priority(entt::registry& reg, EnergySystem& sys,
                                                           uint8_t owner, uint32_t x, uint32_t y,
                                                           uint32_t energy_required, uint8_t priority) {
    auto entity = reg.create();
    uint32_t eid = static_cast<uint32_t>(entity);

    EnergyComponent ec{};
    ec.energy_required = energy_required;
    ec.priority = priority;
    reg.emplace<EnergyComponent>(entity, ec);

    sys.register_consumer(eid, owner);
    sys.register_consumer_position(eid, owner, x, y);
    return eid;
}

// =============================================================================
// Test: Critical priority powered first during deficit
// =============================================================================

TEST(critical_powered_first_during_deficit) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    // Generator: 150 units total
    create_nexus(reg, sys, 0, 150, true);

    // Consumer 1: Critical (priority 1), needs 100
    uint32_t c1 = create_consumer_with_priority(reg, sys, 0, 1, 1, 100, ENERGY_PRIORITY_CRITICAL);
    // Consumer 2: Low (priority 4), needs 100
    uint32_t c2 = create_consumer_with_priority(reg, sys, 0, 2, 2, 100, ENERGY_PRIORITY_LOW);

    sys.update_all_nexus_outputs(0);
    sys.calculate_pool(0);

    // surplus = 150 - 200 = -50 (deficit)
    ASSERT(sys.get_pool(0).surplus < 0);

    sys.distribute_energy(0);

    auto e1 = static_cast<entt::entity>(c1);
    auto e2 = static_cast<entt::entity>(c2);
    const auto* ec1 = reg.try_get<EnergyComponent>(e1);
    const auto* ec2 = reg.try_get<EnergyComponent>(e2);

    // Critical should be powered (100 <= 150 available)
    ASSERT(ec1->is_powered);
    ASSERT_EQ(ec1->energy_received, 100u);

    // Low should be unpowered (100 > 50 remaining)
    ASSERT(!ec2->is_powered);
    ASSERT_EQ(ec2->energy_received, 0u);
}

// =============================================================================
// Test: Full priority ordering
// =============================================================================

TEST(full_priority_ordering) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    // Generator: 250 units total
    create_nexus(reg, sys, 0, 250, true);

    // Create consumers in reverse priority order to verify sort works
    uint32_t c_low = create_consumer_with_priority(reg, sys, 0, 4, 4, 100, ENERGY_PRIORITY_LOW);
    uint32_t c_normal = create_consumer_with_priority(reg, sys, 0, 3, 3, 100, ENERGY_PRIORITY_NORMAL);
    uint32_t c_important = create_consumer_with_priority(reg, sys, 0, 2, 2, 100, ENERGY_PRIORITY_IMPORTANT);
    uint32_t c_critical = create_consumer_with_priority(reg, sys, 0, 1, 1, 100, ENERGY_PRIORITY_CRITICAL);

    sys.update_all_nexus_outputs(0);
    sys.calculate_pool(0);

    // surplus = 250 - 400 = -150 (deficit)
    ASSERT(sys.get_pool(0).surplus < 0);

    sys.distribute_energy(0);

    auto get_ec = [&](uint32_t eid) -> const EnergyComponent* {
        return reg.try_get<EnergyComponent>(static_cast<entt::entity>(eid));
    };

    // Critical (100) -> powered (250-100=150 remaining)
    ASSERT(get_ec(c_critical)->is_powered);
    ASSERT_EQ(get_ec(c_critical)->energy_received, 100u);

    // Important (100) -> powered (150-100=50 remaining)
    ASSERT(get_ec(c_important)->is_powered);
    ASSERT_EQ(get_ec(c_important)->energy_received, 100u);

    // Normal (100) -> unpowered (50 < 100)
    ASSERT(!get_ec(c_normal)->is_powered);
    ASSERT_EQ(get_ec(c_normal)->energy_received, 0u);

    // Low (100) -> unpowered (0 or 50 < 100)
    ASSERT(!get_ec(c_low)->is_powered);
    ASSERT_EQ(get_ec(c_low)->energy_received, 0u);
}

// =============================================================================
// Test: Entity ID tie-breaking for same priority
// =============================================================================

TEST(entity_id_tiebreaker_same_priority) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    // Generator: 150 units
    create_nexus(reg, sys, 0, 150, true);

    // Two consumers with same priority (Normal=3), needing 100 each
    // First created entity gets lower entity_id, should be powered first
    uint32_t c1 = create_consumer_with_priority(reg, sys, 0, 1, 1, 100, ENERGY_PRIORITY_NORMAL);
    uint32_t c2 = create_consumer_with_priority(reg, sys, 0, 2, 2, 100, ENERGY_PRIORITY_NORMAL);

    // c1 should have lower entity_id
    ASSERT(c1 < c2);

    sys.update_all_nexus_outputs(0);
    sys.calculate_pool(0);

    // surplus = 150 - 200 = -50 (deficit)
    ASSERT(sys.get_pool(0).surplus < 0);

    sys.distribute_energy(0);

    auto get_ec = [&](uint32_t eid) -> const EnergyComponent* {
        return reg.try_get<EnergyComponent>(static_cast<entt::entity>(eid));
    };

    // c1 (lower entity_id) -> powered
    ASSERT(get_ec(c1)->is_powered);
    ASSERT_EQ(get_ec(c1)->energy_received, 100u);

    // c2 (higher entity_id) -> unpowered
    ASSERT(!get_ec(c2)->is_powered);
    ASSERT_EQ(get_ec(c2)->energy_received, 0u);
}

// =============================================================================
// Test: Available energy = pool.total_generated during deficit
// =============================================================================

TEST(available_energy_is_total_generated) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    // Generator: 500 units
    create_nexus(reg, sys, 0, 500, true);

    // Consumers: Critical=200, Important=200, Normal=200 = total 600
    uint32_t c1 = create_consumer_with_priority(reg, sys, 0, 1, 1, 200, ENERGY_PRIORITY_CRITICAL);
    uint32_t c2 = create_consumer_with_priority(reg, sys, 0, 2, 2, 200, ENERGY_PRIORITY_IMPORTANT);
    uint32_t c3 = create_consumer_with_priority(reg, sys, 0, 3, 3, 200, ENERGY_PRIORITY_NORMAL);

    sys.update_all_nexus_outputs(0);
    sys.calculate_pool(0);

    // surplus = 500 - 600 = -100 (deficit)
    ASSERT(sys.get_pool(0).surplus < 0);
    ASSERT_EQ(sys.get_pool(0).total_generated, 500u);

    sys.distribute_energy(0);

    auto get_ec = [&](uint32_t eid) -> const EnergyComponent* {
        return reg.try_get<EnergyComponent>(static_cast<entt::entity>(eid));
    };

    // Critical (200) -> powered (500-200=300 remaining)
    ASSERT(get_ec(c1)->is_powered);
    ASSERT_EQ(get_ec(c1)->energy_received, 200u);

    // Important (200) -> powered (300-200=100 remaining)
    ASSERT(get_ec(c2)->is_powered);
    ASSERT_EQ(get_ec(c2)->energy_received, 200u);

    // Normal (200) -> unpowered (100 < 200)
    ASSERT(!get_ec(c3)->is_powered);
    ASSERT_EQ(get_ec(c3)->energy_received, 0u);
}

// =============================================================================
// Test: Consumers outside coverage during rationing
// =============================================================================

TEST(outside_coverage_unpowered_during_rationing) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    // Generator: 100 units
    create_nexus(reg, sys, 0, 100, true);

    // Consumer in coverage: Critical, needs 50
    uint32_t c_in = create_consumer_with_priority(reg, sys, 0, 1, 1, 50, ENERGY_PRIORITY_CRITICAL);

    // Consumer NOT in coverage: Critical, needs 50
    auto entity_out = reg.create();
    uint32_t c_out = static_cast<uint32_t>(entity_out);
    EnergyComponent ec_out{};
    ec_out.energy_required = 50;
    ec_out.priority = ENERGY_PRIORITY_CRITICAL;
    reg.emplace<EnergyComponent>(entity_out, ec_out);
    sys.register_consumer(c_out, 0);
    sys.register_consumer_position(c_out, 0, 50, 50);
    // Do NOT set coverage for (50,50)

    // Consumer in coverage: Low, needs 200 (to force deficit)
    uint32_t c_low = create_consumer_with_priority(reg, sys, 0, 2, 2, 200, ENERGY_PRIORITY_LOW);

    sys.update_all_nexus_outputs(0);
    sys.calculate_pool(0);

    // Only in-coverage consumers count: 50 + 200 = 250 consumed, 100 generated
    ASSERT(sys.get_pool(0).surplus < 0);

    sys.distribute_energy(0);

    auto get_ec = [&](uint32_t eid) -> const EnergyComponent* {
        return reg.try_get<EnergyComponent>(static_cast<entt::entity>(eid));
    };

    // c_in: in coverage, Critical -> powered (100-50=50 remaining)
    ASSERT(get_ec(c_in)->is_powered);
    ASSERT_EQ(get_ec(c_in)->energy_received, 50u);

    // c_out: NOT in coverage -> unpowered regardless
    ASSERT(!get_ec(c_out)->is_powered);
    ASSERT_EQ(get_ec(c_out)->energy_received, 0u);

    // c_low: in coverage, Low -> unpowered (50 < 200)
    ASSERT(!get_ec(c_low)->is_powered);
    ASSERT_EQ(get_ec(c_low)->energy_received, 0u);
}

// =============================================================================
// Test: Zero generation means no consumers powered
// =============================================================================

TEST(zero_generation_all_unpowered) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    // No generators, so total_generated = 0

    uint32_t c1 = create_consumer_with_priority(reg, sys, 0, 1, 1, 50, ENERGY_PRIORITY_CRITICAL);
    uint32_t c2 = create_consumer_with_priority(reg, sys, 0, 2, 2, 50, ENERGY_PRIORITY_LOW);

    sys.calculate_pool(0);

    // surplus = 0 - 100 = -100 (deficit, but generated=0 so pool might be 0)
    // Actually with no nexuses, total_generated=0, total_consumed=100, surplus=-100
    // Pool might remain at surplus >= 0 if no consumers are in coverage...
    // Need to force the deficit by setting pool directly
    sys.get_pool_mut(0).total_generated = 0;
    sys.get_pool_mut(0).total_consumed = 100;
    sys.get_pool_mut(0).surplus = -100;

    sys.distribute_energy(0);

    auto get_ec = [&](uint32_t eid) -> const EnergyComponent* {
        return reg.try_get<EnergyComponent>(static_cast<entt::entity>(eid));
    };

    ASSERT(!get_ec(c1)->is_powered);
    ASSERT_EQ(get_ec(c1)->energy_received, 0u);
    ASSERT(!get_ec(c2)->is_powered);
    ASSERT_EQ(get_ec(c2)->energy_received, 0u);
}

// =============================================================================
// Test: All consumers fit within available energy during deficit
// =============================================================================

TEST(all_consumers_fit_during_deficit) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    // Generator: 300 units
    create_nexus(reg, sys, 0, 300, true);

    // Three consumers: 100 each = 300 total. But one is outside coverage
    // so only 200 in coverage. Surplus = 300 - 200 = 100 (positive).
    // We need to force deficit via pool manipulation.
    uint32_t c1 = create_consumer_with_priority(reg, sys, 0, 1, 1, 50, ENERGY_PRIORITY_CRITICAL);
    uint32_t c2 = create_consumer_with_priority(reg, sys, 0, 2, 2, 50, ENERGY_PRIORITY_NORMAL);

    sys.update_all_nexus_outputs(0);
    sys.calculate_pool(0);

    // Force deficit: pretend consumption is higher
    sys.get_pool_mut(0).surplus = -1;

    sys.distribute_energy(0);

    auto get_ec = [&](uint32_t eid) -> const EnergyComponent* {
        return reg.try_get<EnergyComponent>(static_cast<entt::entity>(eid));
    };

    // Both should be powered since 50+50=100 <= 300 total_generated
    ASSERT(get_ec(c1)->is_powered);
    ASSERT_EQ(get_ec(c1)->energy_received, 50u);
    ASSERT(get_ec(c2)->is_powered);
    ASSERT_EQ(get_ec(c2)->energy_received, 50u);
}

// =============================================================================
// Test: No consumers during rationing (no crash)
// =============================================================================

TEST(no_consumers_rationing_no_crash) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    // Force deficit with no consumers
    sys.get_pool_mut(0).surplus = -100;
    sys.get_pool_mut(0).total_generated = 50;

    // Should not crash
    sys.apply_rationing(0);
}

// =============================================================================
// Test: No registry during rationing (no crash)
// =============================================================================

TEST(no_registry_rationing_no_crash) {
    EnergySystem sys(64, 64);
    // No registry set

    sys.get_pool_mut(0).surplus = -100;

    // Should not crash
    sys.apply_rationing(0);
}

// =============================================================================
// Test: Invalid owner during rationing (no crash)
// =============================================================================

TEST(invalid_owner_rationing_no_crash) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    // Should not crash
    sys.apply_rationing(MAX_PLAYERS);
    sys.apply_rationing(255);
}

// =============================================================================
// Test: tick() integration with rationing
// =============================================================================

TEST(tick_rationing_integration) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    // Nexus at (10,10) with coverage radius 8, generates 150
    create_nexus_at(reg, sys, 0, 150, 10, 10, true);

    // Consumers within coverage radius (distance <= 8 from nexus at 10,10)
    // Critical at (12,10), needs 100
    uint32_t c_crit = create_consumer_no_coverage_with_priority(
        reg, sys, 0, 12, 10, 100, ENERGY_PRIORITY_CRITICAL);
    // Low at (14,10), needs 100
    uint32_t c_low = create_consumer_no_coverage_with_priority(
        reg, sys, 0, 14, 10, 100, ENERGY_PRIORITY_LOW);

    // Total consumption = 200, generation ~= 150 (after aging on first tick)
    // Should trigger rationing

    sys.tick(0.05f);

    auto get_ec = [&](uint32_t eid) -> const EnergyComponent* {
        return reg.try_get<EnergyComponent>(static_cast<entt::entity>(eid));
    };

    // If deficit occurred (150 < 200), critical should be powered, low should not
    if (sys.get_pool(0).surplus < 0) {
        ASSERT(get_ec(c_crit)->is_powered);
        ASSERT_EQ(get_ec(c_crit)->energy_received, 100u);
        ASSERT(!get_ec(c_low)->is_powered);
        ASSERT_EQ(get_ec(c_low)->energy_received, 0u);
    } else {
        // If no deficit (aging might slightly change output), both should be powered
        ASSERT(get_ec(c_crit)->is_powered);
        ASSERT(get_ec(c_low)->is_powered);
    }
}

// =============================================================================
// Test: Exact energy boundary - consumer gets exactly what's available
// =============================================================================

TEST(exact_energy_boundary) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    // Generator: 200 units
    create_nexus(reg, sys, 0, 200, true);

    // Critical: needs exactly 200
    uint32_t c1 = create_consumer_with_priority(reg, sys, 0, 1, 1, 200, ENERGY_PRIORITY_CRITICAL);
    // Normal: needs 100 (to force deficit: total = 300, gen = 200)
    uint32_t c2 = create_consumer_with_priority(reg, sys, 0, 2, 2, 100, ENERGY_PRIORITY_NORMAL);

    sys.update_all_nexus_outputs(0);
    sys.calculate_pool(0);

    ASSERT(sys.get_pool(0).surplus < 0);

    sys.distribute_energy(0);

    auto get_ec = [&](uint32_t eid) -> const EnergyComponent* {
        return reg.try_get<EnergyComponent>(static_cast<entt::entity>(eid));
    };

    // Critical gets exactly 200 (all available)
    ASSERT(get_ec(c1)->is_powered);
    ASSERT_EQ(get_ec(c1)->energy_received, 200u);

    // Normal gets nothing (0 remaining)
    ASSERT(!get_ec(c2)->is_powered);
    ASSERT_EQ(get_ec(c2)->energy_received, 0u);
}

// =============================================================================
// Test: Multi-player rationing isolation
// =============================================================================

TEST(multi_player_rationing_isolation) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    // Player 0: deficit (150 gen, 200 consumed)
    create_nexus(reg, sys, 0, 150, true);
    uint32_t c0_crit = create_consumer_with_priority(reg, sys, 0, 1, 1, 100, ENERGY_PRIORITY_CRITICAL);
    uint32_t c0_low = create_consumer_with_priority(reg, sys, 0, 2, 2, 100, ENERGY_PRIORITY_LOW);

    // Player 1: healthy (1000 gen, 100 consumed)
    create_nexus(reg, sys, 1, 1000, true);
    uint32_t c1_normal = create_consumer_with_priority(reg, sys, 1, 30, 30, 100, ENERGY_PRIORITY_NORMAL);

    sys.update_all_nexus_outputs(0);
    sys.update_all_nexus_outputs(1);
    sys.calculate_pool(0);
    sys.calculate_pool(1);

    ASSERT(sys.get_pool(0).surplus < 0);
    ASSERT(sys.get_pool(1).surplus >= 0);

    sys.distribute_energy(0);
    sys.distribute_energy(1);

    auto get_ec = [&](uint32_t eid) -> const EnergyComponent* {
        return reg.try_get<EnergyComponent>(static_cast<entt::entity>(eid));
    };

    // Player 0: rationing applied
    ASSERT(get_ec(c0_crit)->is_powered);
    ASSERT(!get_ec(c0_low)->is_powered);

    // Player 1: normal distribution, all powered
    ASSERT(get_ec(c1_normal)->is_powered);
    ASSERT_EQ(get_ec(c1_normal)->energy_received, 100u);
}

// =============================================================================
// Main Entry Point
// =============================================================================

int main() {
    printf("=== Priority-Based Rationing Unit Tests (Ticket 5-019) ===\n\n");

    // Priority ordering
    RUN_TEST(critical_powered_first_during_deficit);
    RUN_TEST(full_priority_ordering);
    RUN_TEST(entity_id_tiebreaker_same_priority);

    // Energy budget
    RUN_TEST(available_energy_is_total_generated);
    RUN_TEST(exact_energy_boundary);
    RUN_TEST(all_consumers_fit_during_deficit);

    // Coverage interaction
    RUN_TEST(outside_coverage_unpowered_during_rationing);
    RUN_TEST(zero_generation_all_unpowered);

    // Edge cases
    RUN_TEST(no_consumers_rationing_no_crash);
    RUN_TEST(no_registry_rationing_no_crash);
    RUN_TEST(invalid_owner_rationing_no_crash);

    // Integration
    RUN_TEST(tick_rationing_integration);

    // Multi-player
    RUN_TEST(multi_player_rationing_isolation);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
