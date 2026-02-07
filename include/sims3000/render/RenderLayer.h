/**
 * @file RenderLayer.h
 * @brief Semantic render layer definitions for ordering rendering passes.
 *
 * RenderLayer defines the draw order for all renderable entities. Layers are
 * rendered in ascending enum order (lower values first, higher values on top).
 *
 * The rendering system uses these layers to:
 * 1. Sort entities into layer-specific render queues
 * 2. Ensure correct visual layering (e.g., roads under buildings, effects on top)
 * 3. Enable layer-specific rendering optimizations (e.g., batch by layer)
 * 4. Support layer visibility toggling for data visualization
 *
 * @note All renderable entities must be assigned a layer via RenderComponent.
 * @see RenderComponent in Components.h
 */

#ifndef SIMS3000_RENDER_RENDERLAYER_H
#define SIMS3000_RENDER_RENDERLAYER_H

#include <cstddef>
#include <cstdint>

namespace sims3000 {

/**
 * @enum RenderLayer
 * @brief Semantic render layers for ordering rendering passes.
 *
 * Layers are rendered in ascending enum order (lower values render first).
 * This ensures correct visual depth ordering without per-entity depth sorting
 * within each layer.
 *
 * Layer ordering rationale:
 * - Underground (0): Subsurface infrastructure always at bottom
 * - Terrain (1): Base terrain mesh forms the ground
 * - Water (2): Water surfaces sit on terrain with transparency
 * - Roads (3): Roads are on terrain surface, under buildings
 * - Buildings (4): Main structures visible above roads
 * - Units (5): Cosmetic beings and vehicles move on roads/terrain
 * - Effects (6): Particle effects, construction animations overlay scene
 * - DataOverlay (7): Heat maps, coverage zones overlay everything
 * - UIWorld (8): World-space UI (selection boxes) always on top
 */
enum class RenderLayer : std::uint8_t {
    /// Pipes, tunnels, subsurface infrastructure.
    /// Rendered first (at bottom) when underground view is enabled.
    Underground = 0,

    /// Base terrain mesh including elevation and terrain types.
    /// Forms the foundational ground layer of the scene.
    Terrain = 1,

    /// Water surfaces, rivers, lakes, and water effects.
    /// Rendered with transparency over terrain.
    Water = 2,

    /// Road network, pathways, and transportation infrastructure.
    /// Rendered on terrain surface, under buildings.
    Roads = 3,

    /// All building structures (residential, commercial, industrial, services).
    /// Main visual elements of the city.
    Buildings = 4,

    /// Cosmetic beings (citizens) and vehicles.
    /// Animated entities that move along roads and pathways.
    Units = 5,

    /// Particle effects, construction animations, visual feedback.
    /// Overlays the scene for dynamic visual effects.
    Effects = 6,

    /// Data visualization overlays (heat maps, power coverage, pollution).
    /// Semi-transparent overlays for data inspection modes.
    DataOverlay = 7,

    /// World-space UI elements (selection boxes, placement previews, indicators).
    /// Always rendered on top of all 3D scene elements.
    UIWorld = 8
};

/**
 * @brief Total number of render layers.
 *
 * Useful for creating layer-indexed arrays or iterating over all layers.
 * Example: std::array<RenderQueue, RENDER_LAYER_COUNT> layerQueues;
 */
constexpr std::size_t RENDER_LAYER_COUNT = 9;

/**
 * @brief Get the string name of a render layer (for debugging/logging).
 * @param layer The render layer to get the name of.
 * @return Null-terminated string name of the layer.
 */
constexpr const char* getRenderLayerName(RenderLayer layer) {
    switch (layer) {
        case RenderLayer::Underground: return "Underground";
        case RenderLayer::Terrain:     return "Terrain";
        case RenderLayer::Water:       return "Water";
        case RenderLayer::Roads:       return "Roads";
        case RenderLayer::Buildings:   return "Buildings";
        case RenderLayer::Units:       return "Units";
        case RenderLayer::Effects:     return "Effects";
        case RenderLayer::DataOverlay: return "DataOverlay";
        case RenderLayer::UIWorld:     return "UIWorld";
        default:                       return "Unknown";
    }
}

/**
 * @brief Check if a render layer value is valid.
 * @param layer The render layer to validate.
 * @return True if the layer is a valid RenderLayer enum value.
 */
constexpr bool isValidRenderLayer(RenderLayer layer) {
    return static_cast<std::uint8_t>(layer) < RENDER_LAYER_COUNT;
}

/**
 * @brief Check if a layer is opaque (no transparency blending required).
 * @param layer The render layer to check.
 * @return True if the layer typically contains opaque geometry.
 *
 * Opaque layers can use early-z optimization. Transparent layers
 * (Water, Effects, DataOverlay, UIWorld) require alpha blending.
 */
constexpr bool isOpaqueLayer(RenderLayer layer) {
    switch (layer) {
        case RenderLayer::Underground:
        case RenderLayer::Terrain:
        case RenderLayer::Roads:
        case RenderLayer::Buildings:
        case RenderLayer::Units:
            return true;
        case RenderLayer::Water:
        case RenderLayer::Effects:
        case RenderLayer::DataOverlay:
        case RenderLayer::UIWorld:
            return false;
        default:
            return true;
    }
}

/**
 * @brief Check if a layer should be affected by world lighting.
 * @param layer The render layer to check.
 * @return True if the layer uses world lighting, false for unlit/UI layers.
 *
 * Most 3D scene layers use toon shading with world lighting.
 * DataOverlay and UIWorld are typically rendered without lighting.
 */
constexpr bool isLitLayer(RenderLayer layer) {
    switch (layer) {
        case RenderLayer::DataOverlay:
        case RenderLayer::UIWorld:
            return false;
        default:
            return true;
    }
}

} // namespace sims3000

#endif // SIMS3000_RENDER_RENDERLAYER_H
