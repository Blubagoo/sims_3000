/**
 * @file SectorScanNavigator.cpp
 * @brief Implementation of SectorScanNavigator: smooth camera pan for minimap clicks.
 *
 * Interpolates the camera position from its current location to a target
 * world position over PAN_DURATION seconds using ease-out (quadratic)
 * for smooth deceleration.
 */

#include "sims3000/ui/SectorScanNavigator.h"

#include <algorithm>

namespace sims3000 {
namespace ui {

// =========================================================================
// Construction
// =========================================================================

SectorScanNavigator::SectorScanNavigator() = default;

// =========================================================================
// Navigation
// =========================================================================

void SectorScanNavigator::navigate_to(float world_x, float world_y) {
    // Record current position as the start of the pan
    m_start_x = m_current_x;
    m_start_y = m_current_y;

    m_target_x = world_x;
    m_target_y = world_y;

    m_elapsed = 0.0f;
    m_navigating = true;
}

// =========================================================================
// Update (per-frame interpolation)
// =========================================================================

void SectorScanNavigator::update(float delta_time) {
    if (!m_navigating) {
        return;
    }

    m_elapsed += delta_time;

    if (m_elapsed >= PAN_DURATION) {
        // Snap to target and stop navigating
        m_current_x = m_target_x;
        m_current_y = m_target_y;
        m_navigating = false;
        return;
    }

    // Normalized time [0, 1]
    float t = m_elapsed / PAN_DURATION;
    float eased = ease_out(t);

    // Linearly interpolate start -> target, weighted by eased time
    m_current_x = m_start_x + (m_target_x - m_start_x) * eased;
    m_current_y = m_start_y + (m_target_y - m_start_y) * eased;
}

// =========================================================================
// Camera position accessors
// =========================================================================

void SectorScanNavigator::get_camera_position(float& x, float& y) const {
    x = m_current_x;
    y = m_current_y;
}

void SectorScanNavigator::set_camera_position(float x, float y) {
    m_current_x = x;
    m_current_y = y;

    // Cancel any in-progress navigation
    m_navigating = false;
}

// =========================================================================
// State query
// =========================================================================

bool SectorScanNavigator::is_navigating() const {
    return m_navigating;
}

// =========================================================================
// Ease-out interpolation
// =========================================================================

float SectorScanNavigator::ease_out(float t) {
    // Quadratic ease-out: f(t) = 1 - (1 - t)^2
    // Starts fast, decelerates smoothly to a stop at t=1.
    float inv = 1.0f - t;
    return 1.0f - inv * inv;
}

} // namespace ui
} // namespace sims3000
