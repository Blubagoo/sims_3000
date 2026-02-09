/**
 * @file UIInputRouter.cpp
 * @brief Implementation of UIInputRouter - routes input to UI widgets.
 */

#include "sims3000/ui/UIInputRouter.h"
#include "sims3000/input/InputSystem.h"
#include "sims3000/ui/UIManager.h"

namespace sims3000 {
namespace ui {

UIInputRouter::UIInputRouter() = default;

UIInputResult UIInputRouter::process(const InputSystem& input, UIManager& manager) {
    UIInputResult result;

    // 1. Get mouse state from InputSystem
    const MouseState& mouse = input.getMouse();
    float mouseX = static_cast<float>(mouse.x);
    float mouseY = static_cast<float>(mouse.y);

    // 2. Compute screen bounds on the root widget tree
    Widget* root = manager.get_root();
    if (!root) {
        m_hovered = nullptr;
        m_last_hovered = nullptr;
        m_focused = nullptr;
        return result;
    }
    root->compute_screen_bounds();

    // 3. Hit test: find the deepest widget under the mouse
    m_hovered = root->find_child_at(mouseX, mouseY);

    // 4. Handle hover transitions (enter/leave)
    if (m_hovered != m_last_hovered) {
        // Mouse left the old widget
        if (m_last_hovered) {
            m_last_hovered->on_mouse_leave();
            m_last_hovered->set_hovered(false);
        }
        // Mouse entered the new widget
        if (m_hovered) {
            m_hovered->on_mouse_enter();
            m_hovered->set_hovered(true);
        }
    }

    // 5. Handle mouse move
    if (m_hovered) {
        m_hovered->on_mouse_move(mouseX, mouseY);
    }

    // 6. Handle mouse down (left button just pressed)
    if (input.wasMouseButtonPressed(MouseButton::Left) && m_hovered) {
        m_focused = m_hovered;
        m_focused->on_mouse_down(0, mouseX, mouseY);
        m_focused->set_pressed(true);
    }

    // 7. Handle mouse up (left button just released)
    if (input.wasMouseButtonReleased(MouseButton::Left) && m_focused) {
        m_focused->on_mouse_up(0, mouseX, mouseY);
        m_focused->set_pressed(false);
        m_focused = nullptr;
    }

    // 8. Build result
    result.consumed = (m_hovered != nullptr) || (m_focused != nullptr);
    result.hit_widget = m_hovered;

    // Update tracking state for next frame
    m_last_hovered = m_hovered;
    m_last_mouse_x = mouseX;
    m_last_mouse_y = mouseY;
    m_last_left_down = input.isMouseButtonDown(MouseButton::Left);

    return result;
}

bool UIInputRouter::is_over_ui(Widget* root, float x, float y) const {
    if (!root) {
        return false;
    }
    Widget* hit = root->find_child_at(x, y);
    return hit != nullptr;
}

Widget* UIInputRouter::get_hovered_widget() const {
    return m_hovered;
}

Widget* UIInputRouter::get_focused_widget() const {
    return m_focused;
}

} // namespace ui
} // namespace sims3000
