/**
 * @file test_terrain_normals.cpp
 * @brief Unit tests for TerrainNormals (Ticket 3-024)
 *
 * Tests:
 * - Flat terrain produces normals pointing straight up (0, 1, 0)
 * - Sloped terrain produces correctly oriented normals
 * - Map edge boundary handling (clamping)
 * - Chunk edge handling (uses grid data correctly)
 * - Central differences formula verification
 * - Slope angle calculation
 * - isNormalFlat helper
 */

#include <sims3000/terrain/TerrainNormals.h>
#include <sims3000/terrain/TerrainGrid.h>
#include <sims3000/terrain/TerrainChunk.h>
#include <cstdio>
#include <cmath>

using namespace sims3000::terrain;

// Test counter
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST_ASSERT(condition, message) \
    do { \
        if (condition) { \
            tests_passed++; \
            printf("[PASS] %s\n", message); \
        } else { \
            tests_failed++; \
            printf("[FAIL] %s\n", message); \
        } \
    } while (0)

#define TEST_ASSERT_FLOAT_NEAR(actual, expected, tolerance, message) \
    do { \
        float diff = std::abs((actual) - (expected)); \
        if (diff <= (tolerance)) { \
            tests_passed++; \
            printf("[PASS] %s\n", message); \
        } else { \
            tests_failed++; \
            printf("[FAIL] %s (expected %.6f, got %.6f, diff %.6f)\n", \
                   message, (float)(expected), (float)(actual), diff); \
        } \
    } while (0)

// ============================================================================
// Test: Flat terrain produces upward-pointing normals
// ============================================================================
void test_flat_terrain_normal() {
    printf("\n--- Test: Flat Terrain Normal ---\n");

    // Create a 128x128 grid with all tiles at the same elevation
    TerrainGrid grid(MapSize::Small);

    // Fill with flat terrain at elevation 10
    for (std::size_t i = 0; i < grid.tiles.size(); ++i) {
        grid.tiles[i].setElevation(10);
    }

    // Test center of map
    NormalResult normal = computeTerrainNormal(grid, 64, 64);

    TEST_ASSERT_FLOAT_NEAR(normal.nx, 0.0f, 0.0001f, "Flat terrain nx is 0");
    TEST_ASSERT_FLOAT_NEAR(normal.ny, 1.0f, 0.0001f, "Flat terrain ny is 1");
    TEST_ASSERT_FLOAT_NEAR(normal.nz, 0.0f, 0.0001f, "Flat terrain nz is 0");

    TEST_ASSERT(isNormalFlat(normal), "isNormalFlat returns true for flat terrain");
}

// ============================================================================
// Test: Sloped terrain in X direction
// ============================================================================
void test_slope_x_direction() {
    printf("\n--- Test: Slope in X Direction ---\n");

    // Create a grid with elevation increasing in X direction
    TerrainGrid grid(MapSize::Small);

    // Set elevation = x / 4 (gives values 0-31 across 128 tiles)
    for (std::uint16_t y = 0; y < grid.height; ++y) {
        for (std::uint16_t x = 0; x < grid.width; ++x) {
            std::uint8_t elev = static_cast<std::uint8_t>(x / 4);
            if (elev > 31) elev = 31;
            grid.at(x, y).setElevation(elev);
        }
    }

    // Test at center of map
    NormalResult normal = computeTerrainNormal(grid, 64, 64);

    // With slope increasing in +X, normal should point toward -X
    // (uphill side is brighter in toon shading)
    TEST_ASSERT(normal.nx < 0.0f, "X-slope normal points toward -X (uphill)");
    TEST_ASSERT(normal.ny > 0.0f, "X-slope normal has positive Y");
    TEST_ASSERT_FLOAT_NEAR(normal.nz, 0.0f, 0.001f, "X-slope normal has ~0 Z component");

    TEST_ASSERT(!isNormalFlat(normal), "isNormalFlat returns false for sloped terrain");

    // Verify normalized (length = 1)
    float length = std::sqrt(normal.nx * normal.nx + normal.ny * normal.ny + normal.nz * normal.nz);
    TEST_ASSERT_FLOAT_NEAR(length, 1.0f, 0.0001f, "Normal is unit length");
}

// ============================================================================
// Test: Sloped terrain in Z direction
// ============================================================================
void test_slope_z_direction() {
    printf("\n--- Test: Slope in Z Direction ---\n");

    // Create a grid with elevation increasing in Z direction
    TerrainGrid grid(MapSize::Small);

    // Set elevation = z / 4
    for (std::uint16_t y = 0; y < grid.height; ++y) {
        for (std::uint16_t x = 0; x < grid.width; ++x) {
            std::uint8_t elev = static_cast<std::uint8_t>(y / 4);
            if (elev > 31) elev = 31;
            grid.at(x, y).setElevation(elev);
        }
    }

    // Test at center of map
    NormalResult normal = computeTerrainNormal(grid, 64, 64);

    // With slope increasing in +Z, normal should point toward -Z
    TEST_ASSERT_FLOAT_NEAR(normal.nx, 0.0f, 0.001f, "Z-slope normal has ~0 X component");
    TEST_ASSERT(normal.ny > 0.0f, "Z-slope normal has positive Y");
    TEST_ASSERT(normal.nz < 0.0f, "Z-slope normal points toward -Z (uphill)");

    // Verify normalized
    float length = std::sqrt(normal.nx * normal.nx + normal.ny * normal.ny + normal.nz * normal.nz);
    TEST_ASSERT_FLOAT_NEAR(length, 1.0f, 0.0001f, "Normal is unit length");
}

// ============================================================================
// Test: Diagonal slope
// ============================================================================
void test_diagonal_slope() {
    printf("\n--- Test: Diagonal Slope ---\n");

    // Create a grid with elevation increasing in both X and Z
    TerrainGrid grid(MapSize::Small);

    // Set elevation = (x + z) / 8
    for (std::uint16_t y = 0; y < grid.height; ++y) {
        for (std::uint16_t x = 0; x < grid.width; ++x) {
            std::uint8_t elev = static_cast<std::uint8_t>((x + y) / 8);
            if (elev > 31) elev = 31;
            grid.at(x, y).setElevation(elev);
        }
    }

    // Test at center of map
    NormalResult normal = computeTerrainNormal(grid, 64, 64);

    // Both X and Z should have negative components (pointing uphill)
    TEST_ASSERT(normal.nx < 0.0f, "Diagonal slope normal has negative X");
    TEST_ASSERT(normal.ny > 0.0f, "Diagonal slope normal has positive Y");
    TEST_ASSERT(normal.nz < 0.0f, "Diagonal slope normal has negative Z");

    // For equal slopes, X and Z components should be approximately equal
    TEST_ASSERT_FLOAT_NEAR(normal.nx, normal.nz, 0.001f, "Equal slope gives equal X and Z");

    // Verify normalized
    float length = std::sqrt(normal.nx * normal.nx + normal.ny * normal.ny + normal.nz * normal.nz);
    TEST_ASSERT_FLOAT_NEAR(length, 1.0f, 0.0001f, "Normal is unit length");
}

// ============================================================================
// Test: Map edge handling (X = 0)
// ============================================================================
void test_map_edge_x_zero() {
    printf("\n--- Test: Map Edge X=0 Boundary ---\n");

    TerrainGrid grid(MapSize::Small);

    // Create a slope near the edge
    for (std::uint16_t y = 0; y < grid.height; ++y) {
        for (std::uint16_t x = 0; x < grid.width; ++x) {
            grid.at(x, y).setElevation(static_cast<std::uint8_t>(x / 4));
        }
    }

    // Test at X=0 edge
    NormalResult normal = computeTerrainNormal(grid, 0, 64);

    // Should not crash or produce NaN
    TEST_ASSERT(!std::isnan(normal.nx), "Edge X=0 normal.nx is not NaN");
    TEST_ASSERT(!std::isnan(normal.ny), "Edge X=0 normal.ny is not NaN");
    TEST_ASSERT(!std::isnan(normal.nz), "Edge X=0 normal.nz is not NaN");

    // Normal should still be unit length
    float length = std::sqrt(normal.nx * normal.nx + normal.ny * normal.ny + normal.nz * normal.nz);
    TEST_ASSERT_FLOAT_NEAR(length, 1.0f, 0.0001f, "Edge normal is unit length");
}

// ============================================================================
// Test: Map edge handling (X = max)
// ============================================================================
void test_map_edge_x_max() {
    printf("\n--- Test: Map Edge X=max Boundary ---\n");

    TerrainGrid grid(MapSize::Small);

    // Create a slope
    for (std::uint16_t y = 0; y < grid.height; ++y) {
        for (std::uint16_t x = 0; x < grid.width; ++x) {
            grid.at(x, y).setElevation(static_cast<std::uint8_t>(x / 4));
        }
    }

    // Test at X=127 (max for 128x128 map)
    NormalResult normal = computeTerrainNormal(grid, 127, 64);

    // Should not crash or produce NaN
    TEST_ASSERT(!std::isnan(normal.nx), "Edge X=max normal.nx is not NaN");
    TEST_ASSERT(!std::isnan(normal.ny), "Edge X=max normal.ny is not NaN");
    TEST_ASSERT(!std::isnan(normal.nz), "Edge X=max normal.nz is not NaN");

    // Normal should still be unit length
    float length = std::sqrt(normal.nx * normal.nx + normal.ny * normal.ny + normal.nz * normal.nz);
    TEST_ASSERT_FLOAT_NEAR(length, 1.0f, 0.0001f, "Edge normal is unit length");
}

// ============================================================================
// Test: Map edge handling (Z = 0)
// ============================================================================
void test_map_edge_z_zero() {
    printf("\n--- Test: Map Edge Z=0 Boundary ---\n");

    TerrainGrid grid(MapSize::Small);

    // Create a slope in Z
    for (std::uint16_t y = 0; y < grid.height; ++y) {
        for (std::uint16_t x = 0; x < grid.width; ++x) {
            grid.at(x, y).setElevation(static_cast<std::uint8_t>(y / 4));
        }
    }

    // Test at Z=0 edge
    NormalResult normal = computeTerrainNormal(grid, 64, 0);

    // Should not crash or produce NaN
    TEST_ASSERT(!std::isnan(normal.nx), "Edge Z=0 normal.nx is not NaN");
    TEST_ASSERT(!std::isnan(normal.ny), "Edge Z=0 normal.ny is not NaN");
    TEST_ASSERT(!std::isnan(normal.nz), "Edge Z=0 normal.nz is not NaN");

    // Normal should still be unit length
    float length = std::sqrt(normal.nx * normal.nx + normal.ny * normal.ny + normal.nz * normal.nz);
    TEST_ASSERT_FLOAT_NEAR(length, 1.0f, 0.0001f, "Edge normal is unit length");
}

// ============================================================================
// Test: Map corner handling
// ============================================================================
void test_map_corner() {
    printf("\n--- Test: Map Corner (0,0) ---\n");

    TerrainGrid grid(MapSize::Small);

    // Create a diagonal slope
    for (std::uint16_t y = 0; y < grid.height; ++y) {
        for (std::uint16_t x = 0; x < grid.width; ++x) {
            grid.at(x, y).setElevation(static_cast<std::uint8_t>((x + y) / 8));
        }
    }

    // Test at corner (0, 0)
    NormalResult normal = computeTerrainNormal(grid, 0, 0);

    // Should not crash or produce NaN
    TEST_ASSERT(!std::isnan(normal.nx), "Corner normal.nx is not NaN");
    TEST_ASSERT(!std::isnan(normal.ny), "Corner normal.ny is not NaN");
    TEST_ASSERT(!std::isnan(normal.nz), "Corner normal.nz is not NaN");

    // Normal should be unit length
    float length = std::sqrt(normal.nx * normal.nx + normal.ny * normal.ny + normal.nz * normal.nz);
    TEST_ASSERT_FLOAT_NEAR(length, 1.0f, 0.0001f, "Corner normal is unit length");
}

// ============================================================================
// Test: Chunk boundary (within grid, not map edge)
// ============================================================================
void test_chunk_boundary() {
    printf("\n--- Test: Chunk Boundary (uses grid data) ---\n");

    TerrainGrid grid(MapSize::Small);

    // Create elevation pattern that changes across chunk boundary
    // Chunk boundary at x=32 (CHUNK_SIZE)
    for (std::uint16_t y = 0; y < grid.height; ++y) {
        for (std::uint16_t x = 0; x < grid.width; ++x) {
            if (x < 32) {
                grid.at(x, y).setElevation(10);
            } else {
                grid.at(x, y).setElevation(20);
            }
        }
    }

    // Test at chunk boundary (x=32)
    // This should read elevation from both chunks via the grid
    NormalResult normal = computeTerrainNormal(grid, 32, 64);

    // At x=32, h(x-1)=10*0.25=2.5, h(x+1)=20*0.25=5.0
    // nx = h(x-1) - h(x+1) = 2.5 - 5.0 = -2.5
    // So normal should point toward -X (uphill toward higher elevation)
    TEST_ASSERT(normal.nx < 0.0f, "Chunk boundary normal points toward higher elevation");

    // No slope in Z direction
    TEST_ASSERT_FLOAT_NEAR(normal.nz, 0.0f, 0.001f, "Chunk boundary has no Z slope");

    // Y should be positive
    TEST_ASSERT(normal.ny > 0.0f, "Chunk boundary normal has positive Y");

    // Verify normalized
    float length = std::sqrt(normal.nx * normal.nx + normal.ny * normal.ny + normal.nz * normal.nz);
    TEST_ASSERT_FLOAT_NEAR(length, 1.0f, 0.0001f, "Chunk boundary normal is unit length");
}

// ============================================================================
// Test: Central differences formula verification
// ============================================================================
void test_central_differences_formula() {
    printf("\n--- Test: Central Differences Formula ---\n");

    TerrainGrid grid(MapSize::Small);

    // Set specific elevations for formula verification
    // Position (64, 64) surrounded by:
    // - (63, 64) = elevation 10
    // - (65, 64) = elevation 20
    // - (64, 63) = elevation 15
    // - (64, 65) = elevation 15
    for (std::uint16_t y = 0; y < grid.height; ++y) {
        for (std::uint16_t x = 0; x < grid.width; ++x) {
            grid.at(x, y).setElevation(15);  // Default
        }
    }
    grid.at(63, 64).setElevation(10);
    grid.at(65, 64).setElevation(20);
    grid.at(64, 63).setElevation(15);
    grid.at(64, 65).setElevation(15);

    NormalResult normal = computeTerrainNormal(grid, 64, 64);

    // Manual calculation:
    // h(63,64) = 10 * 0.25 = 2.5
    // h(65,64) = 20 * 0.25 = 5.0
    // h(64,63) = 15 * 0.25 = 3.75
    // h(64,65) = 15 * 0.25 = 3.75
    //
    // nx = h(x-1,z) - h(x+1,z) = 2.5 - 5.0 = -2.5
    // nz = h(x,z-1) - h(x,z+1) = 3.75 - 3.75 = 0.0
    // ny = 2.0 * ELEVATION_HEIGHT = 2.0 * 0.25 = 0.5
    //
    // Length = sqrt((-2.5)^2 + 0.5^2 + 0^2) = sqrt(6.25 + 0.25) = sqrt(6.5) = 2.5495
    // Normalized: nx = -2.5/2.5495 = -0.9806, ny = 0.5/2.5495 = 0.1961, nz = 0

    float expected_length = std::sqrt(2.5f * 2.5f + 0.5f * 0.5f);
    float expected_nx = -2.5f / expected_length;
    float expected_ny = 0.5f / expected_length;
    float expected_nz = 0.0f;

    TEST_ASSERT_FLOAT_NEAR(normal.nx, expected_nx, 0.001f, "Central diff formula: nx correct");
    TEST_ASSERT_FLOAT_NEAR(normal.ny, expected_ny, 0.001f, "Central diff formula: ny correct");
    TEST_ASSERT_FLOAT_NEAR(normal.nz, expected_nz, 0.001f, "Central diff formula: nz correct");
}

// ============================================================================
// Test: Slope angle calculation
// ============================================================================
void test_slope_angle() {
    printf("\n--- Test: Slope Angle Calculation ---\n");

    // Flat terrain: angle = 0
    NormalResult flat_normal = {0.0f, 1.0f, 0.0f};
    float flat_angle = calculateSlopeAngle(flat_normal);
    TEST_ASSERT_FLOAT_NEAR(flat_angle, 0.0f, 0.0001f, "Flat terrain has 0 slope angle");

    // 45 degree slope: ny = cos(45) = 0.707
    NormalResult slope45 = {-0.707f, 0.707f, 0.0f};  // Not quite normalized, but close
    float angle45 = calculateSlopeAngle(slope45);
    float expected_45 = std::acos(0.707f);  // ~0.785 radians (~45 degrees)
    TEST_ASSERT_FLOAT_NEAR(angle45, expected_45, 0.01f, "45-degree slope angle calculation");

    // Vertical cliff: ny = 0, angle = PI/2
    NormalResult cliff_normal = {1.0f, 0.0f, 0.0f};
    float cliff_angle = calculateSlopeAngle(cliff_normal);
    float pi_over_2 = 3.14159265f / 2.0f;
    TEST_ASSERT_FLOAT_NEAR(cliff_angle, pi_over_2, 0.0001f, "Vertical cliff has PI/2 slope angle");
}

// ============================================================================
// Test: sampleElevationClamped helper
// ============================================================================
void test_sample_elevation_clamped() {
    printf("\n--- Test: sampleElevationClamped Helper ---\n");

    TerrainGrid grid(MapSize::Small);

    // Set known elevations at corners
    grid.at(0, 0).setElevation(5);
    grid.at(127, 0).setElevation(10);
    grid.at(0, 127).setElevation(15);
    grid.at(127, 127).setElevation(20);

    // Normal sampling
    float elev_0_0 = sampleElevationClamped(grid, 0, 0);
    TEST_ASSERT_FLOAT_NEAR(elev_0_0, 5.0f * ELEVATION_HEIGHT, 0.0001f, "Sample (0,0) = 1.25");

    // Negative coordinates should clamp to 0
    float elev_neg = sampleElevationClamped(grid, -10, -10);
    TEST_ASSERT_FLOAT_NEAR(elev_neg, 5.0f * ELEVATION_HEIGHT, 0.0001f, "Negative coords clamp to (0,0)");

    // Over-max coordinates should clamp to max
    float elev_over = sampleElevationClamped(grid, 200, 200);
    TEST_ASSERT_FLOAT_NEAR(elev_over, 20.0f * ELEVATION_HEIGHT, 0.0001f, "Over-max coords clamp to (127,127)");
}

// ============================================================================
// Test: Toon shader banding behavior
// ============================================================================
void test_toon_shader_banding() {
    printf("\n--- Test: Toon Shader Banding Behavior ---\n");

    // This test verifies that normals produce expected banding:
    // - Flat terrain (ny ~= 1): Full light band
    // - Gentle slope: Mid light band
    // - Steep slope: Shadow band

    // Create grid with varying slopes
    TerrainGrid grid(MapSize::Small);

    // Flat region (elevation 10 everywhere)
    for (std::uint16_t y = 0; y < 40; ++y) {
        for (std::uint16_t x = 0; x < grid.width; ++x) {
            grid.at(x, y).setElevation(10);
        }
    }

    // Gentle slope region - 1 elevation per 4 tiles for a visible slope
    for (std::uint16_t y = 40; y < 80; ++y) {
        for (std::uint16_t x = 0; x < grid.width; ++x) {
            std::uint8_t elev = static_cast<std::uint8_t>(10 + (y - 40) / 4);
            grid.at(x, y).setElevation(elev);
        }
    }

    // Steep slope region - 1 elevation per 2 tiles for steeper slope
    for (std::uint16_t y = 80; y < 128; ++y) {
        for (std::uint16_t x = 0; x < grid.width; ++x) {
            std::uint8_t elev = static_cast<std::uint8_t>(20 + (y - 80) / 2);
            if (elev > 31) elev = 31;
            grid.at(x, y).setElevation(elev);
        }
    }

    // Test flat region
    NormalResult flat_normal = computeTerrainNormal(grid, 64, 20);
    TEST_ASSERT(isNormalFlat(flat_normal), "Flat region produces flat normal");

    // Test gentle slope region at y=50 (middle of the slope region)
    // Neighbors: y=49 -> elev=10+(49-40)/4=12, y=51 -> elev=10+(51-40)/4=12
    // Actually at y=52: y=51 -> elev=12, y=53 -> elev=13
    NormalResult gentle_normal = computeTerrainNormal(grid, 64, 52);
    float gentle_angle = calculateSlopeAngle(gentle_normal);
    TEST_ASSERT(gentle_angle >= 0.0f && gentle_angle < 1.0f,
                "Gentle slope has small angle (< 1 rad)");

    // Test steep slope region at y=92
    // Neighbors: y=91 -> elev=20+(91-80)/2=25, y=93 -> elev=20+(93-80)/2=26
    NormalResult steep_normal = computeTerrainNormal(grid, 64, 92);
    float steep_angle = calculateSlopeAngle(steep_normal);
    TEST_ASSERT(steep_angle > 0.0f, "Steep slope has positive angle");
    TEST_ASSERT(steep_angle >= gentle_angle, "Steep slope angle >= gentle slope angle");
}

// ============================================================================
// Test: Template version with custom sampler
// ============================================================================
void test_custom_sampler() {
    printf("\n--- Test: Custom Sampler Template ---\n");

    // Test the template version with a simple lambda sampler
    auto sampler = [](std::int32_t x, std::int32_t z) -> float {
        // Create a simple slope: elevation = x * 0.1
        return static_cast<float>(x) * 0.1f * ELEVATION_HEIGHT;
    };

    NormalResult normal = computeTerrainNormalWithSampler(
        sampler, 64, 64, 128, 128);

    // With slope increasing in X, normal should point toward -X
    TEST_ASSERT(normal.nx < 0.0f, "Custom sampler: X-slope points toward -X");
    TEST_ASSERT(normal.ny > 0.0f, "Custom sampler: Normal has positive Y");
    TEST_ASSERT_FLOAT_NEAR(normal.nz, 0.0f, 0.001f, "Custom sampler: No Z slope");

    // Verify normalized
    float length = std::sqrt(normal.nx * normal.nx + normal.ny * normal.ny + normal.nz * normal.nz);
    TEST_ASSERT_FLOAT_NEAR(length, 1.0f, 0.0001f, "Custom sampler: Normal is unit length");
}

// ============================================================================
// Test: Large map (512x512)
// ============================================================================
void test_large_map() {
    printf("\n--- Test: Large Map (512x512) ---\n");

    TerrainGrid grid(MapSize::Large);

    // Fill with varied terrain
    for (std::uint16_t y = 0; y < grid.height; ++y) {
        for (std::uint16_t x = 0; x < grid.width; ++x) {
            std::uint8_t elev = static_cast<std::uint8_t>((x + y) / 32);
            if (elev > 31) elev = 31;
            grid.at(x, y).setElevation(elev);
        }
    }

    // Test various positions
    NormalResult center = computeTerrainNormal(grid, 256, 256);
    NormalResult corner = computeTerrainNormal(grid, 0, 0);
    NormalResult edge = computeTerrainNormal(grid, 511, 256);

    // All should be valid (not NaN)
    TEST_ASSERT(!std::isnan(center.ny), "Large map center normal valid");
    TEST_ASSERT(!std::isnan(corner.ny), "Large map corner normal valid");
    TEST_ASSERT(!std::isnan(edge.ny), "Large map edge normal valid");

    // All should be unit length
    float center_len = std::sqrt(center.nx * center.nx + center.ny * center.ny + center.nz * center.nz);
    TEST_ASSERT_FLOAT_NEAR(center_len, 1.0f, 0.0001f, "Large map center normal unit length");
}

// ============================================================================
// Test: Elevation extremes
// ============================================================================
void test_elevation_extremes() {
    printf("\n--- Test: Elevation Extremes ---\n");

    TerrainGrid grid(MapSize::Small);

    // Test with maximum elevation difference between neighbors
    // Center at elevation 0, neighbors at 31
    for (std::uint16_t y = 0; y < grid.height; ++y) {
        for (std::uint16_t x = 0; x < grid.width; ++x) {
            grid.at(x, y).setElevation(31);  // All at max
        }
    }
    grid.at(64, 64).setElevation(0);  // Center at 0

    // Get normal at center - surrounded by high terrain
    NormalResult normal = computeTerrainNormal(grid, 64, 64);

    // Should be a valid normal (pointing up since neighbors are higher but center is lower)
    TEST_ASSERT(!std::isnan(normal.nx), "Extreme elevation: nx not NaN");
    TEST_ASSERT(!std::isnan(normal.ny), "Extreme elevation: ny not NaN");
    TEST_ASSERT(!std::isnan(normal.nz), "Extreme elevation: nz not NaN");

    float length = std::sqrt(normal.nx * normal.nx + normal.ny * normal.ny + normal.nz * normal.nz);
    TEST_ASSERT_FLOAT_NEAR(length, 1.0f, 0.0001f, "Extreme elevation: unit length");
}

// ============================================================================
// Main
// ============================================================================
int main() {
    printf("===========================================\n");
    printf("TerrainNormals Unit Tests (Ticket 3-024)\n");
    printf("===========================================\n");

    test_flat_terrain_normal();
    test_slope_x_direction();
    test_slope_z_direction();
    test_diagonal_slope();
    test_map_edge_x_zero();
    test_map_edge_x_max();
    test_map_edge_z_zero();
    test_map_corner();
    test_chunk_boundary();
    test_central_differences_formula();
    test_slope_angle();
    test_sample_elevation_clamped();
    test_toon_shader_banding();
    test_custom_sampler();
    test_large_map();
    test_elevation_extremes();

    printf("\n===========================================\n");
    printf("Results: %d passed, %d failed\n", tests_passed, tests_failed);
    printf("===========================================\n");

    return tests_failed > 0 ? 1 : 0;
}
