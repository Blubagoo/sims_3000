/**
 * @file test_fluid_reservoir_config.cpp
 * @brief Unit tests for FluidReservoirConfig (Ticket 6-024)
 *
 * Tests cover:
 * - Default config values match named constants
 * - get_default_reservoir_config() returns correct values
 * - Named constants have expected values per spec
 * - Asymmetric rates per CCR-005: drain_rate (100) > fill_rate (50)
 * - Passive storage (requires_energy = false)
 * - All values are positive / within expected ranges
 */

#include <sims3000/fluid/FluidReservoirConfig.h>
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
// Named Constant Value Tests
// =============================================================================

TEST(constant_capacity) {
    ASSERT_EQ(RESERVOIR_DEFAULT_CAPACITY, 1000u);
}

TEST(constant_fill_rate) {
    ASSERT_EQ(RESERVOIR_DEFAULT_FILL_RATE, static_cast<uint16_t>(50));
}

TEST(constant_drain_rate) {
    ASSERT_EQ(RESERVOIR_DEFAULT_DRAIN_RATE, static_cast<uint16_t>(100));
}

TEST(constant_build_cost) {
    ASSERT_EQ(RESERVOIR_DEFAULT_BUILD_COST, 2000u);
}

TEST(constant_maintenance_cost) {
    ASSERT_EQ(RESERVOIR_DEFAULT_MAINTENANCE_COST, 20u);
}

TEST(constant_coverage_radius) {
    ASSERT_EQ(RESERVOIR_DEFAULT_COVERAGE_RADIUS, 6);
}

TEST(constant_requires_energy) {
    ASSERT_EQ(RESERVOIR_DEFAULT_REQUIRES_ENERGY, false);
}

// =============================================================================
// get_default_reservoir_config() Tests
// =============================================================================

TEST(default_config_capacity) {
    FluidReservoirConfig cfg = get_default_reservoir_config();
    ASSERT_EQ(cfg.capacity, RESERVOIR_DEFAULT_CAPACITY);
}

TEST(default_config_fill_rate) {
    FluidReservoirConfig cfg = get_default_reservoir_config();
    ASSERT_EQ(cfg.fill_rate, RESERVOIR_DEFAULT_FILL_RATE);
}

TEST(default_config_drain_rate) {
    FluidReservoirConfig cfg = get_default_reservoir_config();
    ASSERT_EQ(cfg.drain_rate, RESERVOIR_DEFAULT_DRAIN_RATE);
}

TEST(default_config_build_cost) {
    FluidReservoirConfig cfg = get_default_reservoir_config();
    ASSERT_EQ(cfg.build_cost, RESERVOIR_DEFAULT_BUILD_COST);
}

TEST(default_config_maintenance_cost) {
    FluidReservoirConfig cfg = get_default_reservoir_config();
    ASSERT_EQ(cfg.maintenance_cost, RESERVOIR_DEFAULT_MAINTENANCE_COST);
}

TEST(default_config_coverage_radius) {
    FluidReservoirConfig cfg = get_default_reservoir_config();
    ASSERT_EQ(cfg.coverage_radius, RESERVOIR_DEFAULT_COVERAGE_RADIUS);
}

TEST(default_config_requires_energy) {
    FluidReservoirConfig cfg = get_default_reservoir_config();
    ASSERT_EQ(cfg.requires_energy, RESERVOIR_DEFAULT_REQUIRES_ENERGY);
}

// =============================================================================
// Spec Value Verification Tests
// =============================================================================

TEST(default_config_matches_spec_values) {
    FluidReservoirConfig cfg = get_default_reservoir_config();
    ASSERT_EQ(cfg.capacity, 1000u);
    ASSERT_EQ(cfg.fill_rate, static_cast<uint16_t>(50));
    ASSERT_EQ(cfg.drain_rate, static_cast<uint16_t>(100));
    ASSERT_EQ(cfg.build_cost, 2000u);
    ASSERT_EQ(cfg.maintenance_cost, 20u);
    ASSERT_EQ(cfg.coverage_radius, 6);
    ASSERT_EQ(cfg.requires_energy, false);
}

// =============================================================================
// Asymmetric Rate Tests (CCR-005)
// =============================================================================

TEST(drain_rate_exceeds_fill_rate) {
    // CCR-005: drain_rate (100) > fill_rate (50)
    // Reservoirs must empty faster than they fill
    FluidReservoirConfig cfg = get_default_reservoir_config();
    ASSERT(cfg.drain_rate > cfg.fill_rate);
}

TEST(asymmetric_ratio_is_two_to_one) {
    // CCR-005: drain is exactly 2x fill rate
    FluidReservoirConfig cfg = get_default_reservoir_config();
    ASSERT_EQ(cfg.drain_rate, static_cast<uint16_t>(cfg.fill_rate * 2));
}

TEST(constant_drain_exceeds_constant_fill) {
    // Verify the named constants themselves encode asymmetry
    ASSERT(RESERVOIR_DEFAULT_DRAIN_RATE > RESERVOIR_DEFAULT_FILL_RATE);
}

// =============================================================================
// Passive Storage Tests
// =============================================================================

TEST(reservoir_is_passive_storage) {
    // Reservoirs do not require energy (passive storage)
    FluidReservoirConfig cfg = get_default_reservoir_config();
    ASSERT_EQ(cfg.requires_energy, false);
}

// =============================================================================
// Constraint / Invariant Tests
// =============================================================================

TEST(capacity_positive) {
    FluidReservoirConfig cfg = get_default_reservoir_config();
    ASSERT(cfg.capacity > 0);
}

TEST(fill_rate_positive) {
    FluidReservoirConfig cfg = get_default_reservoir_config();
    ASSERT(cfg.fill_rate > 0);
}

TEST(drain_rate_positive) {
    FluidReservoirConfig cfg = get_default_reservoir_config();
    ASSERT(cfg.drain_rate > 0);
}

TEST(build_cost_positive) {
    FluidReservoirConfig cfg = get_default_reservoir_config();
    ASSERT(cfg.build_cost > 0);
}

TEST(maintenance_cost_positive) {
    FluidReservoirConfig cfg = get_default_reservoir_config();
    ASSERT(cfg.maintenance_cost > 0);
}

TEST(coverage_radius_positive) {
    FluidReservoirConfig cfg = get_default_reservoir_config();
    ASSERT(cfg.coverage_radius > 0);
}

TEST(maintenance_cost_less_than_build_cost) {
    FluidReservoirConfig cfg = get_default_reservoir_config();
    ASSERT(cfg.maintenance_cost < cfg.build_cost);
}

TEST(fill_rate_does_not_exceed_capacity) {
    FluidReservoirConfig cfg = get_default_reservoir_config();
    ASSERT(cfg.fill_rate <= cfg.capacity);
}

TEST(drain_rate_does_not_exceed_capacity) {
    FluidReservoirConfig cfg = get_default_reservoir_config();
    ASSERT(cfg.drain_rate <= cfg.capacity);
}

// =============================================================================
// Main
// =============================================================================

int main() {
    printf("=== FluidReservoirConfig Unit Tests (Ticket 6-024) ===\n\n");

    // Named constant value tests
    RUN_TEST(constant_capacity);
    RUN_TEST(constant_fill_rate);
    RUN_TEST(constant_drain_rate);
    RUN_TEST(constant_build_cost);
    RUN_TEST(constant_maintenance_cost);
    RUN_TEST(constant_coverage_radius);
    RUN_TEST(constant_requires_energy);

    // get_default_reservoir_config() tests
    RUN_TEST(default_config_capacity);
    RUN_TEST(default_config_fill_rate);
    RUN_TEST(default_config_drain_rate);
    RUN_TEST(default_config_build_cost);
    RUN_TEST(default_config_maintenance_cost);
    RUN_TEST(default_config_coverage_radius);
    RUN_TEST(default_config_requires_energy);

    // Spec value verification
    RUN_TEST(default_config_matches_spec_values);

    // Asymmetric rate tests (CCR-005)
    RUN_TEST(drain_rate_exceeds_fill_rate);
    RUN_TEST(asymmetric_ratio_is_two_to_one);
    RUN_TEST(constant_drain_exceeds_constant_fill);

    // Passive storage tests
    RUN_TEST(reservoir_is_passive_storage);

    // Constraint / invariant tests
    RUN_TEST(capacity_positive);
    RUN_TEST(fill_rate_positive);
    RUN_TEST(drain_rate_positive);
    RUN_TEST(build_cost_positive);
    RUN_TEST(maintenance_cost_positive);
    RUN_TEST(coverage_radius_positive);
    RUN_TEST(maintenance_cost_less_than_build_cost);
    RUN_TEST(fill_rate_does_not_exceed_capacity);
    RUN_TEST(drain_rate_does_not_exceed_capacity);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
