/**
 * @file test_population_components.cpp
 * @brief Tests for population component definitions (Tickets E10-010 through E10-013)
 *
 * Verifies:
 * - PopulationData size and default values
 * - EmploymentData size and default values
 * - BuildingOccupancyComponent size and OccupancyState enum values
 * - MigrationFactors size and default values
 */

#include "sims3000/population/PopulationData.h"
#include "sims3000/population/EmploymentData.h"
#include "sims3000/population/BuildingOccupancyComponent.h"
#include "sims3000/population/MigrationFactors.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <type_traits>

using namespace sims3000::population;

// =========================================================================
// Test: PopulationData size and defaults (E10-010)
// =========================================================================

void test_population_data_size() {
    printf("Testing PopulationData size...\n");

    // Target approximately 90 bytes
    printf("  PopulationData sizeof = %zu bytes\n", sizeof(PopulationData));
    assert(sizeof(PopulationData) <= 96);

    printf("  PASS: PopulationData size within target\n");
}

void test_population_data_defaults() {
    printf("Testing PopulationData default values...\n");

    PopulationData pd;

    assert(pd.total_beings == 0);
    assert(pd.max_capacity == 0);

    // Age distribution sums to 100
    assert(pd.youth_percent == 33);
    assert(pd.adult_percent == 34);
    assert(pd.elder_percent == 33);
    assert(pd.youth_percent + pd.adult_percent + pd.elder_percent == 100);

    // Demographic rates
    assert(pd.birth_rate_per_1000 == 15);
    assert(pd.death_rate_per_1000 == 8);

    // Derived values start at zero
    assert(pd.natural_growth == 0);
    assert(pd.net_migration == 0);
    assert(pd.growth_rate == 0.0f);

    // Quality indices default to 50 (neutral)
    assert(pd.harmony_index == 50);
    assert(pd.health_index == 50);
    assert(pd.education_index == 50);

    // History buffer initialized to zeros
    for (int i = 0; i < 12; i++) {
        assert(pd.population_history[i] == 0);
    }
    assert(pd.history_index == 0);

    printf("  PASS: PopulationData defaults correct\n");
}

// =========================================================================
// Test: EmploymentData size and defaults (E10-011)
// =========================================================================

void test_employment_data_size() {
    printf("Testing EmploymentData size...\n");

    // Target approximately 45 bytes
    printf("  EmploymentData sizeof = %zu bytes\n", sizeof(EmploymentData));
    assert(sizeof(EmploymentData) <= 52);

    printf("  PASS: EmploymentData size within target\n");
}

void test_employment_data_defaults() {
    printf("Testing EmploymentData default values...\n");

    EmploymentData ed;

    assert(ed.working_age_beings == 0);
    assert(ed.labor_force == 0);
    assert(ed.employed_laborers == 0);
    assert(ed.unemployed == 0);
    assert(ed.total_jobs == 0);
    assert(ed.exchange_jobs == 0);
    assert(ed.fabrication_jobs == 0);
    assert(ed.unemployment_rate == 0);
    assert(ed.labor_participation == 65);
    assert(ed.exchange_employed == 0);
    assert(ed.fabrication_employed == 0);
    assert(ed.avg_commute_satisfaction == 50);

    printf("  PASS: EmploymentData defaults correct\n");
}

// =========================================================================
// Test: BuildingOccupancyComponent size and defaults (E10-012)
// =========================================================================

void test_building_occupancy_size() {
    printf("Testing BuildingOccupancyComponent size...\n");

    // Target approximately 9 bytes (may be larger due to alignment)
    printf("  BuildingOccupancyComponent sizeof = %zu bytes\n", sizeof(BuildingOccupancyComponent));
    assert(sizeof(BuildingOccupancyComponent) <= 12);

    printf("  PASS: BuildingOccupancyComponent size within target\n");
}

void test_building_occupancy_defaults() {
    printf("Testing BuildingOccupancyComponent default values...\n");

    BuildingOccupancyComponent boc;

    assert(boc.capacity == 0);
    assert(boc.current_occupancy == 0);
    assert(boc.state == OccupancyState::Empty);
    assert(boc.occupancy_changed_tick == 0);

    printf("  PASS: BuildingOccupancyComponent defaults correct\n");
}

void test_occupancy_state_enum() {
    printf("Testing OccupancyState enum values...\n");

    assert(static_cast<uint8_t>(OccupancyState::Empty) == 0);
    assert(static_cast<uint8_t>(OccupancyState::UnderOccupied) == 1);
    assert(static_cast<uint8_t>(OccupancyState::NormalOccupied) == 2);
    assert(static_cast<uint8_t>(OccupancyState::FullyOccupied) == 3);
    assert(static_cast<uint8_t>(OccupancyState::Overcrowded) == 4);

    // Verify enum is uint8_t sized
    assert(sizeof(OccupancyState) == 1);

    printf("  PASS: OccupancyState enum values correct\n");
}

// =========================================================================
// Test: MigrationFactors size and defaults (E10-013)
// =========================================================================

void test_migration_factors_size() {
    printf("Testing MigrationFactors size...\n");

    // Target approximately 12 bytes
    printf("  MigrationFactors sizeof = %zu bytes\n", sizeof(MigrationFactors));
    assert(sizeof(MigrationFactors) <= 12);

    printf("  PASS: MigrationFactors size within target\n");
}

void test_migration_factors_defaults() {
    printf("Testing MigrationFactors default values...\n");

    MigrationFactors mf;

    // Positive factors default to 50 (neutral)
    assert(mf.job_availability == 50);
    assert(mf.housing_availability == 50);
    assert(mf.sector_value_avg == 50);
    assert(mf.service_coverage == 50);
    assert(mf.harmony_level == 50);

    // Negative factors default to 0 (no problems)
    assert(mf.disorder_level == 0);
    assert(mf.contamination_level == 0);
    assert(mf.tribute_burden == 0);
    assert(mf.congestion_level == 0);

    // Computed values default to 0 (neutral)
    assert(mf.net_attraction == 0);
    assert(mf.migration_pressure == 0);

    printf("  PASS: MigrationFactors defaults correct\n");
}

// =========================================================================
// Test: Components are trivially copyable
// =========================================================================

void test_trivially_copyable() {
    printf("Testing components are trivially copyable...\n");

    // All data components should be trivially copyable for ECS performance
    static_assert(std::is_trivially_copyable<PopulationData>::value,
                  "PopulationData must be trivially copyable");
    static_assert(std::is_trivially_copyable<EmploymentData>::value,
                  "EmploymentData must be trivially copyable");
    static_assert(std::is_trivially_copyable<BuildingOccupancyComponent>::value,
                  "BuildingOccupancyComponent must be trivially copyable");
    static_assert(std::is_trivially_copyable<MigrationFactors>::value,
                  "MigrationFactors must be trivially copyable");

    printf("  PASS: All components are trivially copyable\n");
}

// =========================================================================
// Main
// =========================================================================

int main() {
    printf("=== Population Component Tests (E10-010 through E10-013) ===\n\n");

    // E10-010: PopulationData
    test_population_data_size();
    test_population_data_defaults();

    // E10-011: EmploymentData
    test_employment_data_size();
    test_employment_data_defaults();

    // E10-012: BuildingOccupancyComponent
    test_building_occupancy_size();
    test_building_occupancy_defaults();
    test_occupancy_state_enum();

    // E10-013: MigrationFactors
    test_migration_factors_size();
    test_migration_factors_defaults();

    // Cross-cutting: trivially copyable
    test_trivially_copyable();

    printf("\n=== All Population Component tests passed ===\n");
    return 0;
}
