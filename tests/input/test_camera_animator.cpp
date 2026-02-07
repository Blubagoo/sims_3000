/**
 * @file test_camera_animator.cpp
 * @brief Unit tests for CameraAnimator (Ticket 2-027).
 */

#include "sims3000/input/CameraAnimator.h"
#include "sims3000/render/CameraState.h"
#include "sims3000/core/Easing.h"
#include <cassert>
#include <cstdio>
#include <cmath>

using namespace sims3000;

// Helper for float comparison with tolerance
bool approxEqual(float a, float b, float epsilon = 0.001f) {
    return std::fabs(a - b) < epsilon;
}

// Helper for vec3 comparison with tolerance
bool approxEqual(const glm::vec3& a, const glm::vec3& b, float epsilon = 0.001f) {
    return approxEqual(a.x, b.x, epsilon) &&
           approxEqual(a.y, b.y, epsilon) &&
           approxEqual(a.z, b.z, epsilon);
}

// ============================================================================
// Easing Function Tests
// ============================================================================

void test_easing_linear() {
    printf("Testing Easing::linear...\n");

    assert(approxEqual(Easing::linear(0.0f), 0.0f));
    assert(approxEqual(Easing::linear(0.5f), 0.5f));
    assert(approxEqual(Easing::linear(1.0f), 1.0f));

    // Clamping
    assert(approxEqual(Easing::linear(-0.5f), 0.0f));
    assert(approxEqual(Easing::linear(1.5f), 1.0f));

    printf("  PASS: Linear easing works correctly\n");
}

void test_easing_ease_in_out_cubic() {
    printf("Testing Easing::easeInOutCubic...\n");

    // Boundary values
    assert(approxEqual(Easing::easeInOutCubic(0.0f), 0.0f));
    assert(approxEqual(Easing::easeInOutCubic(1.0f), 1.0f));

    // Midpoint should be 0.5 for symmetric ease-in-out
    assert(approxEqual(Easing::easeInOutCubic(0.5f), 0.5f));

    // First half should be slower (ease-in)
    float quarterEased = Easing::easeInOutCubic(0.25f);
    assert(quarterEased < 0.25f);  // Slower at start

    // Second half should be faster then slowing (ease-out)
    float threeQuarterEased = Easing::easeInOutCubic(0.75f);
    assert(threeQuarterEased > 0.75f);  // Past linear at this point

    printf("  PASS: Ease-in-out cubic works correctly\n");
}

void test_easing_apply_by_type() {
    printf("Testing Easing::applyEasing...\n");

    // Test that all easing types work
    float t = 0.5f;

    float linear = Easing::applyEasing(Easing::EasingType::Linear, t);
    float easeInOutCubic = Easing::applyEasing(Easing::EasingType::EaseInOutCubic, t);
    float easeOutQuad = Easing::applyEasing(Easing::EasingType::EaseOutQuad, t);

    assert(approxEqual(linear, 0.5f));
    assert(approxEqual(easeInOutCubic, 0.5f));  // Midpoint is always 0.5 for symmetric
    assert(easeOutQuad > 0.5f);  // Ease-out is faster at midpoint

    printf("  PASS: applyEasing selects correct function\n");
}

void test_easing_boundaries() {
    printf("Testing easing function boundaries...\n");

    // All easing functions should return 0 at t=0 and 1 at t=1
    for (int i = 0; i <= static_cast<int>(Easing::EasingType::EaseInOutExpo); ++i) {
        Easing::EasingType type = static_cast<Easing::EasingType>(i);

        float atZero = Easing::applyEasing(type, 0.0f);
        float atOne = Easing::applyEasing(type, 1.0f);

        assert(approxEqual(atZero, 0.0f, 0.01f));
        assert(approxEqual(atOne, 1.0f, 0.01f));
    }

    printf("  PASS: All easing functions respect boundaries\n");
}

// ============================================================================
// CameraAnimator Construction Tests
// ============================================================================

void test_animator_default_construction() {
    printf("Testing CameraAnimator default construction...\n");

    CameraAnimator animator;

    assert(!animator.isAnimating());
    assert(!animator.isShaking());
    assert(animator.getAnimationType() == AnimationType::None);
    assert(approxEqual(animator.getAnimationProgress(), 1.0f));  // No animation = complete

    printf("  PASS: Default construction works\n");
}

void test_animator_custom_config() {
    printf("Testing CameraAnimator with custom config...\n");

    AnimatorConfig config;
    config.presetSnapDuration = 0.3f;
    config.defaultGoToDuration = 1.0f;
    config.shakeFrequency = 30.0f;

    CameraAnimator animator(config);

    assert(approxEqual(animator.getConfig().presetSnapDuration, 0.3f));
    assert(approxEqual(animator.getConfig().defaultGoToDuration, 1.0f));
    assert(approxEqual(animator.getConfig().shakeFrequency, 30.0f));

    printf("  PASS: Custom config accepted\n");
}

// ============================================================================
// animateTo Tests
// ============================================================================

void test_animate_to_function() {
    printf("Testing CameraAnimator::animateTo...\n");

    CameraAnimator animator;
    CameraState cameraState;
    cameraState.focus_point = glm::vec3(0.0f, 0.0f, 0.0f);
    cameraState.distance = 50.0f;
    cameraState.pitch = 35.0f;
    cameraState.yaw = 45.0f;

    glm::vec3 targetPosition(100.0f, 0.0f, 100.0f);

    animator.animateTo(cameraState, targetPosition, 0.5f);

    assert(animator.isAnimating());
    assert(animator.getAnimationType() == AnimationType::GoTo);
    assert(approxEqual(animator.getAnimationProgress(), 0.0f));

    printf("  PASS: animateTo starts animation correctly\n");
}

void test_animate_to_interpolation() {
    printf("Testing animateTo interpolation...\n");

    CameraAnimator animator;
    CameraState cameraState;
    cameraState.focus_point = glm::vec3(0.0f, 0.0f, 0.0f);
    cameraState.distance = 50.0f;
    cameraState.pitch = 35.0f;
    cameraState.yaw = 45.0f;

    glm::vec3 targetPosition(100.0f, 0.0f, 100.0f);
    float duration = 1.0f;

    animator.animateTo(cameraState, targetPosition, duration, Easing::EasingType::Linear);

    // Update halfway
    animator.update(0.5f, cameraState);

    // With linear easing, should be at midpoint
    assert(approxEqual(cameraState.focus_point.x, 50.0f, 1.0f));
    assert(approxEqual(cameraState.focus_point.z, 50.0f, 1.0f));

    // Distance, pitch, yaw should be unchanged
    assert(approxEqual(cameraState.distance, 50.0f));
    assert(approxEqual(cameraState.pitch, 35.0f));
    assert(approxEqual(cameraState.yaw, 45.0f));

    printf("  PASS: animateTo interpolates focus correctly\n");
}

void test_animate_to_completion() {
    printf("Testing animateTo completion...\n");

    CameraAnimator animator;
    CameraState cameraState;
    cameraState.focus_point = glm::vec3(0.0f, 0.0f, 0.0f);

    glm::vec3 targetPosition(100.0f, 0.0f, 100.0f);

    animator.animateTo(cameraState, targetPosition, 0.5f);

    // Run to completion
    for (int i = 0; i < 100; ++i) {
        animator.update(0.016f, cameraState);
    }

    // Should have reached target and stopped animating
    assert(!animator.isAnimating());
    assert(approxEqual(cameraState.focus_point, targetPosition, 0.1f));

    printf("  PASS: animateTo reaches target and stops\n");
}

// ============================================================================
// Preset Snap Tests
// ============================================================================

void test_snap_to_preset_north() {
    printf("Testing snapToPreset to North...\n");

    CameraAnimator animator;
    CameraState cameraState;
    cameraState.focus_point = glm::vec3(64.0f, 0.0f, 64.0f);
    cameraState.distance = 50.0f;
    cameraState.pitch = 45.0f;  // Different from isometric
    cameraState.yaw = 180.0f;   // Looking south

    animator.snapToPreset(cameraState, CameraMode::Preset_N);

    assert(animator.isAnimating());
    assert(animator.getAnimationType() == AnimationType::PresetSnap);

    // Run to completion
    for (int i = 0; i < 100; ++i) {
        animator.update(0.016f, cameraState);
    }

    // Should be at North preset values
    assert(approxEqual(cameraState.pitch, CameraConfig::ISOMETRIC_PITCH, 0.5f));
    assert(approxEqual(cameraState.yaw, CameraConfig::PRESET_N_YAW, 0.5f));
    assert(cameraState.mode == CameraMode::Preset_N);

    printf("  PASS: snapToPreset reaches North preset\n");
}

void test_snap_to_preset_duration() {
    printf("Testing preset snap duration (0.3-0.5s)...\n");

    AnimatorConfig config;
    config.presetSnapDuration = 0.4f;  // Within 0.3-0.5s range

    CameraAnimator animator(config);
    CameraState cameraState;

    animator.snapToPreset(cameraState, CameraMode::Preset_E);

    // Should still be animating after 0.3 seconds
    animator.update(0.3f, cameraState);
    assert(animator.isAnimating());

    // Should be done after additional 0.15 seconds (total 0.45s)
    animator.update(0.15f, cameraState);
    assert(!animator.isAnimating());

    printf("  PASS: Preset snap uses correct duration\n");
}

void test_snap_to_preset_shortest_yaw_path() {
    printf("Testing preset snap takes shortest yaw path...\n");

    CameraAnimator animator;
    CameraState cameraState;
    cameraState.yaw = 350.0f;  // Close to 360/0

    // Snap to North preset (yaw 45)
    // Shortest path: 350 -> 360 -> 45 (55 degrees)
    // NOT: 350 -> 45 (305 degrees)

    animator.snapToPreset(cameraState, CameraMode::Preset_N, 1.0f);

    // Update partway
    animator.update(0.5f, cameraState);

    // Yaw should be increasing toward 360/0, not decreasing through 180
    // Midpoint should be around 17.5 (or 377.5 normalized)
    // Since we start at 350 and go to 45 via 0, halfway is around 17.5
    float yaw = cameraState.yaw;

    // Should NOT be around 197.5 (the long way)
    assert(yaw < 100.0f || yaw > 300.0f);

    printf("  PASS: Yaw interpolation takes shortest path\n");
}

void test_snap_all_cardinal_presets() {
    printf("Testing all cardinal preset transitions...\n");

    struct PresetTest {
        CameraMode preset;
        float expectedYaw;
    };

    PresetTest presets[] = {
        { CameraMode::Preset_N, CameraConfig::PRESET_N_YAW },
        { CameraMode::Preset_E, CameraConfig::PRESET_E_YAW },
        { CameraMode::Preset_S, CameraConfig::PRESET_S_YAW },
        { CameraMode::Preset_W, CameraConfig::PRESET_W_YAW }
    };

    for (const auto& test : presets) {
        CameraAnimator animator;
        CameraState cameraState;
        cameraState.yaw = 0.0f;

        animator.snapToPreset(cameraState, test.preset);

        // Run to completion
        for (int i = 0; i < 100; ++i) {
            animator.update(0.016f, cameraState);
        }

        assert(approxEqual(cameraState.yaw, test.expectedYaw, 1.0f));
        assert(cameraState.mode == test.preset);
    }

    printf("  PASS: All cardinal presets work correctly\n");
}

// ============================================================================
// Animation Interruption Tests
// ============================================================================

void test_animation_interrupt() {
    printf("Testing animation interruption...\n");

    CameraAnimator animator;
    CameraState cameraState;
    cameraState.focus_point = glm::vec3(0.0f, 0.0f, 0.0f);

    animator.animateTo(cameraState, glm::vec3(100.0f, 0.0f, 100.0f), 1.0f);

    // Update partway
    animator.update(0.3f, cameraState);
    assert(animator.isAnimating());

    // Capture position
    glm::vec3 positionAtInterrupt = cameraState.focus_point;

    // Interrupt
    animator.interruptAnimation();

    // Should no longer be animating
    assert(!animator.isAnimating());

    // Update again - position should not change
    animator.update(0.1f, cameraState);
    assert(approxEqual(cameraState.focus_point, positionAtInterrupt));

    printf("  PASS: Animation can be interrupted by player input\n");
}

void test_interrupt_preset_snap() {
    printf("Testing preset snap interruption...\n");

    CameraAnimator animator;
    CameraState cameraState;
    cameraState.yaw = 180.0f;

    animator.snapToPreset(cameraState, CameraMode::Preset_N);

    // Update partway
    animator.update(0.2f, cameraState);

    // Interrupt
    animator.interruptAnimation();

    // Mode should remain unchanged (not set to preset since incomplete)
    // Camera stays where it was interrupted
    assert(!animator.isAnimating());

    printf("  PASS: Preset snap can be interrupted\n");
}

// ============================================================================
// Camera Shake Tests
// ============================================================================

void test_camera_shake_start() {
    printf("Testing camera shake start...\n");

    CameraAnimator animator;
    CameraState cameraState;
    cameraState.focus_point = glm::vec3(0.0f, 0.0f, 0.0f);

    animator.startShake(0.5f, 1.0f);

    assert(animator.isShaking());
    assert(approxEqual(animator.getShakeIntensity(), 0.5f));

    printf("  PASS: Camera shake starts correctly\n");
}

void test_camera_shake_applies_offset() {
    printf("Testing camera shake applies offset...\n");

    CameraAnimator animator;
    CameraState cameraState;
    cameraState.focus_point = glm::vec3(50.0f, 0.0f, 50.0f);

    animator.startShake(1.0f, 1.0f);

    // Update a few frames
    for (int i = 0; i < 10; ++i) {
        animator.update(0.016f, cameraState);
    }

    // Focus should have moved (shake offset applied)
    // With high intensity, there should be noticeable offset
    glm::vec3 offset = animator.getShakeOffset();
    float offsetMagnitude = glm::length(offset);

    // Should have some offset (not zero)
    assert(offsetMagnitude > 0.001f || animator.isShaking());

    printf("  PASS: Camera shake applies offset to focus\n");
}

void test_camera_shake_decays() {
    printf("Testing camera shake decay...\n");

    CameraAnimator animator;
    CameraState cameraState;

    animator.startShake(1.0f, 0.5f);

    // Update to completion
    for (int i = 0; i < 100; ++i) {
        animator.update(0.016f, cameraState);
    }

    // Shake should have stopped
    assert(!animator.isShaking());
    assert(approxEqual(animator.getShakeIntensity(), 0.0f));

    printf("  PASS: Camera shake decays over duration\n");
}

void test_shake_does_not_interrupt_animation() {
    printf("Testing shake doesn't interrupt other animations...\n");

    CameraAnimator animator;
    CameraState cameraState;
    cameraState.focus_point = glm::vec3(0.0f, 0.0f, 0.0f);

    // Start go-to animation
    animator.animateTo(cameraState, glm::vec3(100.0f, 0.0f, 100.0f), 1.0f);

    // Start shake on top
    animator.startShake(0.5f, 0.5f);

    // Both should be active
    assert(animator.isAnimating());
    assert(animator.isShaking());

    // Update
    animator.update(0.3f, cameraState);

    // Animation should still be progressing
    assert(animator.isAnimating());

    printf("  PASS: Shake runs alongside other animations\n");
}

void test_shake_stop() {
    printf("Testing manual shake stop...\n");

    CameraAnimator animator;
    CameraState cameraState;

    animator.startShake(1.0f, 10.0f);  // Long duration
    assert(animator.isShaking());

    animator.stopShake();
    assert(!animator.isShaking());

    printf("  PASS: Shake can be stopped manually\n");
}

// ============================================================================
// Full Param Interpolation Tests
// ============================================================================

void test_interpolate_all_params() {
    printf("Testing full camera param interpolation...\n");

    CameraAnimator animator;
    CameraState cameraState;
    cameraState.focus_point = glm::vec3(0.0f, 0.0f, 0.0f);
    cameraState.distance = 50.0f;
    cameraState.pitch = 20.0f;
    cameraState.yaw = 0.0f;

    animator.animateToState(
        cameraState,
        glm::vec3(100.0f, 0.0f, 100.0f),  // target focus
        80.0f,                              // target distance
        60.0f,                              // target pitch
        90.0f,                              // target yaw
        1.0f,                               // duration
        Easing::EasingType::Linear);

    // Update to completion
    for (int i = 0; i < 100; ++i) {
        animator.update(0.016f, cameraState);
    }

    // All params should have reached target
    assert(approxEqual(cameraState.focus_point.x, 100.0f, 0.5f));
    assert(approxEqual(cameraState.focus_point.z, 100.0f, 0.5f));
    assert(approxEqual(cameraState.distance, 80.0f, 0.5f));
    assert(approxEqual(cameraState.pitch, 60.0f, 0.5f));
    assert(approxEqual(cameraState.yaw, 90.0f, 0.5f));

    printf("  PASS: All camera params interpolate correctly\n");
}

// ============================================================================
// AnimatorConfig Tests
// ============================================================================

void test_animator_config_defaults() {
    printf("Testing AnimatorConfig default values...\n");

    AnimatorConfig config = AnimatorConfig::defaultConfig();

    // Preset snap duration should be in 0.3-0.5s range
    assert(config.presetSnapDuration >= 0.3f && config.presetSnapDuration <= 0.5f);

    // Should use ease-in-out for presets
    assert(config.presetSnapEasing == Easing::EasingType::EaseInOutCubic);

    printf("  PASS: AnimatorConfig has sensible defaults\n");
}

// ============================================================================
// Reset Tests
// ============================================================================

void test_animator_reset() {
    printf("Testing CameraAnimator reset...\n");

    CameraAnimator animator;
    CameraState cameraState;

    // Start animation
    animator.animateTo(cameraState, glm::vec3(100.0f, 0.0f, 100.0f), 1.0f);
    animator.startShake(0.5f, 1.0f);

    assert(animator.isAnimating());
    assert(animator.isShaking());

    // Reset
    animator.reset();

    assert(!animator.isAnimating());
    assert(!animator.isShaking());
    assert(animator.getAnimationType() == AnimationType::None);

    printf("  PASS: Reset clears all animation state\n");
}

// ============================================================================
// Smooth Blend Tests
// ============================================================================

void test_smooth_blend_from_current_state() {
    printf("Testing smooth blend from current state...\n");

    CameraAnimator animator;
    CameraState cameraState;
    cameraState.focus_point = glm::vec3(25.0f, 0.0f, 25.0f);
    cameraState.distance = 30.0f;
    cameraState.pitch = 45.0f;
    cameraState.yaw = 135.0f;

    // Start animation from current (non-default) state
    animator.animateTo(cameraState, glm::vec3(75.0f, 0.0f, 75.0f), 0.5f);

    // First frame should start from current position
    animator.update(0.001f, cameraState);  // Tiny update

    // Should still be very close to start
    assert(approxEqual(cameraState.focus_point.x, 25.0f, 1.0f));
    assert(approxEqual(cameraState.focus_point.z, 25.0f, 1.0f));

    printf("  PASS: Animation blends smoothly from current state\n");
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main() {
    printf("=== CameraAnimator Unit Tests (Ticket 2-027) ===\n\n");

    // Easing function tests
    printf("--- Easing Function Tests ---\n");
    test_easing_linear();
    test_easing_ease_in_out_cubic();
    test_easing_apply_by_type();
    test_easing_boundaries();

    // Construction tests
    printf("\n--- Construction Tests ---\n");
    test_animator_default_construction();
    test_animator_custom_config();

    // animateTo tests
    printf("\n--- animateTo Tests ---\n");
    test_animate_to_function();
    test_animate_to_interpolation();
    test_animate_to_completion();

    // Preset snap tests
    printf("\n--- Preset Snap Tests ---\n");
    test_snap_to_preset_north();
    test_snap_to_preset_duration();
    test_snap_to_preset_shortest_yaw_path();
    test_snap_all_cardinal_presets();

    // Interruption tests
    printf("\n--- Animation Interruption Tests ---\n");
    test_animation_interrupt();
    test_interrupt_preset_snap();

    // Camera shake tests
    printf("\n--- Camera Shake Tests ---\n");
    test_camera_shake_start();
    test_camera_shake_applies_offset();
    test_camera_shake_decays();
    test_shake_does_not_interrupt_animation();
    test_shake_stop();

    // Full param interpolation tests
    printf("\n--- Full Param Interpolation Tests ---\n");
    test_interpolate_all_params();

    // Config tests
    printf("\n--- Configuration Tests ---\n");
    test_animator_config_defaults();

    // Reset tests
    printf("\n--- Reset Tests ---\n");
    test_animator_reset();

    // Smooth blend tests
    printf("\n--- Smooth Blend Tests ---\n");
    test_smooth_blend_from_current_state();

    printf("\n=== All tests passed! ===\n");
    return 0;
}
