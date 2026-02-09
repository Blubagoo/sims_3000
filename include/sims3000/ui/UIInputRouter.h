/**
 * @file UIInputRouter.h
 * @brief Routes input events from InputSystem to the UI widget tree.
 *
 * UIInputRouter bridges the game's InputSystem with the UI widget hierarchy.
 * Each frame, process() is called after InputSystem has updated, and it:
 * - Hit-tests the mouse position against the widget tree
 * - Dispatches hover enter/leave events
 * - Dispatches mouse down/up/move events to the appropriate widget
 * - Reports whether the UI consumed the input (so the game can skip it)
 *
 * Usage:
 * @code
 *   UIInputRouter router;
 *   // Per-frame, after input.beginFrame() and event processing:
 *   UIInputResult result = router.process(input, ui_manager);
 *   if (!result.consumed) {
 *       // Forward input to game camera / world interaction
 *   }
 * @endcode
 *
 * Thread safety: not thread-safe. Call from the main/render thread only.
 */

#ifndef SIMS3000_UI_UIINPUTROUTER_H
#define SIMS3000_UI_UIINPUTROUTER_H

#include "sims3000/ui/Widget.h"
#include <cstdint>

namespace sims3000 {
class InputSystem; // forward declare
} // namespace sims3000

namespace sims3000 {
namespace ui {

class UIManager; // forward declare

/**
 * @struct UIInputResult
 * @brief Result of UI input processing for a single frame.
 */
struct UIInputResult {
    bool consumed = false;       ///< If true, input was handled by UI (don't pass to game)
    Widget* hit_widget = nullptr; ///< Widget that was hit (or nullptr)
};

/**
 * @class UIInputRouter
 * @brief Routes input events from InputSystem to the UI widget tree.
 *
 * Performs hit testing against the widget tree each frame and dispatches
 * mouse events (enter, leave, down, up, move) to the appropriate widgets.
 * Tracks hover and focus state across frames to generate correct
 * enter/leave transitions.
 */
class UIInputRouter {
public:
    UIInputRouter();

    /**
     * Process current input state and route to UI widgets.
     * Call once per frame after InputSystem has updated.
     * @param input The game's input system
     * @param manager The UI manager owning the widget tree
     * @return Result indicating if input was consumed by UI
     */
    UIInputResult process(const InputSystem& input, UIManager& manager);

    /**
     * Check if a screen position is over any UI widget.
     * @param root The root widget
     * @param x Screen X
     * @param y Screen Y
     * @return true if the point hits a UI widget
     */
    bool is_over_ui(Widget* root, float x, float y) const;

    /**
     * Get the currently hovered widget (from last process() call).
     * @return Pointer to hovered widget, or nullptr if none
     */
    Widget* get_hovered_widget() const;

    /**
     * Get the currently focused/pressed widget.
     * A widget becomes focused on mouse down and remains focused until
     * mouse up, even if the mouse moves off the widget.
     * @return Pointer to focused widget, or nullptr if none
     */
    Widget* get_focused_widget() const;

private:
    Widget* m_hovered = nullptr;       ///< Currently hovered widget
    Widget* m_focused = nullptr;       ///< Widget that received mouse down (tracks until mouse up)
    Widget* m_last_hovered = nullptr;  ///< Previous frame's hovered widget (for enter/leave)

    float m_last_mouse_x = 0.0f;
    float m_last_mouse_y = 0.0f;
    bool m_last_left_down = false;
};

} // namespace ui
} // namespace sims3000

#endif // SIMS3000_UI_UIINPUTROUTER_H
