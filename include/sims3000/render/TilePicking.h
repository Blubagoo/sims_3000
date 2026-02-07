/**
 * @file TilePicking.h
 * @brief Tile picking via 3D ray casting for mouse-based interaction.
 *
 * Provides functions for converting screen coordinates to grid tile coordinates
 * using ray casting through the cursor position. Used for all mouse-based
 * interaction including building placement, selection, and terrain modification.
 *
 * Key features:
 * - Works correctly at all camera angles including extreme tilt
 * - Supports terrain elevation (height-aware picking)
 * - Numerical stability guards for near-horizontal rays
 * - Designed for extension to building bounding box picking
 *
 * Resource ownership: None (pure functions, no GPU/SDL resources).
 */

#ifndef SIMS3000_RENDER_TILEPICKING_H
#define SIMS3000_RENDER_TILEPICKING_H

#include "sims3000/core/types.h"
#include "sims3000/render/ScreenToWorld.h"
#include "sims3000/render/CameraState.h"
#include <glm/glm.hpp>
#include <optional>
#include <functional>

namespace sims3000 {

// ============================================================================
// Tile Picking Result
// ============================================================================

/**
 * @struct TilePickResult
 * @brief Result of a tile picking operation.
 *
 * Contains the grid position and additional context about the pick.
 */
struct TilePickResult {
    GridPosition position;      ///< Grid tile coordinates
    glm::vec3 worldPosition;    ///< Exact world-space intersection point
    float elevation;            ///< Terrain elevation at the picked tile

    TilePickResult() = default;

    TilePickResult(GridPosition pos, glm::vec3 worldPos, float elev = 0.0f)
        : position(pos), worldPosition(worldPos), elevation(elev) {}
};

// ============================================================================
// Terrain Height Provider (for elevation-aware picking)
// ============================================================================

/**
 * @typedef TerrainHeightProvider
 * @brief Function type for retrieving terrain height at a grid position.
 *
 * Returns the elevation (Y coordinate in world space) for a given grid tile.
 * Used for elevation-aware tile picking.
 *
 * @param x Grid X coordinate
 * @param y Grid Y coordinate (note: this is grid Y, not world Y)
 * @return Terrain height (world Y) at the specified grid position
 */
using TerrainHeightProvider = std::function<float(int16_t x, int16_t y)>;

/**
 * @brief Default terrain height provider that returns flat terrain (height 0).
 */
inline float flatTerrainHeight(int16_t /*x*/, int16_t /*y*/) {
    return 0.0f;
}

// ============================================================================
// Tile Picking Configuration
// ============================================================================

/**
 * @brief Configuration constants for tile picking.
 */
namespace TilePickingConfig {
    /// Maximum iterations for iterative elevation refinement
    constexpr int MAX_ELEVATION_ITERATIONS = 3;

    /// Convergence threshold for elevation refinement (world units)
    constexpr float ELEVATION_CONVERGENCE_THRESHOLD = 0.1f;

    /// Grid unit size in world space (1 tile = 1 world unit)
    constexpr float GRID_UNIT_SIZE = 1.0f;

    /// Epsilon for near-horizontal ray rejection
    /// At 15 degrees pitch (PITCH_MIN), ray Y component is ~0.26
    /// We use 0.0001 to only reject truly parallel rays
    constexpr float PARALLEL_RAY_EPSILON = 0.0001f;
}

// ============================================================================
// Core Tile Picking Functions
// ============================================================================

/**
 * @brief Pick a tile from screen coordinates (main entry point).
 *
 * Casts a ray from the camera through the screen position and returns
 * the grid tile at the intersection with the ground/terrain.
 *
 * @param screenPos Screen coordinates (pixels, origin at top-left).
 * @param windowWidth Window width in pixels.
 * @param windowHeight Window height in pixels.
 * @param viewProjection The combined view-projection matrix.
 * @param cameraState Current camera state for position calculation.
 * @param terrainHeight Terrain height provider (default: flat at Y=0).
 * @return TilePickResult if a valid tile was picked, empty otherwise.
 *
 * @note Returns empty optional for:
 *       - Ray parallel to ground plane (near-horizontal camera)
 *       - Intersection behind camera
 *       - Out-of-bounds grid coordinates
 */
std::optional<TilePickResult> pickTile(
    glm::vec2 screenPos,
    float windowWidth,
    float windowHeight,
    const glm::mat4& viewProjection,
    const CameraState& cameraState,
    TerrainHeightProvider terrainHeight = flatTerrainHeight);

/**
 * @brief Pick a tile with explicit terrain elevation.
 *
 * Simplified version for flat terrain at a specific elevation.
 *
 * @param screenPos Screen coordinates (pixels).
 * @param windowWidth Window width in pixels.
 * @param windowHeight Window height in pixels.
 * @param viewProjection The combined view-projection matrix.
 * @param cameraState Current camera state.
 * @param groundHeight Ground plane Y coordinate (default 0).
 * @return TilePickResult if valid, empty otherwise.
 */
std::optional<TilePickResult> pickTileFlat(
    glm::vec2 screenPos,
    float windowWidth,
    float windowHeight,
    const glm::mat4& viewProjection,
    const CameraState& cameraState,
    float groundHeight = 0.0f);

// ============================================================================
// Grid Coordinate Conversion
// ============================================================================

/**
 * @brief Convert world-space position to grid coordinates.
 *
 * Converts a world-space XZ position to grid tile coordinates.
 * Uses floor() for consistent tile mapping.
 *
 * @param worldPos World-space position (X and Z are used, Y is ignored).
 * @return Grid coordinates.
 */
GridPosition worldToGrid(const glm::vec3& worldPos);

/**
 * @brief Convert world-space position to grid coordinates with bounds checking.
 *
 * @param worldPos World-space position.
 * @param mapWidth Map width in tiles.
 * @param mapHeight Map height in tiles.
 * @return Grid coordinates if within bounds, empty otherwise.
 */
std::optional<GridPosition> worldToGridBounded(
    const glm::vec3& worldPos,
    int16_t mapWidth,
    int16_t mapHeight);

/**
 * @brief Convert grid coordinates to world-space center position.
 *
 * Returns the center of the specified grid tile in world space.
 * The Y component is the terrain height at that position.
 *
 * @param gridPos Grid coordinates.
 * @param height World Y coordinate (terrain height).
 * @return World-space center position of the tile.
 */
glm::vec3 gridToWorldCenter(const GridPosition& gridPos, float height = 0.0f);

/**
 * @brief Convert grid coordinates to world-space corner position.
 *
 * Returns the minimum corner (top-left) of the tile in world space.
 *
 * @param gridPos Grid coordinates.
 * @param height World Y coordinate.
 * @return World-space corner position.
 */
glm::vec3 gridToWorldCorner(const GridPosition& gridPos, float height = 0.0f);

// ============================================================================
// Elevation-Aware Picking Helpers
// ============================================================================

/**
 * @brief Pick tile with iterative elevation refinement.
 *
 * For terrain with varying elevation, uses iterative refinement to find
 * the correct tile. Starts with ground level, checks terrain height at
 * that position, and refines the intersection.
 *
 * @param ray The world-space ray from camera.
 * @param terrainHeight Function to get terrain height at grid position.
 * @param maxIterations Maximum refinement iterations.
 * @return TilePickResult if valid intersection found, empty otherwise.
 */
std::optional<TilePickResult> pickTileWithElevation(
    const Ray& ray,
    TerrainHeightProvider terrainHeight,
    int maxIterations = TilePickingConfig::MAX_ELEVATION_ITERATIONS);

/**
 * @brief Check if a ray can produce a valid ground intersection.
 *
 * Returns false if the ray is nearly parallel to the ground plane
 * or pointing away from it. Used as early-out optimization.
 *
 * @param ray The ray to check.
 * @param groundHeight Ground plane height.
 * @return true if ray could intersect ground plane.
 */
bool canIntersectGround(const Ray& ray, float groundHeight = 0.0f);

// ============================================================================
// Validation Helpers
// ============================================================================

/**
 * @brief Check if a grid position is within map bounds.
 *
 * @param pos Grid position to check.
 * @param mapWidth Map width in tiles.
 * @param mapHeight Map height in tiles.
 * @return true if position is valid.
 */
bool isValidGridPosition(
    const GridPosition& pos,
    int16_t mapWidth,
    int16_t mapHeight);

/**
 * @brief Clamp a grid position to map bounds.
 *
 * @param pos Grid position to clamp.
 * @param mapWidth Map width in tiles.
 * @param mapHeight Map height in tiles.
 * @return Clamped grid position.
 */
GridPosition clampToMapBounds(
    const GridPosition& pos,
    int16_t mapWidth,
    int16_t mapHeight);

} // namespace sims3000

#endif // SIMS3000_RENDER_TILEPICKING_H
