/**
 * @file FrustumCuller.cpp
 * @brief Implementation of frustum culling with spatial partitioning.
 */

#include "sims3000/render/FrustumCuller.h"

#include <algorithm>
#include <cmath>

namespace sims3000 {

// ============================================================================
// SpatialCell Implementation
// ============================================================================

void SpatialCell::remove(EntityID entity) {
    auto it = std::find(entities.begin(), entities.end(), entity);
    if (it != entities.end()) {
        // Swap and pop for O(1) removal
        *it = entities.back();
        entities.pop_back();
    }
}

bool SpatialCell::contains(EntityID entity) const {
    return std::find(entities.begin(), entities.end(), entity) != entities.end();
}

// ============================================================================
// FrustumCuller Implementation
// ============================================================================

FrustumCuller::FrustumCuller(int mapWidth, int mapHeight, int cellSize)
    : m_mapWidth(mapWidth)
    , m_mapHeight(mapHeight)
    , m_cellSize(cellSize > 0 ? cellSize : FrustumCullerConfig::DEFAULT_CELL_SIZE)
{
    // Calculate grid dimensions (ceiling to ensure full coverage)
    m_gridWidth = (m_mapWidth + m_cellSize - 1) / m_cellSize;
    m_gridHeight = (m_mapHeight + m_cellSize - 1) / m_cellSize;

    // Allocate grid cells
    m_grid.resize(static_cast<std::size_t>(m_gridWidth) * m_gridHeight);
}

void FrustumCuller::updateFrustum(const glm::mat4& viewProjection) {
    extractFrustumPlanes(viewProjection, m_planes);
    m_frustumValid = true;
    resetStats();
}

void FrustumCuller::registerEntity(EntityID entity, const AABB& worldBounds,
                                   const glm::vec3& worldPosition) {
    // Get cell for this position
    auto [cellX, cellY] = getCellForPosition(worldPosition.x, worldPosition.z);

    // If already registered, check if cell changed
    auto it = m_entityCells.find(entity);
    if (it != m_entityCells.end()) {
        auto [oldCellX, oldCellY] = it->second;
        if (oldCellX == cellX && oldCellY == cellY) {
            // Same cell, just update bounds
            m_entityBounds[entity] = worldBounds;
            return;
        }
        // Different cell - remove from old cell
        int oldIndex = getCellIndex(oldCellX, oldCellY);
        if (oldIndex >= 0) {
            m_grid[oldIndex].remove(entity);
        }
    }

    // Add to new cell
    int newIndex = getCellIndex(cellX, cellY);
    if (newIndex >= 0) {
        m_grid[newIndex].add(entity);
    }

    // Update mappings
    m_entityCells[entity] = {cellX, cellY};
    m_entityBounds[entity] = worldBounds;
}

void FrustumCuller::unregisterEntity(EntityID entity) {
    auto it = m_entityCells.find(entity);
    if (it == m_entityCells.end()) {
        return;  // Not registered
    }

    auto [cellX, cellY] = it->second;
    int index = getCellIndex(cellX, cellY);
    if (index >= 0) {
        m_grid[index].remove(entity);
    }

    m_entityCells.erase(it);
    m_entityBounds.erase(entity);
}

void FrustumCuller::updateEntityPosition(EntityID entity,
                                         const glm::vec3& newWorldPosition) {
    auto it = m_entityCells.find(entity);
    if (it == m_entityCells.end()) {
        return;  // Not registered
    }

    auto [oldCellX, oldCellY] = it->second;
    auto [newCellX, newCellY] = getCellForPosition(newWorldPosition.x,
                                                    newWorldPosition.z);

    // Check if cell changed
    if (oldCellX == newCellX && oldCellY == newCellY) {
        return;  // Same cell, no update needed
    }

    // Move entity between cells
    int oldIndex = getCellIndex(oldCellX, oldCellY);
    int newIndex = getCellIndex(newCellX, newCellY);

    if (oldIndex >= 0) {
        m_grid[oldIndex].remove(entity);
    }
    if (newIndex >= 0) {
        m_grid[newIndex].add(entity);
    }

    it->second = {newCellX, newCellY};
}

void FrustumCuller::clearEntities() {
    for (auto& cell : m_grid) {
        cell.clear();
    }
    m_entityCells.clear();
    m_entityBounds.clear();
}

CullResult FrustumCuller::testAABB(const AABB& worldBounds) const {
    if (!m_frustumValid) {
        return CullResult::Inside;  // Conservative: don't cull if no frustum
    }

    // Apply conservative expansion
    AABB expanded = expandAABB(worldBounds);

    return testAABBAgainstFrustum(expanded, m_planes);
}

bool FrustumCuller::isPointVisible(const glm::vec3& point) const {
    if (!m_frustumValid) {
        return true;  // Conservative
    }

    for (int i = 0; i < FrustumCullerConfig::NUM_FRUSTUM_PLANES; ++i) {
        if (m_planes[i].signedDistance(point) < 0.0f) {
            return false;  // Behind at least one plane
        }
    }
    return true;
}

CullResult FrustumCuller::testSphere(const glm::vec3& center, float radius) const {
    if (!m_frustumValid) {
        return CullResult::Inside;  // Conservative
    }

    bool allInside = true;

    for (int i = 0; i < FrustumCullerConfig::NUM_FRUSTUM_PLANES; ++i) {
        float dist = m_planes[i].signedDistance(center);

        if (dist < -radius) {
            return CullResult::Outside;  // Entirely behind this plane
        }
        if (dist < radius) {
            allInside = false;  // Sphere intersects this plane
        }
    }

    return allInside ? CullResult::Inside : CullResult::Intersects;
}

CullResult FrustumCuller::testCell(int cellX, int cellY) const {
    if (!m_frustumValid) {
        return CullResult::Inside;  // Conservative
    }

    AABB cellBounds = getCellAABB(cellX, cellY);
    return testAABBAgainstFrustum(cellBounds, m_planes);
}

void FrustumCuller::getVisibleEntities(std::vector<EntityID>& outVisible) {
    outVisible.clear();

    if (!m_frustumValid) {
        // No frustum - return all entities (conservative)
        outVisible.reserve(m_entityCells.size());
        for (const auto& [entity, cell] : m_entityCells) {
            outVisible.push_back(entity);
        }
        m_stats.totalEntities = static_cast<std::uint32_t>(m_entityCells.size());
        m_stats.visibleEntities = m_stats.totalEntities;
        return;
    }

    m_stats.totalEntities = static_cast<std::uint32_t>(m_entityCells.size());

    // Phase 1: Test grid cells against frustum (broad phase)
    std::vector<int> visibleCellIndices;
    visibleCellIndices.reserve(m_grid.size());

    for (int cy = 0; cy < m_gridHeight; ++cy) {
        for (int cx = 0; cx < m_gridWidth; ++cx) {
            m_stats.cellsTested++;

            CullResult cellResult = testCell(cx, cy);
            if (cellResult == CullResult::Outside) {
                m_stats.cellsCulled++;
                continue;  // Skip this cell entirely
            }

            int index = getCellIndex(cx, cy);
            if (index >= 0 && !m_grid[index].entities.empty()) {
                visibleCellIndices.push_back(index);
            }
        }
    }

    // Phase 2: Test individual entities in visible cells (narrow phase)
    for (int cellIndex : visibleCellIndices) {
        const SpatialCell& cell = m_grid[cellIndex];

        for (EntityID entity : cell.entities) {
            m_stats.testedEntities++;

            auto boundsIt = m_entityBounds.find(entity);
            if (boundsIt == m_entityBounds.end()) {
                // No bounds - assume visible (conservative)
                outVisible.push_back(entity);
                m_stats.visibleEntities++;
                continue;
            }

            CullResult result = testAABB(boundsIt->second);
            if (result != CullResult::Outside) {
                outVisible.push_back(entity);
                m_stats.visibleEntities++;
            } else {
                m_stats.culledEntities++;
            }
        }
    }

    m_stats.computeRatio();
}

void FrustumCuller::getVisibleCells(std::vector<std::pair<int, int>>& outCells) {
    outCells.clear();

    if (!m_frustumValid) {
        // Return all cells (conservative)
        outCells.reserve(static_cast<std::size_t>(m_gridWidth) * m_gridHeight);
        for (int cy = 0; cy < m_gridHeight; ++cy) {
            for (int cx = 0; cx < m_gridWidth; ++cx) {
                outCells.emplace_back(cx, cy);
            }
        }
        return;
    }

    outCells.reserve(static_cast<std::size_t>(m_gridWidth) * m_gridHeight / 4);

    for (int cy = 0; cy < m_gridHeight; ++cy) {
        for (int cx = 0; cx < m_gridWidth; ++cx) {
            CullResult result = testCell(cx, cy);
            if (result != CullResult::Outside) {
                outCells.emplace_back(cx, cy);
            }
        }
    }
}

GridRect FrustumCuller::getVisibleTileRange() const {
    if (!m_frustumValid) {
        // Return entire map (conservative)
        return GridRect(
            0, 0,
            static_cast<std::int16_t>(m_mapWidth - 1),
            static_cast<std::int16_t>(m_mapHeight - 1)
        );
    }

    // Find min/max visible cells
    int minCellX = m_gridWidth;
    int maxCellX = -1;
    int minCellY = m_gridHeight;
    int maxCellY = -1;

    for (int cy = 0; cy < m_gridHeight; ++cy) {
        for (int cx = 0; cx < m_gridWidth; ++cx) {
            CullResult result = testCell(cx, cy);
            if (result != CullResult::Outside) {
                minCellX = std::min(minCellX, cx);
                maxCellX = std::max(maxCellX, cx);
                minCellY = std::min(minCellY, cy);
                maxCellY = std::max(maxCellY, cy);
            }
        }
    }

    // Convert cell range to tile range
    if (maxCellX < 0) {
        // No visible cells - return empty rect at center
        int centerX = m_mapWidth / 2;
        int centerZ = m_mapHeight / 2;
        return GridRect(
            static_cast<std::int16_t>(centerX),
            static_cast<std::int16_t>(centerZ),
            static_cast<std::int16_t>(centerX),
            static_cast<std::int16_t>(centerZ)
        );
    }

    int minTileX = minCellX * m_cellSize;
    int maxTileX = std::min(m_mapWidth - 1, (maxCellX + 1) * m_cellSize - 1);
    int minTileZ = minCellY * m_cellSize;
    int maxTileZ = std::min(m_mapHeight - 1, (maxCellY + 1) * m_cellSize - 1);

    return GridRect(
        static_cast<std::int16_t>(std::max(0, minTileX)),
        static_cast<std::int16_t>(std::max(0, minTileZ)),
        static_cast<std::int16_t>(maxTileX),
        static_cast<std::int16_t>(maxTileZ)
    );
}

std::pair<int, int> FrustumCuller::getCellForPosition(float worldX,
                                                       float worldZ) const {
    int cellX = static_cast<int>(std::floor(worldX / m_cellSize));
    int cellY = static_cast<int>(std::floor(worldZ / m_cellSize));

    // Clamp to grid bounds
    cellX = std::max(0, std::min(m_gridWidth - 1, cellX));
    cellY = std::max(0, std::min(m_gridHeight - 1, cellY));

    return {cellX, cellY};
}

const SpatialCell* FrustumCuller::getCell(int cellX, int cellY) const {
    int index = getCellIndex(cellX, cellY);
    if (index < 0) {
        return nullptr;
    }
    return &m_grid[index];
}

int FrustumCuller::getCellIndex(int cellX, int cellY) const {
    if (cellX < 0 || cellX >= m_gridWidth ||
        cellY < 0 || cellY >= m_gridHeight) {
        return -1;
    }
    return cellY * m_gridWidth + cellX;
}

AABB FrustumCuller::getCellAABB(int cellX, int cellY) const {
    AABB aabb;

    // Cell spans from cellX * cellSize to (cellX + 1) * cellSize
    aabb.min.x = static_cast<float>(cellX * m_cellSize);
    aabb.min.y = -1000.0f;  // Include all Y values (conservative)
    aabb.min.z = static_cast<float>(cellY * m_cellSize);

    aabb.max.x = static_cast<float>((cellX + 1) * m_cellSize);
    aabb.max.y = 1000.0f;   // Include all Y values (conservative)
    aabb.max.z = static_cast<float>((cellY + 1) * m_cellSize);

    return aabb;
}

AABB FrustumCuller::expandAABB(const AABB& bounds) const {
    AABB expanded = bounds;
    glm::vec3 size = bounds.size();
    glm::vec3 expansion = size * FrustumCullerConfig::CONSERVATIVE_EXPANSION;

    // Ensure minimum expansion for small objects
    expansion = glm::max(expansion, glm::vec3(0.5f));

    expanded.min -= expansion;
    expanded.max += expansion;

    return expanded;
}

// ============================================================================
// Utility Function Implementations
// ============================================================================

void extractFrustumPlanes(const glm::mat4& viewProjection,
                          FrustumPlane outPlanes[6]) {
    // Gribb/Hartmann method for frustum plane extraction
    // https://www.gamedevs.org/uploads/fast-extraction-viewing-frustum-planes-from-world-view-projection-matrix.pdf

    const glm::mat4& m = viewProjection;

    // Left plane: row3 + row0
    glm::vec4 left(
        m[0][3] + m[0][0],
        m[1][3] + m[1][0],
        m[2][3] + m[2][0],
        m[3][3] + m[3][0]
    );
    outPlanes[FrustumCullerConfig::PLANE_LEFT] = FrustumPlane::fromVec4(left);

    // Right plane: row3 - row0
    glm::vec4 right(
        m[0][3] - m[0][0],
        m[1][3] - m[1][0],
        m[2][3] - m[2][0],
        m[3][3] - m[3][0]
    );
    outPlanes[FrustumCullerConfig::PLANE_RIGHT] = FrustumPlane::fromVec4(right);

    // Bottom plane: row3 + row1
    glm::vec4 bottom(
        m[0][3] + m[0][1],
        m[1][3] + m[1][1],
        m[2][3] + m[2][1],
        m[3][3] + m[3][1]
    );
    outPlanes[FrustumCullerConfig::PLANE_BOTTOM] = FrustumPlane::fromVec4(bottom);

    // Top plane: row3 - row1
    glm::vec4 top(
        m[0][3] - m[0][1],
        m[1][3] - m[1][1],
        m[2][3] - m[2][1],
        m[3][3] - m[3][1]
    );
    outPlanes[FrustumCullerConfig::PLANE_TOP] = FrustumPlane::fromVec4(top);

    // Near plane: row3 + row2
    glm::vec4 nearPlane(
        m[0][3] + m[0][2],
        m[1][3] + m[1][2],
        m[2][3] + m[2][2],
        m[3][3] + m[3][2]
    );
    outPlanes[FrustumCullerConfig::PLANE_NEAR] = FrustumPlane::fromVec4(nearPlane);

    // Far plane: row3 - row2
    glm::vec4 farPlane(
        m[0][3] - m[0][2],
        m[1][3] - m[1][2],
        m[2][3] - m[2][2],
        m[3][3] - m[3][2]
    );
    outPlanes[FrustumCullerConfig::PLANE_FAR] = FrustumPlane::fromVec4(farPlane);
}

CullResult testAABBAgainstFrustum(const AABB& bounds,
                                  const FrustumPlane planes[6]) {
    // Conservative AABB-frustum test
    // For each plane, find the corner of the AABB furthest along the plane normal
    // If that corner is behind the plane, the AABB is outside

    bool allInside = true;

    for (int i = 0; i < FrustumCullerConfig::NUM_FRUSTUM_PLANES; ++i) {
        const FrustumPlane& plane = planes[i];

        // Find the positive vertex (furthest along plane normal)
        glm::vec3 positiveVertex;
        positiveVertex.x = (plane.normal.x >= 0.0f) ? bounds.max.x : bounds.min.x;
        positiveVertex.y = (plane.normal.y >= 0.0f) ? bounds.max.y : bounds.min.y;
        positiveVertex.z = (plane.normal.z >= 0.0f) ? bounds.max.z : bounds.min.z;

        // Find the negative vertex (furthest along negative plane normal)
        glm::vec3 negativeVertex;
        negativeVertex.x = (plane.normal.x >= 0.0f) ? bounds.min.x : bounds.max.x;
        negativeVertex.y = (plane.normal.y >= 0.0f) ? bounds.min.y : bounds.max.y;
        negativeVertex.z = (plane.normal.z >= 0.0f) ? bounds.min.z : bounds.max.z;

        // If the positive vertex is behind the plane, AABB is outside
        if (plane.signedDistance(positiveVertex) < 0.0f) {
            return CullResult::Outside;
        }

        // If the negative vertex is behind the plane, AABB intersects
        if (plane.signedDistance(negativeVertex) < 0.0f) {
            allInside = false;
        }
    }

    return allInside ? CullResult::Inside : CullResult::Intersects;
}

AABB transformAABBToWorld(const AABB& localBounds, const glm::mat4& modelMatrix) {
    // Transform all 8 corners and find new AABB
    // This produces an axis-aligned result that may be larger than a tight OBB

    glm::vec3 corners[8] = {
        localBounds.min,
        glm::vec3(localBounds.max.x, localBounds.min.y, localBounds.min.z),
        glm::vec3(localBounds.min.x, localBounds.max.y, localBounds.min.z),
        glm::vec3(localBounds.max.x, localBounds.max.y, localBounds.min.z),
        glm::vec3(localBounds.min.x, localBounds.min.y, localBounds.max.z),
        glm::vec3(localBounds.max.x, localBounds.min.y, localBounds.max.z),
        glm::vec3(localBounds.min.x, localBounds.max.y, localBounds.max.z),
        localBounds.max
    };

    AABB worldBounds = AABB::empty();

    for (int i = 0; i < 8; ++i) {
        glm::vec4 transformed = modelMatrix * glm::vec4(corners[i], 1.0f);
        worldBounds.expand(glm::vec3(transformed));
    }

    return worldBounds;
}

} // namespace sims3000
