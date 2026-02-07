/**
 * @file TransparentRenderQueue.cpp
 * @brief Implementation of transparent object render queue with back-to-front sorting.
 */

#include "sims3000/render/TransparentRenderQueue.h"
#include "sims3000/render/ToonPipeline.h"
#include "sims3000/render/ToonShader.h"
#include "sims3000/render/UniformBufferPool.h"

#include <SDL3/SDL_log.h>
#include <algorithm>
#include <chrono>

namespace sims3000 {

// =============================================================================
// Construction
// =============================================================================

TransparentRenderQueue::TransparentRenderQueue(const TransparentRenderQueueConfig& config)
    : m_config(config)
{
    m_objects.reserve(config.initialCapacity);
}

// =============================================================================
// Frame Lifecycle
// =============================================================================

void TransparentRenderQueue::begin(const glm::vec3& cameraPosition) {
    m_objects.clear();
    m_cameraPosition = cameraPosition;
    m_sorted = false;
    m_stats.reset();
}

void TransparentRenderQueue::sortBackToFront() {
    if (m_objects.empty()) {
        m_sorted = true;
        return;
    }

    auto startTime = std::chrono::high_resolution_clock::now();

    // Calculate distance for each object
    for (auto& obj : m_objects) {
        obj.distanceSquared = calculateDistanceSquared(obj.transform);
    }

    // Sort back-to-front (far objects first, near objects last)
    // This ensures correct alpha blending order
    std::sort(m_objects.begin(), m_objects.end(),
        [](const TransparentObject& a, const TransparentObject& b) {
            // Primary sort: by distance (far to near)
            if (std::abs(a.distanceSquared - b.distanceSquared) > 0.0001f) {
                return a.distanceSquared > b.distanceSquared;
            }
            // Secondary sort: by layer (lower layers first within same distance)
            return static_cast<int>(a.layer) < static_cast<int>(b.layer);
        });

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    m_stats.sortTimeMs = static_cast<float>(duration.count()) / 1000.0f;

    m_sorted = true;
}

std::uint32_t TransparentRenderQueue::render(
    SDL_GPURenderPass* renderPass,
    SDL_GPUCommandBuffer* cmdBuffer,
    const ToonPipeline& pipeline,
    UniformBufferPool& uboPool,
    RenderPassState& state,
    RenderCommandStats* stats)
{
    if (!renderPass || !cmdBuffer) {
        m_lastError = "TransparentRenderQueue::render: null render pass or command buffer";
        return 0;
    }

    if (!m_sorted) {
        SDL_LogWarn(SDL_LOG_CATEGORY_RENDER,
            "TransparentRenderQueue::render: called without sorting - call sortBackToFront() first");
        sortBackToFront();
    }

    if (m_objects.empty()) {
        return 0;
    }

    // Bind transparent pipeline (depth test ON, depth write OFF, alpha blend ON)
    // Use getCurrentTransparentPipeline() to respect wireframe mode setting
    SDL_BindGPUGraphicsPipeline(renderPass, pipeline.getCurrentTransparentPipeline());

    std::uint32_t drawCalls = 0;
    m_stats.objectCount = static_cast<std::uint32_t>(m_objects.size());

    // Render each object in sorted order (back-to-front)
    for (const auto& obj : m_objects) {
        if (!obj.isValid()) {
            continue;
        }

        // Update per-object statistics
        switch (obj.type) {
            case TransparentObjectType::ConstructionGhost:
                m_stats.ghostCount++;
                break;
            case TransparentObjectType::SelectionOverlay:
                m_stats.selectionCount++;
                break;
            case TransparentObjectType::Effect:
                m_stats.effectCount++;
                break;
            default:
                break;
        }

        // Bind mesh buffers
        if (!RenderCommands::bindMeshBuffers(renderPass, *obj.mesh, state, stats)) {
            continue;
        }

        // Bind material textures if available
        if (obj.material) {
            RenderCommands::bindMaterialTextures(renderPass, *obj.material, state, stats);
        }

        // Create instance data for this object
        ToonInstanceData instanceData = RenderCommands::createInstanceData(
            obj.transform,
            obj.color,
            obj.emissive,
            0.0f  // Use global ambient
        );

        // Upload instance data as a push constant or uniform
        // For single-object rendering, we can use a uniform buffer allocation
        UniformBufferAllocation uboAlloc = uboPool.allocate(sizeof(ToonInstanceData));
        if (uboAlloc.isValid()) {
            // Copy instance data to the uniform buffer
            SDL_GPUTransferBufferCreateInfo transferInfo = {};
            transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
            transferInfo.size = sizeof(ToonInstanceData);

            // For simplicity, bind the model matrix via vertex uniform slot 1
            // Note: In a real implementation, this would use push constants or
            // a per-draw uniform buffer binding
            SDL_GPUBuffer* buffers[] = { uboAlloc.buffer };
            SDL_BindGPUVertexStorageBuffers(renderPass, 0, buffers, 1);
        }

        // Issue draw call
        SDL_DrawGPUIndexedPrimitives(
            renderPass,
            obj.mesh->indexCount,
            1,      // Single instance
            0,      // First index
            0,      // Vertex offset
            0       // First instance
        );

        drawCalls++;
        m_stats.trianglesDrawn += obj.mesh->indexCount / 3;

        if (stats) {
            stats->drawCalls++;
            stats->meshesDrawn++;
            stats->trianglesDrawn += obj.mesh->indexCount / 3;
        }
    }

    m_stats.drawCalls = drawCalls;
    return drawCalls;
}

// =============================================================================
// Object Submission
// =============================================================================

void TransparentRenderQueue::addObject(
    const GPUMesh* mesh,
    const GPUMaterial* material,
    const glm::mat4& transform,
    const glm::vec4& color,
    const glm::vec4& emissive,
    RenderLayer layer)
{
    if (!mesh || !mesh->isValid()) {
        return;
    }

    TransparentObject obj;
    obj.mesh = mesh;
    obj.material = material;
    obj.transform = transform;
    obj.color = color;
    obj.emissive = emissive;
    obj.type = TransparentObjectType::Generic;
    obj.layer = layer;

    m_objects.push_back(obj);
    m_sorted = false;
}

void TransparentRenderQueue::addConstructionGhost(
    const GPUMesh* mesh,
    const GPUMaterial* material,
    const glm::mat4& transform,
    float alpha)
{
    if (!mesh || !mesh->isValid()) {
        return;
    }

    float effectiveAlpha = (alpha > 0.0f) ? alpha : m_config.ghostAlpha;

    TransparentObject obj;
    obj.mesh = mesh;
    obj.material = material;
    obj.transform = transform;
    obj.color = glm::vec4(
        m_config.ghostTint.r,
        m_config.ghostTint.g,
        m_config.ghostTint.b,
        effectiveAlpha
    );
    obj.emissive = glm::vec4(0.0f);
    obj.type = TransparentObjectType::ConstructionGhost;
    obj.layer = RenderLayer::UIWorld;  // Ghosts render on top

    m_objects.push_back(obj);
    m_sorted = false;
}

void TransparentRenderQueue::addSelectionOverlay(
    const GPUMesh* mesh,
    const GPUMaterial* material,
    const glm::mat4& transform,
    const glm::vec4& selectionColor)
{
    if (!mesh || !mesh->isValid()) {
        return;
    }

    glm::vec4 effectiveColor = (selectionColor.a > 0.0f)
        ? selectionColor
        : m_config.selectionTint;

    TransparentObject obj;
    obj.mesh = mesh;
    obj.material = material;
    obj.transform = transform;
    obj.color = effectiveColor;
    obj.emissive = glm::vec4(effectiveColor.r, effectiveColor.g, effectiveColor.b, 0.5f);  // Slight glow
    obj.type = TransparentObjectType::SelectionOverlay;
    obj.layer = RenderLayer::UIWorld;

    m_objects.push_back(obj);
    m_sorted = false;
}

void TransparentRenderQueue::addUndergroundGhost(
    const GPUMesh* mesh,
    const GPUMaterial* material,
    const glm::mat4& transform,
    float alpha)
{
    if (!mesh || !mesh->isValid()) {
        return;
    }

    float effectiveAlpha = (alpha > 0.0f) ? alpha : m_config.ghostAlpha;

    TransparentObject obj;
    obj.mesh = mesh;
    obj.material = material;
    obj.transform = transform;
    obj.color = glm::vec4(0.5f, 0.5f, 0.5f, effectiveAlpha);  // Neutral gray ghost
    obj.emissive = glm::vec4(0.0f);
    obj.type = TransparentObjectType::UndergroundGhost;
    obj.layer = RenderLayer::Buildings;  // Same layer as buildings but transparent

    m_objects.push_back(obj);
    m_sorted = false;
}

// =============================================================================
// Configuration
// =============================================================================

void TransparentRenderQueue::setConfig(const TransparentRenderQueueConfig& config) {
    m_config = config;
}

// =============================================================================
// Private Helpers
// =============================================================================

float TransparentRenderQueue::calculateDistanceSquared(const glm::mat4& transform) const {
    glm::vec3 objectPos = extractPosition(transform);
    glm::vec3 delta = objectPos - m_cameraPosition;
    return glm::dot(delta, delta);
}

glm::vec3 TransparentRenderQueue::extractPosition(const glm::mat4& transform) {
    // The translation is in the 4th column of the matrix
    return glm::vec3(transform[3][0], transform[3][1], transform[3][2]);
}

} // namespace sims3000
