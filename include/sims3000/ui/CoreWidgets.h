/**
 * @file CoreWidgets.h
 * @brief Core widget types: Button, Label, Panel, Icon.
 *
 * Provides the fundamental interactive and display widgets that make up
 * most of the game's UI.  All widgets derive from Widget and render
 * through the abstract UIRenderer interface.
 *
 * Usage:
 * @code
 *   auto btn = std::make_unique<ButtonWidget>();
 *   btn->bounds = {10, 10, 120, 40};
 *   btn->text = "OK";
 *   btn->on_click = []{ do_something(); };
 *   panel->add_child(std::move(btn));
 * @endcode
 */

#ifndef SIMS3000_UI_COREWIDGETS_H
#define SIMS3000_UI_COREWIDGETS_H

#include "sims3000/ui/Widget.h"
#include "sims3000/ui/UIRenderer.h"

#include <functional>
#include <string>

namespace sims3000 {
namespace ui {

/**
 * @enum TextAlignment
 * @brief Horizontal text alignment within a label's bounds.
 */
enum class TextAlignment : uint8_t {
    Left   = 0,
    Center = 1,
    Right  = 2
};

// =========================================================================
// ButtonWidget
// =========================================================================

/**
 * @class ButtonWidget
 * @brief Clickable button with text and/or icon.
 *
 * When an icon texture is set the button renders as an icon button via
 * UIRenderer::draw_icon_button(); otherwise it renders as a text button
 * via UIRenderer::draw_button().  The on_click callback is invoked on
 * mouse-up if the button was previously pressed.
 */
class ButtonWidget : public Widget {
public:
    /// Label displayed on the button (ignored when rendering as icon-only)
    std::string text;

    /// Optional icon texture; when set, the button renders as an icon button
    TextureHandle icon = INVALID_TEXTURE;

    /// Callback invoked when the button is clicked (mouse-up after press)
    std::function<void()> on_click;

    void render(UIRenderer* renderer) override;
    void on_mouse_down(int button, float x, float y) override;
    void on_mouse_up(int button, float x, float y) override;
    void on_mouse_enter() override;
    void on_mouse_leave() override;

private:
    /**
     * Derive the visual ButtonState from the widget's current
     * enabled / hovered / pressed flags.
     */
    ButtonState get_visual_state() const;
};

// =========================================================================
// LabelWidget
// =========================================================================

/**
 * @class LabelWidget
 * @brief Non-interactive text display widget.
 *
 * Renders a single line of text with configurable font size, color,
 * and horizontal alignment within the widget's bounds.
 */
class LabelWidget : public Widget {
public:
    /// The text string to display
    std::string text;

    /// Font size category for rendering
    FontSize font_size = FontSize::Normal;

    /// Horizontal alignment of the text within the widget bounds
    TextAlignment alignment = TextAlignment::Left;

    /// Color of the rendered text
    Color text_color = {1.0f, 1.0f, 1.0f, 1.0f};

    void render(UIRenderer* renderer) override;
};

// =========================================================================
// PanelWidget
// =========================================================================

/**
 * @class PanelWidget
 * @brief Container widget with a title bar, optional close button.
 *
 * Panels serve as top-level containers for groups of widgets.  The title
 * bar consumes TITLE_BAR_HEIGHT pixels at the top; children are laid out
 * inside the remaining content area returned by get_content_bounds().
 */
class PanelWidget : public Widget {
public:
    /// Text displayed in the panel's title bar
    std::string title;

    /// Whether the title bar includes a close button
    bool closable = false;

    /// Whether the panel can be dragged (used in holo mode)
    bool draggable = false;

    /// Callback invoked when the close button is activated
    std::function<void()> on_close;

    void render(UIRenderer* renderer) override;

    /**
     * Return the content area inside the panel (below the title bar).
     * @return Rect in screen space representing the usable content region
     */
    Rect get_content_bounds() const;

    /// Height of the title bar in pixels
    static constexpr float TITLE_BAR_HEIGHT = 28.0f;
};

// =========================================================================
// IconWidget
// =========================================================================

/**
 * @class IconWidget
 * @brief Displays a textured icon with an optional color tint.
 *
 * A lightweight display widget that draws a single texture stretched
 * to fill its bounds.  Does nothing if the texture handle is invalid.
 */
class IconWidget : public Widget {
public:
    /// Texture handle for the icon image
    TextureHandle texture = INVALID_TEXTURE;

    /// Multiplicative color tint applied to the icon
    Color tint = {1.0f, 1.0f, 1.0f, 1.0f};

    void render(UIRenderer* renderer) override;
};

} // namespace ui
} // namespace sims3000

#endif // SIMS3000_UI_COREWIDGETS_H
