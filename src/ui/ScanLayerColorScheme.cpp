/**
 * @file ScanLayerColorScheme.cpp
 * @brief Implementation of ScanLayerColorScheme - overlay color mapping and legends.
 */

#include "sims3000/ui/ScanLayerColorScheme.h"

#include <algorithm>

namespace sims3000 {
namespace ui {

// ============================================================================
// Gradient endpoint colors (per overlay type)
// ============================================================================

// Color constants used across schemes:
//   Blue:        (0.0, 0.0, 1.0)
//   Red:         (1.0, 0.0, 0.0)
//   Green:       (0.0, 1.0, 0.0)
//   Purple:      (0.5, 0.0, 0.8)
//   Yellow:      (1.0, 1.0, 0.0)
//   Dark Gray:   (0.2, 0.2, 0.2)
//   Cyan:        (0.0, 1.0, 1.0)

void ScanLayerColorScheme::get_gradient_colors(OverlayType type, Color& low, Color& high) {
    switch (type) {
        case OverlayType::SectorValue:
            // heat_map: Blue -> Red
            low  = { 0.0f, 0.0f, 1.0f, 1.0f };
            high = { 1.0f, 0.0f, 0.0f, 1.0f };
            break;

        case OverlayType::Disorder:
            // green_red: Green -> Red
            low  = { 0.0f, 1.0f, 0.0f, 1.0f };
            high = { 1.0f, 0.0f, 0.0f, 1.0f };
            break;

        case OverlayType::Contamination:
            // purple_yellow: Purple -> Yellow
            low  = { 0.5f, 0.0f, 0.8f, 1.0f };
            high = { 1.0f, 1.0f, 0.0f, 1.0f };
            break;

        case OverlayType::EnergyCoverage:
            // coverage: Dark Gray -> Cyan
            low  = { 0.2f, 0.2f, 0.2f, 1.0f };
            high = { 0.0f, 1.0f, 1.0f, 1.0f };
            break;

        case OverlayType::FluidCoverage:
            // coverage: Dark Gray -> Blue
            low  = { 0.2f, 0.2f, 0.2f, 1.0f };
            high = { 0.0f, 0.0f, 1.0f, 1.0f };
            break;

        case OverlayType::Traffic:
            // heat_map: Green -> Red
            low  = { 0.0f, 1.0f, 0.0f, 1.0f };
            high = { 1.0f, 0.0f, 0.0f, 1.0f };
            break;

        case OverlayType::ServiceCoverage:
            // coverage: Dark Gray -> Green
            low  = { 0.2f, 0.2f, 0.2f, 1.0f };
            high = { 0.0f, 1.0f, 0.0f, 1.0f };
            break;

        case OverlayType::None:
        default:
            // Fallback: white -> white (should not be used in practice)
            low  = { 1.0f, 1.0f, 1.0f, 1.0f };
            high = { 1.0f, 1.0f, 1.0f, 1.0f };
            break;
    }
}

// ============================================================================
// Color lerp
// ============================================================================

Color ScanLayerColorScheme::lerp_color(const Color& a, const Color& b, float t) {
    return {
        a.r + (b.r - a.r) * t,
        a.g + (b.g - a.g) * t,
        a.b + (b.b - a.b) * t,
        a.a + (b.a - a.a) * t
    };
}

// ============================================================================
// Scheme type lookup
// ============================================================================

ColorSchemeType ScanLayerColorScheme::get_scheme_for_overlay(OverlayType type) {
    switch (type) {
        case OverlayType::SectorValue:      return ColorSchemeType::HeatMap;
        case OverlayType::Disorder:          return ColorSchemeType::GreenRed;
        case OverlayType::Contamination:     return ColorSchemeType::PurpleYellow;
        case OverlayType::EnergyCoverage:    return ColorSchemeType::Coverage;
        case OverlayType::FluidCoverage:     return ColorSchemeType::Coverage;
        case OverlayType::ServiceCoverage:   return ColorSchemeType::Coverage;
        case OverlayType::Traffic:           return ColorSchemeType::HeatMap;
        case OverlayType::None:
        default:
            return ColorSchemeType::HeatMap; // sensible default
    }
}

// ============================================================================
// Value mapping
// ============================================================================

Color ScanLayerColorScheme::map_value(OverlayType type, float normalized_value) const {
    // Clamp input
    float t = std::max(0.0f, std::min(1.0f, normalized_value));

    Color low, high;
    get_gradient_colors(type, low, high);

    ColorSchemeType scheme = get_scheme_for_overlay(type);

    switch (scheme) {
        case ColorSchemeType::HeatMap:
        case ColorSchemeType::GreenRed:
        case ColorSchemeType::PurpleYellow:
            // Smooth gradient interpolation
            return lerp_color(low, high, t);

        case ColorSchemeType::Coverage:
            // Binary/stepped: below threshold = dark gray, at/above = full color
            if (t < COVERAGE_THRESHOLD) {
                return low;
            }
            return high;

        default:
            return lerp_color(low, high, t);
    }
}

// ============================================================================
// Legend generation
// ============================================================================

std::vector<ColorLegend> ScanLayerColorScheme::get_legend(OverlayType type) const {
    std::vector<ColorLegend> legend;

    if (type == OverlayType::None) {
        return legend;
    }

    Color low, high;
    get_gradient_colors(type, low, high);

    ColorSchemeType scheme = get_scheme_for_overlay(type);

    switch (scheme) {
        case ColorSchemeType::HeatMap:
        case ColorSchemeType::GreenRed:
        case ColorSchemeType::PurpleYellow: {
            // Gradient: Low -> Mid -> High
            Color mid = lerp_color(low, high, 0.5f);
            legend.push_back({ "Low",  low  });
            legend.push_back({ "Mid",  mid  });
            legend.push_back({ "High", high });
            break;
        }

        case ColorSchemeType::Coverage:
            // Binary: No Coverage / Covered
            legend.push_back({ "No Coverage", low  });
            legend.push_back({ "Covered",     high });
            break;

        default:
            break;
    }

    return legend;
}

} // namespace ui
} // namespace sims3000
