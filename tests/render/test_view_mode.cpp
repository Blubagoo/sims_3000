/**
 * @file test_view_mode.cpp
 * @brief Unit tests for ViewMode enum and ViewModeController class.
 *
 * Tests verify:
 * - ViewMode enum values and naming
 * - ViewModeController construction
 * - Mode setting and cycling
 * - Transition management (start, update, complete, cancel)
 * - Configuration
 * - Layer state application for each mode
 */

#include "sims3000/render/ViewMode.h"
#include "sims3000/render/LayerVisibility.h"
#include "sims3000/render/RenderLayer.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>

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
// ViewMode Enum Tests
// =============================================================================

TEST(view_mode_values) {
    ASSERT_EQ(0, static_cast<int>(ViewMode::Surface));
    ASSERT_EQ(1, static_cast<int>(ViewMode::Underground));
    ASSERT_EQ(2, static_cast<int>(ViewMode::Cutaway));
}

TEST(view_mode_names) {
    ASSERT_STREQ("Surface", getViewModeName(ViewMode::Surface));
    ASSERT_STREQ("Underground", getViewModeName(ViewMode::Underground));
    ASSERT_STREQ("Cutaway", getViewModeName(ViewMode::Cutaway));
    ASSERT_STREQ("Unknown", getViewModeName(static_cast<ViewMode>(255)));
}

TEST(view_mode_validation) {
    ASSERT(isValidViewMode(ViewMode::Surface));
    ASSERT(isValidViewMode(ViewMode::Underground));
    ASSERT(isValidViewMode(ViewMode::Cutaway));
    ASSERT(!isValidViewMode(static_cast<ViewMode>(3)));
    ASSERT(!isValidViewMode(static_cast<ViewMode>(255)));
}

TEST(view_mode_count) {
    ASSERT_EQ(3u, VIEW_MODE_COUNT);
}

// =============================================================================
// ViewModeController Construction Tests
// =============================================================================

TEST(controller_default_construction) {
    LayerVisibility visibility;
    ViewModeController controller(visibility);

    ASSERT_EQ(ViewMode::Surface, controller.getMode());
    ASSERT(!controller.isTransitioning());
}

TEST(controller_custom_config) {
    LayerVisibility visibility;
    ViewModeConfig config;
    config.transitionDuration = 0.5f;
    config.undergroundGhostAlpha = 0.4f;
    config.cutawayUndergroundAlpha = 0.8f;

    ViewModeController controller(visibility, config);

    ASSERT_FLOAT_EQ(0.5f, controller.getConfig().transitionDuration, 0.001f);
    ASSERT_FLOAT_EQ(0.4f, controller.getConfig().undergroundGhostAlpha, 0.001f);
    ASSERT_FLOAT_EQ(0.8f, controller.getConfig().cutawayUndergroundAlpha, 0.001f);
}

TEST(controller_config_clamping) {
    LayerVisibility visibility;
    ViewModeConfig config;
    config.transitionDuration = -1.0f;  // Should clamp to 0
    config.undergroundGhostAlpha = 1.5f;  // Should clamp to 1.0
    config.cutawayUndergroundAlpha = -0.5f;  // Should clamp to 0.0

    ViewModeController controller(visibility, config);

    ASSERT_FLOAT_EQ(0.0f, controller.getConfig().transitionDuration, 0.001f);
    ASSERT_FLOAT_EQ(1.0f, controller.getConfig().undergroundGhostAlpha, 0.001f);
    ASSERT_FLOAT_EQ(0.0f, controller.getConfig().cutawayUndergroundAlpha, 0.001f);
}

// =============================================================================
// Mode Setting Tests
// =============================================================================

TEST(set_mode_underground) {
    LayerVisibility visibility;
    ViewModeConfig config;
    config.transitionDuration = 0.0f;  // Instant transitions

    ViewModeController controller(visibility, config);
    controller.setMode(ViewMode::Underground);

    ASSERT_EQ(ViewMode::Underground, controller.getMode());
    ASSERT(!controller.isTransitioning());
}

TEST(set_mode_cutaway) {
    LayerVisibility visibility;
    ViewModeConfig config;
    config.transitionDuration = 0.0f;

    ViewModeController controller(visibility, config);
    controller.setMode(ViewMode::Cutaway);

    ASSERT_EQ(ViewMode::Cutaway, controller.getMode());
}

TEST(set_same_mode_no_change) {
    LayerVisibility visibility;
    ViewModeConfig config;
    config.transitionDuration = 1.0f;

    ViewModeController controller(visibility, config);
    ASSERT_EQ(ViewMode::Surface, controller.getMode());

    // Setting same mode should not start transition
    controller.setMode(ViewMode::Surface);
    ASSERT(!controller.isTransitioning());
}

TEST(set_invalid_mode_ignored) {
    LayerVisibility visibility;
    ViewModeConfig config;
    config.transitionDuration = 0.0f;

    ViewModeController controller(visibility, config);
    controller.setMode(static_cast<ViewMode>(255));

    // Should remain at Surface
    ASSERT_EQ(ViewMode::Surface, controller.getMode());
}

// =============================================================================
// Mode Cycling Tests
// =============================================================================

TEST(cycle_mode_forward) {
    LayerVisibility visibility;
    ViewModeConfig config;
    config.transitionDuration = 0.0f;

    ViewModeController controller(visibility, config);

    // Start at Surface
    ASSERT_EQ(ViewMode::Surface, controller.getMode());

    // Cycle: Surface -> Underground
    controller.cycleMode();
    ASSERT_EQ(ViewMode::Underground, controller.getMode());

    // Cycle: Underground -> Cutaway
    controller.cycleMode();
    ASSERT_EQ(ViewMode::Cutaway, controller.getMode());

    // Cycle: Cutaway -> Surface
    controller.cycleMode();
    ASSERT_EQ(ViewMode::Surface, controller.getMode());
}

TEST(cycle_mode_reverse) {
    LayerVisibility visibility;
    ViewModeConfig config;
    config.transitionDuration = 0.0f;

    ViewModeController controller(visibility, config);

    // Start at Surface
    ASSERT_EQ(ViewMode::Surface, controller.getMode());

    // Reverse: Surface -> Cutaway
    controller.cycleModeReverse();
    ASSERT_EQ(ViewMode::Cutaway, controller.getMode());

    // Reverse: Cutaway -> Underground
    controller.cycleModeReverse();
    ASSERT_EQ(ViewMode::Underground, controller.getMode());

    // Reverse: Underground -> Surface
    controller.cycleModeReverse();
    ASSERT_EQ(ViewMode::Surface, controller.getMode());
}

TEST(reset_to_surface) {
    LayerVisibility visibility;
    ViewModeConfig config;
    config.transitionDuration = 0.0f;

    ViewModeController controller(visibility, config);
    controller.setMode(ViewMode::Underground);
    ASSERT_EQ(ViewMode::Underground, controller.getMode());

    controller.resetToSurface();
    ASSERT_EQ(ViewMode::Surface, controller.getMode());
}

// =============================================================================
// Transition Tests
// =============================================================================

TEST(transition_starts_on_mode_change) {
    LayerVisibility visibility;
    ViewModeConfig config;
    config.transitionDuration = 1.0f;

    ViewModeController controller(visibility, config);
    controller.setMode(ViewMode::Underground);

    ASSERT(controller.isTransitioning());
    ASSERT_FLOAT_EQ(0.0f, controller.getTransitionProgress(), 0.001f);
}

TEST(transition_progresses_with_update) {
    LayerVisibility visibility;
    ViewModeConfig config;
    config.transitionDuration = 1.0f;

    ViewModeController controller(visibility, config);
    controller.setMode(ViewMode::Underground);

    // Update by 0.5 seconds (half the duration)
    controller.update(0.5f);

    ASSERT(controller.isTransitioning());
    ASSERT_FLOAT_EQ(0.5f, controller.getTransitionProgress(), 0.001f);
}

TEST(transition_completes_at_duration) {
    LayerVisibility visibility;
    ViewModeConfig config;
    config.transitionDuration = 1.0f;

    ViewModeController controller(visibility, config);
    controller.setMode(ViewMode::Underground);

    // Update by full duration
    controller.update(1.0f);

    ASSERT(!controller.isTransitioning());
    ASSERT_FLOAT_EQ(1.0f, controller.getTransitionProgress(), 0.001f);
}

TEST(transition_completes_with_overshoot) {
    LayerVisibility visibility;
    ViewModeConfig config;
    config.transitionDuration = 1.0f;

    ViewModeController controller(visibility, config);
    controller.setMode(ViewMode::Underground);

    // Update by more than duration
    controller.update(2.0f);

    ASSERT(!controller.isTransitioning());
    ASSERT_FLOAT_EQ(1.0f, controller.getTransitionProgress(), 0.001f);
}

TEST(complete_transition_immediately) {
    LayerVisibility visibility;
    ViewModeConfig config;
    config.transitionDuration = 1.0f;

    ViewModeController controller(visibility, config);
    controller.setMode(ViewMode::Underground);

    ASSERT(controller.isTransitioning());

    controller.completeTransition();

    ASSERT(!controller.isTransitioning());
    ASSERT_FLOAT_EQ(1.0f, controller.getTransitionProgress(), 0.001f);
    ASSERT_EQ(ViewMode::Underground, controller.getMode());
}

TEST(cancel_transition) {
    LayerVisibility visibility;
    ViewModeConfig config;
    config.transitionDuration = 1.0f;

    ViewModeController controller(visibility, config);

    // Start transition to Underground
    controller.setMode(ViewMode::Underground);
    controller.update(0.5f);

    ASSERT(controller.isTransitioning());

    // Cancel
    controller.cancelTransition();

    ASSERT(!controller.isTransitioning());
    ASSERT_EQ(ViewMode::Surface, controller.getMode());  // Reverted to previous
}

TEST(cancel_when_not_transitioning_no_op) {
    LayerVisibility visibility;
    ViewModeConfig config;
    config.transitionDuration = 0.0f;

    ViewModeController controller(visibility, config);
    controller.setMode(ViewMode::Underground);

    ASSERT(!controller.isTransitioning());

    // Cancel should do nothing
    controller.cancelTransition();
    ASSERT_EQ(ViewMode::Underground, controller.getMode());
}

TEST(update_when_not_transitioning_no_op) {
    LayerVisibility visibility;
    ViewModeConfig config;
    config.transitionDuration = 0.0f;

    ViewModeController controller(visibility, config);
    controller.setMode(ViewMode::Underground);

    ASSERT(!controller.isTransitioning());

    // Update should do nothing
    controller.update(1.0f);

    ASSERT_EQ(ViewMode::Underground, controller.getMode());
    ASSERT(!controller.isTransitioning());
}

TEST(previous_mode_tracked) {
    LayerVisibility visibility;
    ViewModeConfig config;
    config.transitionDuration = 1.0f;

    ViewModeController controller(visibility, config);

    ASSERT_EQ(ViewMode::Surface, controller.getPreviousMode());

    controller.setMode(ViewMode::Underground);
    ASSERT_EQ(ViewMode::Surface, controller.getPreviousMode());
    ASSERT_EQ(ViewMode::Underground, controller.getMode());

    controller.completeTransition();
    controller.setMode(ViewMode::Cutaway);
    ASSERT_EQ(ViewMode::Underground, controller.getPreviousMode());
    ASSERT_EQ(ViewMode::Cutaway, controller.getMode());
}

// =============================================================================
// Layer State Application Tests - Surface Mode
// =============================================================================

TEST(surface_mode_layer_states) {
    LayerVisibility visibility;
    ViewModeConfig config;
    config.transitionDuration = 0.0f;

    ViewModeController controller(visibility, config);
    // Default is Surface mode

    // Underground should be hidden
    ASSERT(visibility.isHidden(RenderLayer::Underground));

    // Surface layers should be visible
    ASSERT(visibility.isVisible(RenderLayer::Terrain));
    ASSERT(visibility.isVisible(RenderLayer::Water));
    ASSERT(visibility.isVisible(RenderLayer::Roads));
    ASSERT(visibility.isVisible(RenderLayer::Buildings));
    ASSERT(visibility.isVisible(RenderLayer::Units));
    ASSERT(visibility.isVisible(RenderLayer::Effects));
    ASSERT(visibility.isVisible(RenderLayer::DataOverlay));
    ASSERT(visibility.isVisible(RenderLayer::UIWorld));
}

// =============================================================================
// Layer State Application Tests - Underground Mode
// =============================================================================

TEST(underground_mode_layer_states) {
    LayerVisibility visibility;
    ViewModeConfig config;
    config.transitionDuration = 0.0f;

    ViewModeController controller(visibility, config);
    controller.setMode(ViewMode::Underground);

    // Underground should be visible
    ASSERT(visibility.isVisible(RenderLayer::Underground));

    // Surface layers should be ghosted
    ASSERT(visibility.isGhost(RenderLayer::Terrain));
    ASSERT(visibility.isGhost(RenderLayer::Roads));
    ASSERT(visibility.isGhost(RenderLayer::Buildings));
    ASSERT(visibility.isGhost(RenderLayer::Units));

    // Water should remain visible (can see through to underground)
    ASSERT(visibility.isVisible(RenderLayer::Water));

    // Effects and overlays remain visible
    ASSERT(visibility.isVisible(RenderLayer::Effects));
    ASSERT(visibility.isVisible(RenderLayer::DataOverlay));
    ASSERT(visibility.isVisible(RenderLayer::UIWorld));
}

TEST(underground_mode_ghost_alpha) {
    LayerVisibility visibility;
    ViewModeConfig config;
    config.transitionDuration = 0.0f;
    config.undergroundGhostAlpha = 0.35f;

    ViewModeController controller(visibility, config);
    controller.setMode(ViewMode::Underground);

    ASSERT_FLOAT_EQ(0.35f, visibility.getGhostAlpha(), 0.001f);
}

// =============================================================================
// Layer State Application Tests - Cutaway Mode
// =============================================================================

TEST(cutaway_mode_layer_states) {
    LayerVisibility visibility;
    ViewModeConfig config;
    config.transitionDuration = 0.0f;

    ViewModeController controller(visibility, config);
    controller.setMode(ViewMode::Cutaway);

    // Both underground and surface should be visible
    ASSERT(visibility.isVisible(RenderLayer::Underground));
    ASSERT(visibility.isVisible(RenderLayer::Terrain));
    ASSERT(visibility.isVisible(RenderLayer::Water));
    ASSERT(visibility.isVisible(RenderLayer::Roads));
    ASSERT(visibility.isVisible(RenderLayer::Buildings));
    ASSERT(visibility.isVisible(RenderLayer::Units));
    ASSERT(visibility.isVisible(RenderLayer::Effects));
    ASSERT(visibility.isVisible(RenderLayer::DataOverlay));
    ASSERT(visibility.isVisible(RenderLayer::UIWorld));
}

// =============================================================================
// Configuration Tests
// =============================================================================

TEST(set_config) {
    LayerVisibility visibility;
    ViewModeController controller(visibility);

    ViewModeConfig newConfig;
    newConfig.transitionDuration = 0.75f;
    newConfig.undergroundGhostAlpha = 0.5f;
    newConfig.cutawayUndergroundAlpha = 0.9f;

    controller.setConfig(newConfig);

    ASSERT_FLOAT_EQ(0.75f, controller.getConfig().transitionDuration, 0.001f);
    ASSERT_FLOAT_EQ(0.5f, controller.getConfig().undergroundGhostAlpha, 0.001f);
    ASSERT_FLOAT_EQ(0.9f, controller.getConfig().cutawayUndergroundAlpha, 0.001f);
}

TEST(set_transition_duration) {
    LayerVisibility visibility;
    ViewModeController controller(visibility);

    controller.setTransitionDuration(0.5f);
    ASSERT_FLOAT_EQ(0.5f, controller.getConfig().transitionDuration, 0.001f);
}

TEST(set_transition_duration_clamped) {
    LayerVisibility visibility;
    ViewModeController controller(visibility);

    controller.setTransitionDuration(-1.0f);
    ASSERT_FLOAT_EQ(0.0f, controller.getConfig().transitionDuration, 0.001f);
}

// =============================================================================
// Easing Tests
// =============================================================================

TEST(eased_progress_at_zero) {
    LayerVisibility visibility;
    ViewModeConfig config;
    config.transitionDuration = 1.0f;

    ViewModeController controller(visibility, config);
    controller.setMode(ViewMode::Underground);

    // At start, eased progress should be 0
    ASSERT_FLOAT_EQ(0.0f, controller.getTransitionProgress(), 0.001f);
    ASSERT_FLOAT_EQ(0.0f, controller.getEasedProgress(), 0.001f);
}

TEST(eased_progress_at_one) {
    LayerVisibility visibility;
    ViewModeConfig config;
    config.transitionDuration = 1.0f;

    ViewModeController controller(visibility, config);
    controller.setMode(ViewMode::Underground);
    controller.update(1.0f);

    // At end, eased progress should be 1
    ASSERT_FLOAT_EQ(1.0f, controller.getTransitionProgress(), 0.001f);
    ASSERT_FLOAT_EQ(1.0f, controller.getEasedProgress(), 0.001f);
}

TEST(eased_progress_midpoint) {
    LayerVisibility visibility;
    ViewModeConfig config;
    config.transitionDuration = 1.0f;
    config.transitionEasing = Easing::EasingType::Linear;  // Use linear for predictable test

    ViewModeController controller(visibility, config);
    controller.setMode(ViewMode::Underground);
    controller.update(0.5f);

    // With linear easing, eased progress should equal raw progress
    ASSERT_FLOAT_EQ(0.5f, controller.getTransitionProgress(), 0.001f);
    ASSERT_FLOAT_EQ(0.5f, controller.getEasedProgress(), 0.001f);
}

// =============================================================================
// Integration Tests
// =============================================================================

TEST(full_cycle_with_transitions) {
    LayerVisibility visibility;
    ViewModeConfig config;
    config.transitionDuration = 0.25f;

    ViewModeController controller(visibility, config);

    // Surface -> Underground
    controller.cycleMode();
    ASSERT(controller.isTransitioning());
    controller.update(0.25f);
    ASSERT(!controller.isTransitioning());
    ASSERT_EQ(ViewMode::Underground, controller.getMode());
    ASSERT(visibility.isVisible(RenderLayer::Underground));

    // Underground -> Cutaway
    controller.cycleMode();
    ASSERT(controller.isTransitioning());
    controller.update(0.25f);
    ASSERT(!controller.isTransitioning());
    ASSERT_EQ(ViewMode::Cutaway, controller.getMode());

    // Cutaway -> Surface
    controller.cycleMode();
    ASSERT(controller.isTransitioning());
    controller.update(0.25f);
    ASSERT(!controller.isTransitioning());
    ASSERT_EQ(ViewMode::Surface, controller.getMode());
    ASSERT(visibility.isHidden(RenderLayer::Underground));
}

TEST(mode_change_during_transition) {
    LayerVisibility visibility;
    ViewModeConfig config;
    config.transitionDuration = 1.0f;

    ViewModeController controller(visibility, config);

    // Start transition to Underground
    controller.setMode(ViewMode::Underground);
    controller.update(0.5f);
    ASSERT(controller.isTransitioning());

    // Change to Cutaway mid-transition
    controller.setMode(ViewMode::Cutaway);
    ASSERT(controller.isTransitioning());
    ASSERT_EQ(ViewMode::Cutaway, controller.getMode());
    ASSERT_EQ(ViewMode::Underground, controller.getPreviousMode());

    // Complete new transition
    controller.update(1.0f);
    ASSERT(!controller.isTransitioning());
    ASSERT_EQ(ViewMode::Cutaway, controller.getMode());
}

// =============================================================================
// Main
// =============================================================================

int main() {
    int passed = 0;

    printf("Running ViewMode tests...\n\n");

    printf("ViewMode Enum Tests:\n");
    RUN_TEST(view_mode_values);
    RUN_TEST(view_mode_names);
    RUN_TEST(view_mode_validation);
    RUN_TEST(view_mode_count);

    printf("\nViewModeController Construction Tests:\n");
    RUN_TEST(controller_default_construction);
    RUN_TEST(controller_custom_config);
    RUN_TEST(controller_config_clamping);

    printf("\nMode Setting Tests:\n");
    RUN_TEST(set_mode_underground);
    RUN_TEST(set_mode_cutaway);
    RUN_TEST(set_same_mode_no_change);
    RUN_TEST(set_invalid_mode_ignored);

    printf("\nMode Cycling Tests:\n");
    RUN_TEST(cycle_mode_forward);
    RUN_TEST(cycle_mode_reverse);
    RUN_TEST(reset_to_surface);

    printf("\nTransition Tests:\n");
    RUN_TEST(transition_starts_on_mode_change);
    RUN_TEST(transition_progresses_with_update);
    RUN_TEST(transition_completes_at_duration);
    RUN_TEST(transition_completes_with_overshoot);
    RUN_TEST(complete_transition_immediately);
    RUN_TEST(cancel_transition);
    RUN_TEST(cancel_when_not_transitioning_no_op);
    RUN_TEST(update_when_not_transitioning_no_op);
    RUN_TEST(previous_mode_tracked);

    printf("\nSurface Mode Layer State Tests:\n");
    RUN_TEST(surface_mode_layer_states);

    printf("\nUnderground Mode Layer State Tests:\n");
    RUN_TEST(underground_mode_layer_states);
    RUN_TEST(underground_mode_ghost_alpha);

    printf("\nCutaway Mode Layer State Tests:\n");
    RUN_TEST(cutaway_mode_layer_states);

    printf("\nConfiguration Tests:\n");
    RUN_TEST(set_config);
    RUN_TEST(set_transition_duration);
    RUN_TEST(set_transition_duration_clamped);

    printf("\nEasing Tests:\n");
    RUN_TEST(eased_progress_at_zero);
    RUN_TEST(eased_progress_at_one);
    RUN_TEST(eased_progress_midpoint);

    printf("\nIntegration Tests:\n");
    RUN_TEST(full_cycle_with_transitions);
    RUN_TEST(mode_change_during_transition);

    printf("\n========================================\n");
    printf("All %d tests passed!\n", passed);
    printf("========================================\n");

    return 0;
}
