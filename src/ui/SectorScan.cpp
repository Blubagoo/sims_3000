/**
 * @file SectorScan.cpp
 * @brief Implementation of SectorScan: CPU-generated minimap widget.
 *
 * Generates an RGBA8 pixel buffer from the game map via IMinimapDataProvider,
 * renders a background rectangle and viewport indicator, and converts
 * mouse clicks to world-space navigation events.
 */

#include "sims3000/ui/SectorScan.h"

#include <algorithm>

namespace sims3000 {
namespace ui {

// =========================================================================
// Construction
// =========================================================================

SectorScan::SectorScan() {
    bounds = {0.0f, 0.0f, DEFAULT_SIZE, DEFAULT_SIZE};
}

// =========================================================================
// Data provider
// =========================================================================

void SectorScan::set_data_provider(IMinimapDataProvider* provider) {
    m_provider = provider;
    invalidate();
}

// =========================================================================
// Viewport indicator
// =========================================================================

void SectorScan::set_viewport(const ViewportIndicator& vp) {
    m_viewport = vp;
}

// =========================================================================
// Dirty / invalidation
// =========================================================================

void SectorScan::invalidate() {
    m_dirty = true;
}

// =========================================================================
// Pixel buffer regeneration
// =========================================================================

void SectorScan::regenerate() {
    if (!m_provider) {
        return;
    }

    uint32_t map_w = m_provider->get_map_width();
    uint32_t map_h = m_provider->get_map_height();

    if (map_w == 0 || map_h == 0) {
        return;
    }

    m_pixel_width = map_w;
    m_pixel_height = map_h;

    // Allocate RGBA8 buffer: 4 bytes per pixel
    size_t buffer_size = static_cast<size_t>(map_w) * static_cast<size_t>(map_h) * 4;
    m_pixels.resize(buffer_size);

    for (uint32_t y = 0; y < map_h; ++y) {
        for (uint32_t x = 0; x < map_w; ++x) {
            MinimapTile tile = m_provider->get_minimap_tile(x, y);

            size_t offset = (static_cast<size_t>(y) * map_w + x) * 4;
            m_pixels[offset + 0] = tile.r;
            m_pixels[offset + 1] = tile.g;
            m_pixels[offset + 2] = tile.b;
            m_pixels[offset + 3] = 255; // Full alpha
        }
    }

    m_dirty = false;
}

// =========================================================================
// Update
// =========================================================================

void SectorScan::update(float delta_time) {
    if (m_dirty) {
        regenerate();
    }

    // Update children
    Widget::update(delta_time);
}

// =========================================================================
// Rendering
// =========================================================================

void SectorScan::render(UIRenderer* renderer) {
    if (!visible) {
        return;
    }

    // 1. Draw minimap background rectangle (dark substrate)
    Color bg_fill = Color::from_rgba8(COLOR_SUBSTRATE_R, COLOR_SUBSTRATE_G,
                                       COLOR_SUBSTRATE_B, 200);
    Color bg_border = {0.3f, 0.4f, 0.5f, 0.9f};
    renderer->draw_rect(screen_bounds, bg_fill, bg_border);

    // NOTE: The actual pixel buffer is uploaded as a GPU texture during
    // integration. The renderer->draw_icon() call with the texture handle
    // would go here once integration is wired up.

    // 2. Draw viewport indicator rectangle (camera frustum on minimap)
    //    Map normalized viewport coords to screen-space minimap bounds.
    float vp_x = screen_bounds.x + m_viewport.x * screen_bounds.width;
    float vp_y = screen_bounds.y + m_viewport.y * screen_bounds.height;
    float vp_w = m_viewport.width * screen_bounds.width;
    float vp_h = m_viewport.height * screen_bounds.height;

    // Clamp viewport indicator to minimap bounds
    vp_w = std::min(vp_w, screen_bounds.width - (vp_x - screen_bounds.x));
    vp_h = std::min(vp_h, screen_bounds.height - (vp_y - screen_bounds.y));

    if (vp_w > 0.0f && vp_h > 0.0f) {
        Rect vp_rect = {vp_x, vp_y, vp_w, vp_h};

        // Semi-transparent white fill with bright white border for visibility
        Color vp_fill   = {1.0f, 1.0f, 1.0f, 0.1f};
        Color vp_border = {1.0f, 1.0f, 1.0f, 0.9f};
        renderer->draw_rect(vp_rect, vp_fill, vp_border);
    }

    // Render children
    Widget::render(renderer);
}

// =========================================================================
// Navigation callback
// =========================================================================

void SectorScan::set_navigate_callback(NavigateCallback callback) {
    m_navigate_callback = std::move(callback);
}

// =========================================================================
// Mouse input
// =========================================================================

void SectorScan::on_mouse_down(int button, float x, float y) {
    // Left click (button 1) triggers navigation
    if (button == 1 && m_navigate_callback) {
        float world_x = 0.0f;
        float world_y = 0.0f;
        screen_to_world(x, y, world_x, world_y);
        m_navigate_callback(world_x, world_y);
    }
}

// =========================================================================
// Coordinate conversion
// =========================================================================

void SectorScan::screen_to_world(float screen_x, float screen_y,
                                  float& world_x, float& world_y) const {
    // Map screen click position to normalized 0-1 within the minimap widget
    float norm_x = 0.0f;
    float norm_y = 0.0f;

    if (screen_bounds.width > 0.0f) {
        norm_x = (screen_x - screen_bounds.x) / screen_bounds.width;
    }
    if (screen_bounds.height > 0.0f) {
        norm_y = (screen_y - screen_bounds.y) / screen_bounds.height;
    }

    // Clamp to [0, 1]
    norm_x = std::max(0.0f, std::min(1.0f, norm_x));
    norm_y = std::max(0.0f, std::min(1.0f, norm_y));

    // Scale to map dimensions (world tile coordinates)
    if (m_provider) {
        world_x = norm_x * static_cast<float>(m_provider->get_map_width());
        world_y = norm_y * static_cast<float>(m_provider->get_map_height());
    } else {
        world_x = norm_x;
        world_y = norm_y;
    }
}

// =========================================================================
// Pixel buffer accessors
// =========================================================================

const std::vector<uint8_t>& SectorScan::get_pixels() const {
    return m_pixels;
}

uint32_t SectorScan::get_pixel_width() const {
    return m_pixel_width;
}

uint32_t SectorScan::get_pixel_height() const {
    return m_pixel_height;
}

} // namespace ui
} // namespace sims3000
