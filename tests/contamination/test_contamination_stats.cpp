/**
 * @file test_contamination_stats.cpp
 * @brief Unit tests for ContaminationStats (Ticket E10-089)
 *
 * Tests cover:
 * - Total contamination stat
 * - Average contamination stat
 * - Toxic tiles stat
 * - Max contamination stat
 * - Type breakdown stats (Industrial, Traffic, Energy, Terrain)
 * - Stat name retrieval
 * - Stat ID validation
 * - get_contamination_at helper
 */

#include <sims3000/contamination/ContaminationStats.h>
#include <sims3000/contamination/ContaminationGrid.h>
#include <sims3000/contamination/ContaminationType.h>
#include <cstdio>
#include <cstdlib>
#include <cmath>

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
        printf("\n  FAILED: %s == %s (line %d)\n", #a, #b, __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

#define ASSERT_FLOAT_EQ(a, b, epsilon) do { \
    if (std::fabs((a) - (b)) > (epsilon)) { \
        printf("\n  FAILED: %s ~= %s (line %d) [%.2f vs %.2f]\n", #a, #b, __LINE__, (float)(a), (float)(b)); \
        tests_failed++; \
        return; \
    } \
} while(0)

// =============================================================================
// Total Contamination Tests
// =============================================================================

TEST(total_contamination_empty_grid) {
    ContaminationGrid grid(64, 64);
    grid.update_stats();
    float stat = get_contamination_stat(grid, STAT_TOTAL_CONTAMINATION);
    ASSERT_FLOAT_EQ(stat, 0.0f, 0.01f);
}

TEST(total_contamination_single_cell) {
    ContaminationGrid grid(64, 64);
    grid.add_contamination(10, 10, 100, 0);
    grid.update_stats();
    float stat = get_contamination_stat(grid, STAT_TOTAL_CONTAMINATION);
    ASSERT_FLOAT_EQ(stat, 100.0f, 0.01f);
}

TEST(total_contamination_multiple_cells) {
    ContaminationGrid grid(64, 64);
    grid.add_contamination(0, 0, 50, 0);
    grid.add_contamination(1, 0, 75, 1);
    grid.add_contamination(2, 0, 25, 2);
    grid.update_stats();
    float stat = get_contamination_stat(grid, STAT_TOTAL_CONTAMINATION);
    ASSERT_FLOAT_EQ(stat, 150.0f, 0.01f);
}

// =============================================================================
// Average Contamination Tests
// =============================================================================

TEST(average_contamination_empty_grid) {
    ContaminationGrid grid(64, 64);
    grid.update_stats();
    float stat = get_contamination_stat(grid, STAT_AVERAGE_CONTAMINATION);
    ASSERT_FLOAT_EQ(stat, 0.0f, 0.01f);
}

TEST(average_contamination_single_cell) {
    ContaminationGrid grid(64, 64);
    grid.add_contamination(10, 10, 128, 0);
    grid.update_stats();
    float stat = get_contamination_stat(grid, STAT_AVERAGE_CONTAMINATION);
    // 128 / (64 * 64) = 128 / 4096 = 0.03125
    ASSERT_FLOAT_EQ(stat, 0.03125f, 0.001f);
}

TEST(average_contamination_multiple_cells) {
    ContaminationGrid grid(4, 4);
    // Total cells: 16
    grid.add_contamination(0, 0, 16, 0);
    grid.add_contamination(1, 0, 32, 1);
    grid.update_stats();
    float stat = get_contamination_stat(grid, STAT_AVERAGE_CONTAMINATION);
    // (16 + 32) / 16 = 48 / 16 = 3.0
    ASSERT_FLOAT_EQ(stat, 3.0f, 0.01f);
}

// =============================================================================
// Toxic Tiles Tests
// =============================================================================

TEST(toxic_tiles_empty_grid) {
    ContaminationGrid grid(64, 64);
    grid.update_stats();
    float stat = get_contamination_stat(grid, STAT_TOXIC_TILES);
    ASSERT_FLOAT_EQ(stat, 0.0f, 0.01f);
}

TEST(toxic_tiles_below_threshold) {
    ContaminationGrid grid(64, 64);
    grid.add_contamination(0, 0, 127, 0);
    grid.add_contamination(1, 0, 100, 1);
    grid.update_stats();
    float stat = get_contamination_stat(grid, STAT_TOXIC_TILES);
    ASSERT_FLOAT_EQ(stat, 0.0f, 0.01f);
}

TEST(toxic_tiles_at_threshold) {
    ContaminationGrid grid(64, 64);
    grid.add_contamination(0, 0, 128, 0);
    grid.update_stats();
    float stat = get_contamination_stat(grid, STAT_TOXIC_TILES);
    ASSERT_FLOAT_EQ(stat, 1.0f, 0.01f);
}

TEST(toxic_tiles_mixed) {
    ContaminationGrid grid(64, 64);
    grid.add_contamination(0, 0, 127, 0);  // below
    grid.add_contamination(1, 0, 128, 1);  // at
    grid.add_contamination(2, 0, 200, 2);  // above
    grid.add_contamination(3, 0, 255, 3);  // max
    grid.update_stats();
    float stat = get_contamination_stat(grid, STAT_TOXIC_TILES);
    ASSERT_FLOAT_EQ(stat, 3.0f, 0.01f);
}

// =============================================================================
// Max Contamination Tests
// =============================================================================

TEST(max_contamination_empty_grid) {
    ContaminationGrid grid(64, 64);
    float stat = get_contamination_stat(grid, STAT_MAX_CONTAMINATION);
    ASSERT_FLOAT_EQ(stat, 0.0f, 0.01f);
}

TEST(max_contamination_single_cell) {
    ContaminationGrid grid(64, 64);
    grid.add_contamination(10, 10, 200, 0);
    float stat = get_contamination_stat(grid, STAT_MAX_CONTAMINATION);
    ASSERT_FLOAT_EQ(stat, 200.0f, 0.01f);
}

TEST(max_contamination_multiple_cells) {
    ContaminationGrid grid(64, 64);
    grid.add_contamination(0, 0, 100, 0);
    grid.add_contamination(1, 0, 250, 1);
    grid.add_contamination(2, 0, 150, 2);
    float stat = get_contamination_stat(grid, STAT_MAX_CONTAMINATION);
    ASSERT_FLOAT_EQ(stat, 250.0f, 0.01f);
}

TEST(max_contamination_255) {
    ContaminationGrid grid(64, 64);
    grid.add_contamination(10, 10, 255, 0);
    float stat = get_contamination_stat(grid, STAT_MAX_CONTAMINATION);
    ASSERT_FLOAT_EQ(stat, 255.0f, 0.01f);
}

// =============================================================================
// Type Breakdown Tests
// =============================================================================

TEST(industrial_total_empty_grid) {
    ContaminationGrid grid(64, 64);
    float stat = get_contamination_stat(grid, STAT_INDUSTRIAL_TOTAL);
    ASSERT_FLOAT_EQ(stat, 0.0f, 0.01f);
}

TEST(industrial_total_single_cell) {
    ContaminationGrid grid(64, 64);
    grid.add_contamination(10, 10, 100, static_cast<uint8_t>(ContaminationType::Industrial));
    float stat = get_contamination_stat(grid, STAT_INDUSTRIAL_TOTAL);
    ASSERT_FLOAT_EQ(stat, 1.0f, 0.01f);
}

TEST(industrial_total_multiple_cells) {
    ContaminationGrid grid(64, 64);
    grid.add_contamination(0, 0, 100, static_cast<uint8_t>(ContaminationType::Industrial));
    grid.add_contamination(1, 0, 50, static_cast<uint8_t>(ContaminationType::Industrial));
    grid.add_contamination(2, 0, 200, static_cast<uint8_t>(ContaminationType::Industrial));
    float stat = get_contamination_stat(grid, STAT_INDUSTRIAL_TOTAL);
    ASSERT_FLOAT_EQ(stat, 3.0f, 0.01f);
}

TEST(industrial_total_excludes_zero_level) {
    ContaminationGrid grid(64, 64);
    grid.add_contamination(0, 0, 100, static_cast<uint8_t>(ContaminationType::Industrial));
    grid.add_contamination(1, 0, 50, static_cast<uint8_t>(ContaminationType::Industrial));
    grid.apply_decay(1, 0, 50);  // Reduce to zero
    float stat = get_contamination_stat(grid, STAT_INDUSTRIAL_TOTAL);
    ASSERT_FLOAT_EQ(stat, 1.0f, 0.01f);  // Only one cell with level > 0
}

TEST(traffic_total_single_cell) {
    ContaminationGrid grid(64, 64);
    grid.add_contamination(10, 10, 100, static_cast<uint8_t>(ContaminationType::Traffic));
    float stat = get_contamination_stat(grid, STAT_TRAFFIC_TOTAL);
    ASSERT_FLOAT_EQ(stat, 1.0f, 0.01f);
}

TEST(energy_total_single_cell) {
    ContaminationGrid grid(64, 64);
    grid.add_contamination(10, 10, 100, static_cast<uint8_t>(ContaminationType::Energy));
    float stat = get_contamination_stat(grid, STAT_ENERGY_TOTAL);
    ASSERT_FLOAT_EQ(stat, 1.0f, 0.01f);
}

TEST(terrain_total_single_cell) {
    ContaminationGrid grid(64, 64);
    grid.add_contamination(10, 10, 100, static_cast<uint8_t>(ContaminationType::Terrain));
    float stat = get_contamination_stat(grid, STAT_TERRAIN_TOTAL);
    ASSERT_FLOAT_EQ(stat, 1.0f, 0.01f);
}

TEST(type_breakdown_mixed) {
    ContaminationGrid grid(64, 64);
    grid.add_contamination(0, 0, 100, static_cast<uint8_t>(ContaminationType::Industrial));
    grid.add_contamination(1, 0, 100, static_cast<uint8_t>(ContaminationType::Industrial));
    grid.add_contamination(2, 0, 100, static_cast<uint8_t>(ContaminationType::Traffic));
    grid.add_contamination(3, 0, 100, static_cast<uint8_t>(ContaminationType::Energy));
    grid.add_contamination(4, 0, 100, static_cast<uint8_t>(ContaminationType::Terrain));

    ASSERT_FLOAT_EQ(get_contamination_stat(grid, STAT_INDUSTRIAL_TOTAL), 2.0f, 0.01f);
    ASSERT_FLOAT_EQ(get_contamination_stat(grid, STAT_TRAFFIC_TOTAL), 1.0f, 0.01f);
    ASSERT_FLOAT_EQ(get_contamination_stat(grid, STAT_ENERGY_TOTAL), 1.0f, 0.01f);
    ASSERT_FLOAT_EQ(get_contamination_stat(grid, STAT_TERRAIN_TOTAL), 1.0f, 0.01f);
}

TEST(type_breakdown_dominant_type_update) {
    ContaminationGrid grid(64, 64);
    // Start with Industrial
    grid.add_contamination(0, 0, 50, static_cast<uint8_t>(ContaminationType::Industrial));
    // Add more Traffic - should become dominant
    grid.add_contamination(0, 0, 100, static_cast<uint8_t>(ContaminationType::Traffic));

    ASSERT_FLOAT_EQ(get_contamination_stat(grid, STAT_INDUSTRIAL_TOTAL), 0.0f, 0.01f);
    ASSERT_FLOAT_EQ(get_contamination_stat(grid, STAT_TRAFFIC_TOTAL), 1.0f, 0.01f);
}

// =============================================================================
// Stat Name Tests
// =============================================================================

TEST(stat_name_total_contamination) {
    const char* name = get_contamination_stat_name(STAT_TOTAL_CONTAMINATION);
    ASSERT(name != nullptr);
}

TEST(stat_name_industrial) {
    const char* name = get_contamination_stat_name(STAT_INDUSTRIAL_TOTAL);
    ASSERT(name != nullptr);
}

TEST(stat_name_invalid) {
    const char* name = get_contamination_stat_name(999);
    ASSERT(name != nullptr);
}

// =============================================================================
// Stat Validation Tests
// =============================================================================

TEST(is_valid_stat_total) {
    ASSERT(is_valid_contamination_stat(STAT_TOTAL_CONTAMINATION));
}

TEST(is_valid_stat_average) {
    ASSERT(is_valid_contamination_stat(STAT_AVERAGE_CONTAMINATION));
}

TEST(is_valid_stat_toxic) {
    ASSERT(is_valid_contamination_stat(STAT_TOXIC_TILES));
}

TEST(is_valid_stat_max) {
    ASSERT(is_valid_contamination_stat(STAT_MAX_CONTAMINATION));
}

TEST(is_valid_stat_industrial) {
    ASSERT(is_valid_contamination_stat(STAT_INDUSTRIAL_TOTAL));
}

TEST(is_valid_stat_traffic) {
    ASSERT(is_valid_contamination_stat(STAT_TRAFFIC_TOTAL));
}

TEST(is_valid_stat_energy) {
    ASSERT(is_valid_contamination_stat(STAT_ENERGY_TOTAL));
}

TEST(is_valid_stat_terrain) {
    ASSERT(is_valid_contamination_stat(STAT_TERRAIN_TOTAL));
}

TEST(is_valid_stat_invalid_below) {
    ASSERT(!is_valid_contamination_stat(499));
}

TEST(is_valid_stat_invalid_above) {
    ASSERT(!is_valid_contamination_stat(508));
}

// =============================================================================
// get_contamination_at Tests
// =============================================================================

TEST(get_contamination_at_zero) {
    ContaminationGrid grid(64, 64);
    uint8_t level = get_contamination_at(grid, 10, 10);
    ASSERT_EQ(level, 0);
}

TEST(get_contamination_at_nonzero) {
    ContaminationGrid grid(64, 64);
    grid.add_contamination(10, 10, 123, 0);
    uint8_t level = get_contamination_at(grid, 10, 10);
    ASSERT_EQ(level, 123);
}

TEST(get_contamination_at_out_of_bounds) {
    ContaminationGrid grid(64, 64);
    grid.add_contamination(10, 10, 100, 0);
    uint8_t level = get_contamination_at(grid, 64, 64);
    ASSERT_EQ(level, 0);
}

// =============================================================================
// Invalid Stat ID Tests
// =============================================================================

TEST(invalid_stat_returns_zero) {
    ContaminationGrid grid(64, 64);
    grid.add_contamination(10, 10, 100, 0);
    float stat = get_contamination_stat(grid, 999);
    ASSERT_FLOAT_EQ(stat, 0.0f, 0.01f);
}

// =============================================================================
// Main Entry Point
// =============================================================================

int main() {
    printf("=== ContaminationStats Unit Tests (Ticket E10-089) ===\n\n");

    // Total contamination tests
    RUN_TEST(total_contamination_empty_grid);
    RUN_TEST(total_contamination_single_cell);
    RUN_TEST(total_contamination_multiple_cells);

    // Average contamination tests
    RUN_TEST(average_contamination_empty_grid);
    RUN_TEST(average_contamination_single_cell);
    RUN_TEST(average_contamination_multiple_cells);

    // Toxic tiles tests
    RUN_TEST(toxic_tiles_empty_grid);
    RUN_TEST(toxic_tiles_below_threshold);
    RUN_TEST(toxic_tiles_at_threshold);
    RUN_TEST(toxic_tiles_mixed);

    // Max contamination tests
    RUN_TEST(max_contamination_empty_grid);
    RUN_TEST(max_contamination_single_cell);
    RUN_TEST(max_contamination_multiple_cells);
    RUN_TEST(max_contamination_255);

    // Type breakdown tests
    RUN_TEST(industrial_total_empty_grid);
    RUN_TEST(industrial_total_single_cell);
    RUN_TEST(industrial_total_multiple_cells);
    RUN_TEST(industrial_total_excludes_zero_level);
    RUN_TEST(traffic_total_single_cell);
    RUN_TEST(energy_total_single_cell);
    RUN_TEST(terrain_total_single_cell);
    RUN_TEST(type_breakdown_mixed);
    RUN_TEST(type_breakdown_dominant_type_update);

    // Stat name tests
    RUN_TEST(stat_name_total_contamination);
    RUN_TEST(stat_name_industrial);
    RUN_TEST(stat_name_invalid);

    // Stat validation tests
    RUN_TEST(is_valid_stat_total);
    RUN_TEST(is_valid_stat_average);
    RUN_TEST(is_valid_stat_toxic);
    RUN_TEST(is_valid_stat_max);
    RUN_TEST(is_valid_stat_industrial);
    RUN_TEST(is_valid_stat_traffic);
    RUN_TEST(is_valid_stat_energy);
    RUN_TEST(is_valid_stat_terrain);
    RUN_TEST(is_valid_stat_invalid_below);
    RUN_TEST(is_valid_stat_invalid_above);

    // get_contamination_at tests
    RUN_TEST(get_contamination_at_zero);
    RUN_TEST(get_contamination_at_nonzero);
    RUN_TEST(get_contamination_at_out_of_bounds);

    // Invalid stat tests
    RUN_TEST(invalid_stat_returns_zero);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
