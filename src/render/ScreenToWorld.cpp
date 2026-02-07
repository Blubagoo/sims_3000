/**
 * @file ScreenToWorld.cpp
 * @brief Screen-to-world ray casting implementation.
 */

#include "sims3000/render/ScreenToWorld.h"
#include "sims3000/render/CameraState.h"
#include "sims3000/render/ViewMatrix.h"
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

namespace sims3000 {

glm::vec2 screenToNDC(
    float screenX,
    float screenY,
    float windowWidth,
    float windowHeight)
{
    // Handle zero dimensions
    if (windowWidth <= 0.0f) windowWidth = 1.0f;
    if (windowHeight <= 0.0f) windowHeight = 1.0f;

    // Convert to [0, 1] range
    float normalizedX = screenX / windowWidth;
    float normalizedY = screenY / windowHeight;

    // Convert to [-1, 1] range (NDC)
    // Note: Y is flipped because screen Y=0 is top, but NDC Y=1 is top
    float ndcX = normalizedX * 2.0f - 1.0f;
    float ndcY = 1.0f - normalizedY * 2.0f;  // Flip Y

    return glm::vec2(ndcX, ndcY);
}

bool isRayParallelToPlane(
    const glm::vec3& rayDirection,
    const glm::vec3& planeNormal,
    float epsilon)
{
    // Ray is parallel to plane if dot(direction, normal) is near zero
    float dot = std::fabs(glm::dot(rayDirection, planeNormal));
    return dot < epsilon;
}

Ray screenToWorldRay(
    float screenX,
    float screenY,
    float windowWidth,
    float windowHeight,
    const glm::mat4& inverseViewProjection,
    const glm::vec3& cameraPosition)
{
    // Convert screen coords to NDC
    glm::vec2 ndc = screenToNDC(screenX, screenY, windowWidth, windowHeight);

    // Create clip-space points at near and far planes
    // In clip space, Z goes from 0 (near) to 1 (far) for Vulkan/SDL_GPU
    glm::vec4 nearClip(ndc.x, ndc.y, 0.0f, 1.0f);
    glm::vec4 farClip(ndc.x, ndc.y, 1.0f, 1.0f);

    // Transform from clip space to world space
    glm::vec4 nearWorld = inverseViewProjection * nearClip;
    glm::vec4 farWorld = inverseViewProjection * farClip;

    // Perform perspective divide
    if (std::fabs(nearWorld.w) > 0.0001f) {
        nearWorld /= nearWorld.w;
    }
    if (std::fabs(farWorld.w) > 0.0001f) {
        farWorld /= farWorld.w;
    }

    // Ray from near to far plane
    glm::vec3 nearPoint(nearWorld);
    glm::vec3 farPoint(farWorld);

    // Calculate ray direction (from near to far, normalized)
    glm::vec3 direction = glm::normalize(farPoint - nearPoint);

    // For perspective projection, the ray originates from the camera position
    // and passes through the unprojected point
    Ray ray;
    ray.origin = cameraPosition;
    ray.direction = direction;

    return ray;
}

Ray screenToWorldRay(
    float screenX,
    float screenY,
    float windowWidth,
    float windowHeight,
    const glm::mat4& viewProjection,
    const CameraState& cameraState)
{
    // Calculate inverse view-projection matrix
    glm::mat4 inverseVP = glm::inverse(viewProjection);

    // Get camera position from camera state
    glm::vec3 cameraPos = calculateCameraPosition(cameraState);

    return screenToWorldRay(
        screenX, screenY,
        windowWidth, windowHeight,
        inverseVP,
        cameraPos);
}

std::optional<glm::vec3> rayPlaneIntersection(
    const Ray& ray,
    float planeHeight)
{
    // Horizontal plane: normal is (0, 1, 0), point is (0, planeHeight, 0)
    return rayPlaneIntersection(
        ray,
        glm::vec3(0.0f, 1.0f, 0.0f),
        glm::vec3(0.0f, planeHeight, 0.0f));
}

std::optional<glm::vec3> rayPlaneIntersection(
    const Ray& ray,
    const glm::vec3& planeNormal,
    const glm::vec3& planePoint)
{
    // Check if ray is parallel to plane
    if (isRayParallelToPlane(ray.direction, planeNormal)) {
        return std::nullopt;
    }

    // Ray-plane intersection formula:
    // t = dot(planePoint - ray.origin, planeNormal) / dot(ray.direction, planeNormal)
    float denom = glm::dot(ray.direction, planeNormal);

    // Additional check for numerical stability (very small denominator)
    if (std::fabs(denom) < 0.00001f) {
        return std::nullopt;
    }

    glm::vec3 originToPlane = planePoint - ray.origin;
    float t = glm::dot(originToPlane, planeNormal) / denom;

    // Check if intersection is behind the ray origin
    if (t < 0.0f) {
        return std::nullopt;
    }

    // Return intersection point
    return ray.getPoint(t);
}

std::optional<glm::vec3> getCursorWorldPosition(
    float screenX,
    float screenY,
    float windowWidth,
    float windowHeight,
    const glm::mat4& viewProjection,
    const CameraState& cameraState,
    float groundHeight)
{
    // Get ray from camera through cursor
    Ray ray = screenToWorldRay(
        screenX, screenY,
        windowWidth, windowHeight,
        viewProjection,
        cameraState);

    // Intersect with ground plane
    return rayPlaneIntersection(ray, groundHeight);
}

// ============================================================================
// World-to-Screen Projection (Ticket 2-028)
// ============================================================================

ScreenProjectionResult worldToScreen(
    const glm::vec3& worldPos,
    const glm::mat4& viewProjection,
    float viewportX,
    float viewportY,
    float viewportWidth,
    float viewportHeight)
{
    ScreenProjectionResult result;

    // Handle zero dimensions
    if (viewportWidth <= 0.0f) viewportWidth = 1.0f;
    if (viewportHeight <= 0.0f) viewportHeight = 1.0f;

    // Step 1: Transform world position to clip space
    // clipPos = viewProjection * worldPos (homogeneous coordinates)
    glm::vec4 clipPos = viewProjection * glm::vec4(worldPos, 1.0f);

    // Step 2: Check if point is behind camera (w <= 0 or very small)
    // In clip space, w is the homogeneous coordinate. If w <= 0, the point
    // is behind the camera or at the camera position.
    constexpr float W_EPSILON = 0.0001f;
    if (clipPos.w <= W_EPSILON) {
        result.behindCamera = true;
        // Still compute a position for consistency, but mark as behind camera
        // Use a small positive w to avoid division by zero
        clipPos.w = W_EPSILON;
    }

    // Step 3: Perspective divide to get NDC (Normalized Device Coordinates)
    // NDC range: x,y in [-1, 1], z in [0, 1] for Vulkan/SDL_GPU
    glm::vec3 ndc;
    ndc.x = clipPos.x / clipPos.w;
    ndc.y = clipPos.y / clipPos.w;
    ndc.z = clipPos.z / clipPos.w;

    // Store normalized depth
    result.depth = ndc.z;

    // Step 4: Viewport transform from NDC to screen coordinates
    // NDC x: -1 (left) to +1 (right) -> Screen x: viewportX to viewportX + viewportWidth
    // NDC y: +1 (top) to -1 (bottom) -> Screen y: viewportY to viewportY + viewportHeight
    // Note: Y is flipped because screen Y=0 is top, but NDC Y=+1 is top
    float screenX = (ndc.x + 1.0f) * 0.5f * viewportWidth + viewportX;
    float screenY = (1.0f - ndc.y) * 0.5f * viewportHeight + viewportY;

    result.screenPos = glm::vec2(screenX, screenY);

    // Step 5: Check if position is outside viewport bounds
    // A position is outside if:
    // - x < viewportX or x > viewportX + viewportWidth
    // - y < viewportY or y > viewportY + viewportHeight
    // - z < 0 (behind near plane) or z > 1 (beyond far plane)
    if (screenX < viewportX || screenX > viewportX + viewportWidth ||
        screenY < viewportY || screenY > viewportY + viewportHeight ||
        ndc.z < 0.0f || ndc.z > 1.0f) {
        result.outsideViewport = true;
    }

    return result;
}

ScreenProjectionResult worldToScreen(
    const glm::vec3& worldPos,
    const glm::mat4& viewProjection,
    float windowWidth,
    float windowHeight)
{
    // Viewport starts at (0, 0) and covers entire window
    return worldToScreen(worldPos, viewProjection, 0.0f, 0.0f, windowWidth, windowHeight);
}

ScreenProjectionResult worldToScreen(
    const glm::vec3& worldPos,
    const CameraState& cameraState,
    float windowWidth,
    float windowHeight)
{
    // Build view-projection matrix from camera state
    // Note: We need to include ViewMatrix.h and ProjectionMatrix.h for this
    glm::mat4 view = calculateViewMatrix(cameraState);

    // Calculate aspect ratio
    float aspectRatio = (windowHeight > 0.0f) ? (windowWidth / windowHeight) : 1.0f;

    // Use default FOV from CameraConfig
    glm::mat4 proj = glm::perspective(
        glm::radians(CameraConfig::FOV_DEFAULT),
        aspectRatio,
        CameraConfig::NEAR_PLANE,
        CameraConfig::FAR_PLANE
    );

    // Flip Y for Vulkan/SDL_GPU conventions
    proj[1][1] *= -1.0f;

    glm::mat4 viewProj = proj * view;

    return worldToScreen(worldPos, viewProj, windowWidth, windowHeight);
}

bool isWorldPositionVisible(
    const glm::vec3& worldPos,
    const glm::mat4& viewProjection,
    float windowWidth,
    float windowHeight)
{
    ScreenProjectionResult result = worldToScreen(worldPos, viewProjection, windowWidth, windowHeight);
    return result.isOnScreen();
}

std::optional<glm::vec2> getScreenPositionForUI(
    const glm::vec3& worldPos,
    const glm::mat4& viewProjection,
    float windowWidth,
    float windowHeight)
{
    ScreenProjectionResult result = worldToScreen(worldPos, viewProjection, windowWidth, windowHeight);

    if (result.isOnScreen()) {
        return result.screenPos;
    }

    return std::nullopt;
}

} // namespace sims3000
