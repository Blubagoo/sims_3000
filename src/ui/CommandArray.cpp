/**
 * @file CommandArray.cpp
 * @brief Implementation of the classic mode horizontal command array (toolbar).
 */

#include "sims3000/ui/CommandArray.h"

namespace sims3000 {
namespace ui {

// =========================================================================
// Construction
// =========================================================================

CommandArray::CommandArray() {
    title = "";
    closable = false;
    draggable = false;

    // Docked at top of screen, full width placeholder (caller should set width)
    // Height is fixed to BAR_HEIGHT
    bounds = {0.0f, 0.0f, 1280.0f, BAR_HEIGHT};
}

// =========================================================================
// Layout building
// =========================================================================

void CommandArray::build_layout() {
    // Clear any existing children from a previous build
    children.clear();
    m_tool_buttons.clear();

    float x_cursor = 10.0f;

    // -- BUILD group: zone + infrastructure buttons --------------------------
    {
        Widget* build_group = add_group("BUILD", x_cursor);

        // Zone buttons
        add_tool_button(build_group, "H", ToolType::ZoneHabitation);
        add_tool_button(build_group, "E", ToolType::ZoneExchange);
        add_tool_button(build_group, "F", ToolType::ZoneFabrication);

        // Infrastructure buttons
        add_tool_button(build_group, "Path", ToolType::Pathway);
        add_tool_button(build_group, "Eng", ToolType::EnergyConduit);
        add_tool_button(build_group, "Flu", ToolType::FluidConduit);

        // Advance cursor past this group:
        // label area (small) + 6 buttons with spacing
        float group_width = 6.0f * (BUTTON_WIDTH + BUTTON_SPACING);
        x_cursor += group_width + GROUP_SPACING;
    }

    // -- MODIFY group --------------------------------------------------------
    {
        Widget* modify_group = add_group("MODIFY", x_cursor);

        add_tool_button(modify_group, "Blz", ToolType::Bulldoze);
        add_tool_button(modify_group, "Prg", ToolType::Purge);
        add_tool_button(modify_group, "Grd", ToolType::Grade);

        float group_width = 3.0f * (BUTTON_WIDTH + BUTTON_SPACING);
        x_cursor += group_width + GROUP_SPACING;
    }

    // -- INSPECT group -------------------------------------------------------
    {
        Widget* inspect_group = add_group("INSPECT", x_cursor);

        add_tool_button(inspect_group, "Prb", ToolType::Probe);

        float group_width = 1.0f * (BUTTON_WIDTH + BUTTON_SPACING);
        x_cursor += group_width + GROUP_SPACING;
    }

    // -- VIEW group: speed controls ------------------------------------------
    {
        Widget* view_group = add_group("VIEW", x_cursor);

        add_speed_button(view_group, "||", 0);   // Pause
        add_speed_button(view_group, ">",  1);   // Normal
        add_speed_button(view_group, ">>", 2);   // Fast
        add_speed_button(view_group, ">>>", 3);  // Ultra
    }
}

Widget* CommandArray::add_group(const std::string& group_label, float x_offset) {
    // Create a container widget for this group
    auto group = std::make_unique<Widget>();
    group->bounds = {x_offset, 0.0f, 0.0f, BAR_HEIGHT};

    // Add a small label above/at the top of the group
    auto label = std::make_unique<LabelWidget>();
    label->text = group_label;
    label->font_size = FontSize::Small;
    label->text_color = {0.6f, 0.8f, 0.8f, 1.0f}; // Subdued teal
    label->bounds = {0.0f, 2.0f, 100.0f, 12.0f};
    group->add_child(std::move(label));

    Widget* raw_ptr = add_child(std::move(group));
    return raw_ptr;
}

ButtonWidget* CommandArray::add_tool_button(Widget* group, const std::string& label, ToolType tool) {
    // Count existing button children to compute position
    // First child is the label, so buttons start at index 1
    int button_index = static_cast<int>(group->children.size()) - 1;
    if (button_index < 0) button_index = 0;

    auto btn = std::make_unique<ButtonWidget>();
    btn->text = label;
    btn->bounds = {
        static_cast<float>(button_index) * (BUTTON_WIDTH + BUTTON_SPACING),
        14.0f, // Below the group label
        BUTTON_WIDTH,
        BUTTON_HEIGHT
    };

    // Capture tool by value for the callback
    ToolType captured_tool = tool;
    btn->on_click = [this, captured_tool]() {
        if (m_tool_callback) {
            m_tool_callback(captured_tool);
        }
    };

    ButtonWidget* raw_ptr = static_cast<ButtonWidget*>(group->add_child(std::move(btn)));

    // Track for highlighting
    m_tool_buttons.push_back({raw_ptr, tool});

    return raw_ptr;
}

ButtonWidget* CommandArray::add_speed_button(Widget* group, const std::string& label, int speed) {
    // Count existing button children to compute position
    int button_index = static_cast<int>(group->children.size()) - 1;
    if (button_index < 0) button_index = 0;

    auto btn = std::make_unique<ButtonWidget>();
    btn->text = label;
    btn->bounds = {
        static_cast<float>(button_index) * (BUTTON_WIDTH + BUTTON_SPACING),
        14.0f, // Below the group label
        BUTTON_WIDTH,
        BUTTON_HEIGHT
    };

    int captured_speed = speed;
    btn->on_click = [this, captured_speed]() {
        if (m_speed_callback) {
            m_speed_callback(captured_speed);
        }
    };

    ButtonWidget* raw_ptr = static_cast<ButtonWidget*>(group->add_child(std::move(btn)));
    return raw_ptr;
}

// =========================================================================
// Callbacks
// =========================================================================

void CommandArray::set_tool_callback(ToolSelectCallback callback) {
    m_tool_callback = std::move(callback);
}

void CommandArray::set_speed_callback(SpeedCallback callback) {
    m_speed_callback = std::move(callback);
}

// =========================================================================
// Tool highlighting
// =========================================================================

void CommandArray::update_tool_highlight(ToolType current_tool) {
    for (auto& tb : m_tool_buttons) {
        if (tb.widget) {
            // Use the pressed state to visually indicate the active tool
            tb.widget->set_pressed(tb.tool == current_tool);
        }
    }
}

// =========================================================================
// Rendering
// =========================================================================

void CommandArray::render(UIRenderer* renderer) {
    if (!visible) {
        return;
    }

    // Delegate to PanelWidget which draws the panel background and children
    PanelWidget::render(renderer);
}

} // namespace ui
} // namespace sims3000
