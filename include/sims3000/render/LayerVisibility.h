/**
 * @file LayerVisibility.h
 * @brief Layer visibility management for render layer toggling.
 *
 * Provides visibility state control for each RenderLayer. Layers can be:
 * - Visible: Rendered normally with full opacity
 * - Hidden: Skipped entirely during rendering (best performance)
 * - Ghost: Rendered with reduced alpha (transparent overlay)
 *
 * Primary use cases:
 * - Underground view mode: Ghost surface layers, show underground layer
 * - Debug visualization: Toggle specific layers on/off
 * - Data overlay toggling: Show/hide heat maps and coverage zones
 *
 * Usage:
 * @code
 *   LayerVisibility visibility;
 *
 *   // Normal operation - all layers visible by default
 *   if (visibility.shouldRender(RenderLayer::Buildings)) {
 *       // Render buildings normally
 *   }
 *
 *   // Enable underground view
 *   visibility.setLayerVisibility(RenderLayer::Terrain, LayerState::Ghost);
 *   visibility.setLayerVisibility(RenderLayer::Buildings, LayerState::Ghost);
 *   visibility.setLayerVisibility(RenderLayer::Underground, LayerState::Visible);
 *
 *   // Check if layer needs transparent rendering
 *   if (visibility.getState(RenderLayer::Buildings) == LayerState::Ghost) {
 *       float alpha = visibility.getGhostAlpha();
 *       // Render with reduced alpha
 *   }
 *
 *   // Reset to normal view
 *   visibility.resetAll();
 * @endcode
 *
 * Thread safety:
 * - Not thread-safe. Access from render thread only.
 *
 * @see RenderLayer.h for layer definitions
 * @see TransparentRenderQueue.h for ghost rendering implementation
 */

#ifndef SIMS3000_RENDER_LAYER_VISIBILITY_H
#define SIMS3000_RENDER_LAYER_VISIBILITY_H

#include "sims3000/render/RenderLayer.h"

#include <array>
#include <cstdint>

namespace sims3000 {

/**
 * @enum LayerState
 * @brief Visibility state for a render layer.
 */
enum class LayerState : std::uint8_t {
    /// Layer is rendered normally with full opacity.
    /// This is the default state for all layers.
    Visible = 0,

    /// Layer is completely skipped during rendering.
    /// No draw calls are issued for entities in this layer.
    /// Best performance for layers that don't need to be shown.
    Hidden = 1,

    /// Layer is rendered with reduced alpha (transparent).
    /// Used for underground view mode where surface layers are ghosted.
    /// Requires transparent pipeline with alpha blending.
    Ghost = 2
};

/**
 * @brief Get human-readable name for a layer state.
 * @param state The layer state to name.
 * @return Null-terminated string name of the state.
 */
constexpr const char* getLayerStateName(LayerState state) {
    switch (state) {
        case LayerState::Visible: return "Visible";
        case LayerState::Hidden:  return "Hidden";
        case LayerState::Ghost:   return "Ghost";
        default:                  return "Unknown";
    }
}

/**
 * @brief Check if a layer state value is valid.
 * @param state The state to validate.
 * @return True if the state is a valid LayerState enum value.
 */
constexpr bool isValidLayerState(LayerState state) {
    return static_cast<std::uint8_t>(state) <= 2;
}

/**
 * @struct LayerVisibilityConfig
 * @brief Configuration for layer visibility behavior.
 */
struct LayerVisibilityConfig {
    /// Alpha value for ghost (transparent) layers.
    /// Range: 0.0 (fully transparent) to 1.0 (fully opaque).
    /// Default: 0.3 (30% opacity, matching TransparentRenderQueue ghost config).
    float ghostAlpha = 0.3f;

    /// Whether to allow ghosting of opaque layers.
    /// When false, setting an opaque layer (Terrain, Roads, Buildings, Units)
    /// to Ghost will be treated as Visible instead.
    /// Default: true (allow ghosting any layer).
    bool allowOpaqueGhost = true;
};

/**
 * @class LayerVisibility
 * @brief Manages visibility state for all render layers.
 *
 * Maintains per-layer visibility state and provides query methods
 * for the rendering system to determine how each layer should be rendered.
 *
 * Design rationale:
 * - Uses std::array for O(1) access by layer index
 * - Small memory footprint (9 bytes for states + config)
 * - Constexpr-friendly where possible
 * - No heap allocations
 */
class LayerVisibility {
public:
    /**
     * Create layer visibility manager with default config.
     * All layers start in Visible state.
     */
    LayerVisibility() noexcept;

    /**
     * Create layer visibility manager with custom config.
     * All layers start in Visible state.
     * @param config Configuration options
     */
    explicit LayerVisibility(const LayerVisibilityConfig& config) noexcept;

    ~LayerVisibility() = default;

    // Copyable and movable
    LayerVisibility(const LayerVisibility&) = default;
    LayerVisibility& operator=(const LayerVisibility&) = default;
    LayerVisibility(LayerVisibility&&) = default;
    LayerVisibility& operator=(LayerVisibility&&) = default;

    // =========================================================================
    // Core API
    // =========================================================================

    /**
     * Set the visibility state for a render layer.
     *
     * This is the primary API for controlling layer visibility.
     *
     * @param layer The render layer to modify.
     * @param state The new visibility state.
     *
     * Behavior notes:
     * - Invalid layers are ignored (logged in debug builds).
     * - If allowOpaqueGhost is false and layer is opaque,
     *   Ghost state is converted to Visible.
     */
    void setLayerVisibility(RenderLayer layer, LayerState state);

    /**
     * Get the current visibility state for a render layer.
     * @param layer The render layer to query.
     * @return Current LayerState, or Visible for invalid layers.
     */
    LayerState getState(RenderLayer layer) const;

    /**
     * Check if a layer should be rendered (not Hidden).
     *
     * Convenience method for render loop skip logic.
     *
     * @param layer The render layer to check.
     * @return True if layer is Visible or Ghost, false if Hidden.
     */
    bool shouldRender(RenderLayer layer) const;

    /**
     * Check if a layer requires ghost (transparent) rendering.
     * @param layer The render layer to check.
     * @return True if layer state is Ghost.
     */
    bool isGhost(RenderLayer layer) const;

    /**
     * Check if a layer is fully visible (not Ghost or Hidden).
     * @param layer The render layer to check.
     * @return True if layer state is Visible.
     */
    bool isVisible(RenderLayer layer) const;

    /**
     * Check if a layer is hidden (not rendered at all).
     * @param layer The render layer to check.
     * @return True if layer state is Hidden.
     */
    bool isHidden(RenderLayer layer) const;

    // =========================================================================
    // Bulk Operations
    // =========================================================================

    /**
     * Reset all layers to Visible state.
     *
     * Use this when exiting special view modes (underground, debug).
     */
    void resetAll();

    /**
     * Set all layers to the same state.
     * @param state The state to apply to all layers.
     */
    void setAllLayers(LayerState state);

    /**
     * Set a range of layers to the same state.
     *
     * Layers are set from `first` through `last` inclusive.
     *
     * @param first First layer in range.
     * @param last Last layer in range (inclusive).
     * @param state The state to apply.
     */
    void setLayerRange(RenderLayer first, RenderLayer last, LayerState state);

    // =========================================================================
    // Configuration
    // =========================================================================

    /**
     * Get the alpha value for ghost layers.
     * @return Ghost alpha value (0.0 to 1.0).
     */
    float getGhostAlpha() const { return m_config.ghostAlpha; }

    /**
     * Set the alpha value for ghost layers.
     * @param alpha Ghost alpha value (clamped to 0.0 - 1.0).
     */
    void setGhostAlpha(float alpha);

    /**
     * Get the current configuration.
     * @return Current LayerVisibilityConfig.
     */
    const LayerVisibilityConfig& getConfig() const { return m_config; }

    /**
     * Set configuration.
     * @param config New configuration to apply.
     */
    void setConfig(const LayerVisibilityConfig& config);

    // =========================================================================
    // Preset View Modes
    // =========================================================================

    /**
     * Enable underground view mode.
     *
     * This ghosts surface layers (Terrain, Buildings, Roads, Units, Effects)
     * and makes Underground layer visible.
     *
     * Equivalent to:
     * @code
     *   setLayerVisibility(RenderLayer::Underground, LayerState::Visible);
     *   setLayerVisibility(RenderLayer::Terrain, LayerState::Ghost);
     *   setLayerVisibility(RenderLayer::Roads, LayerState::Ghost);
     *   setLayerVisibility(RenderLayer::Buildings, LayerState::Ghost);
     *   setLayerVisibility(RenderLayer::Units, LayerState::Ghost);
     *   // Water, Effects, DataOverlay, UIWorld unchanged
     * @endcode
     */
    void enableUndergroundView();

    /**
     * Disable underground view mode.
     *
     * Resets Underground layer to Hidden (its default for normal view)
     * and surface layers back to Visible.
     */
    void disableUndergroundView();

    /**
     * Check if underground view mode is currently active.
     *
     * Underground view is considered active when:
     * - Underground layer is Visible or Ghost
     * - AND at least one surface layer (Terrain, Buildings) is Ghost
     *
     * @return True if in underground view mode.
     */
    bool isUndergroundViewActive() const;

    // =========================================================================
    // Statistics
    // =========================================================================

    /**
     * Count layers in each state.
     * @param visibleCount Output: number of Visible layers.
     * @param hiddenCount Output: number of Hidden layers.
     * @param ghostCount Output: number of Ghost layers.
     */
    void countStates(
        std::size_t& visibleCount,
        std::size_t& hiddenCount,
        std::size_t& ghostCount) const;

private:
    /// Visibility state for each layer, indexed by RenderLayer enum value.
    std::array<LayerState, RENDER_LAYER_COUNT> m_states;

    /// Configuration options.
    LayerVisibilityConfig m_config;
};

} // namespace sims3000

#endif // SIMS3000_RENDER_LAYER_VISIBILITY_H
