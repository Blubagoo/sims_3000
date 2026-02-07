/**
 * @file TransparentRenderQueue.h
 * @brief Sorted render queue for transparent objects with back-to-front ordering.
 *
 * Manages transparent objects that require back-to-front sorting for correct
 * alpha blending. Transparent objects include:
 * - Construction preview ghosts
 * - Selection overlays
 * - Underground view (ghosted surface)
 * - Water surfaces
 * - Effects and particles
 *
 * Sorting is done by camera distance (far objects rendered first) to ensure
 * correct blending results. The depth buffer is read-only during transparent
 * pass (depth test enabled, depth write disabled).
 *
 * Usage:
 * @code
 *   TransparentRenderQueue queue;
 *
 *   // Each frame:
 *   queue.begin(cameraPosition);
 *
 *   // Add transparent objects (order doesn't matter - will be sorted)
 *   queue.addObject(meshHandle, transform, material, alpha);
 *   queue.addGhost(meshHandle, transform, ghostAlpha);
 *   queue.addSelection(meshHandle, transform, selectionColor);
 *
 *   // Sort and render all transparents back-to-front
 *   queue.sortBackToFront();
 *   queue.render(renderPass, pipeline, state);
 * @endcode
 *
 * Resource ownership:
 * - TransparentRenderQueue does not own meshes or materials (referenced)
 * - Sorting happens in-place on the internal vector
 * - Queue is cleared at the start of each frame
 */

#ifndef SIMS3000_RENDER_TRANSPARENT_RENDER_QUEUE_H
#define SIMS3000_RENDER_TRANSPARENT_RENDER_QUEUE_H

#include "sims3000/render/RenderCommands.h"
#include "sims3000/render/RenderLayer.h"
#include "sims3000/render/GPUMesh.h"

#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <vector>
#include <cstdint>
#include <string>

namespace sims3000 {

// Forward declarations
class GPUDevice;
class ToonPipeline;
class UniformBufferPool;

/**
 * @enum TransparentObjectType
 * @brief Type of transparent object for specialized rendering.
 */
enum class TransparentObjectType : std::uint8_t {
    /// Generic transparent object with alpha blending
    Generic = 0,

    /// Construction preview ghost (semi-transparent preview)
    ConstructionGhost,

    /// Selection overlay (highlighted object)
    SelectionOverlay,

    /// Underground view ghosted surface
    UndergroundGhost,

    /// Water surface
    Water,

    /// Particle/effect
    Effect
};

/**
 * @struct TransparentObject
 * @brief A single transparent object queued for rendering.
 */
struct TransparentObject {
    /// Mesh to render
    const GPUMesh* mesh = nullptr;

    /// Material for texture binding
    const GPUMaterial* material = nullptr;

    /// Model-to-world transform
    glm::mat4 transform{1.0f};

    /// Base color with alpha (alpha controls transparency)
    glm::vec4 color{1.0f, 1.0f, 1.0f, 0.5f};

    /// Emissive color (RGB) + intensity (A)
    glm::vec4 emissive{0.0f};

    /// Object type for specialized rendering
    TransparentObjectType type = TransparentObjectType::Generic;

    /// Render layer (for layer-based sorting within same distance)
    RenderLayer layer = RenderLayer::Effects;

    /// Squared distance from camera (for sorting - computed during sort)
    float distanceSquared = 0.0f;

    /// Whether this object is valid
    bool isValid() const {
        return mesh != nullptr && mesh->isValid();
    }
};

/**
 * @struct TransparentRenderQueueStats
 * @brief Statistics about transparent object rendering.
 */
struct TransparentRenderQueueStats {
    std::uint32_t objectCount = 0;           ///< Total objects in queue
    std::uint32_t drawCalls = 0;             ///< Draw calls issued
    std::uint32_t trianglesDrawn = 0;        ///< Total triangles
    std::uint32_t ghostCount = 0;            ///< Construction ghosts
    std::uint32_t selectionCount = 0;        ///< Selection overlays
    std::uint32_t effectCount = 0;           ///< Effects/particles
    float sortTimeMs = 0.0f;                 ///< Time spent sorting

    void reset() {
        objectCount = 0;
        drawCalls = 0;
        trianglesDrawn = 0;
        ghostCount = 0;
        selectionCount = 0;
        effectCount = 0;
        sortTimeMs = 0.0f;
    }
};

/**
 * @struct TransparentRenderQueueConfig
 * @brief Configuration for the transparent render queue.
 */
struct TransparentRenderQueueConfig {
    /// Initial capacity for object vector (avoids reallocation)
    std::uint32_t initialCapacity = 256;

    /// Default alpha for construction ghosts
    float ghostAlpha = 0.4f;

    /// Default alpha for selection overlays
    float selectionAlpha = 0.3f;

    /// Selection highlight color (added to base color)
    glm::vec4 selectionTint{0.2f, 0.4f, 0.8f, 0.3f};

    /// Ghost tint color
    glm::vec4 ghostTint{0.5f, 0.5f, 1.0f, 0.4f};
};

/**
 * @class TransparentRenderQueue
 * @brief Sorted queue for transparent object rendering.
 *
 * Collects transparent objects during frame update, sorts them back-to-front
 * by camera distance, and renders them with the transparent pipeline.
 */
class TransparentRenderQueue {
public:
    /**
     * Create a transparent render queue.
     * @param config Configuration options
     */
    explicit TransparentRenderQueue(const TransparentRenderQueueConfig& config = {});

    ~TransparentRenderQueue() = default;

    // Non-copyable but movable
    TransparentRenderQueue(const TransparentRenderQueue&) = delete;
    TransparentRenderQueue& operator=(const TransparentRenderQueue&) = delete;
    TransparentRenderQueue(TransparentRenderQueue&&) = default;
    TransparentRenderQueue& operator=(TransparentRenderQueue&&) = default;

    // =========================================================================
    // Frame Lifecycle
    // =========================================================================

    /**
     * Begin a new frame, clearing the queue.
     * @param cameraPosition Camera position for distance sorting
     */
    void begin(const glm::vec3& cameraPosition);

    /**
     * Sort all queued objects back-to-front by camera distance.
     * Must be called before render().
     */
    void sortBackToFront();

    /**
     * Render all queued transparent objects.
     *
     * Prerequisites:
     * - Opaque pass must be complete (depth buffer populated)
     * - Transparent pipeline must be bound
     * - sortBackToFront() must be called first
     *
     * @param renderPass Active GPU render pass
     * @param cmdBuffer Command buffer for uniform uploads
     * @param pipeline Toon pipeline (transparent variant will be used)
     * @param uboPool Uniform buffer pool
     * @param state Render pass state for redundancy tracking
     * @param stats Statistics accumulator (optional)
     * @return Number of draw calls issued
     */
    std::uint32_t render(
        SDL_GPURenderPass* renderPass,
        SDL_GPUCommandBuffer* cmdBuffer,
        const ToonPipeline& pipeline,
        UniformBufferPool& uboPool,
        RenderPassState& state,
        RenderCommandStats* stats = nullptr);

    // =========================================================================
    // Object Submission
    // =========================================================================

    /**
     * Add a generic transparent object.
     *
     * @param mesh Mesh to render
     * @param material Material for textures (can be nullptr)
     * @param transform Model-to-world transform
     * @param color Base color with alpha transparency
     * @param emissive Emissive color (RGB) + intensity (A)
     * @param layer Render layer for layer-based ordering
     */
    void addObject(
        const GPUMesh* mesh,
        const GPUMaterial* material,
        const glm::mat4& transform,
        const glm::vec4& color,
        const glm::vec4& emissive = glm::vec4(0.0f),
        RenderLayer layer = RenderLayer::Effects);

    /**
     * Add a construction preview ghost.
     *
     * @param mesh Mesh to render
     * @param material Material (optional)
     * @param transform World transform for placement preview
     * @param alpha Override ghost alpha (0 = use config default)
     */
    void addConstructionGhost(
        const GPUMesh* mesh,
        const GPUMaterial* material,
        const glm::mat4& transform,
        float alpha = 0.0f);

    /**
     * Add a selection overlay for a selected object.
     *
     * @param mesh Mesh of selected object
     * @param material Material (optional)
     * @param transform World transform of selected object
     * @param selectionColor Override selection tint (alpha 0 = use config default)
     */
    void addSelectionOverlay(
        const GPUMesh* mesh,
        const GPUMaterial* material,
        const glm::mat4& transform,
        const glm::vec4& selectionColor = glm::vec4(0.0f));

    /**
     * Add an underground view ghost (surface seen from below).
     *
     * @param mesh Surface mesh
     * @param material Material
     * @param transform World transform
     * @param alpha Ghost alpha (0 = use config default)
     */
    void addUndergroundGhost(
        const GPUMesh* mesh,
        const GPUMaterial* material,
        const glm::mat4& transform,
        float alpha = 0.0f);

    // =========================================================================
    // Queries
    // =========================================================================

    /**
     * Get number of objects currently in queue.
     * @return Object count
     */
    std::size_t getObjectCount() const { return m_objects.size(); }

    /**
     * Check if queue is empty.
     * @return true if no transparent objects queued
     */
    bool isEmpty() const { return m_objects.empty(); }

    /**
     * Check if queue has been sorted (ready for rendering).
     * @return true if sortBackToFront() was called since last begin()
     */
    bool isSorted() const { return m_sorted; }

    /**
     * Get statistics from last render.
     * @return Queue statistics
     */
    const TransparentRenderQueueStats& getStats() const { return m_stats; }

    /**
     * Get configuration.
     * @return Current configuration
     */
    const TransparentRenderQueueConfig& getConfig() const { return m_config; }

    /**
     * Set configuration.
     * @param config New configuration
     */
    void setConfig(const TransparentRenderQueueConfig& config);

    /**
     * Get the last error message.
     * @return Error string
     */
    const std::string& getLastError() const { return m_lastError; }

private:
    /**
     * Calculate distance squared from camera to object center.
     * @param transform Object transform (extracts position from translation column)
     * @return Squared distance
     */
    float calculateDistanceSquared(const glm::mat4& transform) const;

    /**
     * Extract position from transform matrix.
     * @param transform Model matrix
     * @return World position
     */
    static glm::vec3 extractPosition(const glm::mat4& transform);

    TransparentRenderQueueConfig m_config;
    std::vector<TransparentObject> m_objects;

    glm::vec3 m_cameraPosition{0.0f};
    bool m_sorted = false;

    TransparentRenderQueueStats m_stats;
    std::string m_lastError;
};

} // namespace sims3000

#endif // SIMS3000_RENDER_TRANSPARENT_RENDER_QUEUE_H
