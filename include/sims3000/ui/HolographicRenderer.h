/**
 * @file HolographicRenderer.h
 * @brief Holographic renderer -- stub implementation for testing.
 *
 * Implements the UIRenderer interface with no-op draw methods that
 * simply count calls, plus active holographic effect methods (scanlines,
 * glow).  This allows the widget tree and layout logic to be tested
 * without a live GPU context.
 *
 * The real SDL_GPU-backed holographic rendering will replace the method
 * bodies during render-pipeline integration.
 *
 * Usage:
 * @code
 *   HolographicRenderer renderer;
 *   renderer.begin_glow_effect(0.8f);
 *   renderer.draw_panel({10, 10, 300, 200}, "Budget", true);
 *   renderer.draw_scanlines({10, 10, 300, 200}, 0.5f);
 *   renderer.end_glow_effect();
 *   assert(renderer.get_stats().total() == 4);
 *   assert(!renderer.is_glow_active());
 *   renderer.reset_stats();
 * @endcode
 *
 * Resource ownership:
 * - Holds a UISkin value (create_holo) -- no managed GPU resources
 */

#ifndef SIMS3000_UI_HOLOGRAPHICRENDERER_H
#define SIMS3000_UI_HOLOGRAPHICRENDERER_H

#include "sims3000/ui/UIRenderer.h"
#include "sims3000/ui/UISkin.h"
#include "sims3000/ui/Widget.h" // Rect, Color

#include <cstdint>
#include <string>

namespace sims3000 {
namespace ui {

/**
 * @class HolographicRenderer
 * @brief Holographic renderer implementation.
 *
 * This is initially a stub that records draw calls for testing.
 * Unlike ClassicRenderer, the holographic effect methods (scanlines,
 * glow) are active and track state rather than being no-ops.
 * The real SDL_GPU-backed implementation will be added during integration.
 */
class HolographicRenderer : public UIRenderer {
public:
    HolographicRenderer();
    ~HolographicRenderer() override = default;

    // =========================================================================
    // UIRenderer interface
    // =========================================================================

    void draw_panel(const Rect& bounds, const std::string& title, bool closable) override;
    void draw_panel_background(const Rect& bounds) override;
    void draw_button(const Rect& bounds, const std::string& text, ButtonState state) override;
    void draw_icon_button(const Rect& bounds, TextureHandle icon, ButtonState state) override;
    void draw_text(const std::string& text, float x, float y, FontSize size, const Color& color) override;
    void draw_rect(const Rect& bounds, const Color& fill, const Color& border) override;
    void draw_progress_bar(const Rect& bounds, float progress, const Color& fill_color) override;
    void draw_slider(const Rect& bounds, float value, float min_val, float max_val) override;
    void draw_icon(const Rect& bounds, TextureHandle texture, const Color& tint) override;

    // =========================================================================
    // Holographic effects (active in this renderer)
    // =========================================================================

    /**
     * Draw CRT-style scanlines over a region.
     * @param bounds  Area to apply the effect
     * @param opacity Strength of the scanline effect (0.0 - 1.0)
     */
    void draw_scanlines(const Rect& bounds, float opacity) override;

    /**
     * Begin an outer-glow effect around subsequent draw calls.
     * Sets glow state to active and stores the intensity.
     * @param intensity Glow strength (0.0 = none, 1.0 = full)
     */
    void begin_glow_effect(float intensity) override;

    /**
     * End an outer-glow effect started by begin_glow_effect().
     * Sets glow state to inactive.
     */
    void end_glow_effect() override;

    // =========================================================================
    // Diagnostics -- count draw calls for testing
    // =========================================================================

    /**
     * @struct DrawStats
     * @brief Cumulative draw-call counters for each widget category.
     */
    struct DrawStats {
        uint32_t panel_calls        = 0; ///< draw_panel + draw_panel_background
        uint32_t button_calls       = 0; ///< draw_button + draw_icon_button
        uint32_t text_calls         = 0; ///< draw_text
        uint32_t rect_calls         = 0; ///< draw_rect
        uint32_t progress_bar_calls = 0; ///< draw_progress_bar
        uint32_t slider_calls       = 0; ///< draw_slider
        uint32_t icon_calls         = 0; ///< draw_icon
        uint32_t scanline_calls     = 0; ///< draw_scanlines
        uint32_t glow_begin_calls   = 0; ///< begin_glow_effect
        uint32_t glow_end_calls     = 0; ///< end_glow_effect

        /** @return Sum of all individual counters. */
        uint32_t total() const;
    };

    /**
     * Retrieve the current draw-call statistics.
     * @return Const reference to the internal DrawStats
     */
    const DrawStats& get_stats() const;

    /**
     * Reset all draw-call counters to zero.
     */
    void reset_stats();

    // =========================================================================
    // Glow state (for testing)
    // =========================================================================

    /**
     * @return true if a glow effect is currently active
     *         (begin_glow_effect called without matching end_glow_effect)
     */
    bool is_glow_active() const;

    /**
     * @return The intensity passed to the most recent begin_glow_effect(),
     *         or 0.0 if glow is not active
     */
    float get_glow_intensity() const;

private:
    DrawStats m_stats;
    UISkin    m_skin;
    bool      m_glow_active    = false;
    float     m_glow_intensity = 0.0f;
};

} // namespace ui
} // namespace sims3000

#endif // SIMS3000_UI_HOLOGRAPHICRENDERER_H
