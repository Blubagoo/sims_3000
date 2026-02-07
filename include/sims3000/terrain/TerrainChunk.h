/**
 * @file TerrainChunk.h
 * @brief CPU-side chunk data structure for terrain mesh generation
 *
 * Defines the TerrainChunk struct that manages terrain mesh data for
 * 32x32 tile regions. Each chunk holds GPU buffer handles and a dirty
 * flag for incremental mesh updates.
 *
 * Chunks align with Epic 2 spatial partitioning (32x32 tiles per chunk).
 * The RenderingSystem uses dirty flags to rebuild only modified chunks,
 * avoiding full terrain mesh regeneration.
 *
 * Resource ownership:
 * - TerrainChunk stores GPU buffer handles (created in 3-025)
 * - GPU memory is released via SDL_ReleaseGPUBuffer on cleanup
 * - Chunks must be properly released before destruction
 *
 * @see TerrainVertex for vertex format
 * @see ChunkDirtyTracker for dirty flag tracking
 */

#ifndef SIMS3000_TERRAIN_TERRAINCHUNK_H
#define SIMS3000_TERRAIN_TERRAINCHUNK_H

#include <sims3000/terrain/ChunkDirtyTracker.h>  // For CHUNK_SIZE constant
#include <sims3000/render/GPUMesh.h>  // For AABB
#include <cstdint>
#include <SDL3/SDL.h>

namespace sims3000 {
namespace terrain {

/// Number of tiles in each dimension of a chunk (32x32)
constexpr std::uint16_t TILES_PER_CHUNK = CHUNK_SIZE;

/// Total number of tiles per chunk (32 * 32 = 1024)
constexpr std::uint32_t TILES_PER_CHUNK_TOTAL = TILES_PER_CHUNK * TILES_PER_CHUNK;

/**
 * @brief Calculate vertices per chunk for terrain mesh.
 *
 * For a 32x32 tile chunk, each tile becomes a quad (2 triangles).
 * Vertices are shared at tile corners: (32+1) * (32+1) = 1089 vertices.
 *
 * Using indexed rendering with shared vertices reduces vertex count
 * and enables smooth normal calculation at tile boundaries.
 */
constexpr std::uint32_t VERTICES_PER_CHUNK = (TILES_PER_CHUNK + 1) * (TILES_PER_CHUNK + 1);

/**
 * @brief Calculate indices per chunk for terrain mesh.
 *
 * Each tile = 2 triangles = 6 indices.
 * 32 * 32 tiles = 1024 tiles * 6 = 6144 indices.
 */
constexpr std::uint32_t INDICES_PER_CHUNK = TILES_PER_CHUNK_TOTAL * 6;

/**
 * @brief Height in world units per elevation level.
 *
 * From Epic 2 ticket 2-033: elevation is mapped to world Y
 * via elevation * 0.25. With 32 elevation levels (0-31),
 * the total height range is 0 to 7.75 world units.
 */
constexpr float ELEVATION_HEIGHT = 0.25f;

/**
 * @struct TerrainChunk
 * @brief CPU-side data structure for a 32x32 tile terrain chunk.
 *
 * Manages the GPU resources and state for rendering a chunk of terrain.
 * The chunk covers a fixed 32x32 tile region aligned to chunk boundaries.
 *
 * Lifecycle:
 * 1. Create TerrainChunk with chunk coordinates
 * 2. Mark as dirty when terrain data changes
 * 3. RenderingSystem detects dirty flag and rebuilds mesh
 * 4. GPU buffers are created/updated (ticket 3-025)
 * 5. Clear dirty flag after successful rebuild
 *
 * Thread safety:
 * - Chunk data is accessed from the main thread only
 * - Dirty flags may be set from simulation thread via events
 */
struct TerrainChunk {
    // =========================================================================
    // Chunk Identity
    // =========================================================================

    std::uint16_t chunk_x;   ///< Chunk X coordinate (0 to chunks_x - 1)
    std::uint16_t chunk_y;   ///< Chunk Y coordinate (0 to chunks_y - 1)

    // =========================================================================
    // GPU Resources
    // =========================================================================

    /**
     * @brief Handle to the GPU vertex buffer.
     *
     * Contains VERTICES_PER_CHUNK TerrainVertex structs.
     * nullptr until first mesh generation (ticket 3-025).
     */
    SDL_GPUBuffer* vertex_buffer;

    /**
     * @brief Handle to the GPU index buffer.
     *
     * Contains INDICES_PER_CHUNK uint16/uint32 indices.
     * nullptr until first mesh generation (ticket 3-025).
     */
    SDL_GPUBuffer* index_buffer;

    // =========================================================================
    // Mesh Metadata
    // =========================================================================

    std::uint32_t vertex_count;   ///< Number of vertices in vertex_buffer
    std::uint32_t index_count;    ///< Number of indices in index_buffer

    // =========================================================================
    // Bounding Volume
    // =========================================================================

    /**
     * @brief Axis-aligned bounding box for frustum culling.
     *
     * Computed from chunk world bounds and max elevation within the chunk.
     * - min = (chunk_x * 32, 0, chunk_y * 32)
     * - max = ((chunk_x+1) * 32, max_elevation * ELEVATION_HEIGHT, (chunk_y+1) * 32)
     *
     * Updated whenever chunk mesh is rebuilt.
     */
    sims3000::AABB aabb;

    // =========================================================================
    // State Flags
    // =========================================================================

    /**
     * @brief Dirty flag indicating the chunk needs mesh rebuild.
     *
     * Set when:
     * - Chunk is first created
     * - Terrain data within the chunk is modified
     * - Full terrain reload occurs
     *
     * Cleared by RenderingSystem after successful mesh rebuild.
     */
    bool dirty;

    /**
     * @brief Flag indicating GPU buffers have been created.
     *
     * Used to determine if buffers need creation vs update.
     */
    bool has_gpu_resources;

    // =========================================================================
    // Constructors
    // =========================================================================

    /**
     * @brief Default constructor - creates an uninitialized chunk.
     *
     * Chunk coordinates are set to 0. GPU resources are null.
     * Chunk starts as dirty (needs first build).
     */
    TerrainChunk()
        : chunk_x(0)
        , chunk_y(0)
        , vertex_buffer(nullptr)
        , index_buffer(nullptr)
        , vertex_count(0)
        , index_count(0)
        , aabb()
        , dirty(true)  // New chunks start dirty
        , has_gpu_resources(false)
    {}

    /**
     * @brief Construct a chunk with specific coordinates.
     *
     * @param cx Chunk X coordinate.
     * @param cy Chunk Y coordinate.
     */
    TerrainChunk(std::uint16_t cx, std::uint16_t cy)
        : chunk_x(cx)
        , chunk_y(cy)
        , vertex_buffer(nullptr)
        , index_buffer(nullptr)
        , vertex_count(0)
        , index_count(0)
        , aabb()
        , dirty(true)  // New chunks start dirty
        , has_gpu_resources(false)
    {}

    // Non-copyable (owns GPU resources)
    TerrainChunk(const TerrainChunk&) = delete;
    TerrainChunk& operator=(const TerrainChunk&) = delete;

    // Movable
    TerrainChunk(TerrainChunk&& other) noexcept
        : chunk_x(other.chunk_x)
        , chunk_y(other.chunk_y)
        , vertex_buffer(other.vertex_buffer)
        , index_buffer(other.index_buffer)
        , vertex_count(other.vertex_count)
        , index_count(other.index_count)
        , aabb(other.aabb)
        , dirty(other.dirty)
        , has_gpu_resources(other.has_gpu_resources)
    {
        // Null out source to prevent double-release
        other.vertex_buffer = nullptr;
        other.index_buffer = nullptr;
        other.has_gpu_resources = false;
    }

    TerrainChunk& operator=(TerrainChunk&& other) noexcept {
        if (this != &other) {
            // Note: Caller must release GPU resources before moving
            chunk_x = other.chunk_x;
            chunk_y = other.chunk_y;
            vertex_buffer = other.vertex_buffer;
            index_buffer = other.index_buffer;
            vertex_count = other.vertex_count;
            index_count = other.index_count;
            aabb = other.aabb;
            dirty = other.dirty;
            has_gpu_resources = other.has_gpu_resources;

            other.vertex_buffer = nullptr;
            other.index_buffer = nullptr;
            other.has_gpu_resources = false;
        }
        return *this;
    }

    // =========================================================================
    // Coordinate Methods
    // =========================================================================

    /**
     * @brief Get the minimum tile X coordinate covered by this chunk.
     * @return First tile X in chunk (chunk_x * TILES_PER_CHUNK).
     */
    std::uint16_t getTileMinX() const {
        return static_cast<std::uint16_t>(chunk_x * TILES_PER_CHUNK);
    }

    /**
     * @brief Get the minimum tile Y coordinate covered by this chunk.
     * @return First tile Y in chunk (chunk_y * TILES_PER_CHUNK).
     */
    std::uint16_t getTileMinY() const {
        return static_cast<std::uint16_t>(chunk_y * TILES_PER_CHUNK);
    }

    /**
     * @brief Get the maximum tile X coordinate covered by this chunk (exclusive).
     * @return One past last tile X in chunk.
     */
    std::uint16_t getTileMaxX() const {
        return static_cast<std::uint16_t>((chunk_x + 1) * TILES_PER_CHUNK);
    }

    /**
     * @brief Get the maximum tile Y coordinate covered by this chunk (exclusive).
     * @return One past last tile Y in chunk.
     */
    std::uint16_t getTileMaxY() const {
        return static_cast<std::uint16_t>((chunk_y + 1) * TILES_PER_CHUNK);
    }

    /**
     * @brief Check if a tile coordinate is within this chunk.
     *
     * @param tile_x Tile X coordinate.
     * @param tile_y Tile Y coordinate.
     * @return true if the tile is within this chunk's bounds.
     */
    bool containsTile(std::int32_t tile_x, std::int32_t tile_y) const {
        return tile_x >= static_cast<std::int32_t>(getTileMinX()) &&
               tile_x < static_cast<std::int32_t>(getTileMaxX()) &&
               tile_y >= static_cast<std::int32_t>(getTileMinY()) &&
               tile_y < static_cast<std::int32_t>(getTileMaxY());
    }

    // =========================================================================
    // State Methods
    // =========================================================================

    /**
     * @brief Mark the chunk as needing mesh rebuild.
     */
    void markDirty() {
        dirty = true;
    }

    /**
     * @brief Clear the dirty flag (after successful rebuild).
     */
    void clearDirty() {
        dirty = false;
    }

    /**
     * @brief Check if the chunk needs mesh rebuild.
     * @return true if dirty flag is set.
     */
    bool isDirty() const {
        return dirty;
    }

    /**
     * @brief Check if the chunk has valid GPU buffers.
     * @return true if both vertex and index buffers are created.
     */
    bool hasGPUResources() const {
        return has_gpu_resources &&
               vertex_buffer != nullptr &&
               index_buffer != nullptr;
    }

    /**
     * @brief Check if the chunk is renderable.
     * @return true if chunk has GPU resources and is not dirty.
     */
    bool isRenderable() const {
        return hasGPUResources() && !dirty;
    }

    // =========================================================================
    // AABB Computation
    // =========================================================================

    /**
     * @brief Compute the axis-aligned bounding box for this chunk.
     *
     * Calculates AABB from chunk world bounds and max elevation within the chunk.
     * Call this after terrain data changes and before rendering.
     *
     * Formula:
     * - AABB min = (chunk_x * 32, 0, chunk_y * 32)
     * - AABB max = ((chunk_x+1) * 32, max_elevation * ELEVATION_HEIGHT, (chunk_y+1) * 32)
     *
     * @tparam GridType A type with at(x, y) returning a type with getElevation().
     * @param grid The terrain grid to read elevation data from.
     */
    template<typename GridType>
    void computeAABB(const GridType& grid) {
        // World-space min corner of chunk
        float world_min_x = static_cast<float>(chunk_x * TILES_PER_CHUNK);
        float world_min_z = static_cast<float>(chunk_y * TILES_PER_CHUNK);

        // World-space max corner of chunk (XZ plane)
        float world_max_x = static_cast<float>((chunk_x + 1) * TILES_PER_CHUNK);
        float world_max_z = static_cast<float>((chunk_y + 1) * TILES_PER_CHUNK);

        // Find max elevation within the chunk
        std::uint8_t max_elevation = 0;
        std::uint16_t start_x = getTileMinX();
        std::uint16_t start_y = getTileMinY();
        std::uint16_t end_x = getTileMaxX();
        std::uint16_t end_y = getTileMaxY();

        for (std::uint16_t y = start_y; y < end_y; ++y) {
            for (std::uint16_t x = start_x; x < end_x; ++x) {
                std::uint8_t elev = grid.at(x, y).getElevation();
                if (elev > max_elevation) {
                    max_elevation = elev;
                }
            }
        }

        // Convert max elevation to world height
        float world_max_y = static_cast<float>(max_elevation) * ELEVATION_HEIGHT;

        // Set AABB bounds
        aabb.min = glm::vec3(world_min_x, 0.0f, world_min_z);
        aabb.max = glm::vec3(world_max_x, world_max_y, world_max_z);
    }

    /**
     * @brief Compute AABB with explicit max elevation value.
     *
     * Alternative version when max elevation is already known,
     * avoiding the need to iterate over the grid.
     *
     * @param max_elevation The maximum elevation value in the chunk.
     */
    void computeAABB(std::uint8_t max_elevation) {
        float world_min_x = static_cast<float>(chunk_x * TILES_PER_CHUNK);
        float world_min_z = static_cast<float>(chunk_y * TILES_PER_CHUNK);
        float world_max_x = static_cast<float>((chunk_x + 1) * TILES_PER_CHUNK);
        float world_max_z = static_cast<float>((chunk_y + 1) * TILES_PER_CHUNK);
        float world_max_y = static_cast<float>(max_elevation) * ELEVATION_HEIGHT;

        aabb.min = glm::vec3(world_min_x, 0.0f, world_min_z);
        aabb.max = glm::vec3(world_max_x, world_max_y, world_max_z);
    }

    // =========================================================================
    // GPU Resource Management (implemented in 3-025)
    // =========================================================================

    /**
     * @brief Release GPU resources.
     *
     * Must be called before chunk destruction if GPU resources were created.
     *
     * @param device The SDL GPU device that owns the buffers.
     */
    void releaseGPUResources(SDL_GPUDevice* device) {
        if (vertex_buffer != nullptr) {
            SDL_ReleaseGPUBuffer(device, vertex_buffer);
            vertex_buffer = nullptr;
        }
        if (index_buffer != nullptr) {
            SDL_ReleaseGPUBuffer(device, index_buffer);
            index_buffer = nullptr;
        }
        vertex_count = 0;
        index_count = 0;
        has_gpu_resources = false;
        dirty = true;  // Mark dirty so it gets rebuilt if needed again
    }
};

// Size verification - TerrainChunk should be reasonable size
// 2 + 2 + 8 + 8 + 4 + 4 + 1 + 1 + padding = ~32 bytes (platform dependent)
// No static_assert on size as it contains pointers

} // namespace terrain
} // namespace sims3000

#endif // SIMS3000_TERRAIN_TERRAINCHUNK_H
