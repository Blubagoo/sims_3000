/**
 * @file test_demand_factors_ui.cpp
 * @brief Unit tests for demand factors UI helpers (Ticket E10-047).
 *
 * Tests cover:
 * - get_demand_factors returns correct reference for each zone type
 * - get_dominant_factor_name returns the factor with largest absolute value
 * - get_demand_description returns appropriate strings
 * - sum_factors correctly sums all 6 factors
 * - is_bottlenecked_by identifies negative dominant factors by name
 */

#include <sims3000/demand/DemandFactorsUI.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

using namespace sims3000::demand;

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
        printf("\n  FAILED: %s == %s (%d != %d) (line %d)\n", #a, #b, (int)(a), (int)(b), __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

#define ASSERT_STREQ(a, b) do { \
    if (std::strcmp((a), (b)) != 0) { \
        printf("\n  FAILED: %s == %s (\"%s\" != \"%s\") (line %d)\n", #a, #b, (a), (b), __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

// =============================================================================
// get_demand_factors tests
// =============================================================================

TEST(get_demand_factors_habitation) {
    DemandData data;
    data.habitation_factors.population_factor = 10;
    data.exchange_factors.population_factor = 20;
    data.fabrication_factors.population_factor = 30;

    const DemandFactors& factors = get_demand_factors(data, ZONE_HABITATION);
    ASSERT_EQ(factors.population_factor, 10);
}

TEST(get_demand_factors_exchange) {
    DemandData data;
    data.habitation_factors.employment_factor = 15;
    data.exchange_factors.employment_factor = 25;
    data.fabrication_factors.employment_factor = 35;

    const DemandFactors& factors = get_demand_factors(data, ZONE_EXCHANGE);
    ASSERT_EQ(factors.employment_factor, 25);
}

TEST(get_demand_factors_fabrication) {
    DemandData data;
    data.habitation_factors.services_factor = 5;
    data.exchange_factors.services_factor = 15;
    data.fabrication_factors.services_factor = 25;

    const DemandFactors& factors = get_demand_factors(data, ZONE_FABRICATION);
    ASSERT_EQ(factors.services_factor, 25);
}

TEST(get_demand_factors_invalid_zone_returns_habitation) {
    DemandData data;
    data.habitation_factors.tribute_factor = 42;
    data.exchange_factors.tribute_factor = 99;

    // Invalid zone type (e.g., 99) should return habitation_factors
    const DemandFactors& factors = get_demand_factors(data, 99);
    ASSERT_EQ(factors.tribute_factor, 42);
}

// =============================================================================
// get_dominant_factor_name tests
// =============================================================================

TEST(dominant_factor_population_positive) {
    DemandFactors factors;
    factors.population_factor = 50;
    factors.employment_factor = 10;
    factors.services_factor = 5;

    const char* dominant = get_dominant_factor_name(factors);
    ASSERT_STREQ(dominant, "population");
}

TEST(dominant_factor_employment_negative) {
    DemandFactors factors;
    factors.population_factor = 10;
    factors.employment_factor = -60;
    factors.services_factor = 20;

    const char* dominant = get_dominant_factor_name(factors);
    ASSERT_STREQ(dominant, "employment");
}

TEST(dominant_factor_services) {
    DemandFactors factors;
    factors.population_factor = 5;
    factors.employment_factor = -10;
    factors.services_factor = -70;
    factors.transport_factor = 15;

    const char* dominant = get_dominant_factor_name(factors);
    ASSERT_STREQ(dominant, "services");
}

TEST(dominant_factor_tribute) {
    DemandFactors factors;
    factors.tribute_factor = -80;
    factors.transport_factor = 20;

    const char* dominant = get_dominant_factor_name(factors);
    ASSERT_STREQ(dominant, "tribute");
}

TEST(dominant_factor_transport) {
    DemandFactors factors;
    factors.population_factor = 5;
    factors.transport_factor = 90;

    const char* dominant = get_dominant_factor_name(factors);
    ASSERT_STREQ(dominant, "transport");
}

TEST(dominant_factor_contamination) {
    DemandFactors factors;
    factors.contamination_factor = -100;
    factors.services_factor = 10;

    const char* dominant = get_dominant_factor_name(factors);
    ASSERT_STREQ(dominant, "contamination");
}

TEST(dominant_factor_all_zero) {
    DemandFactors factors; // All zeros by default

    const char* dominant = get_dominant_factor_name(factors);
    ASSERT_STREQ(dominant, "none");
}

TEST(dominant_factor_tie_first_wins) {
    DemandFactors factors;
    factors.population_factor = 50;
    factors.employment_factor = 50;

    // When tied, the first one checked (population) should win
    const char* dominant = get_dominant_factor_name(factors);
    ASSERT_STREQ(dominant, "population");
}

// =============================================================================
// get_demand_description tests
// =============================================================================

TEST(demand_description_strong_growth) {
    ASSERT_STREQ(get_demand_description(100), "Strong Growth");
    ASSERT_STREQ(get_demand_description(75), "Strong Growth");
}

TEST(demand_description_growth) {
    ASSERT_STREQ(get_demand_description(74), "Growth");
    ASSERT_STREQ(get_demand_description(50), "Growth");
    ASSERT_STREQ(get_demand_description(25), "Growth");
}

TEST(demand_description_weak_growth) {
    ASSERT_STREQ(get_demand_description(24), "Weak Growth");
    ASSERT_STREQ(get_demand_description(10), "Weak Growth");
}

TEST(demand_description_stagnant) {
    ASSERT_STREQ(get_demand_description(9), "Stagnant");
    ASSERT_STREQ(get_demand_description(0), "Stagnant");
    ASSERT_STREQ(get_demand_description(-9), "Stagnant");
}

TEST(demand_description_weak_decline) {
    ASSERT_STREQ(get_demand_description(-10), "Weak Decline");
    ASSERT_STREQ(get_demand_description(-20), "Weak Decline");
    ASSERT_STREQ(get_demand_description(-24), "Weak Decline");
}

TEST(demand_description_decline) {
    ASSERT_STREQ(get_demand_description(-25), "Decline");
    ASSERT_STREQ(get_demand_description(-50), "Decline");
    ASSERT_STREQ(get_demand_description(-74), "Decline");
}

TEST(demand_description_strong_decline) {
    ASSERT_STREQ(get_demand_description(-75), "Strong Decline");
    ASSERT_STREQ(get_demand_description(-100), "Strong Decline");
}

// =============================================================================
// sum_factors tests
// =============================================================================

TEST(sum_factors_all_positive) {
    DemandFactors factors;
    factors.population_factor = 10;
    factors.employment_factor = 20;
    factors.services_factor = 15;
    factors.tribute_factor = 5;
    factors.transport_factor = 8;
    factors.contamination_factor = 2;

    int16_t sum = sum_factors(factors);
    ASSERT_EQ(sum, 60);
}

TEST(sum_factors_mixed) {
    DemandFactors factors;
    factors.population_factor = 30;
    factors.employment_factor = -20;
    factors.services_factor = 10;
    factors.tribute_factor = -5;
    factors.transport_factor = 15;
    factors.contamination_factor = -10;

    int16_t sum = sum_factors(factors);
    ASSERT_EQ(sum, 20); // 30 - 20 + 10 - 5 + 15 - 10 = 20
}

TEST(sum_factors_all_negative) {
    DemandFactors factors;
    factors.population_factor = -10;
    factors.employment_factor = -15;
    factors.services_factor = -20;
    factors.tribute_factor = -5;
    factors.transport_factor = -8;
    factors.contamination_factor = -12;

    int16_t sum = sum_factors(factors);
    ASSERT_EQ(sum, -70);
}

TEST(sum_factors_all_zero) {
    DemandFactors factors; // All zeros by default

    int16_t sum = sum_factors(factors);
    ASSERT_EQ(sum, 0);
}

TEST(sum_factors_exceeds_int8_range) {
    DemandFactors factors;
    factors.population_factor = 100;
    factors.employment_factor = 100;
    factors.services_factor = 50;

    int16_t sum = sum_factors(factors);
    ASSERT_EQ(sum, 250); // Exceeds int8_t max (127), but int16_t handles it
}

// =============================================================================
// is_bottlenecked_by tests
// =============================================================================

TEST(bottleneck_by_population) {
    DemandFactors factors;
    factors.population_factor = -50; // Dominant and negative
    factors.employment_factor = 10;
    factors.services_factor = 5;

    ASSERT(is_bottlenecked_by(factors, "population"));
    ASSERT(!is_bottlenecked_by(factors, "employment"));
}

TEST(bottleneck_by_services) {
    DemandFactors factors;
    factors.population_factor = 10;
    factors.services_factor = -60; // Dominant and negative
    factors.transport_factor = 5;

    ASSERT(is_bottlenecked_by(factors, "services"));
    ASSERT(!is_bottlenecked_by(factors, "population"));
}

TEST(no_bottleneck_positive_dominant) {
    DemandFactors factors;
    factors.population_factor = 80; // Dominant but positive
    factors.employment_factor = -10;

    // Not a bottleneck because dominant factor is positive
    ASSERT(!is_bottlenecked_by(factors, "population"));
}

TEST(no_bottleneck_not_dominant) {
    DemandFactors factors;
    factors.employment_factor = -70; // Dominant
    factors.services_factor = -20;   // Negative but not dominant

    // Services is negative but not dominant
    ASSERT(!is_bottlenecked_by(factors, "services"));
    ASSERT(is_bottlenecked_by(factors, "employment"));
}

TEST(bottleneck_invalid_factor_name) {
    DemandFactors factors;
    factors.population_factor = -50;

    ASSERT(!is_bottlenecked_by(factors, "invalid_name"));
    ASSERT(!is_bottlenecked_by(factors, nullptr));
}

TEST(bottleneck_all_factors) {
    DemandFactors factors;

    // Test each factor as dominant bottleneck
    factors.population_factor = -80;
    ASSERT(is_bottlenecked_by(factors, "population"));

    factors = DemandFactors(); // Reset
    factors.employment_factor = -80;
    ASSERT(is_bottlenecked_by(factors, "employment"));

    factors = DemandFactors();
    factors.services_factor = -80;
    ASSERT(is_bottlenecked_by(factors, "services"));

    factors = DemandFactors();
    factors.tribute_factor = -80;
    ASSERT(is_bottlenecked_by(factors, "tribute"));

    factors = DemandFactors();
    factors.transport_factor = -80;
    ASSERT(is_bottlenecked_by(factors, "transport"));

    factors = DemandFactors();
    factors.contamination_factor = -80;
    ASSERT(is_bottlenecked_by(factors, "contamination"));
}

// =============================================================================
// Zone type constants
// =============================================================================

TEST(zone_constants_match_canon) {
    ASSERT_EQ(ZONE_HABITATION, 0);
    ASSERT_EQ(ZONE_EXCHANGE, 1);
    ASSERT_EQ(ZONE_FABRICATION, 2);
}

// =============================================================================
// Main
// =============================================================================

int main() {
    printf("=== Demand Factors UI Tests (E10-047) ===\n\n");

    RUN_TEST(get_demand_factors_habitation);
    RUN_TEST(get_demand_factors_exchange);
    RUN_TEST(get_demand_factors_fabrication);
    RUN_TEST(get_demand_factors_invalid_zone_returns_habitation);

    RUN_TEST(dominant_factor_population_positive);
    RUN_TEST(dominant_factor_employment_negative);
    RUN_TEST(dominant_factor_services);
    RUN_TEST(dominant_factor_tribute);
    RUN_TEST(dominant_factor_transport);
    RUN_TEST(dominant_factor_contamination);
    RUN_TEST(dominant_factor_all_zero);
    RUN_TEST(dominant_factor_tie_first_wins);

    RUN_TEST(demand_description_strong_growth);
    RUN_TEST(demand_description_growth);
    RUN_TEST(demand_description_weak_growth);
    RUN_TEST(demand_description_stagnant);
    RUN_TEST(demand_description_weak_decline);
    RUN_TEST(demand_description_decline);
    RUN_TEST(demand_description_strong_decline);

    RUN_TEST(sum_factors_all_positive);
    RUN_TEST(sum_factors_mixed);
    RUN_TEST(sum_factors_all_negative);
    RUN_TEST(sum_factors_all_zero);
    RUN_TEST(sum_factors_exceeds_int8_range);

    RUN_TEST(bottleneck_by_population);
    RUN_TEST(bottleneck_by_services);
    RUN_TEST(no_bottleneck_positive_dominant);
    RUN_TEST(no_bottleneck_not_dominant);
    RUN_TEST(bottleneck_invalid_factor_name);
    RUN_TEST(bottleneck_all_factors);

    RUN_TEST(zone_constants_match_canon);

    printf("\n=== Results: %d passed, %d failed ===\n",
           tests_passed, tests_failed);

    return tests_failed > 0 ? 1 : 0;
}
