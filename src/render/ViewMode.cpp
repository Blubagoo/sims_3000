/**
 * @file ViewMode.cpp
 * @brief Implementation of view mode state machine.
 */

#include "sims3000/render/ViewMode.h"

#include <algorithm>

namespace sims3000 {

// =============================================================================
// Construction
// =============================================================================

ViewModeController::ViewModeController(LayerVisibility& visibility) noexcept
    : m_visibility(visibility)
    , m_config{}
{
    // Apply default Surface mode
    applyModeStates(ViewMode::Surface, 0.0f);
}

ViewModeController::ViewModeController(
    LayerVisibility& visibility,
    const ViewModeConfig& config) noexcept
    : m_visibility(visibility)
    , m_config(config)
{
    // Clamp configuration values
    m_config.transitionDuration = std::max(0.0f, m_config.transitionDuration);
    m_config.undergroundGhostAlpha = std::clamp(m_config.undergroundGhostAlpha, 0.0f, 1.0f);
    m_config.cutawayUndergroundAlpha = std::clamp(m_config.cutawayUndergroundAlpha, 0.0f, 1.0f);

    // Apply default Surface mode
    applyModeStates(ViewMode::Surface, 0.0f);
}

// =============================================================================
// Mode Control
// =============================================================================

void ViewModeController::setMode(ViewMode mode) {
    if (!isValidViewMode(mode)) {
        return;
    }

    if (mode == m_currentMode && !m_transitioning) {
        return;  // Already in this mode
    }

    // Store previous mode for transition
    m_previousMode = m_currentMode;
    m_currentMode = mode;

    // Start transition if duration > 0
    if (m_config.transitionDuration > 0.0f) {
        m_transitioning = true;
        m_transitionProgress = 0.0f;
    } else {
        // Instant transition
        m_transitioning = false;
        m_transitionProgress = 1.0f;
        applyModeStates(m_currentMode, getTargetGhostAlpha());
    }
}

void ViewModeController::cycleMode() {
    switch (m_currentMode) {
        case ViewMode::Surface:
            setMode(ViewMode::Underground);
            break;
        case ViewMode::Underground:
            setMode(ViewMode::Cutaway);
            break;
        case ViewMode::Cutaway:
            setMode(ViewMode::Surface);
            break;
    }
}

void ViewModeController::cycleModeReverse() {
    switch (m_currentMode) {
        case ViewMode::Surface:
            setMode(ViewMode::Cutaway);
            break;
        case ViewMode::Underground:
            setMode(ViewMode::Surface);
            break;
        case ViewMode::Cutaway:
            setMode(ViewMode::Underground);
            break;
    }
}

void ViewModeController::resetToSurface() {
    setMode(ViewMode::Surface);
}

// =============================================================================
// Transition Management
// =============================================================================

void ViewModeController::update(float deltaTime) {
    if (!m_transitioning) {
        return;
    }

    // Advance progress
    if (m_config.transitionDuration > 0.0f) {
        m_transitionProgress += deltaTime / m_config.transitionDuration;
    } else {
        m_transitionProgress = 1.0f;
    }

    // Check if complete
    if (m_transitionProgress >= 1.0f) {
        m_transitionProgress = 1.0f;
        m_transitioning = false;
        applyModeStates(m_currentMode, getTargetGhostAlpha());
    } else {
        // Interpolate between modes
        float easedProgress = getEasedProgress();
        float sourceAlpha = getSourceGhostAlpha();
        float targetAlpha = getTargetGhostAlpha();
        float currentAlpha = sourceAlpha + (targetAlpha - sourceAlpha) * easedProgress;

        // Apply interpolated state
        applyModeStates(m_currentMode, currentAlpha);

        // For transitions that change layer visibility (not just alpha),
        // we apply the target mode's layer states immediately but with
        // interpolated alpha values
        m_visibility.setGhostAlpha(currentAlpha);
    }
}

float ViewModeController::getEasedProgress() const {
    return Easing::applyEasing(m_config.transitionEasing, m_transitionProgress);
}

void ViewModeController::completeTransition() {
    if (!m_transitioning) {
        return;
    }

    m_transitionProgress = 1.0f;
    m_transitioning = false;
    applyModeStates(m_currentMode, getTargetGhostAlpha());
}

void ViewModeController::cancelTransition() {
    if (!m_transitioning) {
        return;
    }

    m_currentMode = m_previousMode;
    m_transitioning = false;
    m_transitionProgress = 0.0f;
    applyModeStates(m_currentMode, getTargetGhostAlpha());
}

// =============================================================================
// Configuration
// =============================================================================

void ViewModeController::setConfig(const ViewModeConfig& config) {
    m_config = config;
    m_config.transitionDuration = std::max(0.0f, m_config.transitionDuration);
    m_config.undergroundGhostAlpha = std::clamp(m_config.undergroundGhostAlpha, 0.0f, 1.0f);
    m_config.cutawayUndergroundAlpha = std::clamp(m_config.cutawayUndergroundAlpha, 0.0f, 1.0f);
}

void ViewModeController::setTransitionDuration(float duration) {
    m_config.transitionDuration = std::max(0.0f, duration);
}

// =============================================================================
// Private Implementation
// =============================================================================

void ViewModeController::applyModeStates(ViewMode mode, float ghostAlpha) {
    // Set the ghost alpha value
    m_visibility.setGhostAlpha(ghostAlpha);

    switch (mode) {
        case ViewMode::Surface:
            // Surface mode: normal rendering
            // Underground hidden, surface layers visible
            m_visibility.setLayerVisibility(RenderLayer::Underground, LayerState::Hidden);
            m_visibility.setLayerVisibility(RenderLayer::Terrain, LayerState::Visible);
            m_visibility.setLayerVisibility(RenderLayer::Vegetation, LayerState::Visible);
            m_visibility.setLayerVisibility(RenderLayer::Water, LayerState::Visible);
            m_visibility.setLayerVisibility(RenderLayer::Roads, LayerState::Visible);
            m_visibility.setLayerVisibility(RenderLayer::Buildings, LayerState::Visible);
            m_visibility.setLayerVisibility(RenderLayer::Units, LayerState::Visible);
            m_visibility.setLayerVisibility(RenderLayer::Effects, LayerState::Visible);
            m_visibility.setLayerVisibility(RenderLayer::DataOverlay, LayerState::Visible);
            m_visibility.setLayerVisibility(RenderLayer::UIWorld, LayerState::Visible);
            break;

        case ViewMode::Underground:
            // Underground mode: surface ghosted, underground visible
            m_visibility.setLayerVisibility(RenderLayer::Underground, LayerState::Visible);
            m_visibility.setLayerVisibility(RenderLayer::Terrain, LayerState::Ghost);
            m_visibility.setLayerVisibility(RenderLayer::Vegetation, LayerState::Ghost);
            m_visibility.setLayerVisibility(RenderLayer::Water, LayerState::Visible);
            m_visibility.setLayerVisibility(RenderLayer::Roads, LayerState::Ghost);
            m_visibility.setLayerVisibility(RenderLayer::Buildings, LayerState::Ghost);
            m_visibility.setLayerVisibility(RenderLayer::Units, LayerState::Ghost);
            m_visibility.setLayerVisibility(RenderLayer::Effects, LayerState::Visible);
            m_visibility.setLayerVisibility(RenderLayer::DataOverlay, LayerState::Visible);
            m_visibility.setLayerVisibility(RenderLayer::UIWorld, LayerState::Visible);
            break;

        case ViewMode::Cutaway:
            // Cutaway mode: both surface and underground visible
            m_visibility.setLayerVisibility(RenderLayer::Underground, LayerState::Visible);
            m_visibility.setLayerVisibility(RenderLayer::Terrain, LayerState::Visible);
            m_visibility.setLayerVisibility(RenderLayer::Vegetation, LayerState::Visible);
            m_visibility.setLayerVisibility(RenderLayer::Water, LayerState::Visible);
            m_visibility.setLayerVisibility(RenderLayer::Roads, LayerState::Visible);
            m_visibility.setLayerVisibility(RenderLayer::Buildings, LayerState::Visible);
            m_visibility.setLayerVisibility(RenderLayer::Units, LayerState::Visible);
            m_visibility.setLayerVisibility(RenderLayer::Effects, LayerState::Visible);
            m_visibility.setLayerVisibility(RenderLayer::DataOverlay, LayerState::Visible);
            m_visibility.setLayerVisibility(RenderLayer::UIWorld, LayerState::Visible);
            break;
    }
}

float ViewModeController::getTargetGhostAlpha() const {
    switch (m_currentMode) {
        case ViewMode::Surface:
            return 1.0f;  // Full opacity (though Ghost state not used)
        case ViewMode::Underground:
            return m_config.undergroundGhostAlpha;
        case ViewMode::Cutaway:
            return m_config.cutawayUndergroundAlpha;
        default:
            return 1.0f;
    }
}

float ViewModeController::getSourceGhostAlpha() const {
    switch (m_previousMode) {
        case ViewMode::Surface:
            return 1.0f;
        case ViewMode::Underground:
            return m_config.undergroundGhostAlpha;
        case ViewMode::Cutaway:
            return m_config.cutawayUndergroundAlpha;
        default:
            return 1.0f;
    }
}

} // namespace sims3000
