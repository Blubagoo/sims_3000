/**
 * @file CameraUniforms.h
 * @brief View-projection matrix integration and GPU uniform management.
 *
 * Combines view and projection matrices from CameraState into a single
 * view-projection matrix for efficient GPU upload. Handles aspect ratio
 * changes on window resize and provides automatic projection recalculation.
 *
 * Usage:
 * @code
 *   CameraUniforms camera(1920, 1080);
 *
 *   // Each frame:
 *   camera.update(cameraState);
 *
 *   // Upload to GPU via RenderCommands:
 *   RenderCommands::uploadViewProjection(cmdBuffer, uboPool, camera.getUBO());
 *
 *   // On window resize:
 *   camera.onWindowResize(newWidth, newHeight);
 * @endcode
 *
 * Resource ownership:
 * - CameraUniforms does not own any GPU resources
 * - UBO data is copied, not referenced
 * - Thread safety: Not thread-safe, call from render thread only
 */

#ifndef SIMS3000_RENDER_CAMERA_UNIFORMS_H
#define SIMS3000_RENDER_CAMERA_UNIFORMS_H

#include "sims3000/render/CameraState.h"
#include "sims3000/render/ToonShader.h"
#include <glm/glm.hpp>
#include <cstdint>

namespace sims3000 {

/**
 * @class CameraUniforms
 * @brief Manages view-projection matrix calculation and GPU uniform data.
 *
 * Provides a unified interface for:
 * - Combining view and projection matrices
 * - Handling window resize with aspect ratio updates
 * - Preparing UBO data for GPU upload
 */
class CameraUniforms {
public:
    /**
     * @brief Construct camera uniforms with initial window dimensions.
     *
     * @param windowWidth Initial window width in pixels.
     * @param windowHeight Initial window height in pixels.
     * @param fovDegrees Vertical field of view in degrees (default from CameraConfig).
     * @param nearPlane Near clipping plane distance (default from CameraConfig).
     * @param farPlane Far clipping plane distance (default from CameraConfig).
     */
    explicit CameraUniforms(
        std::uint32_t windowWidth = 1280,
        std::uint32_t windowHeight = 720,
        float fovDegrees = CameraConfig::FOV_DEFAULT,
        float nearPlane = CameraConfig::NEAR_PLANE,
        float farPlane = CameraConfig::FAR_PLANE);

    /**
     * @brief Update matrices from camera state.
     *
     * Recalculates the view matrix from the camera state and combines
     * with the projection matrix to produce the view-projection matrix.
     *
     * @param state Current camera state.
     */
    void update(const CameraState& state);

    /**
     * @brief Handle window resize event.
     *
     * Updates the aspect ratio and recalculates the projection matrix.
     * Call this when the window dimensions change.
     *
     * @param newWidth New window width in pixels.
     * @param newHeight New window height in pixels.
     */
    void onWindowResize(std::uint32_t newWidth, std::uint32_t newHeight);

    /**
     * @brief Get the combined view-projection matrix.
     *
     * @return The current view-projection matrix (projection * view).
     */
    const glm::mat4& getViewProjectionMatrix() const { return m_viewProjection; }

    /**
     * @brief Get the view matrix.
     *
     * @return The current view matrix.
     */
    const glm::mat4& getViewMatrix() const { return m_view; }

    /**
     * @brief Get the projection matrix.
     *
     * @return The current projection matrix.
     */
    const glm::mat4& getProjectionMatrix() const { return m_projection; }

    /**
     * @brief Get the UBO data for GPU upload.
     *
     * Returns a reference to the UBO structure containing the view-projection
     * matrix, ready for upload via RenderCommands::uploadViewProjection().
     *
     * @return Reference to the UBO data structure.
     */
    const ToonViewProjectionUBO& getUBO() const { return m_ubo; }

    /**
     * @brief Get the current aspect ratio.
     *
     * @return Width / height ratio.
     */
    float getAspectRatio() const { return m_aspectRatio; }

    /**
     * @brief Get the current window width.
     *
     * @return Window width in pixels.
     */
    std::uint32_t getWindowWidth() const { return m_windowWidth; }

    /**
     * @brief Get the current window height.
     *
     * @return Window height in pixels.
     */
    std::uint32_t getWindowHeight() const { return m_windowHeight; }

    /**
     * @brief Get the current field of view.
     *
     * @return Vertical FOV in degrees.
     */
    float getFOV() const { return m_fovDegrees; }

    /**
     * @brief Set the field of view.
     *
     * Updates the projection matrix with the new FOV.
     *
     * @param fovDegrees New vertical FOV in degrees (clamped to valid range).
     */
    void setFOV(float fovDegrees);

    /**
     * @brief Get the near clipping plane distance.
     *
     * @return Near plane distance.
     */
    float getNearPlane() const { return m_nearPlane; }

    /**
     * @brief Get the far clipping plane distance.
     *
     * @return Far plane distance.
     */
    float getFarPlane() const { return m_farPlane; }

    /**
     * @brief Set near and far clipping planes.
     *
     * Updates the projection matrix with new clipping plane distances.
     *
     * @param nearPlane New near plane distance (must be > 0).
     * @param farPlane New far plane distance (must be > nearPlane).
     */
    void setClippingPlanes(float nearPlane, float farPlane);

    /**
     * @brief Check if the projection matrix was recalculated this frame.
     *
     * Useful for detecting when the projection changed (e.g., after resize).
     *
     * @return true if projection was recalculated since last update.
     */
    bool wasProjectionRecalculated() const { return m_projectionDirty; }

    /**
     * @brief Force recalculation of projection matrix.
     *
     * Call this after changing FOV or clipping planes to ensure
     * the projection is up to date.
     */
    void recalculateProjection();

private:
    /**
     * @brief Update aspect ratio from window dimensions.
     */
    void updateAspectRatio();

    // Window dimensions
    std::uint32_t m_windowWidth;
    std::uint32_t m_windowHeight;

    // Projection parameters
    float m_fovDegrees;
    float m_nearPlane;
    float m_farPlane;
    float m_aspectRatio;

    // Cached matrices
    glm::mat4 m_view{1.0f};
    glm::mat4 m_projection{1.0f};
    glm::mat4 m_viewProjection{1.0f};

    // UBO data for GPU upload
    ToonViewProjectionUBO m_ubo;

    // State tracking
    bool m_projectionDirty{true};
};

} // namespace sims3000

#endif // SIMS3000_RENDER_CAMERA_UNIFORMS_H
