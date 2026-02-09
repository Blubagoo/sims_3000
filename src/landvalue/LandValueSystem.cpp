/**
 * @file LandValueSystem.cpp
 * @brief Implementation of LandValueSystem skeleton.
 *
 * @see LandValueSystem.h for class documentation.
 * @see E10-100
 */

#include <sims3000/landvalue/LandValueSystem.h>

namespace sims3000 {
namespace landvalue {

LandValueSystem::LandValueSystem(uint16_t grid_width, uint16_t grid_height)
    : m_grid(grid_width, grid_height)
{
}

void LandValueSystem::tick(const ISimulationTime& /*time*/) {
    m_grid.reset_values();
    apply_terrain_bonus();
    apply_road_bonus();
    apply_disorder_penalty();
    apply_contamination_penalty();
}

const LandValueGrid& LandValueSystem::get_grid() const {
    return m_grid;
}

LandValueGrid& LandValueSystem::get_grid_mut() {
    return m_grid;
}

float LandValueSystem::get_land_value(uint32_t x, uint32_t y) const {
    return static_cast<float>(m_grid.get_value(static_cast<int32_t>(x), static_cast<int32_t>(y)));
}

void LandValueSystem::recalculate() {
    // Stub: will be implemented by E10-105
}

void LandValueSystem::apply_terrain_bonus() {
    // Stub: will be implemented by E10-101
}

void LandValueSystem::apply_road_bonus() {
    // Stub: will be implemented by E10-102
}

void LandValueSystem::apply_disorder_penalty() {
    // Stub: will be implemented by E10-103
}

void LandValueSystem::apply_contamination_penalty() {
    // Stub: will be implemented by E10-104
}

} // namespace landvalue
} // namespace sims3000
