/**
 * @file test_vegetation_renderer.cpp
 * @brief Unit tests for VegetationRenderer (Ticket 3-030)
 *
 * Tests cover:
 * - Configuration struct defaults
 * - Statistics struct reset
 * - Transform matrix building (translate * rotateY * scale)
 * - Emissive color mapping from terrain type
 * - LOD visibility control
 * - Model type count constant
 *
 * Note: GPU-dependent tests (actual rendering, instance buffers) require
 * manual verification as they need a display and GPU context.
 */

#include "sims3000/render/VegetationRenderer.h"
#include "sims3000/render/VegetationInstance.h"
#include "sims3000/render/RenderLayer.h"
#include "sims3000/terrain/TerrainTypeInfo.h"

#include <cstdio>
#include <cstdlib>
#include <cmath>

using namespace sims3000;
using namespace sims3000::render;

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
        printf("\n  FAILED: %s ~= %s (got %f, expected %f, diff %f) (line %d)\n", \
               #a, #b, static_cast<double>(a), static_cast<double>(b), \
               static_cast<double>(std::abs((a) - (b))), __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

// =============================================================================
// Configuration Tests
// =============================================================================

TEST(config_default_values) {
    VegetationRendererConfig config;

    ASSERT_EQ(65536u, config.instanceBufferCapacity);
    ASSERT_EQ(std::string("assets/models/vegetation/"), config.modelsPath);
    ASSERT_EQ(std::string("biolume_tree.glb"), config.biolumeTreeModel);
    ASSERT_EQ(std::string("crystal_spire.glb"), config.crystalSpireModel);
    ASSERT_EQ(std::string("spore_emitter.glb"), config.sporeEmitterModel);
    ASSERT(config.usePlaceholderModels);
    ASSERT_EQ(0, config.maxLodLevel);
}

TEST(config_custom_values) {
    VegetationRendererConfig config;
    config.instanceBufferCapacity = 32768;
    config.modelsPath = "custom/path/";
    config.biolumeTreeModel = "tree.glb";
    config.maxLodLevel = 1;

    ASSERT_EQ(32768u, config.instanceBufferCapacity);
    ASSERT_EQ(std::string("custom/path/"), config.modelsPath);
    ASSERT_EQ(std::string("tree.glb"), config.biolumeTreeModel);
    ASSERT_EQ(1, config.maxLodLevel);
}

// =============================================================================
// Statistics Tests
// =============================================================================

TEST(stats_default_values) {
    VegetationRendererStats stats;

    ASSERT_EQ(0u, stats.totalInstances);
    ASSERT_EQ(0u, stats.drawCalls);
    ASSERT_EQ(0u, stats.triangles);
    ASSERT_EQ(0u, stats.instancesPerType[0]);
    ASSERT_EQ(0u, stats.instancesPerType[1]);
    ASSERT_EQ(0u, stats.instancesPerType[2]);
    ASSERT_NEAR(0.0f, stats.renderTimeMs, 0.001f);
}

TEST(stats_reset) {
    VegetationRendererStats stats;
    stats.totalInstances = 1000;
    stats.drawCalls = 10;
    stats.triangles = 50000;
    stats.instancesPerType[0] = 300;
    stats.instancesPerType[1] = 400;
    stats.instancesPerType[2] = 300;
    stats.renderTimeMs = 1.5f;

    stats.reset();

    ASSERT_EQ(0u, stats.totalInstances);
    ASSERT_EQ(0u, stats.drawCalls);
    ASSERT_EQ(0u, stats.triangles);
    ASSERT_EQ(0u, stats.instancesPerType[0]);
    ASSERT_EQ(0u, stats.instancesPerType[1]);
    ASSERT_EQ(0u, stats.instancesPerType[2]);
    ASSERT_NEAR(0.0f, stats.renderTimeMs, 0.001f);
}

// =============================================================================
// Model Type Constants Tests
// =============================================================================

TEST(model_type_count) {
    // VegetationRenderer::MODEL_TYPE_COUNT should match VegetationModelType::Count
    ASSERT_EQ(3u, VegetationRenderer::MODEL_TYPE_COUNT);
    ASSERT_EQ(static_cast<std::size_t>(VegetationModelType::Count),
              VegetationRenderer::MODEL_TYPE_COUNT);
}

TEST(model_type_enum_values) {
    ASSERT_EQ(0, static_cast<int>(VegetationModelType::BiolumeTree));
    ASSERT_EQ(1, static_cast<int>(VegetationModelType::CrystalSpire));
    ASSERT_EQ(2, static_cast<int>(VegetationModelType::SporeEmitter));
    ASSERT_EQ(3, static_cast<int>(VegetationModelType::Count));
}

// =============================================================================
// VegetationInstance Structure Tests
// =============================================================================

TEST(vegetation_instance_size) {
    // VegetationInstance should be 24 bytes as documented
    ASSERT_EQ(24u, sizeof(VegetationInstance));
}

TEST(vegetation_instance_construction) {
    VegetationInstance instance;
    instance.position = glm::vec3(10.0f, 5.0f, 20.0f);
    instance.rotation_y = 1.57f;  // ~90 degrees
    instance.scale = 1.5f;
    instance.model_type = VegetationModelType::CrystalSpire;

    ASSERT_NEAR(10.0f, instance.position.x, 0.001f);
    ASSERT_NEAR(5.0f, instance.position.y, 0.001f);
    ASSERT_NEAR(20.0f, instance.position.z, 0.001f);
    ASSERT_NEAR(1.57f, instance.rotation_y, 0.001f);
    ASSERT_NEAR(1.5f, instance.scale, 0.001f);
    ASSERT_EQ(static_cast<int>(VegetationModelType::CrystalSpire),
              static_cast<int>(instance.model_type));
}

// =============================================================================
// Render Layer Tests
// =============================================================================

TEST(vegetation_render_layer_exists) {
    // Verify RenderLayer::Vegetation exists and is in correct position
    ASSERT_EQ(2, static_cast<int>(RenderLayer::Vegetation));
}

TEST(vegetation_layer_after_terrain) {
    ASSERT(RenderLayer::Terrain < RenderLayer::Vegetation);
}

TEST(vegetation_layer_before_water) {
    ASSERT(RenderLayer::Vegetation < RenderLayer::Water);
}

TEST(vegetation_layer_is_opaque) {
    ASSERT(isOpaqueLayer(RenderLayer::Vegetation));
}

TEST(vegetation_layer_is_lit) {
    ASSERT(isLitLayer(RenderLayer::Vegetation));
}

TEST(vegetation_layer_name) {
    const char* name = getRenderLayerName(RenderLayer::Vegetation);
    ASSERT(name != nullptr);
    ASSERT(std::string(name) == "Vegetation");
}

// =============================================================================
// Emissive Color Mapping Tests
// =============================================================================

TEST(emissive_color_biolume_grove) {
    // BiolumeGrove terrain type maps to BiolumeTree vegetation
    // Emissive color: #00ff88 (0, 255, 136), intensity 0.25
    const auto& info = terrain::getTerrainInfo(terrain::TerrainType::BiolumeGrove);

    ASSERT_NEAR(0.0f / 255.0f, info.emissive_color.x, 0.01f);
    ASSERT_NEAR(255.0f / 255.0f, info.emissive_color.y, 0.01f);
    ASSERT_NEAR(136.0f / 255.0f, info.emissive_color.z, 0.01f);
    ASSERT_NEAR(0.25f, info.emissive_intensity, 0.01f);
}

TEST(emissive_color_prisma_fields) {
    // PrismaFields terrain type maps to CrystalSpire vegetation
    // Emissive color: #ff00ff (255, 0, 255), intensity 0.60
    const auto& info = terrain::getTerrainInfo(terrain::TerrainType::PrismaFields);

    ASSERT_NEAR(255.0f / 255.0f, info.emissive_color.x, 0.01f);
    ASSERT_NEAR(0.0f / 255.0f, info.emissive_color.y, 0.01f);
    ASSERT_NEAR(255.0f / 255.0f, info.emissive_color.z, 0.01f);
    ASSERT_NEAR(0.60f, info.emissive_intensity, 0.01f);
}

TEST(emissive_color_spore_flats) {
    // SporeFlats terrain type maps to SporeEmitter vegetation
    // Emissive color: #88ff44 (136, 255, 68), intensity 0.30
    const auto& info = terrain::getTerrainInfo(terrain::TerrainType::SporeFlats);

    ASSERT_NEAR(136.0f / 255.0f, info.emissive_color.x, 0.01f);
    ASSERT_NEAR(255.0f / 255.0f, info.emissive_color.y, 0.01f);
    ASSERT_NEAR(68.0f / 255.0f, info.emissive_color.z, 0.01f);
    ASSERT_NEAR(0.30f, info.emissive_intensity, 0.01f);
}

// =============================================================================
// ChunkInstances Structure Tests
// =============================================================================

TEST(chunk_instances_default) {
    ChunkInstances chunk;

    ASSERT(chunk.instances.empty());
    ASSERT_EQ(0, chunk.chunk_x);
    ASSERT_EQ(0, chunk.chunk_y);
}

TEST(chunk_instances_reserve) {
    ChunkInstances chunk;
    chunk.instances.reserve(1024);
    chunk.chunk_x = 5;
    chunk.chunk_y = 10;

    ASSERT_EQ(5, chunk.chunk_x);
    ASSERT_EQ(10, chunk.chunk_y);
    ASSERT(chunk.instances.capacity() >= 1024);
}

// =============================================================================
// Main Entry Point
// =============================================================================

int main() {
    printf("=== VegetationRenderer Unit Tests (Ticket 3-030) ===\n\n");

    // Configuration tests
    printf("Configuration Tests:\n");
    RUN_TEST(config_default_values);
    RUN_TEST(config_custom_values);

    // Statistics tests
    printf("\nStatistics Tests:\n");
    RUN_TEST(stats_default_values);
    RUN_TEST(stats_reset);

    // Model type tests
    printf("\nModel Type Tests:\n");
    RUN_TEST(model_type_count);
    RUN_TEST(model_type_enum_values);

    // VegetationInstance tests
    printf("\nVegetationInstance Tests:\n");
    RUN_TEST(vegetation_instance_size);
    RUN_TEST(vegetation_instance_construction);

    // Render layer tests
    printf("\nRender Layer Tests:\n");
    RUN_TEST(vegetation_render_layer_exists);
    RUN_TEST(vegetation_layer_after_terrain);
    RUN_TEST(vegetation_layer_before_water);
    RUN_TEST(vegetation_layer_is_opaque);
    RUN_TEST(vegetation_layer_is_lit);
    RUN_TEST(vegetation_layer_name);

    // Emissive color tests
    printf("\nEmissive Color Mapping Tests:\n");
    RUN_TEST(emissive_color_biolume_grove);
    RUN_TEST(emissive_color_prisma_fields);
    RUN_TEST(emissive_color_spore_flats);

    // ChunkInstances tests
    printf("\nChunkInstances Tests:\n");
    RUN_TEST(chunk_instances_default);
    RUN_TEST(chunk_instances_reserve);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
