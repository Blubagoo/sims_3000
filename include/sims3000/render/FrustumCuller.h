/**
 * @file FrustumCuller.h
 * @brief Frustum culling system with spatial partitioning for large maps.
 *
 * Extracts frustum planes from view-projection matrix and culls entities
 * with bounding boxes outside the frustum. Mandatory spatial partitioning
 * (2D grid hash) ensures performance on 512x512 maps with 262k+ entities.
 *
 * Features:
 * - Frustum plane extraction from VP matrix (Gribb/Hartmann method)
 * - Conservative AABB-frustum intersection test (prevents popping)
 * - 2D spatial hash grid (configurable 16x16 or 32x32 cells)
 * - Efficient grid cell-frustum intersection for broad-phase culling
 * - Works correctly at all camera angles (preset and free)
 *
 * Usage:
 * @code
 *   FrustumCuller culler(512, 512);  // Map dimensions
 *
 *   // Each frame:
 *   culler.updateFrustum(viewProjectionMatrix);
 *
 *   // Register entities in spatial grid (once, or on position change)
 *   culler.registerEntity(entityId, worldAABB, gridPosition);
 *
 *   // Get visible entities for rendering
 *   auto visibleEntities = culler.getVisibleEntities();
 *
 *   // Or test individual AABBs
 *   if (culler.isVisible(worldAABB)) {
 *       // Submit for rendering
 *   }
 * @endcode
 *
 * Resource ownership:
 * - FrustumCuller owns spatial hash grid data
 * - No GPU resources (pure CPU culling)
 */

#ifndef SIMS3000_RENDER_FRUSTUM_CULLER_H
#define SIMS3000_RENDER_FRUSTUM_CULLER_H

#include "sims3000/core/types.h"
#include "sims3000/render/GPUMesh.h"
#include "sims3000/render/ViewportBounds.h"

#include <glm/glm.hpp>
#include <vector>
#include <unordered_map>
#include <cstdint>

namespace sims3000 {

// ============================================================================
// Frustum Culler Configuration
// ============================================================================

/**
 * @namespace FrustumCullerConfig
 * @brief Configuration constants for frustum culling.
 */
namespace FrustumCullerConfig {
    /// Default cell size for spatial partitioning (16x16 world units per cell)
    constexpr int DEFAULT_CELL_SIZE = 16;

    /// Alternative cell size for denser maps (32x32 world units per cell)
    constexpr int LARGE_CELL_SIZE = 32;

    /// Conservative expansion factor for bounding boxes (prevents popping)
    /// AABBs are expanded by this percentage to ensure objects on the edge
    /// of the frustum are not culled prematurely.
    constexpr float CONSERVATIVE_EXPANSION = 0.1f;

    /// Maximum number of entities per cell before warning
    constexpr std::size_t MAX_ENTITIES_PER_CELL_WARNING = 1000;

    /// Number of frustum planes
    constexpr int NUM_FRUSTUM_PLANES = 6;

    /// Plane indices for clarity
    constexpr int PLANE_LEFT = 0;
    constexpr int PLANE_RIGHT = 1;
    constexpr int PLANE_BOTTOM = 2;
    constexpr int PLANE_TOP = 3;
    constexpr int PLANE_NEAR = 4;
    constexpr int PLANE_FAR = 5;
}

// ============================================================================
// Frustum Plane
// ============================================================================

/**
 * @struct FrustumPlane
 * @brief A plane in the view frustum represented by normal and distance.
 *
 * Plane equation: normal.x * x + normal.y * y + normal.z * z + distance = 0
 * Points with positive signed distance are in front of the plane (inside frustum).
 */
struct FrustumPlane {
    glm::vec3 normal{0.0f, 0.0f, 1.0f};  ///< Plane normal (pointing inward)
    float distance = 0.0f;                ///< Distance from origin

    /**
     * @brief Compute signed distance from point to plane.
     * @param point Point in world space.
     * @return Positive if in front (inside), negative if behind (outside).
     */
    float signedDistance(const glm::vec3& point) const {
        return glm::dot(normal, point) + distance;
    }

    /**
     * @brief Construct from vec4 representation (normal.xyz, distance).
     * @param plane vec4 where xyz = normal, w = distance.
     */
    static FrustumPlane fromVec4(const glm::vec4& plane) {
        FrustumPlane p;
        float length = glm::length(glm::vec3(plane));
        if (length > 0.0001f) {
            p.normal = glm::vec3(plane) / length;
            p.distance = plane.w / length;
        }
        return p;
    }
};

// ============================================================================
// Spatial Hash Grid Cell
// ============================================================================

/**
 * @struct SpatialCell
 * @brief A cell in the spatial partitioning grid.
 *
 * Contains entity IDs registered in this cell for efficient broad-phase culling.
 */
struct SpatialCell {
    std::vector<EntityID> entities;  ///< Entities in this cell

    void clear() { entities.clear(); }
    void add(EntityID entity) { entities.push_back(entity); }
    void remove(EntityID entity);
    bool contains(EntityID entity) const;
    std::size_t count() const { return entities.size(); }
};

// ============================================================================
// Culling Result
// ============================================================================

/**
 * @enum CullResult
 * @brief Result of frustum culling test.
 */
enum class CullResult : std::uint8_t {
    Outside = 0,    ///< Completely outside frustum (cull)
    Intersects = 1, ///< Partially inside frustum (render)
    Inside = 2      ///< Completely inside frustum (render)
};

// ============================================================================
// Frustum Culler Statistics
// ============================================================================

/**
 * @struct FrustumCullerStats
 * @brief Statistics about culling performance.
 */
struct FrustumCullerStats {
    std::uint32_t totalEntities = 0;       ///< Total registered entities
    std::uint32_t testedEntities = 0;      ///< Entities tested this frame
    std::uint32_t culledEntities = 0;      ///< Entities culled (not visible)
    std::uint32_t visibleEntities = 0;     ///< Entities passed culling
    std::uint32_t cellsTested = 0;         ///< Grid cells tested
    std::uint32_t cellsCulled = 0;         ///< Grid cells entirely culled
    float cullRatio = 0.0f;                ///< Ratio of culled vs total (0-1)

    void reset() {
        totalEntities = 0;
        testedEntities = 0;
        culledEntities = 0;
        visibleEntities = 0;
        cellsTested = 0;
        cellsCulled = 0;
        cullRatio = 0.0f;
    }

    void computeRatio() {
        if (testedEntities > 0) {
            cullRatio = static_cast<float>(culledEntities) /
                        static_cast<float>(testedEntities);
        }
    }
};

// ============================================================================
// Frustum Culler Class
// ============================================================================

/**
 * @class FrustumCuller
 * @brief Frustum culling system with 2D spatial partitioning.
 *
 * Provides efficient frustum culling for large numbers of entities using
 * a two-phase approach:
 * 1. Broad phase: Cull entire grid cells against frustum
 * 2. Narrow phase: Test individual entity AABBs in visible cells
 *
 * Conservative culling ensures no visible objects are incorrectly culled.
 */
class FrustumCuller {
public:
    /**
     * @brief Construct frustum culler for a map.
     * @param mapWidth Map width in tiles/world units.
     * @param mapHeight Map height in tiles/world units.
     * @param cellSize Size of spatial hash cells (default 16).
     */
    FrustumCuller(int mapWidth, int mapHeight,
                  int cellSize = FrustumCullerConfig::DEFAULT_CELL_SIZE);

    ~FrustumCuller() = default;

    // Non-copyable (owns grid data)
    FrustumCuller(const FrustumCuller&) = delete;
    FrustumCuller& operator=(const FrustumCuller&) = delete;

    // Moveable
    FrustumCuller(FrustumCuller&&) = default;
    FrustumCuller& operator=(FrustumCuller&&) = default;

    // =========================================================================
    // Frustum Management
    // =========================================================================

    /**
     * @brief Update frustum planes from view-projection matrix.
     *
     * Must be called each frame before culling queries.
     *
     * @param viewProjection Combined view * projection matrix.
     */
    void updateFrustum(const glm::mat4& viewProjection);

    /**
     * @brief Get the current frustum planes.
     * @return Array of 6 frustum planes (left, right, bottom, top, near, far).
     */
    const FrustumPlane* getFrustumPlanes() const { return m_planes; }

    /**
     * @brief Check if frustum has been set this frame.
     * @return true if updateFrustum() has been called.
     */
    bool isFrustumValid() const { return m_frustumValid; }

    // =========================================================================
    // Entity Registration
    // =========================================================================

    /**
     * @brief Register an entity in the spatial grid.
     *
     * Call when entity is created or position changes significantly.
     *
     * @param entity Entity ID.
     * @param worldBounds World-space AABB of the entity.
     * @param worldPosition World position for grid cell lookup.
     */
    void registerEntity(EntityID entity, const AABB& worldBounds,
                        const glm::vec3& worldPosition);

    /**
     * @brief Unregister an entity from the spatial grid.
     *
     * Call when entity is destroyed.
     *
     * @param entity Entity ID to remove.
     */
    void unregisterEntity(EntityID entity);

    /**
     * @brief Update entity position in spatial grid.
     *
     * Efficiently updates entity's grid cell when position changes.
     *
     * @param entity Entity ID.
     * @param newWorldPosition New world position.
     */
    void updateEntityPosition(EntityID entity, const glm::vec3& newWorldPosition);

    /**
     * @brief Clear all registered entities.
     */
    void clearEntities();

    // =========================================================================
    // Visibility Testing
    // =========================================================================

    /**
     * @brief Test if an AABB is visible within the frustum.
     *
     * Uses conservative testing - if in doubt, returns true (visible).
     *
     * @param worldBounds World-space AABB to test.
     * @return CullResult indicating visibility status.
     */
    CullResult testAABB(const AABB& worldBounds) const;

    /**
     * @brief Test if an AABB is visible (simplified boolean).
     *
     * Conservative: returns true for Intersects and Inside.
     *
     * @param worldBounds World-space AABB to test.
     * @return true if potentially visible, false if definitely culled.
     */
    bool isVisible(const AABB& worldBounds) const {
        return testAABB(worldBounds) != CullResult::Outside;
    }

    /**
     * @brief Test if a point is visible within the frustum.
     * @param point World-space point.
     * @return true if inside frustum.
     */
    bool isPointVisible(const glm::vec3& point) const;

    /**
     * @brief Test if a bounding sphere is visible.
     * @param center Sphere center in world space.
     * @param radius Sphere radius.
     * @return CullResult indicating visibility status.
     */
    CullResult testSphere(const glm::vec3& center, float radius) const;

    /**
     * @brief Test if a grid cell is visible.
     * @param cellX Cell X coordinate.
     * @param cellY Cell Y coordinate (world Z).
     * @return CullResult for the cell's AABB.
     */
    CullResult testCell(int cellX, int cellY) const;

    // =========================================================================
    // Batch Queries
    // =========================================================================

    /**
     * @brief Get all visible entity IDs from registered entities.
     *
     * Uses two-phase culling:
     * 1. Cull grid cells against frustum
     * 2. Test entities in visible cells
     *
     * @param outVisible Output vector of visible entity IDs.
     */
    void getVisibleEntities(std::vector<EntityID>& outVisible);

    /**
     * @brief Get visible grid cell indices.
     *
     * Returns cells that intersect or are inside the frustum.
     * Use for custom entity iteration.
     *
     * @param outCells Output vector of (cellX, cellY) pairs.
     */
    void getVisibleCells(std::vector<std::pair<int, int>>& outCells);

    /**
     * @brief Get visible tile range as a GridRect.
     *
     * Useful for integrating with existing getVisibleTileRange() logic.
     *
     * @return GridRect of tiles that may be visible.
     */
    GridRect getVisibleTileRange() const;

    // =========================================================================
    // Spatial Grid Access
    // =========================================================================

    /**
     * @brief Get cell for a world position.
     * @param worldX World X coordinate.
     * @param worldZ World Z coordinate.
     * @return Cell coordinates (clamped to grid bounds).
     */
    std::pair<int, int> getCellForPosition(float worldX, float worldZ) const;

    /**
     * @brief Get entities in a specific cell.
     * @param cellX Cell X coordinate.
     * @param cellY Cell Y coordinate.
     * @return Pointer to cell, or nullptr if out of bounds.
     */
    const SpatialCell* getCell(int cellX, int cellY) const;

    /**
     * @brief Get grid dimensions.
     * @return (cellCountX, cellCountY) pair.
     */
    std::pair<int, int> getGridDimensions() const {
        return {m_gridWidth, m_gridHeight};
    }

    /**
     * @brief Get cell size in world units.
     * @return Cell size.
     */
    int getCellSize() const { return m_cellSize; }

    /**
     * @brief Get total number of registered entities.
     * @return Entity count.
     */
    std::size_t getEntityCount() const { return m_entityCells.size(); }

    // =========================================================================
    // Statistics
    // =========================================================================

    /**
     * @brief Get culling statistics for the last frame.
     * @return Statistics struct.
     */
    const FrustumCullerStats& getStats() const { return m_stats; }

    /**
     * @brief Reset statistics for a new frame.
     */
    void resetStats() { m_stats.reset(); }

private:
    // =========================================================================
    // Internal Methods
    // =========================================================================

    /**
     * @brief Get the index for a cell in the flat grid array.
     * @param cellX Cell X coordinate.
     * @param cellY Cell Y coordinate.
     * @return Linear index, or -1 if out of bounds.
     */
    int getCellIndex(int cellX, int cellY) const;

    /**
     * @brief Get the AABB for a grid cell.
     * @param cellX Cell X coordinate.
     * @param cellY Cell Y coordinate.
     * @return World-space AABB for the cell.
     */
    AABB getCellAABB(int cellX, int cellY) const;

    /**
     * @brief Apply conservative expansion to an AABB.
     * @param bounds Original AABB.
     * @return Expanded AABB.
     */
    AABB expandAABB(const AABB& bounds) const;

    // =========================================================================
    // Data Members
    // =========================================================================

    // Map configuration
    int m_mapWidth = 0;
    int m_mapHeight = 0;
    int m_cellSize = FrustumCullerConfig::DEFAULT_CELL_SIZE;
    int m_gridWidth = 0;   ///< Number of cells in X
    int m_gridHeight = 0;  ///< Number of cells in Z

    // Frustum planes (6 planes: left, right, bottom, top, near, far)
    FrustumPlane m_planes[FrustumCullerConfig::NUM_FRUSTUM_PLANES];
    bool m_frustumValid = false;

    // Spatial hash grid (flat array for cache efficiency)
    std::vector<SpatialCell> m_grid;

    // Entity-to-cell mapping for fast updates
    std::unordered_map<EntityID, std::pair<int, int>> m_entityCells;

    // Entity bounds storage (for narrow phase testing)
    std::unordered_map<EntityID, AABB> m_entityBounds;

    // Statistics
    mutable FrustumCullerStats m_stats;
};

// ============================================================================
// Utility Functions
// ============================================================================

/**
 * @brief Extract frustum planes from view-projection matrix.
 *
 * Uses the Gribb/Hartmann method to extract normalized frustum planes.
 *
 * @param viewProjection Combined view-projection matrix.
 * @param outPlanes Output array of 6 FrustumPlane structs.
 */
void extractFrustumPlanes(const glm::mat4& viewProjection,
                          FrustumPlane outPlanes[6]);

/**
 * @brief Test AABB against frustum planes.
 *
 * Conservative test: returns Outside only if the AABB is entirely
 * on the negative side of at least one plane.
 *
 * @param bounds World-space AABB.
 * @param planes Array of 6 frustum planes.
 * @return CullResult.
 */
CullResult testAABBAgainstFrustum(const AABB& bounds,
                                  const FrustumPlane planes[6]);

/**
 * @brief Transform a local-space AABB to world space.
 *
 * Applies the model matrix to transform the AABB.
 *
 * @param localBounds AABB in model-local space.
 * @param modelMatrix Model-to-world transform.
 * @return World-space AABB (axis-aligned, may be larger than tight fit).
 */
AABB transformAABBToWorld(const AABB& localBounds, const glm::mat4& modelMatrix);

} // namespace sims3000

#endif // SIMS3000_RENDER_FRUSTUM_CULLER_H
