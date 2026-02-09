/**
 * @file test_global_service_aggregation.cpp
 * @brief Unit tests for GlobalServiceAggregation (Ticket E9-023)
 *
 * Tests cover:
 * - get_beings_per_unit() for all service types
 * - calculate_global_service() with various scenarios:
 *   - Zero population (full effectiveness)
 *   - No buildings (zero effectiveness)
 *   - Exact capacity match
 *   - Over-capacity (clamped to 1.0)
 *   - Under-capacity (partial effectiveness)
 *   - Inactive buildings excluded
 *   - Mixed service types filtered correctly
 *   - Funding modifier applied
 *   - Medical and Education types
 */

#include <sims3000/services/GlobalServiceAggregation.h>
#include <sims3000/services/ServiceConfigs.h>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <vector>

using namespace sims3000::services;

// Helper to create ServiceBuildingData with only the fields relevant to global aggregation
static ServiceBuildingData make_building(ServiceType type, uint16_t capacity, bool active) {
    ServiceBuildingData b{};
    b.x = 0;
    b.y = 0;
    b.type = type;
    b.tier = 1;
    b.effectiveness = 100;
    b.is_active = active;
    b.owner_id = 0;
    b.capacity = capacity;
    return b;
}

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

#define ASSERT_FLOAT_EQ(a, b) do { \
    if (std::fabs((a) - (b)) > 0.001f) { \
        printf("\n  FAILED: %s == %s (got %f vs %f) (line %d)\n", \
               #a, #b, (double)(a), (double)(b), __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

// =============================================================================
// get_beings_per_unit Tests
// =============================================================================

TEST(beings_per_unit_medical) {
    ASSERT_EQ(get_beings_per_unit(ServiceType::Medical), BEINGS_PER_MEDICAL_UNIT);
    ASSERT_EQ(get_beings_per_unit(ServiceType::Medical), 500u);
}

TEST(beings_per_unit_education) {
    ASSERT_EQ(get_beings_per_unit(ServiceType::Education), BEINGS_PER_EDUCATION_UNIT);
    ASSERT_EQ(get_beings_per_unit(ServiceType::Education), 300u);
}

TEST(beings_per_unit_enforcer_returns_zero) {
    ASSERT_EQ(get_beings_per_unit(ServiceType::Enforcer), 0u);
}

TEST(beings_per_unit_hazard_returns_zero) {
    ASSERT_EQ(get_beings_per_unit(ServiceType::HazardResponse), 0u);
}

// =============================================================================
// calculate_global_service - Zero Population Tests
// =============================================================================

TEST(zero_population_returns_full_effectiveness) {
    std::vector<ServiceBuildingData> buildings = {
        make_building(ServiceType::Medical, 500, true)
    };
    auto result = calculate_global_service(ServiceType::Medical, buildings, 0);
    ASSERT_FLOAT_EQ(result.effectiveness, 1.0f);
    ASSERT_EQ(result.total_capacity, 500u);
}

TEST(zero_population_no_buildings_returns_full_effectiveness) {
    std::vector<ServiceBuildingData> buildings;
    auto result = calculate_global_service(ServiceType::Medical, buildings, 0);
    ASSERT_FLOAT_EQ(result.effectiveness, 1.0f);
    ASSERT_EQ(result.total_capacity, 0u);
}

// =============================================================================
// calculate_global_service - No Buildings Tests
// =============================================================================

TEST(no_buildings_returns_zero_effectiveness) {
    std::vector<ServiceBuildingData> buildings;
    auto result = calculate_global_service(ServiceType::Medical, buildings, 1000);
    ASSERT_FLOAT_EQ(result.effectiveness, 0.0f);
    ASSERT_EQ(result.total_capacity, 0u);
}

TEST(all_inactive_buildings_returns_zero_effectiveness) {
    std::vector<ServiceBuildingData> buildings = {
        make_building(ServiceType::Medical, 500, false),
        make_building(ServiceType::Medical, 2000, false)
    };
    auto result = calculate_global_service(ServiceType::Medical, buildings, 1000);
    ASSERT_FLOAT_EQ(result.effectiveness, 0.0f);
    ASSERT_EQ(result.total_capacity, 0u);
}

// =============================================================================
// calculate_global_service - Capacity/Population Ratio Tests
// =============================================================================

TEST(exact_capacity_equals_population) {
    // 1000 capacity serving 1000 population = 100% effectiveness
    std::vector<ServiceBuildingData> buildings = {
        make_building(ServiceType::Medical, 500, true),
        make_building(ServiceType::Medical, 500, true)
    };
    auto result = calculate_global_service(ServiceType::Medical, buildings, 1000);
    ASSERT_FLOAT_EQ(result.effectiveness, 1.0f);
    ASSERT_EQ(result.total_capacity, 1000u);
}

TEST(over_capacity_clamped_to_one) {
    // 2000 capacity serving 500 population = clamped to 1.0
    std::vector<ServiceBuildingData> buildings = {
        make_building(ServiceType::Medical, 2000, true)
    };
    auto result = calculate_global_service(ServiceType::Medical, buildings, 500);
    ASSERT_FLOAT_EQ(result.effectiveness, 1.0f);
    ASSERT_EQ(result.total_capacity, 2000u);
}

TEST(half_capacity_returns_half_effectiveness) {
    // 500 capacity serving 1000 population = 50% effectiveness
    std::vector<ServiceBuildingData> buildings = {
        make_building(ServiceType::Medical, 500, true)
    };
    auto result = calculate_global_service(ServiceType::Medical, buildings, 1000);
    ASSERT_FLOAT_EQ(result.effectiveness, 0.5f);
    ASSERT_EQ(result.total_capacity, 500u);
}

TEST(quarter_capacity) {
    // 250 capacity serving 1000 population = 25%
    std::vector<ServiceBuildingData> buildings = {
        make_building(ServiceType::Medical, 250, true)
    };
    auto result = calculate_global_service(ServiceType::Medical, buildings, 1000);
    ASSERT_FLOAT_EQ(result.effectiveness, 0.25f);
}

// =============================================================================
// calculate_global_service - Building Filtering Tests
// =============================================================================

TEST(inactive_buildings_excluded_from_capacity) {
    std::vector<ServiceBuildingData> buildings = {
        make_building(ServiceType::Medical, 500, true),
        make_building(ServiceType::Medical, 500, false),  // inactive - not counted
        make_building(ServiceType::Medical, 500, true)
    };
    auto result = calculate_global_service(ServiceType::Medical, buildings, 1000);
    ASSERT_FLOAT_EQ(result.effectiveness, 1.0f);
    ASSERT_EQ(result.total_capacity, 1000u);
}

TEST(wrong_type_buildings_excluded) {
    std::vector<ServiceBuildingData> buildings = {
        make_building(ServiceType::Medical, 500, true),
        make_building(ServiceType::Education, 1000, true),  // wrong type - not counted
        make_building(ServiceType::Enforcer, 2000, true)     // wrong type - not counted
    };
    auto result = calculate_global_service(ServiceType::Medical, buildings, 1000);
    ASSERT_FLOAT_EQ(result.effectiveness, 0.5f);
    ASSERT_EQ(result.total_capacity, 500u);
}

TEST(education_type_filtering) {
    std::vector<ServiceBuildingData> buildings = {
        make_building(ServiceType::Education, 300, true),
        make_building(ServiceType::Education, 1200, true),
        make_building(ServiceType::Medical, 5000, true)  // wrong type
    };
    auto result = calculate_global_service(ServiceType::Education, buildings, 1500);
    ASSERT_FLOAT_EQ(result.effectiveness, 1.0f);
    ASSERT_EQ(result.total_capacity, 1500u);
}

// =============================================================================
// calculate_global_service - Funding Modifier Tests
// =============================================================================

TEST(default_funding_100_percent) {
    std::vector<ServiceBuildingData> buildings = {
        make_building(ServiceType::Medical, 500, true)
    };
    // Default funding = 100, should not change effectiveness
    auto result = calculate_global_service(ServiceType::Medical, buildings, 1000);
    ASSERT_FLOAT_EQ(result.effectiveness, 0.5f);
}

TEST(funding_50_percent_halves_effectiveness) {
    std::vector<ServiceBuildingData> buildings = {
        make_building(ServiceType::Medical, 500, true)
    };
    auto result = calculate_global_service(ServiceType::Medical, buildings, 1000, 50);
    // 500/1000 = 0.5, * 0.5 funding = 0.25
    ASSERT_FLOAT_EQ(result.effectiveness, 0.25f);
}

TEST(funding_0_percent_zeroes_effectiveness) {
    std::vector<ServiceBuildingData> buildings = {
        make_building(ServiceType::Medical, 1000, true)
    };
    auto result = calculate_global_service(ServiceType::Medical, buildings, 1000, 0);
    ASSERT_FLOAT_EQ(result.effectiveness, 0.0f);
}

TEST(funding_150_percent_capped_at_115) {
    std::vector<ServiceBuildingData> buildings = {
        make_building(ServiceType::Medical, 1000, true)
    };
    // 1000/1000 = 1.0, * 1.15 (capped) = 1.15, clamped to 1.0
    auto result = calculate_global_service(ServiceType::Medical, buildings, 1000, 150);
    ASSERT_FLOAT_EQ(result.effectiveness, 1.0f);
}

TEST(funding_increases_partial_coverage) {
    std::vector<ServiceBuildingData> buildings = {
        make_building(ServiceType::Medical, 500, true)
    };
    // 500/1000 = 0.5, * 1.15 = 0.575
    auto result = calculate_global_service(ServiceType::Medical, buildings, 1000, 150);
    ASSERT_FLOAT_EQ(result.effectiveness, 0.575f);
}

// =============================================================================
// calculate_global_service - Multiple Building Aggregation
// =============================================================================

TEST(multiple_medical_buildings_aggregate) {
    // Medical Post=500 + Medical Center=2000 + Medical Nexus=5000 = 7500
    std::vector<ServiceBuildingData> buildings = {
        make_building(ServiceType::Medical, 500, true),
        make_building(ServiceType::Medical, 2000, true),
        make_building(ServiceType::Medical, 5000, true)
    };
    auto result = calculate_global_service(ServiceType::Medical, buildings, 10000);
    ASSERT_EQ(result.total_capacity, 7500u);
    ASSERT_FLOAT_EQ(result.effectiveness, 0.75f);
}

TEST(multiple_education_buildings_aggregate) {
    // Learning Center=300 + Archive=1200 + Knowledge Nexus=3000 = 4500
    std::vector<ServiceBuildingData> buildings = {
        make_building(ServiceType::Education, 300, true),
        make_building(ServiceType::Education, 1200, true),
        make_building(ServiceType::Education, 3000, true)
    };
    auto result = calculate_global_service(ServiceType::Education, buildings, 4500);
    ASSERT_EQ(result.total_capacity, 4500u);
    ASSERT_FLOAT_EQ(result.effectiveness, 1.0f);
}

// =============================================================================
// calculate_global_service - Edge Cases
// =============================================================================

TEST(large_population_small_capacity) {
    std::vector<ServiceBuildingData> buildings = {
        make_building(ServiceType::Medical, 500, true)
    };
    auto result = calculate_global_service(ServiceType::Medical, buildings, 100000);
    ASSERT_FLOAT_EQ(result.effectiveness, 0.005f);
}

TEST(single_being_population) {
    std::vector<ServiceBuildingData> buildings = {
        make_building(ServiceType::Medical, 500, true)
    };
    auto result = calculate_global_service(ServiceType::Medical, buildings, 1);
    // 500/1 = 500.0, clamped to 1.0
    ASSERT_FLOAT_EQ(result.effectiveness, 1.0f);
}

// =============================================================================
// Main
// =============================================================================

int main() {
    printf("=== GlobalServiceAggregation Unit Tests (Ticket E9-023) ===\n\n");

    // get_beings_per_unit tests
    RUN_TEST(beings_per_unit_medical);
    RUN_TEST(beings_per_unit_education);
    RUN_TEST(beings_per_unit_enforcer_returns_zero);
    RUN_TEST(beings_per_unit_hazard_returns_zero);

    // Zero population tests
    RUN_TEST(zero_population_returns_full_effectiveness);
    RUN_TEST(zero_population_no_buildings_returns_full_effectiveness);

    // No buildings tests
    RUN_TEST(no_buildings_returns_zero_effectiveness);
    RUN_TEST(all_inactive_buildings_returns_zero_effectiveness);

    // Capacity/population ratio tests
    RUN_TEST(exact_capacity_equals_population);
    RUN_TEST(over_capacity_clamped_to_one);
    RUN_TEST(half_capacity_returns_half_effectiveness);
    RUN_TEST(quarter_capacity);

    // Building filtering tests
    RUN_TEST(inactive_buildings_excluded_from_capacity);
    RUN_TEST(wrong_type_buildings_excluded);
    RUN_TEST(education_type_filtering);

    // Funding modifier tests
    RUN_TEST(default_funding_100_percent);
    RUN_TEST(funding_50_percent_halves_effectiveness);
    RUN_TEST(funding_0_percent_zeroes_effectiveness);
    RUN_TEST(funding_150_percent_capped_at_115);
    RUN_TEST(funding_increases_partial_coverage);

    // Multiple building aggregation
    RUN_TEST(multiple_medical_buildings_aggregate);
    RUN_TEST(multiple_education_buildings_aggregate);

    // Edge cases
    RUN_TEST(large_population_small_capacity);
    RUN_TEST(single_being_population);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
