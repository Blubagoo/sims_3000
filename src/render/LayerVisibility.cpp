/**
 * @file LayerVisibility.cpp
 * @brief Implementation of layer visibility management.
 */

#include "sims3000/render/LayerVisibility.h"

#include <algorithm>

namespace sims3000 {

// =============================================================================
// Construction
// =============================================================================

LayerVisibility::LayerVisibility() noexcept
    : m_config{}
{
    // Initialize all layers to Visible except Underground
    // Underground is Hidden by default (only visible in underground view)
    m_states.fill(LayerState::Visible);
    m_states[static_cast<std::size_t>(RenderLayer::Underground)] = LayerState::Hidden;
}

LayerVisibility::LayerVisibility(const LayerVisibilityConfig& config) noexcept
    : m_config(config)
{
    // Initialize all layers to Visible except Underground
    m_states.fill(LayerState::Visible);
    m_states[static_cast<std::size_t>(RenderLayer::Underground)] = LayerState::Hidden;
}

// =============================================================================
// Core API
// =============================================================================

void LayerVisibility::setLayerVisibility(RenderLayer layer, LayerState state) {
    if (!isValidRenderLayer(layer)) {
        // Invalid layer - ignore silently in release, could log in debug
        return;
    }

    if (!isValidLayerState(state)) {
        // Invalid state - ignore
        return;
    }

    // If Ghost state requested for opaque layer and not allowed, use Visible
    if (state == LayerState::Ghost && !m_config.allowOpaqueGhost) {
        if (isOpaqueLayer(layer)) {
            state = LayerState::Visible;
        }
    }

    m_states[static_cast<std::size_t>(layer)] = state;
}

LayerState LayerVisibility::getState(RenderLayer layer) const {
    if (!isValidRenderLayer(layer)) {
        return LayerState::Visible;  // Safe default
    }
    return m_states[static_cast<std::size_t>(layer)];
}

bool LayerVisibility::shouldRender(RenderLayer layer) const {
    if (!isValidRenderLayer(layer)) {
        return true;  // Safe default - render unknown layers
    }
    return m_states[static_cast<std::size_t>(layer)] != LayerState::Hidden;
}

bool LayerVisibility::isGhost(RenderLayer layer) const {
    if (!isValidRenderLayer(layer)) {
        return false;
    }
    return m_states[static_cast<std::size_t>(layer)] == LayerState::Ghost;
}

bool LayerVisibility::isVisible(RenderLayer layer) const {
    if (!isValidRenderLayer(layer)) {
        return true;  // Safe default
    }
    return m_states[static_cast<std::size_t>(layer)] == LayerState::Visible;
}

bool LayerVisibility::isHidden(RenderLayer layer) const {
    if (!isValidRenderLayer(layer)) {
        return false;
    }
    return m_states[static_cast<std::size_t>(layer)] == LayerState::Hidden;
}

// =============================================================================
// Bulk Operations
// =============================================================================

void LayerVisibility::resetAll() {
    m_states.fill(LayerState::Visible);
    // Underground back to default Hidden state
    m_states[static_cast<std::size_t>(RenderLayer::Underground)] = LayerState::Hidden;
}

void LayerVisibility::setAllLayers(LayerState state) {
    if (!isValidLayerState(state)) {
        return;
    }

    if (state == LayerState::Ghost && !m_config.allowOpaqueGhost) {
        // Must check each layer individually
        for (std::size_t i = 0; i < RENDER_LAYER_COUNT; ++i) {
            RenderLayer layer = static_cast<RenderLayer>(i);
            if (isOpaqueLayer(layer)) {
                m_states[i] = LayerState::Visible;
            } else {
                m_states[i] = LayerState::Ghost;
            }
        }
    } else {
        m_states.fill(state);
    }
}

void LayerVisibility::setLayerRange(RenderLayer first, RenderLayer last, LayerState state) {
    if (!isValidRenderLayer(first) || !isValidRenderLayer(last)) {
        return;
    }

    if (!isValidLayerState(state)) {
        return;
    }

    // Ensure first <= last
    std::size_t startIdx = static_cast<std::size_t>(first);
    std::size_t endIdx = static_cast<std::size_t>(last);
    if (startIdx > endIdx) {
        std::swap(startIdx, endIdx);
    }

    for (std::size_t i = startIdx; i <= endIdx; ++i) {
        RenderLayer layer = static_cast<RenderLayer>(i);
        LayerState effectiveState = state;

        // Handle opaque ghost restriction
        if (state == LayerState::Ghost && !m_config.allowOpaqueGhost) {
            if (isOpaqueLayer(layer)) {
                effectiveState = LayerState::Visible;
            }
        }

        m_states[i] = effectiveState;
    }
}

// =============================================================================
// Configuration
// =============================================================================

void LayerVisibility::setGhostAlpha(float alpha) {
    m_config.ghostAlpha = std::clamp(alpha, 0.0f, 1.0f);
}

void LayerVisibility::setConfig(const LayerVisibilityConfig& config) {
    m_config = config;
    // Clamp ghost alpha
    m_config.ghostAlpha = std::clamp(m_config.ghostAlpha, 0.0f, 1.0f);

    // If allowOpaqueGhost changed to false, convert any opaque Ghost to Visible
    if (!m_config.allowOpaqueGhost) {
        for (std::size_t i = 0; i < RENDER_LAYER_COUNT; ++i) {
            if (m_states[i] == LayerState::Ghost) {
                RenderLayer layer = static_cast<RenderLayer>(i);
                if (isOpaqueLayer(layer)) {
                    m_states[i] = LayerState::Visible;
                }
            }
        }
    }
}

// =============================================================================
// Preset View Modes
// =============================================================================

void LayerVisibility::enableUndergroundView() {
    // Make underground visible
    setLayerVisibility(RenderLayer::Underground, LayerState::Visible);

    // Ghost surface layers
    setLayerVisibility(RenderLayer::Terrain, LayerState::Ghost);
    setLayerVisibility(RenderLayer::Vegetation, LayerState::Ghost);
    setLayerVisibility(RenderLayer::Roads, LayerState::Ghost);
    setLayerVisibility(RenderLayer::Buildings, LayerState::Ghost);
    setLayerVisibility(RenderLayer::Units, LayerState::Ghost);

    // Keep these unchanged from their current state (typically Visible):
    // - Water (can see through to underground)
    // - Effects (construction animations still visible)
    // - DataOverlay (data visualization still visible)
    // - UIWorld (selection boxes still visible)
}

void LayerVisibility::disableUndergroundView() {
    // Hide underground (default state)
    setLayerVisibility(RenderLayer::Underground, LayerState::Hidden);

    // Restore surface layers to visible
    setLayerVisibility(RenderLayer::Terrain, LayerState::Visible);
    setLayerVisibility(RenderLayer::Vegetation, LayerState::Visible);
    setLayerVisibility(RenderLayer::Roads, LayerState::Visible);
    setLayerVisibility(RenderLayer::Buildings, LayerState::Visible);
    setLayerVisibility(RenderLayer::Units, LayerState::Visible);
}

bool LayerVisibility::isUndergroundViewActive() const {
    // Underground view is active when:
    // 1. Underground layer is not Hidden (visible or ghost)
    // 2. At least one surface layer is Ghost

    LayerState undergroundState = getState(RenderLayer::Underground);
    if (undergroundState == LayerState::Hidden) {
        return false;
    }

    // Check if any surface layer is ghosted
    if (isGhost(RenderLayer::Terrain)) return true;
    if (isGhost(RenderLayer::Vegetation)) return true;
    if (isGhost(RenderLayer::Buildings)) return true;
    if (isGhost(RenderLayer::Roads)) return true;
    if (isGhost(RenderLayer::Units)) return true;

    return false;
}

// =============================================================================
// Statistics
// =============================================================================

void LayerVisibility::countStates(
    std::size_t& visibleCount,
    std::size_t& hiddenCount,
    std::size_t& ghostCount) const
{
    visibleCount = 0;
    hiddenCount = 0;
    ghostCount = 0;

    for (std::size_t i = 0; i < RENDER_LAYER_COUNT; ++i) {
        switch (m_states[i]) {
            case LayerState::Visible:
                ++visibleCount;
                break;
            case LayerState::Hidden:
                ++hiddenCount;
                break;
            case LayerState::Ghost:
                ++ghostCount;
                break;
        }
    }
}

} // namespace sims3000
