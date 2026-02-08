/**
 * @file test_energy_provider.cpp
 * @brief Unit tests for EnergySystem IEnergyProvider implementation (Ticket 5-009)
 *
 * Tests cover:
 * - is_powered: queries EnergyComponent.is_powered via registry
 * - is_powered_at: checks coverage + pool surplus
 * - get_energy_required: queries EnergyComponent.energy_required via registry
 * - get_energy_received: queries EnergyComponent.energy_received via registry
 * - No registry set: all methods return safe defaults
 * - Invalid entity IDs: return safe defaults
 * - Entities without EnergyComponent: return safe defaults
 * - Polymorphic access via IEnergyProvider pointer
 */

#include <sims3000/energy/EnergySystem.h>
#include <sims3000/energy/EnergyComponent.h>
#include <entt/entt.hpp>
#include <cstdio>
#include <cstdlib>

using namespace sims3000::energy;

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
// is_powered tests
// =============================================================================

TEST(is_powered_no_registry_returns_false) {
    EnergySystem sys(64, 64);
    // No registry set -> all queries return false
    ASSERT(!sys.is_powered(0));
    ASSERT(!sys.is_powered(1));
    ASSERT(!sys.is_powered(9999));
}

TEST(is_powered_entity_with_powered_component) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    auto ent = reg.create();
    auto& ec = reg.emplace<EnergyComponent>(ent);
    ec.energy_required = 100;
    ec.energy_received = 100;
    ec.is_powered = true;

    uint32_t eid = static_cast<uint32_t>(ent);
    ASSERT(sys.is_powered(eid));
}

TEST(is_powered_entity_with_unpowered_component) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    auto ent = reg.create();
    auto& ec = reg.emplace<EnergyComponent>(ent);
    ec.energy_required = 100;
    ec.energy_received = 50;
    ec.is_powered = false;

    uint32_t eid = static_cast<uint32_t>(ent);
    ASSERT(!sys.is_powered(eid));
}

TEST(is_powered_entity_without_component) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    // Create entity with no EnergyComponent
    auto ent = reg.create();
    uint32_t eid = static_cast<uint32_t>(ent);
    ASSERT(!sys.is_powered(eid));
}

TEST(is_powered_invalid_entity_id) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    // Use an entity ID that was never created
    ASSERT(!sys.is_powered(99999));
}

TEST(is_powered_destroyed_entity) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    auto ent = reg.create();
    auto& ec = reg.emplace<EnergyComponent>(ent);
    ec.is_powered = true;

    uint32_t eid = static_cast<uint32_t>(ent);
    ASSERT(sys.is_powered(eid));

    // Destroy the entity
    reg.destroy(ent);
    ASSERT(!sys.is_powered(eid));
}

TEST(is_powered_multiple_entities) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    // Create three entities: powered, unpowered, no component
    auto ent1 = reg.create();
    auto& ec1 = reg.emplace<EnergyComponent>(ent1);
    ec1.is_powered = true;

    auto ent2 = reg.create();
    auto& ec2 = reg.emplace<EnergyComponent>(ent2);
    ec2.is_powered = false;

    auto ent3 = reg.create();
    // ent3 has no EnergyComponent

    ASSERT(sys.is_powered(static_cast<uint32_t>(ent1)));
    ASSERT(!sys.is_powered(static_cast<uint32_t>(ent2)));
    ASSERT(!sys.is_powered(static_cast<uint32_t>(ent3)));
}

TEST(is_powered_registry_set_to_nullptr) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    auto ent = reg.create();
    auto& ec = reg.emplace<EnergyComponent>(ent);
    ec.is_powered = true;

    uint32_t eid = static_cast<uint32_t>(ent);
    ASSERT(sys.is_powered(eid));

    // Set registry to nullptr - should return false
    sys.set_registry(nullptr);
    ASSERT(!sys.is_powered(eid));
}

// =============================================================================
// is_powered_at tests
// =============================================================================

TEST(is_powered_at_no_coverage_returns_false) {
    EnergySystem sys(64, 64);
    // No coverage set -> all queries return false
    ASSERT(!sys.is_powered_at(10, 10, 0));
    ASSERT(!sys.is_powered_at(10, 10, 1));
    ASSERT(!sys.is_powered_at(10, 10, 2));
    ASSERT(!sys.is_powered_at(10, 10, 3));
}

TEST(is_powered_at_with_coverage_and_surplus) {
    EnergySystem sys(64, 64);
    // Set coverage at (10, 10) for player 0 (overseer_id = 1)
    CoverageGrid& grid = sys.get_coverage_grid_mut();
    grid.set(10, 10, 1);  // overseer_id 1 = player 0

    // Pool surplus defaults to 0 (>= 0 -> true)
    ASSERT(sys.is_powered_at(10, 10, 0));
}

TEST(is_powered_at_with_coverage_and_positive_surplus) {
    EnergySystem sys(64, 64);
    CoverageGrid& grid = sys.get_coverage_grid_mut();
    grid.set(20, 20, 2);  // overseer_id 2 = player 1

    // Set positive surplus for player 1
    PerPlayerEnergyPool& pool = sys.get_pool_mut(1);
    pool.surplus = 500;

    ASSERT(sys.is_powered_at(20, 20, 1));
}

TEST(is_powered_at_with_coverage_and_negative_surplus) {
    EnergySystem sys(64, 64);
    CoverageGrid& grid = sys.get_coverage_grid_mut();
    grid.set(10, 10, 1);  // overseer_id 1 = player 0

    // Set negative surplus for player 0 (deficit)
    PerPlayerEnergyPool& pool = sys.get_pool_mut(0);
    pool.surplus = -100;

    // Has coverage but negative surplus -> false
    ASSERT(!sys.is_powered_at(10, 10, 0));
}

TEST(is_powered_at_no_coverage_but_has_surplus) {
    EnergySystem sys(64, 64);
    // Pool surplus defaults to 0 (>= 0), but tile (10, 10) has no coverage
    ASSERT(!sys.is_powered_at(10, 10, 0));
}

TEST(is_powered_at_coverage_wrong_player) {
    EnergySystem sys(64, 64);
    CoverageGrid& grid = sys.get_coverage_grid_mut();
    grid.set(10, 10, 1);  // overseer_id 1 = player 0

    // Query for player 1 at (10, 10) - coverage belongs to player 0
    ASSERT(!sys.is_powered_at(10, 10, 1));
}

TEST(is_powered_at_invalid_player_id) {
    EnergySystem sys(64, 64);
    // Player ID >= MAX_PLAYERS should return false
    ASSERT(!sys.is_powered_at(10, 10, 4));
    ASSERT(!sys.is_powered_at(10, 10, 255));
    ASSERT(!sys.is_powered_at(10, 10, 1000));
}

TEST(is_powered_at_out_of_bounds_position) {
    EnergySystem sys(64, 64);
    CoverageGrid& grid = sys.get_coverage_grid_mut();
    grid.set(0, 0, 1);  // Set some coverage

    // Out-of-bounds position -> CoverageGrid returns false
    ASSERT(!sys.is_powered_at(100, 100, 0));
    ASSERT(!sys.is_powered_at(64, 64, 0));
}

TEST(is_powered_at_multiple_players) {
    EnergySystem sys(64, 64);
    CoverageGrid& grid = sys.get_coverage_grid_mut();
    grid.set(5, 5, 1);   // overseer_id 1 = player 0
    grid.set(10, 10, 2); // overseer_id 2 = player 1
    grid.set(15, 15, 3); // overseer_id 3 = player 2

    ASSERT(sys.is_powered_at(5, 5, 0));
    ASSERT(sys.is_powered_at(10, 10, 1));
    ASSERT(sys.is_powered_at(15, 15, 2));

    // Cross-checks: wrong player for each position
    ASSERT(!sys.is_powered_at(5, 5, 1));
    ASSERT(!sys.is_powered_at(10, 10, 0));
    ASSERT(!sys.is_powered_at(15, 15, 1));
}

TEST(is_powered_at_surplus_exactly_zero) {
    EnergySystem sys(64, 64);
    CoverageGrid& grid = sys.get_coverage_grid_mut();
    grid.set(10, 10, 1);  // overseer_id 1 = player 0

    // Surplus is exactly 0 -> should return true (>= 0)
    PerPlayerEnergyPool& pool = sys.get_pool_mut(0);
    pool.surplus = 0;

    ASSERT(sys.is_powered_at(10, 10, 0));
}

// =============================================================================
// get_energy_required tests
// =============================================================================

TEST(get_energy_required_no_registry_returns_zero) {
    EnergySystem sys(64, 64);
    ASSERT_EQ(sys.get_energy_required(0), 0u);
    ASSERT_EQ(sys.get_energy_required(42), 0u);
}

TEST(get_energy_required_returns_component_value) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    auto ent = reg.create();
    auto& ec = reg.emplace<EnergyComponent>(ent);
    ec.energy_required = 250;

    uint32_t eid = static_cast<uint32_t>(ent);
    ASSERT_EQ(sys.get_energy_required(eid), 250u);
}

TEST(get_energy_required_zero_value) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    auto ent = reg.create();
    auto& ec = reg.emplace<EnergyComponent>(ent);
    ec.energy_required = 0;

    uint32_t eid = static_cast<uint32_t>(ent);
    ASSERT_EQ(sys.get_energy_required(eid), 0u);
}

TEST(get_energy_required_entity_without_component) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    auto ent = reg.create();
    uint32_t eid = static_cast<uint32_t>(ent);
    ASSERT_EQ(sys.get_energy_required(eid), 0u);
}

TEST(get_energy_required_invalid_entity_id) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    ASSERT_EQ(sys.get_energy_required(99999), 0u);
}

TEST(get_energy_required_multiple_entities) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    auto ent1 = reg.create();
    auto& ec1 = reg.emplace<EnergyComponent>(ent1);
    ec1.energy_required = 100;

    auto ent2 = reg.create();
    auto& ec2 = reg.emplace<EnergyComponent>(ent2);
    ec2.energy_required = 500;

    auto ent3 = reg.create();
    auto& ec3 = reg.emplace<EnergyComponent>(ent3);
    ec3.energy_required = 0;

    ASSERT_EQ(sys.get_energy_required(static_cast<uint32_t>(ent1)), 100u);
    ASSERT_EQ(sys.get_energy_required(static_cast<uint32_t>(ent2)), 500u);
    ASSERT_EQ(sys.get_energy_required(static_cast<uint32_t>(ent3)), 0u);
}

// =============================================================================
// get_energy_received tests
// =============================================================================

TEST(get_energy_received_no_registry_returns_zero) {
    EnergySystem sys(64, 64);
    ASSERT_EQ(sys.get_energy_received(0), 0u);
    ASSERT_EQ(sys.get_energy_received(42), 0u);
}

TEST(get_energy_received_returns_component_value) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    auto ent = reg.create();
    auto& ec = reg.emplace<EnergyComponent>(ent);
    ec.energy_received = 150;

    uint32_t eid = static_cast<uint32_t>(ent);
    ASSERT_EQ(sys.get_energy_received(eid), 150u);
}

TEST(get_energy_received_zero_value) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    auto ent = reg.create();
    auto& ec = reg.emplace<EnergyComponent>(ent);
    ec.energy_received = 0;

    uint32_t eid = static_cast<uint32_t>(ent);
    ASSERT_EQ(sys.get_energy_received(eid), 0u);
}

TEST(get_energy_received_entity_without_component) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    auto ent = reg.create();
    uint32_t eid = static_cast<uint32_t>(ent);
    ASSERT_EQ(sys.get_energy_received(eid), 0u);
}

TEST(get_energy_received_invalid_entity_id) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    ASSERT_EQ(sys.get_energy_received(99999), 0u);
}

TEST(get_energy_received_multiple_entities) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    auto ent1 = reg.create();
    auto& ec1 = reg.emplace<EnergyComponent>(ent1);
    ec1.energy_received = 75;

    auto ent2 = reg.create();
    auto& ec2 = reg.emplace<EnergyComponent>(ent2);
    ec2.energy_received = 300;

    ASSERT_EQ(sys.get_energy_received(static_cast<uint32_t>(ent1)), 75u);
    ASSERT_EQ(sys.get_energy_received(static_cast<uint32_t>(ent2)), 300u);
}

// =============================================================================
// Combined / integration-style tests
// =============================================================================

TEST(full_entity_energy_roundtrip) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    // Create a fully powered entity
    auto ent = reg.create();
    auto& ec = reg.emplace<EnergyComponent>(ent);
    ec.energy_required = 200;
    ec.energy_received = 200;
    ec.is_powered = true;

    uint32_t eid = static_cast<uint32_t>(ent);
    ASSERT(sys.is_powered(eid));
    ASSERT_EQ(sys.get_energy_required(eid), 200u);
    ASSERT_EQ(sys.get_energy_received(eid), 200u);
}

TEST(underpowered_entity_state) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    // Create an underpowered entity
    auto ent = reg.create();
    auto& ec = reg.emplace<EnergyComponent>(ent);
    ec.energy_required = 200;
    ec.energy_received = 50;
    ec.is_powered = false;

    uint32_t eid = static_cast<uint32_t>(ent);
    ASSERT(!sys.is_powered(eid));
    ASSERT_EQ(sys.get_energy_required(eid), 200u);
    ASSERT_EQ(sys.get_energy_received(eid), 50u);
}

TEST(polymorphic_access_via_interface) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    auto ent = reg.create();
    auto& ec = reg.emplace<EnergyComponent>(ent);
    ec.is_powered = true;

    // Access via IEnergyProvider pointer
    sims3000::building::IEnergyProvider* provider = &sys;

    uint32_t eid = static_cast<uint32_t>(ent);
    ASSERT(provider->is_powered(eid));
    ASSERT(!provider->is_powered_at(10, 10, 0));  // No coverage
}

TEST(set_registry_replaces_previous) {
    entt::registry reg1;
    entt::registry reg2;
    EnergySystem sys(64, 64);

    // Set first registry and create entity
    sys.set_registry(&reg1);
    auto ent1 = reg1.create();
    auto& ec1 = reg1.emplace<EnergyComponent>(ent1);
    ec1.is_powered = true;
    ec1.energy_required = 100;

    uint32_t eid1 = static_cast<uint32_t>(ent1);
    ASSERT(sys.is_powered(eid1));
    ASSERT_EQ(sys.get_energy_required(eid1), 100u);

    // Switch to second registry - entity from reg1 should not be accessible
    sys.set_registry(&reg2);
    ASSERT(!sys.is_powered(eid1));
    ASSERT_EQ(sys.get_energy_required(eid1), 0u);

    // Create entity in reg2
    auto ent2 = reg2.create();
    auto& ec2 = reg2.emplace<EnergyComponent>(ent2);
    ec2.is_powered = true;
    ec2.energy_required = 200;

    uint32_t eid2 = static_cast<uint32_t>(ent2);
    ASSERT(sys.is_powered(eid2));
    ASSERT_EQ(sys.get_energy_required(eid2), 200u);
}

// =============================================================================
// Main Entry Point
// =============================================================================

int main() {
    printf("=== EnergyProvider Unit Tests (Ticket 5-009) ===\n\n");

    // is_powered tests
    RUN_TEST(is_powered_no_registry_returns_false);
    RUN_TEST(is_powered_entity_with_powered_component);
    RUN_TEST(is_powered_entity_with_unpowered_component);
    RUN_TEST(is_powered_entity_without_component);
    RUN_TEST(is_powered_invalid_entity_id);
    RUN_TEST(is_powered_destroyed_entity);
    RUN_TEST(is_powered_multiple_entities);
    RUN_TEST(is_powered_registry_set_to_nullptr);

    // is_powered_at tests
    RUN_TEST(is_powered_at_no_coverage_returns_false);
    RUN_TEST(is_powered_at_with_coverage_and_surplus);
    RUN_TEST(is_powered_at_with_coverage_and_positive_surplus);
    RUN_TEST(is_powered_at_with_coverage_and_negative_surplus);
    RUN_TEST(is_powered_at_no_coverage_but_has_surplus);
    RUN_TEST(is_powered_at_coverage_wrong_player);
    RUN_TEST(is_powered_at_invalid_player_id);
    RUN_TEST(is_powered_at_out_of_bounds_position);
    RUN_TEST(is_powered_at_multiple_players);
    RUN_TEST(is_powered_at_surplus_exactly_zero);

    // get_energy_required tests
    RUN_TEST(get_energy_required_no_registry_returns_zero);
    RUN_TEST(get_energy_required_returns_component_value);
    RUN_TEST(get_energy_required_zero_value);
    RUN_TEST(get_energy_required_entity_without_component);
    RUN_TEST(get_energy_required_invalid_entity_id);
    RUN_TEST(get_energy_required_multiple_entities);

    // get_energy_received tests
    RUN_TEST(get_energy_received_no_registry_returns_zero);
    RUN_TEST(get_energy_received_returns_component_value);
    RUN_TEST(get_energy_received_zero_value);
    RUN_TEST(get_energy_received_entity_without_component);
    RUN_TEST(get_energy_received_invalid_entity_id);
    RUN_TEST(get_energy_received_multiple_entities);

    // Combined tests
    RUN_TEST(full_entity_energy_roundtrip);
    RUN_TEST(underpowered_entity_state);
    RUN_TEST(polymorphic_access_via_interface);
    RUN_TEST(set_registry_replaces_previous);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
