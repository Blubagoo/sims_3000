/**
 * @file test_energy_requirements.cpp
 * @brief Unit tests for EnergyRequirements.h and EnergyPriorities.h (Tickets 5-037, 5-038)
 *
 * Tests cover:
 * - Energy requirement constants (habitation, exchange, fabrication, service, infrastructure)
 * - get_energy_requirement() for all zone type + density combinations
 * - get_energy_requirement() for invalid inputs (returns 0)
 * - Energy priority constants
 * - get_energy_priority_for_zone() for all zone types
 * - get_energy_priority_for_zone() default for unknown types
 * - Priority ordering (CRITICAL < IMPORTANT < NORMAL < LOW)
 */

#include <sims3000/energy/EnergyRequirements.h>
#include <sims3000/energy/EnergyPriorities.h>
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
// Energy Requirement Constant Tests (Ticket 5-037)
// =============================================================================

TEST(habitation_energy_constants) {
    ASSERT_EQ(ENERGY_REQ_HABITATION_LOW, 5u);
    ASSERT_EQ(ENERGY_REQ_HABITATION_HIGH, 20u);
}

TEST(exchange_energy_constants) {
    ASSERT_EQ(ENERGY_REQ_EXCHANGE_LOW, 10u);
    ASSERT_EQ(ENERGY_REQ_EXCHANGE_HIGH, 40u);
}

TEST(fabrication_energy_constants) {
    ASSERT_EQ(ENERGY_REQ_FABRICATION_LOW, 15u);
    ASSERT_EQ(ENERGY_REQ_FABRICATION_HIGH, 60u);
}

TEST(service_energy_constants) {
    ASSERT_EQ(ENERGY_REQ_SERVICE_SMALL, 20u);
    ASSERT_EQ(ENERGY_REQ_SERVICE_MEDIUM, 35u);
    ASSERT_EQ(ENERGY_REQ_SERVICE_LARGE, 50u);
}

TEST(infrastructure_energy_constant) {
    ASSERT_EQ(ENERGY_REQ_INFRASTRUCTURE, 0u);
}

TEST(energy_scaling_per_zone_type) {
    // Low density: Habitation < Exchange < Fabrication
    ASSERT(ENERGY_REQ_HABITATION_LOW < ENERGY_REQ_EXCHANGE_LOW);
    ASSERT(ENERGY_REQ_EXCHANGE_LOW < ENERGY_REQ_FABRICATION_LOW);

    // High density: Habitation < Exchange < Fabrication
    ASSERT(ENERGY_REQ_HABITATION_HIGH < ENERGY_REQ_EXCHANGE_HIGH);
    ASSERT(ENERGY_REQ_EXCHANGE_HIGH < ENERGY_REQ_FABRICATION_HIGH);
}

TEST(energy_scaling_per_density) {
    // High density > Low density for every zone type
    ASSERT(ENERGY_REQ_HABITATION_HIGH > ENERGY_REQ_HABITATION_LOW);
    ASSERT(ENERGY_REQ_EXCHANGE_HIGH > ENERGY_REQ_EXCHANGE_LOW);
    ASSERT(ENERGY_REQ_FABRICATION_HIGH > ENERGY_REQ_FABRICATION_LOW);
}

TEST(service_energy_ordering) {
    // Small < Medium < Large
    ASSERT(ENERGY_REQ_SERVICE_SMALL < ENERGY_REQ_SERVICE_MEDIUM);
    ASSERT(ENERGY_REQ_SERVICE_MEDIUM < ENERGY_REQ_SERVICE_LARGE);

    // Service range is 20-50
    ASSERT(ENERGY_REQ_SERVICE_SMALL >= 20u);
    ASSERT(ENERGY_REQ_SERVICE_LARGE <= 50u);
}

// =============================================================================
// get_energy_requirement() Tests (Ticket 5-037)
// =============================================================================

TEST(get_energy_req_habitation_low) {
    ASSERT_EQ(get_energy_requirement(0, 0), ENERGY_REQ_HABITATION_LOW);
}

TEST(get_energy_req_habitation_high) {
    ASSERT_EQ(get_energy_requirement(0, 1), ENERGY_REQ_HABITATION_HIGH);
}

TEST(get_energy_req_exchange_low) {
    ASSERT_EQ(get_energy_requirement(1, 0), ENERGY_REQ_EXCHANGE_LOW);
}

TEST(get_energy_req_exchange_high) {
    ASSERT_EQ(get_energy_requirement(1, 1), ENERGY_REQ_EXCHANGE_HIGH);
}

TEST(get_energy_req_fabrication_low) {
    ASSERT_EQ(get_energy_requirement(2, 0), ENERGY_REQ_FABRICATION_LOW);
}

TEST(get_energy_req_fabrication_high) {
    ASSERT_EQ(get_energy_requirement(2, 1), ENERGY_REQ_FABRICATION_HIGH);
}

TEST(get_energy_req_invalid_zone_type) {
    // Unknown zone types return 0
    ASSERT_EQ(get_energy_requirement(3, 0), 0u);
    ASSERT_EQ(get_energy_requirement(255, 0), 0u);
    ASSERT_EQ(get_energy_requirement(3, 1), 0u);
    ASSERT_EQ(get_energy_requirement(100, 0), 0u);
}

TEST(get_energy_req_invalid_density_treated_as_high) {
    // density != 0 is treated as high density (the else branch)
    ASSERT_EQ(get_energy_requirement(0, 2), ENERGY_REQ_HABITATION_HIGH);
    ASSERT_EQ(get_energy_requirement(1, 255), ENERGY_REQ_EXCHANGE_HIGH);
    ASSERT_EQ(get_energy_requirement(2, 5), ENERGY_REQ_FABRICATION_HIGH);
}

// =============================================================================
// Energy Priority Constant Tests (Ticket 5-038)
// =============================================================================

TEST(priority_constant_values) {
    ASSERT_EQ(ENERGY_PRIORITY_CRITICAL, 1u);
    ASSERT_EQ(ENERGY_PRIORITY_IMPORTANT, 2u);
    ASSERT_EQ(ENERGY_PRIORITY_NORMAL, 3u);
    ASSERT_EQ(ENERGY_PRIORITY_LOW, 4u);
}

TEST(priority_default_is_normal) {
    ASSERT_EQ(ENERGY_PRIORITY_DEFAULT, ENERGY_PRIORITY_NORMAL);
}

TEST(priority_ordering) {
    // Lower number = higher priority = served first during rationing
    ASSERT(ENERGY_PRIORITY_CRITICAL < ENERGY_PRIORITY_IMPORTANT);
    ASSERT(ENERGY_PRIORITY_IMPORTANT < ENERGY_PRIORITY_NORMAL);
    ASSERT(ENERGY_PRIORITY_NORMAL < ENERGY_PRIORITY_LOW);
}

// =============================================================================
// get_energy_priority_for_zone() Tests (Ticket 5-038)
// =============================================================================

TEST(priority_habitation) {
    // Habitation is lowest priority (shuts off last during rationing)
    ASSERT_EQ(get_energy_priority_for_zone(0), ENERGY_PRIORITY_LOW);
}

TEST(priority_exchange) {
    // Exchange is normal priority
    ASSERT_EQ(get_energy_priority_for_zone(1), ENERGY_PRIORITY_NORMAL);
}

TEST(priority_fabrication) {
    // Fabrication is normal priority
    ASSERT_EQ(get_energy_priority_for_zone(2), ENERGY_PRIORITY_NORMAL);
}

TEST(priority_unknown_zone_returns_default) {
    // Unknown zone types get default priority (NORMAL)
    ASSERT_EQ(get_energy_priority_for_zone(3), ENERGY_PRIORITY_DEFAULT);
    ASSERT_EQ(get_energy_priority_for_zone(255), ENERGY_PRIORITY_DEFAULT);
    ASSERT_EQ(get_energy_priority_for_zone(100), ENERGY_PRIORITY_DEFAULT);
}

TEST(priority_habitation_is_lowest) {
    // Habitation should have the lowest priority (highest number) of all zone types
    uint8_t hab_priority = get_energy_priority_for_zone(0);
    uint8_t exc_priority = get_energy_priority_for_zone(1);
    uint8_t fab_priority = get_energy_priority_for_zone(2);

    ASSERT(hab_priority >= exc_priority);
    ASSERT(hab_priority >= fab_priority);
}

// =============================================================================
// Main Entry Point
// =============================================================================

int main() {
    printf("=== Energy Requirements & Priorities Unit Tests ===\n\n");

    // Energy requirement constant tests (5-037)
    RUN_TEST(habitation_energy_constants);
    RUN_TEST(exchange_energy_constants);
    RUN_TEST(fabrication_energy_constants);
    RUN_TEST(service_energy_constants);
    RUN_TEST(infrastructure_energy_constant);
    RUN_TEST(energy_scaling_per_zone_type);
    RUN_TEST(energy_scaling_per_density);
    RUN_TEST(service_energy_ordering);

    // get_energy_requirement() tests (5-037)
    RUN_TEST(get_energy_req_habitation_low);
    RUN_TEST(get_energy_req_habitation_high);
    RUN_TEST(get_energy_req_exchange_low);
    RUN_TEST(get_energy_req_exchange_high);
    RUN_TEST(get_energy_req_fabrication_low);
    RUN_TEST(get_energy_req_fabrication_high);
    RUN_TEST(get_energy_req_invalid_zone_type);
    RUN_TEST(get_energy_req_invalid_density_treated_as_high);

    // Energy priority constant tests (5-038)
    RUN_TEST(priority_constant_values);
    RUN_TEST(priority_default_is_normal);
    RUN_TEST(priority_ordering);

    // get_energy_priority_for_zone() tests (5-038)
    RUN_TEST(priority_habitation);
    RUN_TEST(priority_exchange);
    RUN_TEST(priority_fabrication);
    RUN_TEST(priority_unknown_zone_returns_default);
    RUN_TEST(priority_habitation_is_lowest);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
