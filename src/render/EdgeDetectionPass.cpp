/**
 * @file EdgeDetectionPass.cpp
 * @brief Screen-space Sobel edge detection implementation.
 */

#include "sims3000/render/EdgeDetectionPass.h"
#include "sims3000/render/GPUDevice.h"
#include "sims3000/render/ShaderCompiler.h"
#include "sims3000/render/ToonShaderConfig.h"
#include "sims3000/render/CameraState.h"
#include <SDL3/SDL_log.h>
#include <algorithm>

namespace sims3000 {

// =============================================================================
// Construction / Destruction
// =============================================================================

EdgeDetectionPass::EdgeDetectionPass(GPUDevice& device, SDL_GPUTextureFormat colorFormat)
    : m_device(&device)
    , m_colorFormat(colorFormat)
{
    // Initialize config from ToonShaderConfig singleton
    const auto& shaderConfig = ToonShaderConfig::instance();
    m_config.edgeThickness = shaderConfig.getEdgeLineWidth();

    // Use canon-specified dark purple for outlines
    m_config.outlineColor = glm::vec4(
        ToonShaderConfigDefaults::SHADOW_COLOR_R,
        ToonShaderConfigDefaults::SHADOW_COLOR_G,
        ToonShaderConfigDefaults::SHADOW_COLOR_B,
        1.0f
    );

    // Set camera planes from config
    m_config.nearPlane = CameraConfig::NEAR_PLANE;
    m_config.farPlane = CameraConfig::FAR_PLANE;

    if (!createResources()) {
        SDL_LogError(SDL_LOG_CATEGORY_GPU,
                     "EdgeDetectionPass: Failed to create resources: %s",
                     m_lastError.c_str());
    }
}

EdgeDetectionPass::~EdgeDetectionPass() {
    releaseResources();
}

EdgeDetectionPass::EdgeDetectionPass(EdgeDetectionPass&& other) noexcept
    : m_device(other.m_device)
    , m_colorFormat(other.m_colorFormat)
    , m_config(other.m_config)
    , m_pipeline(other.m_pipeline)
    , m_vertexShader(other.m_vertexShader)
    , m_fragmentShader(other.m_fragmentShader)
    , m_pointSampler(other.m_pointSampler)
    , m_stats(other.m_stats)
    , m_lastError(std::move(other.m_lastError))
{
    other.m_device = nullptr;
    other.m_pipeline = nullptr;
    other.m_vertexShader = nullptr;
    other.m_fragmentShader = nullptr;
    other.m_pointSampler = nullptr;
}

EdgeDetectionPass& EdgeDetectionPass::operator=(EdgeDetectionPass&& other) noexcept {
    if (this != &other) {
        releaseResources();

        m_device = other.m_device;
        m_colorFormat = other.m_colorFormat;
        m_config = other.m_config;
        m_pipeline = other.m_pipeline;
        m_vertexShader = other.m_vertexShader;
        m_fragmentShader = other.m_fragmentShader;
        m_pointSampler = other.m_pointSampler;
        m_stats = other.m_stats;
        m_lastError = std::move(other.m_lastError);

        other.m_device = nullptr;
        other.m_pipeline = nullptr;
        other.m_vertexShader = nullptr;
        other.m_fragmentShader = nullptr;
        other.m_pointSampler = nullptr;
    }
    return *this;
}

// =============================================================================
// Public Interface
// =============================================================================

bool EdgeDetectionPass::isValid() const {
    return m_device != nullptr &&
           m_pipeline != nullptr &&
           m_pointSampler != nullptr;
}

bool EdgeDetectionPass::execute(
    SDL_GPUCommandBuffer* cmdBuffer,
    SDL_GPUTexture* sceneTexture,
    SDL_GPUTexture* normalTexture,
    SDL_GPUTexture* depthTexture,
    SDL_GPUTexture* outputTexture,
    std::uint32_t width,
    std::uint32_t height)
{
    if (!isValid()) {
        m_lastError = "EdgeDetectionPass not valid";
        return false;
    }

    if (!cmdBuffer || !sceneTexture || !normalTexture || !depthTexture || !outputTexture) {
        m_lastError = "Invalid input parameters";
        return false;
    }

    // Update stats
    m_stats.width = width;
    m_stats.height = height;

    // Prepare uniform buffer data
    EdgeDetectionUBO uboData;
    uboData.outlineColor = m_config.outlineColor;
    uboData.texelSize = glm::vec2(1.0f / static_cast<float>(width),
                                   1.0f / static_cast<float>(height));
    uboData.normalThreshold = m_config.normalThreshold;
    uboData.depthThreshold = m_config.depthThreshold;
    uboData.nearPlane = m_config.nearPlane;
    uboData.farPlane = m_config.farPlane;
    uboData.edgeThickness = m_config.edgeThickness;
    uboData._padding = 0.0f;

    // Configure color target for output
    SDL_GPUColorTargetInfo colorTarget = {};
    colorTarget.texture = outputTexture;
    colorTarget.load_op = SDL_GPU_LOADOP_DONT_CARE;  // We're overwriting everything
    colorTarget.store_op = SDL_GPU_STOREOP_STORE;

    // Begin edge detection render pass (no depth target needed)
    SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(cmdBuffer, &colorTarget, 1, nullptr);
    if (!renderPass) {
        m_lastError = std::string("Failed to begin edge detection render pass: ") + SDL_GetError();
        return false;
    }

    // Bind pipeline
    SDL_BindGPUGraphicsPipeline(renderPass, m_pipeline);

    // Bind textures
    SDL_GPUTextureSamplerBinding textureBindings[3] = {};

    // Scene color texture (slot 0)
    textureBindings[0].texture = sceneTexture;
    textureBindings[0].sampler = m_pointSampler;

    // Normal buffer (slot 1)
    textureBindings[1].texture = normalTexture;
    textureBindings[1].sampler = m_pointSampler;

    // Depth buffer (slot 2)
    textureBindings[2].texture = depthTexture;
    textureBindings[2].sampler = m_pointSampler;

    SDL_BindGPUFragmentSamplers(renderPass, 0, textureBindings, 3);

    // Push uniform buffer data
    SDL_PushGPUFragmentUniformData(cmdBuffer, 0, &uboData, sizeof(uboData));

    // Set viewport
    SDL_GPUViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.w = static_cast<float>(width);
    viewport.h = static_cast<float>(height);
    viewport.min_depth = 0.0f;
    viewport.max_depth = 1.0f;
    SDL_SetGPUViewport(renderPass, &viewport);

    // Draw fullscreen triangle (3 vertices, no vertex buffer)
    SDL_DrawGPUPrimitives(renderPass, 3, 1, 0, 0);

    // End render pass
    SDL_EndGPURenderPass(renderPass);

    m_stats.executionTimeMs = 0.0f;  // Would need GPU timing for accurate measurement

    return true;
}

void EdgeDetectionPass::setConfig(const EdgeDetectionConfig& config) {
    m_config = config;
    // Clamp values to valid ranges
    m_config.edgeThickness = std::clamp(m_config.edgeThickness, 0.5f, 3.0f);
    m_config.normalThreshold = std::clamp(m_config.normalThreshold, 0.0f, 1.0f);
    m_config.depthThreshold = std::clamp(m_config.depthThreshold, 0.0f, 1.0f);
}

void EdgeDetectionPass::setOutlineColor(const glm::vec4& color) {
    m_config.outlineColor = color;
}

void EdgeDetectionPass::setEdgeThickness(float thickness) {
    m_config.edgeThickness = std::clamp(thickness, 0.5f, 3.0f);
}

void EdgeDetectionPass::setCameraPlanes(float nearPlane, float farPlane) {
    m_config.nearPlane = nearPlane;
    m_config.farPlane = farPlane;
}

void EdgeDetectionPass::setNormalThreshold(float threshold) {
    m_config.normalThreshold = std::clamp(threshold, 0.0f, 1.0f);
}

void EdgeDetectionPass::setDepthThreshold(float threshold) {
    m_config.depthThreshold = std::clamp(threshold, 0.0f, 1.0f);
}

void EdgeDetectionPass::applyTerrainConfig() {
    if (!m_terrainConfigActive) {
        // Store current config as building config before switching
        m_buildingConfig = m_config;
    }

    // Apply terrain-specific configuration
    m_config.normalThreshold = TerrainEdgeConfig::NORMAL_THRESHOLD;
    m_config.depthThreshold = TerrainEdgeConfig::DEPTH_THRESHOLD;
    m_config.edgeThickness = std::clamp(TerrainEdgeConfig::EDGE_THICKNESS, 0.5f, 3.0f);

    m_terrainConfigActive = true;

    SDL_LogDebug(SDL_LOG_CATEGORY_GPU,
                 "EdgeDetectionPass: Applied terrain config (normal=%.2f, depth=%.2f, thickness=%.1f)",
                 m_config.normalThreshold, m_config.depthThreshold, m_config.edgeThickness);
}

void EdgeDetectionPass::applyBuildingConfig() {
    if (m_terrainConfigActive) {
        // Restore stored building config
        m_config.normalThreshold = m_buildingConfig.normalThreshold;
        m_config.depthThreshold = m_buildingConfig.depthThreshold;
        m_config.edgeThickness = m_buildingConfig.edgeThickness;

        m_terrainConfigActive = false;

        SDL_LogDebug(SDL_LOG_CATEGORY_GPU,
                     "EdgeDetectionPass: Applied building config (normal=%.2f, depth=%.2f, thickness=%.1f)",
                     m_config.normalThreshold, m_config.depthThreshold, m_config.edgeThickness);
    }
}

// =============================================================================
// Private Implementation
// =============================================================================

bool EdgeDetectionPass::createResources() {
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

    // Create point sampler for edge detection (need accurate texel sampling)
    {
        SDL_GPUSamplerCreateInfo samplerInfo = {};
        samplerInfo.min_filter = SDL_GPU_FILTER_NEAREST;
        samplerInfo.mag_filter = SDL_GPU_FILTER_NEAREST;
        samplerInfo.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
        samplerInfo.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        samplerInfo.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        samplerInfo.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;

        m_pointSampler = SDL_CreateGPUSampler(device, &samplerInfo);
        if (!m_pointSampler) {
            m_lastError = std::string("Failed to create point sampler: ") + SDL_GetError();
            releaseResources();
            return false;
        }
    }

    // Create graphics pipeline
    {
        // Color target description (no blending for post-process)
        SDL_GPUColorTargetDescription colorTargetDesc = {};
        colorTargetDesc.format = m_colorFormat;
        colorTargetDesc.blend_state.enable_blend = false;
        colorTargetDesc.blend_state.color_write_mask = 0xF;  // Write all channels

        // Pipeline creation info
        SDL_GPUGraphicsPipelineCreateInfo pipelineInfo = {};
        pipelineInfo.vertex_shader = m_vertexShader;
        pipelineInfo.fragment_shader = m_fragmentShader;

        // No vertex input - fullscreen triangle generated in shader
        pipelineInfo.vertex_input_state.num_vertex_buffers = 0;
        pipelineInfo.vertex_input_state.num_vertex_attributes = 0;

        // Primitive state
        pipelineInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;

        // Rasterizer state
        pipelineInfo.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
        pipelineInfo.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;  // No culling for fullscreen
        pipelineInfo.rasterizer_state.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;

        // No depth testing for post-process
        pipelineInfo.depth_stencil_state.enable_depth_test = false;
        pipelineInfo.depth_stencil_state.enable_depth_write = false;
        pipelineInfo.depth_stencil_state.enable_stencil_test = false;

        // Color targets
        pipelineInfo.target_info.num_color_targets = 1;
        pipelineInfo.target_info.color_target_descriptions = &colorTargetDesc;
        pipelineInfo.target_info.has_depth_stencil_target = false;

        m_pipeline = SDL_CreateGPUGraphicsPipeline(device, &pipelineInfo);
        if (!m_pipeline) {
            m_lastError = std::string("Failed to create edge detection pipeline: ") + SDL_GetError();
            releaseResources();
            return false;
        }
    }

    SDL_LogInfo(SDL_LOG_CATEGORY_GPU, "EdgeDetectionPass: Created resources successfully");
    return true;
}

void EdgeDetectionPass::releaseResources() {
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

    if (m_pointSampler) {
        SDL_ReleaseGPUSampler(device, m_pointSampler);
        m_pointSampler = nullptr;
    }
}

bool EdgeDetectionPass::loadShaders() {
    ShaderCompiler compiler(*m_device);

    // Load fullscreen vertex shader
    ShaderResources vertResources;
    vertResources.numUniformBuffers = 0;
    vertResources.numSamplers = 0;
    vertResources.numStorageBuffers = 0;
    vertResources.numStorageTextures = 0;

    ShaderLoadResult vertResult = compiler.loadShader(
        "shaders/fullscreen.vert",
        ShaderStage::Vertex,
        "main",
        vertResources
    );

    if (!vertResult.isValid()) {
        m_lastError = "Failed to load fullscreen vertex shader: " + vertResult.error.message;
        return false;
    }
    m_vertexShader = vertResult.shader;

    if (vertResult.usedFallback) {
        SDL_LogWarn(SDL_LOG_CATEGORY_GPU,
                    "EdgeDetectionPass: Using fallback vertex shader");
    }

    // Load edge detection fragment shader
    ShaderResources fragResources;
    fragResources.numUniformBuffers = 1;  // EdgeDetectionUBO
    fragResources.numSamplers = 3;         // Scene, normal, depth textures
    fragResources.numStorageBuffers = 0;
    fragResources.numStorageTextures = 0;

    ShaderLoadResult fragResult = compiler.loadShader(
        "shaders/edge_detect.frag",
        ShaderStage::Fragment,
        "main",
        fragResources
    );

    if (!fragResult.isValid()) {
        m_lastError = "Failed to load edge detection fragment shader: " + fragResult.error.message;
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
                    "EdgeDetectionPass: Using fallback fragment shader");
    }

    SDL_LogInfo(SDL_LOG_CATEGORY_GPU, "EdgeDetectionPass: Shaders loaded successfully");
    return true;
}

} // namespace sims3000
