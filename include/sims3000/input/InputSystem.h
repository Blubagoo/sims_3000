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
 * @enum MouseButton
 * @brief Mouse button identifiers.
 */
enum class MouseButton {
    Left,
    Right,
    Middle
};

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
 * @struct DragState
 * @brief Tracks mouse drag state.
 */
struct DragState {
    bool active = false;        // Is a drag currently in progress?
    MouseButton button = MouseButton::Left;  // Which button initiated the drag
    int startX = 0;             // X position where drag started
    int startY = 0;             // Y position where drag started
    int deltaX = 0;             // Total X movement since drag started
    int deltaY = 0;             // Total Y movement since drag started
    static constexpr int DRAG_THRESHOLD = 4;  // Minimum movement to start drag
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
     * Check if a mouse button is currently down.
     * @param button Mouse button to check
     */
    bool isMouseButtonDown(MouseButton button) const;

    /**
     * Check if a mouse button was just pressed this frame.
     * @param button Mouse button to check
     * @return true only on the frame the button was pressed
     */
    bool wasMouseButtonPressed(MouseButton button) const;

    /**
     * Check if a mouse button was just released this frame.
     * @param button Mouse button to check
     * @return true only on the frame the button was released
     */
    bool wasMouseButtonReleased(MouseButton button) const;

    /**
     * Show the mouse cursor.
     */
    void showCursor();

    /**
     * Hide the mouse cursor.
     */
    void hideCursor();

    /**
     * Check if the cursor is currently visible.
     */
    bool isCursorVisible() const;

    /**
     * Check if a drag operation is in progress.
     * A drag starts when a mouse button is held and the mouse moves
     * beyond a threshold distance.
     */
    bool isDragging() const;

    /**
     * Get the position where the current drag started.
     * @param outX Output X coordinate
     * @param outY Output Y coordinate
     * @return true if a drag is active
     */
    bool getDragStartPosition(int& outX, int& outY) const;

    /**
     * Get the total movement since the drag started.
     * @param outDeltaX Output X delta
     * @param outDeltaY Output Y delta
     * @return true if a drag is active
     */
    bool getDragDelta(int& outDeltaX, int& outDeltaY) const;

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
    void updateMouseButtonState(MouseButton button, bool down, int x, int y);
    void updateDragState(int x, int y);

    ActionMapping m_mapping;
    MouseState m_mouse;
    DragState m_drag;

    // Previous frame button state for edge detection
    bool m_prevLeftButton = false;
    bool m_prevRightButton = false;
    bool m_prevMiddleButton = false;

    // Button state changes this frame
    bool m_leftButtonPressed = false;
    bool m_leftButtonReleased = false;
    bool m_rightButtonPressed = false;
    bool m_rightButtonReleased = false;
    bool m_middleButtonPressed = false;
    bool m_middleButtonReleased = false;

    // Potential drag tracking (before threshold is met)
    bool m_potentialDrag = false;
    int m_potentialDragStartX = 0;
    int m_potentialDragStartY = 0;
    MouseButton m_potentialDragButton = MouseButton::Left;

    std::unordered_set<SDL_Scancode> m_keysDown;
    std::unordered_set<SDL_Scancode> m_keysPressed;
    std::unordered_set<SDL_Scancode> m_keysReleased;
};

} // namespace sims3000

#endif // SIMS3000_INPUT_INPUTSYSTEM_H
