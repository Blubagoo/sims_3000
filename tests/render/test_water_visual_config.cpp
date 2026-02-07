/**
 * @file test_water_visual_config.cpp
 * @brief Unit tests for WaterVisualConfig (Ticket 3-028)
 *
 * Tests the water visual configuration struct used for GPU uniform buffer
 * upload. Verifies:
 * - Struct size matches GPU alignment requirements (112 bytes)
 * - Default initialization provides correct water colors
 * - Semi-transparent alpha is in range 0.7-0.8
 * - Emissive colors are set for ocean, river, lake
 * - Flow direction setters work correctly
 * - Water body type can be set
 * - Getters and setters function correctly
 */

#include <sims3000/render/WaterVisualConfig.h>
#include <sims3000/terrain/WaterData.h>
#include <cstdio>
#include <cmath>

using namespace sims3000;
using namespace sims3000::terrain;

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

#define ASSERT_FALSE(cond) do { \
    if (cond) { \
        printf("FAILED: NOT %s at line %d\n", #cond, __LINE__); \
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
        printf("FAILED: %s ~= %s at line %d (got %f, expected %f)\n", \
               #a, #b, __LINE__, (float)(a), (float)(b)); \
        tests_failed++; \
        return; \
    } \
} while(0)

// =============================================================================
// Test: Struct size matches GPU buffer requirements (112 bytes)
// =============================================================================
TEST(test_struct_size) {
    ASSERT_EQ(sizeof(WaterVisualConfig), 112u);
}

// =============================================================================
// Test: Struct alignment for GPU upload (16-byte aligned)
// =============================================================================
TEST(test_struct_alignment) {
    ASSERT_TRUE(alignof(WaterVisualConfig) >= 16);
}

// =============================================================================
// Test: Default construction initializes correctly
// =============================================================================
TEST(test_default_construction) {
    WaterVisualConfig config;

    // Base color should be very dark blue/teal
    ASSERT_FLOAT_EQ(config.base_color.r, WaterVisualConstants::BASE_COLOR_R, 0.001f);
    ASSERT_FLOAT_EQ(config.base_color.g, WaterVisualConstants::BASE_COLOR_G, 0.001f);
    ASSERT_FLOAT_EQ(config.base_color.b, WaterVisualConstants::BASE_COLOR_B, 0.001f);

    // Alpha should be default (0.75)
    ASSERT_FLOAT_EQ(config.base_color.a, WaterVisualConstants::WATER_ALPHA_DEFAULT, 0.001f);

    // Glow time starts at 0
    ASSERT_FLOAT_EQ(config.glow_time, 0.0f, 0.001f);

    // Flow direction starts at 0
    ASSERT_FLOAT_EQ(config.flow_dx, 0.0f, 0.001f);
    ASSERT_FLOAT_EQ(config.flow_dy, 0.0f, 0.001f);

    // Water body type starts at 0 (Ocean)
    ASSERT_EQ(config.water_body_type, 0u);

    // Ambient strength should be default
    ASSERT_FLOAT_EQ(config.ambient_strength, WaterVisualConstants::AMBIENT_STRENGTH, 0.001f);
}

// =============================================================================
// Test: Alpha is in semi-transparent range (0.7-0.8)
// =============================================================================
TEST(test_alpha_in_range) {
    WaterVisualConfig config;

    float alpha = config.getAlpha();
    ASSERT_TRUE(alpha >= WaterVisualConstants::WATER_ALPHA_MIN);
    ASSERT_TRUE(alpha <= WaterVisualConstants::WATER_ALPHA_MAX);
}

// =============================================================================
// Test: Ocean emissive color is blue-white
// =============================================================================
TEST(test_ocean_emissive) {
    WaterVisualConfig config;

    // Ocean should have blue-white glow
    ASSERT_FLOAT_EQ(config.ocean_emissive.r, WaterVisualConstants::OCEAN_EMISSIVE_R, 0.001f);
    ASSERT_FLOAT_EQ(config.ocean_emissive.g, WaterVisualConstants::OCEAN_EMISSIVE_G, 0.001f);
    ASSERT_FLOAT_EQ(config.ocean_emissive.b, WaterVisualConstants::OCEAN_EMISSIVE_B, 0.001f);
    ASSERT_FLOAT_EQ(config.ocean_emissive.a, WaterVisualConstants::OCEAN_EMISSIVE_INTENSITY, 0.001f);

    // Blue component should be highest for blue-white glow
    ASSERT_TRUE(config.ocean_emissive.b > config.ocean_emissive.r);
}

// =============================================================================
// Test: River emissive color is teal
// =============================================================================
TEST(test_river_emissive) {
    WaterVisualConfig config;

    // River should have teal glow
    ASSERT_FLOAT_EQ(config.river_emissive.r, WaterVisualConstants::RIVER_EMISSIVE_R, 0.001f);
    ASSERT_FLOAT_EQ(config.river_emissive.g, WaterVisualConstants::RIVER_EMISSIVE_G, 0.001f);
    ASSERT_FLOAT_EQ(config.river_emissive.b, WaterVisualConstants::RIVER_EMISSIVE_B, 0.001f);
    ASSERT_FLOAT_EQ(config.river_emissive.a, WaterVisualConstants::RIVER_EMISSIVE_INTENSITY, 0.001f);

    // Green component should be highest for teal
    ASSERT_TRUE(config.river_emissive.g > config.river_emissive.r);
}

// =============================================================================
// Test: Lake emissive color is blue-white (calmer)
// =============================================================================
TEST(test_lake_emissive) {
    WaterVisualConfig config;

    // Lake should have blue-white glow (calmer than ocean)
    ASSERT_FLOAT_EQ(config.lake_emissive.r, WaterVisualConstants::LAKE_EMISSIVE_R, 0.001f);
    ASSERT_FLOAT_EQ(config.lake_emissive.g, WaterVisualConstants::LAKE_EMISSIVE_G, 0.001f);
    ASSERT_FLOAT_EQ(config.lake_emissive.b, WaterVisualConstants::LAKE_EMISSIVE_B, 0.001f);
    ASSERT_FLOAT_EQ(config.lake_emissive.a, WaterVisualConstants::LAKE_EMISSIVE_INTENSITY, 0.001f);

    // Lake intensity should be less than ocean (calmer)
    ASSERT_TRUE(config.lake_emissive.a < config.ocean_emissive.a);
}

// =============================================================================
// Test: setGlowTime updates glow_time
// =============================================================================
TEST(test_set_glow_time) {
    WaterVisualConfig config;

    config.setGlowTime(5.5f);
    ASSERT_FLOAT_EQ(config.glow_time, 5.5f, 0.001f);

    config.setGlowTime(123.456f);
    ASSERT_FLOAT_EQ(config.glow_time, 123.456f, 0.001f);
}

// =============================================================================
// Test: setFlowDirection correctly maps FlowDirection enum
// =============================================================================
TEST(test_set_flow_direction) {
    WaterVisualConfig config;

    // Test North (-Y direction)
    config.setFlowDirection(FlowDirection::N);
    ASSERT_FLOAT_EQ(config.flow_dx, 0.0f, 0.001f);
    ASSERT_FLOAT_EQ(config.flow_dy, -1.0f, 0.001f);

    // Test East (+X direction)
    config.setFlowDirection(FlowDirection::E);
    ASSERT_FLOAT_EQ(config.flow_dx, 1.0f, 0.001f);
    ASSERT_FLOAT_EQ(config.flow_dy, 0.0f, 0.001f);

    // Test South (+Y direction)
    config.setFlowDirection(FlowDirection::S);
    ASSERT_FLOAT_EQ(config.flow_dx, 0.0f, 0.001f);
    ASSERT_FLOAT_EQ(config.flow_dy, 1.0f, 0.001f);

    // Test West (-X direction)
    config.setFlowDirection(FlowDirection::W);
    ASSERT_FLOAT_EQ(config.flow_dx, -1.0f, 0.001f);
    ASSERT_FLOAT_EQ(config.flow_dy, 0.0f, 0.001f);

    // Test diagonal: NE (+X, -Y)
    config.setFlowDirection(FlowDirection::NE);
    ASSERT_FLOAT_EQ(config.flow_dx, 1.0f, 0.001f);
    ASSERT_FLOAT_EQ(config.flow_dy, -1.0f, 0.001f);

    // Test None (no flow)
    config.setFlowDirection(FlowDirection::None);
    ASSERT_FLOAT_EQ(config.flow_dx, 0.0f, 0.001f);
    ASSERT_FLOAT_EQ(config.flow_dy, 0.0f, 0.001f);
}

// =============================================================================
// Test: setWaterBodyType sets water type correctly
// =============================================================================
TEST(test_set_water_body_type) {
    WaterVisualConfig config;

    config.setWaterBodyType(WaterBodyType::Ocean);
    ASSERT_EQ(config.water_body_type, 0u);

    config.setWaterBodyType(WaterBodyType::River);
    ASSERT_EQ(config.water_body_type, 1u);

    config.setWaterBodyType(WaterBodyType::Lake);
    ASSERT_EQ(config.water_body_type, 2u);
}

// =============================================================================
// Test: setSunDirection normalizes direction
// =============================================================================
TEST(test_set_sun_direction) {
    WaterVisualConfig config;

    // Set a non-normalized direction
    config.setSunDirection(glm::vec3(3.0f, 4.0f, 0.0f));

    // Should be normalized
    float length = glm::length(config.sun_direction);
    ASSERT_FLOAT_EQ(length, 1.0f, 0.001f);

    // Check normalized values (3, 4, 0) / 5 = (0.6, 0.8, 0)
    ASSERT_FLOAT_EQ(config.sun_direction.x, 0.6f, 0.001f);
    ASSERT_FLOAT_EQ(config.sun_direction.y, 0.8f, 0.001f);
    ASSERT_FLOAT_EQ(config.sun_direction.z, 0.0f, 0.001f);
}

// =============================================================================
// Test: setAmbientStrength clamps to valid range
// =============================================================================
TEST(test_set_ambient_strength) {
    WaterVisualConfig config;

    config.setAmbientStrength(0.5f);
    ASSERT_FLOAT_EQ(config.ambient_strength, 0.5f, 0.001f);

    // Test clamping to 0
    config.setAmbientStrength(-0.5f);
    ASSERT_FLOAT_EQ(config.ambient_strength, 0.0f, 0.001f);

    // Test clamping to 1
    config.setAmbientStrength(1.5f);
    ASSERT_FLOAT_EQ(config.ambient_strength, 1.0f, 0.001f);
}

// =============================================================================
// Test: setBaseColor updates RGB but preserves alpha
// =============================================================================
TEST(test_set_base_color) {
    WaterVisualConfig config;

    float originalAlpha = config.base_color.a;

    config.setBaseColor(glm::vec3(0.1f, 0.2f, 0.3f));

    ASSERT_FLOAT_EQ(config.base_color.r, 0.1f, 0.001f);
    ASSERT_FLOAT_EQ(config.base_color.g, 0.2f, 0.001f);
    ASSERT_FLOAT_EQ(config.base_color.b, 0.3f, 0.001f);
    ASSERT_FLOAT_EQ(config.base_color.a, originalAlpha, 0.001f);
}

// =============================================================================
// Test: setAlpha clamps to valid range
// =============================================================================
TEST(test_set_alpha) {
    WaterVisualConfig config;

    config.setAlpha(0.75f);
    ASSERT_FLOAT_EQ(config.getAlpha(), 0.75f, 0.001f);

    // Test clamping to 0
    config.setAlpha(-0.5f);
    ASSERT_FLOAT_EQ(config.getAlpha(), 0.0f, 0.001f);

    // Test clamping to 1
    config.setAlpha(1.5f);
    ASSERT_FLOAT_EQ(config.getAlpha(), 1.0f, 0.001f);
}

// =============================================================================
// Test: setOceanEmissive updates color and intensity
// =============================================================================
TEST(test_set_ocean_emissive) {
    WaterVisualConfig config;

    config.setOceanEmissive(glm::vec3(1.0f, 0.5f, 0.25f), 0.5f);

    ASSERT_FLOAT_EQ(config.ocean_emissive.r, 1.0f, 0.001f);
    ASSERT_FLOAT_EQ(config.ocean_emissive.g, 0.5f, 0.001f);
    ASSERT_FLOAT_EQ(config.ocean_emissive.b, 0.25f, 0.001f);
    ASSERT_FLOAT_EQ(config.ocean_emissive.a, 0.5f, 0.001f);
}

// =============================================================================
// Test: setRiverEmissive updates color and intensity
// =============================================================================
TEST(test_set_river_emissive) {
    WaterVisualConfig config;

    config.setRiverEmissive(glm::vec3(0.1f, 0.8f, 0.7f), 0.3f);

    ASSERT_FLOAT_EQ(config.river_emissive.r, 0.1f, 0.001f);
    ASSERT_FLOAT_EQ(config.river_emissive.g, 0.8f, 0.001f);
    ASSERT_FLOAT_EQ(config.river_emissive.b, 0.7f, 0.001f);
    ASSERT_FLOAT_EQ(config.river_emissive.a, 0.3f, 0.001f);
}

// =============================================================================
// Test: setLakeEmissive updates color and intensity
// =============================================================================
TEST(test_set_lake_emissive) {
    WaterVisualConfig config;

    config.setLakeEmissive(glm::vec3(0.2f, 0.4f, 0.9f), 0.2f);

    ASSERT_FLOAT_EQ(config.lake_emissive.r, 0.2f, 0.001f);
    ASSERT_FLOAT_EQ(config.lake_emissive.g, 0.4f, 0.001f);
    ASSERT_FLOAT_EQ(config.lake_emissive.b, 0.9f, 0.001f);
    ASSERT_FLOAT_EQ(config.lake_emissive.a, 0.2f, 0.001f);
}

// =============================================================================
// Test: getGPUSize returns correct size
// =============================================================================
TEST(test_get_gpu_size) {
    ASSERT_EQ(WaterVisualConfig::getGPUSize(), 112u);
}

// =============================================================================
// Test: getData returns pointer to struct
// =============================================================================
TEST(test_get_data) {
    WaterVisualConfig config;
    const void* ptr = config.getData();

    ASSERT_TRUE(ptr == &config);
}

// =============================================================================
// Test: getFlowVelocity helper function
// =============================================================================
TEST(test_get_flow_velocity) {
    // Test all 8 directions
    glm::vec2 vel;

    vel = getFlowVelocity(FlowDirection::N);
    ASSERT_FLOAT_EQ(vel.x, 0.0f, 0.001f);
    ASSERT_FLOAT_EQ(vel.y, -1.0f, 0.001f);

    vel = getFlowVelocity(FlowDirection::E);
    ASSERT_FLOAT_EQ(vel.x, 1.0f, 0.001f);
    ASSERT_FLOAT_EQ(vel.y, 0.0f, 0.001f);

    vel = getFlowVelocity(FlowDirection::S);
    ASSERT_FLOAT_EQ(vel.x, 0.0f, 0.001f);
    ASSERT_FLOAT_EQ(vel.y, 1.0f, 0.001f);

    vel = getFlowVelocity(FlowDirection::W);
    ASSERT_FLOAT_EQ(vel.x, -1.0f, 0.001f);
    ASSERT_FLOAT_EQ(vel.y, 0.0f, 0.001f);

    vel = getFlowVelocity(FlowDirection::NE);
    ASSERT_FLOAT_EQ(vel.x, 1.0f, 0.001f);
    ASSERT_FLOAT_EQ(vel.y, -1.0f, 0.001f);

    vel = getFlowVelocity(FlowDirection::SE);
    ASSERT_FLOAT_EQ(vel.x, 1.0f, 0.001f);
    ASSERT_FLOAT_EQ(vel.y, 1.0f, 0.001f);

    vel = getFlowVelocity(FlowDirection::SW);
    ASSERT_FLOAT_EQ(vel.x, -1.0f, 0.001f);
    ASSERT_FLOAT_EQ(vel.y, 1.0f, 0.001f);

    vel = getFlowVelocity(FlowDirection::NW);
    ASSERT_FLOAT_EQ(vel.x, -1.0f, 0.001f);
    ASSERT_FLOAT_EQ(vel.y, -1.0f, 0.001f);

    vel = getFlowVelocity(FlowDirection::None);
    ASSERT_FLOAT_EQ(vel.x, 0.0f, 0.001f);
    ASSERT_FLOAT_EQ(vel.y, 0.0f, 0.001f);
}

// =============================================================================
// Test: Water render state constants
// =============================================================================
TEST(test_water_render_state_constants) {
    // Depth test ON, depth write OFF for water
    ASSERT_TRUE(WaterRenderState::DEPTH_TEST_ENABLED);
    ASSERT_FALSE(WaterRenderState::DEPTH_WRITE_ENABLED);

    // Blend enabled for semi-transparency
    ASSERT_TRUE(WaterRenderState::BLEND_ENABLED);

    // Back-face culling for water surface
    ASSERT_TRUE(WaterRenderState::CULL_BACK_FACE);
}

// =============================================================================
// Test: Water visual constants are in valid ranges
// =============================================================================
TEST(test_water_visual_constants) {
    using namespace WaterVisualConstants;

    // Alpha range is valid
    ASSERT_TRUE(WATER_ALPHA_MIN >= 0.0f && WATER_ALPHA_MIN <= 1.0f);
    ASSERT_TRUE(WATER_ALPHA_MAX >= 0.0f && WATER_ALPHA_MAX <= 1.0f);
    ASSERT_TRUE(WATER_ALPHA_MIN < WATER_ALPHA_MAX);
    ASSERT_TRUE(WATER_ALPHA_DEFAULT >= WATER_ALPHA_MIN);
    ASSERT_TRUE(WATER_ALPHA_DEFAULT <= WATER_ALPHA_MAX);

    // Base color is very dark (< 0.1 for each component)
    ASSERT_TRUE(BASE_COLOR_R < 0.1f);
    ASSERT_TRUE(BASE_COLOR_G < 0.1f);
    ASSERT_TRUE(BASE_COLOR_B < 0.1f);

    // Emissive intensities are in valid range
    ASSERT_TRUE(OCEAN_EMISSIVE_INTENSITY > 0.0f && OCEAN_EMISSIVE_INTENSITY < 1.0f);
    ASSERT_TRUE(RIVER_EMISSIVE_INTENSITY > 0.0f && RIVER_EMISSIVE_INTENSITY < 1.0f);
    ASSERT_TRUE(LAKE_EMISSIVE_INTENSITY > 0.0f && LAKE_EMISSIVE_INTENSITY < 1.0f);

    // Animation periods are positive
    ASSERT_TRUE(OCEAN_PULSE_PERIOD > 0.0f);
    ASSERT_TRUE(LAKE_PULSE_PERIOD > 0.0f);

    // Ocean pulse period is ~6s, lake is ~8s (per spec)
    ASSERT_FLOAT_EQ(OCEAN_PULSE_PERIOD, 6.0f, 0.1f);
    ASSERT_FLOAT_EQ(LAKE_PULSE_PERIOD, 8.0f, 0.1f);
}

// =============================================================================
// Test: Base color is dark (barely visible without glow)
// =============================================================================
TEST(test_base_color_is_dark) {
    WaterVisualConfig config;

    // Calculate brightness (average of RGB)
    float brightness = (config.base_color.r + config.base_color.g + config.base_color.b) / 3.0f;

    // Base should be very dark (brightness < 0.1)
    ASSERT_TRUE(brightness < 0.1f);
}

// =============================================================================
// Test: Shoreline glow colors match water types
// =============================================================================
TEST(test_shoreline_glow_colors) {
    WaterVisualConfig config;

    // Ocean: blue-white (blue > green > red)
    ASSERT_TRUE(config.ocean_emissive.b > config.ocean_emissive.g);
    ASSERT_TRUE(config.ocean_emissive.g > config.ocean_emissive.r);

    // River: teal (green highest)
    ASSERT_TRUE(config.river_emissive.g > config.river_emissive.r);
    ASSERT_TRUE(config.river_emissive.g > config.river_emissive.b);

    // Lake: blue-white (blue > green > red)
    ASSERT_TRUE(config.lake_emissive.b > config.lake_emissive.g);
    ASSERT_TRUE(config.lake_emissive.g > config.lake_emissive.r);
}

// =============================================================================
// Main
// =============================================================================
int main() {
    printf("=== WaterVisualConfig Tests (Ticket 3-028) ===\n\n");

    RUN_TEST(test_struct_size);
    RUN_TEST(test_struct_alignment);
    RUN_TEST(test_default_construction);
    RUN_TEST(test_alpha_in_range);
    RUN_TEST(test_ocean_emissive);
    RUN_TEST(test_river_emissive);
    RUN_TEST(test_lake_emissive);
    RUN_TEST(test_set_glow_time);
    RUN_TEST(test_set_flow_direction);
    RUN_TEST(test_set_water_body_type);
    RUN_TEST(test_set_sun_direction);
    RUN_TEST(test_set_ambient_strength);
    RUN_TEST(test_set_base_color);
    RUN_TEST(test_set_alpha);
    RUN_TEST(test_set_ocean_emissive);
    RUN_TEST(test_set_river_emissive);
    RUN_TEST(test_set_lake_emissive);
    RUN_TEST(test_get_gpu_size);
    RUN_TEST(test_get_data);
    RUN_TEST(test_get_flow_velocity);
    RUN_TEST(test_water_render_state_constants);
    RUN_TEST(test_water_visual_constants);
    RUN_TEST(test_base_color_is_dark);
    RUN_TEST(test_shoreline_glow_colors);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? 1 : 0;
}
