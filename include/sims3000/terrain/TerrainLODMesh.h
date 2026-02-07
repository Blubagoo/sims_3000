/**
 * @file TerrainLODMesh.h
 * @brief Level-of-Detail mesh data structure for terrain chunks.
 *
 * Defines the TerrainLODMesh struct that stores 3 LOD levels for each terrain
 * chunk to reduce triangle count at distance:
 * - LOD 0: Full detail (33x33 = 1089 vertices, 32x32*2 = 2048 triangles)
 * - LOD 1: Half detail (17x17 = 289 vertices, 16x16*2 = 512 triangles)
 * - LOD 2: Quarter detail (9x9 = 81 vertices, 8x8*2 = 128 triangles)
 *
 * Each LOD level uses the same terrain_type and elevation data from the
 * TerrainGrid, just subsampled at different resolutions. Normals are
 * recalculated for each LOD level using coarser sampling.
 *
 * LOD selection is based on chunk distance from camera:
 * - LOD 0: < 64 tiles (default threshold)
 * - LOD 1: 64-128 tiles (default thresholds)
 * - LOD 2: > 128 tiles (default threshold)
 *
 * Resource ownership:
 * - TerrainLODMesh owns its GPU buffers (vertex and index)
 * - GPU memory is released via releaseGPUResources()
 * - Each LOD level has separate vertex/index buffers
 *
 * @see TerrainChunk for the base chunk structure
 * @see TerrainChunkMeshGenerator for LOD mesh generation
 * @see TerrainVertex for the vertex format
 */

#ifndef SIMS3000_TERRAIN_TERRAINLODMESH_H
#define SIMS3000_TERRAIN_TERRAINLODMESH_H

#include <SDL3/SDL.h>
#include <sims3000/render/GPUMesh.h>  // For AABB
#include <cstdint>

namespace sims3000 {
namespace terrain {

// ============================================================================
// LOD Level Constants
// ============================================================================

/// Number of LOD levels for terrain chunks
constexpr std::uint8_t TERRAIN_LOD_LEVEL_COUNT = 3;

/// LOD 0: Full detail - 33x33 vertices (sampling every 1 tile)
constexpr std::uint8_t TERRAIN_LOD_0 = 0;

/// LOD 1: Half detail - 17x17 vertices (sampling every 2 tiles)
constexpr std::uint8_t TERRAIN_LOD_1 = 1;

/// LOD 2: Quarter detail - 9x9 vertices (sampling every 4 tiles)
constexpr std::uint8_t TERRAIN_LOD_2 = 2;

// ============================================================================
// LOD Level Vertex Counts
// ============================================================================

/// Vertices per chunk at LOD 0: (32/1 + 1)^2 = 33^2 = 1089
constexpr std::uint32_t LOD0_VERTEX_GRID_SIZE = 33;
constexpr std::uint32_t LOD0_VERTICES_PER_CHUNK = LOD0_VERTEX_GRID_SIZE * LOD0_VERTEX_GRID_SIZE;

/// Vertices per chunk at LOD 1: (32/2 + 1)^2 = 17^2 = 289
constexpr std::uint32_t LOD1_VERTEX_GRID_SIZE = 17;
constexpr std::uint32_t LOD1_VERTICES_PER_CHUNK = LOD1_VERTEX_GRID_SIZE * LOD1_VERTEX_GRID_SIZE;

/// Vertices per chunk at LOD 2: (32/4 + 1)^2 = 9^2 = 81
constexpr std::uint32_t LOD2_VERTEX_GRID_SIZE = 9;
constexpr std::uint32_t LOD2_VERTICES_PER_CHUNK = LOD2_VERTEX_GRID_SIZE * LOD2_VERTEX_GRID_SIZE;

// ============================================================================
// LOD Level Index Counts
// ============================================================================

/// Indices per chunk at LOD 0: 32*32*6 = 6144
constexpr std::uint32_t LOD0_TILES_PER_SIDE = 32;
constexpr std::uint32_t LOD0_INDICES_PER_CHUNK = LOD0_TILES_PER_SIDE * LOD0_TILES_PER_SIDE * 6;

/// Indices per chunk at LOD 1: 16*16*6 = 1536
constexpr std::uint32_t LOD1_TILES_PER_SIDE = 16;
constexpr std::uint32_t LOD1_INDICES_PER_CHUNK = LOD1_TILES_PER_SIDE * LOD1_TILES_PER_SIDE * 6;

/// Indices per chunk at LOD 2: 8*8*6 = 384
constexpr std::uint32_t LOD2_TILES_PER_SIDE = 8;
constexpr std::uint32_t LOD2_INDICES_PER_CHUNK = LOD2_TILES_PER_SIDE * LOD2_TILES_PER_SIDE * 6;

// ============================================================================
// LOD Distance Thresholds (in tiles)
// ============================================================================

/// Default distance threshold for LOD 0 -> LOD 1 transition
constexpr float DEFAULT_LOD0_TO_LOD1_DISTANCE = 64.0f;

/// Default distance threshold for LOD 1 -> LOD 2 transition
constexpr float DEFAULT_LOD1_TO_LOD2_DISTANCE = 128.0f;

// ============================================================================
// LOD Step Sizes
// ============================================================================

/// Vertex sampling step for LOD 0 (every tile)
constexpr std::uint8_t LOD0_STEP = 1;

/// Vertex sampling step for LOD 1 (every 2nd tile)
constexpr std::uint8_t LOD1_STEP = 2;

/// Vertex sampling step for LOD 2 (every 4th tile)
constexpr std::uint8_t LOD2_STEP = 4;

// ============================================================================
// Skirt Geometry Constants (Ticket 3-033 - LOD Seam Mitigation)
// ============================================================================

/// Default skirt height in world units (extends downward from edge vertices)
/// Set to 0.5 units to hide gaps at LOD transitions while remaining invisible
/// from above on flat terrain.
constexpr float DEFAULT_SKIRT_HEIGHT = 0.5f;

/// Minimum skirt height (useful for configurable settings)
constexpr float MIN_SKIRT_HEIGHT = 0.1f;

/// Maximum skirt height (beyond this, skirts may become visible from side views)
constexpr float MAX_SKIRT_HEIGHT = 2.0f;

// ============================================================================
// TerrainLODLevel Struct
// ============================================================================

/**
 * @struct TerrainLODLevel
 * @brief GPU resources for a single LOD level of a terrain chunk.
 *
 * Contains the vertex and index buffers for one LOD level.
 * Each level has different vertex density based on the sampling step.
 */
struct TerrainLODLevel {
    /// GPU vertex buffer for this LOD level
    SDL_GPUBuffer* vertex_buffer = nullptr;

    /// GPU index buffer for this LOD level
    SDL_GPUBuffer* index_buffer = nullptr;

    /// Number of vertices in the vertex buffer
    std::uint32_t vertex_count = 0;

    /// Number of indices in the index buffer
    std::uint32_t index_count = 0;

    /**
     * @brief Check if this LOD level has valid GPU resources.
     * @return true if both buffers are valid and counts are non-zero.
     */
    bool isValid() const {
        return vertex_buffer != nullptr &&
               index_buffer != nullptr &&
               vertex_count > 0 &&
               index_count > 0;
    }
};

// ============================================================================
// TerrainLODMesh Struct
// ============================================================================

/**
 * @struct TerrainLODMesh
 * @brief Container for all LOD levels of a terrain chunk.
 *
 * Stores 3 LOD levels for a single terrain chunk:
 * - LOD 0: 1089 vertices, 6144 indices (full detail)
 * - LOD 1: 289 vertices, 1536 indices (half detail)
 * - LOD 2: 81 vertices, 384 indices (quarter detail)
 *
 * The RenderingSystem selects the appropriate LOD level based on
 * the chunk's distance from the camera.
 */
struct TerrainLODMesh {
    /// Array of 3 LOD levels
    TerrainLODLevel levels[TERRAIN_LOD_LEVEL_COUNT];

    /// Axis-aligned bounding box (shared across all LOD levels)
    sims3000::AABB aabb;

    /// Chunk X coordinate
    std::uint16_t chunk_x = 0;

    /// Chunk Y coordinate
    std::uint16_t chunk_y = 0;

    /// Whether all LOD levels have been generated
    bool complete = false;

    // =========================================================================
    // Constructors
    // =========================================================================

    /**
     * @brief Default constructor - creates uninitialized LOD mesh.
     */
    TerrainLODMesh() = default;

    /**
     * @brief Construct LOD mesh with chunk coordinates.
     * @param cx Chunk X coordinate.
     * @param cy Chunk Y coordinate.
     */
    TerrainLODMesh(std::uint16_t cx, std::uint16_t cy)
        : chunk_x(cx)
        , chunk_y(cy)
        , complete(false)
    {}

    // Non-copyable (owns GPU resources)
    TerrainLODMesh(const TerrainLODMesh&) = delete;
    TerrainLODMesh& operator=(const TerrainLODMesh&) = delete;

    // Moveable
    TerrainLODMesh(TerrainLODMesh&& other) noexcept
        : aabb(other.aabb)
        , chunk_x(other.chunk_x)
        , chunk_y(other.chunk_y)
        , complete(other.complete)
    {
        for (std::uint8_t i = 0; i < TERRAIN_LOD_LEVEL_COUNT; ++i) {
            levels[i] = other.levels[i];
            other.levels[i].vertex_buffer = nullptr;
            other.levels[i].index_buffer = nullptr;
            other.levels[i].vertex_count = 0;
            other.levels[i].index_count = 0;
        }
        other.complete = false;
    }

    TerrainLODMesh& operator=(TerrainLODMesh&& other) noexcept {
        if (this != &other) {
            // Note: Caller must release GPU resources before moving
            for (std::uint8_t i = 0; i < TERRAIN_LOD_LEVEL_COUNT; ++i) {
                levels[i] = other.levels[i];
                other.levels[i].vertex_buffer = nullptr;
                other.levels[i].index_buffer = nullptr;
                other.levels[i].vertex_count = 0;
                other.levels[i].index_count = 0;
            }
            aabb = other.aabb;
            chunk_x = other.chunk_x;
            chunk_y = other.chunk_y;
            complete = other.complete;
            other.complete = false;
        }
        return *this;
    }

    // =========================================================================
    // Accessors
    // =========================================================================

    /**
     * @brief Get a specific LOD level.
     * @param level LOD level (0, 1, or 2).
     * @return Reference to the LOD level data.
     */
    const TerrainLODLevel& getLevel(std::uint8_t level) const {
        if (level >= TERRAIN_LOD_LEVEL_COUNT) {
            level = TERRAIN_LOD_LEVEL_COUNT - 1;
        }
        return levels[level];
    }

    /**
     * @brief Get a mutable reference to a specific LOD level.
     * @param level LOD level (0, 1, or 2).
     * @return Mutable reference to the LOD level data.
     */
    TerrainLODLevel& getLevel(std::uint8_t level) {
        if (level >= TERRAIN_LOD_LEVEL_COUNT) {
            level = TERRAIN_LOD_LEVEL_COUNT - 1;
        }
        return levels[level];
    }

    /**
     * @brief Check if a specific LOD level is valid.
     * @param level LOD level to check.
     * @return true if the LOD level has valid GPU resources.
     */
    bool isLevelValid(std::uint8_t level) const {
        if (level >= TERRAIN_LOD_LEVEL_COUNT) {
            return false;
        }
        return levels[level].isValid();
    }

    /**
     * @brief Check if all LOD levels are valid and ready for rendering.
     * @return true if all 3 LOD levels have valid GPU resources.
     */
    bool isRenderable() const {
        return complete &&
               levels[0].isValid() &&
               levels[1].isValid() &&
               levels[2].isValid();
    }

    /**
     * @brief Get total vertex count across all LOD levels.
     * @return Sum of vertex counts for all levels.
     */
    std::uint32_t getTotalVertexCount() const {
        std::uint32_t total = 0;
        for (std::uint8_t i = 0; i < TERRAIN_LOD_LEVEL_COUNT; ++i) {
            total += levels[i].vertex_count;
        }
        return total;
    }

    /**
     * @brief Get total index count across all LOD levels.
     * @return Sum of index counts for all levels.
     */
    std::uint32_t getTotalIndexCount() const {
        std::uint32_t total = 0;
        for (std::uint8_t i = 0; i < TERRAIN_LOD_LEVEL_COUNT; ++i) {
            total += levels[i].index_count;
        }
        return total;
    }

    // =========================================================================
    // GPU Resource Management
    // =========================================================================

    /**
     * @brief Release all GPU resources for all LOD levels.
     * @param device The SDL GPU device that owns the buffers.
     */
    void releaseGPUResources(SDL_GPUDevice* device) {
        if (device == nullptr) {
            return;
        }

        for (std::uint8_t i = 0; i < TERRAIN_LOD_LEVEL_COUNT; ++i) {
            if (levels[i].vertex_buffer != nullptr) {
                SDL_ReleaseGPUBuffer(device, levels[i].vertex_buffer);
                levels[i].vertex_buffer = nullptr;
            }
            if (levels[i].index_buffer != nullptr) {
                SDL_ReleaseGPUBuffer(device, levels[i].index_buffer);
                levels[i].index_buffer = nullptr;
            }
            levels[i].vertex_count = 0;
            levels[i].index_count = 0;
        }
        complete = false;
    }

    /**
     * @brief Release GPU resources for a specific LOD level.
     * @param device The SDL GPU device that owns the buffers.
     * @param level LOD level to release (0, 1, or 2).
     */
    void releaseLevelGPUResources(SDL_GPUDevice* device, std::uint8_t level) {
        if (device == nullptr || level >= TERRAIN_LOD_LEVEL_COUNT) {
            return;
        }

        if (levels[level].vertex_buffer != nullptr) {
            SDL_ReleaseGPUBuffer(device, levels[level].vertex_buffer);
            levels[level].vertex_buffer = nullptr;
        }
        if (levels[level].index_buffer != nullptr) {
            SDL_ReleaseGPUBuffer(device, levels[level].index_buffer);
            levels[level].index_buffer = nullptr;
        }
        levels[level].vertex_count = 0;
        levels[level].index_count = 0;

        // Update complete flag
        complete = levels[0].isValid() && levels[1].isValid() && levels[2].isValid();
    }
};

// ============================================================================
// LOD Selection Configuration
// ============================================================================

/**
 * @struct TerrainLODConfig
 * @brief Configuration for terrain LOD distance thresholds.
 *
 * Configurable thresholds for LOD level selection based on
 * chunk distance from camera (measured in tiles).
 */
struct TerrainLODConfig {
    /// Distance threshold for LOD 0 -> LOD 1 transition (tiles)
    float lod0_to_lod1_distance = DEFAULT_LOD0_TO_LOD1_DISTANCE;

    /// Distance threshold for LOD 1 -> LOD 2 transition (tiles)
    float lod1_to_lod2_distance = DEFAULT_LOD1_TO_LOD2_DISTANCE;

    /// Hysteresis margin to prevent rapid LOD switching (tiles)
    float hysteresis = 2.0f;

    /**
     * @brief Select LOD level based on distance.
     * @param distance Distance from camera to chunk center (tiles).
     * @return LOD level (0, 1, or 2).
     */
    std::uint8_t selectLODLevel(float distance) const {
        if (distance < lod0_to_lod1_distance) {
            return TERRAIN_LOD_0;
        } else if (distance < lod1_to_lod2_distance) {
            return TERRAIN_LOD_1;
        } else {
            return TERRAIN_LOD_2;
        }
    }

    /**
     * @brief Select LOD level with hysteresis to prevent rapid switching.
     * @param distance Distance from camera to chunk center (tiles).
     * @param current_level Current LOD level.
     * @return New LOD level (may be same as current if within hysteresis zone).
     */
    std::uint8_t selectLODLevelWithHysteresis(float distance, std::uint8_t current_level) const {
        std::uint8_t new_level = selectLODLevel(distance);

        // Apply hysteresis when moving to lower detail
        if (new_level > current_level) {
            // Moving to lower detail - require crossing threshold + hysteresis
            if (current_level == TERRAIN_LOD_0) {
                if (distance < lod0_to_lod1_distance + hysteresis) {
                    return current_level;
                }
            } else if (current_level == TERRAIN_LOD_1) {
                if (distance < lod1_to_lod2_distance + hysteresis) {
                    return current_level;
                }
            }
        }
        // Moving to higher detail - require crossing threshold - hysteresis
        else if (new_level < current_level) {
            if (current_level == TERRAIN_LOD_1) {
                if (distance > lod0_to_lod1_distance - hysteresis) {
                    return current_level;
                }
            } else if (current_level == TERRAIN_LOD_2) {
                if (distance > lod1_to_lod2_distance - hysteresis) {
                    return current_level;
                }
            }
        }

        return new_level;
    }
};

// ============================================================================
// Utility Functions
// ============================================================================

/**
 * @brief Get the vertex grid size for a given LOD level.
 * @param level LOD level (0, 1, or 2).
 * @return Number of vertices per side for the LOD level.
 */
inline std::uint32_t getVertexGridSize(std::uint8_t level) {
    switch (level) {
        case TERRAIN_LOD_0: return LOD0_VERTEX_GRID_SIZE;
        case TERRAIN_LOD_1: return LOD1_VERTEX_GRID_SIZE;
        case TERRAIN_LOD_2: return LOD2_VERTEX_GRID_SIZE;
        default: return LOD0_VERTEX_GRID_SIZE;
    }
}

/**
 * @brief Get the vertex count for a given LOD level.
 * @param level LOD level (0, 1, or 2).
 * @return Number of vertices for the LOD level.
 */
inline std::uint32_t getVertexCount(std::uint8_t level) {
    switch (level) {
        case TERRAIN_LOD_0: return LOD0_VERTICES_PER_CHUNK;
        case TERRAIN_LOD_1: return LOD1_VERTICES_PER_CHUNK;
        case TERRAIN_LOD_2: return LOD2_VERTICES_PER_CHUNK;
        default: return LOD0_VERTICES_PER_CHUNK;
    }
}

/**
 * @brief Get the index count for a given LOD level.
 * @param level LOD level (0, 1, or 2).
 * @return Number of indices for the LOD level.
 */
inline std::uint32_t getIndexCount(std::uint8_t level) {
    switch (level) {
        case TERRAIN_LOD_0: return LOD0_INDICES_PER_CHUNK;
        case TERRAIN_LOD_1: return LOD1_INDICES_PER_CHUNK;
        case TERRAIN_LOD_2: return LOD2_INDICES_PER_CHUNK;
        default: return LOD0_INDICES_PER_CHUNK;
    }
}

/**
 * @brief Get the sampling step for a given LOD level.
 * @param level LOD level (0, 1, or 2).
 * @return Vertex sampling step (1, 2, or 4).
 */
inline std::uint8_t getLODStep(std::uint8_t level) {
    switch (level) {
        case TERRAIN_LOD_0: return LOD0_STEP;
        case TERRAIN_LOD_1: return LOD1_STEP;
        case TERRAIN_LOD_2: return LOD2_STEP;
        default: return LOD0_STEP;
    }
}

/**
 * @brief Get the tiles per side for a given LOD level.
 * @param level LOD level (0, 1, or 2).
 * @return Number of tiles per side at the LOD level.
 */
inline std::uint32_t getTilesPerSide(std::uint8_t level) {
    switch (level) {
        case TERRAIN_LOD_0: return LOD0_TILES_PER_SIDE;
        case TERRAIN_LOD_1: return LOD1_TILES_PER_SIDE;
        case TERRAIN_LOD_2: return LOD2_TILES_PER_SIDE;
        default: return LOD0_TILES_PER_SIDE;
    }
}

/**
 * @brief Calculate triangle count for a given LOD level.
 * @param level LOD level (0, 1, or 2).
 * @return Number of triangles at the LOD level.
 */
inline std::uint32_t getTriangleCount(std::uint8_t level) {
    return getIndexCount(level) / 3;
}

/**
 * @brief Calculate triangle reduction percentage from LOD 0.
 * @param level LOD level (0, 1, or 2).
 * @return Percentage reduction (0.0 for LOD 0, higher for other levels).
 */
inline float getTriangleReductionPercent(std::uint8_t level) {
    std::uint32_t lod0_tris = getTriangleCount(TERRAIN_LOD_0);
    std::uint32_t level_tris = getTriangleCount(level);
    return 100.0f * (1.0f - static_cast<float>(level_tris) / static_cast<float>(lod0_tris));
}

// ============================================================================
// Skirt Geometry Utility Functions (Ticket 3-033 - LOD Seam Mitigation)
// ============================================================================

/**
 * @brief Get the number of skirt vertices per edge for a given LOD level.
 *
 * Skirt vertices match the edge vertex count:
 * - LOD 0: 33 vertices per edge
 * - LOD 1: 17 vertices per edge
 * - LOD 2: 9 vertices per edge
 *
 * Each edge has original vertices and duplicated skirt vertices (2x per edge).
 *
 * @param level LOD level (0, 1, or 2).
 * @return Number of skirt vertices per edge (same as grid size).
 */
inline std::uint32_t getSkirtVerticesPerEdge(std::uint8_t level) {
    return getVertexGridSize(level);
}

/**
 * @brief Get total skirt vertex count for all 4 edges of a chunk.
 *
 * Each chunk has 4 edges, and each edge needs skirt vertices.
 * Corners are shared between adjacent edges, so we count:
 * - 4 edges x grid_size vertices = 4 * grid_size
 *
 * Note: This is the number of additional vertices for skirts only.
 * The original edge vertices are already in the surface mesh.
 *
 * @param level LOD level (0, 1, or 2).
 * @return Total skirt vertex count for all 4 edges.
 */
inline std::uint32_t getTotalSkirtVertexCount(std::uint8_t level) {
    // 4 edges, each with grid_size skirt vertices
    return 4 * getVertexGridSize(level);
}

/**
 * @brief Get total skirt index count for all 4 edges of a chunk.
 *
 * Each edge creates (grid_size - 1) quads (2 triangles = 6 indices each).
 * 4 edges x (grid_size - 1) quads x 6 indices
 *
 * @param level LOD level (0, 1, or 2).
 * @return Total skirt index count for all 4 edges.
 */
inline std::uint32_t getTotalSkirtIndexCount(std::uint8_t level) {
    std::uint32_t grid_size = getVertexGridSize(level);
    // (grid_size - 1) quads per edge, 4 edges, 6 indices per quad
    return 4 * (grid_size - 1) * 6;
}

} // namespace terrain
} // namespace sims3000

#endif // SIMS3000_TERRAIN_TERRAINLODMESH_H
