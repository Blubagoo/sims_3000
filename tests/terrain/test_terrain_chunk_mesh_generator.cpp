/**
 * @file test_terrain_chunk_mesh_generator.cpp
 * @brief Unit tests for TerrainChunkMeshGenerator (Ticket 3-025)
 *
 * Tests terrain chunk mesh generation including:
 * - Surface mesh generation for 32x32 chunks
 * - Vertex position calculation with ELEVATION_HEIGHT
 * - Normal computation via central differences
 * - Cliff face geometry generation
 * - Incremental rebuild queue management
 * - Performance: single chunk rebuild < 1ms
 */

#include <sims3000/terrain/TerrainChunkMeshGenerator.h>
#include <sims3000/terrain/TerrainGrid.h>
#include <sims3000/terrain/TerrainChunk.h>
#include <sims3000/terrain/ChunkDirtyTracker.h>
#include <cmath>
#include <chrono>
#include <iostream>
#include <iomanip>

namespace {

// Test result tracking
int tests_passed = 0;
int tests_failed = 0;

void test_pass(const char* test_name) {
    std::cout << "[PASS] " << test_name << std::endl;
    tests_passed++;
}

void test_fail(const char* test_name, const char* message) {
    std::cout << "[FAIL] " << test_name << ": " << message << std::endl;
    tests_failed++;
}

#define ASSERT_TRUE(expr, name) \
    if (!(expr)) { test_fail(name, #expr " was false"); return; } \

#define ASSERT_FALSE(expr, name) \
    if (expr) { test_fail(name, #expr " was true"); return; } \

#define ASSERT_EQ(a, b, name) \
    if ((a) != (b)) { \
        std::cout << "[FAIL] " << name << ": " << #a << " (" << (a) << ") != " << #b << " (" << (b) << ")" << std::endl; \
        tests_failed++; return; \
    }

#define ASSERT_NEAR(a, b, eps, name) \
    if (std::abs((a) - (b)) > (eps)) { \
        std::cout << "[FAIL] " << name << ": " << #a << " (" << (a) << ") != " << #b << " (" << (b) << ") within " << (eps) << std::endl; \
        tests_failed++; return; \
    }

using namespace sims3000::terrain;

// ============================================================================
// Initialization Tests
// ============================================================================

void test_DefaultConstruction() {
    const char* name = "test_DefaultConstruction";

    TerrainChunkMeshGenerator generator;

    ASSERT_EQ(generator.getCliffThreshold(), DEFAULT_CLIFF_THRESHOLD, name);
    ASSERT_FALSE(generator.hasPendingRebuilds(), name);
    ASSERT_EQ(generator.getPendingRebuildCount(), 0u, name);

    test_pass(name);
}

void test_Initialize() {
    const char* name = "test_Initialize";

    TerrainChunkMeshGenerator generator;
    generator.initialize(256, 256);

    // 256 / 32 = 8 chunks in each direction
    ASSERT_FALSE(generator.hasPendingRebuilds(), name);

    test_pass(name);
}

void test_SetCliffThreshold() {
    const char* name = "test_SetCliffThreshold";

    TerrainChunkMeshGenerator generator;
    generator.setCliffThreshold(4);

    ASSERT_EQ(generator.getCliffThreshold(), 4u, name);

    generator.setCliffThreshold(0);  // Disable cliff faces
    ASSERT_EQ(generator.getCliffThreshold(), 0u, name);

    test_pass(name);
}

// ============================================================================
// Mesh Generation Tests (CPU-side only)
// ============================================================================

void test_GenerateChunkMesh_FlatTerrain() {
    const char* name = "test_GenerateChunkMesh_FlatTerrain";

    // Create a small terrain grid
    TerrainGrid grid(MapSize::Small);  // 128x128

    // Fill ENTIRE grid with flat terrain at elevation 10
    // This prevents cliff faces at chunk boundaries
    for (std::uint16_t y = 0; y < grid.height; ++y) {
        for (std::uint16_t x = 0; x < grid.width; ++x) {
            grid.at(x, y).setElevation(10);
            grid.at(x, y).setTerrainType(TerrainType::Substrate);
        }
    }

    TerrainChunkMeshGenerator generator;
    generator.initialize(grid.width, grid.height);

    ChunkMeshData mesh_data;
    bool result = generator.generateChunkMesh(grid, 0, 0, mesh_data);

    ASSERT_TRUE(result, name);

    // Expected: (32+1)^2 = 1089 vertices for surface
    ASSERT_EQ(mesh_data.vertices.size(), VERTICES_PER_CHUNK, name);

    // Expected: 32*32*6 = 6144 indices for surface
    ASSERT_EQ(mesh_data.indices.size(), INDICES_PER_CHUNK, name);

    // No cliff faces for flat terrain
    ASSERT_FALSE(mesh_data.has_cliff_faces, name);

    // All vertices at elevation 10
    ASSERT_EQ(mesh_data.max_elevation, 10u, name);
    ASSERT_EQ(mesh_data.min_elevation, 10u, name);

    test_pass(name);
}

void test_GenerateChunkMesh_VertexPositions() {
    const char* name = "test_GenerateChunkMesh_VertexPositions";

    TerrainGrid grid(MapSize::Small);

    // Set varying elevations
    grid.at(0, 0).setElevation(5);
    grid.at(1, 0).setElevation(10);
    grid.at(0, 1).setElevation(15);
    grid.at(1, 1).setElevation(20);

    // Fill rest with flat terrain
    for (std::uint16_t y = 0; y < 32; ++y) {
        for (std::uint16_t x = 0; x < 32; ++x) {
            if (x > 1 || y > 1) {
                grid.at(x, y).setElevation(0);
            }
            grid.at(x, y).setTerrainType(TerrainType::Substrate);
        }
    }

    TerrainChunkMeshGenerator generator;
    generator.initialize(grid.width, grid.height);

    ChunkMeshData mesh_data;
    generator.generateChunkMesh(grid, 0, 0, mesh_data);

    // Check first vertex (0,0) - should be at elevation 5
    const TerrainVertex& v00 = mesh_data.vertices[0];
    ASSERT_NEAR(v00.position_x, 0.0f, 0.001f, name);
    ASSERT_NEAR(v00.position_y, 5.0f * ELEVATION_HEIGHT, 0.001f, name);
    ASSERT_NEAR(v00.position_z, 0.0f, 0.001f, name);

    // Check vertex at (1,0) - should be at elevation 10
    const TerrainVertex& v10 = mesh_data.vertices[1];
    ASSERT_NEAR(v10.position_x, 1.0f, 0.001f, name);
    ASSERT_NEAR(v10.position_y, 10.0f * ELEVATION_HEIGHT, 0.001f, name);
    ASSERT_NEAR(v10.position_z, 0.0f, 0.001f, name);

    test_pass(name);
}

void test_GenerateChunkMesh_Normals() {
    const char* name = "test_GenerateChunkMesh_Normals";

    TerrainGrid grid(MapSize::Small);

    // Create a slope: elevation increases along X axis
    for (std::uint16_t y = 0; y < 32; ++y) {
        for (std::uint16_t x = 0; x < 32; ++x) {
            grid.at(x, y).setElevation(static_cast<std::uint8_t>(x));
            grid.at(x, y).setTerrainType(TerrainType::Substrate);
        }
    }

    TerrainChunkMeshGenerator generator;
    generator.initialize(grid.width, grid.height);

    ChunkMeshData mesh_data;
    generator.generateChunkMesh(grid, 0, 0, mesh_data);

    // Check interior vertex normal - should have negative X component
    // (normal tilts away from slope direction)
    // Interior vertex at (16, 16)
    std::uint32_t idx = 16 * 33 + 16;  // row 16, col 16
    const TerrainVertex& v = mesh_data.vertices[idx];

    // Normal should have nx < 0 (pointing away from increasing elevation)
    // and ny > 0 (still mostly pointing up)
    ASSERT_TRUE(v.normal_x < 0.0f, name);
    ASSERT_TRUE(v.normal_y > 0.0f, name);

    // Normal should be normalized
    float length = std::sqrt(v.normal_x * v.normal_x +
                             v.normal_y * v.normal_y +
                             v.normal_z * v.normal_z);
    ASSERT_NEAR(length, 1.0f, 0.001f, name);

    test_pass(name);
}

void test_GenerateChunkMesh_TerrainType() {
    const char* name = "test_GenerateChunkMesh_TerrainType";

    TerrainGrid grid(MapSize::Small);

    // Set different terrain types
    for (std::uint16_t y = 0; y < 32; ++y) {
        for (std::uint16_t x = 0; x < 32; ++x) {
            grid.at(x, y).setElevation(10);
            if (x < 16) {
                grid.at(x, y).setTerrainType(TerrainType::Substrate);
            } else {
                grid.at(x, y).setTerrainType(TerrainType::Ridge);
            }
        }
    }

    TerrainChunkMeshGenerator generator;
    generator.initialize(grid.width, grid.height);

    ChunkMeshData mesh_data;
    generator.generateChunkMesh(grid, 0, 0, mesh_data);

    // Check vertex at (5, 5) - should be Substrate
    std::uint32_t idx_left = 5 * 33 + 5;
    ASSERT_EQ(mesh_data.vertices[idx_left].terrain_type,
              static_cast<std::uint8_t>(TerrainType::Substrate), name);

    // Check vertex at (20, 5) - should be Ridge
    std::uint32_t idx_right = 5 * 33 + 20;
    ASSERT_EQ(mesh_data.vertices[idx_right].terrain_type,
              static_cast<std::uint8_t>(TerrainType::Ridge), name);

    test_pass(name);
}

void test_GenerateChunkMesh_TileCoordinates() {
    const char* name = "test_GenerateChunkMesh_TileCoordinates";

    TerrainGrid grid(MapSize::Small);

    // Fill with flat terrain
    for (std::uint16_t y = 0; y < 32; ++y) {
        for (std::uint16_t x = 0; x < 32; ++x) {
            grid.at(x, y).setElevation(10);
            grid.at(x, y).setTerrainType(TerrainType::Substrate);
        }
    }

    TerrainChunkMeshGenerator generator;
    generator.initialize(grid.width, grid.height);

    ChunkMeshData mesh_data;
    generator.generateChunkMesh(grid, 0, 0, mesh_data);

    // Check tile coordinates at (10, 15)
    std::uint32_t idx = 15 * 33 + 10;
    ASSERT_NEAR(mesh_data.vertices[idx].tile_coord_x, 10.0f, 0.001f, name);
    ASSERT_NEAR(mesh_data.vertices[idx].tile_coord_y, 15.0f, 0.001f, name);

    test_pass(name);
}

// ============================================================================
// Cliff Face Tests
// ============================================================================

void test_CliffFace_Generation() {
    const char* name = "test_CliffFace_Generation";

    TerrainGrid grid(MapSize::Small);

    // Create a cliff: left half at elevation 0, right half at elevation 5
    for (std::uint16_t y = 0; y < 32; ++y) {
        for (std::uint16_t x = 0; x < 32; ++x) {
            if (x < 16) {
                grid.at(x, y).setElevation(0);
            } else {
                grid.at(x, y).setElevation(5);
            }
            grid.at(x, y).setTerrainType(TerrainType::Substrate);
        }
    }

    TerrainChunkMeshGenerator generator;
    generator.initialize(grid.width, grid.height);
    generator.setCliffThreshold(2);  // Threshold of 2

    ChunkMeshData mesh_data;
    generator.generateChunkMesh(grid, 0, 0, mesh_data);

    // Should have cliff faces (delta = 5 > threshold 2)
    ASSERT_TRUE(mesh_data.has_cliff_faces, name);

    // Cliff faces add extra vertices and indices beyond surface mesh
    ASSERT_TRUE(mesh_data.vertices.size() > VERTICES_PER_CHUNK, name);
    ASSERT_TRUE(mesh_data.indices.size() > INDICES_PER_CHUNK, name);

    test_pass(name);
}

void test_CliffFace_NoGeneration_BelowThreshold() {
    const char* name = "test_CliffFace_NoGeneration_BelowThreshold";

    TerrainGrid grid(MapSize::Small);

    // Create a gentle slope with max delta of 1 between adjacent tiles
    // Fill entire grid to avoid cliff faces at chunk boundaries
    // Use a sawtooth pattern that never exceeds delta=1
    for (std::uint16_t y = 0; y < grid.height; ++y) {
        for (std::uint16_t x = 0; x < grid.width; ++x) {
            // Pattern: 0, 1, 0, 1, 0, 1... (alternating)
            // This ensures no adjacent tiles differ by more than 1
            grid.at(x, y).setElevation(static_cast<std::uint8_t>((x + y) % 2));
            grid.at(x, y).setTerrainType(TerrainType::Substrate);
        }
    }

    TerrainChunkMeshGenerator generator;
    generator.initialize(grid.width, grid.height);
    generator.setCliffThreshold(2);  // Threshold of 2

    ChunkMeshData mesh_data;
    generator.generateChunkMesh(grid, 0, 0, mesh_data);

    // No cliff faces (delta = 1 < threshold 2)
    ASSERT_FALSE(mesh_data.has_cliff_faces, name);

    // Should have exactly the surface mesh vertices/indices
    ASSERT_EQ(mesh_data.vertices.size(), VERTICES_PER_CHUNK, name);
    ASSERT_EQ(mesh_data.indices.size(), INDICES_PER_CHUNK, name);

    test_pass(name);
}

void test_CliffFace_HorizontalNormals() {
    const char* name = "test_CliffFace_HorizontalNormals";

    TerrainGrid grid(MapSize::Small);

    // Create a single cliff edge
    for (std::uint16_t y = 0; y < 32; ++y) {
        for (std::uint16_t x = 0; x < 32; ++x) {
            if (x < 10) {
                grid.at(x, y).setElevation(0);
            } else {
                grid.at(x, y).setElevation(10);
            }
            grid.at(x, y).setTerrainType(TerrainType::Substrate);
        }
    }

    TerrainChunkMeshGenerator generator;
    generator.initialize(grid.width, grid.height);
    generator.setCliffThreshold(2);

    ChunkMeshData mesh_data;
    generator.generateChunkMesh(grid, 0, 0, mesh_data);

    ASSERT_TRUE(mesh_data.has_cliff_faces, name);

    // Find cliff face vertices (they come after surface vertices)
    // Cliff face normals should be horizontal (ny = 0)
    bool found_horizontal_normal = false;
    for (std::size_t i = VERTICES_PER_CHUNK; i < mesh_data.vertices.size(); ++i) {
        const TerrainVertex& v = mesh_data.vertices[i];
        if (std::abs(v.normal_y) < 0.001f) {
            found_horizontal_normal = true;
            // Also check it's normalized
            float length = std::sqrt(v.normal_x * v.normal_x +
                                     v.normal_y * v.normal_y +
                                     v.normal_z * v.normal_z);
            ASSERT_NEAR(length, 1.0f, 0.001f, name);
            break;
        }
    }

    ASSERT_TRUE(found_horizontal_normal, name);

    test_pass(name);
}

void test_CliffFace_Disabled() {
    const char* name = "test_CliffFace_Disabled";

    TerrainGrid grid(MapSize::Small);

    // Create a cliff
    for (std::uint16_t y = 0; y < 32; ++y) {
        for (std::uint16_t x = 0; x < 32; ++x) {
            if (x < 16) {
                grid.at(x, y).setElevation(0);
            } else {
                grid.at(x, y).setElevation(10);
            }
            grid.at(x, y).setTerrainType(TerrainType::Substrate);
        }
    }

    TerrainChunkMeshGenerator generator;
    generator.initialize(grid.width, grid.height);
    generator.setCliffThreshold(0);  // Disable cliff faces

    ChunkMeshData mesh_data;
    generator.generateChunkMesh(grid, 0, 0, mesh_data);

    // No cliff faces even though there's a steep drop
    ASSERT_FALSE(mesh_data.has_cliff_faces, name);
    ASSERT_EQ(mesh_data.vertices.size(), VERTICES_PER_CHUNK, name);

    test_pass(name);
}

// ============================================================================
// Rebuild Queue Tests
// ============================================================================

void test_QueueChunkRebuild() {
    const char* name = "test_QueueChunkRebuild";

    TerrainChunkMeshGenerator generator;
    generator.initialize(256, 256);

    ASSERT_FALSE(generator.hasPendingRebuilds(), name);
    ASSERT_EQ(generator.getPendingRebuildCount(), 0u, name);

    generator.queueChunkRebuild(2, 3);

    ASSERT_TRUE(generator.hasPendingRebuilds(), name);
    ASSERT_EQ(generator.getPendingRebuildCount(), 1u, name);

    // Queue same chunk again - should not duplicate
    generator.queueChunkRebuild(2, 3);
    ASSERT_EQ(generator.getPendingRebuildCount(), 1u, name);

    // Queue different chunk
    generator.queueChunkRebuild(4, 5);
    ASSERT_EQ(generator.getPendingRebuildCount(), 2u, name);

    test_pass(name);
}

void test_QueueDirtyChunks() {
    const char* name = "test_QueueDirtyChunks";

    TerrainChunkMeshGenerator generator;
    generator.initialize(256, 256);

    ChunkDirtyTracker tracker(256, 256);
    tracker.markChunkDirty(0, 0);
    tracker.markChunkDirty(2, 3);
    tracker.markChunkDirty(5, 7);

    generator.queueDirtyChunks(tracker);

    ASSERT_EQ(generator.getPendingRebuildCount(), 3u, name);

    test_pass(name);
}

// ============================================================================
// AABB Tests
// ============================================================================

void test_AABB_Computation() {
    const char* name = "test_AABB_Computation";

    TerrainGrid grid(MapSize::Small);

    // Varying elevations in first chunk
    for (std::uint16_t y = 0; y < 32; ++y) {
        for (std::uint16_t x = 0; x < 32; ++x) {
            grid.at(x, y).setElevation(static_cast<std::uint8_t>((x + y) % 32));
            grid.at(x, y).setTerrainType(TerrainType::Substrate);
        }
    }

    TerrainChunkMeshGenerator generator;
    generator.initialize(grid.width, grid.height);

    ChunkMeshData mesh_data;
    generator.generateChunkMesh(grid, 0, 0, mesh_data);

    // Create a chunk and compute AABB
    TerrainChunk chunk(0, 0);
    chunk.computeAABB(mesh_data.max_elevation);

    // AABB min should be at (0, 0, 0)
    ASSERT_NEAR(chunk.aabb.min.x, 0.0f, 0.001f, name);
    ASSERT_NEAR(chunk.aabb.min.y, 0.0f, 0.001f, name);
    ASSERT_NEAR(chunk.aabb.min.z, 0.0f, 0.001f, name);

    // AABB max X and Z should be at chunk boundary (32)
    ASSERT_NEAR(chunk.aabb.max.x, 32.0f, 0.001f, name);
    ASSERT_NEAR(chunk.aabb.max.z, 32.0f, 0.001f, name);

    // AABB max Y should be at max_elevation * ELEVATION_HEIGHT
    float expected_max_y = static_cast<float>(mesh_data.max_elevation) * ELEVATION_HEIGHT;
    ASSERT_NEAR(chunk.aabb.max.y, expected_max_y, 0.001f, name);

    test_pass(name);
}

// ============================================================================
// Performance Tests
// ============================================================================

void test_Performance_SingleChunkRebuild() {
    const char* name = "test_Performance_SingleChunkRebuild";

    TerrainGrid grid(MapSize::Medium);  // 256x256

    // Fill with varied terrain
    for (std::uint16_t y = 0; y < grid.height; ++y) {
        for (std::uint16_t x = 0; x < grid.width; ++x) {
            grid.at(x, y).setElevation(static_cast<std::uint8_t>((x + y * 3) % 32));
            grid.at(x, y).setTerrainType(
                static_cast<TerrainType>((x + y) % TERRAIN_TYPE_COUNT));
        }
    }

    TerrainChunkMeshGenerator generator;
    generator.initialize(grid.width, grid.height);

    // Warm up
    ChunkMeshData mesh_data;
    generator.generateChunkMesh(grid, 0, 0, mesh_data);

    // Time 10 chunk generations
    const int iterations = 10;
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < iterations; ++i) {
        mesh_data.clear();
        generator.generateChunkMesh(grid, 0, 0, mesh_data);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration_us = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    double avg_ms = static_cast<double>(duration_us) / (1000.0 * iterations);

    std::cout << "  Average chunk mesh generation time: "
              << std::fixed << std::setprecision(3) << avg_ms << " ms" << std::endl;

    // Performance requirement: < 1ms
    if (avg_ms < 1.0) {
        test_pass(name);
    } else {
        test_fail(name, "Performance target not met (< 1ms per chunk)");
    }
}

void test_MultipleChunks_SecondChunk() {
    const char* name = "test_MultipleChunks_SecondChunk";

    TerrainGrid grid(MapSize::Small);

    // Fill all tiles
    for (std::uint16_t y = 0; y < grid.height; ++y) {
        for (std::uint16_t x = 0; x < grid.width; ++x) {
            grid.at(x, y).setElevation(10);
            grid.at(x, y).setTerrainType(TerrainType::Substrate);
        }
    }

    TerrainChunkMeshGenerator generator;
    generator.initialize(grid.width, grid.height);

    // Generate chunk (1, 0) - tiles 32-63 in X, 0-31 in Y
    ChunkMeshData mesh_data;
    generator.generateChunkMesh(grid, 1, 0, mesh_data);

    ASSERT_EQ(mesh_data.vertices.size(), VERTICES_PER_CHUNK, name);

    // First vertex should be at (32, elevation*HEIGHT, 0)
    ASSERT_NEAR(mesh_data.vertices[0].position_x, 32.0f, 0.001f, name);
    ASSERT_NEAR(mesh_data.vertices[0].position_z, 0.0f, 0.001f, name);

    // Tile coordinates should match world position
    ASSERT_NEAR(mesh_data.vertices[0].tile_coord_x, 32.0f, 0.001f, name);

    test_pass(name);
}

// ============================================================================
// Main
// ============================================================================

} // anonymous namespace

int main() {
    std::cout << "=== TerrainChunkMeshGenerator Tests (Ticket 3-025) ===" << std::endl;

    // Initialization tests
    test_DefaultConstruction();
    test_Initialize();
    test_SetCliffThreshold();

    // Mesh generation tests
    test_GenerateChunkMesh_FlatTerrain();
    test_GenerateChunkMesh_VertexPositions();
    test_GenerateChunkMesh_Normals();
    test_GenerateChunkMesh_TerrainType();
    test_GenerateChunkMesh_TileCoordinates();

    // Cliff face tests
    test_CliffFace_Generation();
    test_CliffFace_NoGeneration_BelowThreshold();
    test_CliffFace_HorizontalNormals();
    test_CliffFace_Disabled();

    // Rebuild queue tests
    test_QueueChunkRebuild();
    test_QueueDirtyChunks();

    // AABB tests
    test_AABB_Computation();

    // Multiple chunk tests
    test_MultipleChunks_SecondChunk();

    // Performance tests
    test_Performance_SingleChunkRebuild();

    std::cout << std::endl;
    std::cout << "=== Test Summary ===" << std::endl;
    std::cout << "Passed: " << tests_passed << std::endl;
    std::cout << "Failed: " << tests_failed << std::endl;

    return tests_failed > 0 ? 1 : 0;
}
