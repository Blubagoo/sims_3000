/**
 * @file test_service_coverage_grid.cpp
 * @brief Unit tests for ServiceCoverageGrid class (Epic 9, Ticket E9-010)
 *
 * Tests cover:
 * - Grid creation with correct dimensions
 * - set/get coverage values
 * - Bounds checking returns safe defaults
 * - clear() resets all values
 * - Normalized value retrieval (0.0-1.0)
 * - is_valid() bounds checking
 * - Memory layout (256x256 = 64KB)
 */

#include <sims3000/services/ServiceCoverageGrid.h>
#include <cassert>
#include <cstdio>
#include <cmath>

using namespace sims3000::services;

// =============================================================================
// Construction tests
// =============================================================================

void test_construction_dimensions() {
    printf("Testing grid construction with dimensions...\n");

    ServiceCoverageGrid grid(128, 256);
    assert(grid.get_width() == 128);
    assert(grid.get_height() == 256);

    printf("  PASS: Grid dimensions set correctly\n");
}

void test_construction_initial_values() {
    printf("Testing grid initial values are 0...\n");

    ServiceCoverageGrid grid(16, 16);
    for (uint32_t y = 0; y < 16; ++y) {
        for (uint32_t x = 0; x < 16; ++x) {
            assert(grid.get_coverage_at(x, y) == 0);
        }
    }

    printf("  PASS: All initial values are 0\n");
}

void test_construction_256x256_memory() {
    printf("Testing 256x256 grid creation (64KB)...\n");

    ServiceCoverageGrid grid(256, 256);
    assert(grid.get_width() == 256);
    assert(grid.get_height() == 256);

    // All values should be 0
    assert(grid.get_coverage_at(0, 0) == 0);
    assert(grid.get_coverage_at(255, 255) == 0);

    printf("  PASS: 256x256 grid created successfully\n");
}

// =============================================================================
// Set/Get tests
// =============================================================================

void test_set_and_get() {
    printf("Testing set_coverage_at and get_coverage_at...\n");

    ServiceCoverageGrid grid(32, 32);

    grid.set_coverage_at(5, 10, 200);
    assert(grid.get_coverage_at(5, 10) == 200);

    grid.set_coverage_at(0, 0, 1);
    assert(grid.get_coverage_at(0, 0) == 1);

    grid.set_coverage_at(31, 31, 255);
    assert(grid.get_coverage_at(31, 31) == 255);

    printf("  PASS: Set and get work correctly\n");
}

void test_set_overwrites() {
    printf("Testing set_coverage_at overwrites previous value...\n");

    ServiceCoverageGrid grid(8, 8);

    grid.set_coverage_at(3, 3, 100);
    assert(grid.get_coverage_at(3, 3) == 100);

    grid.set_coverage_at(3, 3, 50);
    assert(grid.get_coverage_at(3, 3) == 50);

    grid.set_coverage_at(3, 3, 0);
    assert(grid.get_coverage_at(3, 3) == 0);

    printf("  PASS: Set overwrites correctly\n");
}

void test_set_min_max_values() {
    printf("Testing min/max coverage values (0 and 255)...\n");

    ServiceCoverageGrid grid(8, 8);

    grid.set_coverage_at(0, 0, 0);
    assert(grid.get_coverage_at(0, 0) == 0);

    grid.set_coverage_at(1, 1, 255);
    assert(grid.get_coverage_at(1, 1) == 255);

    printf("  PASS: Min and max values work correctly\n");
}

void test_independent_cells() {
    printf("Testing cells are independent...\n");

    ServiceCoverageGrid grid(8, 8);

    grid.set_coverage_at(2, 3, 100);
    grid.set_coverage_at(3, 2, 200);

    assert(grid.get_coverage_at(2, 3) == 100);
    assert(grid.get_coverage_at(3, 2) == 200);
    assert(grid.get_coverage_at(2, 2) == 0);
    assert(grid.get_coverage_at(3, 3) == 0);

    printf("  PASS: Cells are independent\n");
}

// =============================================================================
// Bounds checking tests
// =============================================================================

void test_get_out_of_bounds() {
    printf("Testing get_coverage_at returns 0 for out-of-bounds...\n");

    ServiceCoverageGrid grid(8, 8);

    assert(grid.get_coverage_at(8, 0) == 0);
    assert(grid.get_coverage_at(0, 8) == 0);
    assert(grid.get_coverage_at(100, 100) == 0);
    assert(grid.get_coverage_at(UINT32_MAX, UINT32_MAX) == 0);

    printf("  PASS: Out-of-bounds get returns 0\n");
}

void test_set_out_of_bounds() {
    printf("Testing set_coverage_at is no-op for out-of-bounds...\n");

    ServiceCoverageGrid grid(8, 8);

    // These should not crash (no-op)
    grid.set_coverage_at(8, 0, 100);
    grid.set_coverage_at(0, 8, 100);
    grid.set_coverage_at(100, 100, 100);
    grid.set_coverage_at(UINT32_MAX, UINT32_MAX, 100);

    // Grid should be unaffected
    for (uint32_t y = 0; y < 8; ++y) {
        for (uint32_t x = 0; x < 8; ++x) {
            assert(grid.get_coverage_at(x, y) == 0);
        }
    }

    printf("  PASS: Out-of-bounds set is no-op\n");
}

// =============================================================================
// Clear tests
// =============================================================================

void test_clear() {
    printf("Testing clear() resets all values to 0...\n");

    ServiceCoverageGrid grid(16, 16);

    // Set some values
    grid.set_coverage_at(0, 0, 100);
    grid.set_coverage_at(5, 5, 200);
    grid.set_coverage_at(15, 15, 255);

    assert(grid.get_coverage_at(0, 0) == 100);
    assert(grid.get_coverage_at(5, 5) == 200);

    grid.clear();

    // All values should be 0
    for (uint32_t y = 0; y < 16; ++y) {
        for (uint32_t x = 0; x < 16; ++x) {
            assert(grid.get_coverage_at(x, y) == 0);
        }
    }

    printf("  PASS: Clear resets all values to 0\n");
}

void test_clear_then_set() {
    printf("Testing set after clear...\n");

    ServiceCoverageGrid grid(8, 8);

    grid.set_coverage_at(3, 3, 150);
    grid.clear();
    assert(grid.get_coverage_at(3, 3) == 0);

    grid.set_coverage_at(3, 3, 75);
    assert(grid.get_coverage_at(3, 3) == 75);

    printf("  PASS: Set after clear works correctly\n");
}

// =============================================================================
// Normalized value tests
// =============================================================================

void test_normalized_zero() {
    printf("Testing normalized coverage at 0...\n");

    ServiceCoverageGrid grid(8, 8);
    float val = grid.get_coverage_at_normalized(0, 0);
    assert(val == 0.0f);

    printf("  PASS: Normalized coverage at 0 is 0.0f\n");
}

void test_normalized_max() {
    printf("Testing normalized coverage at 255...\n");

    ServiceCoverageGrid grid(8, 8);
    grid.set_coverage_at(0, 0, 255);
    float val = grid.get_coverage_at_normalized(0, 0);
    assert(val == 1.0f);

    printf("  PASS: Normalized coverage at 255 is 1.0f\n");
}

void test_normalized_midpoint() {
    printf("Testing normalized coverage at midpoint...\n");

    ServiceCoverageGrid grid(8, 8);
    grid.set_coverage_at(0, 0, 128);
    float val = grid.get_coverage_at_normalized(0, 0);
    float expected = 128.0f / 255.0f;
    assert(std::fabs(val - expected) < 0.001f);

    printf("  PASS: Normalized midpoint is correct\n");
}

void test_normalized_out_of_bounds() {
    printf("Testing normalized coverage out of bounds returns 0.0f...\n");

    ServiceCoverageGrid grid(8, 8);
    float val = grid.get_coverage_at_normalized(100, 100);
    assert(val == 0.0f);

    printf("  PASS: Normalized out-of-bounds returns 0.0f\n");
}

// =============================================================================
// is_valid tests
// =============================================================================

void test_is_valid() {
    printf("Testing is_valid bounds checking...\n");

    ServiceCoverageGrid grid(16, 32);

    assert(grid.is_valid(0, 0));
    assert(grid.is_valid(15, 31));
    assert(grid.is_valid(8, 16));

    assert(!grid.is_valid(16, 0));
    assert(!grid.is_valid(0, 32));
    assert(!grid.is_valid(16, 32));
    assert(!grid.is_valid(UINT32_MAX, 0));

    printf("  PASS: is_valid works correctly\n");
}

// =============================================================================
// Row-major layout tests
// =============================================================================

void test_row_major_layout() {
    printf("Testing row-major layout (y * width + x)...\n");

    ServiceCoverageGrid grid(4, 4);

    // Set values with distinguishable patterns
    grid.set_coverage_at(0, 0, 1);   // index 0
    grid.set_coverage_at(1, 0, 2);   // index 1
    grid.set_coverage_at(0, 1, 5);   // index 4
    grid.set_coverage_at(3, 3, 16);  // index 15

    assert(grid.get_coverage_at(0, 0) == 1);
    assert(grid.get_coverage_at(1, 0) == 2);
    assert(grid.get_coverage_at(0, 1) == 5);
    assert(grid.get_coverage_at(3, 3) == 16);

    printf("  PASS: Row-major layout correct\n");
}

// =============================================================================
// Main
// =============================================================================

int main() {
    printf("=== ServiceCoverageGrid Unit Tests (Epic 9, Ticket E9-010) ===\n\n");

    test_construction_dimensions();
    test_construction_initial_values();
    test_construction_256x256_memory();
    test_set_and_get();
    test_set_overwrites();
    test_set_min_max_values();
    test_independent_cells();
    test_get_out_of_bounds();
    test_set_out_of_bounds();
    test_clear();
    test_clear_then_set();
    test_normalized_zero();
    test_normalized_max();
    test_normalized_midpoint();
    test_normalized_out_of_bounds();
    test_is_valid();
    test_row_major_layout();

    printf("\n=== All ServiceCoverageGrid Tests Passed ===\n");
    return 0;
}
