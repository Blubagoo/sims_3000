/**
 * @file CommandArray.h
 * @brief Classic mode horizontal command array (toolbar).
 *
 * The CommandArray is the main toolbar for the Legacy (classic) UI mode.
 * It presents a horizontal bar docked at the top of the screen, containing
 * grouped tool buttons for zoning, infrastructure, modification, inspection,
 * and simulation speed control.
 *
 * Usage:
 * @code
 *   auto toolbar = std::make_unique<CommandArray>();
 *   toolbar->set_tool_callback([&](ToolType t){ ui.set_tool(t); });
 *   toolbar->set_speed_callback([&](int s){ sim.set_speed(s); });
 *   toolbar->build_layout();
 *   root->add_child(std::move(toolbar));
 * @endcode
 */

#ifndef SIMS3000_UI_COMMANDARRAY_H
#define SIMS3000_UI_COMMANDARRAY_H

#include "sims3000/ui/CoreWidgets.h"
#include "sims3000/ui/UIManager.h"

#include <functional>
#include <string>
#include <vector>

namespace sims3000 {
namespace ui {

/// Callback type for tool selection
using ToolSelectCallback = std::function<void(ToolType)>;

/**
 * @class CommandArray
 * @brief Classic mode horizontal command array (toolbar).
 *
 * A PanelWidget-derived toolbar that spans the full viewport width and
 * sits at the top of the screen. Contains grouped buttons for:
 * - BUILD: zone placement (H/E/F) and infrastructure (Pathway, Energy, Fluid)
 * - MODIFY: Bulldoze, Purge, Grade
 * - INSPECT: Probe
 * - VIEW: speed controls (Pause, Play, Fast, Ultra)
 *
 * Buttons are text-only stubs; icons will be added later.
 */
class CommandArray : public PanelWidget {
public:
    CommandArray();

    /// Build the toolbar layout with all tool buttons
    void build_layout();

    /// Set callback for when a tool is selected
    void set_tool_callback(ToolSelectCallback callback);

    /// Update visual state to reflect the current tool
    void update_tool_highlight(ToolType current_tool);

    /// Set callback for speed control
    using SpeedCallback = std::function<void(int)>; // 0=pause, 1=normal, 2=fast, 3=ultra
    void set_speed_callback(SpeedCallback callback);

    void render(UIRenderer* renderer) override;

    // Layout constants
    static constexpr float BAR_HEIGHT = 48.0f;
    static constexpr float BUTTON_WIDTH = 40.0f;
    static constexpr float BUTTON_HEIGHT = 32.0f;
    static constexpr float GROUP_SPACING = 16.0f;
    static constexpr float BUTTON_SPACING = 4.0f;

private:
    ToolSelectCallback m_tool_callback;
    SpeedCallback m_speed_callback;

    // Track button references for highlighting
    struct ToolButton {
        ButtonWidget* widget = nullptr;
        ToolType tool;
    };
    std::vector<ToolButton> m_tool_buttons;

    // Build helpers
    ButtonWidget* add_tool_button(Widget* group, const std::string& label, ToolType tool);
    ButtonWidget* add_speed_button(Widget* group, const std::string& label, int speed);
    Widget* add_group(const std::string& group_label, float x_offset);
};

} // namespace ui
} // namespace sims3000

#endif // SIMS3000_UI_COMMANDARRAY_H
