/**
 * @file RenderCommands.cpp
 * @brief Implementation of render command recording for 3D models.
 */

#include "sims3000/render/RenderCommands.h"
#include "sims3000/render/GPUMesh.h"
#include "sims3000/render/ToonShader.h"
#include "sims3000/render/ToonPipeline.h"
#include "sims3000/render/UniformBufferPool.h"
#include "sims3000/assets/TextureLoader.h"

#include <SDL3/SDL_log.h>
#include <cstring>

namespace sims3000 {

// Static error string storage
static std::string s_lastError;

namespace RenderCommands {

// =============================================================================
// Buffer Binding
// =============================================================================

bool bindMeshBuffers(
    SDL_GPURenderPass* renderPass,
    const GPUMesh& mesh,
    RenderPassState& state,
    RenderCommandStats* stats)
{
    if (!renderPass) {
        s_lastError = "bindMeshBuffers: render pass is null";
        return false;
    }

    if (!mesh.isValid()) {
        s_lastError = "bindMeshBuffers: mesh is invalid (null buffers or zero indices)";
        return false;
    }

    // Bind vertex buffer if not already bound
    if (state.boundVertexBuffer != mesh.vertexBuffer) {
        SDL_GPUBufferBinding vertexBinding = {};
        vertexBinding.buffer = mesh.vertexBuffer;
        vertexBinding.offset = 0;

        SDL_BindGPUVertexBuffers(renderPass, 0, &vertexBinding, 1);
        state.boundVertexBuffer = mesh.vertexBuffer;

        if (stats) {
            stats->bufferBinds++;
        }
    }

    // Bind index buffer if not already bound
    if (state.boundIndexBuffer != mesh.indexBuffer) {
        SDL_GPUBufferBinding indexBinding = {};
        indexBinding.buffer = mesh.indexBuffer;
        indexBinding.offset = 0;

        SDL_BindGPUIndexBuffer(renderPass, &indexBinding, SDL_GPU_INDEXELEMENTSIZE_32BIT);
        state.boundIndexBuffer = mesh.indexBuffer;

        if (stats) {
            stats->bufferBinds++;
        }
    }

    return true;
}

// =============================================================================
// Texture Binding
// =============================================================================

bool bindMaterialTextures(
    SDL_GPURenderPass* renderPass,
    const GPUMaterial& material,
    RenderPassState& state,
    RenderCommandStats* stats)
{
    if (!renderPass) {
        s_lastError = "bindMaterialTextures: render pass is null";
        return false;
    }

    // If material has no diffuse texture, nothing to bind
    // Shader will use vertex color or base color factor
    if (!material.hasDiffuseTexture()) {
        // Unbind previous texture by binding null
        if (state.boundDiffuseTexture != nullptr) {
            // Can't unbind in SDL_GPU - just track that we have no texture
            state.boundDiffuseTexture = nullptr;
            state.boundDiffuseSampler = nullptr;
        }
        return true;
    }

    // Get texture and sampler from material's diffuse texture
    // The TextureHandle (Texture*) contains both gpuTexture and sampler
    Texture* tex = material.diffuseTexture;
    if (!tex || !tex->gpuTexture) {
        // Texture handle exists but GPU texture is null
        state.boundDiffuseTexture = nullptr;
        state.boundDiffuseSampler = nullptr;
        return true;
    }

    // Bind texture if not already bound
    if (state.boundDiffuseTexture != tex->gpuTexture ||
        state.boundDiffuseSampler != tex->sampler) {

        SDL_GPUTextureSamplerBinding binding = {};
        binding.texture = tex->gpuTexture;
        binding.sampler = tex->sampler;

        SDL_BindGPUFragmentSamplers(renderPass, 0, &binding, 1);

        state.boundDiffuseTexture = tex->gpuTexture;
        state.boundDiffuseSampler = tex->sampler;

        if (stats) {
            stats->textureBinds++;
        }
    }

    return true;
}

bool bindTexture(
    SDL_GPURenderPass* renderPass,
    const Texture* texture,
    std::uint32_t slot,
    RenderPassState& state,
    RenderCommandStats* stats)
{
    if (!renderPass) {
        s_lastError = "bindTexture: render pass is null";
        return false;
    }

    if (!texture || !texture->gpuTexture) {
        // No texture to bind - this is valid, shader will handle it
        return true;
    }

    // For slot 0 (diffuse), use state tracking
    if (slot == 0) {
        if (state.boundDiffuseTexture != texture->gpuTexture ||
            state.boundDiffuseSampler != texture->sampler) {

            SDL_GPUTextureSamplerBinding binding = {};
            binding.texture = texture->gpuTexture;
            binding.sampler = texture->sampler;

            SDL_BindGPUFragmentSamplers(renderPass, slot, &binding, 1);

            state.boundDiffuseTexture = texture->gpuTexture;
            state.boundDiffuseSampler = texture->sampler;

            if (stats) {
                stats->textureBinds++;
            }
        }
    } else {
        // For other slots, always bind (no state tracking for simplicity)
        SDL_GPUTextureSamplerBinding binding = {};
        binding.texture = texture->gpuTexture;
        binding.sampler = texture->sampler;

        SDL_BindGPUFragmentSamplers(renderPass, slot, &binding, 1);

        if (stats) {
            stats->textureBinds++;
        }
    }

    return true;
}

// =============================================================================
// Uniform Buffer Upload and Binding
// =============================================================================

bool uploadViewProjection(
    SDL_GPUCommandBuffer* cmdBuffer,
    UniformBufferPool& uboPool,
    const ToonViewProjectionUBO& viewProjection,
    RenderCommandStats* stats)
{
    if (!cmdBuffer) {
        s_lastError = "uploadViewProjection: command buffer is null";
        return false;
    }

    // Allocate from uniform buffer pool
    UniformBufferAllocation alloc = uboPool.allocate(sizeof(ToonViewProjectionUBO));
    if (!alloc.isValid()) {
        s_lastError = "uploadViewProjection: failed to allocate uniform buffer - ";
        s_lastError += uboPool.getLastError();
        return false;
    }

    // Copy data via transfer buffer
    // NOTE: SDL_GPU requires using a transfer buffer for uploads
    // For now, we assume the uniform buffer pool handles this internally
    // or we need to use SDL_SetGPUBufferData (which may not exist in SDL_GPU)

    // In SDL_GPU, uniform buffers are typically updated via push constants
    // or by binding pre-filled buffers. For per-draw updates, we need to use
    // a staging/transfer buffer approach.

    // Since UniformBufferPool creates GPU buffers, we need a way to upload data.
    // The proper approach is to use SDL_GPUTransferBuffer.

    // For simplicity in this implementation, we'll assume the caller handles
    // the transfer buffer setup and we just track the allocation.

    if (stats) {
        stats->uniformUploads++;
    }

    return true;
}

void bindViewProjectionBuffer(
    SDL_GPURenderPass* renderPass,
    SDL_GPUBuffer* buffer,
    std::uint32_t offset,
    RenderPassState& state)
{
    if (!renderPass || !buffer) {
        return;
    }

    if (!state.viewProjectionBound) {
        // Bind to vertex uniform slot 0
        // NOTE: SDL_GPU uses SDL_BindGPUVertexStorageBuffers for storage buffers
        // For uniform buffers, we use push constants or pre-bound resources
        // The exact binding depends on shader resource layout

        // In toon shader, view-projection is at register(b0, space1)
        // SDL_GPU maps this to uniform buffer binding

        state.viewProjectionBound = true;
    }
}

bool uploadLighting(
    SDL_GPUCommandBuffer* cmdBuffer,
    UniformBufferPool& uboPool,
    const ToonLightingUBO& lighting,
    RenderCommandStats* stats)
{
    if (!cmdBuffer) {
        s_lastError = "uploadLighting: command buffer is null";
        return false;
    }

    // Allocate from uniform buffer pool
    UniformBufferAllocation alloc = uboPool.allocate(sizeof(ToonLightingUBO));
    if (!alloc.isValid()) {
        s_lastError = "uploadLighting: failed to allocate uniform buffer - ";
        s_lastError += uboPool.getLastError();
        return false;
    }

    if (stats) {
        stats->uniformUploads++;
    }

    return true;
}

void bindLightingBuffer(
    SDL_GPURenderPass* renderPass,
    SDL_GPUBuffer* buffer,
    std::uint32_t offset,
    RenderPassState& state)
{
    if (!renderPass || !buffer) {
        return;
    }

    if (!state.lightingBound) {
        // Bind to fragment uniform slot 0
        // In toon shader, lighting is at register(b0, space3)

        state.lightingBound = true;
    }
}

// =============================================================================
// Instance Data Creation
// =============================================================================

ToonInstanceData createInstanceData(
    const glm::mat4& modelMatrix,
    const glm::vec4& baseColor,
    const glm::vec4& emissiveColor,
    float ambientOverride)
{
    ToonInstanceData data;
    data.model = modelMatrix;
    data.baseColor = baseColor;
    data.emissiveColor = emissiveColor;
    data.ambientStrength = ambientOverride;
    return data;
}

// =============================================================================
// Draw Commands
// =============================================================================

bool drawMesh(
    SDL_GPURenderPass* renderPass,
    SDL_GPUCommandBuffer* cmdBuffer,
    UniformBufferPool& uboPool,
    const DrawMeshParams& params,
    RenderPassState& state,
    RenderCommandStats* stats)
{
    if (!renderPass) {
        s_lastError = "drawMesh: render pass is null";
        return false;
    }

    if (!params.isValid()) {
        s_lastError = "drawMesh: invalid params (null mesh)";
        return false;
    }

    const GPUMesh& mesh = *params.mesh;

    // Validate mesh has valid buffers
    if (!mesh.isValid()) {
        s_lastError = "drawMesh: mesh has invalid buffers or zero indices";
        return false;
    }

    // 1. Bind vertex and index buffers
    if (!bindMeshBuffers(renderPass, mesh, state, stats)) {
        return false;
    }

    // 2. Bind material textures if material is provided
    if (params.material) {
        if (!bindMaterialTextures(renderPass, *params.material, state, stats)) {
            // Non-fatal: continue drawing without texture
            SDL_LogWarn(SDL_LOG_CATEGORY_RENDER,
                "drawMesh: failed to bind material textures, drawing without texture");
        }
    }

    // 3. Create and prepare instance data for storage buffer
    // The instance data contains the model matrix and colors
    // This would be uploaded to the instance storage buffer
    ToonInstanceData instanceData = createInstanceData(
        params.modelMatrix,
        params.baseColor,
        params.emissiveColor,
        params.ambientOverride);

    // NOTE: In a full implementation, we would upload instanceData to
    // a storage buffer and set the instance ID. For now, we track it
    // but actual upload depends on the storage buffer management system.

    if (stats) {
        stats->uniformUploads++;
    }

    // 4. Issue indexed draw call
    drawIndexed(renderPass, mesh.indexCount, 1, 0, 0, params.instanceId, stats);

    if (stats) {
        stats->meshesDrawn++;
    }

    return true;
}

std::uint32_t drawModelAsset(
    SDL_GPURenderPass* renderPass,
    SDL_GPUCommandBuffer* cmdBuffer,
    UniformBufferPool& uboPool,
    const DrawModelParams& params,
    RenderPassState& state,
    RenderCommandStats* stats)
{
    if (!renderPass) {
        s_lastError = "drawModelAsset: render pass is null";
        return 0;
    }

    if (!params.isValid()) {
        s_lastError = "drawModelAsset: invalid params (null asset)";
        return 0;
    }

    const ModelAsset& asset = *params.asset;

    if (!asset.isValid()) {
        s_lastError = "drawModelAsset: model asset has no valid meshes";
        return 0;
    }

    std::uint32_t meshesDrawn = 0;

    // Draw each mesh in the model asset
    for (std::size_t i = 0; i < asset.meshes.size(); ++i) {
        const GPUMesh& mesh = asset.meshes[i];

        // Skip invalid meshes
        if (!mesh.isValid()) {
            continue;
        }

        // Get material for this mesh if available
        const GPUMaterial* material = asset.getMeshMaterial(i);

        // Calculate base color: material factor * override
        glm::vec4 baseColor = params.baseColorOverride;
        if (material) {
            baseColor *= material->baseColorFactor;
        }

        // Calculate emissive: use override if non-zero, else material
        glm::vec4 emissiveColor = params.emissiveOverride;
        if (emissiveColor == glm::vec4(0.0f) && material) {
            emissiveColor = glm::vec4(material->emissiveColor, 1.0f);
        }

        // Prepare draw parameters for this mesh
        DrawMeshParams meshParams;
        meshParams.mesh = &mesh;
        meshParams.material = material;
        meshParams.modelMatrix = params.modelMatrix;
        meshParams.baseColor = baseColor;
        meshParams.emissiveColor = emissiveColor;
        meshParams.ambientOverride = params.ambientOverride;
        meshParams.instanceId = params.baseInstanceId + static_cast<std::uint32_t>(i);

        // Draw the mesh
        if (drawMesh(renderPass, cmdBuffer, uboPool, meshParams, state, stats)) {
            meshesDrawn++;
        }
    }

    return meshesDrawn;
}

void drawIndexed(
    SDL_GPURenderPass* renderPass,
    std::uint32_t indexCount,
    std::uint32_t instanceCount,
    std::uint32_t firstIndex,
    std::int32_t vertexOffset,
    std::uint32_t firstInstance,
    RenderCommandStats* stats)
{
    if (!renderPass) {
        return;
    }

    if (indexCount == 0) {
        return;
    }

    SDL_DrawGPUIndexedPrimitives(
        renderPass,
        indexCount,
        instanceCount,
        firstIndex,
        vertexOffset,
        firstInstance);

    if (stats) {
        stats->drawCalls++;
        stats->trianglesDrawn += (indexCount / 3) * instanceCount;
    }
}

// =============================================================================
// Utilities
// =============================================================================

const std::string& getLastError() {
    return s_lastError;
}

void logStats(const RenderCommandStats& stats, const char* label) {
    if (label) {
        SDL_LogInfo(SDL_LOG_CATEGORY_RENDER, "%s:", label);
    } else {
        SDL_LogInfo(SDL_LOG_CATEGORY_RENDER, "Render Stats:");
    }

    SDL_LogInfo(SDL_LOG_CATEGORY_RENDER, "  Draw calls: %u", stats.drawCalls);
    SDL_LogInfo(SDL_LOG_CATEGORY_RENDER, "  Instanced draw calls: %u", stats.instancedDrawCalls);
    SDL_LogInfo(SDL_LOG_CATEGORY_RENDER, "  Total instances: %u", stats.totalInstances);
    SDL_LogInfo(SDL_LOG_CATEGORY_RENDER, "  Meshes drawn: %u", stats.meshesDrawn);
    SDL_LogInfo(SDL_LOG_CATEGORY_RENDER, "  Triangles: %u", stats.trianglesDrawn);
    SDL_LogInfo(SDL_LOG_CATEGORY_RENDER, "  Buffer binds: %u", stats.bufferBinds);
    SDL_LogInfo(SDL_LOG_CATEGORY_RENDER, "  Texture binds: %u", stats.textureBinds);
    SDL_LogInfo(SDL_LOG_CATEGORY_RENDER, "  Uniform uploads: %u", stats.uniformUploads);
}

// =============================================================================
// Instanced Draw Commands
// =============================================================================

void drawIndexedInstanced(
    SDL_GPURenderPass* renderPass,
    const GPUMesh& mesh,
    std::uint32_t instanceCount,
    std::uint32_t firstInstance,
    RenderCommandStats* stats)
{
    if (!renderPass) {
        return;
    }

    if (instanceCount == 0 || !mesh.isValid()) {
        return;
    }

    SDL_DrawGPUIndexedPrimitives(
        renderPass,
        mesh.indexCount,
        instanceCount,
        0,              // First index
        0,              // Vertex offset
        firstInstance);

    if (stats) {
        stats->drawCalls++;
        stats->instancedDrawCalls++;
        stats->totalInstances += instanceCount;
        stats->meshesDrawn += instanceCount;
        stats->trianglesDrawn += (mesh.indexCount / 3) * instanceCount;
    }
}

std::uint32_t drawModelInstanced(
    SDL_GPURenderPass* renderPass,
    const ModelAsset& asset,
    std::uint32_t instanceCount,
    RenderPassState& state,
    RenderCommandStats* stats)
{
    if (!renderPass) {
        s_lastError = "drawModelInstanced: render pass is null";
        return 0;
    }

    if (instanceCount == 0 || !asset.isValid()) {
        return 0;
    }

    std::uint32_t drawCallsIssued = 0;

    for (std::size_t i = 0; i < asset.meshes.size(); ++i) {
        const GPUMesh& mesh = asset.meshes[i];

        if (!mesh.isValid()) {
            continue;
        }

        // Bind mesh buffers
        if (!bindMeshBuffers(renderPass, mesh, state, stats)) {
            continue;
        }

        // Bind material textures
        const GPUMaterial* material = asset.getMeshMaterial(i);
        if (material) {
            bindMaterialTextures(renderPass, *material, state, stats);
        }

        // Issue instanced draw call
        drawIndexedInstanced(renderPass, mesh, instanceCount, 0, stats);
        drawCallsIssued++;
    }

    return drawCallsIssued;
}

void bindInstanceBuffer(
    SDL_GPURenderPass* renderPass,
    SDL_GPUBuffer* buffer,
    std::uint32_t slot)
{
    if (!renderPass || !buffer) {
        return;
    }

    SDL_GPUBuffer* buffers[] = { buffer };
    SDL_BindGPUVertexStorageBuffers(renderPass, slot, buffers, 1);
}

} // namespace RenderCommands

} // namespace sims3000
