/**
 * @file UIRenderer.h
 * @brief Abstract renderer interface for all UI widget drawing.
 *
 * Defines the virtual interface that concrete renderers (e.g. SDL_GPU-based)
 * must implement. Provides pure-virtual methods for panels, buttons, text,
 * sliders, progress bars, icons, and primitives, plus default no-op methods
 * for holographic visual effects (scanlines, glow).
 *
 * No SDL or platform dependencies -- concrete renderers live elsewhere.
 *
 * Usage:
 * @code
 *   class MyGPURenderer : public UIRenderer {
 *   public:
 *       void draw_panel(const Rect& bounds, const std::string& title, bool closable) override;
 *       void draw_button(const Rect& bounds, const std::string& text, ButtonState state) override;
 *       // ... all other pure-virtual overrides ...
 *   };
 *
 *   MyGPURenderer renderer;
 *   renderer.draw_panel({10, 10, 300, 200}, "Budget", true);
 *   renderer.draw_button({20, 170, 80, 30}, "OK", ButtonState::Normal);
 * @endcode
 *
 * Resource ownership:
 * - UIRenderer owns no resources itself; concrete subclasses manage GPU state
 * - TextureHandle is an opaque 32-bit identifier whose lifetime is managed by
 *   the concrete renderer or an asset system
 */

#ifndef SIMS3000_UI_UIRENDERER_H
#define SIMS3000_UI_UIRENDERER_H

#include <cstdint>
#include <string>

namespace sims3000 {
namespace ui {

// Forward declarations from Widget.h
struct Rect;
struct Color;

/**
 * @enum FontSize
 * @brief Predefined font size categories for UI text rendering.
 */
enum class FontSize : uint8_t {
    Small  = 0,   ///< Small text (tooltips, fine print)
    Normal = 1,   ///< Default body text
    Large  = 2,   ///< Headings, emphasized text
    Title  = 3    ///< Panel titles, major headings
};

/**
 * @enum ButtonState
 * @brief Visual state of a button for rendering purposes.
 */
enum class ButtonState : uint8_t {
    Normal   = 0, ///< Default idle state
    Hovered  = 1, ///< Mouse is over the button
    Pressed  = 2, ///< Button is being clicked
    Disabled = 3  ///< Button is inactive / grayed out
};

/// Opaque handle to a texture resource managed by the concrete renderer.
using TextureHandle = uint32_t;

/// Sentinel value indicating no valid texture.
constexpr TextureHandle INVALID_TEXTURE = 0;

/**
 * @class UIRenderer
 * @brief Abstract interface for rendering UI widgets.
 *
 * All widget drawing goes through this interface so that the widget tree
 * is completely decoupled from the graphics back-end.  A concrete subclass
 * (e.g. SDLGPURenderer) provides the actual draw calls.
 *
 * Pure-virtual methods must be implemented by every renderer.  The
 * holographic-effect methods (draw_scanlines, begin_glow_effect,
 * end_glow_effect) have default no-op implementations so that a classic
 * renderer does not need to handle them.
 */
class UIRenderer {
public:
    virtual ~UIRenderer() = default;

    // =========================================================================
    // Panel rendering
    // =========================================================================

    /**
     * Draw a complete panel (background + border + title bar).
     * @param bounds Screen-space rectangle of the panel
     * @param title  Text displayed in the title bar
     * @param closable Whether to draw a close button in the title bar
     */
    virtual void draw_panel(const Rect& bounds, const std::string& title, bool closable) = 0;

    /**
     * Draw only the panel background (no title bar or border).
     * @param bounds Screen-space rectangle to fill
     */
    virtual void draw_panel_background(const Rect& bounds) = 0;

    // =========================================================================
    // Button rendering
    // =========================================================================

    /**
     * Draw a text button.
     * @param bounds Screen-space rectangle of the button
     * @param text   Label displayed on the button
     * @param state  Visual state (normal, hovered, pressed, disabled)
     */
    virtual void draw_button(const Rect& bounds, const std::string& text, ButtonState state) = 0;

    /**
     * Draw an icon-only button.
     * @param bounds Screen-space rectangle of the button
     * @param icon   Texture handle for the icon image
     * @param state  Visual state (normal, hovered, pressed, disabled)
     */
    virtual void draw_icon_button(const Rect& bounds, TextureHandle icon, ButtonState state) = 0;

    // =========================================================================
    // Text rendering
    // =========================================================================

    /**
     * Draw a single line of text.
     * @param text  The string to render
     * @param x     Screen X position (left edge)
     * @param y     Screen Y position (top edge / baseline, renderer-defined)
     * @param size  Font size category
     * @param color Text color
     */
    virtual void draw_text(const std::string& text, float x, float y, FontSize size, const Color& color) = 0;

    // =========================================================================
    // Primitives
    // =========================================================================

    /**
     * Draw a filled rectangle with a border.
     * @param bounds Screen-space rectangle
     * @param fill   Fill color
     * @param border Border color
     */
    virtual void draw_rect(const Rect& bounds, const Color& fill, const Color& border) = 0;

    /**
     * Draw a horizontal progress bar.
     * @param bounds     Screen-space rectangle of the bar
     * @param progress   Fill fraction (0.0 = empty, 1.0 = full)
     * @param fill_color Color of the filled portion
     */
    virtual void draw_progress_bar(const Rect& bounds, float progress, const Color& fill_color) = 0;

    // =========================================================================
    // Holographic effects (default no-op for classic/legacy skins)
    // =========================================================================

    /**
     * Draw CRT-style scanlines over a region.
     * @param bounds  Area to apply the effect
     * @param opacity Strength of the scanline effect (0.0 - 1.0)
     */
    virtual void draw_scanlines(const Rect& bounds, float opacity) { (void)bounds; (void)opacity; }

    /**
     * Begin an outer-glow effect around subsequent draw calls.
     * Paired with end_glow_effect().
     * @param intensity Glow strength (0.0 = none, 1.0 = full)
     */
    virtual void begin_glow_effect(float intensity) { (void)intensity; }

    /**
     * End an outer-glow effect started by begin_glow_effect().
     */
    virtual void end_glow_effect() {}

    // =========================================================================
    // Slider rendering
    // =========================================================================

    /**
     * Draw a horizontal slider control.
     * @param bounds  Screen-space rectangle of the slider track
     * @param value   Current value
     * @param min_val Minimum value
     * @param max_val Maximum value
     */
    virtual void draw_slider(const Rect& bounds, float value, float min_val, float max_val) = 0;

    // =========================================================================
    // Icon / image rendering
    // =========================================================================

    /**
     * Draw a textured icon or image.
     * @param bounds  Screen-space rectangle to draw into
     * @param texture Texture handle for the image
     * @param tint    Color tint applied multiplicatively
     */
    virtual void draw_icon(const Rect& bounds, TextureHandle texture, const Color& tint) = 0;
};

} // namespace ui
} // namespace sims3000

#endif // SIMS3000_UI_UIRENDERER_H
