/**
 * @file test_terrain_visual_config_integration.cpp
 * @brief Integration tests for TerrainVisualConfig with ToonShaderConfig (Ticket 3-039)
 *
 * Tests the integration of terrain visual configuration with the ToonShaderConfig
 * singleton. Verifies:
 * - TerrainVisualConfigManager singleton access
 * - Dirty flag tracking for GPU uniform updates
 * - Integration with ToonShaderConfig
 * - Default values match Game Designer specifications
 * - Glow behavior parameters are configurable
 * - Changes take effect immediately (no restart required)
 */

#include <sims3000/render/TerrainVisualConfig.h>
#include <sims3000/render/ToonShaderConfig.h>
#include <sims3000/terrain/TerrainTypeInfo.h>
#include <cstdio>
#include <cmath>

// Test counter
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) void name()
#define RUN_TEST(name) do { \
    printf("Running %s... ", #name); \
    name(); \
    printf("PASSED\n"); \
    tests_passed++; \
} while(0)

#define ASSERT_TRUE(cond) do { \
    if (!(cond)) { \
        printf("FAILED: %s at line %d\n", #cond, __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

#define ASSERT_EQ(a, b) do { \
    if ((a) != (b)) { \
        printf("FAILED: %s == %s at line %d\n", #a, #b, __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

#define ASSERT_FLOAT_EQ(a, b, eps) do { \
    if (std::abs((a) - (b)) > (eps)) { \
        printf("FAILED: %s ~= %s at line %d (got %f, expected %f)\n", #a, #b, __LINE__, (float)(a), (float)(b)); \
        tests_failed++; \
        return; \
    } \
} while(0)

// =============================================================================
// Test: TerrainVisualConfigManager singleton access
// =============================================================================
TEST(test_manager_singleton) {
    auto& manager1 = sims3000::TerrainVisualConfigManager::instance();
    auto& manager2 = sims3000::TerrainVisualConfigManager::instance();

    // Same instance
    ASSERT_TRUE(&manager1 == &manager2);
}

// =============================================================================
// Test: ToonShaderConfig provides access to TerrainVisualConfigManager
// =============================================================================
TEST(test_toon_shader_config_integration) {
    auto& toonConfig = sims3000::ToonShaderConfig::instance();
    auto& terrainConfig = toonConfig.getTerrainVisualConfig();

    // Verify it's the same singleton
    auto& directManager = sims3000::TerrainVisualConfigManager::instance();
    ASSERT_TRUE(&terrainConfig == &directManager);
}

// =============================================================================
// Test: Dirty flag tracking
// =============================================================================
TEST(test_dirty_flag_tracking) {
    auto& manager = sims3000::TerrainVisualConfigManager::instance();

    // Reset to known state
    manager.resetToDefaults();

    // Should be dirty after reset
    ASSERT_TRUE(manager.isDirty());

    // Clear dirty flag
    manager.clearDirtyFlag();
    ASSERT_TRUE(!manager.isDirty());

    // Modify a value - should become dirty
    manager.setBaseColor(0, glm::vec4(0.1f, 0.2f, 0.3f, 1.0f));
    ASSERT_TRUE(manager.isDirty());

    // Clear and test emissive color
    manager.clearDirtyFlag();
    ASSERT_TRUE(!manager.isDirty());
    manager.setEmissiveColor(1, glm::vec3(1.0f, 0.5f, 0.0f), 0.5f);
    ASSERT_TRUE(manager.isDirty());

    // Clear and test glow parameters
    manager.clearDirtyFlag();
    ASSERT_TRUE(!manager.isDirty());
    sims3000::GlowParameters params(sims3000::GlowBehavior::Pulse, 4.0f, 0.3f);
    manager.setGlowParameters(2, params);
    ASSERT_TRUE(manager.isDirty());

    // Clear and test sea level
    manager.clearDirtyFlag();
    ASSERT_TRUE(!manager.isDirty());
    manager.setSeaLevel(12.0f);
    ASSERT_TRUE(manager.isDirty());
}

// =============================================================================
// Test: ToonShaderConfig dirty flag integration
// =============================================================================
TEST(test_toon_shader_config_dirty_flags) {
    auto& toonConfig = sims3000::ToonShaderConfig::instance();
    auto& terrainConfig = toonConfig.getTerrainVisualConfig();

    // Clear all flags
    toonConfig.clearAllDirtyFlags();
    ASSERT_TRUE(!toonConfig.isDirty());
    ASSERT_TRUE(!toonConfig.isTerrainConfigDirty());
    ASSERT_TRUE(!toonConfig.isAnyDirty());

    // Modify terrain config
    terrainConfig.setBaseColor(0, glm::vec4(0.15f, 0.15f, 0.2f, 1.0f));
    ASSERT_TRUE(toonConfig.isTerrainConfigDirty());
    ASSERT_TRUE(toonConfig.isAnyDirty());
    ASSERT_TRUE(!toonConfig.isDirty()); // Toon config itself not dirty

    // Clear terrain dirty flag
    toonConfig.clearTerrainDirtyFlag();
    ASSERT_TRUE(!toonConfig.isTerrainConfigDirty());

    // Modify toon config
    toonConfig.setBloomIntensity(1.2f);
    ASSERT_TRUE(toonConfig.isDirty());
    ASSERT_TRUE(toonConfig.isAnyDirty());

    // Clear all
    toonConfig.clearAllDirtyFlags();
    ASSERT_TRUE(!toonConfig.isAnyDirty());
}

// =============================================================================
// Test: Default values match Game Designer specifications
// =============================================================================
TEST(test_default_values_match_spec) {
    auto& manager = sims3000::TerrainVisualConfigManager::instance();
    manager.resetToDefaults();

    const auto& config = manager.getConfig();

    // Verify emissive intensities match TerrainTypeInfo (spec)
    for (std::size_t i = 0; i < sims3000::TERRAIN_PALETTE_SIZE; ++i) {
        const auto& info = sims3000::terrain::TERRAIN_INFO[i];
        ASSERT_FLOAT_EQ(config.emissive_colors[i].a, info.emissive_intensity, 0.001f);
    }

    // Verify emissive colors match TerrainTypeInfo
    for (std::size_t i = 0; i < sims3000::TERRAIN_PALETTE_SIZE; ++i) {
        const auto& info = sims3000::terrain::TERRAIN_INFO[i];
        ASSERT_FLOAT_EQ(config.emissive_colors[i].r, info.emissive_color.x, 0.001f);
        ASSERT_FLOAT_EQ(config.emissive_colors[i].g, info.emissive_color.y, 0.001f);
        ASSERT_FLOAT_EQ(config.emissive_colors[i].b, info.emissive_color.z, 0.001f);
    }

    // Verify default sea level
    ASSERT_FLOAT_EQ(config.sea_level, 8.0f, 0.001f);
}

// =============================================================================
// Test: Glow behavior parameters are configurable
// =============================================================================
TEST(test_glow_parameters_configurable) {
    auto& manager = sims3000::TerrainVisualConfigManager::instance();
    manager.resetToDefaults();

    const auto& config = manager.getConfig();

    // Verify default glow behaviors per spec
    // Substrate (0) - Static
    ASSERT_EQ(static_cast<int>(config.glow_params[0].behavior),
              static_cast<int>(sims3000::GlowBehavior::Static));

    // Ridge (1) - Static
    ASSERT_EQ(static_cast<int>(config.glow_params[1].behavior),
              static_cast<int>(sims3000::GlowBehavior::Static));

    // DeepVoid (2) - Pulse
    ASSERT_EQ(static_cast<int>(config.glow_params[2].behavior),
              static_cast<int>(sims3000::GlowBehavior::Pulse));

    // FlowChannel (3) - Flow
    ASSERT_EQ(static_cast<int>(config.glow_params[3].behavior),
              static_cast<int>(sims3000::GlowBehavior::Flow));

    // PrismaFields (6) - Shimmer
    ASSERT_EQ(static_cast<int>(config.glow_params[6].behavior),
              static_cast<int>(sims3000::GlowBehavior::Shimmer));

    // BlightMires (8) - Irregular
    ASSERT_EQ(static_cast<int>(config.glow_params[8].behavior),
              static_cast<int>(sims3000::GlowBehavior::Irregular));

    // Modify and verify
    sims3000::GlowParameters customParams(sims3000::GlowBehavior::Shimmer, 2.0f, 0.5f, 0.25f);
    manager.setGlowParameters(0, customParams);

    const auto& updatedConfig = manager.getConfig();
    ASSERT_EQ(static_cast<int>(updatedConfig.glow_params[0].behavior),
              static_cast<int>(sims3000::GlowBehavior::Shimmer));
    ASSERT_FLOAT_EQ(updatedConfig.glow_params[0].period, 2.0f, 0.001f);
    ASSERT_FLOAT_EQ(updatedConfig.glow_params[0].amplitude, 0.5f, 0.001f);
    ASSERT_FLOAT_EQ(updatedConfig.glow_params[0].phase_offset, 0.25f, 0.001f);
}

// =============================================================================
// Test: Changes take effect immediately (no restart)
// =============================================================================
TEST(test_changes_immediate) {
    auto& manager = sims3000::TerrainVisualConfigManager::instance();
    manager.resetToDefaults();
    manager.clearDirtyFlag();

    // Get initial values
    const auto& config = manager.getConfig();
    glm::vec4 originalColor = config.base_colors[5];

    // Modify
    glm::vec4 newColor(0.5f, 0.6f, 0.7f, 1.0f);
    manager.setBaseColor(5, newColor);

    // Verify change is immediate
    const auto& updatedConfig = manager.getConfig();
    ASSERT_FLOAT_EQ(updatedConfig.base_colors[5].r, 0.5f, 0.001f);
    ASSERT_FLOAT_EQ(updatedConfig.base_colors[5].g, 0.6f, 0.001f);
    ASSERT_FLOAT_EQ(updatedConfig.base_colors[5].b, 0.7f, 0.001f);

    // Verify dirty flag is set (for GPU upload)
    ASSERT_TRUE(manager.isDirty());
}

// =============================================================================
// Test: Config contains per-type base_color[10] and emissive_color_intensity[10]
// =============================================================================
TEST(test_per_type_arrays) {
    auto& manager = sims3000::TerrainVisualConfigManager::instance();
    const auto& config = manager.getConfig();

    // Verify array sizes
    ASSERT_EQ(config.base_colors.size(), 10u);
    ASSERT_EQ(config.emissive_colors.size(), 10u);
    ASSERT_EQ(config.glow_params.size(), 10u);

    // Verify each entry is accessible
    for (std::size_t i = 0; i < 10; ++i) {
        // base_colors: Vec4 with rgb + alpha
        glm::vec4 baseColor = config.base_colors[i];
        ASSERT_TRUE(baseColor.r >= 0.0f && baseColor.r <= 1.0f);

        // emissive_colors: Vec4 with rgb + intensity in alpha
        glm::vec4 emissiveColor = config.emissive_colors[i];
        ASSERT_TRUE(emissiveColor.a >= 0.0f && emissiveColor.a <= 1.0f);
    }
}

// =============================================================================
// Test: GlowBehavior enum values
// =============================================================================
TEST(test_glow_behavior_enum) {
    // Verify enum values are distinct
    ASSERT_TRUE(static_cast<int>(sims3000::GlowBehavior::Static) !=
                static_cast<int>(sims3000::GlowBehavior::Pulse));
    ASSERT_TRUE(static_cast<int>(sims3000::GlowBehavior::Pulse) !=
                static_cast<int>(sims3000::GlowBehavior::Shimmer));
    ASSERT_TRUE(static_cast<int>(sims3000::GlowBehavior::Shimmer) !=
                static_cast<int>(sims3000::GlowBehavior::Flow));
    ASSERT_TRUE(static_cast<int>(sims3000::GlowBehavior::Flow) !=
                static_cast<int>(sims3000::GlowBehavior::Irregular));
}

// =============================================================================
// Test: GlowParameters default construction
// =============================================================================
TEST(test_glow_parameters_default) {
    sims3000::GlowParameters params;

    ASSERT_EQ(static_cast<int>(params.behavior), static_cast<int>(sims3000::GlowBehavior::Static));
    ASSERT_FLOAT_EQ(params.period, 0.0f, 0.001f);
    ASSERT_FLOAT_EQ(params.amplitude, 0.0f, 0.001f);
    ASSERT_FLOAT_EQ(params.phase_offset, 0.0f, 0.001f);
}

// =============================================================================
// Test: GlowParameters parameterized construction
// =============================================================================
TEST(test_glow_parameters_parameterized) {
    sims3000::GlowParameters params(sims3000::GlowBehavior::Pulse, 5.0f, 0.4f, 0.1f);

    ASSERT_EQ(static_cast<int>(params.behavior), static_cast<int>(sims3000::GlowBehavior::Pulse));
    ASSERT_FLOAT_EQ(params.period, 5.0f, 0.001f);
    ASSERT_FLOAT_EQ(params.amplitude, 0.4f, 0.001f);
    ASSERT_FLOAT_EQ(params.phase_offset, 0.1f, 0.001f);
}

// =============================================================================
// Test: Change callback is invoked
// =============================================================================
TEST(test_change_callback) {
    auto& manager = sims3000::TerrainVisualConfigManager::instance();
    manager.resetToDefaults();
    manager.clearDirtyFlag();

    bool callbackInvoked = false;
    manager.setChangeCallback([&callbackInvoked]() {
        callbackInvoked = true;
    });

    // Modify a value
    manager.setBaseColor(0, glm::vec4(0.2f, 0.2f, 0.3f, 1.0f));

    // Callback should have been invoked
    ASSERT_TRUE(callbackInvoked);

    // Reset callback
    manager.setChangeCallback(nullptr);
}

// =============================================================================
// Test: resetToDefaults restores all values
// =============================================================================
TEST(test_reset_to_defaults) {
    auto& manager = sims3000::TerrainVisualConfigManager::instance();

    // Modify some values
    manager.setBaseColor(0, glm::vec4(0.9f, 0.9f, 0.9f, 1.0f));
    manager.setEmissiveColor(1, glm::vec3(1.0f, 1.0f, 1.0f), 1.0f);
    manager.setSeaLevel(25.0f);

    // Reset
    manager.resetToDefaults();

    const auto& config = manager.getConfig();

    // Verify defaults restored
    ASSERT_FLOAT_EQ(config.sea_level, 8.0f, 0.001f);

    // Emissive should match TerrainTypeInfo again
    const auto& info = sims3000::terrain::TERRAIN_INFO[1];
    ASSERT_FLOAT_EQ(config.emissive_colors[1].a, info.emissive_intensity, 0.001f);
}

// =============================================================================
// Test: Uniform buffer size and alignment
// =============================================================================
TEST(test_uniform_buffer_properties) {
    // TerrainVisualConfig GPU buffer size should be 336 bytes
    ASSERT_EQ(sims3000::TerrainVisualConfig::getGPUSize(), 336u);

    // Should be 16-byte aligned for GPU
    ASSERT_TRUE(alignof(sims3000::TerrainVisualConfig) >= 16);
}

// =============================================================================
// Test: getData returns valid pointer
// =============================================================================
TEST(test_get_data) {
    auto& manager = sims3000::TerrainVisualConfigManager::instance();
    const auto& config = manager.getConfig();

    const void* data = config.getData();
    ASSERT_TRUE(data != nullptr);
    ASSERT_TRUE(data == &config);
}

// =============================================================================
// Main
// =============================================================================
int main() {
    printf("=== TerrainVisualConfig Integration Tests (Ticket 3-039) ===\n\n");

    RUN_TEST(test_manager_singleton);
    RUN_TEST(test_toon_shader_config_integration);
    RUN_TEST(test_dirty_flag_tracking);
    RUN_TEST(test_toon_shader_config_dirty_flags);
    RUN_TEST(test_default_values_match_spec);
    RUN_TEST(test_glow_parameters_configurable);
    RUN_TEST(test_changes_immediate);
    RUN_TEST(test_per_type_arrays);
    RUN_TEST(test_glow_behavior_enum);
    RUN_TEST(test_glow_parameters_default);
    RUN_TEST(test_glow_parameters_parameterized);
    RUN_TEST(test_change_callback);
    RUN_TEST(test_reset_to_defaults);
    RUN_TEST(test_uniform_buffer_properties);
    RUN_TEST(test_get_data);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? 1 : 0;
}
