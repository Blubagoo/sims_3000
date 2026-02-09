/**
 * @file KeyboardShortcuts.h
 * @brief Keyboard shortcut mapping system for tools, panels, and overlays.
 *
 * Provides a configurable keyboard shortcut system that maps key presses
 * to game actions such as tool selection, overlay cycling, pause/resume,
 * and simulation speed control.
 *
 * Features:
 * - Default bindings for all core actions (zone tools, infrastructure, overlays)
 * - Support for modifier keys (Shift, Ctrl, Alt)
 * - Rebindable shortcuts with set_binding()
 * - Reverse lookup for tooltip display (get_binding_for_action)
 * - Human-readable key names for UI display
 *
 * Usage:
 * @code
 *   KeyboardShortcuts shortcuts;
 *   // In your input processing loop:
 *   auto action = shortcuts.process_key(scancode, shift, ctrl, alt);
 *   if (action) {
 *       switch (*action) {
 *           case ShortcutAction::SelectZoneHabitation:
 *               ui_manager.set_tool(ToolType::ZoneHabitation);
 *               break;
 *           case ShortcutAction::TogglePause:
 *               simulation.toggle_pause();
 *               break;
 *           // ...
 *       }
 *   }
 * @endcode
 *
 * Thread safety: not thread-safe. Call from the main/render thread only.
 */

#ifndef SIMS3000_UI_KEYBOARDSHORTCUTS_H
#define SIMS3000_UI_KEYBOARDSHORTCUTS_H

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace sims3000 {
namespace ui {

/**
 * @enum ShortcutAction
 * @brief Actions that can be triggered by keyboard shortcuts.
 */
enum class ShortcutAction : uint8_t {
    SelectZoneHabitation = 0, ///< Select habitation (residential) zone tool [1]
    SelectZoneExchange,       ///< Select exchange (commercial) zone tool [2]
    SelectZoneFabrication,    ///< Select fabrication (industrial) zone tool [3]
    SelectPathway,            ///< Select pathway (road) tool [R]
    SelectEnergyConduit,      ///< Select energy conduit tool [P]
    SelectFluidConduit,       ///< Select fluid conduit tool [W]
    SelectBulldoze,           ///< Select bulldoze tool [B]
    SelectProbe,              ///< Select probe/query tool [Q]
    CycleOverlay,             ///< Cycle through overlay types [TAB]
    CancelOrClose,            ///< Cancel current tool or close panel [ESC]
    TogglePause,              ///< Toggle simulation pause [Space]
    SpeedUp,                  ///< Increase simulation speed [+/=]
    SpeedDown,                ///< Decrease simulation speed [-]
    ToggleUIMode              ///< Toggle between Legacy and Holo UI modes [F1]
};

/**
 * @struct ShortcutBinding
 * @brief Maps a key combination to a shortcut action.
 *
 * A binding consists of an SDL scancode and optional modifier flags.
 * Modifier flags must all match for the binding to trigger.
 */
struct ShortcutBinding {
    int key_code;             ///< SDL_SCANCODE_* value
    ShortcutAction action;    ///< Action to perform when triggered
    bool shift = false;       ///< Requires Shift modifier
    bool ctrl = false;        ///< Requires Ctrl modifier
    bool alt = false;         ///< Requires Alt modifier
};

/**
 * @class KeyboardShortcuts
 * @brief Manages keyboard shortcut bindings and key-to-action resolution.
 *
 * Maintains a list of shortcut bindings that map key combinations to
 * ShortcutAction values. Supports both simple (no modifier) lookups via
 * an unordered_map for performance, and full modifier-aware lookups via
 * linear scan of the binding list.
 *
 * The constructor populates default bindings. Bindings can be customized
 * at runtime via set_binding() and restored via reset_defaults().
 */
class KeyboardShortcuts {
public:
    /**
     * Construct with default shortcut bindings.
     *
     * Default bindings:
     * | Key     | Action                |
     * |---------|-----------------------|
     * | 1       | SelectZoneHabitation  |
     * | 2       | SelectZoneExchange    |
     * | 3       | SelectZoneFabrication |
     * | R       | SelectPathway         |
     * | P       | SelectEnergyConduit   |
     * | W       | SelectFluidConduit    |
     * | B       | SelectBulldoze        |
     * | Q       | SelectProbe           |
     * | TAB     | CycleOverlay          |
     * | ESC     | CancelOrClose         |
     * | Space   | TogglePause           |
     * | =/+     | SpeedUp               |
     * | -       | SpeedDown             |
     * | F1      | ToggleUIMode          |
     */
    KeyboardShortcuts();

    /**
     * Process a key press and return the matching action, if any.
     *
     * First checks the fast lookup map for bindings with no modifiers.
     * If no match or modifiers are active, falls back to a full scan
     * of all bindings checking modifier flags.
     *
     * @param key_code SDL_SCANCODE_* value of the pressed key
     * @param shift    true if Shift is held
     * @param ctrl     true if Ctrl is held
     * @param alt      true if Alt is held
     * @return The matching ShortcutAction, or std::nullopt if no binding matches
     */
    std::optional<ShortcutAction> process_key(int key_code, bool shift, bool ctrl, bool alt) const;

    /**
     * Reverse lookup: find the binding for a given action.
     * Useful for generating tooltip text (e.g., "Bulldoze [B]").
     *
     * @param action The action to look up
     * @return The binding if found, or std::nullopt
     */
    std::optional<ShortcutBinding> get_binding_for_action(ShortcutAction action) const;

    /**
     * Get a human-readable name for a key code.
     * Returns uppercase letter names for common keys, or
     * descriptive names for special keys (Tab, Escape, Space, etc.).
     *
     * @param key_code SDL_SCANCODE_* value
     * @return Human-readable key name (e.g., "B", "Tab", "Space", "F1")
     */
    static std::string get_key_name(int key_code);

    /**
     * Set or replace the binding for an action.
     *
     * If the action already has a binding, it is replaced. Otherwise
     * a new binding is appended. The fast lookup map is rebuilt.
     *
     * @param action   The action to bind
     * @param key_code SDL_SCANCODE_* value for the key
     * @param shift    Require Shift modifier (default: false)
     * @param ctrl     Require Ctrl modifier (default: false)
     * @param alt      Require Alt modifier (default: false)
     */
    void set_binding(ShortcutAction action, int key_code,
                     bool shift = false, bool ctrl = false, bool alt = false);

    /**
     * Reset all bindings to their default values.
     * Clears any custom bindings and restores the factory defaults.
     */
    void reset_defaults();

    /**
     * Get all current bindings (read-only).
     * @return Reference to the internal bindings vector
     */
    const std::vector<ShortcutBinding>& get_bindings() const;

private:
    /// All active shortcut bindings.
    std::vector<ShortcutBinding> m_bindings;

    /// Fast lookup for bindings that have no modifier keys.
    /// Maps key_code -> ShortcutAction for O(1) lookup of simple shortcuts.
    std::unordered_map<int, ShortcutAction> m_simple_lookup;

    /**
     * Rebuild the simple (no-modifier) lookup map from m_bindings.
     * Called after any modification to m_bindings.
     */
    void rebuild_lookup();
};

} // namespace ui
} // namespace sims3000

#endif // SIMS3000_UI_KEYBOARDSHORTCUTS_H
