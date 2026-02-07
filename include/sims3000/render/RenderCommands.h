/**
 * @file RenderCommands.h
 * @brief Render command recording for drawing 3D models with toon shading.
 *
 * Provides functions to record GPU draw commands for ModelAsset and GPUMesh
 * objects within an active render pass. Handles vertex/index buffer binding,
 * uniform buffer uploads for model matrices, and texture binding.
 *
 * Resource ownership:
 * - RenderCommands does not own any GPU resources
 * - UniformBufferPool allocations are valid until pool reset
 * - Caller must ensure pipeline is bound before calling draw functions
 *
 * Usage:
 * @code
 *   // In render loop
 *   SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(...);
 *   SDL_BindGPUGraphicsPipeline(renderPass, pipeline.getOpaquePipeline());
 *
 *   // Upload view-projection matrix once per frame
 *   RenderCommands::bindViewProjection(cmdBuffer, uboPool, viewProjUBO);
 *
 *   // Draw each model
 *   glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), position);
 *   RenderCommands::drawModelAsset(renderPass, cmdBuffer, uboPool,
 *                                   modelAsset, modelMatrix);
 *
 *   SDL_EndGPURenderPass(renderPass);
 * @endcode
 */

#ifndef SIMS3000_RENDER_RENDER_COMMANDS_H
#define SIMS3000_RENDER_RENDER_COMMANDS_H

#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <cstdint>
#include <string>

namespace sims3000 {

// Forward declarations
struct GPUMesh;
struct GPUMaterial;
struct ModelAsset;
struct Texture;
struct ToonViewProjectionUBO;
struct ToonLightingUBO;
struct ToonInstanceData;
class UniformBufferPool;

/**
 * @struct DrawMeshParams
 * @brief Parameters for drawing a single mesh.
 *
 * Encapsulates all data needed to issue a draw call for one GPUMesh.
 */
struct DrawMeshParams {
    const GPUMesh* mesh = nullptr;           ///< Mesh to draw
    const GPUMaterial* material = nullptr;   ///< Material for texture binding
    glm::mat4 modelMatrix{1.0f};             ///< Model-to-world transform
    glm::vec4 baseColor{1.0f};               ///< Base color multiplier
    glm::vec4 emissiveColor{0.0f};           ///< Emissive color (RGB) + intensity (A)
    float ambientOverride = 0.0f;            ///< Per-instance ambient (0 = use global)
    std::uint32_t instanceId = 0;            ///< Instance index for storage buffer

    bool isValid() const {
        return mesh != nullptr;
    }
};

/**
 * @struct DrawModelParams
 * @brief Parameters for drawing a complete model with multiple meshes.
 *
 * Encapsulates data for drawing all meshes in a ModelAsset.
 */
struct DrawModelParams {
    const ModelAsset* asset = nullptr;       ///< Model asset to draw
    glm::mat4 modelMatrix{1.0f};             ///< Model-to-world transform
    glm::vec4 baseColorOverride{1.0f};       ///< Base color multiplier (applied on top of material)
    glm::vec4 emissiveOverride{0.0f};        ///< Emissive override (0 = use material)
    float ambientOverride = 0.0f;            ///< Per-instance ambient (0 = use global)
    std::uint32_t baseInstanceId = 0;        ///< Starting instance ID for storage buffer

    bool isValid() const {
        return asset != nullptr;
    }
};

/**
 * @struct RenderPassState
 * @brief Tracks the current state of a render pass for optimization.
 *
 * Used to avoid redundant state changes during rendering.
 */
struct RenderPassState {
    SDL_GPUBuffer* boundVertexBuffer = nullptr;
    SDL_GPUBuffer* boundIndexBuffer = nullptr;
    SDL_GPUTexture* boundDiffuseTexture = nullptr;
    SDL_GPUSampler* boundDiffuseSampler = nullptr;
    bool viewProjectionBound = false;
    bool lightingBound = false;

    void reset() {
        boundVertexBuffer = nullptr;
        boundIndexBuffer = nullptr;
        boundDiffuseTexture = nullptr;
        boundDiffuseSampler = nullptr;
        viewProjectionBound = false;
        lightingBound = false;
    }
};

/**
 * @struct RenderCommandStats
 * @brief Statistics about render command recording.
 */
struct RenderCommandStats {
    std::uint32_t drawCalls = 0;             ///< Number of draw calls issued
    std::uint32_t meshesDrawn = 0;           ///< Number of meshes drawn
    std::uint32_t trianglesDrawn = 0;        ///< Total triangles drawn
    std::uint32_t bufferBinds = 0;           ///< Vertex/index buffer binds
    std::uint32_t textureBinds = 0;          ///< Texture binds
    std::uint32_t uniformUploads = 0;        ///< Uniform buffer uploads
    std::uint32_t instancedDrawCalls = 0;    ///< Draw calls using GPU instancing
    std::uint32_t totalInstances = 0;        ///< Total instances rendered

    void reset() {
        drawCalls = 0;
        meshesDrawn = 0;
        trianglesDrawn = 0;
        bufferBinds = 0;
        textureBinds = 0;
        uniformUploads = 0;
        instancedDrawCalls = 0;
        totalInstances = 0;
    }
};

/**
 * @namespace RenderCommands
 * @brief Functions for recording GPU draw commands.
 *
 * Provides stateless functions that record draw commands to an active
 * render pass. All functions require a valid render pass and command buffer.
 */
namespace RenderCommands {

    /**
     * @brief Bind vertex and index buffers for a mesh.
     *
     * Binds the mesh's vertex buffer to slot 0 and index buffer for indexed drawing.
     * Tracks bound state to avoid redundant binds.
     *
     * @param renderPass Active render pass
     * @param mesh Mesh with vertex/index buffers
     * @param state Render pass state for redundancy tracking
     * @param stats Statistics accumulator (optional)
     * @return true if buffers were bound (or already bound), false on error
     */
    bool bindMeshBuffers(
        SDL_GPURenderPass* renderPass,
        const GPUMesh& mesh,
        RenderPassState& state,
        RenderCommandStats* stats = nullptr);

    /**
     * @brief Bind diffuse texture and sampler for a material.
     *
     * Binds the material's diffuse texture to fragment texture slot 0.
     * If no texture is available, binds nothing (shader uses vertex color).
     * Tracks bound state to avoid redundant binds.
     *
     * @param renderPass Active render pass
     * @param material Material with texture references
     * @param state Render pass state for redundancy tracking
     * @param stats Statistics accumulator (optional)
     * @return true if texture was bound (or no texture needed), false on error
     */
    bool bindMaterialTextures(
        SDL_GPURenderPass* renderPass,
        const GPUMaterial& material,
        RenderPassState& state,
        RenderCommandStats* stats = nullptr);

    /**
     * @brief Bind a texture and sampler directly.
     *
     * Low-level function to bind a specific texture to a fragment shader slot.
     * Tracks bound state to avoid redundant binds.
     *
     * @param renderPass Active render pass
     * @param texture Texture to bind
     * @param slot Fragment texture slot (0 for diffuse)
     * @param state Render pass state for redundancy tracking
     * @param stats Statistics accumulator (optional)
     * @return true if texture was bound, false on error
     */
    bool bindTexture(
        SDL_GPURenderPass* renderPass,
        const Texture* texture,
        std::uint32_t slot,
        RenderPassState& state,
        RenderCommandStats* stats = nullptr);

    /**
     * @brief Upload view-projection matrix to uniform buffer.
     *
     * Allocates from the uniform buffer pool and binds to vertex uniform slot 0.
     * Call once per frame before drawing.
     *
     * @param cmdBuffer Command buffer for copy commands
     * @param uboPool Uniform buffer pool for allocation
     * @param viewProjection View-projection matrix data
     * @param stats Statistics accumulator (optional)
     * @return true if upload succeeded, false on allocation failure
     */
    bool uploadViewProjection(
        SDL_GPUCommandBuffer* cmdBuffer,
        UniformBufferPool& uboPool,
        const ToonViewProjectionUBO& viewProjection,
        RenderCommandStats* stats = nullptr);

    /**
     * @brief Bind view-projection uniform buffer to render pass.
     *
     * Must be called after uploadViewProjection and after beginning render pass.
     *
     * @param renderPass Active render pass
     * @param buffer GPU buffer containing view-projection data
     * @param offset Byte offset within buffer
     * @param state Render pass state for redundancy tracking
     */
    void bindViewProjectionBuffer(
        SDL_GPURenderPass* renderPass,
        SDL_GPUBuffer* buffer,
        std::uint32_t offset,
        RenderPassState& state);

    /**
     * @brief Upload lighting parameters to uniform buffer.
     *
     * Allocates from the uniform buffer pool and binds to fragment uniform slot 0.
     * Call once per frame before drawing.
     *
     * @param cmdBuffer Command buffer for copy commands
     * @param uboPool Uniform buffer pool for allocation
     * @param lighting Lighting parameters data
     * @param stats Statistics accumulator (optional)
     * @return true if upload succeeded, false on allocation failure
     */
    bool uploadLighting(
        SDL_GPUCommandBuffer* cmdBuffer,
        UniformBufferPool& uboPool,
        const ToonLightingUBO& lighting,
        RenderCommandStats* stats = nullptr);

    /**
     * @brief Bind lighting uniform buffer to render pass.
     *
     * Must be called after uploadLighting and after beginning render pass.
     *
     * @param renderPass Active render pass
     * @param buffer GPU buffer containing lighting data
     * @param offset Byte offset within buffer
     * @param state Render pass state for redundancy tracking
     */
    void bindLightingBuffer(
        SDL_GPURenderPass* renderPass,
        SDL_GPUBuffer* buffer,
        std::uint32_t offset,
        RenderPassState& state);

    /**
     * @brief Create instance data from draw parameters.
     *
     * Populates a ToonInstanceData structure with model matrix and colors
     * for upload to the instance storage buffer.
     *
     * @param modelMatrix Model-to-world transform
     * @param baseColor Base color multiplier
     * @param emissiveColor Emissive color (RGB) + intensity (A)
     * @param ambientOverride Per-instance ambient (0 = use global)
     * @return Populated ToonInstanceData
     */
    ToonInstanceData createInstanceData(
        const glm::mat4& modelMatrix,
        const glm::vec4& baseColor = glm::vec4(1.0f),
        const glm::vec4& emissiveColor = glm::vec4(0.0f),
        float ambientOverride = 0.0f);

    /**
     * @brief Draw a single GPUMesh with the current pipeline state.
     *
     * Binds vertex/index buffers if not already bound, uploads model matrix
     * to uniform buffer, binds material textures, and issues indexed draw call.
     *
     * Prerequisites:
     * - Pipeline bound via SDL_BindGPUGraphicsPipeline
     * - View-projection and lighting uniforms bound
     *
     * @param renderPass Active render pass
     * @param cmdBuffer Command buffer for copy commands
     * @param uboPool Uniform buffer pool for model matrix allocation
     * @param params Draw parameters
     * @param state Render pass state for redundancy tracking
     * @param stats Statistics accumulator (optional)
     * @return true if draw succeeded, false on error
     */
    bool drawMesh(
        SDL_GPURenderPass* renderPass,
        SDL_GPUCommandBuffer* cmdBuffer,
        UniformBufferPool& uboPool,
        const DrawMeshParams& params,
        RenderPassState& state,
        RenderCommandStats* stats = nullptr);

    /**
     * @brief Draw all meshes in a ModelAsset.
     *
     * Iterates through all meshes in the asset, binding buffers and textures
     * as needed (with redundancy elimination), and issues draw calls.
     *
     * Prerequisites:
     * - Pipeline bound via SDL_BindGPUGraphicsPipeline
     * - View-projection and lighting uniforms bound
     *
     * @param renderPass Active render pass
     * @param cmdBuffer Command buffer for copy commands
     * @param uboPool Uniform buffer pool for model matrix allocation
     * @param params Draw parameters including model asset
     * @param state Render pass state for redundancy tracking
     * @param stats Statistics accumulator (optional)
     * @return Number of meshes successfully drawn
     */
    std::uint32_t drawModelAsset(
        SDL_GPURenderPass* renderPass,
        SDL_GPUCommandBuffer* cmdBuffer,
        UniformBufferPool& uboPool,
        const DrawModelParams& params,
        RenderPassState& state,
        RenderCommandStats* stats = nullptr);

    /**
     * @brief Issue indexed draw call for the currently bound mesh.
     *
     * Low-level draw function. Assumes buffers and uniforms are already bound.
     *
     * @param renderPass Active render pass
     * @param indexCount Number of indices to draw
     * @param instanceCount Number of instances (default 1)
     * @param firstIndex Starting index offset (default 0)
     * @param vertexOffset Added to each index (default 0)
     * @param firstInstance Starting instance ID (default 0)
     * @param stats Statistics accumulator (optional)
     */
    void drawIndexed(
        SDL_GPURenderPass* renderPass,
        std::uint32_t indexCount,
        std::uint32_t instanceCount = 1,
        std::uint32_t firstIndex = 0,
        std::int32_t vertexOffset = 0,
        std::uint32_t firstInstance = 0,
        RenderCommandStats* stats = nullptr);

    /**
     * @brief Get the last error message.
     *
     * @return Error string from last failed operation
     */
    const std::string& getLastError();

    /**
     * @brief Log render statistics.
     *
     * @param stats Statistics to log
     * @param label Optional label for the log message
     */
    void logStats(const RenderCommandStats& stats, const char* label = nullptr);

    // =========================================================================
    // Instanced Draw Commands
    // =========================================================================

    /**
     * @brief Issue instanced indexed draw call.
     *
     * Draws multiple instances of the same mesh with a single draw call.
     * Instance data (transforms, colors) must be pre-uploaded to instance buffer.
     *
     * @param renderPass Active render pass
     * @param mesh Mesh to draw (vertex/index buffers must already be bound)
     * @param instanceCount Number of instances to draw
     * @param firstInstance Starting instance ID
     * @param stats Statistics accumulator (optional)
     */
    void drawIndexedInstanced(
        SDL_GPURenderPass* renderPass,
        const GPUMesh& mesh,
        std::uint32_t instanceCount,
        std::uint32_t firstInstance = 0,
        RenderCommandStats* stats = nullptr);

    /**
     * @brief Draw a model with instancing.
     *
     * Draws all meshes in the model with the same instance data.
     * Useful when a model has multiple meshes (e.g., building with windows).
     *
     * @param renderPass Active render pass
     * @param asset Model asset to draw
     * @param instanceCount Number of instances
     * @param state Render pass state for redundancy tracking
     * @param stats Statistics accumulator (optional)
     * @return Number of draw calls issued
     */
    std::uint32_t drawModelInstanced(
        SDL_GPURenderPass* renderPass,
        const ModelAsset& asset,
        std::uint32_t instanceCount,
        RenderPassState& state,
        RenderCommandStats* stats = nullptr);

    /**
     * @brief Bind an instance storage buffer to the render pass.
     *
     * @param renderPass Active render pass
     * @param buffer GPU buffer containing instance data
     * @param slot Vertex storage buffer slot (default 0)
     */
    void bindInstanceBuffer(
        SDL_GPURenderPass* renderPass,
        SDL_GPUBuffer* buffer,
        std::uint32_t slot = 0);

} // namespace RenderCommands

} // namespace sims3000

#endif // SIMS3000_RENDER_RENDER_COMMANDS_H
