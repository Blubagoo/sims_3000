/**
 * @file ScanLayerRenderer.cpp
 * @brief Implementation of ScanLayerRenderer - CPU-side overlay texture generation.
 */

#include "sims3000/ui/ScanLayerRenderer.h"
#include "sims3000/services/IGridOverlay.h"

#include <algorithm>
#include <cstring>

namespace sims3000 {
namespace ui {

// ============================================================================
// Construction
// ============================================================================

ScanLayerRenderer::ScanLayerRenderer() = default;

// ============================================================================
// Map size
// ============================================================================

void ScanLayerRenderer::set_map_size(uint32_t width, uint32_t height) {
    m_map_width = width;
    m_map_height = height;

    m_texture.width = width;
    m_texture.height = height;
    m_texture.pixels.assign(static_cast<size_t>(width) * height * 4, 0);
    m_texture.dirty = true;
}

// ============================================================================
// Full texture update
// ============================================================================

void ScanLayerRenderer::update_texture(const services::IGridOverlay* overlay, float fade_alpha) {
    if (!overlay || m_map_width == 0 || m_map_height == 0) {
        clear();
        return;
    }

    fade_alpha = std::max(0.0f, std::min(1.0f, fade_alpha));

    for (uint32_t y = 0; y < m_map_height; ++y) {
        for (uint32_t x = 0; x < m_map_width; ++x) {
            services::OverlayColor color = overlay->getColorAt(x, y);

            size_t offset = (static_cast<size_t>(y) * m_map_width + x) * 4;
            m_texture.pixels[offset + 0] = color.r;
            m_texture.pixels[offset + 1] = color.g;
            m_texture.pixels[offset + 2] = color.b;
            m_texture.pixels[offset + 3] = static_cast<uint8_t>(color.a * fade_alpha);
        }
    }

    m_texture.dirty = true;
}

// ============================================================================
// Region update
// ============================================================================

void ScanLayerRenderer::update_region(const services::IGridOverlay* overlay,
                                      uint32_t x0, uint32_t y0,
                                      uint32_t x1, uint32_t y1,
                                      float fade_alpha) {
    if (!overlay || m_map_width == 0 || m_map_height == 0) {
        return;
    }

    // Clamp to map bounds
    x0 = std::min(x0, m_map_width);
    y0 = std::min(y0, m_map_height);
    x1 = std::min(x1, m_map_width);
    y1 = std::min(y1, m_map_height);

    if (x0 >= x1 || y0 >= y1) {
        return;
    }

    fade_alpha = std::max(0.0f, std::min(1.0f, fade_alpha));

    for (uint32_t y = y0; y < y1; ++y) {
        for (uint32_t x = x0; x < x1; ++x) {
            services::OverlayColor color = overlay->getColorAt(x, y);

            size_t offset = (static_cast<size_t>(y) * m_map_width + x) * 4;
            m_texture.pixels[offset + 0] = color.r;
            m_texture.pixels[offset + 1] = color.g;
            m_texture.pixels[offset + 2] = color.b;
            m_texture.pixels[offset + 3] = static_cast<uint8_t>(color.a * fade_alpha);
        }
    }

    m_texture.dirty = true;
}

// ============================================================================
// Texture access
// ============================================================================

const OverlayTextureData& ScanLayerRenderer::get_texture_data() const {
    return m_texture;
}

void ScanLayerRenderer::mark_clean() {
    m_texture.dirty = false;
}

bool ScanLayerRenderer::has_content() const {
    return m_texture.width > 0 && !m_texture.pixels.empty();
}

// ============================================================================
// Clear
// ============================================================================

void ScanLayerRenderer::clear() {
    if (!m_texture.pixels.empty()) {
        std::memset(m_texture.pixels.data(), 0, m_texture.pixels.size());
        m_texture.dirty = true;
    }
}

} // namespace ui
} // namespace sims3000
