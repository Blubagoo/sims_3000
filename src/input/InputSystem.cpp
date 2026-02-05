/**
 * @file InputSystem.cpp
 * @brief InputSystem implementation.
 */

#include "sims3000/input/InputSystem.h"

namespace sims3000 {

InputSystem::InputSystem() = default;

bool InputSystem::processEvent(const SDL_Event& event) {
    switch (event.type) {
        case SDL_EVENT_KEY_DOWN:
            if (!event.key.repeat) {
                updateKeyState(event.key.scancode, true);
            }
            return true;

        case SDL_EVENT_KEY_UP:
            updateKeyState(event.key.scancode, false);
            return true;

        case SDL_EVENT_MOUSE_MOTION:
            m_mouse.x = static_cast<int>(event.motion.x);
            m_mouse.y = static_cast<int>(event.motion.y);
            m_mouse.deltaX += static_cast<int>(event.motion.xrel);
            m_mouse.deltaY += static_cast<int>(event.motion.yrel);
            updateDragState(m_mouse.x, m_mouse.y);
            return true;

        case SDL_EVENT_MOUSE_BUTTON_DOWN: {
            int x = static_cast<int>(event.button.x);
            int y = static_cast<int>(event.button.y);
            switch (event.button.button) {
                case SDL_BUTTON_LEFT:
                    updateMouseButtonState(MouseButton::Left, true, x, y);
                    break;
                case SDL_BUTTON_RIGHT:
                    updateMouseButtonState(MouseButton::Right, true, x, y);
                    break;
                case SDL_BUTTON_MIDDLE:
                    updateMouseButtonState(MouseButton::Middle, true, x, y);
                    break;
            }
            return true;
        }

        case SDL_EVENT_MOUSE_BUTTON_UP: {
            int x = static_cast<int>(event.button.x);
            int y = static_cast<int>(event.button.y);
            switch (event.button.button) {
                case SDL_BUTTON_LEFT:
                    updateMouseButtonState(MouseButton::Left, false, x, y);
                    break;
                case SDL_BUTTON_RIGHT:
                    updateMouseButtonState(MouseButton::Right, false, x, y);
                    break;
                case SDL_BUTTON_MIDDLE:
                    updateMouseButtonState(MouseButton::Middle, false, x, y);
                    break;
            }
            return true;
        }

        case SDL_EVENT_MOUSE_WHEEL:
            m_mouse.wheelX += event.wheel.x;
            m_mouse.wheelY += event.wheel.y;
            return true;

        default:
            return false;
    }
}

void InputSystem::beginFrame() {
    m_keysPressed.clear();
    m_keysReleased.clear();
    m_mouse.deltaX = 0;
    m_mouse.deltaY = 0;
    m_mouse.wheelX = 0.0f;
    m_mouse.wheelY = 0.0f;

    // Store previous button states and reset edge detection flags
    m_prevLeftButton = m_mouse.leftButton;
    m_prevRightButton = m_mouse.rightButton;
    m_prevMiddleButton = m_mouse.middleButton;

    m_leftButtonPressed = false;
    m_leftButtonReleased = false;
    m_rightButtonPressed = false;
    m_rightButtonReleased = false;
    m_middleButtonPressed = false;
    m_middleButtonReleased = false;
}

bool InputSystem::isKeyDown(SDL_Scancode scancode) const {
    return m_keysDown.count(scancode) > 0;
}

bool InputSystem::isKeyPressed(SDL_Scancode scancode) const {
    return m_keysPressed.count(scancode) > 0;
}

bool InputSystem::isKeyReleased(SDL_Scancode scancode) const {
    return m_keysReleased.count(scancode) > 0;
}

bool InputSystem::isActionDown(Action action) const {
    const auto& bindings = m_mapping.getBindings(action);
    for (SDL_Scancode scancode : bindings) {
        if (isKeyDown(scancode)) {
            return true;
        }
    }
    return false;
}

bool InputSystem::isActionPressed(Action action) const {
    const auto& bindings = m_mapping.getBindings(action);
    for (SDL_Scancode scancode : bindings) {
        if (isKeyPressed(scancode)) {
            return true;
        }
    }
    return false;
}

const MouseState& InputSystem::getMouse() const {
    return m_mouse;
}

ActionMapping& InputSystem::getMapping() {
    return m_mapping;
}

const ActionMapping& InputSystem::getMapping() const {
    return m_mapping;
}

bool InputSystem::loadConfig(const char* path) {
    // TODO: Implement JSON config loading when nlohmann/json is available
    (void)path;
    return false;
}

bool InputSystem::saveConfig(const char* path) const {
    // TODO: Implement JSON config saving when nlohmann/json is available
    (void)path;
    return false;
}

void InputSystem::updateKeyState(SDL_Scancode scancode, bool down) {
    if (down) {
        m_keysDown.insert(scancode);
        m_keysPressed.insert(scancode);
    } else {
        m_keysDown.erase(scancode);
        m_keysReleased.insert(scancode);
    }
}

void InputSystem::updateMouseButtonState(MouseButton button, bool down, int x, int y) {
    switch (button) {
        case MouseButton::Left:
            m_mouse.leftButton = down;
            if (down) {
                m_leftButtonPressed = true;
            } else {
                m_leftButtonReleased = true;
            }
            break;
        case MouseButton::Right:
            m_mouse.rightButton = down;
            if (down) {
                m_rightButtonPressed = true;
            } else {
                m_rightButtonReleased = true;
            }
            break;
        case MouseButton::Middle:
            m_mouse.middleButton = down;
            if (down) {
                m_middleButtonPressed = true;
            } else {
                m_middleButtonReleased = true;
            }
            break;
    }

    if (down) {
        // Start potential drag tracking
        m_potentialDrag = true;
        m_potentialDragStartX = x;
        m_potentialDragStartY = y;
        m_potentialDragButton = button;
    } else {
        // Button released - end any drag involving this button
        if (m_drag.active && m_drag.button == button) {
            m_drag.active = false;
        }
        if (m_potentialDrag && m_potentialDragButton == button) {
            m_potentialDrag = false;
        }
    }
}

void InputSystem::updateDragState(int x, int y) {
    if (m_drag.active) {
        // Update drag delta
        m_drag.deltaX = x - m_drag.startX;
        m_drag.deltaY = y - m_drag.startY;
    } else if (m_potentialDrag) {
        // Check if we've moved beyond the threshold
        int dx = x - m_potentialDragStartX;
        int dy = y - m_potentialDragStartY;
        int distSq = dx * dx + dy * dy;
        int thresholdSq = DragState::DRAG_THRESHOLD * DragState::DRAG_THRESHOLD;

        if (distSq >= thresholdSq) {
            // Start the drag
            m_drag.active = true;
            m_drag.button = m_potentialDragButton;
            m_drag.startX = m_potentialDragStartX;
            m_drag.startY = m_potentialDragStartY;
            m_drag.deltaX = dx;
            m_drag.deltaY = dy;
            m_potentialDrag = false;
        }
    }
}

bool InputSystem::isMouseButtonDown(MouseButton button) const {
    switch (button) {
        case MouseButton::Left:   return m_mouse.leftButton;
        case MouseButton::Right:  return m_mouse.rightButton;
        case MouseButton::Middle: return m_mouse.middleButton;
    }
    return false;
}

bool InputSystem::wasMouseButtonPressed(MouseButton button) const {
    switch (button) {
        case MouseButton::Left:   return m_leftButtonPressed;
        case MouseButton::Right:  return m_rightButtonPressed;
        case MouseButton::Middle: return m_middleButtonPressed;
    }
    return false;
}

bool InputSystem::wasMouseButtonReleased(MouseButton button) const {
    switch (button) {
        case MouseButton::Left:   return m_leftButtonReleased;
        case MouseButton::Right:  return m_rightButtonReleased;
        case MouseButton::Middle: return m_middleButtonReleased;
    }
    return false;
}

void InputSystem::showCursor() {
    SDL_ShowCursor();
}

void InputSystem::hideCursor() {
    SDL_HideCursor();
}

bool InputSystem::isCursorVisible() const {
    return SDL_CursorVisible();
}

bool InputSystem::isDragging() const {
    return m_drag.active;
}

bool InputSystem::getDragStartPosition(int& outX, int& outY) const {
    if (!m_drag.active) {
        return false;
    }
    outX = m_drag.startX;
    outY = m_drag.startY;
    return true;
}

bool InputSystem::getDragDelta(int& outDeltaX, int& outDeltaY) const {
    if (!m_drag.active) {
        return false;
    }
    outDeltaX = m_drag.deltaX;
    outDeltaY = m_drag.deltaY;
    return true;
}

} // namespace sims3000
