/**
 * @file PathCache.cpp
 * @brief Implementation of PathCache for frequently-queried routes (Epic 7, Ticket E7-041)
 *
 * @see PathCache.h for class documentation.
 */

#include <sims3000/transport/PathCache.h>

namespace sims3000 {
namespace transport {

PathCache::PathCache(uint32_t max_age_ticks)
    : max_age_ticks_(max_age_ticks)
{
}

const PathResult* PathCache::get(const GridPosition& start, const GridPosition& end,
                                  uint32_t current_tick) const {
    PathCacheKey key;
    key.start = start;
    key.end = end;

    auto it = cache_.find(key);
    if (it == cache_.end()) {
        return nullptr;
    }

    // Check if entry has expired
    if ((current_tick - it->second.cached_at_tick) >= max_age_ticks_) {
        return nullptr;
    }

    return &it->second.result;
}

void PathCache::put(const GridPosition& start, const GridPosition& end,
                     const PathResult& result, uint32_t current_tick) {
    PathCacheKey key;
    key.start = start;
    key.end = end;

    CachedPath entry;
    entry.result = result;
    entry.cached_at_tick = current_tick;

    cache_[key] = entry;
}

void PathCache::invalidate() {
    cache_.clear();
}

size_t PathCache::size() const {
    return cache_.size();
}

} // namespace transport
} // namespace sims3000
