/**
 * @file TerrainChunkCuller.h
 * @brief Terrain chunk frustum culling integration with Epic 2's spatial partitioning.
 *
 * Integrates terrain chunks (32x32 tile regions) with the FrustumCuller system
 * for efficient visibility determination. Each chunk maps to one spatial cell
 * in the frustum culler's grid. Only chunks passing the frustum test are
 * submitted for rendering.
 *
 * Key features:
 * - Registers terrain chunks as spatial entities in FrustumCuller
 * - Chunk AABB includes max elevation for correct vertical culling
 * - Conservative culling prevents popping at frustum edges
 * - Provides culling statistics (visible vs total chunks)
 * - Works correctly at all camera angles (preset and free)
 *
 * Performance targets:
 * - 512x512 map: 256 chunks total, 15-40 visible at typical zoom (84-94% culled)
 * - Culling overhead: < 0.1ms per frame
 *
 * Resource ownership:
 * - TerrainChunkCuller does NOT own chunks or FrustumCuller
 * - Caller retains ownership of all passed objects
 *
 * Usage:
 * @code
 *   FrustumCuller culler(512, 512);
 *   std::vector<TerrainChunk> chunks;
 *   TerrainChunkCuller chunkCuller;
 *
 *   // On map load: register all chunks
 *   chunkCuller.registerChunks(culler, chunks);
 *
 *   // Each frame: update frustum and get visible chunks
 *   culler.updateFrustum(viewProjectionMatrix);
 *   chunkCuller.updateVisibleChunks(culler, chunks);
 *
 *   // Render only visible chunks
 *   for (const TerrainChunk* chunk : chunkCuller.getVisibleChunks()) {
 *       renderChunk(*chunk);
 *   }
 * @endcode
 *
 * @see FrustumCuller for frustum culling implementation
 * @see TerrainChunk for chunk structure with AABB
 */

#ifndef SIMS3000_TERRAIN_TERRAINCHUNKCULLER_H
#define SIMS3000_TERRAIN_TERRAINCHUNKCULLER_H

#include <sims3000/terrain/TerrainChunk.h>
#include <sims3000/render/FrustumCuller.h>

#include <vector>
#include <cstdint>

namespace sims3000 {
namespace terrain {

/**
 * @struct TerrainChunkCullingStats
 * @brief Statistics about terrain chunk culling.
 *
 * Provides visibility statistics for debugging and render stats display.
 */
struct TerrainChunkCullingStats {
    std::uint32_t totalChunks = 0;      ///< Total registered chunks
    std::uint32_t visibleChunks = 0;    ///< Chunks passing frustum test
    std::uint32_t culledChunks = 0;     ///< Chunks culled (not visible)
    float cullRatio = 0.0f;             ///< Ratio of culled vs total (0-1)

    void reset() {
        totalChunks = 0;
        visibleChunks = 0;
        culledChunks = 0;
        cullRatio = 0.0f;
    }

    void compute() {
        if (totalChunks > 0) {
            cullRatio = static_cast<float>(culledChunks) /
                        static_cast<float>(totalChunks);
        } else {
            cullRatio = 0.0f;
        }
    }
};

/**
 * @class TerrainChunkCuller
 * @brief Manages terrain chunk registration and visibility queries.
 *
 * Bridges terrain chunks with the FrustumCuller spatial partitioning system.
 * Each chunk is registered as a spatial entity with its AABB for efficient
 * frustum culling.
 */
class TerrainChunkCuller {
public:
    /**
     * @brief Default constructor.
     */
    TerrainChunkCuller() = default;

    /**
     * @brief Destructor.
     */
    ~TerrainChunkCuller() = default;

    // Non-copyable (contains internal state)
    TerrainChunkCuller(const TerrainChunkCuller&) = delete;
    TerrainChunkCuller& operator=(const TerrainChunkCuller&) = delete;

    // Movable
    TerrainChunkCuller(TerrainChunkCuller&&) = default;
    TerrainChunkCuller& operator=(TerrainChunkCuller&&) = default;

    // =========================================================================
    // Chunk Registration
    // =========================================================================

    /**
     * @brief Register all terrain chunks with the frustum culler.
     *
     * Each chunk is registered as a spatial entity using its AABB and
     * center position. Call this after chunks are created and their
     * AABBs are computed.
     *
     * Entity IDs for chunks are assigned as: base_entity_id + chunk_index.
     * The base_entity_id defaults to a high value to avoid conflicts with
     * other registered entities.
     *
     * @param culler FrustumCuller to register with.
     * @param chunks Vector of terrain chunks to register.
     * @param baseEntityId Starting entity ID for chunks (default: 0x80000000).
     */
    void registerChunks(
        FrustumCuller& culler,
        const std::vector<TerrainChunk>& chunks,
        EntityID baseEntityId = 0x80000000
    );

    /**
     * @brief Register a single chunk with the frustum culler.
     *
     * @param culler FrustumCuller to register with.
     * @param chunk Chunk to register.
     * @param chunkIndex Index of the chunk (for entity ID calculation).
     * @param baseEntityId Base entity ID for chunks.
     */
    void registerChunk(
        FrustumCuller& culler,
        const TerrainChunk& chunk,
        std::uint32_t chunkIndex,
        EntityID baseEntityId = 0x80000000
    );

    /**
     * @brief Update a chunk's AABB in the frustum culler.
     *
     * Call this after a chunk's mesh is rebuilt and its AABB changes.
     *
     * @param culler FrustumCuller to update.
     * @param chunk Chunk with updated AABB.
     * @param chunkIndex Index of the chunk.
     * @param baseEntityId Base entity ID for chunks.
     */
    void updateChunkAABB(
        FrustumCuller& culler,
        const TerrainChunk& chunk,
        std::uint32_t chunkIndex,
        EntityID baseEntityId = 0x80000000
    );

    /**
     * @brief Unregister all chunks from the frustum culler.
     *
     * @param culler FrustumCuller to unregister from.
     * @param chunkCount Number of chunks to unregister.
     * @param baseEntityId Base entity ID used during registration.
     */
    void unregisterChunks(
        FrustumCuller& culler,
        std::uint32_t chunkCount,
        EntityID baseEntityId = 0x80000000
    );

    // =========================================================================
    // Visibility Testing
    // =========================================================================

    /**
     * @brief Update the list of visible chunks.
     *
     * Tests each chunk's AABB against the current frustum and builds
     * a list of pointers to visible chunks. Also updates culling statistics.
     *
     * Must be called after FrustumCuller::updateFrustum().
     *
     * @param culler FrustumCuller with updated frustum.
     * @param chunks Vector of terrain chunks to test.
     */
    void updateVisibleChunks(
        const FrustumCuller& culler,
        const std::vector<TerrainChunk>& chunks
    );

    /**
     * @brief Test if a single chunk is visible.
     *
     * @param culler FrustumCuller with updated frustum.
     * @param chunk Chunk to test.
     * @return true if chunk is potentially visible (inside or intersects frustum).
     */
    bool isChunkVisible(
        const FrustumCuller& culler,
        const TerrainChunk& chunk
    ) const;

    /**
     * @brief Get the list of visible chunks.
     *
     * Only valid after updateVisibleChunks() is called.
     *
     * @return Vector of pointers to visible chunks.
     */
    const std::vector<const TerrainChunk*>& getVisibleChunks() const {
        return m_visibleChunks;
    }

    /**
     * @brief Get visible chunk count.
     * @return Number of visible chunks.
     */
    std::size_t getVisibleChunkCount() const {
        return m_visibleChunks.size();
    }

    // =========================================================================
    // Statistics
    // =========================================================================

    /**
     * @brief Get culling statistics.
     * @return Culling statistics from last updateVisibleChunks() call.
     */
    const TerrainChunkCullingStats& getStats() const {
        return m_stats;
    }

    /**
     * @brief Reset statistics.
     */
    void resetStats() {
        m_stats.reset();
    }

private:
    /// List of visible chunk pointers (updated each frame)
    std::vector<const TerrainChunk*> m_visibleChunks;

    /// Culling statistics
    TerrainChunkCullingStats m_stats;

    /// Flag indicating if chunks have been registered
    bool m_registered = false;
};

// ============================================================================
// Utility Functions
// ============================================================================

/**
 * @brief Compute entity ID for a terrain chunk.
 *
 * @param chunkIndex Index of the chunk in the chunk array.
 * @param baseEntityId Base entity ID for terrain chunks.
 * @return Entity ID for the chunk.
 */
inline EntityID computeChunkEntityId(
    std::uint32_t chunkIndex,
    EntityID baseEntityId = 0x80000000
) {
    return baseEntityId + static_cast<EntityID>(chunkIndex);
}

/**
 * @brief Compute chunk center position for spatial grid placement.
 *
 * @param chunk The terrain chunk.
 * @return Center position in world coordinates.
 */
inline glm::vec3 computeChunkCenterPosition(const TerrainChunk& chunk) {
    // Center of chunk in XZ plane, Y at half of AABB height
    return chunk.aabb.center();
}

} // namespace terrain
} // namespace sims3000

#endif // SIMS3000_TERRAIN_TERRAINCHUNKCULLER_H
