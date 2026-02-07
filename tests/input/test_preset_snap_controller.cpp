/**
 * @file test_preset_snap_controller.cpp
 * @brief Unit tests for PresetSnapController (Ticket 2-047).
 *
 * Tests isometric preset snap system:
 * - Q key clockwise rotation (N->E->S->W)
 * - E key counterclockwise rotation (N->W->S->E)
 * - Smooth animation transitions
 * - Preset detection and closest preset calculation
 */

#include "sims3000/input/PresetSnapController.h"
#include "sims3000/input/CameraAnimator.h"
#include "sims3000/input/InputSystem.h"
#include "sims3000/render/CameraState.h"

#include <cassert>
#include <cmath>
#include <iostream>
#include <string>

using namespace sims3000;

// ============================================================================
// Test Utilities
// ============================================================================

static int g_testsPassed = 0;
static int g_testsFailed = 0;

#define TEST(name) \
    void test_##name(); \
    struct test_##name##_registrar { \
        test_##name##_registrar() { \
            std::cout << "Running " #name "... "; \
            try { \
                test_##name(); \
                std::cout << "PASSED\n"; \
                g_testsPassed++; \
            } catch (const std::exception& e) { \
                std::cout << "FAILED: " << e.what() << "\n"; \
                g_testsFailed++; \
            } \
        } \
    } test_##name##_instance; \
    void test_##name()

#define ASSERT_TRUE(cond) \
    if (!(cond)) throw std::runtime_error("Assertion failed: " #cond)

#define ASSERT_FALSE(cond) \
    if (cond) throw std::runtime_error("Assertion failed: NOT " #cond)

#define ASSERT_EQ(a, b) \
    if ((a) != (b)) throw std::runtime_error("Assertion failed: " #a " == " #b)

#define ASSERT_NEAR(a, b, tolerance) \
    if (std::fabs((a) - (b)) > (tolerance)) \
        throw std::runtime_error("Assertion failed: " #a " near " #b)

// ============================================================================
// Construction Tests
// ============================================================================

TEST(DefaultConstruction_SetsPresetN) {
    PresetSnapController controller;

    // Should start at Preset_N
    ASSERT_EQ(controller.getCurrentPreset(), CameraMode::Preset_N);
}

TEST(ConfigConstruction_UsesProvidedConfig) {
    PresetSnapConfig config;
    config.snapDuration = 0.3f;

    PresetSnapController controller(config);

    ASSERT_NEAR(controller.getConfig().snapDuration, 0.3f, 0.001f);
}

// ============================================================================
// Preset Rotation Tests - Clockwise
// ============================================================================

TEST(GetNextClockwise_FromPresetN_ReturnsPresetE) {
    ASSERT_EQ(PresetSnapController::getNextClockwise(CameraMode::Preset_N),
              CameraMode::Preset_E);
}

TEST(GetNextClockwise_FromPresetE_ReturnsPresetS) {
    ASSERT_EQ(PresetSnapController::getNextClockwise(CameraMode::Preset_E),
              CameraMode::Preset_S);
}

TEST(GetNextClockwise_FromPresetS_ReturnsPresetW) {
    ASSERT_EQ(PresetSnapController::getNextClockwise(CameraMode::Preset_S),
              CameraMode::Preset_W);
}

TEST(GetNextClockwise_FromPresetW_ReturnsPresetN) {
    ASSERT_EQ(PresetSnapController::getNextClockwise(CameraMode::Preset_W),
              CameraMode::Preset_N);
}

TEST(GetNextClockwise_FromFree_ReturnsPresetN) {
    // Default behavior for non-preset modes
    ASSERT_EQ(PresetSnapController::getNextClockwise(CameraMode::Free),
              CameraMode::Preset_N);
}

// ============================================================================
// Preset Rotation Tests - Counterclockwise
// ============================================================================

TEST(GetNextCounterclockwise_FromPresetN_ReturnsPresetW) {
    ASSERT_EQ(PresetSnapController::getNextCounterclockwise(CameraMode::Preset_N),
              CameraMode::Preset_W);
}

TEST(GetNextCounterclockwise_FromPresetW_ReturnsPresetS) {
    ASSERT_EQ(PresetSnapController::getNextCounterclockwise(CameraMode::Preset_W),
              CameraMode::Preset_S);
}

TEST(GetNextCounterclockwise_FromPresetS_ReturnsPresetE) {
    ASSERT_EQ(PresetSnapController::getNextCounterclockwise(CameraMode::Preset_S),
              CameraMode::Preset_E);
}

TEST(GetNextCounterclockwise_FromPresetE_ReturnsPresetN) {
    ASSERT_EQ(PresetSnapController::getNextCounterclockwise(CameraMode::Preset_E),
              CameraMode::Preset_N);
}

TEST(GetNextCounterclockwise_FromFree_ReturnsPresetN) {
    // Default behavior for non-preset modes
    ASSERT_EQ(PresetSnapController::getNextCounterclockwise(CameraMode::Free),
              CameraMode::Preset_N);
}

// ============================================================================
// Full Rotation Cycle Tests
// ============================================================================

TEST(ClockwiseRotation_FullCycle_ReturnsToStart) {
    CameraMode current = CameraMode::Preset_N;

    // N -> E -> S -> W -> N
    current = PresetSnapController::getNextClockwise(current);
    ASSERT_EQ(current, CameraMode::Preset_E);

    current = PresetSnapController::getNextClockwise(current);
    ASSERT_EQ(current, CameraMode::Preset_S);

    current = PresetSnapController::getNextClockwise(current);
    ASSERT_EQ(current, CameraMode::Preset_W);

    current = PresetSnapController::getNextClockwise(current);
    ASSERT_EQ(current, CameraMode::Preset_N);
}

TEST(CounterclockwiseRotation_FullCycle_ReturnsToStart) {
    CameraMode current = CameraMode::Preset_N;

    // N -> W -> S -> E -> N
    current = PresetSnapController::getNextCounterclockwise(current);
    ASSERT_EQ(current, CameraMode::Preset_W);

    current = PresetSnapController::getNextCounterclockwise(current);
    ASSERT_EQ(current, CameraMode::Preset_S);

    current = PresetSnapController::getNextCounterclockwise(current);
    ASSERT_EQ(current, CameraMode::Preset_E);

    current = PresetSnapController::getNextCounterclockwise(current);
    ASSERT_EQ(current, CameraMode::Preset_N);
}

// ============================================================================
// Closest Preset Detection Tests
// ============================================================================

TEST(GetClosestPreset_AtExactPresetN_ReturnsPresetN) {
    CameraState state;
    state.yaw = CameraConfig::PRESET_N_YAW;  // 45

    ASSERT_EQ(PresetSnapController::getClosestPreset(state), CameraMode::Preset_N);
}

TEST(GetClosestPreset_AtExactPresetE_ReturnsPresetE) {
    CameraState state;
    state.yaw = CameraConfig::PRESET_E_YAW;  // 135

    ASSERT_EQ(PresetSnapController::getClosestPreset(state), CameraMode::Preset_E);
}

TEST(GetClosestPreset_AtExactPresetS_ReturnsPresetS) {
    CameraState state;
    state.yaw = CameraConfig::PRESET_S_YAW;  // 225

    ASSERT_EQ(PresetSnapController::getClosestPreset(state), CameraMode::Preset_S);
}

TEST(GetClosestPreset_AtExactPresetW_ReturnsPresetW) {
    CameraState state;
    state.yaw = CameraConfig::PRESET_W_YAW;  // 315

    ASSERT_EQ(PresetSnapController::getClosestPreset(state), CameraMode::Preset_W);
}

TEST(GetClosestPreset_NearPresetN_ReturnsPresetN) {
    CameraState state;
    state.yaw = 50.0f;  // Near 45 (Preset_N)

    ASSERT_EQ(PresetSnapController::getClosestPreset(state), CameraMode::Preset_N);
}

TEST(GetClosestPreset_BetweenNAndE_ReturnsCloser) {
    CameraState state;

    // Closer to E (135)
    state.yaw = 100.0f;
    ASSERT_EQ(PresetSnapController::getClosestPreset(state), CameraMode::Preset_E);

    // Closer to N (45)
    state.yaw = 60.0f;
    ASSERT_EQ(PresetSnapController::getClosestPreset(state), CameraMode::Preset_N);
}

TEST(GetClosestPreset_WrapAround_HandlesCorrectly) {
    CameraState state;

    // Near 360/0 should be closer to N (45) or W (315)
    state.yaw = 10.0f;  // Closer to N (45)
    ASSERT_EQ(PresetSnapController::getClosestPreset(state), CameraMode::Preset_N);

    state.yaw = 350.0f;  // Closer to W (315)
    ASSERT_EQ(PresetSnapController::getClosestPreset(state), CameraMode::Preset_W);
}

// ============================================================================
// isInPresetMode Tests
// ============================================================================

TEST(IsInPresetMode_WhenPresetN_ReturnsTrue) {
    CameraState state;
    state.mode = CameraMode::Preset_N;

    ASSERT_TRUE(PresetSnapController::isInPresetMode(state));
}

TEST(IsInPresetMode_WhenPresetE_ReturnsTrue) {
    CameraState state;
    state.mode = CameraMode::Preset_E;

    ASSERT_TRUE(PresetSnapController::isInPresetMode(state));
}

TEST(IsInPresetMode_WhenFree_ReturnsFalse) {
    CameraState state;
    state.mode = CameraMode::Free;

    ASSERT_FALSE(PresetSnapController::isInPresetMode(state));
}

TEST(IsInPresetMode_WhenAnimating_ReturnsFalse) {
    CameraState state;
    state.mode = CameraMode::Animating;

    ASSERT_FALSE(PresetSnapController::isInPresetMode(state));
}

// ============================================================================
// Snap To Preset Tests (Integration with CameraAnimator)
// ============================================================================

TEST(SnapToPreset_UpdatesCurrentPreset) {
    PresetSnapController controller;
    CameraState state;
    CameraAnimator animator;

    // Start at N, snap to E
    controller.snapToPreset(CameraMode::Preset_E, state, animator);

    ASSERT_EQ(controller.getCurrentPreset(), CameraMode::Preset_E);
}

TEST(SnapToPreset_StartsAnimation) {
    PresetSnapController controller;
    CameraState state;
    CameraAnimator animator;

    // Snap to preset E
    controller.snapToPreset(CameraMode::Preset_E, state, animator);

    // Animator should be animating
    ASSERT_TRUE(animator.isAnimating());
}

TEST(SnapClockwise_FromPresetN_AnimatesToPresetE) {
    PresetSnapController controller;
    CameraState state;
    state.mode = CameraMode::Preset_N;
    state.yaw = CameraConfig::PRESET_N_YAW;
    CameraAnimator animator;

    controller.snapClockwise(state, animator);

    // Should target preset E
    ASSERT_EQ(controller.getCurrentPreset(), CameraMode::Preset_E);
    ASSERT_TRUE(animator.isAnimating());
}

TEST(SnapCounterclockwise_FromPresetN_AnimatesToPresetW) {
    PresetSnapController controller;
    CameraState state;
    state.mode = CameraMode::Preset_N;
    state.yaw = CameraConfig::PRESET_N_YAW;
    CameraAnimator animator;

    controller.snapCounterclockwise(state, animator);

    // Should target preset W
    ASSERT_EQ(controller.getCurrentPreset(), CameraMode::Preset_W);
    ASSERT_TRUE(animator.isAnimating());
}

TEST(SnapClockwise_FromFreeMode_UsesClosestPreset) {
    PresetSnapController controller;
    CameraState state;
    state.mode = CameraMode::Free;
    state.yaw = 40.0f;  // Close to N (45)
    CameraAnimator animator;

    controller.snapClockwise(state, animator);

    // Closest is N, so clockwise should go to E
    ASSERT_EQ(controller.getCurrentPreset(), CameraMode::Preset_E);
}

// ============================================================================
// Configuration Tests
// ============================================================================

TEST(SetSnapDuration_ValidDuration_Updates) {
    PresetSnapController controller;

    controller.setSnapDuration(0.5f);

    ASSERT_NEAR(controller.getConfig().snapDuration, 0.5f, 0.001f);
}

TEST(SetSnapDuration_InvalidDuration_Ignored) {
    PresetSnapController controller;
    float originalDuration = controller.getConfig().snapDuration;

    controller.setSnapDuration(-1.0f);

    ASSERT_NEAR(controller.getConfig().snapDuration, originalDuration, 0.001f);
}

TEST(SetConfig_UpdatesAllSettings) {
    PresetSnapController controller;
    PresetSnapConfig config;
    config.snapDuration = 0.3f;

    controller.setConfig(config);

    ASSERT_NEAR(controller.getConfig().snapDuration, 0.3f, 0.001f);
}

// ============================================================================
// Animation Completion Tests
// ============================================================================

TEST(AnimationComplete_SettlesAtExactAngle) {
    PresetSnapController controller;
    CameraState state;
    state.mode = CameraMode::Preset_N;
    state.yaw = CameraConfig::PRESET_N_YAW;
    state.pitch = CameraConfig::ISOMETRIC_PITCH;
    CameraAnimator animator;

    // Snap to preset E
    controller.snapToPreset(CameraMode::Preset_E, state, animator);

    // Simulate animation completion (run update until done)
    float elapsed = 0.0f;
    const float dt = 0.016f;  // 60 FPS
    const float maxTime = 1.0f;  // Should complete well before this

    while (animator.isAnimating() && elapsed < maxTime) {
        animator.update(dt, state);
        elapsed += dt;
    }

    // Animation should have completed
    ASSERT_FALSE(animator.isAnimating());

    // Camera should be at exact preset E angles
    ASSERT_NEAR(state.yaw, CameraConfig::PRESET_E_YAW, 0.01f);
    ASSERT_NEAR(state.pitch, CameraConfig::ISOMETRIC_PITCH, 0.01f);

    // Mode should be Preset_E
    ASSERT_EQ(state.mode, CameraMode::Preset_E);
}

TEST(SnapDuration_WithinAcceptanceCriteria) {
    PresetSnapController controller;

    // Default should be 0.3-0.5 seconds per acceptance criteria
    float duration = controller.getConfig().snapDuration;
    ASSERT_TRUE(duration >= 0.3f && duration <= 0.5f);
}

// ============================================================================
// Preset Angle Verification Tests
// ============================================================================

TEST(PresetAngles_CorrectYawValues) {
    // Verify preset yaw angles are at 45 degree increments
    ASSERT_NEAR(CameraConfig::PRESET_N_YAW, 45.0f, 0.001f);
    ASSERT_NEAR(CameraConfig::PRESET_E_YAW, 135.0f, 0.001f);
    ASSERT_NEAR(CameraConfig::PRESET_S_YAW, 225.0f, 0.001f);
    ASSERT_NEAR(CameraConfig::PRESET_W_YAW, 315.0f, 0.001f);
}

TEST(PresetAngles_IsometricPitch) {
    // Verify isometric pitch is arctan(1/sqrt(2)) = ~35.264 degrees
    ASSERT_NEAR(CameraConfig::ISOMETRIC_PITCH, 35.264f, 0.001f);
}

// ============================================================================
// Default State Tests
// ============================================================================

TEST(DefaultGameStart_IsPresetN) {
    // Per acceptance criteria: default game start is Preset_N
    CameraState state;  // Default constructed

    ASSERT_EQ(state.mode, CameraMode::Preset_N);
    ASSERT_NEAR(state.yaw, CameraConfig::PRESET_N_YAW, 0.001f);
    ASSERT_NEAR(state.pitch, CameraConfig::ISOMETRIC_PITCH, 0.001f);
}

// ============================================================================
// Preset Indicator Tests (Ticket 2-047)
// ============================================================================

TEST(GetCardinalName_PresetN_ReturnsN) {
    ASSERT_EQ(std::string(PresetSnapController::getCardinalName(CameraMode::Preset_N)), "N");
}

TEST(GetCardinalName_PresetE_ReturnsE) {
    ASSERT_EQ(std::string(PresetSnapController::getCardinalName(CameraMode::Preset_E)), "E");
}

TEST(GetCardinalName_PresetS_ReturnsS) {
    ASSERT_EQ(std::string(PresetSnapController::getCardinalName(CameraMode::Preset_S)), "S");
}

TEST(GetCardinalName_PresetW_ReturnsW) {
    ASSERT_EQ(std::string(PresetSnapController::getCardinalName(CameraMode::Preset_W)), "W");
}

TEST(GetCardinalName_Free_ReturnsFree) {
    ASSERT_EQ(std::string(PresetSnapController::getCardinalName(CameraMode::Free)), "Free");
}

TEST(GetCardinalName_Animating_ReturnsFree) {
    ASSERT_EQ(std::string(PresetSnapController::getCardinalName(CameraMode::Animating)), "Free");
}

TEST(GetPresetIndicator_AtPresetN_ReturnsCorrectData) {
    PresetSnapController controller;
    CameraState state;
    state.mode = CameraMode::Preset_N;
    state.yaw = CameraConfig::PRESET_N_YAW;
    CameraAnimator animator;

    PresetIndicator indicator = controller.getPresetIndicator(state, animator);

    ASSERT_EQ(indicator.currentPreset, CameraMode::Preset_N);
    ASSERT_EQ(std::string(indicator.cardinalName), "N");
    ASSERT_NEAR(indicator.yawDegrees, CameraConfig::PRESET_N_YAW, 0.001f);
    ASSERT_FALSE(indicator.isAnimating);
    ASSERT_NEAR(indicator.animationProgress, 1.0f, 0.001f);
}

TEST(GetPresetIndicator_AtPresetE_ReturnsCorrectData) {
    PresetSnapController controller;
    CameraState state;
    state.mode = CameraMode::Preset_E;
    state.yaw = CameraConfig::PRESET_E_YAW;
    CameraAnimator animator;

    PresetIndicator indicator = controller.getPresetIndicator(state, animator);

    ASSERT_EQ(indicator.currentPreset, CameraMode::Preset_E);
    ASSERT_EQ(std::string(indicator.cardinalName), "E");
    ASSERT_NEAR(indicator.yawDegrees, CameraConfig::PRESET_E_YAW, 0.001f);
    ASSERT_FALSE(indicator.isAnimating);
}

TEST(GetPresetIndicator_DuringAnimation_ShowsAnimating) {
    PresetSnapController controller;
    CameraState state;
    state.mode = CameraMode::Preset_N;
    state.yaw = CameraConfig::PRESET_N_YAW;
    state.pitch = CameraConfig::ISOMETRIC_PITCH;
    CameraAnimator animator;

    // Start animation to preset E
    controller.snapToPreset(CameraMode::Preset_E, state, animator);

    // Update partially (not complete)
    animator.update(0.1f, state);

    // Should still be animating
    ASSERT_TRUE(animator.isAnimating());

    PresetIndicator indicator = controller.getPresetIndicator(state, animator);

    ASSERT_EQ(indicator.currentPreset, CameraMode::Preset_E);  // Target preset
    ASSERT_EQ(std::string(indicator.cardinalName), "E");
    ASSERT_TRUE(indicator.isAnimating);
    ASSERT_TRUE(indicator.animationProgress > 0.0f && indicator.animationProgress < 1.0f);
}

TEST(GetPresetIndicator_InFreeMode_UsesClosestPreset) {
    PresetSnapController controller;
    CameraState state;
    state.mode = CameraMode::Free;
    state.yaw = 40.0f;  // Close to N (45)
    CameraAnimator animator;

    PresetIndicator indicator = controller.getPresetIndicator(state, animator);

    ASSERT_EQ(indicator.currentPreset, CameraMode::Preset_N);
    ASSERT_EQ(std::string(indicator.cardinalName), "N");
    ASSERT_FALSE(indicator.isAnimating);
    ASSERT_NEAR(indicator.animationProgress, 0.0f, 0.001f);
}

TEST(GetPresetIndicator_InFreeMode_NearPresetW) {
    PresetSnapController controller;
    CameraState state;
    state.mode = CameraMode::Free;
    state.yaw = 320.0f;  // Close to W (315)
    CameraAnimator animator;

    PresetIndicator indicator = controller.getPresetIndicator(state, animator);

    ASSERT_EQ(indicator.currentPreset, CameraMode::Preset_W);
    ASSERT_EQ(std::string(indicator.cardinalName), "W");
    ASSERT_NEAR(indicator.yawDegrees, 320.0f, 0.001f);
}

TEST(GetPresetIndicator_AfterAnimationComplete_NotAnimating) {
    PresetSnapController controller;
    CameraState state;
    state.mode = CameraMode::Preset_N;
    state.yaw = CameraConfig::PRESET_N_YAW;
    state.pitch = CameraConfig::ISOMETRIC_PITCH;
    CameraAnimator animator;

    // Start animation to preset E
    controller.snapToPreset(CameraMode::Preset_E, state, animator);

    // Run animation to completion
    float elapsed = 0.0f;
    const float dt = 0.016f;
    while (animator.isAnimating() && elapsed < 1.0f) {
        animator.update(dt, state);
        elapsed += dt;
    }

    // Animation should be complete
    ASSERT_FALSE(animator.isAnimating());

    PresetIndicator indicator = controller.getPresetIndicator(state, animator);

    ASSERT_EQ(indicator.currentPreset, CameraMode::Preset_E);
    ASSERT_EQ(std::string(indicator.cardinalName), "E");
    ASSERT_FALSE(indicator.isAnimating);
    ASSERT_NEAR(indicator.animationProgress, 1.0f, 0.001f);
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST(SnapToInvalidPreset_Ignored) {
    PresetSnapController controller;
    CameraState state;
    CameraAnimator animator;

    CameraMode originalPreset = controller.getCurrentPreset();

    // Try to snap to Free (invalid preset)
    controller.snapToPreset(CameraMode::Free, state, animator);

    // Should be unchanged
    ASSERT_EQ(controller.getCurrentPreset(), originalPreset);
    ASSERT_FALSE(animator.isAnimating());
}

TEST(SnapToAnimating_Ignored) {
    PresetSnapController controller;
    CameraState state;
    CameraAnimator animator;

    CameraMode originalPreset = controller.getCurrentPreset();

    // Try to snap to Animating (invalid preset)
    controller.snapToPreset(CameraMode::Animating, state, animator);

    // Should be unchanged
    ASSERT_EQ(controller.getCurrentPreset(), originalPreset);
    ASSERT_FALSE(animator.isAnimating());
}

TEST(MultipleClockwiseSnaps_TraversesAllPresets) {
    PresetSnapController controller;
    CameraState state;
    state.mode = CameraMode::Preset_N;
    state.yaw = CameraConfig::PRESET_N_YAW;
    CameraAnimator animator;

    // First snap: N -> E
    controller.snapClockwise(state, animator);
    ASSERT_EQ(controller.getCurrentPreset(), CameraMode::Preset_E);

    // Complete animation
    while (animator.isAnimating()) {
        animator.update(0.1f, state);
    }

    // Second snap: E -> S
    controller.snapClockwise(state, animator);
    ASSERT_EQ(controller.getCurrentPreset(), CameraMode::Preset_S);

    // Complete animation
    while (animator.isAnimating()) {
        animator.update(0.1f, state);
    }

    // Third snap: S -> W
    controller.snapClockwise(state, animator);
    ASSERT_EQ(controller.getCurrentPreset(), CameraMode::Preset_W);

    // Complete animation
    while (animator.isAnimating()) {
        animator.update(0.1f, state);
    }

    // Fourth snap: W -> N (back to start)
    controller.snapClockwise(state, animator);
    ASSERT_EQ(controller.getCurrentPreset(), CameraMode::Preset_N);
}

// ============================================================================
// Main
// ============================================================================

int main() {
    std::cout << "=== PresetSnapController Tests (Ticket 2-047) ===\n\n";

    // Tests are automatically run by static initializers

    std::cout << "\n=== Results ===\n";
    std::cout << "Passed: " << g_testsPassed << "\n";
    std::cout << "Failed: " << g_testsFailed << "\n";

    return g_testsFailed == 0 ? 0 : 1;
}
