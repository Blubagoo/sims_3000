/**
 * @file test_pool_state_machine.cpp
 * @brief Unit tests for pool state machine (Ticket 5-013)
 *
 * Tests cover:
 * - calculate_pool_state() returns correct state for all scenarios
 * - Threshold calculations: buffer (10% of generated), collapse (50% of consumed)
 * - detect_pool_state_transitions() updates previous_state
 * - State transitions between all four states
 * - Edge cases: zero generation, zero consumption, exact thresholds
 * - tick() integration: state calculated after pool aggregation
 */

#include <sims3000/energy/EnergySystem.h>
#include <sims3000/energy/EnergyComponent.h>
#include <sims3000/energy/EnergyProducerComponent.h>
#include <sims3000/energy/EnergyEnums.h>
#include <sims3000/energy/PerPlayerEnergyPool.h>
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
// Helper: create a pool with specified values for calculate_pool_state testing
// =============================================================================

static PerPlayerEnergyPool make_pool(uint32_t generated, uint32_t consumed) {
    PerPlayerEnergyPool pool{};
    pool.total_generated = generated;
    pool.total_consumed = consumed;
    pool.surplus = static_cast<int32_t>(generated) - static_cast<int32_t>(consumed);
    return pool;
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
// calculate_pool_state: Healthy
// =============================================================================

TEST(healthy_large_surplus) {
    // generated=1000, consumed=500 => surplus=500
    // buffer_threshold = 1000 * 0.10 = 100
    // surplus(500) >= buffer_threshold(100) => Healthy
    PerPlayerEnergyPool pool = make_pool(1000, 500);
    ASSERT_EQ(EnergySystem::calculate_pool_state(pool), EnergyPoolState::Healthy);
}

TEST(healthy_exact_buffer_threshold) {
    // generated=1000, consumed=900 => surplus=100
    // buffer_threshold = 1000 * 0.10 = 100
    // surplus(100) >= buffer_threshold(100) => Healthy
    PerPlayerEnergyPool pool = make_pool(1000, 900);
    ASSERT_EQ(EnergySystem::calculate_pool_state(pool), EnergyPoolState::Healthy);
}

TEST(healthy_no_consumers) {
    // generated=1000, consumed=0 => surplus=1000
    // buffer_threshold = 100, surplus >= 100 => Healthy
    PerPlayerEnergyPool pool = make_pool(1000, 0);
    ASSERT_EQ(EnergySystem::calculate_pool_state(pool), EnergyPoolState::Healthy);
}

TEST(healthy_zero_generation_zero_consumption) {
    // generated=0, consumed=0 => surplus=0
    // buffer_threshold = 0 * 0.10 = 0
    // surplus(0) >= buffer_threshold(0) => Healthy
    PerPlayerEnergyPool pool = make_pool(0, 0);
    ASSERT_EQ(EnergySystem::calculate_pool_state(pool), EnergyPoolState::Healthy);
}

// =============================================================================
// calculate_pool_state: Marginal
// =============================================================================

TEST(marginal_just_below_buffer) {
    // generated=1000, consumed=910 => surplus=90
    // buffer_threshold = 1000 * 0.10 = 100
    // 0 <= surplus(90) < buffer_threshold(100) => Marginal
    PerPlayerEnergyPool pool = make_pool(1000, 910);
    ASSERT_EQ(EnergySystem::calculate_pool_state(pool), EnergyPoolState::Marginal);
}

TEST(marginal_exact_balance) {
    // generated=1000, consumed=1000 => surplus=0
    // buffer_threshold = 1000 * 0.10 = 100
    // 0 <= surplus(0) < buffer_threshold(100) => Marginal
    PerPlayerEnergyPool pool = make_pool(1000, 1000);
    ASSERT_EQ(EnergySystem::calculate_pool_state(pool), EnergyPoolState::Marginal);
}

TEST(marginal_tiny_surplus) {
    // generated=1000, consumed=999 => surplus=1
    // buffer_threshold = 100
    // 0 <= surplus(1) < 100 => Marginal
    PerPlayerEnergyPool pool = make_pool(1000, 999);
    ASSERT_EQ(EnergySystem::calculate_pool_state(pool), EnergyPoolState::Marginal);
}

// =============================================================================
// calculate_pool_state: Deficit
// =============================================================================

TEST(deficit_small_negative_surplus) {
    // generated=1000, consumed=1010 => surplus=-10
    // collapse_threshold = 1010 * 0.50 = 505
    // -505 < surplus(-10) < 0 => Deficit
    PerPlayerEnergyPool pool = make_pool(1000, 1010);
    ASSERT_EQ(EnergySystem::calculate_pool_state(pool), EnergyPoolState::Deficit);
}

TEST(deficit_moderate_negative_surplus) {
    // generated=1000, consumed=1200 => surplus=-200
    // collapse_threshold = 1200 * 0.50 = 600
    // -600 < surplus(-200) < 0 => Deficit
    PerPlayerEnergyPool pool = make_pool(1000, 1200);
    ASSERT_EQ(EnergySystem::calculate_pool_state(pool), EnergyPoolState::Deficit);
}

// =============================================================================
// calculate_pool_state: Collapse
// =============================================================================

TEST(collapse_large_deficit) {
    // generated=100, consumed=1000 => surplus=-900
    // collapse_threshold = 1000 * 0.50 = 500
    // surplus(-900) <= -collapse_threshold(-500) => Collapse
    PerPlayerEnergyPool pool = make_pool(100, 1000);
    ASSERT_EQ(EnergySystem::calculate_pool_state(pool), EnergyPoolState::Collapse);
}

TEST(collapse_exact_threshold) {
    // generated=500, consumed=1000 => surplus=-500
    // collapse_threshold = 1000 * 0.50 = 500
    // surplus(-500) <= -collapse_threshold(-500) => Collapse
    PerPlayerEnergyPool pool = make_pool(500, 1000);
    ASSERT_EQ(EnergySystem::calculate_pool_state(pool), EnergyPoolState::Collapse);
}

TEST(collapse_no_generation) {
    // generated=0, consumed=1000 => surplus=-1000
    // collapse_threshold = 1000 * 0.50 = 500
    // surplus(-1000) <= -500 => Collapse
    PerPlayerEnergyPool pool = make_pool(0, 1000);
    ASSERT_EQ(EnergySystem::calculate_pool_state(pool), EnergyPoolState::Collapse);
}

TEST(collapse_zero_consumed_zero_generated_is_healthy) {
    // Edge: with no consumers, collapse_threshold = 0
    // surplus = 0, buffer_threshold = 0
    // surplus(0) >= buffer_threshold(0) => Healthy
    PerPlayerEnergyPool pool = make_pool(0, 0);
    ASSERT_EQ(EnergySystem::calculate_pool_state(pool), EnergyPoolState::Healthy);
}

// =============================================================================
// detect_pool_state_transitions: updates previous_state
// =============================================================================

TEST(detect_transitions_updates_previous_state) {
    EnergySystem sys(64, 64);

    auto& pool = sys.get_pool_mut(0);
    pool.state = EnergyPoolState::Deficit;
    pool.previous_state = EnergyPoolState::Healthy;

    sys.detect_pool_state_transitions(0);

    ASSERT_EQ(pool.previous_state, EnergyPoolState::Deficit);
}

TEST(detect_transitions_no_change) {
    EnergySystem sys(64, 64);

    auto& pool = sys.get_pool_mut(0);
    pool.state = EnergyPoolState::Healthy;
    pool.previous_state = EnergyPoolState::Healthy;

    sys.detect_pool_state_transitions(0);

    ASSERT_EQ(pool.previous_state, EnergyPoolState::Healthy);
}

TEST(detect_transitions_healthy_to_deficit) {
    EnergySystem sys(64, 64);

    auto& pool = sys.get_pool_mut(0);
    pool.previous_state = EnergyPoolState::Healthy;
    pool.state = EnergyPoolState::Deficit;
    pool.surplus = -100;
    pool.consumer_count = 5;

    sys.detect_pool_state_transitions(0);

    // Should have updated previous_state
    ASSERT_EQ(pool.previous_state, EnergyPoolState::Deficit);
}

TEST(detect_transitions_healthy_to_collapse) {
    EnergySystem sys(64, 64);

    auto& pool = sys.get_pool_mut(0);
    pool.previous_state = EnergyPoolState::Healthy;
    pool.state = EnergyPoolState::Collapse;
    pool.surplus = -500;

    sys.detect_pool_state_transitions(0);

    ASSERT_EQ(pool.previous_state, EnergyPoolState::Collapse);
}

TEST(detect_transitions_deficit_to_healthy) {
    EnergySystem sys(64, 64);

    auto& pool = sys.get_pool_mut(0);
    pool.previous_state = EnergyPoolState::Deficit;
    pool.state = EnergyPoolState::Healthy;
    pool.surplus = 500;

    sys.detect_pool_state_transitions(0);

    ASSERT_EQ(pool.previous_state, EnergyPoolState::Healthy);
}

TEST(detect_transitions_collapse_to_healthy) {
    EnergySystem sys(64, 64);

    auto& pool = sys.get_pool_mut(0);
    pool.previous_state = EnergyPoolState::Collapse;
    pool.state = EnergyPoolState::Healthy;
    pool.surplus = 500;

    sys.detect_pool_state_transitions(0);

    ASSERT_EQ(pool.previous_state, EnergyPoolState::Healthy);
}

TEST(detect_transitions_collapse_to_marginal) {
    EnergySystem sys(64, 64);

    auto& pool = sys.get_pool_mut(0);
    pool.previous_state = EnergyPoolState::Collapse;
    pool.state = EnergyPoolState::Marginal;
    pool.surplus = 10;

    sys.detect_pool_state_transitions(0);

    // Should transition out of collapse AND out of deficit
    ASSERT_EQ(pool.previous_state, EnergyPoolState::Marginal);
}

TEST(detect_transitions_deficit_to_collapse) {
    EnergySystem sys(64, 64);

    auto& pool = sys.get_pool_mut(0);
    pool.previous_state = EnergyPoolState::Deficit;
    pool.state = EnergyPoolState::Collapse;
    pool.surplus = -500;

    sys.detect_pool_state_transitions(0);

    // Transition into collapse from deficit (already in deficit, so no deficit began)
    ASSERT_EQ(pool.previous_state, EnergyPoolState::Collapse);
}

TEST(detect_transitions_collapse_to_deficit) {
    EnergySystem sys(64, 64);

    auto& pool = sys.get_pool_mut(0);
    pool.previous_state = EnergyPoolState::Collapse;
    pool.state = EnergyPoolState::Deficit;
    pool.surplus = -50;

    sys.detect_pool_state_transitions(0);

    // Transition out of collapse but still in deficit (no deficit ended event)
    ASSERT_EQ(pool.previous_state, EnergyPoolState::Deficit);
}

TEST(detect_transitions_invalid_owner_no_crash) {
    EnergySystem sys(64, 64);

    // Should not crash
    sys.detect_pool_state_transitions(MAX_PLAYERS);
    sys.detect_pool_state_transitions(255);
}

// =============================================================================
// Configurable threshold constants
// =============================================================================

TEST(threshold_constants_are_correct) {
    ASSERT(EnergySystem::BUFFER_THRESHOLD_PERCENT > 0.0f);
    ASSERT(EnergySystem::BUFFER_THRESHOLD_PERCENT < 1.0f);
    ASSERT(EnergySystem::COLLAPSE_THRESHOLD_PERCENT > 0.0f);
    ASSERT(EnergySystem::COLLAPSE_THRESHOLD_PERCENT <= 1.0f);

    // Verify default values
    // Use approximate comparison for float
    ASSERT(EnergySystem::BUFFER_THRESHOLD_PERCENT >= 0.09f);
    ASSERT(EnergySystem::BUFFER_THRESHOLD_PERCENT <= 0.11f);
    ASSERT(EnergySystem::COLLAPSE_THRESHOLD_PERCENT >= 0.49f);
    ASSERT(EnergySystem::COLLAPSE_THRESHOLD_PERCENT <= 0.51f);
}

// =============================================================================
// tick() integration: state calculated after pool aggregation
// =============================================================================

TEST(tick_sets_pool_state_healthy) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    // Nexus at (10,10), consumer at (12,10) within coverage radius 8
    create_nexus_at(reg, sys, 0, 1000, 10, 10, true);
    create_consumer_no_coverage(reg, sys, 0, 12, 10, 100);

    sys.tick(0.05f);

    // Large surplus => Healthy
    ASSERT_EQ(sys.get_pool_state(0), EnergyPoolState::Healthy);
}

TEST(tick_sets_pool_state_marginal) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    // generated ~1000 (slightly less due to aging), consumed=950
    // surplus ~50, buffer_threshold ~100 => Marginal
    create_nexus_at(reg, sys, 0, 1000, 10, 10, true);
    create_consumer_no_coverage(reg, sys, 0, 12, 10, 950);

    sys.tick(0.05f);

    // surplus should be small positive, below 10% of generated => Marginal
    const auto& pool = sys.get_pool(0);
    ASSERT(pool.surplus >= 0);
    ASSERT(pool.surplus < static_cast<int32_t>(pool.total_generated));
    ASSERT_EQ(pool.state, EnergyPoolState::Marginal);
}

TEST(tick_sets_pool_state_deficit) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    // generated ~1000, consumed=1100 => surplus ~-100
    // collapse_threshold = 1100 * 0.50 = 550
    // -550 < -100 < 0 => Deficit
    create_nexus_at(reg, sys, 0, 1000, 10, 10, true);
    create_consumer_no_coverage(reg, sys, 0, 12, 10, 1100);

    sys.tick(0.05f);

    ASSERT_EQ(sys.get_pool_state(0), EnergyPoolState::Deficit);
}

TEST(tick_sets_pool_state_collapse) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    // generated ~100, consumed=3000 => surplus ~-2900
    // collapse_threshold = 3000 * 0.50 = 1500
    // surplus(-2900) <= -1500 => Collapse
    create_nexus_at(reg, sys, 0, 100, 10, 10, true);
    create_consumer_no_coverage(reg, sys, 0, 12, 10, 3000);

    sys.tick(0.05f);

    ASSERT_EQ(sys.get_pool_state(0), EnergyPoolState::Collapse);
}

TEST(tick_updates_previous_state_across_ticks) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    // Start healthy
    create_nexus_at(reg, sys, 0, 1000, 10, 10, true);
    uint32_t consumer_eid = create_consumer_no_coverage(reg, sys, 0, 12, 10, 100);

    sys.tick(0.05f);
    ASSERT_EQ(sys.get_pool_state(0), EnergyPoolState::Healthy);

    // Push into collapse
    auto consumer_entity = static_cast<entt::entity>(consumer_eid);
    auto* ec = reg.try_get<EnergyComponent>(consumer_entity);
    ec->energy_required = 5000;

    sys.tick(0.05f);
    ASSERT_EQ(sys.get_pool_state(0), EnergyPoolState::Collapse);
    ASSERT_EQ(sys.get_pool(0).previous_state, EnergyPoolState::Collapse);

    // Recover to healthy
    ec->energy_required = 100;
    sys.tick(0.05f);
    ASSERT_EQ(sys.get_pool_state(0), EnergyPoolState::Healthy);
    ASSERT_EQ(sys.get_pool(0).previous_state, EnergyPoolState::Healthy);
}

TEST(tick_empty_player_stays_healthy) {
    entt::registry reg;
    EnergySystem sys(64, 64);
    sys.set_registry(&reg);

    sys.tick(0.05f);

    // No nexuses, no consumers: 0/0 => Healthy
    for (uint8_t i = 0; i < MAX_PLAYERS; ++i) {
        ASSERT_EQ(sys.get_pool_state(i), EnergyPoolState::Healthy);
    }
}

// =============================================================================
// Main Entry Point
// =============================================================================

int main() {
    printf("=== Pool State Machine Unit Tests (Ticket 5-013) ===\n\n");

    // calculate_pool_state: Healthy
    RUN_TEST(healthy_large_surplus);
    RUN_TEST(healthy_exact_buffer_threshold);
    RUN_TEST(healthy_no_consumers);
    RUN_TEST(healthy_zero_generation_zero_consumption);

    // calculate_pool_state: Marginal
    RUN_TEST(marginal_just_below_buffer);
    RUN_TEST(marginal_exact_balance);
    RUN_TEST(marginal_tiny_surplus);

    // calculate_pool_state: Deficit
    RUN_TEST(deficit_small_negative_surplus);
    RUN_TEST(deficit_moderate_negative_surplus);

    // calculate_pool_state: Collapse
    RUN_TEST(collapse_large_deficit);
    RUN_TEST(collapse_exact_threshold);
    RUN_TEST(collapse_no_generation);
    RUN_TEST(collapse_zero_consumed_zero_generated_is_healthy);

    // detect_pool_state_transitions
    RUN_TEST(detect_transitions_updates_previous_state);
    RUN_TEST(detect_transitions_no_change);
    RUN_TEST(detect_transitions_healthy_to_deficit);
    RUN_TEST(detect_transitions_healthy_to_collapse);
    RUN_TEST(detect_transitions_deficit_to_healthy);
    RUN_TEST(detect_transitions_collapse_to_healthy);
    RUN_TEST(detect_transitions_collapse_to_marginal);
    RUN_TEST(detect_transitions_deficit_to_collapse);
    RUN_TEST(detect_transitions_collapse_to_deficit);
    RUN_TEST(detect_transitions_invalid_owner_no_crash);

    // Threshold constants
    RUN_TEST(threshold_constants_are_correct);

    // tick() integration
    RUN_TEST(tick_sets_pool_state_healthy);
    RUN_TEST(tick_sets_pool_state_marginal);
    RUN_TEST(tick_sets_pool_state_deficit);
    RUN_TEST(tick_sets_pool_state_collapse);
    RUN_TEST(tick_updates_previous_state_across_ticks);
    RUN_TEST(tick_empty_player_stays_healthy);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
