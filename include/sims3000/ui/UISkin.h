/**
 * @file UISkin.h
 * @brief Skin data structure defining visual properties for all UI elements.
 *
 * A UISkin holds every color, opacity, and effect toggle needed to render the
 * UI in a particular visual style.  Two factory methods produce the built-in
 * skins:
 *
 * - **Legacy** -- opaque dark panels with teal/cyan accents, inspired by
 *   classic SimCity 2000 aesthetics.
 * - **Holo** -- translucent holographic panels with glow borders, scanlines,
 *   and subtle flicker, giving a futuristic feel.
 *
 * Usage:
 * @code
 *   UISkin skin = UISkin::create_legacy();   // classic opaque
 *   UISkin skin = UISkin::create_holo();     // holographic
 *
 *   // Skins are value types -- copy and tweak freely
 *   UISkin custom = UISkin::create_holo();
 *   custom.panel_opacity = 0.5f;
 * @endcode
 *
 * Resource ownership:
 * - UISkin is a plain data struct with no managed resources
 */

#ifndef SIMS3000_UI_UISKIN_H
#define SIMS3000_UI_UISKIN_H

#include <string>

#include "sims3000/ui/Widget.h" // for Color

namespace sims3000 {
namespace ui {

/**
 * @enum UIMode
 * @brief Visual rendering mode for the UI.
 */
enum class UIMode : uint8_t {
    Legacy = 0, ///< Classic opaque panels (SimCity 2000 style)
    Holo   = 1  ///< Holographic translucent panels
};

/**
 * @struct UISkin
 * @brief Complete visual skin definition for the UI system.
 *
 * Contains all colors, opacities, and effect toggles required by the
 * renderer to draw every standard UI element.  The two factory methods
 * (create_legacy, create_holo) produce fully initialised skins.
 */
struct UISkin {
    /// Unique identifier for this skin ("legacy", "holo", or custom).
    std::string skin_id;

    /// Rendering mode.
    UIMode mode = UIMode::Legacy;

    // -- Panel colors --------------------------------------------------------

    Color panel_background;       ///< Panel fill color
    Color panel_border;           ///< Panel border color
    Color panel_title_color;      ///< Panel title-bar text color
    float panel_opacity = 1.0f;   ///< Overall panel opacity (1.0 = opaque)
    float border_glow_intensity = 0.0f; ///< Border glow strength (0.0 = off)

    // -- Button colors -------------------------------------------------------

    Color button_normal;   ///< Button idle state
    Color button_hover;    ///< Button hovered state
    Color button_pressed;  ///< Button pressed / active state
    Color button_disabled; ///< Button disabled state

    // -- Text colors ---------------------------------------------------------

    Color text_primary;    ///< Main body text
    Color text_secondary;  ///< Subdued / secondary text
    Color text_accent;     ///< Highlighted / accent text

    // -- Effects -------------------------------------------------------------

    bool  use_scanlines       = false; ///< Draw CRT scanline overlay
    bool  use_hologram_flicker = false; ///< Apply subtle opacity flicker
    float scanline_opacity    = 0.05f; ///< Scanline overlay strength
    float flicker_intensity   = 0.02f; ///< Opacity variation (2 %)

    // -- Factory methods -----------------------------------------------------

    /**
     * Create the classic "Legacy" skin.
     *
     * Dark background (#0a0a12), teal/cyan accents, full opacity, no
     * holographic effects.
     *
     * @return A fully initialised legacy UISkin
     */
    static UISkin create_legacy();

    /**
     * Create the "Holo" (holographic) skin.
     *
     * Translucent dark background at 70 % opacity, cyan glow borders,
     * scanlines enabled, subtle flicker, bioluminescent accent palette.
     *
     * @return A fully initialised holographic UISkin
     */
    static UISkin create_holo();
};

} // namespace ui
} // namespace sims3000

#endif // SIMS3000_UI_UISKIN_H
