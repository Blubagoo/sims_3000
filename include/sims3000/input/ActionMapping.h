/**
 * @file ActionMapping.h
 * @brief Configurable action-to-key bindings.
 */

#ifndef SIMS3000_INPUT_ACTIONMAPPING_H
#define SIMS3000_INPUT_ACTIONMAPPING_H

#include <SDL3/SDL.h>
#include <unordered_map>
#include <vector>

namespace sims3000 {

/**
 * @enum Action
 * @brief Game actions that can be bound to keys.
 */
enum class Action {
    // Camera movement
    PAN_UP,
    PAN_DOWN,
    PAN_LEFT,
    PAN_RIGHT,
    ZOOM_IN,
    ZOOM_OUT,
    ROTATE_CW,
    ROTATE_CCW,

    // Game controls
    PAUSE,
    SPEED_1,
    SPEED_2,
    SPEED_3,

    // UI navigation
    CONFIRM,
    CANCEL,
    MENU,

    // Debug
    DEBUG_TOGGLE,
    DEBUG_CAMERA,
    DEBUG_GRID,
    DEBUG_WIREFRAME,
    DEBUG_STATS,  // Toggle render statistics overlay (F3)
    DEBUG_BOUNDING_BOX,  // Toggle bounding box visualization (B)

    // View modes
    TOGGLE_UNDERGROUND,

    // System
    FULLSCREEN,
    QUIT,

    COUNT  // Must be last
};

/**
 * @class ActionMapping
 * @brief Maps actions to keyboard/mouse inputs.
 *
 * Supports multiple keys per action and provides
 * methods to query active bindings.
 */
class ActionMapping {
public:
    ActionMapping();
    ~ActionMapping() = default;

    /**
     * Bind a key to an action.
     * @param action Action to bind
     * @param scancode Key to bind
     * @param append If true, add to existing bindings; otherwise replace
     */
    void bind(Action action, SDL_Scancode scancode, bool append = false);

    /**
     * Unbind a key from an action.
     * @param action Action to unbind from
     * @param scancode Key to unbind
     */
    void unbind(Action action, SDL_Scancode scancode);

    /**
     * Clear all bindings for an action.
     * @param action Action to clear
     */
    void clearBindings(Action action);

    /**
     * Get all keys bound to an action.
     * @param action Action to query
     * @return Vector of bound scancodes
     */
    const std::vector<SDL_Scancode>& getBindings(Action action) const;

    /**
     * Find action(s) bound to a key.
     * @param scancode Key to look up
     * @return Vector of actions bound to this key
     */
    std::vector<Action> getActionsForKey(SDL_Scancode scancode) const;

    /**
     * Reset to default bindings.
     */
    void resetToDefaults();

    /**
     * Get human-readable name for an action.
     * @param action Action to name
     * @return Action name string
     */
    static const char* getActionName(Action action);

private:
    std::unordered_map<Action, std::vector<SDL_Scancode>> m_bindings;
    static const std::vector<SDL_Scancode> s_emptyBindings;
};

} // namespace sims3000

#endif // SIMS3000_INPUT_ACTIONMAPPING_H
