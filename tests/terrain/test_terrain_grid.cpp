/**
 * @file test_terrain_grid.cpp
 * @brief Unit tests for TerrainGrid.h (Ticket 3-002)
 *
 * Tests cover:
 * - TerrainGrid construction with different map sizes
 * - Row-major storage verification (index = y * width + x)
 * - at(x, y) accessor with coordinate access
 * - in_bounds(x, y) for range checking
 * - Edge tile access (corners and borders)
 * - Memory budget verification for all map sizes
 * - Sea level configuration
 */

#include <sims3000/terrain/TerrainGrid.h>
#include <cstdio>
#include <cstdlib>

using namespace sims3000::terrain;

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
// Construction Tests
// =============================================================================

TEST(default_construction) {
    TerrainGrid grid;
    ASSERT_EQ(grid.width, 0);
    ASSERT_EQ(grid.height, 0);
    ASSERT_EQ(grid.sea_level, DEFAULT_SEA_LEVEL);
    ASSERT(grid.tiles.empty());
    ASSERT(grid.empty());
}

TEST(mapsize_small_construction) {
    TerrainGrid grid(MapSize::Small);
    ASSERT_EQ(grid.width, 128);
    ASSERT_EQ(grid.height, 128);
    ASSERT_EQ(grid.sea_level, DEFAULT_SEA_LEVEL);
    ASSERT_EQ(grid.tiles.size(), 128 * 128);
    ASSERT(!grid.empty());
}

TEST(mapsize_medium_construction) {
    TerrainGrid grid(MapSize::Medium);
    ASSERT_EQ(grid.width, 256);
    ASSERT_EQ(grid.height, 256);
    ASSERT_EQ(grid.sea_level, DEFAULT_SEA_LEVEL);
    ASSERT_EQ(grid.tiles.size(), 256 * 256);
    ASSERT(!grid.empty());
}

TEST(mapsize_large_construction) {
    TerrainGrid grid(MapSize::Large);
    ASSERT_EQ(grid.width, 512);
    ASSERT_EQ(grid.height, 512);
    ASSERT_EQ(grid.sea_level, DEFAULT_SEA_LEVEL);
    ASSERT_EQ(grid.tiles.size(), 512 * 512);
    ASSERT(!grid.empty());
}

TEST(explicit_dimension_construction) {
    TerrainGrid grid(256, 256);
    ASSERT_EQ(grid.width, 256);
    ASSERT_EQ(grid.height, 256);
    ASSERT_EQ(grid.tiles.size(), 256 * 256);
}

TEST(custom_sea_level_construction) {
    TerrainGrid grid(MapSize::Medium, 12);
    ASSERT_EQ(grid.sea_level, 12);
}

TEST(initialize_reinitializes) {
    TerrainGrid grid(MapSize::Small);
    ASSERT_EQ(grid.width, 128);

    grid.initialize(MapSize::Large);
    ASSERT_EQ(grid.width, 512);
    ASSERT_EQ(grid.height, 512);
    ASSERT_EQ(grid.tiles.size(), 512 * 512);
}

TEST(initialize_with_custom_sea_level) {
    TerrainGrid grid;
    grid.initialize(MapSize::Medium, 15);
    ASSERT_EQ(grid.sea_level, 15);
}

// =============================================================================
// Map Size Validation Tests
// =============================================================================

TEST(valid_map_sizes) {
    ASSERT(isValidMapSize(128));
    ASSERT(isValidMapSize(256));
    ASSERT(isValidMapSize(512));
}

TEST(invalid_map_sizes) {
    ASSERT(!isValidMapSize(0));
    ASSERT(!isValidMapSize(64));
    ASSERT(!isValidMapSize(100));
    ASSERT(!isValidMapSize(200));
    ASSERT(!isValidMapSize(300));
    ASSERT(!isValidMapSize(1024));
}

// =============================================================================
// Row-Major Storage Tests (index = y * width + x)
// =============================================================================

TEST(row_major_index_calculation) {
    TerrainGrid grid(MapSize::Small);  // 128x128

    // First tile (0, 0) -> index 0
    ASSERT_EQ(grid.index_of(0, 0), 0);

    // End of first row (127, 0) -> index 127
    ASSERT_EQ(grid.index_of(127, 0), 127);

    // Start of second row (0, 1) -> index 128
    ASSERT_EQ(grid.index_of(0, 1), 128);

    // Tile (5, 3) -> index = 3 * 128 + 5 = 389
    ASSERT_EQ(grid.index_of(5, 3), 389);

    // Last tile (127, 127) -> index = 127 * 128 + 127 = 16383
    ASSERT_EQ(grid.index_of(127, 127), 16383);
}

TEST(row_major_coords_of_inverse) {
    TerrainGrid grid(MapSize::Small);

    std::uint16_t x, y;

    // Index 0 -> (0, 0)
    grid.coords_of(0, x, y);
    ASSERT_EQ(x, 0);
    ASSERT_EQ(y, 0);

    // Index 127 -> (127, 0)
    grid.coords_of(127, x, y);
    ASSERT_EQ(x, 127);
    ASSERT_EQ(y, 0);

    // Index 128 -> (0, 1)
    grid.coords_of(128, x, y);
    ASSERT_EQ(x, 0);
    ASSERT_EQ(y, 1);

    // Index 389 -> (5, 3)
    grid.coords_of(389, x, y);
    ASSERT_EQ(x, 5);
    ASSERT_EQ(y, 3);
}

TEST(row_major_roundtrip) {
    TerrainGrid grid(MapSize::Medium);

    // Test that index_of and coords_of are inverses
    for (std::uint16_t test_y = 0; test_y < 256; test_y += 31) {
        for (std::uint16_t test_x = 0; test_x < 256; test_x += 37) {
            std::size_t idx = grid.index_of(test_x, test_y);
            std::uint16_t rx, ry;
            grid.coords_of(idx, rx, ry);
            ASSERT_EQ(rx, test_x);
            ASSERT_EQ(ry, test_y);
        }
    }
}

// =============================================================================
// at() Accessor Tests
// =============================================================================

TEST(at_read_write) {
    TerrainGrid grid(MapSize::Small);

    // Write to specific tile
    grid.at(10, 20).setTerrainType(TerrainType::Ridge);
    grid.at(10, 20).setElevation(15);

    // Read back
    ASSERT_EQ(grid.at(10, 20).getTerrainType(), TerrainType::Ridge);
    ASSERT_EQ(grid.at(10, 20).getElevation(), 15);
}

TEST(at_const_access) {
    TerrainGrid grid(MapSize::Small);
    grid.at(5, 5).setTerrainType(TerrainType::DeepVoid);

    const TerrainGrid& const_grid = grid;
    ASSERT_EQ(const_grid.at(5, 5).getTerrainType(), TerrainType::DeepVoid);
}

TEST(at_signed_coordinates) {
    TerrainGrid grid(MapSize::Small);

    // Use signed int32_t coordinates
    std::int32_t x = 50;
    std::int32_t y = 60;

    grid.at(x, y).moisture = 200;
    ASSERT_EQ(grid.at(x, y).moisture, 200);
}

TEST(at_matches_direct_index) {
    TerrainGrid grid(MapSize::Medium);

    std::uint16_t x = 100, y = 150;

    // Write via at()
    grid.at(x, y).setTerrainType(TerrainType::BiolumeGrove);

    // Verify via direct index
    std::size_t idx = static_cast<std::size_t>(y) * grid.width + x;
    ASSERT_EQ(grid.tiles[idx].getTerrainType(), TerrainType::BiolumeGrove);
}

// =============================================================================
// in_bounds() Tests
// =============================================================================

TEST(in_bounds_valid_coordinates) {
    TerrainGrid grid(MapSize::Small);  // 128x128

    ASSERT(grid.in_bounds(0, 0));
    ASSERT(grid.in_bounds(127, 0));
    ASSERT(grid.in_bounds(0, 127));
    ASSERT(grid.in_bounds(127, 127));
    ASSERT(grid.in_bounds(64, 64));
}

TEST(in_bounds_negative_coordinates) {
    TerrainGrid grid(MapSize::Small);

    ASSERT(!grid.in_bounds(-1, 0));
    ASSERT(!grid.in_bounds(0, -1));
    ASSERT(!grid.in_bounds(-1, -1));
    ASSERT(!grid.in_bounds(-100, 50));
}

TEST(in_bounds_out_of_range) {
    TerrainGrid grid(MapSize::Small);  // 128x128

    ASSERT(!grid.in_bounds(128, 0));
    ASSERT(!grid.in_bounds(0, 128));
    ASSERT(!grid.in_bounds(128, 128));
    ASSERT(!grid.in_bounds(200, 50));
    ASSERT(!grid.in_bounds(50, 200));
}

TEST(in_bounds_different_sizes) {
    {
        TerrainGrid grid(MapSize::Small);  // 128
        ASSERT(grid.in_bounds(127, 127));
        ASSERT(!grid.in_bounds(128, 127));
    }
    {
        TerrainGrid grid(MapSize::Medium);  // 256
        ASSERT(grid.in_bounds(255, 255));
        ASSERT(!grid.in_bounds(256, 255));
    }
    {
        TerrainGrid grid(MapSize::Large);  // 512
        ASSERT(grid.in_bounds(511, 511));
        ASSERT(!grid.in_bounds(512, 511));
    }
}

// =============================================================================
// Edge Tile Access Tests
// =============================================================================

TEST(corner_tile_access) {
    TerrainGrid grid(MapSize::Small);  // 128x128

    // Top-left corner (0, 0)
    grid.at(0, 0).setTerrainType(TerrainType::DeepVoid);
    ASSERT_EQ(grid.at(0, 0).getTerrainType(), TerrainType::DeepVoid);

    // Top-right corner (127, 0)
    grid.at(127, 0).setTerrainType(TerrainType::EmberCrust);
    ASSERT_EQ(grid.at(127, 0).getTerrainType(), TerrainType::EmberCrust);

    // Bottom-left corner (0, 127)
    grid.at(0, 127).setTerrainType(TerrainType::PrismaFields);
    ASSERT_EQ(grid.at(0, 127).getTerrainType(), TerrainType::PrismaFields);

    // Bottom-right corner (127, 127)
    grid.at(127, 127).setTerrainType(TerrainType::SporeFlats);
    ASSERT_EQ(grid.at(127, 127).getTerrainType(), TerrainType::SporeFlats);
}

TEST(border_tile_iteration) {
    TerrainGrid grid(MapSize::Small);  // 128x128

    // Set all top border tiles
    for (std::uint16_t x = 0; x < 128; ++x) {
        grid.at(x, 0).setTerrainType(TerrainType::DeepVoid);
    }

    // Verify top border
    for (std::uint16_t x = 0; x < 128; ++x) {
        ASSERT_EQ(grid.at(x, 0).getTerrainType(), TerrainType::DeepVoid);
    }

    // Set all left border tiles
    for (std::uint16_t y = 0; y < 128; ++y) {
        grid.at(0, y).setTerrainType(TerrainType::FlowChannel);
    }

    // Verify left border (note: corner (0,0) was overwritten)
    for (std::uint16_t y = 1; y < 128; ++y) {
        ASSERT_EQ(grid.at(0, y).getTerrainType(), TerrainType::FlowChannel);
    }
}

TEST(large_map_edge_access) {
    TerrainGrid grid(MapSize::Large);  // 512x512

    // Access far corners
    grid.at(511, 511).setTerrainType(TerrainType::BlightMires);
    ASSERT_EQ(grid.at(511, 511).getTerrainType(), TerrainType::BlightMires);

    grid.at(0, 511).setElevation(25);
    ASSERT_EQ(grid.at(0, 511).getElevation(), 25);

    grid.at(511, 0).moisture = 180;
    ASSERT_EQ(grid.at(511, 0).moisture, 180);
}

// =============================================================================
// Memory Budget Verification Tests
// =============================================================================

TEST(memory_budget_small) {
    TerrainGrid grid(MapSize::Small);  // 128x128

    // 128 * 128 = 16,384 tiles
    ASSERT_EQ(grid.tile_count(), 16384);

    // 16,384 * 4 bytes = 65,536 bytes = 64KB
    ASSERT_EQ(grid.memory_bytes(), 65536);
}

TEST(memory_budget_medium) {
    TerrainGrid grid(MapSize::Medium);  // 256x256

    // 256 * 256 = 65,536 tiles
    ASSERT_EQ(grid.tile_count(), 65536);

    // 65,536 * 4 bytes = 262,144 bytes = 256KB
    ASSERT_EQ(grid.memory_bytes(), 262144);
}

TEST(memory_budget_large) {
    TerrainGrid grid(MapSize::Large);  // 512x512

    // 512 * 512 = 262,144 tiles
    ASSERT_EQ(grid.tile_count(), 262144);

    // 262,144 * 4 bytes = 1,048,576 bytes = 1MB
    ASSERT_EQ(grid.memory_bytes(), 1048576);
}

TEST(terrain_component_size_verification) {
    // Critical: TerrainComponent must be exactly 4 bytes
    // This is checked in TerrainTypes.h but verify here too
    ASSERT_EQ(sizeof(TerrainComponent), 4);
}

// =============================================================================
// Sea Level Tests
// =============================================================================

TEST(default_sea_level) {
    ASSERT_EQ(DEFAULT_SEA_LEVEL, 8);
}

TEST(sea_level_preserved) {
    TerrainGrid grid(MapSize::Medium, 5);
    ASSERT_EQ(grid.sea_level, 5);

    // Verify sea level is independent of grid data
    grid.at(100, 100).setElevation(3);
    ASSERT_EQ(grid.sea_level, 5);
}

// =============================================================================
// Utility Method Tests
// =============================================================================

TEST(fill_all_tiles) {
    TerrainGrid grid(MapSize::Small);

    TerrainComponent tc = {};
    tc.setTerrainType(TerrainType::Substrate);
    tc.setElevation(10);
    tc.moisture = 100;

    grid.fill(tc);

    // Verify a sampling of tiles
    ASSERT_EQ(grid.at(0, 0).getTerrainType(), TerrainType::Substrate);
    ASSERT_EQ(grid.at(64, 64).getElevation(), 10);
    ASSERT_EQ(grid.at(127, 127).moisture, 100);
}

TEST(fill_type_convenience) {
    TerrainGrid grid(MapSize::Small);

    grid.fill_type(TerrainType::DeepVoid);

    // Verify type set across grid
    ASSERT_EQ(grid.at(0, 0).getTerrainType(), TerrainType::DeepVoid);
    ASSERT_EQ(grid.at(50, 50).getTerrainType(), TerrainType::DeepVoid);
    ASSERT_EQ(grid.at(127, 127).getTerrainType(), TerrainType::DeepVoid);

    // Verify other fields are zeroed
    ASSERT_EQ(grid.at(50, 50).elevation, 0);
    ASSERT_EQ(grid.at(50, 50).moisture, 0);
    ASSERT_EQ(grid.at(50, 50).flags, 0);
}

TEST(empty_check) {
    TerrainGrid empty_grid;
    ASSERT(empty_grid.empty());

    TerrainGrid initialized_grid(MapSize::Small);
    ASSERT(!initialized_grid.empty());
}

// =============================================================================
// Typical Usage Pattern Tests
// =============================================================================

TEST(row_iteration_pattern) {
    // Simulate typical terrain generation: row-by-row iteration
    TerrainGrid grid(MapSize::Small);

    for (std::uint16_t y = 0; y < grid.height; ++y) {
        for (std::uint16_t x = 0; x < grid.width; ++x) {
            // Simple gradient pattern
            grid.at(x, y).setElevation(static_cast<std::uint8_t>((x + y) % 32));
            grid.at(x, y).moisture = static_cast<std::uint8_t>((x * y) % 256);
        }
    }

    // Verify pattern
    ASSERT_EQ(grid.at(10, 5).getElevation(), (10 + 5) % 32);
    ASSERT_EQ(grid.at(10, 5).moisture, (10 * 5) % 256);
}

TEST(coastal_detection_pattern) {
    // Simulate marking coastal tiles based on neighbors
    TerrainGrid grid(MapSize::Small);

    // Set ocean border
    for (std::uint16_t x = 0; x < grid.width; ++x) {
        grid.at(x, 0).setTerrainType(TerrainType::DeepVoid);
    }

    // Mark tiles at y=1 as coastal (adjacent to ocean)
    for (std::uint16_t x = 0; x < grid.width; ++x) {
        if (grid.at(x, 0).getTerrainType() == TerrainType::DeepVoid) {
            if (grid.in_bounds(x, 1)) {
                grid.at(x, 1).setCoastal(true);
            }
        }
    }

    // Verify
    ASSERT(grid.at(50, 1).isCoastal());
    ASSERT(!grid.at(50, 50).isCoastal());
}

// =============================================================================
// Main Entry Point
// =============================================================================

int main() {
    printf("=== TerrainGrid Unit Tests (Ticket 3-002) ===\n\n");

    // Construction tests
    RUN_TEST(default_construction);
    RUN_TEST(mapsize_small_construction);
    RUN_TEST(mapsize_medium_construction);
    RUN_TEST(mapsize_large_construction);
    RUN_TEST(explicit_dimension_construction);
    RUN_TEST(custom_sea_level_construction);
    RUN_TEST(initialize_reinitializes);
    RUN_TEST(initialize_with_custom_sea_level);

    // Map size validation tests
    RUN_TEST(valid_map_sizes);
    RUN_TEST(invalid_map_sizes);

    // Row-major storage tests
    RUN_TEST(row_major_index_calculation);
    RUN_TEST(row_major_coords_of_inverse);
    RUN_TEST(row_major_roundtrip);

    // at() accessor tests
    RUN_TEST(at_read_write);
    RUN_TEST(at_const_access);
    RUN_TEST(at_signed_coordinates);
    RUN_TEST(at_matches_direct_index);

    // in_bounds() tests
    RUN_TEST(in_bounds_valid_coordinates);
    RUN_TEST(in_bounds_negative_coordinates);
    RUN_TEST(in_bounds_out_of_range);
    RUN_TEST(in_bounds_different_sizes);

    // Edge tile access tests
    RUN_TEST(corner_tile_access);
    RUN_TEST(border_tile_iteration);
    RUN_TEST(large_map_edge_access);

    // Memory budget verification tests
    RUN_TEST(memory_budget_small);
    RUN_TEST(memory_budget_medium);
    RUN_TEST(memory_budget_large);
    RUN_TEST(terrain_component_size_verification);

    // Sea level tests
    RUN_TEST(default_sea_level);
    RUN_TEST(sea_level_preserved);

    // Utility method tests
    RUN_TEST(fill_all_tiles);
    RUN_TEST(fill_type_convenience);
    RUN_TEST(empty_check);

    // Typical usage pattern tests
    RUN_TEST(row_iteration_pattern);
    RUN_TEST(coastal_detection_pattern);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
