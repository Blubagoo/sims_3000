/**
 * @file test_toon_shader_config.cpp
 * @brief Unit tests for ToonShaderConfig runtime configuration resource.
 *
 * Tests verify:
 * - Singleton access pattern
 * - Band count and threshold configuration
 * - Shadow color and shift amount
 * - Edge line width configuration
 * - Bloom threshold and intensity
 * - Emissive multiplier
 * - Per-terrain-type emissive presets
 * - Ambient light level
 * - Immediate effect of changes (dirty flag)
 * - Default values match Game Designer specifications
 * - Preset application (day/night/high-contrast)
 */

#include "sims3000/render/ToonShaderConfig.h"
#include <cstdio>
#include <cmath>

using namespace sims3000;

// Simple test framework
static int g_testsPassed = 0;
static int g_testsFailed = 0;

#define TEST(name) void test_##name()
#define RUN_TEST(name) do { \
    printf("Running test: %s... ", #name); \
    ToonShaderConfig::instance().resetToDefaults(); \
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

#define ASSERT_FALSE(condition) do { \
    if ((condition)) { \
        printf("FAILED\n  Condition was true: %s\n", #condition); \
        g_testsFailed++; \
        return; \
    } \
} while(0)

// =============================================================================
// Singleton Tests
// =============================================================================

TEST(Singleton_SameInstance) {
    ToonShaderConfig& a = ToonShaderConfig::instance();
    ToonShaderConfig& b = ToonShaderConfig::instance();
    ASSERT_TRUE(&a == &b);
}

// =============================================================================
// Band Configuration Tests
// =============================================================================

TEST(BandCount_Default) {
    auto& config = ToonShaderConfig::instance();
    ASSERT_EQ(4, config.getBandCount());
}

TEST(BandCount_SetValid) {
    auto& config = ToonShaderConfig::instance();

    config.setBandCount(2);
    ASSERT_EQ(2, config.getBandCount());

    config.setBandCount(1);
    ASSERT_EQ(1, config.getBandCount());

    config.setBandCount(4);
    ASSERT_EQ(4, config.getBandCount());
}

TEST(BandCount_ClampMin) {
    auto& config = ToonShaderConfig::instance();
    config.setBandCount(0);  // Should clamp to 1
    ASSERT_EQ(1, config.getBandCount());
}

TEST(BandCount_ClampMax) {
    auto& config = ToonShaderConfig::instance();
    config.setBandCount(100);  // Should clamp to MAX_BANDS (4)
    ASSERT_EQ(4, config.getBandCount());
}

TEST(BandThreshold_Defaults) {
    auto& config = ToonShaderConfig::instance();
    ASSERT_FLOAT_EQ(0.0f, config.getBandThreshold(0), 0.001f);
    ASSERT_FLOAT_EQ(0.2f, config.getBandThreshold(1), 0.001f);
    ASSERT_FLOAT_EQ(0.4f, config.getBandThreshold(2), 0.001f);
    ASSERT_FLOAT_EQ(0.7f, config.getBandThreshold(3), 0.001f);
}

TEST(BandThreshold_SetValid) {
    auto& config = ToonShaderConfig::instance();

    config.setBandThreshold(1, 0.35f);
    ASSERT_FLOAT_EQ(0.35f, config.getBandThreshold(1), 0.001f);
}

TEST(BandThreshold_ClampRange) {
    auto& config = ToonShaderConfig::instance();

    config.setBandThreshold(0, -0.5f);  // Should clamp to 0
    ASSERT_FLOAT_EQ(0.0f, config.getBandThreshold(0), 0.001f);

    config.setBandThreshold(0, 1.5f);  // Should clamp to 1
    ASSERT_FLOAT_EQ(1.0f, config.getBandThreshold(0), 0.001f);
}

TEST(BandIntensity_Defaults) {
    auto& config = ToonShaderConfig::instance();
    ASSERT_FLOAT_EQ(0.15f, config.getBandIntensity(0), 0.001f);
    ASSERT_FLOAT_EQ(0.35f, config.getBandIntensity(1), 0.001f);
    ASSERT_FLOAT_EQ(0.65f, config.getBandIntensity(2), 0.001f);
    ASSERT_FLOAT_EQ(1.0f, config.getBandIntensity(3), 0.001f);
}

TEST(BandIntensity_SetValid) {
    auto& config = ToonShaderConfig::instance();

    config.setBandIntensity(0, 0.1f);
    ASSERT_FLOAT_EQ(0.1f, config.getBandIntensity(0), 0.001f);
}

TEST(GetBands_ReturnsArray) {
    auto& config = ToonShaderConfig::instance();
    const auto& bands = config.getBands();

    ASSERT_EQ(4, bands.size());
    ASSERT_FLOAT_EQ(0.2f, bands[1].threshold, 0.001f);
    ASSERT_FLOAT_EQ(0.35f, bands[1].intensity, 0.001f);
}

// =============================================================================
// Shadow Configuration Tests
// =============================================================================

TEST(ShadowColor_Default) {
    auto& config = ToonShaderConfig::instance();
    const auto& color = config.getShadowColor();

    // #2A1B3D = 42, 27, 61
    ASSERT_FLOAT_EQ(42.0f / 255.0f, color.r, 0.001f);
    ASSERT_FLOAT_EQ(27.0f / 255.0f, color.g, 0.001f);
    ASSERT_FLOAT_EQ(61.0f / 255.0f, color.b, 0.001f);
}

TEST(ShadowColor_SetValid) {
    auto& config = ToonShaderConfig::instance();

    config.setShadowColor(glm::vec3(0.5f, 0.3f, 0.1f));
    const auto& color = config.getShadowColor();

    ASSERT_FLOAT_EQ(0.5f, color.r, 0.001f);
    ASSERT_FLOAT_EQ(0.3f, color.g, 0.001f);
    ASSERT_FLOAT_EQ(0.1f, color.b, 0.001f);
}

TEST(ShadowShiftAmount_Default) {
    auto& config = ToonShaderConfig::instance();
    ASSERT_FLOAT_EQ(0.7f, config.getShadowShiftAmount(), 0.001f);
}

TEST(ShadowShiftAmount_SetValid) {
    auto& config = ToonShaderConfig::instance();

    config.setShadowShiftAmount(0.5f);
    ASSERT_FLOAT_EQ(0.5f, config.getShadowShiftAmount(), 0.001f);
}

TEST(ShadowShiftAmount_ClampRange) {
    auto& config = ToonShaderConfig::instance();

    config.setShadowShiftAmount(-0.5f);
    ASSERT_FLOAT_EQ(0.0f, config.getShadowShiftAmount(), 0.001f);

    config.setShadowShiftAmount(1.5f);
    ASSERT_FLOAT_EQ(1.0f, config.getShadowShiftAmount(), 0.001f);
}

// =============================================================================
// Edge Configuration Tests
// =============================================================================

TEST(EdgeLineWidth_Default) {
    auto& config = ToonShaderConfig::instance();
    ASSERT_FLOAT_EQ(1.0f, config.getEdgeLineWidth(), 0.001f);
}

TEST(EdgeLineWidth_SetValid) {
    auto& config = ToonShaderConfig::instance();

    config.setEdgeLineWidth(2.5f);
    ASSERT_FLOAT_EQ(2.5f, config.getEdgeLineWidth(), 0.001f);
}

TEST(EdgeLineWidth_ClampRange) {
    auto& config = ToonShaderConfig::instance();

    config.setEdgeLineWidth(-1.0f);
    ASSERT_FLOAT_EQ(0.0f, config.getEdgeLineWidth(), 0.001f);

    config.setEdgeLineWidth(15.0f);
    ASSERT_FLOAT_EQ(10.0f, config.getEdgeLineWidth(), 0.001f);
}

// =============================================================================
// Bloom Configuration Tests
// =============================================================================

TEST(BloomThreshold_Default) {
    auto& config = ToonShaderConfig::instance();
    ASSERT_FLOAT_EQ(0.7f, config.getBloomThreshold(), 0.001f);
}

TEST(BloomThreshold_SetValid) {
    auto& config = ToonShaderConfig::instance();

    config.setBloomThreshold(0.5f);
    ASSERT_FLOAT_EQ(0.5f, config.getBloomThreshold(), 0.001f);
}

TEST(BloomThreshold_ClampRange) {
    auto& config = ToonShaderConfig::instance();

    config.setBloomThreshold(-0.5f);
    ASSERT_FLOAT_EQ(0.0f, config.getBloomThreshold(), 0.001f);

    config.setBloomThreshold(1.5f);
    ASSERT_FLOAT_EQ(1.0f, config.getBloomThreshold(), 0.001f);
}

TEST(BloomIntensity_Default) {
    auto& config = ToonShaderConfig::instance();
    ASSERT_FLOAT_EQ(1.0f, config.getBloomIntensity(), 0.001f);
}

TEST(BloomIntensity_SetValid) {
    auto& config = ToonShaderConfig::instance();

    config.setBloomIntensity(0.8f);
    ASSERT_FLOAT_EQ(0.8f, config.getBloomIntensity(), 0.001f);

    config.setBloomIntensity(1.5f);
    ASSERT_FLOAT_EQ(1.5f, config.getBloomIntensity(), 0.001f);
}

TEST(BloomIntensity_ClampRange) {
    auto& config = ToonShaderConfig::instance();

    config.setBloomIntensity(-0.5f);
    ASSERT_FLOAT_EQ(0.0f, config.getBloomIntensity(), 0.001f);

    config.setBloomIntensity(3.0f);
    ASSERT_FLOAT_EQ(2.0f, config.getBloomIntensity(), 0.001f);
}

// =============================================================================
// Emissive Configuration Tests
// =============================================================================

TEST(EmissiveMultiplier_Default) {
    auto& config = ToonShaderConfig::instance();
    ASSERT_FLOAT_EQ(1.0f, config.getEmissiveMultiplier(), 0.001f);
}

TEST(EmissiveMultiplier_SetValid) {
    auto& config = ToonShaderConfig::instance();

    config.setEmissiveMultiplier(0.5f);
    ASSERT_FLOAT_EQ(0.5f, config.getEmissiveMultiplier(), 0.001f);

    config.setEmissiveMultiplier(1.8f);
    ASSERT_FLOAT_EQ(1.8f, config.getEmissiveMultiplier(), 0.001f);
}

TEST(EmissiveMultiplier_ClampRange) {
    auto& config = ToonShaderConfig::instance();

    config.setEmissiveMultiplier(-0.5f);
    ASSERT_FLOAT_EQ(0.0f, config.getEmissiveMultiplier(), 0.001f);

    config.setEmissiveMultiplier(3.0f);
    ASSERT_FLOAT_EQ(2.0f, config.getEmissiveMultiplier(), 0.001f);
}

// =============================================================================
// Terrain Emissive Preset Tests
// =============================================================================

TEST(TerrainEmissivePresets_Count) {
    // Verify we have exactly 10 terrain types
    ASSERT_EQ(10, TERRAIN_TYPE_COUNT);
}

TEST(TerrainEmissivePresets_AllInitialized) {
    auto& config = ToonShaderConfig::instance();
    const auto& presets = config.getTerrainEmissivePresets();

    ASSERT_EQ(10, presets.size());

    // Verify each preset has reasonable values
    for (std::size_t i = 0; i < presets.size(); ++i) {
        ASSERT_TRUE(presets[i].intensity >= 0.0f && presets[i].intensity <= 1.0f);
        ASSERT_TRUE(presets[i].color.r >= 0.0f && presets[i].color.r <= 1.0f);
        ASSERT_TRUE(presets[i].color.g >= 0.0f && presets[i].color.g <= 1.0f);
        ASSERT_TRUE(presets[i].color.b >= 0.0f && presets[i].color.b <= 1.0f);
    }
}

TEST(TerrainEmissivePresets_CrystalFields) {
    auto& config = ToonShaderConfig::instance();
    const auto& preset = config.getTerrainEmissivePreset(TerrainType::CrystalFields);

    // Crystal fields should have high intensity (magenta/cyan)
    ASSERT_TRUE(preset.intensity >= 0.8f);
    // Should have strong magenta/pink component
    ASSERT_TRUE(preset.color.r >= 0.7f || preset.color.b >= 0.7f);
}

TEST(TerrainEmissivePresets_VolcanicRock) {
    auto& config = ToonShaderConfig::instance();
    const auto& preset = config.getTerrainEmissivePreset(TerrainType::VolcanicRock);

    // Volcanic rock should have orange/red glow
    ASSERT_TRUE(preset.color.r >= 0.5f);
    ASSERT_TRUE(preset.color.g < preset.color.r);
}

TEST(TerrainEmissivePresets_SetCustom) {
    auto& config = ToonShaderConfig::instance();

    TerrainEmissivePreset custom;
    custom.color = glm::vec3(0.1f, 0.2f, 0.3f);
    custom.intensity = 0.75f;

    config.setTerrainEmissivePreset(TerrainType::Forest, custom);
    const auto& result = config.getTerrainEmissivePreset(TerrainType::Forest);

    ASSERT_FLOAT_EQ(0.1f, result.color.r, 0.001f);
    ASSERT_FLOAT_EQ(0.2f, result.color.g, 0.001f);
    ASSERT_FLOAT_EQ(0.3f, result.color.b, 0.001f);
    ASSERT_FLOAT_EQ(0.75f, result.intensity, 0.001f);
}

TEST(TerrainEmissivePresets_InvalidType) {
    auto& config = ToonShaderConfig::instance();

    // Getting invalid type should return FlatGround preset as fallback
    TerrainType invalid = static_cast<TerrainType>(100);
    const auto& preset = config.getTerrainEmissivePreset(invalid);
    const auto& flatGround = config.getTerrainEmissivePreset(TerrainType::FlatGround);

    ASSERT_FLOAT_EQ(flatGround.color.r, preset.color.r, 0.001f);
    ASSERT_FLOAT_EQ(flatGround.intensity, preset.intensity, 0.001f);
}

// =============================================================================
// Ambient Configuration Tests
// =============================================================================

TEST(AmbientLevel_Default) {
    auto& config = ToonShaderConfig::instance();
    ASSERT_FLOAT_EQ(0.08f, config.getAmbientLevel(), 0.001f);
}

TEST(AmbientLevel_SetValid) {
    auto& config = ToonShaderConfig::instance();

    config.setAmbientLevel(0.05f);
    ASSERT_FLOAT_EQ(0.05f, config.getAmbientLevel(), 0.001f);

    config.setAmbientLevel(0.1f);
    ASSERT_FLOAT_EQ(0.1f, config.getAmbientLevel(), 0.001f);
}

TEST(AmbientLevel_ClampRange) {
    auto& config = ToonShaderConfig::instance();

    config.setAmbientLevel(-0.5f);
    ASSERT_FLOAT_EQ(0.0f, config.getAmbientLevel(), 0.001f);

    config.setAmbientLevel(1.5f);
    ASSERT_FLOAT_EQ(1.0f, config.getAmbientLevel(), 0.001f);
}

TEST(AmbientLevel_DefaultInRecommendedRange) {
    auto& config = ToonShaderConfig::instance();
    float level = config.getAmbientLevel();

    // Per acceptance criteria: ~0.05-0.1
    ASSERT_TRUE(level >= 0.05f && level <= 0.1f);
}

// =============================================================================
// Dirty Flag Tests (Immediate Effect)
// =============================================================================

TEST(DirtyFlag_InitiallyDirty) {
    auto& config = ToonShaderConfig::instance();
    config.resetToDefaults();
    ASSERT_TRUE(config.isDirty());
}

TEST(DirtyFlag_ClearAfterRead) {
    auto& config = ToonShaderConfig::instance();
    config.clearDirtyFlag();
    ASSERT_FALSE(config.isDirty());
}

TEST(DirtyFlag_SetOnBandCountChange) {
    auto& config = ToonShaderConfig::instance();
    config.clearDirtyFlag();

    config.setBandCount(2);
    ASSERT_TRUE(config.isDirty());
}

TEST(DirtyFlag_SetOnThresholdChange) {
    auto& config = ToonShaderConfig::instance();
    config.clearDirtyFlag();

    config.setBandThreshold(0, 0.1f);
    ASSERT_TRUE(config.isDirty());
}

TEST(DirtyFlag_SetOnBloomChange) {
    auto& config = ToonShaderConfig::instance();
    config.clearDirtyFlag();

    config.setBloomIntensity(0.5f);
    ASSERT_TRUE(config.isDirty());
}

TEST(DirtyFlag_SetOnEmissiveChange) {
    auto& config = ToonShaderConfig::instance();
    config.clearDirtyFlag();

    config.setEmissiveMultiplier(0.5f);
    ASSERT_TRUE(config.isDirty());
}

TEST(DirtyFlag_SetOnAmbientChange) {
    auto& config = ToonShaderConfig::instance();
    config.clearDirtyFlag();

    config.setAmbientLevel(0.1f);
    ASSERT_TRUE(config.isDirty());
}

TEST(DirtyFlag_NotSetOnSameValue) {
    auto& config = ToonShaderConfig::instance();
    float originalAmbient = config.getAmbientLevel();

    config.clearDirtyFlag();
    config.setAmbientLevel(originalAmbient);  // Same value

    ASSERT_FALSE(config.isDirty());
}

// =============================================================================
// Preset Tests
// =============================================================================

TEST(ResetToDefaults_RestoresAllValues) {
    auto& config = ToonShaderConfig::instance();

    // Modify values
    config.setBandCount(2);
    config.setBloomIntensity(0.5f);
    config.setAmbientLevel(0.15f);

    // Reset
    config.resetToDefaults();

    // Verify defaults restored
    ASSERT_EQ(4, config.getBandCount());
    ASSERT_FLOAT_EQ(1.0f, config.getBloomIntensity(), 0.001f);
    ASSERT_FLOAT_EQ(0.08f, config.getAmbientLevel(), 0.001f);
}

TEST(DayPalette_AdjustsValues) {
    auto& config = ToonShaderConfig::instance();

    config.applyDayPalette();

    // Day should have brighter ambient
    ASSERT_TRUE(config.getAmbientLevel() > 0.08f);
    // Day should have reduced emissive
    ASSERT_TRUE(config.getEmissiveMultiplier() < 1.0f);
}

TEST(NightPalette_AdjustsValues) {
    auto& config = ToonShaderConfig::instance();

    config.applyNightPalette();

    // Night should have darker ambient
    ASSERT_TRUE(config.getAmbientLevel() < 0.08f);
    // Night should have enhanced emissive
    ASSERT_TRUE(config.getEmissiveMultiplier() > 1.0f);
    // Night should have stronger bloom
    ASSERT_TRUE(config.getBloomIntensity() > 1.0f);
}

TEST(HighContrastPreset_AdjustsValues) {
    auto& config = ToonShaderConfig::instance();

    config.applyHighContrastPreset();

    // High contrast should have thicker edges
    ASSERT_TRUE(config.getEdgeLineWidth() > 1.0f);
    // Should have higher ambient for visibility
    ASSERT_TRUE(config.getAmbientLevel() >= 0.1f);
}

// =============================================================================
// Default Value Constants Tests
// =============================================================================

TEST(DefaultConstants_AmbientRange) {
    ASSERT_FLOAT_EQ(0.05f, ToonShaderConfigDefaults::AMBIENT_LEVEL_MIN, 0.001f);
    ASSERT_FLOAT_EQ(0.1f, ToonShaderConfigDefaults::AMBIENT_LEVEL_MAX, 0.001f);
    ASSERT_TRUE(ToonShaderConfigDefaults::AMBIENT_LEVEL >= ToonShaderConfigDefaults::AMBIENT_LEVEL_MIN);
    ASSERT_TRUE(ToonShaderConfigDefaults::AMBIENT_LEVEL <= ToonShaderConfigDefaults::AMBIENT_LEVEL_MAX);
}

TEST(DefaultConstants_ShadowColor) {
    // #2A1B3D = 42, 27, 61
    ASSERT_FLOAT_EQ(42.0f / 255.0f, ToonShaderConfigDefaults::SHADOW_COLOR_R, 0.001f);
    ASSERT_FLOAT_EQ(27.0f / 255.0f, ToonShaderConfigDefaults::SHADOW_COLOR_G, 0.001f);
    ASSERT_FLOAT_EQ(61.0f / 255.0f, ToonShaderConfigDefaults::SHADOW_COLOR_B, 0.001f);
}

TEST(DefaultConstants_BandThresholdsAscending) {
    ASSERT_TRUE(ToonShaderConfigDefaults::BAND_THRESHOLD_0 < ToonShaderConfigDefaults::BAND_THRESHOLD_1);
    ASSERT_TRUE(ToonShaderConfigDefaults::BAND_THRESHOLD_1 < ToonShaderConfigDefaults::BAND_THRESHOLD_2);
    ASSERT_TRUE(ToonShaderConfigDefaults::BAND_THRESHOLD_2 < ToonShaderConfigDefaults::BAND_THRESHOLD_3);
}

TEST(DefaultConstants_BandIntensitiesAscending) {
    ASSERT_TRUE(ToonShaderConfigDefaults::BAND_INTENSITY_0 < ToonShaderConfigDefaults::BAND_INTENSITY_1);
    ASSERT_TRUE(ToonShaderConfigDefaults::BAND_INTENSITY_1 < ToonShaderConfigDefaults::BAND_INTENSITY_2);
    ASSERT_TRUE(ToonShaderConfigDefaults::BAND_INTENSITY_2 < ToonShaderConfigDefaults::BAND_INTENSITY_3);
}

// =============================================================================
// Main
// =============================================================================

int main() {
    printf("=== ToonShaderConfig Unit Tests ===\n\n");

    // Singleton tests
    RUN_TEST(Singleton_SameInstance);

    // Band configuration tests
    RUN_TEST(BandCount_Default);
    RUN_TEST(BandCount_SetValid);
    RUN_TEST(BandCount_ClampMin);
    RUN_TEST(BandCount_ClampMax);
    RUN_TEST(BandThreshold_Defaults);
    RUN_TEST(BandThreshold_SetValid);
    RUN_TEST(BandThreshold_ClampRange);
    RUN_TEST(BandIntensity_Defaults);
    RUN_TEST(BandIntensity_SetValid);
    RUN_TEST(GetBands_ReturnsArray);

    // Shadow configuration tests
    RUN_TEST(ShadowColor_Default);
    RUN_TEST(ShadowColor_SetValid);
    RUN_TEST(ShadowShiftAmount_Default);
    RUN_TEST(ShadowShiftAmount_SetValid);
    RUN_TEST(ShadowShiftAmount_ClampRange);

    // Edge configuration tests
    RUN_TEST(EdgeLineWidth_Default);
    RUN_TEST(EdgeLineWidth_SetValid);
    RUN_TEST(EdgeLineWidth_ClampRange);

    // Bloom configuration tests
    RUN_TEST(BloomThreshold_Default);
    RUN_TEST(BloomThreshold_SetValid);
    RUN_TEST(BloomThreshold_ClampRange);
    RUN_TEST(BloomIntensity_Default);
    RUN_TEST(BloomIntensity_SetValid);
    RUN_TEST(BloomIntensity_ClampRange);

    // Emissive configuration tests
    RUN_TEST(EmissiveMultiplier_Default);
    RUN_TEST(EmissiveMultiplier_SetValid);
    RUN_TEST(EmissiveMultiplier_ClampRange);

    // Terrain emissive preset tests
    RUN_TEST(TerrainEmissivePresets_Count);
    RUN_TEST(TerrainEmissivePresets_AllInitialized);
    RUN_TEST(TerrainEmissivePresets_CrystalFields);
    RUN_TEST(TerrainEmissivePresets_VolcanicRock);
    RUN_TEST(TerrainEmissivePresets_SetCustom);
    RUN_TEST(TerrainEmissivePresets_InvalidType);

    // Ambient configuration tests
    RUN_TEST(AmbientLevel_Default);
    RUN_TEST(AmbientLevel_SetValid);
    RUN_TEST(AmbientLevel_ClampRange);
    RUN_TEST(AmbientLevel_DefaultInRecommendedRange);

    // Dirty flag tests
    RUN_TEST(DirtyFlag_InitiallyDirty);
    RUN_TEST(DirtyFlag_ClearAfterRead);
    RUN_TEST(DirtyFlag_SetOnBandCountChange);
    RUN_TEST(DirtyFlag_SetOnThresholdChange);
    RUN_TEST(DirtyFlag_SetOnBloomChange);
    RUN_TEST(DirtyFlag_SetOnEmissiveChange);
    RUN_TEST(DirtyFlag_SetOnAmbientChange);
    RUN_TEST(DirtyFlag_NotSetOnSameValue);

    // Preset tests
    RUN_TEST(ResetToDefaults_RestoresAllValues);
    RUN_TEST(DayPalette_AdjustsValues);
    RUN_TEST(NightPalette_AdjustsValues);
    RUN_TEST(HighContrastPreset_AdjustsValues);

    // Default constants tests
    RUN_TEST(DefaultConstants_AmbientRange);
    RUN_TEST(DefaultConstants_ShadowColor);
    RUN_TEST(DefaultConstants_BandThresholdsAscending);
    RUN_TEST(DefaultConstants_BandIntensitiesAscending);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", g_testsPassed);
    printf("Failed: %d\n", g_testsFailed);

    return g_testsFailed == 0 ? 0 : 1;
}
