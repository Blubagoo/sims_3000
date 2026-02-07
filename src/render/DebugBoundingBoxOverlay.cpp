/**
 * @file DebugBoundingBoxOverlay.cpp
 * @brief Debug bounding box overlay implementation.
 */

#include "sims3000/render/DebugBoundingBoxOverlay.h"
#include "sims3000/render/GPUDevice.h"
#include "sims3000/render/ShaderCompiler.h"
#include "sims3000/render/CameraUniforms.h"
#include <SDL3/SDL_log.h>
#include <algorithm>
#include <cmath>
#include <cstring>

namespace sims3000 {

// Maximum vertices per frame (each box = 24 vertices for 12 lines)
constexpr std::uint32_t VERTICES_PER_BOX = 24;
constexpr std::uint32_t DEFAULT_BUFFER_CAPACITY = 1000;  // boxes

// =============================================================================
// Construction / Destruction
// =============================================================================

DebugBoundingBoxOverlay::DebugBoundingBoxOverlay(
    GPUDevice& device,
    SDL_GPUTextureFormat colorFormat,
    SDL_GPUTextureFormat depthFormat)
    : m_device(&device)
    , m_colorFormat(colorFormat)
    , m_depthFormat(depthFormat)
{
    if (!createResources()) {
        SDL_LogError(SDL_LOG_CATEGORY_GPU,
                     "DebugBoundingBoxOverlay: Failed to create resources: %s",
                     m_lastError.c_str());
    }
}

DebugBoundingBoxOverlay::~DebugBoundingBoxOverlay() {
    releaseResources();
}

DebugBoundingBoxOverlay::DebugBoundingBoxOverlay(DebugBoundingBoxOverlay&& other) noexcept
    : m_device(other.m_device)
    , m_colorFormat(other.m_colorFormat)
    , m_depthFormat(other.m_depthFormat)
    , m_config(other.m_config)
    , m_enabled(other.m_enabled)
    , m_pipeline(other.m_pipeline)
    , m_vertexShader(other.m_vertexShader)
    , m_fragmentShader(other.m_fragmentShader)
    , m_vertexBuffer(other.m_vertexBuffer)
    , m_transferBuffer(other.m_transferBuffer)
    , m_vertexBufferCapacity(other.m_vertexBufferCapacity)
    , m_renderedBoxCount(other.m_renderedBoxCount)
    , m_lastError(std::move(other.m_lastError))
{
    other.m_device = nullptr;
    other.m_pipeline = nullptr;
    other.m_vertexShader = nullptr;
    other.m_fragmentShader = nullptr;
    other.m_vertexBuffer = nullptr;
    other.m_transferBuffer = nullptr;
}

DebugBoundingBoxOverlay& DebugBoundingBoxOverlay::operator=(DebugBoundingBoxOverlay&& other) noexcept {
    if (this != &other) {
        releaseResources();

        m_device = other.m_device;
        m_colorFormat = other.m_colorFormat;
        m_depthFormat = other.m_depthFormat;
        m_config = other.m_config;
        m_enabled = other.m_enabled;
        m_pipeline = other.m_pipeline;
        m_vertexShader = other.m_vertexShader;
        m_fragmentShader = other.m_fragmentShader;
        m_vertexBuffer = other.m_vertexBuffer;
        m_transferBuffer = other.m_transferBuffer;
        m_vertexBufferCapacity = other.m_vertexBufferCapacity;
        m_renderedBoxCount = other.m_renderedBoxCount;
        m_lastError = std::move(other.m_lastError);

        other.m_device = nullptr;
        other.m_pipeline = nullptr;
        other.m_vertexShader = nullptr;
        other.m_fragmentShader = nullptr;
        other.m_vertexBuffer = nullptr;
        other.m_transferBuffer = nullptr;
    }
    return *this;
}

// =============================================================================
// Public Interface
// =============================================================================

bool DebugBoundingBoxOverlay::isValid() const {
    return m_device != nullptr && m_pipeline != nullptr && m_vertexBuffer != nullptr;
}

void DebugBoundingBoxOverlay::setEnabled(bool enabled) {
    m_enabled = enabled;
    SDL_LogInfo(SDL_LOG_CATEGORY_GPU, "DebugBoundingBoxOverlay: %s",
                enabled ? "Enabled" : "Disabled");
}

void DebugBoundingBoxOverlay::toggle() {
    setEnabled(!m_enabled);
}

void DebugBoundingBoxOverlay::setConfig(const DebugBBoxConfig& config) {
    m_config = config;
}

void DebugBoundingBoxOverlay::setVisibleColor(const glm::vec4& color) {
    m_config.visibleColor = color;
}

void DebugBoundingBoxOverlay::setCulledColor(const glm::vec4& color) {
    m_config.culledColor = color;
}

void DebugBoundingBoxOverlay::setShowCulledBoxes(bool show) {
    m_config.showCulledBoxes = show;
}

bool DebugBoundingBoxOverlay::render(
    SDL_GPUCommandBuffer* cmdBuffer,
    SDL_GPUTexture* outputTexture,
    SDL_GPUTexture* depthTexture,
    std::uint32_t width,
    std::uint32_t height,
    const CameraUniforms& camera,
    const std::vector<BoundingBoxEntry>& entries)
{
    m_renderedBoxCount = 0;

    if (!isValid()) {
        m_lastError = "DebugBoundingBoxOverlay not valid";
        return false;
    }

    if (!m_enabled) {
        return true;  // Not an error, just disabled
    }

    if (!cmdBuffer || !outputTexture) {
        m_lastError = "Invalid input parameters";
        return false;
    }

    if (entries.empty()) {
        return true;  // Nothing to render
    }

    // Count boxes to render
    std::uint32_t boxCount = 0;
    for (const auto& entry : entries) {
        if (entry.isVisible || m_config.showCulledBoxes) {
            boxCount++;
        }
    }

    if (boxCount == 0) {
        return true;
    }

    // Limit to max boxes
    boxCount = std::min(boxCount, m_config.maxBoxes);

    // Generate vertices for all boxes
    std::vector<DebugBBoxVertex> vertices;
    vertices.reserve(static_cast<std::size_t>(boxCount) * VERTICES_PER_BOX);

    std::uint32_t renderedCount = 0;
    for (const auto& entry : entries) {
        if (renderedCount >= boxCount) break;

        if (entry.isVisible) {
            generateBoxVertices(entry.bounds, m_config.visibleColor, vertices);
            renderedCount++;
        } else if (m_config.showCulledBoxes) {
            generateBoxVertices(entry.bounds, m_config.culledColor, vertices);
            renderedCount++;
        }
    }

    if (vertices.empty()) {
        return true;
    }

    // Ensure buffer is large enough
    std::uint32_t requiredCapacity = static_cast<std::uint32_t>(vertices.size());
    if (requiredCapacity > m_vertexBufferCapacity) {
        // Need to recreate buffer with larger size
        SDL_GPUDevice* device = m_device->getHandle();

        if (m_vertexBuffer) {
            SDL_ReleaseGPUBuffer(device, m_vertexBuffer);
            m_vertexBuffer = nullptr;
        }
        if (m_transferBuffer) {
            SDL_ReleaseGPUTransferBuffer(device, m_transferBuffer);
            m_transferBuffer = nullptr;
        }

        // Round up to next power of 2
        std::uint32_t newCapacity = 1;
        while (newCapacity < requiredCapacity) {
            newCapacity *= 2;
        }
        m_vertexBufferCapacity = newCapacity;

        if (!createVertexBuffer()) {
            return false;
        }
    }

    SDL_GPUDevice* device = m_device->getHandle();

    // Map transfer buffer and copy vertex data
    void* mappedData = SDL_MapGPUTransferBuffer(device, m_transferBuffer, false);
    if (!mappedData) {
        m_lastError = std::string("Failed to map transfer buffer: ") + SDL_GetError();
        return false;
    }

    std::memcpy(mappedData, vertices.data(), vertices.size() * sizeof(DebugBBoxVertex));
    SDL_UnmapGPUTransferBuffer(device, m_transferBuffer);

    // Create a copy pass to upload vertex data
    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(cmdBuffer);
    if (!copyPass) {
        m_lastError = std::string("Failed to begin copy pass: ") + SDL_GetError();
        return false;
    }

    SDL_GPUTransferBufferLocation src = {};
    src.transfer_buffer = m_transferBuffer;
    src.offset = 0;

    SDL_GPUBufferRegion dst = {};
    dst.buffer = m_vertexBuffer;
    dst.offset = 0;
    dst.size = static_cast<Uint32>(vertices.size() * sizeof(DebugBBoxVertex));

    SDL_UploadToGPUBuffer(copyPass, &src, &dst, false);
    SDL_EndGPUCopyPass(copyPass);

    // Prepare uniform buffer data
    DebugBBoxUBO uboData;
    uboData.viewProjection = camera.getViewProjectionMatrix();

    // Configure color target - load existing content for overlay
    SDL_GPUColorTargetInfo colorTarget = {};
    colorTarget.texture = outputTexture;
    colorTarget.load_op = SDL_GPU_LOADOP_LOAD;  // Preserve scene content
    colorTarget.store_op = SDL_GPU_STOREOP_STORE;

    // Configure depth target for depth testing
    SDL_GPUDepthStencilTargetInfo depthTarget = {};
    depthTarget.texture = depthTexture;
    depthTarget.load_op = SDL_GPU_LOADOP_LOAD;  // Keep existing depth
    depthTarget.store_op = SDL_GPU_STOREOP_DONT_CARE;  // Don't need to preserve

    // Begin render pass
    SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(
        cmdBuffer, &colorTarget, 1, &depthTarget);
    if (!renderPass) {
        m_lastError = std::string("Failed to begin render pass: ") + SDL_GetError();
        return false;
    }

    // Bind pipeline
    SDL_BindGPUGraphicsPipeline(renderPass, m_pipeline);

    // Push uniform buffer data
    SDL_PushGPUVertexUniformData(cmdBuffer, 0, &uboData, sizeof(uboData));

    // Bind vertex buffer
    SDL_GPUBufferBinding vertexBinding = {};
    vertexBinding.buffer = m_vertexBuffer;
    vertexBinding.offset = 0;
    SDL_BindGPUVertexBuffers(renderPass, 0, &vertexBinding, 1);

    // Set viewport
    SDL_GPUViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.w = static_cast<float>(width);
    viewport.h = static_cast<float>(height);
    viewport.min_depth = 0.0f;
    viewport.max_depth = 1.0f;
    SDL_SetGPUViewport(renderPass, &viewport);

    // Draw all lines
    std::uint32_t vertexCount = static_cast<std::uint32_t>(vertices.size());
    SDL_DrawGPUPrimitives(renderPass, vertexCount, 1, 0, 0);

    // End render pass
    SDL_EndGPURenderPass(renderPass);

    m_renderedBoxCount = renderedCount;
    return true;
}

// =============================================================================
// Private Implementation
// =============================================================================

void DebugBoundingBoxOverlay::generateBoxVertices(
    const AABB& bounds,
    const glm::vec4& color,
    std::vector<DebugBBoxVertex>& outVertices)
{
    // 8 corners of the box
    glm::vec3 corners[8] = {
        {bounds.min.x, bounds.min.y, bounds.min.z},  // 0: min
        {bounds.max.x, bounds.min.y, bounds.min.z},  // 1
        {bounds.max.x, bounds.max.y, bounds.min.z},  // 2
        {bounds.min.x, bounds.max.y, bounds.min.z},  // 3
        {bounds.min.x, bounds.min.y, bounds.max.z},  // 4
        {bounds.max.x, bounds.min.y, bounds.max.z},  // 5
        {bounds.max.x, bounds.max.y, bounds.max.z},  // 6: max
        {bounds.min.x, bounds.max.y, bounds.max.z},  // 7
    };

    // 12 edges of the box (as line pairs)
    // Bottom face
    outVertices.push_back({corners[0], color});
    outVertices.push_back({corners[1], color});
    outVertices.push_back({corners[1], color});
    outVertices.push_back({corners[2], color});
    outVertices.push_back({corners[2], color});
    outVertices.push_back({corners[3], color});
    outVertices.push_back({corners[3], color});
    outVertices.push_back({corners[0], color});

    // Top face
    outVertices.push_back({corners[4], color});
    outVertices.push_back({corners[5], color});
    outVertices.push_back({corners[5], color});
    outVertices.push_back({corners[6], color});
    outVertices.push_back({corners[6], color});
    outVertices.push_back({corners[7], color});
    outVertices.push_back({corners[7], color});
    outVertices.push_back({corners[4], color});

    // Vertical edges
    outVertices.push_back({corners[0], color});
    outVertices.push_back({corners[4], color});
    outVertices.push_back({corners[1], color});
    outVertices.push_back({corners[5], color});
    outVertices.push_back({corners[2], color});
    outVertices.push_back({corners[6], color});
    outVertices.push_back({corners[3], color});
    outVertices.push_back({corners[7], color});
}

bool DebugBoundingBoxOverlay::createResources() {
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

    // Create vertex buffer
    m_vertexBufferCapacity = DEFAULT_BUFFER_CAPACITY * VERTICES_PER_BOX;
    if (!createVertexBuffer()) {
        releaseResources();
        return false;
    }

    // Create graphics pipeline for line rendering
    {
        // Vertex attribute: position (vec3)
        SDL_GPUVertexAttribute posAttr = {};
        posAttr.location = 0;
        posAttr.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
        posAttr.offset = 0;
        posAttr.buffer_slot = 0;

        // Vertex attribute: color (vec4)
        SDL_GPUVertexAttribute colorAttr = {};
        colorAttr.location = 1;
        colorAttr.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4;
        colorAttr.offset = sizeof(glm::vec3);  // 12 bytes
        colorAttr.buffer_slot = 0;

        SDL_GPUVertexAttribute attributes[2] = {posAttr, colorAttr};

        // Vertex buffer description
        SDL_GPUVertexBufferDescription vertexBufferDesc = {};
        vertexBufferDesc.slot = 0;
        vertexBufferDesc.pitch = static_cast<Uint32>(DebugBBoxVertex::stride());
        vertexBufferDesc.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
        vertexBufferDesc.instance_step_rate = 0;

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

        // Vertex input state
        pipelineInfo.vertex_input_state.num_vertex_buffers = 1;
        pipelineInfo.vertex_input_state.vertex_buffer_descriptions = &vertexBufferDesc;
        pipelineInfo.vertex_input_state.num_vertex_attributes = 2;
        pipelineInfo.vertex_input_state.vertex_attributes = attributes;

        // Primitive state - LINE LIST for wireframe
        pipelineInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_LINELIST;

        // Rasterizer state
        pipelineInfo.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
        pipelineInfo.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;  // No culling for lines
        pipelineInfo.rasterizer_state.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;

        // Depth testing (read depth but don't write)
        pipelineInfo.depth_stencil_state.enable_depth_test = true;
        pipelineInfo.depth_stencil_state.enable_depth_write = false;
        pipelineInfo.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_LESS_OR_EQUAL;
        pipelineInfo.depth_stencil_state.enable_stencil_test = false;

        // Color targets
        pipelineInfo.target_info.num_color_targets = 1;
        pipelineInfo.target_info.color_target_descriptions = &colorTargetDesc;
        pipelineInfo.target_info.has_depth_stencil_target = true;
        pipelineInfo.target_info.depth_stencil_format = m_depthFormat;

        m_pipeline = SDL_CreateGPUGraphicsPipeline(device, &pipelineInfo);
        if (!m_pipeline) {
            m_lastError = std::string("Failed to create bbox pipeline: ") + SDL_GetError();
            releaseResources();
            return false;
        }
    }

    SDL_LogInfo(SDL_LOG_CATEGORY_GPU, "DebugBoundingBoxOverlay: Created resources successfully");
    return true;
}

void DebugBoundingBoxOverlay::releaseResources() {
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

    if (m_vertexBuffer) {
        SDL_ReleaseGPUBuffer(device, m_vertexBuffer);
        m_vertexBuffer = nullptr;
    }

    if (m_transferBuffer) {
        SDL_ReleaseGPUTransferBuffer(device, m_transferBuffer);
        m_transferBuffer = nullptr;
    }
}

bool DebugBoundingBoxOverlay::loadShaders() {
    ShaderCompiler compiler(*m_device);

    // Load bbox vertex shader
    ShaderResources vertResources;
    vertResources.numUniformBuffers = 1;  // DebugBBoxUBO
    vertResources.numSamplers = 0;
    vertResources.numStorageBuffers = 0;
    vertResources.numStorageTextures = 0;

    ShaderLoadResult vertResult = compiler.loadShader(
        "shaders/debug_bbox.vert",
        ShaderStage::Vertex,
        "main",
        vertResources
    );

    if (!vertResult.isValid()) {
        m_lastError = "Failed to load debug bbox vertex shader: " + vertResult.error.message;
        return false;
    }
    m_vertexShader = vertResult.shader;

    if (vertResult.usedFallback) {
        SDL_LogWarn(SDL_LOG_CATEGORY_GPU,
                    "DebugBoundingBoxOverlay: Using fallback vertex shader");
    }

    // Load bbox fragment shader
    ShaderResources fragResources;
    fragResources.numUniformBuffers = 0;  // No fragment uniforms needed
    fragResources.numSamplers = 0;
    fragResources.numStorageBuffers = 0;
    fragResources.numStorageTextures = 0;

    ShaderLoadResult fragResult = compiler.loadShader(
        "shaders/debug_bbox.frag",
        ShaderStage::Fragment,
        "main",
        fragResources
    );

    if (!fragResult.isValid()) {
        m_lastError = "Failed to load debug bbox fragment shader: " + fragResult.error.message;
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
                    "DebugBoundingBoxOverlay: Using fallback fragment shader");
    }

    SDL_LogInfo(SDL_LOG_CATEGORY_GPU, "DebugBoundingBoxOverlay: Shaders loaded successfully");
    return true;
}

bool DebugBoundingBoxOverlay::createVertexBuffer() {
    SDL_GPUDevice* device = m_device->getHandle();

    // Create GPU vertex buffer
    SDL_GPUBufferCreateInfo bufferInfo = {};
    bufferInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    bufferInfo.size = m_vertexBufferCapacity * sizeof(DebugBBoxVertex);

    m_vertexBuffer = SDL_CreateGPUBuffer(device, &bufferInfo);
    if (!m_vertexBuffer) {
        m_lastError = std::string("Failed to create vertex buffer: ") + SDL_GetError();
        return false;
    }

    // Create transfer buffer for uploads
    SDL_GPUTransferBufferCreateInfo transferInfo = {};
    transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transferInfo.size = m_vertexBufferCapacity * sizeof(DebugBBoxVertex);

    m_transferBuffer = SDL_CreateGPUTransferBuffer(device, &transferInfo);
    if (!m_transferBuffer) {
        m_lastError = std::string("Failed to create transfer buffer: ") + SDL_GetError();
        SDL_ReleaseGPUBuffer(device, m_vertexBuffer);
        m_vertexBuffer = nullptr;
        return false;
    }

    return true;
}

} // namespace sims3000
