/**
 * @file TerrainModificationVisualPipeline.h
 * @brief Coordinates visual updates when terrain is modified.
 *
 * TerrainModificationVisualPipeline connects terrain modification events to
 * the rendering system, ensuring that chunk meshes, vegetation instances,
 * and water meshes are updated when terrain changes.
 *
 * Key features:
 * - Subscribes to TerrainModifiedEvent notifications
 * - Marks affected terrain chunks as dirty
 * - Queues chunks for incremental mesh rebuild (1 per frame max)
 * - Regenerates vegetation instances for modified chunks
 * - Regenerates water meshes when modifications affect water boundaries
 * - Avoids visual flickering via double-buffered update pattern
 *
 * Usage:
 * @code
 * TerrainGrid grid(MapSize::Medium);
 * WaterData waterData;
 * std::vector<TerrainChunk> chunks(64);
 * std::vector<WaterMesh> waterMeshes;
 *
 * TerrainModificationVisualPipeline pipeline;
 * pipeline.initialize(device, grid, waterData, chunks, waterMeshes, map_seed);
 *
 * // Each frame:
 * pipeline.update(device, delta_time);
 *
 * // When terrain is modified:
 * pipeline.onTerrainModified(event);
 * @endcode
 *
 * @see TerrainModifiedEvent for event data
 * @see TerrainChunkMeshGenerator for chunk mesh rebuilding
 * @see ChunkDirtyTracker for dirty flag management
 * @see VegetationPlacementGenerator for vegetation instances
 * @see WaterMeshGenerator for water mesh regeneration
 */

#ifndef SIMS3000_TERRAIN_TERRAINMODIFICATIONVISUALPIPELINE_H
#define SIMS3000_TERRAIN_TERRAINMODIFICATIONVISUALPIPELINE_H

#include <sims3000/terrain/TerrainEvents.h>
#include <sims3000/terrain/TerrainGrid.h>
#include <sims3000/terrain/TerrainChunk.h>
#include <sims3000/terrain/TerrainChunkMeshGenerator.h>
#include <sims3000/terrain/ChunkDirtyTracker.h>
#include <sims3000/terrain/WaterMesh.h>
#include <sims3000/terrain/WaterData.h>
#include <sims3000/render/VegetationInstance.h>
#include <SDL3/SDL.h>
#include <cstdint>
#include <vector>
#include <unordered_set>
#include <deque>

namespace sims3000 {
namespace terrain {

/**
 * @struct VisualUpdateStats
 * @brief Statistics for visual update operations per frame.
 */
struct VisualUpdateStats {
    std::uint32_t terrain_chunks_rebuilt;      ///< Terrain chunks rebuilt this frame
    std::uint32_t terrain_chunks_pending;      ///< Terrain chunks still pending rebuild
    std::uint32_t vegetation_chunks_updated;   ///< Vegetation chunks regenerated
    std::uint32_t vegetation_chunks_pending;   ///< Vegetation chunks still pending
    std::uint32_t water_bodies_updated;        ///< Water bodies regenerated
    std::uint32_t water_bodies_pending;        ///< Water bodies still pending
    float update_time_ms;                      ///< Total time spent in update (ms)

    VisualUpdateStats()
        : terrain_chunks_rebuilt(0)
        , terrain_chunks_pending(0)
        , vegetation_chunks_updated(0)
        , vegetation_chunks_pending(0)
        , water_bodies_updated(0)
        , water_bodies_pending(0)
        , update_time_ms(0.0f)
    {}
};

/**
 * @class TerrainModificationVisualPipeline
 * @brief Coordinates visual updates when terrain is modified.
 *
 * This class is the central coordinator for all visual updates triggered
 * by terrain modifications. It ensures:
 *
 * 1. Terrain chunk meshes are rebuilt incrementally (1 per frame)
 * 2. Vegetation instances are regenerated for modified chunks
 * 3. Water meshes are regenerated when water boundaries change
 * 4. Updates are rate-limited to avoid GPU stalls
 * 5. No visual flickering during updates
 *
 * Thread safety:
 * - All methods must be called from the main/render thread
 * - onTerrainModified() can be called from any thread (queues work)
 */
class TerrainModificationVisualPipeline {
public:
    /// Maximum terrain chunks to rebuild per frame (1 to avoid GPU stalls)
    static constexpr std::uint32_t MAX_TERRAIN_CHUNKS_PER_FRAME = 1;

    /// Maximum vegetation chunks to regenerate per frame
    static constexpr std::uint32_t MAX_VEGETATION_CHUNKS_PER_FRAME = 2;

    /// Maximum water bodies to regenerate per frame
    static constexpr std::uint32_t MAX_WATER_BODIES_PER_FRAME = 1;

    /**
     * @brief Default constructor.
     */
    TerrainModificationVisualPipeline();

    /**
     * @brief Destructor - releases any pending resources.
     */
    ~TerrainModificationVisualPipeline();

    // Non-copyable
    TerrainModificationVisualPipeline(const TerrainModificationVisualPipeline&) = delete;
    TerrainModificationVisualPipeline& operator=(const TerrainModificationVisualPipeline&) = delete;

    // Movable
    TerrainModificationVisualPipeline(TerrainModificationVisualPipeline&&) noexcept = default;
    TerrainModificationVisualPipeline& operator=(TerrainModificationVisualPipeline&&) noexcept = default;

    // =========================================================================
    // Initialization
    // =========================================================================

    /**
     * @brief Initialize the visual pipeline.
     *
     * Sets up internal state and references to terrain data structures.
     * Must be called before any other methods.
     *
     * @param device SDL GPU device for buffer operations.
     * @param grid Reference to the terrain grid (must outlive pipeline).
     * @param water_data Reference to water data (must outlive pipeline).
     * @param chunks Reference to terrain chunks vector (must outlive pipeline).
     * @param water_meshes Reference to water meshes vector (must outlive pipeline).
     * @param map_seed Map seed for deterministic vegetation generation.
     * @return true if initialization succeeded.
     */
    bool initialize(
        SDL_GPUDevice* device,
        TerrainGrid& grid,
        WaterData& water_data,
        std::vector<TerrainChunk>& chunks,
        std::vector<WaterMesh>& water_meshes,
        std::uint64_t map_seed
    );

    /**
     * @brief Check if the pipeline is initialized.
     * @return true if initialize() was called successfully.
     */
    bool isInitialized() const { return initialized_; }

    // =========================================================================
    // Event Handling
    // =========================================================================

    /**
     * @brief Handle a terrain modification event.
     *
     * Marks affected chunks as dirty and queues them for rebuild.
     * This method can be called from any thread - it queues work
     * for the main thread to process.
     *
     * @param event The terrain modification event.
     */
    void onTerrainModified(const TerrainModifiedEvent& event);

    // =========================================================================
    // Per-Frame Update
    // =========================================================================

    /**
     * @brief Process pending visual updates.
     *
     * Called once per frame to incrementally process pending updates:
     * - Rebuilds at most 1 terrain chunk mesh
     * - Regenerates vegetation for at most 2 chunks
     * - Regenerates at most 1 water body mesh
     *
     * @param device SDL GPU device for buffer operations.
     * @param delta_time Time since last frame (seconds).
     * @return Statistics about updates performed.
     */
    VisualUpdateStats update(SDL_GPUDevice* device, float delta_time);

    // =========================================================================
    // Query Methods
    // =========================================================================

    /**
     * @brief Check if there are pending updates.
     * @return true if any terrain, vegetation, or water updates are pending.
     */
    bool hasPendingUpdates() const;

    /**
     * @brief Get the number of pending terrain chunk rebuilds.
     * @return Number of chunks waiting to be rebuilt.
     */
    std::uint32_t getPendingTerrainChunks() const;

    /**
     * @brief Get the number of pending vegetation chunk updates.
     * @return Number of chunks waiting for vegetation regeneration.
     */
    std::uint32_t getPendingVegetationChunks() const;

    /**
     * @brief Get the number of pending water body updates.
     * @return Number of water bodies waiting for mesh regeneration.
     */
    std::uint32_t getPendingWaterBodies() const;

    // =========================================================================
    // Vegetation Instance Access
    // =========================================================================

    /**
     * @brief Get vegetation instances for a chunk.
     *
     * Returns cached instances if available, otherwise generates them.
     *
     * @param chunk_x Chunk X coordinate.
     * @param chunk_y Chunk Y coordinate.
     * @return Reference to vegetation instances for this chunk.
     */
    const render::ChunkInstances& getVegetationInstances(
        std::int32_t chunk_x,
        std::int32_t chunk_y
    );

    /**
     * @brief Get all vegetation instances.
     * @return Reference to the map of all chunk vegetation instances.
     */
    const std::vector<render::ChunkInstances>& getAllVegetationInstances() const {
        return vegetation_cache_;
    }

    // =========================================================================
    // Manual Control
    // =========================================================================

    /**
     * @brief Force immediate rebuild of all chunks.
     *
     * Bypasses rate limiting and rebuilds all chunks synchronously.
     * Use sparingly - this can cause frame hitches on large maps.
     *
     * @param device SDL GPU device.
     */
    void forceRebuildAll(SDL_GPUDevice* device);

    /**
     * @brief Clear all pending updates.
     *
     * Discards all pending work without processing.
     * Useful when loading a new map.
     */
    void clearPendingUpdates();

private:
    // =========================================================================
    // Internal Update Methods
    // =========================================================================

    /**
     * @brief Process pending terrain chunk mesh rebuilds.
     * @param device SDL GPU device.
     * @param stats Output statistics.
     */
    void updateTerrainChunks(SDL_GPUDevice* device, VisualUpdateStats& stats);

    /**
     * @brief Process pending vegetation chunk regeneration.
     * @param stats Output statistics.
     */
    void updateVegetation(VisualUpdateStats& stats);

    /**
     * @brief Process pending water body mesh regeneration.
     * @param device SDL GPU device.
     * @param stats Output statistics.
     */
    void updateWaterBodies(SDL_GPUDevice* device, VisualUpdateStats& stats);

    /**
     * @brief Calculate affected chunk coordinates from a GridRect.
     *
     * @param rect The affected tile region.
     * @param out_min_cx Output: Minimum chunk X coordinate.
     * @param out_min_cy Output: Minimum chunk Y coordinate.
     * @param out_max_cx Output: Maximum chunk X coordinate (inclusive).
     * @param out_max_cy Output: Maximum chunk Y coordinate (inclusive).
     */
    void getAffectedChunks(
        const GridRect& rect,
        std::uint16_t& out_min_cx,
        std::uint16_t& out_min_cy,
        std::uint16_t& out_max_cx,
        std::uint16_t& out_max_cy
    ) const;

    /**
     * @brief Check if a modification type can affect water boundaries.
     * @param type The modification type.
     * @return true if water meshes may need regeneration.
     */
    bool canAffectWater(ModificationType type) const;

    /**
     * @brief Find water bodies affected by a tile region.
     *
     * @param rect The affected tile region.
     * @return Set of affected water body IDs.
     */
    std::unordered_set<WaterBodyID> findAffectedWaterBodies(const GridRect& rect) const;

    /**
     * @brief Get linear chunk index from coordinates.
     * @param chunk_x Chunk X coordinate.
     * @param chunk_y Chunk Y coordinate.
     * @return Linear index into chunks vector.
     */
    std::uint32_t getChunkIndex(std::uint16_t chunk_x, std::uint16_t chunk_y) const;

    // =========================================================================
    // Member Variables
    // =========================================================================

    bool initialized_;  ///< Whether initialize() was called successfully

    // References to external data (not owned)
    SDL_GPUDevice* device_;           ///< GPU device for buffer operations
    TerrainGrid* grid_;               ///< Reference to terrain grid
    WaterData* water_data_;           ///< Reference to water data
    std::vector<TerrainChunk>* chunks_;        ///< Reference to terrain chunks
    std::vector<WaterMesh>* water_meshes_;     ///< Reference to water meshes

    // Internal systems
    TerrainChunkMeshGenerator mesh_generator_;  ///< Chunk mesh generator
    ChunkDirtyTracker dirty_tracker_;           ///< Dirty flag tracker
    std::uint64_t map_seed_;                    ///< Map seed for vegetation

    // Pending update queues
    std::deque<std::pair<std::uint16_t, std::uint16_t>> pending_vegetation_chunks_;
    std::deque<WaterBodyID> pending_water_bodies_;

    // Vegetation instance cache (indexed by chunk linear index)
    std::vector<render::ChunkInstances> vegetation_cache_;
    std::uint16_t chunks_x_;  ///< Number of chunks in X direction
    std::uint16_t chunks_y_;  ///< Number of chunks in Y direction
};

} // namespace terrain
} // namespace sims3000

#endif // SIMS3000_TERRAIN_TERRAINMODIFICATIONVISUALPIPELINE_H
