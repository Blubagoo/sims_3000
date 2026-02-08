/**
 * @file TransportProviderImpl.cpp
 * @brief Implementation of TransportProviderImpl (Epic 7, Tickets E7-017/E7-018)
 *
 * @see TransportProviderImpl.h
 */

#include <sims3000/transport/TransportProviderImpl.h>

namespace sims3000 {
namespace transport {

// =============================================================================
// Grace period management (E7-019)
// =============================================================================

void TransportProviderImpl::activate_grace_period(uint32_t current_tick) {
    grace_config_.grace_active = true;
    grace_config_.grace_start_tick = current_tick;
}

bool TransportProviderImpl::is_in_grace_period(uint32_t current_tick) const {
    if (!grace_config_.grace_active) {
        return false;
    }
    return (current_tick - grace_config_.grace_start_tick) < grace_config_.grace_period_ticks;
}

void TransportProviderImpl::update_tick(uint32_t current_tick) {
    current_tick_ = current_tick;
}

const TransportGraceConfig& TransportProviderImpl::get_grace_config() const {
    return grace_config_;
}

std::vector<TransportAccessLostEvent> TransportProviderImpl::drain_access_lost_events() {
    std::vector<TransportAccessLostEvent> events;
    events.swap(pending_access_lost_events_);
    return events;
}

// =============================================================================
// Data source configuration
// =============================================================================

void TransportProviderImpl::set_proximity_cache(const ProximityCache* cache) {
    cache_ = cache;
}

void TransportProviderImpl::set_pathway_grid(const PathwayGrid* grid) {
    grid_ = grid;
}

void TransportProviderImpl::set_network_graph(const NetworkGraph* graph) {
    graph_ = graph;
}

// =============================================================================
// Original methods (Epic 4)
// =============================================================================

bool TransportProviderImpl::is_road_accessible_at(std::uint32_t x, std::uint32_t y,
                                                    std::uint32_t max_distance) const {
    // During grace period, allow all access (like stub)
    if (grace_config_.grace_active &&
        (current_tick_ - grace_config_.grace_start_tick) < grace_config_.grace_period_ticks) {
        return true;
    }

    // No cache = permissive (transport system not yet fully initialized)
    if (!cache_) {
        return true;
    }

    uint8_t distance = cache_->get_distance(static_cast<int32_t>(x), static_cast<int32_t>(y));
    bool accessible = distance <= max_distance;

    // Emit TransportAccessLostEvent when access is denied
    if (!accessible) {
        pending_access_lost_events_.emplace_back(x, y, max_distance, distance);
    }

    return accessible;
}

std::uint32_t TransportProviderImpl::get_nearest_road_distance(std::uint32_t x, std::uint32_t y) const {
    if (!cache_) {
        return 255;
    }
    return static_cast<std::uint32_t>(cache_->get_distance(static_cast<int32_t>(x), static_cast<int32_t>(y)));
}

// =============================================================================
// Extended methods (Epic 7, E7-016)
// =============================================================================

bool TransportProviderImpl::is_road_accessible(EntityID entity) const {
    // Stub: always returns true for now.
    // Full implementation will look up entity position from ECS registry
    // and check ProximityCache distance.
    (void)entity;
    return true;
}

bool TransportProviderImpl::is_connected_to_network(std::int32_t x, std::int32_t y) const {
    if (!grid_ || !graph_) {
        return false;
    }
    return connectivity_.is_on_network(*grid_, *graph_, x, y);
}

bool TransportProviderImpl::are_connected(std::int32_t x1, std::int32_t y1,
                                           std::int32_t x2, std::int32_t y2) const {
    if (!grid_ || !graph_) {
        return false;
    }
    return connectivity_.is_connected(*grid_, *graph_, x1, y1, x2, y2);
}

float TransportProviderImpl::get_congestion_at(std::int32_t x, std::int32_t y) const {
    // Stub: no congestion. E7-015 will implement real congestion tracking.
    (void)x; (void)y;
    return 0.0f;
}

std::uint32_t TransportProviderImpl::get_traffic_volume_at(std::int32_t x, std::int32_t y) const {
    // Stub: no traffic. E7-015 will implement real traffic volume tracking.
    (void)x; (void)y;
    return 0;
}

std::uint16_t TransportProviderImpl::get_network_id_at(std::int32_t x, std::int32_t y) const {
    if (!grid_ || !graph_) {
        return 0;
    }
    return connectivity_.get_network_id_at(*grid_, *graph_, x, y);
}

} // namespace transport
} // namespace sims3000
