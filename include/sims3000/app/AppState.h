/**
 * @file AppState.h
 * @brief Application state machine definitions.
 */

#ifndef SIMS3000_APP_APPSTATE_H
#define SIMS3000_APP_APPSTATE_H

namespace sims3000 {

/**
 * @enum AppState
 * @brief Application lifecycle states.
 */
enum class AppState {
    Menu,       // Main menu, not connected
    Connecting, // Connecting to server
    Loading,    // Loading assets/world
    Playing,    // Active gameplay
    Paused,     // Game paused (single player only)
    Exiting     // Shutting down
};

/**
 * @brief Get string name for app state.
 */
inline const char* getAppStateName(AppState state) {
    switch (state) {
        case AppState::Menu:       return "Menu";
        case AppState::Connecting: return "Connecting";
        case AppState::Loading:    return "Loading";
        case AppState::Playing:    return "Playing";
        case AppState::Paused:     return "Paused";
        case AppState::Exiting:    return "Exiting";
        default:                   return "Unknown";
    }
}

/**
 * @interface IStateHandler
 * @brief Interface for state-specific behavior.
 */
class IStateHandler {
public:
    virtual ~IStateHandler() = default;

    /// Called when entering this state
    virtual void onEnter() {}

    /// Called when exiting this state
    virtual void onExit() {}

    /// Called each frame while in this state
    virtual void onUpdate(float deltaTime) { (void)deltaTime; }
};

} // namespace sims3000

#endif // SIMS3000_APP_APPSTATE_H
