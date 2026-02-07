/**
 * @file CameraAnimator.h
 * @brief Camera animation controller for smooth transitions.
 *
 * Implements smooth camera transitions:
 * - animateTo(targetPosition, duration) - "go to" feature
 * - Preset snap transitions (N/E/S/W cardinal directions)
 * - Camera shake for disasters
 * - All camera param interpolation (focus, distance, pitch, yaw)
 * - Animation interruption by player input
 *
 * Resource ownership: None (pure logic, no GPU/SDL resources).
 */

#ifndef SIMS3000_INPUT_CAMERAANIMATOR_H
#define SIMS3000_INPUT_CAMERAANIMATOR_H

#include "sims3000/render/CameraState.h"
#include "sims3000/core/Easing.h"
#include <glm/glm.hpp>
#include <cstdint>
#include <functional>

namespace sims3000 {

// ============================================================================
// Animation Configuration
// ============================================================================

/**
 * @struct AnimatorConfig
 * @brief Configuration for camera animator behavior.
 */
struct AnimatorConfig {
    // Preset snap transition
    float presetSnapDuration = 0.4f;              ///< Duration for preset transitions (0.3-0.5s)
    Easing::EasingType presetSnapEasing = Easing::EasingType::EaseInOutCubic;

    // Go-to animation
    float defaultGoToDuration = 0.5f;             ///< Default duration for animateTo
    Easing::EasingType goToEasing = Easing::EasingType::EaseInOutCubic;

    // Camera shake
    float shakeDecay = 5.0f;                      ///< How quickly shake intensity decays
    float shakeFrequency = 25.0f;                 ///< Oscillation frequency (Hz)
    float maxShakeOffset = 0.5f;                  ///< Maximum offset in world units

    /**
     * @brief Get default configuration.
     */
    static AnimatorConfig defaultConfig();
};

// ============================================================================
// Animation Types
// ============================================================================

/**
 * @enum AnimationType
 * @brief Type of animation currently playing.
 */
enum class AnimationType : std::uint8_t {
    None = 0,          ///< No animation
    GoTo,              ///< animateTo position
    PresetSnap,        ///< Snap to preset (N/E/S/W)
    Shake              ///< Camera shake effect
};

/**
 * @struct ShakeState
 * @brief State for camera shake effect.
 */
struct ShakeState {
    bool active = false;           ///< Is shake currently active
    float intensity = 0.0f;        ///< Current shake intensity (0-1)
    float duration = 0.0f;         ///< Total shake duration
    float elapsed = 0.0f;          ///< Time elapsed
    float phase = 0.0f;            ///< Current phase for oscillation
    glm::vec3 offset{0.0f};        ///< Current shake offset (applied to focus)

    /**
     * @brief Reset shake state.
     */
    void reset() {
        active = false;
        intensity = 0.0f;
        duration = 0.0f;
        elapsed = 0.0f;
        phase = 0.0f;
        offset = glm::vec3(0.0f);
    }
};

/**
 * @struct AnimationState
 * @brief Complete state for an animation.
 */
struct AnimationState {
    AnimationType type = AnimationType::None;
    bool active = false;

    // Start values (captured at animation start)
    glm::vec3 startFocusPoint{0.0f};
    float startDistance = 0.0f;
    float startPitch = 0.0f;
    float startYaw = 0.0f;

    // Target values
    glm::vec3 targetFocusPoint{0.0f};
    float targetDistance = 0.0f;
    float targetPitch = 0.0f;
    float targetYaw = 0.0f;

    // Timing
    float duration = 0.0f;
    float elapsed = 0.0f;

    // Easing
    Easing::EasingType easingType = Easing::EasingType::EaseInOutCubic;

    // For preset transitions
    CameraMode targetMode = CameraMode::Free;

    /**
     * @brief Get normalized progress (0-1).
     */
    float getProgress() const {
        if (duration <= 0.0f) return 1.0f;
        return std::min(elapsed / duration, 1.0f);
    }

    /**
     * @brief Check if animation is complete.
     */
    bool isComplete() const {
        return elapsed >= duration;
    }

    /**
     * @brief Reset animation state.
     */
    void reset() {
        type = AnimationType::None;
        active = false;
        elapsed = 0.0f;
    }
};

// ============================================================================
// Camera Animator
// ============================================================================

/**
 * @class CameraAnimator
 * @brief Controls camera animations with smooth transitions.
 *
 * The animator handles:
 * 1. **Go-to animations** - Smooth fly-to for a target position
 * 2. **Preset snap** - Smooth transition to cardinal direction presets
 * 3. **Camera shake** - Trauma-based shake for disasters
 *
 * Usage:
 * @code
 *   CameraAnimator animator;
 *
 *   // Fly to a position
 *   animator.animateTo(cameraState, targetPosition, 0.5f);
 *
 *   // Snap to preset
 *   animator.snapToPreset(cameraState, CameraMode::Preset_E);
 *
 *   // Start shake
 *   animator.startShake(0.5f, 1.0f);  // intensity 0.5, duration 1 second
 *
 *   // In update loop (check for player input to interrupt)
 *   if (playerInputDetected) {
 *       animator.interruptAnimation();
 *   }
 *   animator.update(deltaTime, cameraState);
 * @endcode
 */
class CameraAnimator {
public:
    /**
     * @brief Construct animator with default configuration.
     */
    CameraAnimator();

    /**
     * @brief Construct animator with custom configuration.
     * @param config Animator configuration.
     */
    explicit CameraAnimator(const AnimatorConfig& config);

    ~CameraAnimator() = default;

    // Non-copyable (stateful controller)
    CameraAnimator(const CameraAnimator&) = delete;
    CameraAnimator& operator=(const CameraAnimator&) = delete;

    // Movable
    CameraAnimator(CameraAnimator&&) = default;
    CameraAnimator& operator=(CameraAnimator&&) = default;

    // ========================================================================
    // Animation Commands
    // ========================================================================

    /**
     * @brief Animate camera to target position.
     *
     * Smoothly moves the camera focus to the target position while
     * maintaining current pitch, yaw, and distance.
     *
     * @param cameraState Current camera state (used to capture start values).
     * @param targetPosition Target focus point in world coordinates.
     * @param duration Animation duration in seconds.
     * @param easing Easing function to use (defaults to config setting).
     */
    void animateTo(
        const CameraState& cameraState,
        const glm::vec3& targetPosition,
        float duration = -1.0f,
        Easing::EasingType easing = Easing::EasingType::EaseInOutCubic);

    /**
     * @brief Animate camera to full target state.
     *
     * Smoothly transitions all camera parameters to target values.
     *
     * @param cameraState Current camera state.
     * @param targetFocus Target focus point.
     * @param targetDistance Target camera distance.
     * @param targetPitch Target pitch angle in degrees.
     * @param targetYaw Target yaw angle in degrees.
     * @param duration Animation duration in seconds.
     * @param easing Easing function to use.
     */
    void animateToState(
        const CameraState& cameraState,
        const glm::vec3& targetFocus,
        float targetDistance,
        float targetPitch,
        float targetYaw,
        float duration,
        Easing::EasingType easing = Easing::EasingType::EaseInOutCubic);

    /**
     * @brief Snap to isometric preset with smooth animation.
     *
     * Animates pitch/yaw/distance to match the preset angles.
     * Duration is 0.3-0.5 seconds with ease-in-out.
     *
     * @param cameraState Current camera state.
     * @param preset Target preset mode (Preset_N, Preset_E, Preset_S, Preset_W).
     * @param duration Optional custom duration (default uses config).
     */
    void snapToPreset(
        const CameraState& cameraState,
        CameraMode preset,
        float duration = -1.0f);

    /**
     * @brief Start camera shake effect.
     *
     * Applies a decaying shake to the camera, useful for disasters.
     * Shake does not interrupt other animations but adds on top.
     *
     * @param intensity Shake intensity (0-1, where 1 is maximum shake).
     * @param duration Shake duration in seconds.
     */
    void startShake(float intensity, float duration);

    /**
     * @brief Stop camera shake immediately.
     */
    void stopShake();

    // ========================================================================
    // Animation Control
    // ========================================================================

    /**
     * @brief Interrupt current animation (except shake).
     *
     * Called when player provides pan/zoom/orbit input.
     * The camera will stop at its current interpolated position.
     */
    void interruptAnimation();

    /**
     * @brief Update animations.
     *
     * Progresses animation time and applies interpolated values to camera.
     * Call every frame.
     *
     * @param deltaTime Frame delta time in seconds.
     * @param cameraState Camera state to modify.
     */
    void update(float deltaTime, CameraState& cameraState);

    /**
     * @brief Reset animator state.
     *
     * Stops all animations and resets to idle state.
     */
    void reset();

    // ========================================================================
    // State Query
    // ========================================================================

    /**
     * @brief Check if any animation is in progress (excluding shake).
     * @return true if animating.
     */
    bool isAnimating() const;

    /**
     * @brief Check if camera is currently shaking.
     * @return true if shake is active.
     */
    bool isShaking() const;

    /**
     * @brief Get current animation type.
     * @return Current animation type.
     */
    AnimationType getAnimationType() const { return m_animation.type; }

    /**
     * @brief Get animation progress (0-1).
     * @return Progress through current animation.
     */
    float getAnimationProgress() const { return m_animation.getProgress(); }

    /**
     * @brief Get current shake intensity.
     * @return Current shake intensity (0-1).
     */
    float getShakeIntensity() const { return m_shake.intensity; }

    /**
     * @brief Get current shake offset.
     * @return Shake offset applied to camera focus.
     */
    const glm::vec3& getShakeOffset() const { return m_shake.offset; }

    // ========================================================================
    // Configuration
    // ========================================================================

    /**
     * @brief Get current configuration.
     * @return Current animator configuration.
     */
    const AnimatorConfig& getConfig() const { return m_config; }

    /**
     * @brief Set configuration.
     * @param config New animator configuration.
     */
    void setConfig(const AnimatorConfig& config);

private:
    /**
     * @brief Update main animation (go-to, preset snap).
     */
    void updateAnimation(float deltaTime, CameraState& cameraState);

    /**
     * @brief Update camera shake.
     */
    void updateShake(float deltaTime, CameraState& cameraState);

    /**
     * @brief Interpolate yaw with shortest path wrapping.
     *
     * Handles the 0/360 degree wrap correctly so animations
     * take the shortest path.
     *
     * @param startYaw Starting yaw in degrees.
     * @param targetYaw Target yaw in degrees.
     * @param t Interpolation factor (0-1).
     * @return Interpolated yaw in degrees.
     */
    float interpolateYaw(float startYaw, float targetYaw, float t) const;

    /**
     * @brief Finalize preset transition.
     *
     * Called when preset snap animation completes.
     */
    void finalizePresetTransition(CameraState& cameraState);

    AnimatorConfig m_config;
    AnimationState m_animation;
    ShakeState m_shake;

    // Random number state for shake (simple LCG)
    std::uint32_t m_shakeRng = 12345;

    /**
     * @brief Generate pseudo-random float in [-1, 1].
     */
    float randomNoise();
};

} // namespace sims3000

#endif // SIMS3000_INPUT_CAMERAANIMATOR_H
