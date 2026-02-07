/**
 * @file test_camera_state.cpp
 * @brief Unit tests for CameraState struct and CameraConfig constants.
 */

#include "sims3000/render/CameraState.h"
#include <cassert>
#include <cstdio>
#include <cmath>

using namespace sims3000;

// Helper for float comparison with tolerance
bool approxEqual(float a, float b, float epsilon = 0.0001f) {
    return std::fabs(a - b) < epsilon;
}

// ============================================================================
// CameraConfig Tests
// ============================================================================

void test_camera_config_pitch_constraints() {
    printf("Testing CameraConfig pitch constraints...\n");

    // Pitch must be clamped between 15 and 80 degrees
    assert(CameraConfig::PITCH_MIN == 15.0f);
    assert(CameraConfig::PITCH_MAX == 80.0f);
    assert(CameraConfig::PITCH_MIN < CameraConfig::PITCH_MAX);

    printf("  PASS: Pitch constraints defined correctly (15-80 degrees)\n");
}

void test_camera_config_distance_constraints() {
    printf("Testing CameraConfig distance constraints...\n");

    // Distance/zoom must be clamped between 5 and 100 units
    assert(CameraConfig::DISTANCE_MIN == 5.0f);
    assert(CameraConfig::DISTANCE_MAX == 100.0f);
    assert(CameraConfig::DISTANCE_MIN < CameraConfig::DISTANCE_MAX);
    assert(CameraConfig::DISTANCE_DEFAULT >= CameraConfig::DISTANCE_MIN);
    assert(CameraConfig::DISTANCE_DEFAULT <= CameraConfig::DISTANCE_MAX);

    printf("  PASS: Distance constraints defined correctly (5-100 units)\n");
}

void test_camera_config_isometric_pitch() {
    printf("Testing CameraConfig isometric pitch...\n");

    // Isometric pitch is arctan(1/sqrt(2)) which is approximately 35.264 degrees
    // This angle creates the "true isometric" projection
    assert(approxEqual(CameraConfig::ISOMETRIC_PITCH, 35.264f, 0.001f));

    printf("  PASS: Isometric pitch is ~35.264 degrees\n");
}

void test_camera_config_preset_yaw_values() {
    printf("Testing CameraConfig preset yaw values...\n");

    // Preset yaw values at 90-degree intervals starting at 45
    assert(CameraConfig::PRESET_N_YAW == 45.0f);
    assert(CameraConfig::PRESET_E_YAW == 135.0f);
    assert(CameraConfig::PRESET_S_YAW == 225.0f);
    assert(CameraConfig::PRESET_W_YAW == 315.0f);

    // Verify 90-degree spacing
    assert(approxEqual(CameraConfig::PRESET_E_YAW - CameraConfig::PRESET_N_YAW, 90.0f));
    assert(approxEqual(CameraConfig::PRESET_S_YAW - CameraConfig::PRESET_E_YAW, 90.0f));
    assert(approxEqual(CameraConfig::PRESET_W_YAW - CameraConfig::PRESET_S_YAW, 90.0f));

    printf("  PASS: Preset yaw values at 45/135/225/315 degrees\n");
}

void test_camera_config_yaw_boundaries() {
    printf("Testing CameraConfig yaw boundaries...\n");

    // Yaw wraps 0-360
    assert(CameraConfig::YAW_MIN == 0.0f);
    assert(CameraConfig::YAW_MAX == 360.0f);

    printf("  PASS: Yaw boundaries 0-360 degrees\n");
}

// ============================================================================
// CameraMode Enum Tests
// ============================================================================

void test_camera_mode_enum_values() {
    printf("Testing CameraMode enum values...\n");

    // Verify all enum values exist
    assert(static_cast<int>(CameraMode::Free) == 0);
    assert(static_cast<int>(CameraMode::Preset_N) == 1);
    assert(static_cast<int>(CameraMode::Preset_E) == 2);
    assert(static_cast<int>(CameraMode::Preset_S) == 3);
    assert(static_cast<int>(CameraMode::Preset_W) == 4);
    assert(static_cast<int>(CameraMode::Animating) == 5);

    printf("  PASS: All CameraMode enum values defined\n");
}

void test_camera_mode_size() {
    printf("Testing CameraMode size...\n");

    // CameraMode should be 1 byte
    assert(sizeof(CameraMode) == 1);

    printf("  PASS: CameraMode is 1 byte\n");
}

// ============================================================================
// TransitionState Tests
// ============================================================================

void test_transition_state_default() {
    printf("Testing TransitionState default values...\n");

    TransitionState ts;

    assert(ts.active == false);
    assert(ts.elapsed_time == 0.0f);
    assert(approxEqual(ts.duration, CameraConfig::TRANSITION_DURATION_SEC));

    printf("  PASS: TransitionState defaults correct\n");
}

void test_transition_state_alpha() {
    printf("Testing TransitionState getAlpha...\n");

    TransitionState ts;
    ts.duration = 1.0f;

    // Alpha at start
    ts.elapsed_time = 0.0f;
    assert(approxEqual(ts.getAlpha(), 0.0f));

    // Alpha at midpoint
    ts.elapsed_time = 0.5f;
    assert(approxEqual(ts.getAlpha(), 0.5f));

    // Alpha at end
    ts.elapsed_time = 1.0f;
    assert(approxEqual(ts.getAlpha(), 1.0f));

    // Alpha clamped above 1
    ts.elapsed_time = 2.0f;
    assert(approxEqual(ts.getAlpha(), 1.0f));

    // Alpha clamped below 0
    ts.elapsed_time = -1.0f;
    assert(approxEqual(ts.getAlpha(), 0.0f));

    // Handle zero duration
    ts.duration = 0.0f;
    ts.elapsed_time = 0.0f;
    assert(approxEqual(ts.getAlpha(), 1.0f)); // Should return 1.0 for instant transition

    printf("  PASS: TransitionState getAlpha works correctly\n");
}

void test_transition_state_complete() {
    printf("Testing TransitionState isComplete...\n");

    TransitionState ts;
    ts.duration = 1.0f;

    ts.elapsed_time = 0.5f;
    assert(!ts.isComplete());

    ts.elapsed_time = 1.0f;
    assert(ts.isComplete());

    ts.elapsed_time = 1.5f;
    assert(ts.isComplete());

    printf("  PASS: TransitionState isComplete works correctly\n");
}

void test_transition_state_reset() {
    printf("Testing TransitionState reset...\n");

    TransitionState ts;
    ts.active = true;
    ts.elapsed_time = 0.75f;

    ts.reset();

    assert(ts.active == false);
    assert(ts.elapsed_time == 0.0f);

    printf("  PASS: TransitionState reset works correctly\n");
}

// ============================================================================
// CameraState Tests
// ============================================================================

void test_camera_state_default_values() {
    printf("Testing CameraState default values...\n");

    CameraState cs;

    // Default mode is Preset_N (per acceptance criteria)
    assert(cs.mode == CameraMode::Preset_N);

    // Default yaw is 45 degrees (Preset_N)
    assert(approxEqual(cs.yaw, CameraConfig::PRESET_N_YAW));

    // Default pitch is isometric (~35.264)
    assert(approxEqual(cs.pitch, CameraConfig::ISOMETRIC_PITCH));

    // Default distance is 50 units
    assert(approxEqual(cs.distance, CameraConfig::DISTANCE_DEFAULT));

    // Default focus point is origin
    assert(approxEqual(cs.focus_point.x, 0.0f));
    assert(approxEqual(cs.focus_point.y, 0.0f));
    assert(approxEqual(cs.focus_point.z, 0.0f));

    printf("  PASS: CameraState defaults to Preset_N (yaw 45, pitch ~35.264)\n");
}

void test_camera_state_pitch_clamping() {
    printf("Testing CameraState pitch clamping...\n");

    CameraState cs;

    // Test clamping below minimum
    cs.pitch = 10.0f; // Below 15
    cs.clampPitch();
    assert(approxEqual(cs.pitch, CameraConfig::PITCH_MIN));

    // Test clamping above maximum
    cs.pitch = 90.0f; // Above 80
    cs.clampPitch();
    assert(approxEqual(cs.pitch, CameraConfig::PITCH_MAX));

    // Test value within range (no change)
    cs.pitch = 45.0f;
    cs.clampPitch();
    assert(approxEqual(cs.pitch, 45.0f));

    // Test boundary values
    cs.pitch = 15.0f;
    cs.clampPitch();
    assert(approxEqual(cs.pitch, 15.0f));

    cs.pitch = 80.0f;
    cs.clampPitch();
    assert(approxEqual(cs.pitch, 80.0f));

    printf("  PASS: Pitch clamped to 15-80 degrees\n");
}

void test_camera_state_yaw_wrapping() {
    printf("Testing CameraState yaw wrapping...\n");

    CameraState cs;

    // Test wrapping above 360
    cs.yaw = 370.0f;
    cs.wrapYaw();
    assert(approxEqual(cs.yaw, 10.0f));

    // Test wrapping below 0
    cs.yaw = -30.0f;
    cs.wrapYaw();
    assert(approxEqual(cs.yaw, 330.0f));

    // Test value within range (no change)
    cs.yaw = 180.0f;
    cs.wrapYaw();
    assert(approxEqual(cs.yaw, 180.0f));

    // Test boundary at 0
    cs.yaw = 0.0f;
    cs.wrapYaw();
    assert(approxEqual(cs.yaw, 0.0f));

    // Test boundary at 360 (should wrap to 0)
    cs.yaw = 360.0f;
    cs.wrapYaw();
    assert(approxEqual(cs.yaw, 0.0f));

    // Test large negative value
    cs.yaw = -400.0f;
    cs.wrapYaw();
    assert(cs.yaw >= 0.0f && cs.yaw < 360.0f);

    // Test large positive value
    cs.yaw = 800.0f;
    cs.wrapYaw();
    assert(cs.yaw >= 0.0f && cs.yaw < 360.0f);

    printf("  PASS: Yaw wraps correctly 0-360 degrees\n");
}

void test_camera_state_distance_clamping() {
    printf("Testing CameraState distance clamping (zoom)...\n");

    CameraState cs;

    // Test clamping below minimum
    cs.distance = 2.0f; // Below 5
    cs.clampDistance();
    assert(approxEqual(cs.distance, CameraConfig::DISTANCE_MIN));

    // Test clamping above maximum
    cs.distance = 150.0f; // Above 100
    cs.clampDistance();
    assert(approxEqual(cs.distance, CameraConfig::DISTANCE_MAX));

    // Test value within range (no change)
    cs.distance = 50.0f;
    cs.clampDistance();
    assert(approxEqual(cs.distance, 50.0f));

    // Test boundary values
    cs.distance = 5.0f;
    cs.clampDistance();
    assert(approxEqual(cs.distance, 5.0f));

    cs.distance = 100.0f;
    cs.clampDistance();
    assert(approxEqual(cs.distance, 100.0f));

    printf("  PASS: Distance (zoom) clamped to 5-100 units\n");
}

void test_camera_state_apply_constraints() {
    printf("Testing CameraState applyConstraints...\n");

    CameraState cs;

    // Set all values out of range
    cs.pitch = 5.0f;    // Below 15
    cs.yaw = 400.0f;    // Above 360
    cs.distance = 0.5f; // Below 5

    cs.applyConstraints();

    assert(approxEqual(cs.pitch, CameraConfig::PITCH_MIN));
    assert(cs.yaw >= 0.0f && cs.yaw < 360.0f);
    assert(approxEqual(cs.distance, CameraConfig::DISTANCE_MIN));

    printf("  PASS: applyConstraints enforces all limits\n");
}

void test_camera_state_preset_pitch_lookup() {
    printf("Testing CameraState getPitchForPreset...\n");

    // All presets should use isometric pitch
    assert(approxEqual(CameraState::getPitchForPreset(CameraMode::Preset_N), CameraConfig::ISOMETRIC_PITCH));
    assert(approxEqual(CameraState::getPitchForPreset(CameraMode::Preset_E), CameraConfig::ISOMETRIC_PITCH));
    assert(approxEqual(CameraState::getPitchForPreset(CameraMode::Preset_S), CameraConfig::ISOMETRIC_PITCH));
    assert(approxEqual(CameraState::getPitchForPreset(CameraMode::Preset_W), CameraConfig::ISOMETRIC_PITCH));

    printf("  PASS: All presets use isometric pitch (~35.264)\n");
}

void test_camera_state_preset_yaw_lookup() {
    printf("Testing CameraState getYawForPreset...\n");

    assert(approxEqual(CameraState::getYawForPreset(CameraMode::Preset_N), 45.0f));
    assert(approxEqual(CameraState::getYawForPreset(CameraMode::Preset_E), 135.0f));
    assert(approxEqual(CameraState::getYawForPreset(CameraMode::Preset_S), 225.0f));
    assert(approxEqual(CameraState::getYawForPreset(CameraMode::Preset_W), 315.0f));

    printf("  PASS: Preset yaw values match specification\n");
}

void test_camera_state_is_preset_mode() {
    printf("Testing CameraState isPresetMode...\n");

    CameraState cs;

    cs.mode = CameraMode::Free;
    assert(!cs.isPresetMode());

    cs.mode = CameraMode::Animating;
    assert(!cs.isPresetMode());

    cs.mode = CameraMode::Preset_N;
    assert(cs.isPresetMode());

    cs.mode = CameraMode::Preset_E;
    assert(cs.isPresetMode());

    cs.mode = CameraMode::Preset_S;
    assert(cs.isPresetMode());

    cs.mode = CameraMode::Preset_W;
    assert(cs.isPresetMode());

    printf("  PASS: isPresetMode correctly identifies preset modes\n");
}

void test_camera_state_is_animating() {
    printf("Testing CameraState isAnimating...\n");

    CameraState cs;

    // Not animating by default
    assert(!cs.isAnimating());

    // Set mode to Animating but transition not active
    cs.mode = CameraMode::Animating;
    cs.transition.active = false;
    assert(!cs.isAnimating());

    // Set transition active
    cs.transition.active = true;
    assert(cs.isAnimating());

    // Different mode with active transition (should not be animating)
    cs.mode = CameraMode::Free;
    cs.transition.active = true;
    assert(!cs.isAnimating());

    printf("  PASS: isAnimating works correctly\n");
}

void test_camera_state_start_transition() {
    printf("Testing CameraState startTransition...\n");

    CameraState cs;
    cs.focus_point = glm::vec3(10.0f, 20.0f, 0.0f);
    cs.distance = 40.0f;
    cs.pitch = 45.0f;
    cs.yaw = 90.0f;
    cs.mode = CameraMode::Free;

    // Start transition to Preset_E
    cs.startTransition(CameraMode::Preset_E, 0.75f);

    // Mode should be Animating
    assert(cs.mode == CameraMode::Animating);

    // Transition should be active
    assert(cs.transition.active);

    // Start values should capture current state
    assert(cs.transition.start_focus_point == glm::vec3(10.0f, 20.0f, 0.0f));
    assert(approxEqual(cs.transition.start_distance, 40.0f));
    assert(approxEqual(cs.transition.start_pitch, 45.0f));
    assert(approxEqual(cs.transition.start_yaw, 90.0f));

    // Target values should be preset values
    assert(approxEqual(cs.transition.target_pitch, CameraConfig::ISOMETRIC_PITCH));
    assert(approxEqual(cs.transition.target_yaw, CameraConfig::PRESET_E_YAW));

    // Target mode set correctly
    assert(cs.transition.target_mode == CameraMode::Preset_E);

    // Duration set correctly
    assert(approxEqual(cs.transition.duration, 0.75f));

    // Elapsed time starts at 0
    assert(approxEqual(cs.transition.elapsed_time, 0.0f));

    printf("  PASS: startTransition captures state and sets targets\n");
}

void test_camera_state_start_transition_to_free() {
    printf("Testing CameraState startTransition to Free mode...\n");

    CameraState cs;
    cs.pitch = 30.0f;
    cs.yaw = 180.0f;
    cs.mode = CameraMode::Preset_S;

    cs.startTransition(CameraMode::Free);

    // Target angles should be preserved (current values)
    assert(approxEqual(cs.transition.target_pitch, 30.0f));
    assert(approxEqual(cs.transition.target_yaw, 180.0f));
    assert(cs.transition.target_mode == CameraMode::Free);

    printf("  PASS: Transition to Free preserves current angles\n");
}

void test_camera_state_reset_to_default() {
    printf("Testing CameraState resetToDefault...\n");

    CameraState cs;

    // Change all values
    cs.focus_point = glm::vec3(100.0f, 100.0f, 50.0f);
    cs.distance = 80.0f;
    cs.pitch = 60.0f;
    cs.yaw = 270.0f;
    cs.mode = CameraMode::Free;
    cs.transition.active = true;
    cs.transition.elapsed_time = 0.5f;

    // Reset
    cs.resetToDefault();

    // Verify defaults restored
    assert(cs.focus_point == glm::vec3(0.0f));
    assert(approxEqual(cs.distance, CameraConfig::DISTANCE_DEFAULT));
    assert(approxEqual(cs.pitch, CameraConfig::ISOMETRIC_PITCH));
    assert(approxEqual(cs.yaw, CameraConfig::PRESET_N_YAW));
    assert(cs.mode == CameraMode::Preset_N);
    assert(!cs.transition.active);
    assert(approxEqual(cs.transition.elapsed_time, 0.0f));

    printf("  PASS: resetToDefault restores Preset_N state\n");
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main() {
    printf("=== CameraState Unit Tests ===\n\n");

    // CameraConfig tests
    printf("--- CameraConfig Tests ---\n");
    test_camera_config_pitch_constraints();
    test_camera_config_distance_constraints();
    test_camera_config_isometric_pitch();
    test_camera_config_preset_yaw_values();
    test_camera_config_yaw_boundaries();

    // CameraMode tests
    printf("\n--- CameraMode Tests ---\n");
    test_camera_mode_enum_values();
    test_camera_mode_size();

    // TransitionState tests
    printf("\n--- TransitionState Tests ---\n");
    test_transition_state_default();
    test_transition_state_alpha();
    test_transition_state_complete();
    test_transition_state_reset();

    // CameraState tests
    printf("\n--- CameraState Tests ---\n");
    test_camera_state_default_values();
    test_camera_state_pitch_clamping();
    test_camera_state_yaw_wrapping();
    test_camera_state_distance_clamping();
    test_camera_state_apply_constraints();
    test_camera_state_preset_pitch_lookup();
    test_camera_state_preset_yaw_lookup();
    test_camera_state_is_preset_mode();
    test_camera_state_is_animating();
    test_camera_state_start_transition();
    test_camera_state_start_transition_to_free();
    test_camera_state_reset_to_default();

    printf("\n=== All tests passed! ===\n");
    return 0;
}
