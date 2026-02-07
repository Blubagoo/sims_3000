/**
 * @file ViewMode.h
 * @brief Underground view mode state machine with smooth transitions.
 *
 * Provides ViewMode enum and ViewModeController for managing rendering view modes:
 * - Surface: Normal rendering (default)
 * - Underground: Surface ghosted, underground visible
 * - Cutaway: Both surface visible and underground visible
 *
 * Transitions between modes are animated with smooth alpha changes.
 *
 * Usage:
 * @code
 *   ViewModeController controller(layerVisibility);
 *
 *   // Toggle through modes with keybind
 *   controller.cycleMode();  // Surface -> Underground -> Cutaway -> Surface
 *
 *   // Or set specific mode
 *   controller.setMode(ViewMode::Underground);
 *
 *   // Update transitions each frame
 *   controller.update(deltaTime);
 *
 *   // Check if transitioning
 *   if (controller.isTransitioning()) {
 *       // Ghost alpha is being animated
 *   }
 * @endcode
 *
 * Thread safety:
 * - Not thread-safe. Access from render thread only.
 *
 * @see LayerVisibility.h for layer state management
 * @see Easing.h for transition curves
 */

#ifndef SIMS3000_RENDER_VIEW_MODE_H
#define SIMS3000_RENDER_VIEW_MODE_H

#include "sims3000/render/LayerVisibility.h"
#include "sims3000/core/Easing.h"

#include <cstdint>

namespace sims3000 {

/**
 * @enum ViewMode
 * @brief Rendering view modes for the game.
 */
enum class ViewMode : std::uint8_t {
    /// Normal rendering - surface visible, underground hidden.
    /// Default mode for standard gameplay.
    Surface = 0,

    /// Underground view - surface ghosted (transparent), underground visible.
    /// Used for viewing pipes, tunnels, and subsurface infrastructure.
    Underground = 1,

    /// Cutaway view - surface visible AND underground visible.
    /// Shows both surface and underground layers simultaneously.
    Cutaway = 2
};

/**
 * @brief Get human-readable name for a view mode.
 * @param mode The view mode to name.
 * @return Null-terminated string name of the mode.
 */
constexpr const char* getViewModeName(ViewMode mode) {
    switch (mode) {
        case ViewMode::Surface:     return "Surface";
        case ViewMode::Underground: return "Underground";
        case ViewMode::Cutaway:     return "Cutaway";
        default:                    return "Unknown";
    }
}

/**
 * @brief Check if a view mode value is valid.
 * @param mode The mode to validate.
 * @return True if the mode is a valid ViewMode enum value.
 */
constexpr bool isValidViewMode(ViewMode mode) {
    return static_cast<std::uint8_t>(mode) <= 2;
}

/**
 * @brief Total number of view modes.
 */
constexpr std::size_t VIEW_MODE_COUNT = 3;

/**
 * @struct ViewModeConfig
 * @brief Configuration for view mode transitions.
 */
struct ViewModeConfig {
    /// Duration of mode transitions in seconds.
    /// Default: 0.25 (250ms, per UI animation patterns).
    float transitionDuration = 0.25f;

    /// Ghost alpha for underground view mode (surface layers).
    /// Default: 0.3 (30% opacity).
    float undergroundGhostAlpha = 0.3f;

    /// Ghost alpha for cutaway view mode (underground layer).
    /// Default: 0.7 (70% opacity - more visible than underground mode surface).
    float cutawayUndergroundAlpha = 0.7f;

    /// Easing function for transitions.
    /// Default: EaseOutCubic (smooth deceleration).
    Easing::EasingType transitionEasing = Easing::EasingType::EaseOutCubic;
};

/**
 * @class ViewModeController
 * @brief Controls view mode state and transitions.
 *
 * Manages the ViewMode state machine and animates smooth transitions
 * between modes by interpolating LayerVisibility ghost alpha values.
 *
 * Design rationale:
 * - Owns a reference to LayerVisibility for direct state manipulation
 * - Tracks transition progress for smooth animations
 * - Uses easing functions for natural-feeling transitions
 */
class ViewModeController {
public:
    /**
     * Create view mode controller with default config.
     * @param visibility Reference to LayerVisibility to control.
     */
    explicit ViewModeController(LayerVisibility& visibility) noexcept;

    /**
     * Create view mode controller with custom config.
     * @param visibility Reference to LayerVisibility to control.
     * @param config Configuration options.
     */
    ViewModeController(LayerVisibility& visibility, const ViewModeConfig& config) noexcept;

    ~ViewModeController() = default;

    // Non-copyable (holds reference)
    ViewModeController(const ViewModeController&) = delete;
    ViewModeController& operator=(const ViewModeController&) = delete;

    // Movable
    ViewModeController(ViewModeController&&) = default;
    ViewModeController& operator=(ViewModeController&&) = default;

    // =========================================================================
    // Mode Control
    // =========================================================================

    /**
     * Set the target view mode.
     *
     * If different from current mode, begins a smooth transition.
     * If already in target mode, does nothing.
     *
     * @param mode The target view mode.
     */
    void setMode(ViewMode mode);

    /**
     * Get the current view mode (target mode during transitions).
     * @return Current view mode.
     */
    ViewMode getMode() const { return m_currentMode; }

    /**
     * Get the mode we're transitioning from (if transitioning).
     * @return Previous mode, or current mode if not transitioning.
     */
    ViewMode getPreviousMode() const { return m_previousMode; }

    /**
     * Cycle to the next view mode.
     *
     * Order: Surface -> Underground -> Cutaway -> Surface
     */
    void cycleMode();

    /**
     * Cycle to the previous view mode.
     *
     * Order: Surface -> Cutaway -> Underground -> Surface
     */
    void cycleModeReverse();

    /**
     * Return to Surface mode.
     *
     * Convenience method for resetting to default view.
     */
    void resetToSurface();

    // =========================================================================
    // Transition Management
    // =========================================================================

    /**
     * Update the transition animation.
     *
     * Call this every frame to advance transition progress.
     *
     * @param deltaTime Time since last frame in seconds.
     */
    void update(float deltaTime);

    /**
     * Check if currently transitioning between modes.
     * @return True if a transition is in progress.
     */
    bool isTransitioning() const { return m_transitioning; }

    /**
     * Get the transition progress.
     * @return Progress from 0.0 (start) to 1.0 (complete).
     */
    float getTransitionProgress() const { return m_transitionProgress; }

    /**
     * Get the transition progress with easing applied.
     * @return Eased progress from 0.0 to 1.0.
     */
    float getEasedProgress() const;

    /**
     * Skip to the end of the current transition immediately.
     *
     * Useful for testing or when instant mode changes are needed.
     */
    void completeTransition();

    /**
     * Cancel the current transition and revert to previous mode.
     *
     * If not transitioning, does nothing.
     */
    void cancelTransition();

    // =========================================================================
    // Configuration
    // =========================================================================

    /**
     * Get the current configuration.
     * @return Current ViewModeConfig.
     */
    const ViewModeConfig& getConfig() const { return m_config; }

    /**
     * Set configuration.
     * @param config New configuration to apply.
     */
    void setConfig(const ViewModeConfig& config);

    /**
     * Set transition duration.
     * @param duration Duration in seconds (clamped to >= 0).
     */
    void setTransitionDuration(float duration);

private:
    /**
     * Apply layer states for a specific view mode.
     * @param mode The mode to apply.
     * @param ghostAlpha The alpha value to use for ghosted layers (0-1).
     */
    void applyModeStates(ViewMode mode, float ghostAlpha);

    /**
     * Calculate the target ghost alpha for the current mode.
     * @return Target ghost alpha value.
     */
    float getTargetGhostAlpha() const;

    /**
     * Calculate the source ghost alpha for the previous mode.
     * @return Source ghost alpha value.
     */
    float getSourceGhostAlpha() const;

    /// Reference to the LayerVisibility we control.
    LayerVisibility& m_visibility;

    /// Current (target) view mode.
    ViewMode m_currentMode = ViewMode::Surface;

    /// Previous view mode (for transitions).
    ViewMode m_previousMode = ViewMode::Surface;

    /// Configuration options.
    ViewModeConfig m_config;

    /// Whether a transition is in progress.
    bool m_transitioning = false;

    /// Current transition progress [0, 1].
    float m_transitionProgress = 0.0f;
};

} // namespace sims3000

#endif // SIMS3000_RENDER_VIEW_MODE_H
