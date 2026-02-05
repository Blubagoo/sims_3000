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
            return true;

        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            switch (event.button.button) {
                case SDL_BUTTON_LEFT:   m_mouse.leftButton = true; break;
                case SDL_BUTTON_RIGHT:  m_mouse.rightButton = true; break;
                case SDL_BUTTON_MIDDLE: m_mouse.middleButton = true; break;
            }
            return true;

        case SDL_EVENT_MOUSE_BUTTON_UP:
            switch (event.button.button) {
                case SDL_BUTTON_LEFT:   m_mouse.leftButton = false; break;
                case SDL_BUTTON_RIGHT:  m_mouse.rightButton = false; break;
                case SDL_BUTTON_MIDDLE: m_mouse.middleButton = false; break;
            }
            return true;

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

} // namespace sims3000
