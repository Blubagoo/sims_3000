/**
 * @file test_industrial_contamination.cpp
 * @brief Unit tests for industrial contamination generation (Ticket E10-083).
 *
 * Tests cover:
 * - Level 1/2/3 output values (50/100/200)
 * - Occupancy scaling
 * - Inactive source produces 0
 */

#include <sims3000/contamination/IndustrialContamination.h>
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
// Level 1/2/3 output values at full occupancy
// =============================================================================

TEST(level_1_full_occupancy_output_50) {
    ContaminationGrid grid(64, 64);
    std::vector<IndustrialSource> sources = {
        { 10, 10, 1, 1.0f, true }
    };

    apply_industrial_contamination(grid, sources);

    ASSERT_EQ(grid.get_level(10, 10), 50);
}

TEST(level_2_full_occupancy_output_100) {
    ContaminationGrid grid(64, 64);
    std::vector<IndustrialSource> sources = {
        { 10, 10, 2, 1.0f, true }
    };

    apply_industrial_contamination(grid, sources);

    ASSERT_EQ(grid.get_level(10, 10), 100);
}

TEST(level_3_full_occupancy_output_200) {
    ContaminationGrid grid(64, 64);
    std::vector<IndustrialSource> sources = {
        { 10, 10, 3, 1.0f, true }
    };

    apply_industrial_contamination(grid, sources);

    ASSERT_EQ(grid.get_level(10, 10), 200);
}

// =============================================================================
// Contamination type is Industrial
// =============================================================================

TEST(contamination_type_is_industrial) {
    ContaminationGrid grid(64, 64);
    std::vector<IndustrialSource> sources = {
        { 10, 10, 1, 1.0f, true }
    };

    apply_industrial_contamination(grid, sources);

    ASSERT_EQ(grid.get_dominant_type(10, 10),
              static_cast<uint8_t>(ContaminationType::Industrial));
}

// =============================================================================
// Occupancy scaling
// =============================================================================

TEST(half_occupancy_halves_output) {
    ContaminationGrid grid(64, 64);
    std::vector<IndustrialSource> sources = {
        { 10, 10, 2, 0.5f, true }  // 100 * 0.5 = 50
    };

    apply_industrial_contamination(grid, sources);

    ASSERT_EQ(grid.get_level(10, 10), 50);
}

TEST(zero_occupancy_zero_output) {
    ContaminationGrid grid(64, 64);
    std::vector<IndustrialSource> sources = {
        { 10, 10, 3, 0.0f, true }  // 200 * 0.0 = 0
    };

    apply_industrial_contamination(grid, sources);

    ASSERT_EQ(grid.get_level(10, 10), 0);
}

TEST(quarter_occupancy_scales_output) {
    ContaminationGrid grid(64, 64);
    std::vector<IndustrialSource> sources = {
        { 10, 10, 2, 0.25f, true }  // 100 * 0.25 = 25
    };

    apply_industrial_contamination(grid, sources);

    ASSERT_EQ(grid.get_level(10, 10), 25);
}

// =============================================================================
// Inactive source produces 0
// =============================================================================

TEST(inactive_source_produces_zero) {
    ContaminationGrid grid(64, 64);
    std::vector<IndustrialSource> sources = {
        { 10, 10, 3, 1.0f, false }  // inactive
    };

    apply_industrial_contamination(grid, sources);

    ASSERT_EQ(grid.get_level(10, 10), 0);
}

TEST(mixed_active_inactive_sources) {
    ContaminationGrid grid(64, 64);
    std::vector<IndustrialSource> sources = {
        { 10, 10, 1, 1.0f, true },   // active -> 50
        { 20, 20, 2, 1.0f, false },  // inactive -> 0
        { 30, 30, 3, 1.0f, true }    // active -> 200
    };

    apply_industrial_contamination(grid, sources);

    ASSERT_EQ(grid.get_level(10, 10), 50);
    ASSERT_EQ(grid.get_level(20, 20), 0);
    ASSERT_EQ(grid.get_level(30, 30), 200);
}

// =============================================================================
// Edge cases
// =============================================================================

TEST(empty_sources_no_changes) {
    ContaminationGrid grid(64, 64);
    std::vector<IndustrialSource> sources;

    apply_industrial_contamination(grid, sources);

    ASSERT_EQ(grid.get_level(0, 0), 0);
    ASSERT_EQ(grid.get_level(32, 32), 0);
}

TEST(base_output_constants_correct) {
    ASSERT_EQ(INDUSTRIAL_BASE_OUTPUT[0], 50);
    ASSERT_EQ(INDUSTRIAL_BASE_OUTPUT[1], 100);
    ASSERT_EQ(INDUSTRIAL_BASE_OUTPUT[2], 200);
}

// =============================================================================
// Main
// =============================================================================

int main() {
    printf("=== Industrial Contamination Tests (E10-083) ===\n\n");

    RUN_TEST(level_1_full_occupancy_output_50);
    RUN_TEST(level_2_full_occupancy_output_100);
    RUN_TEST(level_3_full_occupancy_output_200);
    RUN_TEST(contamination_type_is_industrial);
    RUN_TEST(half_occupancy_halves_output);
    RUN_TEST(zero_occupancy_zero_output);
    RUN_TEST(quarter_occupancy_scales_output);
    RUN_TEST(inactive_source_produces_zero);
    RUN_TEST(mixed_active_inactive_sources);
    RUN_TEST(empty_sources_no_changes);
    RUN_TEST(base_output_constants_correct);

    printf("\n=== Results: %d passed, %d failed ===\n",
           tests_passed, tests_failed);

    return tests_failed > 0 ? 1 : 0;
}
