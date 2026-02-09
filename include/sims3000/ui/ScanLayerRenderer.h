/**
 * @file ScanLayerRenderer.h
 * @brief CPU-side overlay texture generation from IGridOverlay data (Ticket E12-017).
 *
 * ScanLayerRenderer reads per-tile colors from an IGridOverlay and writes them
 * into a CPU-side RGBA8 pixel buffer. The actual GPU texture upload is handled
 * by the render integration layer; this class is purely data-side.
 *
 * Usage:
 * @code
 *   ScanLayerRenderer renderer;
 *   renderer.set_map_size(256, 256);
 *
 *   // At tick boundaries (not every frame):
 *   auto* overlay = scan_manager.get_active_overlay();
 *   float fade    = scan_manager.get_fade_progress();
 *   renderer.update_texture(overlay, fade);
 *
 *   // In render pass:
 *   if (renderer.has_content()) {
 *       const auto& tex = renderer.get_texture_data();
 *       upload_to_gpu(tex.pixels.data(), tex.width, tex.height);
 *       renderer.mark_clean();
 *   }
 * @endcode
 *
 * Thread safety: not thread-safe. Call from the main/render thread only.
 */

#ifndef SIMS3000_UI_SCANLAYERRENDERER_H
#define SIMS3000_UI_SCANLAYERRENDERER_H

#include "sims3000/ui/ScanLayerManager.h"

#include <cstdint>
#include <vector>

namespace sims3000 {
namespace services { class IGridOverlay; }
}

namespace sims3000 {
namespace ui {

// ============================================================================
// Overlay Texture Data
// ============================================================================

/**
 * @struct OverlayTextureData
 * @brief CPU-side overlay texture data in RGBA8 format.
 *
 * Pixels are stored in row-major order (top-left origin). Each pixel is 4
 * bytes: R, G, B, A. Total buffer size is width * height * 4 bytes.
 */
struct OverlayTextureData {
    std::vector<uint8_t> pixels;  ///< RGBA8 pixel data (width * height * 4 bytes)
    uint32_t width = 0;          ///< Texture width in pixels (one per tile column)
    uint32_t height = 0;         ///< Texture height in pixels (one per tile row)
    bool dirty = false;          ///< True if pixels need to be re-uploaded to GPU
};

// ============================================================================
// Scan Layer Renderer
// ============================================================================

/**
 * @class ScanLayerRenderer
 * @brief Generates a CPU-side overlay texture from IGridOverlay data.
 *
 * Each tile maps to one pixel. The overlay's per-tile color is written
 * directly into the pixel buffer, with alpha scaled by the fade_alpha
 * parameter (from ScanLayerManager fade transitions).
 *
 * For a 256x256 map the full update iterates 65 536 tiles, which is fast
 * enough for tick-rate (not per-frame) updates. For partial updates, use
 * update_region() to refresh only a rectangular sub-area.
 */
class ScanLayerRenderer {
public:
    ScanLayerRenderer();

    /// Set the map size for overlay rendering. Allocates the pixel buffer.
    void set_map_size(uint32_t width, uint32_t height);

    /**
     * Update the entire overlay texture from the active overlay.
     * Call at tick boundaries (not every frame).
     * @param overlay  The active overlay source (from ScanLayerManager).
     * @param fade_alpha Overall fade alpha [0.0, 1.0] from ScanLayerManager.
     */
    void update_texture(const services::IGridOverlay* overlay, float fade_alpha = 1.0f);

    /// Get the current texture data (for GPU upload).
    const OverlayTextureData& get_texture_data() const;

    /// Mark texture as clean (call after GPU upload).
    void mark_clean();

    /// Is overlay texture valid and has content?
    bool has_content() const;

    /// Clear the overlay texture (all transparent). Sets dirty=true.
    void clear();

    /**
     * Performance: update only a rectangular region (chunked update).
     * Coordinates are clamped to map bounds.
     * @param overlay    The active overlay source.
     * @param x0         Left column (inclusive).
     * @param y0         Top row (inclusive).
     * @param x1         Right column (exclusive).
     * @param y1         Bottom row (exclusive).
     * @param fade_alpha Overall fade alpha [0.0, 1.0].
     */
    void update_region(const services::IGridOverlay* overlay,
                       uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1,
                       float fade_alpha = 1.0f);

private:
    OverlayTextureData m_texture;
    uint32_t m_map_width = 0;
    uint32_t m_map_height = 0;
};

} // namespace ui
} // namespace sims3000

#endif // SIMS3000_UI_SCANLAYERRENDERER_H
