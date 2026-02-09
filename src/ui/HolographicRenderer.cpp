/**
 * @file HolographicRenderer.cpp
 * @brief Stub implementation of the HolographicRenderer.
 *
 * Every draw method simply increments its respective stats counter.
 * The holographic effect methods additionally manage glow state.
 * The real SDL_GPU draw calls will replace these bodies during
 * render-pipeline integration.
 */

#include "sims3000/ui/HolographicRenderer.h"

namespace sims3000 {
namespace ui {

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

HolographicRenderer::HolographicRenderer()
    : m_stats{}
    , m_skin{UISkin::create_holo()}
{
}

// ---------------------------------------------------------------------------
// UIRenderer interface -- stub implementations (increment counters only)
// ---------------------------------------------------------------------------

void HolographicRenderer::draw_panel(const Rect& /*bounds*/,
                                     const std::string& /*title*/,
                                     bool /*closable*/)
{
    ++m_stats.panel_calls;
}

void HolographicRenderer::draw_panel_background(const Rect& /*bounds*/)
{
    ++m_stats.panel_calls;
}

void HolographicRenderer::draw_button(const Rect& /*bounds*/,
                                      const std::string& /*text*/,
                                      ButtonState /*state*/)
{
    ++m_stats.button_calls;
}

void HolographicRenderer::draw_icon_button(const Rect& /*bounds*/,
                                           TextureHandle /*icon*/,
                                           ButtonState /*state*/)
{
    ++m_stats.button_calls;
}

void HolographicRenderer::draw_text(const std::string& /*text*/,
                                    float /*x*/,
                                    float /*y*/,
                                    FontSize /*size*/,
                                    const Color& /*color*/)
{
    ++m_stats.text_calls;
}

void HolographicRenderer::draw_rect(const Rect& /*bounds*/,
                                    const Color& /*fill*/,
                                    const Color& /*border*/)
{
    ++m_stats.rect_calls;
}

void HolographicRenderer::draw_progress_bar(const Rect& /*bounds*/,
                                            float /*progress*/,
                                            const Color& /*fill_color*/)
{
    ++m_stats.progress_bar_calls;
}

void HolographicRenderer::draw_slider(const Rect& /*bounds*/,
                                      float /*value*/,
                                      float /*min_val*/,
                                      float /*max_val*/)
{
    ++m_stats.slider_calls;
}

void HolographicRenderer::draw_icon(const Rect& /*bounds*/,
                                    TextureHandle /*texture*/,
                                    const Color& /*tint*/)
{
    ++m_stats.icon_calls;
}

// ---------------------------------------------------------------------------
// Holographic effects (active in this renderer)
// ---------------------------------------------------------------------------

void HolographicRenderer::draw_scanlines(const Rect& /*bounds*/,
                                         float /*opacity*/)
{
    ++m_stats.scanline_calls;
}

void HolographicRenderer::begin_glow_effect(float intensity)
{
    m_glow_active = true;
    m_glow_intensity = intensity;
    ++m_stats.glow_begin_calls;
}

void HolographicRenderer::end_glow_effect()
{
    m_glow_active = false;
    ++m_stats.glow_end_calls;
}

// ---------------------------------------------------------------------------
// Diagnostics
// ---------------------------------------------------------------------------

uint32_t HolographicRenderer::DrawStats::total() const
{
    return panel_calls
         + button_calls
         + text_calls
         + rect_calls
         + progress_bar_calls
         + slider_calls
         + icon_calls
         + scanline_calls
         + glow_begin_calls
         + glow_end_calls;
}

const HolographicRenderer::DrawStats& HolographicRenderer::get_stats() const
{
    return m_stats;
}

void HolographicRenderer::reset_stats()
{
    m_stats = DrawStats{};
}

// ---------------------------------------------------------------------------
// Glow state
// ---------------------------------------------------------------------------

bool HolographicRenderer::is_glow_active() const
{
    return m_glow_active;
}

float HolographicRenderer::get_glow_intensity() const
{
    return m_glow_intensity;
}

} // namespace ui
} // namespace sims3000
