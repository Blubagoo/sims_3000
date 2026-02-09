/**
 * @file ContaminationSystem.cpp
 * @brief Implementation of ContaminationSystem skeleton.
 *
 * @see ContaminationSystem.h for class documentation.
 * @see E10-081
 */

#include <sims3000/contamination/ContaminationSystem.h>

namespace sims3000 {
namespace contamination {

ContaminationSystem::ContaminationSystem(uint16_t grid_width, uint16_t grid_height)
    : m_grid(grid_width, grid_height)
{
}

void ContaminationSystem::tick(const ISimulationTime& /*time*/) {
    m_grid.swap_buffers();
    generate();
    apply_spread();
    apply_decay();
    update_stats();
}

const ContaminationGrid& ContaminationSystem::get_grid() const {
    return m_grid;
}

ContaminationGrid& ContaminationSystem::get_grid_mut() {
    return m_grid;
}

uint32_t ContaminationSystem::get_total_contamination() const {
    return m_grid.get_total_contamination();
}

uint32_t ContaminationSystem::get_toxic_tiles(uint8_t threshold) const {
    return m_grid.get_toxic_tiles(threshold);
}

void ContaminationSystem::generate() {
    // Stub: will be implemented by E10-082, 083, 084, 085, 086
}

void ContaminationSystem::apply_spread() {
    // Stub: will be implemented by E10-087
}

void ContaminationSystem::apply_decay() {
    // Stub: will be implemented by E10-088
}

void ContaminationSystem::update_stats() {
    m_grid.update_stats();
}

} // namespace contamination
} // namespace sims3000
