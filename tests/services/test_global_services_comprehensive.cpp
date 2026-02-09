/**
 * @file test_global_services_comprehensive.cpp
 * @brief Comprehensive unit tests for global service aggregation (Ticket E9-051)
 *
 * Extends beyond E9-023 basic tests with:
 * - Single medical building effectiveness at various populations
 * - Multiple buildings capacity stacking across tiers
 * - Population scaling (monotonically decreasing effectiveness)
 * - Funding modifier application across range
 * - Zero buildings edge cases
 * - Education equivalents with tier-specific capacities
 * - Mixed active/inactive building filtering
 * - Cross-type filtering (medical vs education vs radius-based)
 */

#include <sims3000/services/GlobalServiceAggregation.h>
#include <sims3000/services/ServiceConfigs.h>
#include <sims3000/services/FundingModifier.h>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <vector>

using namespace sims3000::services;

// Helper to create ServiceBuildingData with relevant fields for global aggregation
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
    printf("  Running %s...", #name); \
    test_##name(); \
    printf(" PASSED\n"); \
    tests_passed++; \
} while(0)

#define ASSERT(condition) do { \
    if (!(condition)) { \
        printf("\n    FAILED: %s (line %d)\n", #condition, __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

#define ASSERT_EQ(a, b) do { \
    if ((a) != (b)) { \
        printf("\n    FAILED: %s == %s (line %d)\n", #a, #b, __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

#define ASSERT_FLOAT_EQ(a, b) do { \
    if (std::fabs((a) - (b)) > 0.001f) { \
        printf("\n    FAILED: %s == %s (got %f vs %f) (line %d)\n", \
               #a, #b, (double)(a), (double)(b), __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

#define ASSERT_FLOAT_GT(a, b) do { \
    if (!((a) > (b))) { \
        printf("\n    FAILED: %s > %s (got %f vs %f) (line %d)\n", \
               #a, #b, (double)(a), (double)(b), __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

#define ASSERT_FLOAT_GE(a, b) do { \
    if (!((a) >= (b))) { \
        printf("\n    FAILED: %s >= %s (got %f vs %f) (line %d)\n", \
               #a, #b, (double)(a), (double)(b), __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

#define ASSERT_FLOAT_LE(a, b) do { \
    if (!((a) <= (b))) { \
        printf("\n    FAILED: %s <= %s (got %f vs %f) (line %d)\n", \
               #a, #b, (double)(a), (double)(b), __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

// =============================================================================
// 1. Single Medical Building Effectiveness
// =============================================================================

TEST(single_medical_post_exact_pop) {
    // Medical Post (cap=500) with population 500 -> effectiveness 1.0
    std::vector<ServiceBuildingData> buildings = {
        make_building(ServiceType::Medical, 500, true)
    };
    auto result = calculate_global_service(ServiceType::Medical, buildings, 500);
    ASSERT_FLOAT_EQ(result.effectiveness, 1.0f);
    ASSERT_EQ(result.total_capacity, 500u);
}

TEST(single_medical_post_double_pop) {
    // Medical Post (cap=500) with population 1000 -> effectiveness 0.5
    std::vector<ServiceBuildingData> buildings = {
        make_building(ServiceType::Medical, 500, true)
    };
    auto result = calculate_global_service(ServiceType::Medical, buildings, 1000);
    ASSERT_FLOAT_EQ(result.effectiveness, 0.5f);
    ASSERT_EQ(result.total_capacity, 500u);
}

TEST(single_medical_post_half_pop) {
    // Medical Post (cap=500) with population 250 -> clamped to 1.0
    std::vector<ServiceBuildingData> buildings = {
        make_building(ServiceType::Medical, 500, true)
    };
    auto result = calculate_global_service(ServiceType::Medical, buildings, 250);
    ASSERT_FLOAT_EQ(result.effectiveness, 1.0f);
    ASSERT_EQ(result.total_capacity, 500u);
}

TEST(single_medical_center) {
    // Medical Center (cap=2000) with population 2000 -> effectiveness 1.0
    std::vector<ServiceBuildingData> buildings = {
        make_building(ServiceType::Medical, 2000, true)
    };
    auto result = calculate_global_service(ServiceType::Medical, buildings, 2000);
    ASSERT_FLOAT_EQ(result.effectiveness, 1.0f);
    ASSERT_EQ(result.total_capacity, 2000u);
}

TEST(single_medical_nexus) {
    // Medical Nexus (cap=5000) with population 5000 -> effectiveness 1.0
    std::vector<ServiceBuildingData> buildings = {
        make_building(ServiceType::Medical, 5000, true)
    };
    auto result = calculate_global_service(ServiceType::Medical, buildings, 5000);
    ASSERT_FLOAT_EQ(result.effectiveness, 1.0f);
    ASSERT_EQ(result.total_capacity, 5000u);
}

TEST(single_medical_nexus_over_capacity) {
    // Medical Nexus (cap=5000) with population 2500 -> clamped to 1.0
    std::vector<ServiceBuildingData> buildings = {
        make_building(ServiceType::Medical, 5000, true)
    };
    auto result = calculate_global_service(ServiceType::Medical, buildings, 2500);
    ASSERT_FLOAT_EQ(result.effectiveness, 1.0f);
}

// =============================================================================
// 2. Multiple Buildings Capacity Stacking
// =============================================================================

TEST(two_medical_posts_exact) {
    // Two Medical Posts (500+500=1000) with pop 1000 -> effectiveness 1.0
    std::vector<ServiceBuildingData> buildings = {
        make_building(ServiceType::Medical, 500, true),
        make_building(ServiceType::Medical, 500, true)
    };
    auto result = calculate_global_service(ServiceType::Medical, buildings, 1000);
    ASSERT_FLOAT_EQ(result.effectiveness, 1.0f);
    ASSERT_EQ(result.total_capacity, 1000u);
}

TEST(medical_post_plus_center_exact) {
    // Medical Post + Medical Center (500+2000=2500) with pop 2500 -> effectiveness 1.0
    std::vector<ServiceBuildingData> buildings = {
        make_building(ServiceType::Medical, 500, true),
        make_building(ServiceType::Medical, 2000, true)
    };
    auto result = calculate_global_service(ServiceType::Medical, buildings, 2500);
    ASSERT_FLOAT_EQ(result.effectiveness, 1.0f);
    ASSERT_EQ(result.total_capacity, 2500u);
}

TEST(all_three_medical_tiers_exact) {
    // All three tiers (500+2000+5000=7500) with pop 7500 -> effectiveness 1.0
    std::vector<ServiceBuildingData> buildings = {
        make_building(ServiceType::Medical, 500, true),
        make_building(ServiceType::Medical, 2000, true),
        make_building(ServiceType::Medical, 5000, true)
    };
    auto result = calculate_global_service(ServiceType::Medical, buildings, 7500);
    ASSERT_FLOAT_EQ(result.effectiveness, 1.0f);
    ASSERT_EQ(result.total_capacity, 7500u);
}

TEST(all_three_medical_tiers_double_pop) {
    // All three tiers (7500) with pop 15000 -> effectiveness 0.5
    std::vector<ServiceBuildingData> buildings = {
        make_building(ServiceType::Medical, 500, true),
        make_building(ServiceType::Medical, 2000, true),
        make_building(ServiceType::Medical, 5000, true)
    };
    auto result = calculate_global_service(ServiceType::Medical, buildings, 15000);
    ASSERT_FLOAT_EQ(result.effectiveness, 0.5f);
    ASSERT_EQ(result.total_capacity, 7500u);
}

TEST(many_medical_posts_stacking) {
    // 10 Medical Posts (500*10=5000) with pop 5000 -> effectiveness 1.0
    std::vector<ServiceBuildingData> buildings;
    for (int i = 0; i < 10; i++) {
        buildings.push_back(make_building(ServiceType::Medical, 500, true));
    }
    auto result = calculate_global_service(ServiceType::Medical, buildings, 5000);
    ASSERT_FLOAT_EQ(result.effectiveness, 1.0f);
    ASSERT_EQ(result.total_capacity, 5000u);
}

// =============================================================================
// 3. Population Scaling (Monotonically Decreasing)
// =============================================================================

TEST(population_scaling_monotonic_decrease) {
    // Fixed capacity 1000, population from 100 to 10000
    // Effectiveness should monotonically decrease
    std::vector<ServiceBuildingData> buildings = {
        make_building(ServiceType::Medical, 1000, true)
    };

    uint32_t populations[] = {100, 200, 500, 1000, 2000, 5000, 10000};
    int count = sizeof(populations) / sizeof(populations[0]);

    float prev_effectiveness = 2.0f;  // Start above max
    for (int i = 0; i < count; i++) {
        auto result = calculate_global_service(ServiceType::Medical, buildings, populations[i]);
        // Effectiveness should be <= previous (monotonically decreasing or equal due to clamp)
        ASSERT_FLOAT_LE(result.effectiveness, prev_effectiveness);
        // Effectiveness must be in [0, 1]
        ASSERT_FLOAT_GE(result.effectiveness, 0.0f);
        ASSERT_FLOAT_LE(result.effectiveness, 1.0f);
        prev_effectiveness = result.effectiveness;
    }
}

TEST(population_scaling_values) {
    // Fixed capacity 1000: verify specific effectiveness values
    std::vector<ServiceBuildingData> buildings = {
        make_building(ServiceType::Medical, 1000, true)
    };

    // Pop 100: 1000/100 = 10.0 -> clamped to 1.0
    auto r1 = calculate_global_service(ServiceType::Medical, buildings, 100);
    ASSERT_FLOAT_EQ(r1.effectiveness, 1.0f);

    // Pop 500: 1000/500 = 2.0 -> clamped to 1.0
    auto r2 = calculate_global_service(ServiceType::Medical, buildings, 500);
    ASSERT_FLOAT_EQ(r2.effectiveness, 1.0f);

    // Pop 1000: 1000/1000 = 1.0
    auto r3 = calculate_global_service(ServiceType::Medical, buildings, 1000);
    ASSERT_FLOAT_EQ(r3.effectiveness, 1.0f);

    // Pop 2000: 1000/2000 = 0.5
    auto r4 = calculate_global_service(ServiceType::Medical, buildings, 2000);
    ASSERT_FLOAT_EQ(r4.effectiveness, 0.5f);

    // Pop 4000: 1000/4000 = 0.25
    auto r5 = calculate_global_service(ServiceType::Medical, buildings, 4000);
    ASSERT_FLOAT_EQ(r5.effectiveness, 0.25f);

    // Pop 10000: 1000/10000 = 0.1
    auto r6 = calculate_global_service(ServiceType::Medical, buildings, 10000);
    ASSERT_FLOAT_EQ(r6.effectiveness, 0.1f);
}

TEST(population_scaling_strict_decrease_past_capacity) {
    // For populations > capacity, each increase should strictly decrease effectiveness
    std::vector<ServiceBuildingData> buildings = {
        make_building(ServiceType::Medical, 1000, true)
    };

    float prev = 1.0f;
    for (uint32_t pop = 1000; pop <= 10000; pop += 1000) {
        auto result = calculate_global_service(ServiceType::Medical, buildings, pop);
        ASSERT_FLOAT_LE(result.effectiveness, prev);
        if (pop > 1000) {
            // Strictly less for populations beyond capacity
            ASSERT(result.effectiveness < prev);
        }
        prev = result.effectiveness;
    }
}

// =============================================================================
// 4. Funding Modifier
// =============================================================================

TEST(funding_100_percent_normal) {
    // Medical with 100% funding -> normal effectiveness
    std::vector<ServiceBuildingData> buildings = {
        make_building(ServiceType::Medical, 1000, true)
    };
    auto result = calculate_global_service(ServiceType::Medical, buildings, 1000, 100);
    ASSERT_FLOAT_EQ(result.effectiveness, 1.0f);
}

TEST(funding_50_percent_halved) {
    // Medical with 50% funding -> halved effectiveness
    std::vector<ServiceBuildingData> buildings = {
        make_building(ServiceType::Medical, 1000, true)
    };
    // 1000/1000 = 1.0, * 0.5 funding = 0.5
    auto result = calculate_global_service(ServiceType::Medical, buildings, 1000, 50);
    ASSERT_FLOAT_EQ(result.effectiveness, 0.5f);
}

TEST(funding_0_percent_zero) {
    // Medical with 0% funding -> zero effectiveness
    std::vector<ServiceBuildingData> buildings = {
        make_building(ServiceType::Medical, 1000, true)
    };
    auto result = calculate_global_service(ServiceType::Medical, buildings, 1000, 0);
    ASSERT_FLOAT_EQ(result.effectiveness, 0.0f);
}

TEST(funding_150_percent_capped_1_15x) {
    // Medical with 150% funding -> 1.15x effectiveness (capped by MAX_FUNDING_MODIFIER)
    // But then clamped to 1.0 by the final clamp
    std::vector<ServiceBuildingData> buildings = {
        make_building(ServiceType::Medical, 1000, true)
    };
    // 1000/1000 = 1.0, * 1.15 = 1.15, clamped to 1.0
    auto result = calculate_global_service(ServiceType::Medical, buildings, 1000, 150);
    ASSERT_FLOAT_EQ(result.effectiveness, 1.0f);
}

TEST(funding_150_percent_partial_coverage) {
    // With partial coverage, 150% funding shows its 1.15x effect
    std::vector<ServiceBuildingData> buildings = {
        make_building(ServiceType::Medical, 500, true)
    };
    // 500/1000 = 0.5, * 1.15 = 0.575
    auto result = calculate_global_service(ServiceType::Medical, buildings, 1000, 150);
    ASSERT_FLOAT_EQ(result.effectiveness, 0.575f);
}

TEST(funding_200_percent_same_as_150) {
    // 200% funding also caps at 1.15x
    std::vector<ServiceBuildingData> buildings = {
        make_building(ServiceType::Medical, 500, true)
    };
    // 500/1000 = 0.5, * 1.15 (cap) = 0.575
    auto result = calculate_global_service(ServiceType::Medical, buildings, 1000, 200);
    ASSERT_FLOAT_EQ(result.effectiveness, 0.575f);
}

TEST(funding_25_percent) {
    // 25% funding -> 0.25x effectiveness
    std::vector<ServiceBuildingData> buildings = {
        make_building(ServiceType::Medical, 1000, true)
    };
    // 1000/1000 = 1.0, * 0.25 = 0.25
    auto result = calculate_global_service(ServiceType::Medical, buildings, 1000, 25);
    ASSERT_FLOAT_EQ(result.effectiveness, 0.25f);
}

TEST(funding_modifier_function_directly) {
    // Verify the funding modifier function itself
    ASSERT_FLOAT_EQ(calculate_funding_modifier(0), 0.0f);
    ASSERT_FLOAT_EQ(calculate_funding_modifier(50), 0.5f);
    ASSERT_FLOAT_EQ(calculate_funding_modifier(100), 1.0f);
    ASSERT_FLOAT_EQ(calculate_funding_modifier(115), 1.15f);
    ASSERT_FLOAT_EQ(calculate_funding_modifier(150), 1.15f);
    ASSERT_FLOAT_EQ(calculate_funding_modifier(200), 1.15f);
    ASSERT_FLOAT_EQ(calculate_funding_modifier(255), 1.15f);
}

// =============================================================================
// 5. Zero Buildings
// =============================================================================

TEST(zero_buildings_any_population) {
    // No buildings, any population -> 0.0
    std::vector<ServiceBuildingData> buildings;
    auto result = calculate_global_service(ServiceType::Medical, buildings, 5000);
    ASSERT_FLOAT_EQ(result.effectiveness, 0.0f);
    ASSERT_EQ(result.total_capacity, 0u);
}

TEST(zero_buildings_zero_population) {
    // No buildings, zero population -> 1.0 (edge case: pop==0 check happens first)
    std::vector<ServiceBuildingData> buildings;
    auto result = calculate_global_service(ServiceType::Medical, buildings, 0);
    ASSERT_FLOAT_EQ(result.effectiveness, 1.0f);
    ASSERT_EQ(result.total_capacity, 0u);
}

TEST(zero_buildings_population_1) {
    // No buildings, population 1 -> 0.0
    std::vector<ServiceBuildingData> buildings;
    auto result = calculate_global_service(ServiceType::Medical, buildings, 1);
    ASSERT_FLOAT_EQ(result.effectiveness, 0.0f);
    ASSERT_EQ(result.total_capacity, 0u);
}

TEST(zero_buildings_large_population) {
    // No buildings, large population -> 0.0
    std::vector<ServiceBuildingData> buildings;
    auto result = calculate_global_service(ServiceType::Medical, buildings, 1000000);
    ASSERT_FLOAT_EQ(result.effectiveness, 0.0f);
    ASSERT_EQ(result.total_capacity, 0u);
}

// =============================================================================
// 6. Education Equivalents
// =============================================================================

TEST(education_learning_center_exact) {
    // Learning Center (cap=300) with pop 300 -> effectiveness 1.0
    std::vector<ServiceBuildingData> buildings = {
        make_building(ServiceType::Education, 300, true)
    };
    auto result = calculate_global_service(ServiceType::Education, buildings, 300);
    ASSERT_FLOAT_EQ(result.effectiveness, 1.0f);
    ASSERT_EQ(result.total_capacity, 300u);
}

TEST(education_archive_over_capacity) {
    // Archive (cap=1200) with pop 600 -> clamped to 1.0
    std::vector<ServiceBuildingData> buildings = {
        make_building(ServiceType::Education, 1200, true)
    };
    auto result = calculate_global_service(ServiceType::Education, buildings, 600);
    ASSERT_FLOAT_EQ(result.effectiveness, 1.0f);
    ASSERT_EQ(result.total_capacity, 1200u);
}

TEST(education_knowledge_nexus_half) {
    // Knowledge Nexus (cap=3000) with pop 6000 -> effectiveness 0.5
    std::vector<ServiceBuildingData> buildings = {
        make_building(ServiceType::Education, 3000, true)
    };
    auto result = calculate_global_service(ServiceType::Education, buildings, 6000);
    ASSERT_FLOAT_EQ(result.effectiveness, 0.5f);
    ASSERT_EQ(result.total_capacity, 3000u);
}

TEST(education_all_tiers_stacked) {
    // Learning Center + Archive + Knowledge Nexus (300+1200+3000=4500) with pop 4500
    std::vector<ServiceBuildingData> buildings = {
        make_building(ServiceType::Education, 300, true),
        make_building(ServiceType::Education, 1200, true),
        make_building(ServiceType::Education, 3000, true)
    };
    auto result = calculate_global_service(ServiceType::Education, buildings, 4500);
    ASSERT_FLOAT_EQ(result.effectiveness, 1.0f);
    ASSERT_EQ(result.total_capacity, 4500u);
}

TEST(education_with_funding) {
    // Education with 50% funding
    std::vector<ServiceBuildingData> buildings = {
        make_building(ServiceType::Education, 300, true)
    };
    // 300/300 = 1.0, * 0.5 = 0.5
    auto result = calculate_global_service(ServiceType::Education, buildings, 300, 50);
    ASSERT_FLOAT_EQ(result.effectiveness, 0.5f);
}

TEST(education_zero_buildings) {
    // No education buildings with population
    std::vector<ServiceBuildingData> buildings;
    auto result = calculate_global_service(ServiceType::Education, buildings, 1000);
    ASSERT_FLOAT_EQ(result.effectiveness, 0.0f);
    ASSERT_EQ(result.total_capacity, 0u);
}

// =============================================================================
// 7. Mixed Active/Inactive
// =============================================================================

TEST(active_plus_inactive_only_active_counts) {
    // Active + inactive buildings -> only active count
    std::vector<ServiceBuildingData> buildings = {
        make_building(ServiceType::Medical, 500, true),    // active
        make_building(ServiceType::Medical, 2000, false),  // inactive
        make_building(ServiceType::Medical, 500, true)     // active
    };
    auto result = calculate_global_service(ServiceType::Medical, buildings, 1000);
    ASSERT_FLOAT_EQ(result.effectiveness, 1.0f);
    ASSERT_EQ(result.total_capacity, 1000u);  // Only 500+500, not +2000
}

TEST(all_inactive_returns_zero) {
    // All inactive -> 0.0
    std::vector<ServiceBuildingData> buildings = {
        make_building(ServiceType::Medical, 500, false),
        make_building(ServiceType::Medical, 2000, false),
        make_building(ServiceType::Medical, 5000, false)
    };
    auto result = calculate_global_service(ServiceType::Medical, buildings, 1000);
    ASSERT_FLOAT_EQ(result.effectiveness, 0.0f);
    ASSERT_EQ(result.total_capacity, 0u);
}

TEST(one_active_among_many_inactive) {
    // Only one active building among many inactive
    std::vector<ServiceBuildingData> buildings = {
        make_building(ServiceType::Medical, 500, false),
        make_building(ServiceType::Medical, 500, false),
        make_building(ServiceType::Medical, 500, true),   // only this one
        make_building(ServiceType::Medical, 500, false),
        make_building(ServiceType::Medical, 500, false)
    };
    auto result = calculate_global_service(ServiceType::Medical, buildings, 1000);
    ASSERT_FLOAT_EQ(result.effectiveness, 0.5f);
    ASSERT_EQ(result.total_capacity, 500u);
}

TEST(inactive_education_excluded) {
    // Education: active + inactive
    std::vector<ServiceBuildingData> buildings = {
        make_building(ServiceType::Education, 300, true),
        make_building(ServiceType::Education, 1200, false)  // inactive
    };
    auto result = calculate_global_service(ServiceType::Education, buildings, 600);
    ASSERT_FLOAT_EQ(result.effectiveness, 0.5f);
    ASSERT_EQ(result.total_capacity, 300u);
}

// =============================================================================
// 8. Type Filtering
// =============================================================================

TEST(medical_buildings_dont_contribute_to_education) {
    // Medical buildings don't contribute to education calculation
    std::vector<ServiceBuildingData> buildings = {
        make_building(ServiceType::Medical, 5000, true)
    };
    auto result = calculate_global_service(ServiceType::Education, buildings, 1000);
    ASSERT_FLOAT_EQ(result.effectiveness, 0.0f);
    ASSERT_EQ(result.total_capacity, 0u);
}

TEST(education_buildings_dont_contribute_to_medical) {
    // Education buildings don't contribute to medical calculation
    std::vector<ServiceBuildingData> buildings = {
        make_building(ServiceType::Education, 3000, true)
    };
    auto result = calculate_global_service(ServiceType::Medical, buildings, 1000);
    ASSERT_FLOAT_EQ(result.effectiveness, 0.0f);
    ASSERT_EQ(result.total_capacity, 0u);
}

TEST(enforcer_buildings_zero_for_medical) {
    // Enforcer buildings return 0 capacity for medical global calc
    std::vector<ServiceBuildingData> buildings = {
        make_building(ServiceType::Enforcer, 0, true)
    };
    auto result = calculate_global_service(ServiceType::Medical, buildings, 1000);
    ASSERT_FLOAT_EQ(result.effectiveness, 0.0f);
    ASSERT_EQ(result.total_capacity, 0u);
}

TEST(hazard_buildings_zero_for_education) {
    // HazardResponse buildings return 0 capacity for education global calc
    std::vector<ServiceBuildingData> buildings = {
        make_building(ServiceType::HazardResponse, 0, true)
    };
    auto result = calculate_global_service(ServiceType::Education, buildings, 1000);
    ASSERT_FLOAT_EQ(result.effectiveness, 0.0f);
    ASSERT_EQ(result.total_capacity, 0u);
}

TEST(enforcer_beings_per_unit_zero) {
    // Enforcer has 0 beings per unit (radius-based, not capacity-based)
    ASSERT_EQ(get_beings_per_unit(ServiceType::Enforcer), 0u);
}

TEST(hazard_beings_per_unit_zero) {
    // HazardResponse has 0 beings per unit (radius-based, not capacity-based)
    ASSERT_EQ(get_beings_per_unit(ServiceType::HazardResponse), 0u);
}

TEST(mixed_types_only_matching_counted) {
    // Mix of all four service types, querying for medical
    std::vector<ServiceBuildingData> buildings = {
        make_building(ServiceType::Enforcer, 0, true),
        make_building(ServiceType::HazardResponse, 0, true),
        make_building(ServiceType::Medical, 500, true),
        make_building(ServiceType::Education, 300, true)
    };
    auto result_med = calculate_global_service(ServiceType::Medical, buildings, 500);
    ASSERT_FLOAT_EQ(result_med.effectiveness, 1.0f);
    ASSERT_EQ(result_med.total_capacity, 500u);

    auto result_edu = calculate_global_service(ServiceType::Education, buildings, 300);
    ASSERT_FLOAT_EQ(result_edu.effectiveness, 1.0f);
    ASSERT_EQ(result_edu.total_capacity, 300u);
}

TEST(enforcer_queried_as_global_service) {
    // Querying enforcer via calculate_global_service (radius-based service,
    // but the function still works - it just sums capacity which is 0 for enforcers)
    std::vector<ServiceBuildingData> buildings = {
        make_building(ServiceType::Enforcer, 0, true)
    };
    auto result = calculate_global_service(ServiceType::Enforcer, buildings, 1000);
    // Capacity=0 so total_capacity=0, and population>0, so effectiveness=0
    ASSERT_FLOAT_EQ(result.effectiveness, 0.0f);
    ASSERT_EQ(result.total_capacity, 0u);
}

// =============================================================================
// Main
// =============================================================================

int main() {
    printf("=== Global Services Comprehensive Tests (Ticket E9-051) ===\n\n");

    // 1. Single medical building effectiveness
    printf("[1] Single Medical Building Effectiveness\n");
    RUN_TEST(single_medical_post_exact_pop);
    RUN_TEST(single_medical_post_double_pop);
    RUN_TEST(single_medical_post_half_pop);
    RUN_TEST(single_medical_center);
    RUN_TEST(single_medical_nexus);
    RUN_TEST(single_medical_nexus_over_capacity);
    printf("\n");

    // 2. Multiple buildings capacity stacking
    printf("[2] Multiple Buildings Capacity Stacking\n");
    RUN_TEST(two_medical_posts_exact);
    RUN_TEST(medical_post_plus_center_exact);
    RUN_TEST(all_three_medical_tiers_exact);
    RUN_TEST(all_three_medical_tiers_double_pop);
    RUN_TEST(many_medical_posts_stacking);
    printf("\n");

    // 3. Population scaling
    printf("[3] Population Scaling\n");
    RUN_TEST(population_scaling_monotonic_decrease);
    RUN_TEST(population_scaling_values);
    RUN_TEST(population_scaling_strict_decrease_past_capacity);
    printf("\n");

    // 4. Funding modifier
    printf("[4] Funding Modifier\n");
    RUN_TEST(funding_100_percent_normal);
    RUN_TEST(funding_50_percent_halved);
    RUN_TEST(funding_0_percent_zero);
    RUN_TEST(funding_150_percent_capped_1_15x);
    RUN_TEST(funding_150_percent_partial_coverage);
    RUN_TEST(funding_200_percent_same_as_150);
    RUN_TEST(funding_25_percent);
    RUN_TEST(funding_modifier_function_directly);
    printf("\n");

    // 5. Zero buildings
    printf("[5] Zero Buildings\n");
    RUN_TEST(zero_buildings_any_population);
    RUN_TEST(zero_buildings_zero_population);
    RUN_TEST(zero_buildings_population_1);
    RUN_TEST(zero_buildings_large_population);
    printf("\n");

    // 6. Education equivalents
    printf("[6] Education Equivalents\n");
    RUN_TEST(education_learning_center_exact);
    RUN_TEST(education_archive_over_capacity);
    RUN_TEST(education_knowledge_nexus_half);
    RUN_TEST(education_all_tiers_stacked);
    RUN_TEST(education_with_funding);
    RUN_TEST(education_zero_buildings);
    printf("\n");

    // 7. Mixed active/inactive
    printf("[7] Mixed Active/Inactive\n");
    RUN_TEST(active_plus_inactive_only_active_counts);
    RUN_TEST(all_inactive_returns_zero);
    RUN_TEST(one_active_among_many_inactive);
    RUN_TEST(inactive_education_excluded);
    printf("\n");

    // 8. Type filtering
    printf("[8] Type Filtering\n");
    RUN_TEST(medical_buildings_dont_contribute_to_education);
    RUN_TEST(education_buildings_dont_contribute_to_medical);
    RUN_TEST(enforcer_buildings_zero_for_medical);
    RUN_TEST(hazard_buildings_zero_for_education);
    RUN_TEST(enforcer_beings_per_unit_zero);
    RUN_TEST(hazard_beings_per_unit_zero);
    RUN_TEST(mixed_types_only_matching_counted);
    RUN_TEST(enforcer_queried_as_global_service);
    printf("\n");

    printf("=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
