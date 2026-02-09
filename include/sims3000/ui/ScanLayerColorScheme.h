/**
 * @file ScanLayerColorScheme.h
 * @brief Color scheme mapping for scan layer overlays (Ticket E12-018).
 *
 * Maps each OverlayType to a color gradient function and provides legend
 * generation for the UI. Supports three scheme families:
 *
 * - **Gradient** (HeatMap, GreenRed, PurpleYellow): smooth linear
 *   interpolation between a low and high color.
 * - **Coverage**: binary/stepped display (below threshold = dark gray,
 *   above = full color).
 *
 * Usage:
 * @code
 *   ScanLayerColorScheme scheme;
 *
 *   // Map a normalized value [0.0, 1.0] to a display color:
 *   Color c = scheme.map_value(OverlayType::Disorder, 0.75f);
 *
 *   // Get the scheme type for an overlay:
 *   ColorSchemeType st = ScanLayerColorScheme::get_scheme_for_overlay(OverlayType::Traffic);
 *
 *   // Generate a legend for the UI:
 *   auto legend = scheme.get_legend(OverlayType::SectorValue);
 * @endcode
 *
 * Thread safety: all methods are const or static; safe to call from any thread.
 */

#ifndef SIMS3000_UI_SCANLAYERCOLORSCHEME_H
#define SIMS3000_UI_SCANLAYERCOLORSCHEME_H

#include "sims3000/ui/UIManager.h"
#include "sims3000/ui/Widget.h"

#include <string>
#include <vector>

namespace sims3000 {
namespace ui {

// ============================================================================
// Color Scheme Type
// ============================================================================

/**
 * @enum ColorSchemeType
 * @brief Classification of color scheme interpolation modes.
 */
enum class ColorSchemeType : uint8_t {
    HeatMap,        ///< Blue -> Red gradient (sector value, traffic)
    GreenRed,       ///< Green -> Red gradient (disorder)
    PurpleYellow,   ///< Purple -> Yellow gradient (contamination)
    Coverage        ///< Binary/stepped: dark gray -> full color
};

// ============================================================================
// Color Legend
// ============================================================================

/**
 * @struct ColorLegend
 * @brief Single entry in a color legend strip.
 *
 * A legend is a series of labeled color swatches shown beside the overlay
 * to help the player interpret values.
 */
struct ColorLegend {
    std::string label;  ///< Descriptive text (e.g. "Low", "High", "Covered")
    Color color;        ///< Display color for this legend entry
};

// ============================================================================
// Scan Layer Color Scheme
// ============================================================================

/**
 * @class ScanLayerColorScheme
 * @brief Maps OverlayType to color gradient functions and generates legends.
 *
 * All public methods are const or static, making this class lightweight and
 * safe to share across systems.
 */
class ScanLayerColorScheme {
public:
    ScanLayerColorScheme() = default;

    /**
     * Map a normalized overlay value to a display color.
     *
     * For gradient schemes (HeatMap, GreenRed, PurpleYellow) the value is
     * linearly interpolated between the low and high colors. For Coverage
     * schemes the result is binary: dark gray below the threshold, full
     * color at or above it.
     *
     * @param type             The overlay type determining the color scheme.
     * @param normalized_value Value in [0.0, 1.0]; clamped internally.
     * @return The mapped display color (alpha = 1.0).
     */
    Color map_value(OverlayType type, float normalized_value) const;

    /**
     * Get the color scheme type for a given overlay.
     * @param type The overlay type to query.
     * @return The corresponding ColorSchemeType.
     */
    static ColorSchemeType get_scheme_for_overlay(OverlayType type);

    /**
     * Generate a legend strip for the given overlay type.
     *
     * Gradient schemes produce entries for "Low", "Mid", and "High".
     * Coverage schemes produce "No Coverage" and "Covered".
     *
     * @param type The overlay type.
     * @return Ordered vector of ColorLegend entries (low-to-high).
     */
    std::vector<ColorLegend> get_legend(OverlayType type) const;

    // -- Coverage threshold --------------------------------------------------

    /// Threshold for binary coverage display (values >= this are "covered").
    static constexpr float COVERAGE_THRESHOLD = 0.5f;

private:
    /**
     * Get the low and high endpoint colors for an overlay type.
     * @param type   The overlay type.
     * @param[out] low  The color at normalized_value = 0.
     * @param[out] high The color at normalized_value = 1.
     */
    static void get_gradient_colors(OverlayType type, Color& low, Color& high);

    /**
     * Linearly interpolate between two colors.
     * @param a Color at t=0.
     * @param b Color at t=1.
     * @param t Interpolation parameter [0.0, 1.0].
     * @return Blended color.
     */
    static Color lerp_color(const Color& a, const Color& b, float t);
};

} // namespace ui
} // namespace sims3000

#endif // SIMS3000_UI_SCANLAYERCOLORSCHEME_H
