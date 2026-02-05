/**
 * @file InputSystem.h
 * @brief Input polling and state management.
 */

#ifndef SIMS3000_INPUT_INPUTSYSTEM_H
#define SIMS3000_INPUT_INPUTSYSTEM_H

#include "sims3000/input/ActionMapping.h"
#include <SDL3/SDL.h>
#include <cstdint>
#include <unordered_set>

namespace sims3000 {

/**
 * @struct MouseState
 * @brief Current mouse input state.
 */
struct MouseState {
    int x = 0;              // Current X position
    int y = 0;              // Current Y position
    int deltaX = 0;         // Movement since last frame
    int deltaY = 0;         // Movement since last frame
    float wheelX = 0.0f;    // Horizontal scroll
    float wheelY = 0.0f;    // Vertical scroll
    bool leftButton = false;
    bool rightButton = false;
    bool middleButton = false;
};

/**
 * @class InputSystem
 * @brief Manages keyboard and mouse input.
 *
 * Polls SDL events, maintains key states, and provides
 * action-based input queries through ActionMapping.
 */
class InputSystem {
public:
    InputSystem();
    ~InputSystem() = default;

    // Non-copyable
    InputSystem(const InputSystem&) = delete;
    InputSystem& operator=(const InputSystem&) = delete;

    /**
     * Process an SDL event.
     * @param event SDL event to process
     * @return true if event was consumed
     */
    bool processEvent(const SDL_Event& event);

    /**
     * Begin new frame.
     * Resets per-frame state like mouse delta and wheel.
     */
    void beginFrame();

    /**
     * Check if a key is currently pressed.
     * @param scancode SDL scancode
     */
    bool isKeyDown(SDL_Scancode scancode) const;

    /**
     * Check if a key was just pressed this frame.
     * @param scancode SDL scancode
     */
    bool isKeyPressed(SDL_Scancode scancode) const;

    /**
     * Check if a key was just released this frame.
     * @param scancode SDL scancode
     */
    bool isKeyReleased(SDL_Scancode scancode) const;

    /**
     * Check if an action is active.
     * @param action Action to check
     */
    bool isActionDown(Action action) const;

    /**
     * Check if an action was just triggered.
     * @param action Action to check
     */
    bool isActionPressed(Action action) const;

    /**
     * Get current mouse state.
     */
    const MouseState& getMouse() const;

    /**
     * Get action mapping.
     */
    ActionMapping& getMapping();
    const ActionMapping& getMapping() const;

    /**
     * Load key bindings from config file.
     * @param path Path to config file
     * @return true on success
     */
    bool loadConfig(const char* path);

    /**
     * Save current key bindings to config file.
     * @param path Path to config file
     * @return true on success
     */
    bool saveConfig(const char* path) const;

private:
    void updateKeyState(SDL_Scancode scancode, bool down);

    ActionMapping m_mapping;
    MouseState m_mouse;

    std::unordered_set<SDL_Scancode> m_keysDown;
    std::unordered_set<SDL_Scancode> m_keysPressed;
    std::unordered_set<SDL_Scancode> m_keysReleased;
};

} // namespace sims3000

#endif // SIMS3000_INPUT_INPUTSYSTEM_H
