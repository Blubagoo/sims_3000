/**
 * @file BloomPass.cpp
 * @brief Bloom post-process pass implementation with shader-based extraction and blur.
 *
 * Implements the full bloom pipeline:
 * 1. Bright pixel extraction (conservative threshold for dark environment)
 * 2. Gaussian blur (horizontal + vertical separable blur)
 * 3. Additive composite (bloom + original scene)
 *
 * Performance targets:
 * - High (1/2 res): ~0.5ms at 1080p
 * - Medium (1/4 res): ~0.3ms at 1080p
 * - Low (1/8 res): ~0.15ms at 1080p
 */

#include "sims3000/render/BloomPass.h"
#include "sims3000/render/GPUDevice.h"
#include "sims3000/render/ShaderCompiler.h"
#include "sims3000/render/ToonShaderConfig.h"
#include <SDL3/SDL_log.h>
#include <algorithm>
#include <glm/glm.hpp>

namespace sims3000 {

// =============================================================================
// UBO Structures (must match shader cbuffer layouts)
// =============================================================================

/**
 * @struct BloomExtractUBO
 * @brief Uniform buffer for bright pixel extraction shader.
 */
struct BloomExtractUBO {
    float threshold;    // Brightness threshold (0.0 - 1.0)
    float softKnee;     // Soft threshold knee for smooth falloff
    float intensity;    // Pre-extraction intensity multiplier
    float _padding;     // Align to 16 bytes
};
static_assert(sizeof(BloomExtractUBO) == 16, "BloomExtractUBO must be 16 bytes");

/**
 * @struct BloomBlurUBO
 * @brief Uniform buffer for Gaussian blur shader.
 */
struct BloomBlurUBO {
    glm::vec2 texelSize;     // 1.0 / textureSize
    glm::vec2 blurDirection; // (1,0) horizontal, (0,1) vertical
};
static_assert(sizeof(BloomBlurUBO) == 16, "BloomBlurUBO must be 16 bytes");

/**
 * @struct BloomCompositeUBO
 * @brief Uniform buffer for final composite shader.
 */
struct BloomCompositeUBO {
    float bloomIntensity; // Bloom strength multiplier
    float exposure;       // Exposure adjustment (1.0 = no change)
    float _padding0;
    float _padding1;
};
static_assert(sizeof(BloomCompositeUBO) == 16, "BloomCompositeUBO must be 16 bytes");

// =============================================================================
// Construction / Destruction
// =============================================================================

BloomPass::BloomPass(GPUDevice& device, std::uint32_t width, std::uint32_t height)
    : BloomPass(device, width, height, BloomConfig{})
{
}

BloomPass::BloomPass(GPUDevice& device, std::uint32_t width, std::uint32_t height,
                     const BloomConfig& config)
    : m_device(&device)
    , m_width(width)
    , m_height(height)
    , m_config(config)
{
    // Read initial config from ToonShaderConfig singleton
    const auto& shaderConfig = ToonShaderConfig::instance();
    m_config.threshold = shaderConfig.getBloomThreshold();
    m_config.intensity = std::max(shaderConfig.getBloomIntensity(), BloomConfig::MIN_INTENSITY);

    if (!createResources()) {
        SDL_LogError(SDL_LOG_CATEGORY_GPU,
                     "BloomPass: Failed to create resources: %s", m_lastError.c_str());
    }
}

BloomPass::~BloomPass() {
    releaseResources();
}

BloomPass::BloomPass(BloomPass&& other) noexcept
    : m_device(other.m_device)
    , m_width(other.m_width)
    , m_height(other.m_height)
    , m_bloomWidth(other.m_bloomWidth)
    , m_bloomHeight(other.m_bloomHeight)
    , m_config(other.m_config)
    , m_extractionTarget(other.m_extractionTarget)
    , m_blurTargetA(other.m_blurTargetA)
    , m_blurTargetB(other.m_blurTargetB)
    , m_sampler(other.m_sampler)
    , m_extractPipeline(other.m_extractPipeline)
    , m_blurPipeline(other.m_blurPipeline)
    , m_compositePipeline(other.m_compositePipeline)
    , m_fullscreenVertShader(other.m_fullscreenVertShader)
    , m_extractFragShader(other.m_extractFragShader)
    , m_blurFragShader(other.m_blurFragShader)
    , m_compositeFragShader(other.m_compositeFragShader)
    , m_colorFormat(other.m_colorFormat)
    , m_stats(other.m_stats)
    , m_lastError(std::move(other.m_lastError))
{
    other.m_device = nullptr;
    other.m_extractionTarget = nullptr;
    other.m_blurTargetA = nullptr;
    other.m_blurTargetB = nullptr;
    other.m_sampler = nullptr;
    other.m_extractPipeline = nullptr;
    other.m_blurPipeline = nullptr;
    other.m_compositePipeline = nullptr;
    other.m_fullscreenVertShader = nullptr;
    other.m_extractFragShader = nullptr;
    other.m_blurFragShader = nullptr;
    other.m_compositeFragShader = nullptr;
}

BloomPass& BloomPass::operator=(BloomPass&& other) noexcept {
    if (this != &other) {
        releaseResources();

        m_device = other.m_device;
        m_width = other.m_width;
        m_height = other.m_height;
        m_bloomWidth = other.m_bloomWidth;
        m_bloomHeight = other.m_bloomHeight;
        m_config = other.m_config;
        m_extractionTarget = other.m_extractionTarget;
        m_blurTargetA = other.m_blurTargetA;
        m_blurTargetB = other.m_blurTargetB;
        m_sampler = other.m_sampler;
        m_extractPipeline = other.m_extractPipeline;
        m_blurPipeline = other.m_blurPipeline;
        m_compositePipeline = other.m_compositePipeline;
        m_fullscreenVertShader = other.m_fullscreenVertShader;
        m_extractFragShader = other.m_extractFragShader;
        m_blurFragShader = other.m_blurFragShader;
        m_compositeFragShader = other.m_compositeFragShader;
        m_colorFormat = other.m_colorFormat;
        m_stats = other.m_stats;
        m_lastError = std::move(other.m_lastError);

        other.m_device = nullptr;
        other.m_extractionTarget = nullptr;
        other.m_blurTargetA = nullptr;
        other.m_blurTargetB = nullptr;
        other.m_sampler = nullptr;
        other.m_extractPipeline = nullptr;
        other.m_blurPipeline = nullptr;
        other.m_compositePipeline = nullptr;
        other.m_fullscreenVertShader = nullptr;
        other.m_extractFragShader = nullptr;
        other.m_blurFragShader = nullptr;
        other.m_compositeFragShader = nullptr;
    }
    return *this;
}

// =============================================================================
// Public Interface
// =============================================================================

bool BloomPass::isValid() const {
    return m_device != nullptr &&
           m_extractionTarget != nullptr &&
           m_blurTargetA != nullptr &&
           m_blurTargetB != nullptr &&
           m_sampler != nullptr &&
           m_extractPipeline != nullptr &&
           m_blurPipeline != nullptr &&
           m_compositePipeline != nullptr;
}

bool BloomPass::execute(SDL_GPUCommandBuffer* cmdBuffer,
                        SDL_GPUTexture* inputTexture,
                        SDL_GPUTexture* outputTexture) {
    if (!isValid()) {
        m_lastError = "BloomPass not valid";
        return false;
    }

    if (!cmdBuffer || !inputTexture || !outputTexture) {
        m_lastError = "Invalid input parameters";
        return false;
    }

    // Update stats
    m_stats.bloomWidth = m_bloomWidth;
    m_stats.bloomHeight = m_bloomHeight;

    // Stage 1: Extract bright pixels to extraction target
    if (!executeExtraction(cmdBuffer, inputTexture)) {
        return false;
    }

    // Stage 2: Blur the extracted bright pixels
    if (!executeBlur(cmdBuffer)) {
        return false;
    }

    // Stage 3: Composite bloom with original scene
    if (!executeComposite(cmdBuffer, inputTexture, outputTexture)) {
        return false;
    }

    m_stats.totalTimeMs = m_stats.extractionTimeMs + m_stats.blurTimeMs + m_stats.compositeTimeMs;

    return true;
}

bool BloomPass::resize(std::uint32_t width, std::uint32_t height) {
    if (width == m_width && height == m_height) {
        return true;  // No change needed
    }

    m_width = width;
    m_height = height;

    releaseResources();
    return createResources();
}

void BloomPass::setConfig(const BloomConfig& config) {
    m_config = config;
    // Enforce minimum intensity per canon (bloom cannot be fully disabled)
    m_config.intensity = std::max(m_config.intensity, BloomConfig::MIN_INTENSITY);

    // If quality changed, need to recreate resources
    // (handled on next execute or explicit resize)
}

void BloomPass::setThreshold(float threshold) {
    m_config.threshold = std::clamp(threshold, 0.0f, 1.0f);
}

void BloomPass::setIntensity(float intensity) {
    // Enforce minimum intensity
    m_config.intensity = std::clamp(intensity, BloomConfig::MIN_INTENSITY, 2.0f);
}

void BloomPass::setQuality(BloomQuality quality) {
    if (quality != m_config.quality) {
        m_config.quality = quality;
        // Recreate resources at new resolution
        releaseResources();
        createResources();
    }
}

std::uint32_t BloomPass::getBloomWidth() const {
    return m_bloomWidth;
}

std::uint32_t BloomPass::getBloomHeight() const {
    return m_bloomHeight;
}

// =============================================================================
// Shader-Based Pass Execution
// =============================================================================

bool BloomPass::executeExtraction(SDL_GPUCommandBuffer* cmdBuffer, SDL_GPUTexture* inputTexture) {
    // Prepare extraction uniform buffer
    BloomExtractUBO ubo;
    ubo.threshold = m_config.threshold;
    ubo.softKnee = 0.5f;  // Soft knee for smooth falloff
    ubo.intensity = 1.0f; // Pre-extraction intensity (main intensity applied in composite)
    ubo._padding = 0.0f;

    // Configure render target
    SDL_GPUColorTargetInfo colorTarget = {};
    colorTarget.texture = m_extractionTarget;
    colorTarget.load_op = SDL_GPU_LOADOP_DONT_CARE;
    colorTarget.store_op = SDL_GPU_STOREOP_STORE;

    // Begin extraction render pass
    SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(cmdBuffer, &colorTarget, 1, nullptr);
    if (!renderPass) {
        m_lastError = std::string("Failed to begin extraction render pass: ") + SDL_GetError();
        return false;
    }

    // Bind extraction pipeline
    SDL_BindGPUGraphicsPipeline(renderPass, m_extractPipeline);

    // Bind scene texture
    SDL_GPUTextureSamplerBinding textureBinding = {};
    textureBinding.texture = inputTexture;
    textureBinding.sampler = m_sampler;
    SDL_BindGPUFragmentSamplers(renderPass, 0, &textureBinding, 1);

    // Push uniform data
    SDL_PushGPUFragmentUniformData(cmdBuffer, 0, &ubo, sizeof(ubo));

    // Set viewport to bloom resolution
    SDL_GPUViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.w = static_cast<float>(m_bloomWidth);
    viewport.h = static_cast<float>(m_bloomHeight);
    viewport.min_depth = 0.0f;
    viewport.max_depth = 1.0f;
    SDL_SetGPUViewport(renderPass, &viewport);

    // Draw fullscreen triangle
    SDL_DrawGPUPrimitives(renderPass, 3, 1, 0, 0);

    SDL_EndGPURenderPass(renderPass);

    m_stats.extractionTimeMs = 0.0f; // Would need GPU timing

    return true;
}

bool BloomPass::executeBlur(SDL_GPUCommandBuffer* cmdBuffer) {
    BloomBlurUBO ubo;
    ubo.texelSize = glm::vec2(1.0f / static_cast<float>(m_bloomWidth),
                              1.0f / static_cast<float>(m_bloomHeight));

    // Horizontal blur: extraction -> blurTargetA
    {
        ubo.blurDirection = glm::vec2(1.0f, 0.0f);

        SDL_GPUColorTargetInfo colorTarget = {};
        colorTarget.texture = m_blurTargetA;
        colorTarget.load_op = SDL_GPU_LOADOP_DONT_CARE;
        colorTarget.store_op = SDL_GPU_STOREOP_STORE;

        SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(cmdBuffer, &colorTarget, 1, nullptr);
        if (!renderPass) {
            m_lastError = std::string("Failed to begin horizontal blur pass: ") + SDL_GetError();
            return false;
        }

        SDL_BindGPUGraphicsPipeline(renderPass, m_blurPipeline);

        SDL_GPUTextureSamplerBinding textureBinding = {};
        textureBinding.texture = m_extractionTarget;
        textureBinding.sampler = m_sampler;
        SDL_BindGPUFragmentSamplers(renderPass, 0, &textureBinding, 1);

        SDL_PushGPUFragmentUniformData(cmdBuffer, 0, &ubo, sizeof(ubo));

        SDL_GPUViewport viewport = {};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.w = static_cast<float>(m_bloomWidth);
        viewport.h = static_cast<float>(m_bloomHeight);
        viewport.min_depth = 0.0f;
        viewport.max_depth = 1.0f;
        SDL_SetGPUViewport(renderPass, &viewport);

        SDL_DrawGPUPrimitives(renderPass, 3, 1, 0, 0);

        SDL_EndGPURenderPass(renderPass);
    }

    // Vertical blur: blurTargetA -> blurTargetB
    {
        ubo.blurDirection = glm::vec2(0.0f, 1.0f);

        SDL_GPUColorTargetInfo colorTarget = {};
        colorTarget.texture = m_blurTargetB;
        colorTarget.load_op = SDL_GPU_LOADOP_DONT_CARE;
        colorTarget.store_op = SDL_GPU_STOREOP_STORE;

        SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(cmdBuffer, &colorTarget, 1, nullptr);
        if (!renderPass) {
            m_lastError = std::string("Failed to begin vertical blur pass: ") + SDL_GetError();
            return false;
        }

        SDL_BindGPUGraphicsPipeline(renderPass, m_blurPipeline);

        SDL_GPUTextureSamplerBinding textureBinding = {};
        textureBinding.texture = m_blurTargetA;
        textureBinding.sampler = m_sampler;
        SDL_BindGPUFragmentSamplers(renderPass, 0, &textureBinding, 1);

        SDL_PushGPUFragmentUniformData(cmdBuffer, 0, &ubo, sizeof(ubo));

        SDL_GPUViewport viewport = {};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.w = static_cast<float>(m_bloomWidth);
        viewport.h = static_cast<float>(m_bloomHeight);
        viewport.min_depth = 0.0f;
        viewport.max_depth = 1.0f;
        SDL_SetGPUViewport(renderPass, &viewport);

        SDL_DrawGPUPrimitives(renderPass, 3, 1, 0, 0);

        SDL_EndGPURenderPass(renderPass);
    }

    m_stats.blurTimeMs = 0.0f; // Would need GPU timing

    return true;
}

bool BloomPass::executeComposite(SDL_GPUCommandBuffer* cmdBuffer,
                                  SDL_GPUTexture* inputTexture,
                                  SDL_GPUTexture* outputTexture) {
    BloomCompositeUBO ubo;
    ubo.bloomIntensity = m_config.intensity;
    ubo.exposure = 1.0f;  // No exposure adjustment
    ubo._padding0 = 0.0f;
    ubo._padding1 = 0.0f;

    SDL_GPUColorTargetInfo colorTarget = {};
    colorTarget.texture = outputTexture;
    colorTarget.load_op = SDL_GPU_LOADOP_DONT_CARE;
    colorTarget.store_op = SDL_GPU_STOREOP_STORE;

    SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(cmdBuffer, &colorTarget, 1, nullptr);
    if (!renderPass) {
        m_lastError = std::string("Failed to begin composite render pass: ") + SDL_GetError();
        return false;
    }

    SDL_BindGPUGraphicsPipeline(renderPass, m_compositePipeline);

    // Bind scene and bloom textures
    SDL_GPUTextureSamplerBinding textureBindings[2] = {};
    textureBindings[0].texture = inputTexture;
    textureBindings[0].sampler = m_sampler;
    textureBindings[1].texture = m_blurTargetB;  // Final blurred bloom
    textureBindings[1].sampler = m_sampler;
    SDL_BindGPUFragmentSamplers(renderPass, 0, textureBindings, 2);

    SDL_PushGPUFragmentUniformData(cmdBuffer, 0, &ubo, sizeof(ubo));

    // Output at full resolution
    SDL_GPUViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.w = static_cast<float>(m_width);
    viewport.h = static_cast<float>(m_height);
    viewport.min_depth = 0.0f;
    viewport.max_depth = 1.0f;
    SDL_SetGPUViewport(renderPass, &viewport);

    SDL_DrawGPUPrimitives(renderPass, 3, 1, 0, 0);

    SDL_EndGPURenderPass(renderPass);

    m_stats.compositeTimeMs = 0.0f; // Would need GPU timing

    return true;
}

// =============================================================================
// Private Implementation
// =============================================================================

bool BloomPass::createResources() {
    if (!m_device || !m_device->isValid()) {
        m_lastError = "Invalid GPU device";
        return false;
    }

    SDL_GPUDevice* device = m_device->getHandle();
    if (!device) {
        m_lastError = "GPU device handle is null";
        return false;
    }

    calculateBloomResolution();

    // Validate resolution
    if (m_bloomWidth == 0 || m_bloomHeight == 0) {
        m_lastError = "Invalid bloom resolution";
        return false;
    }

    // Create extraction target (holds bright pixels)
    {
        SDL_GPUTextureCreateInfo createInfo = {};
        createInfo.type = SDL_GPU_TEXTURETYPE_2D;
        createInfo.format = m_colorFormat;
        createInfo.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET |
                          SDL_GPU_TEXTUREUSAGE_SAMPLER;
        createInfo.width = m_bloomWidth;
        createInfo.height = m_bloomHeight;
        createInfo.layer_count_or_depth = 1;
        createInfo.num_levels = 1;

        m_extractionTarget = SDL_CreateGPUTexture(device, &createInfo);
        if (!m_extractionTarget) {
            m_lastError = std::string("Failed to create extraction target: ") + SDL_GetError();
            return false;
        }
    }

    // Create blur targets (ping-pong for separable blur)
    {
        SDL_GPUTextureCreateInfo createInfo = {};
        createInfo.type = SDL_GPU_TEXTURETYPE_2D;
        createInfo.format = m_colorFormat;
        createInfo.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET |
                          SDL_GPU_TEXTUREUSAGE_SAMPLER;
        createInfo.width = m_bloomWidth;
        createInfo.height = m_bloomHeight;
        createInfo.layer_count_or_depth = 1;
        createInfo.num_levels = 1;

        m_blurTargetA = SDL_CreateGPUTexture(device, &createInfo);
        if (!m_blurTargetA) {
            m_lastError = std::string("Failed to create blur target A: ") + SDL_GetError();
            releaseResources();
            return false;
        }

        m_blurTargetB = SDL_CreateGPUTexture(device, &createInfo);
        if (!m_blurTargetB) {
            m_lastError = std::string("Failed to create blur target B: ") + SDL_GetError();
            releaseResources();
            return false;
        }
    }

    // Create sampler for texture reads (linear for smooth bloom)
    {
        SDL_GPUSamplerCreateInfo samplerInfo = {};
        samplerInfo.min_filter = SDL_GPU_FILTER_LINEAR;
        samplerInfo.mag_filter = SDL_GPU_FILTER_LINEAR;
        samplerInfo.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
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

    // Load shaders
    if (!loadShaders()) {
        releaseResources();
        return false;
    }

    // Create pipelines
    if (!createPipelines()) {
        releaseResources();
        return false;
    }

    SDL_LogInfo(SDL_LOG_CATEGORY_GPU,
                "BloomPass: Created resources at %ux%u (source: %ux%u, quality: %s)",
                m_bloomWidth, m_bloomHeight, m_width, m_height,
                getBloomQualityName(m_config.quality));

    return true;
}

bool BloomPass::loadShaders() {
    ShaderCompiler compiler(*m_device);

    // Load fullscreen vertex shader (shared by all passes)
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
    m_fullscreenVertShader = vertResult.shader;

    if (vertResult.usedFallback) {
        SDL_LogWarn(SDL_LOG_CATEGORY_GPU,
                    "BloomPass: Using fallback vertex shader");
    }

    // Load extraction fragment shader
    ShaderResources extractResources;
    extractResources.numUniformBuffers = 1;
    extractResources.numSamplers = 1;
    extractResources.numStorageBuffers = 0;
    extractResources.numStorageTextures = 0;

    ShaderLoadResult extractResult = compiler.loadShader(
        "shaders/bloom_extract.frag",
        ShaderStage::Fragment,
        "main",
        extractResources
    );

    if (!extractResult.isValid()) {
        m_lastError = "Failed to load bloom extraction shader: " + extractResult.error.message;
        return false;
    }
    m_extractFragShader = extractResult.shader;

    // Load blur fragment shader
    ShaderResources blurResources;
    blurResources.numUniformBuffers = 1;
    blurResources.numSamplers = 1;
    blurResources.numStorageBuffers = 0;
    blurResources.numStorageTextures = 0;

    ShaderLoadResult blurResult = compiler.loadShader(
        "shaders/bloom_blur.frag",
        ShaderStage::Fragment,
        "main",
        blurResources
    );

    if (!blurResult.isValid()) {
        m_lastError = "Failed to load bloom blur shader: " + blurResult.error.message;
        return false;
    }
    m_blurFragShader = blurResult.shader;

    // Load composite fragment shader
    ShaderResources compositeResources;
    compositeResources.numUniformBuffers = 1;
    compositeResources.numSamplers = 2;  // Scene + bloom textures
    compositeResources.numStorageBuffers = 0;
    compositeResources.numStorageTextures = 0;

    ShaderLoadResult compositeResult = compiler.loadShader(
        "shaders/bloom_composite.frag",
        ShaderStage::Fragment,
        "main",
        compositeResources
    );

    if (!compositeResult.isValid()) {
        m_lastError = "Failed to load bloom composite shader: " + compositeResult.error.message;
        return false;
    }
    m_compositeFragShader = compositeResult.shader;

    SDL_LogInfo(SDL_LOG_CATEGORY_GPU, "BloomPass: Shaders loaded successfully");
    return true;
}

bool BloomPass::createPipelines() {
    SDL_GPUDevice* device = m_device->getHandle();

    // Common pipeline settings for fullscreen post-process passes
    auto createPostProcessPipeline = [&](SDL_GPUShader* fragShader,
                                         SDL_GPUTextureFormat targetFormat) -> SDL_GPUGraphicsPipeline* {
        SDL_GPUColorTargetDescription colorTargetDesc = {};
        colorTargetDesc.format = targetFormat;
        colorTargetDesc.blend_state.enable_blend = false;
        colorTargetDesc.blend_state.color_write_mask = 0xF;

        SDL_GPUGraphicsPipelineCreateInfo pipelineInfo = {};
        pipelineInfo.vertex_shader = m_fullscreenVertShader;
        pipelineInfo.fragment_shader = fragShader;

        // No vertex input - fullscreen triangle generated in shader
        pipelineInfo.vertex_input_state.num_vertex_buffers = 0;
        pipelineInfo.vertex_input_state.num_vertex_attributes = 0;

        pipelineInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;

        pipelineInfo.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
        pipelineInfo.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
        pipelineInfo.rasterizer_state.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;

        // No depth testing for post-process
        pipelineInfo.depth_stencil_state.enable_depth_test = false;
        pipelineInfo.depth_stencil_state.enable_depth_write = false;
        pipelineInfo.depth_stencil_state.enable_stencil_test = false;

        pipelineInfo.target_info.num_color_targets = 1;
        pipelineInfo.target_info.color_target_descriptions = &colorTargetDesc;
        pipelineInfo.target_info.has_depth_stencil_target = false;

        return SDL_CreateGPUGraphicsPipeline(device, &pipelineInfo);
    };

    // Create extraction pipeline (renders to bloom resolution HDR target)
    m_extractPipeline = createPostProcessPipeline(m_extractFragShader, m_colorFormat);
    if (!m_extractPipeline) {
        m_lastError = std::string("Failed to create extraction pipeline: ") + SDL_GetError();
        return false;
    }

    // Create blur pipeline (renders to bloom resolution HDR target)
    m_blurPipeline = createPostProcessPipeline(m_blurFragShader, m_colorFormat);
    if (!m_blurPipeline) {
        m_lastError = std::string("Failed to create blur pipeline: ") + SDL_GetError();
        return false;
    }

    // Create composite pipeline (renders to swapchain format)
    // We use BGRA8 for swapchain compatibility
    m_compositePipeline = createPostProcessPipeline(m_compositeFragShader,
                                                     SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM);
    if (!m_compositePipeline) {
        m_lastError = std::string("Failed to create composite pipeline: ") + SDL_GetError();
        return false;
    }

    SDL_LogInfo(SDL_LOG_CATEGORY_GPU, "BloomPass: Pipelines created successfully");
    return true;
}

void BloomPass::releaseResources() {
    if (!m_device || !m_device->isValid()) {
        return;
    }

    SDL_GPUDevice* device = m_device->getHandle();
    if (!device) {
        return;
    }

    // Release pipelines
    if (m_extractPipeline) {
        SDL_ReleaseGPUGraphicsPipeline(device, m_extractPipeline);
        m_extractPipeline = nullptr;
    }

    if (m_blurPipeline) {
        SDL_ReleaseGPUGraphicsPipeline(device, m_blurPipeline);
        m_blurPipeline = nullptr;
    }

    if (m_compositePipeline) {
        SDL_ReleaseGPUGraphicsPipeline(device, m_compositePipeline);
        m_compositePipeline = nullptr;
    }

    // Release shaders
    if (m_fullscreenVertShader) {
        SDL_ReleaseGPUShader(device, m_fullscreenVertShader);
        m_fullscreenVertShader = nullptr;
    }

    if (m_extractFragShader) {
        SDL_ReleaseGPUShader(device, m_extractFragShader);
        m_extractFragShader = nullptr;
    }

    if (m_blurFragShader) {
        SDL_ReleaseGPUShader(device, m_blurFragShader);
        m_blurFragShader = nullptr;
    }

    if (m_compositeFragShader) {
        SDL_ReleaseGPUShader(device, m_compositeFragShader);
        m_compositeFragShader = nullptr;
    }

    // Release textures
    if (m_extractionTarget) {
        SDL_ReleaseGPUTexture(device, m_extractionTarget);
        m_extractionTarget = nullptr;
    }

    if (m_blurTargetA) {
        SDL_ReleaseGPUTexture(device, m_blurTargetA);
        m_blurTargetA = nullptr;
    }

    if (m_blurTargetB) {
        SDL_ReleaseGPUTexture(device, m_blurTargetB);
        m_blurTargetB = nullptr;
    }

    if (m_sampler) {
        SDL_ReleaseGPUSampler(device, m_sampler);
        m_sampler = nullptr;
    }
}

void BloomPass::calculateBloomResolution() {
    int divisor = getQualityDivisor(m_config.quality);
    m_bloomWidth = std::max(1u, m_width / static_cast<std::uint32_t>(divisor));
    m_bloomHeight = std::max(1u, m_height / static_cast<std::uint32_t>(divisor));
}

int BloomPass::getQualityDivisor(BloomQuality quality) {
    switch (quality) {
        case BloomQuality::High:   return 2;
        case BloomQuality::Medium: return 4;
        case BloomQuality::Low:    return 8;
        default:                   return 4;
    }
}

// =============================================================================
// Utility Functions
// =============================================================================

const char* getBloomQualityName(BloomQuality quality) {
    switch (quality) {
        case BloomQuality::High:   return "High";
        case BloomQuality::Medium: return "Medium";
        case BloomQuality::Low:    return "Low";
        default:                   return "Unknown";
    }
}

} // namespace sims3000
