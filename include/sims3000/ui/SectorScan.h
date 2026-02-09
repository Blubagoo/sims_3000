/**
 * @file SectorScan.h
 * @brief CPU-generated minimap showing a zoomed-out view of the entire map.
 *
 * Provides a "sector scan" widget that renders the full game map as a small
 * pixel buffer, with color-coded tiles based on zone type and ownership.
 * The widget also shows a viewport indicator (the player's current camera
 * frustum) and supports click-to-navigate interaction.
 *
 * The pixel buffer is generated on the CPU; GPU texture upload happens
 * at the integration layer, not here.
 *
 * Usage:
 * @code
 *   auto minimap = std::make_unique<SectorScan>();
 *   minimap->bounds = {10, 500, 200, 200};
 *   minimap->set_data_provider(&my_map_provider);
 *   minimap->set_navigate_callback([](float wx, float wy) {
 *       camera.center_on(wx, wy);
 *   });
 *
 *   ViewportIndicator vp{0.1f, 0.2f, 0.3f, 0.25f};
 *   minimap->set_viewport(vp);
 *
 *   root->add_child(std::move(minimap));
 * @endcode
 */

#ifndef SIMS3000_UI_SECTORSCAN_H
#define SIMS3000_UI_SECTORSCAN_H

#include "sims3000/ui/Widget.h"
#include "sims3000/ui/UIRenderer.h"
#include "sims3000/core/types.h"

#include <cstdint>
#include <vector>
#include <functional>

namespace sims3000 {
namespace ui {

/**
 * @struct MinimapTile
 * @brief Minimap tile data for rendering.
 *
 * Each tile carries an RGB color (determined by zone type or terrain)
 * and an owner_id for player ownership tinting.
 */
struct MinimapTile {
    uint8_t r, g, b;    ///< Tile color (zone/terrain)
    uint8_t owner_id;   ///< 0=unowned, 1-4=player
};

/**
 * @struct ViewportIndicator
 * @brief Camera view rectangle on the minimap in normalized coordinates.
 *
 * All values are in the range 0.0 to 1.0, representing the fraction
 * of the full map that the camera currently shows.
 */
struct ViewportIndicator {
    float x = 0.0f;       ///< Left edge (0.0 = map left)
    float y = 0.0f;       ///< Top edge (0.0 = map top)
    float width = 0.0f;   ///< Width as fraction of map
    float height = 0.0f;  ///< Height as fraction of map
};

/**
 * @class IMinimapDataProvider
 * @brief Interface for providing minimap tile data to the SectorScan widget.
 *
 * Implementations query the game world (terrain, zones, buildings) and
 * return a MinimapTile per grid cell.
 */
class IMinimapDataProvider {
public:
    virtual ~IMinimapDataProvider() = default;

    /**
     * Get the minimap tile data for a given grid position.
     * @param x Tile X coordinate (0-based)
     * @param y Tile Y coordinate (0-based)
     * @return MinimapTile with color and ownership info
     */
    virtual MinimapTile get_minimap_tile(uint32_t x, uint32_t y) const = 0;

    /** @return Map width in tiles. */
    virtual uint32_t get_map_width() const = 0;

    /** @return Map height in tiles. */
    virtual uint32_t get_map_height() const = 0;
};

/**
 * @class SectorScan
 * @brief Sector Scan (Minimap) widget.
 *
 * Generates a CPU-side RGBA8 pixel buffer from an IMinimapDataProvider,
 * displays a viewport indicator rectangle for the current camera view,
 * and converts mouse clicks to world-space navigation events.
 *
 * The pixel buffer can be retrieved via get_pixels() for GPU texture
 * upload by the rendering integration layer.
 */
class SectorScan : public Widget {
public:
    SectorScan();

    /**
     * Set the data provider for minimap tile queries (non-owning).
     * Invalidates the minimap so it will regenerate on next update.
     * @param provider Pointer to an IMinimapDataProvider (lifetime managed externally)
     */
    void set_data_provider(IMinimapDataProvider* provider);

    /**
     * Set the viewport indicator (camera frustum rectangle on map).
     * @param vp Normalized viewport rectangle (0.0-1.0)
     */
    void set_viewport(const ViewportIndicator& vp);

    /**
     * Regenerate the minimap pixel buffer from the data provider.
     * Called automatically during update() when the minimap is dirty.
     */
    void regenerate();

    /**
     * Mark the minimap as needing regeneration on the next update.
     */
    void invalidate();

    void update(float delta_time) override;
    void render(UIRenderer* renderer) override;

    /// Callback type for click-to-navigate. Receives world tile coordinates.
    using NavigateCallback = std::function<void(float world_x, float world_y)>;

    /**
     * Set the callback invoked when the user clicks on the minimap.
     * @param callback Function receiving (world_x, world_y) tile coordinates
     */
    void set_navigate_callback(NavigateCallback callback);

    void on_mouse_down(int button, float x, float y) override;

    /**
     * Get the CPU pixel buffer (RGBA8 format).
     * @return Const reference to the pixel data vector
     */
    const std::vector<uint8_t>& get_pixels() const;

    /** @return Width of the pixel buffer in pixels. */
    uint32_t get_pixel_width() const;

    /** @return Height of the pixel buffer in pixels. */
    uint32_t get_pixel_height() const;

    // -- Layout constants ----------------------------------------------------

    /// Default minimap widget size (width and height) in pixels
    static constexpr float DEFAULT_SIZE = 200.0f;

    // -- Color scheme (zone types) -------------------------------------------

    /// Substrate (empty terrain) color
    static constexpr uint8_t COLOR_SUBSTRATE_R = 0x40;
    static constexpr uint8_t COLOR_SUBSTRATE_G = 0x40;
    static constexpr uint8_t COLOR_SUBSTRATE_B = 0x40;

    /// Deep Void (water) color
    static constexpr uint8_t COLOR_DEEP_VOID_R = 0x1A;
    static constexpr uint8_t COLOR_DEEP_VOID_G = 0x4D;
    static constexpr uint8_t COLOR_DEEP_VOID_B = 0x7A;

    /// Habitation (residential) zone color
    static constexpr uint8_t COLOR_HABITATION_R = 0x00;
    static constexpr uint8_t COLOR_HABITATION_G = 0xAA;
    static constexpr uint8_t COLOR_HABITATION_B = 0x00;

    /// Exchange (commercial) zone color
    static constexpr uint8_t COLOR_EXCHANGE_R = 0x00;
    static constexpr uint8_t COLOR_EXCHANGE_G = 0x66;
    static constexpr uint8_t COLOR_EXCHANGE_B = 0xCC;

    /// Fabrication (industrial) zone color
    static constexpr uint8_t COLOR_FABRICATION_R = 0xCC;
    static constexpr uint8_t COLOR_FABRICATION_G = 0xCC;
    static constexpr uint8_t COLOR_FABRICATION_B = 0x00;

    /// Pathway (road) color
    static constexpr uint8_t COLOR_PATHWAY_R = 0xCC;
    static constexpr uint8_t COLOR_PATHWAY_G = 0xCC;
    static constexpr uint8_t COLOR_PATHWAY_B = 0xCC;

private:
    IMinimapDataProvider* m_provider = nullptr;
    NavigateCallback m_navigate_callback;
    ViewportIndicator m_viewport{};

    std::vector<uint8_t> m_pixels;  ///< RGBA8 pixel data
    uint32_t m_pixel_width = 0;
    uint32_t m_pixel_height = 0;
    bool m_dirty = true;

    /**
     * Convert a screen-space click position to world tile coordinates.
     * @param screen_x Screen X of the click
     * @param screen_y Screen Y of the click
     * @param[out] world_x World tile X coordinate
     * @param[out] world_y World tile Y coordinate
     */
    void screen_to_world(float screen_x, float screen_y,
                         float& world_x, float& world_y) const;
};

} // namespace ui
} // namespace sims3000

#endif // SIMS3000_UI_SECTORSCAN_H
