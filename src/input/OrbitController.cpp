/**
 * @file OrbitController.cpp
 * @brief Orbit and tilt controller implementation for free camera mode.
 */

#include "sims3000/input/OrbitController.h"
#include "sims3000/input/InputSystem.h"
#include <cmath>
#include <algorithm>

namespace sims3000 {

// ============================================================================
// OrbitConfig Implementation
// ============================================================================

OrbitConfig OrbitConfig::defaultConfig() {
    OrbitConfig config;
    // Default values already set in struct definition
    return config;
}

// ============================================================================
// OrbitController Implementation
// ============================================================================

OrbitController::OrbitController()
    : m_config()
    , m_targetYaw(CameraConfig::PRESET_N_YAW)
    , m_targetPitch(CameraConfig::ISOMETRIC_PITCH)
    , m_currentYaw(CameraConfig::PRESET_N_YAW)
    , m_currentPitch(CameraConfig::ISOMETRIC_PITCH)
    , m_isOrbiting(false)
    , m_lastDragDeltaX(0)
    , m_lastDragDeltaY(0)
{
}

OrbitController::OrbitController(const OrbitConfig& config)
    : m_config(config)
    , m_targetYaw(CameraConfig::PRESET_N_YAW)
    , m_targetPitch(CameraConfig::ISOMETRIC_PITCH)
    , m_currentYaw(CameraConfig::PRESET_N_YAW)
    , m_currentPitch(CameraConfig::ISOMETRIC_PITCH)
    , m_isOrbiting(false)
    , m_lastDragDeltaX(0)
    , m_lastDragDeltaY(0)
{
}

bool OrbitController::handleInput(const InputSystem& input, CameraState& cameraState) {
    // Check for middle mouse button drag
    bool middleDown = input.isMouseButtonDown(MouseButton::Middle);

    if (middleDown && input.isDragging()) {
        // Get total drag delta from InputSystem
        int totalDragDeltaX = 0, totalDragDeltaY = 0;
        input.getDragDelta(totalDragDeltaX, totalDragDeltaY);

        // Calculate frame delta from stored last values
        int frameDeltaX = totalDragDeltaX - m_lastDragDeltaX;
        int frameDeltaY = totalDragDeltaY - m_lastDragDeltaY;

        if (frameDeltaX != 0 || frameDeltaY != 0) {
            // Apply orbit/tilt
            bool applied = handleOrbitTilt(frameDeltaX, frameDeltaY, cameraState);

            if (applied) {
                m_isOrbiting = true;
            }
        }

        m_lastDragDeltaX = totalDragDeltaX;
        m_lastDragDeltaY = totalDragDeltaY;

        return m_isOrbiting;
    } else {
        // Reset drag tracking when not dragging
        if (m_isOrbiting) {
            m_lastDragDeltaX = 0;
            m_lastDragDeltaY = 0;
        }
        m_isOrbiting = false;
    }

    return false;
}

bool OrbitController::handleOrbitTilt(int deltaX, int deltaY, CameraState& cameraState) {
    if (deltaX == 0 && deltaY == 0) {
        return false;
    }

    // If camera is in preset mode, instantly switch to free mode (no animation)
    if (cameraState.isPresetMode()) {
        // Cancel any active transition
        cameraState.transition.reset();
        cameraState.mode = CameraMode::Free;
    }

    // Also handle if we're animating to a preset - cancel and go free
    if (cameraState.mode == CameraMode::Animating) {
        cameraState.transition.reset();
        cameraState.mode = CameraMode::Free;
    }

    // Sync current state with camera state on first input
    m_currentYaw = cameraState.yaw;
    m_currentPitch = cameraState.pitch;

    // Calculate orbit delta (horizontal drag = yaw change)
    float orbitDelta = static_cast<float>(deltaX) * m_config.orbitSensitivity;
    if (m_config.invertOrbit) {
        orbitDelta = -orbitDelta;
    }

    // Calculate tilt delta (vertical drag = pitch change)
    // Dragging down increases pitch (more top-down), dragging up decreases pitch
    float tiltDelta = static_cast<float>(deltaY) * m_config.tiltSensitivity;
    if (m_config.invertTilt) {
        tiltDelta = -tiltDelta;
    }

    // Update target values
    m_targetYaw = wrapYaw(m_currentYaw + orbitDelta);
    m_targetPitch = clampPitch(m_currentPitch + tiltDelta);

    return true;
}

void OrbitController::update(float deltaTime, CameraState& cameraState) {
    // Exponential smoothing for natural feel
    float t = 1.0f - std::exp(-m_config.smoothingFactor * deltaTime);
    t = std::clamp(t, 0.0f, 1.0f);

    // Interpolate yaw using shortest path
    float yawDelta = calculateYawDelta(m_currentYaw, m_targetYaw);
    m_currentYaw = wrapYaw(m_currentYaw + yawDelta * t);

    // Interpolate pitch linearly
    m_currentPitch = m_currentPitch + (m_targetPitch - m_currentPitch) * t;

    // Apply to camera state
    cameraState.yaw = m_currentYaw;
    cameraState.pitch = m_currentPitch;

    // Apply constraints (belt and suspenders - should already be valid)
    cameraState.wrapYaw();
    cameraState.clampPitch();
}

void OrbitController::setTargetYaw(float yaw) {
    m_targetYaw = wrapYaw(yaw);
}

void OrbitController::setTargetPitch(float pitch) {
    m_targetPitch = clampPitch(pitch);
}

void OrbitController::setYawImmediate(float yaw, CameraState& cameraState) {
    float wrappedYaw = wrapYaw(yaw);
    m_targetYaw = wrappedYaw;
    m_currentYaw = wrappedYaw;
    cameraState.yaw = wrappedYaw;
}

void OrbitController::setPitchImmediate(float pitch, CameraState& cameraState) {
    float clampedPitch = clampPitch(pitch);
    m_targetPitch = clampedPitch;
    m_currentPitch = clampedPitch;
    cameraState.pitch = clampedPitch;
}

void OrbitController::reset(const CameraState& cameraState) {
    m_targetYaw = cameraState.yaw;
    m_targetPitch = cameraState.pitch;
    m_currentYaw = cameraState.yaw;
    m_currentPitch = cameraState.pitch;
    m_isOrbiting = false;
    m_lastDragDeltaX = 0;
    m_lastDragDeltaY = 0;
}

bool OrbitController::isOrbiting() const {
    return m_isOrbiting;
}

bool OrbitController::isInterpolating() const {
    float yawDelta = std::fabs(calculateYawDelta(m_currentYaw, m_targetYaw));
    float pitchDelta = std::fabs(m_targetPitch - m_currentPitch);

    return yawDelta > INTERPOLATION_THRESHOLD || pitchDelta > INTERPOLATION_THRESHOLD;
}

void OrbitController::setConfig(const OrbitConfig& config) {
    m_config = config;
}

void OrbitController::setOrbitSensitivity(float sensitivity) {
    if (sensitivity > 0.0f) {
        m_config.orbitSensitivity = sensitivity;
    }
}

void OrbitController::setTiltSensitivity(float sensitivity) {
    if (sensitivity > 0.0f) {
        m_config.tiltSensitivity = sensitivity;
    }
}

float OrbitController::wrapYaw(float yaw) const {
    // Wrap to [0, 360)
    while (yaw < CameraConfig::YAW_MIN) {
        yaw += CameraConfig::YAW_MAX;
    }
    while (yaw >= CameraConfig::YAW_MAX) {
        yaw -= CameraConfig::YAW_MAX;
    }
    return yaw;
}

float OrbitController::clampPitch(float pitch) const {
    return std::clamp(pitch, m_config.pitchMin, m_config.pitchMax);
}

float OrbitController::calculateYawDelta(float current, float target) const {
    // Calculate the shortest angular distance between two yaw values
    // Both should already be in [0, 360) range

    float delta = target - current;

    // If delta is more than 180, it's shorter to go the other way
    if (delta > 180.0f) {
        delta -= 360.0f;
    } else if (delta < -180.0f) {
        delta += 360.0f;
    }

    return delta;
}

} // namespace sims3000
