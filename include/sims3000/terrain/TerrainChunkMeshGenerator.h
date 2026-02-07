/**
 * @file TerrainChunkMeshGenerator.h
 * @brief Terrain chunk mesh generation for GPU rendering.
 *
 * Converts TerrainGrid data into GPU-ready vertex/index buffers for 32x32
 * tile chunks. Generates shared-corner vertices with position, normal, UV,
 * tile_coord, and terrain_type. Handles cliff face geometry for steep
 * elevation transitions.
 *
 * Key features:
 * - Generates complete mesh for 32x32 tile chunks
 * - Vertex positions: x = tile_x, y = elevation * ELEVATION_HEIGHT, z = tile_z
 * - Normals computed via central differences (TerrainNormals.h)
 * - Per-vertex terrain_type for shader lookup
 * - Cliff face geometry when elevation delta > threshold (default: 2 levels)
 * - Cliff face normals oriented horizontally for toon shader shadow bands
 * - Incremental rebuild: only regenerate dirty chunks
 * - At most 1 chunk rebuilt per frame to avoid GPU stalls
 * - Performance target: single chunk rebuild < 1ms
 *
 * Resource ownership:
 * - TerrainChunkMeshGenerator does NOT own GPU resources
 * - TerrainChunk owns its GPU buffers (created via SDL_CreateGPUBuffer)
 * - GPU memory is released via TerrainChunk::releaseGPUResources()
 *
 * @see TerrainVertex for vertex format (44 bytes)
 * @see TerrainChunk for chunk structure with AABB
 * @see TerrainNormals for normal calculation
 * @see ChunkDirtyTracker for dirty flag tracking
 */

#ifndef SIMS3000_TERRAIN_TERRAINCHUNKMESHGENERATOR_H
#define SIMS3000_TERRAIN_TERRAINCHUNKMESHGENERATOR_H

#include <sims3000/terrain/TerrainGrid.h>
#include <sims3000/terrain/TerrainChunk.h>
#include <sims3000/terrain/TerrainVertex.h>
#include <sims3000/terrain/TerrainNormals.h>
#include <sims3000/terrain/TerrainLODMesh.h>
#include <sims3000/terrain/ChunkDirtyTracker.h>
#include <SDL3/SDL.h>
#include <cstdint>
#include <vector>
#include <deque>

namespace sims3000 {
namespace terrain {

/// Default elevation delta threshold for cliff face generation (2 levels)
constexpr std::uint8_t DEFAULT_CLIFF_THRESHOLD = 2;

/// Maximum number of chunks to rebuild per frame (1 to avoid GPU stalls)
constexpr std::uint32_t MAX_CHUNKS_PER_FRAME = 1;

/**
 * @struct ChunkMeshData
 * @brief CPU-side mesh data generated for a chunk.
 *
 * Contains vertex and index data ready for GPU upload.
 * This struct is used as an intermediate representation before
 * uploading to GPU buffers.
 */
struct ChunkMeshData {
    std::vector<TerrainVertex> vertices;   ///< Vertex data
    std::vector<std::uint32_t> indices;    ///< Index data (uint32 for large meshes)
    std::uint8_t max_elevation;            ///< Maximum elevation in chunk (for AABB)
    std::uint8_t min_elevation;            ///< Minimum elevation in chunk
    bool has_cliff_faces;                  ///< Whether cliff faces were generated

    ChunkMeshData()
        : vertices()
        , indices()
        , max_elevation(0)
        , min_elevation(255)
        , has_cliff_faces(false)
    {}

    void clear() {
        vertices.clear();
        indices.clear();
        max_elevation = 0;
        min_elevation = 255;
        has_cliff_faces = false;
    }

    void reserve(std::uint32_t vertex_count, std::uint32_t index_count) {
        vertices.reserve(vertex_count);
        indices.reserve(index_count);
    }
};

/**
 * @struct ChunkRebuildStats
 * @brief Statistics for chunk mesh rebuild operations.
 */
struct ChunkRebuildStats {
    std::uint32_t chunks_rebuilt;          ///< Number of chunks rebuilt this frame
    std::uint32_t vertices_generated;      ///< Total vertices generated
    std::uint32_t indices_generated;       ///< Total indices generated
    std::uint32_t cliff_faces_generated;   ///< Number of cliff face quads generated
    float rebuild_time_ms;                 ///< Time spent rebuilding (milliseconds)
    std::uint32_t chunks_pending;          ///< Number of dirty chunks still pending

    ChunkRebuildStats()
        : chunks_rebuilt(0)
        , vertices_generated(0)
        , indices_generated(0)
        , cliff_faces_generated(0)
        , rebuild_time_ms(0.0f)
        , chunks_pending(0)
    {}
};

/**
 * @class TerrainChunkMeshGenerator
 * @brief Generates terrain chunk meshes from grid data.
 *
 * Responsible for:
 * - Converting TerrainGrid data to GPU-ready vertex/index buffers
 * - Generating shared-corner vertices with proper attributes
 * - Creating cliff face geometry for steep transitions
 * - Managing incremental chunk rebuilds (at most 1 per frame)
 * - Queueing dirty chunks for processing
 *
 * Usage:
 * @code
 * TerrainGrid grid(MapSize::Medium);
 * std::vector<TerrainChunk> chunks(16);  // 256x256 / 32 = 8x8 = 64 chunks
 *
 * TerrainChunkMeshGenerator generator;
 * generator.initialize(grid.width, grid.height);
 *
 * // Initial build: generate all chunks
 * generator.buildAllChunks(device, grid, chunks);
 *
 * // Per-frame incremental update
 * generator.updateDirtyChunks(device, grid, chunks, dirty_tracker);
 * @endcode
 */
class TerrainChunkMeshGenerator {
public:
    /**
     * @brief Default constructor.
     */
    TerrainChunkMeshGenerator();

    /**
     * @brief Initialize the generator for a specific map size.
     *
     * @param map_width Map width in tiles (128, 256, or 512).
     * @param map_height Map height in tiles (128, 256, or 512).
     */
    void initialize(std::uint16_t map_width, std::uint16_t map_height);

    /**
     * @brief Set the cliff face threshold.
     *
     * Cliff faces are generated when the elevation delta between adjacent
     * tiles exceeds this threshold. Default is 2 elevation levels.
     *
     * @param threshold Elevation delta threshold for cliff face generation.
     */
    void setCliffThreshold(std::uint8_t threshold);

    /**
     * @brief Get the current cliff face threshold.
     * @return Cliff face threshold in elevation levels.
     */
    std::uint8_t getCliffThreshold() const { return cliff_threshold_; }

    /**
     * @brief Set the skirt height for LOD seam mitigation.
     *
     * Skirt geometry extends edge vertices downward by this amount to hide
     * gaps between chunks at different LOD levels. Default is 0.5 world units.
     *
     * @param height Skirt extension height in world units.
     */
    void setSkirtHeight(float height);

    /**
     * @brief Get the current skirt height.
     * @return Skirt height in world units.
     */
    float getSkirtHeight() const { return skirt_height_; }

    // =========================================================================
    // Full Map Building
    // =========================================================================

    /**
     * @brief Build all chunk meshes during initial map loading.
     *
     * Generates meshes for ALL chunks synchronously. This should only be
     * called during map loading, not during gameplay.
     *
     * @param device SDL GPU device for buffer creation.
     * @param grid The terrain grid data.
     * @param chunks Vector of chunks to fill (must be pre-sized).
     * @return true if all chunks were built successfully.
     */
    bool buildAllChunks(
        SDL_GPUDevice* device,
        const TerrainGrid& grid,
        std::vector<TerrainChunk>& chunks
    );

    // =========================================================================
    // Incremental Updates
    // =========================================================================

    /**
     * @brief Queue a chunk for rebuild.
     *
     * Adds the chunk to the rebuild queue. The chunk will be rebuilt
     * during the next call to updateDirtyChunks().
     *
     * @param chunk_x Chunk X coordinate.
     * @param chunk_y Chunk Y coordinate.
     */
    void queueChunkRebuild(std::uint16_t chunk_x, std::uint16_t chunk_y);

    /**
     * @brief Queue all dirty chunks from the tracker.
     *
     * Scans the dirty tracker and queues all dirty chunks for rebuild.
     *
     * @param tracker The chunk dirty tracker.
     */
    void queueDirtyChunks(const ChunkDirtyTracker& tracker);

    /**
     * @brief Update dirty chunks (at most 1 per frame).
     *
     * Processes the rebuild queue, rebuilding at most MAX_CHUNKS_PER_FRAME
     * chunks per call. Returns statistics about the rebuild operation.
     *
     * @param device SDL GPU device for buffer creation.
     * @param grid The terrain grid data.
     * @param chunks Vector of chunks to update.
     * @param tracker Dirty tracker to clear flags after rebuild.
     * @return Statistics about chunks rebuilt.
     */
    ChunkRebuildStats updateDirtyChunks(
        SDL_GPUDevice* device,
        const TerrainGrid& grid,
        std::vector<TerrainChunk>& chunks,
        ChunkDirtyTracker& tracker
    );

    /**
     * @brief Check if there are chunks pending rebuild.
     * @return true if rebuild queue is not empty.
     */
    bool hasPendingRebuilds() const { return !rebuild_queue_.empty(); }

    /**
     * @brief Get number of chunks pending rebuild.
     * @return Number of chunks in rebuild queue.
     */
    std::uint32_t getPendingRebuildCount() const {
        return static_cast<std::uint32_t>(rebuild_queue_.size());
    }

    // =========================================================================
    // Single Chunk Operations
    // =========================================================================

    /**
     * @brief Generate mesh data for a single chunk (CPU-side only).
     *
     * Generates vertex and index data for the specified chunk.
     * Does NOT upload to GPU - use uploadChunkMesh() for that.
     *
     * @param grid The terrain grid data.
     * @param chunk_x Chunk X coordinate.
     * @param chunk_y Chunk Y coordinate.
     * @param out_data Output mesh data.
     * @return true if mesh was generated successfully.
     */
    bool generateChunkMesh(
        const TerrainGrid& grid,
        std::uint16_t chunk_x,
        std::uint16_t chunk_y,
        ChunkMeshData& out_data
    );

    /**
     * @brief Upload chunk mesh data to GPU.
     *
     * Creates or updates GPU buffers for the chunk using the provided
     * mesh data. Updates the chunk's AABB based on elevation data.
     *
     * @param device SDL GPU device.
     * @param mesh_data Mesh data to upload.
     * @param chunk Chunk to update.
     * @return true if upload succeeded.
     */
    bool uploadChunkMesh(
        SDL_GPUDevice* device,
        const ChunkMeshData& mesh_data,
        TerrainChunk& chunk
    );

    /**
     * @brief Rebuild a single chunk completely.
     *
     * Generates mesh data and uploads to GPU in one operation.
     *
     * @param device SDL GPU device.
     * @param grid The terrain grid data.
     * @param chunk Chunk to rebuild.
     * @return true if rebuild succeeded.
     */
    bool rebuildChunk(
        SDL_GPUDevice* device,
        const TerrainGrid& grid,
        TerrainChunk& chunk
    );

    // =========================================================================
    // LOD Mesh Generation (Ticket 3-032)
    // =========================================================================

    /**
     * @brief Generate mesh data for a specific LOD level (CPU-side only).
     *
     * Generates vertex and index data for the specified chunk at a given
     * LOD level. Uses subsampling based on the LOD step:
     * - LOD 0: every tile (step=1), 33x33 = 1089 vertices
     * - LOD 1: every 2nd tile (step=2), 17x17 = 289 vertices
     * - LOD 2: every 4th tile (step=4), 9x9 = 81 vertices
     *
     * Normals are recalculated for each LOD level using coarser sampling.
     *
     * @param grid The terrain grid data.
     * @param chunk_x Chunk X coordinate.
     * @param chunk_y Chunk Y coordinate.
     * @param lod_level LOD level (0, 1, or 2).
     * @param out_data Output mesh data.
     * @return true if mesh was generated successfully.
     */
    bool generateLODMesh(
        const TerrainGrid& grid,
        std::uint16_t chunk_x,
        std::uint16_t chunk_y,
        std::uint8_t lod_level,
        ChunkMeshData& out_data
    );

    /**
     * @brief Generate all 3 LOD levels for a chunk (CPU-side only).
     *
     * Generates vertex and index data for all 3 LOD levels of a chunk.
     * Returns data in an array indexed by LOD level.
     *
     * @param grid The terrain grid data.
     * @param chunk_x Chunk X coordinate.
     * @param chunk_y Chunk Y coordinate.
     * @param out_lod_data Output array of 3 ChunkMeshData (one per LOD level).
     * @return true if all LOD levels were generated successfully.
     */
    bool generateAllLODMeshes(
        const TerrainGrid& grid,
        std::uint16_t chunk_x,
        std::uint16_t chunk_y,
        ChunkMeshData out_lod_data[TERRAIN_LOD_LEVEL_COUNT]
    );

    /**
     * @brief Upload a single LOD level to GPU.
     *
     * Creates or updates GPU buffers for one LOD level of a TerrainLODMesh.
     *
     * @param device SDL GPU device.
     * @param mesh_data Mesh data to upload.
     * @param lod_mesh Target LOD mesh.
     * @param lod_level LOD level (0, 1, or 2).
     * @return true if upload succeeded.
     */
    bool uploadLODMesh(
        SDL_GPUDevice* device,
        const ChunkMeshData& mesh_data,
        TerrainLODMesh& lod_mesh,
        std::uint8_t lod_level
    );

    /**
     * @brief Generate and upload all 3 LOD levels for a chunk.
     *
     * Generates mesh data for all LOD levels and uploads them to GPU.
     * Updates the TerrainLODMesh AABB and marks it complete.
     *
     * @param device SDL GPU device.
     * @param grid The terrain grid data.
     * @param lod_mesh Target LOD mesh to fill.
     * @return true if all LOD levels were generated and uploaded successfully.
     */
    bool rebuildAllLODLevels(
        SDL_GPUDevice* device,
        const TerrainGrid& grid,
        TerrainLODMesh& lod_mesh
    );

    /**
     * @brief Build all LOD meshes for the entire terrain.
     *
     * Generates all 3 LOD levels for ALL chunks synchronously.
     * Should only be called during initial map loading.
     *
     * @param device SDL GPU device.
     * @param grid The terrain grid data.
     * @param lod_meshes Vector of LOD meshes to fill (must be pre-sized).
     * @return true if all chunks were built successfully.
     */
    bool buildAllLODMeshes(
        SDL_GPUDevice* device,
        const TerrainGrid& grid,
        std::vector<TerrainLODMesh>& lod_meshes
    );

private:
    // =========================================================================
    // Mesh Generation Helpers
    // =========================================================================

    /**
     * @brief Generate terrain surface vertices for a chunk.
     *
     * Creates vertices at tile corners with:
     * - Position: (tile_x, elevation * ELEVATION_HEIGHT, tile_y)
     * - Normal: computed via central differences
     * - UV: normalized to chunk (0-1 range)
     * - Terrain type: from grid data
     *
     * @param grid The terrain grid.
     * @param chunk_x Chunk X coordinate.
     * @param chunk_y Chunk Y coordinate.
     * @param out_data Output mesh data.
     */
    void generateSurfaceVertices(
        const TerrainGrid& grid,
        std::uint16_t chunk_x,
        std::uint16_t chunk_y,
        ChunkMeshData& out_data
    );

    /**
     * @brief Generate surface indices for a chunk.
     *
     * Creates triangle indices for the terrain surface quads.
     * Uses counter-clockwise winding order.
     *
     * @param out_data Output mesh data (appends indices).
     */
    void generateSurfaceIndices(ChunkMeshData& out_data);

    /**
     * @brief Generate cliff face geometry for steep transitions.
     *
     * Checks all tile edges within the chunk for elevation deltas
     * exceeding the cliff threshold. Generates vertical quad geometry
     * with horizontally-oriented normals for toon shader shadow bands.
     *
     * @param grid The terrain grid.
     * @param chunk_x Chunk X coordinate.
     * @param chunk_y Chunk Y coordinate.
     * @param out_data Output mesh data (appends vertices and indices).
     * @return Number of cliff face quads generated.
     */
    std::uint32_t generateCliffFaces(
        const TerrainGrid& grid,
        std::uint16_t chunk_x,
        std::uint16_t chunk_y,
        ChunkMeshData& out_data
    );

    /**
     * @brief Generate a single cliff face quad.
     *
     * @param x1 Start X coordinate.
     * @param z1 Start Z coordinate.
     * @param x2 End X coordinate.
     * @param z2 End Z coordinate.
     * @param high_elevation Higher elevation value.
     * @param low_elevation Lower elevation value.
     * @param terrain_type Terrain type for the cliff face.
     * @param normal_x Cliff face normal X component.
     * @param normal_z Cliff face normal Z component.
     * @param out_data Output mesh data (appends vertices and indices).
     */
    void generateCliffFaceQuad(
        float x1, float z1,
        float x2, float z2,
        std::uint8_t high_elevation,
        std::uint8_t low_elevation,
        std::uint8_t terrain_type,
        float normal_x, float normal_z,
        ChunkMeshData& out_data
    );

    /**
     * @brief Get the linear index for a vertex in the surface grid.
     *
     * @param local_x Local X coordinate within chunk (0 to TILES_PER_CHUNK).
     * @param local_y Local Y coordinate within chunk (0 to TILES_PER_CHUNK).
     * @return Linear index into vertex array.
     */
    std::uint32_t getSurfaceVertexIndex(std::uint16_t local_x, std::uint16_t local_y) const {
        return static_cast<std::uint32_t>(local_y) * (TILES_PER_CHUNK + 1) + local_x;
    }

    // =========================================================================
    // LOD Mesh Generation Helpers
    // =========================================================================

    /**
     * @brief Generate terrain surface vertices for a chunk at a specific LOD level.
     *
     * Creates vertices at subsampled tile corners based on LOD step.
     *
     * @param grid The terrain grid.
     * @param chunk_x Chunk X coordinate.
     * @param chunk_y Chunk Y coordinate.
     * @param lod_level LOD level (0, 1, or 2).
     * @param out_data Output mesh data.
     */
    void generateLODSurfaceVertices(
        const TerrainGrid& grid,
        std::uint16_t chunk_x,
        std::uint16_t chunk_y,
        std::uint8_t lod_level,
        ChunkMeshData& out_data
    );

    /**
     * @brief Generate surface indices for a chunk at a specific LOD level.
     *
     * Creates triangle indices for the subsampled terrain surface quads.
     *
     * @param lod_level LOD level (0, 1, or 2).
     * @param out_data Output mesh data (appends indices).
     */
    void generateLODSurfaceIndices(std::uint8_t lod_level, ChunkMeshData& out_data);

    /**
     * @brief Get the linear index for a vertex in an LOD surface grid.
     *
     * @param local_x Local X coordinate within LOD grid.
     * @param local_y Local Y coordinate within LOD grid.
     * @param grid_size Number of vertices per side for this LOD level.
     * @return Linear index into vertex array.
     */
    std::uint32_t getLODSurfaceVertexIndex(
        std::uint16_t local_x,
        std::uint16_t local_y,
        std::uint32_t grid_size
    ) const {
        return static_cast<std::uint32_t>(local_y) * grid_size + local_x;
    }

    // =========================================================================
    // Skirt Geometry Helpers (Ticket 3-033 - LOD Seam Mitigation)
    // =========================================================================

    /**
     * @brief Generate skirt geometry for all 4 edges of a chunk at a specific LOD level.
     *
     * Skirt geometry extends edge vertices downward by a configurable height to
     * hide gaps at LOD transitions. Each edge generates:
     * - grid_size skirt vertices (duplicates of edge vertices, extended downward)
     * - (grid_size - 1) quads connecting original edge to skirt edge
     *
     * The skirt uses the same normals as the surface vertices (pointing outward/up)
     * to ensure consistent lighting at LOD boundaries.
     *
     * @param grid The terrain grid.
     * @param chunk_x Chunk X coordinate.
     * @param chunk_y Chunk Y coordinate.
     * @param lod_level LOD level (0, 1, or 2).
     * @param skirt_height How far down to extend skirt vertices (default: 0.5 units).
     * @param out_data Output mesh data (appends vertices and indices).
     */
    void generateLODSkirtGeometry(
        const TerrainGrid& grid,
        std::uint16_t chunk_x,
        std::uint16_t chunk_y,
        std::uint8_t lod_level,
        float skirt_height,
        ChunkMeshData& out_data
    );

    /**
     * @brief Generate skirt for a single edge of a chunk.
     *
     * Creates skirt vertices and triangles for one edge:
     * - North edge (Z min): connects vertices at local_y = 0
     * - South edge (Z max): connects vertices at local_y = grid_size - 1
     * - West edge (X min): connects vertices at local_x = 0
     * - East edge (X max): connects vertices at local_x = grid_size - 1
     *
     * @param grid The terrain grid.
     * @param chunk_x Chunk X coordinate.
     * @param chunk_y Chunk Y coordinate.
     * @param lod_level LOD level (0, 1, or 2).
     * @param edge Edge identifier (0=North, 1=East, 2=South, 3=West).
     * @param skirt_height How far down to extend skirt vertices.
     * @param out_data Output mesh data (appends vertices and indices).
     */
    void generateLODSkirtEdge(
        const TerrainGrid& grid,
        std::uint16_t chunk_x,
        std::uint16_t chunk_y,
        std::uint8_t lod_level,
        std::uint8_t edge,
        float skirt_height,
        ChunkMeshData& out_data
    );

    // =========================================================================
    // Member Variables
    // =========================================================================

    std::uint16_t map_width_;           ///< Map width in tiles
    std::uint16_t map_height_;          ///< Map height in tiles
    std::uint16_t chunks_x_;            ///< Number of chunks in X direction
    std::uint16_t chunks_y_;            ///< Number of chunks in Y direction
    std::uint8_t cliff_threshold_;      ///< Elevation delta for cliff faces
    float skirt_height_;                ///< Skirt extension height for LOD seam mitigation
    bool initialized_;                  ///< Whether generator is initialized

    /// Queue of chunk coordinates pending rebuild (pair: chunk_x, chunk_y)
    std::deque<std::pair<std::uint16_t, std::uint16_t>> rebuild_queue_;

    /// Reusable mesh data buffer (avoids allocation per chunk)
    ChunkMeshData temp_mesh_data_;
};

} // namespace terrain
} // namespace sims3000

#endif // SIMS3000_TERRAIN_TERRAINCHUNKMESHGENERATOR_H
