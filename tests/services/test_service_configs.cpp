/**
 * @file test_service_configs.cpp
 * @brief Unit tests for ServiceConfigs (Tickets E9-030, E9-031, E9-032, E9-033)
 *
 * Tests cover:
 * - All 12 service building configs (4 types * 3 tiers)
 * - Config values: radii, effectiveness, capacities, footprints
 * - Service-specific gameplay constants
 * - Lookup functions: get_service_building_config, service_config_index
 * - Helper functions: is_radius_based_service, is_capacity_based_service
 * - Config array consistency checks
 */

#include <sims3000/services/ServiceConfigs.h>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cstring>

using namespace sims3000::services;

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
        printf("\n  FAILED: %s == %s (line %d)\n", #a, #b, __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

#define ASSERT_STR_EQ(a, b) do { \
    if (std::strcmp((a), (b)) != 0) { \
        printf("\n  FAILED: %s == %s (got \"%s\" vs \"%s\") (line %d)\n", \
               #a, #b, (a), (b), __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

// =============================================================================
// Enforcer Config Tests (E9-030)
// =============================================================================

TEST(enforcer_post_config) {
    const auto& cfg = get_service_building_config(ServiceType::Enforcer, ServiceTier::Post);
    ASSERT_EQ(cfg.type, ServiceType::Enforcer);
    ASSERT_EQ(cfg.tier, ServiceTier::Post);
    ASSERT_STR_EQ(cfg.name, "Enforcer Post");
    ASSERT_EQ(cfg.radius, 8);
    ASSERT_EQ(cfg.effectiveness, 100);
    ASSERT_EQ(cfg.capacity, 0);
    ASSERT_EQ(cfg.footprint_w, 1);
    ASSERT_EQ(cfg.footprint_h, 1);
    ASSERT_EQ(cfg.requires_power, true);
}

TEST(enforcer_station_config) {
    const auto& cfg = get_service_building_config(ServiceType::Enforcer, ServiceTier::Station);
    ASSERT_EQ(cfg.type, ServiceType::Enforcer);
    ASSERT_EQ(cfg.tier, ServiceTier::Station);
    ASSERT_STR_EQ(cfg.name, "Enforcer Station");
    ASSERT_EQ(cfg.radius, 12);
    ASSERT_EQ(cfg.effectiveness, 100);
    ASSERT_EQ(cfg.capacity, 0);
    ASSERT_EQ(cfg.footprint_w, 2);
    ASSERT_EQ(cfg.footprint_h, 2);
    ASSERT_EQ(cfg.requires_power, true);
}

TEST(enforcer_nexus_config) {
    const auto& cfg = get_service_building_config(ServiceType::Enforcer, ServiceTier::Nexus);
    ASSERT_EQ(cfg.type, ServiceType::Enforcer);
    ASSERT_EQ(cfg.tier, ServiceTier::Nexus);
    ASSERT_STR_EQ(cfg.name, "Enforcer Nexus");
    ASSERT_EQ(cfg.radius, 16);
    ASSERT_EQ(cfg.effectiveness, 100);
    ASSERT_EQ(cfg.capacity, 0);
    ASSERT_EQ(cfg.footprint_w, 3);
    ASSERT_EQ(cfg.footprint_h, 3);
    ASSERT_EQ(cfg.requires_power, true);
}

// =============================================================================
// Hazard Response Config Tests (E9-031)
// =============================================================================

TEST(hazard_post_config) {
    const auto& cfg = get_service_building_config(ServiceType::HazardResponse, ServiceTier::Post);
    ASSERT_EQ(cfg.type, ServiceType::HazardResponse);
    ASSERT_EQ(cfg.tier, ServiceTier::Post);
    ASSERT_STR_EQ(cfg.name, "Hazard Post");
    ASSERT_EQ(cfg.radius, 10);
    ASSERT_EQ(cfg.effectiveness, 100);
    ASSERT_EQ(cfg.capacity, 0);
    ASSERT_EQ(cfg.footprint_w, 1);
    ASSERT_EQ(cfg.footprint_h, 1);
    ASSERT_EQ(cfg.requires_power, true);
}

TEST(hazard_station_config) {
    const auto& cfg = get_service_building_config(ServiceType::HazardResponse, ServiceTier::Station);
    ASSERT_EQ(cfg.type, ServiceType::HazardResponse);
    ASSERT_EQ(cfg.tier, ServiceTier::Station);
    ASSERT_STR_EQ(cfg.name, "Hazard Station");
    ASSERT_EQ(cfg.radius, 15);
    ASSERT_EQ(cfg.effectiveness, 100);
    ASSERT_EQ(cfg.capacity, 0);
    ASSERT_EQ(cfg.footprint_w, 2);
    ASSERT_EQ(cfg.footprint_h, 2);
    ASSERT_EQ(cfg.requires_power, true);
}

TEST(hazard_nexus_config) {
    const auto& cfg = get_service_building_config(ServiceType::HazardResponse, ServiceTier::Nexus);
    ASSERT_EQ(cfg.type, ServiceType::HazardResponse);
    ASSERT_EQ(cfg.tier, ServiceTier::Nexus);
    ASSERT_STR_EQ(cfg.name, "Hazard Nexus");
    ASSERT_EQ(cfg.radius, 20);
    ASSERT_EQ(cfg.effectiveness, 100);
    ASSERT_EQ(cfg.capacity, 0);
    ASSERT_EQ(cfg.footprint_w, 3);
    ASSERT_EQ(cfg.footprint_h, 3);
    ASSERT_EQ(cfg.requires_power, true);
}

// =============================================================================
// Medical Config Tests (E9-032)
// =============================================================================

TEST(medical_post_config) {
    const auto& cfg = get_service_building_config(ServiceType::Medical, ServiceTier::Post);
    ASSERT_EQ(cfg.type, ServiceType::Medical);
    ASSERT_EQ(cfg.tier, ServiceTier::Post);
    ASSERT_STR_EQ(cfg.name, "Medical Post");
    ASSERT_EQ(cfg.radius, 0);
    ASSERT_EQ(cfg.effectiveness, 100);
    ASSERT_EQ(cfg.capacity, 500);
    ASSERT_EQ(cfg.footprint_w, 1);
    ASSERT_EQ(cfg.footprint_h, 1);
    ASSERT_EQ(cfg.requires_power, true);
}

TEST(medical_center_config) {
    const auto& cfg = get_service_building_config(ServiceType::Medical, ServiceTier::Station);
    ASSERT_EQ(cfg.type, ServiceType::Medical);
    ASSERT_EQ(cfg.tier, ServiceTier::Station);
    ASSERT_STR_EQ(cfg.name, "Medical Center");
    ASSERT_EQ(cfg.radius, 0);
    ASSERT_EQ(cfg.effectiveness, 100);
    ASSERT_EQ(cfg.capacity, 2000);
    ASSERT_EQ(cfg.footprint_w, 2);
    ASSERT_EQ(cfg.footprint_h, 2);
    ASSERT_EQ(cfg.requires_power, true);
}

TEST(medical_nexus_config) {
    const auto& cfg = get_service_building_config(ServiceType::Medical, ServiceTier::Nexus);
    ASSERT_EQ(cfg.type, ServiceType::Medical);
    ASSERT_EQ(cfg.tier, ServiceTier::Nexus);
    ASSERT_STR_EQ(cfg.name, "Medical Nexus");
    ASSERT_EQ(cfg.radius, 0);
    ASSERT_EQ(cfg.effectiveness, 100);
    ASSERT_EQ(cfg.capacity, 5000);
    ASSERT_EQ(cfg.footprint_w, 3);
    ASSERT_EQ(cfg.footprint_h, 3);
    ASSERT_EQ(cfg.requires_power, true);
}

// =============================================================================
// Education Config Tests (E9-033)
// =============================================================================

TEST(learning_center_config) {
    const auto& cfg = get_service_building_config(ServiceType::Education, ServiceTier::Post);
    ASSERT_EQ(cfg.type, ServiceType::Education);
    ASSERT_EQ(cfg.tier, ServiceTier::Post);
    ASSERT_STR_EQ(cfg.name, "Learning Center");
    ASSERT_EQ(cfg.radius, 0);
    ASSERT_EQ(cfg.effectiveness, 100);
    ASSERT_EQ(cfg.capacity, 300);
    ASSERT_EQ(cfg.footprint_w, 1);
    ASSERT_EQ(cfg.footprint_h, 1);
    ASSERT_EQ(cfg.requires_power, true);
}

TEST(archive_config) {
    const auto& cfg = get_service_building_config(ServiceType::Education, ServiceTier::Station);
    ASSERT_EQ(cfg.type, ServiceType::Education);
    ASSERT_EQ(cfg.tier, ServiceTier::Station);
    ASSERT_STR_EQ(cfg.name, "Archive");
    ASSERT_EQ(cfg.radius, 0);
    ASSERT_EQ(cfg.effectiveness, 100);
    ASSERT_EQ(cfg.capacity, 1200);
    ASSERT_EQ(cfg.footprint_w, 2);
    ASSERT_EQ(cfg.footprint_h, 2);
    ASSERT_EQ(cfg.requires_power, true);
}

TEST(knowledge_nexus_config) {
    const auto& cfg = get_service_building_config(ServiceType::Education, ServiceTier::Nexus);
    ASSERT_EQ(cfg.type, ServiceType::Education);
    ASSERT_EQ(cfg.tier, ServiceTier::Nexus);
    ASSERT_STR_EQ(cfg.name, "Knowledge Nexus");
    ASSERT_EQ(cfg.radius, 0);
    ASSERT_EQ(cfg.effectiveness, 100);
    ASSERT_EQ(cfg.capacity, 3000);
    ASSERT_EQ(cfg.footprint_w, 3);
    ASSERT_EQ(cfg.footprint_h, 3);
    ASSERT_EQ(cfg.requires_power, true);
}

// =============================================================================
// Gameplay Constants Tests
// =============================================================================

TEST(enforcer_suppression_multiplier) {
    ASSERT_FLOAT_EQ(ENFORCER_SUPPRESSION_MULTIPLIER, 0.7f);
    // Verify it represents a reduction (less than 1.0)
    ASSERT(ENFORCER_SUPPRESSION_MULTIPLIER < 1.0f);
    ASSERT(ENFORCER_SUPPRESSION_MULTIPLIER > 0.0f);
}

TEST(hazard_suppression_speed) {
    ASSERT_FLOAT_EQ(HAZARD_SUPPRESSION_SPEED, 3.0f);
    // Verify it represents a speedup (greater than 1.0)
    ASSERT(HAZARD_SUPPRESSION_SPEED > 1.0f);
}

TEST(medical_longevity_constants) {
    ASSERT_EQ(MEDICAL_BASE_LONGEVITY, 60u);
    ASSERT_EQ(MEDICAL_MAX_LONGEVITY_BONUS, 40u);
    // Total max longevity = base + bonus = 100
    ASSERT_EQ(MEDICAL_BASE_LONGEVITY + MEDICAL_MAX_LONGEVITY_BONUS, 100u);
}

TEST(beings_per_medical_unit) {
    ASSERT_EQ(BEINGS_PER_MEDICAL_UNIT, 500u);
    ASSERT(BEINGS_PER_MEDICAL_UNIT > 0);
}

TEST(beings_per_education_unit) {
    ASSERT_EQ(BEINGS_PER_EDUCATION_UNIT, 300u);
    ASSERT(BEINGS_PER_EDUCATION_UNIT > 0);
}

TEST(education_knowledge_bonus) {
    ASSERT_FLOAT_EQ(EDUCATION_KNOWLEDGE_BONUS, 0.1f);
    // Verify it's a reasonable percentage bonus
    ASSERT(EDUCATION_KNOWLEDGE_BONUS > 0.0f);
    ASSERT(EDUCATION_KNOWLEDGE_BONUS <= 1.0f);
}

// =============================================================================
// Lookup Function Tests
// =============================================================================

TEST(service_config_index_calculation) {
    // Enforcer: indices 0, 1, 2  (tier values 1,2,3 -> indices 0,1,2)
    ASSERT_EQ(service_config_index(ServiceType::Enforcer, ServiceTier::Post), 0);
    ASSERT_EQ(service_config_index(ServiceType::Enforcer, ServiceTier::Station), 1);
    ASSERT_EQ(service_config_index(ServiceType::Enforcer, ServiceTier::Nexus), 2);

    // HazardResponse: indices 3, 4, 5
    ASSERT_EQ(service_config_index(ServiceType::HazardResponse, ServiceTier::Post), 3);
    ASSERT_EQ(service_config_index(ServiceType::HazardResponse, ServiceTier::Station), 4);
    ASSERT_EQ(service_config_index(ServiceType::HazardResponse, ServiceTier::Nexus), 5);

    // Medical: indices 6, 7, 8
    ASSERT_EQ(service_config_index(ServiceType::Medical, ServiceTier::Post), 6);
    ASSERT_EQ(service_config_index(ServiceType::Medical, ServiceTier::Station), 7);
    ASSERT_EQ(service_config_index(ServiceType::Medical, ServiceTier::Nexus), 8);

    // Education: indices 9, 10, 11
    ASSERT_EQ(service_config_index(ServiceType::Education, ServiceTier::Post), 9);
    ASSERT_EQ(service_config_index(ServiceType::Education, ServiceTier::Station), 10);
    ASSERT_EQ(service_config_index(ServiceType::Education, ServiceTier::Nexus), 11);
}

TEST(get_service_building_config_returns_correct_type_and_tier) {
    // Verify every config entry matches its expected type and tier
    for (uint8_t t = 0; t < SERVICE_TYPE_COUNT; ++t) {
        for (uint8_t r = 1; r <= SERVICE_TIER_COUNT; ++r) {
            auto type = static_cast<ServiceType>(t);
            auto tier = static_cast<ServiceTier>(r);
            const auto& cfg = get_service_building_config(type, tier);
            ASSERT_EQ(cfg.type, type);
            ASSERT_EQ(cfg.tier, tier);
        }
    }
}

// =============================================================================
// Helper Function Tests
// =============================================================================

TEST(is_radius_based_service_check) {
    ASSERT_EQ(is_radius_based_service(ServiceType::Enforcer), true);
    ASSERT_EQ(is_radius_based_service(ServiceType::HazardResponse), true);
    ASSERT_EQ(is_radius_based_service(ServiceType::Medical), false);
    ASSERT_EQ(is_radius_based_service(ServiceType::Education), false);
}

TEST(is_capacity_based_service_check) {
    ASSERT_EQ(is_capacity_based_service(ServiceType::Enforcer), false);
    ASSERT_EQ(is_capacity_based_service(ServiceType::HazardResponse), false);
    ASSERT_EQ(is_capacity_based_service(ServiceType::Medical), true);
    ASSERT_EQ(is_capacity_based_service(ServiceType::Education), true);
}

TEST(get_service_footprint_area_values) {
    // Post: 1x1 = 1
    ASSERT_EQ(get_service_footprint_area(ServiceType::Enforcer, ServiceTier::Post), 1);
    ASSERT_EQ(get_service_footprint_area(ServiceType::HazardResponse, ServiceTier::Post), 1);
    ASSERT_EQ(get_service_footprint_area(ServiceType::Medical, ServiceTier::Post), 1);
    ASSERT_EQ(get_service_footprint_area(ServiceType::Education, ServiceTier::Post), 1);

    // Station: 2x2 = 4
    ASSERT_EQ(get_service_footprint_area(ServiceType::Enforcer, ServiceTier::Station), 4);
    ASSERT_EQ(get_service_footprint_area(ServiceType::HazardResponse, ServiceTier::Station), 4);
    ASSERT_EQ(get_service_footprint_area(ServiceType::Medical, ServiceTier::Station), 4);
    ASSERT_EQ(get_service_footprint_area(ServiceType::Education, ServiceTier::Station), 4);

    // Nexus: 3x3 = 9
    ASSERT_EQ(get_service_footprint_area(ServiceType::Enforcer, ServiceTier::Nexus), 9);
    ASSERT_EQ(get_service_footprint_area(ServiceType::HazardResponse, ServiceTier::Nexus), 9);
    ASSERT_EQ(get_service_footprint_area(ServiceType::Medical, ServiceTier::Nexus), 9);
    ASSERT_EQ(get_service_footprint_area(ServiceType::Education, ServiceTier::Nexus), 9);
}

TEST(service_type_to_string_values) {
    ASSERT_STR_EQ(service_type_to_string(ServiceType::Enforcer), "Enforcer");
    ASSERT_STR_EQ(service_type_to_string(ServiceType::HazardResponse), "HazardResponse");
    ASSERT_STR_EQ(service_type_to_string(ServiceType::Medical), "Medical");
    ASSERT_STR_EQ(service_type_to_string(ServiceType::Education), "Education");
}

TEST(service_tier_to_string_values) {
    ASSERT_STR_EQ(service_tier_to_string(ServiceTier::Post), "Post");
    ASSERT_STR_EQ(service_tier_to_string(ServiceTier::Station), "Station");
    ASSERT_STR_EQ(service_tier_to_string(ServiceTier::Nexus), "Nexus");
}

// =============================================================================
// Config Array Consistency Tests
// =============================================================================

TEST(config_array_all_names_non_null) {
    for (uint8_t i = 0; i < SERVICE_CONFIG_COUNT; ++i) {
        ASSERT(SERVICE_CONFIGS[i].name != nullptr);
        ASSERT(std::strlen(SERVICE_CONFIGS[i].name) > 0);
    }
}

TEST(config_array_all_effectiveness_100) {
    for (uint8_t i = 0; i < SERVICE_CONFIG_COUNT; ++i) {
        ASSERT_EQ(SERVICE_CONFIGS[i].effectiveness, 100);
    }
}

TEST(config_array_all_require_power) {
    for (uint8_t i = 0; i < SERVICE_CONFIG_COUNT; ++i) {
        ASSERT_EQ(SERVICE_CONFIGS[i].requires_power, true);
    }
}

TEST(config_array_footprint_increases_with_tier) {
    for (uint8_t t = 0; t < SERVICE_TYPE_COUNT; ++t) {
        auto type = static_cast<ServiceType>(t);
        const auto& post = get_service_building_config(type, ServiceTier::Post);
        const auto& station = get_service_building_config(type, ServiceTier::Station);
        const auto& nexus = get_service_building_config(type, ServiceTier::Nexus);

        // Post < Station < Nexus footprint
        ASSERT(post.footprint_w < station.footprint_w);
        ASSERT(station.footprint_w < nexus.footprint_w);
        ASSERT(post.footprint_h < station.footprint_h);
        ASSERT(station.footprint_h < nexus.footprint_h);
    }
}

TEST(config_array_radius_based_services_have_increasing_radius) {
    // Enforcer and HazardResponse should have increasing radius by tier
    ServiceType radius_types[] = { ServiceType::Enforcer, ServiceType::HazardResponse };
    for (auto type : radius_types) {
        const auto& post = get_service_building_config(type, ServiceTier::Post);
        const auto& station = get_service_building_config(type, ServiceTier::Station);
        const auto& nexus = get_service_building_config(type, ServiceTier::Nexus);

        ASSERT(post.radius < station.radius);
        ASSERT(station.radius < nexus.radius);
        ASSERT(post.radius > 0);
    }
}

TEST(config_array_capacity_based_services_have_zero_radius) {
    // Medical and Education should have radius=0 (global)
    ServiceType capacity_types[] = { ServiceType::Medical, ServiceType::Education };
    for (auto type : capacity_types) {
        for (uint8_t r = 1; r <= SERVICE_TIER_COUNT; ++r) {
            auto tier = static_cast<ServiceTier>(r);
            const auto& cfg = get_service_building_config(type, tier);
            ASSERT_EQ(cfg.radius, 0);
        }
    }
}

TEST(config_array_capacity_based_services_have_increasing_capacity) {
    // Medical and Education should have increasing capacity by tier
    ServiceType capacity_types[] = { ServiceType::Medical, ServiceType::Education };
    for (auto type : capacity_types) {
        const auto& post = get_service_building_config(type, ServiceTier::Post);
        const auto& station = get_service_building_config(type, ServiceTier::Station);
        const auto& nexus = get_service_building_config(type, ServiceTier::Nexus);

        ASSERT(post.capacity < station.capacity);
        ASSERT(station.capacity < nexus.capacity);
        ASSERT(post.capacity > 0);
    }
}

TEST(config_array_radius_based_services_have_zero_capacity) {
    // Enforcer and HazardResponse should have capacity=0
    ServiceType radius_types[] = { ServiceType::Enforcer, ServiceType::HazardResponse };
    for (auto type : radius_types) {
        for (uint8_t r = 1; r <= SERVICE_TIER_COUNT; ++r) {
            auto tier = static_cast<ServiceTier>(r);
            const auto& cfg = get_service_building_config(type, tier);
            ASSERT_EQ(cfg.capacity, 0);
        }
    }
}

TEST(config_array_indexed_correctly) {
    // Verify each entry's type/tier matches its position in array
    // Type = index / SERVICE_TIER_COUNT, Tier = (index % SERVICE_TIER_COUNT) + 1
    for (uint8_t i = 0; i < SERVICE_CONFIG_COUNT; ++i) {
        uint8_t expected_type = i / SERVICE_TIER_COUNT;
        uint8_t expected_tier = (i % SERVICE_TIER_COUNT) + 1;  // 1-based tiers
        ASSERT_EQ(static_cast<uint8_t>(SERVICE_CONFIGS[i].type), expected_type);
        ASSERT_EQ(static_cast<uint8_t>(SERVICE_CONFIGS[i].tier), expected_tier);
    }
}

// =============================================================================
// Main
// =============================================================================

int main() {
    printf("=== ServiceConfigs Unit Tests (Tickets E9-030, E9-031, E9-032, E9-033) ===\n\n");

    // Enforcer config tests (E9-030)
    RUN_TEST(enforcer_post_config);
    RUN_TEST(enforcer_station_config);
    RUN_TEST(enforcer_nexus_config);

    // Hazard config tests (E9-031)
    RUN_TEST(hazard_post_config);
    RUN_TEST(hazard_station_config);
    RUN_TEST(hazard_nexus_config);

    // Medical config tests (E9-032)
    RUN_TEST(medical_post_config);
    RUN_TEST(medical_center_config);
    RUN_TEST(medical_nexus_config);

    // Education config tests (E9-033)
    RUN_TEST(learning_center_config);
    RUN_TEST(archive_config);
    RUN_TEST(knowledge_nexus_config);

    // Gameplay constants tests
    RUN_TEST(enforcer_suppression_multiplier);
    RUN_TEST(hazard_suppression_speed);
    RUN_TEST(medical_longevity_constants);
    RUN_TEST(beings_per_medical_unit);
    RUN_TEST(beings_per_education_unit);
    RUN_TEST(education_knowledge_bonus);

    // Lookup function tests
    RUN_TEST(service_config_index_calculation);
    RUN_TEST(get_service_building_config_returns_correct_type_and_tier);

    // Helper function tests
    RUN_TEST(is_radius_based_service_check);
    RUN_TEST(is_capacity_based_service_check);
    RUN_TEST(get_service_footprint_area_values);
    RUN_TEST(service_type_to_string_values);
    RUN_TEST(service_tier_to_string_values);

    // Config array consistency tests
    RUN_TEST(config_array_all_names_non_null);
    RUN_TEST(config_array_all_effectiveness_100);
    RUN_TEST(config_array_all_require_power);
    RUN_TEST(config_array_footprint_increases_with_tier);
    RUN_TEST(config_array_radius_based_services_have_increasing_radius);
    RUN_TEST(config_array_capacity_based_services_have_zero_radius);
    RUN_TEST(config_array_capacity_based_services_have_increasing_capacity);
    RUN_TEST(config_array_radius_based_services_have_zero_capacity);
    RUN_TEST(config_array_indexed_correctly);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
