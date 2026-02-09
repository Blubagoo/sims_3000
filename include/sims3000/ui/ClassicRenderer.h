/**
 * @file ClassicRenderer.h
 * @brief Classic (Legacy) renderer -- stub implementation for testing.
 *
 * Implements the UIRenderer interface with no-op draw methods that
 * simply count calls.  This allows the widget tree and layout logic
 * to be tested without a live GPU context.
 *
 * The real SDL_GPU-backed rendering will replace the method bodies
 * during render-pipeline integration.
 *
 * Usage:
 * @code
 *   ClassicRenderer renderer;
 *   renderer.draw_panel({10, 10, 300, 200}, "Budget", true);
 *   renderer.draw_button({20, 170, 80, 30}, "OK", ButtonState::Normal);
 *   assert(renderer.get_stats().total() == 2);
 *   renderer.reset_stats();
 * @endcode
 *
 * Resource ownership:
 * - Holds a UISkin value (create_legacy) -- no managed GPU resources
 */

#ifndef SIMS3000_UI_CLASSICRENDERER_H
#define SIMS3000_UI_CLASSICRENDERER_H

#include "sims3000/ui/UIRenderer.h"
#include "sims3000/ui/UISkin.h"
#include "sims3000/ui/Widget.h" // Rect, Color

#include <cstdint>
#include <string>

namespace sims3000 {
namespace ui {

/**
 * @class ClassicRenderer
 * @brief Classic (Legacy) renderer implementation.
 *
 * This is initially a stub that records draw calls for testing.
 * The real SDL_GPU-backed implementation will be added during integration.
 */
class ClassicRenderer : public UIRenderer {
public:
    ClassicRenderer();
    ~ClassicRenderer() override = default;

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

private:
    DrawStats m_stats;
    UISkin    m_skin;
};

} // namespace ui
} // namespace sims3000

#endif // SIMS3000_UI_CLASSICRENDERER_H
