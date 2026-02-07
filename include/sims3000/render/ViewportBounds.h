/**
 * @file ViewportBounds.h
 * @brief Viewport bounds calculation and map boundary clamping.
 *
 * Calculates visible world area from camera frustum projection onto the ground
 * plane. With perspective projection, the visible area is a frustum footprint
 * (trapezoid), not an axis-aligned rectangle.
 *
 * Provides:
 * - Frustum footprint calculation (4 corner points on ground plane)
 * - Visible tile range for culling queries (axis-aligned bounding GridRect)
 * - Focus point clamping to map boundaries with soft deceleration
 * - Map size configuration (128/256/512)
 *
 * Coordinate system:
 * - X-axis: East (right), tiles 0 to width-1
 * - Y-axis: Up (elevation, ground plane at Y=0)
 * - Z-axis: South (down), tiles 0 to height-1
 *
 * Resource ownership: None (pure functions and simple data structs).
 */

#ifndef SIMS3000_RENDER_VIEWPORTBOUNDS_H
#define SIMS3000_RENDER_VIEWPORTBOUNDS_H

#include "sims3000/core/types.h"
#include "sims3000/render/CameraState.h"
#include <glm/glm.hpp>
#include <array>
#include <cstdint>

namespace sims3000 {

// ============================================================================
// Viewport Configuration Constants
// ============================================================================

/**
 * @namespace ViewportConfig
 * @brief Configuration parameters for viewport bounds calculation.
 */
namespace ViewportConfig {
    /// Default map size tier (medium: 256x256)
    constexpr MapSizeTier DEFAULT_MAP_SIZE = MapSizeTier::Medium;

    /// Map size for small maps (128x128 tiles)
    constexpr int MAP_SIZE_SMALL = 128;

    /// Map size for medium maps (256x256 tiles)
    constexpr int MAP_SIZE_MEDIUM = 256;

    /// Map size for large maps (512x512 tiles)
    constexpr int MAP_SIZE_LARGE = 512;

    /// Soft boundary margin in tiles (deceleration zone)
    /// Focus point deceleration begins this many tiles from the edge.
    constexpr float SOFT_BOUNDARY_MARGIN = 16.0f;

    /// Minimum deceleration factor at the hard boundary edge (0 = full stop)
    /// This creates a gentle slowdown as the camera approaches the map edge.
    constexpr float MIN_DECELERATION_FACTOR = 0.0f;

    /// Maximum allowed overshoot past map boundary (tiles)
    /// Allows slight visual overshoot for a smoother feel.
    constexpr float MAX_BOUNDARY_OVERSHOOT = 2.0f;

    /// Padding around visible tiles for culling margin (tiles)
    /// Adds extra tiles to visible range to prevent popping.
    constexpr int CULLING_PADDING = 2;

    /**
     * @brief Get map dimension for a given size tier.
     * @param tier The map size tier.
     * @return Map dimension in tiles (width == height for square maps).
     */
    constexpr int getMapDimension(MapSizeTier tier) {
        switch (tier) {
            case MapSizeTier::Small:  return MAP_SIZE_SMALL;
            case MapSizeTier::Medium: return MAP_SIZE_MEDIUM;
            case MapSizeTier::Large:  return MAP_SIZE_LARGE;
            default:                  return MAP_SIZE_MEDIUM;
        }
    }
}

// ============================================================================
// Grid Rectangle Structure
// ============================================================================

/**
 * @struct GridRect
 * @brief Axis-aligned rectangle in grid coordinates for culling queries.
 *
 * Represents a range of tiles from (minX, minY) to (maxX, maxY) inclusive.
 * Used by rendering systems to determine which tiles need to be drawn.
 */
struct GridRect {
    GridPosition min{0, 0};  ///< Top-left corner (inclusive)
    GridPosition max{0, 0};  ///< Bottom-right corner (inclusive)

    /**
     * @brief Default constructor creates an empty rect at origin.
     */
    GridRect() = default;

    /**
     * @brief Construct from corner positions.
     * @param minPos Minimum corner (top-left).
     * @param maxPos Maximum corner (bottom-right).
     */
    GridRect(GridPosition minPos, GridPosition maxPos)
        : min(minPos), max(maxPos) {}

    /**
     * @brief Construct from explicit coordinates.
     * @param minX Minimum X coordinate.
     * @param minY Minimum Y coordinate (map Z in world space).
     * @param maxX Maximum X coordinate.
     * @param maxY Maximum Y coordinate (map Z in world space).
     */
    GridRect(std::int16_t minX, std::int16_t minY,
             std::int16_t maxX, std::int16_t maxY)
        : min{minX, minY}, max{maxX, maxY} {}

    /**
     * @brief Get width of the rectangle in tiles.
     * @return Number of tiles in X direction (inclusive).
     */
    int width() const {
        return static_cast<int>(max.x - min.x) + 1;
    }

    /**
     * @brief Get height of the rectangle in tiles.
     * @return Number of tiles in Y direction (inclusive).
     */
    int height() const {
        return static_cast<int>(max.y - min.y) + 1;
    }

    /**
     * @brief Get total tile count in the rectangle.
     * @return Width * height.
     */
    int tileCount() const {
        return width() * height();
    }

    /**
     * @brief Check if a grid position is inside the rectangle.
     * @param pos Position to test.
     * @return true if pos is within bounds (inclusive).
     */
    bool contains(GridPosition pos) const {
        return pos.x >= min.x && pos.x <= max.x &&
               pos.y >= min.y && pos.y <= max.y;
    }

    /**
     * @brief Check if this rectangle overlaps with another.
     * @param other The other rectangle.
     * @return true if rectangles have any overlap.
     */
    bool overlaps(const GridRect& other) const {
        return min.x <= other.max.x && max.x >= other.min.x &&
               min.y <= other.max.y && max.y >= other.min.y;
    }

    /**
     * @brief Check if the rectangle is valid (min <= max).
     * @return true if rectangle has positive area.
     */
    bool isValid() const {
        return min.x <= max.x && min.y <= max.y;
    }

    bool operator==(const GridRect& other) const {
        return min == other.min && max == other.max;
    }

    bool operator!=(const GridRect& other) const {
        return !(*this == other);
    }
};

static_assert(sizeof(GridRect) == 8, "GridRect must be 8 bytes");

// ============================================================================
// Frustum Footprint Structure
// ============================================================================

/**
 * @struct FrustumFootprint
 * @brief The four corners of the camera frustum projected onto the ground plane.
 *
 * With perspective projection, the visible area on the ground is a trapezoid
 * (wider at the far end than the near end). This structure holds the four
 * corner points in world coordinates.
 *
 * Corner order (looking from above):
 * - corners[0]: Near-left
 * - corners[1]: Near-right
 * - corners[2]: Far-right
 * - corners[3]: Far-left
 */
struct FrustumFootprint {
    std::array<glm::vec3, 4> corners;  ///< Four corner points on ground plane

    /**
     * @brief Calculate the axis-aligned bounding box of the footprint.
     * @return Min/max X and Z coordinates as vec4(minX, minZ, maxX, maxZ).
     */
    glm::vec4 getAABB() const;

    /**
     * @brief Check if a point is inside the trapezoid (approximate).
     *
     * Uses point-in-polygon test for the four corners.
     *
     * @param x World X coordinate.
     * @param z World Z coordinate.
     * @return true if point is approximately inside the frustum footprint.
     */
    bool containsPoint(float x, float z) const;

    /**
     * @brief Check if all corners are valid (not NaN or Inf).
     * @return true if all corners have valid coordinates.
     */
    bool isValid() const;
};

// ============================================================================
// Map Boundary Configuration
// ============================================================================

/**
 * @struct MapBoundary
 * @brief Configuration for map boundary clamping.
 *
 * Defines the map size and soft boundary parameters. The focus point
 * is clamped to stay within these bounds, with gentle deceleration
 * as it approaches the edges.
 */
struct MapBoundary {
    MapSizeTier sizeTier = ViewportConfig::DEFAULT_MAP_SIZE;  ///< Map size tier
    int width = ViewportConfig::MAP_SIZE_MEDIUM;              ///< Map width in tiles
    int height = ViewportConfig::MAP_SIZE_MEDIUM;             ///< Map height in tiles
    float softMargin = ViewportConfig::SOFT_BOUNDARY_MARGIN;  ///< Deceleration zone size
    float maxOvershoot = ViewportConfig::MAX_BOUNDARY_OVERSHOOT; ///< Allowed overshoot

    /**
     * @brief Default constructor creates medium-sized map boundary.
     */
    MapBoundary() = default;

    /**
     * @brief Construct from map size tier.
     * @param tier The map size tier.
     */
    explicit MapBoundary(MapSizeTier tier)
        : sizeTier(tier)
        , width(ViewportConfig::getMapDimension(tier))
        , height(ViewportConfig::getMapDimension(tier))
    {}

    /**
     * @brief Construct with explicit dimensions.
     * @param w Map width in tiles.
     * @param h Map height in tiles.
     */
    MapBoundary(int w, int h)
        : width(w), height(h) {}

    /**
     * @brief Get the center of the map in world coordinates.
     * @return World position at map center (X, 0, Z).
     */
    glm::vec3 getCenter() const {
        return glm::vec3(
            static_cast<float>(width) / 2.0f,
            0.0f,
            static_cast<float>(height) / 2.0f
        );
    }

    /**
     * @brief Get minimum valid focus point (accounting for soft margin).
     * @return Minimum X and Z as vec2.
     */
    glm::vec2 getMinBound() const {
        return glm::vec2(-maxOvershoot, -maxOvershoot);
    }

    /**
     * @brief Get maximum valid focus point (accounting for soft margin).
     * @return Maximum X and Z as vec2.
     */
    glm::vec2 getMaxBound() const {
        return glm::vec2(
            static_cast<float>(width) + maxOvershoot,
            static_cast<float>(height) + maxOvershoot
        );
    }
};

// ============================================================================
// Viewport Bounds Functions
// ============================================================================

/**
 * @brief Calculate the frustum footprint on the ground plane.
 *
 * Projects the four corners of the camera frustum onto the ground plane
 * (Y = groundHeight) to determine the visible world area.
 *
 * @param cameraState Current camera state.
 * @param fovDegrees Vertical field of view in degrees.
 * @param aspectRatio Screen width / height.
 * @param groundHeight Y coordinate of the ground plane (default 0).
 * @return FrustumFootprint with four corner points.
 */
FrustumFootprint calculateFrustumFootprint(
    const CameraState& cameraState,
    float fovDegrees,
    float aspectRatio,
    float groundHeight = 0.0f);

/**
 * @brief Calculate visible tile range for culling.
 *
 * Computes the axis-aligned bounding rectangle of all tiles that may
 * be visible within the camera frustum. Includes a padding margin
 * to prevent pop-in artifacts.
 *
 * The result is clamped to valid tile indices (0 to mapSize-1).
 *
 * @param cameraState Current camera state.
 * @param fovDegrees Vertical field of view in degrees.
 * @param aspectRatio Screen width / height.
 * @param mapBoundary Map boundary configuration for clamping.
 * @param groundHeight Y coordinate of the ground plane (default 0).
 * @return GridRect containing all potentially visible tiles.
 */
GridRect getVisibleTileRange(
    const CameraState& cameraState,
    float fovDegrees,
    float aspectRatio,
    const MapBoundary& mapBoundary,
    float groundHeight = 0.0f);

/**
 * @brief Calculate deceleration factor for soft boundary.
 *
 * Returns a multiplier (0.0 to 1.0) indicating how much to scale
 * camera movement as the focus point approaches the map edge.
 * - 1.0 = full speed (not near edge)
 * - 0.0 = full stop (at or past edge)
 *
 * The deceleration uses a smooth ease-out curve for natural feel.
 *
 * @param position Current focus point position (X or Z coordinate).
 * @param minBound Minimum boundary coordinate.
 * @param maxBound Maximum boundary coordinate.
 * @param softMargin Size of the deceleration zone in tiles.
 * @return Deceleration factor [0, 1].
 */
float calculateBoundaryDeceleration(
    float position,
    float minBound,
    float maxBound,
    float softMargin);

/**
 * @brief Calculate 2D deceleration factor for focus point movement.
 *
 * Calculates deceleration for both X and Z axes and returns the
 * minimum (most restrictive) factor. This ensures smooth deceleration
 * when approaching corners.
 *
 * @param focusPoint Current focus point position.
 * @param boundary Map boundary configuration.
 * @return Deceleration factor [0, 1].
 */
float calculateBoundaryDeceleration(
    const glm::vec3& focusPoint,
    const MapBoundary& boundary);

/**
 * @brief Clamp focus point to map boundaries with soft deceleration.
 *
 * Applies soft boundary clamping to the focus point. Instead of a hard
 * stop at the boundary, the movement is scaled by a deceleration factor
 * as the focus point approaches the edge.
 *
 * @param focusPoint Current focus point to clamp.
 * @param boundary Map boundary configuration.
 * @return Clamped focus point position.
 */
glm::vec3 clampFocusPointToBoundary(
    const glm::vec3& focusPoint,
    const MapBoundary& boundary);

/**
 * @brief Apply soft boundary clamping with velocity damping.
 *
 * Used when the camera is moving - scales the movement velocity
 * by the boundary deceleration factor for smooth slowdown.
 *
 * @param currentFocus Current focus point position.
 * @param velocity Desired movement velocity.
 * @param boundary Map boundary configuration.
 * @return Adjusted velocity with boundary deceleration applied.
 */
glm::vec3 applyBoundaryDeceleration(
    const glm::vec3& currentFocus,
    const glm::vec3& velocity,
    const MapBoundary& boundary);

/**
 * @brief Check if focus point is within the soft boundary zone.
 *
 * Returns true if the focus point is close enough to the map edge
 * that deceleration should be applied.
 *
 * @param focusPoint Focus point to test.
 * @param boundary Map boundary configuration.
 * @return true if in soft boundary zone.
 */
bool isInSoftBoundaryZone(
    const glm::vec3& focusPoint,
    const MapBoundary& boundary);

/**
 * @brief Check if focus point is at the hard boundary edge.
 *
 * Returns true if the focus point has reached the absolute limit
 * (including any allowed overshoot).
 *
 * @param focusPoint Focus point to test.
 * @param boundary Map boundary configuration.
 * @return true if at hard boundary.
 */
bool isAtHardBoundary(
    const glm::vec3& focusPoint,
    const MapBoundary& boundary);

/**
 * @brief Get the direction towards map center from a focus point.
 *
 * Used to gently push the camera back when it's past the boundary.
 *
 * @param focusPoint Current focus point.
 * @param boundary Map boundary configuration.
 * @return Normalized direction towards map center.
 */
glm::vec3 getDirectionToMapCenter(
    const glm::vec3& focusPoint,
    const MapBoundary& boundary);

// ============================================================================
// Utility Functions
// ============================================================================

/**
 * @brief Expand a GridRect by a padding amount.
 *
 * @param rect The rectangle to expand.
 * @param padding Tiles to add on each side.
 * @param mapBoundary Map boundary for clamping.
 * @return Expanded and clamped GridRect.
 */
GridRect expandGridRect(
    const GridRect& rect,
    int padding,
    const MapBoundary& mapBoundary);

/**
 * @brief Convert world position to grid position.
 *
 * Converts world X/Z coordinates to grid tile coordinates.
 * World coordinates map directly to grid indices (1 tile = 1 world unit).
 *
 * @param worldX World X coordinate.
 * @param worldZ World Z coordinate.
 * @return Grid position (clamped to int16 range).
 */
GridPosition worldToGrid(float worldX, float worldZ);

/**
 * @brief Convert grid position to world position.
 *
 * Returns the center of the tile in world coordinates.
 *
 * @param grid Grid position.
 * @param height Y coordinate (default 0 for ground).
 * @return World position at tile center.
 */
glm::vec3 gridToWorld(GridPosition grid, float height = 0.0f);

} // namespace sims3000

#endif // SIMS3000_RENDER_VIEWPORTBOUNDS_H
