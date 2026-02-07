/**
 * @file VegetationRenderer.h
 * @brief GPU instanced rendering system for vegetation.
 *
 * Renders vegetation instances (trees, crystals, spore emitters) using
 * GPU instancing for high performance. Instances are generated per-chunk
 * by VegetationPlacementGenerator and batched by model type.
 *
 * Features:
 * - Loads 3 vegetation model types (BiolumeTree, CrystalSpire, SporeEmitter)
 * - GPU instancing with per-instance transform, tint, emissive
 * - Chunk-based instance buffer management
 * - LOD 0 only (vegetation hidden at higher LOD levels)
 * - Batched draw calls (one per model type)
 *
 * Resource ownership:
 * - VegetationRenderer owns ModelAsset instances for vegetation models
 * - VegetationRenderer owns InstanceBuffer for each model type
 * - GPUDevice and TextureLoader must outlive VegetationRenderer
 *
 * Usage:
 * @code
 *   VegetationRenderer renderer(device, textureLoader, modelLoader);
 *   renderer.initialize();
 *
 *   // Each frame:
 *   renderer.beginFrame();
 *
 *   // Update instance buffers for visible chunks
 *   for (const auto& chunk : visibleChunks) {
 *       ChunkInstances instances = placementGen.generateForChunk(chunk.x, chunk.y);
 *       renderer.updateChunkInstances(chunk.x, chunk.y, instances);
 *   }
 *
 *   renderer.uploadInstances(cmdBuffer);
 *   renderer.render(renderPass, pipeline, uboPool, state);
 * @endcode
 *
 * @see VegetationInstance.h for instance data structure
 * @see VegetationPlacementGenerator for instance generation
 */

#ifndef SIMS3000_RENDER_VEGETATION_RENDERER_H
#define SIMS3000_RENDER_VEGETATION_RENDERER_H

#include "sims3000/render/VegetationInstance.h"
#include "sims3000/render/InstanceBuffer.h"
#include "sims3000/render/GPUMesh.h"
#include "sims3000/render/RenderCommands.h"
#include "sims3000/render/ToonPipeline.h"
#include "sims3000/terrain/TerrainTypeInfo.h"

#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <array>
#include <memory>
#include <string>
#include <unordered_map>
#include <cstdint>

namespace sims3000 {

// Forward declarations
class GPUDevice;
class TextureLoader;
class ModelLoader;
class UniformBufferPool;

/**
 * @struct VegetationRendererConfig
 * @brief Configuration for the vegetation renderer.
 */
struct VegetationRendererConfig {
    /// Instance buffer capacity per model type
    /// Default: 65536 (enough for 16 chunks with ~4000 instances each)
    std::uint32_t instanceBufferCapacity = 65536;

    /// Path to vegetation models directory
    std::string modelsPath = "assets/models/vegetation/";

    /// Model filenames for each vegetation type
    std::string biolumeTreeModel = "biolume_tree.glb";
    std::string crystalSpireModel = "crystal_spire.glb";
    std::string sporeEmitterModel = "spore_emitter.glb";

    /// Use placeholder models if real models not found
    bool usePlaceholderModels = true;

    /// Maximum LOD level at which vegetation is rendered (0 = closest only)
    std::uint8_t maxLodLevel = 0;
};

/**
 * @struct VegetationRendererStats
 * @brief Statistics about vegetation rendering.
 */
struct VegetationRendererStats {
    std::uint32_t totalInstances = 0;      ///< Total vegetation instances rendered
    std::uint32_t drawCalls = 0;           ///< Total draw calls issued
    std::uint32_t triangles = 0;           ///< Total triangles rendered
    std::uint32_t instancesPerType[3] = {0, 0, 0};  ///< Instances per model type
    float renderTimeMs = 0.0f;             ///< Approximate render time

    void reset() {
        totalInstances = 0;
        drawCalls = 0;
        triangles = 0;
        instancesPerType[0] = 0;
        instancesPerType[1] = 0;
        instancesPerType[2] = 0;
        renderTimeMs = 0.0f;
    }
};

/**
 * @class VegetationRenderer
 * @brief GPU instanced renderer for vegetation.
 *
 * Manages loading of vegetation models and instanced rendering
 * using VegetationInstance data from VegetationPlacementGenerator.
 */
class VegetationRenderer {
public:
    /// Number of vegetation model types
    static constexpr std::size_t MODEL_TYPE_COUNT =
        static_cast<std::size_t>(render::VegetationModelType::Count);

    /**
     * Create a vegetation renderer.
     *
     * @param device GPU device for resource creation
     * @param textureLoader Texture loader for model textures
     * @param modelLoader Model loader for vegetation models
     * @param config Configuration options
     */
    VegetationRenderer(
        GPUDevice& device,
        TextureLoader& textureLoader,
        ModelLoader& modelLoader,
        const VegetationRendererConfig& config = {});

    ~VegetationRenderer();

    // Non-copyable
    VegetationRenderer(const VegetationRenderer&) = delete;
    VegetationRenderer& operator=(const VegetationRenderer&) = delete;

    /**
     * Initialize the renderer (load models, create buffers).
     *
     * @return true if initialization succeeded
     */
    bool initialize();

    /**
     * Check if renderer is valid and ready for use.
     * @return true if initialized successfully
     */
    bool isValid() const { return m_initialized; }

    // =========================================================================
    // Frame Lifecycle
    // =========================================================================

    /**
     * Begin a new frame.
     *
     * Clears all instance buffers for new data.
     */
    void beginFrame();

    /**
     * Add instances for a chunk.
     *
     * Converts VegetationInstance data to GPU instance data and adds
     * to the appropriate model type's instance buffer.
     *
     * @param instances Chunk instances from VegetationPlacementGenerator
     */
    void addChunkInstances(const render::ChunkInstances& instances);

    /**
     * Add a single vegetation instance.
     *
     * @param instance Vegetation instance data
     */
    void addInstance(const render::VegetationInstance& instance);

    /**
     * Upload all instance data to GPU.
     *
     * @param cmdBuffer Command buffer for upload commands
     * @return true if upload succeeded
     */
    bool uploadInstances(SDL_GPUCommandBuffer* cmdBuffer);

    /**
     * Render all vegetation instances.
     *
     * @param renderPass Active render pass
     * @param pipeline Toon rendering pipeline
     * @param uboPool Uniform buffer pool
     * @param state Render pass state for redundancy tracking
     * @param stats Statistics accumulator (optional)
     * @return Number of draw calls issued
     */
    std::uint32_t render(
        SDL_GPURenderPass* renderPass,
        const ToonPipeline& pipeline,
        UniformBufferPool& uboPool,
        RenderPassState& state,
        RenderCommandStats* stats = nullptr);

    // =========================================================================
    // LOD Control
    // =========================================================================

    /**
     * Set the current LOD level.
     *
     * Vegetation is only rendered at LOD 0 (closest zoom).
     * At higher LOD levels, vegetation is hidden for performance.
     *
     * @param lodLevel Current LOD level (0 = closest)
     */
    void setLodLevel(std::uint8_t lodLevel) { m_currentLodLevel = lodLevel; }

    /**
     * Get the current LOD level.
     * @return Current LOD level
     */
    std::uint8_t getLodLevel() const { return m_currentLodLevel; }

    /**
     * Check if vegetation should be rendered at current LOD.
     * @return true if vegetation is visible at current LOD
     */
    bool isVisibleAtCurrentLod() const {
        return m_currentLodLevel <= m_config.maxLodLevel;
    }

    // =========================================================================
    // Configuration and Statistics
    // =========================================================================

    /**
     * Get the configuration.
     * @return Current configuration
     */
    const VegetationRendererConfig& getConfig() const { return m_config; }

    /**
     * Get rendering statistics from last frame.
     * @return Render statistics
     */
    const VegetationRendererStats& getStats() const { return m_stats; }

    /**
     * Get the last error message.
     * @return Error string
     */
    const std::string& getLastError() const { return m_lastError; }

    /**
     * Check if a specific model type is loaded.
     * @param type Model type to check
     * @return true if model is loaded and valid
     */
    bool isModelLoaded(render::VegetationModelType type) const;

private:
    /**
     * Load vegetation models from disk or create placeholders.
     * @return true if at least placeholder models are available
     */
    bool loadModels();

    /**
     * Create a placeholder cube model for a vegetation type.
     * @param type Model type to create placeholder for
     * @return true if placeholder created successfully
     */
    bool createPlaceholderModel(render::VegetationModelType type);

    /**
     * Get emissive color for a vegetation model type.
     * @param type Model type
     * @return Emissive color (RGB) with intensity in alpha
     */
    glm::vec4 getEmissiveColor(render::VegetationModelType type) const;

    /**
     * Build transform matrix from VegetationInstance data.
     * @param instance Source instance
     * @return 4x4 model matrix
     */
    static glm::mat4 buildTransformMatrix(const render::VegetationInstance& instance);

    GPUDevice* m_device = nullptr;
    TextureLoader* m_textureLoader = nullptr;
    ModelLoader* m_modelLoader = nullptr;
    VegetationRendererConfig m_config;

    // Models for each vegetation type
    std::array<ModelAsset, MODEL_TYPE_COUNT> m_models;
    std::array<bool, MODEL_TYPE_COUNT> m_modelsLoaded = {false, false, false};

    // Instance buffers for each vegetation type
    std::array<std::unique_ptr<InstanceBuffer>, MODEL_TYPE_COUNT> m_instanceBuffers;

    // State
    bool m_initialized = false;
    std::uint8_t m_currentLodLevel = 0;

    // Statistics
    VegetationRendererStats m_stats;

    std::string m_lastError;
};

} // namespace sims3000

#endif // SIMS3000_RENDER_VEGETATION_RENDERER_H
