/**
 * @file test_fluid_requirements.cpp
 * @brief Unit tests for FluidRequirements (Epic 6, Ticket 6-039)
 *
 * Tests cover:
 * - Fluid requirement constants (habitation, exchange, fabrication, service, infrastructure)
 * - get_zone_fluid_requirement() for all zone type + density combinations
 * - get_zone_fluid_requirement() for invalid inputs (returns 0)
 * - get_service_fluid_requirement() for all service types
 * - get_service_fluid_requirement() for unknown types (safe default)
 * - Values match energy requirements exactly (CCR-007)
 * - Scaling relationships between zone types and densities
 */

#include <sims3000/fluid/FluidRequirements.h>
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
// Fluid Requirement Constant Tests (Ticket 6-039)
// =============================================================================

TEST(habitation_fluid_constants) {
    ASSERT_EQ(FLUID_REQ_HABITATION_LOW, 5u);
    ASSERT_EQ(FLUID_REQ_HABITATION_HIGH, 20u);
}

TEST(exchange_fluid_constants) {
    ASSERT_EQ(FLUID_REQ_EXCHANGE_LOW, 10u);
    ASSERT_EQ(FLUID_REQ_EXCHANGE_HIGH, 40u);
}

TEST(fabrication_fluid_constants) {
    ASSERT_EQ(FLUID_REQ_FABRICATION_LOW, 15u);
    ASSERT_EQ(FLUID_REQ_FABRICATION_HIGH, 60u);
}

TEST(service_fluid_constants) {
    ASSERT_EQ(FLUID_REQ_SERVICE_SMALL, 20u);
    ASSERT_EQ(FLUID_REQ_SERVICE_MEDIUM, 35u);
    ASSERT_EQ(FLUID_REQ_SERVICE_LARGE, 50u);
}

TEST(infrastructure_fluid_constant) {
    ASSERT_EQ(FLUID_REQ_INFRASTRUCTURE, 0u);
}

// =============================================================================
// CCR-007 Parity: Values Match Energy Requirements Exactly
// =============================================================================

TEST(values_match_energy_habitation) {
    // Energy: ENERGY_REQ_HABITATION_LOW=5, ENERGY_REQ_HABITATION_HIGH=20
    ASSERT_EQ(FLUID_REQ_HABITATION_LOW, 5u);
    ASSERT_EQ(FLUID_REQ_HABITATION_HIGH, 20u);
}

TEST(values_match_energy_exchange) {
    // Energy: ENERGY_REQ_EXCHANGE_LOW=10, ENERGY_REQ_EXCHANGE_HIGH=40
    ASSERT_EQ(FLUID_REQ_EXCHANGE_LOW, 10u);
    ASSERT_EQ(FLUID_REQ_EXCHANGE_HIGH, 40u);
}

TEST(values_match_energy_fabrication) {
    // Energy: ENERGY_REQ_FABRICATION_LOW=15, ENERGY_REQ_FABRICATION_HIGH=60
    ASSERT_EQ(FLUID_REQ_FABRICATION_LOW, 15u);
    ASSERT_EQ(FLUID_REQ_FABRICATION_HIGH, 60u);
}

TEST(values_match_energy_service) {
    // Energy: ENERGY_REQ_SERVICE_SMALL=20, MEDIUM=35, LARGE=50
    ASSERT_EQ(FLUID_REQ_SERVICE_SMALL, 20u);
    ASSERT_EQ(FLUID_REQ_SERVICE_MEDIUM, 35u);
    ASSERT_EQ(FLUID_REQ_SERVICE_LARGE, 50u);
}

TEST(values_match_energy_infrastructure) {
    // Energy: ENERGY_REQ_INFRASTRUCTURE=0
    ASSERT_EQ(FLUID_REQ_INFRASTRUCTURE, 0u);
}

// =============================================================================
// Scaling Relationship Tests
// =============================================================================

TEST(fluid_scaling_per_zone_type) {
    // Low density: Habitation < Exchange < Fabrication
    ASSERT(FLUID_REQ_HABITATION_LOW < FLUID_REQ_EXCHANGE_LOW);
    ASSERT(FLUID_REQ_EXCHANGE_LOW < FLUID_REQ_FABRICATION_LOW);

    // High density: Habitation < Exchange < Fabrication
    ASSERT(FLUID_REQ_HABITATION_HIGH < FLUID_REQ_EXCHANGE_HIGH);
    ASSERT(FLUID_REQ_EXCHANGE_HIGH < FLUID_REQ_FABRICATION_HIGH);
}

TEST(fluid_scaling_per_density) {
    // High density > Low density for every zone type
    ASSERT(FLUID_REQ_HABITATION_HIGH > FLUID_REQ_HABITATION_LOW);
    ASSERT(FLUID_REQ_EXCHANGE_HIGH > FLUID_REQ_EXCHANGE_LOW);
    ASSERT(FLUID_REQ_FABRICATION_HIGH > FLUID_REQ_FABRICATION_LOW);
}

TEST(service_fluid_ordering) {
    // Small < Medium < Large
    ASSERT(FLUID_REQ_SERVICE_SMALL < FLUID_REQ_SERVICE_MEDIUM);
    ASSERT(FLUID_REQ_SERVICE_MEDIUM < FLUID_REQ_SERVICE_LARGE);

    // Service range is 20-50
    ASSERT(FLUID_REQ_SERVICE_SMALL >= 20u);
    ASSERT(FLUID_REQ_SERVICE_LARGE <= 50u);
}

// =============================================================================
// get_zone_fluid_requirement() Tests (Ticket 6-039)
// =============================================================================

TEST(get_zone_req_habitation_low) {
    ASSERT_EQ(get_zone_fluid_requirement(0, 0), FLUID_REQ_HABITATION_LOW);
}

TEST(get_zone_req_habitation_high) {
    ASSERT_EQ(get_zone_fluid_requirement(0, 1), FLUID_REQ_HABITATION_HIGH);
}

TEST(get_zone_req_exchange_low) {
    ASSERT_EQ(get_zone_fluid_requirement(1, 0), FLUID_REQ_EXCHANGE_LOW);
}

TEST(get_zone_req_exchange_high) {
    ASSERT_EQ(get_zone_fluid_requirement(1, 1), FLUID_REQ_EXCHANGE_HIGH);
}

TEST(get_zone_req_fabrication_low) {
    ASSERT_EQ(get_zone_fluid_requirement(2, 0), FLUID_REQ_FABRICATION_LOW);
}

TEST(get_zone_req_fabrication_high) {
    ASSERT_EQ(get_zone_fluid_requirement(2, 1), FLUID_REQ_FABRICATION_HIGH);
}

TEST(get_zone_req_invalid_zone_type) {
    // Unknown zone types return 0
    ASSERT_EQ(get_zone_fluid_requirement(3, 0), 0u);
    ASSERT_EQ(get_zone_fluid_requirement(255, 0), 0u);
    ASSERT_EQ(get_zone_fluid_requirement(3, 1), 0u);
    ASSERT_EQ(get_zone_fluid_requirement(100, 0), 0u);
}

TEST(get_zone_req_invalid_density_treated_as_high) {
    // density != 0 is treated as high density (the else branch)
    ASSERT_EQ(get_zone_fluid_requirement(0, 2), FLUID_REQ_HABITATION_HIGH);
    ASSERT_EQ(get_zone_fluid_requirement(1, 255), FLUID_REQ_EXCHANGE_HIGH);
    ASSERT_EQ(get_zone_fluid_requirement(2, 5), FLUID_REQ_FABRICATION_HIGH);
}

// =============================================================================
// get_service_fluid_requirement() Tests (Ticket 6-039)
// =============================================================================

TEST(get_service_req_small) {
    ASSERT_EQ(get_service_fluid_requirement(0), FLUID_REQ_SERVICE_SMALL);
}

TEST(get_service_req_medium) {
    ASSERT_EQ(get_service_fluid_requirement(1), FLUID_REQ_SERVICE_MEDIUM);
}

TEST(get_service_req_large) {
    ASSERT_EQ(get_service_fluid_requirement(2), FLUID_REQ_SERVICE_LARGE);
}

TEST(get_service_req_unknown_returns_medium) {
    // Unknown service types default to medium (safe default)
    ASSERT_EQ(get_service_fluid_requirement(3), FLUID_REQ_SERVICE_MEDIUM);
    ASSERT_EQ(get_service_fluid_requirement(255), FLUID_REQ_SERVICE_MEDIUM);
    ASSERT_EQ(get_service_fluid_requirement(100), FLUID_REQ_SERVICE_MEDIUM);
}

TEST(service_range_20_to_50) {
    // All service requirements fall in [20, 50] range
    ASSERT(get_service_fluid_requirement(0) >= 20u);
    ASSERT(get_service_fluid_requirement(0) <= 50u);
    ASSERT(get_service_fluid_requirement(1) >= 20u);
    ASSERT(get_service_fluid_requirement(1) <= 50u);
    ASSERT(get_service_fluid_requirement(2) >= 20u);
    ASSERT(get_service_fluid_requirement(2) <= 50u);
}

// =============================================================================
// Main Entry Point
// =============================================================================

int main() {
    printf("=== Fluid Requirements Unit Tests (Epic 6, Ticket 6-039) ===\n\n");

    // Fluid requirement constant tests
    RUN_TEST(habitation_fluid_constants);
    RUN_TEST(exchange_fluid_constants);
    RUN_TEST(fabrication_fluid_constants);
    RUN_TEST(service_fluid_constants);
    RUN_TEST(infrastructure_fluid_constant);

    // CCR-007 parity tests
    RUN_TEST(values_match_energy_habitation);
    RUN_TEST(values_match_energy_exchange);
    RUN_TEST(values_match_energy_fabrication);
    RUN_TEST(values_match_energy_service);
    RUN_TEST(values_match_energy_infrastructure);

    // Scaling relationship tests
    RUN_TEST(fluid_scaling_per_zone_type);
    RUN_TEST(fluid_scaling_per_density);
    RUN_TEST(service_fluid_ordering);

    // get_zone_fluid_requirement() tests
    RUN_TEST(get_zone_req_habitation_low);
    RUN_TEST(get_zone_req_habitation_high);
    RUN_TEST(get_zone_req_exchange_low);
    RUN_TEST(get_zone_req_exchange_high);
    RUN_TEST(get_zone_req_fabrication_low);
    RUN_TEST(get_zone_req_fabrication_high);
    RUN_TEST(get_zone_req_invalid_zone_type);
    RUN_TEST(get_zone_req_invalid_density_treated_as_high);

    // get_service_fluid_requirement() tests
    RUN_TEST(get_service_req_small);
    RUN_TEST(get_service_req_medium);
    RUN_TEST(get_service_req_large);
    RUN_TEST(get_service_req_unknown_returns_medium);
    RUN_TEST(service_range_20_to_50);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
