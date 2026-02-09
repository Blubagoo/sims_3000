/**
 * @file ScanLayerManager.h
 * @brief Scan layer (overlay) activation, data source binding, and toggle system.
 *
 * ScanLayerManager owns the mapping between OverlayType and IGridOverlay data
 * sources, handles cycle-through toggling, and drives a short fade transition
 * when switching between layers.
 *
 * Usage:
 * @code
 *   ScanLayerManager scans;
 *   scans.register_overlay(OverlayType::Disorder, &disorder_overlay);
 *   scans.set_active(OverlayType::Disorder);
 *
 *   // Per-frame:
 *   scans.update(delta_time);
 *   float fade = scans.get_fade_progress();
 * @endcode
 *
 * Thread safety: not thread-safe. Call from the main/render thread only.
 */

#ifndef SIMS3000_UI_SCANLAYERMANAGER_H
#define SIMS3000_UI_SCANLAYERMANAGER_H

#include "sims3000/ui/UIManager.h"

#include <cstdint>
#include <array>
#include <functional>
#include <string>

namespace sims3000 {
namespace services { class IGridOverlay; }  // forward declare
}

namespace sims3000 {
namespace ui {

/// Manages scan layer (overlay) activation and data source binding.
class ScanLayerManager {
public:
    ScanLayerManager();

    /// Register an overlay data source for a given type (non-owning).
    void register_overlay(OverlayType type, services::IGridOverlay* overlay);

    /// Unregister an overlay.
    void unregister_overlay(OverlayType type);

    /// Get the active overlay data source (nullptr if None active).
    services::IGridOverlay* get_active_overlay() const;

    /// Get the overlay for a specific type (nullptr if not registered).
    services::IGridOverlay* get_overlay(OverlayType type) const;

    /// Set the active overlay type.
    void set_active(OverlayType type);

    /// Get the active overlay type.
    OverlayType get_active_type() const;

    /// Cycle to the next overlay type: None -> Disorder -> ... -> Traffic -> None.
    void cycle_next();

    /// Cycle to the previous overlay type.
    void cycle_previous();

    /// Get display name for an overlay type (alien terminology).
    static const std::string& get_display_name(OverlayType type);

    /// Callback when overlay changes.
    using OverlayChangeCallback = std::function<void(OverlayType old_type, OverlayType new_type)>;
    void set_on_change(OverlayChangeCallback callback);

    /// Fade transition support.
    float get_fade_progress() const;  // 0.0 = fading out, 1.0 = fully visible
    void update(float delta_time);

    static constexpr float FADE_DURATION = 0.25f; // 250ms transition

    /// Total number of overlay types (excluding None).
    static constexpr int OVERLAY_COUNT = 7;

private:
    OverlayType m_active_type = OverlayType::None;
    std::array<services::IGridOverlay*, 8> m_overlays{}; // index 0=None (unused), 1-7 = overlay types
    OverlayChangeCallback m_on_change;

    float m_fade_progress = 1.0f;
    OverlayType m_fade_target = OverlayType::None;
    bool m_fading = false;
};

} // namespace ui
} // namespace sims3000

#endif // SIMS3000_UI_SCANLAYERMANAGER_H
