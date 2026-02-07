/**
 * @file test_terrain_visual_config.cpp
 * @brief Unit tests for TerrainVisualConfig (Ticket 3-026)
 *
 * Tests the terrain visual configuration struct used for GPU uniform buffer
 * upload. Verifies:
 * - Struct size matches GPU alignment requirements (336 bytes)
 * - Initialization from TerrainTypeInfo populates all 10 terrain types
 * - Base colors and emissive colors are set correctly
 * - Glow animation constants are defined
 * - Crevice glow configuration is correct
 */

#include <sims3000/render/TerrainVisualConfig.h>
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
// Test: Struct size matches GPU buffer requirements
// =============================================================================
TEST(test_struct_size) {
    // TerrainVisualConfigGPU must be exactly 336 bytes for GPU uniform buffer
    // float4[10] * 2 + float * 4 = 160 + 160 + 16 = 336 bytes
    ASSERT_EQ(sizeof(sims3000::TerrainVisualConfigGPU), 336u);

    // TerrainVisualConfig contains GPU data plus glow_params
    // It must be larger than 336 bytes but still properly aligned
    ASSERT_TRUE(sizeof(sims3000::TerrainVisualConfig) > 336u);
}

// =============================================================================
// Test: Struct alignment for GPU upload
// =============================================================================
TEST(test_struct_alignment) {
    // Must be at least 16-byte aligned for GPU uniform buffer
    ASSERT_TRUE(alignof(sims3000::TerrainVisualConfig) >= 16);
}

// =============================================================================
// Test: Default construction initializes from TerrainTypeInfo
// =============================================================================
TEST(test_default_construction) {
    sims3000::TerrainVisualConfig config;

    // All 10 terrain types should have colors initialized
    for (std::size_t i = 0; i < sims3000::TERRAIN_PALETTE_SIZE; ++i) {
        // Base color should have some value (not all zeros)
        // Alpha should be 1.0
        ASSERT_FLOAT_EQ(config.base_colors[i].a, 1.0f, 0.001f);

        // Emissive alpha contains intensity from TerrainTypeInfo
        const auto& info = sims3000::terrain::TERRAIN_INFO[i];
        ASSERT_FLOAT_EQ(config.emissive_colors[i].a, info.emissive_intensity, 0.001f);
    }

    // Default values
    ASSERT_FLOAT_EQ(config.glow_time, 0.0f, 0.001f);
    ASSERT_FLOAT_EQ(config.sea_level, 8.0f, 0.001f);
}

// =============================================================================
// Test: Emissive colors match TerrainTypeInfo RGB values
// =============================================================================
TEST(test_emissive_colors_from_terrain_info) {
    sims3000::TerrainVisualConfig config;

    for (std::size_t i = 0; i < sims3000::TERRAIN_PALETTE_SIZE; ++i) {
        const auto& info = sims3000::terrain::TERRAIN_INFO[i];

        // RGB should match TerrainTypeInfo emissive_color
        ASSERT_FLOAT_EQ(config.emissive_colors[i].r, info.emissive_color.x, 0.001f);
        ASSERT_FLOAT_EQ(config.emissive_colors[i].g, info.emissive_color.y, 0.001f);
        ASSERT_FLOAT_EQ(config.emissive_colors[i].b, info.emissive_color.z, 0.001f);
    }
}

// =============================================================================
// Test: Emissive intensity hierarchy (from spec)
// =============================================================================
TEST(test_emissive_intensity_hierarchy) {
    sims3000::TerrainVisualConfig config;

    // Substrate (0.05) < Ridge (0.10)
    ASSERT_TRUE(config.emissive_colors[0].a < config.emissive_colors[1].a);

    // Ridge (0.10) <= Water types (0.10-0.12)
    ASSERT_TRUE(config.emissive_colors[1].a <= config.emissive_colors[2].a);

    // Water types < BiolumeGrove (0.25)
    ASSERT_TRUE(config.emissive_colors[4].a < config.emissive_colors[5].a);

    // BiolumeGrove (0.25) < SporeFlats/BlightMires (0.30)
    ASSERT_TRUE(config.emissive_colors[5].a < config.emissive_colors[7].a);

    // EmberCrust (0.35) < PrismaFields (0.60)
    ASSERT_TRUE(config.emissive_colors[9].a < config.emissive_colors[6].a);

    // PrismaFields should be the maximum at 0.60
    ASSERT_FLOAT_EQ(config.emissive_colors[6].a, 0.60f, 0.001f);
}

// =============================================================================
// Test: setGlowTime updates glow_time
// =============================================================================
TEST(test_set_glow_time) {
    sims3000::TerrainVisualConfig config;

    config.setGlowTime(5.5f);
    ASSERT_FLOAT_EQ(config.glow_time, 5.5f, 0.001f);

    config.setGlowTime(123.456f);
    ASSERT_FLOAT_EQ(config.glow_time, 123.456f, 0.001f);
}

// =============================================================================
// Test: setSeaLevel updates sea_level
// =============================================================================
TEST(test_set_sea_level) {
    sims3000::TerrainVisualConfig config;

    config.setSeaLevel(12.0f);
    ASSERT_FLOAT_EQ(config.sea_level, 12.0f, 0.001f);
}

// =============================================================================
// Test: setBaseColor modifies specific terrain type
// =============================================================================
TEST(test_set_base_color) {
    sims3000::TerrainVisualConfig config;

    glm::vec4 newColor(0.1f, 0.2f, 0.3f, 0.4f);
    config.setBaseColor(3, newColor);

    ASSERT_FLOAT_EQ(config.base_colors[3].r, 0.1f, 0.001f);
    ASSERT_FLOAT_EQ(config.base_colors[3].g, 0.2f, 0.001f);
    ASSERT_FLOAT_EQ(config.base_colors[3].b, 0.3f, 0.001f);
    ASSERT_FLOAT_EQ(config.base_colors[3].a, 0.4f, 0.001f);
}

// =============================================================================
// Test: setEmissiveColor modifies specific terrain type
// =============================================================================
TEST(test_set_emissive_color) {
    sims3000::TerrainVisualConfig config;

    glm::vec3 newColor(1.0f, 0.5f, 0.0f);
    float newIntensity = 0.75f;
    config.setEmissiveColor(6, newColor, newIntensity);

    ASSERT_FLOAT_EQ(config.emissive_colors[6].r, 1.0f, 0.001f);
    ASSERT_FLOAT_EQ(config.emissive_colors[6].g, 0.5f, 0.001f);
    ASSERT_FLOAT_EQ(config.emissive_colors[6].b, 0.0f, 0.001f);
    ASSERT_FLOAT_EQ(config.emissive_colors[6].a, 0.75f, 0.001f);
}

// =============================================================================
// Test: Out-of-bounds setBaseColor is ignored
// =============================================================================
TEST(test_out_of_bounds_base_color) {
    sims3000::TerrainVisualConfig config;

    // Store original value
    glm::vec4 original = config.base_colors[0];

    // Try to set out-of-bounds index (should be ignored)
    config.setBaseColor(100, glm::vec4(1.0f));

    // Original value should be unchanged
    ASSERT_FLOAT_EQ(config.base_colors[0].r, original.r, 0.001f);
}

// =============================================================================
// Test: getGPUSize returns correct size
// =============================================================================
TEST(test_get_gpu_size) {
    ASSERT_EQ(sims3000::TerrainVisualConfig::getGPUSize(), 336u);
}

// =============================================================================
// Test: getData returns pointer to struct
// =============================================================================
TEST(test_get_data) {
    sims3000::TerrainVisualConfig config;
    const void* ptr = config.getData();

    // Pointer should be to the start of the struct
    ASSERT_TRUE(ptr == &config);
}

// =============================================================================
// Test: Glow animation constants are defined
// =============================================================================
TEST(test_glow_animation_constants) {
    using namespace sims3000::TerrainGlowAnimation;

    // Verify pulse periods are positive
    ASSERT_TRUE(WATER_PULSE_PERIOD > 0.0f);
    ASSERT_TRUE(BIOLUME_PULSE_PERIOD > 0.0f);
    ASSERT_TRUE(PRISMA_SHIMMER_PERIOD > 0.0f);
    ASSERT_TRUE(SPORE_PULSE_PERIOD > 0.0f);
    ASSERT_TRUE(BLIGHT_BUBBLE_PERIOD > 0.0f);
    ASSERT_TRUE(EMBER_THROB_PERIOD > 0.0f);

    // Verify amplitudes are in valid range
    ASSERT_TRUE(PULSE_AMPLITUDE > 0.0f && PULSE_AMPLITUDE <= 1.0f);
    ASSERT_TRUE(SHIMMER_AMPLITUDE > 0.0f && SHIMMER_AMPLITUDE <= 1.0f);
    ASSERT_TRUE(SUBTLE_AMPLITUDE > 0.0f && SUBTLE_AMPLITUDE <= 1.0f);

    // Verify period ranges per spec
    ASSERT_TRUE(WATER_PULSE_PERIOD >= 6.0f && WATER_PULSE_PERIOD <= 8.0f);
    ASSERT_FLOAT_EQ(BIOLUME_PULSE_PERIOD, 4.0f, 0.1f);
    ASSERT_FLOAT_EQ(SPORE_PULSE_PERIOD, 3.0f, 0.1f);
    ASSERT_FLOAT_EQ(EMBER_THROB_PERIOD, 5.0f, 0.1f);
}

// =============================================================================
// Test: Crevice glow configuration
// =============================================================================
TEST(test_crevice_glow_config) {
    using namespace sims3000::CreviceGlow;

    // Verify threshold is in valid range [0, 1]
    ASSERT_TRUE(NORMAL_THRESHOLD >= 0.0f && NORMAL_THRESHOLD <= 1.0f);

    // Verify max boost is greater than 1.0
    ASSERT_TRUE(MAX_BOOST > 1.0f);

    // Only Ridge (1) and EmberCrust (9) should have crevice glow
    ASSERT_TRUE(hasCreviceGlow(1));   // Ridge
    ASSERT_TRUE(hasCreviceGlow(9));   // EmberCrust
    ASSERT_TRUE(!hasCreviceGlow(0));  // Substrate
    ASSERT_TRUE(!hasCreviceGlow(2));  // DeepVoid
    ASSERT_TRUE(!hasCreviceGlow(6));  // PrismaFields
}

// =============================================================================
// Test: Base colors are dark (per alien aesthetic)
// =============================================================================
TEST(test_base_colors_are_dark) {
    sims3000::TerrainVisualConfig config;

    // Base colors should be dark (brightness < 0.5)
    for (std::size_t i = 0; i < sims3000::TERRAIN_PALETTE_SIZE; ++i) {
        float brightness = (config.base_colors[i].r +
                           config.base_colors[i].g +
                           config.base_colors[i].b) / 3.0f;
        ASSERT_TRUE(brightness < 0.5f);
    }
}

// =============================================================================
// Main
// =============================================================================
int main() {
    printf("=== TerrainVisualConfig Tests (Ticket 3-026) ===\n\n");

    RUN_TEST(test_struct_size);
    RUN_TEST(test_struct_alignment);
    RUN_TEST(test_default_construction);
    RUN_TEST(test_emissive_colors_from_terrain_info);
    RUN_TEST(test_emissive_intensity_hierarchy);
    RUN_TEST(test_set_glow_time);
    RUN_TEST(test_set_sea_level);
    RUN_TEST(test_set_base_color);
    RUN_TEST(test_set_emissive_color);
    RUN_TEST(test_out_of_bounds_base_color);
    RUN_TEST(test_get_gpu_size);
    RUN_TEST(test_get_data);
    RUN_TEST(test_glow_animation_constants);
    RUN_TEST(test_crevice_glow_config);
    RUN_TEST(test_base_colors_are_dark);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? 1 : 0;
}
