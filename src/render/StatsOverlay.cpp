/**
 * @file StatsOverlay.cpp
 * @brief StatsOverlay implementation.
 */

#include "sims3000/render/StatsOverlay.h"
#include "sims3000/render/GPUDevice.h"
#include "sims3000/render/Window.h"
#include "sims3000/render/MainRenderPass.h"
#include "sims3000/app/FrameStats.h"

#include <cstdio>
#include <cstring>
#include <algorithm>

namespace sims3000 {

// Vertex structure for background quad
struct StatsQuadVertex {
    float x, y;       // Position (0-1 normalized screen space)
    float r, g, b, a; // Color
};

// Vertices per quad (2 triangles = 6 vertices)
static constexpr int VERTICES_PER_QUAD = 6;

StatsOverlay::StatsOverlay(GPUDevice& device, Window& window)
    : StatsOverlay(device, window, StatsOverlayConfig{}) {
}

StatsOverlay::StatsOverlay(GPUDevice& device, Window& window, const StatsOverlayConfig& config)
    : m_device(&device)
    , m_window(&window)
    , m_config(config) {
    if (!initialize()) {
        releaseResources();
    }
}

StatsOverlay::~StatsOverlay() {
    releaseResources();
}

StatsOverlay::StatsOverlay(StatsOverlay&& other) noexcept
    : m_device(other.m_device)
    , m_window(other.m_window)
    , m_config(other.m_config)
    , m_enabled(other.m_enabled)
    , m_stats(other.m_stats)
    , m_textEngine(other.m_textEngine)
    , m_font(other.m_font)
    , m_textObjects(other.m_textObjects)
    , m_textStrings(std::move(other.m_textStrings))
    , m_bgPipeline(other.m_bgPipeline)
    , m_bgVertexBuffer(other.m_bgVertexBuffer)
    , m_bgTransferBuffer(other.m_bgTransferBuffer)
    , m_textSampler(other.m_textSampler)
    , m_swapchainFormat(other.m_swapchainFormat)
    , m_lastError(std::move(other.m_lastError)) {
    // Clear other's pointers
    other.m_device = nullptr;
    other.m_window = nullptr;
    other.m_textEngine = nullptr;
    other.m_font = nullptr;
    other.m_textObjects.fill(nullptr);
    other.m_bgPipeline = nullptr;
    other.m_bgVertexBuffer = nullptr;
    other.m_bgTransferBuffer = nullptr;
    other.m_textSampler = nullptr;
}

StatsOverlay& StatsOverlay::operator=(StatsOverlay&& other) noexcept {
    if (this != &other) {
        releaseResources();

        m_device = other.m_device;
        m_window = other.m_window;
        m_config = other.m_config;
        m_enabled = other.m_enabled;
        m_stats = other.m_stats;
        m_textEngine = other.m_textEngine;
        m_font = other.m_font;
        m_textObjects = other.m_textObjects;
        m_textStrings = std::move(other.m_textStrings);
        m_bgPipeline = other.m_bgPipeline;
        m_bgVertexBuffer = other.m_bgVertexBuffer;
        m_bgTransferBuffer = other.m_bgTransferBuffer;
        m_textSampler = other.m_textSampler;
        m_swapchainFormat = other.m_swapchainFormat;
        m_lastError = std::move(other.m_lastError);

        // Clear other's pointers
        other.m_device = nullptr;
        other.m_window = nullptr;
        other.m_textEngine = nullptr;
        other.m_font = nullptr;
        other.m_textObjects.fill(nullptr);
        other.m_bgPipeline = nullptr;
        other.m_bgVertexBuffer = nullptr;
        other.m_bgTransferBuffer = nullptr;
        other.m_textSampler = nullptr;
    }
    return *this;
}

bool StatsOverlay::isValid() const {
    return m_device != nullptr &&
           m_window != nullptr &&
           m_textEngine != nullptr &&
           m_font != nullptr &&
           m_bgPipeline != nullptr;
}

void StatsOverlay::setEnabled(bool enabled) {
    m_enabled = enabled;
}

void StatsOverlay::toggle() {
    m_enabled = !m_enabled;
}

void StatsOverlay::update(const FrameStats& frameStats, const MainRenderPassStats& renderStats) {
    m_stats.fps = frameStats.getFPS();
    m_stats.frameTimeMs = frameStats.getFrameTimeMs();
    m_stats.minFrameTimeMs = frameStats.getMinFrameTimeMs();
    m_stats.maxFrameTimeMs = frameStats.getMaxFrameTimeMs();
    m_stats.drawCalls = renderStats.totalDrawCalls;
    m_stats.triangles = renderStats.totalTriangles;
    m_stats.totalFrames = frameStats.getTotalFrames();

    updateTextContent();
}

void StatsOverlay::update(const StatsData& stats) {
    m_stats = stats;
    updateTextContent();
}

bool StatsOverlay::render(
    SDL_GPUCommandBuffer* cmdBuffer,
    SDL_GPUTexture* outputTexture,
    std::uint32_t width,
    std::uint32_t height) {

    if (!isValid() || !m_enabled || cmdBuffer == nullptr || outputTexture == nullptr) {
        return false;
    }

    // Calculate text dimensions and background size
    float lineHeight = m_config.fontSize * m_config.lineSpacing;
    float totalHeight = LINE_COUNT * lineHeight + m_config.paddingY * 2;

    // Calculate max text width (estimate based on longest line)
    float maxWidth = 200.0f;  // Default width, will be calculated from actual text
    for (int i = 0; i < LINE_COUNT; ++i) {
        if (m_textObjects[i]) {
            int w = 0, h = 0;
            TTF_GetTextSize(m_textObjects[i], &w, &h);
            maxWidth = std::max(maxWidth, static_cast<float>(w));
        }
    }
    float totalWidth = maxWidth + m_config.paddingX * 2;

    // Calculate position based on config
    float bgX = m_config.offsetX;
    float bgY = m_config.offsetY;
    if (m_config.position == 1 || m_config.position == 3) {
        bgX = static_cast<float>(width) - totalWidth - m_config.offsetX;
    }
    if (m_config.position == 2 || m_config.position == 3) {
        bgY = static_cast<float>(height) - totalHeight - m_config.offsetY;
    }

    // Normalize coordinates to 0-1 range
    float nx = bgX / static_cast<float>(width);
    float ny = bgY / static_cast<float>(height);
    float nw = totalWidth / static_cast<float>(width);
    float nh = totalHeight / static_cast<float>(height);

    // Build background quad vertices
    float bgR = m_config.bgR / 255.0f;
    float bgG = m_config.bgG / 255.0f;
    float bgB = m_config.bgB / 255.0f;
    float bgA = m_config.bgA / 255.0f;

    StatsQuadVertex vertices[VERTICES_PER_QUAD];
    // Triangle 1: top-left, bottom-left, bottom-right
    vertices[0] = {nx, ny, bgR, bgG, bgB, bgA};
    vertices[1] = {nx, ny + nh, bgR, bgG, bgB, bgA};
    vertices[2] = {nx + nw, ny + nh, bgR, bgG, bgB, bgA};
    // Triangle 2: top-left, bottom-right, top-right
    vertices[3] = {nx, ny, bgR, bgG, bgB, bgA};
    vertices[4] = {nx + nw, ny + nh, bgR, bgG, bgB, bgA};
    vertices[5] = {nx + nw, ny, bgR, bgG, bgB, bgA};

    // Upload background vertices
    void* mapped = SDL_MapGPUTransferBuffer(m_device->getHandle(), m_bgTransferBuffer, false);
    if (mapped) {
        std::memcpy(mapped, vertices, sizeof(vertices));
        SDL_UnmapGPUTransferBuffer(m_device->getHandle(), m_bgTransferBuffer);

        // Copy to GPU buffer
        SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(cmdBuffer);
        if (copyPass) {
            SDL_GPUTransferBufferLocation src = {};
            src.transfer_buffer = m_bgTransferBuffer;
            src.offset = 0;

            SDL_GPUBufferRegion dst = {};
            dst.buffer = m_bgVertexBuffer;
            dst.offset = 0;
            dst.size = sizeof(vertices);

            SDL_UploadToGPUBuffer(copyPass, &src, &dst, false);
            SDL_EndGPUCopyPass(copyPass);
        }
    }

    // Begin render pass - preserve existing content
    SDL_GPUColorTargetInfo colorTarget = {};
    colorTarget.texture = outputTexture;
    colorTarget.load_op = SDL_GPU_LOADOP_LOAD;  // Preserve existing content
    colorTarget.store_op = SDL_GPU_STOREOP_STORE;

    SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(cmdBuffer, &colorTarget, 1, nullptr);
    if (!pass) {
        m_lastError = "Failed to begin render pass for stats overlay";
        return false;
    }

    // Draw background quad
    SDL_BindGPUGraphicsPipeline(pass, m_bgPipeline);

    SDL_GPUBufferBinding vertexBinding = {};
    vertexBinding.buffer = m_bgVertexBuffer;
    vertexBinding.offset = 0;
    SDL_BindGPUVertexBuffers(pass, 0, &vertexBinding, 1);

    SDL_DrawGPUPrimitives(pass, VERTICES_PER_QUAD, 1, 0, 0);

    SDL_EndGPURenderPass(pass);

    // Draw text using SDL_ttf GPU text drawing
    // TTF_GPUAtlasDrawSequence provides atlas data for each text object
    // Text is positioned relative to background
    float textX = bgX + m_config.paddingX;
    float textY = bgY + m_config.paddingY;

    for (int i = 0; i < LINE_COUNT; ++i) {
        if (m_textObjects[i]) {
            // Get GPU text draw data
            TTF_GPUAtlasDrawSequence* sequence = TTF_GetGPUTextDrawData(m_textObjects[i]);

            // Process draw sequences (atlas textures and vertex data)
            // Note: Full text rendering requires a text pipeline with texture sampling
            // For now, we validate the API works - full implementation follows POC-6 pattern
            while (sequence) {
                if (sequence->atlas_texture && sequence->num_vertices > 0) {
                    // In full implementation: upload vertices and draw with atlas texture
                    // The sequence contains xy positions, uv coords, and indices
                }
                sequence = sequence->next;
            }

            (void)textX;  // Suppress unused warning
            (void)textY;
        }
    }

    (void)lineHeight;  // Suppress unused warning

    return true;
}

void StatsOverlay::setConfig(const StatsOverlayConfig& config) {
    bool fontSizeChanged = (config.fontSize != m_config.fontSize);
    m_config = config;

    if (fontSizeChanged && m_font) {
        // Recreate font with new size
        TTF_CloseFont(m_font);
        m_font = nullptr;
        loadFont();
        createTextObjects();
    }
}

bool StatsOverlay::initialize() {
    if (!m_device || !m_device->isValid() || !m_window || !m_window->isValid()) {
        m_lastError = "Invalid device or window";
        return false;
    }

    // Get swapchain format
    m_swapchainFormat = m_window->getSwapchainTextureFormat();
    if (m_swapchainFormat == SDL_GPU_TEXTUREFORMAT_INVALID) {
        m_lastError = "Invalid swapchain format";
        return false;
    }

    // Initialize TTF if not already done
    if (!TTF_WasInit()) {
        if (!TTF_Init()) {
            m_lastError = std::string("Failed to initialize SDL_ttf: ") + SDL_GetError();
            return false;
        }
    }

    // Create GPU text engine
    m_textEngine = TTF_CreateGPUTextEngine(m_device->getHandle());
    if (!m_textEngine) {
        m_lastError = std::string("Failed to create GPU text engine: ") + SDL_GetError();
        return false;
    }

    // Load font
    if (!loadFont()) {
        return false;
    }

    // Create text objects
    if (!createTextObjects()) {
        return false;
    }

    // Create GPU resources
    if (!createResources()) {
        return false;
    }

    return true;
}

bool StatsOverlay::createResources() {
    SDL_GPUDevice* device = m_device->getHandle();

    // Create sampler for text atlas
    SDL_GPUSamplerCreateInfo samplerInfo = {};
    samplerInfo.min_filter = SDL_GPU_FILTER_LINEAR;
    samplerInfo.mag_filter = SDL_GPU_FILTER_LINEAR;
    samplerInfo.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
    samplerInfo.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    samplerInfo.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    samplerInfo.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    m_textSampler = SDL_CreateGPUSampler(device, &samplerInfo);
    if (!m_textSampler) {
        m_lastError = std::string("Failed to create text sampler: ") + SDL_GetError();
        return false;
    }

    // Create background quad vertex buffer
    std::uint32_t bufferSize = VERTICES_PER_QUAD * sizeof(StatsQuadVertex);

    SDL_GPUBufferCreateInfo bufferInfo = {};
    bufferInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    bufferInfo.size = bufferSize;
    m_bgVertexBuffer = SDL_CreateGPUBuffer(device, &bufferInfo);
    if (!m_bgVertexBuffer) {
        m_lastError = std::string("Failed to create vertex buffer: ") + SDL_GetError();
        return false;
    }

    SDL_GPUTransferBufferCreateInfo transferInfo = {};
    transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transferInfo.size = bufferSize;
    m_bgTransferBuffer = SDL_CreateGPUTransferBuffer(device, &transferInfo);
    if (!m_bgTransferBuffer) {
        m_lastError = std::string("Failed to create transfer buffer: ") + SDL_GetError();
        return false;
    }

    // Create background quad pipeline (simple colored quad)
    // Vertex shader transforms normalized coords to clip space
    // Fragment shader outputs solid color

    // For now, use an inline shader approach with simple passthrough
    // In production, this would use compiled shaders like POC-6

    // Create pipeline for colored quad rendering
    SDL_GPUColorTargetBlendState blendState = {};
    blendState.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
    blendState.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    blendState.color_blend_op = SDL_GPU_BLENDOP_ADD;
    blendState.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
    blendState.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    blendState.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
    blendState.enable_blend = true;
    blendState.color_write_mask = SDL_GPU_COLORCOMPONENT_R | SDL_GPU_COLORCOMPONENT_G |
                                   SDL_GPU_COLORCOMPONENT_B | SDL_GPU_COLORCOMPONENT_A;

    SDL_GPUColorTargetDescription colorTarget = {};
    colorTarget.format = m_swapchainFormat;
    colorTarget.blend_state = blendState;

    SDL_GPUGraphicsPipelineTargetInfo targetInfo = {};
    targetInfo.color_target_descriptions = &colorTarget;
    targetInfo.num_color_targets = 1;
    targetInfo.has_depth_stencil_target = false;

    SDL_GPURasterizerState rasterizer = {};
    rasterizer.fill_mode = SDL_GPU_FILLMODE_FILL;
    rasterizer.cull_mode = SDL_GPU_CULLMODE_NONE;

    // Load shaders from compiled files
    SDL_GPUShaderFormat supportedFormats = SDL_GetGPUShaderFormats(device);

    // Try to load UI quad shaders
    std::string shaderPath;
    SDL_GPUShaderFormat format = SDL_GPU_SHADERFORMAT_INVALID;

    if (supportedFormats & SDL_GPU_SHADERFORMAT_DXIL) {
        shaderPath = "shaders/stats_overlay";
        format = SDL_GPU_SHADERFORMAT_DXIL;
    } else if (supportedFormats & SDL_GPU_SHADERFORMAT_SPIRV) {
        shaderPath = "shaders/stats_overlay";
        format = SDL_GPU_SHADERFORMAT_SPIRV;
    }

    if (format == SDL_GPU_SHADERFORMAT_INVALID) {
        m_lastError = "No compatible shader format available";
        return false;
    }

    // Load vertex shader
    std::string vertPath = shaderPath + ".vert." +
        (format == SDL_GPU_SHADERFORMAT_DXIL ? "dxil" : "spv");

    SDL_IOStream* vertFile = SDL_IOFromFile(vertPath.c_str(), "rb");
    if (!vertFile) {
        m_lastError = std::string("Failed to open vertex shader: ") + vertPath;
        return false;
    }

    Sint64 vertSize = SDL_GetIOSize(vertFile);
    std::vector<std::uint8_t> vertCode(static_cast<size_t>(vertSize));
    SDL_ReadIO(vertFile, vertCode.data(), static_cast<size_t>(vertSize));
    SDL_CloseIO(vertFile);

    SDL_GPUShaderCreateInfo vertShaderInfo = {};
    vertShaderInfo.code = vertCode.data();
    vertShaderInfo.code_size = vertCode.size();
    vertShaderInfo.entrypoint = "main";
    vertShaderInfo.format = format;
    vertShaderInfo.stage = SDL_GPU_SHADERSTAGE_VERTEX;

    SDL_GPUShader* vertShader = SDL_CreateGPUShader(device, &vertShaderInfo);
    if (!vertShader) {
        m_lastError = std::string("Failed to create vertex shader: ") + SDL_GetError();
        return false;
    }

    // Load fragment shader
    std::string fragPath = shaderPath + ".frag." +
        (format == SDL_GPU_SHADERFORMAT_DXIL ? "dxil" : "spv");

    SDL_IOStream* fragFile = SDL_IOFromFile(fragPath.c_str(), "rb");
    if (!fragFile) {
        SDL_ReleaseGPUShader(device, vertShader);
        m_lastError = std::string("Failed to open fragment shader: ") + fragPath;
        return false;
    }

    Sint64 fragSize = SDL_GetIOSize(fragFile);
    std::vector<std::uint8_t> fragCode(static_cast<size_t>(fragSize));
    SDL_ReadIO(fragFile, fragCode.data(), static_cast<size_t>(fragSize));
    SDL_CloseIO(fragFile);

    SDL_GPUShaderCreateInfo fragShaderInfo = {};
    fragShaderInfo.code = fragCode.data();
    fragShaderInfo.code_size = fragCode.size();
    fragShaderInfo.entrypoint = "main";
    fragShaderInfo.format = format;
    fragShaderInfo.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;

    SDL_GPUShader* fragShader = SDL_CreateGPUShader(device, &fragShaderInfo);
    if (!fragShader) {
        SDL_ReleaseGPUShader(device, vertShader);
        m_lastError = std::string("Failed to create fragment shader: ") + SDL_GetError();
        return false;
    }

    // Vertex attributes: position (2 floats) + color (4 floats)
    SDL_GPUVertexBufferDescription vertexBufferDesc = {};
    vertexBufferDesc.slot = 0;
    vertexBufferDesc.pitch = sizeof(StatsQuadVertex);
    vertexBufferDesc.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;

    SDL_GPUVertexAttribute attributes[2] = {};
    // Position
    attributes[0].location = 0;
    attributes[0].buffer_slot = 0;
    attributes[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
    attributes[0].offset = offsetof(StatsQuadVertex, x);
    // Color
    attributes[1].location = 1;
    attributes[1].buffer_slot = 0;
    attributes[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4;
    attributes[1].offset = offsetof(StatsQuadVertex, r);

    SDL_GPUVertexInputState vertexInput = {};
    vertexInput.vertex_buffer_descriptions = &vertexBufferDesc;
    vertexInput.num_vertex_buffers = 1;
    vertexInput.vertex_attributes = attributes;
    vertexInput.num_vertex_attributes = 2;

    SDL_GPUGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.vertex_shader = vertShader;
    pipelineInfo.fragment_shader = fragShader;
    pipelineInfo.vertex_input_state = vertexInput;
    pipelineInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    pipelineInfo.rasterizer_state = rasterizer;
    pipelineInfo.target_info = targetInfo;

    m_bgPipeline = SDL_CreateGPUGraphicsPipeline(device, &pipelineInfo);

    // Release shaders (pipeline holds references)
    SDL_ReleaseGPUShader(device, vertShader);
    SDL_ReleaseGPUShader(device, fragShader);

    if (!m_bgPipeline) {
        m_lastError = std::string("Failed to create pipeline: ") + SDL_GetError();
        return false;
    }

    return true;
}

void StatsOverlay::releaseResources() {
    if (m_device && m_device->getHandle()) {
        SDL_GPUDevice* device = m_device->getHandle();

        if (m_bgPipeline) {
            SDL_ReleaseGPUGraphicsPipeline(device, m_bgPipeline);
            m_bgPipeline = nullptr;
        }

        if (m_bgVertexBuffer) {
            SDL_ReleaseGPUBuffer(device, m_bgVertexBuffer);
            m_bgVertexBuffer = nullptr;
        }

        if (m_bgTransferBuffer) {
            SDL_ReleaseGPUTransferBuffer(device, m_bgTransferBuffer);
            m_bgTransferBuffer = nullptr;
        }

        if (m_textSampler) {
            SDL_ReleaseGPUSampler(device, m_textSampler);
            m_textSampler = nullptr;
        }
    }

    // Destroy text objects
    for (int i = 0; i < LINE_COUNT; ++i) {
        if (m_textObjects[i]) {
            TTF_DestroyText(m_textObjects[i]);
            m_textObjects[i] = nullptr;
        }
    }

    // Close font
    if (m_font) {
        TTF_CloseFont(m_font);
        m_font = nullptr;
    }

    // Destroy text engine
    if (m_textEngine) {
        TTF_DestroyGPUTextEngine(m_textEngine);
        m_textEngine = nullptr;
    }
}

bool StatsOverlay::loadFont() {
    // Try common system font paths
    const char* fontPaths[] = {
        "C:/Windows/Fonts/consola.ttf",    // Consolas (monospace, good for numbers)
        "C:/Windows/Fonts/cour.ttf",       // Courier New
        "C:/Windows/Fonts/arial.ttf",      // Arial
        "C:/Windows/Fonts/segoeui.ttf",    // Segoe UI
        "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",  // Linux
        "/System/Library/Fonts/Monaco.dfont",                    // macOS
        nullptr
    };

    for (int i = 0; fontPaths[i] != nullptr; ++i) {
        m_font = TTF_OpenFont(fontPaths[i], m_config.fontSize);
        if (m_font) {
            return true;
        }
    }

    m_lastError = std::string("Failed to load any system font: ") + SDL_GetError();
    return false;
}

bool StatsOverlay::createTextObjects() {
    // Initialize text strings
    m_textStrings[0] = "FPS: ---";
    m_textStrings[1] = "Frame: --.- ms";
    m_textStrings[2] = "Draws: ---";
    m_textStrings[3] = "Tris: ---";

    // Create text objects
    for (int i = 0; i < LINE_COUNT; ++i) {
        m_textObjects[i] = TTF_CreateText(m_textEngine, m_font, m_textStrings[i].c_str(), 0);
        if (!m_textObjects[i]) {
            m_lastError = std::string("Failed to create text object: ") + SDL_GetError();
            return false;
        }

        // Set text color
        TTF_SetTextColor(m_textObjects[i],
            m_config.textR, m_config.textG, m_config.textB, m_config.textA);
    }

    return true;
}

void StatsOverlay::updateTextContent() {
    // Format stats strings
    char buffer[64];

    // FPS
    std::snprintf(buffer, sizeof(buffer), "FPS: %.1f", m_stats.fps);
    m_textStrings[0] = buffer;

    // Frame time
    std::snprintf(buffer, sizeof(buffer), "Frame: %.2f ms", m_stats.frameTimeMs);
    m_textStrings[1] = buffer;

    // Draw calls
    std::snprintf(buffer, sizeof(buffer), "Draws: %u", m_stats.drawCalls);
    m_textStrings[2] = buffer;

    // Triangles
    if (m_stats.triangles >= 1000000) {
        std::snprintf(buffer, sizeof(buffer), "Tris: %.2fM", m_stats.triangles / 1000000.0f);
    } else if (m_stats.triangles >= 1000) {
        std::snprintf(buffer, sizeof(buffer), "Tris: %.1fK", m_stats.triangles / 1000.0f);
    } else {
        std::snprintf(buffer, sizeof(buffer), "Tris: %u", m_stats.triangles);
    }
    m_textStrings[3] = buffer;

    // Update text objects with new content
    for (int i = 0; i < LINE_COUNT; ++i) {
        if (m_textObjects[i]) {
            TTF_SetTextString(m_textObjects[i], m_textStrings[i].c_str(), 0);
        }
    }
}

std::string StatsOverlay::formatLine(const char* label, float value, const char* unit) {
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "%s: %.2f%s", label, value, unit);
    return buffer;
}

std::string StatsOverlay::formatLine(const char* label, std::uint32_t value, const char* unit) {
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "%s: %u%s", label, value, unit);
    return buffer;
}

std::string StatsOverlay::formatLine(const char* label, std::uint64_t value, const char* unit) {
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "%s: %llu%s", label,
        static_cast<unsigned long long>(value), unit);
    return buffer;
}

} // namespace sims3000
