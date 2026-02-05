#pragma once

#include <cstdint>
#include <vector>

// PathwayGrid: Dense 512x512 grid, 4 bytes per tile (uint32_t entity ID, 0 = no pathway)
// Provides O(1) lookup for pathway at any position.
// Part of the dense_grid_exception pattern per patterns.yaml.

class PathwayGrid {
public:
    PathwayGrid() = default;

    PathwayGrid(uint32_t width, uint32_t height)
        : width_(width), height_(height), data_(width * height, 0) {}

    void resize(uint32_t width, uint32_t height) {
        width_ = width;
        height_ = height;
        data_.assign(width * height, 0);
    }

    // O(1) lookup - unchecked for performance
    uint32_t at(int32_t x, int32_t y) const {
        return data_[y * width_ + x];
    }

    uint32_t& at(int32_t x, int32_t y) {
        return data_[y * width_ + x];
    }

    // O(1) lookup with entity ID
    uint32_t get_pathway_at(int32_t x, int32_t y) const {
        if (!in_bounds(x, y)) return 0;
        return data_[y * width_ + x];
    }

    bool has_pathway(int32_t x, int32_t y) const {
        if (!in_bounds(x, y)) return false;
        return data_[y * width_ + x] != 0;
    }

    void set_pathway(int32_t x, int32_t y, uint32_t entity_id) {
        if (in_bounds(x, y)) {
            data_[y * width_ + x] = entity_id;
        }
    }

    void clear_pathway(int32_t x, int32_t y) {
        if (in_bounds(x, y)) {
            data_[y * width_ + x] = 0;
        }
    }

    bool in_bounds(int32_t x, int32_t y) const {
        return x >= 0 && x < static_cast<int32_t>(width_) &&
               y >= 0 && y < static_cast<int32_t>(height_);
    }

    uint32_t width() const { return width_; }
    uint32_t height() const { return height_; }
    size_t tile_count() const { return data_.size(); }

    // Memory info
    size_t memory_bytes() const { return data_.size() * sizeof(uint32_t); }
    size_t bytes_per_tile() const { return sizeof(uint32_t); }

    // Raw data access for serialization
    const uint32_t* raw_data() const { return data_.data(); }
    uint32_t* raw_data() { return data_.data(); }
    size_t raw_size() const { return data_.size() * sizeof(uint32_t); }

    // Iterators for range-based for
    auto begin() { return data_.begin(); }
    auto end() { return data_.end(); }
    auto begin() const { return data_.begin(); }
    auto end() const { return data_.end(); }

private:
    uint32_t width_ = 0;
    uint32_t height_ = 0;
    std::vector<uint32_t> data_;
};
