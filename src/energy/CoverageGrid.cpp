/**
 * @file CoverageGrid.cpp
 * @brief Implementation of CoverageGrid dense 2D array for energy coverage.
 *
 * @see CoverageGrid.h for class documentation.
 */

#include <sims3000/energy/CoverageGrid.h>
#include <algorithm>

namespace sims3000 {
namespace energy {

CoverageGrid::CoverageGrid(uint32_t width, uint32_t height)
    : m_width(width)
    , m_height(height)
    , m_data(static_cast<size_t>(width) * height, 0)
{
}

bool CoverageGrid::is_in_coverage(uint32_t x, uint32_t y, uint8_t owner) const {
    if (!is_valid(x, y)) {
        return false;
    }
    return m_data[index(x, y)] == owner;
}

uint8_t CoverageGrid::get_coverage_owner(uint32_t x, uint32_t y) const {
    if (!is_valid(x, y)) {
        return 0;
    }
    return m_data[index(x, y)];
}

void CoverageGrid::set(uint32_t x, uint32_t y, uint8_t owner) {
    if (!is_valid(x, y)) {
        return;
    }
    m_data[index(x, y)] = owner;
}

void CoverageGrid::clear(uint32_t x, uint32_t y) {
    if (!is_valid(x, y)) {
        return;
    }
    m_data[index(x, y)] = 0;
}

void CoverageGrid::clear_all_for_owner(uint8_t owner) {
    for (size_t i = 0; i < m_data.size(); ++i) {
        if (m_data[i] == owner) {
            m_data[i] = 0;
        }
    }
}

void CoverageGrid::clear_all() {
    std::fill(m_data.begin(), m_data.end(), static_cast<uint8_t>(0));
}

uint32_t CoverageGrid::get_width() const {
    return m_width;
}

uint32_t CoverageGrid::get_height() const {
    return m_height;
}

bool CoverageGrid::is_valid(uint32_t x, uint32_t y) const {
    return x < m_width && y < m_height;
}

uint32_t CoverageGrid::get_coverage_count(uint8_t owner) const {
    uint32_t count = 0;
    for (size_t i = 0; i < m_data.size(); ++i) {
        if (m_data[i] == owner) {
            ++count;
        }
    }
    return count;
}

uint32_t CoverageGrid::index(uint32_t x, uint32_t y) const {
    return y * m_width + x;
}

} // namespace energy
} // namespace sims3000
