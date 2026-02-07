/**
 * @file test_water_distance_field.cpp
 * @brief Unit tests for WaterDistanceField.h (Ticket 3-006)
 *
 * Tests cover:
 * - WaterDistanceField construction with different map sizes
 * - Memory budget verification (1 byte per tile)
 * - Multi-source BFS computation
 * - Water tile distance = 0
 * - Manhattan distance correctness
 * - Distance capping at 255
 * - get_water_distance() O(1) query
 * - Recomputation on water body changes
 * - Performance verification for 512x512
 * - Edge cases: all water, no water, single water tile
 */

#include <sims3000/terrain/WaterDistanceField.h>
#include <cstdio>
#include <cstdlib>
#include <chrono>
#include <cmath>

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
        printf("\n  FAILED: %s == %s (got %d vs %d, line %d)\n", #a, #b, static_cast<int>(a), static_cast<int>(b), __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

// =============================================================================
// Construction Tests
// =============================================================================

TEST(default_construction) {
    WaterDistanceField field;
    ASSERT_EQ(field.width, 0);
    ASSERT_EQ(field.height, 0);
    ASSERT(field.distances.empty());
    ASSERT(field.empty());
}

TEST(mapsize_small_construction) {
    WaterDistanceField field(MapSize::Small);
    ASSERT_EQ(field.width, 128);
    ASSERT_EQ(field.height, 128);
    ASSERT_EQ(field.distances.size(), 128 * 128);
    ASSERT(!field.empty());
}

TEST(mapsize_medium_construction) {
    WaterDistanceField field(MapSize::Medium);
    ASSERT_EQ(field.width, 256);
    ASSERT_EQ(field.height, 256);
    ASSERT_EQ(field.distances.size(), 256 * 256);
    ASSERT(!field.empty());
}

TEST(mapsize_large_construction) {
    WaterDistanceField field(MapSize::Large);
    ASSERT_EQ(field.width, 512);
    ASSERT_EQ(field.height, 512);
    ASSERT_EQ(field.distances.size(), 512 * 512);
    ASSERT(!field.empty());
}

TEST(explicit_dimension_construction) {
    WaterDistanceField field(256, 256);
    ASSERT_EQ(field.width, 256);
    ASSERT_EQ(field.height, 256);
    ASSERT_EQ(field.distances.size(), 256 * 256);
}

TEST(initialize_reinitializes) {
    WaterDistanceField field(MapSize::Small);
    ASSERT_EQ(field.width, 128);

    field.initialize(MapSize::Large);
    ASSERT_EQ(field.width, 512);
    ASSERT_EQ(field.height, 512);
    ASSERT_EQ(field.distances.size(), 512 * 512);
}

// =============================================================================
// Memory Budget Verification Tests
// =============================================================================

TEST(memory_budget_small) {
    WaterDistanceField field(MapSize::Small);  // 128x128

    // 128 * 128 = 16,384 tiles
    ASSERT_EQ(field.tile_count(), 16384);

    // 16,384 * 1 byte = 16,384 bytes (16KB)
    ASSERT_EQ(field.memory_bytes(), 16384);
}

TEST(memory_budget_medium) {
    WaterDistanceField field(MapSize::Medium);  // 256x256

    // 256 * 256 = 65,536 tiles
    ASSERT_EQ(field.tile_count(), 65536);

    // 65,536 * 1 byte = 65,536 bytes (64KB)
    ASSERT_EQ(field.memory_bytes(), 65536);
}

TEST(memory_budget_large) {
    WaterDistanceField field(MapSize::Large);  // 512x512

    // 512 * 512 = 262,144 tiles
    ASSERT_EQ(field.tile_count(), 262144);

    // 262,144 * 1 byte = 262,144 bytes (256KB)
    ASSERT_EQ(field.memory_bytes(), 262144);
}

TEST(storage_type_size_verification) {
    // Distance storage must be exactly 1 byte per tile
    ASSERT_EQ(sizeof(std::uint8_t), 1);
}

// =============================================================================
// Water Type Detection Tests
// =============================================================================

TEST(water_type_detection_deep_void) {
    ASSERT(WaterDistanceField::is_water_type(TerrainType::DeepVoid));
}

TEST(water_type_detection_flow_channel) {
    ASSERT(WaterDistanceField::is_water_type(TerrainType::FlowChannel));
}

TEST(water_type_detection_still_basin) {
    ASSERT(WaterDistanceField::is_water_type(TerrainType::StillBasin));
}

TEST(water_type_detection_non_water) {
    ASSERT(!WaterDistanceField::is_water_type(TerrainType::Substrate));
    ASSERT(!WaterDistanceField::is_water_type(TerrainType::Ridge));
    ASSERT(!WaterDistanceField::is_water_type(TerrainType::BiolumeGrove));
    ASSERT(!WaterDistanceField::is_water_type(TerrainType::PrismaFields));
    ASSERT(!WaterDistanceField::is_water_type(TerrainType::SporeFlats));
    ASSERT(!WaterDistanceField::is_water_type(TerrainType::BlightMires));
    ASSERT(!WaterDistanceField::is_water_type(TerrainType::EmberCrust));
}

// =============================================================================
// Multi-Source BFS Computation Tests
// =============================================================================

TEST(water_tile_distance_zero) {
    // Create terrain with a water tile
    TerrainGrid terrain(MapSize::Small);
    terrain.fill_type(TerrainType::Substrate);
    terrain.at(64, 64).setTerrainType(TerrainType::DeepVoid);

    WaterDistanceField field(MapSize::Small);
    field.compute(terrain);

    // Water tile should have distance 0
    ASSERT_EQ(field.get_water_distance(64, 64), 0);
}

TEST(adjacent_tile_distance_one) {
    // Create terrain with a water tile at center
    TerrainGrid terrain(MapSize::Small);
    terrain.fill_type(TerrainType::Substrate);
    terrain.at(64, 64).setTerrainType(TerrainType::StillBasin);

    WaterDistanceField field(MapSize::Small);
    field.compute(terrain);

    // 4-connected neighbors should have distance 1
    ASSERT_EQ(field.get_water_distance(65, 64), 1);  // East
    ASSERT_EQ(field.get_water_distance(63, 64), 1);  // West
    ASSERT_EQ(field.get_water_distance(64, 65), 1);  // South
    ASSERT_EQ(field.get_water_distance(64, 63), 1);  // North
}

TEST(manhattan_distance_correctness) {
    // Create terrain with a single water tile at (10, 10)
    TerrainGrid terrain(MapSize::Small);
    terrain.fill_type(TerrainType::Substrate);
    terrain.at(10, 10).setTerrainType(TerrainType::FlowChannel);

    WaterDistanceField field(MapSize::Small);
    field.compute(terrain);

    // Test Manhattan distances
    // Distance = |dx| + |dy|
    ASSERT_EQ(field.get_water_distance(10, 10), 0);   // Water tile
    ASSERT_EQ(field.get_water_distance(11, 10), 1);   // |1| + |0| = 1
    ASSERT_EQ(field.get_water_distance(12, 10), 2);   // |2| + |0| = 2
    ASSERT_EQ(field.get_water_distance(15, 10), 5);   // |5| + |0| = 5
    ASSERT_EQ(field.get_water_distance(10, 15), 5);   // |0| + |5| = 5
    ASSERT_EQ(field.get_water_distance(13, 13), 6);   // |3| + |3| = 6
    ASSERT_EQ(field.get_water_distance(20, 25), 25);  // |10| + |15| = 25
}

TEST(multi_source_bfs_nearest_water) {
    // Create terrain with two water tiles
    TerrainGrid terrain(MapSize::Small);
    terrain.fill_type(TerrainType::Substrate);
    terrain.at(10, 50).setTerrainType(TerrainType::DeepVoid);    // Water at (10, 50)
    terrain.at(100, 50).setTerrainType(TerrainType::DeepVoid);   // Water at (100, 50)

    WaterDistanceField field(MapSize::Small);
    field.compute(terrain);

    // Tile at (50, 50) should be distance 40 from left water (closer than 50 from right)
    // Distance to (10,50) = |50-10| + |50-50| = 40
    // Distance to (100,50) = |50-100| + |50-50| = 50
    ASSERT_EQ(field.get_water_distance(50, 50), 40);

    // Tile at (60, 50) is equidistant: 50 from left, 40 from right
    // Should be 40 (minimum)
    ASSERT_EQ(field.get_water_distance(60, 50), 40);
}

TEST(ocean_border_distances) {
    // Create terrain with ocean on top edge
    TerrainGrid terrain(MapSize::Small);
    terrain.fill_type(TerrainType::Substrate);
    for (std::uint16_t x = 0; x < 128; ++x) {
        terrain.at(x, 0).setTerrainType(TerrainType::DeepVoid);
    }

    WaterDistanceField field(MapSize::Small);
    field.compute(terrain);

    // Top row (y=0) should all be distance 0
    for (std::uint16_t x = 0; x < 128; ++x) {
        ASSERT_EQ(field.get_water_distance(x, 0), 0);
    }

    // Row y=1 should all be distance 1
    for (std::uint16_t x = 0; x < 128; ++x) {
        ASSERT_EQ(field.get_water_distance(x, 1), 1);
    }

    // Row y=10 should all be distance 10
    for (std::uint16_t x = 0; x < 128; ++x) {
        ASSERT_EQ(field.get_water_distance(x, 10), 10);
    }

    // Bottom row (y=127) should be distance 127
    ASSERT_EQ(field.get_water_distance(64, 127), 127);
}

TEST(river_distances) {
    // Create terrain with a vertical river at x=64
    TerrainGrid terrain(MapSize::Small);
    terrain.fill_type(TerrainType::Substrate);
    for (std::uint16_t y = 0; y < 128; ++y) {
        terrain.at(64, y).setTerrainType(TerrainType::FlowChannel);
    }

    WaterDistanceField field(MapSize::Small);
    field.compute(terrain);

    // River tiles should be distance 0
    ASSERT_EQ(field.get_water_distance(64, 0), 0);
    ASSERT_EQ(field.get_water_distance(64, 64), 0);
    ASSERT_EQ(field.get_water_distance(64, 127), 0);

    // Tiles at x=0 should be distance 64
    ASSERT_EQ(field.get_water_distance(0, 50), 64);

    // Tiles at x=127 should be distance 63
    ASSERT_EQ(field.get_water_distance(127, 50), 63);
}

// =============================================================================
// Distance Capping Tests
// =============================================================================

TEST(distance_capping_at_255) {
    // Create small terrain (128x128) with single water tile in corner
    // Max possible distance is 127+127=254, within uint8 range
    TerrainGrid terrain(MapSize::Small);
    terrain.fill_type(TerrainType::Substrate);
    terrain.at(0, 0).setTerrainType(TerrainType::DeepVoid);

    WaterDistanceField field(MapSize::Small);
    field.compute(terrain);

    // Far corner should be distance 127+127 = 254
    ASSERT_EQ(field.get_water_distance(127, 127), 254);
}

TEST(distance_capping_large_map) {
    // Create large terrain (512x512) with single water tile in corner
    // Max possible Manhattan distance is 511+511=1022, but capped at 255
    TerrainGrid terrain(MapSize::Large);
    terrain.fill_type(TerrainType::Substrate);
    terrain.at(0, 0).setTerrainType(TerrainType::DeepVoid);

    WaterDistanceField field(MapSize::Large);
    field.compute(terrain);

    // Near water should have correct distances
    ASSERT_EQ(field.get_water_distance(0, 0), 0);
    ASSERT_EQ(field.get_water_distance(1, 0), 1);
    ASSERT_EQ(field.get_water_distance(100, 100), 200);
    ASSERT_EQ(field.get_water_distance(127, 127), 254);

    // Far corner should be capped at 255
    ASSERT_EQ(field.get_water_distance(511, 511), 255);
    ASSERT_EQ(field.get_water_distance(400, 400), 255);
    ASSERT_EQ(field.get_water_distance(300, 300), 255);  // 600 > 255
}

// =============================================================================
// Edge Case Tests
// =============================================================================

TEST(all_water_map) {
    // Create terrain that is entirely water
    TerrainGrid terrain(MapSize::Small);
    terrain.fill_type(TerrainType::DeepVoid);

    WaterDistanceField field(MapSize::Small);
    field.compute(terrain);

    // All tiles should have distance 0
    for (std::uint16_t y = 0; y < 128; y += 17) {
        for (std::uint16_t x = 0; x < 128; x += 19) {
            ASSERT_EQ(field.get_water_distance(x, y), 0);
        }
    }
}

TEST(no_water_map) {
    // Create terrain with no water
    TerrainGrid terrain(MapSize::Small);
    terrain.fill_type(TerrainType::Substrate);

    WaterDistanceField field(MapSize::Small);
    field.compute(terrain);

    // All tiles should have distance 255 (max)
    for (std::uint16_t y = 0; y < 128; y += 17) {
        for (std::uint16_t x = 0; x < 128; x += 19) {
            ASSERT_EQ(field.get_water_distance(x, y), 255);
        }
    }
}

TEST(single_water_tile_center) {
    TerrainGrid terrain(MapSize::Small);
    terrain.fill_type(TerrainType::Substrate);
    terrain.at(64, 64).setTerrainType(TerrainType::StillBasin);

    WaterDistanceField field(MapSize::Small);
    field.compute(terrain);

    // Verify concentric Manhattan distance rings
    // Ring at distance 2
    ASSERT_EQ(field.get_water_distance(66, 64), 2);
    ASSERT_EQ(field.get_water_distance(65, 65), 2);
    ASSERT_EQ(field.get_water_distance(64, 66), 2);

    // Ring at distance 10
    ASSERT_EQ(field.get_water_distance(74, 64), 10);
    ASSERT_EQ(field.get_water_distance(64, 74), 10);
    ASSERT_EQ(field.get_water_distance(69, 69), 10);
}

TEST(corner_water_tile) {
    TerrainGrid terrain(MapSize::Small);
    terrain.fill_type(TerrainType::Substrate);
    terrain.at(0, 0).setTerrainType(TerrainType::DeepVoid);

    WaterDistanceField field(MapSize::Small);
    field.compute(terrain);

    // Corner tile is water
    ASSERT_EQ(field.get_water_distance(0, 0), 0);

    // Adjacent tiles
    ASSERT_EQ(field.get_water_distance(1, 0), 1);
    ASSERT_EQ(field.get_water_distance(0, 1), 1);
    ASSERT_EQ(field.get_water_distance(1, 1), 2);
}

// =============================================================================
// Recomputation Tests
// =============================================================================

TEST(recomputation_on_water_change) {
    TerrainGrid terrain(MapSize::Small);
    terrain.fill_type(TerrainType::Substrate);
    terrain.at(10, 10).setTerrainType(TerrainType::StillBasin);

    WaterDistanceField field(MapSize::Small);
    field.compute(terrain);

    // Initial: tile at (20, 10) is distance 10 from water at (10, 10)
    ASSERT_EQ(field.get_water_distance(20, 10), 10);

    // Add new water closer to (20, 10)
    terrain.at(18, 10).setTerrainType(TerrainType::StillBasin);
    field.compute(terrain);  // Recompute

    // Now (20, 10) should be distance 2 from water at (18, 10)
    ASSERT_EQ(field.get_water_distance(20, 10), 2);
}

TEST(recomputation_on_water_removal) {
    TerrainGrid terrain(MapSize::Small);
    terrain.fill_type(TerrainType::Substrate);
    terrain.at(10, 10).setTerrainType(TerrainType::StillBasin);
    terrain.at(100, 10).setTerrainType(TerrainType::StillBasin);

    WaterDistanceField field(MapSize::Small);
    field.compute(terrain);

    // Tile at (50, 10) is distance 40 from water at (10, 10)
    ASSERT_EQ(field.get_water_distance(50, 10), 40);

    // Remove water at (10, 10) - now only (100, 10) remains
    terrain.at(10, 10).setTerrainType(TerrainType::Substrate);
    field.compute(terrain);  // Recompute

    // Now (50, 10) should be distance 50 from water at (100, 10)
    ASSERT_EQ(field.get_water_distance(50, 10), 50);
}

// =============================================================================
// O(1) Query Performance Tests
// =============================================================================

TEST(get_water_distance_is_o1) {
    // Setup: compute distances once
    TerrainGrid terrain(MapSize::Large);
    terrain.fill_type(TerrainType::Substrate);
    for (std::uint16_t x = 0; x < 512; ++x) {
        terrain.at(x, 0).setTerrainType(TerrainType::DeepVoid);
    }

    WaterDistanceField field(MapSize::Large);
    field.compute(terrain);

    // Perform many queries - should be very fast (O(1))
    auto start = std::chrono::high_resolution_clock::now();

    volatile std::uint8_t sum = 0;  // volatile to prevent optimization
    for (int i = 0; i < 1000000; ++i) {
        std::uint16_t x = static_cast<std::uint16_t>(i % 512);
        std::uint16_t y = static_cast<std::uint16_t>((i / 512) % 512);
        sum += field.get_water_distance(x, y);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration_us = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    // 1 million O(1) queries should complete in well under 100ms
    // Typical time is < 10ms on modern hardware
    printf(" [1M queries in %lld us]", static_cast<long long>(duration_us));
    ASSERT(duration_us < 100000);  // < 100ms
}

// =============================================================================
// Performance Verification Tests
// =============================================================================

TEST(bfs_performance_512x512) {
    // Create a 512x512 terrain with ocean border and scattered lakes
    TerrainGrid terrain(MapSize::Large);
    terrain.fill_type(TerrainType::Substrate);

    // Add ocean border (top and left edges)
    for (std::uint16_t x = 0; x < 512; ++x) {
        terrain.at(x, 0).setTerrainType(TerrainType::DeepVoid);
    }
    for (std::uint16_t y = 0; y < 512; ++y) {
        terrain.at(0, y).setTerrainType(TerrainType::DeepVoid);
    }

    // Add some scattered lakes
    for (std::uint16_t y = 100; y < 110; ++y) {
        for (std::uint16_t x = 200; x < 220; ++x) {
            terrain.at(x, y).setTerrainType(TerrainType::StillBasin);
        }
    }
    for (std::uint16_t y = 300; y < 320; ++y) {
        for (std::uint16_t x = 400; x < 430; ++x) {
            terrain.at(x, y).setTerrainType(TerrainType::StillBasin);
        }
    }

    WaterDistanceField field(MapSize::Large);

    // Measure BFS computation time
    auto start = std::chrono::high_resolution_clock::now();
    field.compute(terrain);
    auto end = std::chrono::high_resolution_clock::now();

    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    printf(" [BFS computed in %lld ms]", static_cast<long long>(duration_ms));

    // Performance requirement: <5ms for 512x512
    ASSERT(duration_ms < 5);

    // Verify computation is correct
    ASSERT_EQ(field.get_water_distance(0, 0), 0);     // Ocean corner
    ASSERT_EQ(field.get_water_distance(1, 1), 1);     // Adjacent to ocean
    ASSERT_EQ(field.get_water_distance(200, 100), 0); // Lake tile
}

TEST(bfs_performance_worst_case) {
    // Worst case: single water tile in corner, must propagate across entire map
    TerrainGrid terrain(MapSize::Large);
    terrain.fill_type(TerrainType::Substrate);
    terrain.at(0, 0).setTerrainType(TerrainType::DeepVoid);

    WaterDistanceField field(MapSize::Large);

    auto start = std::chrono::high_resolution_clock::now();
    field.compute(terrain);
    auto end = std::chrono::high_resolution_clock::now();

    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    printf(" [worst-case BFS in %lld ms]", static_cast<long long>(duration_ms));

    // Even worst case should complete in <5ms
    ASSERT(duration_ms < 5);

    // Verify correctness
    ASSERT_EQ(field.get_water_distance(0, 0), 0);
    ASSERT_EQ(field.get_water_distance(254, 0), 254);
    ASSERT_EQ(field.get_water_distance(511, 511), 255);  // Capped
}

// =============================================================================
// In-Bounds Check Tests
// =============================================================================

TEST(in_bounds_valid_coordinates) {
    WaterDistanceField field(MapSize::Small);  // 128x128

    ASSERT(field.in_bounds(0, 0));
    ASSERT(field.in_bounds(127, 0));
    ASSERT(field.in_bounds(0, 127));
    ASSERT(field.in_bounds(127, 127));
    ASSERT(field.in_bounds(64, 64));
}

TEST(in_bounds_negative_coordinates) {
    WaterDistanceField field(MapSize::Small);

    ASSERT(!field.in_bounds(-1, 0));
    ASSERT(!field.in_bounds(0, -1));
    ASSERT(!field.in_bounds(-1, -1));
}

TEST(in_bounds_out_of_range) {
    WaterDistanceField field(MapSize::Small);  // 128x128

    ASSERT(!field.in_bounds(128, 0));
    ASSERT(!field.in_bounds(0, 128));
    ASSERT(!field.in_bounds(128, 128));
}

// =============================================================================
// Clear and Reset Tests
// =============================================================================

TEST(clear_resets_all_distances) {
    WaterDistanceField field(MapSize::Small);

    // Set some distances manually
    field.set_distance(10, 10, 5);
    field.set_distance(50, 50, 100);
    ASSERT_EQ(field.get_water_distance(10, 10), 5);
    ASSERT_EQ(field.get_water_distance(50, 50), 100);

    // Clear should reset to MAX_WATER_DISTANCE
    field.clear();
    ASSERT_EQ(field.get_water_distance(10, 10), MAX_WATER_DISTANCE);
    ASSERT_EQ(field.get_water_distance(50, 50), MAX_WATER_DISTANCE);
}

// =============================================================================
// Integration Test: Realistic Terrain
// =============================================================================

TEST(realistic_terrain_distances) {
    // Create a realistic terrain with:
    // - Ocean on west and south edges
    // - A river running from north to south
    // - A lake in the northeast
    TerrainGrid terrain(MapSize::Medium);  // 256x256
    terrain.fill_type(TerrainType::Substrate);

    // West ocean (x = 0..9)
    for (std::uint16_t y = 0; y < 256; ++y) {
        for (std::uint16_t x = 0; x < 10; ++x) {
            terrain.at(x, y).setTerrainType(TerrainType::DeepVoid);
        }
    }

    // South ocean (y = 246..255)
    for (std::uint16_t y = 246; y < 256; ++y) {
        for (std::uint16_t x = 0; x < 256; ++x) {
            terrain.at(x, y).setTerrainType(TerrainType::DeepVoid);
        }
    }

    // River from (128, 0) to (128, 245)
    for (std::uint16_t y = 0; y < 246; ++y) {
        terrain.at(128, y).setTerrainType(TerrainType::FlowChannel);
    }

    // Lake at (200, 50) with radius ~10
    for (std::int32_t dy = -10; dy <= 10; ++dy) {
        for (std::int32_t dx = -10; dx <= 10; ++dx) {
            if (dx * dx + dy * dy <= 100) {  // Circular-ish
                std::uint16_t x = static_cast<std::uint16_t>(200 + dx);
                std::uint16_t y = static_cast<std::uint16_t>(50 + dy);
                if (x < 256 && y < 256) {
                    terrain.at(x, y).setTerrainType(TerrainType::StillBasin);
                }
            }
        }
    }

    WaterDistanceField field(MapSize::Medium);
    field.compute(terrain);

    // Verify water tiles
    ASSERT_EQ(field.get_water_distance(0, 0), 0);     // Ocean
    ASSERT_EQ(field.get_water_distance(128, 100), 0); // River
    ASSERT_EQ(field.get_water_distance(200, 50), 0);  // Lake

    // Tile between ocean and river
    // At (70, 100): distance to ocean (x=9) is 61, distance to river (x=128) is 58
    // Should be 58 (closer to river)
    ASSERT_EQ(field.get_water_distance(70, 100), 58);

    // Tile near lake
    // At (220, 50): distance to lake edge (~210, 50) is about 10
    std::uint8_t dist_to_lake = field.get_water_distance(220, 50);
    ASSERT(dist_to_lake <= 20);  // Should be close to lake
    ASSERT(dist_to_lake > 0);     // But not in lake
}

// =============================================================================
// Main Entry Point
// =============================================================================

int main() {
    printf("=== WaterDistanceField Unit Tests (Ticket 3-006) ===\n\n");

    // Construction tests
    RUN_TEST(default_construction);
    RUN_TEST(mapsize_small_construction);
    RUN_TEST(mapsize_medium_construction);
    RUN_TEST(mapsize_large_construction);
    RUN_TEST(explicit_dimension_construction);
    RUN_TEST(initialize_reinitializes);

    // Memory budget tests
    RUN_TEST(memory_budget_small);
    RUN_TEST(memory_budget_medium);
    RUN_TEST(memory_budget_large);
    RUN_TEST(storage_type_size_verification);

    // Water type detection tests
    RUN_TEST(water_type_detection_deep_void);
    RUN_TEST(water_type_detection_flow_channel);
    RUN_TEST(water_type_detection_still_basin);
    RUN_TEST(water_type_detection_non_water);

    // BFS computation tests
    RUN_TEST(water_tile_distance_zero);
    RUN_TEST(adjacent_tile_distance_one);
    RUN_TEST(manhattan_distance_correctness);
    RUN_TEST(multi_source_bfs_nearest_water);
    RUN_TEST(ocean_border_distances);
    RUN_TEST(river_distances);

    // Distance capping tests
    RUN_TEST(distance_capping_at_255);
    RUN_TEST(distance_capping_large_map);

    // Edge case tests
    RUN_TEST(all_water_map);
    RUN_TEST(no_water_map);
    RUN_TEST(single_water_tile_center);
    RUN_TEST(corner_water_tile);

    // Recomputation tests
    RUN_TEST(recomputation_on_water_change);
    RUN_TEST(recomputation_on_water_removal);

    // O(1) query performance test
    RUN_TEST(get_water_distance_is_o1);

    // Performance verification tests
    RUN_TEST(bfs_performance_512x512);
    RUN_TEST(bfs_performance_worst_case);

    // In-bounds tests
    RUN_TEST(in_bounds_valid_coordinates);
    RUN_TEST(in_bounds_negative_coordinates);
    RUN_TEST(in_bounds_out_of_range);

    // Clear and reset tests
    RUN_TEST(clear_resets_all_distances);

    // Integration test
    RUN_TEST(realistic_terrain_distances);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
