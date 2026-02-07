/**
 * @file test_water_mesh.cpp
 * @brief Unit tests for WaterMesh and WaterMeshGenerator (Ticket 3-027)
 *
 * Tests:
 * - WaterVertex struct size and layout
 * - WaterMesh construction and state
 * - WaterMeshGenerator mesh generation
 * - Shore factor calculation
 * - Per-body mesh generation (ocean, rivers, lakes)
 * - Draw call estimation
 */

#include <sims3000/terrain/WaterMesh.h>
#include <cstdio>
#include <cmath>
#include <unordered_set>

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

#define TEST_ASSERT_EQ(actual, expected, message) \
    do { \
        if ((actual) == (expected)) { \
            tests_passed++; \
            printf("[PASS] %s\n", message); \
        } else { \
            tests_failed++; \
            printf("[FAIL] %s (expected %d, got %d)\n", message, (int)(expected), (int)(actual)); \
        } \
    } while (0)

#define TEST_ASSERT_FLOAT_EQ(actual, expected, message) \
    do { \
        if (std::fabs((actual) - (expected)) < 0.0001f) { \
            tests_passed++; \
            printf("[PASS] %s\n", message); \
        } else { \
            tests_failed++; \
            printf("[FAIL] %s (expected %.4f, got %.4f)\n", message, (float)(expected), (float)(actual)); \
        } \
    } while (0)

// ============================================================================
// Test: WaterVertex struct size and layout
// ============================================================================
void test_water_vertex_layout() {
    printf("\n--- Test: WaterVertex Layout ---\n");

    // WaterVertex should be 28 bytes
    TEST_ASSERT_EQ(sizeof(WaterVertex), 28, "WaterVertex is 28 bytes");

    // Check offsets
    TEST_ASSERT_EQ(offsetof(WaterVertex, position_x), 0, "position_x at offset 0");
    TEST_ASSERT_EQ(offsetof(WaterVertex, position_y), 4, "position_y at offset 4");
    TEST_ASSERT_EQ(offsetof(WaterVertex, position_z), 8, "position_z at offset 8");
    TEST_ASSERT_EQ(offsetof(WaterVertex, shore_factor), 12, "shore_factor at offset 12");
    TEST_ASSERT_EQ(offsetof(WaterVertex, water_body_id), 16, "water_body_id at offset 16");
    TEST_ASSERT_EQ(offsetof(WaterVertex, uv_u), 20, "uv_u at offset 20");
    TEST_ASSERT_EQ(offsetof(WaterVertex, uv_v), 24, "uv_v at offset 24");
}

// ============================================================================
// Test: WaterVertex default construction
// ============================================================================
void test_water_vertex_default_construction() {
    printf("\n--- Test: WaterVertex Default Construction ---\n");

    WaterVertex vert;

    TEST_ASSERT_FLOAT_EQ(vert.position_x, 0.0f, "Default position_x is 0");
    TEST_ASSERT_FLOAT_EQ(vert.position_y, 0.0f, "Default position_y is 0");
    TEST_ASSERT_FLOAT_EQ(vert.position_z, 0.0f, "Default position_z is 0");
    TEST_ASSERT_FLOAT_EQ(vert.shore_factor, 0.0f, "Default shore_factor is 0");
    TEST_ASSERT_EQ(vert.water_body_id, 0, "Default water_body_id is 0");
    TEST_ASSERT_FLOAT_EQ(vert.uv_u, 0.0f, "Default uv_u is 0");
    TEST_ASSERT_FLOAT_EQ(vert.uv_v, 0.0f, "Default uv_v is 0");
}

// ============================================================================
// Test: WaterVertex parameterized construction
// ============================================================================
void test_water_vertex_param_construction() {
    printf("\n--- Test: WaterVertex Parameterized Construction ---\n");

    WaterVertex vert(10.0f, 2.0f, 20.0f, 0.5f, 42, 0.25f, 0.75f);

    TEST_ASSERT_FLOAT_EQ(vert.position_x, 10.0f, "position_x is 10.0");
    TEST_ASSERT_FLOAT_EQ(vert.position_y, 2.0f, "position_y is 2.0");
    TEST_ASSERT_FLOAT_EQ(vert.position_z, 20.0f, "position_z is 20.0");
    TEST_ASSERT_FLOAT_EQ(vert.shore_factor, 0.5f, "shore_factor is 0.5");
    TEST_ASSERT_EQ(vert.water_body_id, 42, "water_body_id is 42");
    TEST_ASSERT_FLOAT_EQ(vert.uv_u, 0.25f, "uv_u is 0.25");
    TEST_ASSERT_FLOAT_EQ(vert.uv_v, 0.75f, "uv_v is 0.75");
}

// ============================================================================
// Test: WaterMesh default construction
// ============================================================================
void test_water_mesh_default_construction() {
    printf("\n--- Test: WaterMesh Default Construction ---\n");

    WaterMesh mesh;

    TEST_ASSERT_EQ(mesh.body_id, NO_WATER_BODY, "Default body_id is NO_WATER_BODY");
    TEST_ASSERT(mesh.vertex_buffer == nullptr, "Default vertex_buffer is nullptr");
    TEST_ASSERT(mesh.index_buffer == nullptr, "Default index_buffer is nullptr");
    TEST_ASSERT_EQ(mesh.vertex_count, 0, "Default vertex_count is 0");
    TEST_ASSERT_EQ(mesh.index_count, 0, "Default index_count is 0");
    TEST_ASSERT(mesh.vertices.empty(), "Default vertices is empty");
    TEST_ASSERT(mesh.indices.empty(), "Default indices is empty");
    TEST_ASSERT(mesh.dirty, "Default mesh is dirty");
    TEST_ASSERT(!mesh.has_gpu_resources, "Default has_gpu_resources is false");
}

// ============================================================================
// Test: WaterMesh state methods
// ============================================================================
void test_water_mesh_state_methods() {
    printf("\n--- Test: WaterMesh State Methods ---\n");

    WaterMesh mesh(1, WaterBodyType::Ocean);

    // Initial state
    TEST_ASSERT(mesh.isDirty(), "New mesh is dirty");
    TEST_ASSERT(!mesh.hasGPUResources(), "New mesh has no GPU resources");
    TEST_ASSERT(!mesh.isRenderable(), "New mesh is not renderable");
    TEST_ASSERT(mesh.isEmpty(), "New mesh is empty");

    // After adding some data
    mesh.vertices.push_back(WaterVertex());
    mesh.indices.push_back(0);
    mesh.index_count = 1;

    TEST_ASSERT(!mesh.isEmpty(), "Mesh with indices is not empty");

    // Clear dirty
    mesh.clearDirty();
    TEST_ASSERT(!mesh.isDirty(), "After clearDirty, isDirty is false");

    // Mark dirty
    mesh.markDirty();
    TEST_ASSERT(mesh.isDirty(), "After markDirty, isDirty is true");

    // Simulate GPU resources
    mesh.vertex_buffer = reinterpret_cast<SDL_GPUBuffer*>(0x12345678);
    mesh.index_buffer = reinterpret_cast<SDL_GPUBuffer*>(0x87654321);
    mesh.has_gpu_resources = true;

    TEST_ASSERT(mesh.hasGPUResources(), "After setting buffers, hasGPUResources is true");
    TEST_ASSERT(!mesh.isRenderable(), "Dirty mesh is not renderable");

    mesh.clearDirty();
    TEST_ASSERT(mesh.isRenderable(), "Clean mesh with resources is renderable");

    // Reset fake pointers
    mesh.vertex_buffer = nullptr;
    mesh.index_buffer = nullptr;
    mesh.has_gpu_resources = false;
}

// ============================================================================
// Test: WaterMesh move semantics
// ============================================================================
void test_water_mesh_move_semantics() {
    printf("\n--- Test: WaterMesh Move Semantics ---\n");

    WaterMesh mesh1(5, WaterBodyType::River);
    mesh1.vertices.push_back(WaterVertex(1.0f, 2.0f, 3.0f, 0.5f, 5, 0.0f, 0.0f));
    mesh1.indices.push_back(0);
    mesh1.vertex_buffer = reinterpret_cast<SDL_GPUBuffer*>(0x11111111);
    mesh1.index_buffer = reinterpret_cast<SDL_GPUBuffer*>(0x22222222);
    mesh1.has_gpu_resources = true;

    // Move construct
    WaterMesh mesh2(std::move(mesh1));

    TEST_ASSERT_EQ(mesh2.body_id, 5, "Moved body_id is 5");
    TEST_ASSERT(mesh2.body_type == WaterBodyType::River, "Moved body_type is River");
    TEST_ASSERT_EQ(mesh2.vertices.size(), 1, "Moved vertices size is 1");
    TEST_ASSERT(mesh2.vertex_buffer == reinterpret_cast<SDL_GPUBuffer*>(0x11111111), "Moved vertex_buffer preserved");
    TEST_ASSERT(mesh2.has_gpu_resources, "Moved has_gpu_resources is true");

    // Source should be nulled
    TEST_ASSERT(mesh1.vertex_buffer == nullptr, "Source vertex_buffer is nullptr");
    TEST_ASSERT(mesh1.index_buffer == nullptr, "Source index_buffer is nullptr");
    TEST_ASSERT(!mesh1.has_gpu_resources, "Source has_gpu_resources is false");

    // Reset fake pointers
    mesh2.vertex_buffer = nullptr;
    mesh2.index_buffer = nullptr;
    mesh2.has_gpu_resources = false;
}

// ============================================================================
// Helper: Create a small test grid with water
// ============================================================================
void createTestGridWithWater(TerrainGrid& grid, WaterData& waterData) {
    // Create a 128x128 grid
    grid.initialize(MapSize::Small);
    waterData.initialize(MapSize::Small);

    // Set sea level
    grid.sea_level = 8;

    // Fill with Substrate (land)
    for (std::uint16_t y = 0; y < grid.height; ++y) {
        for (std::uint16_t x = 0; x < grid.width; ++x) {
            grid.at(x, y).setTerrainType(TerrainType::Substrate);
            grid.at(x, y).setElevation(10);  // Above sea level
        }
    }

    // Create a small lake (4x4 tiles) at position (10, 10)
    WaterBodyID lake_id = 1;
    for (std::uint16_t y = 10; y < 14; ++y) {
        for (std::uint16_t x = 10; x < 14; ++x) {
            grid.at(x, y).setTerrainType(TerrainType::StillBasin);
            grid.at(x, y).setElevation(grid.sea_level);
            waterData.set_water_body_id(x, y, lake_id);
        }
    }

    // Create a small river (8 tiles long) at y=20
    WaterBodyID river_id = 2;
    for (std::uint16_t x = 5; x < 13; ++x) {
        grid.at(x, 20).setTerrainType(TerrainType::FlowChannel);
        grid.at(x, 20).setElevation(grid.sea_level);
        waterData.set_water_body_id(x, 20, river_id);
        waterData.set_flow_direction(x, 20, FlowDirection::E);
    }

    // Create ocean at the edges (first 3 rows)
    WaterBodyID ocean_id = 3;
    for (std::uint16_t y = 0; y < 3; ++y) {
        for (std::uint16_t x = 0; x < grid.width; ++x) {
            grid.at(x, y).setTerrainType(TerrainType::DeepVoid);
            grid.at(x, y).setElevation(grid.sea_level);
            waterData.set_water_body_id(x, y, ocean_id);
        }
    }
}

// ============================================================================
// Test: Water mesh generation - basic functionality
// ============================================================================
void test_mesh_generation_basic() {
    printf("\n--- Test: Mesh Generation Basic ---\n");

    TerrainGrid grid;
    WaterData waterData;
    createTestGridWithWater(grid, waterData);

    WaterMeshGenerationResult result = WaterMeshGenerator::generate(grid, waterData);

    // Should have 3 meshes (lake, river, ocean)
    TEST_ASSERT_EQ(result.meshes.size(), 3, "Generated 3 water meshes");
    TEST_ASSERT_EQ(result.lake_mesh_count, 1, "1 lake mesh");
    TEST_ASSERT_EQ(result.river_mesh_count, 1, "1 river mesh");
    TEST_ASSERT_EQ(result.ocean_mesh_count, 1, "1 ocean mesh");

    // All meshes should have vertices and indices
    for (const auto& mesh : result.meshes) {
        TEST_ASSERT(mesh.vertex_count > 0, "Mesh has vertices");
        TEST_ASSERT(mesh.index_count > 0, "Mesh has indices");
        TEST_ASSERT(mesh.index_count % 3 == 0, "Index count is multiple of 3 (triangles)");
    }

    // Total counts should be non-zero
    TEST_ASSERT(result.total_vertex_count > 0, "Total vertices > 0");
    TEST_ASSERT(result.total_index_count > 0, "Total indices > 0");
}

// ============================================================================
// Test: Lake mesh generation
// ============================================================================
void test_lake_mesh_generation() {
    printf("\n--- Test: Lake Mesh Generation ---\n");

    TerrainGrid grid;
    WaterData waterData;
    createTestGridWithWater(grid, waterData);

    WaterMeshGenerationResult result = WaterMeshGenerator::generate(grid, waterData);

    // Find the lake mesh (body_id = 1)
    WaterMesh* lake_mesh = nullptr;
    for (auto& mesh : result.meshes) {
        if (mesh.body_id == 1) {
            lake_mesh = &mesh;
            break;
        }
    }

    TEST_ASSERT(lake_mesh != nullptr, "Found lake mesh");
    if (!lake_mesh) return;

    TEST_ASSERT(lake_mesh->body_type == WaterBodyType::Lake, "Lake mesh type is Lake");

    // 4x4 lake = 16 tiles = 16 * 6 = 96 indices
    TEST_ASSERT_EQ(lake_mesh->index_count, 96, "Lake has 96 indices (16 tiles * 6)");

    // Vertices: (4+1) * (4+1) = 25 corners, but we use shared vertices
    // Actual count may vary based on implementation
    TEST_ASSERT(lake_mesh->vertex_count >= 25, "Lake has at least 25 vertices");

    // Check water surface elevation
    float expected_y = static_cast<float>(grid.sea_level) * ELEVATION_HEIGHT;
    for (const auto& vert : lake_mesh->vertices) {
        TEST_ASSERT_FLOAT_EQ(vert.position_y, expected_y, "Vertex Y is at sea level");
    }
}

// ============================================================================
// Test: Shore factor calculation
// ============================================================================
void test_shore_factor() {
    printf("\n--- Test: Shore Factor Calculation ---\n");

    TerrainGrid grid;
    WaterData waterData;
    createTestGridWithWater(grid, waterData);

    WaterMeshGenerationResult result = WaterMeshGenerator::generate(grid, waterData);

    // Find the lake mesh (4x4 at position 10,10)
    WaterMesh* lake_mesh = nullptr;
    for (auto& mesh : result.meshes) {
        if (mesh.body_id == 1) {
            lake_mesh = &mesh;
            break;
        }
    }

    TEST_ASSERT(lake_mesh != nullptr, "Found lake mesh for shore factor test");
    if (!lake_mesh) return;

    // Count vertices with shore_factor = 1.0 (edges) and 0.0 (interior)
    int shore_count = 0;
    int interior_count = 0;

    for (const auto& vert : lake_mesh->vertices) {
        if (vert.shore_factor >= 0.99f) {
            shore_count++;
        } else if (vert.shore_factor <= 0.01f) {
            interior_count++;
        }
    }

    // 4x4 lake:
    // - Edge vertices: 12 (perimeter of 5x5 minus 4 corners shared)
    // - But corners touch land, so all perimeter vertices should be shore
    // - Interior vertices: (4-1) * (4-1) = 9 internal corners

    TEST_ASSERT(shore_count > 0, "Some vertices have shore_factor = 1.0");
    TEST_ASSERT(interior_count > 0, "Some vertices have shore_factor = 0.0");

    printf("  [INFO] Shore vertices: %d, Interior vertices: %d\n", shore_count, interior_count);
}

// ============================================================================
// Test: River mesh generation
// ============================================================================
void test_river_mesh_generation() {
    printf("\n--- Test: River Mesh Generation ---\n");

    TerrainGrid grid;
    WaterData waterData;
    createTestGridWithWater(grid, waterData);

    WaterMeshGenerationResult result = WaterMeshGenerator::generate(grid, waterData);

    // Find the river mesh (body_id = 2)
    WaterMesh* river_mesh = nullptr;
    for (auto& mesh : result.meshes) {
        if (mesh.body_id == 2) {
            river_mesh = &mesh;
            break;
        }
    }

    TEST_ASSERT(river_mesh != nullptr, "Found river mesh");
    if (!river_mesh) return;

    TEST_ASSERT(river_mesh->body_type == WaterBodyType::River, "River mesh type is River");

    // 8 tiles long river = 8 * 6 = 48 indices
    TEST_ASSERT_EQ(river_mesh->index_count, 48, "River has 48 indices (8 tiles * 6)");
}

// ============================================================================
// Test: Ocean mesh generation
// ============================================================================
void test_ocean_mesh_generation() {
    printf("\n--- Test: Ocean Mesh Generation ---\n");

    TerrainGrid grid;
    WaterData waterData;
    createTestGridWithWater(grid, waterData);

    WaterMeshGenerationResult result = WaterMeshGenerator::generate(grid, waterData);

    // Find the ocean mesh (body_id = 3)
    WaterMesh* ocean_mesh = nullptr;
    for (auto& mesh : result.meshes) {
        if (mesh.body_id == 3) {
            ocean_mesh = &mesh;
            break;
        }
    }

    TEST_ASSERT(ocean_mesh != nullptr, "Found ocean mesh");
    if (!ocean_mesh) return;

    TEST_ASSERT(ocean_mesh->body_type == WaterBodyType::Ocean, "Ocean mesh type is Ocean");

    // 3 rows * 128 tiles = 384 tiles = 384 * 6 = 2304 indices
    TEST_ASSERT_EQ(ocean_mesh->index_count, 2304, "Ocean has 2304 indices (384 tiles * 6)");
}

// ============================================================================
// Test: Draw call estimation
// ============================================================================
void test_draw_call_estimation() {
    printf("\n--- Test: Draw Call Estimation ---\n");

    // The ticket specifies: "Estimated draw calls: 5-15 for all water on map"
    // Each water body = 1 draw call, so we expect 5-15 water bodies typically

    TerrainGrid grid;
    WaterData waterData;
    createTestGridWithWater(grid, waterData);

    WaterMeshGenerationResult result = WaterMeshGenerator::generate(grid, waterData);

    std::size_t draw_calls = result.meshes.size();

    TEST_ASSERT(draw_calls >= 1, "At least 1 draw call");
    TEST_ASSERT(draw_calls <= 20, "At most 20 draw calls (reasonable for test)");

    printf("  [INFO] Test map draw calls: %zu\n", draw_calls);
}

// ============================================================================
// Test: AABB computation
// ============================================================================
void test_aabb_computation() {
    printf("\n--- Test: AABB Computation ---\n");

    TerrainGrid grid;
    WaterData waterData;
    createTestGridWithWater(grid, waterData);

    WaterMeshGenerationResult result = WaterMeshGenerator::generate(grid, waterData);

    // Find the lake mesh (4x4 at position 10,10)
    WaterMesh* lake_mesh = nullptr;
    for (auto& mesh : result.meshes) {
        if (mesh.body_id == 1) {
            lake_mesh = &mesh;
            break;
        }
    }

    TEST_ASSERT(lake_mesh != nullptr, "Found lake mesh for AABB test");
    if (!lake_mesh) return;

    // Lake is at tiles (10-13, 10-13), vertices at corners (10-14, 10-14)
    TEST_ASSERT(lake_mesh->aabb.isValid(), "Lake AABB is valid");

    // Check AABB bounds
    TEST_ASSERT_FLOAT_EQ(lake_mesh->aabb.min.x, 10.0f, "Lake AABB min.x is 10");
    TEST_ASSERT_FLOAT_EQ(lake_mesh->aabb.min.z, 10.0f, "Lake AABB min.z is 10");
    TEST_ASSERT_FLOAT_EQ(lake_mesh->aabb.max.x, 14.0f, "Lake AABB max.x is 14");
    TEST_ASSERT_FLOAT_EQ(lake_mesh->aabb.max.z, 14.0f, "Lake AABB max.z is 14");

    // Y should be at sea level
    float expected_y = static_cast<float>(grid.sea_level) * ELEVATION_HEIGHT;
    TEST_ASSERT_FLOAT_EQ(lake_mesh->aabb.min.y, expected_y, "Lake AABB min.y is at sea level");
    TEST_ASSERT_FLOAT_EQ(lake_mesh->aabb.max.y, expected_y, "Lake AABB max.y is at sea level");
}

// ============================================================================
// Test: Empty water data
// ============================================================================
void test_empty_water_data() {
    printf("\n--- Test: Empty Water Data ---\n");

    TerrainGrid grid(MapSize::Small);
    WaterData waterData(MapSize::Small);

    // No water bodies - all land
    for (std::uint16_t y = 0; y < grid.height; ++y) {
        for (std::uint16_t x = 0; x < grid.width; ++x) {
            grid.at(x, y).setTerrainType(TerrainType::Substrate);
            grid.at(x, y).setElevation(10);
        }
    }

    WaterMeshGenerationResult result = WaterMeshGenerator::generate(grid, waterData);

    TEST_ASSERT(result.meshes.empty(), "No meshes for land-only map");
    TEST_ASSERT_EQ(result.total_vertex_count, 0, "Zero total vertices");
    TEST_ASSERT_EQ(result.total_index_count, 0, "Zero total indices");
}

// ============================================================================
// Test: Regenerate single body
// ============================================================================
void test_regenerate_body() {
    printf("\n--- Test: Regenerate Single Body ---\n");

    TerrainGrid grid;
    WaterData waterData;
    createTestGridWithWater(grid, waterData);

    // Regenerate just the lake (body_id = 1)
    WaterMesh mesh;
    bool success = WaterMeshGenerator::regenerateBody(grid, waterData, 1, mesh);

    TEST_ASSERT(success, "Regenerate body succeeded");
    TEST_ASSERT_EQ(mesh.body_id, 1, "Regenerated body_id is 1");
    TEST_ASSERT(mesh.body_type == WaterBodyType::Lake, "Regenerated body_type is Lake");
    TEST_ASSERT(!mesh.isEmpty(), "Regenerated mesh is not empty");
    TEST_ASSERT_EQ(mesh.index_count, 96, "Regenerated lake has 96 indices");

    // Try to regenerate non-existent body
    WaterMesh empty_mesh;
    bool fail = WaterMeshGenerator::regenerateBody(grid, waterData, 999, empty_mesh);

    TEST_ASSERT(!fail, "Regenerate non-existent body returns false");
}

// ============================================================================
// Test: Water body ID in vertices
// ============================================================================
void test_vertex_body_id() {
    printf("\n--- Test: Vertex Body ID ---\n");

    TerrainGrid grid;
    WaterData waterData;
    createTestGridWithWater(grid, waterData);

    WaterMeshGenerationResult result = WaterMeshGenerator::generate(grid, waterData);

    // Each mesh's vertices should have the correct body ID
    for (const auto& mesh : result.meshes) {
        for (const auto& vert : mesh.vertices) {
            TEST_ASSERT_EQ(vert.water_body_id, mesh.body_id, "Vertex body_id matches mesh body_id");
        }
    }
}

// ============================================================================
// Test: UV coordinates
// ============================================================================
void test_uv_coordinates() {
    printf("\n--- Test: UV Coordinates ---\n");

    TerrainGrid grid;
    WaterData waterData;
    createTestGridWithWater(grid, waterData);

    WaterMeshGenerationResult result = WaterMeshGenerator::generate(grid, waterData);

    // Find the lake mesh
    WaterMesh* lake_mesh = nullptr;
    for (auto& mesh : result.meshes) {
        if (mesh.body_id == 1) {
            lake_mesh = &mesh;
            break;
        }
    }

    TEST_ASSERT(lake_mesh != nullptr, "Found lake mesh for UV test");
    if (!lake_mesh) return;

    // UV coordinates should be based on world position
    for (const auto& vert : lake_mesh->vertices) {
        TEST_ASSERT_FLOAT_EQ(vert.uv_u, vert.position_x, "UV U matches position X");
        TEST_ASSERT_FLOAT_EQ(vert.uv_v, vert.position_z, "UV V matches position Z");
    }
}

// ============================================================================
// Test: Vertex buffer description
// ============================================================================
void test_vertex_buffer_description() {
    printf("\n--- Test: Vertex Buffer Description ---\n");

    SDL_GPUVertexBufferDescription desc = getWaterVertexBufferDescription(0);

    TEST_ASSERT_EQ(desc.slot, 0, "Buffer slot is 0");
    TEST_ASSERT_EQ(desc.pitch, sizeof(WaterVertex), "Pitch is sizeof(WaterVertex)");
    TEST_ASSERT(desc.input_rate == SDL_GPU_VERTEXINPUTRATE_VERTEX, "Input rate is per-vertex");
}

// ============================================================================
// Test: Vertex attributes
// ============================================================================
void test_vertex_attributes() {
    printf("\n--- Test: Vertex Attributes ---\n");

    SDL_GPUVertexAttribute attrs[4];
    std::uint32_t count;
    getWaterVertexAttributes(0, attrs, &count);

    TEST_ASSERT_EQ(count, 4, "4 vertex attributes");
    TEST_ASSERT_EQ(WATER_VERTEX_ATTRIBUTE_COUNT, 4, "WATER_VERTEX_ATTRIBUTE_COUNT is 4");

    // Position attribute
    TEST_ASSERT_EQ(attrs[0].location, 0, "Position at location 0");
    TEST_ASSERT(attrs[0].format == SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, "Position is FLOAT3");

    // Shore factor attribute
    TEST_ASSERT_EQ(attrs[1].location, 1, "Shore factor at location 1");
    TEST_ASSERT(attrs[1].format == SDL_GPU_VERTEXELEMENTFORMAT_FLOAT, "Shore factor is FLOAT");

    // Body ID attribute
    TEST_ASSERT_EQ(attrs[2].location, 2, "Body ID at location 2");

    // UV attribute
    TEST_ASSERT_EQ(attrs[3].location, 3, "UV at location 3");
    TEST_ASSERT(attrs[3].format == SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, "UV is FLOAT2");
}

// ============================================================================
// Main
// ============================================================================
int main() {
    printf("===========================================\n");
    printf("WaterMesh Unit Tests (Ticket 3-027)\n");
    printf("===========================================\n");

    test_water_vertex_layout();
    test_water_vertex_default_construction();
    test_water_vertex_param_construction();
    test_water_mesh_default_construction();
    test_water_mesh_state_methods();
    test_water_mesh_move_semantics();
    test_mesh_generation_basic();
    test_lake_mesh_generation();
    test_shore_factor();
    test_river_mesh_generation();
    test_ocean_mesh_generation();
    test_draw_call_estimation();
    test_aabb_computation();
    test_empty_water_data();
    test_regenerate_body();
    test_vertex_body_id();
    test_uv_coordinates();
    test_vertex_buffer_description();
    test_vertex_attributes();

    printf("\n===========================================\n");
    printf("Results: %d passed, %d failed\n", tests_passed, tests_failed);
    printf("===========================================\n");

    return tests_failed > 0 ? 1 : 0;
}
