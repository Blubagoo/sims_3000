/**
 * @file test_contamination_overlay.cpp
 * @brief Unit tests for ContaminationOverlay (Ticket E10-090)
 *
 * Tests cover:
 * - getName() returns correct name
 * - getColorAt() for various contamination levels
 * - isActive() returns true
 * - Out-of-bounds coordinates
 * - Color mapping thresholds
 */

#include <sims3000/contamination/ContaminationOverlay.h>
#include <sims3000/contamination/ContaminationGrid.h>
#include <sims3000/services/IGridOverlay.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

using namespace sims3000::contamination;
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

// =============================================================================
// getName() Tests
// =============================================================================

TEST(get_name_returns_contamination) {
    ContaminationGrid grid(64, 64);
    ContaminationOverlay overlay(grid);
    const char* name = overlay.getName();
    ASSERT(name != nullptr);
    ASSERT(std::strcmp(name, "Contamination") == 0);
}

// =============================================================================
// isActive() Tests
// =============================================================================

TEST(is_active_returns_true) {
    ContaminationGrid grid(64, 64);
    ContaminationOverlay overlay(grid);
    ASSERT(overlay.isActive());
}

// =============================================================================
// getColorAt() Basic Tests
// =============================================================================

TEST(get_color_at_zero_contamination) {
    ContaminationGrid grid(64, 64);
    ContaminationOverlay overlay(grid);
    OverlayColor color = overlay.getColorAt(10, 10);
    ASSERT_EQ(color.r, 0);
    ASSERT_EQ(color.g, 0);
    ASSERT_EQ(color.b, 0);
    ASSERT_EQ(color.a, 0);
}

TEST(get_color_at_low_contamination) {
    ContaminationGrid grid(64, 64);
    grid.add_contamination(10, 10, 32, 0);
    ContaminationOverlay overlay(grid);
    OverlayColor color = overlay.getColorAt(10, 10);
    // Should be green
    ASSERT_EQ(color.r, 0);
    ASSERT_EQ(color.g, 255);
    ASSERT_EQ(color.b, 0);
    ASSERT(color.a > 0);
}

TEST(get_color_at_medium_contamination) {
    ContaminationGrid grid(64, 64);
    grid.add_contamination(10, 10, 96, 0);
    ContaminationOverlay overlay(grid);
    OverlayColor color = overlay.getColorAt(10, 10);
    // Should be yellow
    ASSERT_EQ(color.r, 255);
    ASSERT_EQ(color.g, 255);
    ASSERT_EQ(color.b, 0);
    ASSERT(color.a > 0);
}

TEST(get_color_at_high_contamination) {
    ContaminationGrid grid(64, 64);
    grid.add_contamination(10, 10, 160, 0);
    ContaminationOverlay overlay(grid);
    OverlayColor color = overlay.getColorAt(10, 10);
    // Should be orange
    ASSERT_EQ(color.r, 255);
    ASSERT_EQ(color.g, 165);
    ASSERT_EQ(color.b, 0);
    ASSERT(color.a > 0);
}

TEST(get_color_at_toxic_contamination) {
    ContaminationGrid grid(64, 64);
    grid.add_contamination(10, 10, 200, 0);
    ContaminationOverlay overlay(grid);
    OverlayColor color = overlay.getColorAt(10, 10);
    // Should be red
    ASSERT_EQ(color.r, 255);
    ASSERT_EQ(color.g, 0);
    ASSERT_EQ(color.b, 0);
    ASSERT(color.a > 0);
}

TEST(get_color_at_max_contamination) {
    ContaminationGrid grid(64, 64);
    grid.add_contamination(10, 10, 255, 0);
    ContaminationOverlay overlay(grid);
    OverlayColor color = overlay.getColorAt(10, 10);
    // Should be red
    ASSERT_EQ(color.r, 255);
    ASSERT_EQ(color.g, 0);
    ASSERT_EQ(color.b, 0);
    ASSERT(color.a > 0);
}

// =============================================================================
// Color Threshold Tests
// =============================================================================

TEST(color_threshold_level_1) {
    ContaminationGrid grid(64, 64);
    grid.add_contamination(10, 10, 1, 0);
    ContaminationOverlay overlay(grid);
    OverlayColor color = overlay.getColorAt(10, 10);
    // Should be green (level < 64)
    ASSERT_EQ(color.r, 0);
    ASSERT_EQ(color.g, 255);
    ASSERT_EQ(color.b, 0);
}

TEST(color_threshold_level_63) {
    ContaminationGrid grid(64, 64);
    grid.add_contamination(10, 10, 63, 0);
    ContaminationOverlay overlay(grid);
    OverlayColor color = overlay.getColorAt(10, 10);
    // Should be green (level < 64)
    ASSERT_EQ(color.r, 0);
    ASSERT_EQ(color.g, 255);
    ASSERT_EQ(color.b, 0);
}

TEST(color_threshold_level_64) {
    ContaminationGrid grid(64, 64);
    grid.add_contamination(10, 10, 64, 0);
    ContaminationOverlay overlay(grid);
    OverlayColor color = overlay.getColorAt(10, 10);
    // Should be yellow (level >= 64 and < 128)
    ASSERT_EQ(color.r, 255);
    ASSERT_EQ(color.g, 255);
    ASSERT_EQ(color.b, 0);
}

TEST(color_threshold_level_127) {
    ContaminationGrid grid(64, 64);
    grid.add_contamination(10, 10, 127, 0);
    ContaminationOverlay overlay(grid);
    OverlayColor color = overlay.getColorAt(10, 10);
    // Should be yellow (level < 128)
    ASSERT_EQ(color.r, 255);
    ASSERT_EQ(color.g, 255);
    ASSERT_EQ(color.b, 0);
}

TEST(color_threshold_level_128) {
    ContaminationGrid grid(64, 64);
    grid.add_contamination(10, 10, 128, 0);
    ContaminationOverlay overlay(grid);
    OverlayColor color = overlay.getColorAt(10, 10);
    // Should be orange (level >= 128 and < 192)
    ASSERT_EQ(color.r, 255);
    ASSERT_EQ(color.g, 165);
    ASSERT_EQ(color.b, 0);
}

TEST(color_threshold_level_191) {
    ContaminationGrid grid(64, 64);
    grid.add_contamination(10, 10, 191, 0);
    ContaminationOverlay overlay(grid);
    OverlayColor color = overlay.getColorAt(10, 10);
    // Should be orange (level < 192)
    ASSERT_EQ(color.r, 255);
    ASSERT_EQ(color.g, 165);
    ASSERT_EQ(color.b, 0);
}

TEST(color_threshold_level_192) {
    ContaminationGrid grid(64, 64);
    grid.add_contamination(10, 10, 192, 0);
    ContaminationOverlay overlay(grid);
    OverlayColor color = overlay.getColorAt(10, 10);
    // Should be red (level >= 192)
    ASSERT_EQ(color.r, 255);
    ASSERT_EQ(color.g, 0);
    ASSERT_EQ(color.b, 0);
}

// =============================================================================
// Alpha Channel Tests
// =============================================================================

TEST(alpha_increases_with_level) {
    ContaminationGrid grid(64, 64);
    grid.add_contamination(0, 0, 10, 0);
    grid.add_contamination(1, 0, 50, 0);
    ContaminationOverlay overlay(grid);

    OverlayColor color1 = overlay.getColorAt(0, 0);
    OverlayColor color2 = overlay.getColorAt(1, 0);

    // Higher contamination should have higher alpha
    ASSERT(color2.a > color1.a);
}

TEST(alpha_zero_for_no_contamination) {
    ContaminationGrid grid(64, 64);
    ContaminationOverlay overlay(grid);
    OverlayColor color = overlay.getColorAt(10, 10);
    ASSERT_EQ(color.a, 0);
}

TEST(alpha_nonzero_for_contamination) {
    ContaminationGrid grid(64, 64);
    grid.add_contamination(10, 10, 1, 0);
    ContaminationOverlay overlay(grid);
    OverlayColor color = overlay.getColorAt(10, 10);
    ASSERT(color.a > 0);
}

// =============================================================================
// Out-of-Bounds Tests
// =============================================================================

TEST(out_of_bounds_returns_transparent) {
    ContaminationGrid grid(64, 64);
    grid.add_contamination(10, 10, 200, 0);
    ContaminationOverlay overlay(grid);

    OverlayColor color = overlay.getColorAt(64, 64);
    ASSERT_EQ(color.r, 0);
    ASSERT_EQ(color.g, 0);
    ASSERT_EQ(color.b, 0);
    ASSERT_EQ(color.a, 0);
}

TEST(out_of_bounds_negative) {
    ContaminationGrid grid(64, 64);
    grid.add_contamination(10, 10, 200, 0);
    ContaminationOverlay overlay(grid);

    // Note: uint32_t will wrap, but grid.get_level will handle it
    OverlayColor color = overlay.getColorAt(static_cast<uint32_t>(-1), 0);
    ASSERT_EQ(color.a, 0);
}

// =============================================================================
// Multiple Cell Tests
// =============================================================================

TEST(multiple_cells_different_colors) {
    ContaminationGrid grid(64, 64);
    grid.add_contamination(0, 0, 32, 0);   // Low - green
    grid.add_contamination(1, 0, 96, 0);   // Medium - yellow
    grid.add_contamination(2, 0, 160, 0);  // High - orange
    grid.add_contamination(3, 0, 200, 0);  // Toxic - red

    ContaminationOverlay overlay(grid);

    OverlayColor c0 = overlay.getColorAt(0, 0);
    OverlayColor c1 = overlay.getColorAt(1, 0);
    OverlayColor c2 = overlay.getColorAt(2, 0);
    OverlayColor c3 = overlay.getColorAt(3, 0);

    // Verify green
    ASSERT_EQ(c0.g, 255);
    ASSERT_EQ(c0.r, 0);

    // Verify yellow
    ASSERT_EQ(c1.r, 255);
    ASSERT_EQ(c1.g, 255);

    // Verify orange
    ASSERT_EQ(c2.r, 255);
    ASSERT_EQ(c2.g, 165);

    // Verify red
    ASSERT_EQ(c3.r, 255);
    ASSERT_EQ(c3.g, 0);
}

// =============================================================================
// Interface Compliance Tests
// =============================================================================

TEST(implements_igrid_overlay) {
    ContaminationGrid grid(64, 64);
    ContaminationOverlay overlay(grid);
    IGridOverlay* iface = &overlay;
    ASSERT(iface != nullptr);
    ASSERT(iface->getName() != nullptr);
}

// =============================================================================
// Main Entry Point
// =============================================================================

int main() {
    printf("=== ContaminationOverlay Unit Tests (Ticket E10-090) ===\n\n");

    // getName() tests
    RUN_TEST(get_name_returns_contamination);

    // isActive() tests
    RUN_TEST(is_active_returns_true);

    // Basic color tests
    RUN_TEST(get_color_at_zero_contamination);
    RUN_TEST(get_color_at_low_contamination);
    RUN_TEST(get_color_at_medium_contamination);
    RUN_TEST(get_color_at_high_contamination);
    RUN_TEST(get_color_at_toxic_contamination);
    RUN_TEST(get_color_at_max_contamination);

    // Color threshold tests
    RUN_TEST(color_threshold_level_1);
    RUN_TEST(color_threshold_level_63);
    RUN_TEST(color_threshold_level_64);
    RUN_TEST(color_threshold_level_127);
    RUN_TEST(color_threshold_level_128);
    RUN_TEST(color_threshold_level_191);
    RUN_TEST(color_threshold_level_192);

    // Alpha channel tests
    RUN_TEST(alpha_increases_with_level);
    RUN_TEST(alpha_zero_for_no_contamination);
    RUN_TEST(alpha_nonzero_for_contamination);

    // Out-of-bounds tests
    RUN_TEST(out_of_bounds_returns_transparent);
    RUN_TEST(out_of_bounds_negative);

    // Multiple cell tests
    RUN_TEST(multiple_cells_different_colors);

    // Interface compliance
    RUN_TEST(implements_igrid_overlay);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
