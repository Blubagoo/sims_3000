/**
 * @file ClassicRenderer.cpp
 * @brief Stub implementation of the ClassicRenderer.
 *
 * Every draw method simply increments its respective stats counter.
 * The real SDL_GPU draw calls will replace these bodies during
 * render-pipeline integration.
 */

#include "sims3000/ui/ClassicRenderer.h"

namespace sims3000 {
namespace ui {

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

ClassicRenderer::ClassicRenderer()
    : m_stats{}
    , m_skin{UISkin::create_legacy()}
{
}

// ---------------------------------------------------------------------------
// UIRenderer interface -- stub implementations (increment counters only)
// ---------------------------------------------------------------------------

void ClassicRenderer::draw_panel(const Rect& /*bounds*/,
                                 const std::string& /*title*/,
                                 bool /*closable*/)
{
    ++m_stats.panel_calls;
}

void ClassicRenderer::draw_panel_background(const Rect& /*bounds*/)
{
    ++m_stats.panel_calls;
}

void ClassicRenderer::draw_button(const Rect& /*bounds*/,
                                  const std::string& /*text*/,
                                  ButtonState /*state*/)
{
    ++m_stats.button_calls;
}

void ClassicRenderer::draw_icon_button(const Rect& /*bounds*/,
                                       TextureHandle /*icon*/,
                                       ButtonState /*state*/)
{
    ++m_stats.button_calls;
}

void ClassicRenderer::draw_text(const std::string& /*text*/,
                                float /*x*/,
                                float /*y*/,
                                FontSize /*size*/,
                                const Color& /*color*/)
{
    ++m_stats.text_calls;
}

void ClassicRenderer::draw_rect(const Rect& /*bounds*/,
                                const Color& /*fill*/,
                                const Color& /*border*/)
{
    ++m_stats.rect_calls;
}

void ClassicRenderer::draw_progress_bar(const Rect& /*bounds*/,
                                        float /*progress*/,
                                        const Color& /*fill_color*/)
{
    ++m_stats.progress_bar_calls;
}

void ClassicRenderer::draw_slider(const Rect& /*bounds*/,
                                  float /*value*/,
                                  float /*min_val*/,
                                  float /*max_val*/)
{
    ++m_stats.slider_calls;
}

void ClassicRenderer::draw_icon(const Rect& /*bounds*/,
                                TextureHandle /*texture*/,
                                const Color& /*tint*/)
{
    ++m_stats.icon_calls;
}

// ---------------------------------------------------------------------------
// Diagnostics
// ---------------------------------------------------------------------------

uint32_t ClassicRenderer::DrawStats::total() const
{
    return panel_calls
         + button_calls
         + text_calls
         + rect_calls
         + progress_bar_calls
         + slider_calls
         + icon_calls;
}

const ClassicRenderer::DrawStats& ClassicRenderer::get_stats() const
{
    return m_stats;
}

void ClassicRenderer::reset_stats()
{
    m_stats = DrawStats{};
}

} // namespace ui
} // namespace sims3000
