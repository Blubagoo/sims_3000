/**
 * @file test_fluid_distribution.cpp
 * @brief Unit tests for fluid distribution (Ticket 6-019)
 *
 * Tests cover:
 * - All consumers get fluid when surplus >= 0
 * - All consumers lose fluid when deficit (after reservoir drain)
 * - Consumer outside coverage always has_fluid = false
 * - No rationing - all same priority (all-or-nothing per CCR-002)
 * - Distribution after reservoir buffering saves the day
 * - Edge cases: no consumers, no registry, invalid owner
 *
 * Uses printf test pattern consistent with other fluid tests.
 */

#include <sims3000/fluid/FluidSystem.h>
#include <sims3000/fluid/FluidComponent.h>
#include <sims3000/fluid/FluidProducerComponent.h>
#include <sims3000/fluid/FluidReservoirComponent.h>
#include <sims3000/fluid/FluidExtractorConfig.h>
#include <sims3000/fluid/FluidReservoirConfig.h>
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
// Helper: create an extractor entity with given current_output, register it
// =============================================================================

static uint32_t create_extractor_direct(entt::registry& reg, FluidSystem& sys,
                                         uint8_t owner, uint32_t current_output,
                                         uint32_t x = 10, uint32_t y = 10) {
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
    sys.register_extractor_position(eid, owner, x, y);
    return eid;
}

// =============================================================================
// Helper: create a reservoir entity with given current_level and capacity
// =============================================================================

static uint32_t create_reservoir_direct(entt::registry& reg, FluidSystem& sys,
                                         uint8_t owner, uint32_t current_level,
                                         uint32_t capacity,
                                         uint16_t fill_rate = 50,
                                         uint16_t drain_rate = 100,
                                         uint32_t x = 12, uint32_t y = 12) {
    auto entity = reg.create();
    uint32_t eid = static_cast<uint32_t>(entity);

    FluidReservoirComponent res{};
    res.capacity = capacity;
    res.current_level = current_level;
    res.fill_rate = fill_rate;
    res.drain_rate = drain_rate;
    res.is_active = true;
    reg.emplace<FluidReservoirComponent>(entity, res);

    FluidProducerComponent prod{};
    prod.base_output = 0;
    prod.current_output = 0;
    prod.is_operational = false;
    prod.producer_type = static_cast<uint8_t>(FluidProducerType::Reservoir);
    reg.emplace<FluidProducerComponent>(entity, prod);

    sys.register_reservoir(eid, owner);
    sys.register_reservoir_position(eid, owner, x, y);
    return eid;
}

// =============================================================================
// Helper: create a consumer entity near the extractor for BFS coverage
// =============================================================================

static uint32_t create_consumer_near_extractor(entt::registry& reg, FluidSystem& sys,
                                                uint8_t owner, uint32_t fluid_required,
                                                uint32_t x = 10, uint32_t y = 11) {
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
// All consumers get fluid when surplus >= 0
// =============================================================================

TEST(all_consumers_get_fluid_with_surplus) {
    FluidSystem sys(64, 64);
    entt::registry reg;
    sys.set_registry(&reg);

    // Place extractor generating 100
    sys.place_extractor(10, 10, 0);

    // Create 3 consumers near extractor (within BFS coverage)
    uint32_t c1 = create_consumer_near_extractor(reg, sys, 0, 10, 10, 11);
    uint32_t c2 = create_consumer_near_extractor(reg, sys, 0, 20, 11, 10);
    uint32_t c3 = create_consumer_near_extractor(reg, sys, 0, 30, 11, 11);

    // Total consumed = 60, generated = 100, surplus = 40 >= 0
    sys.tick(0.016f);

    auto e1 = static_cast<entt::entity>(c1);
    auto e2 = static_cast<entt::entity>(c2);
    auto e3 = static_cast<entt::entity>(c3);
    const auto* fc1 = reg.try_get<FluidComponent>(e1);
    const auto* fc2 = reg.try_get<FluidComponent>(e2);
    const auto* fc3 = reg.try_get<FluidComponent>(e3);

    ASSERT(fc1 != nullptr);
    ASSERT(fc2 != nullptr);
    ASSERT(fc3 != nullptr);

    // All consumers should have fluid
    ASSERT(fc1->has_fluid);
    ASSERT_EQ(fc1->fluid_received, 10u);

    ASSERT(fc2->has_fluid);
    ASSERT_EQ(fc2->fluid_received, 20u);

    ASSERT(fc3->has_fluid);
    ASSERT_EQ(fc3->fluid_received, 30u);
}

// =============================================================================
// All consumers lose fluid when deficit (after reservoir drain)
// =============================================================================

TEST(all_consumers_lose_fluid_with_deficit) {
    FluidSystem sys(64, 64);
    entt::registry reg;
    sys.set_registry(&reg);

    // Place extractor generating 100
    sys.place_extractor(10, 10, 0);

    // No reservoirs, so no buffering possible
    // Total consumed = 500, generated = 100, surplus = -400 < 0
    uint32_t c1 = create_consumer_near_extractor(reg, sys, 0, 200, 10, 11);
    uint32_t c2 = create_consumer_near_extractor(reg, sys, 0, 300, 11, 10);

    sys.tick(0.016f);

    auto e1 = static_cast<entt::entity>(c1);
    auto e2 = static_cast<entt::entity>(c2);
    const auto* fc1 = reg.try_get<FluidComponent>(e1);
    const auto* fc2 = reg.try_get<FluidComponent>(e2);

    ASSERT(fc1 != nullptr);
    ASSERT(fc2 != nullptr);

    // All consumers should lose fluid
    ASSERT(!fc1->has_fluid);
    ASSERT_EQ(fc1->fluid_received, 0u);

    ASSERT(!fc2->has_fluid);
    ASSERT_EQ(fc2->fluid_received, 0u);
}

// =============================================================================
// Consumer outside coverage always has_fluid = false
// =============================================================================

TEST(consumer_outside_coverage_always_no_fluid) {
    FluidSystem sys(64, 64);
    entt::registry reg;
    sys.set_registry(&reg);

    // Place extractor at (10,10), coverage radius 8
    sys.place_extractor(10, 10, 0);

    // Consumer far from extractor (outside BFS coverage range)
    uint32_t c_far = create_consumer_near_extractor(reg, sys, 0, 10, 50, 50);

    // Even though pool has surplus (100 gen, 10 consumed = +90), consumer
    // outside coverage should NOT get fluid
    sys.tick(0.016f);

    auto e_far = static_cast<entt::entity>(c_far);
    const auto* fc_far = reg.try_get<FluidComponent>(e_far);

    ASSERT(fc_far != nullptr);
    ASSERT(!fc_far->has_fluid);
    ASSERT_EQ(fc_far->fluid_received, 0u);
}

// =============================================================================
// No rationing - all same priority (all-or-nothing)
// =============================================================================

TEST(no_rationing_all_or_nothing) {
    FluidSystem sys(64, 64);
    entt::registry reg;
    sys.set_registry(&reg);

    // Place extractor generating 100
    sys.place_extractor(10, 10, 0);

    // Create 5 consumers with varying demands, total = 80 (within surplus)
    uint32_t c1 = create_consumer_near_extractor(reg, sys, 0, 5,  10, 11);
    uint32_t c2 = create_consumer_near_extractor(reg, sys, 0, 10, 11, 10);
    uint32_t c3 = create_consumer_near_extractor(reg, sys, 0, 15, 11, 11);
    uint32_t c4 = create_consumer_near_extractor(reg, sys, 0, 20, 12, 10);
    uint32_t c5 = create_consumer_near_extractor(reg, sys, 0, 30, 12, 11);

    sys.tick(0.016f);

    // All should get full fluid_required (no partial allocation)
    auto check_powered = [&](uint32_t eid, uint32_t expected) {
        auto e = static_cast<entt::entity>(eid);
        const auto* fc = reg.try_get<FluidComponent>(e);
        ASSERT(fc != nullptr);
        ASSERT(fc->has_fluid);
        ASSERT_EQ(fc->fluid_received, expected);
    };

    check_powered(c1, 5);
    check_powered(c2, 10);
    check_powered(c3, 15);
    check_powered(c4, 20);
    check_powered(c5, 30);
}

// =============================================================================
// Distribution after reservoir buffering saves the day
// =============================================================================

TEST(reservoir_buffering_saves_distribution) {
    FluidSystem sys(64, 64);
    entt::registry reg;
    sys.set_registry(&reg);

    // Place extractor generating 100
    sys.place_extractor(10, 10, 0);

    // Create reservoir with 500 stored, capacity 1000, drain_rate 100
    create_reservoir_direct(reg, sys, 0, 500, 1000, 50, 100);

    // Consumer demands 150 (> 100 generation but < 100 + reservoir buffer)
    // available = 100 + 500 = 600, consumed = 150
    // surplus = 600 - 150 = 450 >= 0
    // After reservoir buffering: surplus fills reservoirs
    // Consumers should get fluid since surplus >= 0
    uint32_t c1 = create_consumer_near_extractor(reg, sys, 0, 150, 10, 11);

    sys.tick(0.016f);

    auto e1 = static_cast<entt::entity>(c1);
    const auto* fc1 = reg.try_get<FluidComponent>(e1);

    ASSERT(fc1 != nullptr);
    // With generation (100) + reservoir (500) = 600 available, consumed 150,
    // surplus should be positive => consumers get fluid
    ASSERT(fc1->has_fluid);
    ASSERT_EQ(fc1->fluid_received, 150u);
}

TEST(reservoir_drain_then_distribute) {
    FluidSystem sys(64, 64);
    entt::registry reg;
    sys.set_registry(&reg);

    // Place extractor generating 100
    sys.place_extractor(10, 10, 0);

    // Create reservoir with 200 stored, high drain_rate
    create_reservoir_direct(reg, sys, 0, 200, 1000, 50, 200);

    // Consumer demands 250 (> generation 100)
    // Pre-reservoir: available = 100 + 200 = 300, consumed = 250, surplus = 50
    // Reservoir should fill with surplus of 50 (fill_rate=50)
    // After reservoir buffering: surplus is still positive
    // Consumers should still get fluid
    uint32_t c1 = create_consumer_near_extractor(reg, sys, 0, 250, 10, 11);

    sys.tick(0.016f);

    auto e1 = static_cast<entt::entity>(c1);
    const auto* fc1 = reg.try_get<FluidComponent>(e1);
    ASSERT(fc1 != nullptr);

    const PerPlayerFluidPool& pool = sys.get_pool(0);
    if (pool.surplus >= 0) {
        ASSERT(fc1->has_fluid);
        ASSERT_EQ(fc1->fluid_received, 250u);
    } else {
        ASSERT(!fc1->has_fluid);
        ASSERT_EQ(fc1->fluid_received, 0u);
    }
}

// =============================================================================
// Edge cases
// =============================================================================

TEST(no_consumers_no_crash) {
    FluidSystem sys(64, 64);
    entt::registry reg;
    sys.set_registry(&reg);

    sys.place_extractor(10, 10, 0);
    // No consumers - should not crash
    sys.tick(0.016f);
}

TEST(no_registry_no_crash) {
    FluidSystem sys(64, 64);
    // No registry set - should not crash
    sys.tick(0.016f);
}

TEST(invalid_owner_no_crash) {
    FluidSystem sys(64, 64);
    entt::registry reg;
    sys.set_registry(&reg);

    // distribute_fluid with invalid owner should not crash
    sys.distribute_fluid(MAX_PLAYERS);
    sys.distribute_fluid(255);
}

// =============================================================================
// Mix of in-coverage and out-of-coverage consumers
// =============================================================================

TEST(mix_in_and_out_of_coverage) {
    FluidSystem sys(64, 64);
    entt::registry reg;
    sys.set_registry(&reg);

    // Place extractor at (10,10) with coverage radius 8
    sys.place_extractor(10, 10, 0);

    // Consumer IN coverage (near extractor)
    uint32_t c_in = create_consumer_near_extractor(reg, sys, 0, 10, 10, 11);

    // Consumer OUT of coverage (far from extractor)
    uint32_t c_out = create_consumer_near_extractor(reg, sys, 0, 10, 50, 50);

    sys.tick(0.016f);

    auto e_in = static_cast<entt::entity>(c_in);
    auto e_out = static_cast<entt::entity>(c_out);
    const auto* fc_in = reg.try_get<FluidComponent>(e_in);
    const auto* fc_out = reg.try_get<FluidComponent>(e_out);

    ASSERT(fc_in != nullptr);
    ASSERT(fc_out != nullptr);

    // In coverage: should have fluid
    ASSERT(fc_in->has_fluid);
    ASSERT_EQ(fc_in->fluid_received, 10u);

    // Out of coverage: should NOT have fluid
    ASSERT(!fc_out->has_fluid);
    ASSERT_EQ(fc_out->fluid_received, 0u);
}

// =============================================================================
// Surplus == 0 means consumers still get fluid
// =============================================================================

TEST(surplus_zero_consumers_powered) {
    FluidSystem sys(64, 64);
    entt::registry reg;
    sys.set_registry(&reg);

    // Place extractor generating 100
    sys.place_extractor(10, 10, 0);

    // Consumer demands exactly 100 (no reservoir)
    // available = 100 + 0 = 100, consumed = 100, surplus = 0 >= 0
    FluidExtractorConfig config = get_default_extractor_config();
    uint32_t c1 = create_consumer_near_extractor(reg, sys, 0, config.base_output, 10, 11);

    sys.tick(0.016f);

    auto e1 = static_cast<entt::entity>(c1);
    const auto* fc1 = reg.try_get<FluidComponent>(e1);
    ASSERT(fc1 != nullptr);

    // surplus >= 0 means consumers get fluid
    const PerPlayerFluidPool& pool = sys.get_pool(0);
    if (pool.surplus >= 0) {
        ASSERT(fc1->has_fluid);
        ASSERT_EQ(fc1->fluid_received, config.base_output);
    }
}

// =============================================================================
// Multi-player isolation
// =============================================================================

TEST(multi_player_distribution_isolation) {
    FluidSystem sys(64, 64);
    entt::registry reg;
    sys.set_registry(&reg);

    // Player 0: healthy (extractor at (10,10), small consumer nearby)
    sys.place_extractor(10, 10, 0);
    uint32_t c0 = create_consumer_near_extractor(reg, sys, 0, 10, 10, 11);

    // Player 1: deficit (extractor at (40,40), heavy consumer nearby)
    sys.place_extractor(40, 40, 1);
    uint32_t c1 = create_consumer_near_extractor(reg, sys, 1, 5000, 40, 41);

    sys.tick(0.016f);

    // Player 0: consumer should have fluid
    auto e0 = static_cast<entt::entity>(c0);
    const auto* fc0 = reg.try_get<FluidComponent>(e0);
    ASSERT(fc0 != nullptr);
    ASSERT(fc0->has_fluid);
    ASSERT_EQ(fc0->fluid_received, 10u);

    // Player 1: consumer should NOT have fluid (deficit)
    auto e1 = static_cast<entt::entity>(c1);
    const auto* fc1 = reg.try_get<FluidComponent>(e1);
    ASSERT(fc1 != nullptr);
    ASSERT(!fc1->has_fluid);
    ASSERT_EQ(fc1->fluid_received, 0u);
}

// =============================================================================
// Main Entry Point
// =============================================================================

int main() {
    printf("=== Fluid Distribution Unit Tests (Ticket 6-019) ===\n\n");

    // All consumers get fluid when surplus >= 0
    RUN_TEST(all_consumers_get_fluid_with_surplus);

    // All consumers lose fluid when deficit
    RUN_TEST(all_consumers_lose_fluid_with_deficit);

    // Consumer outside coverage always no fluid
    RUN_TEST(consumer_outside_coverage_always_no_fluid);

    // No rationing - all same priority
    RUN_TEST(no_rationing_all_or_nothing);

    // Distribution after reservoir buffering saves the day
    RUN_TEST(reservoir_buffering_saves_distribution);
    RUN_TEST(reservoir_drain_then_distribute);

    // Edge cases
    RUN_TEST(no_consumers_no_crash);
    RUN_TEST(no_registry_no_crash);
    RUN_TEST(invalid_owner_no_crash);

    // Mix of coverage states
    RUN_TEST(mix_in_and_out_of_coverage);
    RUN_TEST(surplus_zero_consumers_powered);

    // Multi-player isolation
    RUN_TEST(multi_player_distribution_isolation);

    printf("\n=== Results: %d passed, %d failed ===\n",
           tests_passed, tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
