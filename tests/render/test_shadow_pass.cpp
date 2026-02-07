/**
 * @file test_shadow_pass.cpp
 * @brief Unit tests for ShadowConfig and ShadowPass (non-GPU portions).
 *
 * Tests cover:
 * - ShadowConfig defaults and quality presets
 * - Shadow map resolution calculations
 * - PCF sample count per quality tier
 * - Quality preset application
 * - Configuration enable/disable logic
 *
 * Note: GPU-dependent tests require manual verification.
 * These tests focus on configuration structs and pure functions.
 */

#include "sims3000/render/ShadowConfig.h"
#include <cstdio>
#include <cmath>
#include <cstring>

static int s_testsPassed = 0;
static int s_testsFailed = 0;

#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            printf("FAIL: %s (line %d): %s\n", __FUNCTION__, __LINE__, message); \
            s_testsFailed++; \
            return; \
        } \
    } while(0)

#define TEST_ASSERT_EQ(actual, expected, message) \
    do { \
        if ((actual) != (expected)) { \
            printf("FAIL: %s (line %d): %s (expected %d, got %d)\n", \
                   __FUNCTION__, __LINE__, message, (int)(expected), (int)(actual)); \
            s_testsFailed++; \
            return; \
        } \
    } while(0)

#define TEST_ASSERT_FLOAT_EQ(actual, expected, epsilon, message) \
    do { \
        if (std::abs((actual) - (expected)) > (epsilon)) { \
            printf("FAIL: %s (line %d): %s (expected %.4f, got %.4f)\n", \
                   __FUNCTION__, __LINE__, message, (float)(expected), (float)(actual)); \
            s_testsFailed++; \
            return; \
        } \
    } while(0)

#define TEST_PASS() \
    do { \
        printf("PASS: %s\n", __FUNCTION__); \
        s_testsPassed++; \
    } while(0)

using namespace sims3000;

// =============================================================================
// ShadowConfig Tests
// =============================================================================

void test_ShadowConfig_DefaultValues() {
    ShadowConfig config;

    // Check default quality
    TEST_ASSERT_EQ(config.quality, ShadowQuality::High, "Default quality should be High");
    TEST_ASSERT(config.enabled, "Should be enabled by default");

    // Check default light direction (normalized (1, 2, 1))
    TEST_ASSERT_FLOAT_EQ(config.lightDirection.x, 0.408248f, 0.001f, "Light direction X");
    TEST_ASSERT_FLOAT_EQ(config.lightDirection.y, 0.816497f, 0.001f, "Light direction Y");
    TEST_ASSERT_FLOAT_EQ(config.lightDirection.z, 0.408248f, 0.001f, "Light direction Z");

    // Check default shadow color (purple #2A1B3D)
    TEST_ASSERT_FLOAT_EQ(config.shadowColor.r, 0.165f, 0.01f, "Shadow color R");
    TEST_ASSERT_FLOAT_EQ(config.shadowColor.g, 0.106f, 0.01f, "Shadow color G");
    TEST_ASSERT_FLOAT_EQ(config.shadowColor.b, 0.239f, 0.01f, "Shadow color B");

    // Check default shadow intensity
    TEST_ASSERT_FLOAT_EQ(config.shadowIntensity, 0.6f, 0.001f, "Default shadow intensity");

    // Check texel snapping enabled by default
    TEST_ASSERT(config.texelSnapping, "Texel snapping should be enabled by default");

    TEST_PASS();
}

void test_ShadowConfig_QualityResolutions() {
    ShadowConfig config;

    // Test Disabled
    config.quality = ShadowQuality::Disabled;
    TEST_ASSERT_EQ(config.getShadowMapResolution(), 0u, "Disabled quality resolution");

    // Test Low
    config.quality = ShadowQuality::Low;
    TEST_ASSERT_EQ(config.getShadowMapResolution(), 512u, "Low quality resolution");

    // Test Medium
    config.quality = ShadowQuality::Medium;
    TEST_ASSERT_EQ(config.getShadowMapResolution(), 1024u, "Medium quality resolution");

    // Test High
    config.quality = ShadowQuality::High;
    TEST_ASSERT_EQ(config.getShadowMapResolution(), 2048u, "High quality resolution");

    // Test Ultra
    config.quality = ShadowQuality::Ultra;
    TEST_ASSERT_EQ(config.getShadowMapResolution(), 4096u, "Ultra quality resolution");

    TEST_PASS();
}

void test_ShadowConfig_PCFSampleCounts() {
    ShadowConfig config;

    // Test Disabled
    config.quality = ShadowQuality::Disabled;
    TEST_ASSERT_EQ(config.getPCFSampleCount(), 0u, "Disabled PCF samples");

    // Test Low (no PCF)
    config.quality = ShadowQuality::Low;
    TEST_ASSERT_EQ(config.getPCFSampleCount(), 1u, "Low PCF samples");

    // Test Medium (2x2)
    config.quality = ShadowQuality::Medium;
    TEST_ASSERT_EQ(config.getPCFSampleCount(), 4u, "Medium PCF samples");

    // Test High (3x3)
    config.quality = ShadowQuality::High;
    TEST_ASSERT_EQ(config.getPCFSampleCount(), 9u, "High PCF samples");

    // Test Ultra (4x4)
    config.quality = ShadowQuality::Ultra;
    TEST_ASSERT_EQ(config.getPCFSampleCount(), 16u, "Ultra PCF samples");

    TEST_PASS();
}

void test_ShadowConfig_IsEnabled() {
    ShadowConfig config;

    // Enabled and High quality = enabled
    config.enabled = true;
    config.quality = ShadowQuality::High;
    TEST_ASSERT(config.isEnabled(), "Should be enabled with High quality");

    // Enabled but Disabled quality = not enabled
    config.quality = ShadowQuality::Disabled;
    TEST_ASSERT(!config.isEnabled(), "Should not be enabled with Disabled quality");

    // Not enabled but High quality = not enabled
    config.enabled = false;
    config.quality = ShadowQuality::High;
    TEST_ASSERT(!config.isEnabled(), "Should not be enabled when disabled flag is false");

    TEST_PASS();
}

void test_ShadowConfig_QualityPresets() {
    ShadowConfig config;

    // Apply Low preset
    config.applyQualityPreset(ShadowQuality::Low);
    TEST_ASSERT_EQ(config.quality, ShadowQuality::Low, "Quality should be Low after preset");
    TEST_ASSERT(config.enabled, "Should still be enabled after Low preset");
    TEST_ASSERT_FLOAT_EQ(config.shadowSoftness, 0.0f, 0.001f, "Low preset should have hard shadows");

    // Apply High preset
    config.applyQualityPreset(ShadowQuality::High);
    TEST_ASSERT_EQ(config.quality, ShadowQuality::High, "Quality should be High after preset");
    TEST_ASSERT_FLOAT_EQ(config.shadowSoftness, 0.2f, 0.001f, "High preset shadow softness");
    TEST_ASSERT_FLOAT_EQ(config.depthBias, 0.0005f, 0.0001f, "High preset depth bias");

    // Apply Disabled preset
    config.applyQualityPreset(ShadowQuality::Disabled);
    TEST_ASSERT_EQ(config.quality, ShadowQuality::Disabled, "Quality should be Disabled after preset");
    TEST_ASSERT(!config.enabled, "Should be disabled after Disabled preset");

    // Apply Ultra preset
    config.applyQualityPreset(ShadowQuality::Ultra);
    TEST_ASSERT_EQ(config.quality, ShadowQuality::Ultra, "Quality should be Ultra after preset");
    TEST_ASSERT(config.enabled, "Should be enabled after Ultra preset");
    TEST_ASSERT_FLOAT_EQ(config.shadowSoftness, 0.25f, 0.001f, "Ultra preset shadow softness");

    TEST_PASS();
}

void test_ShadowConfig_ResetToDefaults() {
    ShadowConfig config;

    // Modify values
    config.quality = ShadowQuality::Low;
    config.enabled = false;
    config.shadowIntensity = 0.1f;
    config.depthBias = 0.01f;

    // Reset
    config.resetToDefaults();

    // Verify defaults restored
    TEST_ASSERT_EQ(config.quality, ShadowQuality::High, "Quality should be High after reset");
    TEST_ASSERT(config.enabled, "Should be enabled after reset");
    TEST_ASSERT_FLOAT_EQ(config.shadowIntensity, 0.6f, 0.001f, "Shadow intensity after reset");
    TEST_ASSERT_FLOAT_EQ(config.depthBias, 0.0005f, 0.0001f, "Depth bias after reset");

    TEST_PASS();
}

void test_ShadowConfig_FrustumSettings() {
    ShadowConfig config;

    // Check default frustum settings
    TEST_ASSERT_FLOAT_EQ(config.frustumPadding, 5.0f, 0.001f, "Default frustum padding");
    TEST_ASSERT_FLOAT_EQ(config.minFrustumSize, 50.0f, 0.001f, "Default min frustum size");
    TEST_ASSERT_FLOAT_EQ(config.maxFrustumSize, 500.0f, 0.001f, "Default max frustum size");

    TEST_PASS();
}

void test_ShadowConfig_BiasSettings() {
    ShadowConfig config;

    // Check default bias values
    TEST_ASSERT_FLOAT_EQ(config.depthBias, 0.0005f, 0.0001f, "Default depth bias");
    TEST_ASSERT_FLOAT_EQ(config.slopeBias, 0.002f, 0.0001f, "Default slope bias");
    TEST_ASSERT_FLOAT_EQ(config.normalBias, 0.02f, 0.001f, "Default normal bias");

    TEST_PASS();
}

void test_ShadowQualityName() {
    TEST_ASSERT(strcmp(getShadowQualityName(ShadowQuality::Disabled), "Disabled") == 0,
                "Disabled name");
    TEST_ASSERT(strcmp(getShadowQualityName(ShadowQuality::Low), "Low") == 0,
                "Low name");
    TEST_ASSERT(strcmp(getShadowQualityName(ShadowQuality::Medium), "Medium") == 0,
                "Medium name");
    TEST_ASSERT(strcmp(getShadowQualityName(ShadowQuality::High), "High") == 0,
                "High name");
    TEST_ASSERT(strcmp(getShadowQualityName(ShadowQuality::Ultra), "Ultra") == 0,
                "Ultra name");

    TEST_PASS();
}

void test_ShadowConfig_DarkEnvironmentTuning() {
    ShadowConfig config;

    // Shadows should be visible but not harsh in dark environment
    // Default intensity of 0.6 means 60% shadow darkness
    TEST_ASSERT(config.shadowIntensity >= 0.5f && config.shadowIntensity <= 0.7f,
                "Shadow intensity should be moderate for dark environment");

    // Shadow softness should be low for toon-appropriate clean edges
    TEST_ASSERT(config.shadowSoftness < 0.5f,
                "Shadow softness should be low for clean toon edges");

    // Shadow color should be purple per alien aesthetic
    TEST_ASSERT(config.shadowColor.b > config.shadowColor.r,
                "Shadow color should have purple tint (more blue than red)");

    TEST_PASS();
}

// =============================================================================
// Main
// =============================================================================

int main() {
    printf("=== ShadowPass Unit Tests ===\n\n");

    // ShadowConfig tests
    test_ShadowConfig_DefaultValues();
    test_ShadowConfig_QualityResolutions();
    test_ShadowConfig_PCFSampleCounts();
    test_ShadowConfig_IsEnabled();
    test_ShadowConfig_QualityPresets();
    test_ShadowConfig_ResetToDefaults();
    test_ShadowConfig_FrustumSettings();
    test_ShadowConfig_BiasSettings();
    test_ShadowQualityName();
    test_ShadowConfig_DarkEnvironmentTuning();

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", s_testsPassed);
    printf("Failed: %d\n", s_testsFailed);

    return s_testsFailed > 0 ? 1 : 0;
}
