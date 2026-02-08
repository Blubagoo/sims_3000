/**
 * @file test_nexus_type_config.cpp
 * @brief Unit tests for NexusTypeConfig (Ticket 5-023)
 *
 * Tests cover:
 * - All 6 MVP nexus config values (Carbon, Petrochemical, Gaseous, Nuclear, Wind, Solar)
 * - get_nexus_config lookup for each type
 * - Contamination ordering (Carbon > Petrochemical > Gaseous > 0 for clean types)
 * - Aging floor ordering (increases from Carbon to Solar)
 * - Coverage radius values
 * - Variable output flag (Wind and Solar only)
 * - Terrain requirement values
 * - Fallback behavior for non-MVP types
 */

#include <sims3000/energy/NexusTypeConfig.h>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cstring>

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
// Carbon Nexus Config Tests
// =============================================================================

TEST(carbon_config_values) {
    const NexusTypeConfig& cfg = NEXUS_CONFIGS[0];
    ASSERT_EQ(cfg.type, NexusType::Carbon);
    ASSERT_STR_EQ(cfg.name, "Carbon");
    ASSERT_EQ(cfg.base_output, 100u);
    ASSERT_EQ(cfg.build_cost, 5000u);
    ASSERT_EQ(cfg.maintenance_cost, 50u);
    ASSERT_EQ(cfg.contamination, 200u);
    ASSERT_EQ(cfg.coverage_radius, 8);
    ASSERT_FLOAT_EQ(cfg.aging_floor, 0.60f);
    ASSERT_EQ(cfg.terrain_req, TerrainRequirement::None);
    ASSERT_EQ(cfg.is_variable_output, false);
}

// =============================================================================
// Petrochemical Nexus Config Tests
// =============================================================================

TEST(petrochemical_config_values) {
    const NexusTypeConfig& cfg = NEXUS_CONFIGS[1];
    ASSERT_EQ(cfg.type, NexusType::Petrochemical);
    ASSERT_STR_EQ(cfg.name, "Petrochemical");
    ASSERT_EQ(cfg.base_output, 150u);
    ASSERT_EQ(cfg.build_cost, 8000u);
    ASSERT_EQ(cfg.maintenance_cost, 75u);
    ASSERT_EQ(cfg.contamination, 120u);
    ASSERT_EQ(cfg.coverage_radius, 8);
    ASSERT_FLOAT_EQ(cfg.aging_floor, 0.65f);
    ASSERT_EQ(cfg.terrain_req, TerrainRequirement::None);
    ASSERT_EQ(cfg.is_variable_output, false);
}

// =============================================================================
// Gaseous Nexus Config Tests
// =============================================================================

TEST(gaseous_config_values) {
    const NexusTypeConfig& cfg = NEXUS_CONFIGS[2];
    ASSERT_EQ(cfg.type, NexusType::Gaseous);
    ASSERT_STR_EQ(cfg.name, "Gaseous");
    ASSERT_EQ(cfg.base_output, 120u);
    ASSERT_EQ(cfg.build_cost, 10000u);
    ASSERT_EQ(cfg.maintenance_cost, 60u);
    ASSERT_EQ(cfg.contamination, 40u);
    ASSERT_EQ(cfg.coverage_radius, 8);
    ASSERT_FLOAT_EQ(cfg.aging_floor, 0.70f);
    ASSERT_EQ(cfg.terrain_req, TerrainRequirement::None);
    ASSERT_EQ(cfg.is_variable_output, false);
}

// =============================================================================
// Nuclear Nexus Config Tests
// =============================================================================

TEST(nuclear_config_values) {
    const NexusTypeConfig& cfg = NEXUS_CONFIGS[3];
    ASSERT_EQ(cfg.type, NexusType::Nuclear);
    ASSERT_STR_EQ(cfg.name, "Nuclear");
    ASSERT_EQ(cfg.base_output, 400u);
    ASSERT_EQ(cfg.build_cost, 50000u);
    ASSERT_EQ(cfg.maintenance_cost, 200u);
    ASSERT_EQ(cfg.contamination, 0u);
    ASSERT_EQ(cfg.coverage_radius, 10);
    ASSERT_FLOAT_EQ(cfg.aging_floor, 0.75f);
    ASSERT_EQ(cfg.terrain_req, TerrainRequirement::None);
    ASSERT_EQ(cfg.is_variable_output, false);
}

// =============================================================================
// Wind Nexus Config Tests
// =============================================================================

TEST(wind_config_values) {
    const NexusTypeConfig& cfg = NEXUS_CONFIGS[4];
    ASSERT_EQ(cfg.type, NexusType::Wind);
    ASSERT_STR_EQ(cfg.name, "Wind");
    ASSERT_EQ(cfg.base_output, 30u);
    ASSERT_EQ(cfg.build_cost, 3000u);
    ASSERT_EQ(cfg.maintenance_cost, 20u);
    ASSERT_EQ(cfg.contamination, 0u);
    ASSERT_EQ(cfg.coverage_radius, 4);
    ASSERT_FLOAT_EQ(cfg.aging_floor, 0.80f);
    ASSERT_EQ(cfg.terrain_req, TerrainRequirement::Ridges);
    ASSERT_EQ(cfg.is_variable_output, true);
}

// =============================================================================
// Solar Nexus Config Tests
// =============================================================================

TEST(solar_config_values) {
    const NexusTypeConfig& cfg = NEXUS_CONFIGS[5];
    ASSERT_EQ(cfg.type, NexusType::Solar);
    ASSERT_STR_EQ(cfg.name, "Solar");
    ASSERT_EQ(cfg.base_output, 50u);
    ASSERT_EQ(cfg.build_cost, 6000u);
    ASSERT_EQ(cfg.maintenance_cost, 30u);
    ASSERT_EQ(cfg.contamination, 0u);
    ASSERT_EQ(cfg.coverage_radius, 6);
    ASSERT_FLOAT_EQ(cfg.aging_floor, 0.85f);
    ASSERT_EQ(cfg.terrain_req, TerrainRequirement::None);
    ASSERT_EQ(cfg.is_variable_output, true);
}

// =============================================================================
// get_nexus_config Lookup Tests
// =============================================================================

TEST(get_nexus_config_carbon) {
    const NexusTypeConfig& cfg = get_nexus_config(NexusType::Carbon);
    ASSERT_EQ(cfg.type, NexusType::Carbon);
    ASSERT_STR_EQ(cfg.name, "Carbon");
}

TEST(get_nexus_config_petrochemical) {
    const NexusTypeConfig& cfg = get_nexus_config(NexusType::Petrochemical);
    ASSERT_EQ(cfg.type, NexusType::Petrochemical);
    ASSERT_STR_EQ(cfg.name, "Petrochemical");
}

TEST(get_nexus_config_gaseous) {
    const NexusTypeConfig& cfg = get_nexus_config(NexusType::Gaseous);
    ASSERT_EQ(cfg.type, NexusType::Gaseous);
    ASSERT_STR_EQ(cfg.name, "Gaseous");
}

TEST(get_nexus_config_nuclear) {
    const NexusTypeConfig& cfg = get_nexus_config(NexusType::Nuclear);
    ASSERT_EQ(cfg.type, NexusType::Nuclear);
    ASSERT_STR_EQ(cfg.name, "Nuclear");
}

TEST(get_nexus_config_wind) {
    const NexusTypeConfig& cfg = get_nexus_config(NexusType::Wind);
    ASSERT_EQ(cfg.type, NexusType::Wind);
    ASSERT_STR_EQ(cfg.name, "Wind");
}

TEST(get_nexus_config_solar) {
    const NexusTypeConfig& cfg = get_nexus_config(NexusType::Solar);
    ASSERT_EQ(cfg.type, NexusType::Solar);
    ASSERT_STR_EQ(cfg.name, "Solar");
}

TEST(get_nexus_config_fallback_for_non_mvp) {
    // Non-MVP types should fall back to Carbon
    const NexusTypeConfig& cfg = get_nexus_config(NexusType::Hydro);
    ASSERT_EQ(cfg.type, NexusType::Carbon);

    const NexusTypeConfig& cfg2 = get_nexus_config(NexusType::Fusion);
    ASSERT_EQ(cfg2.type, NexusType::Carbon);

    const NexusTypeConfig& cfg3 = get_nexus_config(NexusType::MicrowaveReceiver);
    ASSERT_EQ(cfg3.type, NexusType::Carbon);
}

// =============================================================================
// Contamination Ordering Tests
// =============================================================================

TEST(contamination_carbon_highest) {
    const NexusTypeConfig& carbon = get_nexus_config(NexusType::Carbon);
    const NexusTypeConfig& petro = get_nexus_config(NexusType::Petrochemical);
    const NexusTypeConfig& gaseous = get_nexus_config(NexusType::Gaseous);

    ASSERT(carbon.contamination > petro.contamination);
    ASSERT(petro.contamination > gaseous.contamination);
    ASSERT(gaseous.contamination > 0);
}

TEST(contamination_clean_types_zero) {
    const NexusTypeConfig& nuclear = get_nexus_config(NexusType::Nuclear);
    const NexusTypeConfig& wind = get_nexus_config(NexusType::Wind);
    const NexusTypeConfig& solar = get_nexus_config(NexusType::Solar);

    ASSERT_EQ(nuclear.contamination, 0u);
    ASSERT_EQ(wind.contamination, 0u);
    ASSERT_EQ(solar.contamination, 0u);
}

TEST(contamination_dirty_types_nonzero) {
    const NexusTypeConfig& carbon = get_nexus_config(NexusType::Carbon);
    const NexusTypeConfig& petro = get_nexus_config(NexusType::Petrochemical);
    const NexusTypeConfig& gaseous = get_nexus_config(NexusType::Gaseous);

    ASSERT(carbon.contamination > 0);
    ASSERT(petro.contamination > 0);
    ASSERT(gaseous.contamination > 0);
}

// =============================================================================
// Aging Floor Ordering Tests
// =============================================================================

TEST(aging_floor_increases_carbon_to_solar) {
    const NexusTypeConfig& carbon = get_nexus_config(NexusType::Carbon);
    const NexusTypeConfig& petro = get_nexus_config(NexusType::Petrochemical);
    const NexusTypeConfig& gaseous = get_nexus_config(NexusType::Gaseous);
    const NexusTypeConfig& nuclear = get_nexus_config(NexusType::Nuclear);
    const NexusTypeConfig& wind = get_nexus_config(NexusType::Wind);
    const NexusTypeConfig& solar = get_nexus_config(NexusType::Solar);

    ASSERT(carbon.aging_floor < petro.aging_floor);
    ASSERT(petro.aging_floor < gaseous.aging_floor);
    ASSERT(gaseous.aging_floor < nuclear.aging_floor);
    ASSERT(nuclear.aging_floor < wind.aging_floor);
    ASSERT(wind.aging_floor < solar.aging_floor);
}

TEST(aging_floor_all_below_one) {
    for (uint8_t i = 0; i < NEXUS_TYPE_MVP_COUNT; ++i) {
        ASSERT(NEXUS_CONFIGS[i].aging_floor < 1.0f);
        ASSERT(NEXUS_CONFIGS[i].aging_floor > 0.0f);
    }
}

// =============================================================================
// Coverage Radius Tests
// =============================================================================

TEST(coverage_radius_values) {
    ASSERT_EQ(get_nexus_config(NexusType::Carbon).coverage_radius, 8);
    ASSERT_EQ(get_nexus_config(NexusType::Petrochemical).coverage_radius, 8);
    ASSERT_EQ(get_nexus_config(NexusType::Gaseous).coverage_radius, 8);
    ASSERT_EQ(get_nexus_config(NexusType::Nuclear).coverage_radius, 10);
    ASSERT_EQ(get_nexus_config(NexusType::Wind).coverage_radius, 4);
    ASSERT_EQ(get_nexus_config(NexusType::Solar).coverage_radius, 6);
}

TEST(coverage_radius_nuclear_largest) {
    const NexusTypeConfig& nuclear = get_nexus_config(NexusType::Nuclear);
    for (uint8_t i = 0; i < NEXUS_TYPE_MVP_COUNT; ++i) {
        ASSERT(nuclear.coverage_radius >= NEXUS_CONFIGS[i].coverage_radius);
    }
}

TEST(coverage_radius_wind_smallest) {
    const NexusTypeConfig& wind = get_nexus_config(NexusType::Wind);
    for (uint8_t i = 0; i < NEXUS_TYPE_MVP_COUNT; ++i) {
        ASSERT(wind.coverage_radius <= NEXUS_CONFIGS[i].coverage_radius);
    }
}

// =============================================================================
// Variable Output Tests
// =============================================================================

TEST(variable_output_only_wind_and_solar) {
    ASSERT_EQ(get_nexus_config(NexusType::Carbon).is_variable_output, false);
    ASSERT_EQ(get_nexus_config(NexusType::Petrochemical).is_variable_output, false);
    ASSERT_EQ(get_nexus_config(NexusType::Gaseous).is_variable_output, false);
    ASSERT_EQ(get_nexus_config(NexusType::Nuclear).is_variable_output, false);
    ASSERT_EQ(get_nexus_config(NexusType::Wind).is_variable_output, true);
    ASSERT_EQ(get_nexus_config(NexusType::Solar).is_variable_output, true);
}

// =============================================================================
// Terrain Requirement Tests
// =============================================================================

TEST(terrain_requirement_values) {
    ASSERT_EQ(get_nexus_config(NexusType::Carbon).terrain_req, TerrainRequirement::None);
    ASSERT_EQ(get_nexus_config(NexusType::Petrochemical).terrain_req, TerrainRequirement::None);
    ASSERT_EQ(get_nexus_config(NexusType::Gaseous).terrain_req, TerrainRequirement::None);
    ASSERT_EQ(get_nexus_config(NexusType::Nuclear).terrain_req, TerrainRequirement::None);
    ASSERT_EQ(get_nexus_config(NexusType::Wind).terrain_req, TerrainRequirement::Ridges);
    ASSERT_EQ(get_nexus_config(NexusType::Solar).terrain_req, TerrainRequirement::None);
}

// =============================================================================
// Config Array Consistency Tests
// =============================================================================

TEST(config_array_indexed_by_type_ordinal) {
    for (uint8_t i = 0; i < NEXUS_TYPE_MVP_COUNT; ++i) {
        ASSERT_EQ(static_cast<uint8_t>(NEXUS_CONFIGS[i].type), i);
    }
}

TEST(config_array_all_names_non_null) {
    for (uint8_t i = 0; i < NEXUS_TYPE_MVP_COUNT; ++i) {
        ASSERT(NEXUS_CONFIGS[i].name != nullptr);
        ASSERT(std::strlen(NEXUS_CONFIGS[i].name) > 0);
    }
}

TEST(config_array_all_outputs_positive) {
    for (uint8_t i = 0; i < NEXUS_TYPE_MVP_COUNT; ++i) {
        ASSERT(NEXUS_CONFIGS[i].base_output > 0);
    }
}

TEST(config_array_all_build_costs_positive) {
    for (uint8_t i = 0; i < NEXUS_TYPE_MVP_COUNT; ++i) {
        ASSERT(NEXUS_CONFIGS[i].build_cost > 0);
    }
}

TEST(config_array_all_maintenance_costs_positive) {
    for (uint8_t i = 0; i < NEXUS_TYPE_MVP_COUNT; ++i) {
        ASSERT(NEXUS_CONFIGS[i].maintenance_cost > 0);
    }
}

TEST(config_array_all_coverage_radii_positive) {
    for (uint8_t i = 0; i < NEXUS_TYPE_MVP_COUNT; ++i) {
        ASSERT(NEXUS_CONFIGS[i].coverage_radius > 0);
    }
}

// =============================================================================
// Main
// =============================================================================

int main() {
    printf("=== NexusTypeConfig Unit Tests (Ticket 5-023) ===\n\n");

    // Individual config value tests
    RUN_TEST(carbon_config_values);
    RUN_TEST(petrochemical_config_values);
    RUN_TEST(gaseous_config_values);
    RUN_TEST(nuclear_config_values);
    RUN_TEST(wind_config_values);
    RUN_TEST(solar_config_values);

    // get_nexus_config lookup tests
    RUN_TEST(get_nexus_config_carbon);
    RUN_TEST(get_nexus_config_petrochemical);
    RUN_TEST(get_nexus_config_gaseous);
    RUN_TEST(get_nexus_config_nuclear);
    RUN_TEST(get_nexus_config_wind);
    RUN_TEST(get_nexus_config_solar);
    RUN_TEST(get_nexus_config_fallback_for_non_mvp);

    // Contamination ordering tests
    RUN_TEST(contamination_carbon_highest);
    RUN_TEST(contamination_clean_types_zero);
    RUN_TEST(contamination_dirty_types_nonzero);

    // Aging floor ordering tests
    RUN_TEST(aging_floor_increases_carbon_to_solar);
    RUN_TEST(aging_floor_all_below_one);

    // Coverage radius tests
    RUN_TEST(coverage_radius_values);
    RUN_TEST(coverage_radius_nuclear_largest);
    RUN_TEST(coverage_radius_wind_smallest);

    // Variable output tests
    RUN_TEST(variable_output_only_wind_and_solar);

    // Terrain requirement tests
    RUN_TEST(terrain_requirement_values);

    // Config array consistency tests
    RUN_TEST(config_array_indexed_by_type_ordinal);
    RUN_TEST(config_array_all_names_non_null);
    RUN_TEST(config_array_all_outputs_positive);
    RUN_TEST(config_array_all_build_costs_positive);
    RUN_TEST(config_array_all_maintenance_costs_positive);
    RUN_TEST(config_array_all_coverage_radii_positive);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
