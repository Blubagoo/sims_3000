/**
 * @file ChunkDirtyTracker.h
 * @brief Per-chunk dirty flag tracking for terrain rendering optimization
 *
 * Tracks which 32x32 tile chunks have been modified and need their
 * render data rebuilt. Supports both event-based notification (primary)
 * and dirty flag polling (fallback) as specified by ITerrainRenderData.
 *
 * Chunk size is 32x32 tiles to align with Epic 2 spatial partitioning.
 * The chunk grid size is derived from the map dimensions.
 *
 * Usage:
 * 1. Initialize with map dimensions
 * 2. Call markChunkDirty() or markTilesDirty() when terrain changes
 * 3. RenderingSystem polls isChunkDirty() each frame
 * 4. After rebuilding a chunk, call clearChunkDirty()
 *
 * @see TerrainModifiedEvent for event-based notification
 */

#ifndef SIMS3000_TERRAIN_CHUNKDIRTYTRACKER_H
#define SIMS3000_TERRAIN_CHUNKDIRTYTRACKER_H

#include <sims3000/terrain/TerrainEvents.h>
#include <cstdint>
#include <vector>
#include <utility>

namespace sims3000 {
namespace terrain {

/// Chunk size in tiles (32x32 tiles per chunk)
constexpr std::uint16_t CHUNK_SIZE = 32;

/**
 * @class ChunkDirtyTracker
 * @brief Manages per-chunk dirty flags for terrain rendering optimization.
 *
 * The terrain is divided into 32x32 tile chunks. When terrain is modified,
 * the affected chunks are marked dirty. The rendering system rebuilds
 * at most one dirty chunk per frame to avoid GPU stalls.
 *
 * Dirty flags enable incremental terrain mesh updates:
 * - Modified chunks are flagged for rebuild
 * - Unmodified chunks retain their cached render data
 * - Clearing a flag after rebuild prevents redundant updates
 */
class ChunkDirtyTracker {
public:
    /**
     * @brief Default constructor (no chunks, must call initialize()).
     */
    ChunkDirtyTracker() = default;

    /**
     * @brief Construct and initialize with map dimensions.
     * @param map_width Map width in tiles.
     * @param map_height Map height in tiles.
     */
    ChunkDirtyTracker(std::uint16_t map_width, std::uint16_t map_height);

    /**
     * @brief Initialize or reinitialize with new map dimensions.
     *
     * Clears all existing dirty flags and resizes the chunk grid.
     * All chunks start as NOT dirty after initialization.
     *
     * @param map_width Map width in tiles.
     * @param map_height Map height in tiles.
     */
    void initialize(std::uint16_t map_width, std::uint16_t map_height);

    // =========================================================================
    // Core Dirty Flag Operations
    // =========================================================================

    /**
     * @brief Mark a specific chunk as dirty.
     * @param chunk_x Chunk X coordinate (0 to chunks_x - 1).
     * @param chunk_y Chunk Y coordinate (0 to chunks_y - 1).
     * @return true if the chunk was valid and marked, false if out of bounds.
     */
    bool markChunkDirty(std::uint16_t chunk_x, std::uint16_t chunk_y);

    /**
     * @brief Query if a specific chunk is dirty.
     * @param chunk_x Chunk X coordinate (0 to chunks_x - 1).
     * @param chunk_y Chunk Y coordinate (0 to chunks_y - 1).
     * @return true if the chunk is dirty, false if clean or out of bounds.
     */
    bool isChunkDirty(std::uint16_t chunk_x, std::uint16_t chunk_y) const;

    /**
     * @brief Clear the dirty flag for a specific chunk.
     *
     * Called by RenderingSystem after rebuilding a chunk's render data.
     *
     * @param chunk_x Chunk X coordinate (0 to chunks_x - 1).
     * @param chunk_y Chunk Y coordinate (0 to chunks_y - 1).
     * @return true if the chunk was valid and cleared, false if out of bounds.
     */
    bool clearChunkDirty(std::uint16_t chunk_x, std::uint16_t chunk_y);

    // =========================================================================
    // Tile-to-Chunk Operations
    // =========================================================================

    /**
     * @brief Mark the chunk containing a specific tile as dirty.
     * @param tile_x Tile X coordinate.
     * @param tile_y Tile Y coordinate.
     * @return true if the tile mapped to a valid chunk, false otherwise.
     */
    bool markTileDirty(std::int16_t tile_x, std::int16_t tile_y);

    /**
     * @brief Mark all chunks overlapping a rectangular tile region as dirty.
     *
     * This is the primary method called when processing TerrainModifiedEvent.
     *
     * @param rect The affected tile region.
     * @return Number of chunks that were marked dirty.
     */
    std::uint32_t markTilesDirty(const GridRect& rect);

    /**
     * @brief Process a TerrainModifiedEvent and mark affected chunks dirty.
     *
     * Convenience method that extracts the affected_area from the event
     * and marks all overlapping chunks as dirty.
     *
     * @param event The terrain modification event.
     * @return Number of chunks that were marked dirty.
     */
    std::uint32_t processEvent(const TerrainModifiedEvent& event);

    // =========================================================================
    // Coordinate Conversion
    // =========================================================================

    /**
     * @brief Convert tile coordinates to chunk coordinates.
     * @param tile_x Tile X coordinate.
     * @param tile_y Tile Y coordinate.
     * @param chunk_x Output: Chunk X coordinate.
     * @param chunk_y Output: Chunk Y coordinate.
     */
    static void tileToChunk(std::int16_t tile_x, std::int16_t tile_y,
                            std::uint16_t& chunk_x, std::uint16_t& chunk_y);

    // =========================================================================
    // Bulk Operations
    // =========================================================================

    /**
     * @brief Mark all chunks as dirty.
     *
     * Used when the entire terrain needs rebuilding (e.g., after map load).
     */
    void markAllDirty();

    /**
     * @brief Clear all dirty flags.
     *
     * Used after a full terrain rebuild or when resetting state.
     */
    void clearAllDirty();

    // =========================================================================
    // Query Methods
    // =========================================================================

    /**
     * @brief Check if any chunk is dirty.
     * @return true if at least one chunk is marked dirty.
     */
    bool hasAnyDirty() const;

    /**
     * @brief Count the number of dirty chunks.
     * @return Number of chunks currently marked dirty.
     */
    std::uint32_t countDirty() const;

    /**
     * @brief Get the next dirty chunk (for sequential processing).
     *
     * Returns the first dirty chunk found in row-major order.
     * Useful for processing one chunk per frame.
     *
     * @param chunk_x Output: Chunk X coordinate of dirty chunk.
     * @param chunk_y Output: Chunk Y coordinate of dirty chunk.
     * @return true if a dirty chunk was found, false if all clean.
     */
    bool getNextDirty(std::uint16_t& chunk_x, std::uint16_t& chunk_y) const;

    // =========================================================================
    // Dimension Accessors
    // =========================================================================

    /// Get the number of chunks in the X direction.
    std::uint16_t getChunksX() const { return chunks_x_; }

    /// Get the number of chunks in the Y direction.
    std::uint16_t getChunksY() const { return chunks_y_; }

    /// Get the total number of chunks.
    std::uint32_t getTotalChunks() const {
        return static_cast<std::uint32_t>(chunks_x_) * chunks_y_;
    }

    /// Get the map width in tiles.
    std::uint16_t getMapWidth() const { return map_width_; }

    /// Get the map height in tiles.
    std::uint16_t getMapHeight() const { return map_height_; }

    /// Check if the tracker has been initialized.
    bool isInitialized() const { return chunks_x_ > 0 && chunks_y_ > 0; }

private:
    /**
     * @brief Get the linear index for a chunk coordinate.
     * @param chunk_x Chunk X coordinate.
     * @param chunk_y Chunk Y coordinate.
     * @return Linear index into the dirty flags array.
     */
    std::size_t getChunkIndex(std::uint16_t chunk_x, std::uint16_t chunk_y) const;

    /**
     * @brief Check if chunk coordinates are valid.
     * @param chunk_x Chunk X coordinate.
     * @param chunk_y Chunk Y coordinate.
     * @return true if coordinates are within bounds.
     */
    bool isValidChunk(std::uint16_t chunk_x, std::uint16_t chunk_y) const;

    std::vector<bool> dirty_flags_;   ///< Per-chunk dirty flags (row-major order)
    std::uint16_t chunks_x_ = 0;      ///< Number of chunks in X direction
    std::uint16_t chunks_y_ = 0;      ///< Number of chunks in Y direction
    std::uint16_t map_width_ = 0;     ///< Map width in tiles
    std::uint16_t map_height_ = 0;    ///< Map height in tiles
    std::uint32_t dirty_count_ = 0;   ///< Cached count of dirty chunks
};

} // namespace terrain
} // namespace sims3000

#endif // SIMS3000_TERRAIN_CHUNKDIRTYTRACKER_H
