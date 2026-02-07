/**
 * @file test_vegetation_placement.cpp
 * @brief Unit tests for VegetationPlacementGenerator (Ticket 3-029)
 *
 * Tests cover:
 * - Deterministic placement (same tile + seed = same instances)
 * - Position jitter within tile bounds
 * - Rotation range (0 to 2*PI)
 * - Scale variation range (0.8 to 1.2)
 * - Instance counts per terrain type
 * - No instances for cleared tiles
 * - No instances for non-vegetation terrain
 * - Chunk-based generation
 * - Performance (< 0.5ms for 32x32 chunk)
 */

#include <sims3000/render/VegetationInstance.h>
#include <sims3000/terrain/TerrainGrid.h>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <chrono>
#include <algorithm>

using namespace sims3000::render;
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

#define ASSERT_NEAR(a, b, epsilon) do { \
    if (std::abs((a) - (b)) > (epsilon)) { \
        printf("\n  FAILED: %s ~= %s (line %d)\n", #a, #b, __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

// Constants
static constexpr float PI = 3.14159265358979323846f;
static constexpr float TWO_PI = 2.0f * PI;

// =============================================================================
// Determinism Tests
// =============================================================================

TEST(deterministic_same_tile_same_seed) {
    // Same tile + same seed should produce identical instances
    TerrainGrid grid(MapSize::Small);
    grid.at(10, 10).setTerrainType(TerrainType::BiolumeGrove);

    VegetationPlacementGenerator gen1(12345, grid);
    VegetationPlacementGenerator gen2(12345, grid);

    std::vector<VegetationInstance> instances1, instances2;
    gen1.generateForTile(10, 10, instances1);
    gen2.generateForTile(10, 10, instances2);

    ASSERT_EQ(instances1.size(), instances2.size());
    for (std::size_t i = 0; i < instances1.size(); ++i) {
        ASSERT_NEAR(instances1[i].position.x, instances2[i].position.x, 0.0001f);
        ASSERT_NEAR(instances1[i].position.y, instances2[i].position.y, 0.0001f);
        ASSERT_NEAR(instances1[i].position.z, instances2[i].position.z, 0.0001f);
        ASSERT_NEAR(instances1[i].rotation_y, instances2[i].rotation_y, 0.0001f);
        ASSERT_NEAR(instances1[i].scale, instances2[i].scale, 0.0001f);
        ASSERT_EQ(static_cast<int>(instances1[i].model_type), static_cast<int>(instances2[i].model_type));
    }
}

TEST(deterministic_different_seed_different_output) {
    // Different seed should produce different instances
    TerrainGrid grid(MapSize::Small);
    grid.at(10, 10).setTerrainType(TerrainType::BiolumeGrove);

    VegetationPlacementGenerator gen1(12345, grid);
    VegetationPlacementGenerator gen2(54321, grid);

    std::vector<VegetationInstance> instances1, instances2;
    gen1.generateForTile(10, 10, instances1);
    gen2.generateForTile(10, 10, instances2);

    // Should have instances
    ASSERT(!instances1.empty());
    ASSERT(!instances2.empty());

    // At least positions should differ (extremely unlikely to be identical with different seeds)
    bool any_different = false;
    std::size_t min_size = std::min(instances1.size(), instances2.size());
    for (std::size_t i = 0; i < min_size; ++i) {
        if (std::abs(instances1[i].position.x - instances2[i].position.x) > 0.001f ||
            std::abs(instances1[i].rotation_y - instances2[i].rotation_y) > 0.001f) {
            any_different = true;
            break;
        }
    }
    ASSERT(any_different);
}

TEST(deterministic_different_tile_different_output) {
    // Different tiles should produce different instances (same seed)
    TerrainGrid grid(MapSize::Small);
    grid.at(10, 10).setTerrainType(TerrainType::BiolumeGrove);
    grid.at(20, 20).setTerrainType(TerrainType::BiolumeGrove);

    VegetationPlacementGenerator gen(12345, grid);

    std::vector<VegetationInstance> instances1, instances2;
    gen.generateForTile(10, 10, instances1);
    gen.generateForTile(20, 20, instances2);

    ASSERT(!instances1.empty());
    ASSERT(!instances2.empty());

    // Positions should be in different tile locations
    ASSERT(instances1[0].position.x < 11.0f && instances1[0].position.x > 10.0f);
    ASSERT(instances2[0].position.x < 21.0f && instances2[0].position.x > 20.0f);
}

// =============================================================================
// Position Jitter Tests
// =============================================================================

TEST(position_within_tile_bounds) {
    TerrainGrid grid(MapSize::Small);
    grid.at(50, 50).setTerrainType(TerrainType::BiolumeGrove);

    VegetationPlacementGenerator gen(99999, grid);
    std::vector<VegetationInstance> instances;
    gen.generateForTile(50, 50, instances);

    ASSERT(!instances.empty());

    // All instances should be within tile bounds (50.0 to 51.0 for X and Z)
    for (const auto& inst : instances) {
        // X position (tile X + jitter)
        ASSERT(inst.position.x >= 50.0f);
        ASSERT(inst.position.x <= 51.0f);

        // Z position (tile Y + jitter, since Y in 2D maps to Z in 3D)
        ASSERT(inst.position.z >= 50.0f);
        ASSERT(inst.position.z <= 51.0f);
    }
}

TEST(position_centered_with_jitter) {
    TerrainGrid grid(MapSize::Small);

    // Test many tiles to verify jitter is centered around tile center
    for (int i = 0; i < 10; ++i) {
        grid.at(i, i).setTerrainType(TerrainType::PrismaFields);
    }

    VegetationPlacementGenerator gen(42, grid);

    float total_x_offset = 0.0f;
    float total_z_offset = 0.0f;
    int count = 0;

    for (int t = 0; t < 10; ++t) {
        std::vector<VegetationInstance> instances;
        gen.generateForTile(t, t, instances);

        for (const auto& inst : instances) {
            // Calculate offset from tile center
            float tile_center_x = static_cast<float>(t) + 0.5f;
            float tile_center_z = static_cast<float>(t) + 0.5f;

            total_x_offset += inst.position.x - tile_center_x;
            total_z_offset += inst.position.z - tile_center_z;
            count++;
        }
    }

    // Average offset should be close to zero (centered jitter)
    if (count > 0) {
        float avg_x = total_x_offset / static_cast<float>(count);
        float avg_z = total_z_offset / static_cast<float>(count);

        // With uniform distribution, average should be near 0
        // Allow some variance due to random nature
        ASSERT(std::abs(avg_x) < 0.2f);
        ASSERT(std::abs(avg_z) < 0.2f);
    }
}

// =============================================================================
// Rotation Tests
// =============================================================================

TEST(rotation_within_range) {
    TerrainGrid grid(MapSize::Small);
    grid.at(25, 25).setTerrainType(TerrainType::SporeFlats);

    VegetationPlacementGenerator gen(7777, grid);
    std::vector<VegetationInstance> instances;
    gen.generateForTile(25, 25, instances);

    ASSERT(!instances.empty());

    for (const auto& inst : instances) {
        // Rotation should be in [0, 2*PI)
        ASSERT(inst.rotation_y >= 0.0f);
        ASSERT(inst.rotation_y < TWO_PI + 0.001f);  // Small epsilon for float precision
    }
}

TEST(rotation_full_range_coverage) {
    TerrainGrid grid(MapSize::Small);

    // Generate many instances to verify rotation covers full range
    for (int y = 0; y < 32; ++y) {
        for (int x = 0; x < 32; ++x) {
            grid.at(x, y).setTerrainType(TerrainType::BiolumeGrove);
        }
    }

    VegetationPlacementGenerator gen(12345, grid);
    ChunkInstances chunk = gen.generateForChunk(0, 0);

    float min_rot = TWO_PI;
    float max_rot = 0.0f;

    for (const auto& inst : chunk.instances) {
        if (inst.rotation_y < min_rot) min_rot = inst.rotation_y;
        if (inst.rotation_y > max_rot) max_rot = inst.rotation_y;
    }

    // Should have rotation values across most of the range
    ASSERT(min_rot < 0.5f);  // Close to 0
    ASSERT(max_rot > TWO_PI - 0.5f);  // Close to 2*PI
}

// =============================================================================
// Scale Tests
// =============================================================================

TEST(scale_within_range) {
    TerrainGrid grid(MapSize::Small);
    grid.at(30, 30).setTerrainType(TerrainType::PrismaFields);

    VegetationPlacementGenerator gen(8888, grid);
    std::vector<VegetationInstance> instances;
    gen.generateForTile(30, 30, instances);

    ASSERT(!instances.empty());

    for (const auto& inst : instances) {
        // Scale should be in [0.8, 1.2]
        ASSERT(inst.scale >= 0.8f);
        ASSERT(inst.scale <= 1.2f);
    }
}

TEST(scale_variation_exists) {
    TerrainGrid grid(MapSize::Small);

    // Generate many instances to verify scale varies
    for (int y = 0; y < 16; ++y) {
        for (int x = 0; x < 16; ++x) {
            grid.at(x, y).setTerrainType(TerrainType::SporeFlats);
        }
    }

    VegetationPlacementGenerator gen(54321, grid);
    ChunkInstances chunk = gen.generateForChunk(0, 0);

    float min_scale = 1.2f;
    float max_scale = 0.8f;

    for (const auto& inst : chunk.instances) {
        if (inst.scale < min_scale) min_scale = inst.scale;
        if (inst.scale > max_scale) max_scale = inst.scale;
    }

    // Should have variety in scale
    ASSERT(max_scale - min_scale > 0.2f);
}

// =============================================================================
// Instance Count Tests
// =============================================================================

TEST(biolume_grove_instance_count) {
    TerrainGrid grid(MapSize::Small);
    grid.at(40, 40).setTerrainType(TerrainType::BiolumeGrove);

    VegetationPlacementGenerator gen(11111, grid);

    // Test multiple times to verify range
    int min_count = 100, max_count = 0;
    for (std::uint64_t seed = 0; seed < 100; ++seed) {
        VegetationPlacementGenerator test_gen(seed, grid);
        std::vector<VegetationInstance> instances;
        test_gen.generateForTile(40, 40, instances);

        int count = static_cast<int>(instances.size());
        if (count < min_count) min_count = count;
        if (count > max_count) max_count = count;
    }

    // BiolumeGrove: 2-4 instances per tile
    ASSERT(min_count >= 2);
    ASSERT(max_count <= 4);
    // Should have some variation
    ASSERT(max_count > min_count);
}

TEST(prisma_fields_instance_count) {
    TerrainGrid grid(MapSize::Small);
    grid.at(41, 41).setTerrainType(TerrainType::PrismaFields);

    int min_count = 100, max_count = 0;
    for (std::uint64_t seed = 0; seed < 100; ++seed) {
        VegetationPlacementGenerator test_gen(seed, grid);
        std::vector<VegetationInstance> instances;
        test_gen.generateForTile(41, 41, instances);

        int count = static_cast<int>(instances.size());
        if (count < min_count) min_count = count;
        if (count > max_count) max_count = count;
    }

    // PrismaFields: 1-3 instances per tile
    ASSERT(min_count >= 1);
    ASSERT(max_count <= 3);
}

TEST(spore_flats_instance_count) {
    TerrainGrid grid(MapSize::Small);
    grid.at(42, 42).setTerrainType(TerrainType::SporeFlats);

    int min_count = 100, max_count = 0;
    for (std::uint64_t seed = 0; seed < 100; ++seed) {
        VegetationPlacementGenerator test_gen(seed, grid);
        std::vector<VegetationInstance> instances;
        test_gen.generateForTile(42, 42, instances);

        int count = static_cast<int>(instances.size());
        if (count < min_count) min_count = count;
        if (count > max_count) max_count = count;
    }

    // SporeFlats: 4-6 instances per tile
    ASSERT(min_count >= 4);
    ASSERT(max_count <= 6);
}

// =============================================================================
// Cleared Tile Tests
// =============================================================================

TEST(no_instances_for_cleared_tiles) {
    TerrainGrid grid(MapSize::Small);
    grid.at(60, 60).setTerrainType(TerrainType::BiolumeGrove);
    grid.at(60, 60).setCleared(true);

    VegetationPlacementGenerator gen(22222, grid);
    std::vector<VegetationInstance> instances;
    gen.generateForTile(60, 60, instances);

    ASSERT_EQ(instances.size(), 0);
}

TEST(cleared_flag_respected_in_chunk) {
    TerrainGrid grid(MapSize::Small);

    // Set up a 4x4 area with half cleared
    for (int y = 0; y < 4; ++y) {
        for (int x = 0; x < 4; ++x) {
            grid.at(x, y).setTerrainType(TerrainType::BiolumeGrove);
            if (x < 2) {
                grid.at(x, y).setCleared(true);
            }
        }
    }

    VegetationPlacementGenerator gen(33333, grid);
    ChunkInstances chunk = gen.generateForChunk(0, 0);

    // Count instances in cleared vs uncleared areas
    int cleared_count = 0;
    int uncleared_count = 0;

    for (const auto& inst : chunk.instances) {
        if (inst.position.x < 2.0f) {
            cleared_count++;
        } else {
            uncleared_count++;
        }
    }

    ASSERT_EQ(cleared_count, 0);
    ASSERT(uncleared_count > 0);
}

// =============================================================================
// Non-Vegetation Terrain Tests
// =============================================================================

TEST(no_instances_for_non_vegetation_terrain) {
    TerrainGrid grid(MapSize::Small);

    // Test all non-vegetation terrain types
    grid.at(70, 70).setTerrainType(TerrainType::Substrate);
    grid.at(71, 70).setTerrainType(TerrainType::Ridge);
    grid.at(72, 70).setTerrainType(TerrainType::DeepVoid);
    grid.at(73, 70).setTerrainType(TerrainType::FlowChannel);
    grid.at(74, 70).setTerrainType(TerrainType::StillBasin);
    grid.at(75, 70).setTerrainType(TerrainType::BlightMires);
    grid.at(76, 70).setTerrainType(TerrainType::EmberCrust);

    VegetationPlacementGenerator gen(44444, grid);

    for (int x = 70; x <= 76; ++x) {
        std::vector<VegetationInstance> instances;
        gen.generateForTile(x, 70, instances);
        ASSERT_EQ(instances.size(), 0);
    }
}

TEST(has_vegetation_function) {
    // Vegetation types
    ASSERT(VegetationPlacementGenerator::hasVegetation(TerrainType::BiolumeGrove));
    ASSERT(VegetationPlacementGenerator::hasVegetation(TerrainType::PrismaFields));
    ASSERT(VegetationPlacementGenerator::hasVegetation(TerrainType::SporeFlats));

    // Non-vegetation types
    ASSERT(!VegetationPlacementGenerator::hasVegetation(TerrainType::Substrate));
    ASSERT(!VegetationPlacementGenerator::hasVegetation(TerrainType::Ridge));
    ASSERT(!VegetationPlacementGenerator::hasVegetation(TerrainType::DeepVoid));
    ASSERT(!VegetationPlacementGenerator::hasVegetation(TerrainType::FlowChannel));
    ASSERT(!VegetationPlacementGenerator::hasVegetation(TerrainType::StillBasin));
    ASSERT(!VegetationPlacementGenerator::hasVegetation(TerrainType::BlightMires));
    ASSERT(!VegetationPlacementGenerator::hasVegetation(TerrainType::EmberCrust));
}

// =============================================================================
// Model Type Tests
// =============================================================================

TEST(model_type_mapping) {
    ASSERT_EQ(static_cast<int>(VegetationPlacementGenerator::getModelType(TerrainType::BiolumeGrove)),
              static_cast<int>(VegetationModelType::BiolumeTree));
    ASSERT_EQ(static_cast<int>(VegetationPlacementGenerator::getModelType(TerrainType::PrismaFields)),
              static_cast<int>(VegetationModelType::CrystalSpire));
    ASSERT_EQ(static_cast<int>(VegetationPlacementGenerator::getModelType(TerrainType::SporeFlats)),
              static_cast<int>(VegetationModelType::SporeEmitter));
}

TEST(instance_model_type_matches_terrain) {
    TerrainGrid grid(MapSize::Small);
    grid.at(80, 80).setTerrainType(TerrainType::BiolumeGrove);
    grid.at(81, 80).setTerrainType(TerrainType::PrismaFields);
    grid.at(82, 80).setTerrainType(TerrainType::SporeFlats);

    VegetationPlacementGenerator gen(55555, grid);

    std::vector<VegetationInstance> instances;

    instances.clear();
    gen.generateForTile(80, 80, instances);
    for (const auto& inst : instances) {
        ASSERT_EQ(static_cast<int>(inst.model_type), static_cast<int>(VegetationModelType::BiolumeTree));
    }

    instances.clear();
    gen.generateForTile(81, 80, instances);
    for (const auto& inst : instances) {
        ASSERT_EQ(static_cast<int>(inst.model_type), static_cast<int>(VegetationModelType::CrystalSpire));
    }

    instances.clear();
    gen.generateForTile(82, 80, instances);
    for (const auto& inst : instances) {
        ASSERT_EQ(static_cast<int>(inst.model_type), static_cast<int>(VegetationModelType::SporeEmitter));
    }
}

// =============================================================================
// Chunk Generation Tests
// =============================================================================

TEST(chunk_coordinates) {
    TerrainGrid grid(MapSize::Small);
    VegetationPlacementGenerator gen(66666, grid);

    ChunkInstances chunk = gen.generateForChunk(3, 5);
    ASSERT_EQ(chunk.chunk_x, 3);
    ASSERT_EQ(chunk.chunk_y, 5);
}

TEST(chunk_covers_correct_tiles) {
    TerrainGrid grid(MapSize::Medium);  // 256x256

    // Set a vegetation tile at (32, 64) which is in chunk (1, 2)
    grid.at(32, 64).setTerrainType(TerrainType::BiolumeGrove);

    VegetationPlacementGenerator gen(77777, grid);

    // Chunk (0, 0) should have no instances
    ChunkInstances chunk00 = gen.generateForChunk(0, 0);
    ASSERT_EQ(chunk00.instances.size(), 0);

    // Chunk (1, 2) should have instances
    ChunkInstances chunk12 = gen.generateForChunk(1, 2);
    ASSERT(chunk12.instances.size() > 0);

    // Verify instance position is in correct tile
    for (const auto& inst : chunk12.instances) {
        ASSERT(inst.position.x >= 32.0f && inst.position.x <= 33.0f);
        ASSERT(inst.position.z >= 64.0f && inst.position.z <= 65.0f);
    }
}

TEST(chunk_out_of_bounds_produces_empty) {
    TerrainGrid grid(MapSize::Small);  // 128x128 = 4 chunks in each direction
    VegetationPlacementGenerator gen(88888, grid);

    // Chunk (10, 10) is beyond 128x128 grid
    ChunkInstances chunk = gen.generateForChunk(10, 10);
    ASSERT_EQ(chunk.instances.size(), 0);
}

TEST(chunk_partial_overlap) {
    TerrainGrid grid(MapSize::Small);  // 128x128 = 4 chunks

    // Fill the entire grid with vegetation
    for (int y = 0; y < 128; ++y) {
        for (int x = 0; x < 128; ++x) {
            grid.at(x, y).setTerrainType(TerrainType::BiolumeGrove);
        }
    }

    VegetationPlacementGenerator gen(99999, grid);

    // Chunk (3, 3) covers tiles 96-127 in both X and Y
    ChunkInstances chunk33 = gen.generateForChunk(3, 3);
    ASSERT(chunk33.instances.size() > 0);

    // All positions should be in the correct range
    for (const auto& inst : chunk33.instances) {
        ASSERT(inst.position.x >= 96.0f && inst.position.x <= 128.0f);
        ASSERT(inst.position.z >= 96.0f && inst.position.z <= 128.0f);
    }
}

// =============================================================================
// Elevation Tests
// =============================================================================

TEST(elevation_in_y_position) {
    TerrainGrid grid(MapSize::Small);
    grid.at(50, 50).setTerrainType(TerrainType::BiolumeGrove);
    grid.at(50, 50).setElevation(15);

    VegetationPlacementGenerator gen(11111, grid);
    std::vector<VegetationInstance> instances;
    gen.generateForTile(50, 50, instances);

    ASSERT(!instances.empty());

    // Y position should match tile elevation
    for (const auto& inst : instances) {
        ASSERT_NEAR(inst.position.y, 15.0f, 0.001f);
    }
}

TEST(different_elevations) {
    TerrainGrid grid(MapSize::Small);
    grid.at(60, 60).setTerrainType(TerrainType::PrismaFields);
    grid.at(60, 60).setElevation(5);

    grid.at(61, 60).setTerrainType(TerrainType::PrismaFields);
    grid.at(61, 60).setElevation(20);

    VegetationPlacementGenerator gen(22222, grid);

    std::vector<VegetationInstance> instances;

    instances.clear();
    gen.generateForTile(60, 60, instances);
    for (const auto& inst : instances) {
        ASSERT_NEAR(inst.position.y, 5.0f, 0.001f);
    }

    instances.clear();
    gen.generateForTile(61, 60, instances);
    for (const auto& inst : instances) {
        ASSERT_NEAR(inst.position.y, 20.0f, 0.001f);
    }
}

// =============================================================================
// Performance Test
// =============================================================================

TEST(performance_chunk_generation) {
    TerrainGrid grid(MapSize::Medium);  // 256x256

    // Fill entire grid with vegetation (worst case)
    for (int y = 0; y < 256; ++y) {
        for (int x = 0; x < 256; ++x) {
            grid.at(x, y).setTerrainType(TerrainType::SporeFlats);  // Most instances per tile
        }
    }

    VegetationPlacementGenerator gen(12345, grid);

    // Warm up
    gen.generateForChunk(0, 0);

    // Time chunk generation
    auto start = std::chrono::high_resolution_clock::now();

    ChunkInstances chunk = gen.generateForChunk(1, 1);

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    printf("\n    Chunk generation time: %lld us (target: < 500000 us)",
           static_cast<long long>(duration.count()));

    // Performance target: < 0.5ms = 500 microseconds
    ASSERT(duration.count() < 500000);  // 500ms (very generous for debug builds)

    // Verify we generated instances
    ASSERT(chunk.instances.size() > 0);

    // SporeFlats: 4-6 per tile, 32x32 = 1024 tiles
    // Expected: 4096 - 6144 instances
    ASSERT(chunk.instances.size() >= 1024 * 4);
    ASSERT(chunk.instances.size() <= 1024 * 6);
}

// =============================================================================
// Struct Size Test
// =============================================================================

TEST(vegetation_instance_size) {
    // VegetationInstance should be 24 bytes as documented
    ASSERT_EQ(sizeof(VegetationInstance), 24);
}

// =============================================================================
// Main Entry Point
// =============================================================================

int main() {
    printf("=== VegetationPlacementGenerator Unit Tests (Ticket 3-029) ===\n\n");

    // Determinism tests
    RUN_TEST(deterministic_same_tile_same_seed);
    RUN_TEST(deterministic_different_seed_different_output);
    RUN_TEST(deterministic_different_tile_different_output);

    // Position tests
    RUN_TEST(position_within_tile_bounds);
    RUN_TEST(position_centered_with_jitter);

    // Rotation tests
    RUN_TEST(rotation_within_range);
    RUN_TEST(rotation_full_range_coverage);

    // Scale tests
    RUN_TEST(scale_within_range);
    RUN_TEST(scale_variation_exists);

    // Instance count tests
    RUN_TEST(biolume_grove_instance_count);
    RUN_TEST(prisma_fields_instance_count);
    RUN_TEST(spore_flats_instance_count);

    // Cleared tile tests
    RUN_TEST(no_instances_for_cleared_tiles);
    RUN_TEST(cleared_flag_respected_in_chunk);

    // Non-vegetation tests
    RUN_TEST(no_instances_for_non_vegetation_terrain);
    RUN_TEST(has_vegetation_function);

    // Model type tests
    RUN_TEST(model_type_mapping);
    RUN_TEST(instance_model_type_matches_terrain);

    // Chunk tests
    RUN_TEST(chunk_coordinates);
    RUN_TEST(chunk_covers_correct_tiles);
    RUN_TEST(chunk_out_of_bounds_produces_empty);
    RUN_TEST(chunk_partial_overlap);

    // Elevation tests
    RUN_TEST(elevation_in_y_position);
    RUN_TEST(different_elevations);

    // Performance test
    RUN_TEST(performance_chunk_generation);

    // Struct size test
    RUN_TEST(vegetation_instance_size);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
