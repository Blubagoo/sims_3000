/**
 * @file VegetationRenderer.cpp
 * @brief Implementation of GPU instanced vegetation rendering.
 */

#include "sims3000/render/VegetationRenderer.h"
#include "sims3000/render/GPUDevice.h"
#include "sims3000/render/UniformBufferPool.h"
#include "sims3000/assets/TextureLoader.h"
#include "sims3000/assets/ModelLoader.h"

#include <SDL3/SDL_log.h>
#include <glm/gtc/matrix_transform.hpp>

namespace sims3000 {

// =============================================================================
// Construction / Destruction
// =============================================================================

VegetationRenderer::VegetationRenderer(
    GPUDevice& device,
    TextureLoader& textureLoader,
    ModelLoader& modelLoader,
    const VegetationRendererConfig& config)
    : m_device(&device)
    , m_textureLoader(&textureLoader)
    , m_modelLoader(&modelLoader)
    , m_config(config)
{
}

VegetationRenderer::~VegetationRenderer() {
    // Instance buffers will clean themselves up via unique_ptr
    // ModelAsset textures need to be released
    for (std::size_t i = 0; i < MODEL_TYPE_COUNT; ++i) {
        if (m_modelsLoaded[i] && m_textureLoader) {
            m_models[i].releaseTextures(*m_textureLoader);
        }
    }
}

bool VegetationRenderer::initialize() {
    if (!m_device || !m_device->isValid()) {
        m_lastError = "VegetationRenderer::initialize: invalid GPU device";
        return false;
    }

    // Load vegetation models
    if (!loadModels()) {
        SDL_LogWarn(SDL_LOG_CATEGORY_RENDER,
            "VegetationRenderer: some models failed to load: %s",
            m_lastError.c_str());
        // Continue anyway if we have placeholder models
    }

    // Create instance buffers for each model type
    for (std::size_t i = 0; i < MODEL_TYPE_COUNT; ++i) {
        m_instanceBuffers[i] = std::make_unique<InstanceBuffer>(*m_device);

        if (!m_instanceBuffers[i]->create(m_config.instanceBufferCapacity, false)) {
            m_lastError = "VegetationRenderer::initialize: failed to create instance buffer for type ";
            m_lastError += std::to_string(i);
            m_lastError += " - ";
            m_lastError += m_instanceBuffers[i]->getLastError();
            return false;
        }
    }

    m_initialized = true;

    SDL_LogInfo(SDL_LOG_CATEGORY_RENDER,
        "VegetationRenderer: initialized with capacity %u per model type",
        m_config.instanceBufferCapacity);

    return true;
}

// =============================================================================
// Model Loading
// =============================================================================

bool VegetationRenderer::loadModels() {
    // Model file names for each type
    const std::string modelFiles[MODEL_TYPE_COUNT] = {
        m_config.modelsPath + m_config.biolumeTreeModel,
        m_config.modelsPath + m_config.crystalSpireModel,
        m_config.modelsPath + m_config.sporeEmitterModel
    };

    const char* modelNames[MODEL_TYPE_COUNT] = {
        "BiolumeTree",
        "CrystalSpire",
        "SporeEmitter"
    };

    bool allLoaded = true;

    for (std::size_t i = 0; i < MODEL_TYPE_COUNT; ++i) {
        // Try to load the actual model
        ModelHandle model = m_modelLoader->load(modelFiles[i]);

        if (model) {
            // Create ModelAsset with resolved textures
            m_models[i] = ModelAsset::fromModel(model, *m_textureLoader);
            m_modelsLoaded[i] = m_models[i].isValid();

            if (m_modelsLoaded[i]) {
                SDL_LogInfo(SDL_LOG_CATEGORY_RENDER,
                    "VegetationRenderer: loaded %s from %s",
                    modelNames[i], modelFiles[i].c_str());
            }
        }

        // If model failed to load and placeholders are enabled, create one
        if (!m_modelsLoaded[i] && m_config.usePlaceholderModels) {
            SDL_LogInfo(SDL_LOG_CATEGORY_RENDER,
                "VegetationRenderer: creating placeholder for %s",
                modelNames[i]);

            if (createPlaceholderModel(static_cast<render::VegetationModelType>(i))) {
                m_modelsLoaded[i] = true;
            } else {
                allLoaded = false;
            }
        } else if (!m_modelsLoaded[i]) {
            m_lastError = "Failed to load model: ";
            m_lastError += modelNames[i];
            allLoaded = false;
        }
    }

    return allLoaded;
}

bool VegetationRenderer::createPlaceholderModel(render::VegetationModelType type) {
    // Use the fallback model from ModelLoader (a simple cube)
    ModelHandle fallback = m_modelLoader->createFallback();
    if (!fallback) {
        m_lastError = "Failed to create fallback model for placeholder";
        return false;
    }

    std::size_t idx = static_cast<std::size_t>(type);
    m_models[idx] = ModelAsset::fromModelNoTextures(fallback);

    return m_models[idx].isValid();
}

bool VegetationRenderer::isModelLoaded(render::VegetationModelType type) const {
    std::size_t idx = static_cast<std::size_t>(type);
    return idx < MODEL_TYPE_COUNT && m_modelsLoaded[idx];
}

// =============================================================================
// Frame Lifecycle
// =============================================================================

void VegetationRenderer::beginFrame() {
    m_stats.reset();

    for (auto& buffer : m_instanceBuffers) {
        if (buffer) {
            buffer->begin();
        }
    }
}

void VegetationRenderer::addChunkInstances(const render::ChunkInstances& chunk) {
    for (const auto& instance : chunk.instances) {
        addInstance(instance);
    }
}

void VegetationRenderer::addInstance(const render::VegetationInstance& instance) {
    std::size_t typeIdx = static_cast<std::size_t>(instance.model_type);
    if (typeIdx >= MODEL_TYPE_COUNT) {
        return;
    }

    if (!m_modelsLoaded[typeIdx] || !m_instanceBuffers[typeIdx]) {
        return;
    }

    // Build transform matrix from instance data
    glm::mat4 transform = buildTransformMatrix(instance);

    // Get tint color (white for vegetation, could be customized)
    glm::vec4 tintColor(1.0f);

    // Get emissive color based on model type
    glm::vec4 emissiveColor = getEmissiveColor(instance.model_type);

    // Add to instance buffer
    std::uint32_t index = m_instanceBuffers[typeIdx]->add(
        transform,
        tintColor,
        emissiveColor,
        0.0f  // No ambient override
    );

    if (index != UINT32_MAX) {
        m_stats.instancesPerType[typeIdx]++;
        m_stats.totalInstances++;
    }
}

glm::mat4 VegetationRenderer::buildTransformMatrix(const render::VegetationInstance& instance) {
    // Build transform: translate * rotateY * scale
    glm::mat4 transform = glm::mat4(1.0f);

    // Translation
    transform = glm::translate(transform, instance.position);

    // Y-axis rotation
    transform = glm::rotate(transform, instance.rotation_y, glm::vec3(0.0f, 1.0f, 0.0f));

    // Uniform scale
    transform = glm::scale(transform, glm::vec3(instance.scale));

    return transform;
}

glm::vec4 VegetationRenderer::getEmissiveColor(render::VegetationModelType type) const {
    // Get emissive color from terrain type info
    // Map vegetation model types to their corresponding terrain types
    terrain::TerrainType terrainType;

    switch (type) {
        case render::VegetationModelType::BiolumeTree:
            terrainType = terrain::TerrainType::BiolumeGrove;
            break;
        case render::VegetationModelType::CrystalSpire:
            terrainType = terrain::TerrainType::PrismaFields;
            break;
        case render::VegetationModelType::SporeEmitter:
            terrainType = terrain::TerrainType::SporeFlats;
            break;
        default:
            return glm::vec4(0.0f);
    }

    const terrain::TerrainTypeInfo& info = terrain::getTerrainInfo(terrainType);

    // RGB from emissive color, intensity in alpha
    return glm::vec4(
        info.emissive_color.x,
        info.emissive_color.y,
        info.emissive_color.z,
        info.emissive_intensity
    );
}

bool VegetationRenderer::uploadInstances(SDL_GPUCommandBuffer* cmdBuffer) {
    if (!cmdBuffer) {
        m_lastError = "VegetationRenderer::uploadInstances: command buffer is null";
        return false;
    }

    bool allSucceeded = true;

    for (std::size_t i = 0; i < MODEL_TYPE_COUNT; ++i) {
        if (!m_instanceBuffers[i] || m_instanceBuffers[i]->getInstanceCount() == 0) {
            continue;
        }

        if (!m_instanceBuffers[i]->end(cmdBuffer)) {
            SDL_LogWarn(SDL_LOG_CATEGORY_RENDER,
                "VegetationRenderer: failed to upload instances for type %zu: %s",
                i, m_instanceBuffers[i]->getLastError().c_str());
            allSucceeded = false;
        }
    }

    return allSucceeded;
}

// =============================================================================
// Rendering
// =============================================================================

std::uint32_t VegetationRenderer::render(
    SDL_GPURenderPass* renderPass,
    const ToonPipeline& pipeline,
    UniformBufferPool& uboPool,
    RenderPassState& state,
    RenderCommandStats* stats)
{
    if (!renderPass) {
        m_lastError = "VegetationRenderer::render: render pass is null";
        return 0;
    }

    // Check LOD - vegetation only renders at LOD 0
    if (!isVisibleAtCurrentLod()) {
        return 0;
    }

    std::uint32_t drawCalls = 0;

    for (std::size_t i = 0; i < MODEL_TYPE_COUNT; ++i) {
        if (!m_modelsLoaded[i] || !m_instanceBuffers[i]) {
            continue;
        }

        std::uint32_t instanceCount = m_instanceBuffers[i]->getInstanceCount();
        if (instanceCount == 0) {
            continue;
        }

        const ModelAsset& asset = m_models[i];
        if (!asset.isValid()) {
            continue;
        }

        // Bind instance buffer to storage slot 0
        m_instanceBuffers[i]->bind(renderPass, 0);

        // Render each mesh in the model
        for (std::size_t meshIdx = 0; meshIdx < asset.meshes.size(); ++meshIdx) {
            const GPUMesh& mesh = asset.meshes[meshIdx];
            if (!mesh.isValid()) {
                continue;
            }

            // Bind mesh buffers
            if (!RenderCommands::bindMeshBuffers(renderPass, mesh, state, stats)) {
                continue;
            }

            // Bind material textures
            const GPUMaterial* material = asset.getMeshMaterial(meshIdx);
            if (material) {
                RenderCommands::bindMaterialTextures(renderPass, *material, state, stats);
            }

            // Issue instanced draw call
            SDL_DrawGPUIndexedPrimitives(
                renderPass,
                mesh.indexCount,    // Indices per instance
                instanceCount,      // Number of instances
                0,                  // First index
                0,                  // Vertex offset
                0                   // First instance
            );

            drawCalls++;
            m_stats.drawCalls++;
            m_stats.triangles += (mesh.indexCount / 3) * instanceCount;

            if (stats) {
                stats->drawCalls++;
                stats->meshesDrawn += instanceCount;
                stats->trianglesDrawn += (mesh.indexCount / 3) * instanceCount;
            }
        }
    }

    return drawCalls;
}

} // namespace sims3000
