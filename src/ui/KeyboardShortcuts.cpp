/**
 * @file KeyboardShortcuts.cpp
 * @brief Implementation of KeyboardShortcuts - keyboard shortcut mapping system.
 */

#include "sims3000/ui/KeyboardShortcuts.h"

#include <SDL3/SDL_scancode.h>

namespace sims3000 {
namespace ui {

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

KeyboardShortcuts::KeyboardShortcuts() {
    reset_defaults();
}

// ---------------------------------------------------------------------------
// Default bindings
// ---------------------------------------------------------------------------

void KeyboardShortcuts::reset_defaults() {
    m_bindings.clear();
    m_bindings.reserve(14);

    // Zone tools: 1, 2, 3
    m_bindings.push_back({SDL_SCANCODE_1, ShortcutAction::SelectZoneHabitation, false, false, false});
    m_bindings.push_back({SDL_SCANCODE_2, ShortcutAction::SelectZoneExchange,   false, false, false});
    m_bindings.push_back({SDL_SCANCODE_3, ShortcutAction::SelectZoneFabrication, false, false, false});

    // Infrastructure tools: R, P, W
    m_bindings.push_back({SDL_SCANCODE_R, ShortcutAction::SelectPathway,       false, false, false});
    m_bindings.push_back({SDL_SCANCODE_P, ShortcutAction::SelectEnergyConduit, false, false, false});
    m_bindings.push_back({SDL_SCANCODE_W, ShortcutAction::SelectFluidConduit,  false, false, false});

    // Other tools: B, Q
    m_bindings.push_back({SDL_SCANCODE_B, ShortcutAction::SelectBulldoze,      false, false, false});
    m_bindings.push_back({SDL_SCANCODE_Q, ShortcutAction::SelectProbe,         false, false, false});

    // Overlay cycling: TAB
    m_bindings.push_back({SDL_SCANCODE_TAB,    ShortcutAction::CycleOverlay,   false, false, false});

    // Cancel / close: ESC
    m_bindings.push_back({SDL_SCANCODE_ESCAPE, ShortcutAction::CancelOrClose,  false, false, false});

    // Pause / resume: Space
    m_bindings.push_back({SDL_SCANCODE_SPACE,  ShortcutAction::TogglePause,    false, false, false});

    // Speed control: =/+ and -
    m_bindings.push_back({SDL_SCANCODE_EQUALS, ShortcutAction::SpeedUp,        false, false, false});
    m_bindings.push_back({SDL_SCANCODE_MINUS,  ShortcutAction::SpeedDown,      false, false, false});

    // UI mode toggle: F1
    m_bindings.push_back({SDL_SCANCODE_F1,     ShortcutAction::ToggleUIMode,   false, false, false});

    rebuild_lookup();
}

// ---------------------------------------------------------------------------
// Key processing
// ---------------------------------------------------------------------------

std::optional<ShortcutAction> KeyboardShortcuts::process_key(
    int key_code, bool shift, bool ctrl, bool alt) const
{
    // Fast path: if no modifiers are held, use the hash map
    if (!shift && !ctrl && !alt) {
        auto it = m_simple_lookup.find(key_code);
        if (it != m_simple_lookup.end()) {
            return it->second;
        }
    }

    // Slow path: scan all bindings for a modifier-aware match
    for (const auto& binding : m_bindings) {
        if (binding.key_code == key_code &&
            binding.shift == shift &&
            binding.ctrl  == ctrl &&
            binding.alt   == alt) {
            return binding.action;
        }
    }

    return std::nullopt;
}

// ---------------------------------------------------------------------------
// Reverse lookup
// ---------------------------------------------------------------------------

std::optional<ShortcutBinding> KeyboardShortcuts::get_binding_for_action(
    ShortcutAction action) const
{
    for (const auto& binding : m_bindings) {
        if (binding.action == action) {
            return binding;
        }
    }
    return std::nullopt;
}

// ---------------------------------------------------------------------------
// Human-readable key names
// ---------------------------------------------------------------------------

std::string KeyboardShortcuts::get_key_name(int key_code) {
    // Letters A-Z
    if (key_code >= SDL_SCANCODE_A && key_code <= SDL_SCANCODE_Z) {
        char letter = static_cast<char>('A' + (key_code - SDL_SCANCODE_A));
        return std::string(1, letter);
    }

    // Number row 1-0
    if (key_code >= SDL_SCANCODE_1 && key_code <= SDL_SCANCODE_9) {
        char digit = static_cast<char>('1' + (key_code - SDL_SCANCODE_1));
        return std::string(1, digit);
    }
    if (key_code == SDL_SCANCODE_0) {
        return "0";
    }

    // Function keys F1-F12
    if (key_code >= SDL_SCANCODE_F1 && key_code <= SDL_SCANCODE_F12) {
        int fn = 1 + (key_code - SDL_SCANCODE_F1);
        return "F" + std::to_string(fn);
    }

    // Special keys
    switch (key_code) {
        case SDL_SCANCODE_TAB:       return "Tab";
        case SDL_SCANCODE_ESCAPE:    return "Esc";
        case SDL_SCANCODE_SPACE:     return "Space";
        case SDL_SCANCODE_RETURN:    return "Enter";
        case SDL_SCANCODE_BACKSPACE: return "Backspace";
        case SDL_SCANCODE_DELETE:    return "Delete";
        case SDL_SCANCODE_INSERT:    return "Insert";
        case SDL_SCANCODE_HOME:      return "Home";
        case SDL_SCANCODE_END:       return "End";
        case SDL_SCANCODE_PAGEUP:    return "PageUp";
        case SDL_SCANCODE_PAGEDOWN:  return "PageDown";
        case SDL_SCANCODE_UP:        return "Up";
        case SDL_SCANCODE_DOWN:      return "Down";
        case SDL_SCANCODE_LEFT:      return "Left";
        case SDL_SCANCODE_RIGHT:     return "Right";
        case SDL_SCANCODE_EQUALS:    return "=";
        case SDL_SCANCODE_MINUS:     return "-";
        case SDL_SCANCODE_LSHIFT:    return "LShift";
        case SDL_SCANCODE_RSHIFT:    return "RShift";
        case SDL_SCANCODE_LCTRL:     return "LCtrl";
        case SDL_SCANCODE_RCTRL:     return "RCtrl";
        case SDL_SCANCODE_LALT:      return "LAlt";
        case SDL_SCANCODE_RALT:      return "RAlt";
        default:                     return "Key(" + std::to_string(key_code) + ")";
    }
}

// ---------------------------------------------------------------------------
// Rebinding
// ---------------------------------------------------------------------------

void KeyboardShortcuts::set_binding(
    ShortcutAction action, int key_code,
    bool shift, bool ctrl, bool alt)
{
    // Replace existing binding for this action, if any
    for (auto& binding : m_bindings) {
        if (binding.action == action) {
            binding.key_code = key_code;
            binding.shift = shift;
            binding.ctrl = ctrl;
            binding.alt = alt;
            rebuild_lookup();
            return;
        }
    }

    // No existing binding - add a new one
    m_bindings.push_back({key_code, action, shift, ctrl, alt});
    rebuild_lookup();
}

// ---------------------------------------------------------------------------
// Accessors
// ---------------------------------------------------------------------------

const std::vector<ShortcutBinding>& KeyboardShortcuts::get_bindings() const {
    return m_bindings;
}

// ---------------------------------------------------------------------------
// Internal: rebuild fast lookup
// ---------------------------------------------------------------------------

void KeyboardShortcuts::rebuild_lookup() {
    m_simple_lookup.clear();
    for (const auto& binding : m_bindings) {
        // Only index bindings that have no modifier requirements
        if (!binding.shift && !binding.ctrl && !binding.alt) {
            m_simple_lookup[binding.key_code] = binding.action;
        }
    }
}

} // namespace ui
} // namespace sims3000
