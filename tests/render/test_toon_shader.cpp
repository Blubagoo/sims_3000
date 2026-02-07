/**
 * @file test_toon_shader.cpp
 * @brief Unit tests for toon shader data structures and configuration.
 *
 * Tests the ToonShader.h structures for:
 * - Correct memory layout and alignment
 * - Default value initialization
 * - Structure size assertions (must match HLSL cbuffer layout)
 * - Factory function behavior
 */

#include "sims3000/render/ToonShader.h"
#include <cstdio>
#include <cmath>
#include <cstring>

using namespace sims3000;

// Simple test framework
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

// =============================================================================
// Structure Size Tests
// =============================================================================

TEST(ViewProjectionUBO_Size) {
    // ViewProjection UBO must be exactly 128 bytes (two mat4: viewProjection + lightViewProjection)
    ASSERT_EQ(128, sizeof(ToonViewProjectionUBO));
}

TEST(InstanceData_Size) {
    // InstanceData must be 112 bytes:
    // - mat4 model: 64 bytes
    // - vec4 baseColor: 16 bytes
    // - vec4 emissiveColor: 16 bytes
    // - float ambientStrength: 4 bytes
    // - float _padding[3]: 12 bytes
    // Total: 112 bytes
    ASSERT_EQ(112, sizeof(ToonInstanceData));
}

TEST(LightingUBO_Size) {
    // LightingUBO must be exactly 80 bytes:
    // - vec3 sunDirection + float globalAmbient: 16 bytes
    // - vec3 ambientColor + float shadowEnabled: 16 bytes
    // - vec3 deepShadowColor + float shadowIntensity: 16 bytes
    // - vec3 shadowTintColor + float shadowBias: 16 bytes
    // - vec3 shadowColor + float shadowSoftness: 16 bytes
    // Total: 80 bytes
    ASSERT_EQ(80, sizeof(ToonLightingUBO));
}

// =============================================================================
// Default Value Tests
// =============================================================================

TEST(ViewProjectionUBO_DefaultValues) {
    ToonViewProjectionUBO ubo;

    // Default should be identity matrix
    ASSERT_FLOAT_EQ(1.0f, ubo.viewProjection[0][0], 0.0001f);
    ASSERT_FLOAT_EQ(0.0f, ubo.viewProjection[0][1], 0.0001f);
    ASSERT_FLOAT_EQ(0.0f, ubo.viewProjection[0][2], 0.0001f);
    ASSERT_FLOAT_EQ(0.0f, ubo.viewProjection[0][3], 0.0001f);

    ASSERT_FLOAT_EQ(1.0f, ubo.viewProjection[1][1], 0.0001f);
    ASSERT_FLOAT_EQ(1.0f, ubo.viewProjection[2][2], 0.0001f);
    ASSERT_FLOAT_EQ(1.0f, ubo.viewProjection[3][3], 0.0001f);
}

TEST(InstanceData_DefaultValues) {
    ToonInstanceData data;

    // Default model matrix is identity
    ASSERT_FLOAT_EQ(1.0f, data.model[0][0], 0.0001f);
    ASSERT_FLOAT_EQ(1.0f, data.model[1][1], 0.0001f);
    ASSERT_FLOAT_EQ(1.0f, data.model[2][2], 0.0001f);
    ASSERT_FLOAT_EQ(1.0f, data.model[3][3], 0.0001f);

    // Default base color is white with full alpha
    ASSERT_FLOAT_EQ(1.0f, data.baseColor.r, 0.0001f);
    ASSERT_FLOAT_EQ(1.0f, data.baseColor.g, 0.0001f);
    ASSERT_FLOAT_EQ(1.0f, data.baseColor.b, 0.0001f);
    ASSERT_FLOAT_EQ(1.0f, data.baseColor.a, 0.0001f);

    // Default emissive is black (no emission)
    ASSERT_FLOAT_EQ(0.0f, data.emissiveColor.r, 0.0001f);
    ASSERT_FLOAT_EQ(0.0f, data.emissiveColor.g, 0.0001f);
    ASSERT_FLOAT_EQ(0.0f, data.emissiveColor.b, 0.0001f);
    ASSERT_FLOAT_EQ(0.0f, data.emissiveColor.a, 0.0001f);

    // Default ambient override is 0 (use global)
    ASSERT_FLOAT_EQ(0.0f, data.ambientStrength, 0.0001f);
}

TEST(LightingUBO_DefaultValues) {
    ToonLightingUBO ubo;

    // Default sun direction is normalized (1, 2, 1)
    float expectedMagnitude = std::sqrt(1.0f + 4.0f + 1.0f);  // ~2.449
    ASSERT_FLOAT_EQ(1.0f / expectedMagnitude, ubo.sunDirection.x, 0.001f);
    ASSERT_FLOAT_EQ(2.0f / expectedMagnitude, ubo.sunDirection.y, 0.001f);
    ASSERT_FLOAT_EQ(1.0f / expectedMagnitude, ubo.sunDirection.z, 0.001f);

    // Default global ambient
    ASSERT_FLOAT_EQ(0.08f, ubo.globalAmbient, 0.0001f);

    // Default ambient color (cool blue)
    ASSERT_FLOAT_EQ(0.6f, ubo.ambientColor.r, 0.0001f);
    ASSERT_FLOAT_EQ(0.65f, ubo.ambientColor.g, 0.0001f);
    ASSERT_FLOAT_EQ(0.8f, ubo.ambientColor.b, 0.0001f);

    // Deep shadow color (#2A1B3D)
    ASSERT_FLOAT_EQ(42.0f / 255.0f, ubo.deepShadowColor.r, 0.001f);
    ASSERT_FLOAT_EQ(27.0f / 255.0f, ubo.deepShadowColor.g, 0.001f);
    ASSERT_FLOAT_EQ(61.0f / 255.0f, ubo.deepShadowColor.b, 0.001f);

    // Shadow tint color (teal)
    ASSERT_FLOAT_EQ(0.1f, ubo.shadowTintColor.r, 0.0001f);
    ASSERT_FLOAT_EQ(0.2f, ubo.shadowTintColor.g, 0.0001f);
    ASSERT_FLOAT_EQ(0.25f, ubo.shadowTintColor.b, 0.0001f);
}

// =============================================================================
// Factory Function Tests
// =============================================================================

TEST(createDefaultLightingUBO) {
    ToonLightingUBO ubo = createDefaultLightingUBO();

    // Verify sun direction is normalized
    float magnitude = std::sqrt(
        ubo.sunDirection.x * ubo.sunDirection.x +
        ubo.sunDirection.y * ubo.sunDirection.y +
        ubo.sunDirection.z * ubo.sunDirection.z
    );
    ASSERT_FLOAT_EQ(1.0f, magnitude, 0.001f);

    // Verify ambient is in valid range
    ASSERT_TRUE(ubo.globalAmbient >= ToonShaderDefaults::MIN_AMBIENT);
    ASSERT_TRUE(ubo.globalAmbient <= ToonShaderDefaults::MAX_AMBIENT);
}

TEST(createInstanceData_WithDefaults) {
    ToonInstanceData data = createInstanceData();

    // Should have identity model matrix
    ASSERT_FLOAT_EQ(1.0f, data.model[0][0], 0.0001f);

    // Should have white base color
    ASSERT_FLOAT_EQ(1.0f, data.baseColor.r, 0.0001f);

    // Should have no emissive
    ASSERT_FLOAT_EQ(0.0f, data.emissiveColor.a, 0.0001f);

    // Should use global ambient
    ASSERT_FLOAT_EQ(0.0f, data.ambientStrength, 0.0001f);
}

TEST(createInstanceData_WithCustomValues) {
    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(10.0f, 20.0f, 30.0f));
    glm::vec4 baseColor(1.0f, 0.0f, 0.0f, 1.0f);  // Red
    glm::vec4 emissiveColor(0.0f, 1.0f, 0.0f, 0.5f);  // Green with 0.5 intensity
    float ambientOverride = 0.15f;

    ToonInstanceData data = createInstanceData(model, baseColor, emissiveColor, ambientOverride);

    // Verify translation
    ASSERT_FLOAT_EQ(10.0f, data.model[3][0], 0.0001f);
    ASSERT_FLOAT_EQ(20.0f, data.model[3][1], 0.0001f);
    ASSERT_FLOAT_EQ(30.0f, data.model[3][2], 0.0001f);

    // Verify colors
    ASSERT_FLOAT_EQ(1.0f, data.baseColor.r, 0.0001f);
    ASSERT_FLOAT_EQ(0.0f, data.baseColor.g, 0.0001f);
    ASSERT_FLOAT_EQ(1.0f, data.emissiveColor.g, 0.0001f);
    ASSERT_FLOAT_EQ(0.5f, data.emissiveColor.a, 0.0001f);

    // Verify ambient override
    ASSERT_FLOAT_EQ(0.15f, data.ambientStrength, 0.0001f);
}

// =============================================================================
// Constant Tests
// =============================================================================

TEST(LightingBandThresholds) {
    // Verify thresholds are in ascending order
    ASSERT_TRUE(ToonShaderDefaults::DEEP_SHADOW_THRESHOLD < ToonShaderDefaults::SHADOW_THRESHOLD);
    ASSERT_TRUE(ToonShaderDefaults::SHADOW_THRESHOLD < ToonShaderDefaults::MID_THRESHOLD);
    ASSERT_TRUE(ToonShaderDefaults::MID_THRESHOLD < 1.0f);

    // Verify expected values
    ASSERT_FLOAT_EQ(0.2f, ToonShaderDefaults::DEEP_SHADOW_THRESHOLD, 0.0001f);
    ASSERT_FLOAT_EQ(0.4f, ToonShaderDefaults::SHADOW_THRESHOLD, 0.0001f);
    ASSERT_FLOAT_EQ(0.7f, ToonShaderDefaults::MID_THRESHOLD, 0.0001f);
}

TEST(LightingBandIntensities) {
    // Verify intensities are in ascending order
    ASSERT_TRUE(ToonShaderDefaults::DEEP_SHADOW_INTENSITY < ToonShaderDefaults::SHADOW_INTENSITY);
    ASSERT_TRUE(ToonShaderDefaults::SHADOW_INTENSITY < ToonShaderDefaults::MID_INTENSITY);
    ASSERT_TRUE(ToonShaderDefaults::MID_INTENSITY < ToonShaderDefaults::LIT_INTENSITY);

    // Verify expected values
    ASSERT_FLOAT_EQ(0.15f, ToonShaderDefaults::DEEP_SHADOW_INTENSITY, 0.0001f);
    ASSERT_FLOAT_EQ(0.35f, ToonShaderDefaults::SHADOW_INTENSITY, 0.0001f);
    ASSERT_FLOAT_EQ(0.65f, ToonShaderDefaults::MID_INTENSITY, 0.0001f);
    ASSERT_FLOAT_EQ(1.0f, ToonShaderDefaults::LIT_INTENSITY, 0.0001f);
}

TEST(DeepShadowColorMatchesCanon) {
    // Canon specifies #2A1B3D for deep shadow
    // #2A = 42, #1B = 27, #3D = 61
    ASSERT_FLOAT_EQ(42.0f / 255.0f, ToonShaderDefaults::DEEP_SHADOW_R, 0.001f);
    ASSERT_FLOAT_EQ(27.0f / 255.0f, ToonShaderDefaults::DEEP_SHADOW_G, 0.001f);
    ASSERT_FLOAT_EQ(61.0f / 255.0f, ToonShaderDefaults::DEEP_SHADOW_B, 0.001f);
}

TEST(SunDirectionIsNormalized) {
    float magnitude = std::sqrt(
        ToonShaderDefaults::SUN_DIR_X * ToonShaderDefaults::SUN_DIR_X +
        ToonShaderDefaults::SUN_DIR_Y * ToonShaderDefaults::SUN_DIR_Y +
        ToonShaderDefaults::SUN_DIR_Z * ToonShaderDefaults::SUN_DIR_Z
    );
    ASSERT_FLOAT_EQ(1.0f, magnitude, 0.001f);
}

// =============================================================================
// Resource Constant Tests
// =============================================================================

TEST(ShaderResourceCounts) {
    // Vertex shader resources
    ASSERT_EQ(1, ToonShaderResources::VERTEX_UNIFORM_BUFFERS);
    ASSERT_EQ(1, ToonShaderResources::VERTEX_STORAGE_BUFFERS);
    ASSERT_EQ(0, ToonShaderResources::VERTEX_SAMPLERS);
    ASSERT_EQ(0, ToonShaderResources::VERTEX_STORAGE_TEXTURES);

    // Fragment shader resources (1 sampler for shadow map)
    ASSERT_EQ(1, ToonShaderResources::FRAGMENT_UNIFORM_BUFFERS);
    ASSERT_EQ(0, ToonShaderResources::FRAGMENT_STORAGE_BUFFERS);
    ASSERT_EQ(1, ToonShaderResources::FRAGMENT_SAMPLERS);  // Shadow map comparison sampler
    ASSERT_EQ(0, ToonShaderResources::FRAGMENT_STORAGE_TEXTURES);
}

// =============================================================================
// Memory Layout Tests (GPU alignment verification)
// =============================================================================

TEST(InstanceData_FieldOffsets) {
    // Verify field offsets match expected HLSL layout
    ToonInstanceData data;

    // model at offset 0
    ASSERT_EQ(0, (char*)&data.model - (char*)&data);

    // baseColor at offset 64
    ASSERT_EQ(64, (char*)&data.baseColor - (char*)&data);

    // emissiveColor at offset 80
    ASSERT_EQ(80, (char*)&data.emissiveColor - (char*)&data);

    // ambientStrength at offset 96
    ASSERT_EQ(96, (char*)&data.ambientStrength - (char*)&data);
}

TEST(LightingUBO_FieldOffsets) {
    ToonLightingUBO ubo;

    // sunDirection at offset 0
    ASSERT_EQ(0, (char*)&ubo.sunDirection - (char*)&ubo);

    // globalAmbient at offset 12
    ASSERT_EQ(12, (char*)&ubo.globalAmbient - (char*)&ubo);

    // ambientColor at offset 16
    ASSERT_EQ(16, (char*)&ubo.ambientColor - (char*)&ubo);

    // shadowEnabled at offset 28
    ASSERT_EQ(28, (char*)&ubo.shadowEnabled - (char*)&ubo);

    // deepShadowColor at offset 32
    ASSERT_EQ(32, (char*)&ubo.deepShadowColor - (char*)&ubo);

    // shadowIntensity at offset 44
    ASSERT_EQ(44, (char*)&ubo.shadowIntensity - (char*)&ubo);

    // shadowTintColor at offset 48
    ASSERT_EQ(48, (char*)&ubo.shadowTintColor - (char*)&ubo);

    // shadowBias at offset 60
    ASSERT_EQ(60, (char*)&ubo.shadowBias - (char*)&ubo);

    // shadowColor at offset 64
    ASSERT_EQ(64, (char*)&ubo.shadowColor - (char*)&ubo);

    // shadowSoftness at offset 76
    ASSERT_EQ(76, (char*)&ubo.shadowSoftness - (char*)&ubo);
}

// =============================================================================
// Main
// =============================================================================

int main() {
    printf("=== Toon Shader Unit Tests ===\n\n");

    // Structure size tests
    RUN_TEST(ViewProjectionUBO_Size);
    RUN_TEST(InstanceData_Size);
    RUN_TEST(LightingUBO_Size);

    // Default value tests
    RUN_TEST(ViewProjectionUBO_DefaultValues);
    RUN_TEST(InstanceData_DefaultValues);
    RUN_TEST(LightingUBO_DefaultValues);

    // Factory function tests
    RUN_TEST(createDefaultLightingUBO);
    RUN_TEST(createInstanceData_WithDefaults);
    RUN_TEST(createInstanceData_WithCustomValues);

    // Constant tests
    RUN_TEST(LightingBandThresholds);
    RUN_TEST(LightingBandIntensities);
    RUN_TEST(DeepShadowColorMatchesCanon);
    RUN_TEST(SunDirectionIsNormalized);

    // Resource constant tests
    RUN_TEST(ShaderResourceCounts);

    // Memory layout tests
    RUN_TEST(InstanceData_FieldOffsets);
    RUN_TEST(LightingUBO_FieldOffsets);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", g_testsPassed);
    printf("Failed: %d\n", g_testsFailed);

    return g_testsFailed == 0 ? 0 : 1;
}
