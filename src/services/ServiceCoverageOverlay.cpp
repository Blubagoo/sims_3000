/**
 * @file ServiceCoverageOverlay.cpp
 * @brief Implementation of ServiceCoverageOverlay (Ticket E9-043)
 *
 * @see ServiceCoverageOverlay.h for interface documentation
 */

#include <sims3000/services/ServiceCoverageOverlay.h>

namespace sims3000 {
namespace services {

ServiceCoverageOverlay::ServiceCoverageOverlay(
    const char* name,
    const ServiceCoverageGrid* grid,
    uint8_t base_r,
    uint8_t base_g,
    uint8_t base_b)
    : m_name(name)
    , m_grid(grid)
    , m_base_r(base_r)
    , m_base_g(base_g)
    , m_base_b(base_b)
{
}

const char* ServiceCoverageOverlay::getName() const {
    return m_name;
}

OverlayColor ServiceCoverageOverlay::getColorAt(uint32_t x, uint32_t y) const {
    if (!m_grid) {
        return { 0, 0, 0, 0 };
    }

    if (!m_grid->is_valid(x, y)) {
        return { 0, 0, 0, 0 };
    }

    uint8_t coverage = m_grid->get_coverage_at(x, y);
    return { m_base_r, m_base_g, m_base_b, coverage };
}

bool ServiceCoverageOverlay::isActive() const {
    return m_active;
}

void ServiceCoverageOverlay::setActive(bool active) {
    m_active = active;
}

void ServiceCoverageOverlay::setGrid(const ServiceCoverageGrid* grid) {
    m_grid = grid;
}

} // namespace services
} // namespace sims3000
