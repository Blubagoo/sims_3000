/**
 * @file test_energy_contamination.cpp
 * @brief Unit tests for energy contamination generation (Ticket E10-084).
 *
 * Tests cover:
 * - Carbon=200, Petrochem=120, Gaseous=40
 * - Clean energy (type>=3) produces 0
 * - Inactive source produces 0
 */

#include <sims3000/contamination/EnergyContamination.h>
#include <sims3000/contamination/ContaminationGrid.h>
#include <sims3000/contamination/ContaminationType.h>
#include <cstdio>
#include <cstdlib>
#include <vector>

using namespace sims3000::contamination;

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

// =============================================================================
// Nexus type output values
// =============================================================================

TEST(carbon_output_200) {
    ContaminationGrid grid(64, 64);
    std::vector<EnergySource> sources = {
        { 10, 10, 0, true }  // carbon
    };

    apply_energy_contamination(grid, sources);

    ASSERT_EQ(grid.get_level(10, 10), 200);
}

TEST(petrochem_output_120) {
    ContaminationGrid grid(64, 64);
    std::vector<EnergySource> sources = {
        { 10, 10, 1, true }  // petrochem
    };

    apply_energy_contamination(grid, sources);

    ASSERT_EQ(grid.get_level(10, 10), 120);
}

TEST(gaseous_output_40) {
    ContaminationGrid grid(64, 64);
    std::vector<EnergySource> sources = {
        { 10, 10, 2, true }  // gaseous
    };

    apply_energy_contamination(grid, sources);

    ASSERT_EQ(grid.get_level(10, 10), 40);
}

// =============================================================================
// Contamination type is Energy
// =============================================================================

TEST(contamination_type_is_energy) {
    ContaminationGrid grid(64, 64);
    std::vector<EnergySource> sources = {
        { 10, 10, 0, true }
    };

    apply_energy_contamination(grid, sources);

    ASSERT_EQ(grid.get_dominant_type(10, 10),
              static_cast<uint8_t>(ContaminationType::Energy));
}

// =============================================================================
// Clean energy produces 0
// =============================================================================

TEST(clean_energy_type_3_produces_zero) {
    ContaminationGrid grid(64, 64);
    std::vector<EnergySource> sources = {
        { 10, 10, 3, true }  // clean
    };

    apply_energy_contamination(grid, sources);

    ASSERT_EQ(grid.get_level(10, 10), 0);
}

TEST(clean_energy_type_4_produces_zero) {
    ContaminationGrid grid(64, 64);
    std::vector<EnergySource> sources = {
        { 10, 10, 4, true }  // clean (higher type)
    };

    apply_energy_contamination(grid, sources);

    ASSERT_EQ(grid.get_level(10, 10), 0);
}

TEST(clean_energy_type_255_produces_zero) {
    ContaminationGrid grid(64, 64);
    std::vector<EnergySource> sources = {
        { 10, 10, 255, true }  // clean (max type)
    };

    apply_energy_contamination(grid, sources);

    ASSERT_EQ(grid.get_level(10, 10), 0);
}

// =============================================================================
// Inactive source produces 0
// =============================================================================

TEST(inactive_source_produces_zero) {
    ContaminationGrid grid(64, 64);
    std::vector<EnergySource> sources = {
        { 10, 10, 0, false }  // carbon but inactive
    };

    apply_energy_contamination(grid, sources);

    ASSERT_EQ(grid.get_level(10, 10), 0);
}

TEST(inactive_dirty_source_produces_zero) {
    ContaminationGrid grid(64, 64);
    std::vector<EnergySource> sources = {
        { 10, 10, 0, false },  // carbon inactive
        { 20, 20, 1, false },  // petrochem inactive
        { 30, 30, 2, false }   // gaseous inactive
    };

    apply_energy_contamination(grid, sources);

    ASSERT_EQ(grid.get_level(10, 10), 0);
    ASSERT_EQ(grid.get_level(20, 20), 0);
    ASSERT_EQ(grid.get_level(30, 30), 0);
}

// =============================================================================
// Mixed sources
// =============================================================================

TEST(mixed_active_and_clean_sources) {
    ContaminationGrid grid(64, 64);
    std::vector<EnergySource> sources = {
        { 10, 10, 0, true },   // carbon -> 200
        { 20, 20, 3, true },   // clean -> 0
        { 30, 30, 1, true }    // petrochem -> 120
    };

    apply_energy_contamination(grid, sources);

    ASSERT_EQ(grid.get_level(10, 10), 200);
    ASSERT_EQ(grid.get_level(20, 20), 0);
    ASSERT_EQ(grid.get_level(30, 30), 120);
}

// =============================================================================
// Edge cases
// =============================================================================

TEST(empty_sources_no_changes) {
    ContaminationGrid grid(64, 64);
    std::vector<EnergySource> sources;

    apply_energy_contamination(grid, sources);

    ASSERT_EQ(grid.get_level(0, 0), 0);
    ASSERT_EQ(grid.get_level(32, 32), 0);
}

TEST(output_constants_correct) {
    ASSERT_EQ(ENERGY_CONTAMINATION_OUTPUT[0], 200);
    ASSERT_EQ(ENERGY_CONTAMINATION_OUTPUT[1], 120);
    ASSERT_EQ(ENERGY_CONTAMINATION_OUTPUT[2], 40);
}

// =============================================================================
// Main
// =============================================================================

int main() {
    printf("=== Energy Contamination Tests (E10-084) ===\n\n");

    RUN_TEST(carbon_output_200);
    RUN_TEST(petrochem_output_120);
    RUN_TEST(gaseous_output_40);
    RUN_TEST(contamination_type_is_energy);
    RUN_TEST(clean_energy_type_3_produces_zero);
    RUN_TEST(clean_energy_type_4_produces_zero);
    RUN_TEST(clean_energy_type_255_produces_zero);
    RUN_TEST(inactive_source_produces_zero);
    RUN_TEST(inactive_dirty_source_produces_zero);
    RUN_TEST(mixed_active_and_clean_sources);
    RUN_TEST(empty_sources_no_changes);
    RUN_TEST(output_constants_correct);

    printf("\n=== Results: %d passed, %d failed ===\n",
           tests_passed, tests_failed);

    return tests_failed > 0 ? 1 : 0;
}
