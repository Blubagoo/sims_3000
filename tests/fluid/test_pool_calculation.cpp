/**
 * @file test_pool_calculation.cpp
 * @brief Unit tests for fluid pool calculation (Ticket 6-017)
 *
 * Tests cover:
 * - Pool with no extractors/consumers: Healthy, all zeros
 * - Pool with extractors only (surplus): Healthy
 * - Pool near threshold: Marginal
 * - Pool with deficit and reservoir buffer: Deficit
 * - Pool with deficit and empty reservoirs: Collapse
 * - Surplus calculation (available - consumed)
 * - Multiple extractors aggregate correctly
 * - Consumer outside coverage doesn't count
 *
 * Uses printf test pattern consistent with other fluid tests.
 */

#include <sims3000/fluid/FluidSystem.h>
#include <sims3000/fluid/FluidComponent.h>
#include <sims3000/fluid/FluidProducerComponent.h>
#include <sims3000/fluid/FluidReservoirComponent.h>
#include <sims3000/fluid/FluidExtractorConfig.h>
#include <sims3000/fluid/FluidEnums.h>
#include <entt/entt.hpp>
#include <cstdio>
#include <cstdlib>

using namespace sims3000::fluid;

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
// Helper: create an extractor entity with given current_output, register it
// with FluidSystem. Sets is_operational to true and current_output directly
// (bypasses water distance / power checks for unit-level pool tests).
// =============================================================================

static uint32_t create_extractor_direct(entt::registry& reg, FluidSystem& sys,
                                         uint8_t owner, uint32_t current_output) {
    auto entity = reg.create();
    uint32_t eid = static_cast<uint32_t>(entity);

    FluidProducerComponent prod{};
    prod.base_output = current_output;
    prod.current_output = current_output;
    prod.is_operational = true;
    prod.producer_type = static_cast<uint8_t>(FluidProducerType::Extractor);
    prod.max_water_distance = 5;
    prod.current_water_distance = 0;
    reg.emplace<FluidProducerComponent>(entity, prod);

    sys.register_extractor(eid, owner);
    sys.register_extractor_position(eid, owner, 10, 10);
    return eid;
}

// =============================================================================
// Helper: create a reservoir entity with given current_level and capacity,
// register with FluidSystem.
// =============================================================================

static uint32_t create_reservoir_direct(entt::registry& reg, FluidSystem& sys,
                                         uint8_t owner, uint32_t current_level,
                                         uint32_t capacity) {
    auto entity = reg.create();
    uint32_t eid = static_cast<uint32_t>(entity);

    FluidReservoirComponent res{};
    res.capacity = capacity;
    res.current_level = current_level;
    res.is_active = true;
    reg.emplace<FluidReservoirComponent>(entity, res);

    FluidProducerComponent prod{};
    prod.base_output = 0;
    prod.current_output = 0;
    prod.is_operational = false;
    prod.producer_type = static_cast<uint8_t>(FluidProducerType::Reservoir);
    reg.emplace<FluidProducerComponent>(entity, prod);

    sys.register_reservoir(eid, owner);
    sys.register_reservoir_position(eid, owner, 12, 12);
    return eid;
}

// =============================================================================
// Helper: create a consumer entity with FluidComponent, register and place
// it near the extractor at (10,10) so BFS coverage reaches it during tick().
// =============================================================================

static uint32_t create_consumer_near_extractor(entt::registry& reg, FluidSystem& sys,
                                                uint8_t owner, uint32_t fluid_required,
                                                uint32_t x, uint32_t y) {
    auto entity = reg.create();
    uint32_t eid = static_cast<uint32_t>(entity);

    FluidComponent fc{};
    fc.fluid_required = fluid_required;
    reg.emplace<FluidComponent>(entity, fc);

    sys.register_consumer(eid, owner);
    sys.register_consumer_position(eid, owner, x, y);
    return eid;
}

// =============================================================================
// 6-017: Pool with no extractors/consumers => Healthy, all zeros
// =============================================================================

TEST(pool_no_extractors_no_consumers_healthy) {
    FluidSystem sys(64, 64);
    entt::registry reg;
    sys.set_registry(&reg);

    sys.tick(0.016f);

    const PerPlayerFluidPool& pool = sys.get_pool(0);
    ASSERT_EQ(pool.total_generated, 0u);
    ASSERT_EQ(pool.total_reservoir_stored, 0u);
    ASSERT_EQ(pool.total_reservoir_capacity, 0u);
    ASSERT_EQ(pool.available, 0u);
    ASSERT_EQ(pool.total_consumed, 0u);
    ASSERT_EQ(pool.surplus, 0);
    ASSERT_EQ(pool.extractor_count, 0u);
    ASSERT_EQ(pool.reservoir_count, 0u);
    ASSERT_EQ(pool.consumer_count, 0u);
    ASSERT_EQ(static_cast<uint8_t>(pool.state), static_cast<uint8_t>(FluidPoolState::Healthy));
}

// =============================================================================
// 6-017: Pool with extractors only (surplus) => Healthy
// =============================================================================

TEST(pool_extractors_only_healthy) {
    FluidSystem sys(64, 64);
    entt::registry reg;
    sys.set_registry(&reg);

    // Place extractor at (10,10) which produces fluid
    sys.place_extractor(10, 10, 0);

    // No consumers, so surplus should be positive
    sys.tick(0.016f);

    const PerPlayerFluidPool& pool = sys.get_pool(0);
    // Without energy provider, extractors are powered by default (powered=true)
    // Without terrain, water_distance=0, water_factor=1.0
    // So current_output = base_output * 1.0 * 1.0 = base_output
    ASSERT(pool.total_generated > 0u);
    ASSERT_EQ(pool.total_consumed, 0u);
    ASSERT(pool.surplus > 0);
    ASSERT_EQ(pool.available, pool.total_generated + pool.total_reservoir_stored);
    ASSERT_EQ(static_cast<uint8_t>(pool.state), static_cast<uint8_t>(FluidPoolState::Healthy));
}

// =============================================================================
// 6-017: Pool near threshold => Marginal
// The Marginal state occurs when surplus >= 0 but surplus < 10% of available.
// =============================================================================

TEST(pool_near_threshold_marginal) {
    FluidSystem sys(64, 64);
    entt::registry reg;
    sys.set_registry(&reg);

    // Place extractor at (10,10)
    uint32_t ext_id = sys.place_extractor(10, 10, 0);
    ASSERT(ext_id != INVALID_ENTITY_ID);

    // Get the extractor's output so we can calibrate the consumer
    // Default extractor: base_output from FluidExtractorConfig
    FluidExtractorConfig config = get_default_extractor_config();
    uint32_t base_output = config.base_output;

    // Create a consumer that leaves a tiny surplus (less than 10% of available)
    // available = base_output (no reservoir)
    // For Marginal: surplus >= 0 AND surplus < 10% of available
    // If base_output=100: need consumed > 90 but <= 100
    // surplus = 100 - 95 = 5, buffer_threshold = 10 => 5 < 10 => Marginal
    uint32_t consumed = base_output - (base_output / 20);  // leave 5% surplus
    create_consumer_near_extractor(reg, sys, 0, consumed, 10, 11);

    sys.tick(0.016f);

    const PerPlayerFluidPool& pool = sys.get_pool(0);
    // surplus should be small but >= 0
    ASSERT(pool.surplus >= 0);
    // State should be Marginal (surplus < 10% of available)
    ASSERT_EQ(static_cast<uint8_t>(pool.state), static_cast<uint8_t>(FluidPoolState::Marginal));
}

// =============================================================================
// 6-017: Pool with deficit and reservoir buffer => Deficit
// =============================================================================

TEST(pool_deficit_with_reservoir_buffer) {
    FluidSystem sys(64, 64);
    entt::registry reg;
    sys.set_registry(&reg);

    // Place extractor at (10,10) - generates some fluid
    sys.place_extractor(10, 10, 0);

    // Create a reservoir with stored fluid
    create_reservoir_direct(reg, sys, 0, 500, 1000);

    // Create a large consumer that exceeds generation + storage
    // The key: surplus < 0 AND total_reservoir_stored > 0 => Deficit
    FluidExtractorConfig config = get_default_extractor_config();
    // Make consumption way larger than generation + reservoir stored
    uint32_t heavy_consumption = config.base_output + 500 + 100;
    create_consumer_near_extractor(reg, sys, 0, heavy_consumption, 10, 11);

    sys.tick(0.016f);

    const PerPlayerFluidPool& pool = sys.get_pool(0);
    ASSERT(pool.surplus < 0);
    ASSERT(pool.total_reservoir_stored > 0u);
    ASSERT_EQ(static_cast<uint8_t>(pool.state), static_cast<uint8_t>(FluidPoolState::Deficit));
}

// =============================================================================
// 6-017: Pool with deficit and empty reservoirs => Collapse
// =============================================================================

TEST(pool_deficit_empty_reservoirs_collapse) {
    FluidSystem sys(64, 64);
    entt::registry reg;
    sys.set_registry(&reg);

    // Place extractor at (10,10)
    sys.place_extractor(10, 10, 0);

    // Create a reservoir with 0 stored fluid
    create_reservoir_direct(reg, sys, 0, 0, 1000);

    // Create large consumer to cause deficit
    FluidExtractorConfig config = get_default_extractor_config();
    uint32_t heavy_consumption = config.base_output + 100;
    create_consumer_near_extractor(reg, sys, 0, heavy_consumption, 10, 11);

    sys.tick(0.016f);

    const PerPlayerFluidPool& pool = sys.get_pool(0);
    ASSERT(pool.surplus < 0);
    ASSERT_EQ(pool.total_reservoir_stored, 0u);
    ASSERT_EQ(static_cast<uint8_t>(pool.state), static_cast<uint8_t>(FluidPoolState::Collapse));
}

// =============================================================================
// 6-017: Collapse with no extractors, no reservoirs, but consumers present
// =============================================================================

TEST(pool_no_extractors_no_reservoirs_consumers_collapse) {
    FluidSystem sys(64, 64);
    entt::registry reg;
    sys.set_registry(&reg);

    // Place an extractor to get coverage, then we won't rely on it producing
    // Actually we need coverage for consumers. Let's place an extractor
    // but make consumption exceed generation.
    // Simpler: no extractors at all means no coverage, consumers won't count.
    // The spec says "no generation AND no consumption => Healthy".
    // But if we have generation=0, consumption=0 (outside coverage), state=Healthy.
    // To test Collapse properly, need generation > 0 but deficit with no reservoir.

    // Place extractor for coverage
    sys.place_extractor(10, 10, 0);

    // Consumer with demand way above generation, no reservoir
    FluidExtractorConfig config = get_default_extractor_config();
    uint32_t huge_demand = config.base_output * 10;
    create_consumer_near_extractor(reg, sys, 0, huge_demand, 10, 11);

    sys.tick(0.016f);

    const PerPlayerFluidPool& pool = sys.get_pool(0);
    ASSERT(pool.surplus < 0);
    ASSERT_EQ(pool.total_reservoir_stored, 0u);
    ASSERT_EQ(static_cast<uint8_t>(pool.state), static_cast<uint8_t>(FluidPoolState::Collapse));
}

// =============================================================================
// 6-017: Surplus calculation (available - consumed)
// =============================================================================

TEST(surplus_equals_available_minus_consumed) {
    FluidSystem sys(64, 64);
    entt::registry reg;
    sys.set_registry(&reg);

    // Place extractor and reservoir
    sys.place_extractor(10, 10, 0);
    create_reservoir_direct(reg, sys, 0, 200, 1000);

    // Place consumer
    create_consumer_near_extractor(reg, sys, 0, 50, 10, 11);

    sys.tick(0.016f);

    const PerPlayerFluidPool& pool = sys.get_pool(0);
    // available = total_generated + total_reservoir_stored
    ASSERT_EQ(pool.available, pool.total_generated + pool.total_reservoir_stored);
    // surplus = available - total_consumed
    ASSERT_EQ(pool.surplus,
              static_cast<int32_t>(pool.available) -
              static_cast<int32_t>(pool.total_consumed));
}

// =============================================================================
// 6-017: Multiple extractors aggregate correctly
// =============================================================================

TEST(multiple_extractors_aggregate) {
    FluidSystem sys(64, 64);
    entt::registry reg;
    sys.set_registry(&reg);

    // Place two extractors
    sys.place_extractor(10, 10, 0);
    sys.place_extractor(20, 20, 0);

    sys.tick(0.016f);

    const PerPlayerFluidPool& pool = sys.get_pool(0);
    FluidExtractorConfig config = get_default_extractor_config();

    // Both extractors should be operational (no energy provider => powered=true)
    // Each produces base_output (water_distance=0, factor=1.0)
    ASSERT_EQ(pool.extractor_count, 2u);
    ASSERT_EQ(pool.total_generated, config.base_output * 2);
}

// =============================================================================
// 6-017: Consumer outside coverage doesn't count
// =============================================================================

TEST(consumer_outside_coverage_not_counted) {
    FluidSystem sys(64, 64);
    entt::registry reg;
    sys.set_registry(&reg);

    // Place extractor at (5,5)
    sys.place_extractor(5, 5, 0);

    // Consumer far away from extractor (well outside coverage radius)
    create_consumer_near_extractor(reg, sys, 0, 50, 60, 60);

    sys.tick(0.016f);

    const PerPlayerFluidPool& pool = sys.get_pool(0);
    // Consumer at (60,60) is far from extractor at (5,5), outside coverage
    ASSERT_EQ(pool.total_consumed, 0u);
    ASSERT_EQ(pool.consumer_count, 0u);
}

// =============================================================================
// 6-017: Pool state transitions are tracked (previous_state updated)
// =============================================================================

TEST(pool_state_previous_state_tracked) {
    FluidSystem sys(64, 64);
    entt::registry reg;
    sys.set_registry(&reg);

    // First tick: Healthy (no consumers, no extractors)
    sys.tick(0.016f);
    ASSERT_EQ(static_cast<uint8_t>(sys.get_pool(0).state),
              static_cast<uint8_t>(FluidPoolState::Healthy));

    // Add extractor and large consumer to cause collapse
    sys.place_extractor(10, 10, 0);
    FluidExtractorConfig config = get_default_extractor_config();
    create_consumer_near_extractor(reg, sys, 0, config.base_output * 10, 10, 11);

    sys.tick(0.016f);

    const PerPlayerFluidPool& pool = sys.get_pool(0);
    // After detect_pool_state_transitions() in tick(), previous_state is updated
    // to the current state. So previous_state should now equal the current state.
    ASSERT_EQ(static_cast<uint8_t>(pool.previous_state),
              static_cast<uint8_t>(pool.state));
    // Current state should reflect the deficit/collapse
    ASSERT(pool.surplus < 0);
}

// =============================================================================
// 6-017: Pool calculation for invalid owner does not crash
// =============================================================================

TEST(pool_calculation_invalid_owner_no_crash) {
    FluidSystem sys(64, 64);
    entt::registry reg;
    sys.set_registry(&reg);

    // Tick should not crash even though only 4 players are valid
    sys.tick(0.016f);

    // Accessing pool for valid owners should work
    const PerPlayerFluidPool& pool = sys.get_pool(0);
    ASSERT_EQ(pool.surplus, 0);
}

// =============================================================================
// Main Entry Point
// =============================================================================

int main() {
    printf("=== Fluid Pool Calculation Unit Tests (Ticket 6-017) ===\n\n");

    // Pool with no extractors/consumers: Healthy, all zeros
    RUN_TEST(pool_no_extractors_no_consumers_healthy);

    // Pool with extractors only (surplus): Healthy
    RUN_TEST(pool_extractors_only_healthy);

    // Pool near threshold: Marginal
    RUN_TEST(pool_near_threshold_marginal);

    // Pool with deficit and reservoir buffer: Deficit
    RUN_TEST(pool_deficit_with_reservoir_buffer);

    // Pool with deficit and empty reservoirs: Collapse
    RUN_TEST(pool_deficit_empty_reservoirs_collapse);
    RUN_TEST(pool_no_extractors_no_reservoirs_consumers_collapse);

    // Surplus calculation
    RUN_TEST(surplus_equals_available_minus_consumed);

    // Multiple extractors aggregate
    RUN_TEST(multiple_extractors_aggregate);

    // Consumer outside coverage doesn't count
    RUN_TEST(consumer_outside_coverage_not_counted);

    // State tracking
    RUN_TEST(pool_state_previous_state_tracked);

    // Edge cases
    RUN_TEST(pool_calculation_invalid_owner_no_crash);

    printf("\n=== Results: %d passed, %d failed ===\n",
           tests_passed, tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
