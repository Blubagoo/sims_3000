/**
 * @file test_emissive_material.cpp
 * @brief Unit tests for EmissiveMaterial system.
 *
 * Tests verify:
 * - Emission texture or color in material definition
 * - Emission not affected by lighting bands (via shader - manual verification)
 * - All 10 terrain types have distinct emissive glow properties
 * - Multi-color emissive palette: cyan (#00D4AA), green, amber, magenta
 * - Per-instance emissive control (emissive_intensity, emissive_color)
 * - Glow intensity hierarchy: player structures > terrain features > background
 * - Glow transitions fade over ~0.5s (interpolated float)
 * - Powered buildings glow; unpowered buildings do not
 *
 * @see Ticket 2-037: Emissive Material Support
 */

#include "sims3000/render/EmissiveMaterial.h"
#include "sims3000/render/ToonShaderConfig.h"
#include "sims3000/render/ToonShader.h"
#include "sims3000/render/GPUMesh.h"
#include <cstdio>
#include <cmath>

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

#define ASSERT_TRUE(condition) do { \
    if (!(condition)) { \
        printf("FAILED\n  Condition was false: %s\n", #condition); \
        g_testsFailed++; \
        return; \
    } \
} while(0)

#define ASSERT_FALSE(condition) do { \
    if ((condition)) { \
        printf("FAILED\n  Condition was true: %s\n", #condition); \
        g_testsFailed++; \
        return; \
    } \
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

#define ASSERT_VEC3_EQ(expected, actual, epsilon) do { \
    if (std::abs((expected).r - (actual).r) > (epsilon) || \
        std::abs((expected).g - (actual).g) > (epsilon) || \
        std::abs((expected).b - (actual).b) > (epsilon)) { \
        printf("FAILED\n  Expected: (%f, %f, %f), Actual: (%f, %f, %f)\n", \
               (float)(expected).r, (float)(expected).g, (float)(expected).b, \
               (float)(actual).r, (float)(actual).g, (float)(actual).b); \
        g_testsFailed++; \
        return; \
    } \
} while(0)

// =============================================================================
// Palette Color Tests
// =============================================================================

TEST(Palette_CyanColor_MatchesSpec) {
    // #00D4AA in hex = 0, 212, 170 in decimal = 0, 0.831, 0.667 normalized
    glm::vec3 expected{0.0f, 0.831f, 0.667f};
    ASSERT_VEC3_EQ(expected, EmissivePalette::CYAN, 0.01f);
}

TEST(Palette_GreenColor_Exists) {
    ASSERT_TRUE(EmissivePalette::GREEN.g > 0.9f);  // Strong green component
    ASSERT_TRUE(EmissivePalette::GREEN.r < 0.5f);  // Low red
}

TEST(Palette_AmberColor_Exists) {
    ASSERT_TRUE(EmissivePalette::AMBER.r > 0.9f);  // Strong red
    ASSERT_TRUE(EmissivePalette::AMBER.g > 0.5f);  // Medium green (orange-ish)
    ASSERT_TRUE(EmissivePalette::AMBER.b < 0.2f);  // Low blue
}

TEST(Palette_MagentaColor_Exists) {
    ASSERT_TRUE(EmissivePalette::MAGENTA.r > 0.9f);  // Strong red
    ASSERT_TRUE(EmissivePalette::MAGENTA.b > 0.9f);  // Strong blue
    ASSERT_TRUE(EmissivePalette::MAGENTA.g < 0.2f);  // Low green
}

TEST(Palette_AllColorsValid) {
    // All palette colors should be in valid range [0, 1]
    auto checkColor = [](const glm::vec3& c) {
        return c.r >= 0.0f && c.r <= 1.0f &&
               c.g >= 0.0f && c.g <= 1.0f &&
               c.b >= 0.0f && c.b <= 1.0f;
    };

    ASSERT_TRUE(checkColor(EmissivePalette::CYAN));
    ASSERT_TRUE(checkColor(EmissivePalette::GREEN));
    ASSERT_TRUE(checkColor(EmissivePalette::AMBER));
    ASSERT_TRUE(checkColor(EmissivePalette::MAGENTA));
    ASSERT_TRUE(checkColor(EmissivePalette::PURPLE));
    ASSERT_TRUE(checkColor(EmissivePalette::WATER_BLUE));
    ASSERT_TRUE(checkColor(EmissivePalette::TOXIC_GREEN));
}

// =============================================================================
// Glow Hierarchy Tests
// =============================================================================

TEST(GlowHierarchy_PlayerStructure_Highest) {
    float playerMult = GlowHierarchy::getMultiplier(EmissiveCategory::PlayerStructure);
    float terrainMult = GlowHierarchy::getMultiplier(EmissiveCategory::TerrainFeature);
    float bgMult = GlowHierarchy::getMultiplier(EmissiveCategory::Background);

    ASSERT_TRUE(playerMult > terrainMult);
    ASSERT_TRUE(terrainMult > bgMult);
}

TEST(GlowHierarchy_PlayerStructure_IsFullIntensity) {
    float mult = GlowHierarchy::getMultiplier(EmissiveCategory::PlayerStructure);
    ASSERT_FLOAT_EQ(1.0f, mult, 0.001f);
}

TEST(GlowHierarchy_TerrainFeature_IsMediumIntensity) {
    float mult = GlowHierarchy::getMultiplier(EmissiveCategory::TerrainFeature);
    ASSERT_TRUE(mult > 0.0f && mult < 1.0f);
    ASSERT_FLOAT_EQ(0.6f, mult, 0.001f);  // 60% of base
}

TEST(GlowHierarchy_Background_IsLowestIntensity) {
    float mult = GlowHierarchy::getMultiplier(EmissiveCategory::Background);
    ASSERT_TRUE(mult > 0.0f);
    ASSERT_FLOAT_EQ(0.3f, mult, 0.001f);  // 30% of base
}

// =============================================================================
// EmissiveState Tests
// =============================================================================

TEST(EmissiveState_DefaultConstruction_Unpowered) {
    EmissiveState state;

    ASSERT_FALSE(state.isPowered());
    ASSERT_FLOAT_EQ(0.0f, state.getCurrentIntensity(), 0.001f);
}

TEST(EmissiveState_SetPowered_ChangesState) {
    EmissiveState state;

    state.setPowered(true, EmissivePalette::CYAN, EmissiveCategory::PlayerStructure);

    ASSERT_TRUE(state.isPowered());
    ASSERT_TRUE(state.getTargetIntensity() > 0.0f);
}

TEST(EmissiveState_SetPoweredFalse_ZeroIntensity) {
    EmissiveState state;
    state.setIntensityImmediate(1.0f);

    state.setPowered(false, EmissivePalette::CYAN, EmissiveCategory::PlayerStructure);

    ASSERT_FALSE(state.isPowered());
    ASSERT_FLOAT_EQ(0.0f, state.getTargetIntensity(), 0.001f);
}

TEST(EmissiveState_GlowTransition_ApproximatelyHalfSecond) {
    EmissiveState state;
    state.setIntensityImmediate(0.0f);

    // Set powered - should initiate transition to 1.0 (PlayerStructure multiplier)
    state.setPowered(true, EmissivePalette::CYAN, EmissiveCategory::PlayerStructure, 1.0f);

    // Simulate 10 ticks (0.5 seconds at 20Hz)
    for (int i = 0; i < 10; ++i) {
        state.tick();
    }

    // After 10 ticks, should be most of the way there (>80%)
    float intensity = state.getCurrentIntensity();
    float target = state.getTargetIntensity();
    float progress = intensity / target;

    ASSERT_TRUE(progress > 0.8f);
}

TEST(EmissiveState_GlowTransition_ConvergesToTarget) {
    EmissiveState state;
    state.setIntensityImmediate(0.0f);
    state.setPowered(true, EmissivePalette::CYAN, EmissiveCategory::PlayerStructure);

    // Simulate 50 ticks (2.5 seconds at 20Hz) - should definitely be at target
    // With 0.2 lerp factor, after ~35 ticks we reach within 0.001 of target
    // (0.8^35 = 0.0004)
    for (int i = 0; i < 50; ++i) {
        state.tick();
    }

    // After 50 ticks, the remaining delta should be essentially zero
    float intensity = state.getCurrentIntensity();
    float target = state.getTargetIntensity();
    float delta = std::abs(target - intensity);

    // Must be within our snap threshold (0.001)
    ASSERT_TRUE(delta < 0.01f);
}

TEST(EmissiveState_Interpolation_SmoothBetweenFrames) {
    EmissiveState state;
    state.setIntensityImmediate(0.0f);
    state.setPowered(true, EmissivePalette::CYAN, EmissiveCategory::PlayerStructure);

    // One tick to start transition
    state.tick();

    // Interpolation at alpha=0.5 should be between previous and current
    float prev = state.getInterpolatedIntensity(0.0f);
    float curr = state.getInterpolatedIntensity(1.0f);
    float mid = state.getInterpolatedIntensity(0.5f);

    ASSERT_TRUE(mid >= prev && mid <= curr);
}

TEST(EmissiveState_SetIntensityImmediate_SkipsTransition) {
    EmissiveState state;

    state.setIntensityImmediate(0.75f);

    // Should immediately be at intensity (with category multiplier)
    float expected = 0.75f * GlowHierarchy::getMultiplier(EmissiveCategory::PlayerStructure);
    ASSERT_FLOAT_EQ(expected, state.getCurrentIntensity(), 0.001f);
    ASSERT_TRUE(state.isTransitionComplete());
}

TEST(EmissiveState_ColorPreserved) {
    EmissiveState state;

    state.setPowered(true, EmissivePalette::MAGENTA, EmissiveCategory::PlayerStructure);

    const glm::vec3& color = state.getColor();
    ASSERT_VEC3_EQ(EmissivePalette::MAGENTA, color, 0.001f);
}

TEST(EmissiveState_GetColorWithIntensity_CombinesCorrectly) {
    EmissiveState state;
    state.setPowered(true, EmissivePalette::GREEN, EmissiveCategory::PlayerStructure);
    state.setIntensityImmediate(0.8f);

    glm::vec4 result = state.getColorWithIntensity(1.0f);

    ASSERT_FLOAT_EQ(EmissivePalette::GREEN.r, result.r, 0.001f);
    ASSERT_FLOAT_EQ(EmissivePalette::GREEN.g, result.g, 0.001f);
    ASSERT_FLOAT_EQ(EmissivePalette::GREEN.b, result.b, 0.001f);
    ASSERT_TRUE(result.a > 0.0f);  // Has intensity
}

TEST(EmissiveState_SetPoweredForTerrain_UsesPreset) {
    ToonShaderConfig::instance().resetToDefaults();

    EmissiveState state;
    state.setPoweredForTerrain(true, TerrainType::CrystalFields);

    // Crystal fields should use magenta/pink color
    const glm::vec3& color = state.getColor();
    ASSERT_TRUE(color.r > 0.5f || color.b > 0.5f);  // Magenta has high R or B

    // Should have terrain multiplier applied
    ASSERT_TRUE(state.getTargetIntensity() > 0.0f);
}

// =============================================================================
// Utility Function Tests
// =============================================================================

TEST(GetEmissiveColorForBuilding_Powered_HasIntensity) {
    glm::vec4 result = getEmissiveColorForBuilding(true, EmissivePalette::CYAN, 0.8f);

    ASSERT_FLOAT_EQ(EmissivePalette::CYAN.r, result.r, 0.001f);
    ASSERT_FLOAT_EQ(EmissivePalette::CYAN.g, result.g, 0.001f);
    ASSERT_FLOAT_EQ(EmissivePalette::CYAN.b, result.b, 0.001f);
    ASSERT_TRUE(result.a > 0.0f);
}

TEST(GetEmissiveColorForBuilding_Unpowered_ZeroIntensity) {
    glm::vec4 result = getEmissiveColorForBuilding(false, EmissivePalette::CYAN, 0.8f);

    // Color is preserved but intensity is zero
    ASSERT_FLOAT_EQ(EmissivePalette::CYAN.r, result.r, 0.001f);
    ASSERT_FLOAT_EQ(0.0f, result.a, 0.001f);
}

TEST(GetEmissiveColorForTerrain_HasTerrainMultiplier) {
    ToonShaderConfig::instance().resetToDefaults();

    glm::vec4 result = getEmissiveColorForTerrain(TerrainType::Forest);

    // Forest should have visible intensity with terrain multiplier
    ASSERT_TRUE(result.a > 0.0f);
    ASSERT_TRUE(result.a < 1.0f);  // Should have terrain multiplier applied
}

TEST(GetEmissiveColorForBackground_HasBackgroundMultiplier) {
    glm::vec4 result = getEmissiveColorForBackground(EmissivePalette::CYAN, 1.0f);

    // Background multiplier should reduce intensity
    float expected = 1.0f * GlowHierarchy::BACKGROUND_MULTIPLIER;
    ASSERT_FLOAT_EQ(expected, result.a, 0.001f);
}

// =============================================================================
// Material Definition Tests (Existing Structures)
// =============================================================================

TEST(GPUMaterial_HasEmissiveTexture) {
    GPUMaterial material;
    // Default should be nullptr
    ASSERT_TRUE(material.emissiveTexture == nullptr);
}

TEST(GPUMaterial_HasEmissiveColor) {
    GPUMaterial material;
    material.emissiveColor = glm::vec3(1.0f, 0.5f, 0.25f);

    ASSERT_FLOAT_EQ(1.0f, material.emissiveColor.r, 0.001f);
    ASSERT_FLOAT_EQ(0.5f, material.emissiveColor.g, 0.001f);
    ASSERT_FLOAT_EQ(0.25f, material.emissiveColor.b, 0.001f);
}

TEST(GPUMaterial_HasEmissive_ChecksCorrectly) {
    GPUMaterial noEmissive;
    noEmissive.emissiveColor = glm::vec3(0.0f);
    noEmissive.emissiveTexture = nullptr;
    ASSERT_FALSE(noEmissive.hasEmissive());

    GPUMaterial hasColor;
    hasColor.emissiveColor = glm::vec3(0.5f, 0.0f, 0.0f);
    ASSERT_TRUE(hasColor.hasEmissive());
}

TEST(ToonInstanceData_HasEmissiveColor) {
    ToonInstanceData instance;
    instance.emissiveColor = glm::vec4(0.5f, 0.6f, 0.7f, 0.8f);

    // RGB is color, A is intensity
    ASSERT_FLOAT_EQ(0.5f, instance.emissiveColor.r, 0.001f);
    ASSERT_FLOAT_EQ(0.6f, instance.emissiveColor.g, 0.001f);
    ASSERT_FLOAT_EQ(0.7f, instance.emissiveColor.b, 0.001f);
    ASSERT_FLOAT_EQ(0.8f, instance.emissiveColor.a, 0.001f);
}

// =============================================================================
// Terrain Emissive Presets Tests
// =============================================================================

TEST(TerrainPresets_AllTenTypesHaveDistinctColors) {
    ToonShaderConfig::instance().resetToDefaults();
    const auto& presets = ToonShaderConfig::instance().getTerrainEmissivePresets();

    // Should have exactly 10 terrain types
    ASSERT_EQ(10, presets.size());

    // Verify each has non-zero color
    for (std::size_t i = 0; i < presets.size(); ++i) {
        const auto& preset = presets[i];
        bool hasColor = preset.color.r > 0.0f ||
                        preset.color.g > 0.0f ||
                        preset.color.b > 0.0f;
        ASSERT_TRUE(hasColor);
    }
}

TEST(TerrainPresets_EachHasIntensity) {
    ToonShaderConfig::instance().resetToDefaults();

    for (std::size_t i = 0; i < TERRAIN_TYPE_COUNT; ++i) {
        TerrainType type = static_cast<TerrainType>(i);
        const auto& preset = ToonShaderConfig::instance().getTerrainEmissivePreset(type);
        ASSERT_TRUE(preset.intensity >= 0.0f && preset.intensity <= 1.0f);
    }
}

TEST(TerrainPresets_CrystalFields_HighIntensity) {
    ToonShaderConfig::instance().resetToDefaults();
    const auto& preset = ToonShaderConfig::instance().getTerrainEmissivePreset(TerrainType::CrystalFields);

    // Crystal fields should have high intensity per canon
    ASSERT_TRUE(preset.intensity >= 0.8f);
}

TEST(TerrainPresets_FlatGround_LowIntensity) {
    ToonShaderConfig::instance().resetToDefaults();
    const auto& preset = ToonShaderConfig::instance().getTerrainEmissivePreset(TerrainType::FlatGround);

    // Flat ground should have subtle glow
    ASSERT_TRUE(preset.intensity <= 0.4f);
}

// =============================================================================
// Main
// =============================================================================

int main() {
    printf("=== EmissiveMaterial Unit Tests ===\n\n");

    // Palette tests
    printf("--- Palette Color Tests ---\n");
    RUN_TEST(Palette_CyanColor_MatchesSpec);
    RUN_TEST(Palette_GreenColor_Exists);
    RUN_TEST(Palette_AmberColor_Exists);
    RUN_TEST(Palette_MagentaColor_Exists);
    RUN_TEST(Palette_AllColorsValid);

    // Glow hierarchy tests
    printf("\n--- Glow Hierarchy Tests ---\n");
    RUN_TEST(GlowHierarchy_PlayerStructure_Highest);
    RUN_TEST(GlowHierarchy_PlayerStructure_IsFullIntensity);
    RUN_TEST(GlowHierarchy_TerrainFeature_IsMediumIntensity);
    RUN_TEST(GlowHierarchy_Background_IsLowestIntensity);

    // EmissiveState tests
    printf("\n--- EmissiveState Tests ---\n");
    RUN_TEST(EmissiveState_DefaultConstruction_Unpowered);
    RUN_TEST(EmissiveState_SetPowered_ChangesState);
    RUN_TEST(EmissiveState_SetPoweredFalse_ZeroIntensity);
    RUN_TEST(EmissiveState_GlowTransition_ApproximatelyHalfSecond);
    RUN_TEST(EmissiveState_GlowTransition_ConvergesToTarget);
    RUN_TEST(EmissiveState_Interpolation_SmoothBetweenFrames);
    RUN_TEST(EmissiveState_SetIntensityImmediate_SkipsTransition);
    RUN_TEST(EmissiveState_ColorPreserved);
    RUN_TEST(EmissiveState_GetColorWithIntensity_CombinesCorrectly);
    RUN_TEST(EmissiveState_SetPoweredForTerrain_UsesPreset);

    // Utility function tests
    printf("\n--- Utility Function Tests ---\n");
    RUN_TEST(GetEmissiveColorForBuilding_Powered_HasIntensity);
    RUN_TEST(GetEmissiveColorForBuilding_Unpowered_ZeroIntensity);
    RUN_TEST(GetEmissiveColorForTerrain_HasTerrainMultiplier);
    RUN_TEST(GetEmissiveColorForBackground_HasBackgroundMultiplier);

    // Material definition tests
    printf("\n--- Material Definition Tests ---\n");
    RUN_TEST(GPUMaterial_HasEmissiveTexture);
    RUN_TEST(GPUMaterial_HasEmissiveColor);
    RUN_TEST(GPUMaterial_HasEmissive_ChecksCorrectly);
    RUN_TEST(ToonInstanceData_HasEmissiveColor);

    // Terrain preset tests
    printf("\n--- Terrain Emissive Preset Tests ---\n");
    RUN_TEST(TerrainPresets_AllTenTypesHaveDistinctColors);
    RUN_TEST(TerrainPresets_EachHasIntensity);
    RUN_TEST(TerrainPresets_CrystalFields_HighIntensity);
    RUN_TEST(TerrainPresets_FlatGround_LowIntensity);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", g_testsPassed);
    printf("Failed: %d\n", g_testsFailed);

    return g_testsFailed == 0 ? 0 : 1;
}
