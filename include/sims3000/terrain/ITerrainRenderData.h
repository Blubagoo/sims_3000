/**
 * @file ITerrainRenderData.h
 * @brief Interface for RenderingSystem to access terrain data for mesh generation
 *
 * ITerrainRenderData is the data contract between TerrainSystem and the terrain
 * rendering subsystem. It provides:
 * - Const reference to the full TerrainGrid for mesh generation
 * - TerrainTypeInfo lookup for emissive colors and rendering properties
 * - Dirty chunk tracking for incremental mesh rebuilds
 * - Water body queries for single-mesh-per-body water rendering
 * - Flow direction queries for directional UV scrolling on rivers
 *
 * All methods are const to ensure rendering cannot modify terrain data.
 * The chunk size is a hardcoded constant of 32x32 tiles, aligned with
 * Epic 2 spatial partitioning.
 *
 * @see /docs/canon/interfaces.yaml (terrain.ITerrainRenderData)
 * @see TerrainGrid.h for dense grid storage
 * @see TerrainTypeInfo.h for per-type rendering properties
 * @see ChunkDirtyTracker.h for dirty flag system
 * @see WaterData.h for water body and flow direction data
 */

#ifndef SIMS3000_TERRAIN_ITERRAINRENDERDATA_H
#define SIMS3000_TERRAIN_ITERRAINRENDERDATA_H

#include <sims3000/terrain/TerrainGrid.h>
#include <sims3000/terrain/TerrainTypeInfo.h>
#include <sims3000/terrain/WaterData.h>
#include <cstdint>

namespace sims3000 {
namespace terrain {

/// Chunk size in tiles for terrain rendering (32x32)
constexpr std::uint32_t TERRAIN_CHUNK_SIZE = 32;

/**
 * @interface ITerrainRenderData
 * @brief Read-only terrain data interface for RenderingSystem mesh generation.
 *
 * Abstract interface that TerrainSystem implements. Provides efficient access
 * to terrain data organized for GPU mesh building.
 *
 * Key design principles:
 * - All methods are const (read-only access)
 * - Returns const references where possible (no copies)
 * - Chunk-based dirty tracking for incremental updates
 * - Thread-safe for read access during render
 *
 * Chunk organization:
 * - Terrain is divided into 32x32 tile chunks
 * - Chunks are numbered (0,0) to (chunks_x-1, chunks_y-1)
 * - A 256x256 map has 8x8 = 64 chunks
 * - A 512x512 map has 16x16 = 256 chunks
 */
class ITerrainRenderData {
public:
    /// Virtual destructor for proper polymorphic cleanup
    virtual ~ITerrainRenderData() = default;

    // =========================================================================
    // Grid Access - Full terrain data for mesh generation
    // =========================================================================

    /**
     * @brief Get const reference to the full terrain grid.
     *
     * Provides direct access to terrain tile data for mesh generation.
     * The returned reference is valid for the lifetime of the TerrainSystem.
     *
     * @return Const reference to the TerrainGrid.
     */
    virtual const TerrainGrid& get_grid() const = 0;

    // =========================================================================
    // Type Info Lookup - Per-type rendering properties
    // =========================================================================

    /**
     * @brief Get rendering properties for a terrain type.
     *
     * Returns static info including emissive color and intensity.
     * Used by shaders for terrain glow effects.
     *
     * @param type The terrain type to look up.
     * @return Const reference to TerrainTypeInfo for the type.
     */
    virtual const TerrainTypeInfo& get_type_info(TerrainType type) const = 0;

    // =========================================================================
    // Dirty Chunk Tracking - Incremental mesh rebuilds
    // =========================================================================

    /**
     * @brief Check if a chunk needs its render data rebuilt.
     *
     * Returns true if terrain within the chunk has been modified since
     * the last call to clear_chunk_dirty() for this chunk.
     *
     * @param chunk_x Chunk X coordinate (0 to chunks_x - 1).
     * @param chunk_y Chunk Y coordinate (0 to chunks_y - 1).
     * @return true if the chunk is dirty and needs rebuild.
     *         Returns false for out-of-bounds coordinates.
     */
    virtual bool is_chunk_dirty(std::uint32_t chunk_x, std::uint32_t chunk_y) const = 0;

    /**
     * @brief Clear the dirty flag for a chunk after rebuilding.
     *
     * Called by RenderingSystem after successfully rebuilding a chunk's
     * mesh data. This prevents redundant rebuilds on subsequent frames.
     *
     * @param chunk_x Chunk X coordinate (0 to chunks_x - 1).
     * @param chunk_y Chunk Y coordinate (0 to chunks_y - 1).
     *
     * @note This is the ONLY non-const method. It's logically const
     *       (doesn't change terrain data), but modifies internal tracking.
     */
    virtual void clear_chunk_dirty(std::uint32_t chunk_x, std::uint32_t chunk_y) = 0;

    /**
     * @brief Get the chunk size in tiles.
     *
     * Returns 32 (TERRAIN_CHUNK_SIZE constant).
     * Provided as a method for interface completeness.
     *
     * @return 32 (tiles per chunk in each dimension).
     */
    virtual std::uint32_t get_chunk_size() const = 0;

    // =========================================================================
    // Water Body Queries - Single-mesh-per-body water rendering
    // =========================================================================

    /**
     * @brief Get the water body ID for a tile position.
     *
     * Each contiguous water region has a unique ID (1-65535).
     * Tiles not in any water body return NO_WATER_BODY (0).
     *
     * Water body IDs enable rendering all tiles of a body as a single mesh.
     *
     * @param x Tile X coordinate.
     * @param y Tile Y coordinate.
     * @return Water body ID (0 = not in water body).
     *         Returns 0 for out-of-bounds coordinates.
     */
    virtual WaterBodyID get_water_body_id(std::int32_t x, std::int32_t y) const = 0;

    // =========================================================================
    // Flow Direction Queries - River UV scrolling
    // =========================================================================

    /**
     * @brief Get the flow direction for a tile position.
     *
     * Flow direction indicates which way water flows FROM this tile.
     * Used for directional UV scrolling on FlowChannel (river) tiles.
     *
     * Non-river tiles and still water return FlowDirection::None.
     *
     * @param x Tile X coordinate.
     * @param y Tile Y coordinate.
     * @return Flow direction (None for non-river tiles).
     *         Returns FlowDirection::None for out-of-bounds coordinates.
     */
    virtual FlowDirection get_flow_direction(std::int32_t x, std::int32_t y) const = 0;

    // =========================================================================
    // Map Metadata - Chunk grid dimensions
    // =========================================================================

    /**
     * @brief Get map width in tiles.
     * @return Map width (128, 256, or 512).
     */
    virtual std::uint32_t get_map_width() const = 0;

    /**
     * @brief Get map height in tiles.
     * @return Map height (128, 256, or 512).
     */
    virtual std::uint32_t get_map_height() const = 0;

    /**
     * @brief Get number of chunks in X direction.
     * @return Number of chunks horizontally (map_width / 32).
     */
    virtual std::uint32_t get_chunks_x() const = 0;

    /**
     * @brief Get number of chunks in Y direction.
     * @return Number of chunks vertically (map_height / 32).
     */
    virtual std::uint32_t get_chunks_y() const = 0;

protected:
    /// Protected default constructor (interface cannot be instantiated directly)
    ITerrainRenderData() = default;

    /// Non-copyable (interfaces should be used via pointers/references)
    ITerrainRenderData(const ITerrainRenderData&) = delete;
    ITerrainRenderData& operator=(const ITerrainRenderData&) = delete;

    /// Movable for flexibility
    ITerrainRenderData(ITerrainRenderData&&) = default;
    ITerrainRenderData& operator=(ITerrainRenderData&&) = default;
};

} // namespace terrain
} // namespace sims3000

#endif // SIMS3000_TERRAIN_ITERRAINRENDERDATA_H
