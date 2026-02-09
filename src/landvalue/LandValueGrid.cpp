/**
 * @file LandValueGrid.cpp
 * @brief Implementation of LandValueGrid dense 2D grid.
 *
 * @see LandValueGrid.h for class documentation.
 * @see E10-062
 */

#include <sims3000/landvalue/LandValueGrid.h>
#include <algorithm>

namespace sims3000 {
namespace landvalue {

LandValueGrid::LandValueGrid(uint16_t width, uint16_t height)
    : m_width(width)
    , m_height(height)
    , m_grid(static_cast<size_t>(width) * height)
    , m_value_cache(static_cast<size_t>(width) * height, 128)
{
    // Default LandValueCell constructor sets total_value=128, terrain_bonus=0
}

uint16_t LandValueGrid::get_width() const {
    return m_width;
}

uint16_t LandValueGrid::get_height() const {
    return m_height;
}

uint8_t LandValueGrid::get_value(int32_t x, int32_t y) const {
    if (!is_valid(x, y)) {
        return 0;
    }
    return m_grid[index(x, y)].total_value;
}

uint8_t LandValueGrid::get_terrain_bonus(int32_t x, int32_t y) const {
    if (!is_valid(x, y)) {
        return 0;
    }
    return m_grid[index(x, y)].terrain_bonus;
}

void LandValueGrid::set_value(int32_t x, int32_t y, uint8_t value) {
    if (!is_valid(x, y)) {
        return;
    }
    m_grid[index(x, y)].total_value = value;
    m_value_cache_dirty = true;
}

void LandValueGrid::subtract_value(int32_t x, int32_t y, uint8_t amount) {
    if (!is_valid(x, y)) {
        return;
    }
    uint8_t& current = m_grid[index(x, y)].total_value;
    if (amount >= current) {
        current = 0;
    } else {
        current -= amount;
    }
    m_value_cache_dirty = true;
}

void LandValueGrid::set_terrain_bonus(int32_t x, int32_t y, uint8_t bonus) {
    if (!is_valid(x, y)) {
        return;
    }
    m_grid[index(x, y)].terrain_bonus = bonus;
}

void LandValueGrid::reset_values() {
    for (size_t i = 0; i < m_grid.size(); ++i) {
        m_grid[i].total_value = 128;
    }
    m_value_cache_dirty = true;
}

const uint8_t* LandValueGrid::get_value_data() const {
    if (m_value_cache_dirty) {
        for (size_t i = 0; i < m_grid.size(); ++i) {
            m_value_cache[i] = m_grid[i].total_value;
        }
        m_value_cache_dirty = false;
    }
    return m_value_cache.data();
}

void LandValueGrid::clear() {
    LandValueCell default_cell;
    std::fill(m_grid.begin(), m_grid.end(), default_cell);
    std::fill(m_value_cache.begin(), m_value_cache.end(), static_cast<uint8_t>(128));
    m_value_cache_dirty = false;
}

bool LandValueGrid::is_valid(int32_t x, int32_t y) const {
    return x >= 0 && y >= 0 &&
           static_cast<uint32_t>(x) < m_width &&
           static_cast<uint32_t>(y) < m_height;
}

size_t LandValueGrid::index(int32_t x, int32_t y) const {
    return static_cast<size_t>(y) * m_width + static_cast<size_t>(x);
}

} // namespace landvalue
} // namespace sims3000
