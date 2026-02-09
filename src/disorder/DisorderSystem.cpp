/**
 * @file DisorderSystem.cpp
 * @brief Disorder simulation system implementation (Ticket E10-072)
 *
 * Skeleton implementation with phased tick processing.
 * Phase method bodies are stubs; filled in by later tickets
 * (E10-073 through E10-076).
 */

#include "sims3000/disorder/DisorderSystem.h"

namespace sims3000 {
namespace disorder {

DisorderSystem::DisorderSystem(uint16_t grid_width, uint16_t grid_height)
    : m_grid(grid_width, grid_height)
{
}

void DisorderSystem::tick(const ISimulationTime& /*time*/) {
    // Phase 1: Swap double buffers (previous becomes readable, current becomes writable)
    m_grid.swap_buffers();

    // Phase 2: Generate disorder from sources (stub)
    generate();

    // Phase 3: Apply land value modifiers (stub)
    apply_land_value();

    // Phase 4: Spread disorder to neighbors (stub)
    apply_spread();

    // Phase 5: Apply enforcer suppression (stub)
    apply_suppression();

    // Phase 6: Update aggregate statistics
    update_stats();
}

const DisorderGrid& DisorderSystem::get_grid() const {
    return m_grid;
}

DisorderGrid& DisorderSystem::get_grid_mut() {
    return m_grid;
}

uint32_t DisorderSystem::get_total_disorder() const {
    return m_grid.get_total_disorder();
}

uint32_t DisorderSystem::get_high_disorder_tiles(uint8_t threshold) const {
    return m_grid.get_high_disorder_tiles(threshold);
}

// Stub implementations — filled in by later tickets (E10-073 through E10-076)

void DisorderSystem::generate() {
    // Stub: disorder source generation (E10-073)
}

void DisorderSystem::apply_land_value() {
    // Stub: land value modifier application (E10-074)
}

void DisorderSystem::apply_spread() {
    // Stub: disorder diffusion/spread (E10-075)
}

void DisorderSystem::apply_suppression() {
    // Stub: enforcer suppression application (E10-076)
}

void DisorderSystem::update_stats() {
    m_grid.update_stats();
}

} // namespace disorder
} // namespace sims3000
