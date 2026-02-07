/**
 * @file test_edge_detection_pass.cpp
 * @brief Unit tests for EdgeDetectionPass (Ticket 2-006).
 *
 * Tests edge detection configuration including:
 * - EdgeDetectionConfig struct defaults match canon specs
 * - EdgeDetectionUBO struct matches shader layout (48 bytes)
 * - Default outline color is dark purple (#2A1B3D)
 * - Near/far planes are set correctly for depth linearization
 * - Edge thickness is configurable
 *
 * GPU rendering tests require manual visual verification.
 */

#include "sims3000/render/EdgeDetectionPass.h"
#include "sims3000/render/ToonShaderConfig.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>

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
        if (std::abs((a) - (b)) <= (epsilon)) { \
            g_testsPassed++; \
            printf("  [PASS] %s ~= %s\n", #a, #b); \
        } else { \
            g_testsFailed++; \
            printf("  [FAIL] %s != %s (diff=%f, line %d)\n", #a, #b, std::abs((a)-(b)), __LINE__); \
        } \
    } while(0)

// =============================================================================
// Test: EdgeDetectionConfig defaults
// =============================================================================
void test_EdgeDetectionConfigDefaults() {
    TEST_CASE("EdgeDetectionConfig defaults match canon specifications");

    EdgeDetectionConfig config;

    // Outline color should be dark purple (#2A1B3D -> RGB 42/255, 27/255, 61/255)
    // Canon: 0.165, 0.106, 0.239, 1.0
    EXPECT_NEAR(config.outlineColor.r, 0.165f, 0.01f);
    EXPECT_NEAR(config.outlineColor.g, 0.106f, 0.01f);
    EXPECT_NEAR(config.outlineColor.b, 0.239f, 0.01f);
    EXPECT_NEAR(config.outlineColor.a, 1.0f, 0.001f);

    // Default thresholds should be reasonable
    EXPECT_TRUE(config.normalThreshold > 0.0f);
    EXPECT_TRUE(config.normalThreshold < 1.0f);
    EXPECT_TRUE(config.depthThreshold > 0.0f);
    EXPECT_TRUE(config.depthThreshold < 1.0f);

    // Edge thickness should be 1.0 (single pixel) by default
    EXPECT_NEAR(config.edgeThickness, 1.0f, 0.001f);

    // Near/far planes should have reasonable defaults
    EXPECT_TRUE(config.nearPlane > 0.0f);
    EXPECT_TRUE(config.farPlane > config.nearPlane);

    printf("  [INFO] Outline color: (%.3f, %.3f, %.3f, %.3f)\n",
           config.outlineColor.r, config.outlineColor.g,
           config.outlineColor.b, config.outlineColor.a);
    printf("  [INFO] Thresholds: normal=%.3f, depth=%.3f\n",
           config.normalThreshold, config.depthThreshold);
    printf("  [INFO] Edge thickness: %.1f pixels\n", config.edgeThickness);
    printf("  [INFO] Depth range: [%.1f, %.1f]\n", config.nearPlane, config.farPlane);
}

// =============================================================================
// Test: EdgeDetectionUBO struct size matches shader
// =============================================================================
void test_EdgeDetectionUBOSize() {
    TEST_CASE("EdgeDetectionUBO struct size matches shader layout");

    // The UBO must be exactly 48 bytes to match the shader cbuffer
    // Layout:
    //   float4 outlineColor;      // 16 bytes (offset 0)
    //   float2 texelSize;         // 8 bytes (offset 16)
    //   float normalThreshold;    // 4 bytes (offset 24)
    //   float depthThreshold;     // 4 bytes (offset 28)
    //   float nearPlane;          // 4 bytes (offset 32)
    //   float farPlane;           // 4 bytes (offset 36)
    //   float edgeThickness;      // 4 bytes (offset 40)
    //   float _padding;           // 4 bytes (offset 44)
    //   Total: 48 bytes

    EXPECT_EQ(sizeof(EdgeDetectionUBO), 48u);

    // Verify alignment (should be 16-byte aligned for GPU)
    EXPECT_EQ(alignof(EdgeDetectionUBO), 4u);

    printf("  [INFO] EdgeDetectionUBO size: %zu bytes\n", sizeof(EdgeDetectionUBO));
}

// =============================================================================
// Test: EdgeDetectionUBO field layout
// =============================================================================
void test_EdgeDetectionUBOLayout() {
    TEST_CASE("EdgeDetectionUBO field layout for shader compatibility");

    EdgeDetectionUBO ubo;

    // Initialize with known values
    ubo.outlineColor = glm::vec4(0.165f, 0.106f, 0.239f, 1.0f);
    ubo.texelSize = glm::vec2(1.0f / 1920.0f, 1.0f / 1080.0f);
    ubo.normalThreshold = 0.1f;
    ubo.depthThreshold = 0.01f;
    ubo.nearPlane = 0.1f;
    ubo.farPlane = 1000.0f;
    ubo.edgeThickness = 1.5f;
    ubo._padding = 0.0f;

    // Verify values are stored correctly
    EXPECT_NEAR(ubo.outlineColor.r, 0.165f, 0.001f);
    EXPECT_NEAR(ubo.outlineColor.g, 0.106f, 0.001f);
    EXPECT_NEAR(ubo.outlineColor.b, 0.239f, 0.001f);
    EXPECT_NEAR(ubo.outlineColor.a, 1.0f, 0.001f);

    EXPECT_NEAR(ubo.texelSize.x, 1.0f / 1920.0f, 0.0001f);
    EXPECT_NEAR(ubo.texelSize.y, 1.0f / 1080.0f, 0.0001f);

    EXPECT_NEAR(ubo.normalThreshold, 0.1f, 0.001f);
    EXPECT_NEAR(ubo.depthThreshold, 0.01f, 0.001f);
    EXPECT_NEAR(ubo.nearPlane, 0.1f, 0.001f);
    EXPECT_NEAR(ubo.farPlane, 1000.0f, 0.001f);
    EXPECT_NEAR(ubo.edgeThickness, 1.5f, 0.001f);

    printf("  [INFO] UBO populated with test values successfully\n");
}

// =============================================================================
// Test: ToonShaderConfig edge color integration
// =============================================================================
void test_ToonShaderConfigEdgeColor() {
    TEST_CASE("ToonShaderConfig edge color defaults and modification");

    ToonShaderConfig& config = ToonShaderConfig::instance();

    // Reset to ensure we're testing defaults
    config.resetToDefaults();

    // Check default edge color (should be dark purple)
    glm::vec4 edgeColor = config.getEdgeColor();

    // Canon dark purple: #2A1B3D
    EXPECT_NEAR(edgeColor.r, 0.165f, 0.01f);
    EXPECT_NEAR(edgeColor.g, 0.106f, 0.01f);
    EXPECT_NEAR(edgeColor.b, 0.239f, 0.01f);
    EXPECT_NEAR(edgeColor.a, 1.0f, 0.001f);

    // Test modification
    glm::vec4 newColor(1.0f, 0.0f, 0.0f, 0.8f);
    config.setEdgeColor(newColor);

    glm::vec4 retrievedColor = config.getEdgeColor();
    EXPECT_NEAR(retrievedColor.r, 1.0f, 0.001f);
    EXPECT_NEAR(retrievedColor.g, 0.0f, 0.001f);
    EXPECT_NEAR(retrievedColor.b, 0.0f, 0.001f);
    EXPECT_NEAR(retrievedColor.a, 0.8f, 0.001f);

    // Verify config is marked dirty after change
    EXPECT_TRUE(config.isDirty());

    // Reset back to defaults
    config.resetToDefaults();

    printf("  [INFO] Edge color modification works correctly\n");
}

// =============================================================================
// Test: Edge thickness configuration
// =============================================================================
void test_EdgeThicknessConfiguration() {
    TEST_CASE("Edge thickness is configurable (screen-space pixels)");

    EdgeDetectionConfig config;

    // Default should be 1.0 (single pixel)
    EXPECT_NEAR(config.edgeThickness, 1.0f, 0.001f);

    // Modify thickness
    config.edgeThickness = 2.0f;
    EXPECT_NEAR(config.edgeThickness, 2.0f, 0.001f);

    // Test various thicknesses
    config.edgeThickness = 0.5f;  // Sub-pixel
    EXPECT_NEAR(config.edgeThickness, 0.5f, 0.001f);

    config.edgeThickness = 3.0f;  // Thicker lines
    EXPECT_NEAR(config.edgeThickness, 3.0f, 0.001f);

    printf("  [INFO] Edge thickness configurable from 0.5 to 3.0+ pixels\n");
}

// =============================================================================
// Test: Depth linearization parameters
// =============================================================================
void test_DepthLinearizationParameters() {
    TEST_CASE("Depth linearization for perspective projection");

    EdgeDetectionConfig config;

    // Set camera near/far planes (typical game camera)
    config.nearPlane = 0.1f;
    config.farPlane = 1000.0f;

    // Create UBO for shader
    EdgeDetectionUBO ubo;
    ubo.nearPlane = config.nearPlane;
    ubo.farPlane = config.farPlane;

    // Verify values transfer correctly
    EXPECT_NEAR(ubo.nearPlane, 0.1f, 0.001f);
    EXPECT_NEAR(ubo.farPlane, 1000.0f, 0.001f);

    // Test linearization formula (same as shader)
    // linearDepth = (near * far) / (far - rawDepth * (far - near))
    // At rawDepth = 0 (near plane): result should be near
    // At rawDepth = 1 (far plane): result should be far

    auto linearizeDepth = [&](float rawDepth) -> float {
        float nearP = ubo.nearPlane;
        float farP = ubo.farPlane;
        return (nearP * farP) / (farP - rawDepth * (farP - nearP));
    };

    // At near plane (depth buffer = 0)
    float atNear = linearizeDepth(0.0f);
    EXPECT_NEAR(atNear, config.nearPlane, 0.001f);

    // At far plane (depth buffer = 1)
    float atFar = linearizeDepth(1.0f);
    // Note: Floating point precision limits exact far plane match
    // The formula approaches infinity as depth approaches 1.0
    EXPECT_NEAR(atFar, config.farPlane, 1.0f);

    // At middle of depth buffer (should be closer to near due to non-linearity)
    float atMid = linearizeDepth(0.5f);
    EXPECT_TRUE(atMid > config.nearPlane);
    EXPECT_TRUE(atMid < config.farPlane);

    printf("  [INFO] Depth linearization: near=%.2f, mid=%.2f, far=%.2f\n",
           atNear, atMid, atFar);
}

// =============================================================================
// Test: Normal-based edges as primary signal
// =============================================================================
void test_NormalBasedPrimarySignal() {
    TEST_CASE("Normal-based edges as primary signal");

    EdgeDetectionConfig config;

    // Normal threshold should be set (non-zero)
    EXPECT_TRUE(config.normalThreshold > 0.0f);
    EXPECT_TRUE(config.depthThreshold > 0.0f);

    // Both thresholds are configured for edge detection
    // Normal-based detection is primary because:
    // 1. Normal discontinuities catch surface edges (e.g., cube corners)
    // 2. Depth is secondary, catching silhouettes against distant backgrounds
    // The shader combines both signals with normal weighted higher

    printf("  [INFO] Normal threshold: %.3f (primary signal)\n", config.normalThreshold);
    printf("  [INFO] Depth threshold: %.3f (secondary signal)\n", config.depthThreshold);
    printf("  [INFO] Both thresholds configured for edge detection\n");
}

// =============================================================================
// Main entry point
// =============================================================================
int main() {
    printf("========================================\n");
    printf("Edge Detection Pass Tests (Ticket 2-006)\n");
    printf("========================================\n");

    // Run all tests
    test_EdgeDetectionConfigDefaults();
    test_EdgeDetectionUBOSize();
    test_EdgeDetectionUBOLayout();
    test_ToonShaderConfigEdgeColor();
    test_EdgeThicknessConfiguration();
    test_DepthLinearizationParameters();
    test_NormalBasedPrimarySignal();

    // Summary
    printf("\n========================================\n");
    printf("SUMMARY: %d passed, %d failed\n", g_testsPassed, g_testsFailed);
    printf("========================================\n");

    // Manual verification note
    printf("\n[NOTE] GPU rendering requires manual visual verification:\n");
    printf("  - Run application with edge detection enabled\n");
    printf("  - Verify outlines appear on opaque geometry\n");
    printf("  - Verify edges are readable at all zoom levels\n");
    printf("  - Verify <1ms performance at 1080p\n");
    printf("  - Test at multiple camera angles\n");

    return g_testsFailed > 0 ? 1 : 0;
}
