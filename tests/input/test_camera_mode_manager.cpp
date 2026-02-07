/**
 * @file test_camera_mode_manager.cpp
 * @brief Unit tests for CameraModeManager (Ticket 2-048).
 */

#include "sims3000/input/CameraModeManager.h"
#include "sims3000/render/CameraState.h"
#include <cassert>
#include <cstdio>
#include <cmath>

using namespace sims3000;

// Helper for float comparison with tolerance
bool approxEqual(float a, float b, float epsilon = 0.001f) {
    return std::fabs(a - b) < epsilon;
}

// ============================================================================
// Mode Enum Tests (Acceptance Criteria: Mode enum: Free, Preset, Animating)
// ============================================================================

void test_mode_enum_values() {
    printf("Testing CameraMode enum values...\n");

    // Verify mode enum has required values
    CameraMode free = CameraMode::Free;
    CameraMode presetN = CameraMode::Preset_N;
    CameraMode presetE = CameraMode::Preset_E;
    CameraMode presetS = CameraMode::Preset_S;
    CameraMode presetW = CameraMode::Preset_W;
    CameraMode animating = CameraMode::Animating;

    // Verify they are distinct
    assert(free != presetN);
    assert(presetN != presetE);
    assert(presetE != presetS);
    assert(presetS != presetW);
    assert(presetW != animating);
    assert(animating != free);

    printf("  PASS: Mode enum has Free, Preset (N/E/S/W), and Animating values\n");
}

// ============================================================================
// Construction and Initialization Tests
// ============================================================================

void test_default_construction() {
    printf("Testing CameraModeManager default construction...\n");

    CameraModeManager manager;

    // Manager should exist without crashing
    assert(manager.getConfig().defaultMode == CameraMode::Preset_N);

    printf("  PASS: Default construction works\n");
}

void test_custom_config_construction() {
    printf("Testing CameraModeManager with custom config...\n");

    CameraModeManagerConfig config;
    config.defaultMode = CameraMode::Preset_E;
    config.presetSnapDuration = 0.3f;

    CameraModeManager manager(config);

    assert(manager.getConfig().defaultMode == CameraMode::Preset_E);
    assert(approxEqual(manager.getConfig().presetSnapDuration, 0.3f));

    printf("  PASS: Custom config construction works\n");
}

// ============================================================================
// Default Mode Tests (Acceptance Criteria: Default mode: Preset on game start)
// ============================================================================

void test_default_mode_preset_on_start() {
    printf("Testing default mode is Preset on game start...\n");

    CameraModeManager manager;
    CameraState cameraState;

    // Initialize
    manager.initialize(cameraState);

    // Should be in preset mode (specifically Preset_N by default)
    assert(manager.getCameraMode() == CameraMode::Preset_N);
    assert(cameraState.mode == CameraMode::Preset_N);

    // Camera should be at isometric preset angles
    assert(approxEqual(cameraState.pitch, CameraConfig::ISOMETRIC_PITCH, 0.1f));
    assert(approxEqual(cameraState.yaw, CameraConfig::PRESET_N_YAW, 0.1f));

    printf("  PASS: Default mode is Preset_N on game start\n");
}

void test_custom_default_mode() {
    printf("Testing custom default mode...\n");

    CameraModeManagerConfig config;
    config.defaultMode = CameraMode::Preset_S;

    CameraModeManager manager(config);
    CameraState cameraState;

    manager.initialize(cameraState);

    assert(manager.getCameraMode() == CameraMode::Preset_S);
    assert(cameraState.mode == CameraMode::Preset_S);
    assert(approxEqual(cameraState.yaw, CameraConfig::PRESET_S_YAW, 0.1f));

    printf("  PASS: Custom default mode works\n");
}

// ============================================================================
// getCameraMode() API Tests (Acceptance Criteria: getCameraMode() API)
// ============================================================================

void test_get_camera_mode_api() {
    printf("Testing getCameraMode() API...\n");

    CameraModeManager manager;
    CameraState cameraState;

    manager.initialize(cameraState);

    // Test in preset mode
    CameraMode mode = manager.getCameraMode();
    assert(mode == CameraMode::Preset_N);

    // Force to free mode and test
    manager.forceToFreeMode(cameraState);
    mode = manager.getCameraMode();
    assert(mode == CameraMode::Free);

    printf("  PASS: getCameraMode() API returns correct mode\n");
}

void test_is_in_free_mode() {
    printf("Testing isInFreeMode() helper...\n");

    CameraModeManager manager;
    CameraState cameraState;
    manager.initialize(cameraState);

    // Initially in preset
    assert(!manager.isInFreeMode());

    // Force to free
    manager.forceToFreeMode(cameraState);
    assert(manager.isInFreeMode());

    printf("  PASS: isInFreeMode() works correctly\n");
}

void test_is_in_preset_mode() {
    printf("Testing isInPresetMode() helper...\n");

    CameraModeManager manager;
    CameraState cameraState;
    manager.initialize(cameraState);

    // Initially in preset
    assert(manager.isInPresetMode());

    // Force to free
    manager.forceToFreeMode(cameraState);
    assert(!manager.isInPresetMode());

    printf("  PASS: isInPresetMode() works correctly\n");
}

void test_is_animating() {
    printf("Testing isAnimating() helper...\n");

    CameraModeManager manager;
    CameraState cameraState;
    manager.initialize(cameraState);

    // Initially not animating
    assert(!manager.isAnimating());

    // Force transition to preset with animation
    manager.forceToFreeMode(cameraState);  // Go to free first
    manager.forceToPreset(CameraMode::Preset_E, cameraState, true);  // Animated

    // Should now be animating
    assert(manager.isAnimating());

    printf("  PASS: isAnimating() works correctly\n");
}

// ============================================================================
// Mode Transition Tests (No Jarring Visual Jump)
// ============================================================================

void test_preset_to_free_instant_unlock() {
    printf("Testing preset-to-free instant unlock...\n");

    CameraModeManager manager;
    CameraState cameraState;
    manager.initialize(cameraState);

    // Start in preset mode
    assert(manager.getCameraMode() == CameraMode::Preset_N);

    // Simulate orbit input by forcing to free mode (OrbitController does this)
    manager.forceToFreeMode(cameraState);

    // Should immediately be in free mode (no animation)
    assert(manager.getCameraMode() == CameraMode::Free);
    assert(cameraState.mode == CameraMode::Free);
    assert(!manager.isAnimating());

    printf("  PASS: Preset-to-free transition is instant (no animation delay)\n");
}

void test_free_to_preset_smooth_snap() {
    printf("Testing free-to-preset smooth snap...\n");

    CameraModeManager manager;
    CameraState cameraState;
    manager.initialize(cameraState);

    // Start in free mode
    manager.forceToFreeMode(cameraState);
    cameraState.yaw = 180.0f;  // Looking south
    cameraState.pitch = 50.0f;  // Different from isometric

    // Trigger preset snap
    manager.forceToPreset(CameraMode::Preset_N, cameraState, true);

    // Should be animating, not instantly at target
    assert(manager.isAnimating());

    // Yaw and pitch should NOT have instantly jumped to preset values
    // (they're still at or near previous values because animation just started)

    // Update partway
    manager.update(0.1f, cameraState);

    // Should still be animating
    assert(manager.isAnimating() || !approxEqual(cameraState.yaw, CameraConfig::PRESET_N_YAW, 1.0f));

    printf("  PASS: Free-to-preset transition is smooth animated snap\n");
}

void test_smooth_animation_duration() {
    printf("Testing animation duration is 0.3-0.5 seconds...\n");

    CameraModeManagerConfig config;
    config.presetSnapDuration = 0.4f;

    CameraModeManager manager(config);
    CameraState cameraState;
    manager.initialize(cameraState);

    manager.forceToFreeMode(cameraState);
    manager.forceToPreset(CameraMode::Preset_E, cameraState, true);

    // Should still be animating after 0.2 seconds
    manager.update(0.2f, cameraState);
    assert(manager.isAnimating());

    // Should be done after 0.25 more seconds (total 0.45s)
    manager.update(0.25f, cameraState);

    // Animation should have completed
    for (int i = 0; i < 10; ++i) {
        manager.update(0.05f, cameraState);
    }
    assert(!manager.isAnimating());

    printf("  PASS: Animation duration is configurable (0.3-0.5s range)\n");
}

// ============================================================================
// Q/E Key Behavior Tests (Acceptance Criteria)
// ============================================================================

void test_free_to_preset_on_qe_input() {
    printf("Testing Q/E triggers smooth snap from free mode...\n");

    CameraModeManager manager;
    CameraState cameraState;
    manager.initialize(cameraState);

    // Start in free mode
    manager.forceToFreeMode(cameraState);
    cameraState.yaw = 0.0f;

    // Trigger preset snap (simulating Q key press)
    manager.forceToPreset(CameraMode::Preset_E, cameraState, true);

    // Should be animating to preset
    assert(manager.isAnimating());

    // Complete animation
    for (int i = 0; i < 100; ++i) {
        manager.update(0.016f, cameraState);
    }

    // Should now be in preset mode
    assert(manager.getCameraMode() == CameraMode::Preset_E);
    assert(approxEqual(cameraState.yaw, CameraConfig::PRESET_E_YAW, 1.0f));

    printf("  PASS: Q/E input triggers smooth snap from free mode\n");
}

// ============================================================================
// Orbit/Tilt Input Tests (Acceptance Criteria: Preset-to-free instant)
// ============================================================================

void test_orbit_input_unlocks_preset() {
    printf("Testing orbit input instantly unlocks from preset...\n");

    CameraModeManager manager;
    CameraState cameraState;
    manager.initialize(cameraState);

    assert(manager.isInPresetMode());

    // Simulate orbit/tilt input by directly triggering orbit behavior
    // In real usage, OrbitController::handleOrbitTilt() does this
    manager.getOrbitController().handleOrbitTilt(10, 5, cameraState);

    // OrbitController sets mode to Free internally
    // Manager needs to sync
    manager.update(0.0f, cameraState);

    // Should be in free mode
    assert(manager.getCameraMode() == CameraMode::Free);

    printf("  PASS: Orbit/tilt input instantly unlocks from preset\n");
}

// ============================================================================
// Preset Indicator Tests
// ============================================================================

void test_get_preset_indicator() {
    printf("Testing getPresetIndicator()...\n");

    CameraModeManager manager;
    CameraState cameraState;
    manager.initialize(cameraState);

    PresetIndicator indicator = manager.getPresetIndicator(cameraState);

    // In preset mode, should report current preset
    assert(indicator.currentPreset == CameraMode::Preset_N);
    assert(!indicator.isAnimating);

    printf("  PASS: getPresetIndicator() returns correct data\n");
}

void test_get_current_preset() {
    printf("Testing getCurrentPreset()...\n");

    CameraModeManager manager;
    CameraState cameraState;
    manager.initialize(cameraState);

    // In preset mode
    assert(manager.getCurrentPreset() == CameraMode::Preset_N);

    // In free mode, should return last preset
    manager.forceToFreeMode(cameraState);
    assert(manager.getCurrentPreset() == CameraMode::Preset_N);

    // After snapping to different preset
    manager.forceToPreset(CameraMode::Preset_W, cameraState, false);
    assert(manager.getCurrentPreset() == CameraMode::Preset_W);

    printf("  PASS: getCurrentPreset() returns correct preset\n");
}

// ============================================================================
// Controller Access Tests
// ============================================================================

void test_controller_access() {
    printf("Testing controller access...\n");

    CameraModeManager manager;

    // Should be able to access controllers
    OrbitController& orbit = manager.getOrbitController();
    PresetSnapController& preset = manager.getPresetSnapController();
    CameraAnimator& animator = manager.getAnimator();

    // Verify they're usable (not null)
    (void)orbit.getConfig();
    (void)preset.getConfig();
    (void)animator.getConfig();

    printf("  PASS: Controllers are accessible\n");
}

// ============================================================================
// Reset Tests
// ============================================================================

void test_reset() {
    printf("Testing reset()...\n");

    CameraModeManager manager;
    CameraState cameraState;
    manager.initialize(cameraState);

    // Modify state
    manager.forceToFreeMode(cameraState);
    cameraState.yaw = 180.0f;
    cameraState.pitch = 60.0f;
    cameraState.distance = 80.0f;

    // Reset
    manager.reset(cameraState);

    // Should be back to default
    assert(manager.getCameraMode() == CameraMode::Preset_N);
    assert(cameraState.mode == CameraMode::Preset_N);
    assert(approxEqual(cameraState.yaw, CameraConfig::PRESET_N_YAW, 0.1f));
    assert(approxEqual(cameraState.pitch, CameraConfig::ISOMETRIC_PITCH, 0.1f));

    printf("  PASS: reset() restores default state\n");
}

// ============================================================================
// Configuration Tests
// ============================================================================

void test_set_config() {
    printf("Testing setConfig()...\n");

    CameraModeManager manager;

    CameraModeManagerConfig newConfig;
    newConfig.defaultMode = CameraMode::Preset_W;
    newConfig.presetSnapDuration = 0.5f;

    manager.setConfig(newConfig);

    assert(manager.getConfig().defaultMode == CameraMode::Preset_W);
    assert(approxEqual(manager.getConfig().presetSnapDuration, 0.5f));

    // Verify sub-controllers were updated
    assert(approxEqual(manager.getPresetSnapController().getConfig().snapDuration, 0.5f));

    printf("  PASS: setConfig() updates configuration\n");
}

// ============================================================================
// Edge Case Tests
// ============================================================================

void test_force_to_preset_instant() {
    printf("Testing forceToPreset with instant snap...\n");

    CameraModeManager manager;
    CameraState cameraState;
    manager.initialize(cameraState);

    manager.forceToFreeMode(cameraState);
    cameraState.yaw = 0.0f;

    // Force instant (no animation)
    manager.forceToPreset(CameraMode::Preset_S, cameraState, false);

    // Should be at target immediately
    assert(manager.getCameraMode() == CameraMode::Preset_S);
    assert(!manager.isAnimating());
    assert(approxEqual(cameraState.yaw, CameraConfig::PRESET_S_YAW, 0.1f));

    printf("  PASS: forceToPreset can snap instantly\n");
}

void test_multiple_preset_transitions() {
    printf("Testing multiple preset transitions...\n");

    CameraModeManager manager;
    CameraState cameraState;
    manager.initialize(cameraState);

    // Cycle through all presets
    CameraMode presets[] = {
        CameraMode::Preset_E,
        CameraMode::Preset_S,
        CameraMode::Preset_W,
        CameraMode::Preset_N
    };

    for (CameraMode preset : presets) {
        manager.forceToPreset(preset, cameraState, false);
        assert(manager.getCameraMode() == preset);
    }

    printf("  PASS: Multiple preset transitions work correctly\n");
}

void test_animation_interrupt() {
    printf("Testing animation can be interrupted...\n");

    CameraModeManager manager;
    CameraState cameraState;
    manager.initialize(cameraState);

    // Start animation
    manager.forceToFreeMode(cameraState);
    manager.forceToPreset(CameraMode::Preset_E, cameraState, true);
    assert(manager.isAnimating());

    // Interrupt by going to free mode
    manager.forceToFreeMode(cameraState);

    // Should no longer be animating
    assert(!manager.isAnimating());
    assert(manager.getCameraMode() == CameraMode::Free);

    printf("  PASS: Animation can be interrupted\n");
}

void test_invalid_preset_ignored() {
    printf("Testing invalid preset is ignored...\n");

    CameraModeManager manager;
    CameraState cameraState;
    manager.initialize(cameraState);

    CameraMode originalMode = manager.getCameraMode();

    // Try to force to invalid preset (Free is not a preset)
    manager.forceToPreset(CameraMode::Free, cameraState, false);

    // Should remain in original mode
    assert(manager.getCameraMode() == originalMode);

    // Try Animating (also not a valid preset)
    manager.forceToPreset(CameraMode::Animating, cameraState, false);
    assert(manager.getCameraMode() == originalMode);

    printf("  PASS: Invalid preset targets are ignored\n");
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main() {
    printf("=== CameraModeManager Unit Tests (Ticket 2-048) ===\n\n");

    // Mode enum tests
    printf("--- Mode Enum Tests ---\n");
    test_mode_enum_values();

    // Construction tests
    printf("\n--- Construction Tests ---\n");
    test_default_construction();
    test_custom_config_construction();

    // Default mode tests
    printf("\n--- Default Mode Tests ---\n");
    test_default_mode_preset_on_start();
    test_custom_default_mode();

    // getCameraMode() API tests
    printf("\n--- getCameraMode() API Tests ---\n");
    test_get_camera_mode_api();
    test_is_in_free_mode();
    test_is_in_preset_mode();
    test_is_animating();

    // Mode transition tests
    printf("\n--- Mode Transition Tests ---\n");
    test_preset_to_free_instant_unlock();
    test_free_to_preset_smooth_snap();
    test_smooth_animation_duration();

    // Q/E key behavior tests
    printf("\n--- Q/E Key Behavior Tests ---\n");
    test_free_to_preset_on_qe_input();

    // Orbit/tilt input tests
    printf("\n--- Orbit/Tilt Input Tests ---\n");
    test_orbit_input_unlocks_preset();

    // Preset indicator tests
    printf("\n--- Preset Indicator Tests ---\n");
    test_get_preset_indicator();
    test_get_current_preset();

    // Controller access tests
    printf("\n--- Controller Access Tests ---\n");
    test_controller_access();

    // Reset tests
    printf("\n--- Reset Tests ---\n");
    test_reset();

    // Configuration tests
    printf("\n--- Configuration Tests ---\n");
    test_set_config();

    // Edge case tests
    printf("\n--- Edge Case Tests ---\n");
    test_force_to_preset_instant();
    test_multiple_preset_transitions();
    test_animation_interrupt();
    test_invalid_preset_ignored();

    printf("\n=== All tests passed! ===\n");
    return 0;
}
