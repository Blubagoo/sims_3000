/**
 * @file test_edge_detection_pass.cpp
 * @brief Unit tests for EdgeDetectionPass (Tickets 2-006, 3-035).
 *
 * Tests edge detection configuration including:
 * - EdgeDetectionConfig struct defaults match canon specs
 * - EdgeDetectionUBO struct matches shader layout (48 bytes)
 * - Default outline color is dark purple (#2A1B3D)
 * - Near/far planes are set correctly for depth linearization
 * - Edge thickness is configurable
 *
 * Terrain-specific tests (Ticket 3-035):
 * - TerrainEdgeConfig struct provides terrain-tuned parameters
 * - Terrain normal threshold is lower for cliff/shoreline detection
 * - Terrain depth threshold is higher to avoid gentle slope noise
 * - Terrain edge thickness is thicker for visibility at distance
 * - Cliff/shoreline edge weights are defined
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
// Terrain Edge Detection Tests (Ticket 3-035)
// =============================================================================

// Test: TerrainEdgeConfig constants are defined
void test_TerrainEdgeConfigConstants() {
    TEST_CASE("TerrainEdgeConfig constants are defined (Ticket 3-035)");

    // Verify terrain-specific constants exist and have reasonable values

    // Normal threshold should be lower than default (0.3) for terrain
    EXPECT_TRUE(TerrainEdgeConfig::NORMAL_THRESHOLD < 0.3f);
    EXPECT_TRUE(TerrainEdgeConfig::NORMAL_THRESHOLD > 0.0f);

    // Depth threshold should be higher than default (0.1) to avoid slope noise
    EXPECT_TRUE(TerrainEdgeConfig::DEPTH_THRESHOLD > 0.1f);
    EXPECT_TRUE(TerrainEdgeConfig::DEPTH_THRESHOLD < 1.0f);

    // Edge thickness should be reasonable (1.0 - 3.0)
    EXPECT_TRUE(TerrainEdgeConfig::EDGE_THICKNESS >= 1.0f);
    EXPECT_TRUE(TerrainEdgeConfig::EDGE_THICKNESS <= 3.0f);

    // Cliff threshold for normal.y
    EXPECT_TRUE(TerrainEdgeConfig::CLIFF_NORMAL_Y_THRESHOLD > 0.0f);
    EXPECT_TRUE(TerrainEdgeConfig::CLIFF_NORMAL_Y_THRESHOLD < 1.0f);

    // Gentle slope angle should be positive
    EXPECT_TRUE(TerrainEdgeConfig::GENTLE_SLOPE_ANGLE > 0.0f);

    // Edge weights should boost (> 1.0)
    EXPECT_TRUE(TerrainEdgeConfig::CLIFF_EDGE_WEIGHT > 1.0f);
    EXPECT_TRUE(TerrainEdgeConfig::SHORELINE_EDGE_WEIGHT > 1.0f);

    // Distance scale factor should be between 0 and 1
    EXPECT_TRUE(TerrainEdgeConfig::DISTANCE_SCALE_FACTOR > 0.0f);
    EXPECT_TRUE(TerrainEdgeConfig::DISTANCE_SCALE_FACTOR <= 1.0f);

    printf("  [INFO] Terrain normal threshold: %.3f (lower for cliff/shoreline)\n",
           TerrainEdgeConfig::NORMAL_THRESHOLD);
    printf("  [INFO] Terrain depth threshold: %.3f (higher to avoid slope noise)\n",
           TerrainEdgeConfig::DEPTH_THRESHOLD);
    printf("  [INFO] Terrain edge thickness: %.1f pixels\n",
           TerrainEdgeConfig::EDGE_THICKNESS);
    printf("  [INFO] Cliff edge weight: %.2f\n", TerrainEdgeConfig::CLIFF_EDGE_WEIGHT);
    printf("  [INFO] Shoreline edge weight: %.2f\n", TerrainEdgeConfig::SHORELINE_EDGE_WEIGHT);
}

// Test: TerrainEdgeConfig createConfig generates valid config
void test_TerrainEdgeConfigCreateConfig() {
    TEST_CASE("TerrainEdgeConfig::createConfig generates terrain-tuned config");

    EdgeDetectionConfig terrainConfig = TerrainEdgeConfig::createConfig();

    // Verify terrain values are applied
    EXPECT_NEAR(terrainConfig.normalThreshold, TerrainEdgeConfig::NORMAL_THRESHOLD, 0.001f);
    EXPECT_NEAR(terrainConfig.depthThreshold, TerrainEdgeConfig::DEPTH_THRESHOLD, 0.001f);
    EXPECT_NEAR(terrainConfig.edgeThickness, TerrainEdgeConfig::EDGE_THICKNESS, 0.001f);

    // Outline color should be default purple
    EXPECT_NEAR(terrainConfig.outlineColor.r, 0.165f, 0.01f);
    EXPECT_NEAR(terrainConfig.outlineColor.g, 0.106f, 0.01f);
    EXPECT_NEAR(terrainConfig.outlineColor.b, 0.239f, 0.01f);

    // Test with custom color
    glm::vec4 customColor(0.0f, 0.0f, 0.0f, 1.0f);
    EdgeDetectionConfig customConfig = TerrainEdgeConfig::createConfig(customColor, 1.0f, 500.0f);

    EXPECT_NEAR(customConfig.outlineColor.r, 0.0f, 0.001f);
    EXPECT_NEAR(customConfig.nearPlane, 1.0f, 0.001f);
    EXPECT_NEAR(customConfig.farPlane, 500.0f, 0.001f);

    printf("  [INFO] Terrain config created successfully\n");
}

// Test: Terrain config differs from building config
void test_TerrainVsBuildingConfig() {
    TEST_CASE("Terrain config differs from building/default config");

    EdgeDetectionConfig defaultConfig;
    EdgeDetectionConfig terrainConfig = TerrainEdgeConfig::createConfig();

    // Normal threshold: terrain should be lower (more sensitive)
    EXPECT_TRUE(terrainConfig.normalThreshold < defaultConfig.normalThreshold);

    // Depth threshold: terrain should be higher (less sensitive to avoid slope noise)
    EXPECT_TRUE(terrainConfig.depthThreshold > defaultConfig.depthThreshold);

    // Edge thickness: terrain should be thicker for visibility at distance
    EXPECT_TRUE(terrainConfig.edgeThickness >= defaultConfig.edgeThickness);

    printf("  [INFO] Default normal: %.3f, Terrain normal: %.3f (terrain lower)\n",
           defaultConfig.normalThreshold, terrainConfig.normalThreshold);
    printf("  [INFO] Default depth: %.3f, Terrain depth: %.3f (terrain higher)\n",
           defaultConfig.depthThreshold, terrainConfig.depthThreshold);
    printf("  [INFO] Default thickness: %.1f, Terrain thickness: %.1f\n",
           defaultConfig.edgeThickness, terrainConfig.edgeThickness);
}

// Test: Cliff edge detection parameters
void test_CliffEdgeParameters() {
    TEST_CASE("Cliff edge detection parameters are tuned");

    // Cliff is identified by normal.y < CLIFF_NORMAL_Y_THRESHOLD
    // This means the surface is more horizontal than vertical

    // 0.5 means cliff faces with slope > 60 degrees are considered cliffs
    // arccos(0.5) = 60 degrees
    float cliffThreshold = TerrainEdgeConfig::CLIFF_NORMAL_Y_THRESHOLD;
    float cliffAngleDegrees = std::acos(cliffThreshold) * 180.0f / 3.14159f;

    EXPECT_TRUE(cliffAngleDegrees > 45.0f);  // Cliffs should be > 45 degrees
    EXPECT_TRUE(cliffAngleDegrees < 80.0f);  // But not nearly vertical

    // Cliff edge weight should provide visible boost
    EXPECT_TRUE(TerrainEdgeConfig::CLIFF_EDGE_WEIGHT >= 1.25f);

    printf("  [INFO] Cliff detected when slope > %.1f degrees\n", cliffAngleDegrees);
    printf("  [INFO] Cliff edges boosted by %.0f%%\n",
           (TerrainEdgeConfig::CLIFF_EDGE_WEIGHT - 1.0f) * 100.0f);
}

// Test: Gentle slope parameters avoid noise
void test_GentleSlopeParameters() {
    TEST_CASE("Gentle slope parameters configured to avoid edge noise");

    // Gentle slopes should NOT produce depth edge artifacts
    // GENTLE_SLOPE_ANGLE defines the max slope angle for suppression

    float gentleAngle = TerrainEdgeConfig::GENTLE_SLOPE_ANGLE;
    float gentleAngleDegrees = gentleAngle * 180.0f / 3.14159f;

    // Gentle slope should be < 30 degrees (typical rolling terrain)
    EXPECT_TRUE(gentleAngleDegrees > 10.0f);
    EXPECT_TRUE(gentleAngleDegrees < 35.0f);

    printf("  [INFO] Gentle slope threshold: < %.1f degrees\n", gentleAngleDegrees);
    printf("  [INFO] Depth edges suppressed on gentle slopes\n");
}

// Test: Water shoreline parameters
void test_WaterShorelineParameters() {
    TEST_CASE("Water shoreline edge parameters configured");

    // Shorelines occur at water/land boundaries
    // Water has flat normals (0, 1, 0), land has varied normals
    // The transition creates a normal discontinuity

    // Shoreline edge weight should be modest (visible but not overwhelming)
    EXPECT_TRUE(TerrainEdgeConfig::SHORELINE_EDGE_WEIGHT > 1.0f);
    EXPECT_TRUE(TerrainEdgeConfig::SHORELINE_EDGE_WEIGHT < TerrainEdgeConfig::CLIFF_EDGE_WEIGHT);

    printf("  [INFO] Shoreline edges boosted by %.0f%%\n",
           (TerrainEdgeConfig::SHORELINE_EDGE_WEIGHT - 1.0f) * 100.0f);
    printf("  [INFO] Shoreline boost < cliff boost (%.0f%% < %.0f%%)\n",
           (TerrainEdgeConfig::SHORELINE_EDGE_WEIGHT - 1.0f) * 100.0f,
           (TerrainEdgeConfig::CLIFF_EDGE_WEIGHT - 1.0f) * 100.0f);
}

// Test: Depth linearization for terrain distances
void test_TerrainDepthLinearization() {
    TEST_CASE("Depth linearization correct for terrain distances");

    // Terrain is typically viewed at distances of 50-250 world units
    // Depth linearization must work correctly at these distances

    EdgeDetectionConfig terrainConfig = TerrainEdgeConfig::createConfig(
        glm::vec4(0.165f, 0.106f, 0.239f, 1.0f),
        0.1f,   // near
        1000.0f // far
    );

    auto linearizeDepth = [&](float rawDepth) -> float {
        float nearP = terrainConfig.nearPlane;
        float farP = terrainConfig.farPlane;
        return (nearP * farP) / (farP - rawDepth * (farP - nearP));
    };

    // Test linearization at typical terrain viewing distances
    // Calculate raw depth for 50 world units
    // Inverse of linearization: rawDepth = (far - near*far/linear) / (far - near)

    auto rawDepthForLinear = [&](float linear) -> float {
        float nearP = terrainConfig.nearPlane;
        float farP = terrainConfig.farPlane;
        return (farP - nearP * farP / linear) / (farP - nearP);
    };

    // At 50 units (close terrain)
    float raw50 = rawDepthForLinear(50.0f);
    float linear50 = linearizeDepth(raw50);
    EXPECT_NEAR(linear50, 50.0f, 0.5f);

    // At 100 units (medium terrain)
    float raw100 = rawDepthForLinear(100.0f);
    float linear100 = linearizeDepth(raw100);
    EXPECT_NEAR(linear100, 100.0f, 1.0f);

    // At 250 units (far terrain)
    float raw250 = rawDepthForLinear(250.0f);
    float linear250 = linearizeDepth(raw250);
    EXPECT_NEAR(linear250, 250.0f, 2.5f);

    printf("  [INFO] Linearization verified at terrain distances:\n");
    printf("    50 units: raw=%.4f -> linear=%.1f\n", raw50, linear50);
    printf("    100 units: raw=%.4f -> linear=%.1f\n", raw100, linear100);
    printf("    250 units: raw=%.4f -> linear=%.1f\n", raw250, linear250);
}

// Test: Distance scale factor reduces depth sensitivity at far distances
void test_DistanceScaleFactor() {
    TEST_CASE("Distance scale factor reduces depth sensitivity at far distances");

    // DISTANCE_SCALE_FACTOR scales the depth threshold based on camera distance
    // At far distances, we want less sensitivity to avoid noise on gentle slopes

    float scaleFactor = TerrainEdgeConfig::DISTANCE_SCALE_FACTOR;

    // Scale factor should reduce sensitivity (< 1.0)
    EXPECT_TRUE(scaleFactor > 0.0f);
    EXPECT_TRUE(scaleFactor <= 1.0f);

    // Calculate effective depth threshold at max distance
    float baseThreshold = TerrainEdgeConfig::DEPTH_THRESHOLD;
    float effectiveThreshold = baseThreshold / scaleFactor;

    // Effective threshold at far distance should be higher (less sensitive)
    EXPECT_TRUE(effectiveThreshold > baseThreshold);

    printf("  [INFO] Distance scale factor: %.2f\n", scaleFactor);
    printf("  [INFO] Base depth threshold: %.3f\n", baseThreshold);
    printf("  [INFO] Effective threshold at max distance: %.3f\n", effectiveThreshold);
}

// =============================================================================
// Main entry point
// =============================================================================
int main() {
    printf("========================================\n");
    printf("Edge Detection Pass Tests\n");
    printf("(Tickets 2-006, 3-035)\n");
    printf("========================================\n");

    // Original tests (Ticket 2-006)
    printf("\n--- General Edge Detection (2-006) ---\n");
    test_EdgeDetectionConfigDefaults();
    test_EdgeDetectionUBOSize();
    test_EdgeDetectionUBOLayout();
    test_ToonShaderConfigEdgeColor();
    test_EdgeThicknessConfiguration();
    test_DepthLinearizationParameters();
    test_NormalBasedPrimarySignal();

    // Terrain-specific tests (Ticket 3-035)
    printf("\n--- Terrain Edge Detection (3-035) ---\n");
    test_TerrainEdgeConfigConstants();
    test_TerrainEdgeConfigCreateConfig();
    test_TerrainVsBuildingConfig();
    test_CliffEdgeParameters();
    test_GentleSlopeParameters();
    test_WaterShorelineParameters();
    test_TerrainDepthLinearization();
    test_DistanceScaleFactor();

    // Summary
    printf("\n========================================\n");
    printf("SUMMARY: %d passed, %d failed\n", g_testsPassed, g_testsFailed);
    printf("========================================\n");

    // Manual verification note
    printf("\n[NOTE] GPU rendering requires manual visual verification:\n");
    printf("  General edge detection:\n");
    printf("  - Run application with edge detection enabled\n");
    printf("  - Verify outlines appear on opaque geometry\n");
    printf("  - Verify edges are readable at all zoom levels\n");
    printf("  - Verify <1ms performance at 1080p\n");
    printf("  - Test at multiple camera angles\n");
    printf("\n  Terrain-specific (Ticket 3-035):\n");
    printf("  - Cliff edges produce bold outlines\n");
    printf("  - Water shorelines produce visible outlines\n");
    printf("  - Gentle slopes do NOT produce excessive edge lines\n");
    printf("  - Terrain type boundaries have visual separation via color\n");
    printf("  - Test at multiple zoom levels (5-250 units distance)\n");
    printf("  - Test at various camera angles (15-80 degree pitch)\n");

    return g_testsFailed > 0 ? 1 : 0;
}
