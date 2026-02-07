/**
 * @file CameraModeManager.h
 * @brief Unified camera mode state machine manager.
 *
 * Coordinates camera mode transitions across OrbitController and PresetSnapController:
 * - Free mode: Full orbit/pan/zoom/tilt control
 * - Preset mode: Locked to isometric preset angle (N/E/S/W)
 * - Animating mode: Transitioning between modes/presets
 *
 * Mode transition rules:
 * - Preset-to-Free: Instant unlock on orbit/tilt input (no animation delay)
 * - Free-to-Preset: Smooth animated snap on Q/E key press (0.3-0.5s ease-in-out)
 *
 * Provides getCameraMode() API for other systems to query current mode.
 *
 * Resource ownership: None (pure logic, no GPU/SDL resources).
 */

#ifndef SIMS3000_INPUT_CAMERAMODEMANAGER_H
#define SIMS3000_INPUT_CAMERAMODEMANAGER_H

#include "sims3000/render/CameraState.h"
#include "sims3000/input/OrbitController.h"
#include "sims3000/input/PresetSnapController.h"
#include "sims3000/input/CameraAnimator.h"
#include <cstdint>

namespace sims3000 {

// Forward declarations
class InputSystem;

// ============================================================================
// Mode Manager Configuration
// ============================================================================

/**
 * @struct CameraModeManagerConfig
 * @brief Configuration for camera mode manager behavior.
 */
struct CameraModeManagerConfig {
    // Default mode on game start
    CameraMode defaultMode = CameraMode::Preset_N;

    // Animation duration for free-to-preset transitions (0.3-0.5s per spec)
    float presetSnapDuration = 0.4f;

    /**
     * @brief Get default configuration.
     */
    static CameraModeManagerConfig defaultConfig();
};

// ============================================================================
// Camera Mode Manager
// ============================================================================

/**
 * @class CameraModeManager
 * @brief Unified state machine for camera mode management.
 *
 * The CameraModeManager acts as a coordinator for the camera system, managing
 * transitions between Free, Preset, and Animating modes. It ensures:
 *
 * 1. **Preset-to-Free (instant):** When the player initiates orbit/tilt input
 *    (e.g., middle mouse drag), the camera instantly unlocks from preset mode
 *    to free mode without any animation delay. This provides immediate,
 *    responsive control.
 *
 * 2. **Free-to-Preset (smooth):** When the player presses Q or E to snap to
 *    a preset, the camera smoothly animates to the preset angle with
 *    ease-in-out easing over 0.3-0.5 seconds.
 *
 * 3. **Mode Queries:** Other systems can query the current mode via
 *    getCameraMode() to adjust their behavior (e.g., UI indicators).
 *
 * Usage:
 * @code
 *   CameraModeManager modeManager;
 *   CameraState cameraState;
 *
 *   // Initialize to default preset mode
 *   modeManager.initialize(cameraState);
 *
 *   // In game loop:
 *   modeManager.handleInput(inputSystem, cameraState);
 *   modeManager.update(deltaTime, cameraState);
 *
 *   // Query current mode:
 *   CameraMode mode = modeManager.getCameraMode();
 * @endcode
 */
class CameraModeManager {
public:
    /**
     * @brief Construct mode manager with default configuration.
     */
    CameraModeManager();

    /**
     * @brief Construct mode manager with custom configuration.
     * @param config Mode manager configuration.
     */
    explicit CameraModeManager(const CameraModeManagerConfig& config);

    ~CameraModeManager() = default;

    // Non-copyable (owns controllers)
    CameraModeManager(const CameraModeManager&) = delete;
    CameraModeManager& operator=(const CameraModeManager&) = delete;

    // Movable
    CameraModeManager(CameraModeManager&&) = default;
    CameraModeManager& operator=(CameraModeManager&&) = default;

    // ========================================================================
    // Initialization
    // ========================================================================

    /**
     * @brief Initialize camera to default mode.
     *
     * Sets the camera to the configured default mode (Preset_N by default).
     * Call once at game start.
     *
     * @param cameraState Camera state to initialize.
     */
    void initialize(CameraState& cameraState);

    /**
     * @brief Reset to default mode.
     *
     * Resets camera and all controllers to initial state.
     *
     * @param cameraState Camera state to reset.
     */
    void reset(CameraState& cameraState);

    // ========================================================================
    // Input Handling
    // ========================================================================

    /**
     * @brief Process input and handle mode transitions.
     *
     * Checks for:
     * - Orbit/tilt input (middle mouse drag) -> instant unlock to Free
     * - Q/E key press -> smooth snap to Preset
     *
     * @param input Input system for queries.
     * @param cameraState Camera state to modify.
     * @return true if input was handled and affected camera mode.
     */
    bool handleInput(const InputSystem& input, CameraState& cameraState);

    // ========================================================================
    // Update
    // ========================================================================

    /**
     * @brief Update animations and controllers.
     *
     * Progresses any active animations and applies smoothing.
     * Call every frame.
     *
     * @param deltaTime Frame delta time in seconds.
     * @param cameraState Camera state to modify.
     */
    void update(float deltaTime, CameraState& cameraState);

    // ========================================================================
    // Mode Queries (API for other systems)
    // ========================================================================

    /**
     * @brief Get current camera mode.
     *
     * Returns the current camera operating mode:
     * - Free: Full orbit/pan/zoom/tilt control active
     * - Preset_N/E/S/W: Locked to isometric preset view
     * - Animating: Transitioning between modes
     *
     * @return Current camera mode.
     */
    CameraMode getCameraMode() const { return m_currentMode; }

    /**
     * @brief Check if camera is in free mode.
     * @return true if mode is Free.
     */
    bool isInFreeMode() const { return m_currentMode == CameraMode::Free; }

    /**
     * @brief Check if camera is in any preset mode.
     * @return true if mode is Preset_N, Preset_E, Preset_S, or Preset_W.
     */
    bool isInPresetMode() const;

    /**
     * @brief Check if camera is currently animating.
     * @return true if mode is Animating.
     */
    bool isAnimating() const { return m_currentMode == CameraMode::Animating; }

    /**
     * @brief Get the current or target preset.
     *
     * If in preset mode, returns current preset.
     * If animating to preset, returns target preset.
     * If in free mode, returns the last preset or closest preset.
     *
     * @return Current/target preset mode.
     */
    CameraMode getCurrentPreset() const;

    /**
     * @brief Get the preset indicator data for UI.
     *
     * Provides all information needed to render a camera mode indicator.
     *
     * @param cameraState Current camera state.
     * @return PresetIndicator with current mode and animation state.
     */
    PresetIndicator getPresetIndicator(const CameraState& cameraState) const;

    // ========================================================================
    // Direct Mode Control
    // ========================================================================

    /**
     * @brief Force camera to free mode immediately.
     *
     * Cancels any active animation and enters free mode.
     * Used for programmatic control.
     *
     * @param cameraState Camera state to modify.
     */
    void forceToFreeMode(CameraState& cameraState);

    /**
     * @brief Force camera to a specific preset with animation.
     *
     * Initiates a smooth transition to the specified preset.
     *
     * @param preset Target preset (Preset_N, Preset_E, Preset_S, Preset_W).
     * @param cameraState Camera state to modify.
     * @param animate If true, animate to preset. If false, snap instantly.
     */
    void forceToPreset(CameraMode preset, CameraState& cameraState, bool animate = true);

    // ========================================================================
    // Controller Access (for advanced use)
    // ========================================================================

    /**
     * @brief Get the orbit controller.
     * @return Reference to OrbitController.
     */
    OrbitController& getOrbitController() { return m_orbitController; }

    /**
     * @brief Get the orbit controller (const).
     * @return Const reference to OrbitController.
     */
    const OrbitController& getOrbitController() const { return m_orbitController; }

    /**
     * @brief Get the preset snap controller.
     * @return Reference to PresetSnapController.
     */
    PresetSnapController& getPresetSnapController() { return m_presetController; }

    /**
     * @brief Get the preset snap controller (const).
     * @return Const reference to PresetSnapController.
     */
    const PresetSnapController& getPresetSnapController() const { return m_presetController; }

    /**
     * @brief Get the camera animator.
     * @return Reference to CameraAnimator.
     */
    CameraAnimator& getAnimator() { return m_animator; }

    /**
     * @brief Get the camera animator (const).
     * @return Const reference to CameraAnimator.
     */
    const CameraAnimator& getAnimator() const { return m_animator; }

    // ========================================================================
    // Configuration
    // ========================================================================

    /**
     * @brief Get current configuration.
     * @return Current mode manager configuration.
     */
    const CameraModeManagerConfig& getConfig() const { return m_config; }

    /**
     * @brief Set configuration.
     * @param config New configuration.
     */
    void setConfig(const CameraModeManagerConfig& config);

private:
    /**
     * @brief Sync mode state from camera state.
     *
     * Ensures m_currentMode matches cameraState.mode.
     */
    void syncModeFromCameraState(const CameraState& cameraState);

    /**
     * @brief Handle transition to free mode (instant unlock).
     */
    void transitionToFreeMode(CameraState& cameraState);

    /**
     * @brief Handle transition to preset mode (animated snap).
     */
    void transitionToPreset(CameraMode preset, CameraState& cameraState);

    CameraModeManagerConfig m_config;

    // Current tracked mode
    CameraMode m_currentMode = CameraMode::Preset_N;

    // Sub-controllers
    OrbitController m_orbitController;
    PresetSnapController m_presetController;
    CameraAnimator m_animator;

    // Track last preset for returning from free mode
    CameraMode m_lastPreset = CameraMode::Preset_N;
};

} // namespace sims3000

#endif // SIMS3000_INPUT_CAMERAMODEMANAGER_H
