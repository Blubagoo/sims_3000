/**
 * @file ChunkDirtyTracker.cpp
 * @brief Implementation of ChunkDirtyTracker
 */

#include <sims3000/terrain/ChunkDirtyTracker.h>
#include <algorithm>

namespace sims3000 {
namespace terrain {

ChunkDirtyTracker::ChunkDirtyTracker(std::uint16_t map_width, std::uint16_t map_height) {
    initialize(map_width, map_height);
}

void ChunkDirtyTracker::initialize(std::uint16_t map_width, std::uint16_t map_height) {
    map_width_ = map_width;
    map_height_ = map_height;

    // Calculate chunk grid dimensions (ceiling division)
    // e.g., 512/32 = 16 chunks, 100/32 = 4 chunks (rounded up)
    chunks_x_ = (map_width + CHUNK_SIZE - 1) / CHUNK_SIZE;
    chunks_y_ = (map_height + CHUNK_SIZE - 1) / CHUNK_SIZE;

    // Allocate and clear dirty flags
    std::size_t total_chunks = static_cast<std::size_t>(chunks_x_) * chunks_y_;
    dirty_flags_.assign(total_chunks, false);
    dirty_count_ = 0;
}

bool ChunkDirtyTracker::markChunkDirty(std::uint16_t chunk_x, std::uint16_t chunk_y) {
    if (!isValidChunk(chunk_x, chunk_y)) {
        return false;
    }

    std::size_t index = getChunkIndex(chunk_x, chunk_y);
    if (!dirty_flags_[index]) {
        dirty_flags_[index] = true;
        ++dirty_count_;
    }
    return true;
}

bool ChunkDirtyTracker::isChunkDirty(std::uint16_t chunk_x, std::uint16_t chunk_y) const {
    if (!isValidChunk(chunk_x, chunk_y)) {
        return false;
    }

    return dirty_flags_[getChunkIndex(chunk_x, chunk_y)];
}

bool ChunkDirtyTracker::clearChunkDirty(std::uint16_t chunk_x, std::uint16_t chunk_y) {
    if (!isValidChunk(chunk_x, chunk_y)) {
        return false;
    }

    std::size_t index = getChunkIndex(chunk_x, chunk_y);
    if (dirty_flags_[index]) {
        dirty_flags_[index] = false;
        --dirty_count_;
    }
    return true;
}

bool ChunkDirtyTracker::markTileDirty(std::int16_t tile_x, std::int16_t tile_y) {
    // Negative coordinates are out of bounds
    if (tile_x < 0 || tile_y < 0) {
        return false;
    }

    // Check bounds
    if (tile_x >= static_cast<std::int16_t>(map_width_) ||
        tile_y >= static_cast<std::int16_t>(map_height_)) {
        return false;
    }

    std::uint16_t chunk_x, chunk_y;
    tileToChunk(tile_x, tile_y, chunk_x, chunk_y);
    return markChunkDirty(chunk_x, chunk_y);
}

std::uint32_t ChunkDirtyTracker::markTilesDirty(const GridRect& rect) {
    if (rect.isEmpty()) {
        return 0;
    }

    // Clamp the rectangle to map bounds
    std::int16_t min_x = std::max(rect.x, static_cast<std::int16_t>(0));
    std::int16_t min_y = std::max(rect.y, static_cast<std::int16_t>(0));
    std::int16_t max_x = std::min(rect.right(), static_cast<std::int16_t>(map_width_));
    std::int16_t max_y = std::min(rect.bottom(), static_cast<std::int16_t>(map_height_));

    // Check if rectangle is entirely out of bounds
    if (min_x >= max_x || min_y >= max_y) {
        return 0;
    }

    // Convert tile bounds to chunk bounds
    std::uint16_t chunk_min_x = static_cast<std::uint16_t>(min_x / CHUNK_SIZE);
    std::uint16_t chunk_min_y = static_cast<std::uint16_t>(min_y / CHUNK_SIZE);
    std::uint16_t chunk_max_x = static_cast<std::uint16_t>((max_x - 1) / CHUNK_SIZE);
    std::uint16_t chunk_max_y = static_cast<std::uint16_t>((max_y - 1) / CHUNK_SIZE);

    // Mark all chunks in the range
    std::uint32_t marked_count = 0;
    for (std::uint16_t cy = chunk_min_y; cy <= chunk_max_y; ++cy) {
        for (std::uint16_t cx = chunk_min_x; cx <= chunk_max_x; ++cx) {
            std::size_t index = getChunkIndex(cx, cy);
            if (!dirty_flags_[index]) {
                dirty_flags_[index] = true;
                ++dirty_count_;
                ++marked_count;
            }
        }
    }

    return marked_count;
}

std::uint32_t ChunkDirtyTracker::processEvent(const TerrainModifiedEvent& event) {
    return markTilesDirty(event.affected_area);
}

void ChunkDirtyTracker::tileToChunk(std::int16_t tile_x, std::int16_t tile_y,
                                     std::uint16_t& chunk_x, std::uint16_t& chunk_y) {
    // Assumes non-negative tile coordinates
    // Caller should validate bounds before calling
    chunk_x = static_cast<std::uint16_t>(tile_x) / CHUNK_SIZE;
    chunk_y = static_cast<std::uint16_t>(tile_y) / CHUNK_SIZE;
}

void ChunkDirtyTracker::markAllDirty() {
    std::fill(dirty_flags_.begin(), dirty_flags_.end(), true);
    dirty_count_ = static_cast<std::uint32_t>(dirty_flags_.size());
}

void ChunkDirtyTracker::clearAllDirty() {
    std::fill(dirty_flags_.begin(), dirty_flags_.end(), false);
    dirty_count_ = 0;
}

bool ChunkDirtyTracker::hasAnyDirty() const {
    return dirty_count_ > 0;
}

std::uint32_t ChunkDirtyTracker::countDirty() const {
    return dirty_count_;
}

bool ChunkDirtyTracker::getNextDirty(std::uint16_t& chunk_x, std::uint16_t& chunk_y) const {
    if (dirty_count_ == 0) {
        return false;
    }

    // Linear scan in row-major order
    for (std::uint16_t cy = 0; cy < chunks_y_; ++cy) {
        for (std::uint16_t cx = 0; cx < chunks_x_; ++cx) {
            if (dirty_flags_[getChunkIndex(cx, cy)]) {
                chunk_x = cx;
                chunk_y = cy;
                return true;
            }
        }
    }

    return false;  // Should not reach here if dirty_count_ > 0
}

std::size_t ChunkDirtyTracker::getChunkIndex(std::uint16_t chunk_x,
                                              std::uint16_t chunk_y) const {
    return static_cast<std::size_t>(chunk_y) * chunks_x_ + chunk_x;
}

bool ChunkDirtyTracker::isValidChunk(std::uint16_t chunk_x, std::uint16_t chunk_y) const {
    return chunk_x < chunks_x_ && chunk_y < chunks_y_;
}

} // namespace terrain
} // namespace sims3000
