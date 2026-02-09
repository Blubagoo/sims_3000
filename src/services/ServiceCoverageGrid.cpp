/**
 * @file ServiceCoverageGrid.cpp
 * @brief Implementation of ServiceCoverageGrid dense 2D array for service coverage.
 *
 * @see ServiceCoverageGrid.h for class documentation.
 */

#include <sims3000/services/ServiceCoverageGrid.h>
#include <algorithm>

namespace sims3000 {
namespace services {

ServiceCoverageGrid::ServiceCoverageGrid(uint32_t width, uint32_t height)
    : m_width(width)
    , m_height(height)
    , m_data(static_cast<size_t>(width) * height, 0)
{
}

uint8_t ServiceCoverageGrid::get_coverage_at(uint32_t x, uint32_t y) const {
    if (!is_valid(x, y)) {
        return 0;
    }
    return m_data[index(x, y)];
}

float ServiceCoverageGrid::get_coverage_at_normalized(uint32_t x, uint32_t y) const {
    if (!is_valid(x, y)) {
        return 0.0f;
    }
    return static_cast<float>(m_data[index(x, y)]) / 255.0f;
}

void ServiceCoverageGrid::set_coverage_at(uint32_t x, uint32_t y, uint8_t value) {
    if (!is_valid(x, y)) {
        return;
    }
    m_data[index(x, y)] = value;
}

void ServiceCoverageGrid::clear() {
    std::fill(m_data.begin(), m_data.end(), static_cast<uint8_t>(0));
}

uint32_t ServiceCoverageGrid::get_width() const {
    return m_width;
}

uint32_t ServiceCoverageGrid::get_height() const {
    return m_height;
}

bool ServiceCoverageGrid::is_valid(uint32_t x, uint32_t y) const {
    return x < m_width && y < m_height;
}

uint32_t ServiceCoverageGrid::index(uint32_t x, uint32_t y) const {
    return y * m_width + x;
}

} // namespace services
} // namespace sims3000
