/**
 * @file MainRenderPass.cpp
 * @brief Main render pass implementation.
 */

#include "sims3000/render/MainRenderPass.h"
#include "sims3000/render/GPUDevice.h"
#include "sims3000/render/Window.h"
#include "sims3000/render/CameraUniforms.h"
#include "sims3000/render/ToonPipeline.h"
#include "sims3000/render/InstancedRenderer.h"
#include "sims3000/render/UniformBufferPool.h"
#include "sims3000/render/ToonShader.h"
#include <SDL3/SDL_log.h>

namespace sims3000 {

// =============================================================================
// Construction / Destruction
// =============================================================================

MainRenderPass::MainRenderPass(GPUDevice& device, Window& window)
    : MainRenderPass(device, window, MainRenderPassConfig{})
{
}

MainRenderPass::MainRenderPass(GPUDevice& device, Window& window,
                               const MainRenderPassConfig& config)
    : m_device(&device)
    , m_window(&window)
    , m_config(config)
    , m_depthBuffer(device, static_cast<std::uint32_t>(window.getWidth()),
                    static_cast<std::uint32_t>(window.getHeight()), config.depthFormat)
    , m_normalBuffer(device, static_cast<std::uint32_t>(window.getWidth()),
                     static_cast<std::uint32_t>(window.getHeight()))
    , m_edgePass(device, SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM)  // Will update format on first frame
    , m_shadowPass(device, config.shadowConfig)
    , m_bloomPass(device, static_cast<std::uint32_t>(window.getWidth()),
                  static_cast<std::uint32_t>(window.getHeight()), config.bloomConfig)
    , m_width(static_cast<std::uint32_t>(window.getWidth()))
    , m_height(static_cast<std::uint32_t>(window.getHeight()))
{
    if (!initialize()) {
        SDL_Log("MainRenderPass: Failed to initialize: %s", m_lastError.c_str());
    }
}

MainRenderPass::~MainRenderPass() {
    // Ensure we're not in a frame
    if (m_inRenderPass && m_renderPass) {
        SDL_EndGPURenderPass(m_renderPass);
        m_renderPass = nullptr;
        m_inRenderPass = false;
    }

    if (m_inFrame && m_commandBuffer && m_device && m_device->isValid()) {
        // Submit any pending work
        m_device->submit(m_commandBuffer);
        m_commandBuffer = nullptr;
        m_inFrame = false;
    }

    // Release scene color target if allocated
    if (m_sceneColorTarget && m_device && m_device->isValid()) {
        SDL_ReleaseGPUTexture(m_device->getHandle(), m_sceneColorTarget);
        m_sceneColorTarget = nullptr;
    }
}

MainRenderPass::MainRenderPass(MainRenderPass&& other) noexcept
    : m_device(other.m_device)
    , m_window(other.m_window)
    , m_config(other.m_config)
    , m_depthBuffer(std::move(other.m_depthBuffer))
    , m_normalBuffer(std::move(other.m_normalBuffer))
    , m_edgePass(std::move(other.m_edgePass))
    , m_shadowPass(std::move(other.m_shadowPass))
    , m_bloomPass(std::move(other.m_bloomPass))
    , m_sceneColorTarget(other.m_sceneColorTarget)
    , m_commandBuffer(other.m_commandBuffer)
    , m_renderPass(other.m_renderPass)
    , m_swapchainTexture(other.m_swapchainTexture)
    , m_swapchainFormat(other.m_swapchainFormat)
    , m_inFrame(other.m_inFrame)
    , m_inRenderPass(other.m_inRenderPass)
    , m_state(other.m_state)
    , m_width(other.m_width)
    , m_height(other.m_height)
    , m_stats(other.m_stats)
    , m_frameNumber(other.m_frameNumber)
    , m_lastError(std::move(other.m_lastError))
{
    other.m_device = nullptr;
    other.m_window = nullptr;
    other.m_sceneColorTarget = nullptr;
    other.m_commandBuffer = nullptr;
    other.m_renderPass = nullptr;
    other.m_swapchainTexture = nullptr;
    other.m_inFrame = false;
    other.m_inRenderPass = false;
}

MainRenderPass& MainRenderPass::operator=(MainRenderPass&& other) noexcept {
    if (this != &other) {
        // Clean up current state
        if (m_inRenderPass && m_renderPass) {
            SDL_EndGPURenderPass(m_renderPass);
        }
        if (m_inFrame && m_commandBuffer && m_device && m_device->isValid()) {
            m_device->submit(m_commandBuffer);
        }

        // Clean up scene color target
        if (m_sceneColorTarget && m_device && m_device->isValid()) {
            SDL_ReleaseGPUTexture(m_device->getHandle(), m_sceneColorTarget);
        }

        m_device = other.m_device;
        m_window = other.m_window;
        m_config = other.m_config;
        m_depthBuffer = std::move(other.m_depthBuffer);
        m_normalBuffer = std::move(other.m_normalBuffer);
        m_edgePass = std::move(other.m_edgePass);
        m_shadowPass = std::move(other.m_shadowPass);
        m_bloomPass = std::move(other.m_bloomPass);
        m_sceneColorTarget = other.m_sceneColorTarget;
        m_commandBuffer = other.m_commandBuffer;
        m_renderPass = other.m_renderPass;
        m_swapchainTexture = other.m_swapchainTexture;
        m_swapchainFormat = other.m_swapchainFormat;
        m_inFrame = other.m_inFrame;
        m_inRenderPass = other.m_inRenderPass;
        m_state = other.m_state;
        m_width = other.m_width;
        m_height = other.m_height;
        m_stats = other.m_stats;
        m_frameNumber = other.m_frameNumber;
        m_lastError = std::move(other.m_lastError);

        other.m_device = nullptr;
        other.m_window = nullptr;
        other.m_sceneColorTarget = nullptr;
        other.m_commandBuffer = nullptr;
        other.m_renderPass = nullptr;
        other.m_swapchainTexture = nullptr;
        other.m_inFrame = false;
        other.m_inRenderPass = false;
    }
    return *this;
}

// =============================================================================
// Public Interface
// =============================================================================

bool MainRenderPass::isValid() const {
    return m_device != nullptr &&
           m_device->isValid() &&
           m_window != nullptr &&
           m_window->isValid() &&
           m_window->isClaimed() &&
           m_depthBuffer.isValid() &&
           m_normalBuffer.isValid() &&
           m_edgePass.isValid() &&
           m_shadowPass.isValid() &&
           m_bloomPass.isValid();
}

// =============================================================================
// Frame Lifecycle
// =============================================================================

bool MainRenderPass::beginFrame() {
    if (!isValid()) {
        m_lastError = "MainRenderPass not valid";
        return false;
    }

    if (m_inFrame) {
        m_lastError = "Already in frame - call endFrame() first";
        return false;
    }

    // Reset stats for new frame
    m_stats.reset();
    m_stats.frameNumber = ++m_frameNumber;

    // Reset render pass state
    m_state.reset();

    // Check for window resize
    if (!checkAndHandleResize()) {
        return false;
    }

    // Acquire command buffer
    m_commandBuffer = m_device->acquireCommandBuffer();
    if (!m_commandBuffer) {
        m_lastError = "Failed to acquire command buffer";
        return false;
    }

    // Acquire swap chain texture
    std::uint32_t swapWidth = 0, swapHeight = 0;
    if (!m_window->acquireSwapchainTexture(m_commandBuffer, &m_swapchainTexture,
                                            &swapWidth, &swapHeight)) {
        // Still need to submit the command buffer
        m_device->submit(m_commandBuffer);
        m_commandBuffer = nullptr;
        m_lastError = "Failed to acquire swapchain texture";
        return false;
    }

    if (!m_swapchainTexture) {
        // Swapchain unavailable (minimized window, etc.)
        m_device->submit(m_commandBuffer);
        m_commandBuffer = nullptr;
        m_stats.swapchainAcquired = false;
        return false;
    }

    m_stats.swapchainAcquired = true;
    m_swapchainFormat = m_window->getSwapchainTextureFormat();
    m_inFrame = true;

    return true;
}

bool MainRenderPass::beginRenderPass() {
    if (!m_inFrame) {
        m_lastError = "Not in frame - call beginFrame() first";
        return false;
    }

    if (m_inRenderPass) {
        m_lastError = "Already in render pass - call endRenderPass() first";
        return false;
    }

    // Configure color target with dark bioluminescent clear color
    SDL_GPUColorTargetInfo colorTarget = {};
    colorTarget.texture = m_swapchainTexture;
    colorTarget.clear_color = {
        m_config.clearColors.color.r,
        m_config.clearColors.color.g,
        m_config.clearColors.color.b,
        m_config.clearColors.color.a
    };
    colorTarget.load_op = SDL_GPU_LOADOP_CLEAR;
    colorTarget.store_op = SDL_GPU_STOREOP_STORE;

    // Configure depth target with clear to 1.0 (sampleable for edge detection)
    SDL_GPUDepthStencilTargetInfo depthTarget = m_depthBuffer.getDepthStencilTargetInfoSampleable(
        m_config.clearColors.depth);

    // Begin render pass
    m_renderPass = SDL_BeginGPURenderPass(m_commandBuffer, &colorTarget, 1, &depthTarget);
    if (!m_renderPass) {
        m_lastError = std::string("Failed to begin render pass: ") + SDL_GetError();
        return false;
    }

    m_inRenderPass = true;
    return true;
}

void MainRenderPass::endRenderPass() {
    if (!m_inRenderPass || !m_renderPass) {
        return;
    }

    SDL_EndGPURenderPass(m_renderPass);
    m_renderPass = nullptr;
    m_inRenderPass = false;
}

bool MainRenderPass::executeEdgeDetection() {
    if (!m_inFrame) {
        m_lastError = "executeEdgeDetection: Not in frame";
        return false;
    }

    if (!m_config.enableEdgeDetection) {
        return true;  // Edge detection disabled, nothing to do
    }

    if (!m_swapchainTexture) {
        m_lastError = "executeEdgeDetection: No swapchain texture";
        return false;
    }

    // Edge detection must run AFTER opaque render pass ends.
    // If we're still in a render pass, end it first.
    if (m_inRenderPass) {
        endRenderPass();
    }

    // Execute edge detection pass on opaque geometry only.
    // This reads from the current scene color (containing opaques) and
    // writes edges back to the scene color texture.
    bool result = m_edgePass.execute(
        m_commandBuffer,
        m_swapchainTexture,
        m_normalBuffer.getHandle(),
        m_depthBuffer.getHandle(),
        m_swapchainTexture,  // In-place edge compositing
        m_width,
        m_height
    );

    if (result) {
        m_stats.edgeDetectionTimeMs = m_edgePass.getStats().executionTimeMs;
    }

    return result;
}

bool MainRenderPass::beginTransparentRenderPass() {
    if (!m_inFrame) {
        m_lastError = "beginTransparentRenderPass: Not in frame";
        return false;
    }

    if (m_inRenderPass) {
        m_lastError = "beginTransparentRenderPass: Already in render pass";
        return false;
    }

    // Configure color target - LOAD existing content (with edges) for transparent blending
    SDL_GPUColorTargetInfo colorTarget = {};
    colorTarget.texture = m_swapchainTexture;
    colorTarget.load_op = SDL_GPU_LOADOP_LOAD;  // Preserve opaque geometry + edges
    colorTarget.store_op = SDL_GPU_STOREOP_STORE;

    // Configure depth target - LOAD existing depth for occlusion testing
    SDL_GPUDepthStencilTargetInfo depthTarget = {};
    depthTarget.texture = m_depthBuffer.getHandle();
    depthTarget.load_op = SDL_GPU_LOADOP_LOAD;  // Preserve depth from opaques
    depthTarget.store_op = SDL_GPU_STOREOP_DONT_CARE;  // Transparents don't write depth
    depthTarget.stencil_load_op = SDL_GPU_LOADOP_DONT_CARE;
    depthTarget.stencil_store_op = SDL_GPU_STOREOP_DONT_CARE;

    // Begin render pass for transparent geometry
    m_renderPass = SDL_BeginGPURenderPass(m_commandBuffer, &colorTarget, 1, &depthTarget);
    if (!m_renderPass) {
        m_lastError = std::string("Failed to begin transparent render pass: ") + SDL_GetError();
        return false;
    }

    m_inRenderPass = true;
    return true;
}

bool MainRenderPass::beginUIOverlayPass() {
    if (!m_inFrame) {
        m_lastError = "beginUIOverlayPass: Not in frame";
        return false;
    }

    if (m_inRenderPass) {
        m_lastError = "beginUIOverlayPass: Already in render pass - call endRenderPass() first";
        return false;
    }

    if (!m_swapchainTexture) {
        m_lastError = "beginUIOverlayPass: No swapchain texture";
        return false;
    }

    // Configure color target - LOAD existing content (3D scene with bloom)
    // UI renders on TOP without erasing the 3D scene
    SDL_GPUColorTargetInfo colorTarget = {};
    colorTarget.texture = m_swapchainTexture;
    colorTarget.load_op = SDL_GPU_LOADOP_LOAD;  // Preserve 3D scene
    colorTarget.store_op = SDL_GPU_STOREOP_STORE;

    // NO depth target - UI always renders on top regardless of depth
    // This is the Epic 12 UISystem integration point

    m_renderPass = SDL_BeginGPURenderPass(m_commandBuffer, &colorTarget, 1, nullptr);
    if (!m_renderPass) {
        m_lastError = std::string("Failed to begin UI overlay render pass: ") + SDL_GetError();
        return false;
    }

    m_inRenderPass = true;
    return true;
}

bool MainRenderPass::endFrame() {
    if (!m_inFrame) {
        m_lastError = "Not in frame";
        return false;
    }

    // Ensure render pass is ended
    if (m_inRenderPass) {
        endRenderPass();
    }

    // NOTE: Edge detection is no longer executed here.
    // Callers must call executeEdgeDetection() after opaque layers
    // and before renderTransparentPass() to satisfy the requirement:
    // "Edges render only on opaque geometry (before transparents)"

    // Execute bloom pass (mandatory per canon)
    if (m_config.enableBloom && m_swapchainTexture) {
        // Note: Full bloom would render to an intermediate target and composite
        // For now, execute the bloom pass for pipeline structure
        m_bloomPass.execute(m_commandBuffer, m_swapchainTexture, m_swapchainTexture);
        m_stats.bloomTimeMs = m_bloomPass.getStats().totalTimeMs;
    }

    // NOTE: UI overlay rendering happens here via beginUIOverlayPass() if the caller
    // needs to render UI. The caller is responsible for calling beginUIOverlayPass()
    // between bloom and this submission. See Epic 12 UISystem integration documentation.

    // Submit command buffer
    bool submitted = m_device->submit(m_commandBuffer);
    m_commandBuffer = nullptr;
    m_swapchainTexture = nullptr;
    m_inFrame = false;

    // Calculate totals
    m_stats.totalDrawCalls = m_stats.terrainDrawCalls +
                             m_stats.buildingsDrawCalls +
                             m_stats.effectsDrawCalls +
                             m_stats.transparentDrawCalls;
    m_stats.totalTriangles = m_stats.terrainTriangles +
                             m_stats.buildingsTriangles +
                             m_stats.effectsTriangles +
                             m_stats.transparentTriangles;

    if (!submitted) {
        m_lastError = "Failed to submit command buffer";
        return false;
    }

    return true;
}

// =============================================================================
// Camera and Pipeline Binding
// =============================================================================

bool MainRenderPass::bindCameraUniforms(const CameraUniforms& camera, UniformBufferPool& uboPool) {
    if (!m_inRenderPass || !m_renderPass) {
        m_lastError = "Not in render pass";
        return false;
    }

    // Upload view-projection matrix via RenderCommands
    RenderCommandStats uploadStats;
    bool uploaded = RenderCommands::uploadViewProjection(
        m_commandBuffer, uboPool, camera.getUBO(), &uploadStats);

    if (!uploaded) {
        m_lastError = "Failed to upload camera uniforms";
        return false;
    }

    return true;
}

void MainRenderPass::bindPipelineOpaque(const ToonPipeline& pipeline) {
    if (!m_inRenderPass || !m_renderPass) {
        return;
    }

    // Use getCurrentOpaquePipeline() to respect wireframe mode setting
    SDL_BindGPUGraphicsPipeline(m_renderPass, pipeline.getCurrentOpaquePipeline());
}

void MainRenderPass::bindPipelineTransparent(const ToonPipeline& pipeline) {
    if (!m_inRenderPass || !m_renderPass) {
        return;
    }

    // Use getCurrentTransparentPipeline() to respect wireframe mode setting
    SDL_BindGPUGraphicsPipeline(m_renderPass, pipeline.getCurrentTransparentPipeline());
}

// =============================================================================
// Layer Rendering
// =============================================================================

void MainRenderPass::renderTerrainLayer(InstancedRenderer& renderer, const ToonPipeline& pipeline,
                                         UniformBufferPool& uboPool) {
    if (!m_inRenderPass || !m_renderPass) {
        return;
    }

    // Bind opaque pipeline for terrain
    bindPipelineOpaque(pipeline);

    // Render terrain instances
    RenderCommandStats stats;
    renderer.render(m_renderPass, m_commandBuffer, pipeline, uboPool, m_state, &stats);

    m_stats.terrainDrawCalls = stats.drawCalls;
    m_stats.terrainTriangles = stats.trianglesDrawn;
}

void MainRenderPass::renderBuildingsLayer(InstancedRenderer& renderer, const ToonPipeline& pipeline,
                                           UniformBufferPool& uboPool) {
    if (!m_inRenderPass || !m_renderPass) {
        return;
    }

    // Bind opaque pipeline for buildings
    bindPipelineOpaque(pipeline);

    // Render building instances
    RenderCommandStats stats;
    renderer.render(m_renderPass, m_commandBuffer, pipeline, uboPool, m_state, &stats);

    m_stats.buildingsDrawCalls = stats.drawCalls;
    m_stats.buildingsTriangles = stats.trianglesDrawn;
}

void MainRenderPass::renderEffectsLayer(InstancedRenderer& renderer, const ToonPipeline& pipeline,
                                         UniformBufferPool& uboPool) {
    if (!m_inRenderPass || !m_renderPass) {
        return;
    }

    // Bind transparent pipeline for effects
    bindPipelineTransparent(pipeline);

    // Render effect instances
    RenderCommandStats stats;
    renderer.render(m_renderPass, m_commandBuffer, pipeline, uboPool, m_state, &stats);

    m_stats.effectsDrawCalls = stats.drawCalls;
    m_stats.effectsTriangles = stats.trianglesDrawn;
}

void MainRenderPass::renderLayer(RenderLayer layer, const LayerRenderCallback& callback) {
    if (!m_inRenderPass || !m_renderPass || !callback) {
        return;
    }

    RenderCommandStats stats;
    callback(m_renderPass, stats);

    // Update stats based on layer
    switch (layer) {
        case RenderLayer::Terrain:
            m_stats.terrainDrawCalls += stats.drawCalls;
            m_stats.terrainTriangles += stats.trianglesDrawn;
            break;
        case RenderLayer::Buildings:
            m_stats.buildingsDrawCalls += stats.drawCalls;
            m_stats.buildingsTriangles += stats.trianglesDrawn;
            break;
        case RenderLayer::Effects:
            m_stats.effectsDrawCalls += stats.drawCalls;
            m_stats.effectsTriangles += stats.trianglesDrawn;
            break;
        default:
            // Other layers contribute to total but not tracked separately
            break;
    }
}

// =============================================================================
// Transparent Rendering (Ticket 2-016)
// =============================================================================

void MainRenderPass::beginTransparentPass(const glm::vec3& cameraPosition) {
    m_transparentQueue.begin(cameraPosition);
}

void MainRenderPass::renderTransparentPass(TransparentRenderQueue& queue, const ToonPipeline& pipeline,
                                            UniformBufferPool& uboPool) {
    if (!m_inRenderPass || !m_renderPass) {
        m_lastError = "MainRenderPass::renderTransparentPass: not in render pass";
        return;
    }

    if (queue.isEmpty()) {
        return;
    }

    // Sort transparent objects back-to-front by camera distance
    queue.sortBackToFront();
    m_stats.transparentSortTimeMs = queue.getStats().sortTimeMs;

    // Bind transparent pipeline (depth test ON, depth write OFF, alpha blend ON)
    bindPipelineTransparent(pipeline);

    // Render all transparent objects in sorted order
    RenderCommandStats stats;
    std::uint32_t drawCalls = queue.render(m_renderPass, m_commandBuffer, pipeline, uboPool, m_state, &stats);

    m_stats.transparentDrawCalls = drawCalls;
    m_stats.transparentTriangles = stats.trianglesDrawn;
}

// =============================================================================
// Configuration
// =============================================================================

void MainRenderPass::setClearColor(const glm::vec4& color) {
    m_config.clearColors.color = color;
}

void MainRenderPass::setBloomConfig(const BloomConfig& config) {
    m_config.bloomConfig = config;
    m_bloomPass.setConfig(config);
}

void MainRenderPass::setEdgeDetectionConfig(const EdgeDetectionConfig& config) {
    m_config.edgeConfig = config;
    m_edgePass.setConfig(config);
}

void MainRenderPass::setShadowConfig(const ShadowConfig& config) {
    m_config.shadowConfig = config;
    m_shadowPass.setConfig(config);
}

void MainRenderPass::setShadowQuality(ShadowQuality quality) {
    m_config.shadowConfig.applyQualityPreset(quality);
    m_shadowPass.setQuality(quality);
}

void MainRenderPass::setShadowsEnabled(bool enable) {
    m_config.enableShadows = enable;
    m_shadowPass.setEnabled(enable);
}

SDL_GPUTexture* MainRenderPass::getShadowMap() const {
    if (areShadowsEnabled()) {
        return m_shadowPass.getShadowMap();
    }
    return nullptr;
}

// =============================================================================
// Window Resize Handling
// =============================================================================

bool MainRenderPass::onResize(std::uint32_t width, std::uint32_t height) {
    if (width == 0 || height == 0) {
        return false;
    }

    m_width = width;
    m_height = height;

    // Resize depth buffer
    if (!m_depthBuffer.resize(width, height)) {
        m_lastError = "Failed to resize depth buffer: " + m_depthBuffer.getLastError();
        return false;
    }

    // Resize normal buffer
    if (!m_normalBuffer.resize(width, height)) {
        m_lastError = "Failed to resize normal buffer: " + m_normalBuffer.getLastError();
        return false;
    }

    // Resize bloom pass
    if (!m_bloomPass.resize(width, height)) {
        m_lastError = "Failed to resize bloom pass: " + m_bloomPass.getLastError();
        return false;
    }

    SDL_Log("MainRenderPass: Resized to %ux%u", width, height);
    return true;
}

// =============================================================================
// Private Implementation
// =============================================================================

bool MainRenderPass::initialize() {
    if (!m_device || !m_device->isValid()) {
        m_lastError = "Invalid GPU device";
        return false;
    }

    if (!m_window || !m_window->isValid()) {
        m_lastError = "Invalid window";
        return false;
    }

    if (!m_window->isClaimed()) {
        m_lastError = "Window not claimed for GPU device";
        return false;
    }

    if (!m_depthBuffer.isValid()) {
        m_lastError = "Failed to create depth buffer: " + m_depthBuffer.getLastError();
        return false;
    }

    if (!m_normalBuffer.isValid()) {
        m_lastError = "Failed to create normal buffer: " + m_normalBuffer.getLastError();
        return false;
    }

    if (!m_edgePass.isValid()) {
        m_lastError = "Failed to create edge detection pass: " + m_edgePass.getLastError();
        return false;
    }

    if (!m_shadowPass.isValid()) {
        m_lastError = "Failed to create shadow pass: " + m_shadowPass.getLastError();
        return false;
    }

    if (!m_bloomPass.isValid()) {
        m_lastError = "Failed to create bloom pass: " + m_bloomPass.getLastError();
        return false;
    }

    SDL_Log("MainRenderPass: Initialized at %ux%u", m_width, m_height);
    SDL_Log("MainRenderPass: Clear color: {%.3f, %.3f, %.3f, %.3f}",
            m_config.clearColors.color.r, m_config.clearColors.color.g,
            m_config.clearColors.color.b, m_config.clearColors.color.a);
    SDL_Log("MainRenderPass: Edge detection enabled: %s", m_config.enableEdgeDetection ? "yes" : "no");
    SDL_Log("MainRenderPass: Shadows enabled: %s (quality: %s, resolution: %u)",
            m_config.enableShadows ? "yes" : "no",
            getShadowQualityName(m_config.shadowConfig.quality),
            m_config.shadowConfig.getShadowMapResolution());
    SDL_Log("MainRenderPass: Bloom enabled: %s", m_config.enableBloom ? "yes" : "no");

    return true;
}

bool MainRenderPass::checkAndHandleResize() {
    if (!m_window) {
        return false;
    }

    std::uint32_t currentWidth = static_cast<std::uint32_t>(m_window->getWidth());
    std::uint32_t currentHeight = static_cast<std::uint32_t>(m_window->getHeight());

    if (currentWidth != m_width || currentHeight != m_height) {
        return onResize(currentWidth, currentHeight);
    }

    return true;
}

} // namespace sims3000
