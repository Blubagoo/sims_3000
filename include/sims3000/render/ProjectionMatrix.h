/**
 * @file ProjectionMatrix.h
 * @brief Perspective projection matrix calculation for the camera system.
 *
 * Calculates perspective projection matrix with configurable vertical FOV
 * (default 35 degrees for a natural free camera feel with minimal foreshortening
 * at isometric preset angles).
 *
 * Coordinate system:
 * - Uses right-handed coordinate system (OpenGL/Vulkan convention)
 * - Near plane: 0.1 (close to camera for detailed nearby rendering)
 * - Far plane: 1000.0 (distant enough for large maps)
 * - Depth range: [0, 1] (Vulkan/SDL_GPU convention)
 *
 * Resource ownership: None (pure functions, no GPU/SDL resources).
 */

#ifndef SIMS3000_RENDER_PROJECTIONMATRIX_H
#define SIMS3000_RENDER_PROJECTIONMATRIX_H

#include <glm/glm.hpp>

namespace sims3000 {

// ============================================================================
// Projection Configuration Constants
// ============================================================================

/**
 * @namespace ProjectionConfig
 * @brief Configuration parameters for perspective projection.
 */
namespace ProjectionConfig {
    /// Default vertical field of view in degrees.
    /// 35 degrees provides a natural free camera feel with minimal foreshortening
    /// at the isometric preset pitch angle (~35.264 degrees).
    constexpr float DEFAULT_FOV_DEGREES = 35.0f;

    /// Minimum allowed vertical FOV in degrees.
    /// Prevents extreme telephoto distortion.
    constexpr float MIN_FOV_DEGREES = 20.0f;

    /// Maximum allowed vertical FOV in degrees.
    /// Prevents extreme wide-angle distortion.
    constexpr float MAX_FOV_DEGREES = 90.0f;

    /// Near clipping plane distance.
    /// Objects closer than this are clipped.
    constexpr float NEAR_PLANE = 0.1f;

    /// Far clipping plane distance.
    /// Objects farther than this are clipped.
    constexpr float FAR_PLANE = 1000.0f;

    /// Degrees to radians conversion factor.
    constexpr float DEG_TO_RAD = 3.14159265358979323846f / 180.0f;
}

// ============================================================================
// Projection Matrix Functions
// ============================================================================

/**
 * @brief Calculate perspective projection matrix.
 *
 * Creates a perspective projection matrix using the specified vertical FOV,
 * aspect ratio, and near/far planes. Uses right-handed coordinate system
 * with depth range [0, 1] for Vulkan/SDL_GPU compatibility.
 *
 * The perspective divide is applied by the GPU during rasterization:
 * clip_coords = projection * view * model * vertex
 * ndc_coords = clip_coords.xyz / clip_coords.w
 *
 * @param fov_degrees Vertical field of view in degrees.
 * @param aspect_ratio Width / height ratio (e.g., 16/9 for widescreen).
 * @param near_plane Distance to near clipping plane (must be > 0).
 * @param far_plane Distance to far clipping plane (must be > near_plane).
 * @return 4x4 perspective projection matrix.
 *
 * @note FOV is clamped to [MIN_FOV_DEGREES, MAX_FOV_DEGREES].
 * @note Aspect ratio <= 0 is treated as 1.0 to avoid division by zero.
 */
glm::mat4 calculateProjectionMatrix(
    float fov_degrees,
    float aspect_ratio,
    float near_plane = ProjectionConfig::NEAR_PLANE,
    float far_plane = ProjectionConfig::FAR_PLANE);

/**
 * @brief Calculate perspective projection matrix with default FOV.
 *
 * Convenience function using the default 35-degree vertical FOV.
 * Use this when you only need to specify aspect ratio and want default settings.
 *
 * @param aspect_ratio Width / height ratio.
 * @param near_plane Distance to near clipping plane (default 0.1).
 * @param far_plane Distance to far clipping plane (default 1000.0).
 * @return 4x4 perspective projection matrix.
 */
glm::mat4 calculateProjectionMatrixDefault(
    float aspect_ratio,
    float near_plane = ProjectionConfig::NEAR_PLANE,
    float far_plane = ProjectionConfig::FAR_PLANE);

/**
 * @brief Calculate perspective projection matrix from window dimensions.
 *
 * Computes aspect ratio from width/height and uses default projection settings.
 *
 * @param window_width Window width in pixels (must be > 0).
 * @param window_height Window height in pixels (must be > 0).
 * @param fov_degrees Vertical field of view in degrees.
 * @param near_plane Distance to near clipping plane (default 0.1).
 * @param far_plane Distance to far clipping plane (default 1000.0).
 * @return 4x4 perspective projection matrix.
 *
 * @note If window dimensions are invalid (<=0), uses aspect ratio of 1.0.
 */
glm::mat4 calculateProjectionMatrixFromDimensions(
    int window_width,
    int window_height,
    float fov_degrees = ProjectionConfig::DEFAULT_FOV_DEGREES,
    float near_plane = ProjectionConfig::NEAR_PLANE,
    float far_plane = ProjectionConfig::FAR_PLANE);

/**
 * @brief Calculate view-projection matrix.
 *
 * Combines a view matrix and projection matrix into a single matrix
 * for efficient GPU uploads. Order: projection * view.
 *
 * @param view_matrix The view matrix (from calculateViewMatrix).
 * @param projection_matrix The projection matrix.
 * @return Combined view-projection matrix.
 */
glm::mat4 calculateViewProjectionMatrix(
    const glm::mat4& view_matrix,
    const glm::mat4& projection_matrix);

/**
 * @brief Clamp FOV to valid range.
 *
 * Ensures FOV stays within [MIN_FOV_DEGREES, MAX_FOV_DEGREES].
 *
 * @param fov_degrees Input FOV in degrees.
 * @return Clamped FOV in degrees.
 */
float clampFOV(float fov_degrees);

/**
 * @brief Calculate aspect ratio from dimensions.
 *
 * @param width Width in pixels.
 * @param height Height in pixels.
 * @return Aspect ratio (width/height), or 1.0 if dimensions are invalid.
 */
float calculateAspectRatio(int width, int height);

/**
 * @brief Check if projection parameters are valid.
 *
 * Validates that FOV, aspect ratio, and near/far planes are in acceptable ranges.
 *
 * @param fov_degrees Field of view in degrees.
 * @param aspect_ratio Width / height ratio.
 * @param near_plane Near clipping distance.
 * @param far_plane Far clipping distance.
 * @return true if all parameters are valid.
 */
bool validateProjectionParameters(
    float fov_degrees,
    float aspect_ratio,
    float near_plane,
    float far_plane);

} // namespace sims3000

#endif // SIMS3000_RENDER_PROJECTIONMATRIX_H
