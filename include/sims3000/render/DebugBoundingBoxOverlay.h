/**
 * @file DebugBoundingBoxOverlay.h
 * @brief Debug visualization of entity bounding boxes for culling verification.
 *
 * Renders wireframe AABB outlines around entities to help verify frustum
 * culling behavior. Visible entities are shown in green, culled entities
 * in red. Toggle via debug key (B).
 *
 * Features:
 * - Wireframe AABB rendering for registered entities
 * - Color-coded visibility status (green=visible, red=culled)
 * - Toggle on/off via debug key
 * - Integration with FrustumCuller for visibility queries
 *
 * Resource ownership:
 * - DebugBoundingBoxOverlay owns pipeline and shader resources
 * - GPUDevice must outlive DebugBoundingBoxOverlay
 * - Call release() before destroying to clean up GPU resources
 *
 * Usage:
 * @code
 *   DebugBoundingBoxOverlay overlay(device, colorFormat, depthFormat);
 *
 *   // Toggle with debug key
 *   if (bboxKeyPressed) overlay.toggle();
 *
 *   // Each frame, after scene rendering:
 *   if (overlay.isEnabled()) {
 *       overlay.render(cmdBuffer, outputTexture, depthTexture,
 *                      width, height, camera, culler, entities);
 *   }
 * @endcode
 */

#ifndef SIMS3000_RENDER_DEBUG_BOUNDING_BOX_OVERLAY_H
#define SIMS3000_RENDER_DEBUG_BOUNDING_BOX_OVERLAY_H

#include "sims3000/render/GPUMesh.h"
#include "sims3000/render/FrustumCuller.h"
#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <cstdint>
#include <string>
#include <vector>

namespace sims3000 {

// Forward declarations
class GPUDevice;
class CameraUniforms;

/**
 * @struct DebugBBoxConfig
 * @brief Configuration for bounding box overlay rendering.
 */
struct DebugBBoxConfig {
    /// Color for visible entities (green)
    glm::vec4 visibleColor{0.2f, 1.0f, 0.3f, 0.8f};

    /// Color for culled entities (red)
    glm::vec4 culledColor{1.0f, 0.2f, 0.2f, 0.6f};

    /// Base line thickness in screen-space pixels
    float lineThickness = 2.0f;

    /// Whether to show culled boxes (for debugging)
    bool showCulledBoxes = true;

    /// Maximum number of boxes to render (performance limit)
    std::uint32_t maxBoxes = 10000;
};

/**
 * @struct DebugBBoxVertex
 * @brief Vertex data for bounding box wireframe rendering.
 */
struct DebugBBoxVertex {
    glm::vec3 position;  ///< World-space position
    glm::vec4 color;     ///< RGBA color

    static constexpr std::size_t stride() { return sizeof(DebugBBoxVertex); }
};
static_assert(sizeof(DebugBBoxVertex) == 28, "DebugBBoxVertex must be 28 bytes");

/**
 * @struct DebugBBoxUBO
 * @brief Uniform buffer data for bounding box shader.
 *
 * Matches the cbuffer layout in debug_bbox.vert.hlsl.
 */
struct DebugBBoxUBO {
    glm::mat4 viewProjection;  // 64 bytes: View-projection matrix
};
static_assert(sizeof(DebugBBoxUBO) == 64, "DebugBBoxUBO must be 64 bytes");

/**
 * @struct BoundingBoxEntry
 * @brief An entity's bounding box with visibility status.
 */
struct BoundingBoxEntry {
    AABB bounds;         ///< World-space bounding box
    bool isVisible;      ///< Whether the entity is visible (not culled)
};

/**
 * @class DebugBoundingBoxOverlay
 * @brief Renders wireframe bounding boxes for debugging frustum culling.
 *
 * Provides visual verification that frustum culling is working correctly
 * by drawing wireframe boxes around entities with color-coded visibility.
 */
class DebugBoundingBoxOverlay {
public:
    /**
     * Create bounding box overlay.
     * @param device GPU device for resource creation
     * @param colorFormat Format of the color render target
     * @param depthFormat Format of the depth buffer
     */
    DebugBoundingBoxOverlay(GPUDevice& device,
                            SDL_GPUTextureFormat colorFormat,
                            SDL_GPUTextureFormat depthFormat);

    ~DebugBoundingBoxOverlay();

    // Non-copyable
    DebugBoundingBoxOverlay(const DebugBoundingBoxOverlay&) = delete;
    DebugBoundingBoxOverlay& operator=(const DebugBoundingBoxOverlay&) = delete;

    // Movable
    DebugBoundingBoxOverlay(DebugBoundingBoxOverlay&& other) noexcept;
    DebugBoundingBoxOverlay& operator=(DebugBoundingBoxOverlay&& other) noexcept;

    /**
     * Check if overlay is valid and ready to use.
     * @return true if resources were created successfully
     */
    bool isValid() const;

    /**
     * Enable or disable the overlay.
     * @param enabled True to enable, false to disable
     */
    void setEnabled(bool enabled);

    /**
     * Toggle the overlay on/off.
     */
    void toggle();

    /**
     * Check if overlay is currently enabled.
     * @return true if enabled
     */
    bool isEnabled() const { return m_enabled; }

    /**
     * Get current configuration.
     * @return Current configuration
     */
    const DebugBBoxConfig& getConfig() const { return m_config; }

    /**
     * Set configuration.
     * @param config New configuration
     */
    void setConfig(const DebugBBoxConfig& config);

    /**
     * Set color for visible entities.
     * @param color RGBA color (default: green)
     */
    void setVisibleColor(const glm::vec4& color);

    /**
     * Set color for culled entities.
     * @param color RGBA color (default: red)
     */
    void setCulledColor(const glm::vec4& color);

    /**
     * Set whether to show culled boxes.
     * @param show True to show culled boxes, false to hide
     */
    void setShowCulledBoxes(bool show);

    /**
     * Render bounding boxes for a list of entries.
     *
     * @param cmdBuffer Command buffer for recording
     * @param outputTexture Output color texture to render to
     * @param depthTexture Depth texture for depth testing
     * @param width Texture width
     * @param height Texture height
     * @param camera Camera uniforms for view-projection matrix
     * @param entries Bounding box entries to render
     * @return true if rendering succeeded
     */
    bool render(
        SDL_GPUCommandBuffer* cmdBuffer,
        SDL_GPUTexture* outputTexture,
        SDL_GPUTexture* depthTexture,
        std::uint32_t width,
        std::uint32_t height,
        const CameraUniforms& camera,
        const std::vector<BoundingBoxEntry>& entries);

    /**
     * Get the last error message.
     * @return Error string
     */
    const std::string& getLastError() const { return m_lastError; }

    /**
     * Get count of boxes rendered last frame.
     * @return Number of boxes rendered
     */
    std::uint32_t getRenderedBoxCount() const { return m_renderedBoxCount; }

private:
    /**
     * Create pipeline and resources.
     * @return true if creation succeeded
     */
    bool createResources();

    /**
     * Release all resources.
     */
    void releaseResources();

    /**
     * Load and compile shaders.
     * @return true if shaders loaded successfully
     */
    bool loadShaders();

    /**
     * Create vertex buffer for box wireframes.
     * @return true if buffer created successfully
     */
    bool createVertexBuffer();

    /**
     * Generate wireframe vertices for a single AABB.
     * @param bounds The bounding box
     * @param color Color for the wireframe
     * @param outVertices Output vertex array (appends 24 vertices)
     */
    void generateBoxVertices(const AABB& bounds,
                             const glm::vec4& color,
                             std::vector<DebugBBoxVertex>& outVertices);

    GPUDevice* m_device = nullptr;
    SDL_GPUTextureFormat m_colorFormat = SDL_GPU_TEXTUREFORMAT_INVALID;
    SDL_GPUTextureFormat m_depthFormat = SDL_GPU_TEXTUREFORMAT_INVALID;

    // Configuration
    DebugBBoxConfig m_config;
    bool m_enabled = false;

    // Pipeline and shaders
    SDL_GPUGraphicsPipeline* m_pipeline = nullptr;
    SDL_GPUShader* m_vertexShader = nullptr;
    SDL_GPUShader* m_fragmentShader = nullptr;

    // Vertex buffer (dynamic, updated each frame)
    SDL_GPUBuffer* m_vertexBuffer = nullptr;
    SDL_GPUTransferBuffer* m_transferBuffer = nullptr;
    std::uint32_t m_vertexBufferCapacity = 0;

    // Stats
    std::uint32_t m_renderedBoxCount = 0;

    std::string m_lastError;
};

} // namespace sims3000

#endif // SIMS3000_RENDER_DEBUG_BOUNDING_BOX_OVERLAY_H
