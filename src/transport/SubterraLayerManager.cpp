/**
 * @file SubterraLayerManager.cpp
 * @brief Subterra layer grid manager implementation for Epic 7 (Ticket E7-042)
 */

#include <sims3000/transport/SubterraLayerManager.h>

namespace sims3000 {
namespace transport {

SubterraLayerManager::SubterraLayerManager(uint32_t width, uint32_t height)
    : subterra_grid_(static_cast<size_t>(width) * height, 0)
    , width_(width)
    , height_(height)
{
}

uint32_t SubterraLayerManager::get_subterra_at(int32_t x, int32_t y) const {
    if (!in_bounds(x, y)) {
        return 0;
    }
    return subterra_grid_[static_cast<size_t>(y) * width_ + static_cast<size_t>(x)];
}

bool SubterraLayerManager::has_subterra(int32_t x, int32_t y) const {
    return get_subterra_at(x, y) != 0;
}

void SubterraLayerManager::set_subterra(int32_t x, int32_t y, uint32_t entity_id) {
    if (!in_bounds(x, y)) {
        return;
    }
    subterra_grid_[static_cast<size_t>(y) * width_ + static_cast<size_t>(x)] = entity_id;
}

void SubterraLayerManager::clear_subterra(int32_t x, int32_t y) {
    set_subterra(x, y, 0);
}

bool SubterraLayerManager::can_build_subterra_at(int32_t x, int32_t y) const {
    if (!in_bounds(x, y)) {
        return false;
    }
    // Check not occupied (entity_id == 0 means empty)
    return get_subterra_at(x, y) == 0;
}

bool SubterraLayerManager::can_build_subterra_at(int32_t x, int32_t y, bool require_adjacent) const {
    // Rule 1: Bounds check (negative coordinates and out of bounds rejected)
    if (!in_bounds(x, y)) {
        return false;
    }

    // Rule 2: Not already occupied
    if (get_subterra_at(x, y) != 0) {
        return false;
    }

    // Rule 3: Adjacency requirement
    if (require_adjacent) {
        // Allow first placement if grid is completely empty
        if (is_grid_empty()) {
            return true;
        }
        // Otherwise, at least one N/S/E/W neighbor must have subterra
        if (!has_adjacent_subterra(x, y)) {
            return false;
        }
    }

    return true;
}

uint32_t SubterraLayerManager::width() const {
    return width_;
}

uint32_t SubterraLayerManager::height() const {
    return height_;
}

bool SubterraLayerManager::in_bounds(int32_t x, int32_t y) const {
    return x >= 0 && y >= 0 &&
           static_cast<uint32_t>(x) < width_ &&
           static_cast<uint32_t>(y) < height_;
}

bool SubterraLayerManager::has_adjacent_subterra(int32_t x, int32_t y) const {
    // Check N/S/E/W cardinal directions
    static const int32_t dx[] = { 0, 0, -1, 1 };
    static const int32_t dy[] = { -1, 1, 0, 0 };

    for (int i = 0; i < 4; ++i) {
        if (has_subterra(x + dx[i], y + dy[i])) {
            return true;
        }
    }
    return false;
}

bool SubterraLayerManager::is_grid_empty() const {
    for (size_t i = 0; i < subterra_grid_.size(); ++i) {
        if (subterra_grid_[i] != 0) {
            return false;
        }
    }
    return true;
}

} // namespace transport
} // namespace sims3000
