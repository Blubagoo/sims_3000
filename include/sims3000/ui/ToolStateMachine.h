/**
 * @file ToolStateMachine.h
 * @brief Tool state machine with cursor display and placement validation.
 *
 * Manages tool transitions, cursor visual state, and placement validity
 * for the Overseer's tool palette. Each tool type maps to a specific
 * cursor mode and color, with callbacks on tool change.
 *
 * Usage:
 * @code
 *   ToolStateMachine tsm;
 *   tsm.set_on_tool_change([](ToolType old_t, ToolType new_t) {
 *       log("Tool changed from {} to {}", old_t, new_t);
 *   });
 *
 *   tsm.set_tool(ToolType::ZoneHabitation);
 *   const auto& vs = tsm.get_visual_state();
 *   // vs.cursor_mode == CursorMode::ZoneBrush
 *   // vs.cursor_color == green
 *
 *   tsm.set_placement_validity(PlacementValidity::Invalid);
 *   tsm.cancel();  // reverts to Select
 * @endcode
 *
 * Thread safety: not thread-safe. Call from the main/render thread only.
 */

#ifndef SIMS3000_UI_TOOLSTATEMACHINE_H
#define SIMS3000_UI_TOOLSTATEMACHINE_H

#include "sims3000/ui/UIManager.h"
#include "sims3000/ui/Widget.h"

#include <cstdint>
#include <functional>
#include <string>

namespace sims3000 {
namespace ui {

/// Cursor display mode corresponding to tool behaviour.
enum class CursorMode : uint8_t {
    Arrow = 0,        ///< Default pointer cursor
    ZoneBrush,        ///< Area-drag brush for zone painting
    LinePlacement,    ///< Point-to-point line placement
    Bulldoze,         ///< Demolition crosshair
    Probe,            ///< Inspection / query cursor
    Grade,            ///< Terrain grading cursor
    Purge             ///< Zone removal cursor
};

/// Placement validity state for the current tool action.
enum class PlacementValidity : uint8_t {
    Unknown = 0,      ///< Validity not yet determined
    Valid,            ///< Placement is allowed
    Invalid           ///< Placement is blocked
};

/**
 * @struct ToolVisualState
 * @brief Current visual representation of the active tool's cursor.
 *
 * Updated whenever the tool changes or placement validity is set.
 * Consumed by the rendering layer to display the correct cursor.
 */
struct ToolVisualState {
    CursorMode cursor_mode = CursorMode::Arrow;
    Color cursor_color = {1.0f, 1.0f, 1.0f, 1.0f};
    PlacementValidity placement_valid = PlacementValidity::Unknown;
    std::string tooltip_text;
};

/**
 * @class ToolStateMachine
 * @brief Manages tool transitions and cursor display state.
 *
 * Tracks the currently active tool, computes the corresponding cursor
 * mode and color, and notifies listeners on tool changes. Also tracks
 * placement validity that game logic can update per-frame.
 */
class ToolStateMachine {
public:
    ToolStateMachine();

    /**
     * Set the active tool (triggers state transition).
     * If the tool is already active, this is a no-op.
     * @param tool The tool to activate
     */
    void set_tool(ToolType tool);

    /// Get the current tool.
    ToolType get_tool() const;

    /// Get the visual state for rendering the cursor.
    const ToolVisualState& get_visual_state() const;

    /**
     * Update placement validity (called by game logic each frame).
     * @param validity The current placement validity
     */
    void set_placement_validity(PlacementValidity validity);

    /// Cancel current tool (revert to Select).
    void cancel();

    /**
     * Get display name for current tool (alien terminology).
     * @return Reference to a static string - safe to hold across frames
     */
    const std::string& get_tool_display_name() const;

    /// Check if current tool is a placement tool (zone, infra, or structure).
    bool is_placement_tool() const;

    /// Check if current tool is a zone tool.
    bool is_zone_tool() const;

    /// Callback signature for tool change notifications.
    using ToolChangeCallback = std::function<void(ToolType old_tool, ToolType new_tool)>;

    /**
     * Register a callback invoked whenever the active tool changes.
     * @param callback Function to call with (old_tool, new_tool)
     */
    void set_on_tool_change(ToolChangeCallback callback);

private:
    ToolType m_current_tool = ToolType::Select;
    ToolVisualState m_visual_state;
    ToolChangeCallback m_on_change;

    /// Recompute m_visual_state from m_current_tool.
    void update_visual_state();

    /// Map a tool type to its cursor mode.
    CursorMode cursor_mode_for_tool(ToolType tool) const;

    /// Map a tool type to its cursor color.
    Color color_for_tool(ToolType tool) const;

    /// Map a tool type to its display name string.
    static const std::string& display_name_for_tool(ToolType tool);
};

} // namespace ui
} // namespace sims3000

#endif // SIMS3000_UI_TOOLSTATEMACHINE_H
