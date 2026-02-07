/**
 * @file test_render_layer.cpp
 * @brief Unit tests for RenderLayer enum and utilities.
 *
 * Tests verify:
 * - All layer values exist and have correct ordering
 * - Helper functions work correctly (name, validation, opacity, lighting)
 * - RENDER_LAYER_COUNT is accurate
 * - Layer usage in RenderComponent
 */

#include "sims3000/render/RenderLayer.h"
#include "sims3000/ecs/Components.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

// Simple test framework macros
#define TEST(name) static void test_##name()
#define RUN_TEST(name) do { \
    printf("  Running %s...\n", #name); \
    test_##name(); \
    passed++; \
} while(0)

#define ASSERT(cond) do { \
    if (!(cond)) { \
        printf("FAIL: %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        exit(1); \
    } \
} while(0)

#define ASSERT_EQ(expected, actual) do { \
    if ((expected) != (actual)) { \
        printf("FAIL: %s:%d: expected %d, got %d\n", __FILE__, __LINE__, \
               static_cast<int>(expected), static_cast<int>(actual)); \
        exit(1); \
    } \
} while(0)

#define ASSERT_STREQ(expected, actual) do { \
    if (strcmp((expected), (actual)) != 0) { \
        printf("FAIL: %s:%d: expected \"%s\", got \"%s\"\n", __FILE__, __LINE__, \
               (expected), (actual)); \
        exit(1); \
    } \
} while(0)

using namespace sims3000;

// =============================================================================
// Layer Value Tests
// =============================================================================

TEST(layer_values_are_sequential) {
    // Verify all layers have sequential values starting from 0
    ASSERT_EQ(0, static_cast<int>(RenderLayer::Underground));
    ASSERT_EQ(1, static_cast<int>(RenderLayer::Terrain));
    ASSERT_EQ(2, static_cast<int>(RenderLayer::Vegetation));
    ASSERT_EQ(3, static_cast<int>(RenderLayer::Water));
    ASSERT_EQ(4, static_cast<int>(RenderLayer::Roads));
    ASSERT_EQ(5, static_cast<int>(RenderLayer::Buildings));
    ASSERT_EQ(6, static_cast<int>(RenderLayer::Units));
    ASSERT_EQ(7, static_cast<int>(RenderLayer::Effects));
    ASSERT_EQ(8, static_cast<int>(RenderLayer::DataOverlay));
    ASSERT_EQ(9, static_cast<int>(RenderLayer::UIWorld));
}

TEST(layer_count_is_correct) {
    // RENDER_LAYER_COUNT should equal the number of layers (10)
    ASSERT_EQ(10u, RENDER_LAYER_COUNT);
}

TEST(layer_ordering_is_correct) {
    // Layers should be ordered for correct rendering (lower first, higher on top)
    ASSERT(RenderLayer::Underground < RenderLayer::Terrain);
    ASSERT(RenderLayer::Terrain < RenderLayer::Vegetation);
    ASSERT(RenderLayer::Vegetation < RenderLayer::Water);
    ASSERT(RenderLayer::Water < RenderLayer::Roads);
    ASSERT(RenderLayer::Roads < RenderLayer::Buildings);
    ASSERT(RenderLayer::Buildings < RenderLayer::Units);
    ASSERT(RenderLayer::Units < RenderLayer::Effects);
    ASSERT(RenderLayer::Effects < RenderLayer::DataOverlay);
    ASSERT(RenderLayer::DataOverlay < RenderLayer::UIWorld);
}

// =============================================================================
// getRenderLayerName() Tests
// =============================================================================

TEST(get_layer_name_underground) {
    ASSERT_STREQ("Underground", getRenderLayerName(RenderLayer::Underground));
}

TEST(get_layer_name_terrain) {
    ASSERT_STREQ("Terrain", getRenderLayerName(RenderLayer::Terrain));
}

TEST(get_layer_name_vegetation) {
    ASSERT_STREQ("Vegetation", getRenderLayerName(RenderLayer::Vegetation));
}

TEST(get_layer_name_water) {
    ASSERT_STREQ("Water", getRenderLayerName(RenderLayer::Water));
}

TEST(get_layer_name_roads) {
    ASSERT_STREQ("Roads", getRenderLayerName(RenderLayer::Roads));
}

TEST(get_layer_name_buildings) {
    ASSERT_STREQ("Buildings", getRenderLayerName(RenderLayer::Buildings));
}

TEST(get_layer_name_units) {
    ASSERT_STREQ("Units", getRenderLayerName(RenderLayer::Units));
}

TEST(get_layer_name_effects) {
    ASSERT_STREQ("Effects", getRenderLayerName(RenderLayer::Effects));
}

TEST(get_layer_name_data_overlay) {
    ASSERT_STREQ("DataOverlay", getRenderLayerName(RenderLayer::DataOverlay));
}

TEST(get_layer_name_ui_world) {
    ASSERT_STREQ("UIWorld", getRenderLayerName(RenderLayer::UIWorld));
}

TEST(get_layer_name_unknown) {
    // Out-of-range value should return "Unknown"
    ASSERT_STREQ("Unknown", getRenderLayerName(static_cast<RenderLayer>(255)));
}

// =============================================================================
// isValidRenderLayer() Tests
// =============================================================================

TEST(valid_layers_are_valid) {
    ASSERT(isValidRenderLayer(RenderLayer::Underground));
    ASSERT(isValidRenderLayer(RenderLayer::Terrain));
    ASSERT(isValidRenderLayer(RenderLayer::Vegetation));
    ASSERT(isValidRenderLayer(RenderLayer::Water));
    ASSERT(isValidRenderLayer(RenderLayer::Roads));
    ASSERT(isValidRenderLayer(RenderLayer::Buildings));
    ASSERT(isValidRenderLayer(RenderLayer::Units));
    ASSERT(isValidRenderLayer(RenderLayer::Effects));
    ASSERT(isValidRenderLayer(RenderLayer::DataOverlay));
    ASSERT(isValidRenderLayer(RenderLayer::UIWorld));
}

TEST(invalid_layers_are_invalid) {
    // Values >= RENDER_LAYER_COUNT should be invalid
    ASSERT(!isValidRenderLayer(static_cast<RenderLayer>(10)));
    ASSERT(!isValidRenderLayer(static_cast<RenderLayer>(11)));
    ASSERT(!isValidRenderLayer(static_cast<RenderLayer>(255)));
}

// =============================================================================
// isOpaqueLayer() Tests
// =============================================================================

TEST(opaque_layers_are_opaque) {
    // Scene geometry layers are opaque
    ASSERT(isOpaqueLayer(RenderLayer::Underground));
    ASSERT(isOpaqueLayer(RenderLayer::Terrain));
    ASSERT(isOpaqueLayer(RenderLayer::Vegetation));
    ASSERT(isOpaqueLayer(RenderLayer::Roads));
    ASSERT(isOpaqueLayer(RenderLayer::Buildings));
    ASSERT(isOpaqueLayer(RenderLayer::Units));
}

TEST(transparent_layers_are_not_opaque) {
    // Overlay and effect layers require transparency
    ASSERT(!isOpaqueLayer(RenderLayer::Water));
    ASSERT(!isOpaqueLayer(RenderLayer::Effects));
    ASSERT(!isOpaqueLayer(RenderLayer::DataOverlay));
    ASSERT(!isOpaqueLayer(RenderLayer::UIWorld));
}

// =============================================================================
// isLitLayer() Tests
// =============================================================================

TEST(lit_layers_use_lighting) {
    // 3D scene layers use world lighting
    ASSERT(isLitLayer(RenderLayer::Underground));
    ASSERT(isLitLayer(RenderLayer::Terrain));
    ASSERT(isLitLayer(RenderLayer::Vegetation));
    ASSERT(isLitLayer(RenderLayer::Water));
    ASSERT(isLitLayer(RenderLayer::Roads));
    ASSERT(isLitLayer(RenderLayer::Buildings));
    ASSERT(isLitLayer(RenderLayer::Units));
    ASSERT(isLitLayer(RenderLayer::Effects));
}

TEST(unlit_layers_skip_lighting) {
    // UI and overlay layers don't use world lighting
    ASSERT(!isLitLayer(RenderLayer::DataOverlay));
    ASSERT(!isLitLayer(RenderLayer::UIWorld));
}

// =============================================================================
// RenderComponent Integration Tests
// =============================================================================

TEST(render_component_default_layer) {
    RenderComponent comp;
    // Default layer should be Buildings (most common use case)
    ASSERT_EQ(RenderLayer::Buildings, comp.layer);
}

TEST(render_component_layer_assignment) {
    RenderComponent comp;

    // Test assigning each layer
    comp.layer = RenderLayer::Underground;
    ASSERT_EQ(RenderLayer::Underground, comp.layer);

    comp.layer = RenderLayer::Terrain;
    ASSERT_EQ(RenderLayer::Terrain, comp.layer);

    comp.layer = RenderLayer::Vegetation;
    ASSERT_EQ(RenderLayer::Vegetation, comp.layer);

    comp.layer = RenderLayer::Water;
    ASSERT_EQ(RenderLayer::Water, comp.layer);

    comp.layer = RenderLayer::Roads;
    ASSERT_EQ(RenderLayer::Roads, comp.layer);

    comp.layer = RenderLayer::Units;
    ASSERT_EQ(RenderLayer::Units, comp.layer);

    comp.layer = RenderLayer::Effects;
    ASSERT_EQ(RenderLayer::Effects, comp.layer);

    comp.layer = RenderLayer::DataOverlay;
    ASSERT_EQ(RenderLayer::DataOverlay, comp.layer);

    comp.layer = RenderLayer::UIWorld;
    ASSERT_EQ(RenderLayer::UIWorld, comp.layer);
}

TEST(render_component_size_unchanged) {
    // Verify RenderComponent size is still 56 bytes (not changed by layer enum)
    static_assert(sizeof(RenderComponent) == 56, "RenderComponent size check");
    ASSERT_EQ(56u, sizeof(RenderComponent));
}

// =============================================================================
// Compile-Time Constexpr Tests
// =============================================================================

TEST(constexpr_layer_count) {
    // These should all be constexpr-evaluable
    constexpr std::size_t count = RENDER_LAYER_COUNT;
    ASSERT_EQ(10u, count);
}

TEST(constexpr_layer_name) {
    // constexpr name lookup
    constexpr const char* name = getRenderLayerName(RenderLayer::Buildings);
    ASSERT_STREQ("Buildings", name);
}

TEST(constexpr_is_valid) {
    constexpr bool valid = isValidRenderLayer(RenderLayer::Buildings);
    constexpr bool invalid = isValidRenderLayer(static_cast<RenderLayer>(100));
    ASSERT(valid);
    ASSERT(!invalid);
}

TEST(constexpr_is_opaque) {
    constexpr bool opaque = isOpaqueLayer(RenderLayer::Buildings);
    constexpr bool transparent = isOpaqueLayer(RenderLayer::Water);
    ASSERT(opaque);
    ASSERT(!transparent);
}

TEST(constexpr_is_lit) {
    constexpr bool lit = isLitLayer(RenderLayer::Buildings);
    constexpr bool unlit = isLitLayer(RenderLayer::UIWorld);
    ASSERT(lit);
    ASSERT(!unlit);
}

// =============================================================================
// Main
// =============================================================================

int main() {
    int passed = 0;

    printf("Running RenderLayer tests...\n\n");

    printf("Layer Value Tests:\n");
    RUN_TEST(layer_values_are_sequential);
    RUN_TEST(layer_count_is_correct);
    RUN_TEST(layer_ordering_is_correct);

    printf("\ngetRenderLayerName() Tests:\n");
    RUN_TEST(get_layer_name_underground);
    RUN_TEST(get_layer_name_terrain);
    RUN_TEST(get_layer_name_vegetation);
    RUN_TEST(get_layer_name_water);
    RUN_TEST(get_layer_name_roads);
    RUN_TEST(get_layer_name_buildings);
    RUN_TEST(get_layer_name_units);
    RUN_TEST(get_layer_name_effects);
    RUN_TEST(get_layer_name_data_overlay);
    RUN_TEST(get_layer_name_ui_world);
    RUN_TEST(get_layer_name_unknown);

    printf("\nisValidRenderLayer() Tests:\n");
    RUN_TEST(valid_layers_are_valid);
    RUN_TEST(invalid_layers_are_invalid);

    printf("\nisOpaqueLayer() Tests:\n");
    RUN_TEST(opaque_layers_are_opaque);
    RUN_TEST(transparent_layers_are_not_opaque);

    printf("\nisLitLayer() Tests:\n");
    RUN_TEST(lit_layers_use_lighting);
    RUN_TEST(unlit_layers_skip_lighting);

    printf("\nRenderComponent Integration Tests:\n");
    RUN_TEST(render_component_default_layer);
    RUN_TEST(render_component_layer_assignment);
    RUN_TEST(render_component_size_unchanged);

    printf("\nConstexpr Tests:\n");
    RUN_TEST(constexpr_layer_count);
    RUN_TEST(constexpr_layer_name);
    RUN_TEST(constexpr_is_valid);
    RUN_TEST(constexpr_is_opaque);
    RUN_TEST(constexpr_is_lit);

    printf("\n========================================\n");
    printf("All %d tests passed!\n", passed);
    printf("========================================\n");

    return 0;
}
