/**
 * @file OrbitController.h
 * @brief Orbit and tilt controller for free camera mode.
 *
 * Implements orbit (yaw rotation around focus point) and tilt (pitch adjustment)
 * controls for the camera system:
 * - Middle mouse drag orbits the camera (rotates yaw around focus point)
 * - Vertical drag adjusts pitch (tilt)
 * - Pitch clamped to 15-80 degrees (using CameraConfig::PITCH_MIN/MAX)
 * - Yaw wraps around 0-360 degrees
 * - Orbit/tilt input instantly unlocks from preset mode (no animation delay)
 * - Configurable mouse sensitivity
 *
 * Resource ownership: None (pure logic, no GPU/SDL resources).
 */

#ifndef SIMS3000_INPUT_ORBITCONTROLLER_H
#define SIMS3000_INPUT_ORBITCONTROLLER_H

#include "sims3000/render/CameraState.h"
#include <cstdint>

namespace sims3000 {

// Forward declarations
class InputSystem;

// ============================================================================
// Orbit Configuration
// ============================================================================

/**
 * @struct OrbitConfig
 * @brief Configuration for orbit and tilt behavior.
 */
struct OrbitConfig {
    // Sensitivity settings
    float orbitSensitivity = 0.3f;    ///< Degrees of yaw per pixel of horizontal drag
    float tiltSensitivity = 0.2f;     ///< Degrees of pitch per pixel of vertical drag

    // Smoothing
    float smoothingFactor = 12.0f;    ///< Interpolation smoothing (higher = faster response)

    // Inversion options
    bool invertOrbit = false;         ///< Invert horizontal orbit direction
    bool invertTilt = false;          ///< Invert vertical tilt direction

    // Pitch limits (use CameraConfig values by default)
    float pitchMin = CameraConfig::PITCH_MIN;   ///< Minimum pitch (shallow view)
    float pitchMax = CameraConfig::PITCH_MAX;   ///< Maximum pitch (top-down view)

    /**
     * @brief Get default orbit configuration.
     */
    static OrbitConfig defaultConfig();
};

// ============================================================================
// Orbit Controller
// ============================================================================

/**
 * @class OrbitController
 * @brief Controls camera orbit and tilt with smooth interpolation.
 *
 * Provides "walking around a diorama" feel where the camera orbits around
 * the focus point. Middle mouse drag controls orbit (yaw) and tilt (pitch).
 *
 * When orbit/tilt input is detected while in a preset mode, the camera
 * instantly unlocks to free mode (no animation delay) to provide immediate
 * responsive control.
 *
 * Usage:
 * @code
 *   OrbitController orbit;
 *
 *   // In input processing:
 *   orbit.handleInput(input, cameraState);
 *
 *   // In update loop:
 *   orbit.update(deltaTime, cameraState);
 * @endcode
 */
class OrbitController {
public:
    /**
     * @brief Construct orbit controller with default configuration.
     */
    OrbitController();

    /**
     * @brief Construct orbit controller with custom configuration.
     * @param config Orbit configuration.
     */
    explicit OrbitController(const OrbitConfig& config);

    ~OrbitController() = default;

    // Non-copyable (stateful controller)
    OrbitController(const OrbitController&) = delete;
    OrbitController& operator=(const OrbitController&) = delete;

    // Movable
    OrbitController(OrbitController&&) = default;
    OrbitController& operator=(OrbitController&&) = default;

    // ========================================================================
    // Input Handling
    // ========================================================================

    /**
     * @brief Handle input and calculate orbit/tilt changes.
     *
     * Reads middle mouse button drag input and calculates the target yaw and
     * pitch for orbit/tilt behavior. If in preset mode, instantly switches to
     * free mode (no animation delay).
     *
     * @param input Input system for mouse state queries.
     * @param cameraState Current camera state.
     * @return true if orbit/tilt input was processed (middle mouse drag active).
     */
    bool handleInput(const InputSystem& input, CameraState& cameraState);

    /**
     * @brief Handle orbit/tilt input with explicit delta values.
     *
     * Overload for cases where drag delta is known externally.
     *
     * @param deltaX Horizontal drag delta in pixels (positive = right).
     * @param deltaY Vertical drag delta in pixels (positive = down).
     * @param cameraState Current camera state.
     * @return true if orbit/tilt was applied.
     */
    bool handleOrbitTilt(int deltaX, int deltaY, CameraState& cameraState);

    // ========================================================================
    // Update
    // ========================================================================

    /**
     * @brief Update orbit/tilt interpolation.
     *
     * Smoothly interpolates camera yaw and pitch toward target values.
     * Call every frame.
     *
     * @param deltaTime Frame delta time in seconds.
     * @param cameraState Camera state to modify (yaw and pitch).
     */
    void update(float deltaTime, CameraState& cameraState);

    // ========================================================================
    // Direct Control
    // ========================================================================

    /**
     * @brief Set target yaw directly (bypasses input handling).
     *
     * Useful for programmatic camera rotation. The controller will smoothly
     * interpolate to this yaw value.
     *
     * @param yaw Target yaw in degrees (will be wrapped to 0-360).
     */
    void setTargetYaw(float yaw);

    /**
     * @brief Set target pitch directly (bypasses input handling).
     *
     * Useful for programmatic camera tilt. The controller will smoothly
     * interpolate to this pitch value.
     *
     * @param pitch Target pitch in degrees (will be clamped to min/max).
     */
    void setTargetPitch(float pitch);

    /**
     * @brief Set yaw immediately (no interpolation).
     *
     * @param yaw Yaw in degrees (will be wrapped to 0-360).
     * @param cameraState Camera state to modify.
     */
    void setYawImmediate(float yaw, CameraState& cameraState);

    /**
     * @brief Set pitch immediately (no interpolation).
     *
     * @param pitch Pitch in degrees (will be clamped to min/max).
     * @param cameraState Camera state to modify.
     */
    void setPitchImmediate(float pitch, CameraState& cameraState);

    /**
     * @brief Reset orbit/tilt state.
     *
     * Syncs internal state with camera state, clearing any pending animation.
     *
     * @param cameraState Current camera state to sync with.
     */
    void reset(const CameraState& cameraState);

    // ========================================================================
    // State Query
    // ========================================================================

    /**
     * @brief Check if orbit/tilt is currently active.
     * @return true if middle mouse drag is in progress.
     */
    bool isOrbiting() const;

    /**
     * @brief Check if orbit/tilt interpolation is in progress.
     * @return true if interpolating toward target yaw/pitch.
     */
    bool isInterpolating() const;

    /**
     * @brief Get current target yaw.
     * @return Target yaw in degrees.
     */
    float getTargetYaw() const { return m_targetYaw; }

    /**
     * @brief Get current target pitch.
     * @return Target pitch in degrees.
     */
    float getTargetPitch() const { return m_targetPitch; }

    // ========================================================================
    // Configuration
    // ========================================================================

    /**
     * @brief Get current configuration.
     * @return Current orbit configuration.
     */
    const OrbitConfig& getConfig() const { return m_config; }

    /**
     * @brief Set configuration.
     * @param config New orbit configuration.
     */
    void setConfig(const OrbitConfig& config);

    /**
     * @brief Set orbit sensitivity.
     * @param sensitivity Degrees per pixel of horizontal drag.
     */
    void setOrbitSensitivity(float sensitivity);

    /**
     * @brief Set tilt sensitivity.
     * @param sensitivity Degrees per pixel of vertical drag.
     */
    void setTiltSensitivity(float sensitivity);

private:
    /**
     * @brief Wrap yaw to valid range (0-360).
     * @param yaw Yaw angle in degrees.
     * @return Wrapped yaw in range [0, 360).
     */
    float wrapYaw(float yaw) const;

    /**
     * @brief Clamp pitch to valid range.
     * @param pitch Pitch angle in degrees.
     * @return Clamped pitch within min/max bounds.
     */
    float clampPitch(float pitch) const;

    /**
     * @brief Calculate shortest rotation path between two yaw angles.
     *
     * When interpolating yaw, we want to take the shortest path around
     * the circle (e.g., 350->10 should go +20, not -340).
     *
     * @param current Current yaw in degrees.
     * @param target Target yaw in degrees.
     * @return Delta to add to current to reach target via shortest path.
     */
    float calculateYawDelta(float current, float target) const;

    OrbitConfig m_config;

    // Target state for interpolation
    float m_targetYaw = CameraConfig::PRESET_N_YAW;
    float m_targetPitch = CameraConfig::ISOMETRIC_PITCH;

    // Current interpolated state
    float m_currentYaw = CameraConfig::PRESET_N_YAW;
    float m_currentPitch = CameraConfig::ISOMETRIC_PITCH;

    // Input state tracking
    bool m_isOrbiting = false;
    int m_lastDragDeltaX = 0;
    int m_lastDragDeltaY = 0;

    // Threshold for considering interpolation complete
    static constexpr float INTERPOLATION_THRESHOLD = 0.01f;
};

} // namespace sims3000

#endif // SIMS3000_INPUT_ORBITCONTROLLER_H
