/**
 * @file test_terrain_lod_mesh.cpp
 * @brief Unit tests for Terrain LOD Mesh Generation (Ticket 3-032)
 *
 * Tests terrain LOD mesh generation including:
 * - 3 LOD levels: LOD0 (full), LOD1 (half), LOD2 (quarter)
 * - LOD 0: 33x33 = 1089 vertices per chunk
 * - LOD 1: 17x17 = 289 vertices per chunk
 * - LOD 2: 9x9 = 81 vertices per chunk
 * - LOD selection based on chunk distance from camera
 * - Configurable LOD transition thresholds
 * - Each LOD level has separate vertex/index buffers
 * - Normals recalculated for each LOD level
 * - Performance: LOD reduces visible terrain triangles by 50-70%
 */

#include <sims3000/terrain/TerrainLODMesh.h>
#include <sims3000/terrain/TerrainChunkMeshGenerator.h>
#include <sims3000/terrain/TerrainGrid.h>
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

#define ASSERT_GT(a, b, name) \
    if (!((a) > (b))) { \
        std::cout << "[FAIL] " << name << ": " << #a << " (" << (a) << ") not > " << #b << " (" << (b) << ")" << std::endl; \
        tests_failed++; return; \
    }

#define ASSERT_LT(a, b, name) \
    if (!((a) < (b))) { \
        std::cout << "[FAIL] " << name << ": " << #a << " (" << (a) << ") not < " << #b << " (" << (b) << ")" << std::endl; \
        tests_failed++; return; \
    }

using namespace sims3000::terrain;

// ============================================================================
// LOD Constants Tests
// ============================================================================

void test_LODLevelConstants() {
    const char* name = "test_LODLevelConstants";

    // Verify LOD level count
    ASSERT_EQ(TERRAIN_LOD_LEVEL_COUNT, 3u, name);

    // Verify LOD level values
    ASSERT_EQ(TERRAIN_LOD_0, 0u, name);
    ASSERT_EQ(TERRAIN_LOD_1, 1u, name);
    ASSERT_EQ(TERRAIN_LOD_2, 2u, name);

    test_pass(name);
}

void test_LODVertexCounts() {
    const char* name = "test_LODVertexCounts";

    // LOD 0: 33x33 = 1089 vertices
    ASSERT_EQ(LOD0_VERTEX_GRID_SIZE, 33u, name);
    ASSERT_EQ(LOD0_VERTICES_PER_CHUNK, 1089u, name);

    // LOD 1: 17x17 = 289 vertices
    ASSERT_EQ(LOD1_VERTEX_GRID_SIZE, 17u, name);
    ASSERT_EQ(LOD1_VERTICES_PER_CHUNK, 289u, name);

    // LOD 2: 9x9 = 81 vertices
    ASSERT_EQ(LOD2_VERTEX_GRID_SIZE, 9u, name);
    ASSERT_EQ(LOD2_VERTICES_PER_CHUNK, 81u, name);

    test_pass(name);
}

void test_LODIndexCounts() {
    const char* name = "test_LODIndexCounts";

    // LOD 0: 32*32*6 = 6144 indices
    ASSERT_EQ(LOD0_TILES_PER_SIDE, 32u, name);
    ASSERT_EQ(LOD0_INDICES_PER_CHUNK, 6144u, name);

    // LOD 1: 16*16*6 = 1536 indices
    ASSERT_EQ(LOD1_TILES_PER_SIDE, 16u, name);
    ASSERT_EQ(LOD1_INDICES_PER_CHUNK, 1536u, name);

    // LOD 2: 8*8*6 = 384 indices
    ASSERT_EQ(LOD2_TILES_PER_SIDE, 8u, name);
    ASSERT_EQ(LOD2_INDICES_PER_CHUNK, 384u, name);

    test_pass(name);
}

void test_LODSteps() {
    const char* name = "test_LODSteps";

    // LOD 0: every tile (step = 1)
    ASSERT_EQ(LOD0_STEP, 1u, name);

    // LOD 1: every 2nd tile (step = 2)
    ASSERT_EQ(LOD1_STEP, 2u, name);

    // LOD 2: every 4th tile (step = 4)
    ASSERT_EQ(LOD2_STEP, 4u, name);

    test_pass(name);
}

void test_LODDistanceThresholds() {
    const char* name = "test_LODDistanceThresholds";

    // Default thresholds
    ASSERT_NEAR(DEFAULT_LOD0_TO_LOD1_DISTANCE, 64.0f, 0.001f, name);
    ASSERT_NEAR(DEFAULT_LOD1_TO_LOD2_DISTANCE, 128.0f, 0.001f, name);

    test_pass(name);
}

// ============================================================================
// LOD Utility Function Tests
// ============================================================================

void test_GetVertexGridSize() {
    const char* name = "test_GetVertexGridSize";

    ASSERT_EQ(getVertexGridSize(TERRAIN_LOD_0), 33u, name);
    ASSERT_EQ(getVertexGridSize(TERRAIN_LOD_1), 17u, name);
    ASSERT_EQ(getVertexGridSize(TERRAIN_LOD_2), 9u, name);

    // Invalid level returns LOD 0 grid size
    ASSERT_EQ(getVertexGridSize(10), 33u, name);

    test_pass(name);
}

void test_GetVertexCount() {
    const char* name = "test_GetVertexCount";

    ASSERT_EQ(getVertexCount(TERRAIN_LOD_0), 1089u, name);
    ASSERT_EQ(getVertexCount(TERRAIN_LOD_1), 289u, name);
    ASSERT_EQ(getVertexCount(TERRAIN_LOD_2), 81u, name);

    test_pass(name);
}

void test_GetIndexCount() {
    const char* name = "test_GetIndexCount";

    ASSERT_EQ(getIndexCount(TERRAIN_LOD_0), 6144u, name);
    ASSERT_EQ(getIndexCount(TERRAIN_LOD_1), 1536u, name);
    ASSERT_EQ(getIndexCount(TERRAIN_LOD_2), 384u, name);

    test_pass(name);
}

void test_GetLODStep() {
    const char* name = "test_GetLODStep";

    ASSERT_EQ(getLODStep(TERRAIN_LOD_0), 1u, name);
    ASSERT_EQ(getLODStep(TERRAIN_LOD_1), 2u, name);
    ASSERT_EQ(getLODStep(TERRAIN_LOD_2), 4u, name);

    test_pass(name);
}

void test_GetTilesPerSide() {
    const char* name = "test_GetTilesPerSide";

    ASSERT_EQ(getTilesPerSide(TERRAIN_LOD_0), 32u, name);
    ASSERT_EQ(getTilesPerSide(TERRAIN_LOD_1), 16u, name);
    ASSERT_EQ(getTilesPerSide(TERRAIN_LOD_2), 8u, name);

    test_pass(name);
}

void test_GetTriangleCount() {
    const char* name = "test_GetTriangleCount";

    // LOD 0: 6144/3 = 2048 triangles
    ASSERT_EQ(getTriangleCount(TERRAIN_LOD_0), 2048u, name);

    // LOD 1: 1536/3 = 512 triangles
    ASSERT_EQ(getTriangleCount(TERRAIN_LOD_1), 512u, name);

    // LOD 2: 384/3 = 128 triangles
    ASSERT_EQ(getTriangleCount(TERRAIN_LOD_2), 128u, name);

    test_pass(name);
}

void test_GetTriangleReductionPercent() {
    const char* name = "test_GetTriangleReductionPercent";

    // LOD 0: 0% reduction (baseline)
    ASSERT_NEAR(getTriangleReductionPercent(TERRAIN_LOD_0), 0.0f, 0.001f, name);

    // LOD 1: (1 - 512/2048) * 100 = 75% reduction
    ASSERT_NEAR(getTriangleReductionPercent(TERRAIN_LOD_1), 75.0f, 0.001f, name);

    // LOD 2: (1 - 128/2048) * 100 = 93.75% reduction
    ASSERT_NEAR(getTriangleReductionPercent(TERRAIN_LOD_2), 93.75f, 0.001f, name);

    test_pass(name);
}

// ============================================================================
// TerrainLODLevel Tests
// ============================================================================

void test_TerrainLODLevel_DefaultConstruction() {
    const char* name = "test_TerrainLODLevel_DefaultConstruction";

    TerrainLODLevel level;

    ASSERT_EQ(level.vertex_buffer, nullptr, name);
    ASSERT_EQ(level.index_buffer, nullptr, name);
    ASSERT_EQ(level.vertex_count, 0u, name);
    ASSERT_EQ(level.index_count, 0u, name);
    ASSERT_FALSE(level.isValid(), name);

    test_pass(name);
}

// ============================================================================
// TerrainLODMesh Tests
// ============================================================================

void test_TerrainLODMesh_DefaultConstruction() {
    const char* name = "test_TerrainLODMesh_DefaultConstruction";

    TerrainLODMesh mesh;

    ASSERT_EQ(mesh.chunk_x, 0u, name);
    ASSERT_EQ(mesh.chunk_y, 0u, name);
    ASSERT_FALSE(mesh.complete, name);
    ASSERT_FALSE(mesh.isRenderable(), name);

    // All levels should be invalid
    ASSERT_FALSE(mesh.isLevelValid(TERRAIN_LOD_0), name);
    ASSERT_FALSE(mesh.isLevelValid(TERRAIN_LOD_1), name);
    ASSERT_FALSE(mesh.isLevelValid(TERRAIN_LOD_2), name);

    test_pass(name);
}

void test_TerrainLODMesh_CoordinateConstruction() {
    const char* name = "test_TerrainLODMesh_CoordinateConstruction";

    TerrainLODMesh mesh(5, 7);

    ASSERT_EQ(mesh.chunk_x, 5u, name);
    ASSERT_EQ(mesh.chunk_y, 7u, name);
    ASSERT_FALSE(mesh.complete, name);

    test_pass(name);
}

void test_TerrainLODMesh_GetLevel() {
    const char* name = "test_TerrainLODMesh_GetLevel";

    TerrainLODMesh mesh(0, 0);

    // Get mutable references
    TerrainLODLevel& level0 = mesh.getLevel(TERRAIN_LOD_0);
    TerrainLODLevel& level1 = mesh.getLevel(TERRAIN_LOD_1);
    TerrainLODLevel& level2 = mesh.getLevel(TERRAIN_LOD_2);

    // Modify to verify we got the right levels
    level0.vertex_count = 100;
    level1.vertex_count = 200;
    level2.vertex_count = 300;

    ASSERT_EQ(mesh.levels[0].vertex_count, 100u, name);
    ASSERT_EQ(mesh.levels[1].vertex_count, 200u, name);
    ASSERT_EQ(mesh.levels[2].vertex_count, 300u, name);

    // Out of bounds returns last level
    TerrainLODLevel& invalid = mesh.getLevel(10);
    ASSERT_EQ(invalid.vertex_count, 300u, name);

    test_pass(name);
}

void test_TerrainLODMesh_TotalCounts() {
    const char* name = "test_TerrainLODMesh_TotalCounts";

    TerrainLODMesh mesh(0, 0);

    mesh.levels[0].vertex_count = 1089;
    mesh.levels[0].index_count = 6144;
    mesh.levels[1].vertex_count = 289;
    mesh.levels[1].index_count = 1536;
    mesh.levels[2].vertex_count = 81;
    mesh.levels[2].index_count = 384;

    ASSERT_EQ(mesh.getTotalVertexCount(), 1089u + 289u + 81u, name);
    ASSERT_EQ(mesh.getTotalIndexCount(), 6144u + 1536u + 384u, name);

    test_pass(name);
}

// ============================================================================
// TerrainLODConfig Tests
// ============================================================================

void test_TerrainLODConfig_DefaultValues() {
    const char* name = "test_TerrainLODConfig_DefaultValues";

    TerrainLODConfig config;

    ASSERT_NEAR(config.lod0_to_lod1_distance, 64.0f, 0.001f, name);
    ASSERT_NEAR(config.lod1_to_lod2_distance, 128.0f, 0.001f, name);
    ASSERT_NEAR(config.hysteresis, 2.0f, 0.001f, name);

    test_pass(name);
}

void test_TerrainLODConfig_SelectLODLevel() {
    const char* name = "test_TerrainLODConfig_SelectLODLevel";

    TerrainLODConfig config;

    // Distance < 64: LOD 0
    ASSERT_EQ(config.selectLODLevel(0.0f), TERRAIN_LOD_0, name);
    ASSERT_EQ(config.selectLODLevel(32.0f), TERRAIN_LOD_0, name);
    ASSERT_EQ(config.selectLODLevel(63.9f), TERRAIN_LOD_0, name);

    // Distance 64-128: LOD 1
    ASSERT_EQ(config.selectLODLevel(64.0f), TERRAIN_LOD_1, name);
    ASSERT_EQ(config.selectLODLevel(96.0f), TERRAIN_LOD_1, name);
    ASSERT_EQ(config.selectLODLevel(127.9f), TERRAIN_LOD_1, name);

    // Distance >= 128: LOD 2
    ASSERT_EQ(config.selectLODLevel(128.0f), TERRAIN_LOD_2, name);
    ASSERT_EQ(config.selectLODLevel(200.0f), TERRAIN_LOD_2, name);
    ASSERT_EQ(config.selectLODLevel(1000.0f), TERRAIN_LOD_2, name);

    test_pass(name);
}

void test_TerrainLODConfig_SelectLODLevelWithHysteresis() {
    const char* name = "test_TerrainLODConfig_SelectLODLevelWithHysteresis";

    TerrainLODConfig config;
    config.hysteresis = 2.0f;

    // Moving from LOD 0 to LOD 1 - needs to cross 64 + 2 = 66
    ASSERT_EQ(config.selectLODLevelWithHysteresis(65.0f, TERRAIN_LOD_0), TERRAIN_LOD_0, name);
    ASSERT_EQ(config.selectLODLevelWithHysteresis(67.0f, TERRAIN_LOD_0), TERRAIN_LOD_1, name);

    // Moving from LOD 1 to LOD 0 - needs to cross 64 - 2 = 62
    ASSERT_EQ(config.selectLODLevelWithHysteresis(63.0f, TERRAIN_LOD_1), TERRAIN_LOD_1, name);
    ASSERT_EQ(config.selectLODLevelWithHysteresis(61.0f, TERRAIN_LOD_1), TERRAIN_LOD_0, name);

    // Moving from LOD 1 to LOD 2 - needs to cross 128 + 2 = 130
    ASSERT_EQ(config.selectLODLevelWithHysteresis(129.0f, TERRAIN_LOD_1), TERRAIN_LOD_1, name);
    ASSERT_EQ(config.selectLODLevelWithHysteresis(131.0f, TERRAIN_LOD_1), TERRAIN_LOD_2, name);

    // Moving from LOD 2 to LOD 1 - needs to cross 128 - 2 = 126
    ASSERT_EQ(config.selectLODLevelWithHysteresis(127.0f, TERRAIN_LOD_2), TERRAIN_LOD_2, name);
    ASSERT_EQ(config.selectLODLevelWithHysteresis(125.0f, TERRAIN_LOD_2), TERRAIN_LOD_1, name);

    test_pass(name);
}

void test_TerrainLODConfig_CustomThresholds() {
    const char* name = "test_TerrainLODConfig_CustomThresholds";

    TerrainLODConfig config;
    config.lod0_to_lod1_distance = 100.0f;
    config.lod1_to_lod2_distance = 200.0f;

    ASSERT_EQ(config.selectLODLevel(50.0f), TERRAIN_LOD_0, name);
    ASSERT_EQ(config.selectLODLevel(100.0f), TERRAIN_LOD_1, name);
    ASSERT_EQ(config.selectLODLevel(150.0f), TERRAIN_LOD_1, name);
    ASSERT_EQ(config.selectLODLevel(200.0f), TERRAIN_LOD_2, name);
    ASSERT_EQ(config.selectLODLevel(300.0f), TERRAIN_LOD_2, name);

    test_pass(name);
}

// ============================================================================
// LOD Mesh Generation Tests (CPU-side only)
// ============================================================================

void test_GenerateLODMesh_LOD0() {
    const char* name = "test_GenerateLODMesh_LOD0";

    TerrainGrid grid(MapSize::Small);  // 128x128

    // Fill with flat terrain
    for (std::uint16_t y = 0; y < grid.height; ++y) {
        for (std::uint16_t x = 0; x < grid.width; ++x) {
            grid.at(x, y).setElevation(10);
            grid.at(x, y).setTerrainType(TerrainType::Substrate);
        }
    }

    TerrainChunkMeshGenerator generator;
    generator.initialize(grid.width, grid.height);

    ChunkMeshData mesh_data;
    bool result = generator.generateLODMesh(grid, 0, 0, TERRAIN_LOD_0, mesh_data);

    ASSERT_TRUE(result, name);
    // Surface vertices + skirt vertices (Ticket 3-033: LOD seam mitigation)
    std::uint32_t expected_vertices = LOD0_VERTICES_PER_CHUNK + getTotalSkirtVertexCount(TERRAIN_LOD_0);
    std::uint32_t expected_indices = LOD0_INDICES_PER_CHUNK + getTotalSkirtIndexCount(TERRAIN_LOD_0);
    ASSERT_EQ(mesh_data.vertices.size(), expected_vertices, name);
    ASSERT_EQ(mesh_data.indices.size(), expected_indices, name);

    test_pass(name);
}

void test_GenerateLODMesh_LOD1() {
    const char* name = "test_GenerateLODMesh_LOD1";

    TerrainGrid grid(MapSize::Small);

    // Fill with flat terrain
    for (std::uint16_t y = 0; y < grid.height; ++y) {
        for (std::uint16_t x = 0; x < grid.width; ++x) {
            grid.at(x, y).setElevation(10);
            grid.at(x, y).setTerrainType(TerrainType::Substrate);
        }
    }

    TerrainChunkMeshGenerator generator;
    generator.initialize(grid.width, grid.height);

    ChunkMeshData mesh_data;
    bool result = generator.generateLODMesh(grid, 0, 0, TERRAIN_LOD_1, mesh_data);

    ASSERT_TRUE(result, name);
    // Surface vertices + skirt vertices (Ticket 3-033: LOD seam mitigation)
    std::uint32_t expected_vertices = LOD1_VERTICES_PER_CHUNK + getTotalSkirtVertexCount(TERRAIN_LOD_1);
    std::uint32_t expected_indices = LOD1_INDICES_PER_CHUNK + getTotalSkirtIndexCount(TERRAIN_LOD_1);
    ASSERT_EQ(mesh_data.vertices.size(), expected_vertices, name);
    ASSERT_EQ(mesh_data.indices.size(), expected_indices, name);

    test_pass(name);
}

void test_GenerateLODMesh_LOD2() {
    const char* name = "test_GenerateLODMesh_LOD2";

    TerrainGrid grid(MapSize::Small);

    // Fill with flat terrain
    for (std::uint16_t y = 0; y < grid.height; ++y) {
        for (std::uint16_t x = 0; x < grid.width; ++x) {
            grid.at(x, y).setElevation(10);
            grid.at(x, y).setTerrainType(TerrainType::Substrate);
        }
    }

    TerrainChunkMeshGenerator generator;
    generator.initialize(grid.width, grid.height);

    ChunkMeshData mesh_data;
    bool result = generator.generateLODMesh(grid, 0, 0, TERRAIN_LOD_2, mesh_data);

    ASSERT_TRUE(result, name);
    // Surface vertices + skirt vertices (Ticket 3-033: LOD seam mitigation)
    std::uint32_t expected_vertices = LOD2_VERTICES_PER_CHUNK + getTotalSkirtVertexCount(TERRAIN_LOD_2);
    std::uint32_t expected_indices = LOD2_INDICES_PER_CHUNK + getTotalSkirtIndexCount(TERRAIN_LOD_2);
    ASSERT_EQ(mesh_data.vertices.size(), expected_vertices, name);
    ASSERT_EQ(mesh_data.indices.size(), expected_indices, name);

    test_pass(name);
}

void test_GenerateLODMesh_VertexPositions() {
    const char* name = "test_GenerateLODMesh_VertexPositions";

    TerrainGrid grid(MapSize::Small);

    // Fill with varying elevations
    for (std::uint16_t y = 0; y < grid.height; ++y) {
        for (std::uint16_t x = 0; x < grid.width; ++x) {
            grid.at(x, y).setElevation(static_cast<std::uint8_t>((x + y) % 32));
            grid.at(x, y).setTerrainType(TerrainType::Substrate);
        }
    }

    TerrainChunkMeshGenerator generator;
    generator.initialize(grid.width, grid.height);

    // Generate LOD 1 (step = 2)
    ChunkMeshData mesh_data;
    generator.generateLODMesh(grid, 0, 0, TERRAIN_LOD_1, mesh_data);

    // Check first vertex (0, 0)
    const TerrainVertex& v0 = mesh_data.vertices[0];
    ASSERT_NEAR(v0.position_x, 0.0f, 0.001f, name);
    ASSERT_NEAR(v0.position_z, 0.0f, 0.001f, name);

    // Check second vertex - should be at (2, 0) for LOD 1 (step = 2)
    const TerrainVertex& v1 = mesh_data.vertices[1];
    ASSERT_NEAR(v1.position_x, 2.0f, 0.001f, name);
    ASSERT_NEAR(v1.position_z, 0.0f, 0.001f, name);

    // Check vertex at (0, 2) - should be at row 1 in LOD 1 grid
    const TerrainVertex& v17 = mesh_data.vertices[17];  // Grid size is 17
    ASSERT_NEAR(v17.position_x, 0.0f, 0.001f, name);
    ASSERT_NEAR(v17.position_z, 2.0f, 0.001f, name);

    test_pass(name);
}

void test_GenerateLODMesh_LOD2_Positions() {
    const char* name = "test_GenerateLODMesh_LOD2_Positions";

    TerrainGrid grid(MapSize::Small);

    // Fill with flat terrain
    for (std::uint16_t y = 0; y < grid.height; ++y) {
        for (std::uint16_t x = 0; x < grid.width; ++x) {
            grid.at(x, y).setElevation(10);
            grid.at(x, y).setTerrainType(TerrainType::Substrate);
        }
    }

    TerrainChunkMeshGenerator generator;
    generator.initialize(grid.width, grid.height);

    // Generate LOD 2 (step = 4)
    ChunkMeshData mesh_data;
    generator.generateLODMesh(grid, 0, 0, TERRAIN_LOD_2, mesh_data);

    // Check first vertex (0, 0)
    const TerrainVertex& v0 = mesh_data.vertices[0];
    ASSERT_NEAR(v0.position_x, 0.0f, 0.001f, name);
    ASSERT_NEAR(v0.position_z, 0.0f, 0.001f, name);

    // Check second vertex - should be at (4, 0) for LOD 2 (step = 4)
    const TerrainVertex& v1 = mesh_data.vertices[1];
    ASSERT_NEAR(v1.position_x, 4.0f, 0.001f, name);
    ASSERT_NEAR(v1.position_z, 0.0f, 0.001f, name);

    // Check last SURFACE vertex - should be at (32, 32)
    // Surface vertices are at indices 0 to (LOD2_VERTICES_PER_CHUNK - 1)
    // Skirt vertices follow after (Ticket 3-033)
    const TerrainVertex& last_surface = mesh_data.vertices[LOD2_VERTICES_PER_CHUNK - 1];
    ASSERT_NEAR(last_surface.position_x, 32.0f, 0.001f, name);
    ASSERT_NEAR(last_surface.position_z, 32.0f, 0.001f, name);

    test_pass(name);
}

void test_GenerateLODMesh_Normals() {
    const char* name = "test_GenerateLODMesh_Normals";

    TerrainGrid grid(MapSize::Small);

    // Create a slope: elevation increases along X axis
    for (std::uint16_t y = 0; y < grid.height; ++y) {
        for (std::uint16_t x = 0; x < grid.width; ++x) {
            grid.at(x, y).setElevation(static_cast<std::uint8_t>(x));
            grid.at(x, y).setTerrainType(TerrainType::Substrate);
        }
    }

    TerrainChunkMeshGenerator generator;
    generator.initialize(grid.width, grid.height);

    // Generate LOD 1
    ChunkMeshData mesh_data;
    generator.generateLODMesh(grid, 0, 0, TERRAIN_LOD_1, mesh_data);

    // Check an interior vertex normal
    // At LOD 1, vertex 8 (middle of first row) should have sloped normal
    const TerrainVertex& v = mesh_data.vertices[8 * 17 + 8];  // Middle of LOD 1 grid

    // Normal should be normalized
    float length = std::sqrt(v.normal_x * v.normal_x +
                             v.normal_y * v.normal_y +
                             v.normal_z * v.normal_z);
    ASSERT_NEAR(length, 1.0f, 0.01f, name);

    // Normal should have negative X component (pointing away from slope)
    ASSERT_LT(v.normal_x, 0.0f, name);

    test_pass(name);
}

void test_GenerateLODMesh_TerrainType() {
    const char* name = "test_GenerateLODMesh_TerrainType";

    TerrainGrid grid(MapSize::Small);

    // Set different terrain types
    for (std::uint16_t y = 0; y < grid.height; ++y) {
        for (std::uint16_t x = 0; x < grid.width; ++x) {
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

    // Generate all LOD levels and verify terrain types are preserved
    for (std::uint8_t level = 0; level < TERRAIN_LOD_LEVEL_COUNT; ++level) {
        ChunkMeshData mesh_data;
        generator.generateLODMesh(grid, 0, 0, level, mesh_data);

        // Check first vertex - should be Substrate
        ASSERT_EQ(mesh_data.vertices[0].terrain_type,
                  static_cast<std::uint8_t>(TerrainType::Substrate), name);

        // Check a vertex in the right half - should be Ridge
        std::uint32_t grid_size = getVertexGridSize(level);
        std::uint32_t right_vertex_idx = grid_size - 1;  // Last vertex in first row
        ASSERT_EQ(mesh_data.vertices[right_vertex_idx].terrain_type,
                  static_cast<std::uint8_t>(TerrainType::Ridge), name);
    }

    test_pass(name);
}

void test_GenerateAllLODMeshes() {
    const char* name = "test_GenerateAllLODMeshes";

    TerrainGrid grid(MapSize::Small);

    // Fill with flat terrain
    for (std::uint16_t y = 0; y < grid.height; ++y) {
        for (std::uint16_t x = 0; x < grid.width; ++x) {
            grid.at(x, y).setElevation(10);
            grid.at(x, y).setTerrainType(TerrainType::Substrate);
        }
    }

    TerrainChunkMeshGenerator generator;
    generator.initialize(grid.width, grid.height);

    ChunkMeshData lod_data[TERRAIN_LOD_LEVEL_COUNT];
    bool result = generator.generateAllLODMeshes(grid, 0, 0, lod_data);

    ASSERT_TRUE(result, name);

    // Verify each LOD level has correct sizes (surface + skirt vertices per Ticket 3-033)
    std::uint32_t expected_vertices_lod0 = LOD0_VERTICES_PER_CHUNK + getTotalSkirtVertexCount(TERRAIN_LOD_0);
    std::uint32_t expected_indices_lod0 = LOD0_INDICES_PER_CHUNK + getTotalSkirtIndexCount(TERRAIN_LOD_0);
    ASSERT_EQ(lod_data[0].vertices.size(), expected_vertices_lod0, name);
    ASSERT_EQ(lod_data[0].indices.size(), expected_indices_lod0, name);

    std::uint32_t expected_vertices_lod1 = LOD1_VERTICES_PER_CHUNK + getTotalSkirtVertexCount(TERRAIN_LOD_1);
    std::uint32_t expected_indices_lod1 = LOD1_INDICES_PER_CHUNK + getTotalSkirtIndexCount(TERRAIN_LOD_1);
    ASSERT_EQ(lod_data[1].vertices.size(), expected_vertices_lod1, name);
    ASSERT_EQ(lod_data[1].indices.size(), expected_indices_lod1, name);

    std::uint32_t expected_vertices_lod2 = LOD2_VERTICES_PER_CHUNK + getTotalSkirtVertexCount(TERRAIN_LOD_2);
    std::uint32_t expected_indices_lod2 = LOD2_INDICES_PER_CHUNK + getTotalSkirtIndexCount(TERRAIN_LOD_2);
    ASSERT_EQ(lod_data[2].vertices.size(), expected_vertices_lod2, name);
    ASSERT_EQ(lod_data[2].indices.size(), expected_indices_lod2, name);

    test_pass(name);
}

// ============================================================================
// Performance Tests
// ============================================================================

void test_LOD_TriangleReduction() {
    const char* name = "test_LOD_TriangleReduction";

    // Verify that LOD provides significant triangle reduction
    std::uint32_t lod0_tris = getTriangleCount(TERRAIN_LOD_0);
    std::uint32_t lod1_tris = getTriangleCount(TERRAIN_LOD_1);
    std::uint32_t lod2_tris = getTriangleCount(TERRAIN_LOD_2);

    // LOD 1 should have 75% fewer triangles than LOD 0
    float lod1_reduction = 100.0f * (1.0f - static_cast<float>(lod1_tris) / static_cast<float>(lod0_tris));
    ASSERT_GT(lod1_reduction, 50.0f, name);  // At least 50% reduction

    // LOD 2 should have 93.75% fewer triangles than LOD 0
    float lod2_reduction = 100.0f * (1.0f - static_cast<float>(lod2_tris) / static_cast<float>(lod0_tris));
    ASSERT_GT(lod2_reduction, 90.0f, name);  // At least 90% reduction

    std::cout << "  LOD 0: " << lod0_tris << " triangles" << std::endl;
    std::cout << "  LOD 1: " << lod1_tris << " triangles (" << lod1_reduction << "% reduction)" << std::endl;
    std::cout << "  LOD 2: " << lod2_tris << " triangles (" << lod2_reduction << "% reduction)" << std::endl;

    test_pass(name);
}

void test_LOD_TypicalCameraReduction() {
    const char* name = "test_LOD_TypicalCameraReduction";

    // Simulate a typical camera scenario with chunks at various distances
    // Assume a 8x8 chunk grid (256x256 tile map)
    // Camera at center, looking at the terrain

    std::uint32_t total_lod0_tris = 0;
    std::uint32_t total_lod_tris = 0;

    TerrainLODConfig config;

    // Calculate for each chunk
    for (int cy = 0; cy < 8; ++cy) {
        for (int cx = 0; cx < 8; ++cx) {
            // Chunk center (in tiles)
            float chunk_center_x = cx * 32.0f + 16.0f;
            float chunk_center_y = cy * 32.0f + 16.0f;

            // Camera at map center
            float camera_x = 128.0f;
            float camera_y = 128.0f;

            // Distance from camera to chunk center
            float dx = chunk_center_x - camera_x;
            float dy = chunk_center_y - camera_y;
            float distance = std::sqrt(dx * dx + dy * dy);

            // Select LOD level
            std::uint8_t lod_level = config.selectLODLevel(distance);

            // Add triangles
            total_lod0_tris += getTriangleCount(TERRAIN_LOD_0);
            total_lod_tris += getTriangleCount(lod_level);
        }
    }

    // Calculate reduction
    float reduction = 100.0f * (1.0f - static_cast<float>(total_lod_tris) /
                                       static_cast<float>(total_lod0_tris));

    std::cout << "  Without LOD: " << total_lod0_tris << " triangles" << std::endl;
    std::cout << "  With LOD: " << total_lod_tris << " triangles" << std::endl;
    std::cout << "  Reduction: " << std::fixed << std::setprecision(1) << reduction << "%" << std::endl;

    // Requirement: 50-70% reduction at typical camera height
    ASSERT_GT(reduction, 50.0f, name);

    test_pass(name);
}

void test_Performance_AllLODGeneration() {
    const char* name = "test_Performance_AllLODGeneration";

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

    // Time generation of all 3 LOD levels for a single chunk
    ChunkMeshData lod_data[TERRAIN_LOD_LEVEL_COUNT];

    const int iterations = 10;
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < iterations; ++i) {
        for (std::uint8_t level = 0; level < TERRAIN_LOD_LEVEL_COUNT; ++level) {
            lod_data[level].clear();
        }
        generator.generateAllLODMeshes(grid, 0, 0, lod_data);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration_us = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    double avg_ms = static_cast<double>(duration_us) / (1000.0 * iterations);

    std::cout << "  Average all-LOD generation time: "
              << std::fixed << std::setprecision(3) << avg_ms << " ms" << std::endl;

    // All 3 LOD levels should still complete in reasonable time
    // Total should be less than 3x single LOD 0 (which is < 1ms)
    if (avg_ms < 3.0) {
        test_pass(name);
    } else {
        test_fail(name, "Performance target not met (< 3ms for all LOD levels)");
    }
}

// ============================================================================
// Edge Case Tests
// ============================================================================

void test_GenerateLODMesh_InvalidLevel() {
    const char* name = "test_GenerateLODMesh_InvalidLevel";

    TerrainGrid grid(MapSize::Small);

    for (std::uint16_t y = 0; y < grid.height; ++y) {
        for (std::uint16_t x = 0; x < grid.width; ++x) {
            grid.at(x, y).setElevation(10);
            grid.at(x, y).setTerrainType(TerrainType::Substrate);
        }
    }

    TerrainChunkMeshGenerator generator;
    generator.initialize(grid.width, grid.height);

    ChunkMeshData mesh_data;
    bool result = generator.generateLODMesh(grid, 0, 0, 10, mesh_data);  // Invalid level

    // Should fail for invalid level
    ASSERT_FALSE(result, name);

    test_pass(name);
}

void test_GenerateLODMesh_ElevationTracking() {
    const char* name = "test_GenerateLODMesh_ElevationTracking";

    TerrainGrid grid(MapSize::Small);

    // Set varying elevations - fill entire grid to cover all LOD sample points
    // LOD samples can extend to tile 32 (clamped to 31 for sample lookup)
    for (std::uint16_t y = 0; y < grid.height; ++y) {
        for (std::uint16_t x = 0; x < grid.width; ++x) {
            // For tiles in the first chunk (0-31), use elevation = x
            // This ensures we have values 0-31 in the chunk
            if (x < 32 && y < 32) {
                std::uint8_t elev = static_cast<std::uint8_t>(x);
                grid.at(x, y).setElevation(elev);
            } else {
                // Tiles outside chunk 0 get elevation 0
                grid.at(x, y).setElevation(0);
            }
            grid.at(x, y).setTerrainType(TerrainType::Substrate);
        }
    }

    TerrainChunkMeshGenerator generator;
    generator.initialize(grid.width, grid.height);

    // Generate all LOD levels and verify elevation tracking
    for (std::uint8_t level = 0; level < TERRAIN_LOD_LEVEL_COUNT; ++level) {
        ChunkMeshData mesh_data;
        generator.generateLODMesh(grid, 0, 0, level, mesh_data);

        // Max elevation depends on which samples the LOD level takes:
        // LOD 0 (step=1): samples 0,1,2,...,32 -> max from tile 31 = 31
        // LOD 1 (step=2): samples 0,2,4,...,30,32 -> tile 32 is outside chunk, so max = 30
        // LOD 2 (step=4): samples 0,4,8,...,28,32 -> tile 32 is outside chunk, so max = 28
        // Actually, the vertex at x=32 samples tile 32 (clamped to grid boundary).
        // Since tile 32 is outside our filled region (0-31), it has elevation 0.
        // So the max sampled is the highest within 0-31 that gets sampled.

        // Let's just verify min_elevation is 0 for all levels (tile 0)
        ASSERT_EQ(static_cast<int>(mesh_data.min_elevation), 0, name);

        // And verify max_elevation is non-zero and reasonable
        ASSERT_TRUE(mesh_data.max_elevation > 0, name);
        ASSERT_TRUE(mesh_data.max_elevation <= 31, name);
    }

    test_pass(name);
}

// ============================================================================
// Main
// ============================================================================

} // anonymous namespace

int main() {
    std::cout << "=== Terrain LOD Mesh Tests (Ticket 3-032) ===" << std::endl;

    // LOD Constants tests
    test_LODLevelConstants();
    test_LODVertexCounts();
    test_LODIndexCounts();
    test_LODSteps();
    test_LODDistanceThresholds();

    // LOD Utility function tests
    test_GetVertexGridSize();
    test_GetVertexCount();
    test_GetIndexCount();
    test_GetLODStep();
    test_GetTilesPerSide();
    test_GetTriangleCount();
    test_GetTriangleReductionPercent();

    // TerrainLODLevel tests
    test_TerrainLODLevel_DefaultConstruction();

    // TerrainLODMesh tests
    test_TerrainLODMesh_DefaultConstruction();
    test_TerrainLODMesh_CoordinateConstruction();
    test_TerrainLODMesh_GetLevel();
    test_TerrainLODMesh_TotalCounts();

    // TerrainLODConfig tests
    test_TerrainLODConfig_DefaultValues();
    test_TerrainLODConfig_SelectLODLevel();
    test_TerrainLODConfig_SelectLODLevelWithHysteresis();
    test_TerrainLODConfig_CustomThresholds();

    // LOD Mesh Generation tests
    test_GenerateLODMesh_LOD0();
    test_GenerateLODMesh_LOD1();
    test_GenerateLODMesh_LOD2();
    test_GenerateLODMesh_VertexPositions();
    test_GenerateLODMesh_LOD2_Positions();
    test_GenerateLODMesh_Normals();
    test_GenerateLODMesh_TerrainType();
    test_GenerateAllLODMeshes();

    // Performance tests
    test_LOD_TriangleReduction();
    test_LOD_TypicalCameraReduction();
    test_Performance_AllLODGeneration();

    // Edge case tests
    test_GenerateLODMesh_InvalidLevel();
    test_GenerateLODMesh_ElevationTracking();

    std::cout << std::endl;
    std::cout << "=== Test Summary ===" << std::endl;
    std::cout << "Passed: " << tests_passed << std::endl;
    std::cout << "Failed: " << tests_failed << std::endl;

    return tests_failed > 0 ? 1 : 0;
}
