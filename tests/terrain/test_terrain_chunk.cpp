/**
 * @file test_terrain_chunk.cpp
 * @brief Unit tests for TerrainChunk struct (Ticket 3-023)
 *
 * Tests:
 * - TerrainChunk constants (tile counts, vertex counts, index counts)
 * - Default construction
 * - Coordinate constructor
 * - Tile coordinate methods
 * - State flag methods
 * - Move semantics
 */

#include <sims3000/terrain/TerrainChunk.h>
#include <glm/glm.hpp>
#include <cstdio>
#include <utility>

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

// ============================================================================
// Test: TerrainChunk constants
// ============================================================================
void test_chunk_constants() {
    printf("\n--- Test: Chunk Constants ---\n");

    // TILES_PER_CHUNK should be 32 (matching CHUNK_SIZE from ChunkDirtyTracker)
    TEST_ASSERT_EQ(TILES_PER_CHUNK, 32, "TILES_PER_CHUNK is 32");
    TEST_ASSERT_EQ(TILES_PER_CHUNK, CHUNK_SIZE, "TILES_PER_CHUNK matches CHUNK_SIZE");

    // Total tiles per chunk: 32 * 32 = 1024
    TEST_ASSERT_EQ(TILES_PER_CHUNK_TOTAL, 1024, "TILES_PER_CHUNK_TOTAL is 1024");

    // Vertices per chunk: (32+1) * (32+1) = 1089 (shared vertices at corners)
    TEST_ASSERT_EQ(VERTICES_PER_CHUNK, 1089, "VERTICES_PER_CHUNK is 1089");

    // Indices per chunk: 1024 tiles * 6 indices per tile = 6144
    TEST_ASSERT_EQ(INDICES_PER_CHUNK, 6144, "INDICES_PER_CHUNK is 6144");
}

// ============================================================================
// Test: Default construction
// ============================================================================
void test_default_construction() {
    printf("\n--- Test: Default Construction ---\n");

    TerrainChunk chunk;

    // Coordinates should be 0
    TEST_ASSERT_EQ(chunk.chunk_x, 0, "Default chunk_x is 0");
    TEST_ASSERT_EQ(chunk.chunk_y, 0, "Default chunk_y is 0");

    // GPU resources should be null
    TEST_ASSERT(chunk.vertex_buffer == nullptr, "Default vertex_buffer is nullptr");
    TEST_ASSERT(chunk.index_buffer == nullptr, "Default index_buffer is nullptr");

    // Counts should be 0
    TEST_ASSERT_EQ(chunk.vertex_count, 0, "Default vertex_count is 0");
    TEST_ASSERT_EQ(chunk.index_count, 0, "Default index_count is 0");

    // Chunk should start dirty
    TEST_ASSERT(chunk.dirty == true, "Default chunk is dirty");
    TEST_ASSERT(chunk.has_gpu_resources == false, "Default has_gpu_resources is false");
}

// ============================================================================
// Test: Coordinate constructor
// ============================================================================
void test_coordinate_construction() {
    printf("\n--- Test: Coordinate Construction ---\n");

    TerrainChunk chunk(5, 7);

    TEST_ASSERT_EQ(chunk.chunk_x, 5, "chunk_x set to 5");
    TEST_ASSERT_EQ(chunk.chunk_y, 7, "chunk_y set to 7");

    // Other fields should still be default
    TEST_ASSERT(chunk.vertex_buffer == nullptr, "vertex_buffer is nullptr");
    TEST_ASSERT(chunk.dirty == true, "Constructed chunk is dirty");
}

// ============================================================================
// Test: Tile coordinate methods
// ============================================================================
void test_tile_coordinates() {
    printf("\n--- Test: Tile Coordinate Methods ---\n");

    // Chunk at (0, 0) covers tiles (0,0) to (31,31)
    TerrainChunk chunk0(0, 0);
    TEST_ASSERT_EQ(chunk0.getTileMinX(), 0, "Chunk 0,0 min X is 0");
    TEST_ASSERT_EQ(chunk0.getTileMinY(), 0, "Chunk 0,0 min Y is 0");
    TEST_ASSERT_EQ(chunk0.getTileMaxX(), 32, "Chunk 0,0 max X is 32");
    TEST_ASSERT_EQ(chunk0.getTileMaxY(), 32, "Chunk 0,0 max Y is 32");

    // Chunk at (1, 0) covers tiles (32,0) to (63,31)
    TerrainChunk chunk1(1, 0);
    TEST_ASSERT_EQ(chunk1.getTileMinX(), 32, "Chunk 1,0 min X is 32");
    TEST_ASSERT_EQ(chunk1.getTileMinY(), 0, "Chunk 1,0 min Y is 0");
    TEST_ASSERT_EQ(chunk1.getTileMaxX(), 64, "Chunk 1,0 max X is 64");
    TEST_ASSERT_EQ(chunk1.getTileMaxY(), 32, "Chunk 1,0 max Y is 32");

    // Chunk at (3, 2) covers tiles (96,64) to (127,95)
    TerrainChunk chunk2(3, 2);
    TEST_ASSERT_EQ(chunk2.getTileMinX(), 96, "Chunk 3,2 min X is 96");
    TEST_ASSERT_EQ(chunk2.getTileMinY(), 64, "Chunk 3,2 min Y is 64");
    TEST_ASSERT_EQ(chunk2.getTileMaxX(), 128, "Chunk 3,2 max X is 128");
    TEST_ASSERT_EQ(chunk2.getTileMaxY(), 96, "Chunk 3,2 max Y is 96");
}

// ============================================================================
// Test: containsTile method
// ============================================================================
void test_contains_tile() {
    printf("\n--- Test: containsTile Method ---\n");

    TerrainChunk chunk(1, 1);  // Covers tiles (32,32) to (63,63)

    // Corners of the chunk
    TEST_ASSERT(chunk.containsTile(32, 32), "Contains (32,32) - min corner");
    TEST_ASSERT(chunk.containsTile(63, 63), "Contains (63,63) - max corner");
    TEST_ASSERT(chunk.containsTile(32, 63), "Contains (32,63)");
    TEST_ASSERT(chunk.containsTile(63, 32), "Contains (63,32)");

    // Center of the chunk
    TEST_ASSERT(chunk.containsTile(47, 47), "Contains (47,47) - center");

    // Just outside the chunk
    TEST_ASSERT(!chunk.containsTile(31, 32), "Does not contain (31,32)");
    TEST_ASSERT(!chunk.containsTile(32, 31), "Does not contain (32,31)");
    TEST_ASSERT(!chunk.containsTile(64, 32), "Does not contain (64,32)");
    TEST_ASSERT(!chunk.containsTile(32, 64), "Does not contain (32,64)");

    // Way outside
    TEST_ASSERT(!chunk.containsTile(0, 0), "Does not contain (0,0)");
    TEST_ASSERT(!chunk.containsTile(100, 100), "Does not contain (100,100)");

    // Negative coordinates
    TEST_ASSERT(!chunk.containsTile(-1, 32), "Does not contain negative x");
    TEST_ASSERT(!chunk.containsTile(32, -1), "Does not contain negative y");
}

// ============================================================================
// Test: Dirty flag methods
// ============================================================================
void test_dirty_flag() {
    printf("\n--- Test: Dirty Flag Methods ---\n");

    TerrainChunk chunk(0, 0);

    // Starts dirty
    TEST_ASSERT(chunk.isDirty(), "New chunk is dirty");
    TEST_ASSERT(chunk.dirty, "dirty field is true");

    // Clear dirty
    chunk.clearDirty();
    TEST_ASSERT(!chunk.isDirty(), "After clearDirty, isDirty is false");
    TEST_ASSERT(!chunk.dirty, "dirty field is false");

    // Mark dirty again
    chunk.markDirty();
    TEST_ASSERT(chunk.isDirty(), "After markDirty, isDirty is true");
    TEST_ASSERT(chunk.dirty, "dirty field is true");
}

// ============================================================================
// Test: GPU resource state methods
// ============================================================================
void test_gpu_resource_state() {
    printf("\n--- Test: GPU Resource State Methods ---\n");

    TerrainChunk chunk(0, 0);

    // Initial state
    TEST_ASSERT(!chunk.hasGPUResources(), "New chunk has no GPU resources");
    TEST_ASSERT(!chunk.isRenderable(), "New chunk is not renderable");

    // Simulate GPU resource creation (without actual GPU)
    // Note: We can't actually create GPU resources in this test
    // Just verify the logic works
    chunk.vertex_buffer = reinterpret_cast<SDL_GPUBuffer*>(0x12345678);
    chunk.index_buffer = reinterpret_cast<SDL_GPUBuffer*>(0x87654321);
    chunk.has_gpu_resources = true;

    TEST_ASSERT(chunk.hasGPUResources(), "After setting buffers, hasGPUResources is true");
    TEST_ASSERT(!chunk.isRenderable(), "Still dirty, so not renderable");

    // Clear dirty
    chunk.clearDirty();
    TEST_ASSERT(chunk.isRenderable(), "After clearing dirty, isRenderable is true");

    // Mark dirty again
    chunk.markDirty();
    TEST_ASSERT(!chunk.isRenderable(), "After marking dirty, isRenderable is false");

    // Reset for safety (don't leave fake pointers)
    chunk.vertex_buffer = nullptr;
    chunk.index_buffer = nullptr;
    chunk.has_gpu_resources = false;
}

// ============================================================================
// Test: Move construction
// ============================================================================
void test_move_construction() {
    printf("\n--- Test: Move Construction ---\n");

    TerrainChunk chunk1(3, 4);
    chunk1.vertex_buffer = reinterpret_cast<SDL_GPUBuffer*>(0x11111111);
    chunk1.index_buffer = reinterpret_cast<SDL_GPUBuffer*>(0x22222222);
    chunk1.vertex_count = 1089;
    chunk1.index_count = 6144;
    chunk1.has_gpu_resources = true;
    chunk1.dirty = false;

    // Move construct
    TerrainChunk chunk2(std::move(chunk1));

    // chunk2 should have chunk1's values
    TEST_ASSERT_EQ(chunk2.chunk_x, 3, "Moved chunk_x is 3");
    TEST_ASSERT_EQ(chunk2.chunk_y, 4, "Moved chunk_y is 4");
    TEST_ASSERT(chunk2.vertex_buffer == reinterpret_cast<SDL_GPUBuffer*>(0x11111111),
                "Moved vertex_buffer preserved");
    TEST_ASSERT(chunk2.index_buffer == reinterpret_cast<SDL_GPUBuffer*>(0x22222222),
                "Moved index_buffer preserved");
    TEST_ASSERT_EQ(chunk2.vertex_count, 1089, "Moved vertex_count is 1089");
    TEST_ASSERT_EQ(chunk2.index_count, 6144, "Moved index_count is 6144");
    TEST_ASSERT(chunk2.has_gpu_resources, "Moved has_gpu_resources is true");
    TEST_ASSERT(!chunk2.dirty, "Moved dirty is false");

    // chunk1 should be nulled out
    TEST_ASSERT(chunk1.vertex_buffer == nullptr, "Source vertex_buffer is nullptr");
    TEST_ASSERT(chunk1.index_buffer == nullptr, "Source index_buffer is nullptr");
    TEST_ASSERT(!chunk1.has_gpu_resources, "Source has_gpu_resources is false");
}

// ============================================================================
// Test: Move assignment
// ============================================================================
void test_move_assignment() {
    printf("\n--- Test: Move Assignment ---\n");

    TerrainChunk chunk1(5, 6);
    chunk1.vertex_buffer = reinterpret_cast<SDL_GPUBuffer*>(0x33333333);
    chunk1.index_buffer = reinterpret_cast<SDL_GPUBuffer*>(0x44444444);
    chunk1.has_gpu_resources = true;

    TerrainChunk chunk2(0, 0);

    // Move assign
    chunk2 = std::move(chunk1);

    // chunk2 should have chunk1's values
    TEST_ASSERT_EQ(chunk2.chunk_x, 5, "Assigned chunk_x is 5");
    TEST_ASSERT_EQ(chunk2.chunk_y, 6, "Assigned chunk_y is 6");
    TEST_ASSERT(chunk2.vertex_buffer == reinterpret_cast<SDL_GPUBuffer*>(0x33333333),
                "Assigned vertex_buffer preserved");
    TEST_ASSERT(chunk2.has_gpu_resources, "Assigned has_gpu_resources is true");

    // chunk1 should be nulled out
    TEST_ASSERT(chunk1.vertex_buffer == nullptr, "Source vertex_buffer is nullptr after assignment");
    TEST_ASSERT(!chunk1.has_gpu_resources, "Source has_gpu_resources is false after assignment");
}

// ============================================================================
// Test: Chunk count calculations for map sizes
// ============================================================================
void test_chunk_count_for_maps() {
    printf("\n--- Test: Chunk Counts for Map Sizes ---\n");

    // 128x128 map = 4x4 chunks
    std::uint16_t chunks_128 = 128 / TILES_PER_CHUNK;
    TEST_ASSERT_EQ(chunks_128, 4, "128x128 map has 4 chunks per dimension");

    // 256x256 map = 8x8 chunks
    std::uint16_t chunks_256 = 256 / TILES_PER_CHUNK;
    TEST_ASSERT_EQ(chunks_256, 8, "256x256 map has 8 chunks per dimension");

    // 512x512 map = 16x16 chunks
    std::uint16_t chunks_512 = 512 / TILES_PER_CHUNK;
    TEST_ASSERT_EQ(chunks_512, 16, "512x512 map has 16 chunks per dimension");
}

// ============================================================================
// Test: Memory estimates
// ============================================================================
void test_memory_estimates() {
    printf("\n--- Test: Memory Estimates ---\n");

    // Per chunk vertex buffer size (in bytes)
    // 1089 vertices * 44 bytes per vertex = 47916 bytes (~47KB)
    std::size_t vertex_buffer_size = VERTICES_PER_CHUNK * 44;
    TEST_ASSERT_EQ(vertex_buffer_size, 47916, "Vertex buffer is ~47KB per chunk");

    // Per chunk index buffer size (in bytes)
    // 6144 indices * 2 bytes (uint16) = 12288 bytes (~12KB)
    // Or 6144 * 4 bytes (uint32) = 24576 bytes (~24KB)
    std::size_t index_buffer_size_16 = INDICES_PER_CHUNK * sizeof(std::uint16_t);
    std::size_t index_buffer_size_32 = INDICES_PER_CHUNK * sizeof(std::uint32_t);
    TEST_ASSERT_EQ(index_buffer_size_16, 12288, "Index buffer (uint16) is ~12KB per chunk");
    TEST_ASSERT_EQ(index_buffer_size_32, 24576, "Index buffer (uint32) is ~24KB per chunk");

    // Total per chunk (using uint16 indices): ~47KB + ~12KB = ~60KB
    printf("  [INFO] Per-chunk GPU memory: %zu bytes vertex + %zu bytes index (uint16) = %zu bytes\n",
           vertex_buffer_size, index_buffer_size_16, vertex_buffer_size + index_buffer_size_16);
}

// ============================================================================
// Test: AABB computation with explicit max elevation
// ============================================================================
void test_aabb_explicit() {
    printf("\n--- Test: AABB Explicit Max Elevation ---\n");

    // Chunk at (0, 0) with max elevation 0
    TerrainChunk chunk0(0, 0);
    chunk0.computeAABB(static_cast<std::uint8_t>(0));

    TEST_ASSERT(chunk0.aabb.min.x == 0.0f, "Chunk 0,0 AABB min.x is 0");
    TEST_ASSERT(chunk0.aabb.min.y == 0.0f, "Chunk 0,0 AABB min.y is 0");
    TEST_ASSERT(chunk0.aabb.min.z == 0.0f, "Chunk 0,0 AABB min.z is 0");
    TEST_ASSERT(chunk0.aabb.max.x == 32.0f, "Chunk 0,0 AABB max.x is 32");
    TEST_ASSERT(chunk0.aabb.max.y == 0.0f, "Chunk 0,0 AABB max.y is 0 (elevation 0)");
    TEST_ASSERT(chunk0.aabb.max.z == 32.0f, "Chunk 0,0 AABB max.z is 32");

    // Chunk at (1, 2) with max elevation 31
    TerrainChunk chunk1(1, 2);
    chunk1.computeAABB(static_cast<std::uint8_t>(31));

    TEST_ASSERT(chunk1.aabb.min.x == 32.0f, "Chunk 1,2 AABB min.x is 32");
    TEST_ASSERT(chunk1.aabb.min.y == 0.0f, "Chunk 1,2 AABB min.y is 0");
    TEST_ASSERT(chunk1.aabb.min.z == 64.0f, "Chunk 1,2 AABB min.z is 64");
    TEST_ASSERT(chunk1.aabb.max.x == 64.0f, "Chunk 1,2 AABB max.x is 64");
    // max.y = 31 * 0.25 = 7.75
    float expected_max_y = 31.0f * ELEVATION_HEIGHT;
    TEST_ASSERT(chunk1.aabb.max.y == expected_max_y, "Chunk 1,2 AABB max.y is 7.75 (elevation 31)");
    TEST_ASSERT(chunk1.aabb.max.z == 96.0f, "Chunk 1,2 AABB max.z is 96");

    // Test mid-range elevation
    TerrainChunk chunk2(3, 3);
    chunk2.computeAABB(static_cast<std::uint8_t>(16));
    float expected_y = 16.0f * ELEVATION_HEIGHT;  // 4.0
    TEST_ASSERT(chunk2.aabb.max.y == expected_y, "Chunk 3,3 AABB max.y is 4.0 (elevation 16)");
}

// ============================================================================
// Test: AABB validity
// ============================================================================
void test_aabb_validity() {
    printf("\n--- Test: AABB Validity ---\n");

    TerrainChunk chunk(0, 0);
    chunk.computeAABB(static_cast<std::uint8_t>(10));

    TEST_ASSERT(chunk.aabb.isValid(), "AABB is valid after computation");

    // Check center calculation
    glm::vec3 center = chunk.aabb.center();
    TEST_ASSERT(center.x == 16.0f, "AABB center.x is 16");
    TEST_ASSERT(center.z == 16.0f, "AABB center.z is 16");

    // Check size calculation
    glm::vec3 size = chunk.aabb.size();
    TEST_ASSERT(size.x == 32.0f, "AABB size.x is 32");
    TEST_ASSERT(size.z == 32.0f, "AABB size.z is 32");
}

// ============================================================================
// Test: ELEVATION_HEIGHT constant
// ============================================================================
void test_elevation_height_constant() {
    printf("\n--- Test: ELEVATION_HEIGHT Constant ---\n");

    // ELEVATION_HEIGHT should be 0.25 (from Epic 2 ticket 2-033)
    TEST_ASSERT(ELEVATION_HEIGHT == 0.25f, "ELEVATION_HEIGHT is 0.25");

    // Max elevation (31) should yield max height of 7.75
    float max_height = 31.0f * ELEVATION_HEIGHT;
    TEST_ASSERT(max_height == 7.75f, "Max elevation (31) yields height 7.75");
}

// ============================================================================
// Main
// ============================================================================
int main() {
    printf("===========================================\n");
    printf("TerrainChunk Unit Tests (Ticket 3-023)\n");
    printf("===========================================\n");

    test_chunk_constants();
    test_default_construction();
    test_coordinate_construction();
    test_tile_coordinates();
    test_contains_tile();
    test_dirty_flag();
    test_gpu_resource_state();
    test_move_construction();
    test_move_assignment();
    test_chunk_count_for_maps();
    test_memory_estimates();
    test_aabb_explicit();
    test_aabb_validity();
    test_elevation_height_constant();

    printf("\n===========================================\n");
    printf("Results: %d passed, %d failed\n", tests_passed, tests_failed);
    printf("===========================================\n");

    return tests_failed > 0 ? 1 : 0;
}
