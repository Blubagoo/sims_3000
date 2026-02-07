/**
 * @file InstancedRenderer.h
 * @brief High-level instanced rendering system for terrain and buildings.
 *
 * Provides automatic batching and instancing for rendering many instances
 * of the same model with a single draw call. Critical for terrain tiles
 * and common buildings on large maps (up to 262k tiles on 512x512 maps).
 *
 * Features:
 * - Automatic batching by model type
 * - Per-instance transforms, tint colors, and emissive properties
 * - Chunked instancing for large instance counts
 * - Frustum culling at chunk level
 * - Draw call reduction (10x+ for repeated geometry)
 *
 * Usage:
 * @code
 *   InstancedRenderer renderer(device);
 *
 *   // Setup
 *   renderer.registerModel(terrainModelId, terrainAsset);
 *   renderer.registerModel(buildingModelId, buildingAsset);
 *
 *   // Each frame:
 *   renderer.beginFrame();
 *
 *   // Add terrain tiles (same model, different positions)
 *   for (const auto& tile : terrainTiles) {
 *       renderer.addInstance(terrainModelId, tile.transform, tile.tint, tile.emissive);
 *   }
 *
 *   // Add buildings (batched by building type)
 *   for (const auto& building : buildings) {
 *       renderer.addInstance(building.modelId, building.transform, building.tint, building.emissive);
 *   }
 *
 *   // Render all batches
 *   renderer.render(renderPass, cmdBuffer, pipeline, uboPool, state, stats);
 * @endcode
 *
 * Resource ownership:
 * - InstancedRenderer owns InstanceBuffer instances
 * - ModelAsset references are not owned (owned by caller/ModelLoader)
 * - GPUDevice must outlive InstancedRenderer
 */

#ifndef SIMS3000_RENDER_INSTANCED_RENDERER_H
#define SIMS3000_RENDER_INSTANCED_RENDERER_H

#include "sims3000/render/InstanceBuffer.h"
#include "sims3000/render/GPUMesh.h"
#include "sims3000/render/RenderCommands.h"
#include "sims3000/render/ToonPipeline.h"

#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <unordered_map>
#include <memory>
#include <cstdint>
#include <string>

namespace sims3000 {

// Forward declarations
class GPUDevice;
class UniformBufferPool;

/**
 * @struct ModelBatch
 * @brief A batch of instances for a single model.
 */
struct ModelBatch {
    std::uint64_t modelId = 0;              ///< Unique model identifier
    const ModelAsset* asset = nullptr;       ///< Pointer to model asset (not owned)
    std::unique_ptr<InstanceBuffer> buffer; ///< Instance buffer for this model
};

/**
 * @struct InstancedRendererStats
 * @brief Statistics about instanced rendering.
 */
struct InstancedRendererStats {
    std::uint32_t totalInstances = 0;      ///< Total instances rendered
    std::uint32_t totalDrawCalls = 0;       ///< Total draw calls issued
    std::uint32_t totalTriangles = 0;       ///< Total triangles rendered
    std::uint32_t batchCount = 0;           ///< Number of model batches
    std::uint32_t instancedDrawCalls = 0;   ///< Draw calls using instancing
    std::uint32_t nonInstancedDrawCalls = 0; ///< Draw calls without instancing (single instance)
    float drawCallReduction = 0.0f;         ///< Ratio of draw call reduction (1 - actual/naive)

    void reset() {
        totalInstances = 0;
        totalDrawCalls = 0;
        totalTriangles = 0;
        batchCount = 0;
        instancedDrawCalls = 0;
        nonInstancedDrawCalls = 0;
        drawCallReduction = 0.0f;
    }
};

/**
 * @struct InstancedRendererConfig
 * @brief Configuration for the instanced renderer.
 */
struct InstancedRendererConfig {
    /// Default capacity for new instance buffers
    std::uint32_t defaultBufferCapacity = 4096;

    /// Enable chunked instancing for large maps
    bool enableChunking = true;

    /// Instances per chunk for chunked instancing
    std::uint32_t chunkSize = InstanceBuffer::DEFAULT_CHUNK_SIZE;

    /// Terrain buffer capacity (262k for 512x512 maps)
    std::uint32_t terrainBufferCapacity = 262144;

    /// Building buffer capacity per building type
    std::uint32_t buildingBufferCapacity = 4096;

    /// Enable frustum culling at chunk level
    bool enableFrustumCulling = true;
};

/**
 * @class InstancedRenderer
 * @brief High-level instanced rendering system.
 *
 * Manages instance batching and rendering for multiple model types.
 * Automatically groups instances by model and issues instanced draw calls.
 */
class InstancedRenderer {
public:
    /// Model ID for terrain tiles (well-known ID)
    static constexpr std::uint64_t TERRAIN_MODEL_ID = 0;

    /**
     * Create an instanced renderer.
     * @param device GPU device for resource creation
     * @param config Configuration options
     */
    explicit InstancedRenderer(
        GPUDevice& device,
        const InstancedRendererConfig& config = {});

    ~InstancedRenderer();

    // Non-copyable
    InstancedRenderer(const InstancedRenderer&) = delete;
    InstancedRenderer& operator=(const InstancedRenderer&) = delete;

    // =========================================================================
    // Model Registration
    // =========================================================================

    /**
     * Register a model for instanced rendering.
     *
     * @param modelId Unique identifier for the model
     * @param asset Model asset (not owned, must outlive renderer)
     * @param capacity Instance buffer capacity (0 = use default)
     * @return true if registration succeeded
     */
    bool registerModel(
        std::uint64_t modelId,
        const ModelAsset* asset,
        std::uint32_t capacity = 0);

    /**
     * Register the terrain model.
     *
     * @param asset Terrain model asset
     * @return true if registration succeeded
     */
    bool registerTerrainModel(const ModelAsset* asset);

    /**
     * Unregister a model.
     *
     * @param modelId Model to unregister
     */
    void unregisterModel(std::uint64_t modelId);

    /**
     * Check if a model is registered.
     *
     * @param modelId Model to check
     * @return true if model is registered
     */
    bool isModelRegistered(std::uint64_t modelId) const;

    // =========================================================================
    // Instance Submission
    // =========================================================================

    /**
     * Begin a new frame for instance collection.
     */
    void beginFrame();

    /**
     * Add an instance for rendering.
     *
     * @param modelId Model type identifier
     * @param transform Model-to-world transform matrix
     * @param tintColor Per-instance tint color (RGBA)
     * @param emissiveColor Emissive color (RGB) + intensity (A)
     * @param ambientOverride Per-instance ambient (0 = use global)
     * @return true if instance was added, false if buffer is full
     */
    bool addInstance(
        std::uint64_t modelId,
        const glm::mat4& transform,
        const glm::vec4& tintColor = glm::vec4(1.0f),
        const glm::vec4& emissiveColor = glm::vec4(0.0f),
        float ambientOverride = 0.0f);

    /**
     * Add a terrain tile instance.
     *
     * @param transform Model-to-world transform
     * @param tintColor Per-instance tint color
     * @param emissiveColor Emissive color + intensity
     * @return true if instance was added
     */
    bool addTerrainInstance(
        const glm::mat4& transform,
        const glm::vec4& tintColor = glm::vec4(1.0f),
        const glm::vec4& emissiveColor = glm::vec4(0.0f));

    // =========================================================================
    // Rendering
    // =========================================================================

    /**
     * Upload all instance data to GPU.
     *
     * @param cmdBuffer Command buffer for upload commands
     * @return true if all uploads succeeded
     */
    bool uploadInstances(SDL_GPUCommandBuffer* cmdBuffer);

    /**
     * Render all registered model batches.
     *
     * @param renderPass Active render pass
     * @param cmdBuffer Command buffer (for any needed copies)
     * @param pipeline Toon rendering pipeline
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

    /**
     * Render a specific model batch.
     *
     * @param modelId Model to render
     * @param renderPass Active render pass
     * @param pipeline Toon rendering pipeline
     * @param state Render pass state
     * @param stats Statistics accumulator (optional)
     * @return Number of draw calls issued
     */
    std::uint32_t renderModel(
        std::uint64_t modelId,
        SDL_GPURenderPass* renderPass,
        const ToonPipeline& pipeline,
        RenderPassState& state,
        RenderCommandStats* stats = nullptr);

    // =========================================================================
    // Frustum Culling
    // =========================================================================

    /**
     * Update frustum planes for chunk-level culling.
     *
     * @param viewProjection View-projection matrix
     */
    void setViewProjection(const glm::mat4& viewProjection);

    /**
     * Extract frustum planes from view-projection matrix.
     *
     * @param viewProjection View-projection matrix
     * @param outPlanes Array of 6 frustum planes (left, right, bottom, top, near, far)
     */
    static void extractFrustumPlanes(
        const glm::mat4& viewProjection,
        glm::vec4 outPlanes[6]);

    // =========================================================================
    // Statistics
    // =========================================================================

    /**
     * Get rendering statistics for the last frame.
     * @return Rendering statistics
     */
    const InstancedRendererStats& getStats() const { return m_stats; }

    /**
     * Get the last error message.
     * @return Error string
     */
    const std::string& getLastError() const { return m_lastError; }

    /**
     * Get configuration.
     * @return Current configuration
     */
    const InstancedRendererConfig& getConfig() const { return m_config; }

private:
    /**
     * Get or create a batch for a model.
     *
     * @param modelId Model identifier
     * @return Batch pointer, or nullptr if model is not registered
     */
    ModelBatch* getBatch(std::uint64_t modelId);

    GPUDevice* m_device = nullptr;
    InstancedRendererConfig m_config;

    std::unordered_map<std::uint64_t, ModelBatch> m_batches;

    // Frustum planes for culling (extracted from view-projection)
    glm::vec4 m_frustumPlanes[6];
    bool m_frustumPlanesValid = false;

    // Statistics
    InstancedRendererStats m_stats;

    std::string m_lastError;
};

} // namespace sims3000

#endif // SIMS3000_RENDER_INSTANCED_RENDERER_H
