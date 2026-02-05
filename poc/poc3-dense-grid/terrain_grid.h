#pragma once

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <vector>

// 4-byte terrain tile - matches Epic 3 spec exactly
struct TerrainComponent {
    uint8_t terrain_type; // TerrainType enum value
    uint8_t elevation;    // 0-255 height levels
    uint8_t moisture;     // 0-255 moisture level
    uint8_t flags;        // Bitfield: buildable, has_road, has_water, etc.
};

static_assert(sizeof(TerrainComponent) == 4, "TerrainComponent must be exactly 4 bytes");

// Dense 2D grid - row-major layout for cache-friendly iteration
class TerrainGrid {
public:
    TerrainGrid() : width_(0), height_(0) {}

    TerrainGrid(uint32_t width, uint32_t height)
        : width_(width), height_(height), data_(width * height) {}

    void resize(uint32_t width, uint32_t height) {
        width_ = width;
        height_ = height;
        data_.resize(width * height);
    }

    // O(1) coordinate lookup
    TerrainComponent& at(uint32_t x, uint32_t y) {
        return data_[y * width_ + x];
    }

    const TerrainComponent& at(uint32_t x, uint32_t y) const {
        return data_[y * width_ + x];
    }

    // Bounds check
    bool in_bounds(int32_t x, int32_t y) const {
        return x >= 0 && x < static_cast<int32_t>(width_) &&
               y >= 0 && y < static_cast<int32_t>(height_);
    }

    // Raw data access for serialization
    const uint8_t* raw_data() const {
        return reinterpret_cast<const uint8_t*>(data_.data());
    }

    uint8_t* raw_data() {
        return reinterpret_cast<uint8_t*>(data_.data());
    }

    size_t raw_size() const {
        return data_.size() * sizeof(TerrainComponent);
    }

    // Iterators for range-based for loops
    TerrainComponent* begin() { return data_.data(); }
    TerrainComponent* end() { return data_.data() + data_.size(); }
    const TerrainComponent* begin() const { return data_.data(); }
    const TerrainComponent* end() const { return data_.data() + data_.size(); }

    uint32_t width() const { return width_; }
    uint32_t height() const { return height_; }
    size_t tile_count() const { return data_.size(); }

    // Memory footprint of grid data only
    size_t memory_bytes() const {
        return data_.capacity() * sizeof(TerrainComponent);
    }

private:
    uint32_t width_;
    uint32_t height_;
    std::vector<TerrainComponent> data_;
};
