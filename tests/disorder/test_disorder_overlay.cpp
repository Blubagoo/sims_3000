/**
 * @file test_disorder_overlay.cpp
 * @brief Unit tests for DisorderOverlay (Ticket E10-078)
 *
 * Tests cover:
 * - getName returns "Disorder"
 * - isActive returns true
 * - getColorAt maps disorder levels to colors correctly
 * - Out-of-bounds queries return transparent black
 */

#include <sims3000/disorder/DisorderOverlay.h>
#include <sims3000/disorder/DisorderGrid.h>
#include <sims3000/services/IGridOverlay.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

using namespace sims3000::disorder;
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

#define ASSERT_STR_EQ(a, b) do { \
    if (std::strcmp((a), (b)) != 0) { \
        printf("\n  FAILED: %s == %s (line %d, got '%s' vs '%s')\n", #a, #b, __LINE__, (a), (b)); \
        tests_failed++; \
        return; \
    } \
} while(0)

// =============================================================================
// Basic Interface Tests
// =============================================================================

TEST(get_name) {
    DisorderGrid grid(64, 64);
    DisorderOverlay overlay(grid);
    ASSERT_STR_EQ(overlay.getName(), "Disorder");
}

TEST(is_active_returns_true) {
    DisorderGrid grid(64, 64);
    DisorderOverlay overlay(grid);
    ASSERT(overlay.isActive());
}

// =============================================================================
// Color Mapping Tests
// =============================================================================

TEST(color_at_zero_disorder_is_transparent) {
    DisorderGrid grid(64, 64);
    DisorderOverlay overlay(grid);
    // Default disorder is 0
    OverlayColor color = overlay.getColorAt(10, 10);
    ASSERT_EQ(color.r, 0);
    ASSERT_EQ(color.g, 0);
    ASSERT_EQ(color.b, 0);
    ASSERT_EQ(color.a, 0);
}

TEST(color_at_low_disorder_is_green) {
    DisorderGrid grid(64, 64);
    grid.set_level(10, 10, 50); // Low disorder
    DisorderOverlay overlay(grid);
    OverlayColor color = overlay.getColorAt(10, 10);
    ASSERT_EQ(color.r, 0);
    ASSERT_EQ(color.g, 255);
    ASSERT_EQ(color.b, 0);
    ASSERT_EQ(color.a, 50);
}

TEST(color_at_low_disorder_boundary) {
    DisorderGrid grid(64, 64);
    grid.set_level(10, 10, 85); // At low boundary
    DisorderOverlay overlay(grid);
    OverlayColor color = overlay.getColorAt(10, 10);
    ASSERT_EQ(color.r, 0);
    ASSERT_EQ(color.g, 255);
    ASSERT_EQ(color.b, 0);
    ASSERT_EQ(color.a, 85);
}

TEST(color_at_medium_disorder_is_yellow) {
    DisorderGrid grid(64, 64);
    grid.set_level(10, 10, 100); // Medium disorder
    DisorderOverlay overlay(grid);
    OverlayColor color = overlay.getColorAt(10, 10);
    ASSERT_EQ(color.r, 255);
    ASSERT_EQ(color.g, 255);
    ASSERT_EQ(color.b, 0);
    ASSERT_EQ(color.a, 100);
}

TEST(color_at_medium_disorder_boundary) {
    DisorderGrid grid(64, 64);
    grid.set_level(10, 10, 170); // At medium boundary
    DisorderOverlay overlay(grid);
    OverlayColor color = overlay.getColorAt(10, 10);
    ASSERT_EQ(color.r, 255);
    ASSERT_EQ(color.g, 255);
    ASSERT_EQ(color.b, 0);
    ASSERT_EQ(color.a, 170);
}

TEST(color_at_high_disorder_is_red) {
    DisorderGrid grid(64, 64);
    grid.set_level(10, 10, 200); // High disorder
    DisorderOverlay overlay(grid);
    OverlayColor color = overlay.getColorAt(10, 10);
    ASSERT_EQ(color.r, 255);
    ASSERT_EQ(color.g, 0);
    ASSERT_EQ(color.b, 0);
    ASSERT_EQ(color.a, 200);
}

TEST(color_at_max_disorder_is_red) {
    DisorderGrid grid(64, 64);
    grid.set_level(10, 10, 255); // Max disorder
    DisorderOverlay overlay(grid);
    OverlayColor color = overlay.getColorAt(10, 10);
    ASSERT_EQ(color.r, 255);
    ASSERT_EQ(color.g, 0);
    ASSERT_EQ(color.b, 0);
    ASSERT_EQ(color.a, 255);
}

TEST(color_at_medium_disorder_start) {
    DisorderGrid grid(64, 64);
    grid.set_level(10, 10, 86); // Just above low
    DisorderOverlay overlay(grid);
    OverlayColor color = overlay.getColorAt(10, 10);
    ASSERT_EQ(color.r, 255);
    ASSERT_EQ(color.g, 255);
    ASSERT_EQ(color.b, 0);
    ASSERT_EQ(color.a, 86);
}

TEST(color_at_high_disorder_start) {
    DisorderGrid grid(64, 64);
    grid.set_level(10, 10, 171); // Just above medium
    DisorderOverlay overlay(grid);
    OverlayColor color = overlay.getColorAt(10, 10);
    ASSERT_EQ(color.r, 255);
    ASSERT_EQ(color.g, 0);
    ASSERT_EQ(color.b, 0);
    ASSERT_EQ(color.a, 171);
}

// =============================================================================
// Multiple Position Tests
// =============================================================================

TEST(color_at_multiple_positions) {
    DisorderGrid grid(64, 64);
    grid.set_level(0, 0, 0);     // Transparent
    grid.set_level(10, 10, 50);  // Green
    grid.set_level(20, 20, 100); // Yellow
    grid.set_level(30, 30, 200); // Red

    DisorderOverlay overlay(grid);

    OverlayColor c1 = overlay.getColorAt(0, 0);
    ASSERT_EQ(c1.a, 0); // Transparent

    OverlayColor c2 = overlay.getColorAt(10, 10);
    ASSERT_EQ(c2.g, 255); // Green

    OverlayColor c3 = overlay.getColorAt(20, 20);
    ASSERT_EQ(c3.r, 255);
    ASSERT_EQ(c3.g, 255); // Yellow

    OverlayColor c4 = overlay.getColorAt(30, 30);
    ASSERT_EQ(c4.r, 255);
    ASSERT_EQ(c4.g, 0); // Red
}

// =============================================================================
// Bounds Tests
// =============================================================================

TEST(color_at_out_of_bounds_returns_transparent) {
    DisorderGrid grid(64, 64);
    grid.set_level(0, 0, 100);
    DisorderOverlay overlay(grid);

    // Out of bounds should return disorder=0, which maps to transparent
    OverlayColor color = overlay.getColorAt(64, 0);
    ASSERT_EQ(color.r, 0);
    ASSERT_EQ(color.g, 0);
    ASSERT_EQ(color.b, 0);
    ASSERT_EQ(color.a, 0);
}

TEST(color_at_corners) {
    DisorderGrid grid(64, 64);
    grid.set_level(0, 0, 10);
    grid.set_level(63, 0, 20);
    grid.set_level(0, 63, 30);
    grid.set_level(63, 63, 40);

    DisorderOverlay overlay(grid);

    ASSERT_EQ(overlay.getColorAt(0, 0).a, 10);
    ASSERT_EQ(overlay.getColorAt(63, 0).a, 20);
    ASSERT_EQ(overlay.getColorAt(0, 63).a, 30);
    ASSERT_EQ(overlay.getColorAt(63, 63).a, 40);
}

// =============================================================================
// Main Entry Point
// =============================================================================

int main() {
    printf("=== DisorderOverlay Unit Tests (Ticket E10-078) ===\n\n");

    // Basic interface tests
    RUN_TEST(get_name);
    RUN_TEST(is_active_returns_true);

    // Color mapping tests
    RUN_TEST(color_at_zero_disorder_is_transparent);
    RUN_TEST(color_at_low_disorder_is_green);
    RUN_TEST(color_at_low_disorder_boundary);
    RUN_TEST(color_at_medium_disorder_is_yellow);
    RUN_TEST(color_at_medium_disorder_boundary);
    RUN_TEST(color_at_high_disorder_is_red);
    RUN_TEST(color_at_max_disorder_is_red);
    RUN_TEST(color_at_medium_disorder_start);
    RUN_TEST(color_at_high_disorder_start);

    // Multiple position tests
    RUN_TEST(color_at_multiple_positions);

    // Bounds tests
    RUN_TEST(color_at_out_of_bounds_returns_transparent);
    RUN_TEST(color_at_corners);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
