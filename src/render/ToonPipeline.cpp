/**
 * @file ToonPipeline.cpp
 * @brief Implementation of toon graphics pipeline creation and management.
 */

#include "sims3000/render/ToonPipeline.h"
#include "sims3000/render/GPUDevice.h"
#include "sims3000/render/DepthState.h"
#include "sims3000/render/BlendState.h"
#include "sims3000/assets/ModelLoader.h"
#include <SDL3/SDL.h>
#include <cstring>

namespace sims3000 {

// =============================================================================
// ToonVertexLayout Implementation
// =============================================================================

SDL_GPUVertexInputState ToonVertexLayout::getVertexInputState() {
    // Static storage for vertex attributes and bindings
    // These must persist until pipeline creation completes
    static SDL_GPUVertexBufferDescription vertexBufferDesc;
    static SDL_GPUVertexAttribute vertexAttributes[3];

    // Configure vertex buffer binding
    vertexBufferDesc.slot = 0;
    vertexBufferDesc.pitch = VERTEX_STRIDE;
    vertexBufferDesc.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
    vertexBufferDesc.instance_step_rate = 0;  // Not instanced - we use storage buffer for instances

    // Position attribute (vec3 at offset 0)
    // Shader: float3 position : TEXCOORD0
    vertexAttributes[0].location = POSITION_LOCATION;
    vertexAttributes[0].buffer_slot = 0;
    vertexAttributes[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
    vertexAttributes[0].offset = POSITION_OFFSET;

    // Normal attribute (vec3 at offset 12)
    // Shader: float3 normal : TEXCOORD1
    vertexAttributes[1].location = NORMAL_LOCATION;
    vertexAttributes[1].buffer_slot = 0;
    vertexAttributes[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
    vertexAttributes[1].offset = NORMAL_OFFSET;

    // TexCoord attribute (vec2 at offset 24)
    // Shader: float2 uv : TEXCOORD2
    vertexAttributes[2].location = TEXCOORD_LOCATION;
    vertexAttributes[2].buffer_slot = 0;
    vertexAttributes[2].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
    vertexAttributes[2].offset = TEXCOORD_OFFSET;

    // Note: Color attribute (vec4 at offset 32) is not used by the toon shader
    // but is present in the Vertex struct from ModelLoader

    SDL_GPUVertexInputState inputState = {};
    inputState.vertex_buffer_descriptions = &vertexBufferDesc;
    inputState.num_vertex_buffers = 1;
    inputState.vertex_attributes = vertexAttributes;
    inputState.num_vertex_attributes = 3;

    return inputState;
}

bool ToonVertexLayout::validate() {
    // Validate against ModelLoader::Vertex struct layout
    // This catches mismatches at runtime (compile-time would be better with static_assert)

    bool valid = true;

    // Check stride matches Vertex struct size
    if (VERTEX_STRIDE != sizeof(Vertex)) {
        SDL_LogWarn(SDL_LOG_CATEGORY_GPU,
            "ToonVertexLayout: Stride mismatch! Expected %zu, got %d",
            sizeof(Vertex), VERTEX_STRIDE);
        valid = false;
    }

    // Check position offset (vec3 at start)
    if (POSITION_OFFSET != offsetof(Vertex, position)) {
        SDL_LogWarn(SDL_LOG_CATEGORY_GPU,
            "ToonVertexLayout: Position offset mismatch! Expected %zu, got %d",
            offsetof(Vertex, position), POSITION_OFFSET);
        valid = false;
    }

    // Check normal offset (vec3 after position)
    if (NORMAL_OFFSET != offsetof(Vertex, normal)) {
        SDL_LogWarn(SDL_LOG_CATEGORY_GPU,
            "ToonVertexLayout: Normal offset mismatch! Expected %zu, got %d",
            offsetof(Vertex, normal), NORMAL_OFFSET);
        valid = false;
    }

    // Check texCoord offset (vec2 after normal)
    if (TEXCOORD_OFFSET != offsetof(Vertex, texCoord)) {
        SDL_LogWarn(SDL_LOG_CATEGORY_GPU,
            "ToonVertexLayout: TexCoord offset mismatch! Expected %zu, got %d",
            offsetof(Vertex, texCoord), TEXCOORD_OFFSET);
        valid = false;
    }

    // Check color offset (vec4 after texCoord)
    if (COLOR_OFFSET != offsetof(Vertex, color)) {
        SDL_LogWarn(SDL_LOG_CATEGORY_GPU,
            "ToonVertexLayout: Color offset mismatch! Expected %zu, got %d",
            offsetof(Vertex, color), COLOR_OFFSET);
        valid = false;
    }

    if (valid) {
        SDL_LogDebug(SDL_LOG_CATEGORY_GPU, "ToonVertexLayout: Validation passed");
    }

    return valid;
}

// =============================================================================
// ToonPipeline Implementation
// =============================================================================

ToonPipeline::ToonPipeline(GPUDevice& device)
    : m_device(&device)
{
}

ToonPipeline::~ToonPipeline() {
    cleanup();
}

ToonPipeline::ToonPipeline(ToonPipeline&& other) noexcept
    : m_device(other.m_device)
    , m_opaquePipeline(other.m_opaquePipeline)
    , m_transparentPipeline(other.m_transparentPipeline)
    , m_opaqueWireframePipeline(other.m_opaqueWireframePipeline)
    , m_transparentWireframePipeline(other.m_transparentWireframePipeline)
    , m_lastError(std::move(other.m_lastError))
    , m_wireframeEnabled(other.m_wireframeEnabled)
    , m_colorFormat(other.m_colorFormat)
    , m_depthFormat(other.m_depthFormat)
    , m_config(other.m_config)
{
    other.m_opaquePipeline = nullptr;
    other.m_transparentPipeline = nullptr;
    other.m_opaqueWireframePipeline = nullptr;
    other.m_transparentWireframePipeline = nullptr;
}

ToonPipeline& ToonPipeline::operator=(ToonPipeline&& other) noexcept {
    if (this != &other) {
        cleanup();

        m_device = other.m_device;
        m_opaquePipeline = other.m_opaquePipeline;
        m_transparentPipeline = other.m_transparentPipeline;
        m_opaqueWireframePipeline = other.m_opaqueWireframePipeline;
        m_transparentWireframePipeline = other.m_transparentWireframePipeline;
        m_lastError = std::move(other.m_lastError);
        m_wireframeEnabled = other.m_wireframeEnabled;
        m_colorFormat = other.m_colorFormat;
        m_depthFormat = other.m_depthFormat;
        m_config = other.m_config;

        other.m_opaquePipeline = nullptr;
        other.m_transparentPipeline = nullptr;
        other.m_opaqueWireframePipeline = nullptr;
        other.m_transparentWireframePipeline = nullptr;
    }
    return *this;
}

bool ToonPipeline::create(
    SDL_GPUShader* vertexShader,
    SDL_GPUShader* fragmentShader,
    SDL_GPUTextureFormat colorFormat,
    SDL_GPUTextureFormat depthFormat,
    const ToonPipelineConfig& config)
{
    // Clean up existing pipelines
    cleanup();

    // Validate inputs
    if (!vertexShader) {
        m_lastError = "Vertex shader is null";
        return false;
    }
    if (!fragmentShader) {
        m_lastError = "Fragment shader is null";
        return false;
    }
    if (!m_device || !m_device->isValid()) {
        m_lastError = "GPU device is invalid";
        return false;
    }

    // Validate vertex layout matches ModelLoader::Vertex
    if (!ToonVertexLayout::validate()) {
        SDL_LogWarn(SDL_LOG_CATEGORY_GPU,
            "ToonPipeline: Vertex layout validation warning - check struct alignment");
    }

    // Store configuration
    m_colorFormat = colorFormat;
    m_depthFormat = depthFormat;
    m_config = config;

    // Create opaque pipeline
    m_opaquePipeline = createPipeline(
        vertexShader, fragmentShader,
        colorFormat, depthFormat,
        true,   // enableDepthWrite
        false,  // enableBlend
        config);

    if (!m_opaquePipeline) {
        // Error message already set by createPipeline
        return false;
    }

    // Create transparent pipeline
    m_transparentPipeline = createPipeline(
        vertexShader, fragmentShader,
        colorFormat, depthFormat,
        false,  // enableDepthWrite
        true,   // enableBlend
        config);

    if (!m_transparentPipeline) {
        // Clean up opaque pipeline on failure
        SDL_ReleaseGPUGraphicsPipeline(m_device->getHandle(), m_opaquePipeline);
        m_opaquePipeline = nullptr;
        return false;
    }

    // Create wireframe pipelines for debug rendering
    ToonPipelineConfig wireframeConfig = config;
    wireframeConfig.fillMode = SDL_GPU_FILLMODE_LINE;

    // Create opaque wireframe pipeline
    m_opaqueWireframePipeline = createPipeline(
        vertexShader, fragmentShader,
        colorFormat, depthFormat,
        true,   // enableDepthWrite
        false,  // enableBlend
        wireframeConfig);

    if (!m_opaqueWireframePipeline) {
        // Clean up on failure
        SDL_ReleaseGPUGraphicsPipeline(m_device->getHandle(), m_opaquePipeline);
        SDL_ReleaseGPUGraphicsPipeline(m_device->getHandle(), m_transparentPipeline);
        m_opaquePipeline = nullptr;
        m_transparentPipeline = nullptr;
        return false;
    }

    // Create transparent wireframe pipeline
    m_transparentWireframePipeline = createPipeline(
        vertexShader, fragmentShader,
        colorFormat, depthFormat,
        false,  // enableDepthWrite
        true,   // enableBlend
        wireframeConfig);

    if (!m_transparentWireframePipeline) {
        // Clean up on failure
        SDL_ReleaseGPUGraphicsPipeline(m_device->getHandle(), m_opaquePipeline);
        SDL_ReleaseGPUGraphicsPipeline(m_device->getHandle(), m_transparentPipeline);
        SDL_ReleaseGPUGraphicsPipeline(m_device->getHandle(), m_opaqueWireframePipeline);
        m_opaquePipeline = nullptr;
        m_transparentPipeline = nullptr;
        m_opaqueWireframePipeline = nullptr;
        return false;
    }

    SDL_LogInfo(SDL_LOG_CATEGORY_GPU,
        "ToonPipeline: Created opaque, transparent, and wireframe pipelines");

    return true;
}

void ToonPipeline::destroy() {
    cleanup();
}

bool ToonPipeline::isValid() const {
    return m_opaquePipeline != nullptr &&
           m_transparentPipeline != nullptr &&
           m_opaqueWireframePipeline != nullptr &&
           m_transparentWireframePipeline != nullptr;
}

SDL_GPUGraphicsPipeline* ToonPipeline::getOpaquePipeline() const {
    return m_opaquePipeline;
}

SDL_GPUGraphicsPipeline* ToonPipeline::getTransparentPipeline() const {
    return m_transparentPipeline;
}

// =============================================================================
// Wireframe Mode
// =============================================================================

bool ToonPipeline::isWireframeEnabled() const {
    return m_wireframeEnabled;
}

bool ToonPipeline::toggleWireframe() {
    m_wireframeEnabled = !m_wireframeEnabled;
    SDL_LogInfo(SDL_LOG_CATEGORY_GPU, "ToonPipeline: Wireframe mode %s",
        m_wireframeEnabled ? "ENABLED" : "DISABLED");
    return m_wireframeEnabled;
}

void ToonPipeline::setWireframeEnabled(bool enabled) {
    if (m_wireframeEnabled != enabled) {
        m_wireframeEnabled = enabled;
        SDL_LogInfo(SDL_LOG_CATEGORY_GPU, "ToonPipeline: Wireframe mode %s",
            m_wireframeEnabled ? "ENABLED" : "DISABLED");
    }
}

SDL_GPUGraphicsPipeline* ToonPipeline::getOpaqueWireframePipeline() const {
    return m_opaqueWireframePipeline;
}

SDL_GPUGraphicsPipeline* ToonPipeline::getTransparentWireframePipeline() const {
    return m_transparentWireframePipeline;
}

SDL_GPUGraphicsPipeline* ToonPipeline::getCurrentOpaquePipeline() const {
    return m_wireframeEnabled ? m_opaqueWireframePipeline : m_opaquePipeline;
}

SDL_GPUGraphicsPipeline* ToonPipeline::getCurrentTransparentPipeline() const {
    return m_wireframeEnabled ? m_transparentWireframePipeline : m_transparentPipeline;
}

const std::string& ToonPipeline::getLastError() const {
    return m_lastError;
}

void ToonPipeline::logConfiguration() const {
    SDL_LogInfo(SDL_LOG_CATEGORY_GPU, "ToonPipeline Configuration:");
    SDL_LogInfo(SDL_LOG_CATEGORY_GPU, "  Color format: %d", static_cast<int>(m_colorFormat));
    SDL_LogInfo(SDL_LOG_CATEGORY_GPU, "  Depth format: %d", static_cast<int>(m_depthFormat));
    SDL_LogInfo(SDL_LOG_CATEGORY_GPU, "  Cull mode: %s",
        m_config.cullMode == SDL_GPU_CULLMODE_BACK ? "BACK" :
        m_config.cullMode == SDL_GPU_CULLMODE_FRONT ? "FRONT" : "NONE");
    SDL_LogInfo(SDL_LOG_CATEGORY_GPU, "  Front face: %s",
        m_config.frontFace == SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE ? "CCW" : "CW");
    SDL_LogInfo(SDL_LOG_CATEGORY_GPU, "  Fill mode: %s",
        m_config.fillMode == SDL_GPU_FILLMODE_FILL ? "FILL" : "LINE");
    SDL_LogInfo(SDL_LOG_CATEGORY_GPU, "  Emissive MRT: %s",
        m_config.enableEmissiveMRT ? "enabled" : "disabled");
    SDL_LogInfo(SDL_LOG_CATEGORY_GPU, "  Wireframe mode: %s",
        m_wireframeEnabled ? "enabled" : "disabled");

    SDL_LogInfo(SDL_LOG_CATEGORY_GPU, "  Opaque pipeline:");
    SDL_LogInfo(SDL_LOG_CATEGORY_GPU, "    Depth test: LESS");
    SDL_LogInfo(SDL_LOG_CATEGORY_GPU, "    Depth write: enabled");
    SDL_LogInfo(SDL_LOG_CATEGORY_GPU, "    Blend: disabled");

    SDL_LogInfo(SDL_LOG_CATEGORY_GPU, "  Transparent pipeline:");
    SDL_LogInfo(SDL_LOG_CATEGORY_GPU, "    Depth test: LESS");
    SDL_LogInfo(SDL_LOG_CATEGORY_GPU, "    Depth write: disabled");
    SDL_LogInfo(SDL_LOG_CATEGORY_GPU, "    Blend: srcAlpha, 1-srcAlpha");

    SDL_LogInfo(SDL_LOG_CATEGORY_GPU, "  Wireframe pipelines: available (toggle with 'F' key)");
}

SDL_GPUColorTargetDescription ToonPipeline::getOpaqueColorTarget(
    SDL_GPUTextureFormat format)
{
    SDL_GPUColorTargetDescription desc = {};
    desc.format = format;
    desc.blend_state = BlendState::opaque();
    return desc;
}

SDL_GPUColorTargetDescription ToonPipeline::getTransparentColorTarget(
    SDL_GPUTextureFormat format)
{
    SDL_GPUColorTargetDescription desc = {};
    desc.format = format;
    desc.blend_state = BlendState::transparent();
    return desc;
}

SDL_GPUGraphicsPipeline* ToonPipeline::createPipeline(
    SDL_GPUShader* vertexShader,
    SDL_GPUShader* fragmentShader,
    SDL_GPUTextureFormat colorFormat,
    SDL_GPUTextureFormat depthFormat,
    bool enableDepthWrite,
    bool enableBlend,
    const ToonPipelineConfig& config)
{
    // Get vertex input state
    SDL_GPUVertexInputState vertexInputState = ToonVertexLayout::getVertexInputState();

    // Configure rasterizer state
    SDL_GPURasterizerState rasterizerState = {};
    rasterizerState.cull_mode = config.cullMode;
    rasterizerState.front_face = config.frontFace;
    rasterizerState.fill_mode = config.fillMode;
    rasterizerState.depth_bias_constant_factor = config.depthBiasConstant;
    rasterizerState.depth_bias_slope_factor = config.depthBiasSlope;
    rasterizerState.depth_bias_clamp = config.depthBiasClamp;
    rasterizerState.enable_depth_bias = (config.depthBiasConstant != 0.0f ||
                                          config.depthBiasSlope != 0.0f);
    rasterizerState.enable_depth_clip = true;

    // Configure depth/stencil state using DepthState factory
    SDL_GPUDepthStencilState depthStencilState;
    if (enableDepthWrite) {
        // Opaque pass: depth test ON, depth write ON, compare LESS
        depthStencilState = DepthState::opaque();
    } else {
        // Transparent pass: depth test ON, depth write OFF, compare LESS
        depthStencilState = DepthState::transparent();
    }

    // Configure color target
    SDL_GPUColorTargetDescription colorTargetDesc = {};
    colorTargetDesc.format = colorFormat;
    if (enableBlend) {
        colorTargetDesc.blend_state = BlendState::transparent();
    } else {
        colorTargetDesc.blend_state = BlendState::opaque();
    }

    // Create the pipeline
    SDL_GPUGraphicsPipelineCreateInfo createInfo = {};
    createInfo.vertex_shader = vertexShader;
    createInfo.fragment_shader = fragmentShader;
    createInfo.vertex_input_state = vertexInputState;
    createInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    createInfo.rasterizer_state = rasterizerState;
    createInfo.multisample_state.sample_count = SDL_GPU_SAMPLECOUNT_1;
    createInfo.multisample_state.sample_mask = 0xFFFFFFFF;
    createInfo.depth_stencil_state = depthStencilState;

    // Color target attachment
    createInfo.target_info.color_target_descriptions = &colorTargetDesc;
    createInfo.target_info.num_color_targets = 1;

    // Depth target attachment
    createInfo.target_info.depth_stencil_format = depthFormat;
    createInfo.target_info.has_depth_stencil_target = true;

    // Create the pipeline
    SDL_GPUGraphicsPipeline* pipeline = SDL_CreateGPUGraphicsPipeline(
        m_device->getHandle(), &createInfo);

    if (!pipeline) {
        m_lastError = "Failed to create graphics pipeline: ";
        m_lastError += SDL_GetError();
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "%s", m_lastError.c_str());
    }

    return pipeline;
}

void ToonPipeline::cleanup() {
    if (m_device && m_device->isValid()) {
        if (m_opaquePipeline) {
            SDL_ReleaseGPUGraphicsPipeline(m_device->getHandle(), m_opaquePipeline);
            m_opaquePipeline = nullptr;
        }
        if (m_transparentPipeline) {
            SDL_ReleaseGPUGraphicsPipeline(m_device->getHandle(), m_transparentPipeline);
            m_transparentPipeline = nullptr;
        }
        if (m_opaqueWireframePipeline) {
            SDL_ReleaseGPUGraphicsPipeline(m_device->getHandle(), m_opaqueWireframePipeline);
            m_opaqueWireframePipeline = nullptr;
        }
        if (m_transparentWireframePipeline) {
            SDL_ReleaseGPUGraphicsPipeline(m_device->getHandle(), m_transparentWireframePipeline);
            m_transparentWireframePipeline = nullptr;
        }
    }
    m_colorFormat = SDL_GPU_TEXTUREFORMAT_INVALID;
    m_depthFormat = SDL_GPU_TEXTUREFORMAT_INVALID;
    m_wireframeEnabled = false;
}

} // namespace sims3000
