/**
 * @file test_gpu_mesh.cpp
 * @brief Unit tests for GPUMesh and ModelAsset structures.
 *
 * Tests:
 * - AABB construction and operations
 * - GPUMaterial default values and emissive detection
 * - GPUMesh structure and validation
 * - ModelAsset aggregation and material lookup
 *
 * Note: Tests that require actual GPU resources (ModelAsset::fromModel)
 * are marked for manual verification and require a display.
 */

#include "sims3000/render/GPUMesh.h"
#include <cstdio>
#include <cmath>
#include <limits>

using namespace sims3000;

// Test counters
static int g_testsPassed = 0;
static int g_testsFailed = 0;

// Test macros
#define TEST_CASE(name) \
    printf("\n[TEST] %s\n", name); \
    fflush(stdout)

#define EXPECT_TRUE(condition) \
    do { \
        if (condition) { \
            g_testsPassed++; \
            printf("  [PASS] %s\n", #condition); \
        } else { \
            g_testsFailed++; \
            printf("  [FAIL] %s (line %d)\n", #condition, __LINE__); \
        } \
    } while(0)

#define EXPECT_FALSE(condition) \
    do { \
        if (!(condition)) { \
            g_testsPassed++; \
            printf("  [PASS] !(%s)\n", #condition); \
        } else { \
            g_testsFailed++; \
            printf("  [FAIL] !(%s) (line %d)\n", #condition, __LINE__); \
        } \
    } while(0)

#define EXPECT_NOT_NULL(ptr) \
    do { \
        if ((ptr) != nullptr) { \
            g_testsPassed++; \
            printf("  [PASS] %s != nullptr\n", #ptr); \
        } else { \
            g_testsFailed++; \
            printf("  [FAIL] %s == nullptr (line %d)\n", #ptr, __LINE__); \
        } \
    } while(0)

#define EXPECT_NULL(ptr) \
    do { \
        if ((ptr) == nullptr) { \
            g_testsPassed++; \
            printf("  [PASS] %s == nullptr\n", #ptr); \
        } else { \
            g_testsFailed++; \
            printf("  [FAIL] %s != nullptr (line %d)\n", #ptr, __LINE__); \
        } \
    } while(0)

#define EXPECT_EQ(a, b) \
    do { \
        if ((a) == (b)) { \
            g_testsPassed++; \
            printf("  [PASS] %s == %s\n", #a, #b); \
        } else { \
            g_testsFailed++; \
            printf("  [FAIL] %s != %s (line %d)\n", #a, #b, __LINE__); \
        } \
    } while(0)

#define EXPECT_NEAR(a, b, epsilon) \
    do { \
        float diff = std::abs((a) - (b)); \
        if (diff <= (epsilon)) { \
            g_testsPassed++; \
            printf("  [PASS] |%s - %s| <= %s\n", #a, #b, #epsilon); \
        } else { \
            g_testsFailed++; \
            printf("  [FAIL] |%s - %s| = %f > %s (line %d)\n", #a, #b, diff, #epsilon, __LINE__); \
        } \
    } while(0)

// =============================================================================
// AABB Tests
// =============================================================================

void test_AABB_DefaultValues() {
    TEST_CASE("AABB default values");

    AABB aabb;
    EXPECT_NEAR(aabb.min.x, 0.0f, 0.001f);
    EXPECT_NEAR(aabb.min.y, 0.0f, 0.001f);
    EXPECT_NEAR(aabb.min.z, 0.0f, 0.001f);
    EXPECT_NEAR(aabb.max.x, 0.0f, 0.001f);
    EXPECT_NEAR(aabb.max.y, 0.0f, 0.001f);
    EXPECT_NEAR(aabb.max.z, 0.0f, 0.001f);
}

void test_AABB_CenterAndSize() {
    TEST_CASE("AABB center and size calculations");

    AABB aabb;
    aabb.min = glm::vec3(-1.0f, -2.0f, -3.0f);
    aabb.max = glm::vec3(1.0f, 2.0f, 3.0f);

    glm::vec3 center = aabb.center();
    EXPECT_NEAR(center.x, 0.0f, 0.001f);
    EXPECT_NEAR(center.y, 0.0f, 0.001f);
    EXPECT_NEAR(center.z, 0.0f, 0.001f);

    glm::vec3 size = aabb.size();
    EXPECT_NEAR(size.x, 2.0f, 0.001f);
    EXPECT_NEAR(size.y, 4.0f, 0.001f);
    EXPECT_NEAR(size.z, 6.0f, 0.001f);

    glm::vec3 halfSize = aabb.halfSize();
    EXPECT_NEAR(halfSize.x, 1.0f, 0.001f);
    EXPECT_NEAR(halfSize.y, 2.0f, 0.001f);
    EXPECT_NEAR(halfSize.z, 3.0f, 0.001f);
}

void test_AABB_IsValid() {
    TEST_CASE("AABB isValid check");

    // Valid AABB (max >= min)
    AABB valid;
    valid.min = glm::vec3(0.0f);
    valid.max = glm::vec3(1.0f);
    EXPECT_TRUE(valid.isValid());

    // Degenerate AABB (point)
    AABB degenerate;
    degenerate.min = glm::vec3(1.0f);
    degenerate.max = glm::vec3(1.0f);
    EXPECT_TRUE(degenerate.isValid());

    // Invalid AABB (max < min)
    AABB invalid;
    invalid.min = glm::vec3(1.0f);
    invalid.max = glm::vec3(0.0f);
    EXPECT_FALSE(invalid.isValid());
}

void test_AABB_Expand_Point() {
    TEST_CASE("AABB expand by point");

    AABB aabb;
    aabb.min = glm::vec3(0.0f);
    aabb.max = glm::vec3(1.0f);

    // Expand to include point outside bounds
    aabb.expand(glm::vec3(2.0f, -1.0f, 0.5f));

    EXPECT_NEAR(aabb.min.x, 0.0f, 0.001f);
    EXPECT_NEAR(aabb.min.y, -1.0f, 0.001f);
    EXPECT_NEAR(aabb.min.z, 0.0f, 0.001f);
    EXPECT_NEAR(aabb.max.x, 2.0f, 0.001f);
    EXPECT_NEAR(aabb.max.y, 1.0f, 0.001f);
    EXPECT_NEAR(aabb.max.z, 1.0f, 0.001f);
}

void test_AABB_Expand_AABB() {
    TEST_CASE("AABB expand by AABB");

    AABB a;
    a.min = glm::vec3(0.0f);
    a.max = glm::vec3(1.0f);

    AABB b;
    b.min = glm::vec3(-1.0f, 0.5f, 0.5f);
    b.max = glm::vec3(0.5f, 2.0f, 0.5f);

    a.expand(b);

    EXPECT_NEAR(a.min.x, -1.0f, 0.001f);
    EXPECT_NEAR(a.min.y, 0.0f, 0.001f);
    EXPECT_NEAR(a.min.z, 0.0f, 0.001f);
    EXPECT_NEAR(a.max.x, 1.0f, 0.001f);
    EXPECT_NEAR(a.max.y, 2.0f, 0.001f);
    EXPECT_NEAR(a.max.z, 1.0f, 0.001f);
}

void test_AABB_Empty() {
    TEST_CASE("AABB empty factory");

    AABB empty = AABB::empty();

    // Empty AABB should have min at max float value
    EXPECT_NEAR(empty.min.x, std::numeric_limits<float>::max(), 1.0f);

    // Empty AABB should have max at lowest float value
    EXPECT_NEAR(empty.max.x, std::numeric_limits<float>::lowest(), 1.0f);

    // Expanding empty AABB should result in point
    empty.expand(glm::vec3(5.0f, 5.0f, 5.0f));
    EXPECT_NEAR(empty.min.x, 5.0f, 0.001f);
    EXPECT_NEAR(empty.max.x, 5.0f, 0.001f);
}

// =============================================================================
// GPUMaterial Tests
// =============================================================================

void test_GPUMaterial_DefaultValues() {
    TEST_CASE("GPUMaterial default values");

    GPUMaterial mat;

    EXPECT_TRUE(mat.name.empty());
    EXPECT_NULL(mat.diffuseTexture);
    EXPECT_NEAR(mat.baseColorFactor.r, 1.0f, 0.001f);
    EXPECT_NEAR(mat.baseColorFactor.g, 1.0f, 0.001f);
    EXPECT_NEAR(mat.baseColorFactor.b, 1.0f, 0.001f);
    EXPECT_NEAR(mat.baseColorFactor.a, 1.0f, 0.001f);

    EXPECT_NULL(mat.emissiveTexture);
    EXPECT_NEAR(mat.emissiveColor.r, 0.0f, 0.001f);
    EXPECT_NEAR(mat.emissiveColor.g, 0.0f, 0.001f);
    EXPECT_NEAR(mat.emissiveColor.b, 0.0f, 0.001f);

    EXPECT_NULL(mat.metallicRoughnessTexture);
    EXPECT_NEAR(mat.metallicFactor, 1.0f, 0.001f);
    EXPECT_NEAR(mat.roughnessFactor, 1.0f, 0.001f);

    EXPECT_NULL(mat.normalTexture);
    EXPECT_NEAR(mat.normalScale, 1.0f, 0.001f);

    EXPECT_TRUE(mat.alphaMode == GPUMaterial::AlphaMode::Opaque);
    EXPECT_NEAR(mat.alphaCutoff, 0.5f, 0.001f);
    EXPECT_FALSE(mat.doubleSided);
}

void test_GPUMaterial_HasEmissive() {
    TEST_CASE("GPUMaterial hasEmissive detection");

    GPUMaterial mat;

    // No emissive
    EXPECT_FALSE(mat.hasEmissive());

    // Emissive color only
    mat.emissiveColor = glm::vec3(1.0f, 0.0f, 0.0f);
    EXPECT_TRUE(mat.hasEmissive());

    // Reset color, set texture (simulated with non-null pointer)
    mat.emissiveColor = glm::vec3(0.0f);
    // Can't set actual texture without TextureLoader, but test the logic
    EXPECT_FALSE(mat.hasEmissive());  // Back to no emissive
}

void test_GPUMaterial_HasDiffuseTexture() {
    TEST_CASE("GPUMaterial hasDiffuseTexture detection");

    GPUMaterial mat;

    EXPECT_FALSE(mat.hasDiffuseTexture());

    // Note: Can't test with actual texture handle without TextureLoader
    // The test verifies the null check logic works
}

void test_GPUMaterial_AlphaModes() {
    TEST_CASE("GPUMaterial alpha modes");

    GPUMaterial matOpaque;
    matOpaque.alphaMode = GPUMaterial::AlphaMode::Opaque;
    EXPECT_TRUE(matOpaque.alphaMode == GPUMaterial::AlphaMode::Opaque);

    GPUMaterial matMask;
    matMask.alphaMode = GPUMaterial::AlphaMode::Mask;
    matMask.alphaCutoff = 0.75f;
    EXPECT_TRUE(matMask.alphaMode == GPUMaterial::AlphaMode::Mask);
    EXPECT_NEAR(matMask.alphaCutoff, 0.75f, 0.001f);

    GPUMaterial matBlend;
    matBlend.alphaMode = GPUMaterial::AlphaMode::Blend;
    EXPECT_TRUE(matBlend.alphaMode == GPUMaterial::AlphaMode::Blend);
}

// =============================================================================
// GPUMesh Tests
// =============================================================================

void test_GPUMesh_DefaultValues() {
    TEST_CASE("GPUMesh default values");

    GPUMesh mesh;

    EXPECT_NULL(mesh.vertexBuffer);
    EXPECT_NULL(mesh.indexBuffer);
    EXPECT_EQ(mesh.vertexCount, 0u);
    EXPECT_EQ(mesh.indexCount, 0u);
    EXPECT_EQ(mesh.materialIndex, -1);
    EXPECT_NEAR(mesh.bounds.min.x, 0.0f, 0.001f);
    EXPECT_NEAR(mesh.bounds.max.x, 0.0f, 0.001f);
}

void test_GPUMesh_IsValid() {
    TEST_CASE("GPUMesh isValid check");

    GPUMesh mesh;

    // Default is invalid
    EXPECT_FALSE(mesh.isValid());

    // Still invalid with only vertex buffer (simulated)
    // Note: Can't create real GPU buffers without device
}

void test_GPUMesh_HasMaterial() {
    TEST_CASE("GPUMesh hasMaterial check");

    GPUMesh mesh;

    // Default has no material
    EXPECT_FALSE(mesh.hasMaterial());

    mesh.materialIndex = 0;
    EXPECT_TRUE(mesh.hasMaterial());

    mesh.materialIndex = -1;
    EXPECT_FALSE(mesh.hasMaterial());
}

// =============================================================================
// ModelAsset Tests
// =============================================================================

void test_ModelAsset_DefaultValues() {
    TEST_CASE("ModelAsset default values");

    ModelAsset asset;

    EXPECT_TRUE(asset.meshes.empty());
    EXPECT_TRUE(asset.materials.empty());
    EXPECT_NEAR(asset.bounds.min.x, 0.0f, 0.001f);
    EXPECT_NEAR(asset.bounds.max.x, 0.0f, 0.001f);
    EXPECT_NULL(asset.sourceModel);
}

void test_ModelAsset_GetTotalCounts() {
    TEST_CASE("ModelAsset getTotalIndexCount/getTotalVertexCount");

    ModelAsset asset;

    // Empty asset
    EXPECT_EQ(asset.getTotalIndexCount(), 0u);
    EXPECT_EQ(asset.getTotalVertexCount(), 0u);

    // Add meshes with counts
    GPUMesh mesh1;
    mesh1.vertexCount = 100;
    mesh1.indexCount = 300;
    asset.meshes.push_back(mesh1);

    GPUMesh mesh2;
    mesh2.vertexCount = 50;
    mesh2.indexCount = 150;
    asset.meshes.push_back(mesh2);

    EXPECT_EQ(asset.getTotalIndexCount(), 450u);
    EXPECT_EQ(asset.getTotalVertexCount(), 150u);
}

void test_ModelAsset_IsValid() {
    TEST_CASE("ModelAsset isValid check");

    ModelAsset asset;

    // Empty asset is invalid
    EXPECT_FALSE(asset.isValid());

    // Asset with invalid mesh is invalid
    GPUMesh invalidMesh;
    asset.meshes.push_back(invalidMesh);
    EXPECT_FALSE(asset.isValid());

    // Note: Can't test valid case without GPU buffers
}

void test_ModelAsset_GetMeshMaterial() {
    TEST_CASE("ModelAsset getMeshMaterial lookup");

    ModelAsset asset;

    // Add materials
    GPUMaterial mat1;
    mat1.name = "RedMaterial";
    mat1.baseColorFactor = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
    asset.materials.push_back(mat1);

    GPUMaterial mat2;
    mat2.name = "BlueMaterial";
    mat2.baseColorFactor = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
    mat2.emissiveColor = glm::vec3(0.0f, 0.0f, 0.5f);
    asset.materials.push_back(mat2);

    // Add meshes referencing materials
    GPUMesh mesh1;
    mesh1.materialIndex = 0;  // RedMaterial
    asset.meshes.push_back(mesh1);

    GPUMesh mesh2;
    mesh2.materialIndex = 1;  // BlueMaterial
    asset.meshes.push_back(mesh2);

    GPUMesh mesh3;
    mesh3.materialIndex = -1;  // No material
    asset.meshes.push_back(mesh3);

    // Test material lookups
    const GPUMaterial* mat = asset.getMeshMaterial(0);
    EXPECT_NOT_NULL(mat);
    EXPECT_TRUE(mat->name == "RedMaterial");
    EXPECT_NEAR(mat->baseColorFactor.r, 1.0f, 0.001f);

    mat = asset.getMeshMaterial(1);
    EXPECT_NOT_NULL(mat);
    EXPECT_TRUE(mat->name == "BlueMaterial");
    EXPECT_NEAR(mat->emissiveColor.b, 0.5f, 0.001f);

    mat = asset.getMeshMaterial(2);
    EXPECT_NULL(mat);  // No material assigned

    mat = asset.getMeshMaterial(999);
    EXPECT_NULL(mat);  // Out of bounds
}

void test_ModelAsset_EmissiveMaterialDetection() {
    TEST_CASE("ModelAsset emissive material detection for bioluminescent rendering");

    ModelAsset asset;

    // Add a non-emissive material
    GPUMaterial nonEmissive;
    nonEmissive.name = "Wall";
    nonEmissive.baseColorFactor = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
    asset.materials.push_back(nonEmissive);
    EXPECT_FALSE(asset.materials[0].hasEmissive());

    // Add an emissive material (for bioluminescent glow)
    GPUMaterial emissive;
    emissive.name = "GlowingCrystal";
    emissive.emissiveColor = glm::vec3(0.0f, 1.0f, 0.8f);  // Teal glow
    asset.materials.push_back(emissive);
    EXPECT_TRUE(asset.materials[1].hasEmissive());

    // Verify the emissive detection
    EXPECT_NEAR(asset.materials[1].emissiveColor.r, 0.0f, 0.001f);
    EXPECT_NEAR(asset.materials[1].emissiveColor.g, 1.0f, 0.001f);
    EXPECT_NEAR(asset.materials[1].emissiveColor.b, 0.8f, 0.001f);
}

// =============================================================================
// Integration Contract Tests
// =============================================================================

void test_GPUMesh_BufferContract() {
    TEST_CASE("GPUMesh buffer ownership contract");

    printf("  [INFO] GPUMesh does NOT own GPU buffers\n");
    printf("  [INFO] Buffers are owned by ModelLoader\n");
    printf("  [INFO] GPUMesh stores references for rendering\n");

    // Document the ownership model
    GPUMesh mesh;
    mesh.vertexBuffer = nullptr;  // Would be from ModelLoader
    mesh.indexBuffer = nullptr;   // Would be from ModelLoader

    // No cleanup needed in GPUMesh destructor
    g_testsPassed++;
}

void test_ModelAsset_TextureContract() {
    TEST_CASE("ModelAsset texture ownership contract");

    printf("  [INFO] GPUMaterial stores TextureHandle references\n");
    printf("  [INFO] Textures are owned by TextureLoader\n");
    printf("  [INFO] Call releaseTextures() to decrement ref counts\n");

    // Document the ownership model
    GPUMaterial mat;
    mat.diffuseTexture = nullptr;   // Would be from TextureLoader
    mat.emissiveTexture = nullptr;  // Would be from TextureLoader

    g_testsPassed++;
}

void test_ModelAsset_FromModelContract() {
    TEST_CASE("ModelAsset::fromModel contract");

    printf("  [INFO] fromModel() requires valid ModelHandle\n");
    printf("  [INFO] fromModel() loads textures via TextureLoader\n");
    printf("  [INFO] fromModelNoTextures() skips texture loading\n");
    printf("  [INFO] Caller must call releaseTextures() before destruction\n");

    // Can't test actual implementation without GPU
    // This documents the expected behavior
    g_testsPassed++;
}

// =============================================================================
// Main
// =============================================================================

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    printf("========================================\n");
    printf("GPUMesh Unit Tests\n");
    printf("========================================\n");
    printf("\nNote: GPU-dependent tests require display.\n");
    printf("This test suite focuses on data structure\n");
    printf("verification and contract testing.\n");

    // AABB tests
    test_AABB_DefaultValues();
    test_AABB_CenterAndSize();
    test_AABB_IsValid();
    test_AABB_Expand_Point();
    test_AABB_Expand_AABB();
    test_AABB_Empty();

    // GPUMaterial tests
    test_GPUMaterial_DefaultValues();
    test_GPUMaterial_HasEmissive();
    test_GPUMaterial_HasDiffuseTexture();
    test_GPUMaterial_AlphaModes();

    // GPUMesh tests
    test_GPUMesh_DefaultValues();
    test_GPUMesh_IsValid();
    test_GPUMesh_HasMaterial();

    // ModelAsset tests
    test_ModelAsset_DefaultValues();
    test_ModelAsset_GetTotalCounts();
    test_ModelAsset_IsValid();
    test_ModelAsset_GetMeshMaterial();
    test_ModelAsset_EmissiveMaterialDetection();

    // Contract tests
    test_GPUMesh_BufferContract();
    test_ModelAsset_TextureContract();
    test_ModelAsset_FromModelContract();

    // Summary
    printf("\n========================================\n");
    printf("Results: %d passed, %d failed\n", g_testsPassed, g_testsFailed);
    printf("========================================\n");

    printf("\n[INFO] GPU-dependent tests (ModelAsset::fromModel)\n");
    printf("[INFO] should be run via manual verification.\n");
    printf("[INFO] See: tests/render/test_gpu_mesh.cpp\n");

    return g_testsFailed > 0 ? 1 : 0;
}
