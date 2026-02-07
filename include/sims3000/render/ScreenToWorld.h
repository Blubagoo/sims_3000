/**
 * @file ScreenToWorld.h
 * @brief Screen-to-world ray casting utilities for perspective projection.
 *
 * Provides functions for converting screen coordinates to world-space rays
 * and intersecting those rays with planes. Used for:
 * - Zoom-to-cursor calculations
 * - Tile picking
 * - World-space UI anchoring
 *
 * Key perspective projection considerations:
 * - Rays diverge from camera position through screen points (not parallel)
 * - Must handle non-linear depth buffer for depth linearization
 * - Near-horizontal rays need numerical stability guards
 *
 * Resource ownership: None (pure functions, no GPU/SDL resources).
 */

#ifndef SIMS3000_RENDER_SCREENTOWORLD_H
#define SIMS3000_RENDER_SCREENTOWORLD_H

#include <glm/glm.hpp>
#include <optional>

namespace sims3000 {

// Forward declarations
struct CameraState;

// ============================================================================
// Ray Structure
// ============================================================================

/**
 * @struct Ray
 * @brief A ray defined by origin and direction.
 */
struct Ray {
    glm::vec3 origin{0.0f};     ///< Ray origin point (camera position for perspective)
    glm::vec3 direction{0.0f, 0.0f, -1.0f}; ///< Normalized ray direction

    /**
     * @brief Get a point along the ray at distance t.
     * @param t Distance along the ray.
     * @return Point at origin + t * direction.
     */
    glm::vec3 getPoint(float t) const {
        return origin + direction * t;
    }
};

// ============================================================================
// Screen-to-World Functions
// ============================================================================

/**
 * @brief Unproject screen coordinates to a world-space ray.
 *
 * Converts 2D screen coordinates to a 3D ray in world space using
 * the inverse view-projection matrix. For perspective projection,
 * rays diverge from the camera position through the screen point.
 *
 * @param screenX Screen X coordinate (pixels, 0 = left).
 * @param screenY Screen Y coordinate (pixels, 0 = top).
 * @param windowWidth Window width in pixels.
 * @param windowHeight Window height in pixels.
 * @param inverseViewProjection Inverse of the view-projection matrix.
 * @param cameraPosition World-space camera position (ray origin).
 * @return Ray from camera through the screen point.
 */
Ray screenToWorldRay(
    float screenX,
    float screenY,
    float windowWidth,
    float windowHeight,
    const glm::mat4& inverseViewProjection,
    const glm::vec3& cameraPosition);

/**
 * @brief Unproject screen coordinates using camera state.
 *
 * Convenience overload that extracts camera position from state
 * and calculates inverse matrices internally.
 *
 * @param screenX Screen X coordinate (pixels).
 * @param screenY Screen Y coordinate (pixels).
 * @param windowWidth Window width in pixels.
 * @param windowHeight Window height in pixels.
 * @param viewProjection The view-projection matrix.
 * @param cameraState Current camera state for position calculation.
 * @return Ray from camera through the screen point.
 */
Ray screenToWorldRay(
    float screenX,
    float screenY,
    float windowWidth,
    float windowHeight,
    const glm::mat4& viewProjection,
    const CameraState& cameraState);

// ============================================================================
// Ray-Plane Intersection
// ============================================================================

/**
 * @brief Intersect a ray with a horizontal plane (Y = height).
 *
 * Calculates the intersection point of a ray with a horizontal plane.
 * Returns empty optional if ray is parallel to plane or intersection
 * is behind the ray origin.
 *
 * @param ray The ray to intersect.
 * @param planeHeight Y coordinate of the horizontal plane (default 0 = ground).
 * @return World-space intersection point, or empty if no valid intersection.
 */
std::optional<glm::vec3> rayPlaneIntersection(
    const Ray& ray,
    float planeHeight = 0.0f);

/**
 * @brief Intersect a ray with an arbitrary plane.
 *
 * @param ray The ray to intersect.
 * @param planeNormal Normal vector of the plane.
 * @param planePoint A point on the plane.
 * @return World-space intersection point, or empty if no valid intersection.
 */
std::optional<glm::vec3> rayPlaneIntersection(
    const Ray& ray,
    const glm::vec3& planeNormal,
    const glm::vec3& planePoint);

/**
 * @brief Intersect a ray with the ground plane (Y = height).
 *
 * Alias for rayPlaneIntersection with horizontal plane for clearer API.
 * Use this for tile picking and zoom-to-cursor calculations.
 *
 * Numerical stability notes:
 * - For near-horizontal rays (pitch close to 0 or 180 degrees), the
 *   intersection may be very far away or not exist.
 * - Returns empty optional if ray is parallel to ground (no intersection).
 *
 * @param ray The ray to intersect (from camera through screen point).
 * @param height Ground plane Y coordinate (default 0 for flat terrain).
 * @return World-space intersection point, or empty if no valid intersection.
 */
inline std::optional<glm::vec3> rayGroundIntersection(
    const Ray& ray,
    float height = 0.0f)
{
    return rayPlaneIntersection(ray, height);
}

// ============================================================================
// Cursor World Position
// ============================================================================

/**
 * @brief Get world position under cursor on the ground plane.
 *
 * Convenience function that combines ray casting and ground plane intersection.
 * Used for zoom-to-cursor calculations.
 *
 * @param screenX Cursor X coordinate (pixels).
 * @param screenY Cursor Y coordinate (pixels).
 * @param windowWidth Window width in pixels.
 * @param windowHeight Window height in pixels.
 * @param viewProjection The view-projection matrix.
 * @param cameraState Current camera state.
 * @param groundHeight Ground plane Y coordinate (default 0).
 * @return World-space position on ground plane, or empty if no intersection.
 */
std::optional<glm::vec3> getCursorWorldPosition(
    float screenX,
    float screenY,
    float windowWidth,
    float windowHeight,
    const glm::mat4& viewProjection,
    const CameraState& cameraState,
    float groundHeight = 0.0f);

// ============================================================================
// Utility Functions
// ============================================================================

/**
 * @brief Convert screen coordinates to normalized device coordinates.
 *
 * Transforms screen pixel coordinates to NDC range [-1, 1].
 * Note: Y is flipped (screen Y=0 is top, NDC Y=1 is top).
 *
 * @param screenX Screen X coordinate.
 * @param screenY Screen Y coordinate.
 * @param windowWidth Window width.
 * @param windowHeight Window height.
 * @return NDC coordinates (x, y) in range [-1, 1].
 */
glm::vec2 screenToNDC(
    float screenX,
    float screenY,
    float windowWidth,
    float windowHeight);

/**
 * @brief Check if a ray is approximately parallel to a plane.
 *
 * Used to detect degenerate cases where ray-plane intersection
 * would be numerically unstable.
 *
 * @param rayDirection Normalized ray direction.
 * @param planeNormal Normalized plane normal.
 * @param epsilon Tolerance for parallel detection.
 * @return true if ray is nearly parallel to plane.
 */
bool isRayParallelToPlane(
    const glm::vec3& rayDirection,
    const glm::vec3& planeNormal,
    float epsilon = 0.0001f);

// ============================================================================
// World-to-Screen Projection (Ticket 2-028)
// ============================================================================

/**
 * @struct ScreenProjectionResult
 * @brief Result of world-to-screen projection.
 *
 * Contains the screen coordinates and visibility information for a projected
 * world position. Use isOnScreen() to check if the projected point is visible.
 */
struct ScreenProjectionResult {
    glm::vec2 screenPos{0.0f};   ///< Screen position in pixels (0,0 = top-left)
    float depth{0.0f};           ///< Normalized depth (0 = near, 1 = far)
    bool behindCamera{false};    ///< True if the point is behind the camera
    bool outsideViewport{false}; ///< True if the point is outside the screen bounds

    /**
     * @brief Check if the projected point is visible on screen.
     * @return true if the point is in front of camera and within viewport.
     */
    bool isOnScreen() const {
        return !behindCamera && !outsideViewport;
    }

    /**
     * @brief Check if the projection is valid (in front of camera).
     * @return true if the point is in front of camera (may be off-screen).
     */
    bool isValid() const {
        return !behindCamera;
    }
};

/**
 * @brief Transform world position to screen coordinates.
 *
 * Applies the full transformation pipeline:
 * 1. World position -> View space (view matrix)
 * 2. View space -> Clip space (projection matrix)
 * 3. Clip space -> NDC (perspective divide by w)
 * 4. NDC -> Screen coordinates (viewport transform)
 *
 * This is the inverse of screen-to-world ray casting.
 *
 * @param worldPos World-space position to project.
 * @param viewProjection Combined view-projection matrix.
 * @param viewportX Viewport X offset in pixels (usually 0).
 * @param viewportY Viewport Y offset in pixels (usually 0).
 * @param viewportWidth Viewport width in pixels.
 * @param viewportHeight Viewport height in pixels.
 * @return ScreenProjectionResult with screen position and visibility flags.
 *
 * @note For off-screen positions, screenPos is still computed but outsideViewport
 *       is set to true. For positions behind the camera, behindCamera is true
 *       and screenPos may be invalid.
 */
ScreenProjectionResult worldToScreen(
    const glm::vec3& worldPos,
    const glm::mat4& viewProjection,
    float viewportX,
    float viewportY,
    float viewportWidth,
    float viewportHeight);

/**
 * @brief Transform world position to screen coordinates (convenience overload).
 *
 * Convenience function assuming viewport starts at (0, 0).
 *
 * @param worldPos World-space position to project.
 * @param viewProjection Combined view-projection matrix.
 * @param windowWidth Window width in pixels.
 * @param windowHeight Window height in pixels.
 * @return ScreenProjectionResult with screen position and visibility flags.
 */
ScreenProjectionResult worldToScreen(
    const glm::vec3& worldPos,
    const glm::mat4& viewProjection,
    float windowWidth,
    float windowHeight);

/**
 * @brief Transform world position to screen coordinates using camera state.
 *
 * Convenience function that builds the view-projection matrix from camera state.
 *
 * @param worldPos World-space position to project.
 * @param cameraState Current camera state.
 * @param windowWidth Window width in pixels.
 * @param windowHeight Window height in pixels.
 * @return ScreenProjectionResult with screen position and visibility flags.
 */
ScreenProjectionResult worldToScreen(
    const glm::vec3& worldPos,
    const CameraState& cameraState,
    float windowWidth,
    float windowHeight);

/**
 * @brief Check if a world position is visible on screen.
 *
 * Utility function that performs projection and returns visibility.
 *
 * @param worldPos World-space position to check.
 * @param viewProjection Combined view-projection matrix.
 * @param windowWidth Window width in pixels.
 * @param windowHeight Window height in pixels.
 * @return true if the position is in front of camera and within viewport.
 */
bool isWorldPositionVisible(
    const glm::vec3& worldPos,
    const glm::mat4& viewProjection,
    float windowWidth,
    float windowHeight);

/**
 * @brief Get screen position for UI element anchored to world position.
 *
 * Convenience function for positioning UI elements at world locations.
 * Returns std::nullopt if the world position is not visible.
 *
 * @param worldPos World-space position to anchor to.
 * @param viewProjection Combined view-projection matrix.
 * @param windowWidth Window width in pixels.
 * @param windowHeight Window height in pixels.
 * @return Screen position if visible, empty optional otherwise.
 */
std::optional<glm::vec2> getScreenPositionForUI(
    const glm::vec3& worldPos,
    const glm::mat4& viewProjection,
    float windowWidth,
    float windowHeight);

} // namespace sims3000

#endif // SIMS3000_RENDER_SCREENTOWORLD_H
