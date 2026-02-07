/**
 * @file test_lod_system.cpp
 * @brief Unit tests for LODSystem - distance-based LOD selection.
 *
 * Tests cover:
 * - LOD level selection based on distance thresholds
 * - 2+ LOD levels configuration
 * - Configurable distance thresholds
 * - Hysteresis to prevent pop-in
 * - Crossfade blending calculations
 * - Framework extensibility
 * - Performance with 512x512 map entity counts
 */

#include "sims3000/render/LODSystem.h"

#include <iostream>
#include <string>
#include <cmath>
#include <chrono>
#include <vector>

// Simple test framework
static int g_testsRun = 0;
static int g_testsPassed = 0;

#define TEST_ASSERT(condition, message) \
    do { \
        g_testsRun++; \
        if (!(condition)) { \
            std::cerr << "FAIL: " << message << " (" << __FILE__ << ":" << __LINE__ << ")\n"; \
        } else { \
            g_testsPassed++; \
        } \
    } while (0)

#define TEST_ASSERT_FLOAT_EQ(a, b, epsilon, message) \
    TEST_ASSERT(std::abs((a) - (b)) < (epsilon), message)

using namespace sims3000;

// =============================================================================
// Test: LODConfig Validation
// =============================================================================

void test_LODConfig_Default2Level() {
    std::cout << "test_LODConfig_Default2Level... ";

    LODConfig config = LODConfig::createDefault2Level();

    TEST_ASSERT(config.isValid(), "Default 2-level config should be valid");
    TEST_ASSERT(config.getLevelCount() == 2, "Should have 2 LOD levels");
    TEST_ASSERT(config.thresholds.size() == 1, "Should have 1 threshold");
    TEST_ASSERT_FLOAT_EQ(config.thresholds[0].distance, 50.0f, 0.01f, "First threshold at 50m");

    std::cout << "PASSED\n";
}

void test_LODConfig_Default3Level() {
    std::cout << "test_LODConfig_Default3Level... ";

    LODConfig config = LODConfig::createDefault3Level();

    TEST_ASSERT(config.isValid(), "Default 3-level config should be valid");
    TEST_ASSERT(config.getLevelCount() == 3, "Should have 3 LOD levels");
    TEST_ASSERT(config.thresholds.size() == 2, "Should have 2 thresholds");
    TEST_ASSERT_FLOAT_EQ(config.thresholds[0].distance, 50.0f, 0.01f, "First threshold at 50m");
    TEST_ASSERT_FLOAT_EQ(config.thresholds[1].distance, 150.0f, 0.01f, "Second threshold at 150m");

    std::cout << "PASSED\n";
}

void test_LODConfig_InvalidOrder() {
    std::cout << "test_LODConfig_InvalidOrder... ";

    LODConfig config;
    config.thresholds.push_back({100.0f, 2.0f});  // 100m
    config.thresholds.push_back({50.0f, 2.0f});   // 50m (wrong order!)

    TEST_ASSERT(!config.isValid(), "Non-ascending thresholds should be invalid");

    std::cout << "PASSED\n";
}

void test_LODConfig_TooManyLevels() {
    std::cout << "test_LODConfig_TooManyLevels... ";

    LODConfig config;
    // Add more thresholds than MAX_LOD_LEVELS - 1
    for (int i = 0; i < 10; ++i) {
        config.thresholds.push_back({static_cast<float>(i + 1) * 10.0f, 2.0f});
    }

    TEST_ASSERT(!config.isValid(), "Too many LOD levels should be invalid");

    std::cout << "PASSED\n";
}

// =============================================================================
// Test: LOD Selection with Default Config
// =============================================================================

void test_LODSelection_ClosestDistance() {
    std::cout << "test_LODSelection_ClosestDistance... ";

    LODSystem system;
    // Default 2-level: LOD 0 < 50m, LOD 1 >= 50m

    LODResult result = system.selectLODDefault(10.0f);  // 10m from camera

    TEST_ASSERT(result.level == 0, "Should select LOD 0 at 10m");
    TEST_ASSERT(!result.isBlending, "Should not be blending");

    std::cout << "PASSED\n";
}

void test_LODSelection_MediumDistance() {
    std::cout << "test_LODSelection_MediumDistance... ";

    LODSystem system;
    // Default 2-level: LOD 0 < 50m, LOD 1 >= 50m

    LODResult result = system.selectLODDefault(75.0f);  // 75m from camera

    TEST_ASSERT(result.level == 1, "Should select LOD 1 at 75m");
    TEST_ASSERT(!result.isBlending, "Should not be blending (aggressive mode)");

    std::cout << "PASSED\n";
}

void test_LODSelection_FarDistance() {
    std::cout << "test_LODSelection_FarDistance... ";

    LODSystem system;

    // Use 3-level config
    system.setDefaultConfig(LODConfig::createDefault3Level());
    // LOD 0 < 50m, LOD 1 50-150m, LOD 2 >= 150m

    LODResult result = system.selectLODDefault(200.0f);  // 200m from camera

    TEST_ASSERT(result.level == 2, "Should select LOD 2 at 200m");

    std::cout << "PASSED\n";
}

void test_LODSelection_AtThreshold() {
    std::cout << "test_LODSelection_AtThreshold... ";

    LODSystem system;
    // Default 2-level: LOD 0 < 50m, LOD 1 >= 50m

    // Exactly at threshold should go to next level
    LODResult result = system.selectLODDefault(50.0f);

    TEST_ASSERT(result.level == 1, "Should select LOD 1 at exactly 50m");

    std::cout << "PASSED\n";
}

void test_LODSelection_JustBeforeThreshold() {
    std::cout << "test_LODSelection_JustBeforeThreshold... ";

    LODSystem system;

    LODResult result = system.selectLODDefault(49.9f);

    TEST_ASSERT(result.level == 0, "Should select LOD 0 just before 50m");

    std::cout << "PASSED\n";
}

// =============================================================================
// Test: Per-Model Configuration
// =============================================================================

void test_LODSelection_PerModelConfig() {
    std::cout << "test_LODSelection_PerModelConfig... ";

    LODSystem system;

    // Configure model type 1 with custom thresholds
    LODConfig customConfig;
    customConfig.thresholds.push_back({30.0f, 2.0f});   // LOD 0 < 30m
    customConfig.thresholds.push_back({100.0f, 2.0f});  // LOD 1 30-100m, LOD 2 >= 100m

    bool success = system.setConfig(1, customConfig);
    TEST_ASSERT(success, "Should set valid config");

    // Test model type 1 uses custom config
    LODResult result1 = system.selectLOD(1, 40.0f);
    TEST_ASSERT(result1.level == 1, "Model 1 at 40m should be LOD 1 (30-100m range)");

    // Test unconfigured model type 2 uses default config
    LODResult result2 = system.selectLOD(2, 40.0f);
    TEST_ASSERT(result2.level == 0, "Model 2 at 40m should be LOD 0 (default <50m)");

    std::cout << "PASSED\n";
}

void test_LODSelection_RemoveConfig() {
    std::cout << "test_LODSelection_RemoveConfig... ";

    LODSystem system;

    LODConfig customConfig;
    customConfig.thresholds.push_back({20.0f, 2.0f});
    system.setConfig(1, customConfig);

    // Verify custom config is used
    LODResult result1 = system.selectLOD(1, 30.0f);
    TEST_ASSERT(result1.level == 1, "Should use custom config initially");

    // Remove config
    system.removeConfig(1);

    // Verify default config is now used
    LODResult result2 = system.selectLOD(1, 30.0f);
    TEST_ASSERT(result2.level == 0, "Should use default config after removal");

    std::cout << "PASSED\n";
}

// =============================================================================
// Test: Crossfade Blending
// =============================================================================

void test_LODSelection_CrossfadeEnabled() {
    std::cout << "test_LODSelection_CrossfadeEnabled... ";

    LODSystem system;

    LODConfig config;
    config.thresholds.push_back({50.0f, 2.0f});
    config.transitionMode = LODTransitionMode::Crossfade;
    config.crossfadeRange = 5.0f;  // Crossfade zone: 45-50m

    system.setDefaultConfig(config);

    // In crossfade zone
    LODResult result = system.selectLODDefault(47.5f);

    TEST_ASSERT(result.isBlending, "Should be blending in crossfade zone");
    TEST_ASSERT(result.level == 0, "Primary level should be 0");
    TEST_ASSERT(result.nextLevel == 1, "Next level should be 1");
    TEST_ASSERT_FLOAT_EQ(result.blendAlpha, 0.5f, 0.1f, "Should be 50% through blend");

    std::cout << "PASSED\n";
}

void test_LODSelection_CrossfadeAtStart() {
    std::cout << "test_LODSelection_CrossfadeAtStart... ";

    LODSystem system;

    LODConfig config;
    config.thresholds.push_back({50.0f, 2.0f});
    config.transitionMode = LODTransitionMode::Crossfade;
    config.crossfadeRange = 5.0f;

    system.setDefaultConfig(config);

    // At start of crossfade zone (45m)
    LODResult result = system.selectLODDefault(45.0f);

    TEST_ASSERT(result.isBlending, "Should be blending at start of crossfade");
    TEST_ASSERT_FLOAT_EQ(result.blendAlpha, 0.0f, 0.01f, "Blend alpha should be 0 at start");

    std::cout << "PASSED\n";
}

void test_LODSelection_CrossfadeAtEnd() {
    std::cout << "test_LODSelection_CrossfadeAtEnd... ";

    LODSystem system;

    LODConfig config;
    config.thresholds.push_back({50.0f, 2.0f});
    config.transitionMode = LODTransitionMode::Crossfade;
    config.crossfadeRange = 5.0f;

    system.setDefaultConfig(config);

    // Just before end of crossfade zone (49.9m)
    LODResult result = system.selectLODDefault(49.9f);

    TEST_ASSERT(result.isBlending, "Should be blending near end of crossfade");
    TEST_ASSERT(result.blendAlpha > 0.9f, "Blend alpha should be near 1 at end");

    std::cout << "PASSED\n";
}

// =============================================================================
// Test: Disabled LOD
// =============================================================================

void test_LODSelection_Disabled() {
    std::cout << "test_LODSelection_Disabled... ";

    LODSystem system;

    LODConfig config = LODConfig::createDefault2Level();
    config.enabled = false;

    system.setDefaultConfig(config);

    // Even at far distance, should return LOD 0
    LODResult result = system.selectLODDefault(500.0f);

    TEST_ASSERT(result.level == 0, "Should always return LOD 0 when disabled");
    TEST_ASSERT(!result.isBlending, "Should not blend when disabled");

    std::cout << "PASSED\n";
}

// =============================================================================
// Test: Distance Computation
// =============================================================================

void test_ComputeDistance() {
    std::cout << "test_ComputeDistance... ";

    glm::vec3 entityPos(10.0f, 0.0f, 10.0f);
    glm::vec3 cameraPos(0.0f, 0.0f, 0.0f);

    float distance = LODSystem::computeDistance(entityPos, cameraPos);
    float expected = std::sqrt(10.0f * 10.0f + 10.0f * 10.0f);

    TEST_ASSERT_FLOAT_EQ(distance, expected, 0.001f, "Distance should be ~14.14m");

    std::cout << "PASSED\n";
}

void test_ComputeDistanceSquared() {
    std::cout << "test_ComputeDistanceSquared... ";

    glm::vec3 entityPos(3.0f, 4.0f, 0.0f);
    glm::vec3 cameraPos(0.0f, 0.0f, 0.0f);

    float distSq = computeDistanceSquared(entityPos, cameraPos);
    float expected = 3.0f * 3.0f + 4.0f * 4.0f;  // 25

    TEST_ASSERT_FLOAT_EQ(distSq, expected, 0.001f, "Distance squared should be 25");

    std::cout << "PASSED\n";
}

void test_SelectLODForPosition() {
    std::cout << "test_SelectLODForPosition... ";

    LODSystem system;

    glm::vec3 entityPos(100.0f, 0.0f, 0.0f);  // 100m away on X axis
    glm::vec3 cameraPos(0.0f, 0.0f, 0.0f);

    LODResult result = system.selectLODForPosition(0, entityPos, cameraPos);

    TEST_ASSERT(result.level == 1, "Should select LOD 1 at 100m distance");

    std::cout << "PASSED\n";
}

// =============================================================================
// Test: Statistics
// =============================================================================

void test_LODStats_Recording() {
    std::cout << "test_LODStats_Recording... ";

    LODSystem system;

    system.beginFrame();

    // Record some selections
    system.recordSelection(system.selectLODDefault(10.0f));   // LOD 0
    system.recordSelection(system.selectLODDefault(20.0f));   // LOD 0
    system.recordSelection(system.selectLODDefault(75.0f));   // LOD 1
    system.recordSelection(system.selectLODDefault(100.0f));  // LOD 1

    const LODStats& stats = system.getStats();

    TEST_ASSERT(stats.totalEvaluated == 4, "Should have evaluated 4 entities");
    TEST_ASSERT(stats.levelCounts[0] == 2, "Should have 2 entities at LOD 0");
    TEST_ASSERT(stats.levelCounts[1] == 2, "Should have 2 entities at LOD 1");

    std::cout << "PASSED\n";
}

void test_LODStats_Reset() {
    std::cout << "test_LODStats_Reset... ";

    LODSystem system;

    // Record some selections
    system.recordSelection(system.selectLODDefault(10.0f));
    system.recordSelection(system.selectLODDefault(75.0f));

    // Begin new frame should reset
    system.beginFrame();

    const LODStats& stats = system.getStats();

    TEST_ASSERT(stats.totalEvaluated == 0, "Stats should be reset after beginFrame");
    TEST_ASSERT(stats.levelCounts[0] == 0, "Level counts should be reset");

    std::cout << "PASSED\n";
}

// =============================================================================
// Test: Hysteresis
// =============================================================================

void test_LODHysteresis_Tracking() {
    std::cout << "test_LODHysteresis_Tracking... ";

    LODSystem system;

    // Initially, no hysteresis data
    std::uint8_t lastLevel = system.getLastLevel(42);
    TEST_ASSERT(lastLevel == LODDefaults::INVALID_LOD_LEVEL, "Should return invalid for untracked entity");

    // Track entity
    system.updateHysteresis(42, 1);

    lastLevel = system.getLastLevel(42);
    TEST_ASSERT(lastLevel == 1, "Should return tracked level");

    // Update
    system.updateHysteresis(42, 2);

    lastLevel = system.getLastLevel(42);
    TEST_ASSERT(lastLevel == 2, "Should return updated level");

    std::cout << "PASSED\n";
}

void test_LODHysteresis_Clear() {
    std::cout << "test_LODHysteresis_Clear... ";

    LODSystem system;

    system.updateHysteresis(1, 0);
    system.updateHysteresis(2, 1);
    system.updateHysteresis(3, 2);

    system.clearHysteresis();

    TEST_ASSERT(system.getLastLevel(1) == LODDefaults::INVALID_LOD_LEVEL, "Should be cleared");
    TEST_ASSERT(system.getLastLevel(2) == LODDefaults::INVALID_LOD_LEVEL, "Should be cleared");
    TEST_ASSERT(system.getLastLevel(3) == LODDefaults::INVALID_LOD_LEVEL, "Should be cleared");

    std::cout << "PASSED\n";
}

// =============================================================================
// Test: Framework Extensibility
// =============================================================================

void test_LODExtensibility_5Levels() {
    std::cout << "test_LODExtensibility_5Levels... ";

    LODSystem system;

    LODConfig config;
    config.thresholds.push_back({20.0f, 2.0f});   // LOD 0: <20m
    config.thresholds.push_back({40.0f, 2.0f});   // LOD 1: 20-40m
    config.thresholds.push_back({80.0f, 2.0f});   // LOD 2: 40-80m
    config.thresholds.push_back({160.0f, 2.0f});  // LOD 3: 80-160m
                                                   // LOD 4: >160m

    TEST_ASSERT(config.isValid(), "5-level config should be valid");
    TEST_ASSERT(config.getLevelCount() == 5, "Should have 5 levels");

    system.setDefaultConfig(config);

    TEST_ASSERT(system.selectLODDefault(10.0f).level == 0, "10m should be LOD 0");
    TEST_ASSERT(system.selectLODDefault(30.0f).level == 1, "30m should be LOD 1");
    TEST_ASSERT(system.selectLODDefault(60.0f).level == 2, "60m should be LOD 2");
    TEST_ASSERT(system.selectLODDefault(120.0f).level == 3, "120m should be LOD 3");
    TEST_ASSERT(system.selectLODDefault(200.0f).level == 4, "200m should be LOD 4");

    std::cout << "PASSED\n";
}

// =============================================================================
// Test: Debug Color
// =============================================================================

void test_LODDebugColor() {
    std::cout << "test_LODDebugColor... ";

    glm::vec4 color0 = getLODDebugColor(0);
    glm::vec4 color1 = getLODDebugColor(1);
    glm::vec4 color2 = getLODDebugColor(2);

    TEST_ASSERT(color0.g == 1.0f, "LOD 0 should be green");
    TEST_ASSERT(color1.r == 1.0f && color1.g == 1.0f, "LOD 1 should be yellow");
    TEST_ASSERT(color2.r == 1.0f && color2.g == 0.5f, "LOD 2 should be orange");

    std::cout << "PASSED\n";
}

// =============================================================================
// Test: Performance with 512x512 Map Entity Counts
// =============================================================================

void test_Performance_LargeEntityCount() {
    std::cout << "test_Performance_LargeEntityCount... ";

    LODSystem system;
    system.setDefaultConfig(LODConfig::createDefault3Level());

    // 512x512 = 262,144 tiles
    constexpr int ENTITY_COUNT = 262144;
    std::vector<float> distances(ENTITY_COUNT);

    // Generate random distances
    for (int i = 0; i < ENTITY_COUNT; ++i) {
        distances[i] = static_cast<float>(i % 300);  // 0-299m range
    }

    // Measure time to evaluate all entities
    auto start = std::chrono::high_resolution_clock::now();

    system.beginFrame();
    for (int i = 0; i < ENTITY_COUNT; ++i) {
        LODResult result = system.selectLODDefault(distances[i]);
        system.recordSelection(result);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    const LODStats& stats = system.getStats();
    TEST_ASSERT(stats.totalEvaluated == ENTITY_COUNT, "Should have evaluated all entities");

    // Should complete in reasonable time (< 100ms for 262k entities)
    bool fastEnough = duration.count() < 100000;  // 100ms in microseconds
    TEST_ASSERT(fastEnough, "LOD evaluation should be < 100ms for 262k entities");

    std::cout << "PASSED (" << duration.count() / 1000.0 << "ms for " << ENTITY_COUNT << " entities)\n";
}

// =============================================================================
// Test: Edge Cases
// =============================================================================

void test_LODSelection_ZeroDistance() {
    std::cout << "test_LODSelection_ZeroDistance... ";

    LODSystem system;

    LODResult result = system.selectLODDefault(0.0f);

    TEST_ASSERT(result.level == 0, "Zero distance should be LOD 0");

    std::cout << "PASSED\n";
}

void test_LODSelection_NegativeDistance() {
    std::cout << "test_LODSelection_NegativeDistance... ";

    LODSystem system;

    // Negative distance (shouldn't happen in practice but handle gracefully)
    LODResult result = system.selectLODDefault(-10.0f);

    TEST_ASSERT(result.level == 0, "Negative distance should be LOD 0");

    std::cout << "PASSED\n";
}

void test_LODSelection_VeryLargeDistance() {
    std::cout << "test_LODSelection_VeryLargeDistance... ";

    LODSystem system;
    system.setDefaultConfig(LODConfig::createDefault3Level());

    // Very far away
    LODResult result = system.selectLODDefault(10000.0f);

    TEST_ASSERT(result.level == 2, "Very large distance should be highest LOD level");

    std::cout << "PASSED\n";
}

// =============================================================================
// Main
// =============================================================================

int main() {
    std::cout << "=== LOD System Tests ===\n\n";

    // LODConfig tests
    test_LODConfig_Default2Level();
    test_LODConfig_Default3Level();
    test_LODConfig_InvalidOrder();
    test_LODConfig_TooManyLevels();

    // LOD Selection tests
    test_LODSelection_ClosestDistance();
    test_LODSelection_MediumDistance();
    test_LODSelection_FarDistance();
    test_LODSelection_AtThreshold();
    test_LODSelection_JustBeforeThreshold();

    // Per-model configuration tests
    test_LODSelection_PerModelConfig();
    test_LODSelection_RemoveConfig();

    // Crossfade tests
    test_LODSelection_CrossfadeEnabled();
    test_LODSelection_CrossfadeAtStart();
    test_LODSelection_CrossfadeAtEnd();

    // Disabled LOD test
    test_LODSelection_Disabled();

    // Distance computation tests
    test_ComputeDistance();
    test_ComputeDistanceSquared();
    test_SelectLODForPosition();

    // Statistics tests
    test_LODStats_Recording();
    test_LODStats_Reset();

    // Hysteresis tests
    test_LODHysteresis_Tracking();
    test_LODHysteresis_Clear();

    // Extensibility tests
    test_LODExtensibility_5Levels();

    // Debug color test
    test_LODDebugColor();

    // Performance test
    test_Performance_LargeEntityCount();

    // Edge case tests
    test_LODSelection_ZeroDistance();
    test_LODSelection_NegativeDistance();
    test_LODSelection_VeryLargeDistance();

    std::cout << "\n=== Results ===\n";
    std::cout << "Tests run: " << g_testsRun << "\n";
    std::cout << "Tests passed: " << g_testsPassed << "\n";
    std::cout << "Tests failed: " << (g_testsRun - g_testsPassed) << "\n";

    return (g_testsRun == g_testsPassed) ? 0 : 1;
}
