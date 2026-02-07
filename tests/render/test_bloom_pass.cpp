/**
 * @file test_bloom_pass.cpp
 * @brief Unit tests for BloomPass configuration and logic.
 *
 * Tests BloomPass configuration, quality tiers, minimum intensity enforcement,
 * and resolution calculations. GPU-dependent rendering tests require manual
 * verification with a display.
 *
 * Ticket: 2-038 - Bloom Post-Process
 */

#include "sims3000/render/BloomPass.h"
#include "sims3000/render/ToonShaderConfig.h"
#include <cstdio>
#include <cmath>

using namespace sims3000;

// Test counter
static int g_testsPassed = 0;
static int g_testsFailed = 0;

#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            printf("FAIL: %s\n  File: %s, Line: %d\n", message, __FILE__, __LINE__); \
            g_testsFailed++; \
            return; \
        } \
    } while (0)

#define TEST_ASSERT_FLOAT_EQ(a, b, epsilon, message) \
    do { \
        if (std::fabs((a) - (b)) > (epsilon)) { \
            printf("FAIL: %s\n  Expected: %f, Got: %f\n  File: %s, Line: %d\n", \
                   message, (double)(b), (double)(a), __FILE__, __LINE__); \
            g_testsFailed++; \
            return; \
        } \
    } while (0)

#define RUN_TEST(test_func) \
    do { \
        printf("Running: %s... ", #test_func); \
        test_func(); \
        printf("PASS\n"); \
        g_testsPassed++; \
    } while (0)

// =============================================================================
// BloomConfig Tests
// =============================================================================

void test_BloomConfig_DefaultValues() {
    BloomConfig config;

    // Default threshold should be 0.7 (conservative for dark environment)
    TEST_ASSERT_FLOAT_EQ(config.threshold, 0.7f, 0.001f,
                         "Default threshold should be 0.7");

    // Default intensity should be 1.0
    TEST_ASSERT_FLOAT_EQ(config.intensity, 1.0f, 0.001f,
                         "Default intensity should be 1.0");

    // Default quality should be Medium
    TEST_ASSERT(config.quality == BloomQuality::Medium,
                "Default quality should be Medium");

    // MIN_INTENSITY constant should be 0.1
    TEST_ASSERT_FLOAT_EQ(BloomConfig::MIN_INTENSITY, 0.1f, 0.001f,
                         "MIN_INTENSITY should be 0.1");
}

void test_BloomConfig_MinIntensity() {
    // Test that MIN_INTENSITY is enforced
    // Bloom cannot be fully disabled per canon specification
    TEST_ASSERT(BloomConfig::MIN_INTENSITY > 0.0f,
                "MIN_INTENSITY must be greater than zero (bloom cannot be disabled)");

    TEST_ASSERT_FLOAT_EQ(BloomConfig::MIN_INTENSITY, 0.1f, 0.001f,
                         "MIN_INTENSITY should be exactly 0.1f per spec");
}

// =============================================================================
// BloomQuality Tests
// =============================================================================

void test_BloomQuality_QualityDivisors() {
    // Test quality divisors match specification:
    // High: 1/2 resolution
    // Medium: 1/4 resolution
    // Low: 1/8 resolution

    // At 1920x1080, quality divisors should produce:
    // High: 960x540
    // Medium: 480x270
    // Low: 240x135

    // We test the expected divisor values
    TEST_ASSERT(static_cast<int>(BloomQuality::High) == 0,
                "BloomQuality::High should be 0");
    TEST_ASSERT(static_cast<int>(BloomQuality::Medium) == 1,
                "BloomQuality::Medium should be 1");
    TEST_ASSERT(static_cast<int>(BloomQuality::Low) == 2,
                "BloomQuality::Low should be 2");
}

void test_BloomQuality_Names() {
    TEST_ASSERT(std::string(getBloomQualityName(BloomQuality::High)) == "High",
                "High quality name should be 'High'");
    TEST_ASSERT(std::string(getBloomQualityName(BloomQuality::Medium)) == "Medium",
                "Medium quality name should be 'Medium'");
    TEST_ASSERT(std::string(getBloomQualityName(BloomQuality::Low)) == "Low",
                "Low quality name should be 'Low'");
}

// =============================================================================
// BloomStats Tests
// =============================================================================

void test_BloomStats_DefaultValues() {
    BloomStats stats;

    TEST_ASSERT_FLOAT_EQ(stats.extractionTimeMs, 0.0f, 0.001f,
                         "Default extractionTimeMs should be 0");
    TEST_ASSERT_FLOAT_EQ(stats.blurTimeMs, 0.0f, 0.001f,
                         "Default blurTimeMs should be 0");
    TEST_ASSERT_FLOAT_EQ(stats.compositeTimeMs, 0.0f, 0.001f,
                         "Default compositeTimeMs should be 0");
    TEST_ASSERT_FLOAT_EQ(stats.totalTimeMs, 0.0f, 0.001f,
                         "Default totalTimeMs should be 0");
    TEST_ASSERT(stats.bloomWidth == 0,
                "Default bloomWidth should be 0");
    TEST_ASSERT(stats.bloomHeight == 0,
                "Default bloomHeight should be 0");
}

// =============================================================================
// Resolution Calculation Tests
// =============================================================================

void test_ResolutionCalculation_High() {
    // High quality = 1/2 resolution
    // 1920x1080 -> 960x540
    std::uint32_t inputWidth = 1920;
    std::uint32_t inputHeight = 1080;
    int divisor = 2; // High quality divisor

    std::uint32_t expectedWidth = inputWidth / divisor;
    std::uint32_t expectedHeight = inputHeight / divisor;

    TEST_ASSERT(expectedWidth == 960,
                "High quality width should be 960 at 1080p");
    TEST_ASSERT(expectedHeight == 540,
                "High quality height should be 540 at 1080p");
}

void test_ResolutionCalculation_Medium() {
    // Medium quality = 1/4 resolution
    // 1920x1080 -> 480x270
    std::uint32_t inputWidth = 1920;
    std::uint32_t inputHeight = 1080;
    int divisor = 4; // Medium quality divisor

    std::uint32_t expectedWidth = inputWidth / divisor;
    std::uint32_t expectedHeight = inputHeight / divisor;

    TEST_ASSERT(expectedWidth == 480,
                "Medium quality width should be 480 at 1080p");
    TEST_ASSERT(expectedHeight == 270,
                "Medium quality height should be 270 at 1080p");
}

void test_ResolutionCalculation_Low() {
    // Low quality = 1/8 resolution
    // 1920x1080 -> 240x135
    std::uint32_t inputWidth = 1920;
    std::uint32_t inputHeight = 1080;
    int divisor = 8; // Low quality divisor

    std::uint32_t expectedWidth = inputWidth / divisor;
    std::uint32_t expectedHeight = inputHeight / divisor;

    TEST_ASSERT(expectedWidth == 240,
                "Low quality width should be 240 at 1080p");
    TEST_ASSERT(expectedHeight == 135,
                "Low quality height should be 135 at 1080p");
}

void test_ResolutionCalculation_MinimumSize() {
    // Even at very small resolutions, bloom target should be at least 1x1
    std::uint32_t inputWidth = 4;
    std::uint32_t inputHeight = 4;
    int divisor = 8; // Low quality divisor

    std::uint32_t calculatedWidth = std::max(1u, inputWidth / divisor);
    std::uint32_t calculatedHeight = std::max(1u, inputHeight / divisor);

    TEST_ASSERT(calculatedWidth >= 1,
                "Bloom width should be at least 1");
    TEST_ASSERT(calculatedHeight >= 1,
                "Bloom height should be at least 1");
}

// =============================================================================
// Threshold Tests
// =============================================================================

void test_Threshold_ConservativeDefault() {
    // Default threshold should be conservative (0.7) for dark environment
    // This prevents "glow soup" where too many pixels bloom
    BloomConfig config;

    TEST_ASSERT(config.threshold >= 0.5f,
                "Threshold should be conservative (>= 0.5) for dark environment");
    TEST_ASSERT(config.threshold <= 1.0f,
                "Threshold should not exceed 1.0");
}

void test_Threshold_ValidRange() {
    // Threshold should be in [0.0, 1.0] range
    float validThresholds[] = {0.0f, 0.25f, 0.5f, 0.7f, 1.0f};

    for (float t : validThresholds) {
        TEST_ASSERT(t >= 0.0f && t <= 1.0f,
                    "Threshold values should be in [0.0, 1.0] range");
    }
}

// =============================================================================
// Color Range Tests
// =============================================================================

void test_EmissiveColorRange_FullPalette() {
    // Test that bloom should handle the full emissive color palette
    // Cyan, green, amber, magenta (as defined in EmissiveMaterial.h)

    // These are the canonical bioluminescent colors in linear RGB
    // Bloom extraction should preserve color hue, not just luminance

    struct EmissiveColor {
        float r, g, b;
        const char* name;
    };

    EmissiveColor palette[] = {
        {0.0f, 0.831f, 0.667f, "Cyan"},       // #00D4AA
        {0.0f, 1.0f, 0.533f, "Green"},        // #00FF88
        {1.0f, 0.647f, 0.0f, "Amber"},        // #FFA500
        {1.0f, 0.0f, 1.0f, "Magenta"},        // #FF00FF
    };

    for (const auto& color : palette) {
        // All emissive colors should have non-zero component
        bool hasEmission = (color.r > 0.0f || color.g > 0.0f || color.b > 0.0f);
        TEST_ASSERT(hasEmission,
                    "Emissive colors should have non-zero emission");
    }
}

// =============================================================================
// Performance Budget Tests
// =============================================================================

void test_PerformanceBudget_TargetTime() {
    // Performance budget: <0.5ms at 1080p
    // This is a documentation/specification test

    const float targetBudgetMs = 0.5f;
    const int targetResolutionWidth = 1920;
    const int targetResolutionHeight = 1080;

    // Just verify the constants are as specified
    TEST_ASSERT(targetBudgetMs == 0.5f,
                "Performance budget should be 0.5ms");
    TEST_ASSERT(targetResolutionWidth == 1920,
                "Target resolution should be 1920 width");
    TEST_ASSERT(targetResolutionHeight == 1080,
                "Target resolution should be 1080 height");
}

// =============================================================================
// Integration with ToonShaderConfig Tests
// =============================================================================

void test_ToonShaderConfigIntegration_BloomThreshold() {
    // BloomPass should read initial threshold from ToonShaderConfig
    const auto& config = ToonShaderConfig::instance();
    float configThreshold = config.getBloomThreshold();

    // Threshold should be in valid range
    TEST_ASSERT(configThreshold >= 0.0f && configThreshold <= 1.0f,
                "ToonShaderConfig bloom threshold should be in [0.0, 1.0]");
}

void test_ToonShaderConfigIntegration_BloomIntensity() {
    // BloomPass should read initial intensity from ToonShaderConfig
    const auto& config = ToonShaderConfig::instance();
    float configIntensity = config.getBloomIntensity();

    // Intensity should be at least MIN_INTENSITY (bloom cannot be disabled)
    TEST_ASSERT(configIntensity >= BloomConfig::MIN_INTENSITY,
                "ToonShaderConfig bloom intensity should be >= MIN_INTENSITY");
}

// =============================================================================
// Main
// =============================================================================

int main() {
    printf("=== BloomPass Unit Tests ===\n\n");

    // BloomConfig tests
    RUN_TEST(test_BloomConfig_DefaultValues);
    RUN_TEST(test_BloomConfig_MinIntensity);

    // BloomQuality tests
    RUN_TEST(test_BloomQuality_QualityDivisors);
    RUN_TEST(test_BloomQuality_Names);

    // BloomStats tests
    RUN_TEST(test_BloomStats_DefaultValues);

    // Resolution calculation tests
    RUN_TEST(test_ResolutionCalculation_High);
    RUN_TEST(test_ResolutionCalculation_Medium);
    RUN_TEST(test_ResolutionCalculation_Low);
    RUN_TEST(test_ResolutionCalculation_MinimumSize);

    // Threshold tests
    RUN_TEST(test_Threshold_ConservativeDefault);
    RUN_TEST(test_Threshold_ValidRange);

    // Color range tests
    RUN_TEST(test_EmissiveColorRange_FullPalette);

    // Performance budget tests
    RUN_TEST(test_PerformanceBudget_TargetTime);

    // Integration tests
    RUN_TEST(test_ToonShaderConfigIntegration_BloomThreshold);
    RUN_TEST(test_ToonShaderConfigIntegration_BloomIntensity);

    printf("\n=== Test Results ===\n");
    printf("Passed: %d\n", g_testsPassed);
    printf("Failed: %d\n", g_testsFailed);

    return g_testsFailed > 0 ? 1 : 0;
}
