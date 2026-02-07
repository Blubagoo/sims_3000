/**
 * @file TerrainDebugOverlay.cpp
 * @brief Terrain debug overlay implementation.
 */

#include "sims3000/render/TerrainDebugOverlay.h"
#include "sims3000/render/GPUDevice.h"
#include "sims3000/render/ShaderCompiler.h"
#include "sims3000/render/CameraUniforms.h"
#include "sims3000/render/CameraState.h"
#include "sims3000/terrain/ITerrainRenderData.h"
#include "sims3000/terrain/ITerrainQueryable.h"
#include "sims3000/terrain/TerrainGrid.h"
#include "sims3000/terrain/TerrainTypes.h"
#include "sims3000/terrain/TerrainNormals.h"
#include "sims3000/terrain/WaterData.h"
#include <SDL3/SDL_log.h>
#include <algorithm>
#include <cmath>

namespace sims3000 {

// =============================================================================
// Construction / Destruction
// =============================================================================

TerrainDebugOverlay::TerrainDebugOverlay(GPUDevice& device, SDL_GPUTextureFormat colorFormat)
    : m_device(&device)
    , m_colorFormat(colorFormat)
{
    if (!createResources()) {
        SDL_LogError(SDL_LOG_CATEGORY_GPU,
                     "TerrainDebugOverlay: Failed to create resources: %s",
                     m_lastError.c_str());
    }
}

TerrainDebugOverlay::~TerrainDebugOverlay() {
    releaseResources();
}

TerrainDebugOverlay::TerrainDebugOverlay(TerrainDebugOverlay&& other) noexcept
    : m_device(other.m_device)
    , m_colorFormat(other.m_colorFormat)
    , m_config(other.m_config)
    , m_activeModeMask(other.m_activeModeMask)
    , m_terrainRenderData(other.m_terrainRenderData)
    , m_terrainQueryable(other.m_terrainQueryable)
    , m_chunkLODLevels(std::move(other.m_chunkLODLevels))
    , m_chunksX(other.m_chunksX)
    , m_chunksY(other.m_chunksY)
    , m_mapWidth(other.m_mapWidth)
    , m_mapHeight(other.m_mapHeight)
    , m_pipeline(other.m_pipeline)
    , m_vertexShader(other.m_vertexShader)
    , m_fragmentShader(other.m_fragmentShader)
    , m_sampler(other.m_sampler)
    , m_dataTexture(other.m_dataTexture)
    , m_dataTextureDirty(other.m_dataTextureDirty)
    , m_normalsTexture(other.m_normalsTexture)
    , m_normalsTextureDirty(other.m_normalsTextureDirty)
    , m_lastError(std::move(other.m_lastError))
{
    other.m_device = nullptr;
    other.m_pipeline = nullptr;
    other.m_vertexShader = nullptr;
    other.m_fragmentShader = nullptr;
    other.m_sampler = nullptr;
    other.m_dataTexture = nullptr;
    other.m_normalsTexture = nullptr;
}

TerrainDebugOverlay& TerrainDebugOverlay::operator=(TerrainDebugOverlay&& other) noexcept {
    if (this != &other) {
        releaseResources();

        m_device = other.m_device;
        m_colorFormat = other.m_colorFormat;
        m_config = other.m_config;
        m_activeModeMask = other.m_activeModeMask;
        m_terrainRenderData = other.m_terrainRenderData;
        m_terrainQueryable = other.m_terrainQueryable;
        m_chunkLODLevels = std::move(other.m_chunkLODLevels);
        m_chunksX = other.m_chunksX;
        m_chunksY = other.m_chunksY;
        m_mapWidth = other.m_mapWidth;
        m_mapHeight = other.m_mapHeight;
        m_pipeline = other.m_pipeline;
        m_vertexShader = other.m_vertexShader;
        m_fragmentShader = other.m_fragmentShader;
        m_sampler = other.m_sampler;
        m_dataTexture = other.m_dataTexture;
        m_dataTextureDirty = other.m_dataTextureDirty;
        m_normalsTexture = other.m_normalsTexture;
        m_normalsTextureDirty = other.m_normalsTextureDirty;
        m_lastError = std::move(other.m_lastError);

        other.m_device = nullptr;
        other.m_pipeline = nullptr;
        other.m_vertexShader = nullptr;
        other.m_fragmentShader = nullptr;
        other.m_sampler = nullptr;
        other.m_dataTexture = nullptr;
        other.m_normalsTexture = nullptr;
    }
    return *this;
}

// =============================================================================
// Public Interface
// =============================================================================

bool TerrainDebugOverlay::isValid() const {
    return m_device != nullptr && m_pipeline != nullptr;
}

void TerrainDebugOverlay::setModeEnabled(TerrainDebugMode mode, bool enabled) {
    std::uint32_t bit = 1u << static_cast<std::uint32_t>(mode);
    if (enabled) {
        m_activeModeMask |= bit;
    } else {
        m_activeModeMask &= ~bit;
    }
    SDL_LogInfo(SDL_LOG_CATEGORY_GPU, "TerrainDebugOverlay: %s %s",
                getDebugModeName(mode), enabled ? "enabled" : "disabled");
}

void TerrainDebugOverlay::toggleMode(TerrainDebugMode mode) {
    setModeEnabled(mode, !isModeEnabled(mode));
}

bool TerrainDebugOverlay::isModeEnabled(TerrainDebugMode mode) const {
    std::uint32_t bit = 1u << static_cast<std::uint32_t>(mode);
    return (m_activeModeMask & bit) != 0;
}

bool TerrainDebugOverlay::hasActiveMode() const {
    return m_activeModeMask != 0;
}

void TerrainDebugOverlay::disableAllModes() {
    m_activeModeMask = 0;
    SDL_LogInfo(SDL_LOG_CATEGORY_GPU, "TerrainDebugOverlay: All modes disabled");
}

void TerrainDebugOverlay::setTerrainRenderData(const terrain::ITerrainRenderData* data) {
    m_terrainRenderData = data;
    m_dataTextureDirty = true;
    m_normalsTextureDirty = true;

    if (data != nullptr) {
        m_mapWidth = data->get_map_width();
        m_mapHeight = data->get_map_height();
        m_chunksX = data->get_chunks_x();
        m_chunksY = data->get_chunks_y();
    }
}

void TerrainDebugOverlay::setTerrainQueryable(const terrain::ITerrainQueryable* queryable) {
    m_terrainQueryable = queryable;
    m_dataTextureDirty = true;
}

void TerrainDebugOverlay::setChunkLODLevels(const std::vector<ChunkLODInfo>& lodLevels,
                                             std::uint32_t chunksX, std::uint32_t chunksY) {
    m_chunkLODLevels = lodLevels;
    m_chunksX = chunksX;
    m_chunksY = chunksY;
    m_dataTextureDirty = true;
}

void TerrainDebugOverlay::setConfig(const TerrainDebugConfig& config) {
    m_config = config;
}

void TerrainDebugOverlay::setOpacity(float opacity) {
    m_config.overlayOpacity = std::clamp(opacity, 0.0f, 1.0f);
}

bool TerrainDebugOverlay::render(
    SDL_GPUCommandBuffer* cmdBuffer,
    SDL_GPUTexture* outputTexture,
    std::uint32_t width,
    std::uint32_t height,
    const CameraUniforms& camera,
    const CameraState& state)
{
    if (!isValid()) {
        m_lastError = "TerrainDebugOverlay not valid";
        return false;
    }

    // Skip if no modes are active
    if (!hasActiveMode()) {
        return true;
    }

    if (!cmdBuffer || !outputTexture) {
        m_lastError = "Invalid input parameters";
        return false;
    }

    // Update data texture if terrain data changed
    if (m_dataTextureDirty) {
        if (!updateDataTexture()) {
            SDL_LogWarn(SDL_LOG_CATEGORY_GPU,
                        "TerrainDebugOverlay: Failed to update data texture");
            // Continue rendering with stale data
        }
    }

    // Update normals texture if terrain data changed
    if (m_normalsTextureDirty) {
        if (!updateNormalsTexture()) {
            SDL_LogWarn(SDL_LOG_CATEGORY_GPU,
                        "TerrainDebugOverlay: Failed to update normals texture");
            // Continue rendering with stale data
        }
    }

    // Prepare uniform buffer data
    TerrainDebugUBO uboData;
    uboData.viewProjection = camera.getViewProjectionMatrix();
    uboData.elevationColorLow = m_config.elevationColorLow;
    uboData.elevationColorMid = m_config.elevationColorMid;
    uboData.elevationColorHigh = m_config.elevationColorHigh;
    uboData.buildableColor = m_config.buildableColor;
    uboData.unbuildableColor = m_config.unbuildableColor;
    uboData.chunkBoundaryColor = m_config.chunkBoundaryColor;
    uboData.mapSize = glm::vec2(static_cast<float>(m_mapWidth),
                                 static_cast<float>(m_mapHeight));
    uboData.chunkSize = 32.0f;  // TERRAIN_CHUNK_SIZE
    uboData.lineThickness = m_config.chunkLineThickness;
    uboData.opacity = m_config.overlayOpacity;
    uboData.activeModeMask = m_activeModeMask;
    uboData.cameraDistance = state.distance;
    uboData._padding = 0.0f;

    // Configure color target - load existing content for overlay
    SDL_GPUColorTargetInfo colorTarget = {};
    colorTarget.texture = outputTexture;
    colorTarget.load_op = SDL_GPU_LOADOP_LOAD;  // Preserve scene content
    colorTarget.store_op = SDL_GPU_STOREOP_STORE;

    // Begin debug overlay render pass
    SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(cmdBuffer, &colorTarget, 1, nullptr);
    if (!renderPass) {
        m_lastError = std::string("Failed to begin debug render pass: ") + SDL_GetError();
        return false;
    }

    // Bind pipeline
    SDL_BindGPUGraphicsPipeline(renderPass, m_pipeline);

    // Push uniform buffer data
    SDL_PushGPUVertexUniformData(cmdBuffer, 0, &uboData, sizeof(uboData));

    // Bind data texture and normals texture with samplers
    if (m_dataTexture != nullptr && m_sampler != nullptr) {
        SDL_GPUTextureSamplerBinding bindings[2] = {};
        // Binding 0: Data texture (elevation, terrain type, flags, LOD)
        bindings[0].texture = m_dataTexture;
        bindings[0].sampler = m_sampler;
        // Binding 1: Normals texture (RGB encoded normals)
        bindings[1].texture = m_normalsTexture ? m_normalsTexture : m_dataTexture;  // Fallback to data if normals not available
        bindings[1].sampler = m_sampler;
        SDL_BindGPUFragmentSamplers(renderPass, 0, bindings, 2);
    }

    // Set viewport
    SDL_GPUViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.w = static_cast<float>(width);
    viewport.h = static_cast<float>(height);
    viewport.min_depth = 0.0f;
    viewport.max_depth = 1.0f;
    SDL_SetGPUViewport(renderPass, &viewport);

    // Draw fullscreen quad as 6 vertices (2 triangles)
    SDL_DrawGPUPrimitives(renderPass, 6, 1, 0, 0);

    // End render pass
    SDL_EndGPURenderPass(renderPass);

    return true;
}

// =============================================================================
// Private Implementation
// =============================================================================

bool TerrainDebugOverlay::createResources() {
    if (!m_device || !m_device->isValid()) {
        m_lastError = "Invalid GPU device";
        return false;
    }

    SDL_GPUDevice* device = m_device->getHandle();
    if (!device) {
        m_lastError = "GPU device handle is null";
        return false;
    }

    // Load shaders
    if (!loadShaders()) {
        return false;
    }

    // Create sampler for data texture lookup
    {
        SDL_GPUSamplerCreateInfo samplerInfo = {};
        samplerInfo.min_filter = SDL_GPU_FILTER_NEAREST;
        samplerInfo.mag_filter = SDL_GPU_FILTER_NEAREST;
        samplerInfo.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
        samplerInfo.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        samplerInfo.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        samplerInfo.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;

        m_sampler = SDL_CreateGPUSampler(device, &samplerInfo);
        if (!m_sampler) {
            m_lastError = std::string("Failed to create sampler: ") + SDL_GetError();
            releaseResources();
            return false;
        }
    }

    // Create graphics pipeline for debug overlay rendering
    {
        // Color target description with alpha blending for overlay
        SDL_GPUColorTargetDescription colorTargetDesc = {};
        colorTargetDesc.format = m_colorFormat;
        colorTargetDesc.blend_state.enable_blend = true;
        colorTargetDesc.blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
        colorTargetDesc.blend_state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
        colorTargetDesc.blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD;
        colorTargetDesc.blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
        colorTargetDesc.blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
        colorTargetDesc.blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
        colorTargetDesc.blend_state.color_write_mask = 0xF;  // Write all channels

        // Pipeline creation info
        SDL_GPUGraphicsPipelineCreateInfo pipelineInfo = {};
        pipelineInfo.vertex_shader = m_vertexShader;
        pipelineInfo.fragment_shader = m_fragmentShader;

        // No vertex input - quad generated procedurally in shader
        pipelineInfo.vertex_input_state.num_vertex_buffers = 0;
        pipelineInfo.vertex_input_state.num_vertex_attributes = 0;

        // Primitive state
        pipelineInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;

        // Rasterizer state
        pipelineInfo.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
        pipelineInfo.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
        pipelineInfo.rasterizer_state.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;

        // No depth testing - overlay renders on top
        pipelineInfo.depth_stencil_state.enable_depth_test = false;
        pipelineInfo.depth_stencil_state.enable_depth_write = false;
        pipelineInfo.depth_stencil_state.enable_stencil_test = false;

        // Color targets
        pipelineInfo.target_info.num_color_targets = 1;
        pipelineInfo.target_info.color_target_descriptions = &colorTargetDesc;
        pipelineInfo.target_info.has_depth_stencil_target = false;

        m_pipeline = SDL_CreateGPUGraphicsPipeline(device, &pipelineInfo);
        if (!m_pipeline) {
            m_lastError = std::string("Failed to create pipeline: ") + SDL_GetError();
            releaseResources();
            return false;
        }
    }

    SDL_LogInfo(SDL_LOG_CATEGORY_GPU, "TerrainDebugOverlay: Created resources successfully");
    return true;
}

void TerrainDebugOverlay::releaseResources() {
    if (!m_device || !m_device->isValid()) {
        return;
    }

    SDL_GPUDevice* device = m_device->getHandle();
    if (!device) {
        return;
    }

    if (m_dataTexture) {
        SDL_ReleaseGPUTexture(device, m_dataTexture);
        m_dataTexture = nullptr;
    }

    if (m_normalsTexture) {
        SDL_ReleaseGPUTexture(device, m_normalsTexture);
        m_normalsTexture = nullptr;
    }

    if (m_sampler) {
        SDL_ReleaseGPUSampler(device, m_sampler);
        m_sampler = nullptr;
    }

    if (m_pipeline) {
        SDL_ReleaseGPUGraphicsPipeline(device, m_pipeline);
        m_pipeline = nullptr;
    }

    if (m_vertexShader) {
        SDL_ReleaseGPUShader(device, m_vertexShader);
        m_vertexShader = nullptr;
    }

    if (m_fragmentShader) {
        SDL_ReleaseGPUShader(device, m_fragmentShader);
        m_fragmentShader = nullptr;
    }
}

bool TerrainDebugOverlay::loadShaders() {
    ShaderCompiler compiler(*m_device);

    // Load debug vertex shader
    ShaderResources vertResources;
    vertResources.numUniformBuffers = 1;  // TerrainDebugUBO
    vertResources.numSamplers = 0;
    vertResources.numStorageBuffers = 0;
    vertResources.numStorageTextures = 0;

    ShaderLoadResult vertResult = compiler.loadShader(
        "shaders/terrain_debug.vert",
        ShaderStage::Vertex,
        "main",
        vertResources
    );

    if (!vertResult.isValid()) {
        m_lastError = "Failed to load terrain debug vertex shader: " + vertResult.error.message;
        return false;
    }
    m_vertexShader = vertResult.shader;

    if (vertResult.usedFallback) {
        SDL_LogWarn(SDL_LOG_CATEGORY_GPU,
                    "TerrainDebugOverlay: Using fallback vertex shader");
    }

    // Load debug fragment shader
    ShaderResources fragResources;
    fragResources.numUniformBuffers = 0;  // Fragment uniforms passed via vertex
    fragResources.numSamplers = 2;        // Data texture + normals texture samplers
    fragResources.numStorageBuffers = 0;
    fragResources.numStorageTextures = 0;

    ShaderLoadResult fragResult = compiler.loadShader(
        "shaders/terrain_debug.frag",
        ShaderStage::Fragment,
        "main",
        fragResources
    );

    if (!fragResult.isValid()) {
        m_lastError = "Failed to load terrain debug fragment shader: " + fragResult.error.message;
        // Release vertex shader on failure
        if (m_vertexShader && m_device) {
            SDL_ReleaseGPUShader(m_device->getHandle(), m_vertexShader);
            m_vertexShader = nullptr;
        }
        return false;
    }
    m_fragmentShader = fragResult.shader;

    if (fragResult.usedFallback) {
        SDL_LogWarn(SDL_LOG_CATEGORY_GPU,
                    "TerrainDebugOverlay: Using fallback fragment shader");
    }

    SDL_LogInfo(SDL_LOG_CATEGORY_GPU, "TerrainDebugOverlay: Shaders loaded successfully");
    return true;
}

bool TerrainDebugOverlay::createDataTexture() {
    if (!m_device || !m_device->isValid()) {
        return false;
    }

    SDL_GPUDevice* device = m_device->getHandle();

    // Release old texture if exists
    if (m_dataTexture) {
        SDL_ReleaseGPUTexture(device, m_dataTexture);
        m_dataTexture = nullptr;
    }

    // Create RGBA8 texture for terrain data lookup
    // R = elevation (0-31), G = terrain type (0-9), B = flags, A = LOD/water body
    SDL_GPUTextureCreateInfo texInfo = {};
    texInfo.type = SDL_GPU_TEXTURETYPE_2D;
    texInfo.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    texInfo.width = m_mapWidth;
    texInfo.height = m_mapHeight;
    texInfo.layer_count_or_depth = 1;
    texInfo.num_levels = 1;
    texInfo.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;

    m_dataTexture = SDL_CreateGPUTexture(device, &texInfo);
    if (!m_dataTexture) {
        m_lastError = std::string("Failed to create data texture: ") + SDL_GetError();
        return false;
    }

    return true;
}

bool TerrainDebugOverlay::updateDataTexture() {
    if (!m_terrainRenderData) {
        m_dataTextureDirty = false;
        return true;  // No data to update - not an error
    }

    // Get map dimensions from terrain data
    m_mapWidth = m_terrainRenderData->get_map_width();
    m_mapHeight = m_terrainRenderData->get_map_height();

    // Create texture if needed or if dimensions changed
    if (!m_dataTexture) {
        if (!createDataTexture()) {
            return false;
        }
    }

    // Prepare pixel data
    std::vector<std::uint8_t> pixelData(m_mapWidth * m_mapHeight * 4);

    const terrain::TerrainGrid& grid = m_terrainRenderData->get_grid();

    // Fill pixel data from terrain grid
    for (std::uint32_t y = 0; y < m_mapHeight; ++y) {
        for (std::uint32_t x = 0; x < m_mapWidth; ++x) {
            std::size_t pixelIndex = (y * m_mapWidth + x) * 4;
            const terrain::TerrainComponent& tile = grid.at(x, y);

            // R = elevation (normalized to 0-255, max elevation is 31)
            pixelData[pixelIndex + 0] = static_cast<std::uint8_t>(
                std::min(255u, tile.getElevation() * 8u));

            // G = terrain type (0-9 scaled to visible range)
            pixelData[pixelIndex + 1] = static_cast<std::uint8_t>(
                tile.terrain_type * 25);

            // B = flags (buildability in bit 0)
            std::uint8_t flags = 0;
            if (m_terrainQueryable) {
                if (m_terrainQueryable->is_buildable(static_cast<std::int32_t>(x),
                                                      static_cast<std::int32_t>(y))) {
                    flags |= 0x01;  // Buildable flag
                }
            }
            // Add water body indicator
            terrain::WaterBodyID bodyId = m_terrainRenderData->get_water_body_id(
                static_cast<std::int32_t>(x), static_cast<std::int32_t>(y));
            if (bodyId != terrain::NO_WATER_BODY) {
                flags |= 0x02;  // Water body flag
            }
            pixelData[pixelIndex + 2] = flags;

            // A = LOD level for the chunk containing this tile
            std::uint8_t lodLevel = 0;
            if (!m_chunkLODLevels.empty() && m_chunksX > 0) {
                std::uint32_t chunkX = x / 32;
                std::uint32_t chunkY = y / 32;
                std::size_t chunkIndex = chunkY * m_chunksX + chunkX;
                if (chunkIndex < m_chunkLODLevels.size()) {
                    lodLevel = m_chunkLODLevels[chunkIndex].lodLevel;
                }
            }
            // Also encode water body ID in upper bits for coloring
            std::uint8_t waterBodyLower = static_cast<std::uint8_t>(bodyId & 0xFF);
            pixelData[pixelIndex + 3] = static_cast<std::uint8_t>(
                (lodLevel & 0x03) | ((waterBodyLower & 0x3F) << 2));
        }
    }

    // Upload to GPU texture
    SDL_GPUDevice* device = m_device->getHandle();

    // Create transfer buffer
    SDL_GPUTransferBufferCreateInfo transferInfo = {};
    transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transferInfo.size = static_cast<Uint32>(pixelData.size());

    SDL_GPUTransferBuffer* transferBuffer = SDL_CreateGPUTransferBuffer(device, &transferInfo);
    if (!transferBuffer) {
        m_lastError = std::string("Failed to create transfer buffer: ") + SDL_GetError();
        return false;
    }

    // Map and copy data
    void* mapped = SDL_MapGPUTransferBuffer(device, transferBuffer, false);
    if (!mapped) {
        SDL_ReleaseGPUTransferBuffer(device, transferBuffer);
        m_lastError = std::string("Failed to map transfer buffer: ") + SDL_GetError();
        return false;
    }
    std::memcpy(mapped, pixelData.data(), pixelData.size());
    SDL_UnmapGPUTransferBuffer(device, transferBuffer);

    // Create command buffer for upload
    SDL_GPUCommandBuffer* cmdBuffer = SDL_AcquireGPUCommandBuffer(device);
    if (!cmdBuffer) {
        SDL_ReleaseGPUTransferBuffer(device, transferBuffer);
        m_lastError = std::string("Failed to acquire command buffer: ") + SDL_GetError();
        return false;
    }

    // Begin copy pass
    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(cmdBuffer);
    if (!copyPass) {
        SDL_ReleaseGPUTransferBuffer(device, transferBuffer);
        m_lastError = std::string("Failed to begin copy pass: ") + SDL_GetError();
        return false;
    }

    // Upload texture data
    SDL_GPUTextureTransferInfo srcInfo = {};
    srcInfo.transfer_buffer = transferBuffer;
    srcInfo.offset = 0;

    SDL_GPUTextureRegion dstRegion = {};
    dstRegion.texture = m_dataTexture;
    dstRegion.w = m_mapWidth;
    dstRegion.h = m_mapHeight;
    dstRegion.d = 1;

    SDL_UploadToGPUTexture(copyPass, &srcInfo, &dstRegion, false);

    SDL_EndGPUCopyPass(copyPass);
    SDL_SubmitGPUCommandBuffer(cmdBuffer);

    // Release transfer buffer
    SDL_ReleaseGPUTransferBuffer(device, transferBuffer);

    m_dataTextureDirty = false;
    SDL_LogDebug(SDL_LOG_CATEGORY_GPU,
                 "TerrainDebugOverlay: Updated data texture (%ux%u)",
                 m_mapWidth, m_mapHeight);

    return true;
}

bool TerrainDebugOverlay::createNormalsTexture() {
    if (!m_device || !m_device->isValid()) {
        return false;
    }

    SDL_GPUDevice* device = m_device->getHandle();

    // Release old texture if exists
    if (m_normalsTexture) {
        SDL_ReleaseGPUTexture(device, m_normalsTexture);
        m_normalsTexture = nullptr;
    }

    // Create RGBA8 texture for terrain normals lookup
    // R = nx * 0.5 + 0.5, G = ny * 0.5 + 0.5, B = nz * 0.5 + 0.5, A = reserved
    SDL_GPUTextureCreateInfo texInfo = {};
    texInfo.type = SDL_GPU_TEXTURETYPE_2D;
    texInfo.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    texInfo.width = m_mapWidth;
    texInfo.height = m_mapHeight;
    texInfo.layer_count_or_depth = 1;
    texInfo.num_levels = 1;
    texInfo.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;

    m_normalsTexture = SDL_CreateGPUTexture(device, &texInfo);
    if (!m_normalsTexture) {
        m_lastError = std::string("Failed to create normals texture: ") + SDL_GetError();
        return false;
    }

    return true;
}

bool TerrainDebugOverlay::updateNormalsTexture() {
    if (!m_terrainRenderData) {
        m_normalsTextureDirty = false;
        return true;  // No data to update - not an error
    }

    // Get map dimensions from terrain data
    m_mapWidth = m_terrainRenderData->get_map_width();
    m_mapHeight = m_terrainRenderData->get_map_height();

    // Create texture if needed or if dimensions changed
    if (!m_normalsTexture) {
        if (!createNormalsTexture()) {
            return false;
        }
    }

    // Prepare pixel data for normals
    std::vector<std::uint8_t> pixelData(m_mapWidth * m_mapHeight * 4);

    const terrain::TerrainGrid& grid = m_terrainRenderData->get_grid();

    // Fill pixel data with computed normals from terrain grid
    // Using RGB encoding: R = nx * 0.5 + 0.5, G = ny * 0.5 + 0.5, B = nz * 0.5 + 0.5
    for (std::uint32_t y = 0; y < m_mapHeight; ++y) {
        for (std::uint32_t x = 0; x < m_mapWidth; ++x) {
            std::size_t pixelIndex = (y * m_mapWidth + x) * 4;

            // Compute the terrain normal at this position
            terrain::NormalResult normal = terrain::computeTerrainNormal(
                grid,
                static_cast<std::int32_t>(x),
                static_cast<std::int32_t>(y));

            // Encode normal components to 0-255 range
            // normal components are in range [-1, 1], we map to [0, 255]
            // Formula: (n * 0.5 + 0.5) * 255
            pixelData[pixelIndex + 0] = static_cast<std::uint8_t>(
                std::clamp((normal.nx * 0.5f + 0.5f) * 255.0f, 0.0f, 255.0f));
            pixelData[pixelIndex + 1] = static_cast<std::uint8_t>(
                std::clamp((normal.ny * 0.5f + 0.5f) * 255.0f, 0.0f, 255.0f));
            pixelData[pixelIndex + 2] = static_cast<std::uint8_t>(
                std::clamp((normal.nz * 0.5f + 0.5f) * 255.0f, 0.0f, 255.0f));
            pixelData[pixelIndex + 3] = 255;  // Reserved, set to opaque
        }
    }

    // Upload to GPU texture
    SDL_GPUDevice* device = m_device->getHandle();

    // Create transfer buffer
    SDL_GPUTransferBufferCreateInfo transferInfo = {};
    transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transferInfo.size = static_cast<Uint32>(pixelData.size());

    SDL_GPUTransferBuffer* transferBuffer = SDL_CreateGPUTransferBuffer(device, &transferInfo);
    if (!transferBuffer) {
        m_lastError = std::string("Failed to create transfer buffer for normals: ") + SDL_GetError();
        return false;
    }

    // Map and copy data
    void* mapped = SDL_MapGPUTransferBuffer(device, transferBuffer, false);
    if (!mapped) {
        SDL_ReleaseGPUTransferBuffer(device, transferBuffer);
        m_lastError = std::string("Failed to map transfer buffer for normals: ") + SDL_GetError();
        return false;
    }
    std::memcpy(mapped, pixelData.data(), pixelData.size());
    SDL_UnmapGPUTransferBuffer(device, transferBuffer);

    // Create command buffer for upload
    SDL_GPUCommandBuffer* cmdBuffer = SDL_AcquireGPUCommandBuffer(device);
    if (!cmdBuffer) {
        SDL_ReleaseGPUTransferBuffer(device, transferBuffer);
        m_lastError = std::string("Failed to acquire command buffer for normals: ") + SDL_GetError();
        return false;
    }

    // Begin copy pass
    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(cmdBuffer);
    if (!copyPass) {
        SDL_ReleaseGPUTransferBuffer(device, transferBuffer);
        m_lastError = std::string("Failed to begin copy pass for normals: ") + SDL_GetError();
        return false;
    }

    // Upload texture data
    SDL_GPUTextureTransferInfo srcInfo = {};
    srcInfo.transfer_buffer = transferBuffer;
    srcInfo.offset = 0;

    SDL_GPUTextureRegion dstRegion = {};
    dstRegion.texture = m_normalsTexture;
    dstRegion.w = m_mapWidth;
    dstRegion.h = m_mapHeight;
    dstRegion.d = 1;

    SDL_UploadToGPUTexture(copyPass, &srcInfo, &dstRegion, false);

    SDL_EndGPUCopyPass(copyPass);
    SDL_SubmitGPUCommandBuffer(cmdBuffer);

    // Release transfer buffer
    SDL_ReleaseGPUTransferBuffer(device, transferBuffer);

    m_normalsTextureDirty = false;
    SDL_LogDebug(SDL_LOG_CATEGORY_GPU,
                 "TerrainDebugOverlay: Updated normals texture (%ux%u)",
                 m_mapWidth, m_mapHeight);

    return true;
}

} // namespace sims3000
