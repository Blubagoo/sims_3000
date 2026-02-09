/**
 * @file test_service_coverage_overlay.cpp
 * @brief Unit tests for IGridOverlay and ServiceCoverageOverlay (Ticket E9-043)
 *
 * Tests cover:
 * - ServiceCoverageOverlay construction and name
 * - Active/inactive state management
 * - Color output with coverage data
 * - Null grid handling
 * - Out-of-bounds coordinate handling
 * - Enforcer and Hazard color constants
 * - Grid reassignment via setGrid()
 */

#include <sims3000/services/ServiceCoverageOverlay.h>
#include <sims3000/services/IGridOverlay.h>
#include <cstdio>
#include <cstdlib>
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

#define ASSERT_STR_EQ(a, b) do { \
    if (std::strcmp((a), (b)) != 0) { \
        printf("\n  FAILED: %s == %s (got \"%s\" vs \"%s\") (line %d)\n", \
               #a, #b, (a), (b), __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

// =============================================================================
// Construction and Name Tests
// =============================================================================

TEST(overlay_construction_and_name) {
    ServiceCoverageGrid grid(64, 64);
    ServiceCoverageOverlay overlay("Test Overlay", &grid, 100, 200, 50);
    ASSERT_STR_EQ(overlay.getName(), "Test Overlay");
}

TEST(overlay_name_enforcer) {
    ServiceCoverageOverlay overlay("Enforcer Coverage", nullptr,
                                   ENFORCER_OVERLAY_R, ENFORCER_OVERLAY_G, ENFORCER_OVERLAY_B);
    ASSERT_STR_EQ(overlay.getName(), "Enforcer Coverage");
}

TEST(overlay_name_hazard) {
    ServiceCoverageOverlay overlay("Hazard Coverage", nullptr,
                                   HAZARD_OVERLAY_R, HAZARD_OVERLAY_G, HAZARD_OVERLAY_B);
    ASSERT_STR_EQ(overlay.getName(), "Hazard Coverage");
}

// =============================================================================
// Active State Tests
// =============================================================================

TEST(overlay_initially_inactive) {
    ServiceCoverageGrid grid(64, 64);
    ServiceCoverageOverlay overlay("Test", &grid, 0, 200, 255);
    ASSERT_EQ(overlay.isActive(), false);
}

TEST(overlay_set_active) {
    ServiceCoverageGrid grid(64, 64);
    ServiceCoverageOverlay overlay("Test", &grid, 0, 200, 255);
    overlay.setActive(true);
    ASSERT_EQ(overlay.isActive(), true);
}

TEST(overlay_set_inactive) {
    ServiceCoverageGrid grid(64, 64);
    ServiceCoverageOverlay overlay("Test", &grid, 0, 200, 255);
    overlay.setActive(true);
    overlay.setActive(false);
    ASSERT_EQ(overlay.isActive(), false);
}

// =============================================================================
// Color Output Tests
// =============================================================================

TEST(color_at_zero_coverage) {
    ServiceCoverageGrid grid(64, 64);
    // Grid initialized to 0 coverage
    ServiceCoverageOverlay overlay("Test", &grid, 0, 200, 255);
    OverlayColor color = overlay.getColorAt(0, 0);
    ASSERT_EQ(color.r, 0);
    ASSERT_EQ(color.g, 200);
    ASSERT_EQ(color.b, 255);
    ASSERT_EQ(color.a, 0);  // zero coverage = zero alpha
}

TEST(color_at_full_coverage) {
    ServiceCoverageGrid grid(64, 64);
    grid.set_coverage_at(10, 10, 255);
    ServiceCoverageOverlay overlay("Test", &grid, 0, 200, 255);
    OverlayColor color = overlay.getColorAt(10, 10);
    ASSERT_EQ(color.r, 0);
    ASSERT_EQ(color.g, 200);
    ASSERT_EQ(color.b, 255);
    ASSERT_EQ(color.a, 255);  // full coverage = full alpha
}

TEST(color_at_partial_coverage) {
    ServiceCoverageGrid grid(64, 64);
    grid.set_coverage_at(5, 5, 128);
    ServiceCoverageOverlay overlay("Test", &grid, 0, 200, 255);
    OverlayColor color = overlay.getColorAt(5, 5);
    ASSERT_EQ(color.r, 0);
    ASSERT_EQ(color.g, 200);
    ASSERT_EQ(color.b, 255);
    ASSERT_EQ(color.a, 128);
}

TEST(enforcer_color_scheme) {
    ServiceCoverageGrid grid(32, 32);
    grid.set_coverage_at(1, 1, 200);
    ServiceCoverageOverlay overlay("Enforcer", &grid,
                                   ENFORCER_OVERLAY_R, ENFORCER_OVERLAY_G, ENFORCER_OVERLAY_B);
    OverlayColor color = overlay.getColorAt(1, 1);
    ASSERT_EQ(color.r, 0);
    ASSERT_EQ(color.g, 200);
    ASSERT_EQ(color.b, 255);
    ASSERT_EQ(color.a, 200);
}

TEST(hazard_color_scheme) {
    ServiceCoverageGrid grid(32, 32);
    grid.set_coverage_at(2, 3, 150);
    ServiceCoverageOverlay overlay("Hazard", &grid,
                                   HAZARD_OVERLAY_R, HAZARD_OVERLAY_G, HAZARD_OVERLAY_B);
    OverlayColor color = overlay.getColorAt(2, 3);
    ASSERT_EQ(color.r, 255);
    ASSERT_EQ(color.g, 180);
    ASSERT_EQ(color.b, 0);
    ASSERT_EQ(color.a, 150);
}

// =============================================================================
// Null Grid Tests
// =============================================================================

TEST(null_grid_returns_transparent) {
    ServiceCoverageOverlay overlay("Test", nullptr, 0, 200, 255);
    OverlayColor color = overlay.getColorAt(0, 0);
    ASSERT_EQ(color.r, 0);
    ASSERT_EQ(color.g, 0);
    ASSERT_EQ(color.b, 0);
    ASSERT_EQ(color.a, 0);
}

TEST(null_grid_any_coordinate_returns_transparent) {
    ServiceCoverageOverlay overlay("Test", nullptr, 255, 180, 0);
    OverlayColor color = overlay.getColorAt(100, 200);
    ASSERT_EQ(color.r, 0);
    ASSERT_EQ(color.g, 0);
    ASSERT_EQ(color.b, 0);
    ASSERT_EQ(color.a, 0);
}

// =============================================================================
// Out-of-Bounds Tests
// =============================================================================

TEST(out_of_bounds_returns_transparent) {
    ServiceCoverageGrid grid(16, 16);
    ServiceCoverageOverlay overlay("Test", &grid, 0, 200, 255);
    OverlayColor color = overlay.getColorAt(16, 16);  // beyond bounds
    ASSERT_EQ(color.r, 0);
    ASSERT_EQ(color.g, 0);
    ASSERT_EQ(color.b, 0);
    ASSERT_EQ(color.a, 0);
}

TEST(large_out_of_bounds_returns_transparent) {
    ServiceCoverageGrid grid(16, 16);
    ServiceCoverageOverlay overlay("Test", &grid, 0, 200, 255);
    OverlayColor color = overlay.getColorAt(9999, 9999);
    ASSERT_EQ(color.r, 0);
    ASSERT_EQ(color.g, 0);
    ASSERT_EQ(color.b, 0);
    ASSERT_EQ(color.a, 0);
}

// =============================================================================
// Grid Reassignment Tests
// =============================================================================

TEST(set_grid_changes_source) {
    ServiceCoverageGrid grid1(16, 16);
    ServiceCoverageGrid grid2(16, 16);
    grid1.set_coverage_at(0, 0, 100);
    grid2.set_coverage_at(0, 0, 200);

    ServiceCoverageOverlay overlay("Test", &grid1, 0, 200, 255);
    OverlayColor color1 = overlay.getColorAt(0, 0);
    ASSERT_EQ(color1.a, 100);

    overlay.setGrid(&grid2);
    OverlayColor color2 = overlay.getColorAt(0, 0);
    ASSERT_EQ(color2.a, 200);
}

TEST(set_grid_to_null) {
    ServiceCoverageGrid grid(16, 16);
    grid.set_coverage_at(0, 0, 255);

    ServiceCoverageOverlay overlay("Test", &grid, 0, 200, 255);
    ASSERT_EQ(overlay.getColorAt(0, 0).a, 255);

    overlay.setGrid(nullptr);
    OverlayColor color = overlay.getColorAt(0, 0);
    ASSERT_EQ(color.r, 0);
    ASSERT_EQ(color.g, 0);
    ASSERT_EQ(color.b, 0);
    ASSERT_EQ(color.a, 0);
}

// =============================================================================
// IGridOverlay Polymorphism Test
// =============================================================================

TEST(igridoverlay_polymorphism) {
    ServiceCoverageGrid grid(32, 32);
    grid.set_coverage_at(5, 5, 100);
    ServiceCoverageOverlay overlay("Poly Test", &grid, 255, 180, 0);
    overlay.setActive(true);

    // Access through base interface pointer
    IGridOverlay* base = &overlay;
    ASSERT_STR_EQ(base->getName(), "Poly Test");
    ASSERT_EQ(base->isActive(), true);

    OverlayColor color = base->getColorAt(5, 5);
    ASSERT_EQ(color.r, 255);
    ASSERT_EQ(color.g, 180);
    ASSERT_EQ(color.b, 0);
    ASSERT_EQ(color.a, 100);
}

// =============================================================================
// Color Constants Tests
// =============================================================================

TEST(enforcer_overlay_color_constants) {
    ASSERT_EQ(ENFORCER_OVERLAY_R, 0);
    ASSERT_EQ(ENFORCER_OVERLAY_G, 200);
    ASSERT_EQ(ENFORCER_OVERLAY_B, 255);
}

TEST(hazard_overlay_color_constants) {
    ASSERT_EQ(HAZARD_OVERLAY_R, 255);
    ASSERT_EQ(HAZARD_OVERLAY_G, 180);
    ASSERT_EQ(HAZARD_OVERLAY_B, 0);
}

// =============================================================================
// Main
// =============================================================================

int main() {
    printf("=== ServiceCoverageOverlay Unit Tests (Ticket E9-043) ===\n\n");

    // Construction and name
    RUN_TEST(overlay_construction_and_name);
    RUN_TEST(overlay_name_enforcer);
    RUN_TEST(overlay_name_hazard);

    // Active state
    RUN_TEST(overlay_initially_inactive);
    RUN_TEST(overlay_set_active);
    RUN_TEST(overlay_set_inactive);

    // Color output
    RUN_TEST(color_at_zero_coverage);
    RUN_TEST(color_at_full_coverage);
    RUN_TEST(color_at_partial_coverage);
    RUN_TEST(enforcer_color_scheme);
    RUN_TEST(hazard_color_scheme);

    // Null grid
    RUN_TEST(null_grid_returns_transparent);
    RUN_TEST(null_grid_any_coordinate_returns_transparent);

    // Out-of-bounds
    RUN_TEST(out_of_bounds_returns_transparent);
    RUN_TEST(large_out_of_bounds_returns_transparent);

    // Grid reassignment
    RUN_TEST(set_grid_changes_source);
    RUN_TEST(set_grid_to_null);

    // Polymorphism
    RUN_TEST(igridoverlay_polymorphism);

    // Color constants
    RUN_TEST(enforcer_overlay_color_constants);
    RUN_TEST(hazard_overlay_color_constants);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
