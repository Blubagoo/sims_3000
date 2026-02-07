/**
 * @file test_layer_visibility.cpp
 * @brief Unit tests for LayerVisibility class.
 *
 * Tests verify:
 * - Default state initialization
 * - setLayerVisibility() and getState() for all layers
 * - shouldRender(), isGhost(), isVisible(), isHidden() queries
 * - Bulk operations (resetAll, setAllLayers, setLayerRange)
 * - Ghost alpha configuration
 * - Underground view mode preset
 * - State counting statistics
 */

#include "sims3000/render/LayerVisibility.h"
#include "sims3000/render/RenderLayer.h"

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

#define ASSERT_FLOAT_EQ(expected, actual, epsilon) do { \
    float diff = (expected) - (actual); \
    if (diff < 0) diff = -diff; \
    if (diff > (epsilon)) { \
        printf("FAIL: %s:%d: expected %f, got %f\n", __FILE__, __LINE__, \
               static_cast<double>(expected), static_cast<double>(actual)); \
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
// LayerState Enum Tests
// =============================================================================

TEST(layer_state_values) {
    ASSERT_EQ(0, static_cast<int>(LayerState::Visible));
    ASSERT_EQ(1, static_cast<int>(LayerState::Hidden));
    ASSERT_EQ(2, static_cast<int>(LayerState::Ghost));
}

TEST(layer_state_names) {
    ASSERT_STREQ("Visible", getLayerStateName(LayerState::Visible));
    ASSERT_STREQ("Hidden", getLayerStateName(LayerState::Hidden));
    ASSERT_STREQ("Ghost", getLayerStateName(LayerState::Ghost));
    ASSERT_STREQ("Unknown", getLayerStateName(static_cast<LayerState>(255)));
}

TEST(layer_state_validation) {
    ASSERT(isValidLayerState(LayerState::Visible));
    ASSERT(isValidLayerState(LayerState::Hidden));
    ASSERT(isValidLayerState(LayerState::Ghost));
    ASSERT(!isValidLayerState(static_cast<LayerState>(3)));
    ASSERT(!isValidLayerState(static_cast<LayerState>(255)));
}

// =============================================================================
// Default State Tests
// =============================================================================

TEST(default_construction) {
    LayerVisibility visibility;

    // All layers except Underground should be Visible by default
    ASSERT_EQ(LayerState::Hidden, visibility.getState(RenderLayer::Underground));
    ASSERT_EQ(LayerState::Visible, visibility.getState(RenderLayer::Terrain));
    ASSERT_EQ(LayerState::Visible, visibility.getState(RenderLayer::Water));
    ASSERT_EQ(LayerState::Visible, visibility.getState(RenderLayer::Roads));
    ASSERT_EQ(LayerState::Visible, visibility.getState(RenderLayer::Buildings));
    ASSERT_EQ(LayerState::Visible, visibility.getState(RenderLayer::Units));
    ASSERT_EQ(LayerState::Visible, visibility.getState(RenderLayer::Effects));
    ASSERT_EQ(LayerState::Visible, visibility.getState(RenderLayer::DataOverlay));
    ASSERT_EQ(LayerState::Visible, visibility.getState(RenderLayer::UIWorld));
}

TEST(default_ghost_alpha) {
    LayerVisibility visibility;
    ASSERT_FLOAT_EQ(0.3f, visibility.getGhostAlpha(), 0.001f);
}

TEST(custom_config_construction) {
    LayerVisibilityConfig config;
    config.ghostAlpha = 0.5f;
    config.allowOpaqueGhost = false;

    LayerVisibility visibility(config);
    ASSERT_FLOAT_EQ(0.5f, visibility.getGhostAlpha(), 0.001f);
    ASSERT(!visibility.getConfig().allowOpaqueGhost);
}

// =============================================================================
// setLayerVisibility() Tests
// =============================================================================

TEST(set_layer_visibility_visible) {
    LayerVisibility visibility;

    visibility.setLayerVisibility(RenderLayer::Underground, LayerState::Visible);
    ASSERT_EQ(LayerState::Visible, visibility.getState(RenderLayer::Underground));
}

TEST(set_layer_visibility_hidden) {
    LayerVisibility visibility;

    visibility.setLayerVisibility(RenderLayer::Buildings, LayerState::Hidden);
    ASSERT_EQ(LayerState::Hidden, visibility.getState(RenderLayer::Buildings));
}

TEST(set_layer_visibility_ghost) {
    LayerVisibility visibility;

    visibility.setLayerVisibility(RenderLayer::Terrain, LayerState::Ghost);
    ASSERT_EQ(LayerState::Ghost, visibility.getState(RenderLayer::Terrain));
}

TEST(set_all_layers_to_each_state) {
    LayerVisibility visibility;

    // Test setting each layer to each state
    for (std::size_t i = 0; i < RENDER_LAYER_COUNT; ++i) {
        RenderLayer layer = static_cast<RenderLayer>(i);

        visibility.setLayerVisibility(layer, LayerState::Visible);
        ASSERT_EQ(LayerState::Visible, visibility.getState(layer));

        visibility.setLayerVisibility(layer, LayerState::Hidden);
        ASSERT_EQ(LayerState::Hidden, visibility.getState(layer));

        visibility.setLayerVisibility(layer, LayerState::Ghost);
        ASSERT_EQ(LayerState::Ghost, visibility.getState(layer));
    }
}

TEST(set_invalid_layer_ignored) {
    LayerVisibility visibility;

    // Invalid layer should be ignored without crash
    visibility.setLayerVisibility(static_cast<RenderLayer>(255), LayerState::Hidden);

    // Verify other layers unchanged
    ASSERT_EQ(LayerState::Visible, visibility.getState(RenderLayer::Buildings));
}

TEST(set_invalid_state_ignored) {
    LayerVisibility visibility;

    // Invalid state should be ignored
    visibility.setLayerVisibility(RenderLayer::Buildings, static_cast<LayerState>(99));

    // State should remain at default
    ASSERT_EQ(LayerState::Visible, visibility.getState(RenderLayer::Buildings));
}

// =============================================================================
// Query Function Tests
// =============================================================================

TEST(should_render_visible_layer) {
    LayerVisibility visibility;

    visibility.setLayerVisibility(RenderLayer::Buildings, LayerState::Visible);
    ASSERT(visibility.shouldRender(RenderLayer::Buildings));
}

TEST(should_render_ghost_layer) {
    LayerVisibility visibility;

    visibility.setLayerVisibility(RenderLayer::Buildings, LayerState::Ghost);
    ASSERT(visibility.shouldRender(RenderLayer::Buildings));
}

TEST(should_not_render_hidden_layer) {
    LayerVisibility visibility;

    visibility.setLayerVisibility(RenderLayer::Buildings, LayerState::Hidden);
    ASSERT(!visibility.shouldRender(RenderLayer::Buildings));
}

TEST(is_ghost_true) {
    LayerVisibility visibility;

    visibility.setLayerVisibility(RenderLayer::Terrain, LayerState::Ghost);
    ASSERT(visibility.isGhost(RenderLayer::Terrain));
}

TEST(is_ghost_false_when_visible) {
    LayerVisibility visibility;

    visibility.setLayerVisibility(RenderLayer::Terrain, LayerState::Visible);
    ASSERT(!visibility.isGhost(RenderLayer::Terrain));
}

TEST(is_ghost_false_when_hidden) {
    LayerVisibility visibility;

    visibility.setLayerVisibility(RenderLayer::Terrain, LayerState::Hidden);
    ASSERT(!visibility.isGhost(RenderLayer::Terrain));
}

TEST(is_visible_true) {
    LayerVisibility visibility;

    ASSERT(visibility.isVisible(RenderLayer::Buildings));
}

TEST(is_visible_false_when_ghost) {
    LayerVisibility visibility;

    visibility.setLayerVisibility(RenderLayer::Buildings, LayerState::Ghost);
    ASSERT(!visibility.isVisible(RenderLayer::Buildings));
}

TEST(is_visible_false_when_hidden) {
    LayerVisibility visibility;

    visibility.setLayerVisibility(RenderLayer::Buildings, LayerState::Hidden);
    ASSERT(!visibility.isVisible(RenderLayer::Buildings));
}

TEST(is_hidden_true) {
    LayerVisibility visibility;

    visibility.setLayerVisibility(RenderLayer::Buildings, LayerState::Hidden);
    ASSERT(visibility.isHidden(RenderLayer::Buildings));
}

TEST(is_hidden_false_when_visible) {
    LayerVisibility visibility;

    ASSERT(!visibility.isHidden(RenderLayer::Buildings));
}

TEST(is_hidden_false_when_ghost) {
    LayerVisibility visibility;

    visibility.setLayerVisibility(RenderLayer::Buildings, LayerState::Ghost);
    ASSERT(!visibility.isHidden(RenderLayer::Buildings));
}

TEST(query_invalid_layer) {
    LayerVisibility visibility;

    RenderLayer invalid = static_cast<RenderLayer>(255);

    // Invalid layers return safe defaults
    ASSERT_EQ(LayerState::Visible, visibility.getState(invalid));
    ASSERT(visibility.shouldRender(invalid));
    ASSERT(!visibility.isGhost(invalid));
    ASSERT(visibility.isVisible(invalid));
    ASSERT(!visibility.isHidden(invalid));
}

// =============================================================================
// Bulk Operation Tests
// =============================================================================

TEST(reset_all) {
    LayerVisibility visibility;

    // Set some layers to non-default states
    visibility.setLayerVisibility(RenderLayer::Underground, LayerState::Visible);
    visibility.setLayerVisibility(RenderLayer::Terrain, LayerState::Ghost);
    visibility.setLayerVisibility(RenderLayer::Buildings, LayerState::Hidden);

    // Reset
    visibility.resetAll();

    // Verify all reset to defaults
    ASSERT_EQ(LayerState::Hidden, visibility.getState(RenderLayer::Underground));
    ASSERT_EQ(LayerState::Visible, visibility.getState(RenderLayer::Terrain));
    ASSERT_EQ(LayerState::Visible, visibility.getState(RenderLayer::Buildings));
    ASSERT_EQ(LayerState::Visible, visibility.getState(RenderLayer::UIWorld));
}

TEST(set_all_layers_visible) {
    LayerVisibility visibility;

    // First hide everything
    visibility.setAllLayers(LayerState::Hidden);

    // Then make all visible
    visibility.setAllLayers(LayerState::Visible);

    for (std::size_t i = 0; i < RENDER_LAYER_COUNT; ++i) {
        RenderLayer layer = static_cast<RenderLayer>(i);
        ASSERT_EQ(LayerState::Visible, visibility.getState(layer));
    }
}

TEST(set_all_layers_hidden) {
    LayerVisibility visibility;

    visibility.setAllLayers(LayerState::Hidden);

    for (std::size_t i = 0; i < RENDER_LAYER_COUNT; ++i) {
        RenderLayer layer = static_cast<RenderLayer>(i);
        ASSERT_EQ(LayerState::Hidden, visibility.getState(layer));
    }
}

TEST(set_all_layers_ghost) {
    LayerVisibility visibility;

    visibility.setAllLayers(LayerState::Ghost);

    for (std::size_t i = 0; i < RENDER_LAYER_COUNT; ++i) {
        RenderLayer layer = static_cast<RenderLayer>(i);
        ASSERT_EQ(LayerState::Ghost, visibility.getState(layer));
    }
}

TEST(set_layer_range) {
    LayerVisibility visibility;

    // Set Roads through Units to Ghost
    visibility.setLayerRange(RenderLayer::Roads, RenderLayer::Units, LayerState::Ghost);

    // Before range - unchanged
    ASSERT_EQ(LayerState::Visible, visibility.getState(RenderLayer::Water));

    // In range - Ghost
    ASSERT_EQ(LayerState::Ghost, visibility.getState(RenderLayer::Roads));
    ASSERT_EQ(LayerState::Ghost, visibility.getState(RenderLayer::Buildings));
    ASSERT_EQ(LayerState::Ghost, visibility.getState(RenderLayer::Units));

    // After range - unchanged
    ASSERT_EQ(LayerState::Visible, visibility.getState(RenderLayer::Effects));
}

TEST(set_layer_range_reversed) {
    LayerVisibility visibility;

    // Set range in reverse order (should still work)
    visibility.setLayerRange(RenderLayer::Units, RenderLayer::Roads, LayerState::Hidden);

    ASSERT_EQ(LayerState::Hidden, visibility.getState(RenderLayer::Roads));
    ASSERT_EQ(LayerState::Hidden, visibility.getState(RenderLayer::Buildings));
    ASSERT_EQ(LayerState::Hidden, visibility.getState(RenderLayer::Units));
}

// =============================================================================
// Configuration Tests
// =============================================================================

TEST(set_ghost_alpha) {
    LayerVisibility visibility;

    visibility.setGhostAlpha(0.7f);
    ASSERT_FLOAT_EQ(0.7f, visibility.getGhostAlpha(), 0.001f);
}

TEST(ghost_alpha_clamped_low) {
    LayerVisibility visibility;

    visibility.setGhostAlpha(-0.5f);
    ASSERT_FLOAT_EQ(0.0f, visibility.getGhostAlpha(), 0.001f);
}

TEST(ghost_alpha_clamped_high) {
    LayerVisibility visibility;

    visibility.setGhostAlpha(1.5f);
    ASSERT_FLOAT_EQ(1.0f, visibility.getGhostAlpha(), 0.001f);
}

TEST(allow_opaque_ghost_false) {
    LayerVisibilityConfig config;
    config.allowOpaqueGhost = false;

    LayerVisibility visibility(config);

    // Opaque layer (Buildings) should not ghost - converted to Visible
    visibility.setLayerVisibility(RenderLayer::Buildings, LayerState::Ghost);
    ASSERT_EQ(LayerState::Visible, visibility.getState(RenderLayer::Buildings));

    // Transparent layer (Water) should ghost normally
    visibility.setLayerVisibility(RenderLayer::Water, LayerState::Ghost);
    ASSERT_EQ(LayerState::Ghost, visibility.getState(RenderLayer::Water));
}

TEST(allow_opaque_ghost_true) {
    LayerVisibilityConfig config;
    config.allowOpaqueGhost = true;

    LayerVisibility visibility(config);

    // Opaque layer should ghost normally when allowed
    visibility.setLayerVisibility(RenderLayer::Buildings, LayerState::Ghost);
    ASSERT_EQ(LayerState::Ghost, visibility.getState(RenderLayer::Buildings));
}

TEST(set_config) {
    LayerVisibility visibility;

    LayerVisibilityConfig newConfig;
    newConfig.ghostAlpha = 0.8f;
    newConfig.allowOpaqueGhost = false;

    // First set a ghost on opaque layer
    visibility.setLayerVisibility(RenderLayer::Terrain, LayerState::Ghost);
    ASSERT_EQ(LayerState::Ghost, visibility.getState(RenderLayer::Terrain));

    // Apply new config
    visibility.setConfig(newConfig);

    // Ghost alpha updated
    ASSERT_FLOAT_EQ(0.8f, visibility.getGhostAlpha(), 0.001f);

    // Opaque ghost should be converted to Visible
    ASSERT_EQ(LayerState::Visible, visibility.getState(RenderLayer::Terrain));
}

// =============================================================================
// Underground View Mode Tests
// =============================================================================

TEST(enable_underground_view) {
    LayerVisibility visibility;

    visibility.enableUndergroundView();

    // Underground visible
    ASSERT_EQ(LayerState::Visible, visibility.getState(RenderLayer::Underground));

    // Surface layers ghosted
    ASSERT_EQ(LayerState::Ghost, visibility.getState(RenderLayer::Terrain));
    ASSERT_EQ(LayerState::Ghost, visibility.getState(RenderLayer::Roads));
    ASSERT_EQ(LayerState::Ghost, visibility.getState(RenderLayer::Buildings));
    ASSERT_EQ(LayerState::Ghost, visibility.getState(RenderLayer::Units));

    // Other layers unchanged
    ASSERT_EQ(LayerState::Visible, visibility.getState(RenderLayer::Water));
    ASSERT_EQ(LayerState::Visible, visibility.getState(RenderLayer::Effects));
    ASSERT_EQ(LayerState::Visible, visibility.getState(RenderLayer::DataOverlay));
    ASSERT_EQ(LayerState::Visible, visibility.getState(RenderLayer::UIWorld));
}

TEST(disable_underground_view) {
    LayerVisibility visibility;

    // First enable
    visibility.enableUndergroundView();

    // Then disable
    visibility.disableUndergroundView();

    // Underground hidden again
    ASSERT_EQ(LayerState::Hidden, visibility.getState(RenderLayer::Underground));

    // Surface layers visible again
    ASSERT_EQ(LayerState::Visible, visibility.getState(RenderLayer::Terrain));
    ASSERT_EQ(LayerState::Visible, visibility.getState(RenderLayer::Roads));
    ASSERT_EQ(LayerState::Visible, visibility.getState(RenderLayer::Buildings));
    ASSERT_EQ(LayerState::Visible, visibility.getState(RenderLayer::Units));
}

TEST(is_underground_view_active_default) {
    LayerVisibility visibility;

    // Underground hidden by default
    ASSERT(!visibility.isUndergroundViewActive());
}

TEST(is_underground_view_active_after_enable) {
    LayerVisibility visibility;

    visibility.enableUndergroundView();
    ASSERT(visibility.isUndergroundViewActive());
}

TEST(is_underground_view_active_after_disable) {
    LayerVisibility visibility;

    visibility.enableUndergroundView();
    visibility.disableUndergroundView();
    ASSERT(!visibility.isUndergroundViewActive());
}

TEST(underground_view_active_custom_setup) {
    LayerVisibility visibility;

    // Manual setup that should count as underground view
    visibility.setLayerVisibility(RenderLayer::Underground, LayerState::Visible);
    visibility.setLayerVisibility(RenderLayer::Terrain, LayerState::Ghost);

    ASSERT(visibility.isUndergroundViewActive());
}

TEST(underground_view_not_active_just_underground_visible) {
    LayerVisibility visibility;

    // Just showing underground without ghosting surface = not underground view mode
    visibility.setLayerVisibility(RenderLayer::Underground, LayerState::Visible);
    // No surface layers ghosted

    ASSERT(!visibility.isUndergroundViewActive());
}

// =============================================================================
// Statistics Tests
// =============================================================================

TEST(count_states_default) {
    LayerVisibility visibility;

    std::size_t visible, hidden, ghost;
    visibility.countStates(visible, hidden, ghost);

    // Default: 9 visible, 1 hidden (Underground), 0 ghost
    ASSERT_EQ(9u, visible);
    ASSERT_EQ(1u, hidden);
    ASSERT_EQ(0u, ghost);
}

TEST(count_states_all_hidden) {
    LayerVisibility visibility;
    visibility.setAllLayers(LayerState::Hidden);

    std::size_t visible, hidden, ghost;
    visibility.countStates(visible, hidden, ghost);

    ASSERT_EQ(0u, visible);
    ASSERT_EQ(10u, hidden);
    ASSERT_EQ(0u, ghost);
}

TEST(count_states_all_ghost) {
    LayerVisibility visibility;
    visibility.setAllLayers(LayerState::Ghost);

    std::size_t visible, hidden, ghost;
    visibility.countStates(visible, hidden, ghost);

    ASSERT_EQ(0u, visible);
    ASSERT_EQ(0u, hidden);
    ASSERT_EQ(10u, ghost);
}

TEST(count_states_mixed) {
    LayerVisibility visibility;

    visibility.setLayerVisibility(RenderLayer::Terrain, LayerState::Ghost);
    visibility.setLayerVisibility(RenderLayer::Buildings, LayerState::Ghost);
    visibility.setLayerVisibility(RenderLayer::Water, LayerState::Hidden);

    std::size_t visible, hidden, ghost;
    visibility.countStates(visible, hidden, ghost);

    // Underground(hidden), Terrain(ghost), Vegetation(visible), Water(hidden), Roads(visible),
    // Buildings(ghost), Units(visible), Effects(visible), DataOverlay(visible), UIWorld(visible)
    ASSERT_EQ(6u, visible);
    ASSERT_EQ(2u, hidden);
    ASSERT_EQ(2u, ghost);
}

// =============================================================================
// Hidden Layers Skip Rendering Tests
// =============================================================================

TEST(hidden_layer_skips_rendering) {
    LayerVisibility visibility;

    // When a layer is hidden, shouldRender returns false
    visibility.setLayerVisibility(RenderLayer::Buildings, LayerState::Hidden);

    // This is the check the render loop would use to skip the layer entirely
    ASSERT(!visibility.shouldRender(RenderLayer::Buildings));

    // Verify it's truly hidden
    ASSERT(visibility.isHidden(RenderLayer::Buildings));
    ASSERT(!visibility.isVisible(RenderLayer::Buildings));
    ASSERT(!visibility.isGhost(RenderLayer::Buildings));
}

TEST(hidden_layers_for_debug) {
    LayerVisibility visibility;

    // Use case: Debug mode - hide all except one layer
    visibility.setAllLayers(LayerState::Hidden);
    visibility.setLayerVisibility(RenderLayer::Buildings, LayerState::Visible);

    // Only Buildings should render
    ASSERT(!visibility.shouldRender(RenderLayer::Underground));
    ASSERT(!visibility.shouldRender(RenderLayer::Terrain));
    ASSERT(!visibility.shouldRender(RenderLayer::Water));
    ASSERT(!visibility.shouldRender(RenderLayer::Roads));
    ASSERT(visibility.shouldRender(RenderLayer::Buildings));
    ASSERT(!visibility.shouldRender(RenderLayer::Units));
    ASSERT(!visibility.shouldRender(RenderLayer::Effects));
    ASSERT(!visibility.shouldRender(RenderLayer::DataOverlay));
    ASSERT(!visibility.shouldRender(RenderLayer::UIWorld));
}

// =============================================================================
// Ghost Layers Render at Reduced Alpha Tests
// =============================================================================

TEST(ghost_layer_renders_transparent) {
    LayerVisibility visibility;
    visibility.setGhostAlpha(0.4f);

    visibility.setLayerVisibility(RenderLayer::Buildings, LayerState::Ghost);

    // Layer should render (not skipped)
    ASSERT(visibility.shouldRender(RenderLayer::Buildings));

    // But it's ghost, so render system should use reduced alpha
    ASSERT(visibility.isGhost(RenderLayer::Buildings));
    ASSERT_FLOAT_EQ(0.4f, visibility.getGhostAlpha(), 0.001f);
}

TEST(ghost_alpha_used_for_underground_view) {
    LayerVisibility visibility;
    visibility.setGhostAlpha(0.25f);

    visibility.enableUndergroundView();

    // Ghosted surface layers should use the configured alpha
    ASSERT(visibility.isGhost(RenderLayer::Terrain));
    ASSERT(visibility.isGhost(RenderLayer::Buildings));
    ASSERT_FLOAT_EQ(0.25f, visibility.getGhostAlpha(), 0.001f);
}

// =============================================================================
// Copy and Move Tests
// =============================================================================

TEST(copy_construction) {
    LayerVisibility original;
    original.setLayerVisibility(RenderLayer::Buildings, LayerState::Ghost);
    original.setGhostAlpha(0.6f);

    LayerVisibility copy = original;

    ASSERT_EQ(LayerState::Ghost, copy.getState(RenderLayer::Buildings));
    ASSERT_FLOAT_EQ(0.6f, copy.getGhostAlpha(), 0.001f);
}

TEST(copy_assignment) {
    LayerVisibility original;
    original.setLayerVisibility(RenderLayer::Terrain, LayerState::Hidden);

    LayerVisibility copy;
    copy = original;

    ASSERT_EQ(LayerState::Hidden, copy.getState(RenderLayer::Terrain));
}

TEST(move_construction) {
    LayerVisibility original;
    original.setLayerVisibility(RenderLayer::Water, LayerState::Ghost);

    LayerVisibility moved = std::move(original);

    ASSERT_EQ(LayerState::Ghost, moved.getState(RenderLayer::Water));
}

// =============================================================================
// Main
// =============================================================================

int main() {
    int passed = 0;

    printf("Running LayerVisibility tests...\n\n");

    printf("LayerState Enum Tests:\n");
    RUN_TEST(layer_state_values);
    RUN_TEST(layer_state_names);
    RUN_TEST(layer_state_validation);

    printf("\nDefault State Tests:\n");
    RUN_TEST(default_construction);
    RUN_TEST(default_ghost_alpha);
    RUN_TEST(custom_config_construction);

    printf("\nsetLayerVisibility() Tests:\n");
    RUN_TEST(set_layer_visibility_visible);
    RUN_TEST(set_layer_visibility_hidden);
    RUN_TEST(set_layer_visibility_ghost);
    RUN_TEST(set_all_layers_to_each_state);
    RUN_TEST(set_invalid_layer_ignored);
    RUN_TEST(set_invalid_state_ignored);

    printf("\nQuery Function Tests:\n");
    RUN_TEST(should_render_visible_layer);
    RUN_TEST(should_render_ghost_layer);
    RUN_TEST(should_not_render_hidden_layer);
    RUN_TEST(is_ghost_true);
    RUN_TEST(is_ghost_false_when_visible);
    RUN_TEST(is_ghost_false_when_hidden);
    RUN_TEST(is_visible_true);
    RUN_TEST(is_visible_false_when_ghost);
    RUN_TEST(is_visible_false_when_hidden);
    RUN_TEST(is_hidden_true);
    RUN_TEST(is_hidden_false_when_visible);
    RUN_TEST(is_hidden_false_when_ghost);
    RUN_TEST(query_invalid_layer);

    printf("\nBulk Operation Tests:\n");
    RUN_TEST(reset_all);
    RUN_TEST(set_all_layers_visible);
    RUN_TEST(set_all_layers_hidden);
    RUN_TEST(set_all_layers_ghost);
    RUN_TEST(set_layer_range);
    RUN_TEST(set_layer_range_reversed);

    printf("\nConfiguration Tests:\n");
    RUN_TEST(set_ghost_alpha);
    RUN_TEST(ghost_alpha_clamped_low);
    RUN_TEST(ghost_alpha_clamped_high);
    RUN_TEST(allow_opaque_ghost_false);
    RUN_TEST(allow_opaque_ghost_true);
    RUN_TEST(set_config);

    printf("\nUnderground View Mode Tests:\n");
    RUN_TEST(enable_underground_view);
    RUN_TEST(disable_underground_view);
    RUN_TEST(is_underground_view_active_default);
    RUN_TEST(is_underground_view_active_after_enable);
    RUN_TEST(is_underground_view_active_after_disable);
    RUN_TEST(underground_view_active_custom_setup);
    RUN_TEST(underground_view_not_active_just_underground_visible);

    printf("\nStatistics Tests:\n");
    RUN_TEST(count_states_default);
    RUN_TEST(count_states_all_hidden);
    RUN_TEST(count_states_all_ghost);
    RUN_TEST(count_states_mixed);

    printf("\nHidden Layers Skip Rendering Tests:\n");
    RUN_TEST(hidden_layer_skips_rendering);
    RUN_TEST(hidden_layers_for_debug);

    printf("\nGhost Layers Render at Reduced Alpha Tests:\n");
    RUN_TEST(ghost_layer_renders_transparent);
    RUN_TEST(ghost_alpha_used_for_underground_view);

    printf("\nCopy and Move Tests:\n");
    RUN_TEST(copy_construction);
    RUN_TEST(copy_assignment);
    RUN_TEST(move_construction);

    printf("\n========================================\n");
    printf("All %d tests passed!\n", passed);
    printf("========================================\n");

    return 0;
}
