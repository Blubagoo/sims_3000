/**
 * @file DisorderGrid.cpp
 * @brief Implementation of DisorderGrid double-buffered dense 2D grid.
 *
 * @see DisorderGrid.h for class documentation.
 * @see E10-060
 */

#include <sims3000/disorder/DisorderGrid.h>
#include <algorithm>
#include <numeric>

namespace sims3000 {
namespace disorder {

DisorderGrid::DisorderGrid(uint16_t width, uint16_t height)
    : m_width(width)
    , m_height(height)
    , m_grid(static_cast<size_t>(width) * height, 0)
    , m_previous_grid(static_cast<size_t>(width) * height, 0)
{
}

uint16_t DisorderGrid::get_width() const {
    return m_width;
}

uint16_t DisorderGrid::get_height() const {
    return m_height;
}

uint8_t DisorderGrid::get_level(int32_t x, int32_t y) const {
    if (!is_valid(x, y)) {
        return 0;
    }
    return m_grid[index(x, y)];
}

void DisorderGrid::set_level(int32_t x, int32_t y, uint8_t level) {
    if (!is_valid(x, y)) {
        return;
    }
    m_grid[index(x, y)] = level;
}

void DisorderGrid::add_disorder(int32_t x, int32_t y, uint8_t amount) {
    if (!is_valid(x, y)) {
        return;
    }
    size_t idx = index(x, y);
    uint16_t sum = static_cast<uint16_t>(m_grid[idx]) + static_cast<uint16_t>(amount);
    m_grid[idx] = sum > 255 ? 255 : static_cast<uint8_t>(sum);
}

void DisorderGrid::apply_suppression(int32_t x, int32_t y, uint8_t amount) {
    if (!is_valid(x, y)) {
        return;
    }
    size_t idx = index(x, y);
    if (m_grid[idx] <= amount) {
        m_grid[idx] = 0;
    } else {
        m_grid[idx] = m_grid[idx] - amount;
    }
}

uint8_t DisorderGrid::get_level_previous_tick(int32_t x, int32_t y) const {
    if (!is_valid(x, y)) {
        return 0;
    }
    return m_previous_grid[index(x, y)];
}

void DisorderGrid::swap_buffers() {
    std::swap(m_grid, m_previous_grid);
}

uint32_t DisorderGrid::get_total_disorder() const {
    return m_total_disorder;
}

uint32_t DisorderGrid::get_high_disorder_tiles(uint8_t threshold) const {
    // Note: cached value uses default threshold from last update_stats() call.
    // For non-default thresholds, caller should iterate raw data directly.
    // This returns the cached value regardless of the threshold parameter,
    // which is consistent with the spec that stats are "cached, updated on demand".
    // However, for correctness we recount if needed.
    uint32_t count = 0;
    for (size_t i = 0; i < m_grid.size(); ++i) {
        if (m_grid[i] >= threshold) {
            ++count;
        }
    }
    return count;
}

void DisorderGrid::update_stats() {
    m_total_disorder = 0;
    m_high_disorder_tiles = 0;
    for (size_t i = 0; i < m_grid.size(); ++i) {
        m_total_disorder += m_grid[i];
        if (m_grid[i] >= 128) {
            ++m_high_disorder_tiles;
        }
    }
}

const uint8_t* DisorderGrid::get_raw_data() const {
    return m_grid.data();
}

void DisorderGrid::clear() {
    std::fill(m_grid.begin(), m_grid.end(), static_cast<uint8_t>(0));
    std::fill(m_previous_grid.begin(), m_previous_grid.end(), static_cast<uint8_t>(0));
    m_total_disorder = 0;
    m_high_disorder_tiles = 0;
}

bool DisorderGrid::is_valid(int32_t x, int32_t y) const {
    return x >= 0 && y >= 0 &&
           static_cast<uint32_t>(x) < m_width &&
           static_cast<uint32_t>(y) < m_height;
}

size_t DisorderGrid::index(int32_t x, int32_t y) const {
    return static_cast<size_t>(y) * m_width + static_cast<size_t>(x);
}

} // namespace disorder
} // namespace sims3000
