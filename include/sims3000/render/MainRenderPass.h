/**
 * @file MainRenderPass.h
 * @brief Main render pass structure bringing together all rendering stages.
 *
 * Orchestrates the complete per-frame render flow:
 * 1. Acquire command buffer and swap chain texture
 * 2. Clear color buffer to dark bioluminescent base
 * 3. Clear depth buffer to 1.0
 * 4. Begin render pass with color and depth targets
 * 5. Bind camera uniforms (view/projection matrices)
 * 6. Render terrain layer (opaque)
 * 7. Render buildings layer (opaque)
 * 8. End opaque render pass
 * 9. Edge detection pass (on opaque geometry ONLY)
 * 10. Begin transparent render pass
 * 11. Render transparent objects (sorted back-to-front)
 * 12. End transparent render pass
 * 13. Bloom pass (mandatory pipeline stage)
 * 14. UI overlay pass (Epic 12 UISystem integration point)
 * 15. Submit command buffer
 * 16. Present frame
 *
 * IMPORTANT: Edge detection runs AFTER opaque geometry but BEFORE transparents.
 * This ensures "Edges render only on opaque geometry (before transparents)".
 *
 * Epic 12 UISystem Integration Point:
 * -----------------------------------
 * The UI overlay should be rendered AFTER the bloom pass and BEFORE frame submission.
 * UI uses SDL_GPU for rendering (SDL_Renderer cannot coexist per POC-6).
 *
 * To integrate UISystem:
 * 1. After endFrame() calls executeBloom(), call UISystem::render(cmdBuffer, swapchainTexture)
 * 2. UISystem should use RenderLayer::UIWorld for proper layering
 * 3. UISystem should use DepthState::disabled() for UI elements (no depth testing)
 * 4. UISystem should use BlendState::alphaBlend() for transparent UI elements
 * 5. The swapchain texture contains the complete 3D scene with edges and bloom
 * 6. UI renders on top without erasing the 3D scene (load existing content)
 *
 * Resource ownership:
 * - MainRenderPass owns BloomPass, DepthBuffer, and associated render resources
 * - MainRenderPass does NOT own GPUDevice or Window (external ownership)
 * - MainRenderPass does NOT own ToonPipeline (passed per-frame)
 * - Destruction order: release render resources -> destroy MainRenderPass
 *
 * Usage:
 * @code
 *   GPUDevice device;
 *   Window window(...);
 *   ToonPipeline pipeline(device);
 *   CameraUniforms camera(...);
 *
 *   MainRenderPass renderPass(device, window);
 *
 *   // Each frame:
 *   if (renderPass.beginFrame()) {
 *       renderPass.beginRenderPass();
 *       renderPass.bindCameraUniforms(camera, uboPool);
 *
 *       // 1. Render opaque layers
 *       renderPass.renderTerrainLayer(terrainRenderer, pipeline, uboPool);
 *       renderPass.renderBuildingsLayer(buildingRenderer, pipeline, uboPool);
 *       renderPass.endRenderPass();
 *
 *       // 2. Edge detection (on opaques only, before transparents)
 *       renderPass.executeEdgeDetection();
 *
 *       // 3. Render transparent objects
 *       renderPass.beginTransparentRenderPass();
 *       renderPass.renderTransparentPass(transparentQueue, pipeline, uboPool);
 *       renderPass.endRenderPass();
 *
 *       // 4. End frame (bloom + present)
 *       renderPass.endFrame();
 *   }
 * @endcode
 */

#ifndef SIMS3000_RENDER_MAIN_RENDER_PASS_H
#define SIMS3000_RENDER_MAIN_RENDER_PASS_H

#include "sims3000/render/BloomPass.h"
#include "sims3000/render/DepthBuffer.h"
#include "sims3000/render/NormalBuffer.h"
#include "sims3000/render/EdgeDetectionPass.h"
#include "sims3000/render/ShadowPass.h"
#include "sims3000/render/RenderLayer.h"
#include "sims3000/render/RenderCommands.h"
#include "sims3000/render/TransparentRenderQueue.h"

#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <cstdint>
#include <string>
#include <functional>

namespace sims3000 {

// Forward declarations
class GPUDevice;
class Window;
class CameraUniforms;
class ToonPipeline;
class InstancedRenderer;
class UniformBufferPool;

/**
 * @struct ClearColors
 * @brief Clear colors for the render pass.
 */
struct ClearColors {
    /// Dark bioluminescent base color (deep blue-black)
    /// Canon specification: {0.02, 0.02, 0.05, 1.0}
    glm::vec4 color{0.02f, 0.02f, 0.05f, 1.0f};

    /// Depth clear value (1.0 = far plane)
    float depth = 1.0f;

    /// Stencil clear value (if using stencil buffer)
    std::uint8_t stencil = 0;
};

/**
 * @struct MainRenderPassConfig
 * @brief Configuration for the main render pass.
 */
struct MainRenderPassConfig {
    /// Clear colors for the pass
    ClearColors clearColors;

    /// Bloom configuration
    BloomConfig bloomConfig;

    /// Enable bloom pass (always true per canon, but can be reduced)
    bool enableBloom = true;

    /// Enable edge detection pass
    bool enableEdgeDetection = true;

    /// Edge detection configuration
    EdgeDetectionConfig edgeConfig;

    /// Depth buffer format
    DepthFormat depthFormat = DepthFormat::D32_FLOAT;

    /// Shadow configuration
    ShadowConfig shadowConfig;

    /// Enable shadow pass
    bool enableShadows = true;
};

/**
 * @struct MainRenderPassStats
 * @brief Statistics about render pass execution.
 */
struct MainRenderPassStats {
    // Per-layer stats
    std::uint32_t terrainDrawCalls = 0;
    std::uint32_t buildingsDrawCalls = 0;
    std::uint32_t effectsDrawCalls = 0;
    std::uint32_t transparentDrawCalls = 0;
    std::uint32_t totalDrawCalls = 0;

    // Triangles
    std::uint32_t terrainTriangles = 0;
    std::uint32_t buildingsTriangles = 0;
    std::uint32_t effectsTriangles = 0;
    std::uint32_t transparentTriangles = 0;
    std::uint32_t totalTriangles = 0;

    // Timing (approximate, not GPU-timed)
    float shadowPassTimeMs = 0.0f;
    float sceneRenderTimeMs = 0.0f;
    float transparentSortTimeMs = 0.0f;
    float edgeDetectionTimeMs = 0.0f;
    float bloomTimeMs = 0.0f;
    float totalFrameTimeMs = 0.0f;

    // Shadow stats
    std::uint32_t shadowMapResolution = 0;
    bool shadowsEnabled = false;

    // Frame info
    std::uint32_t frameNumber = 0;
    bool swapchainAcquired = false;

    void reset() {
        terrainDrawCalls = 0;
        buildingsDrawCalls = 0;
        effectsDrawCalls = 0;
        transparentDrawCalls = 0;
        totalDrawCalls = 0;
        terrainTriangles = 0;
        buildingsTriangles = 0;
        effectsTriangles = 0;
        transparentTriangles = 0;
        totalTriangles = 0;
        shadowPassTimeMs = 0.0f;
        sceneRenderTimeMs = 0.0f;
        transparentSortTimeMs = 0.0f;
        edgeDetectionTimeMs = 0.0f;
        bloomTimeMs = 0.0f;
        totalFrameTimeMs = 0.0f;
        shadowsEnabled = false;
        swapchainAcquired = false;
    }
};

/**
 * @brief Layer render callback type.
 *
 * Called by MainRenderPass when rendering each layer.
 * Callback receives render pass handle and command stats to populate.
 */
using LayerRenderCallback = std::function<void(SDL_GPURenderPass*, RenderCommandStats&)>;

/**
 * @class MainRenderPass
 * @brief Orchestrates the complete per-frame render pipeline.
 *
 * Manages the render flow from command buffer acquisition through
 * bloom post-process to frame presentation.
 */
class MainRenderPass {
public:
    /**
     * Create main render pass with default configuration.
     * @param device GPU device for resource creation
     * @param window Window for swap chain operations
     */
    MainRenderPass(GPUDevice& device, Window& window);

    /**
     * Create main render pass with specified configuration.
     * @param device GPU device for resource creation
     * @param window Window for swap chain operations
     * @param config Render pass configuration
     */
    MainRenderPass(GPUDevice& device, Window& window, const MainRenderPassConfig& config);

    ~MainRenderPass();

    // Non-copyable
    MainRenderPass(const MainRenderPass&) = delete;
    MainRenderPass& operator=(const MainRenderPass&) = delete;

    // Movable
    MainRenderPass(MainRenderPass&& other) noexcept;
    MainRenderPass& operator=(MainRenderPass&& other) noexcept;

    /**
     * Check if render pass is valid and ready to use.
     * @return true if resources were created successfully
     */
    bool isValid() const;

    // =========================================================================
    // Frame Lifecycle
    // =========================================================================

    /**
     * Begin a new frame.
     *
     * Performs:
     * - Acquire command buffer
     * - Acquire swap chain texture
     * - Check for window resize and handle
     *
     * @return true if frame can proceed, false if swap chain unavailable
     */
    bool beginFrame();

    /**
     * Begin the main render pass.
     *
     * Performs:
     * - Clear color buffer to dark bioluminescent base
     * - Clear depth buffer to 1.0
     * - Begin GPU render pass with color and depth targets
     *
     * Must be called after beginFrame() and before rendering layers.
     *
     * @return true if render pass began successfully
     */
    bool beginRenderPass();

    /**
     * End the current render pass.
     *
     * Must be called after all layers are rendered, before endFrame().
     */
    void endRenderPass();

    /**
     * Execute edge detection pass on opaque geometry.
     *
     * IMPORTANT: This method MUST be called:
     * - AFTER all opaque layers (terrain, buildings) are rendered
     * - BEFORE renderTransparentPass() is called
     *
     * This ensures edges are detected only on opaque geometry, satisfying
     * the requirement: "Edges render only on opaque geometry (before transparents)"
     *
     * If currently in a render pass, this method will end it before executing
     * edge detection. Call beginTransparentRenderPass() afterward to continue
     * rendering transparent objects.
     *
     * @return true if edge detection executed successfully
     */
    bool executeEdgeDetection();

    /**
     * Begin the transparent render pass.
     *
     * Must be called AFTER executeEdgeDetection() and BEFORE renderTransparentPass().
     * This starts a new GPU render pass that preserves the existing scene color
     * (including edges) and depth buffer for proper transparent blending.
     *
     * @return true if render pass began successfully
     */
    bool beginTransparentRenderPass();

    /**
     * End the frame and present.
     *
     * Performs:
     * - Execute bloom pass (mandatory)
     * - Submit command buffer
     * - Present frame
     *
     * NOTE: Edge detection is NOT executed here. Callers must explicitly call
     * executeEdgeDetection() after opaque layers and before transparent pass.
     *
     * @return true if frame was presented successfully
     */
    bool endFrame();

    // =========================================================================
    // Camera and Pipeline Binding
    // =========================================================================

    /**
     * Bind camera uniforms for rendering.
     *
     * Uploads view-projection matrix to GPU uniform buffer.
     * Must be called after beginRenderPass(), before rendering layers.
     *
     * @param camera Camera uniforms containing view-projection matrix
     * @param uboPool Uniform buffer pool for allocation
     * @return true if binding succeeded
     */
    bool bindCameraUniforms(const CameraUniforms& camera, UniformBufferPool& uboPool);

    /**
     * Bind the toon pipeline for opaque rendering.
     *
     * @param pipeline Toon pipeline to bind
     */
    void bindPipelineOpaque(const ToonPipeline& pipeline);

    /**
     * Bind the toon pipeline for transparent rendering.
     *
     * @param pipeline Toon pipeline to bind
     */
    void bindPipelineTransparent(const ToonPipeline& pipeline);

    // =========================================================================
    // Layer Rendering
    // =========================================================================

    /**
     * Render the terrain layer.
     *
     * @param renderer Instanced renderer with terrain instances
     * @param pipeline Toon pipeline for rendering
     * @param uboPool Uniform buffer pool
     */
    void renderTerrainLayer(InstancedRenderer& renderer, const ToonPipeline& pipeline,
                            UniformBufferPool& uboPool);

    /**
     * Render the buildings layer.
     *
     * @param renderer Instanced renderer with building instances
     * @param pipeline Toon pipeline for rendering
     * @param uboPool Uniform buffer pool
     */
    void renderBuildingsLayer(InstancedRenderer& renderer, const ToonPipeline& pipeline,
                              UniformBufferPool& uboPool);

    /**
     * Render the effects layer.
     *
     * @param renderer Instanced renderer with effect instances
     * @param pipeline Toon pipeline for rendering
     * @param uboPool Uniform buffer pool
     */
    void renderEffectsLayer(InstancedRenderer& renderer, const ToonPipeline& pipeline,
                            UniformBufferPool& uboPool);

    /**
     * Render a layer with a custom callback.
     *
     * @param layer Render layer to render
     * @param callback Custom render callback
     */
    void renderLayer(RenderLayer layer, const LayerRenderCallback& callback);

    // =========================================================================
    // Transparent Rendering (Ticket 2-016)
    // =========================================================================

    /**
     * Render transparent objects from the queue.
     *
     * This method:
     * 1. Binds the transparent pipeline (depth test ON, depth write OFF, alpha blend)
     * 2. Sorts transparent objects back-to-front by camera distance
     * 3. Renders each object in sorted order
     *
     * Must be called AFTER all opaque layers are rendered to ensure the depth
     * buffer is fully populated for correct occlusion.
     *
     * @param queue TransparentRenderQueue with objects to render
     * @param pipeline Toon pipeline (transparent variant will be used)
     * @param uboPool Uniform buffer pool
     */
    void renderTransparentPass(TransparentRenderQueue& queue, const ToonPipeline& pipeline,
                               UniformBufferPool& uboPool);

    /**
     * Get the transparent render queue for adding objects.
     *
     * Call beginTransparentPass() at the start of each frame before adding
     * transparent objects.
     *
     * @return Reference to the transparent render queue
     */
    TransparentRenderQueue& getTransparentQueue() { return m_transparentQueue; }
    const TransparentRenderQueue& getTransparentQueue() const { return m_transparentQueue; }

    /**
     * Begin the transparent pass, clearing the queue and setting camera position.
     *
     * @param cameraPosition Camera world position for distance sorting
     */
    void beginTransparentPass(const glm::vec3& cameraPosition);

    // =========================================================================
    // UI Overlay (Epic 12 Integration Point)
    // =========================================================================

    /**
     * Begin a UI overlay render pass.
     *
     * This is the integration point for Epic 12 UISystem. Call this method
     * AFTER the transparent pass ends and BEFORE endFrame().
     *
     * The UI pass:
     * - Uses SDL_GPU_LOADOP_LOAD to preserve the 3D scene (including bloom)
     * - Uses depth testing disabled (UI always on top)
     * - Uses alpha blending for transparent UI elements
     *
     * Usage:
     * @code
     *   // After transparent pass
     *   renderPass.endRenderPass();
     *
     *   // Begin UI overlay
     *   if (renderPass.beginUIOverlayPass()) {
     *       uiSystem.render(renderPass.getRenderPass());
     *       renderPass.endRenderPass();
     *   }
     *
     *   renderPass.endFrame();
     * @endcode
     *
     * @return true if UI pass began successfully
     */
    bool beginUIOverlayPass();

    /**
     * Check if the current frame is ready for UI rendering.
     *
     * @return true if in a valid state for UI rendering (after bloom, before submit)
     */
    bool isReadyForUI() const { return m_inFrame && !m_inRenderPass && m_swapchainTexture != nullptr; }

    // =========================================================================
    // Configuration
    // =========================================================================

    /**
     * Get current configuration.
     * @return Current configuration
     */
    const MainRenderPassConfig& getConfig() const { return m_config; }

    /**
     * Set clear color.
     * @param color New clear color
     */
    void setClearColor(const glm::vec4& color);

    /**
     * Set bloom configuration.
     * @param config New bloom configuration
     */
    void setBloomConfig(const BloomConfig& config);

    // =========================================================================
    // Window Resize Handling
    // =========================================================================

    /**
     * Handle window resize.
     *
     * Recreates depth buffer and bloom targets at new resolution.
     *
     * @param width New width
     * @param height New height
     * @return true if resize succeeded
     */
    bool onResize(std::uint32_t width, std::uint32_t height);

    // =========================================================================
    // Access
    // =========================================================================

    /**
     * Get the current command buffer.
     * Only valid between beginFrame() and endFrame().
     * @return Command buffer, or nullptr if not in frame
     */
    SDL_GPUCommandBuffer* getCommandBuffer() const { return m_commandBuffer; }

    /**
     * Get the current render pass.
     * Only valid between beginRenderPass() and endRenderPass().
     * @return Render pass, or nullptr if not in render pass
     */
    SDL_GPURenderPass* getRenderPass() const { return m_renderPass; }

    /**
     * Get the depth buffer.
     * @return Reference to depth buffer
     */
    DepthBuffer& getDepthBuffer() { return m_depthBuffer; }
    const DepthBuffer& getDepthBuffer() const { return m_depthBuffer; }

    /**
     * Get the bloom pass.
     * @return Reference to bloom pass
     */
    BloomPass& getBloomPass() { return m_bloomPass; }
    const BloomPass& getBloomPass() const { return m_bloomPass; }

    /**
     * Get the shadow pass.
     * @return Reference to shadow pass
     */
    ShadowPass& getShadowPass() { return m_shadowPass; }
    const ShadowPass& getShadowPass() const { return m_shadowPass; }

    /**
     * Set shadow configuration.
     * @param config New shadow configuration
     */
    void setShadowConfig(const ShadowConfig& config);

    /**
     * Set shadow quality tier.
     * @param quality Quality tier (Disabled, Low, Medium, High, Ultra)
     */
    void setShadowQuality(ShadowQuality quality);

    /**
     * Enable or disable shadows.
     * @param enable Whether to enable shadow rendering
     */
    void setShadowsEnabled(bool enable);

    /**
     * Check if shadows are enabled.
     * @return true if shadows are enabled
     */
    bool areShadowsEnabled() const { return m_config.enableShadows && m_shadowPass.isEnabled(); }

    /**
     * Get the shadow map texture for binding.
     * @return Shadow map depth texture, or nullptr if shadows disabled
     */
    SDL_GPUTexture* getShadowMap() const;

    /**
     * Get the light view-projection matrix.
     * @return Light space view-projection matrix for shadow mapping
     */
    const glm::mat4& getLightViewProjectionMatrix() const { return m_shadowPass.getLightViewProjectionMatrix(); }

    /**
     * Get the normal buffer.
     * @return Reference to normal buffer
     */
    NormalBuffer& getNormalBuffer() { return m_normalBuffer; }
    const NormalBuffer& getNormalBuffer() const { return m_normalBuffer; }

    /**
     * Get the edge detection pass.
     * @return Reference to edge detection pass
     */
    EdgeDetectionPass& getEdgeDetectionPass() { return m_edgePass; }
    const EdgeDetectionPass& getEdgeDetectionPass() const { return m_edgePass; }

    /**
     * Set edge detection configuration.
     * @param config New edge detection configuration
     */
    void setEdgeDetectionConfig(const EdgeDetectionConfig& config);

    /**
     * Get render pass state for redundancy tracking.
     * @return Reference to render pass state
     */
    RenderPassState& getState() { return m_state; }

    /**
     * Get execution statistics from last frame.
     * @return Render statistics
     */
    const MainRenderPassStats& getStats() const { return m_stats; }

    /**
     * Get the last error message.
     * @return Error string
     */
    const std::string& getLastError() const { return m_lastError; }

    /**
     * Get current swap chain texture format.
     * @return Texture format
     */
    SDL_GPUTextureFormat getSwapchainFormat() const { return m_swapchainFormat; }

private:
    /**
     * Initialize render pass resources.
     * @return true if initialization succeeded
     */
    bool initialize();

    /**
     * Check and handle window resize.
     * @return true if resize was needed and succeeded, or no resize needed
     */
    bool checkAndHandleResize();

    GPUDevice* m_device = nullptr;
    Window* m_window = nullptr;

    // Configuration
    MainRenderPassConfig m_config;

    // Owned resources
    DepthBuffer m_depthBuffer;
    NormalBuffer m_normalBuffer;
    EdgeDetectionPass m_edgePass;
    ShadowPass m_shadowPass;
    BloomPass m_bloomPass;
    TransparentRenderQueue m_transparentQueue;

    // Intermediate render target for edge detection
    SDL_GPUTexture* m_sceneColorTarget = nullptr;

    // Frame state
    SDL_GPUCommandBuffer* m_commandBuffer = nullptr;
    SDL_GPURenderPass* m_renderPass = nullptr;
    SDL_GPUTexture* m_swapchainTexture = nullptr;
    SDL_GPUTextureFormat m_swapchainFormat = SDL_GPU_TEXTUREFORMAT_INVALID;
    bool m_inFrame = false;
    bool m_inRenderPass = false;

    // Render pass state tracking
    RenderPassState m_state;

    // Cached dimensions
    std::uint32_t m_width = 0;
    std::uint32_t m_height = 0;

    // Statistics
    MainRenderPassStats m_stats;
    std::uint32_t m_frameNumber = 0;

    std::string m_lastError;
};

} // namespace sims3000

#endif // SIMS3000_RENDER_MAIN_RENDER_PASS_H
