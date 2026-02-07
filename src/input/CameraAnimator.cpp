/**
 * @file CameraAnimator.cpp
 * @brief Camera animation controller implementation.
 */

#include "sims3000/input/CameraAnimator.h"
#include <cmath>
#include <algorithm>

namespace sims3000 {

// ============================================================================
// AnimatorConfig Implementation
// ============================================================================

AnimatorConfig AnimatorConfig::defaultConfig() {
    return AnimatorConfig();  // Use default member values
}

// ============================================================================
// CameraAnimator Implementation
// ============================================================================

CameraAnimator::CameraAnimator()
    : m_config()
    , m_animation()
    , m_shake()
    , m_shakeRng(12345)
{
}

CameraAnimator::CameraAnimator(const AnimatorConfig& config)
    : m_config(config)
    , m_animation()
    , m_shake()
    , m_shakeRng(12345)
{
}

// ============================================================================
// Animation Commands
// ============================================================================

void CameraAnimator::animateTo(
    const CameraState& cameraState,
    const glm::vec3& targetPosition,
    float duration,
    Easing::EasingType easing)
{
    // Use config default if duration not specified
    if (duration < 0.0f) {
        duration = m_config.defaultGoToDuration;
    }

    // Capture current state
    m_animation.type = AnimationType::GoTo;
    m_animation.active = true;
    m_animation.startFocusPoint = cameraState.focus_point;
    m_animation.startDistance = cameraState.distance;
    m_animation.startPitch = cameraState.pitch;
    m_animation.startYaw = cameraState.yaw;

    // Set target (only focus changes, other params stay same)
    m_animation.targetFocusPoint = targetPosition;
    m_animation.targetDistance = cameraState.distance;
    m_animation.targetPitch = cameraState.pitch;
    m_animation.targetYaw = cameraState.yaw;

    // Timing
    m_animation.duration = duration;
    m_animation.elapsed = 0.0f;
    m_animation.easingType = easing;
    m_animation.targetMode = CameraMode::Free;
}

void CameraAnimator::animateToState(
    const CameraState& cameraState,
    const glm::vec3& targetFocus,
    float targetDistance,
    float targetPitch,
    float targetYaw,
    float duration,
    Easing::EasingType easing)
{
    // Capture current state
    m_animation.type = AnimationType::GoTo;
    m_animation.active = true;
    m_animation.startFocusPoint = cameraState.focus_point;
    m_animation.startDistance = cameraState.distance;
    m_animation.startPitch = cameraState.pitch;
    m_animation.startYaw = cameraState.yaw;

    // Set target
    m_animation.targetFocusPoint = targetFocus;
    m_animation.targetDistance = targetDistance;
    m_animation.targetPitch = targetPitch;
    m_animation.targetYaw = targetYaw;

    // Timing
    m_animation.duration = duration;
    m_animation.elapsed = 0.0f;
    m_animation.easingType = easing;
    m_animation.targetMode = CameraMode::Free;
}

void CameraAnimator::snapToPreset(
    const CameraState& cameraState,
    CameraMode preset,
    float duration)
{
    // Validate preset mode
    if (preset != CameraMode::Preset_N &&
        preset != CameraMode::Preset_E &&
        preset != CameraMode::Preset_S &&
        preset != CameraMode::Preset_W) {
        return;  // Invalid preset
    }

    // Use config default if duration not specified
    if (duration < 0.0f) {
        duration = m_config.presetSnapDuration;
    }

    // Capture current state
    m_animation.type = AnimationType::PresetSnap;
    m_animation.active = true;
    m_animation.startFocusPoint = cameraState.focus_point;
    m_animation.startDistance = cameraState.distance;
    m_animation.startPitch = cameraState.pitch;
    m_animation.startYaw = cameraState.yaw;

    // Set target from preset
    m_animation.targetFocusPoint = cameraState.focus_point;  // Focus doesn't change
    m_animation.targetDistance = cameraState.distance;        // Distance preserved
    m_animation.targetPitch = CameraState::getPitchForPreset(preset);
    m_animation.targetYaw = CameraState::getYawForPreset(preset);

    // Timing
    m_animation.duration = duration;
    m_animation.elapsed = 0.0f;
    m_animation.easingType = m_config.presetSnapEasing;
    m_animation.targetMode = preset;
}

void CameraAnimator::startShake(float intensity, float duration) {
    m_shake.active = true;
    m_shake.intensity = std::clamp(intensity, 0.0f, 1.0f);
    m_shake.duration = duration;
    m_shake.elapsed = 0.0f;
    m_shake.phase = 0.0f;
    m_shake.offset = glm::vec3(0.0f);
}

void CameraAnimator::stopShake() {
    m_shake.reset();
}

// ============================================================================
// Animation Control
// ============================================================================

void CameraAnimator::interruptAnimation() {
    // Only interrupt main animations (go-to, preset snap)
    // Shake continues even when interrupted
    if (m_animation.active) {
        m_animation.reset();
    }
}

void CameraAnimator::update(float deltaTime, CameraState& cameraState) {
    // Update main animation
    if (m_animation.active) {
        updateAnimation(deltaTime, cameraState);
    }

    // Update shake (independent of main animation)
    if (m_shake.active) {
        updateShake(deltaTime, cameraState);
    }
}

void CameraAnimator::reset() {
    m_animation.reset();
    m_shake.reset();
}

// ============================================================================
// State Query
// ============================================================================

bool CameraAnimator::isAnimating() const {
    return m_animation.active;
}

bool CameraAnimator::isShaking() const {
    return m_shake.active;
}

void CameraAnimator::setConfig(const AnimatorConfig& config) {
    m_config = config;
}

// ============================================================================
// Private Implementation
// ============================================================================

void CameraAnimator::updateAnimation(float deltaTime, CameraState& cameraState) {
    // Progress time
    m_animation.elapsed += deltaTime;

    // Calculate eased progress
    float rawProgress = m_animation.getProgress();
    float easedProgress = Easing::applyEasing(m_animation.easingType, rawProgress);

    // Interpolate focus point
    cameraState.focus_point = glm::mix(
        m_animation.startFocusPoint,
        m_animation.targetFocusPoint,
        easedProgress);

    // Interpolate distance
    cameraState.distance = glm::mix(
        m_animation.startDistance,
        m_animation.targetDistance,
        easedProgress);

    // Interpolate pitch (linear)
    cameraState.pitch = glm::mix(
        m_animation.startPitch,
        m_animation.targetPitch,
        easedProgress);

    // Interpolate yaw (with wrapping)
    cameraState.yaw = interpolateYaw(
        m_animation.startYaw,
        m_animation.targetYaw,
        easedProgress);

    // Apply constraints
    cameraState.applyConstraints();

    // Check for completion
    if (m_animation.isComplete()) {
        // Snap to exact target values
        cameraState.focus_point = m_animation.targetFocusPoint;
        cameraState.distance = m_animation.targetDistance;
        cameraState.pitch = m_animation.targetPitch;
        cameraState.yaw = m_animation.targetYaw;
        cameraState.applyConstraints();

        // Handle preset transition completion
        if (m_animation.type == AnimationType::PresetSnap) {
            finalizePresetTransition(cameraState);
        }

        // Clear animation state
        m_animation.reset();
    }
}

void CameraAnimator::updateShake(float deltaTime, CameraState& cameraState) {
    // Progress time
    m_shake.elapsed += deltaTime;

    if (m_shake.elapsed >= m_shake.duration) {
        // Shake complete
        m_shake.reset();

        // Remove shake offset (it was applied to focus)
        // The offset will naturally be zero after reset
        return;
    }

    // Calculate decay (intensity decreases over time)
    float progress = m_shake.elapsed / m_shake.duration;
    float decayFactor = 1.0f - progress;  // Linear decay
    // Could use exponential: decayFactor = std::exp(-m_config.shakeDecay * progress);

    float currentIntensity = m_shake.intensity * decayFactor;

    // Update phase
    m_shake.phase += deltaTime * m_config.shakeFrequency * 2.0f * 3.14159f;

    // Generate shake offset using noise-like oscillation
    // Use a combination of sin waves at different frequencies for organic feel
    float offsetX = std::sin(m_shake.phase) * randomNoise();
    float offsetZ = std::cos(m_shake.phase * 1.3f) * randomNoise();  // Slightly different freq

    // Y shake is typically smaller for a ground-based camera
    float offsetY = std::sin(m_shake.phase * 0.7f) * randomNoise() * 0.3f;

    // Apply intensity and max offset
    glm::vec3 newOffset(
        offsetX * currentIntensity * m_config.maxShakeOffset,
        offsetY * currentIntensity * m_config.maxShakeOffset,
        offsetZ * currentIntensity * m_config.maxShakeOffset);

    // Calculate delta from previous offset
    glm::vec3 offsetDelta = newOffset - m_shake.offset;

    // Apply delta to camera focus
    cameraState.focus_point += offsetDelta;

    // Store new offset
    m_shake.offset = newOffset;
}

float CameraAnimator::interpolateYaw(float startYaw, float targetYaw, float t) const {
    // Normalize yaw values to 0-360 range
    auto normalizeYaw = [](float yaw) {
        while (yaw < 0.0f) yaw += 360.0f;
        while (yaw >= 360.0f) yaw -= 360.0f;
        return yaw;
    };

    startYaw = normalizeYaw(startYaw);
    targetYaw = normalizeYaw(targetYaw);

    // Calculate the shortest path
    float delta = targetYaw - startYaw;

    // If delta > 180, go the other way (shorter path)
    if (delta > 180.0f) {
        delta -= 360.0f;
    } else if (delta < -180.0f) {
        delta += 360.0f;
    }

    // Interpolate
    float result = startYaw + delta * t;

    // Normalize result
    return normalizeYaw(result);
}

void CameraAnimator::finalizePresetTransition(CameraState& cameraState) {
    // Set the camera mode to the target preset
    cameraState.mode = m_animation.targetMode;

    // Clear any transition state in CameraState itself
    cameraState.transition.reset();
}

float CameraAnimator::randomNoise() {
    // Simple LCG for pseudo-random numbers
    // This gives us deterministic but chaotic-looking shake
    m_shakeRng = m_shakeRng * 1664525u + 1013904223u;

    // Convert to float in [-1, 1]
    return (static_cast<float>(m_shakeRng & 0xFFFF) / 32768.0f) - 1.0f;
}

} // namespace sims3000
