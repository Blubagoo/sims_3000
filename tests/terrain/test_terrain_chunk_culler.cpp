/**
 * @file test_terrain_chunk_culler.cpp
 * @brief Unit tests for TerrainChunkCuller (Ticket 3-034).
 *
 * Tests:
 * - Chunk registration with FrustumCuller
 * - Chunk AABB computation (includes max elevation)
 * - Frustum culling of chunks at various camera angles
 * - Visibility statistics (visible vs total)
 * - Conservative culling (no visible popping)
 * - Performance on 512x512 maps
 */

#include <sims3000/terrain/TerrainChunkCuller.h>
#include <sims3000/terrain/TerrainChunk.h>
#include <sims3000/render/FrustumCuller.h>
#include <sims3000/render/ViewMatrix.h>
#include <sims3000/render/ProjectionMatrix.h>
#include <sims3000/render/CameraState.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <cmath>
#include <chrono>

using namespace sims3000;
using namespace sims3000::terrain;

// Test counter
static int g_testsPassed = 0;
static int g_testsFailed = 0;

#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            std::cerr << "FAIL: " << message << " (line " << __LINE__ << ")" << std::endl; \
            g_testsFailed++; \
            return false; \
        } \
    } while (0)

#define TEST_ASSERT_EQ(actual, expected, message) \
    do { \
        if ((actual) != (expected)) { \
            std::cerr << "FAIL: " << message << " (expected " << (expected) << ", got " << (actual) << ")" << std::endl; \
            g_testsFailed++; \
            return false; \
        } \
    } while (0)

#define TEST_ASSERT_FLOAT_EQ(a, b, epsilon, message) \
    TEST_ASSERT(std::abs((a) - (b)) < (epsilon), message)

// ============================================================================
// Helper Functions
// ============================================================================

glm::mat4 createViewProjection(const CameraState& camera,
                               int windowWidth = 1920,
                               int windowHeight = 1080) {
    glm::mat4 view = calculateViewMatrix(camera);
    glm::mat4 proj = calculateProjectionMatrixFromDimensions(
        windowWidth, windowHeight, CameraConfig::FOV_DEFAULT);
    return proj * view;
}

// Create a simple chunk with computed AABB
TerrainChunk createChunk(std::uint16_t cx, std::uint16_t cy, std::uint8_t maxElevation) {
    TerrainChunk chunk(cx, cy);
    chunk.computeAABB(maxElevation);
    // Simulate having GPU resources
    chunk.vertex_buffer = reinterpret_cast<SDL_GPUBuffer*>(0x12345678);
    chunk.index_buffer = reinterpret_cast<SDL_GPUBuffer*>(0x87654321);
    chunk.has_gpu_resources = true;
    chunk.clearDirty();
    return chunk;
}

// ============================================================================
// Test Cases
// ============================================================================

bool test_ChunkAABB_BasicComputation() {
    std::cout << "test_ChunkAABB_BasicComputation... ";

    // Chunk at (0, 0) with max elevation 0
    TerrainChunk chunk0(0, 0);
    chunk0.computeAABB(static_cast<std::uint8_t>(0));

    TEST_ASSERT_FLOAT_EQ(chunk0.aabb.min.x, 0.0f, 0.001f, "Chunk 0,0 min.x should be 0");
    TEST_ASSERT_FLOAT_EQ(chunk0.aabb.min.y, 0.0f, 0.001f, "Chunk 0,0 min.y should be 0");
    TEST_ASSERT_FLOAT_EQ(chunk0.aabb.min.z, 0.0f, 0.001f, "Chunk 0,0 min.z should be 0");
    TEST_ASSERT_FLOAT_EQ(chunk0.aabb.max.x, 32.0f, 0.001f, "Chunk 0,0 max.x should be 32");
    TEST_ASSERT_FLOAT_EQ(chunk0.aabb.max.y, 0.0f, 0.001f, "Chunk 0,0 max.y should be 0");
    TEST_ASSERT_FLOAT_EQ(chunk0.aabb.max.z, 32.0f, 0.001f, "Chunk 0,0 max.z should be 32");

    // Chunk at (1, 2) with max elevation 20
    TerrainChunk chunk1(1, 2);
    chunk1.computeAABB(static_cast<std::uint8_t>(20));

    TEST_ASSERT_FLOAT_EQ(chunk1.aabb.min.x, 32.0f, 0.001f, "Chunk 1,2 min.x should be 32");
    TEST_ASSERT_FLOAT_EQ(chunk1.aabb.min.z, 64.0f, 0.001f, "Chunk 1,2 min.z should be 64");
    TEST_ASSERT_FLOAT_EQ(chunk1.aabb.max.x, 64.0f, 0.001f, "Chunk 1,2 max.x should be 64");
    TEST_ASSERT_FLOAT_EQ(chunk1.aabb.max.z, 96.0f, 0.001f, "Chunk 1,2 max.z should be 96");
    // max.y = 20 * 0.25 = 5.0
    TEST_ASSERT_FLOAT_EQ(chunk1.aabb.max.y, 5.0f, 0.001f, "Chunk 1,2 max.y should be 5.0");

    std::cout << "PASS" << std::endl;
    g_testsPassed++;
    return true;
}

bool test_ChunkAABB_MaxElevation() {
    std::cout << "test_ChunkAABB_MaxElevation... ";

    // Test that max elevation is included in AABB
    TerrainChunk chunk(3, 3);
    chunk.computeAABB(static_cast<std::uint8_t>(31));  // Max elevation

    // max.y = 31 * 0.25 = 7.75
    float expectedMaxY = 31.0f * ELEVATION_HEIGHT;
    TEST_ASSERT_FLOAT_EQ(chunk.aabb.max.y, expectedMaxY, 0.001f,
        "AABB max.y should include max elevation * 0.25");

    std::cout << "PASS" << std::endl;
    g_testsPassed++;
    return true;
}

bool test_ChunkRegistration() {
    std::cout << "test_ChunkRegistration... ";

    // Create culler for 128x128 map (4x4 chunks)
    FrustumCuller culler(128, 128);
    TerrainChunkCuller chunkCuller;

    // Create 16 chunks
    std::vector<TerrainChunk> chunks;
    chunks.reserve(16);
    for (std::uint16_t cy = 0; cy < 4; ++cy) {
        for (std::uint16_t cx = 0; cx < 4; ++cx) {
            chunks.push_back(createChunk(cx, cy, 10));
        }
    }

    // Register chunks
    chunkCuller.registerChunks(culler, chunks);

    // Check that entities are registered
    TEST_ASSERT_EQ(culler.getEntityCount(), 16u, "Should have 16 entities registered");

    std::cout << "PASS" << std::endl;
    g_testsPassed++;
    return true;
}

bool test_ChunkUnregistration() {
    std::cout << "test_ChunkUnregistration... ";

    FrustumCuller culler(128, 128);
    TerrainChunkCuller chunkCuller;

    // Create and register 16 chunks
    std::vector<TerrainChunk> chunks;
    for (std::uint16_t cy = 0; cy < 4; ++cy) {
        for (std::uint16_t cx = 0; cx < 4; ++cx) {
            chunks.push_back(createChunk(cx, cy, 10));
        }
    }
    chunkCuller.registerChunks(culler, chunks);
    TEST_ASSERT_EQ(culler.getEntityCount(), 16u, "Should have 16 entities before unregister");

    // Unregister all chunks
    chunkCuller.unregisterChunks(culler, 16);
    TEST_ASSERT_EQ(culler.getEntityCount(), 0u, "Should have 0 entities after unregister");

    std::cout << "PASS" << std::endl;
    g_testsPassed++;
    return true;
}

bool test_VisibleChunks_CenterFocus() {
    std::cout << "test_VisibleChunks_CenterFocus... ";

    // Create culler for 128x128 map
    FrustumCuller culler(128, 128);
    TerrainChunkCuller chunkCuller;

    // Create 16 chunks
    std::vector<TerrainChunk> chunks;
    for (std::uint16_t cy = 0; cy < 4; ++cy) {
        for (std::uint16_t cx = 0; cx < 4; ++cx) {
            chunks.push_back(createChunk(cx, cy, 10));
        }
    }
    chunkCuller.registerChunks(culler, chunks);

    // Camera looking at center of map
    CameraState camera;
    camera.focus_point = glm::vec3(64.0f, 0.0f, 64.0f);
    camera.distance = 50.0f;
    camera.pitch = CameraConfig::ISOMETRIC_PITCH;
    camera.yaw = CameraConfig::PRESET_N_YAW;

    glm::mat4 vp = createViewProjection(camera);
    culler.updateFrustum(vp);

    // Update visible chunks
    chunkCuller.updateVisibleChunks(culler, chunks);

    // At center focus, center chunks should be visible
    TEST_ASSERT(chunkCuller.getVisibleChunkCount() > 0,
        "At least some chunks should be visible at center focus");

    // Get statistics
    const auto& stats = chunkCuller.getStats();
    TEST_ASSERT_EQ(stats.totalChunks, 16u, "Should have 16 total chunks");
    TEST_ASSERT(stats.visibleChunks > 0, "Should have some visible chunks");
    TEST_ASSERT(stats.visibleChunks + stats.culledChunks == stats.totalChunks,
        "Visible + culled should equal total");

    std::cout << "PASS" << std::endl;
    g_testsPassed++;
    return true;
}

bool test_FrustumCulling_CornerChunks() {
    std::cout << "test_FrustumCulling_CornerChunks... ";

    // Create culler for 256x256 map (8x8 chunks)
    FrustumCuller culler(256, 256);
    TerrainChunkCuller chunkCuller;

    // Create 64 chunks
    std::vector<TerrainChunk> chunks;
    for (std::uint16_t cy = 0; cy < 8; ++cy) {
        for (std::uint16_t cx = 0; cx < 8; ++cx) {
            chunks.push_back(createChunk(cx, cy, 10));
        }
    }
    chunkCuller.registerChunks(culler, chunks);

    // Camera at center with close zoom
    CameraState camera;
    camera.focus_point = glm::vec3(128.0f, 0.0f, 128.0f);
    camera.distance = 30.0f;  // Close zoom
    camera.pitch = CameraConfig::ISOMETRIC_PITCH;
    camera.yaw = CameraConfig::PRESET_N_YAW;

    glm::mat4 vp = createViewProjection(camera);
    culler.updateFrustum(vp);

    // Update visible chunks
    chunkCuller.updateVisibleChunks(culler, chunks);

    // At close zoom, corner chunks should be culled
    const auto& stats = chunkCuller.getStats();
    TEST_ASSERT(stats.culledChunks > 0, "At close zoom, some chunks should be culled");
    TEST_ASSERT(stats.visibleChunks < stats.totalChunks,
        "Not all chunks should be visible at close zoom");

    std::cout << "PASS (visible: " << stats.visibleChunks
              << "/" << stats.totalChunks << ", cull ratio: "
              << (stats.cullRatio * 100.0f) << "%)" << std::endl;
    g_testsPassed++;
    return true;
}

bool test_ConservativeCulling_NoPopping() {
    std::cout << "test_ConservativeCulling_NoPopping... ";

    FrustumCuller culler(256, 256);
    TerrainChunkCuller chunkCuller;

    // Create chunks
    std::vector<TerrainChunk> chunks;
    for (std::uint16_t cy = 0; cy < 8; ++cy) {
        for (std::uint16_t cx = 0; cx < 8; ++cx) {
            chunks.push_back(createChunk(cx, cy, 10));
        }
    }
    chunkCuller.registerChunks(culler, chunks);

    // Camera at center
    CameraState camera;
    camera.focus_point = glm::vec3(128.0f, 0.0f, 128.0f);
    camera.distance = 50.0f;
    camera.pitch = CameraConfig::ISOMETRIC_PITCH;
    camera.yaw = CameraConfig::PRESET_N_YAW;

    glm::mat4 vp = createViewProjection(camera);
    culler.updateFrustum(vp);
    chunkCuller.updateVisibleChunks(culler, chunks);

    // Find chunks at focus point - they should ALWAYS be visible
    // Center chunks are at indices (3,3), (3,4), (4,3), (4,4)
    bool centerVisible = false;
    for (const TerrainChunk* visChunk : chunkCuller.getVisibleChunks()) {
        if ((visChunk->chunk_x == 3 || visChunk->chunk_x == 4) &&
            (visChunk->chunk_y == 3 || visChunk->chunk_y == 4)) {
            centerVisible = true;
            break;
        }
    }
    TEST_ASSERT(centerVisible, "Center chunks must always be visible (conservative culling)");

    std::cout << "PASS" << std::endl;
    g_testsPassed++;
    return true;
}

bool test_AllCameraAngles() {
    std::cout << "test_AllCameraAngles... ";

    FrustumCuller culler(256, 256);
    TerrainChunkCuller chunkCuller;

    // Create chunks
    std::vector<TerrainChunk> chunks;
    for (std::uint16_t cy = 0; cy < 8; ++cy) {
        for (std::uint16_t cx = 0; cx < 8; ++cx) {
            chunks.push_back(createChunk(cx, cy, 10));
        }
    }
    chunkCuller.registerChunks(culler, chunks);

    // Test all preset angles
    CameraMode presets[] = {
        CameraMode::Preset_N,
        CameraMode::Preset_E,
        CameraMode::Preset_S,
        CameraMode::Preset_W
    };

    for (CameraMode preset : presets) {
        CameraState camera;
        camera.focus_point = glm::vec3(128.0f, 0.0f, 128.0f);
        camera.distance = 50.0f;
        camera.pitch = CameraState::getPitchForPreset(preset);
        camera.yaw = CameraState::getYawForPreset(preset);

        glm::mat4 vp = createViewProjection(camera);
        culler.updateFrustum(vp);
        chunkCuller.updateVisibleChunks(culler, chunks);

        // At medium distance, center chunks should be visible from any angle
        TEST_ASSERT(chunkCuller.getVisibleChunkCount() > 0,
            "Should have visible chunks from all preset angles");
    }

    // Test free camera at extreme pitch
    CameraState freeCamera;
    freeCamera.focus_point = glm::vec3(128.0f, 0.0f, 128.0f);
    freeCamera.distance = 50.0f;
    freeCamera.pitch = CameraConfig::PITCH_MAX;
    freeCamera.yaw = 180.0f;

    glm::mat4 vp = createViewProjection(freeCamera);
    culler.updateFrustum(vp);
    chunkCuller.updateVisibleChunks(culler, chunks);

    TEST_ASSERT(chunkCuller.getVisibleChunkCount() > 0,
        "Should have visible chunks at extreme pitch");

    std::cout << "PASS" << std::endl;
    g_testsPassed++;
    return true;
}

bool test_IsChunkVisible() {
    std::cout << "test_IsChunkVisible... ";

    FrustumCuller culler(256, 256);
    TerrainChunkCuller chunkCuller;

    // Create a single chunk at center
    TerrainChunk centerChunk = createChunk(4, 4, 10);  // At (128-160, 128-160)
    TerrainChunk cornerChunk = createChunk(0, 0, 10);  // At (0-32, 0-32)

    // Camera at center with close zoom
    CameraState camera;
    camera.focus_point = glm::vec3(144.0f, 0.0f, 144.0f);  // Center of chunk (4,4)
    camera.distance = 20.0f;  // Very close
    camera.pitch = CameraConfig::ISOMETRIC_PITCH;
    camera.yaw = CameraConfig::PRESET_N_YAW;

    glm::mat4 vp = createViewProjection(camera);
    culler.updateFrustum(vp);

    // Center chunk should be visible
    TEST_ASSERT(chunkCuller.isChunkVisible(culler, centerChunk),
        "Center chunk should be visible");

    // Corner chunk should be culled at close zoom
    TEST_ASSERT(!chunkCuller.isChunkVisible(culler, cornerChunk),
        "Corner chunk should be culled at close zoom");

    std::cout << "PASS" << std::endl;
    g_testsPassed++;
    return true;
}

bool test_Statistics() {
    std::cout << "test_Statistics... ";

    FrustumCuller culler(256, 256);
    TerrainChunkCuller chunkCuller;

    // Create 64 chunks
    std::vector<TerrainChunk> chunks;
    for (std::uint16_t cy = 0; cy < 8; ++cy) {
        for (std::uint16_t cx = 0; cx < 8; ++cx) {
            chunks.push_back(createChunk(cx, cy, 10));
        }
    }
    chunkCuller.registerChunks(culler, chunks);

    // Camera at center with medium zoom
    CameraState camera;
    camera.focus_point = glm::vec3(128.0f, 0.0f, 128.0f);
    camera.distance = 50.0f;
    camera.pitch = CameraConfig::ISOMETRIC_PITCH;
    camera.yaw = CameraConfig::PRESET_N_YAW;

    glm::mat4 vp = createViewProjection(camera);
    culler.updateFrustum(vp);
    chunkCuller.updateVisibleChunks(culler, chunks);

    const auto& stats = chunkCuller.getStats();

    // Verify statistics are consistent
    TEST_ASSERT_EQ(stats.totalChunks, 64u, "Total chunks should be 64");
    TEST_ASSERT(stats.visibleChunks + stats.culledChunks == stats.totalChunks,
        "Visible + culled should equal total");
    TEST_ASSERT(stats.cullRatio >= 0.0f && stats.cullRatio <= 1.0f,
        "Cull ratio should be between 0 and 1");

    // Verify cull ratio calculation
    float expectedRatio = static_cast<float>(stats.culledChunks) /
                          static_cast<float>(stats.totalChunks);
    TEST_ASSERT_FLOAT_EQ(stats.cullRatio, expectedRatio, 0.001f,
        "Cull ratio calculation should be correct");

    std::cout << "PASS (visible: " << stats.visibleChunks
              << ", culled: " << stats.culledChunks
              << ", ratio: " << (stats.cullRatio * 100.0f) << "%)" << std::endl;
    g_testsPassed++;
    return true;
}

bool test_LargeMap_Performance() {
    std::cout << "test_LargeMap_Performance... ";

    // Create culler for 512x512 map (16x16 = 256 chunks)
    FrustumCuller culler(512, 512);
    TerrainChunkCuller chunkCuller;

    // Create 256 chunks
    std::vector<TerrainChunk> chunks;
    chunks.reserve(256);
    for (std::uint16_t cy = 0; cy < 16; ++cy) {
        for (std::uint16_t cx = 0; cx < 16; ++cx) {
            chunks.push_back(createChunk(cx, cy, 15));  // Mid elevation
        }
    }

    auto startRegister = std::chrono::high_resolution_clock::now();
    chunkCuller.registerChunks(culler, chunks);
    auto endRegister = std::chrono::high_resolution_clock::now();
    auto registerTime = std::chrono::duration_cast<std::chrono::microseconds>(
        endRegister - startRegister).count();

    // Camera at center, typical zoom
    CameraState camera;
    camera.focus_point = glm::vec3(256.0f, 0.0f, 256.0f);
    camera.distance = 80.0f;  // Typical gameplay zoom
    camera.pitch = CameraConfig::ISOMETRIC_PITCH;
    camera.yaw = CameraConfig::PRESET_N_YAW;

    glm::mat4 vp = createViewProjection(camera);

    auto startCull = std::chrono::high_resolution_clock::now();

    culler.updateFrustum(vp);
    chunkCuller.updateVisibleChunks(culler, chunks);

    auto endCull = std::chrono::high_resolution_clock::now();
    auto cullTime = std::chrono::duration_cast<std::chrono::microseconds>(
        endCull - startCull).count();

    const auto& stats = chunkCuller.getStats();

    // Print performance info
    std::cout << std::endl;
    std::cout << "  512x512 map (256 chunks):" << std::endl;
    std::cout << "  Registration time: " << registerTime << " us" << std::endl;
    std::cout << "  Culling time: " << cullTime << " us" << std::endl;
    std::cout << "  Visible chunks: " << stats.visibleChunks
              << "/" << stats.totalChunks << std::endl;
    std::cout << "  Cull ratio: " << (stats.cullRatio * 100.0f) << "%" << std::endl;

    // Performance requirements
    TEST_ASSERT(cullTime < 1000,
        "Culling should complete in under 1ms");

    // At typical zoom, we expect significant culling (target: 84-94% culled)
    // This is a 15-40 visible out of 256 chunks
    TEST_ASSERT(stats.visibleChunks <= 80,
        "At typical zoom, should have at most ~80 visible chunks");
    TEST_ASSERT(stats.cullRatio >= 0.6f,
        "At typical zoom, should cull at least 60% of chunks");

    std::cout << "  PASS" << std::endl;
    g_testsPassed++;
    return true;
}

bool test_EmptyChunks() {
    std::cout << "test_EmptyChunks... ";

    FrustumCuller culler(128, 128);
    TerrainChunkCuller chunkCuller;

    std::vector<TerrainChunk> chunks;  // Empty

    // Should not crash with empty chunks
    chunkCuller.registerChunks(culler, chunks);

    CameraState camera;
    camera.focus_point = glm::vec3(64.0f, 0.0f, 64.0f);
    camera.distance = 50.0f;
    camera.pitch = CameraConfig::ISOMETRIC_PITCH;
    camera.yaw = CameraConfig::PRESET_N_YAW;

    glm::mat4 vp = createViewProjection(camera);
    culler.updateFrustum(vp);

    chunkCuller.updateVisibleChunks(culler, chunks);

    TEST_ASSERT_EQ(chunkCuller.getVisibleChunkCount(), 0u,
        "Empty chunks should result in 0 visible");
    TEST_ASSERT_EQ(chunkCuller.getStats().totalChunks, 0u,
        "Stats should show 0 total chunks");

    std::cout << "PASS" << std::endl;
    g_testsPassed++;
    return true;
}

bool test_ChunksWithoutGPUResources() {
    std::cout << "test_ChunksWithoutGPUResources... ";

    FrustumCuller culler(128, 128);
    TerrainChunkCuller chunkCuller;

    // Create chunks without GPU resources
    std::vector<TerrainChunk> chunks;
    for (std::uint16_t cy = 0; cy < 4; ++cy) {
        for (std::uint16_t cx = 0; cx < 4; ++cx) {
            TerrainChunk chunk(cx, cy);
            chunk.computeAABB(static_cast<std::uint8_t>(10));
            // NOT setting has_gpu_resources = true
            chunks.push_back(std::move(chunk));
        }
    }

    chunkCuller.registerChunks(culler, chunks);

    CameraState camera;
    camera.focus_point = glm::vec3(64.0f, 0.0f, 64.0f);
    camera.distance = 50.0f;
    camera.pitch = CameraConfig::ISOMETRIC_PITCH;
    camera.yaw = CameraConfig::PRESET_N_YAW;

    glm::mat4 vp = createViewProjection(camera);
    culler.updateFrustum(vp);

    chunkCuller.updateVisibleChunks(culler, chunks);

    // Chunks without GPU resources should not be included in visible list
    TEST_ASSERT_EQ(chunkCuller.getVisibleChunkCount(), 0u,
        "Chunks without GPU resources should not be visible");

    std::cout << "PASS" << std::endl;
    g_testsPassed++;
    return true;
}

bool test_EntityIdComputation() {
    std::cout << "test_EntityIdComputation... ";

    // Test entity ID calculation
    EntityID id0 = computeChunkEntityId(0);
    EntityID id1 = computeChunkEntityId(1);
    EntityID id255 = computeChunkEntityId(255);

    // Default base is 0x80000000
    TEST_ASSERT_EQ(id0, 0x80000000u, "Chunk 0 entity ID should be base");
    TEST_ASSERT_EQ(id1, 0x80000001u, "Chunk 1 entity ID should be base+1");
    TEST_ASSERT_EQ(id255, 0x800000FFu, "Chunk 255 entity ID should be base+255");

    // Test with custom base
    EntityID customId = computeChunkEntityId(10, 0x10000000);
    TEST_ASSERT_EQ(customId, 0x1000000Au, "Custom base entity ID calculation");

    std::cout << "PASS" << std::endl;
    g_testsPassed++;
    return true;
}

bool test_ChunkCenterPosition() {
    std::cout << "test_ChunkCenterPosition... ";

    TerrainChunk chunk(2, 3);  // Covers (64-96, 96-128)
    chunk.computeAABB(static_cast<std::uint8_t>(20));  // max.y = 5.0

    glm::vec3 center = computeChunkCenterPosition(chunk);

    // Center should be at (80, 2.5, 112)
    // XZ: (64+96)/2 = 80, (96+128)/2 = 112
    // Y: (0+5)/2 = 2.5
    TEST_ASSERT_FLOAT_EQ(center.x, 80.0f, 0.001f, "Center X should be 80");
    TEST_ASSERT_FLOAT_EQ(center.y, 2.5f, 0.001f, "Center Y should be 2.5");
    TEST_ASSERT_FLOAT_EQ(center.z, 112.0f, 0.001f, "Center Z should be 112");

    std::cout << "PASS" << std::endl;
    g_testsPassed++;
    return true;
}

// ============================================================================
// Main
// ============================================================================

int main() {
    std::cout << "===== Terrain Chunk Culler Tests (Ticket 3-034) =====" << std::endl;
    std::cout << std::endl;

    test_ChunkAABB_BasicComputation();
    test_ChunkAABB_MaxElevation();
    test_ChunkRegistration();
    test_ChunkUnregistration();
    test_VisibleChunks_CenterFocus();
    test_FrustumCulling_CornerChunks();
    test_ConservativeCulling_NoPopping();
    test_AllCameraAngles();
    test_IsChunkVisible();
    test_Statistics();
    test_LargeMap_Performance();
    test_EmptyChunks();
    test_ChunksWithoutGPUResources();
    test_EntityIdComputation();
    test_ChunkCenterPosition();

    std::cout << std::endl;
    std::cout << "===== Results =====" << std::endl;
    std::cout << "Passed: " << g_testsPassed << std::endl;
    std::cout << "Failed: " << g_testsFailed << std::endl;

    return g_testsFailed > 0 ? 1 : 0;
}
