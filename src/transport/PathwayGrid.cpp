/**
 * @file PathwayGrid.cpp
 * @brief Implementation of PathwayGrid dense 2D array for pathway placement.
 *        (Epic 7, Ticket E7-005)
 *
 * @see PathwayGrid.h for class documentation.
 */

#include <sims3000/transport/PathwayGrid.h>

namespace sims3000 {
namespace transport {

PathwayGrid::PathwayGrid(uint32_t width, uint32_t height)
    : grid_(static_cast<size_t>(width) * height)
    , width_(width)
    , height_(height)
    , network_dirty_(true)
{
}

// ============================================================================
// Core operations
// ============================================================================

uint32_t PathwayGrid::get_pathway_at(int32_t x, int32_t y) const {
    if (!in_bounds(x, y)) {
        return 0;
    }
    return grid_[index(x, y)].entity_id;
}

bool PathwayGrid::has_pathway(int32_t x, int32_t y) const {
    if (!in_bounds(x, y)) {
        return false;
    }
    return grid_[index(x, y)].entity_id != 0;
}

void PathwayGrid::set_pathway(int32_t x, int32_t y, uint32_t entity_id) {
    if (!in_bounds(x, y)) {
        return;
    }
    grid_[index(x, y)].entity_id = entity_id;
    network_dirty_ = true;
}

void PathwayGrid::clear_pathway(int32_t x, int32_t y) {
    if (!in_bounds(x, y)) {
        return;
    }
    grid_[index(x, y)].entity_id = 0;
    network_dirty_ = true;
}

// ============================================================================
// Dirty tracking for network rebuild
// ============================================================================

bool PathwayGrid::is_network_dirty() const {
    return network_dirty_;
}

void PathwayGrid::mark_network_dirty() {
    network_dirty_ = true;
}

void PathwayGrid::mark_network_clean() {
    network_dirty_ = false;
}

// ============================================================================
// Dimensions
// ============================================================================

uint32_t PathwayGrid::width() const {
    return width_;
}

uint32_t PathwayGrid::height() const {
    return height_;
}

// ============================================================================
// Bounds check
// ============================================================================

bool PathwayGrid::in_bounds(int32_t x, int32_t y) const {
    return x >= 0 && y >= 0
        && static_cast<uint32_t>(x) < width_
        && static_cast<uint32_t>(y) < height_;
}

// ============================================================================
// Private helpers
// ============================================================================

uint32_t PathwayGrid::index(int32_t x, int32_t y) const {
    return static_cast<uint32_t>(y) * width_ + static_cast<uint32_t>(x);
}

} // namespace transport
} // namespace sims3000
