/**
 * @file PathCache.h
 * @brief PathCache for frequently-queried routes (Epic 7, Ticket E7-041)
 *
 * Caches pathfinding results (PathResult) keyed by start/end GridPosition pairs.
 * Entries expire after max_age_ticks. The entire cache is invalidated when the
 * network changes (pathways added or removed).
 *
 * Uses canonical alien terminology per /docs/canon/terminology.yaml
 */

#pragma once

#include <sims3000/transport/Pathfinding.h>
#include <sims3000/transport/NetworkGraph.h>
#include <cstdint>
#include <unordered_map>

namespace sims3000 {
namespace transport {

/**
 * @struct PathCacheKey
 * @brief Key for cached path lookups (start + end positions).
 */
struct PathCacheKey {
    GridPosition start;
    GridPosition end;

    bool operator==(const PathCacheKey& other) const {
        return start == other.start && end == other.end;
    }
};

/**
 * @struct PathCacheKeyHash
 * @brief Hash functor for PathCacheKey.
 *
 * Combines hashes of start and end positions using XOR + shift.
 */
struct PathCacheKeyHash {
    size_t operator()(const PathCacheKey& key) const {
        auto h1 = GridPositionHash{}(key.start);
        auto h2 = GridPositionHash{}(key.end);
        return h1 ^ (h2 << 1);
    }
};

/**
 * @struct CachedPath
 * @brief A cached pathfinding result with timestamp.
 */
struct CachedPath {
    PathResult result;           ///< The cached pathfinding result
    uint32_t cached_at_tick = 0; ///< Tick when this result was cached
};

/**
 * @class PathCache
 * @brief Cache for frequently-queried pathfinding routes.
 *
 * Stores PathResult entries keyed by (start, end) positions.
 * Entries are valid for max_age_ticks before being considered stale.
 * The entire cache is invalidated when the network topology changes.
 */
class PathCache {
public:
    /**
     * @brief Construct PathCache with configurable max age.
     *
     * @param max_age_ticks Maximum age of cached entries in ticks (default 100).
     */
    explicit PathCache(uint32_t max_age_ticks = 100);

    /**
     * @brief Look up a cached path result.
     *
     * Returns nullptr if the entry does not exist or has expired
     * (current_tick - cached_at_tick >= max_age_ticks).
     *
     * @param start Starting grid position.
     * @param end Ending grid position.
     * @param current_tick Current simulation tick for age checking.
     * @return Pointer to cached PathResult, or nullptr if not found/expired.
     */
    const PathResult* get(const GridPosition& start, const GridPosition& end,
                          uint32_t current_tick) const;

    /**
     * @brief Store a path result in the cache.
     *
     * Overwrites any existing entry for the same key.
     *
     * @param start Starting grid position.
     * @param end Ending grid position.
     * @param result The PathResult to cache.
     * @param current_tick Current simulation tick (used as timestamp).
     */
    void put(const GridPosition& start, const GridPosition& end,
             const PathResult& result, uint32_t current_tick);

    /**
     * @brief Invalidate all cached paths.
     *
     * Should be called when the network topology changes (pathways
     * added or removed), as cached routes may no longer be valid.
     */
    void invalidate();

    /**
     * @brief Get the number of entries currently in the cache.
     * @return Number of cached entries.
     */
    size_t size() const;

private:
    std::unordered_map<PathCacheKey, CachedPath, PathCacheKeyHash> cache_;
    uint32_t max_age_ticks_;
};

} // namespace transport
} // namespace sims3000
