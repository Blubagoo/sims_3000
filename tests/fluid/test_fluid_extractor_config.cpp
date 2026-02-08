/**
 * @file test_fluid_extractor_config.cpp
 * @brief Unit tests for FluidExtractorConfig (Ticket 6-023)
 *
 * Tests cover:
 * - Default config values match named constants
 * - get_default_extractor_config() returns correct values
 * - Named constants have expected values per spec
 * - Energy priority is ENERGY_PRIORITY_IMPORTANT (2) per CCR-008
 * - All values are positive / within expected ranges
 */

#include <sims3000/fluid/FluidExtractorConfig.h>
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

TEST(constant_base_output) {
    ASSERT_EQ(EXTRACTOR_DEFAULT_BASE_OUTPUT, 100u);
}

TEST(constant_build_cost) {
    ASSERT_EQ(EXTRACTOR_DEFAULT_BUILD_COST, 3000u);
}

TEST(constant_maintenance_cost) {
    ASSERT_EQ(EXTRACTOR_DEFAULT_MAINTENANCE_COST, 30u);
}

TEST(constant_energy_required) {
    ASSERT_EQ(EXTRACTOR_DEFAULT_ENERGY_REQUIRED, 20u);
}

TEST(constant_energy_priority) {
    // CCR-008: ENERGY_PRIORITY_IMPORTANT = 2
    ASSERT_EQ(EXTRACTOR_DEFAULT_ENERGY_PRIORITY, 2);
}

TEST(constant_max_placement_distance) {
    ASSERT_EQ(EXTRACTOR_DEFAULT_MAX_PLACEMENT_DISTANCE, 8);
}

TEST(constant_max_operational_distance) {
    ASSERT_EQ(EXTRACTOR_DEFAULT_MAX_OPERATIONAL_DISTANCE, 5);
}

TEST(constant_coverage_radius) {
    ASSERT_EQ(EXTRACTOR_DEFAULT_COVERAGE_RADIUS, 8);
}

// =============================================================================
// get_default_extractor_config() Tests
// =============================================================================

TEST(default_config_base_output) {
    FluidExtractorConfig cfg = get_default_extractor_config();
    ASSERT_EQ(cfg.base_output, EXTRACTOR_DEFAULT_BASE_OUTPUT);
}

TEST(default_config_build_cost) {
    FluidExtractorConfig cfg = get_default_extractor_config();
    ASSERT_EQ(cfg.build_cost, EXTRACTOR_DEFAULT_BUILD_COST);
}

TEST(default_config_maintenance_cost) {
    FluidExtractorConfig cfg = get_default_extractor_config();
    ASSERT_EQ(cfg.maintenance_cost, EXTRACTOR_DEFAULT_MAINTENANCE_COST);
}

TEST(default_config_energy_required) {
    FluidExtractorConfig cfg = get_default_extractor_config();
    ASSERT_EQ(cfg.energy_required, EXTRACTOR_DEFAULT_ENERGY_REQUIRED);
}

TEST(default_config_energy_priority) {
    FluidExtractorConfig cfg = get_default_extractor_config();
    ASSERT_EQ(cfg.energy_priority, EXTRACTOR_DEFAULT_ENERGY_PRIORITY);
}

TEST(default_config_max_placement_distance) {
    FluidExtractorConfig cfg = get_default_extractor_config();
    ASSERT_EQ(cfg.max_placement_distance, EXTRACTOR_DEFAULT_MAX_PLACEMENT_DISTANCE);
}

TEST(default_config_max_operational_distance) {
    FluidExtractorConfig cfg = get_default_extractor_config();
    ASSERT_EQ(cfg.max_operational_distance, EXTRACTOR_DEFAULT_MAX_OPERATIONAL_DISTANCE);
}

TEST(default_config_coverage_radius) {
    FluidExtractorConfig cfg = get_default_extractor_config();
    ASSERT_EQ(cfg.coverage_radius, EXTRACTOR_DEFAULT_COVERAGE_RADIUS);
}

// =============================================================================
// Spec Value Verification Tests
// =============================================================================

TEST(default_config_matches_spec_values) {
    FluidExtractorConfig cfg = get_default_extractor_config();
    ASSERT_EQ(cfg.base_output, 100u);
    ASSERT_EQ(cfg.build_cost, 3000u);
    ASSERT_EQ(cfg.maintenance_cost, 30u);
    ASSERT_EQ(cfg.energy_required, 20u);
    ASSERT_EQ(cfg.energy_priority, 2);
    ASSERT_EQ(cfg.max_placement_distance, 8);
    ASSERT_EQ(cfg.max_operational_distance, 5);
    ASSERT_EQ(cfg.coverage_radius, 8);
}

// =============================================================================
// Constraint / Invariant Tests
// =============================================================================

TEST(all_values_positive) {
    FluidExtractorConfig cfg = get_default_extractor_config();
    ASSERT(cfg.base_output > 0);
    ASSERT(cfg.build_cost > 0);
    ASSERT(cfg.maintenance_cost > 0);
    ASSERT(cfg.energy_required > 0);
    ASSERT(cfg.energy_priority > 0);
    ASSERT(cfg.max_placement_distance > 0);
    ASSERT(cfg.max_operational_distance > 0);
    ASSERT(cfg.coverage_radius > 0);
}

TEST(operational_distance_less_than_placement_distance) {
    FluidExtractorConfig cfg = get_default_extractor_config();
    ASSERT(cfg.max_operational_distance <= cfg.max_placement_distance);
}

TEST(energy_priority_is_important_level) {
    // CCR-008: extractors should be ENERGY_PRIORITY_IMPORTANT (2)
    // Priority levels: 1=critical, 2=important, 3=normal
    FluidExtractorConfig cfg = get_default_extractor_config();
    ASSERT_EQ(cfg.energy_priority, 2);
}

TEST(maintenance_cost_less_than_build_cost) {
    FluidExtractorConfig cfg = get_default_extractor_config();
    ASSERT(cfg.maintenance_cost < cfg.build_cost);
}

// =============================================================================
// Main
// =============================================================================

int main() {
    printf("=== FluidExtractorConfig Unit Tests (Ticket 6-023) ===\n\n");

    // Named constant value tests
    RUN_TEST(constant_base_output);
    RUN_TEST(constant_build_cost);
    RUN_TEST(constant_maintenance_cost);
    RUN_TEST(constant_energy_required);
    RUN_TEST(constant_energy_priority);
    RUN_TEST(constant_max_placement_distance);
    RUN_TEST(constant_max_operational_distance);
    RUN_TEST(constant_coverage_radius);

    // get_default_extractor_config() tests
    RUN_TEST(default_config_base_output);
    RUN_TEST(default_config_build_cost);
    RUN_TEST(default_config_maintenance_cost);
    RUN_TEST(default_config_energy_required);
    RUN_TEST(default_config_energy_priority);
    RUN_TEST(default_config_max_placement_distance);
    RUN_TEST(default_config_max_operational_distance);
    RUN_TEST(default_config_coverage_radius);

    // Spec value verification
    RUN_TEST(default_config_matches_spec_values);

    // Constraint / invariant tests
    RUN_TEST(all_values_positive);
    RUN_TEST(operational_distance_less_than_placement_distance);
    RUN_TEST(energy_priority_is_important_level);
    RUN_TEST(maintenance_cost_less_than_build_cost);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
