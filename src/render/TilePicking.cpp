/**
 * @file TilePicking.cpp
 * @brief Tile picking implementation via 3D ray casting.
 */

#include "sims3000/render/TilePicking.h"
#include "sims3000/render/ViewMatrix.h"
#include <glm/gtc/matrix_inverse.hpp>
#include <cmath>
#include <algorithm>

namespace sims3000 {

// ============================================================================
// Grid Coordinate Conversion
// ============================================================================

GridPosition worldToGrid(const glm::vec3& worldPos) {
    // Grid uses X and Z (ground plane coordinates)
    // Y is up (elevation), not a grid axis
    // Use floor for consistent tile mapping across positive and negative coordinates
    return GridPosition{
        static_cast<int16_t>(std::floor(worldPos.x / TilePickingConfig::GRID_UNIT_SIZE)),
        static_cast<int16_t>(std::floor(worldPos.z / TilePickingConfig::GRID_UNIT_SIZE))
    };
}

std::optional<GridPosition> worldToGridBounded(
    const glm::vec3& worldPos,
    int16_t mapWidth,
    int16_t mapHeight)
{
    GridPosition pos = worldToGrid(worldPos);

    if (!isValidGridPosition(pos, mapWidth, mapHeight)) {
        return std::nullopt;
    }

    return pos;
}

glm::vec3 gridToWorldCenter(const GridPosition& gridPos, float height) {
    // Center of tile is at (x + 0.5, height, y + 0.5) in world space
    return glm::vec3(
        (static_cast<float>(gridPos.x) + 0.5f) * TilePickingConfig::GRID_UNIT_SIZE,
        height,
        (static_cast<float>(gridPos.y) + 0.5f) * TilePickingConfig::GRID_UNIT_SIZE
    );
}

glm::vec3 gridToWorldCorner(const GridPosition& gridPos, float height) {
    // Corner of tile is at (x, height, y)
    return glm::vec3(
        static_cast<float>(gridPos.x) * TilePickingConfig::GRID_UNIT_SIZE,
        height,
        static_cast<float>(gridPos.y) * TilePickingConfig::GRID_UNIT_SIZE
    );
}

// ============================================================================
// Validation Helpers
// ============================================================================

bool isValidGridPosition(
    const GridPosition& pos,
    int16_t mapWidth,
    int16_t mapHeight)
{
    return pos.x >= 0 && pos.x < mapWidth &&
           pos.y >= 0 && pos.y < mapHeight;
}

GridPosition clampToMapBounds(
    const GridPosition& pos,
    int16_t mapWidth,
    int16_t mapHeight)
{
    return GridPosition{
        static_cast<int16_t>(std::max(static_cast<int16_t>(0),
            std::min(pos.x, static_cast<int16_t>(mapWidth - 1)))),
        static_cast<int16_t>(std::max(static_cast<int16_t>(0),
            std::min(pos.y, static_cast<int16_t>(mapHeight - 1))))
    };
}

// ============================================================================
// Ray Intersection Helpers
// ============================================================================

bool canIntersectGround(const Ray& ray, float groundHeight) {
    // Check if ray is parallel to ground plane (Y direction too small)
    if (std::fabs(ray.direction.y) < TilePickingConfig::PARALLEL_RAY_EPSILON) {
        return false;
    }

    // Check if ray is pointing toward ground
    // If camera is above ground, ray.direction.y should be negative
    // If camera is below ground (unusual), ray.direction.y should be positive
    if (ray.origin.y > groundHeight && ray.direction.y >= 0.0f) {
        return false;  // Ray pointing up from above ground
    }
    if (ray.origin.y < groundHeight && ray.direction.y <= 0.0f) {
        return false;  // Ray pointing down from below ground
    }

    return true;
}

// ============================================================================
// Elevation-Aware Picking
// ============================================================================

std::optional<TilePickResult> pickTileWithElevation(
    const Ray& ray,
    TerrainHeightProvider terrainHeight,
    int maxIterations)
{
    // Start with ground level (height 0)
    float currentHeight = 0.0f;

    for (int i = 0; i < maxIterations; ++i) {
        // Intersect ray with plane at current height
        auto intersection = rayGroundIntersection(ray, currentHeight);
        if (!intersection.has_value()) {
            return std::nullopt;
        }

        // Convert to grid position
        GridPosition gridPos = worldToGrid(*intersection);

        // Get terrain height at this grid position
        float terrainHeightAtPos = terrainHeight(gridPos.x, gridPos.y);

        // Check if we've converged
        if (std::fabs(terrainHeightAtPos - currentHeight) <
            TilePickingConfig::ELEVATION_CONVERGENCE_THRESHOLD) {
            // We've found the correct tile
            return TilePickResult{
                gridPos,
                *intersection,
                terrainHeightAtPos
            };
        }

        // Update height for next iteration
        currentHeight = terrainHeightAtPos;
    }

    // After max iterations, use the last result
    auto finalIntersection = rayGroundIntersection(ray, currentHeight);
    if (!finalIntersection.has_value()) {
        return std::nullopt;
    }

    GridPosition finalPos = worldToGrid(*finalIntersection);
    return TilePickResult{
        finalPos,
        *finalIntersection,
        terrainHeight(finalPos.x, finalPos.y)
    };
}

// ============================================================================
// Core Tile Picking Functions
// ============================================================================

std::optional<TilePickResult> pickTile(
    glm::vec2 screenPos,
    float windowWidth,
    float windowHeight,
    const glm::mat4& viewProjection,
    const CameraState& cameraState,
    TerrainHeightProvider terrainHeight)
{
    // Cast ray from camera through screen position
    Ray ray = screenToWorldRay(
        screenPos.x, screenPos.y,
        windowWidth, windowHeight,
        viewProjection,
        cameraState
    );

    // Early out for invalid rays
    if (!canIntersectGround(ray, 0.0f)) {
        // Try checking at various heights if flat check fails
        // This handles cases where terrain is elevated
        for (float testHeight : {5.0f, 10.0f, 20.0f}) {
            if (canIntersectGround(ray, testHeight)) {
                return pickTileWithElevation(ray, terrainHeight);
            }
        }
        return std::nullopt;
    }

    // Use elevation-aware picking
    return pickTileWithElevation(ray, terrainHeight);
}

std::optional<TilePickResult> pickTileFlat(
    glm::vec2 screenPos,
    float windowWidth,
    float windowHeight,
    const glm::mat4& viewProjection,
    const CameraState& cameraState,
    float groundHeight)
{
    // Cast ray from camera through screen position
    Ray ray = screenToWorldRay(
        screenPos.x, screenPos.y,
        windowWidth, windowHeight,
        viewProjection,
        cameraState
    );

    // Check if ray can intersect ground
    if (!canIntersectGround(ray, groundHeight)) {
        return std::nullopt;
    }

    // Get intersection with ground plane
    auto intersection = rayGroundIntersection(ray, groundHeight);
    if (!intersection.has_value()) {
        return std::nullopt;
    }

    // Convert to grid position
    GridPosition gridPos = worldToGrid(*intersection);

    return TilePickResult{
        gridPos,
        *intersection,
        groundHeight
    };
}

} // namespace sims3000
