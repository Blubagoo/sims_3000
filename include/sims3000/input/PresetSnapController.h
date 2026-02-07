/**
 * @file PresetSnapController.h
 * @brief Controller for isometric preset snap views.
 *
 * Handles Q/E key input to rotate camera between four cardinal isometric
 * presets (N/E/S/W at 45 degree yaw increments, ~35.264 pitch).
 * Uses CameraAnimator.snapToPreset() for smooth transitions.
 *
 * Preset rotation order:
 * - Q (clockwise):        N -> E -> S -> W -> N
 * - E (counterclockwise): N -> W -> S -> E -> N
 *
 * Resource ownership: None (pure logic, no GPU/SDL resources).
 */

#ifndef SIMS3000_INPUT_PRESETSNAPCONTROLLER_H
#define SIMS3000_INPUT_PRESETSNAPCONTROLLER_H

#include "sims3000/render/CameraState.h"
#include <cstdint>

namespace sims3000 {

// Forward declarations
class InputSystem;
class CameraAnimator;

// ============================================================================
// Preset Indicator (for UI display)
// ============================================================================

/**
 * @struct PresetIndicator
 * @brief Data structure for UI to display current cardinal direction indicator.
 *
 * Provides all information needed by the UISystem (Epic 12) to render
 * a visual indicator showing the current camera preset/direction.
 */
struct PresetIndicator {
    CameraMode currentPreset;       ///< Current or target preset
    const char* cardinalName;       ///< "N", "E", "S", "W", or "Free"
    float yawDegrees;               ///< 0-360 for compass rotation
    bool isAnimating;               ///< True during snap transitions
    float animationProgress;        ///< 0-1 during animation
};

// ============================================================================
// Preset Snap Configuration
// ============================================================================

/**
 * @struct PresetSnapConfig
 * @brief Configuration for preset snap behavior.
 */
struct PresetSnapConfig {
    // Animation duration (0.3-0.5 seconds per acceptance criteria)
    float snapDuration = 0.4f;  ///< Duration for preset transition animation

    /**
     * @brief Get default configuration.
     */
    static PresetSnapConfig defaultConfig();
};

// ============================================================================
// Preset Snap Controller
// ============================================================================

/**
 * @class PresetSnapController
 * @brief Controls isometric preset snap views with Q/E key rotation.
 *
 * The controller handles:
 * 1. **Q key** - Snap to next clockwise preset (N->E->S->W)
 * 2. **E key** - Snap to next counterclockwise preset (N->W->S->E)
 * 3. **Smooth transitions** - Uses CameraAnimator for 0.3-0.5s ease-in-out
 *
 * When orbit/tilt input is detected (via CameraAnimator being interrupted),
 * the camera enters free mode. Q/E keys bring it back to preset mode.
 *
 * Usage:
 * @code
 *   PresetSnapController presetController;
 *   CameraAnimator animator;
 *
 *   // In input processing:
 *   presetController.handleInput(input, cameraState, animator);
 * @endcode
 */
class PresetSnapController {
public:
    /**
     * @brief Construct preset snap controller with default configuration.
     */
    PresetSnapController();

    /**
     * @brief Construct preset snap controller with custom configuration.
     * @param config Preset snap configuration.
     */
    explicit PresetSnapController(const PresetSnapConfig& config);

    ~PresetSnapController() = default;

    // Non-copyable (stateful controller)
    PresetSnapController(const PresetSnapController&) = delete;
    PresetSnapController& operator=(const PresetSnapController&) = delete;

    // Movable
    PresetSnapController(PresetSnapController&&) = default;
    PresetSnapController& operator=(PresetSnapController&&) = default;

    // ========================================================================
    // Input Handling
    // ========================================================================

    /**
     * @brief Handle input and trigger preset snapping.
     *
     * Reads Q/E key presses and initiates preset snap transitions.
     * Q rotates clockwise (N->E->S->W), E rotates counterclockwise (N->W->S->E).
     *
     * @param input Input system for key state queries.
     * @param cameraState Current camera state.
     * @param animator Camera animator to trigger snap transitions.
     * @return true if a preset snap was initiated.
     */
    bool handleInput(const InputSystem& input, CameraState& cameraState, CameraAnimator& animator);

    /**
     * @brief Snap to next clockwise preset.
     *
     * Rotation order: N -> E -> S -> W -> N
     *
     * @param cameraState Current camera state.
     * @param animator Camera animator to trigger snap transition.
     */
    void snapClockwise(CameraState& cameraState, CameraAnimator& animator);

    /**
     * @brief Snap to next counterclockwise preset.
     *
     * Rotation order: N -> W -> S -> E -> N
     *
     * @param cameraState Current camera state.
     * @param animator Camera animator to trigger snap transition.
     */
    void snapCounterclockwise(CameraState& cameraState, CameraAnimator& animator);

    /**
     * @brief Snap directly to a specific preset.
     *
     * @param preset Target preset mode (Preset_N, Preset_E, Preset_S, Preset_W).
     * @param cameraState Current camera state.
     * @param animator Camera animator to trigger snap transition.
     */
    void snapToPreset(CameraMode preset, CameraState& cameraState, CameraAnimator& animator);

    // ========================================================================
    // State Query
    // ========================================================================

    /**
     * @brief Get the current target preset.
     *
     * Returns the preset the camera is currently in or animating toward.
     * If in free mode, returns the last preset before entering free mode.
     *
     * @return Current target preset.
     */
    CameraMode getCurrentPreset() const { return m_currentPreset; }

    /**
     * @brief Get the preset indicator data for UI display.
     *
     * Returns a PresetIndicator struct containing all data needed to
     * render a cardinal direction indicator in the UI (Epic 12).
     *
     * @param cameraState Current camera state.
     * @param animator Camera animator to check animation progress.
     * @return PresetIndicator with current direction and animation state.
     */
    PresetIndicator getPresetIndicator(const CameraState& cameraState, const CameraAnimator& animator) const;

    /**
     * @brief Check if camera is currently in a preset mode (not free/animating).
     * @param cameraState Camera state to check.
     * @return true if camera is settled in a preset.
     */
    static bool isInPresetMode(const CameraState& cameraState);

    /**
     * @brief Get the preset closest to the current camera yaw.
     *
     * Useful for determining which preset to return to from free mode.
     *
     * @param cameraState Camera state to check.
     * @return Closest preset mode based on current yaw.
     */
    static CameraMode getClosestPreset(const CameraState& cameraState);

    // ========================================================================
    // Preset Rotation Helpers
    // ========================================================================

    /**
     * @brief Get the next clockwise preset from a given preset.
     * @param current Current preset mode.
     * @return Next preset in clockwise order.
     */
    static CameraMode getNextClockwise(CameraMode current);

    /**
     * @brief Get the next counterclockwise preset from a given preset.
     * @param current Current preset mode.
     * @return Next preset in counterclockwise order.
     */
    static CameraMode getNextCounterclockwise(CameraMode current);

    /**
     * @brief Get the cardinal name for a preset mode.
     *
     * @param mode Camera mode.
     * @return "N", "E", "S", "W" for presets, or "Free" for non-preset modes.
     */
    static const char* getCardinalName(CameraMode mode);

    // ========================================================================
    // Configuration
    // ========================================================================

    /**
     * @brief Get current configuration.
     * @return Current preset snap configuration.
     */
    const PresetSnapConfig& getConfig() const { return m_config; }

    /**
     * @brief Set configuration.
     * @param config New preset snap configuration.
     */
    void setConfig(const PresetSnapConfig& config);

    /**
     * @brief Set snap animation duration.
     * @param duration Duration in seconds (should be 0.3-0.5s per spec).
     */
    void setSnapDuration(float duration);

private:
    PresetSnapConfig m_config;

    // Track current/target preset for rotation calculations
    CameraMode m_currentPreset = CameraMode::Preset_N;
};

} // namespace sims3000

#endif // SIMS3000_INPUT_PRESETSNAPCONTROLLER_H
