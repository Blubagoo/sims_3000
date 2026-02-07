/**
 * @file ViewMatrix.h
 * @brief View matrix calculation from camera state.
 *
 * Calculates view matrix from CameraState's spherical coordinates (focus_point,
 * distance, pitch, yaw). Camera position is derived from these parameters and
 * the view matrix is computed via standard lookAt function.
 *
 * Coordinate system:
 * - X-axis: East (right)
 * - Y-axis: Up (elevation)
 * - Z-axis: South (down in map, into screen at default view)
 *
 * Spherical coordinate conventions:
 * - Pitch: Angle from horizontal (0 = horizontal, 90 = straight down)
 *          Clamped to 15-80 degrees by CameraState to avoid gimbal lock
 * - Yaw: Angle around Y-axis from North (0 = looking South, rotates clockwise)
 *        Preset yaw values: N=45, E=135, S=225, W=315
 *
 * Resource ownership: None (pure functions, no GPU/SDL resources).
 */

#ifndef SIMS3000_RENDER_VIEWMATRIX_H
#define SIMS3000_RENDER_VIEWMATRIX_H

#include "sims3000/render/CameraState.h"
#include <glm/glm.hpp>

namespace sims3000 {

// ============================================================================
// View Matrix Constants
// ============================================================================

/**
 * @brief Constants for view matrix calculation.
 */
namespace ViewMatrixConfig {
    /// World up vector - Y is up
    constexpr glm::vec3 WORLD_UP{0.0f, 1.0f, 0.0f};

    /// Degrees to radians conversion factor
    constexpr float DEG_TO_RAD = 3.14159265358979323846f / 180.0f;
}

// ============================================================================
// View Matrix Functions
// ============================================================================

/**
 * @brief Calculate camera position from spherical coordinates.
 *
 * Converts the camera's orbital parameters (focus_point, distance, pitch, yaw)
 * into a world-space camera position. The camera orbits around the focus point
 * at the specified distance, looking towards the focus point.
 *
 * @param focus_point The point the camera looks at (world coordinates).
 * @param distance Distance from focus point to camera position.
 * @param pitch_degrees Vertical angle in degrees (15-80, where higher = more top-down).
 * @param yaw_degrees Horizontal angle in degrees (0-360, where 0 = looking South).
 * @return Camera world-space position.
 *
 * @note Pitch is measured from horizontal (0 = horizontal view, 90 = straight down).
 *       The CameraState clamps pitch to 15-80 degrees, avoiding gimbal lock at poles.
 */
glm::vec3 calculateCameraPosition(
    const glm::vec3& focus_point,
    float distance,
    float pitch_degrees,
    float yaw_degrees);

/**
 * @brief Calculate camera position from CameraState.
 *
 * Convenience overload that extracts parameters from CameraState.
 *
 * @param state The camera state containing focus_point, distance, pitch, yaw.
 * @return Camera world-space position.
 */
glm::vec3 calculateCameraPosition(const CameraState& state);

/**
 * @brief Calculate view matrix from explicit parameters.
 *
 * Creates a view matrix that transforms world coordinates to view coordinates.
 * Uses the standard lookAt formulation: camera at position, looking at target,
 * with up vector (0, 1, 0).
 *
 * @param focus_point The point the camera looks at (target).
 * @param distance Distance from focus point to camera.
 * @param pitch_degrees Vertical angle in degrees.
 * @param yaw_degrees Horizontal angle in degrees.
 * @return 4x4 view matrix for rendering.
 */
glm::mat4 calculateViewMatrix(
    const glm::vec3& focus_point,
    float distance,
    float pitch_degrees,
    float yaw_degrees);

/**
 * @brief Calculate view matrix from CameraState.
 *
 * Main function for view matrix calculation. Extracts all parameters from
 * CameraState and computes the view matrix.
 *
 * Handles transitioning state by using current interpolated values
 * (the caller is responsible for interpolating CameraState values
 * during animation).
 *
 * @param state The camera state.
 * @return 4x4 view matrix for rendering.
 */
glm::mat4 calculateViewMatrix(const CameraState& state);

/**
 * @brief Get the forward direction vector for the camera.
 *
 * Returns the normalized direction the camera is looking (from camera to target).
 * This is useful for raycasting, picking, and orientation queries.
 *
 * @param pitch_degrees Vertical angle in degrees.
 * @param yaw_degrees Horizontal angle in degrees.
 * @return Normalized forward direction vector.
 */
glm::vec3 calculateCameraForward(float pitch_degrees, float yaw_degrees);

/**
 * @brief Get the right direction vector for the camera.
 *
 * Returns the normalized right direction (perpendicular to forward and up).
 * Useful for camera-relative movement.
 *
 * @param yaw_degrees Horizontal angle in degrees.
 * @return Normalized right direction vector.
 */
glm::vec3 calculateCameraRight(float yaw_degrees);

} // namespace sims3000

#endif // SIMS3000_RENDER_VIEWMATRIX_H
