/**
 * @file TerrainChunkCuller.cpp
 * @brief Implementation of terrain chunk frustum culling integration.
 *
 * Registers terrain chunks with Epic 2's spatial partitioning and frustum
 * culling system. Each 32x32 chunk maps to one spatial cell. Chunk AABBs
 * include max elevation for correct vertical culling.
 */

#include <sims3000/terrain/TerrainChunkCuller.h>
#include <sims3000/core/Logger.h>

namespace sims3000 {
namespace terrain {

// ============================================================================
// Chunk Registration
// ============================================================================

void TerrainChunkCuller::registerChunks(
    FrustumCuller& culler,
    const std::vector<TerrainChunk>& chunks,
    EntityID baseEntityId
) {
    m_visibleChunks.clear();
    m_visibleChunks.reserve(chunks.size());

    for (std::uint32_t i = 0; i < chunks.size(); ++i) {
        registerChunk(culler, chunks[i], i, baseEntityId);
    }

    m_registered = true;

    LOG_DEBUG("Registered {} terrain chunks with frustum culler", chunks.size());
}

void TerrainChunkCuller::registerChunk(
    FrustumCuller& culler,
    const TerrainChunk& chunk,
    std::uint32_t chunkIndex,
    EntityID baseEntityId
) {
    EntityID entityId = computeChunkEntityId(chunkIndex, baseEntityId);
    glm::vec3 centerPos = computeChunkCenterPosition(chunk);

    culler.registerEntity(entityId, chunk.aabb, centerPos);
}

void TerrainChunkCuller::updateChunkAABB(
    FrustumCuller& culler,
    const TerrainChunk& chunk,
    std::uint32_t chunkIndex,
    EntityID baseEntityId
) {
    // Re-register with updated AABB
    // FrustumCuller::registerEntity handles update if entity already exists
    registerChunk(culler, chunk, chunkIndex, baseEntityId);
}

void TerrainChunkCuller::unregisterChunks(
    FrustumCuller& culler,
    std::uint32_t chunkCount,
    EntityID baseEntityId
) {
    for (std::uint32_t i = 0; i < chunkCount; ++i) {
        EntityID entityId = computeChunkEntityId(i, baseEntityId);
        culler.unregisterEntity(entityId);
    }

    m_visibleChunks.clear();
    m_registered = false;

    LOG_DEBUG("Unregistered {} terrain chunks from frustum culler", chunkCount);
}

// ============================================================================
// Visibility Testing
// ============================================================================

void TerrainChunkCuller::updateVisibleChunks(
    const FrustumCuller& culler,
    const std::vector<TerrainChunk>& chunks
) {
    m_visibleChunks.clear();
    m_stats.reset();

    if (chunks.empty()) {
        return;
    }

    m_stats.totalChunks = static_cast<std::uint32_t>(chunks.size());
    m_visibleChunks.reserve(chunks.size() / 4);  // Estimate ~25% visible

    for (const TerrainChunk& chunk : chunks) {
        // Only test chunks that have GPU resources
        if (!chunk.hasGPUResources()) {
            continue;
        }

        // Test chunk AABB against frustum
        // Conservative culling: isVisible returns true for Intersects and Inside
        if (culler.isVisible(chunk.aabb)) {
            m_visibleChunks.push_back(&chunk);
            m_stats.visibleChunks++;
        } else {
            m_stats.culledChunks++;
        }
    }

    m_stats.compute();
}

bool TerrainChunkCuller::isChunkVisible(
    const FrustumCuller& culler,
    const TerrainChunk& chunk
) const {
    // Conservative test: visible if intersects or fully inside
    return culler.isVisible(chunk.aabb);
}

} // namespace terrain
} // namespace sims3000
