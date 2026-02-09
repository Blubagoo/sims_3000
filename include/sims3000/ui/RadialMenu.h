/**
 * @file RadialMenu.h
 * @brief Holographic radial menu for tool/command selection.
 *
 * Provides a radial "command array" menu that opens on right-click,
 * displaying tool categories in an inner ring and individual tools
 * in an outer sub-ring when a category is hovered.
 *
 * The menu is structured as two concentric rings:
 * - Inner ring (INNER_RADIUS to OUTER_RADIUS): category segments
 * - Outer ring (OUTER_RADIUS to SUB_RING_RADIUS): tool items for
 *   the currently hovered category
 *
 * Usage:
 * @code
 *   auto menu = std::make_unique<RadialMenu>();
 *   menu->build_default_categories();
 *   menu->set_tool_callback([&](ToolType t) { ui.set_tool(t); });
 *
 *   // On right-click:
 *   menu->show(mouse_x, mouse_y);
 *
 *   // Per-frame:
 *   menu->update(dt);
 *   menu->render(renderer);
 * @endcode
 */

#ifndef SIMS3000_UI_RADIALMENU_H
#define SIMS3000_UI_RADIALMENU_H

#include "sims3000/ui/Widget.h"
#include "sims3000/ui/UIRenderer.h"
#include "sims3000/ui/UIManager.h"

#include <cmath>
#include <functional>
#include <string>
#include <vector>

namespace sims3000 {
namespace ui {

/**
 * @struct RadialMenuItem
 * @brief A single selectable item in the radial menu's outer ring.
 */
struct RadialMenuItem {
    /// Display label for the item
    std::string label;

    /// Tool type this item activates
    ToolType tool = ToolType::Select;

    /// Optional custom callback invoked on selection
    std::function<void()> on_select;
};

/**
 * @struct RadialCategory
 * @brief A category segment in the radial menu's inner ring.
 *
 * Each category contains a group of related RadialMenuItems that
 * appear in the outer sub-ring when the category is hovered.
 */
struct RadialCategory {
    /// Display label for the category (e.g. "BUILD", "MODIFY")
    std::string label;

    /// Items belonging to this category
    std::vector<RadialMenuItem> items;
};

/**
 * @class RadialMenu
 * @brief Holographic radial menu that appears on right-click.
 *
 * Implements a two-ring radial menu with smooth open/close animation.
 * The inner ring displays categories; hovering a category expands the
 * outer ring to show that category's tool items.  Releasing the mouse
 * on an item selects it and fires the tool callback.
 */
class RadialMenu : public Widget {
public:
    RadialMenu();

    /**
     * Build the default menu structure with four categories:
     * BUILD, MODIFY, INSPECT, VIEW.
     */
    void build_default_categories();

    /**
     * Show the menu centered at a screen position.
     * @param center_x Screen X for the menu center
     * @param center_y Screen Y for the menu center
     */
    void show(float center_x, float center_y);

    /**
     * Hide the menu and reset hover state.
     */
    void hide();

    /**
     * Check whether the menu is currently visible.
     * @return true if the menu is open
     */
    bool is_open() const;

    /// Callback type for tool selection notifications.
    using ToolSelectCallback = std::function<void(ToolType)>;

    /**
     * Set the callback invoked when a tool is selected from the menu.
     * @param callback Function receiving the selected ToolType
     */
    void set_tool_callback(ToolSelectCallback callback);

    void update(float delta_time) override;
    void render(UIRenderer* renderer) override;
    bool hit_test(float x, float y) const override;

    void on_mouse_move(float x, float y) override;
    void on_mouse_up(int button, float x, float y) override;

    // -- Layout constants ----------------------------------------------------

    /// Radius of the dead zone at the center (no selection)
    static constexpr float INNER_RADIUS = 50.0f;

    /// Outer edge of the category ring
    static constexpr float OUTER_RADIUS = 120.0f;

    /// Outer edge of the sub-item ring
    static constexpr float SUB_RING_RADIUS = 180.0f;

    /// Animation interpolation speed (higher = faster open)
    static constexpr float ANIMATION_SPEED = 8.0f;

private:
    std::vector<RadialCategory> m_categories;
    ToolSelectCallback m_tool_callback;

    float m_center_x = 0.0f;
    float m_center_y = 0.0f;
    bool m_open = false;
    float m_animation_progress = 0.0f; ///< 0 = closed, 1 = fully open

    int m_hovered_category = -1; ///< Index into m_categories, -1 = none
    int m_hovered_item = -1;     ///< Index into hovered category's items, -1 = none

    /**
     * Determine which category segment the given point falls in.
     * @param x Screen X
     * @param y Screen Y
     * @return Category index, or -1 if not over a category
     */
    int get_category_at(float x, float y) const;

    /**
     * Determine which sub-item the given point falls on.
     * Only valid when a category is hovered.
     * @param x Screen X
     * @param y Screen Y
     * @return Item index within the hovered category, or -1 if none
     */
    int get_item_at(float x, float y) const;

    /**
     * Compute the distance from the menu center to a point.
     * @param x Screen X
     * @param y Screen Y
     * @return Euclidean distance in pixels
     */
    float distance_from_center(float x, float y) const;

    /**
     * Compute the angle from the menu center to a point.
     * @param x Screen X
     * @param y Screen Y
     * @return Angle in radians (atan2 convention)
     */
    float angle_from_center(float x, float y) const;
};

} // namespace ui
} // namespace sims3000

#endif // SIMS3000_UI_RADIALMENU_H
