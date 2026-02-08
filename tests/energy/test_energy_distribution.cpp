/**
 * @file test_energy_distribution.cpp
 * @brief Unit tests for energy distribution (Ticket 5-018)
 *
 * Tests cover:
 * - distribute_energy() sets is_powered and energy_received for consumers
 * - Consumers in coverage with surplus >= 0 get powered
 * - Consumers in coverage with surplus < 0 get unpowered
 * - Consumers outside coverage always get unpowered
 * - tick() integration: distribution happens after pool calculation
 * - Multi-player isolation
 * - Edge cases: no consumers, no registry, invalid owner
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
// Helper: create consumer without manual coverage (for tick tests)
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
// distribute_energy: surplus >= 0 powers consumers in coverage
// =============================================================================

TEST(surplus_positive_consumers_powered) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    create_nexus(reg, sys, 0, 1000, true);
    uint32_t c1 = create_consumer(reg, sys, 0, 5, 5, 100);
    uint32_t c2 = create_consumer(reg, sys, 0, 10, 10, 200);

    sys.update_all_nexus_outputs(0);
    sys.calculate_pool(0);

    // surplus should be positive (1000 - 300 = 700)
    ASSERT(sys.get_pool(0).surplus > 0);

    sys.distribute_energy(0);

    auto e1 = static_cast<entt::entity>(c1);
    auto e2 = static_cast<entt::entity>(c2);
    const auto* ec1 = reg.try_get<EnergyComponent>(e1);
    const auto* ec2 = reg.try_get<EnergyComponent>(e2);

    ASSERT(ec1->is_powered);
    ASSERT_EQ(ec1->energy_received, 100u);
    ASSERT(ec2->is_powered);
    ASSERT_EQ(ec2->energy_received, 200u);
}

TEST(surplus_zero_consumers_powered) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    create_nexus(reg, sys, 0, 500, true);
    uint32_t c1 = create_consumer(reg, sys, 0, 5, 5, 500);

    sys.update_all_nexus_outputs(0);
    sys.calculate_pool(0);

    // surplus = 0 (500 - 500)
    ASSERT_EQ(sys.get_pool(0).surplus, 0);

    sys.distribute_energy(0);

    auto e1 = static_cast<entt::entity>(c1);
    const auto* ec1 = reg.try_get<EnergyComponent>(e1);

    ASSERT(ec1->is_powered);
    ASSERT_EQ(ec1->energy_received, 500u);
}

// =============================================================================
// distribute_energy: surplus < 0 unpowers consumers in coverage
// =============================================================================

TEST(deficit_consumers_unpowered) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    create_nexus(reg, sys, 0, 100, true);
    uint32_t c1 = create_consumer(reg, sys, 0, 5, 5, 300);
    uint32_t c2 = create_consumer(reg, sys, 0, 10, 10, 400);

    sys.update_all_nexus_outputs(0);
    sys.calculate_pool(0);

    // surplus = 100 - 700 = -600
    ASSERT(sys.get_pool(0).surplus < 0);

    sys.distribute_energy(0);

    auto e1 = static_cast<entt::entity>(c1);
    auto e2 = static_cast<entt::entity>(c2);
    const auto* ec1 = reg.try_get<EnergyComponent>(e1);
    const auto* ec2 = reg.try_get<EnergyComponent>(e2);

    ASSERT(!ec1->is_powered);
    ASSERT_EQ(ec1->energy_received, 0u);
    ASSERT(!ec2->is_powered);
    ASSERT_EQ(ec2->energy_received, 0u);
}

// =============================================================================
// distribute_energy: consumers outside coverage always unpowered
// =============================================================================

TEST(consumer_outside_coverage_unpowered) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    create_nexus(reg, sys, 0, 1000, true);

    // Consumer at (50,50) NOT in coverage
    auto entity = reg.create();
    uint32_t eid = static_cast<uint32_t>(entity);
    EnergyComponent ec{};
    ec.energy_required = 100;
    reg.emplace<EnergyComponent>(entity, ec);
    sys.register_consumer(eid, 0);
    sys.register_consumer_position(eid, 0, 50, 50);
    // Do NOT set coverage at (50,50)

    sys.update_all_nexus_outputs(0);
    sys.calculate_pool(0);

    ASSERT(sys.get_pool(0).surplus > 0);

    sys.distribute_energy(0);

    auto e = static_cast<entt::entity>(eid);
    const auto* ec_result = reg.try_get<EnergyComponent>(e);

    ASSERT(!ec_result->is_powered);
    ASSERT_EQ(ec_result->energy_received, 0u);
}

TEST(mix_in_and_out_of_coverage) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    create_nexus(reg, sys, 0, 1000, true);

    // Consumer IN coverage
    uint32_t c_in = create_consumer(reg, sys, 0, 5, 5, 100);

    // Consumer OUT of coverage
    auto entity_out = reg.create();
    uint32_t c_out = static_cast<uint32_t>(entity_out);
    EnergyComponent ec_out{};
    ec_out.energy_required = 200;
    reg.emplace<EnergyComponent>(entity_out, ec_out);
    sys.register_consumer(c_out, 0);
    sys.register_consumer_position(c_out, 0, 50, 50);

    sys.update_all_nexus_outputs(0);
    sys.calculate_pool(0);
    sys.distribute_energy(0);

    auto e_in = static_cast<entt::entity>(c_in);
    auto e_out = static_cast<entt::entity>(c_out);
    const auto* ec_in = reg.try_get<EnergyComponent>(e_in);
    const auto* ec_out_result = reg.try_get<EnergyComponent>(e_out);

    ASSERT(ec_in->is_powered);
    ASSERT_EQ(ec_in->energy_received, 100u);
    ASSERT(!ec_out_result->is_powered);
    ASSERT_EQ(ec_out_result->energy_received, 0u);
}

// =============================================================================
// distribute_energy: edge cases
// =============================================================================

TEST(no_consumers_no_crash) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    create_nexus(reg, sys, 0, 1000, true);
    sys.update_all_nexus_outputs(0);
    sys.calculate_pool(0);

    // Should not crash
    sys.distribute_energy(0);
}

TEST(no_registry_no_crash) {
    EnergySystem sys(64, 64);
    // No registry set

    // Should not crash
    sys.distribute_energy(0);
}

TEST(invalid_owner_no_crash) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    // Should not crash
    sys.distribute_energy(MAX_PLAYERS);
    sys.distribute_energy(255);
}

// =============================================================================
// tick() integration: distribution happens after pool calculation
// =============================================================================

TEST(tick_powers_consumers_healthy_pool) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    // Nexus at (10,10), consumer at (12,10) within coverage radius 8
    create_nexus_at(reg, sys, 0, 1000, 10, 10, true);
    uint32_t c1 = create_consumer_no_coverage(reg, sys, 0, 12, 10, 100);

    sys.tick(0.05f);

    auto e1 = static_cast<entt::entity>(c1);
    const auto* ec1 = reg.try_get<EnergyComponent>(e1);

    ASSERT(ec1->is_powered);
    ASSERT_EQ(ec1->energy_received, 100u);
}

TEST(tick_unpowers_consumers_deficit_pool) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    // Nexus at (10,10), consumer at (12,10)
    // generated ~100, consumed=3000 => deficit
    create_nexus_at(reg, sys, 0, 100, 10, 10, true);
    uint32_t c1 = create_consumer_no_coverage(reg, sys, 0, 12, 10, 3000);

    sys.tick(0.05f);

    auto e1 = static_cast<entt::entity>(c1);
    const auto* ec1 = reg.try_get<EnergyComponent>(e1);

    ASSERT(!ec1->is_powered);
    ASSERT_EQ(ec1->energy_received, 0u);
}

TEST(tick_consumer_outside_bfs_coverage_unpowered) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    // Nexus at (10,10), coverage radius 8
    // Consumer at (50,50) - far outside radius
    create_nexus_at(reg, sys, 0, 1000, 10, 10, true);
    uint32_t c_far = create_consumer_no_coverage(reg, sys, 0, 50, 50, 100);

    sys.tick(0.05f);

    auto e_far = static_cast<entt::entity>(c_far);
    const auto* ec_far = reg.try_get<EnergyComponent>(e_far);

    ASSERT(!ec_far->is_powered);
    ASSERT_EQ(ec_far->energy_received, 0u);
}

TEST(tick_transitions_powered_to_unpowered) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    create_nexus_at(reg, sys, 0, 1000, 10, 10, true);
    uint32_t c1 = create_consumer_no_coverage(reg, sys, 0, 12, 10, 100);

    // First tick: healthy => powered
    sys.tick(0.05f);

    auto e1 = static_cast<entt::entity>(c1);
    auto* ec1 = reg.try_get<EnergyComponent>(e1);
    ASSERT(ec1->is_powered);
    ASSERT_EQ(ec1->energy_received, 100u);

    // Increase consumption to create deficit
    ec1->energy_required = 5000;

    sys.tick(0.05f);

    // Now deficit => unpowered
    ASSERT(!ec1->is_powered);
    ASSERT_EQ(ec1->energy_received, 0u);
}

TEST(tick_transitions_unpowered_to_powered) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    create_nexus_at(reg, sys, 0, 1000, 10, 10, true);
    uint32_t c1 = create_consumer_no_coverage(reg, sys, 0, 12, 10, 5000);

    // First tick: deficit => unpowered
    sys.tick(0.05f);

    auto e1 = static_cast<entt::entity>(c1);
    auto* ec1 = reg.try_get<EnergyComponent>(e1);
    ASSERT(!ec1->is_powered);

    // Reduce consumption to restore surplus
    ec1->energy_required = 100;

    sys.tick(0.05f);

    // Now healthy => powered
    ASSERT(ec1->is_powered);
    ASSERT_EQ(ec1->energy_received, 100u);
}

// =============================================================================
// Multi-player isolation
// =============================================================================

TEST(multi_player_distribution_isolation) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    // Player 0: healthy (1000 gen, 100 consumed)
    create_nexus_at(reg, sys, 0, 1000, 10, 10, true);
    uint32_t c0 = create_consumer_no_coverage(reg, sys, 0, 12, 10, 100);

    // Player 1: deficit (100 gen, 3000 consumed)
    create_nexus_at(reg, sys, 1, 100, 40, 40, true);
    uint32_t c1 = create_consumer_no_coverage(reg, sys, 1, 42, 40, 3000);

    sys.tick(0.05f);

    // Player 0 consumer powered
    auto e0 = static_cast<entt::entity>(c0);
    const auto* ec0 = reg.try_get<EnergyComponent>(e0);
    ASSERT(ec0->is_powered);
    ASSERT_EQ(ec0->energy_received, 100u);

    // Player 1 consumer unpowered (deficit)
    auto e1 = static_cast<entt::entity>(c1);
    const auto* ec1 = reg.try_get<EnergyComponent>(e1);
    ASSERT(!ec1->is_powered);
    ASSERT_EQ(ec1->energy_received, 0u);
}

TEST(distribute_energy_multiple_consumers_all_powered) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    create_nexus(reg, sys, 0, 5000, true);

    uint32_t c1 = create_consumer(reg, sys, 0, 1, 1, 100);
    uint32_t c2 = create_consumer(reg, sys, 0, 2, 2, 200);
    uint32_t c3 = create_consumer(reg, sys, 0, 3, 3, 300);
    uint32_t c4 = create_consumer(reg, sys, 0, 4, 4, 400);

    sys.update_all_nexus_outputs(0);
    sys.calculate_pool(0);
    sys.distribute_energy(0);

    auto check_powered = [&](uint32_t eid, uint32_t expected_received) {
        auto e = static_cast<entt::entity>(eid);
        const auto* ec = reg.try_get<EnergyComponent>(e);
        ASSERT(ec->is_powered);
        ASSERT_EQ(ec->energy_received, expected_received);
    };

    check_powered(c1, 100);
    check_powered(c2, 200);
    check_powered(c3, 300);
    check_powered(c4, 400);
}

TEST(distribute_energy_multiple_consumers_all_unpowered_deficit) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    create_nexus(reg, sys, 0, 100, true);

    uint32_t c1 = create_consumer(reg, sys, 0, 1, 1, 500);
    uint32_t c2 = create_consumer(reg, sys, 0, 2, 2, 500);
    uint32_t c3 = create_consumer(reg, sys, 0, 3, 3, 500);

    sys.update_all_nexus_outputs(0);
    sys.calculate_pool(0);

    ASSERT(sys.get_pool(0).surplus < 0);

    sys.distribute_energy(0);

    auto check_unpowered = [&](uint32_t eid) {
        auto e = static_cast<entt::entity>(eid);
        const auto* ec = reg.try_get<EnergyComponent>(e);
        ASSERT(!ec->is_powered);
        ASSERT_EQ(ec->energy_received, 0u);
    };

    check_unpowered(c1);
    check_unpowered(c2);
    check_unpowered(c3);
}

// =============================================================================
// Main Entry Point
// =============================================================================

int main() {
    printf("=== Energy Distribution Unit Tests (Ticket 5-018) ===\n\n");

    // Surplus >= 0: consumers powered
    RUN_TEST(surplus_positive_consumers_powered);
    RUN_TEST(surplus_zero_consumers_powered);

    // Surplus < 0: consumers unpowered
    RUN_TEST(deficit_consumers_unpowered);

    // Outside coverage: always unpowered
    RUN_TEST(consumer_outside_coverage_unpowered);
    RUN_TEST(mix_in_and_out_of_coverage);

    // Edge cases
    RUN_TEST(no_consumers_no_crash);
    RUN_TEST(no_registry_no_crash);
    RUN_TEST(invalid_owner_no_crash);

    // tick() integration
    RUN_TEST(tick_powers_consumers_healthy_pool);
    RUN_TEST(tick_unpowers_consumers_deficit_pool);
    RUN_TEST(tick_consumer_outside_bfs_coverage_unpowered);
    RUN_TEST(tick_transitions_powered_to_unpowered);
    RUN_TEST(tick_transitions_unpowered_to_powered);

    // Multi-player isolation
    RUN_TEST(multi_player_distribution_isolation);
    RUN_TEST(distribute_energy_multiple_consumers_all_powered);
    RUN_TEST(distribute_energy_multiple_consumers_all_unpowered_deficit);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
