/**
 * @file ViewportBounds.cpp
 * @brief Viewport bounds calculation and map boundary clamping implementation.
 */

#include "sims3000/render/ViewportBounds.h"
#include "sims3000/render/ViewMatrix.h"
#include "sims3000/render/ProjectionMatrix.h"
#include "sims3000/render/ScreenToWorld.h"
#include <glm/gtc/matrix_inverse.hpp>
#include <cmath>
#include <algorithm>
#include <limits>

namespace sims3000 {

// ============================================================================
// FrustumFootprint Implementation
// ============================================================================

glm::vec4 FrustumFootprint::getAABB() const {
    float minX = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float minZ = std::numeric_limits<float>::max();
    float maxZ = std::numeric_limits<float>::lowest();

    for (const auto& corner : corners) {
        if (!std::isnan(corner.x) && !std::isinf(corner.x)) {
            minX = std::min(minX, corner.x);
            maxX = std::max(maxX, corner.x);
        }
        if (!std::isnan(corner.z) && !std::isinf(corner.z)) {
            minZ = std::min(minZ, corner.z);
            maxZ = std::max(maxZ, corner.z);
        }
    }

    // If we got invalid bounds, return zeros
    if (minX > maxX || minZ > maxZ) {
        return glm::vec4(0.0f);
    }

    return glm::vec4(minX, minZ, maxX, maxZ);
}

bool FrustumFootprint::containsPoint(float x, float z) const {
    // Point-in-polygon test using ray casting
    // We treat the four corners as a quad and test if (x, z) is inside
    int crossings = 0;

    for (int i = 0; i < 4; ++i) {
        int j = (i + 1) % 4;
        float x1 = corners[i].x;
        float z1 = corners[i].z;
        float x2 = corners[j].x;
        float z2 = corners[j].z;

        // Check if the edge crosses a horizontal ray from (x, z) going right
        if (((z1 <= z && z < z2) || (z2 <= z && z < z1)) &&
            x < (x2 - x1) * (z - z1) / (z2 - z1) + x1) {
            ++crossings;
        }
    }

    return (crossings % 2) == 1;
}

bool FrustumFootprint::isValid() const {
    for (const auto& corner : corners) {
        if (std::isnan(corner.x) || std::isnan(corner.y) || std::isnan(corner.z) ||
            std::isinf(corner.x) || std::isinf(corner.y) || std::isinf(corner.z)) {
            return false;
        }
    }
    return true;
}

// ============================================================================
// Frustum Footprint Calculation
// ============================================================================

FrustumFootprint calculateFrustumFootprint(
    const CameraState& cameraState,
    float fovDegrees,
    float aspectRatio,
    float groundHeight)
{
    FrustumFootprint footprint;

    // Calculate matrices
    const glm::mat4 viewMatrix = calculateViewMatrix(cameraState);
    const glm::mat4 projMatrix = calculateProjectionMatrix(fovDegrees, aspectRatio);
    const glm::mat4 viewProjMatrix = projMatrix * viewMatrix;
    const glm::mat4 invViewProj = glm::inverse(viewProjMatrix);

    // Get camera position for ray origin
    const glm::vec3 cameraPos = calculateCameraPosition(cameraState);

    // Screen corners in NDC coordinates:
    // Near-left (-1, -1), Near-right (1, -1), Far-right (1, 1), Far-left (-1, 1)
    // Note: In screen space, Y=-1 is bottom (near for our tilted camera), Y=1 is top (far)
    const glm::vec2 screenCorners[4] = {
        {-1.0f, -1.0f},  // Near-left (bottom-left of screen)
        { 1.0f, -1.0f},  // Near-right (bottom-right of screen)
        { 1.0f,  1.0f},  // Far-right (top-right of screen)
        {-1.0f,  1.0f}   // Far-left (top-left of screen)
    };

    for (int i = 0; i < 4; ++i) {
        // Convert NDC to world-space point at near plane
        const glm::vec4 nearNDC(screenCorners[i].x, screenCorners[i].y, 0.0f, 1.0f);
        glm::vec4 nearWorld = invViewProj * nearNDC;
        nearWorld /= nearWorld.w;

        // Create ray from camera through this point
        Ray ray;
        ray.origin = cameraPos;
        ray.direction = glm::normalize(glm::vec3(nearWorld) - cameraPos);

        // Intersect with ground plane
        auto intersection = rayPlaneIntersection(ray, groundHeight);
        if (intersection.has_value()) {
            footprint.corners[i] = intersection.value();
        } else {
            // Ray is nearly parallel to ground - use a far point along the ray
            // This can happen with very shallow camera angles
            footprint.corners[i] = ray.getPoint(1000.0f);
        }
    }

    return footprint;
}

// ============================================================================
// Visible Tile Range Calculation
// ============================================================================

GridRect getVisibleTileRange(
    const CameraState& cameraState,
    float fovDegrees,
    float aspectRatio,
    const MapBoundary& mapBoundary,
    float groundHeight)
{
    // Calculate frustum footprint
    FrustumFootprint footprint = calculateFrustumFootprint(
        cameraState, fovDegrees, aspectRatio, groundHeight);

    // Get AABB of the footprint
    glm::vec4 aabb = footprint.getAABB();

    // Convert to grid coordinates with padding
    int minX = static_cast<int>(std::floor(aabb.x)) - ViewportConfig::CULLING_PADDING;
    int minZ = static_cast<int>(std::floor(aabb.y)) - ViewportConfig::CULLING_PADDING;
    int maxX = static_cast<int>(std::ceil(aabb.z)) + ViewportConfig::CULLING_PADDING;
    int maxZ = static_cast<int>(std::ceil(aabb.w)) + ViewportConfig::CULLING_PADDING;

    // Clamp to map bounds
    minX = std::max(0, minX);
    minZ = std::max(0, minZ);
    maxX = std::min(mapBoundary.width - 1, maxX);
    maxZ = std::min(mapBoundary.height - 1, maxZ);

    // Ensure valid range
    if (minX > maxX) {
        minX = maxX = mapBoundary.width / 2;
    }
    if (minZ > maxZ) {
        minZ = maxZ = mapBoundary.height / 2;
    }

    return GridRect(
        static_cast<std::int16_t>(minX),
        static_cast<std::int16_t>(minZ),
        static_cast<std::int16_t>(maxX),
        static_cast<std::int16_t>(maxZ)
    );
}

// ============================================================================
// Boundary Deceleration
// ============================================================================

float calculateBoundaryDeceleration(
    float position,
    float minBound,
    float maxBound,
    float softMargin)
{
    // Calculate distance from each boundary
    float distFromMin = position - minBound;
    float distFromMax = maxBound - position;

    // Find minimum distance to any boundary
    float minDist = std::min(distFromMin, distFromMax);

    // If outside boundaries, return 0 (full stop)
    if (minDist < 0.0f) {
        return ViewportConfig::MIN_DECELERATION_FACTOR;
    }

    // If outside soft margin zone, return 1 (full speed)
    if (minDist >= softMargin) {
        return 1.0f;
    }

    // Inside soft margin - apply smooth deceleration
    // Use smooth ease-out curve: 1 - (1 - t)^2 where t = dist/margin
    float t = minDist / softMargin;

    // Smoother cubic ease-out: t * (2 - t) transitions nicely
    float factor = t * (2.0f - t);

    // Clamp to valid range
    return std::max(ViewportConfig::MIN_DECELERATION_FACTOR, std::min(1.0f, factor));
}

float calculateBoundaryDeceleration(
    const glm::vec3& focusPoint,
    const MapBoundary& boundary)
{
    glm::vec2 minBound = boundary.getMinBound();
    glm::vec2 maxBound = boundary.getMaxBound();

    // Calculate deceleration for X and Z axes
    float decelX = calculateBoundaryDeceleration(
        focusPoint.x, minBound.x, maxBound.x, boundary.softMargin);
    float decelZ = calculateBoundaryDeceleration(
        focusPoint.z, minBound.y, maxBound.y, boundary.softMargin);

    // Return minimum (most restrictive) deceleration
    return std::min(decelX, decelZ);
}

// ============================================================================
// Focus Point Clamping
// ============================================================================

glm::vec3 clampFocusPointToBoundary(
    const glm::vec3& focusPoint,
    const MapBoundary& boundary)
{
    glm::vec2 minBound = boundary.getMinBound();
    glm::vec2 maxBound = boundary.getMaxBound();

    // Hard clamp to boundary (including overshoot allowance)
    float clampedX = std::max(minBound.x, std::min(maxBound.x, focusPoint.x));
    float clampedZ = std::max(minBound.y, std::min(maxBound.y, focusPoint.z));

    return glm::vec3(clampedX, focusPoint.y, clampedZ);
}

glm::vec3 applyBoundaryDeceleration(
    const glm::vec3& currentFocus,
    const glm::vec3& velocity,
    const MapBoundary& boundary)
{
    glm::vec2 minBound = boundary.getMinBound();
    glm::vec2 maxBound = boundary.getMaxBound();

    // Calculate per-axis deceleration
    float decelX = calculateBoundaryDeceleration(
        currentFocus.x, minBound.x, maxBound.x, boundary.softMargin);
    float decelZ = calculateBoundaryDeceleration(
        currentFocus.z, minBound.y, maxBound.y, boundary.softMargin);

    // Special handling: if moving towards the boundary, apply deceleration
    // If moving away from the boundary (back towards center), don't slow down
    glm::vec3 mapCenter = boundary.getCenter();

    // X-axis: check if moving towards or away from center
    float velocityX = velocity.x;
    if (decelX < 1.0f) {
        // We're in the soft boundary zone
        float distToCenter = mapCenter.x - currentFocus.x;
        bool movingTowardsCenter = (velocityX * distToCenter) > 0.0f;
        if (!movingTowardsCenter) {
            velocityX *= decelX;
        }
    }

    // Z-axis: check if moving towards or away from center
    float velocityZ = velocity.z;
    if (decelZ < 1.0f) {
        float distToCenter = mapCenter.z - currentFocus.z;
        bool movingTowardsCenter = (velocityZ * distToCenter) > 0.0f;
        if (!movingTowardsCenter) {
            velocityZ *= decelZ;
        }
    }

    return glm::vec3(velocityX, velocity.y, velocityZ);
}

bool isInSoftBoundaryZone(
    const glm::vec3& focusPoint,
    const MapBoundary& boundary)
{
    float decel = calculateBoundaryDeceleration(focusPoint, boundary);
    return decel < 1.0f;
}

bool isAtHardBoundary(
    const glm::vec3& focusPoint,
    const MapBoundary& boundary)
{
    glm::vec2 minBound = boundary.getMinBound();
    glm::vec2 maxBound = boundary.getMaxBound();

    constexpr float epsilon = 0.001f;

    return focusPoint.x <= minBound.x + epsilon ||
           focusPoint.x >= maxBound.x - epsilon ||
           focusPoint.z <= minBound.y + epsilon ||
           focusPoint.z >= maxBound.y - epsilon;
}

glm::vec3 getDirectionToMapCenter(
    const glm::vec3& focusPoint,
    const MapBoundary& boundary)
{
    glm::vec3 center = boundary.getCenter();
    glm::vec3 direction = center - focusPoint;

    // Only consider XZ plane
    direction.y = 0.0f;

    float length = glm::length(direction);
    if (length < 0.001f) {
        return glm::vec3(0.0f);  // Already at center
    }

    return direction / length;
}

// ============================================================================
// Utility Functions
// ============================================================================

GridRect expandGridRect(
    const GridRect& rect,
    int padding,
    const MapBoundary& mapBoundary)
{
    int minX = std::max(0, static_cast<int>(rect.min.x) - padding);
    int minY = std::max(0, static_cast<int>(rect.min.y) - padding);
    int maxX = std::min(mapBoundary.width - 1, static_cast<int>(rect.max.x) + padding);
    int maxY = std::min(mapBoundary.height - 1, static_cast<int>(rect.max.y) + padding);

    return GridRect(
        static_cast<std::int16_t>(minX),
        static_cast<std::int16_t>(minY),
        static_cast<std::int16_t>(maxX),
        static_cast<std::int16_t>(maxY)
    );
}

GridPosition worldToGrid(float worldX, float worldZ) {
    // World coordinates map directly to grid (1 tile = 1 world unit)
    // Floor to get the tile the point is in
    int x = static_cast<int>(std::floor(worldX));
    int z = static_cast<int>(std::floor(worldZ));

    // Clamp to int16 range
    x = std::max(static_cast<int>(std::numeric_limits<std::int16_t>::min()),
                 std::min(static_cast<int>(std::numeric_limits<std::int16_t>::max()), x));
    z = std::max(static_cast<int>(std::numeric_limits<std::int16_t>::min()),
                 std::min(static_cast<int>(std::numeric_limits<std::int16_t>::max()), z));

    return GridPosition{static_cast<std::int16_t>(x), static_cast<std::int16_t>(z)};
}

glm::vec3 gridToWorld(GridPosition grid, float height) {
    // Return center of the tile (add 0.5 to get center)
    return glm::vec3(
        static_cast<float>(grid.x) + 0.5f,
        height,
        static_cast<float>(grid.y) + 0.5f
    );
}

} // namespace sims3000
