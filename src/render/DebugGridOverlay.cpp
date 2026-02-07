/**
 * @file DebugGridOverlay.cpp
 * @brief Debug grid overlay implementation.
 */

#include "sims3000/render/DebugGridOverlay.h"
#include "sims3000/render/GPUDevice.h"
#include "sims3000/render/ShaderCompiler.h"
#include "sims3000/render/CameraUniforms.h"
#include "sims3000/render/CameraState.h"
#include <SDL3/SDL_log.h>
#include <algorithm>
#include <cmath>

namespace sims3000 {

// =============================================================================
// Construction / Destruction
// =============================================================================

DebugGridOverlay::DebugGridOverlay(GPUDevice& device, SDL_GPUTextureFormat colorFormat)
    : m_device(&device)
    , m_colorFormat(colorFormat)
{
    if (!createResources()) {
        SDL_LogError(SDL_LOG_CATEGORY_GPU,
                     "DebugGridOverlay: Failed to create resources: %s",
                     m_lastError.c_str());
    }
}

DebugGridOverlay::~DebugGridOverlay() {
    releaseResources();
}

DebugGridOverlay::DebugGridOverlay(DebugGridOverlay&& other) noexcept
    : m_device(other.m_device)
    , m_colorFormat(other.m_colorFormat)
    , m_config(other.m_config)
    , m_mapWidth(other.m_mapWidth)
    , m_mapHeight(other.m_mapHeight)
    , m_enabled(other.m_enabled)
    , m_pipeline(other.m_pipeline)
    , m_vertexShader(other.m_vertexShader)
    , m_fragmentShader(other.m_fragmentShader)
    , m_lastError(std::move(other.m_lastError))
{
    other.m_device = nullptr;
    other.m_pipeline = nullptr;
    other.m_vertexShader = nullptr;
    other.m_fragmentShader = nullptr;
}

DebugGridOverlay& DebugGridOverlay::operator=(DebugGridOverlay&& other) noexcept {
    if (this != &other) {
        releaseResources();

        m_device = other.m_device;
        m_colorFormat = other.m_colorFormat;
        m_config = other.m_config;
        m_mapWidth = other.m_mapWidth;
        m_mapHeight = other.m_mapHeight;
        m_enabled = other.m_enabled;
        m_pipeline = other.m_pipeline;
        m_vertexShader = other.m_vertexShader;
        m_fragmentShader = other.m_fragmentShader;
        m_lastError = std::move(other.m_lastError);

        other.m_device = nullptr;
        other.m_pipeline = nullptr;
        other.m_vertexShader = nullptr;
        other.m_fragmentShader = nullptr;
    }
    return *this;
}

// =============================================================================
// Public Interface
// =============================================================================

bool DebugGridOverlay::isValid() const {
    return m_device != nullptr && m_pipeline != nullptr;
}

void DebugGridOverlay::setEnabled(bool enabled) {
    m_enabled = enabled;
    SDL_LogInfo(SDL_LOG_CATEGORY_GPU, "DebugGridOverlay: %s",
                enabled ? "Enabled" : "Disabled");
}

void DebugGridOverlay::toggle() {
    setEnabled(!m_enabled);
}

void DebugGridOverlay::setMapSize(std::uint32_t width, std::uint32_t height) {
    m_mapWidth = width;
    m_mapHeight = height;
    SDL_LogInfo(SDL_LOG_CATEGORY_GPU, "DebugGridOverlay: Map size set to %ux%u",
                width, height);
}

void DebugGridOverlay::setConfig(const DebugGridConfig& config) {
    m_config = config;
}

void DebugGridOverlay::setFineGridColor(const glm::vec4& color) {
    m_config.fineGridColor = color;
}

void DebugGridOverlay::setCoarseGridColor(const glm::vec4& color) {
    m_config.coarseGridColor = color;
}

void DebugGridOverlay::setLineThickness(float thickness) {
    m_config.lineThickness = std::max(0.5f, thickness);
}

bool DebugGridOverlay::render(
    SDL_GPUCommandBuffer* cmdBuffer,
    SDL_GPUTexture* outputTexture,
    std::uint32_t width,
    std::uint32_t height,
    const CameraUniforms& camera,
    const CameraState& state)
{
    if (!isValid()) {
        m_lastError = "DebugGridOverlay not valid";
        return false;
    }

    if (!m_enabled) {
        return true;  // Not an error, just disabled
    }

    if (!cmdBuffer || !outputTexture) {
        m_lastError = "Invalid input parameters";
        return false;
    }

    // Calculate opacity based on camera pitch
    float opacity = calculateTiltOpacity(state.pitch);

    // Skip rendering if opacity is too low
    if (opacity < 0.01f) {
        return true;
    }

    // Prepare uniform buffer data
    DebugGridUBO uboData;
    uboData.viewProjection = camera.getViewProjectionMatrix();
    uboData.fineGridColor = m_config.fineGridColor;
    uboData.coarseGridColor = m_config.coarseGridColor;
    uboData.mapSize = glm::vec2(static_cast<float>(m_mapWidth),
                                 static_cast<float>(m_mapHeight));
    uboData.fineGridSpacing = static_cast<float>(m_config.fineGridSpacing);
    uboData.coarseGridSpacing = static_cast<float>(m_config.coarseGridSpacing);
    uboData.lineThickness = m_config.lineThickness;
    uboData.opacity = opacity;
    uboData.cameraDistance = state.distance;
    uboData._padding = 0.0f;

    // Configure color target - load existing content for overlay
    SDL_GPUColorTargetInfo colorTarget = {};
    colorTarget.texture = outputTexture;
    colorTarget.load_op = SDL_GPU_LOADOP_LOAD;  // Preserve scene content
    colorTarget.store_op = SDL_GPU_STOREOP_STORE;

    // Begin grid render pass (no depth target - always on top)
    SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(cmdBuffer, &colorTarget, 1, nullptr);
    if (!renderPass) {
        m_lastError = std::string("Failed to begin grid render pass: ") + SDL_GetError();
        return false;
    }

    // Bind pipeline
    SDL_BindGPUGraphicsPipeline(renderPass, m_pipeline);

    // Push uniform buffer data
    SDL_PushGPUVertexUniformData(cmdBuffer, 0, &uboData, sizeof(uboData));

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
    // The vertex shader generates a grid covering the entire map
    SDL_DrawGPUPrimitives(renderPass, 6, 1, 0, 0);

    // End render pass
    SDL_EndGPURenderPass(renderPass);

    return true;
}

// =============================================================================
// Private Implementation
// =============================================================================

float DebugGridOverlay::calculateTiltOpacity(float pitchDegrees) const {
    // At low pitch angles (looking more top-down), full opacity
    // At high pitch angles (looking more from the side), reduce opacity to prevent clutter

    if (pitchDegrees <= m_config.minPitchForFullOpacity) {
        return 1.0f;
    }

    if (pitchDegrees >= m_config.maxPitchForFade) {
        return m_config.minOpacityAtExtremeTilt;
    }

    // Linear interpolation between full opacity and minimum opacity
    float t = (pitchDegrees - m_config.minPitchForFullOpacity) /
              (m_config.maxPitchForFade - m_config.minPitchForFullOpacity);
    return 1.0f - t * (1.0f - m_config.minOpacityAtExtremeTilt);
}

bool DebugGridOverlay::createResources() {
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

    // Create graphics pipeline for grid rendering
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

        // No vertex input - grid generated procedurally in shader
        pipelineInfo.vertex_input_state.num_vertex_buffers = 0;
        pipelineInfo.vertex_input_state.num_vertex_attributes = 0;

        // Primitive state
        pipelineInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;

        // Rasterizer state
        pipelineInfo.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
        pipelineInfo.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;  // No culling for fullscreen
        pipelineInfo.rasterizer_state.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;

        // No depth testing - grid renders on top of everything
        pipelineInfo.depth_stencil_state.enable_depth_test = false;
        pipelineInfo.depth_stencil_state.enable_depth_write = false;
        pipelineInfo.depth_stencil_state.enable_stencil_test = false;

        // Color targets
        pipelineInfo.target_info.num_color_targets = 1;
        pipelineInfo.target_info.color_target_descriptions = &colorTargetDesc;
        pipelineInfo.target_info.has_depth_stencil_target = false;

        m_pipeline = SDL_CreateGPUGraphicsPipeline(device, &pipelineInfo);
        if (!m_pipeline) {
            m_lastError = std::string("Failed to create grid pipeline: ") + SDL_GetError();
            releaseResources();
            return false;
        }
    }

    SDL_LogInfo(SDL_LOG_CATEGORY_GPU, "DebugGridOverlay: Created resources successfully");
    return true;
}

void DebugGridOverlay::releaseResources() {
    if (!m_device || !m_device->isValid()) {
        return;
    }

    SDL_GPUDevice* device = m_device->getHandle();
    if (!device) {
        return;
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

bool DebugGridOverlay::loadShaders() {
    ShaderCompiler compiler(*m_device);

    // Load grid vertex shader
    ShaderResources vertResources;
    vertResources.numUniformBuffers = 1;  // DebugGridUBO
    vertResources.numSamplers = 0;
    vertResources.numStorageBuffers = 0;
    vertResources.numStorageTextures = 0;

    ShaderLoadResult vertResult = compiler.loadShader(
        "shaders/debug_grid.vert",
        ShaderStage::Vertex,
        "main",
        vertResources
    );

    if (!vertResult.isValid()) {
        m_lastError = "Failed to load debug grid vertex shader: " + vertResult.error.message;
        return false;
    }
    m_vertexShader = vertResult.shader;

    if (vertResult.usedFallback) {
        SDL_LogWarn(SDL_LOG_CATEGORY_GPU,
                    "DebugGridOverlay: Using fallback vertex shader");
    }

    // Load grid fragment shader
    ShaderResources fragResources;
    fragResources.numUniformBuffers = 0;  // No fragment uniforms needed
    fragResources.numSamplers = 0;
    fragResources.numStorageBuffers = 0;
    fragResources.numStorageTextures = 0;

    ShaderLoadResult fragResult = compiler.loadShader(
        "shaders/debug_grid.frag",
        ShaderStage::Fragment,
        "main",
        fragResources
    );

    if (!fragResult.isValid()) {
        m_lastError = "Failed to load debug grid fragment shader: " + fragResult.error.message;
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
                    "DebugGridOverlay: Using fallback fragment shader");
    }

    SDL_LogInfo(SDL_LOG_CATEGORY_GPU, "DebugGridOverlay: Shaders loaded successfully");
    return true;
}

} // namespace sims3000
