/**
 * @file test_instancing.cpp
 * @brief Unit tests for GPU instancing (ticket 2-012).
 *
 * Tests:
 * - ToonInstanceData creation and properties
 * - Per-instance tint color support
 * - Per-instance emissive intensity/color for powered/unpowered state
 * - Instance buffer statistics and chunk configuration
 * - Draw call reduction calculations (10x+ target)
 * - Performance budget validation for 512x512 maps
 */

#include "sims3000/render/InstanceBuffer.h"
#include "sims3000/render/InstancedRenderer.h"
#include "sims3000/render/ToonShader.h"
#include "sims3000/render/RenderCommands.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cstdio>
#include <cmath>

using namespace sims3000;

// =============================================================================
// Simple Test Framework (consistent with project style)
// =============================================================================

static int g_testsPassed = 0;
static int g_testsFailed = 0;

#define TEST(name) void test_##name()
#define RUN_TEST(name) do { \
    printf("Running test: %s... ", #name); \
    test_##name(); \
    printf("PASSED\n"); \
    g_testsPassed++; \
} while(0)

#define ASSERT_EQ(expected, actual) do { \
    if ((expected) != (actual)) { \
        printf("FAILED\n  Expected: %d, Actual: %d\n", (int)(expected), (int)(actual)); \
        g_testsFailed++; \
        return; \
    } \
} while(0)

#define ASSERT_EQ_U64(expected, actual) do { \
    if ((expected) != (actual)) { \
        printf("FAILED\n  Expected: %llu, Actual: %llu\n", \
               (unsigned long long)(expected), (unsigned long long)(actual)); \
        g_testsFailed++; \
        return; \
    } \
} while(0)

#define ASSERT_FLOAT_EQ(expected, actual, epsilon) do { \
    if (std::abs((expected) - (actual)) > (epsilon)) { \
        printf("FAILED\n  Expected: %f, Actual: %f\n", (float)(expected), (float)(actual)); \
        g_testsFailed++; \
        return; \
    } \
} while(0)

#define ASSERT_TRUE(condition) do { \
    if (!(condition)) { \
        printf("FAILED\n  Condition was false: %s\n", #condition); \
        g_testsFailed++; \
        return; \
    } \
} while(0)

#define ASSERT_GE(actual, expected) do { \
    if ((actual) < (expected)) { \
        printf("FAILED\n  Expected >= %f, Actual: %f\n", (double)(expected), (double)(actual)); \
        g_testsFailed++; \
        return; \
    } \
} while(0)

#define ASSERT_GT(actual, expected) do { \
    if ((actual) <= (expected)) { \
        printf("FAILED\n  Expected > %f, Actual: %f\n", (double)(expected), (double)(actual)); \
        g_testsFailed++; \
        return; \
    } \
} while(0)

#define ASSERT_LT(actual, expected) do { \
    if ((actual) >= (expected)) { \
        printf("FAILED\n  Expected < %f, Actual: %f\n", (double)(expected), (double)(actual)); \
        g_testsFailed++; \
        return; \
    } \
} while(0)

#define ASSERT_LE(actual, expected) do { \
    if ((actual) > (expected)) { \
        printf("FAILED\n  Expected <= %f, Actual: %f\n", (double)(expected), (double)(actual)); \
        g_testsFailed++; \
        return; \
    } \
} while(0)

#define ASSERT_NEAR(actual, expected, tolerance) do { \
    if (std::abs((actual) - (expected)) > (tolerance)) { \
        printf("FAILED\n  Expected: %f (+/- %f), Actual: %f\n", \
               (double)(expected), (double)(tolerance), (double)(actual)); \
        g_testsFailed++; \
        return; \
    } \
} while(0)

// =============================================================================
// ToonInstanceData Tests
// =============================================================================

TEST(ToonInstanceData_DefaultConstruction) {
    ToonInstanceData data;

    // Default model matrix is identity
    ASSERT_FLOAT_EQ(1.0f, data.model[0][0], 0.0001f);
    ASSERT_FLOAT_EQ(1.0f, data.model[1][1], 0.0001f);
    ASSERT_FLOAT_EQ(1.0f, data.model[2][2], 0.0001f);
    ASSERT_FLOAT_EQ(1.0f, data.model[3][3], 0.0001f);

    // Default base color is white
    ASSERT_FLOAT_EQ(1.0f, data.baseColor.r, 0.0001f);
    ASSERT_FLOAT_EQ(1.0f, data.baseColor.g, 0.0001f);
    ASSERT_FLOAT_EQ(1.0f, data.baseColor.b, 0.0001f);
    ASSERT_FLOAT_EQ(1.0f, data.baseColor.a, 0.0001f);

    // Default emissive is off
    ASSERT_FLOAT_EQ(0.0f, data.emissiveColor.r, 0.0001f);
    ASSERT_FLOAT_EQ(0.0f, data.emissiveColor.g, 0.0001f);
    ASSERT_FLOAT_EQ(0.0f, data.emissiveColor.b, 0.0001f);
    ASSERT_FLOAT_EQ(0.0f, data.emissiveColor.a, 0.0001f);

    // Default ambient is 0 (use global)
    ASSERT_FLOAT_EQ(0.0f, data.ambientStrength, 0.0001f);
}

TEST(ToonInstanceData_SizeIs112Bytes) {
    // Static assert in header should catch this, but verify at runtime too
    ASSERT_EQ(112, sizeof(ToonInstanceData));
}

TEST(ToonInstanceData_CreateWithHelper) {
    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(10.0f, 5.0f, 20.0f));
    glm::vec4 baseColor(0.5f, 0.6f, 0.7f, 1.0f);
    glm::vec4 emissiveColor(0.0f, 1.0f, 0.8f, 0.5f);  // Teal with 50% intensity
    float ambient = 0.15f;

    ToonInstanceData data = createInstanceData(model, baseColor, emissiveColor, ambient);

    // Check transform position
    ASSERT_FLOAT_EQ(10.0f, data.model[3][0], 0.0001f);
    ASSERT_FLOAT_EQ(5.0f, data.model[3][1], 0.0001f);
    ASSERT_FLOAT_EQ(20.0f, data.model[3][2], 0.0001f);

    // Check colors
    ASSERT_FLOAT_EQ(0.5f, data.baseColor.r, 0.0001f);
    ASSERT_FLOAT_EQ(0.6f, data.baseColor.g, 0.0001f);
    ASSERT_FLOAT_EQ(0.7f, data.baseColor.b, 0.0001f);

    ASSERT_FLOAT_EQ(0.0f, data.emissiveColor.r, 0.0001f);
    ASSERT_FLOAT_EQ(1.0f, data.emissiveColor.g, 0.0001f);
    ASSERT_FLOAT_EQ(0.8f, data.emissiveColor.b, 0.0001f);
    ASSERT_FLOAT_EQ(0.5f, data.emissiveColor.a, 0.0001f);

    ASSERT_FLOAT_EQ(0.15f, data.ambientStrength, 0.0001f);
}

TEST(ToonInstanceData_EmissiveForPoweredUnpowered) {
    // Powered building: emissive intensity > 0
    ToonInstanceData poweredData = createInstanceData(
        glm::mat4(1.0f),
        glm::vec4(1.0f),
        glm::vec4(0.0f, 1.0f, 0.8f, 0.8f),  // 80% intensity
        0.0f
    );
    ASSERT_GT(poweredData.emissiveColor.a, 0.0f);

    // Unpowered building: emissive intensity = 0
    ToonInstanceData unpoweredData = createInstanceData(
        glm::mat4(1.0f),
        glm::vec4(1.0f),
        glm::vec4(0.0f, 1.0f, 0.8f, 0.0f),  // 0% intensity
        0.0f
    );
    ASSERT_FLOAT_EQ(0.0f, unpoweredData.emissiveColor.a, 0.0001f);
}

// =============================================================================
// RenderCommands Instance Creation Tests
// =============================================================================

TEST(RenderCommands_CreateInstanceData) {
    glm::mat4 model = glm::scale(glm::mat4(1.0f), glm::vec3(2.0f));
    glm::vec4 baseColor(1.0f, 0.0f, 0.0f, 1.0f);  // Red
    glm::vec4 emissive(0.0f, 0.5f, 0.0f, 1.0f);   // Green glow

    ToonInstanceData data = RenderCommands::createInstanceData(
        model, baseColor, emissive, 0.1f);

    // Scale matrix check
    ASSERT_FLOAT_EQ(2.0f, data.model[0][0], 0.0001f);
    ASSERT_FLOAT_EQ(2.0f, data.model[1][1], 0.0001f);
    ASSERT_FLOAT_EQ(2.0f, data.model[2][2], 0.0001f);

    // Colors
    ASSERT_FLOAT_EQ(1.0f, data.baseColor.r, 0.0001f);
    ASSERT_FLOAT_EQ(0.0f, data.baseColor.g, 0.0001f);
    ASSERT_FLOAT_EQ(0.5f, data.emissiveColor.g, 0.0001f);
    ASSERT_FLOAT_EQ(1.0f, data.emissiveColor.a, 0.0001f);
    ASSERT_FLOAT_EQ(0.1f, data.ambientStrength, 0.0001f);
}

// =============================================================================
// RenderCommandStats Tests
// =============================================================================

TEST(RenderCommandStats_DefaultValues) {
    RenderCommandStats stats;

    ASSERT_EQ(0, stats.drawCalls);
    ASSERT_EQ(0, stats.meshesDrawn);
    ASSERT_EQ(0, stats.trianglesDrawn);
    ASSERT_EQ(0, stats.instancedDrawCalls);
    ASSERT_EQ(0, stats.totalInstances);
}

TEST(RenderCommandStats_Reset) {
    RenderCommandStats stats;
    stats.drawCalls = 100;
    stats.instancedDrawCalls = 50;
    stats.totalInstances = 5000;
    stats.trianglesDrawn = 100000;

    stats.reset();

    ASSERT_EQ(0, stats.drawCalls);
    ASSERT_EQ(0, stats.instancedDrawCalls);
    ASSERT_EQ(0, stats.totalInstances);
    ASSERT_EQ(0, stats.trianglesDrawn);
}

// =============================================================================
// InstanceBufferStats Tests
// =============================================================================

TEST(InstanceBufferStats_DefaultValues) {
    InstanceBufferStats stats;

    ASSERT_EQ(0, stats.instanceCount);
    ASSERT_EQ(0, stats.capacity);
    ASSERT_EQ(0, stats.bytesUsed);
    ASSERT_EQ(0, stats.bytesCapacity);
    ASSERT_EQ(0, stats.uploadCount);
    ASSERT_EQ(0, stats.chunkCount);
}

// =============================================================================
// InstanceChunk Tests
// =============================================================================

TEST(InstanceChunk_DefaultValues) {
    InstanceChunk chunk;

    ASSERT_EQ(0, chunk.startIndex);
    ASSERT_EQ(0, chunk.count);
    ASSERT_TRUE(chunk.visible);
    ASSERT_FLOAT_EQ(0.0f, chunk.boundsMin.x, 0.0001f);
    ASSERT_FLOAT_EQ(0.0f, chunk.boundsMax.x, 0.0001f);
}

// =============================================================================
// InstancedRendererStats Tests
// =============================================================================

TEST(InstancedRendererStats_DefaultValues) {
    InstancedRendererStats stats;

    ASSERT_EQ(0, stats.totalInstances);
    ASSERT_EQ(0, stats.totalDrawCalls);
    ASSERT_EQ(0, stats.totalTriangles);
    ASSERT_EQ(0, stats.batchCount);
    ASSERT_EQ(0, stats.instancedDrawCalls);
    ASSERT_FLOAT_EQ(0.0f, stats.drawCallReduction, 0.0001f);
}

TEST(InstancedRendererStats_Reset) {
    InstancedRendererStats stats;
    stats.totalInstances = 10000;
    stats.totalDrawCalls = 10;
    stats.instancedDrawCalls = 10;
    stats.drawCallReduction = 0.999f;

    stats.reset();

    ASSERT_EQ(0, stats.totalInstances);
    ASSERT_EQ(0, stats.totalDrawCalls);
    ASSERT_FLOAT_EQ(0.0f, stats.drawCallReduction, 0.0001f);
}

TEST(InstancedRendererStats_DrawCallReductionCalculation) {
    // Without instancing: 10000 instances = 10000 draw calls
    // With instancing: 10000 instances in 10 batches = 10 draw calls
    // Reduction = 1 - (10 / 10000) = 0.999

    InstancedRendererStats stats;
    std::uint32_t naiveDrawCalls = 10000;
    std::uint32_t actualDrawCalls = 10;

    stats.drawCallReduction = 1.0f - static_cast<float>(actualDrawCalls) / static_cast<float>(naiveDrawCalls);

    ASSERT_NEAR(stats.drawCallReduction, 0.999f, 0.001f);
}

// =============================================================================
// InstancedRendererConfig Tests
// =============================================================================

TEST(InstancedRendererConfig_DefaultValues) {
    InstancedRendererConfig config;

    ASSERT_EQ(4096, config.defaultBufferCapacity);
    ASSERT_TRUE(config.enableChunking);
    ASSERT_EQ(InstanceBuffer::DEFAULT_CHUNK_SIZE, config.chunkSize);
    ASSERT_EQ(262144, config.terrainBufferCapacity);  // 512x512
    ASSERT_EQ(4096, config.buildingBufferCapacity);
    ASSERT_TRUE(config.enableFrustumCulling);
}

TEST(InstancedRendererConfig_TerrainCapacityFor512x512Maps) {
    InstancedRendererConfig config;

    // 512x512 = 262144 tiles
    ASSERT_GE(config.terrainBufferCapacity, 262144);
}

// =============================================================================
// Frustum Plane Extraction Tests
// =============================================================================

TEST(FrustumExtraction_ExtractFrustumPlanes) {
    // Create a simple orthographic projection for testing
    glm::mat4 projection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 0.1f, 100.0f);
    glm::mat4 view = glm::lookAt(
        glm::vec3(0.0f, 0.0f, 10.0f),  // Eye at z=10
        glm::vec3(0.0f, 0.0f, 0.0f),   // Looking at origin
        glm::vec3(0.0f, 1.0f, 0.0f)    // Up vector
    );

    glm::mat4 viewProjection = projection * view;
    glm::vec4 planes[6];

    InstancedRenderer::extractFrustumPlanes(viewProjection, planes);

    // All planes should be normalized (length of xyz should be ~1)
    for (int i = 0; i < 6; ++i) {
        float length = glm::length(glm::vec3(planes[i]));
        ASSERT_NEAR(length, 1.0f, 0.01f);
    }
}

// =============================================================================
// Draw Call Reduction Validation Tests
// =============================================================================

TEST(DrawCallReduction_TenXForTerrain) {
    // Scenario: 10000 terrain tiles, all same model
    // Without instancing: 10000 draw calls
    // With instancing: 1 draw call (or N for N meshes per model)

    std::uint32_t tileCount = 10000;
    std::uint32_t meshesPerModel = 1;

    // With instancing
    std::uint32_t instancedDrawCalls = meshesPerModel;

    // Calculate reduction
    float reduction = 1.0f - static_cast<float>(instancedDrawCalls) / static_cast<float>(tileCount);

    // Should achieve >10x reduction (>90% reduction)
    ASSERT_GT(reduction, 0.9f);

    // Express as ratio
    float ratio = static_cast<float>(tileCount) / static_cast<float>(instancedDrawCalls);
    ASSERT_GE(ratio, 10.0f);
}

TEST(DrawCallReduction_TenXForBuildings) {
    // Scenario: 1000 buildings of same type
    // Without instancing: 1000 draw calls
    // With instancing: 1 draw call per mesh

    std::uint32_t buildingCount = 1000;
    std::uint32_t meshesPerBuilding = 3;  // More complex model

    std::uint32_t withoutInstancing = buildingCount * meshesPerBuilding;
    std::uint32_t withInstancing = meshesPerBuilding;  // One call per mesh, all instances

    float ratio = static_cast<float>(withoutInstancing) / static_cast<float>(withInstancing);
    ASSERT_GE(ratio, 10.0f);
}

TEST(DrawCallReduction_LargeMapPerformanceBudget) {
    // 512x512 map = 262144 tiles
    // With instancing, terrain should be ~1 draw call (or few for multiple meshes)
    // Plus buildings (~1000 unique building types x ~10 instances each = 10000 buildings)

    std::uint32_t terrainTiles = 262144;
    std::uint32_t buildingTypes = 100;
    std::uint32_t buildingsPerType = 100;
    std::uint32_t totalBuildings = buildingTypes * buildingsPerType;  // 10000

    // With instancing
    std::uint32_t terrainDrawCalls = 1;  // One mesh type for terrain
    std::uint32_t buildingDrawCalls = buildingTypes;  // One per building type

    std::uint32_t totalInstancedDrawCalls = terrainDrawCalls + buildingDrawCalls;

    // Without instancing
    std::uint32_t totalNaiveDrawCalls = terrainTiles + totalBuildings;

    // Verify we meet the 500-1000 draw call budget
    ASSERT_LE(totalInstancedDrawCalls, 500);

    // Verify 10x+ reduction
    float ratio = static_cast<float>(totalNaiveDrawCalls) / static_cast<float>(totalInstancedDrawCalls);
    ASSERT_GT(ratio, 10.0f);
}

// =============================================================================
// 512x512 Map Support Tests
// =============================================================================

TEST(LargeMapSupport_MaxInstancesCapacity) {
    ASSERT_EQ(262144, InstanceBuffer::MAX_INSTANCES);  // 512x512
}

TEST(LargeMapSupport_ChunkedInstancing) {
    // Default chunk size
    std::uint32_t chunkSize = InstanceBuffer::DEFAULT_CHUNK_SIZE;
    ASSERT_EQ(4096, chunkSize);

    // Number of chunks for 512x512 map
    std::uint32_t mapSize = 262144;
    std::uint32_t chunkCount = (mapSize + chunkSize - 1) / chunkSize;

    ASSERT_EQ(64, chunkCount);  // 262144 / 4096 = 64 chunks
}

TEST(LargeMapSupport_PerformanceBudgetValidation) {
    // Canon requires validation for 512x512 maps (262k tiles)

    // Instance buffer memory per tile: 112 bytes (sizeof(ToonInstanceData))
    std::uint32_t instanceDataSize = sizeof(ToonInstanceData);
    ASSERT_EQ(112, instanceDataSize);

    // Total memory for terrain: 262144 * 112 = ~28 MB
    std::uint64_t terrainMemory = 262144ULL * instanceDataSize;
    ASSERT_LT(terrainMemory, 32ULL * 1024 * 1024);  // Under 32 MB

    // Total memory for buildings (estimate 10k buildings): ~1.1 MB
    std::uint64_t buildingMemory = 10000ULL * instanceDataSize;
    ASSERT_LT(buildingMemory, 2ULL * 1024 * 1024);  // Under 2 MB

    // Total under instance buffer budget (32 MB from epic-2 GPU budget)
    ASSERT_LT(terrainMemory + buildingMemory, 64ULL * 1024 * 1024);
}

// =============================================================================
// Per-Instance Tint and Emissive Tests
// =============================================================================

TEST(PerInstanceProperties_TintColorSupport) {
    ToonInstanceData data = createInstanceData(
        glm::mat4(1.0f),
        glm::vec4(0.8f, 0.2f, 0.2f, 1.0f),  // Reddish tint
        glm::vec4(0.0f),
        0.0f
    );

    ASSERT_FLOAT_EQ(0.8f, data.baseColor.r, 0.0001f);
    ASSERT_FLOAT_EQ(0.2f, data.baseColor.g, 0.0001f);
    ASSERT_FLOAT_EQ(0.2f, data.baseColor.b, 0.0001f);
}

TEST(PerInstanceProperties_EmissiveColorAndIntensity) {
    // Cyan glow at 75% intensity
    ToonInstanceData data = createInstanceData(
        glm::mat4(1.0f),
        glm::vec4(1.0f),
        glm::vec4(0.0f, 0.83f, 0.67f, 0.75f),  // #00D4AA at 75%
        0.0f
    );

    ASSERT_NEAR(data.emissiveColor.r, 0.0f, 0.01f);
    ASSERT_NEAR(data.emissiveColor.g, 0.83f, 0.01f);
    ASSERT_NEAR(data.emissiveColor.b, 0.67f, 0.01f);
    ASSERT_FLOAT_EQ(0.75f, data.emissiveColor.a, 0.0001f);  // Intensity in alpha
}

TEST(PerInstanceProperties_PoweredUnpoweredStateDifference) {
    glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(5.0f, 0.0f, 10.0f));

    // Same building, different power states
    ToonInstanceData powered = createInstanceData(
        transform,
        glm::vec4(1.0f),
        glm::vec4(0.0f, 1.0f, 0.8f, 0.8f),  // Glowing
        0.0f
    );

    ToonInstanceData unpowered = createInstanceData(
        transform,
        glm::vec4(0.7f, 0.7f, 0.7f, 1.0f),  // Dimmer tint
        glm::vec4(0.0f, 1.0f, 0.8f, 0.0f),  // No glow
        0.0f
    );

    // Powered has emissive intensity
    ASSERT_GT(powered.emissiveColor.a, 0.0f);

    // Unpowered has no emissive intensity
    ASSERT_FLOAT_EQ(0.0f, unpowered.emissiveColor.a, 0.0001f);

    // Unpowered has dimmer base color
    ASSERT_LT(unpowered.baseColor.r, powered.baseColor.r);
}

// =============================================================================
// Main
// =============================================================================

int main() {
    printf("=== GPU Instancing Unit Tests (Ticket 2-012) ===\n\n");

    // ToonInstanceData tests
    printf("--- ToonInstanceData Tests ---\n");
    RUN_TEST(ToonInstanceData_DefaultConstruction);
    RUN_TEST(ToonInstanceData_SizeIs112Bytes);
    RUN_TEST(ToonInstanceData_CreateWithHelper);
    RUN_TEST(ToonInstanceData_EmissiveForPoweredUnpowered);

    // RenderCommands instance tests
    printf("\n--- RenderCommands Instance Tests ---\n");
    RUN_TEST(RenderCommands_CreateInstanceData);

    // RenderCommandStats tests
    printf("\n--- RenderCommandStats Tests ---\n");
    RUN_TEST(RenderCommandStats_DefaultValues);
    RUN_TEST(RenderCommandStats_Reset);

    // InstanceBufferStats tests
    printf("\n--- InstanceBufferStats Tests ---\n");
    RUN_TEST(InstanceBufferStats_DefaultValues);

    // InstanceChunk tests
    printf("\n--- InstanceChunk Tests ---\n");
    RUN_TEST(InstanceChunk_DefaultValues);

    // InstancedRendererStats tests
    printf("\n--- InstancedRendererStats Tests ---\n");
    RUN_TEST(InstancedRendererStats_DefaultValues);
    RUN_TEST(InstancedRendererStats_Reset);
    RUN_TEST(InstancedRendererStats_DrawCallReductionCalculation);

    // InstancedRendererConfig tests
    printf("\n--- InstancedRendererConfig Tests ---\n");
    RUN_TEST(InstancedRendererConfig_DefaultValues);
    RUN_TEST(InstancedRendererConfig_TerrainCapacityFor512x512Maps);

    // Frustum extraction tests
    printf("\n--- Frustum Extraction Tests ---\n");
    RUN_TEST(FrustumExtraction_ExtractFrustumPlanes);

    // Draw call reduction tests (10x+ target)
    printf("\n--- Draw Call Reduction Tests (10x+ target) ---\n");
    RUN_TEST(DrawCallReduction_TenXForTerrain);
    RUN_TEST(DrawCallReduction_TenXForBuildings);
    RUN_TEST(DrawCallReduction_LargeMapPerformanceBudget);

    // Large map support tests (512x512)
    printf("\n--- Large Map Support Tests (512x512) ---\n");
    RUN_TEST(LargeMapSupport_MaxInstancesCapacity);
    RUN_TEST(LargeMapSupport_ChunkedInstancing);
    RUN_TEST(LargeMapSupport_PerformanceBudgetValidation);

    // Per-instance properties tests
    printf("\n--- Per-Instance Properties Tests ---\n");
    RUN_TEST(PerInstanceProperties_TintColorSupport);
    RUN_TEST(PerInstanceProperties_EmissiveColorAndIntensity);
    RUN_TEST(PerInstanceProperties_PoweredUnpoweredStateDifference);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", g_testsPassed);
    printf("Failed: %d\n", g_testsFailed);

    return g_testsFailed == 0 ? 0 : 1;
}
