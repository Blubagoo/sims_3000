/**
 * @file ShadowPass.h
 * @brief Shadow map generation pass for directional light shadows.
 *
 * Implements basic shadow mapping for the alien sun directional light:
 * - Renders scene from light's perspective to create shadow map
 * - Orthographic projection fitted to camera frustum
 * - Texel snapping for stable shadows during camera movement
 * - World-space fixed light direction (alien sun)
 *
 * The shadow frustum is calculated to tightly bound the main camera's
 * view frustum from the light's perspective, maximizing shadow map
 * resolution usage.
 *
 * Texel snapping prevents shadow map texels from shifting position
 * during camera panning, eliminating shimmering artifacts.
 *
 * Resource ownership:
 * - ShadowPass owns the shadow map depth texture
 * - ShadowPass owns the shadow depth pipeline
 * - GPUDevice must outlive ShadowPass
 * - ShadowPass does NOT own shaders (loaded externally)
 *
 * Usage:
 * @code
 *   GPUDevice device;
 *   ShadowConfig config;
 *   ShadowPass shadowPass(device, config);
 *
 *   // Each frame:
 *   glm::mat4 cameraView = ...;
 *   glm::mat4 cameraProjection = ...;
 *
 *   // Calculate light matrices
 *   shadowPass.updateLightMatrices(cameraView, cameraProjection);
 *
 *   // Execute shadow pass
 *   SDL_GPUCommandBuffer* cmd = device.acquireCommandBuffer();
 *   shadowPass.begin(cmd);
 *   // ... bind shadow pipeline and render shadow-casting geometry ...
 *   shadowPass.end(cmd);
 *
 *   // Use shadow map in main pass
 *   SDL_GPUTexture* shadowMap = shadowPass.getShadowMap();
 * @endcode
 */

#ifndef SIMS3000_RENDER_SHADOW_PASS_H
#define SIMS3000_RENDER_SHADOW_PASS_H

#include "sims3000/render/ShadowConfig.h"
#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <cstdint>
#include <string>

namespace sims3000 {

// Forward declarations
class GPUDevice;

/**
 * @struct ShadowPassStats
 * @brief Statistics about shadow pass execution.
 */
struct ShadowPassStats {
    std::uint32_t drawCalls = 0;
    std::uint32_t triangles = 0;
    float executionTimeMs = 0.0f;
    std::uint32_t resolution = 0;
    bool executed = false;

    void reset() {
        drawCalls = 0;
        triangles = 0;
        executionTimeMs = 0.0f;
        executed = false;
    }
};

/**
 * @class ShadowPass
 * @brief Generates shadow map from directional light perspective.
 *
 * Creates and manages the shadow map depth texture, calculates the
 * light-space view/projection matrices, and provides the render pass
 * configuration for shadow geometry rendering.
 */
class ShadowPass {
public:
    /**
     * Create shadow pass with configuration.
     * @param device GPU device for resource creation
     * @param config Shadow configuration
     */
    ShadowPass(GPUDevice& device, const ShadowConfig& config = ShadowConfig{});

    ~ShadowPass();

    // Non-copyable
    ShadowPass(const ShadowPass&) = delete;
    ShadowPass& operator=(const ShadowPass&) = delete;

    // Movable
    ShadowPass(ShadowPass&& other) noexcept;
    ShadowPass& operator=(ShadowPass&& other) noexcept;

    /**
     * Check if shadow pass is valid and ready.
     * @return true if resources are created successfully
     */
    bool isValid() const;

    // =========================================================================
    // Configuration
    // =========================================================================

    /**
     * Get current configuration.
     * @return Shadow configuration reference
     */
    const ShadowConfig& getConfig() const { return m_config; }

    /**
     * Set shadow configuration.
     * @param config New configuration
     *
     * Recreates shadow map if resolution changed.
     */
    void setConfig(const ShadowConfig& config);

    /**
     * Set shadow quality tier.
     * @param quality Quality tier
     *
     * Convenience method that applies quality preset.
     */
    void setQuality(ShadowQuality quality);

    /**
     * Enable or disable shadows.
     * @param enable Whether to enable shadows
     */
    void setEnabled(bool enable) { m_config.enabled = enable; }

    /**
     * Check if shadows are enabled.
     * @return true if shadows are enabled and quality is not Disabled
     */
    bool isEnabled() const { return m_config.isEnabled(); }

    // =========================================================================
    // Light Matrix Calculation
    // =========================================================================

    /**
     * Update light-space matrices based on camera frustum.
     *
     * Calculates the orthographic projection that tightly bounds the
     * camera's view frustum from the light's perspective.
     *
     * @param cameraView Main camera view matrix
     * @param cameraProjection Main camera projection matrix
     * @param cameraPosition Main camera world position (for frustum calculation)
     */
    void updateLightMatrices(
        const glm::mat4& cameraView,
        const glm::mat4& cameraProjection,
        const glm::vec3& cameraPosition);

    /**
     * Get the light-space view matrix.
     * @return Light view matrix (world to light space)
     */
    const glm::mat4& getLightViewMatrix() const { return m_lightView; }

    /**
     * Get the light-space projection matrix.
     * @return Light orthographic projection matrix
     */
    const glm::mat4& getLightProjectionMatrix() const { return m_lightProjection; }

    /**
     * Get the combined light view-projection matrix.
     * @return Light view-projection matrix for shader upload
     */
    const glm::mat4& getLightViewProjectionMatrix() const { return m_lightViewProjection; }

    // =========================================================================
    // Render Pass Execution
    // =========================================================================

    /**
     * Begin shadow pass.
     *
     * Starts a render pass targeting the shadow map depth texture.
     * Clears depth to 1.0.
     *
     * @param cmdBuffer Command buffer to record to
     * @return true if render pass started successfully
     */
    bool begin(SDL_GPUCommandBuffer* cmdBuffer);

    /**
     * End shadow pass.
     *
     * Ends the current render pass.
     */
    void end();

    /**
     * Get the current render pass handle.
     * @return Render pass handle, or nullptr if not in pass
     */
    SDL_GPURenderPass* getRenderPass() const { return m_renderPass; }

    // =========================================================================
    // Shadow Map Access
    // =========================================================================

    /**
     * Get the shadow map depth texture.
     * @return Shadow map texture for sampling in main pass
     */
    SDL_GPUTexture* getShadowMap() const { return m_shadowMap; }

    /**
     * Get shadow map resolution.
     * @return Shadow map width/height in pixels
     */
    std::uint32_t getResolution() const { return m_resolution; }

    // =========================================================================
    // Statistics
    // =========================================================================

    /**
     * Get execution statistics.
     * @return Shadow pass statistics
     */
    const ShadowPassStats& getStats() const { return m_stats; }

    /**
     * Get last error message.
     * @return Error string
     */
    const std::string& getLastError() const { return m_lastError; }

private:
    /**
     * Create shadow map texture and related resources.
     * @return true if creation succeeded
     */
    bool createResources();

    /**
     * Release all GPU resources.
     */
    void releaseResources();

    /**
     * Apply texel snapping to light matrices.
     *
     * Quantizes the light frustum to shadow map texels to prevent
     * shimmering during camera movement.
     */
    void applyTexelSnapping();

    /**
     * Calculate frustum corners in world space.
     * @param inverseViewProjection Inverse of camera view-projection matrix
     * @param corners Output array of 8 frustum corners
     */
    void calculateFrustumCorners(
        const glm::mat4& inverseViewProjection,
        glm::vec3 corners[8]) const;

    /**
     * Calculate light-space bounding box from world-space points.
     * @param corners World-space frustum corners
     * @param minBounds Output minimum bounds in light space
     * @param maxBounds Output maximum bounds in light space
     */
    void calculateLightSpaceBounds(
        const glm::vec3 corners[8],
        glm::vec3& minBounds,
        glm::vec3& maxBounds) const;

    GPUDevice* m_device = nullptr;
    ShadowConfig m_config;

    // GPU resources
    SDL_GPUTexture* m_shadowMap = nullptr;
    SDL_GPURenderPass* m_renderPass = nullptr;
    SDL_GPUCommandBuffer* m_currentCmdBuffer = nullptr;
    std::uint32_t m_resolution = 0;

    // Light matrices
    glm::mat4 m_lightView{1.0f};
    glm::mat4 m_lightProjection{1.0f};
    glm::mat4 m_lightViewProjection{1.0f};

    // Frustum center for texel snapping
    glm::vec3 m_frustumCenter{0.0f};

    // State tracking
    bool m_inPass = false;
    ShadowPassStats m_stats;
    std::string m_lastError;
};

} // namespace sims3000

#endif // SIMS3000_RENDER_SHADOW_PASS_H
