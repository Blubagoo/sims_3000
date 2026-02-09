/**
 * @file Widget.h
 * @brief Base class and hierarchy for all UI widgets.
 *
 * Provides the fundamental widget abstraction including:
 * - Rect and Color utility types for layout/rendering
 * - Parent-child hierarchy with ownership via unique_ptr
 * - Recursive update, render, and screen-bounds computation
 * - Hit testing and mouse event dispatch (virtual, default no-op)
 * - Z-order support for layered rendering
 *
 * No SDL dependencies - this is a pure C++ widget tree.
 *
 * Usage:
 * @code
 *   auto root = std::make_unique<Widget>();
 *   root->bounds = {0, 0, 1280, 720};
 *
 *   auto btn = std::make_unique<Button>();
 *   btn->bounds = {10, 10, 120, 40};
 *   root->add_child(std::move(btn));
 *
 *   root->compute_screen_bounds();
 *   root->update(dt);
 *   root->render(renderer);
 * @endcode
 */

#ifndef SIMS3000_UI_WIDGET_H
#define SIMS3000_UI_WIDGET_H

#include <cstdint>
#include <memory>
#include <vector>

namespace sims3000 {
namespace ui {

/**
 * @struct Rect
 * @brief Rectangle in screen space (pixels).
 */
struct Rect {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;

    /**
     * Test whether a point is inside this rectangle.
     * @param px X coordinate of the point
     * @param py Y coordinate of the point
     * @return true if the point is inside
     */
    bool contains(float px, float py) const;
};

/**
 * @struct Color
 * @brief Color as RGBA floats (0.0 - 1.0).
 */
struct Color {
    float r = 1.0f;
    float g = 1.0f;
    float b = 1.0f;
    float a = 1.0f;

    /**
     * Create a Color from 8-bit RGBA values.
     * @param r Red   (0-255)
     * @param g Green (0-255)
     * @param b Blue  (0-255)
     * @param a Alpha (0-255, default 255)
     * @return Color with components converted to 0.0-1.0 range
     */
    static Color from_rgba8(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a = 255);
};

// Forward declaration - renderer is implemented separately
class UIRenderer;

/**
 * @class Widget
 * @brief Base class for all UI widgets.
 *
 * Manages a tree of widgets with parent-child relationships, recursive
 * update/render, hit testing, and mouse event dispatch.  Concrete widget
 * types (Button, Panel, Label, ...) derive from this class and override
 * the virtual methods they need.
 */
class Widget {
public:
    virtual ~Widget() = default;

    // -- Lifecycle -----------------------------------------------------------

    /**
     * Update this widget and all children.
     * @param delta_time Seconds since last frame
     */
    virtual void update(float delta_time);

    /**
     * Render this widget and all visible children.
     * @param renderer The UI renderer to draw with
     */
    virtual void render(UIRenderer* renderer);

    // -- Layout --------------------------------------------------------------

    /// Position and size in parent space
    Rect bounds;

    /// Computed position in screen (root) space
    Rect screen_bounds;

    /// Whether the widget is drawn and receives events
    bool visible = true;

    /// Whether the widget accepts interaction
    bool enabled = true;

    // -- Hierarchy -----------------------------------------------------------

    /// Non-owning pointer to the parent widget (nullptr for root)
    Widget* parent = nullptr;

    /// Owning list of child widgets
    std::vector<std::unique_ptr<Widget>> children;

    /**
     * Add a child widget.  Ownership is transferred to this widget.
     * @param child The widget to add
     * @return Raw pointer to the added child (for convenience)
     */
    Widget* add_child(std::unique_ptr<Widget> child);

    /**
     * Remove a child by raw pointer.  The child is destroyed.
     * @param child Pointer to the child to remove
     */
    void remove_child(Widget* child);

    /**
     * Recursive hit test: find the deepest child at the given screen
     * coordinates.  Children are tested in reverse order so that higher
     * z-order widgets (rendered last / on top) are found first.
     * @param x Screen X
     * @param y Screen Y
     * @return The deepest widget under the point, or nullptr
     */
    Widget* find_child_at(float x, float y);

    // -- Screen bounds -------------------------------------------------------

    /**
     * Recursively compute screen_bounds from parent screen_bounds + this
     * widget's bounds offset.  Must be called after layout changes and
     * before hit testing or rendering.
     */
    void compute_screen_bounds();

    // -- Hit testing ---------------------------------------------------------

    /**
     * Test whether a screen-space point hits this widget.
     * Default implementation checks screen_bounds, visible, and enabled.
     * @param x Screen X
     * @param y Screen Y
     * @return true if the point hits this widget
     */
    virtual bool hit_test(float x, float y) const;

    // -- Mouse events (virtual, default no-op) --------------------------------

    virtual void on_mouse_enter();
    virtual void on_mouse_leave();
    virtual void on_mouse_down(int button, float x, float y);
    virtual void on_mouse_up(int button, float x, float y);
    virtual void on_mouse_move(float x, float y);

    // -- State accessors -----------------------------------------------------

    bool is_hovered() const { return m_hovered; }
    bool is_pressed() const { return m_pressed; }
    void set_hovered(bool h) { m_hovered = h; }
    void set_pressed(bool p) { m_pressed = p; }

    // -- Z-order (higher = on top) -------------------------------------------

    int z_order = 0;

protected:
    bool m_hovered = false;
    bool m_pressed = false;
};

} // namespace ui
} // namespace sims3000

#endif // SIMS3000_UI_WIDGET_H
