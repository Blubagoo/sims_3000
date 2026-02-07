/**
 * @file PresetSnapController.cpp
 * @brief Isometric preset snap controller implementation.
 */

#include "sims3000/input/PresetSnapController.h"
#include "sims3000/input/InputSystem.h"
#include "sims3000/input/CameraAnimator.h"
#include <cmath>

namespace sims3000 {

// ============================================================================
// PresetSnapConfig Implementation
// ============================================================================

PresetSnapConfig PresetSnapConfig::defaultConfig() {
    PresetSnapConfig config;
    // Default values already set in struct definition
    return config;
}

// ============================================================================
// PresetSnapController Implementation
// ============================================================================

PresetSnapController::PresetSnapController()
    : m_config()
    , m_currentPreset(CameraMode::Preset_N)
{
}

PresetSnapController::PresetSnapController(const PresetSnapConfig& config)
    : m_config(config)
    , m_currentPreset(CameraMode::Preset_N)
{
}

bool PresetSnapController::handleInput(
    const InputSystem& input,
    CameraState& cameraState,
    CameraAnimator& animator)
{
    // Check for Q key (clockwise rotation)
    if (input.isKeyPressed(SDL_SCANCODE_Q)) {
        snapClockwise(cameraState, animator);
        return true;
    }

    // Check for E key (counterclockwise rotation)
    if (input.isKeyPressed(SDL_SCANCODE_E)) {
        snapCounterclockwise(cameraState, animator);
        return true;
    }

    return false;
}

void PresetSnapController::snapClockwise(CameraState& cameraState, CameraAnimator& animator) {
    // Determine current preset position
    // If in free mode or animating, use the closest preset as the starting point
    CameraMode startPreset = m_currentPreset;
    if (cameraState.mode == CameraMode::Free || cameraState.mode == CameraMode::Animating) {
        startPreset = getClosestPreset(cameraState);
    } else if (cameraState.isPresetMode()) {
        startPreset = cameraState.mode;
    }

    // Get next clockwise preset
    CameraMode targetPreset = getNextClockwise(startPreset);

    // Snap to the target
    snapToPreset(targetPreset, cameraState, animator);
}

void PresetSnapController::snapCounterclockwise(CameraState& cameraState, CameraAnimator& animator) {
    // Determine current preset position
    // If in free mode or animating, use the closest preset as the starting point
    CameraMode startPreset = m_currentPreset;
    if (cameraState.mode == CameraMode::Free || cameraState.mode == CameraMode::Animating) {
        startPreset = getClosestPreset(cameraState);
    } else if (cameraState.isPresetMode()) {
        startPreset = cameraState.mode;
    }

    // Get next counterclockwise preset
    CameraMode targetPreset = getNextCounterclockwise(startPreset);

    // Snap to the target
    snapToPreset(targetPreset, cameraState, animator);
}

void PresetSnapController::snapToPreset(
    CameraMode preset,
    CameraState& cameraState,
    CameraAnimator& animator)
{
    // Validate preset mode
    if (preset != CameraMode::Preset_N &&
        preset != CameraMode::Preset_E &&
        preset != CameraMode::Preset_S &&
        preset != CameraMode::Preset_W) {
        return;  // Invalid preset
    }

    // Update tracked current preset
    m_currentPreset = preset;

    // Use CameraAnimator to perform the smooth transition
    animator.snapToPreset(cameraState, preset, m_config.snapDuration);
}

bool PresetSnapController::isInPresetMode(const CameraState& cameraState) {
    return cameraState.isPresetMode();
}

CameraMode PresetSnapController::getClosestPreset(const CameraState& cameraState) {
    // Get current yaw (normalized to 0-360)
    float yaw = cameraState.yaw;
    while (yaw < 0.0f) yaw += 360.0f;
    while (yaw >= 360.0f) yaw -= 360.0f;

    // Preset yaw values
    constexpr float PRESET_N = CameraConfig::PRESET_N_YAW;   // 45
    constexpr float PRESET_E = CameraConfig::PRESET_E_YAW;   // 135
    constexpr float PRESET_S = CameraConfig::PRESET_S_YAW;   // 225
    constexpr float PRESET_W = CameraConfig::PRESET_W_YAW;   // 315

    // Calculate angular distances (accounting for wrap-around)
    auto angularDistance = [](float a, float b) -> float {
        float diff = std::fabs(a - b);
        return std::min(diff, 360.0f - diff);
    };

    float distN = angularDistance(yaw, PRESET_N);
    float distE = angularDistance(yaw, PRESET_E);
    float distS = angularDistance(yaw, PRESET_S);
    float distW = angularDistance(yaw, PRESET_W);

    // Find minimum
    float minDist = distN;
    CameraMode closest = CameraMode::Preset_N;

    if (distE < minDist) {
        minDist = distE;
        closest = CameraMode::Preset_E;
    }
    if (distS < minDist) {
        minDist = distS;
        closest = CameraMode::Preset_S;
    }
    if (distW < minDist) {
        closest = CameraMode::Preset_W;
    }

    return closest;
}

CameraMode PresetSnapController::getNextClockwise(CameraMode current) {
    // Clockwise order: N(45) -> E(135) -> S(225) -> W(315) -> N
    switch (current) {
        case CameraMode::Preset_N: return CameraMode::Preset_E;
        case CameraMode::Preset_E: return CameraMode::Preset_S;
        case CameraMode::Preset_S: return CameraMode::Preset_W;
        case CameraMode::Preset_W: return CameraMode::Preset_N;
        default: return CameraMode::Preset_N;  // Default to N for Free/Animating
    }
}

CameraMode PresetSnapController::getNextCounterclockwise(CameraMode current) {
    // Counterclockwise order: N(45) -> W(315) -> S(225) -> E(135) -> N
    switch (current) {
        case CameraMode::Preset_N: return CameraMode::Preset_W;
        case CameraMode::Preset_W: return CameraMode::Preset_S;
        case CameraMode::Preset_S: return CameraMode::Preset_E;
        case CameraMode::Preset_E: return CameraMode::Preset_N;
        default: return CameraMode::Preset_N;  // Default to N for Free/Animating
    }
}

void PresetSnapController::setConfig(const PresetSnapConfig& config) {
    m_config = config;
}

void PresetSnapController::setSnapDuration(float duration) {
    if (duration > 0.0f) {
        m_config.snapDuration = duration;
    }
}

const char* PresetSnapController::getCardinalName(CameraMode mode) {
    switch (mode) {
        case CameraMode::Preset_N: return "N";
        case CameraMode::Preset_E: return "E";
        case CameraMode::Preset_S: return "S";
        case CameraMode::Preset_W: return "W";
        default: return "Free";
    }
}

PresetIndicator PresetSnapController::getPresetIndicator(
    const CameraState& cameraState,
    const CameraAnimator& animator) const
{
    PresetIndicator indicator;

    // Determine current/target preset
    if (animator.isAnimating() && animator.getAnimationType() == AnimationType::PresetSnap) {
        // During preset snap animation, use the target preset
        indicator.currentPreset = m_currentPreset;
        indicator.isAnimating = true;
        indicator.animationProgress = animator.getAnimationProgress();
    } else if (cameraState.isPresetMode()) {
        // Settled in a preset
        indicator.currentPreset = cameraState.mode;
        indicator.isAnimating = false;
        indicator.animationProgress = 1.0f;
    } else {
        // Free or other mode - use closest preset for reference
        indicator.currentPreset = getClosestPreset(cameraState);
        indicator.isAnimating = false;
        indicator.animationProgress = 0.0f;
    }

    // Set cardinal name
    indicator.cardinalName = getCardinalName(indicator.currentPreset);

    // Set yaw degrees for compass rotation
    indicator.yawDegrees = cameraState.yaw;

    return indicator;
}

} // namespace sims3000
