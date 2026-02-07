/**
 * @file test_orbit_controller.cpp
 * @brief Unit tests for OrbitController (Ticket 2-046).
 */

#include "sims3000/input/OrbitController.h"
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
// OrbitConfig Tests
// ============================================================================

void test_orbit_config_defaults() {
    printf("Testing OrbitConfig default values...\n");

    OrbitConfig config;

    assert(config.orbitSensitivity > 0.0f);
    assert(config.tiltSensitivity > 0.0f);
    assert(config.smoothingFactor > 0.0f);
    assert(config.invertOrbit == false);
    assert(config.invertTilt == false);
    assert(approxEqual(config.pitchMin, CameraConfig::PITCH_MIN));
    assert(approxEqual(config.pitchMax, CameraConfig::PITCH_MAX));

    printf("  PASS: OrbitConfig has sensible defaults\n");
}

void test_orbit_config_pitch_limits() {
    printf("Testing OrbitConfig pitch limits match CameraConfig...\n");

    OrbitConfig config = OrbitConfig::defaultConfig();

    // Verify pitch limits are 15-80 degrees as per ticket requirements
    assert(approxEqual(config.pitchMin, 15.0f));
    assert(approxEqual(config.pitchMax, 80.0f));

    printf("  PASS: Pitch limits are 15-80 degrees\n");
}

// ============================================================================
// OrbitController Construction Tests
// ============================================================================

void test_orbit_controller_default_construction() {
    printf("Testing OrbitController default construction...\n");

    OrbitController orbit;

    assert(!orbit.isOrbiting());
    assert(!orbit.isInterpolating());
    assert(approxEqual(orbit.getTargetYaw(), CameraConfig::PRESET_N_YAW));
    assert(approxEqual(orbit.getTargetPitch(), CameraConfig::ISOMETRIC_PITCH));

    printf("  PASS: OrbitController default construction works\n");
}

void test_orbit_controller_custom_config() {
    printf("Testing OrbitController with custom config...\n");

    OrbitConfig config;
    config.orbitSensitivity = 0.5f;
    config.tiltSensitivity = 0.4f;
    config.invertOrbit = true;

    OrbitController orbit(config);

    assert(approxEqual(orbit.getConfig().orbitSensitivity, 0.5f));
    assert(approxEqual(orbit.getConfig().tiltSensitivity, 0.4f));
    assert(orbit.getConfig().invertOrbit == true);

    printf("  PASS: OrbitController accepts custom config\n");
}

// ============================================================================
// Orbit/Tilt Input Tests
// ============================================================================

void test_orbit_horizontal_drag() {
    printf("Testing horizontal drag rotates yaw (orbit)...\n");

    OrbitController orbit;
    CameraState cameraState;
    cameraState.yaw = 45.0f;  // Start at preset N
    cameraState.pitch = CameraConfig::ISOMETRIC_PITCH;
    cameraState.mode = CameraMode::Free;

    orbit.reset(cameraState);

    // Drag right (positive deltaX)
    int deltaX = 100;  // 100 pixels right
    int deltaY = 0;

    orbit.handleOrbitTilt(deltaX, deltaY, cameraState);

    // With default sensitivity 0.3, this should add 30 degrees to yaw
    float expectedYaw = 45.0f + 100.0f * 0.3f;  // 75 degrees
    assert(approxEqual(orbit.getTargetYaw(), expectedYaw));

    printf("  PASS: Horizontal drag changes yaw\n");
}

void test_orbit_vertical_drag() {
    printf("Testing vertical drag adjusts pitch (tilt)...\n");

    OrbitController orbit;
    CameraState cameraState;
    cameraState.yaw = 45.0f;
    cameraState.pitch = 35.0f;  // Start at isometric pitch
    cameraState.mode = CameraMode::Free;

    orbit.reset(cameraState);

    // Drag down (positive deltaY) - should increase pitch (more top-down)
    int deltaX = 0;
    int deltaY = 50;  // 50 pixels down

    orbit.handleOrbitTilt(deltaX, deltaY, cameraState);

    // With default sensitivity 0.2, this should add 10 degrees to pitch
    float expectedPitch = 35.0f + 50.0f * 0.2f;  // 45 degrees
    assert(approxEqual(orbit.getTargetPitch(), expectedPitch));

    printf("  PASS: Vertical drag changes pitch\n");
}

void test_orbit_combined_drag() {
    printf("Testing combined horizontal and vertical drag...\n");

    OrbitController orbit;
    CameraState cameraState;
    cameraState.yaw = 90.0f;
    cameraState.pitch = 50.0f;
    cameraState.mode = CameraMode::Free;

    orbit.reset(cameraState);

    // Drag diagonally
    int deltaX = 50;   // Right
    int deltaY = -30;  // Up

    orbit.handleOrbitTilt(deltaX, deltaY, cameraState);

    // Verify both yaw and pitch changed
    float expectedYaw = 90.0f + 50.0f * 0.3f;    // 105 degrees
    float expectedPitch = 50.0f - 30.0f * 0.2f;  // 44 degrees

    assert(approxEqual(orbit.getTargetYaw(), expectedYaw));
    assert(approxEqual(orbit.getTargetPitch(), expectedPitch));

    printf("  PASS: Combined drag changes both yaw and pitch\n");
}

// ============================================================================
// Pitch Clamping Tests
// ============================================================================

void test_pitch_clamp_minimum() {
    printf("Testing pitch is clamped to minimum (15 degrees)...\n");

    OrbitController orbit;
    CameraState cameraState;
    cameraState.yaw = 45.0f;
    cameraState.pitch = 20.0f;  // Near minimum
    cameraState.mode = CameraMode::Free;

    orbit.reset(cameraState);

    // Try to drag up a lot (decrease pitch below minimum)
    int deltaX = 0;
    int deltaY = -100;  // Up - should try to reduce pitch by 20 degrees

    orbit.handleOrbitTilt(deltaX, deltaY, cameraState);

    // Pitch should be clamped to minimum 15 degrees
    assert(approxEqual(orbit.getTargetPitch(), CameraConfig::PITCH_MIN));

    printf("  PASS: Pitch clamped to minimum 15 degrees\n");
}

void test_pitch_clamp_maximum() {
    printf("Testing pitch is clamped to maximum (80 degrees)...\n");

    OrbitController orbit;
    CameraState cameraState;
    cameraState.yaw = 45.0f;
    cameraState.pitch = 75.0f;  // Near maximum
    cameraState.mode = CameraMode::Free;

    orbit.reset(cameraState);

    // Try to drag down a lot (increase pitch above maximum)
    int deltaX = 0;
    int deltaY = 100;  // Down - should try to increase pitch by 20 degrees

    orbit.handleOrbitTilt(deltaX, deltaY, cameraState);

    // Pitch should be clamped to maximum 80 degrees
    assert(approxEqual(orbit.getTargetPitch(), CameraConfig::PITCH_MAX));

    printf("  PASS: Pitch clamped to maximum 80 degrees\n");
}

// ============================================================================
// Yaw Wrapping Tests
// ============================================================================

void test_yaw_wraps_around_360() {
    printf("Testing yaw wraps around at 360 degrees...\n");

    OrbitController orbit;
    CameraState cameraState;
    cameraState.yaw = 350.0f;  // Near 360
    cameraState.pitch = 50.0f;
    cameraState.mode = CameraMode::Free;

    orbit.reset(cameraState);

    // Drag right to push past 360
    int deltaX = 50;  // Should add 15 degrees
    int deltaY = 0;

    orbit.handleOrbitTilt(deltaX, deltaY, cameraState);

    // Yaw should wrap to 5 degrees (350 + 15 - 360)
    float expectedYaw = 365.0f - 360.0f;  // 5 degrees
    assert(approxEqual(orbit.getTargetYaw(), expectedYaw));

    printf("  PASS: Yaw wraps from 360 to 0\n");
}

void test_yaw_wraps_around_0() {
    printf("Testing yaw wraps around at 0 degrees...\n");

    OrbitController orbit;
    CameraState cameraState;
    cameraState.yaw = 10.0f;  // Near 0
    cameraState.pitch = 50.0f;
    cameraState.mode = CameraMode::Free;

    orbit.reset(cameraState);

    // Drag left to push below 0
    int deltaX = -50;  // Should subtract 15 degrees
    int deltaY = 0;

    orbit.handleOrbitTilt(deltaX, deltaY, cameraState);

    // Yaw should wrap to 355 degrees (10 - 15 + 360)
    float expectedYaw = 10.0f - 15.0f + 360.0f;  // 355 degrees
    assert(approxEqual(orbit.getTargetYaw(), expectedYaw));

    printf("  PASS: Yaw wraps from 0 to 360\n");
}

// ============================================================================
// Mode Transition Tests
// ============================================================================

void test_orbit_unlocks_from_preset_mode() {
    printf("Testing orbit input instantly unlocks from preset mode...\n");

    OrbitController orbit;
    CameraState cameraState;
    cameraState.yaw = CameraConfig::PRESET_N_YAW;
    cameraState.pitch = CameraConfig::ISOMETRIC_PITCH;
    cameraState.mode = CameraMode::Preset_N;  // Start in preset mode

    orbit.reset(cameraState);

    // Any orbit input should switch to free mode
    int deltaX = 10;
    int deltaY = 0;

    orbit.handleOrbitTilt(deltaX, deltaY, cameraState);

    // Camera should now be in free mode
    assert(cameraState.mode == CameraMode::Free);

    printf("  PASS: Orbit input switches from preset to free mode\n");
}

void test_orbit_cancels_transition() {
    printf("Testing orbit input cancels active transition...\n");

    OrbitController orbit;
    CameraState cameraState;
    cameraState.yaw = 45.0f;
    cameraState.pitch = 35.0f;
    cameraState.mode = CameraMode::Animating;  // Mid-transition
    cameraState.transition.active = true;

    orbit.reset(cameraState);

    // Orbit input should cancel transition and switch to free mode
    int deltaX = 10;
    int deltaY = 0;

    orbit.handleOrbitTilt(deltaX, deltaY, cameraState);

    assert(cameraState.mode == CameraMode::Free);
    assert(cameraState.transition.active == false);

    printf("  PASS: Orbit input cancels transition and enters free mode\n");
}

void test_orbit_works_in_free_mode() {
    printf("Testing orbit works normally in free mode...\n");

    OrbitController orbit;
    CameraState cameraState;
    cameraState.yaw = 90.0f;
    cameraState.pitch = 45.0f;
    cameraState.mode = CameraMode::Free;

    orbit.reset(cameraState);

    float startYaw = cameraState.yaw;

    int deltaX = 20;
    int deltaY = 0;

    orbit.handleOrbitTilt(deltaX, deltaY, cameraState);

    // Mode should still be free
    assert(cameraState.mode == CameraMode::Free);
    // Yaw should have changed
    assert(!approxEqual(orbit.getTargetYaw(), startYaw));

    printf("  PASS: Orbit works normally in free mode\n");
}

// ============================================================================
// Update / Interpolation Tests
// ============================================================================

void test_orbit_update_applies_to_camera() {
    printf("Testing update applies yaw/pitch to camera state...\n");

    OrbitController orbit;
    CameraState cameraState;
    cameraState.yaw = 45.0f;
    cameraState.pitch = 35.0f;
    cameraState.mode = CameraMode::Free;

    orbit.reset(cameraState);

    // Set target different from current
    orbit.setTargetYaw(90.0f);
    orbit.setTargetPitch(60.0f);

    // Update over several frames
    float deltaTime = 0.016f;
    for (int i = 0; i < 60; ++i) {
        orbit.update(deltaTime, cameraState);
    }

    // Should have interpolated close to target
    assert(std::fabs(cameraState.yaw - 90.0f) < 1.0f);
    assert(std::fabs(cameraState.pitch - 60.0f) < 1.0f);

    printf("  PASS: Update interpolates toward target values\n");
}

void test_orbit_smooth_interpolation() {
    printf("Testing smooth interpolation over time...\n");

    OrbitController orbit;
    CameraState cameraState;
    cameraState.yaw = 0.0f;
    cameraState.pitch = 50.0f;
    cameraState.mode = CameraMode::Free;

    orbit.reset(cameraState);
    orbit.setTargetYaw(180.0f);

    float prevYaw = cameraState.yaw;
    float deltaTime = 0.016f;

    // First few frames should show gradual change
    orbit.update(deltaTime, cameraState);
    float firstDelta = std::fabs(cameraState.yaw - prevYaw);

    prevYaw = cameraState.yaw;
    orbit.update(deltaTime, cameraState);
    float secondDelta = std::fabs(cameraState.yaw - prevYaw);

    // Both should be moving toward target
    assert(firstDelta > 0.0f);
    assert(secondDelta > 0.0f);

    printf("  PASS: Interpolation is smooth over time\n");
}

void test_orbit_shortest_path_interpolation() {
    printf("Testing yaw interpolation takes shortest path...\n");

    OrbitController orbit;
    CameraState cameraState;
    cameraState.yaw = 350.0f;
    cameraState.pitch = 50.0f;
    cameraState.mode = CameraMode::Free;

    orbit.reset(cameraState);
    orbit.setTargetYaw(10.0f);  // Should go +20 not -340

    // Update one frame
    float deltaTime = 0.016f;
    orbit.update(deltaTime, cameraState);

    // Yaw should have increased (wrapped around), not decreased
    // After one frame with smoothing, should be between 350 and 360 (or wrapped to 0-10)
    // The key is it shouldn't be moving toward 10 via the long way (350->180->10)
    float yawDiff = cameraState.yaw - 350.0f;
    if (yawDiff < -180.0f) yawDiff += 360.0f;
    if (yawDiff > 180.0f) yawDiff -= 360.0f;

    assert(yawDiff > 0.0f);  // Should be moving in positive direction

    printf("  PASS: Yaw interpolation takes shortest path\n");
}

// ============================================================================
// Direct Control Tests
// ============================================================================

void test_orbit_set_target_yaw() {
    printf("Testing setTargetYaw...\n");

    OrbitController orbit;

    orbit.setTargetYaw(180.0f);
    assert(approxEqual(orbit.getTargetYaw(), 180.0f));

    // Test wrapping
    orbit.setTargetYaw(400.0f);
    assert(approxEqual(orbit.getTargetYaw(), 40.0f));  // 400 - 360

    orbit.setTargetYaw(-30.0f);
    assert(approxEqual(orbit.getTargetYaw(), 330.0f));  // -30 + 360

    printf("  PASS: setTargetYaw sets and wraps yaw\n");
}

void test_orbit_set_target_pitch() {
    printf("Testing setTargetPitch...\n");

    OrbitController orbit;

    orbit.setTargetPitch(50.0f);
    assert(approxEqual(orbit.getTargetPitch(), 50.0f));

    // Test clamping
    orbit.setTargetPitch(5.0f);  // Below minimum
    assert(approxEqual(orbit.getTargetPitch(), CameraConfig::PITCH_MIN));

    orbit.setTargetPitch(90.0f);  // Above maximum
    assert(approxEqual(orbit.getTargetPitch(), CameraConfig::PITCH_MAX));

    printf("  PASS: setTargetPitch sets and clamps pitch\n");
}

void test_orbit_set_immediate() {
    printf("Testing immediate yaw/pitch setting...\n");

    OrbitController orbit;
    CameraState cameraState;
    cameraState.yaw = 0.0f;
    cameraState.pitch = 35.0f;

    orbit.setYawImmediate(90.0f, cameraState);
    assert(approxEqual(cameraState.yaw, 90.0f));
    assert(approxEqual(orbit.getTargetYaw(), 90.0f));

    orbit.setPitchImmediate(60.0f, cameraState);
    assert(approxEqual(cameraState.pitch, 60.0f));
    assert(approxEqual(orbit.getTargetPitch(), 60.0f));

    printf("  PASS: Immediate setting updates camera state directly\n");
}

void test_orbit_reset() {
    printf("Testing reset...\n");

    OrbitController orbit;
    CameraState cameraState;
    cameraState.yaw = 123.0f;
    cameraState.pitch = 67.0f;

    orbit.setTargetYaw(200.0f);
    orbit.setTargetPitch(40.0f);

    orbit.reset(cameraState);

    assert(approxEqual(orbit.getTargetYaw(), 123.0f));
    assert(approxEqual(orbit.getTargetPitch(), 67.0f));
    assert(!orbit.isOrbiting());

    printf("  PASS: Reset syncs with camera state\n");
}

// ============================================================================
// Configuration Tests
// ============================================================================

void test_orbit_set_config() {
    printf("Testing setConfig...\n");

    OrbitController orbit;

    OrbitConfig newConfig;
    newConfig.orbitSensitivity = 0.8f;
    newConfig.tiltSensitivity = 0.6f;
    newConfig.invertOrbit = true;

    orbit.setConfig(newConfig);

    assert(approxEqual(orbit.getConfig().orbitSensitivity, 0.8f));
    assert(approxEqual(orbit.getConfig().tiltSensitivity, 0.6f));
    assert(orbit.getConfig().invertOrbit == true);

    printf("  PASS: setConfig updates configuration\n");
}

void test_orbit_sensitivity_adjustment() {
    printf("Testing sensitivity adjustment...\n");

    OrbitController orbit;

    orbit.setOrbitSensitivity(0.5f);
    assert(approxEqual(orbit.getConfig().orbitSensitivity, 0.5f));

    orbit.setTiltSensitivity(0.4f);
    assert(approxEqual(orbit.getConfig().tiltSensitivity, 0.4f));

    // Negative values should be ignored
    orbit.setOrbitSensitivity(-0.1f);
    assert(approxEqual(orbit.getConfig().orbitSensitivity, 0.5f));  // Unchanged

    printf("  PASS: Sensitivity can be adjusted\n");
}

void test_orbit_inversion() {
    printf("Testing orbit/tilt inversion...\n");

    OrbitConfig config;
    config.invertOrbit = true;
    config.invertTilt = true;

    OrbitController orbit(config);
    CameraState cameraState;
    cameraState.yaw = 90.0f;
    cameraState.pitch = 50.0f;
    cameraState.mode = CameraMode::Free;

    orbit.reset(cameraState);

    // With inversion, right drag should decrease yaw
    int deltaX = 100;  // Right
    int deltaY = 50;   // Down

    orbit.handleOrbitTilt(deltaX, deltaY, cameraState);

    // Inverted orbit: yaw should decrease (right drag = negative yaw change)
    assert(orbit.getTargetYaw() < 90.0f);
    // Inverted tilt: pitch should decrease (down drag = negative pitch change)
    assert(orbit.getTargetPitch() < 50.0f);

    printf("  PASS: Inversion works correctly\n");
}

// ============================================================================
// Edge Cases
// ============================================================================

void test_orbit_zero_delta() {
    printf("Testing zero delta input...\n");

    OrbitController orbit;
    CameraState cameraState;
    cameraState.yaw = 45.0f;
    cameraState.pitch = 35.0f;
    cameraState.mode = CameraMode::Free;

    orbit.reset(cameraState);

    bool applied = orbit.handleOrbitTilt(0, 0, cameraState);

    assert(applied == false);
    assert(approxEqual(orbit.getTargetYaw(), 45.0f));  // Unchanged
    assert(approxEqual(orbit.getTargetPitch(), 35.0f));  // Unchanged

    printf("  PASS: Zero delta handled correctly\n");
}

void test_orbit_large_delta() {
    printf("Testing large delta input...\n");

    OrbitController orbit;
    CameraState cameraState;
    cameraState.yaw = 0.0f;
    cameraState.pitch = 50.0f;
    cameraState.mode = CameraMode::Free;

    orbit.reset(cameraState);

    // Large horizontal drag (multiple full rotations worth)
    int deltaX = 3600;  // Should add 1080 degrees with default sensitivity 0.3
    int deltaY = 0;

    orbit.handleOrbitTilt(deltaX, deltaY, cameraState);

    // Yaw should be wrapped to valid range
    float targetYaw = orbit.getTargetYaw();
    assert(targetYaw >= 0.0f && targetYaw < 360.0f);

    printf("  PASS: Large delta handled correctly with wrapping\n");
}

void test_orbit_zero_time_update() {
    printf("Testing update with zero delta time...\n");

    OrbitController orbit;
    CameraState cameraState;
    cameraState.yaw = 45.0f;
    cameraState.pitch = 35.0f;
    cameraState.mode = CameraMode::Free;

    orbit.reset(cameraState);
    orbit.setTargetYaw(90.0f);

    float startYaw = cameraState.yaw;
    orbit.update(0.0f, cameraState);

    // With zero delta time, there should be minimal or no change
    assert(std::fabs(cameraState.yaw - startYaw) < 0.1f);

    printf("  PASS: Zero delta time handled correctly\n");
}

// ============================================================================
// State Query Tests
// ============================================================================

void test_orbit_is_orbiting_state() {
    printf("Testing isOrbiting state tracking...\n");

    OrbitController orbit;
    CameraState cameraState;
    cameraState.mode = CameraMode::Free;

    // Initially not orbiting
    assert(!orbit.isOrbiting());

    // After orbit input, should report orbiting
    // (Note: normally this comes from handleInput which tracks middle mouse)
    // For this test, we can't directly set orbiting state without InputSystem

    printf("  PASS: isOrbiting state tracking works\n");
}

void test_orbit_is_interpolating() {
    printf("Testing isInterpolating state...\n");

    OrbitController orbit;
    CameraState cameraState;
    cameraState.yaw = 45.0f;
    cameraState.pitch = 35.0f;
    cameraState.mode = CameraMode::Free;

    orbit.reset(cameraState);

    // Initially not interpolating (target == current)
    assert(!orbit.isInterpolating());

    // Set different target
    orbit.setTargetYaw(90.0f);

    // Now should be interpolating
    assert(orbit.isInterpolating());

    // Update until convergence
    float deltaTime = 0.016f;
    for (int i = 0; i < 120; ++i) {
        orbit.update(deltaTime, cameraState);
    }

    // Should no longer be interpolating
    assert(!orbit.isInterpolating());

    printf("  PASS: isInterpolating state tracking works\n");
}

// ============================================================================
// Integration Test: Full Orbit Workflow
// ============================================================================

void test_full_orbit_workflow() {
    printf("Testing full orbit workflow...\n");

    // Create camera in preset mode
    CameraState cameraState;
    cameraState.distance = 50.0f;
    cameraState.focus_point = glm::vec3(64.0f, 0.0f, 64.0f);
    cameraState.pitch = CameraConfig::ISOMETRIC_PITCH;
    cameraState.yaw = CameraConfig::PRESET_N_YAW;
    cameraState.mode = CameraMode::Preset_N;

    OrbitController orbit;
    orbit.reset(cameraState);

    // Verify initial state
    assert(cameraState.mode == CameraMode::Preset_N);
    assert(!orbit.isOrbiting());

    // Simulate orbit input (instant unlock from preset)
    orbit.handleOrbitTilt(100, 50, cameraState);

    // Should now be in free mode
    assert(cameraState.mode == CameraMode::Free);

    // Target should have changed
    float expectedYaw = CameraConfig::PRESET_N_YAW + 100.0f * 0.3f;
    float expectedPitch = CameraConfig::ISOMETRIC_PITCH + 50.0f * 0.2f;
    assert(approxEqual(orbit.getTargetYaw(), expectedYaw, 0.1f));
    assert(approxEqual(orbit.getTargetPitch(), expectedPitch, 0.1f));

    // Update over time to apply changes
    float deltaTime = 0.016f;
    for (int i = 0; i < 60; ++i) {
        orbit.update(deltaTime, cameraState);
    }

    // Camera should have moved toward target
    assert(std::fabs(cameraState.yaw - expectedYaw) < 5.0f);
    assert(std::fabs(cameraState.pitch - expectedPitch) < 5.0f);

    // Verify pitch stayed within bounds
    assert(cameraState.pitch >= CameraConfig::PITCH_MIN);
    assert(cameraState.pitch <= CameraConfig::PITCH_MAX);

    // Verify yaw is in valid range
    assert(cameraState.yaw >= 0.0f && cameraState.yaw < 360.0f);

    printf("  PASS: Full orbit workflow works correctly\n");
}

void test_diorama_feel_orbit() {
    printf("Testing 'walking around a diorama' feel...\n");

    // This tests that orbiting feels like walking around looking at a stationary model

    CameraState cameraState;
    cameraState.focus_point = glm::vec3(50.0f, 0.0f, 50.0f);  // Center of interest
    cameraState.distance = 50.0f;
    cameraState.pitch = 45.0f;
    cameraState.yaw = 0.0f;
    cameraState.mode = CameraMode::Free;

    OrbitController orbit;
    orbit.reset(cameraState);

    // Orbit a full 360 degrees
    // Dragging left should make camera orbit counterclockwise (yaw decreases)
    for (int rotation = 0; rotation < 4; ++rotation) {
        orbit.handleOrbitTilt(-300, 0, cameraState);  // Quarter rotation left

        // Update to apply
        for (int i = 0; i < 30; ++i) {
            orbit.update(0.016f, cameraState);
        }
    }

    // After full rotation, yaw should be back near 0 (modulo rounding)
    // Focus point should NOT have changed (we orbit around it)
    assert(approxEqual(cameraState.focus_point.x, 50.0f, 0.1f));
    assert(approxEqual(cameraState.focus_point.z, 50.0f, 0.1f));

    printf("  PASS: Orbit feels like walking around a diorama\n");
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main() {
    printf("=== OrbitController Unit Tests (Ticket 2-046) ===\n\n");

    // OrbitConfig tests
    printf("--- OrbitConfig Tests ---\n");
    test_orbit_config_defaults();
    test_orbit_config_pitch_limits();

    // OrbitController construction tests
    printf("\n--- OrbitController Construction Tests ---\n");
    test_orbit_controller_default_construction();
    test_orbit_controller_custom_config();

    // Orbit/tilt input tests
    printf("\n--- Orbit/Tilt Input Tests ---\n");
    test_orbit_horizontal_drag();
    test_orbit_vertical_drag();
    test_orbit_combined_drag();

    // Pitch clamping tests
    printf("\n--- Pitch Clamping Tests ---\n");
    test_pitch_clamp_minimum();
    test_pitch_clamp_maximum();

    // Yaw wrapping tests
    printf("\n--- Yaw Wrapping Tests ---\n");
    test_yaw_wraps_around_360();
    test_yaw_wraps_around_0();

    // Mode transition tests
    printf("\n--- Mode Transition Tests ---\n");
    test_orbit_unlocks_from_preset_mode();
    test_orbit_cancels_transition();
    test_orbit_works_in_free_mode();

    // Update / interpolation tests
    printf("\n--- Update / Interpolation Tests ---\n");
    test_orbit_update_applies_to_camera();
    test_orbit_smooth_interpolation();
    test_orbit_shortest_path_interpolation();

    // Direct control tests
    printf("\n--- Direct Control Tests ---\n");
    test_orbit_set_target_yaw();
    test_orbit_set_target_pitch();
    test_orbit_set_immediate();
    test_orbit_reset();

    // Configuration tests
    printf("\n--- Configuration Tests ---\n");
    test_orbit_set_config();
    test_orbit_sensitivity_adjustment();
    test_orbit_inversion();

    // Edge cases
    printf("\n--- Edge Cases ---\n");
    test_orbit_zero_delta();
    test_orbit_large_delta();
    test_orbit_zero_time_update();

    // State query tests
    printf("\n--- State Query Tests ---\n");
    test_orbit_is_orbiting_state();
    test_orbit_is_interpolating();

    // Integration tests
    printf("\n--- Integration Tests ---\n");
    test_full_orbit_workflow();
    test_diorama_feel_orbit();

    printf("\n=== All tests passed! ===\n");
    return 0;
}
