/**
 * @file ContaminationGrid.cpp
 * @brief Implementation of ContaminationGrid double-buffered dense 2D grid.
 *
 * @see ContaminationGrid.h for class documentation.
 * @see E10-061
 */

#include <sims3000/contamination/ContaminationGrid.h>
#include <algorithm>

namespace sims3000 {
namespace contamination {

ContaminationGrid::ContaminationGrid(uint16_t width, uint16_t height)
    : m_width(width)
    , m_height(height)
    , m_grid(static_cast<size_t>(width) * height)
    , m_previous_grid(static_cast<size_t>(width) * height)
    , m_level_cache(static_cast<size_t>(width) * height, 0)
{
}

uint16_t ContaminationGrid::get_width() const {
    return m_width;
}

uint16_t ContaminationGrid::get_height() const {
    return m_height;
}

uint8_t ContaminationGrid::get_level(int32_t x, int32_t y) const {
    if (!is_valid(x, y)) {
        return 0;
    }
    return m_grid[index(x, y)].level;
}

uint8_t ContaminationGrid::get_dominant_type(int32_t x, int32_t y) const {
    if (!is_valid(x, y)) {
        return 0;
    }
    return m_grid[index(x, y)].dominant_type;
}

void ContaminationGrid::set_level(int32_t x, int32_t y, uint8_t level) {
    if (!is_valid(x, y)) {
        return;
    }
    m_grid[index(x, y)].level = level;
    m_level_cache_dirty = true;
}

void ContaminationGrid::add_contamination(int32_t x, int32_t y, uint8_t amount, uint8_t type) {
    if (!is_valid(x, y)) {
        return;
    }
    size_t idx = index(x, y);
    ContaminationCell& cell = m_grid[idx];

    uint16_t sum = static_cast<uint16_t>(cell.level) + static_cast<uint16_t>(amount);
    cell.level = sum > 255 ? 255 : static_cast<uint8_t>(sum);

    // Update dominant type: if the cell was empty or the new contribution
    // is significant, adopt the new type
    if (cell.dominant_type == 0 || amount > 0) {
        cell.dominant_type = type;
    }

    m_level_cache_dirty = true;
}

void ContaminationGrid::apply_decay(int32_t x, int32_t y, uint8_t amount) {
    if (!is_valid(x, y)) {
        return;
    }
    size_t idx = index(x, y);
    ContaminationCell& cell = m_grid[idx];

    if (cell.level <= amount) {
        cell.level = 0;
        cell.dominant_type = 0;
    } else {
        cell.level = cell.level - amount;
    }

    m_level_cache_dirty = true;
}

uint8_t ContaminationGrid::get_level_previous_tick(int32_t x, int32_t y) const {
    if (!is_valid(x, y)) {
        return 0;
    }
    return m_previous_grid[index(x, y)].level;
}

uint8_t ContaminationGrid::get_dominant_type_previous_tick(int32_t x, int32_t y) const {
    if (!is_valid(x, y)) {
        return 0;
    }
    return m_previous_grid[index(x, y)].dominant_type;
}

void ContaminationGrid::swap_buffers() {
    std::swap(m_grid, m_previous_grid);
    m_level_cache_dirty = true;
}

uint32_t ContaminationGrid::get_total_contamination() const {
    return m_total_contamination;
}

uint32_t ContaminationGrid::get_toxic_tiles(uint8_t threshold) const {
    uint32_t count = 0;
    for (size_t i = 0; i < m_grid.size(); ++i) {
        if (m_grid[i].level >= threshold) {
            ++count;
        }
    }
    return count;
}

void ContaminationGrid::update_stats() {
    m_total_contamination = 0;
    m_toxic_tiles = 0;
    for (size_t i = 0; i < m_grid.size(); ++i) {
        m_total_contamination += m_grid[i].level;
        if (m_grid[i].level >= 128) {
            ++m_toxic_tiles;
        }
    }
}

const uint8_t* ContaminationGrid::get_level_data() const {
    if (m_level_cache_dirty) {
        for (size_t i = 0; i < m_grid.size(); ++i) {
            m_level_cache[i] = m_grid[i].level;
        }
        m_level_cache_dirty = false;
    }
    return m_level_cache.data();
}

void ContaminationGrid::clear() {
    ContaminationCell empty;
    std::fill(m_grid.begin(), m_grid.end(), empty);
    std::fill(m_previous_grid.begin(), m_previous_grid.end(), empty);
    std::fill(m_level_cache.begin(), m_level_cache.end(), static_cast<uint8_t>(0));
    m_level_cache_dirty = false;
    m_total_contamination = 0;
    m_toxic_tiles = 0;
}

bool ContaminationGrid::is_valid(int32_t x, int32_t y) const {
    return x >= 0 && y >= 0 &&
           static_cast<uint32_t>(x) < m_width &&
           static_cast<uint32_t>(y) < m_height;
}

size_t ContaminationGrid::index(int32_t x, int32_t y) const {
    return static_cast<size_t>(y) * m_width + static_cast<size_t>(x);
}

} // namespace contamination
} // namespace sims3000
