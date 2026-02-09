/**
 * @file ScanLayerManager.cpp
 * @brief Implementation of ScanLayerManager - overlay toggling and fade transitions.
 */

#include "sims3000/ui/ScanLayerManager.h"
#include "sims3000/services/IGridOverlay.h"

#include <algorithm>
#include <unordered_map>

namespace sims3000 {
namespace ui {

// ============================================================================
// Construction
// ============================================================================

ScanLayerManager::ScanLayerManager() {
    m_overlays.fill(nullptr);
}

// ============================================================================
// Overlay registration
// ============================================================================

void ScanLayerManager::register_overlay(OverlayType type, services::IGridOverlay* overlay) {
    m_overlays[static_cast<uint8_t>(type)] = overlay;
}

void ScanLayerManager::unregister_overlay(OverlayType type) {
    m_overlays[static_cast<uint8_t>(type)] = nullptr;
}

// ============================================================================
// Active overlay access
// ============================================================================

services::IGridOverlay* ScanLayerManager::get_active_overlay() const {
    if (m_active_type == OverlayType::None) {
        return nullptr;
    }
    return m_overlays[static_cast<uint8_t>(m_active_type)];
}

services::IGridOverlay* ScanLayerManager::get_overlay(OverlayType type) const {
    return m_overlays[static_cast<uint8_t>(type)];
}

// ============================================================================
// Active type management
// ============================================================================

void ScanLayerManager::set_active(OverlayType type) {
    if (type == m_active_type) {
        return;
    }

    OverlayType old_type = m_active_type;

    // Start fade transition
    m_fade_target = type;
    m_fading = true;
    m_fade_progress = 0.0f;

    if (m_on_change) {
        m_on_change(old_type, type);
    }
}

OverlayType ScanLayerManager::get_active_type() const {
    return m_active_type;
}

// ============================================================================
// Cycling
// ============================================================================

void ScanLayerManager::cycle_next() {
    int current = static_cast<int>(m_active_type);
    // None(0) -> Disorder(1) -> ... -> Traffic(7) -> None(0)
    int next = (current + 1) % (OVERLAY_COUNT + 1);
    set_active(static_cast<OverlayType>(next));
}

void ScanLayerManager::cycle_previous() {
    int current = static_cast<int>(m_active_type);
    // None(0) -> Traffic(7) -> ... -> Disorder(1) -> None(0)
    int prev = (current - 1 + OVERLAY_COUNT + 1) % (OVERLAY_COUNT + 1);
    set_active(static_cast<OverlayType>(prev));
}

// ============================================================================
// Display names
// ============================================================================

const std::string& ScanLayerManager::get_display_name(OverlayType type) {
    // Static map ensures stable references - no dangling pointers
    static const std::unordered_map<uint8_t, std::string> names = {
        { static_cast<uint8_t>(OverlayType::None),             "None" },
        { static_cast<uint8_t>(OverlayType::Disorder),         "Disorder Index" },
        { static_cast<uint8_t>(OverlayType::Contamination),    "Contamination" },
        { static_cast<uint8_t>(OverlayType::SectorValue),      "Sector Value" },
        { static_cast<uint8_t>(OverlayType::EnergyCoverage),   "Energy Coverage" },
        { static_cast<uint8_t>(OverlayType::FluidCoverage),    "Fluid Coverage" },
        { static_cast<uint8_t>(OverlayType::ServiceCoverage),  "Service Coverage" },
        { static_cast<uint8_t>(OverlayType::Traffic),          "Flow Analysis" }
    };

    static const std::string unknown = "Unknown";

    auto it = names.find(static_cast<uint8_t>(type));
    if (it != names.end()) {
        return it->second;
    }
    return unknown;
}

// ============================================================================
// Callback
// ============================================================================

void ScanLayerManager::set_on_change(OverlayChangeCallback callback) {
    m_on_change = std::move(callback);
}

// ============================================================================
// Fade transition
// ============================================================================

float ScanLayerManager::get_fade_progress() const {
    return m_fade_progress;
}

void ScanLayerManager::update(float delta_time) {
    if (!m_fading) {
        return;
    }

    m_fade_progress += delta_time / FADE_DURATION;
    m_fade_progress = std::min(m_fade_progress, 1.0f);

    if (m_fade_progress >= 1.0f) {
        m_fading = false;
        m_active_type = m_fade_target;
    }
}

} // namespace ui
} // namespace sims3000
