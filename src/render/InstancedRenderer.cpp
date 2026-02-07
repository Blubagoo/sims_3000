/**
 * @file InstancedRenderer.cpp
 * @brief Implementation of high-level instanced rendering system.
 */

#include "sims3000/render/InstancedRenderer.h"
#include "sims3000/render/GPUDevice.h"
#include "sims3000/render/UniformBufferPool.h"

#include <SDL3/SDL_log.h>
#include <algorithm>

namespace sims3000 {

// =============================================================================
// InstancedRenderer Implementation
// =============================================================================

InstancedRenderer::InstancedRenderer(
    GPUDevice& device,
    const InstancedRendererConfig& config)
    : m_device(&device)
    , m_config(config)
{
    // Initialize frustum planes to identity (all visible)
    for (int i = 0; i < 6; ++i) {
        m_frustumPlanes[i] = glm::vec4(0.0f);
    }
}

InstancedRenderer::~InstancedRenderer() {
    // Batches will clean up their instance buffers automatically
    m_batches.clear();
}

bool InstancedRenderer::registerModel(
    std::uint64_t modelId,
    const ModelAsset* asset,
    std::uint32_t capacity)
{
    if (!m_device || !m_device->isValid()) {
        m_lastError = "InstancedRenderer::registerModel: invalid GPU device";
        return false;
    }

    if (!asset || !asset->isValid()) {
        m_lastError = "InstancedRenderer::registerModel: invalid model asset";
        return false;
    }

    // Check if already registered
    auto it = m_batches.find(modelId);
    if (it != m_batches.end()) {
        // Update asset pointer, keep existing buffer
        it->second.asset = asset;
        return true;
    }

    // Determine capacity
    std::uint32_t effectiveCapacity = capacity > 0 ? capacity : m_config.defaultBufferCapacity;
    if (modelId == TERRAIN_MODEL_ID) {
        effectiveCapacity = m_config.terrainBufferCapacity;
    }

    // Create new batch
    ModelBatch batch;
    batch.modelId = modelId;
    batch.asset = asset;
    batch.buffer = std::make_unique<InstanceBuffer>(*m_device);

    if (!batch.buffer->create(effectiveCapacity, m_config.enableChunking, m_config.chunkSize)) {
        m_lastError = "InstancedRenderer::registerModel: failed to create instance buffer - ";
        m_lastError += batch.buffer->getLastError();
        return false;
    }

    m_batches[modelId] = std::move(batch);

    SDL_LogInfo(SDL_LOG_CATEGORY_RENDER,
        "InstancedRenderer: registered model %llu with capacity %u",
        static_cast<unsigned long long>(modelId), effectiveCapacity);

    return true;
}

bool InstancedRenderer::registerTerrainModel(const ModelAsset* asset) {
    return registerModel(TERRAIN_MODEL_ID, asset, m_config.terrainBufferCapacity);
}

void InstancedRenderer::unregisterModel(std::uint64_t modelId) {
    auto it = m_batches.find(modelId);
    if (it != m_batches.end()) {
        m_batches.erase(it);
    }
}

bool InstancedRenderer::isModelRegistered(std::uint64_t modelId) const {
    return m_batches.find(modelId) != m_batches.end();
}

void InstancedRenderer::beginFrame() {
    m_stats.reset();

    for (auto& pair : m_batches) {
        pair.second.buffer->begin();
    }
}

bool InstancedRenderer::addInstance(
    std::uint64_t modelId,
    const glm::mat4& transform,
    const glm::vec4& tintColor,
    const glm::vec4& emissiveColor,
    float ambientOverride)
{
    ModelBatch* batch = getBatch(modelId);
    if (!batch) {
        // Model not registered - this is a silent failure
        // (caller may be adding instances for models not yet loaded)
        return false;
    }

    std::uint32_t index = batch->buffer->add(transform, tintColor, emissiveColor, ambientOverride);
    return index != UINT32_MAX;
}

bool InstancedRenderer::addTerrainInstance(
    const glm::mat4& transform,
    const glm::vec4& tintColor,
    const glm::vec4& emissiveColor)
{
    return addInstance(TERRAIN_MODEL_ID, transform, tintColor, emissiveColor, 0.0f);
}

bool InstancedRenderer::uploadInstances(SDL_GPUCommandBuffer* cmdBuffer) {
    if (!cmdBuffer) {
        m_lastError = "InstancedRenderer::uploadInstances: command buffer is null";
        return false;
    }

    bool allSucceeded = true;

    for (auto& pair : m_batches) {
        InstanceBuffer* buffer = pair.second.buffer.get();
        if (buffer->getInstanceCount() > 0) {
            // Update chunk visibility if frustum culling is enabled
            if (m_config.enableFrustumCulling && m_frustumPlanesValid && buffer->isChunked()) {
                buffer->updateChunkVisibility(m_frustumPlanes);
            }

            if (!buffer->end(cmdBuffer)) {
                SDL_LogWarn(SDL_LOG_CATEGORY_RENDER,
                    "InstancedRenderer: failed to upload instances for model %llu: %s",
                    static_cast<unsigned long long>(pair.first),
                    buffer->getLastError().c_str());
                allSucceeded = false;
            }
        }
    }

    return allSucceeded;
}

std::uint32_t InstancedRenderer::render(
    SDL_GPURenderPass* renderPass,
    SDL_GPUCommandBuffer* cmdBuffer,
    const ToonPipeline& pipeline,
    UniformBufferPool& uboPool,
    RenderPassState& state,
    RenderCommandStats* stats)
{
    if (!renderPass) {
        m_lastError = "InstancedRenderer::render: render pass is null";
        return 0;
    }

    std::uint32_t totalDrawCalls = 0;
    std::uint32_t naiveDrawCalls = 0;  // What it would be without instancing

    for (auto& pair : m_batches) {
        const ModelBatch& batch = pair.second;
        InstanceBuffer* buffer = batch.buffer.get();
        const ModelAsset* asset = batch.asset;

        std::uint32_t instanceCount = buffer->getInstanceCount();
        if (instanceCount == 0 || !asset) {
            continue;
        }

        m_stats.totalInstances += instanceCount;
        m_stats.batchCount++;
        naiveDrawCalls += instanceCount;  // Without instancing, one draw per instance

        // Bind the instance buffer to storage slot 0
        buffer->bind(renderPass, 0);

        // Render each mesh in the model
        for (std::size_t meshIdx = 0; meshIdx < asset->meshes.size(); ++meshIdx) {
            const GPUMesh& mesh = asset->meshes[meshIdx];
            if (!mesh.isValid()) {
                continue;
            }

            // Bind mesh buffers
            if (!RenderCommands::bindMeshBuffers(renderPass, mesh, state, stats)) {
                continue;
            }

            // Bind material textures
            const GPUMaterial* material = asset->getMeshMaterial(meshIdx);
            if (material) {
                RenderCommands::bindMaterialTextures(renderPass, *material, state, stats);
            }

            // Issue instanced draw call
            SDL_DrawGPUIndexedPrimitives(
                renderPass,
                mesh.indexCount,    // Number of indices per instance
                instanceCount,      // Number of instances
                0,                  // First index
                0,                  // Vertex offset
                0                   // First instance
            );

            totalDrawCalls++;
            m_stats.instancedDrawCalls++;
            m_stats.totalTriangles += (mesh.indexCount / 3) * instanceCount;

            if (stats) {
                stats->drawCalls++;
                stats->meshesDrawn += instanceCount;
                stats->trianglesDrawn += (mesh.indexCount / 3) * instanceCount;
            }
        }
    }

    m_stats.totalDrawCalls = totalDrawCalls;

    // Calculate draw call reduction ratio
    if (naiveDrawCalls > 0) {
        m_stats.drawCallReduction = 1.0f - static_cast<float>(totalDrawCalls) / static_cast<float>(naiveDrawCalls);
    }

    return totalDrawCalls;
}

std::uint32_t InstancedRenderer::renderModel(
    std::uint64_t modelId,
    SDL_GPURenderPass* renderPass,
    const ToonPipeline& pipeline,
    RenderPassState& state,
    RenderCommandStats* stats)
{
    if (!renderPass) {
        return 0;
    }

    auto it = m_batches.find(modelId);
    if (it == m_batches.end()) {
        return 0;
    }

    const ModelBatch& batch = it->second;
    InstanceBuffer* buffer = batch.buffer.get();
    const ModelAsset* asset = batch.asset;

    std::uint32_t instanceCount = buffer->getInstanceCount();
    if (instanceCount == 0 || !asset) {
        return 0;
    }

    std::uint32_t drawCalls = 0;

    // Bind the instance buffer
    buffer->bind(renderPass, 0);

    // Render each mesh
    for (std::size_t meshIdx = 0; meshIdx < asset->meshes.size(); ++meshIdx) {
        const GPUMesh& mesh = asset->meshes[meshIdx];
        if (!mesh.isValid()) {
            continue;
        }

        RenderCommands::bindMeshBuffers(renderPass, mesh, state, stats);

        const GPUMaterial* material = asset->getMeshMaterial(meshIdx);
        if (material) {
            RenderCommands::bindMaterialTextures(renderPass, *material, state, stats);
        }

        SDL_DrawGPUIndexedPrimitives(
            renderPass,
            mesh.indexCount,
            instanceCount,
            0, 0, 0
        );

        drawCalls++;

        if (stats) {
            stats->drawCalls++;
            stats->meshesDrawn += instanceCount;
            stats->trianglesDrawn += (mesh.indexCount / 3) * instanceCount;
        }
    }

    return drawCalls;
}

void InstancedRenderer::setViewProjection(const glm::mat4& viewProjection) {
    extractFrustumPlanes(viewProjection, m_frustumPlanes);
    m_frustumPlanesValid = true;
}

void InstancedRenderer::extractFrustumPlanes(
    const glm::mat4& viewProjection,
    glm::vec4 outPlanes[6])
{
    // Extract frustum planes from the view-projection matrix
    // Using the Gribb/Hartmann method

    const glm::mat4& m = viewProjection;

    // Left plane: row3 + row0
    outPlanes[0] = glm::vec4(
        m[0][3] + m[0][0],
        m[1][3] + m[1][0],
        m[2][3] + m[2][0],
        m[3][3] + m[3][0]
    );

    // Right plane: row3 - row0
    outPlanes[1] = glm::vec4(
        m[0][3] - m[0][0],
        m[1][3] - m[1][0],
        m[2][3] - m[2][0],
        m[3][3] - m[3][0]
    );

    // Bottom plane: row3 + row1
    outPlanes[2] = glm::vec4(
        m[0][3] + m[0][1],
        m[1][3] + m[1][1],
        m[2][3] + m[2][1],
        m[3][3] + m[3][1]
    );

    // Top plane: row3 - row1
    outPlanes[3] = glm::vec4(
        m[0][3] - m[0][1],
        m[1][3] - m[1][1],
        m[2][3] - m[2][1],
        m[3][3] - m[3][1]
    );

    // Near plane: row3 + row2
    outPlanes[4] = glm::vec4(
        m[0][3] + m[0][2],
        m[1][3] + m[1][2],
        m[2][3] + m[2][2],
        m[3][3] + m[3][2]
    );

    // Far plane: row3 - row2
    outPlanes[5] = glm::vec4(
        m[0][3] - m[0][2],
        m[1][3] - m[1][2],
        m[2][3] - m[2][2],
        m[3][3] - m[3][2]
    );

    // Normalize all planes
    for (int i = 0; i < 6; ++i) {
        float length = glm::length(glm::vec3(outPlanes[i]));
        if (length > 0.0001f) {
            outPlanes[i] /= length;
        }
    }
}

ModelBatch* InstancedRenderer::getBatch(std::uint64_t modelId) {
    auto it = m_batches.find(modelId);
    if (it != m_batches.end()) {
        return &it->second;
    }
    return nullptr;
}

} // namespace sims3000
