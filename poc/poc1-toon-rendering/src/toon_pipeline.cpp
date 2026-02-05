#include "toon_pipeline.h"
#include "gpu_device.h"
#include "gpu_mesh.h"
#include "camera.h"
#include "instance_buffer.h"
#include "benchmark.h"
#include "shader_loader.h"
#include "model_loader.h"
#include "scene.h"

#include <string>

using namespace sims3000;

ToonPipeline::ToonPipeline(GPUDevice& device)
    : m_device(&device)
{
    // Set default outline thickness
    m_outlineUniforms.outlineThickness = 0.05f;
    m_outlineUniforms.padding[0] = 0.0f;
    m_outlineUniforms.padding[1] = 0.0f;
    m_outlineUniforms.padding[2] = 0.0f;
}

ToonPipeline::~ToonPipeline()
{
    Cleanup();
}

ToonPipeline::ToonPipeline(ToonPipeline&& other) noexcept
    : m_device(other.m_device)
    , m_toonVertexShader(other.m_toonVertexShader)
    , m_toonFragmentShader(other.m_toonFragmentShader)
    , m_outlineVertexShader(other.m_outlineVertexShader)
    , m_outlineFragmentShader(other.m_outlineFragmentShader)
    , m_toonPipeline(other.m_toonPipeline)
    , m_outlinePipeline(other.m_outlinePipeline)
    , m_depthTexture(other.m_depthTexture)
    , m_depthWidth(other.m_depthWidth)
    , m_depthHeight(other.m_depthHeight)
    , m_vpUniforms(other.m_vpUniforms)
    , m_outlineUniforms(other.m_outlineUniforms)
    , m_initialized(other.m_initialized)
{
    other.m_device = nullptr;
    other.m_toonVertexShader = nullptr;
    other.m_toonFragmentShader = nullptr;
    other.m_outlineVertexShader = nullptr;
    other.m_outlineFragmentShader = nullptr;
    other.m_toonPipeline = nullptr;
    other.m_outlinePipeline = nullptr;
    other.m_depthTexture = nullptr;
    other.m_initialized = false;
}

ToonPipeline& ToonPipeline::operator=(ToonPipeline&& other) noexcept
{
    if (this != &other) {
        Cleanup();

        m_device = other.m_device;
        m_toonVertexShader = other.m_toonVertexShader;
        m_toonFragmentShader = other.m_toonFragmentShader;
        m_outlineVertexShader = other.m_outlineVertexShader;
        m_outlineFragmentShader = other.m_outlineFragmentShader;
        m_toonPipeline = other.m_toonPipeline;
        m_outlinePipeline = other.m_outlinePipeline;
        m_depthTexture = other.m_depthTexture;
        m_depthWidth = other.m_depthWidth;
        m_depthHeight = other.m_depthHeight;
        m_vpUniforms = other.m_vpUniforms;
        m_outlineUniforms = other.m_outlineUniforms;
        m_initialized = other.m_initialized;

        other.m_device = nullptr;
        other.m_toonVertexShader = nullptr;
        other.m_toonFragmentShader = nullptr;
        other.m_outlineVertexShader = nullptr;
        other.m_outlineFragmentShader = nullptr;
        other.m_toonPipeline = nullptr;
        other.m_outlinePipeline = nullptr;
        other.m_depthTexture = nullptr;
        other.m_initialized = false;
    }
    return *this;
}

bool ToonPipeline::Initialize(const char* shaderPath)
{
    if (!m_device || !m_device->IsValid()) {
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "ToonPipeline: Invalid GPU device");
        return false;
    }

    if (!shaderPath) {
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "ToonPipeline: Null shader path");
        return false;
    }

    // Load all shaders
    if (!CreateShaders(shaderPath)) {
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "ToonPipeline: Failed to create shaders");
        Cleanup();
        return false;
    }

    // Query swapchain format from the device window
    SDL_GPUTextureFormat swapchainFormat = SDL_GetGPUSwapchainTextureFormat(
        m_device->GetDevice(), m_device->GetWindow());

    // Create rendering pipelines
    if (!CreatePipelines(swapchainFormat)) {
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "ToonPipeline: Failed to create pipelines");
        Cleanup();
        return false;
    }

    m_initialized = true;
    SDL_LogInfo(SDL_LOG_CATEGORY_GPU, "ToonPipeline: Initialized successfully");
    return true;
}

bool ToonPipeline::CreateShaders(const char* shaderPath)
{
    SDL_GPUDevice* device = m_device->GetDevice();
    std::string basePath(shaderPath);

    // Ensure path ends with separator
    if (!basePath.empty() && basePath.back() != '/' && basePath.back() != '\\') {
        basePath += '/';
    }

    // Build shader base paths (without extension - shader loader auto-detects .spv or .dxil)
    std::string toonVertPath = basePath + "toon.vert";
    std::string toonFragPath = basePath + "toon.frag";
    std::string outlineVertPath = basePath + "outline.vert";
    std::string outlineFragPath = basePath + "outline.frag";

    // Vertex shaders need: 1 uniform buffer (VP matrix), 1 storage buffer (instances)
    // Outline vertex also needs outline uniforms
    ShaderLoader::ShaderResources vertexResources = {};
    vertexResources.numUniformBuffers = 1;
    vertexResources.numStorageBuffers = 1;

    ShaderLoader::ShaderResources outlineVertResources = {};
    outlineVertResources.numUniformBuffers = 2; // VP matrix + outline thickness
    outlineVertResources.numStorageBuffers = 1;

    // Fragment shaders: no special resources needed for basic toon shading
    ShaderLoader::ShaderResources fragmentResources = {};

    // Load toon shaders
    m_toonVertexShader = ShaderLoader::LoadShader(
        device,
        toonVertPath.c_str(),
        ShaderLoader::Stage::Vertex,
        "main",
        vertexResources
    );
    if (!m_toonVertexShader) {
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "ToonPipeline: Failed to load toon vertex shader from %s", toonVertPath.c_str());
        return false;
    }

    m_toonFragmentShader = ShaderLoader::LoadShader(
        device,
        toonFragPath.c_str(),
        ShaderLoader::Stage::Fragment,
        "main",
        fragmentResources
    );
    if (!m_toonFragmentShader) {
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "ToonPipeline: Failed to load toon fragment shader from %s", toonFragPath.c_str());
        return false;
    }

    // Load outline shaders
    m_outlineVertexShader = ShaderLoader::LoadShader(
        device,
        outlineVertPath.c_str(),
        ShaderLoader::Stage::Vertex,
        "main",
        outlineVertResources
    );
    if (!m_outlineVertexShader) {
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "ToonPipeline: Failed to load outline vertex shader from %s", outlineVertPath.c_str());
        return false;
    }

    m_outlineFragmentShader = ShaderLoader::LoadShader(
        device,
        outlineFragPath.c_str(),
        ShaderLoader::Stage::Fragment,
        "main",
        fragmentResources
    );
    if (!m_outlineFragmentShader) {
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "ToonPipeline: Failed to load outline fragment shader from %s", outlineFragPath.c_str());
        return false;
    }

    SDL_LogInfo(SDL_LOG_CATEGORY_GPU, "ToonPipeline: All shaders loaded successfully");
    return true;
}

bool ToonPipeline::CreatePipelines(SDL_GPUTextureFormat swapchainFormat)
{
    SDL_GPUDevice* device = m_device->GetDevice();

    // Vertex input state: position (vec3) and normal (vec3)
    // Vertex stride = sizeof(Vertex) = 24 bytes
    SDL_GPUVertexBufferDescription vertexBufferDesc = {};
    vertexBufferDesc.slot = 0;
    vertexBufferDesc.pitch = sizeof(Vertex); // 24 bytes
    vertexBufferDesc.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
    vertexBufferDesc.instance_step_rate = 0;

    SDL_GPUVertexAttribute vertexAttributes[2] = {};

    // Position attribute: vec3 at offset 0
    vertexAttributes[0].location = 0;
    vertexAttributes[0].buffer_slot = 0;
    vertexAttributes[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
    vertexAttributes[0].offset = 0;

    // Normal attribute: vec3 at offset 12
    vertexAttributes[1].location = 1;
    vertexAttributes[1].buffer_slot = 0;
    vertexAttributes[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
    vertexAttributes[1].offset = 12;

    SDL_GPUVertexInputState vertexInputState = {};
    vertexInputState.vertex_buffer_descriptions = &vertexBufferDesc;
    vertexInputState.num_vertex_buffers = 1;
    vertexInputState.vertex_attributes = vertexAttributes;
    vertexInputState.num_vertex_attributes = 2;

    // Color target description
    SDL_GPUColorTargetDescription colorTargetDesc = {};
    colorTargetDesc.format = swapchainFormat;

    // Blend state - standard alpha blending
    SDL_GPUColorTargetBlendState blendState = {};
    blendState.enable_blend = false;
    blendState.src_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
    blendState.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ZERO;
    blendState.color_blend_op = SDL_GPU_BLENDOP_ADD;
    blendState.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
    blendState.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ZERO;
    blendState.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
    blendState.color_write_mask = SDL_GPU_COLORCOMPONENT_R | SDL_GPU_COLORCOMPONENT_G |
                                   SDL_GPU_COLORCOMPONENT_B | SDL_GPU_COLORCOMPONENT_A;
    colorTargetDesc.blend_state = blendState;

    // Create outline pipeline (front-face culling for inverted hull)
    m_outlinePipeline = ShaderLoader::LoadGraphicsPipeline(
        device,
        m_outlineVertexShader,
        m_outlineFragmentShader,
        vertexInputState,
        colorTargetDesc,
        SDL_GPU_TEXTUREFORMAT_D32_FLOAT,  // Depth format
        SDL_GPU_CULLMODE_FRONT,            // Front-face culling (inverted hull)
        SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE,
        true,                              // Enable depth test
        true                               // Enable depth write
    );
    if (!m_outlinePipeline) {
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "ToonPipeline: Failed to create outline pipeline");
        return false;
    }

    // Create toon pipeline (back-face culling for normal rendering)
    m_toonPipeline = ShaderLoader::LoadGraphicsPipeline(
        device,
        m_toonVertexShader,
        m_toonFragmentShader,
        vertexInputState,
        colorTargetDesc,
        SDL_GPU_TEXTUREFORMAT_D32_FLOAT,  // Depth format
        SDL_GPU_CULLMODE_BACK,             // Back-face culling (normal)
        SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE,
        true,                              // Enable depth test
        true                               // Enable depth write
    );
    if (!m_toonPipeline) {
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "ToonPipeline: Failed to create toon pipeline");
        return false;
    }

    SDL_LogInfo(SDL_LOG_CATEGORY_GPU, "ToonPipeline: Pipelines created successfully");
    return true;
}

bool ToonPipeline::CreateDepthTexture(uint32_t width, uint32_t height)
{
    if (!m_device || !m_device->IsValid()) {
        return false;
    }

    // Release existing depth texture if dimensions changed
    if (m_depthTexture && (m_depthWidth != width || m_depthHeight != height)) {
        ReleaseDepthTexture();
    }

    // Already have valid depth texture
    if (m_depthTexture) {
        return true;
    }

    SDL_GPUTextureCreateInfo depthTextureInfo = {};
    depthTextureInfo.type = SDL_GPU_TEXTURETYPE_2D;
    depthTextureInfo.format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
    depthTextureInfo.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
    depthTextureInfo.width = width;
    depthTextureInfo.height = height;
    depthTextureInfo.layer_count_or_depth = 1;
    depthTextureInfo.num_levels = 1;
    depthTextureInfo.sample_count = SDL_GPU_SAMPLECOUNT_1;

    m_depthTexture = SDL_CreateGPUTexture(m_device->GetDevice(), &depthTextureInfo);
    if (!m_depthTexture) {
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "ToonPipeline: Failed to create depth texture: %s", SDL_GetError());
        return false;
    }

    m_depthWidth = width;
    m_depthHeight = height;

    SDL_LogInfo(SDL_LOG_CATEGORY_GPU, "ToonPipeline: Created depth texture %ux%u", width, height);
    return true;
}

void ToonPipeline::ReleaseDepthTexture()
{
    if (m_depthTexture && m_device && m_device->IsValid()) {
        SDL_ReleaseGPUTexture(m_device->GetDevice(), m_depthTexture);
        m_depthTexture = nullptr;
        m_depthWidth = 0;
        m_depthHeight = 0;
    }
}

void ToonPipeline::SetCamera(const Camera& camera)
{
    m_vpUniforms.viewProjection = camera.GetViewProjectionMatrix();
}

void ToonPipeline::Render(
    SDL_GPUCommandBuffer* commandBuffer,
    SDL_GPUTexture* swapchain,
    const std::vector<std::unique_ptr<GPUMesh>>& meshes,
    poc1::InstanceBuffer& instances,
    const std::vector<poc1::ModelGroup>& modelGroups,
    poc1::Benchmark& benchmark)
{
    if (!m_initialized || !commandBuffer || !swapchain) {
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "ToonPipeline: Invalid render state");
        return;
    }

    if (instances.GetInstanceCount() == 0 || modelGroups.empty()) {
        return;
    }

    // Get swapchain dimensions for depth buffer
    uint32_t swapchainWidth = 0;
    uint32_t swapchainHeight = 0;
    SDL_GetWindowSizeInPixels(m_device->GetWindow(), (int*)&swapchainWidth, (int*)&swapchainHeight);

    if (!CreateDepthTexture(swapchainWidth, swapchainHeight)) {
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "ToonPipeline: Failed to create/resize depth texture");
        return;
    }

    // Set up color target - clear to sky blue
    SDL_GPUColorTargetInfo colorTarget = {};
    colorTarget.texture = swapchain;
    colorTarget.mip_level = 0;
    colorTarget.layer_or_depth_plane = 0;
    colorTarget.clear_color.r = 0.5f;
    colorTarget.clear_color.g = 0.7f;
    colorTarget.clear_color.b = 0.9f;
    colorTarget.clear_color.a = 1.0f;
    colorTarget.load_op = SDL_GPU_LOADOP_CLEAR;
    colorTarget.store_op = SDL_GPU_STOREOP_STORE;
    colorTarget.resolve_texture = nullptr;
    colorTarget.resolve_mip_level = 0;
    colorTarget.resolve_layer = 0;
    colorTarget.cycle = false;
    colorTarget.cycle_resolve_texture = false;

    // Set up depth target - clear to 1.0
    SDL_GPUDepthStencilTargetInfo depthTarget = {};
    depthTarget.texture = m_depthTexture;
    depthTarget.clear_depth = 1.0f;
    depthTarget.load_op = SDL_GPU_LOADOP_CLEAR;
    depthTarget.store_op = SDL_GPU_STOREOP_STORE;
    depthTarget.stencil_load_op = SDL_GPU_LOADOP_DONT_CARE;
    depthTarget.stencil_store_op = SDL_GPU_STOREOP_DONT_CARE;
    depthTarget.cycle = false;
    depthTarget.clear_stencil = 0;

    // Begin render pass
    SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(
        commandBuffer, &colorTarget, 1, &depthTarget);

    if (!renderPass) {
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "ToonPipeline: Failed to begin render pass: %s", SDL_GetError());
        return;
    }

    // Bind instance storage buffer (shared across all draws)
    SDL_GPUBuffer* storageBuffers[] = { instances.GetBuffer() };

    // ==================
    // Pass 1 - Outlines (all models)
    // ==================
    SDL_BindGPUGraphicsPipeline(renderPass, m_outlinePipeline);
    SDL_PushGPUVertexUniformData(commandBuffer, 0, &m_vpUniforms, sizeof(m_vpUniforms));
    SDL_PushGPUVertexUniformData(commandBuffer, 1, &m_outlineUniforms, sizeof(m_outlineUniforms));
    SDL_BindGPUVertexStorageBuffers(renderPass, 0, storageBuffers, 1);

    for (const auto& group : modelGroups) {
        if (group.modelIndex >= meshes.size() || !meshes[group.modelIndex] ||
            !meshes[group.modelIndex]->IsValid()) {
            continue;
        }

        GPUMesh& mesh = *meshes[group.modelIndex];

        SDL_GPUBufferBinding vertexBinding = {};
        vertexBinding.buffer = mesh.GetVertexBuffer();
        vertexBinding.offset = 0;
        SDL_BindGPUVertexBuffers(renderPass, 0, &vertexBinding, 1);

        SDL_GPUBufferBinding indexBinding = {};
        indexBinding.buffer = mesh.GetIndexBuffer();
        indexBinding.offset = 0;
        SDL_BindGPUIndexBuffer(renderPass, &indexBinding, SDL_GPU_INDEXELEMENTSIZE_32BIT);

        SDL_DrawGPUIndexedPrimitives(
            renderPass,
            mesh.GetIndexCount(),
            group.instanceCount,
            0, 0,
            group.firstInstance);
        benchmark.IncrementDrawCalls();
    }

    // ==================
    // Pass 2 - Toon shading (all models)
    // ==================
    SDL_BindGPUGraphicsPipeline(renderPass, m_toonPipeline);
    SDL_PushGPUVertexUniformData(commandBuffer, 0, &m_vpUniforms, sizeof(m_vpUniforms));
    SDL_BindGPUVertexStorageBuffers(renderPass, 0, storageBuffers, 1);

    for (const auto& group : modelGroups) {
        if (group.modelIndex >= meshes.size() || !meshes[group.modelIndex] ||
            !meshes[group.modelIndex]->IsValid()) {
            continue;
        }

        GPUMesh& mesh = *meshes[group.modelIndex];

        SDL_GPUBufferBinding vertexBinding = {};
        vertexBinding.buffer = mesh.GetVertexBuffer();
        vertexBinding.offset = 0;
        SDL_BindGPUVertexBuffers(renderPass, 0, &vertexBinding, 1);

        SDL_GPUBufferBinding indexBinding = {};
        indexBinding.buffer = mesh.GetIndexBuffer();
        indexBinding.offset = 0;
        SDL_BindGPUIndexBuffer(renderPass, &indexBinding, SDL_GPU_INDEXELEMENTSIZE_32BIT);

        SDL_DrawGPUIndexedPrimitives(
            renderPass,
            mesh.GetIndexCount(),
            group.instanceCount,
            0, 0,
            group.firstInstance);
        benchmark.IncrementDrawCalls();
    }

    SDL_EndGPURenderPass(renderPass);
}

void ToonPipeline::Cleanup()
{
    if (!m_device || !m_device->IsValid()) {
        return;
    }

    SDL_GPUDevice* device = m_device->GetDevice();

    // Release pipelines
    if (m_outlinePipeline) {
        SDL_ReleaseGPUGraphicsPipeline(device, m_outlinePipeline);
        m_outlinePipeline = nullptr;
    }

    if (m_toonPipeline) {
        SDL_ReleaseGPUGraphicsPipeline(device, m_toonPipeline);
        m_toonPipeline = nullptr;
    }

    // Release shaders
    if (m_toonVertexShader) {
        SDL_ReleaseGPUShader(device, m_toonVertexShader);
        m_toonVertexShader = nullptr;
    }

    if (m_toonFragmentShader) {
        SDL_ReleaseGPUShader(device, m_toonFragmentShader);
        m_toonFragmentShader = nullptr;
    }

    if (m_outlineVertexShader) {
        SDL_ReleaseGPUShader(device, m_outlineVertexShader);
        m_outlineVertexShader = nullptr;
    }

    if (m_outlineFragmentShader) {
        SDL_ReleaseGPUShader(device, m_outlineFragmentShader);
        m_outlineFragmentShader = nullptr;
    }

    // Release depth texture
    ReleaseDepthTexture();

    m_initialized = false;
    SDL_LogInfo(SDL_LOG_CATEGORY_GPU, "ToonPipeline: Cleaned up all resources");
}

bool ToonPipeline::IsValid() const
{
    return m_initialized &&
           m_device != nullptr &&
           m_device->IsValid() &&
           m_toonPipeline != nullptr &&
           m_outlinePipeline != nullptr;
}
