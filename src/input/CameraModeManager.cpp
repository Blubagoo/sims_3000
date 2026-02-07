/**
 * @file CameraModeManager.cpp
 * @brief Unified camera mode state machine manager implementation.
 */

#include "sims3000/input/CameraModeManager.h"
#include "sims3000/input/InputSystem.h"

namespace sims3000 {

// ============================================================================
// CameraModeManagerConfig Implementation
// ============================================================================

CameraModeManagerConfig CameraModeManagerConfig::defaultConfig() {
    CameraModeManagerConfig config;
    // Default values already set in struct definition
    return config;
}

// ============================================================================
// CameraModeManager Implementation
// ============================================================================

CameraModeManager::CameraModeManager()
    : m_config()
    , m_currentMode(CameraMode::Preset_N)
    , m_orbitController()
    , m_presetController()
    , m_animator()
    , m_lastPreset(CameraMode::Preset_N)
{
    // Configure preset controller with our duration
    PresetSnapConfig presetConfig;
    presetConfig.snapDuration = m_config.presetSnapDuration;
    m_presetController.setConfig(presetConfig);

    // Configure animator with matching duration
    AnimatorConfig animConfig;
    animConfig.presetSnapDuration = m_config.presetSnapDuration;
    m_animator.setConfig(animConfig);
}

CameraModeManager::CameraModeManager(const CameraModeManagerConfig& config)
    : m_config(config)
    , m_currentMode(config.defaultMode)
    , m_orbitController()
    , m_presetController()
    , m_animator()
    , m_lastPreset(CameraMode::Preset_N)
{
    // Set last preset based on default mode
    if (config.defaultMode != CameraMode::Free &&
        config.defaultMode != CameraMode::Animating) {
        m_lastPreset = config.defaultMode;
    }

    // Configure preset controller with our duration
    PresetSnapConfig presetConfig;
    presetConfig.snapDuration = config.presetSnapDuration;
    m_presetController.setConfig(presetConfig);

    // Configure animator with matching duration
    AnimatorConfig animConfig;
    animConfig.presetSnapDuration = config.presetSnapDuration;
    m_animator.setConfig(animConfig);
}

// ============================================================================
// Initialization
// ============================================================================

void CameraModeManager::initialize(CameraState& cameraState) {
    // Set camera to default mode
    CameraMode defaultMode = m_config.defaultMode;

    if (defaultMode == CameraMode::Free) {
        cameraState.mode = CameraMode::Free;
    } else if (defaultMode != CameraMode::Animating) {
        // Default to preset mode
        cameraState.mode = defaultMode;
        cameraState.pitch = CameraState::getPitchForPreset(defaultMode);
        cameraState.yaw = CameraState::getYawForPreset(defaultMode);
        m_lastPreset = defaultMode;
    } else {
        // Animating is not a valid default, use Preset_N
        cameraState.mode = CameraMode::Preset_N;
        cameraState.pitch = CameraConfig::ISOMETRIC_PITCH;
        cameraState.yaw = CameraConfig::PRESET_N_YAW;
        m_lastPreset = CameraMode::Preset_N;
    }

    m_currentMode = cameraState.mode;

    // Reset all controllers
    m_orbitController.reset(cameraState);
    m_animator.reset();
}

void CameraModeManager::reset(CameraState& cameraState) {
    // Reset camera state
    cameraState.resetToDefault();

    // Reset mode tracking
    m_currentMode = CameraMode::Preset_N;
    m_lastPreset = CameraMode::Preset_N;

    // Reset all controllers
    m_orbitController.reset(cameraState);
    m_animator.reset();
}

// ============================================================================
// Input Handling
// ============================================================================

bool CameraModeManager::handleInput(const InputSystem& input, CameraState& cameraState) {
    // Sync mode from camera state (in case it was modified externally)
    syncModeFromCameraState(cameraState);

    // Check for Q/E preset snap keys first (free-to-preset transition)
    // This takes priority because it's an explicit user action
    if (m_presetController.handleInput(input, cameraState, m_animator)) {
        // PresetSnapController initiated a snap animation
        // Mode will be set to Animating by the animator
        m_currentMode = CameraMode::Animating;
        return true;
    }

    // Check for orbit/tilt input (preset-to-free transition)
    // OrbitController handles instant unlock internally
    if (m_orbitController.handleInput(input, cameraState)) {
        // Orbit input was detected and processed
        // If we were in preset mode, OrbitController already set us to Free
        syncModeFromCameraState(cameraState);

        // Track that we left preset mode
        if (m_currentMode == CameraMode::Free && isInPresetMode()) {
            // Save which preset we came from
            m_lastPreset = m_currentMode;
        }

        return true;
    }

    return false;
}

// ============================================================================
// Update
// ============================================================================

void CameraModeManager::update(float deltaTime, CameraState& cameraState) {
    // Update animator (handles preset snap and other animations)
    m_animator.update(deltaTime, cameraState);

    // Update orbit controller (handles smooth interpolation)
    m_orbitController.update(deltaTime, cameraState);

    // Sync mode from camera state (animator may have changed it)
    syncModeFromCameraState(cameraState);

    // If we finished animating to a preset, record it
    if (!m_animator.isAnimating() && cameraState.isPresetMode()) {
        m_lastPreset = cameraState.mode;
    }
}

// ============================================================================
// Mode Queries
// ============================================================================

bool CameraModeManager::isInPresetMode() const {
    return m_currentMode == CameraMode::Preset_N ||
           m_currentMode == CameraMode::Preset_E ||
           m_currentMode == CameraMode::Preset_S ||
           m_currentMode == CameraMode::Preset_W;
}

CameraMode CameraModeManager::getCurrentPreset() const {
    if (isInPresetMode()) {
        return m_currentMode;
    }

    // If animating, get the target preset from the animator
    if (m_currentMode == CameraMode::Animating && m_animator.isAnimating()) {
        // PresetSnapController tracks the target preset
        return m_presetController.getCurrentPreset();
    }

    // In free mode, return last preset
    return m_lastPreset;
}

PresetIndicator CameraModeManager::getPresetIndicator(const CameraState& cameraState) const {
    return m_presetController.getPresetIndicator(cameraState, m_animator);
}

// ============================================================================
// Direct Mode Control
// ============================================================================

void CameraModeManager::forceToFreeMode(CameraState& cameraState) {
    transitionToFreeMode(cameraState);
}

void CameraModeManager::forceToPreset(CameraMode preset, CameraState& cameraState, bool animate) {
    // Validate preset mode
    if (preset != CameraMode::Preset_N &&
        preset != CameraMode::Preset_E &&
        preset != CameraMode::Preset_S &&
        preset != CameraMode::Preset_W) {
        return;  // Invalid preset
    }

    if (animate) {
        transitionToPreset(preset, cameraState);
    } else {
        // Instant snap
        cameraState.mode = preset;
        cameraState.pitch = CameraState::getPitchForPreset(preset);
        cameraState.yaw = CameraState::getYawForPreset(preset);
        cameraState.applyConstraints();

        m_currentMode = preset;
        m_lastPreset = preset;

        // Sync controllers
        m_orbitController.reset(cameraState);
        m_animator.reset();
    }
}

// ============================================================================
// Configuration
// ============================================================================

void CameraModeManager::setConfig(const CameraModeManagerConfig& config) {
    m_config = config;

    // Update sub-controller configs
    PresetSnapConfig presetConfig = m_presetController.getConfig();
    presetConfig.snapDuration = config.presetSnapDuration;
    m_presetController.setConfig(presetConfig);

    AnimatorConfig animConfig = m_animator.getConfig();
    animConfig.presetSnapDuration = config.presetSnapDuration;
    m_animator.setConfig(animConfig);
}

// ============================================================================
// Private Implementation
// ============================================================================

void CameraModeManager::syncModeFromCameraState(const CameraState& cameraState) {
    m_currentMode = cameraState.mode;
}

void CameraModeManager::transitionToFreeMode(CameraState& cameraState) {
    // If animating, cancel the animation
    if (m_animator.isAnimating()) {
        m_animator.interruptAnimation();
    }

    // Clear any transition state
    cameraState.transition.reset();

    // Set to free mode immediately (no animation)
    cameraState.mode = CameraMode::Free;
    m_currentMode = CameraMode::Free;

    // Sync orbit controller with current state
    m_orbitController.reset(cameraState);
}

void CameraModeManager::transitionToPreset(CameraMode preset, CameraState& cameraState) {
    // Use animator for smooth transition
    m_animator.snapToPreset(cameraState, preset, m_config.presetSnapDuration);

    // Mode will be set to Animating by animator's snapToPreset
    // It will automatically set to the preset mode when animation completes
    m_currentMode = CameraMode::Animating;
    m_lastPreset = preset;
}

} // namespace sims3000
