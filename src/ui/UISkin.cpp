/**
 * @file UISkin.cpp
 * @brief Factory method implementations for built-in UI skins.
 */

#include "sims3000/ui/UISkin.h"

namespace sims3000 {
namespace ui {

// =============================================================================
// Legacy skin -- classic opaque panels, teal/cyan accents
// =============================================================================

UISkin UISkin::create_legacy() {
    UISkin skin;
    skin.skin_id = "legacy";
    skin.mode    = UIMode::Legacy;

    // Panel: dark background (#0a0a12), teal border (#00aaaa)
    skin.panel_background   = Color{0.039f, 0.039f, 0.071f, 1.0f};
    skin.panel_border       = Color{0.0f, 0.667f, 0.667f, 1.0f};
    skin.panel_title_color  = Color{0.0f, 0.667f, 0.667f, 1.0f};
    skin.panel_opacity      = 1.0f;
    skin.border_glow_intensity = 0.0f;

    // Buttons
    skin.button_normal   = Color{0.15f, 0.15f, 0.18f, 1.0f};   // dark gray
    skin.button_hover    = Color{0.0f,  0.5f,  0.5f,  1.0f};    // teal highlight
    skin.button_pressed  = Color{0.0f,  0.8f,  0.8f,  1.0f};    // bright cyan
    skin.button_disabled = Color{0.1f,  0.1f,  0.1f,  0.5f};    // dim, half alpha

    // Text
    skin.text_primary   = Color{0.9f,  0.9f,  0.9f,  1.0f};    // near-white
    skin.text_secondary = Color{0.6f,  0.6f,  0.6f,  1.0f};    // medium gray
    skin.text_accent    = Color{0.0f,  0.667f, 0.667f, 1.0f};  // teal accent

    // Effects -- all off for legacy
    skin.use_scanlines        = false;
    skin.use_hologram_flicker = false;
    skin.scanline_opacity     = 0.0f;
    skin.flicker_intensity    = 0.0f;

    return skin;
}

// =============================================================================
// Holo skin -- holographic translucent panels, bioluminescent accents
// =============================================================================

UISkin UISkin::create_holo() {
    UISkin skin;
    skin.skin_id = "holo";
    skin.mode    = UIMode::Holo;

    // Panel: translucent dark background (#0a0a12 at 70 % alpha), cyan glow border
    skin.panel_background   = Color{0.039f, 0.039f, 0.071f, 0.7f};
    skin.panel_border       = Color{0.0f, 0.8f, 1.0f, 0.9f};       // bright cyan
    skin.panel_title_color  = Color{0.0f, 0.9f, 1.0f, 1.0f};       // vivid cyan
    skin.panel_opacity      = 0.7f;
    skin.border_glow_intensity = 1.0f;

    // Buttons -- translucent with cyan / teal tones
    skin.button_normal   = Color{0.05f, 0.15f, 0.2f,  0.6f};   // dark teal, translucent
    skin.button_hover    = Color{0.0f,  0.6f,  0.8f,  0.7f};   // bright cyan, semi
    skin.button_pressed  = Color{0.0f,  0.9f,  1.0f,  0.85f};  // vivid cyan
    skin.button_disabled = Color{0.05f, 0.08f, 0.1f,  0.3f};   // very dim, mostly transparent

    // Text -- bioluminescent palette
    skin.text_primary   = Color{0.7f,  0.95f, 1.0f,  1.0f};    // ice-blue
    skin.text_secondary = Color{0.4f,  0.7f,  0.8f,  0.8f};    // muted teal
    skin.text_accent    = Color{0.0f,  1.0f,  0.8f,  1.0f};    // neon green-cyan

    // Effects -- holographic
    skin.use_scanlines        = true;
    skin.use_hologram_flicker = true;
    skin.scanline_opacity     = 0.05f;  // subtle
    skin.flicker_intensity    = 0.02f;  // 2 % opacity variation

    return skin;
}

} // namespace ui
} // namespace sims3000
