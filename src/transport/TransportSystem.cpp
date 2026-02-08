/**
 * @file TransportSystem.cpp
 * @brief Implementation of TransportSystem orchestrator (Epic 7, Ticket E7-022)
 *
 * Ties all transport subsystems together with a 6-phase tick loop:
 * 1. Rebuild network graph + proximity cache if dirty
 * 2. Clear previous tick flow
 * 3. Propagate flow (diffusion model)
 * 4. Calculate congestion from flow vs capacity
 * 5. Apply decay (every 100 ticks)
 * 6. Emit events (clear at tick start)
 *
 * @see TransportSystem.h for class documentation.
 */

#include <sims3000/transport/TransportSystem.h>
#include <sims3000/transport/FlowDistribution.h>

namespace sims3000 {
namespace transport {

// =============================================================================
// Construction
// =============================================================================

TransportSystem::TransportSystem(uint32_t map_width, uint32_t map_height)
    : map_width_(map_width)
    , map_height_(map_height)
    , pathway_grid_(map_width, map_height)
    , proximity_cache_(map_width, map_height)
{
    // Wire up TransportProviderImpl with our internal data
    provider_impl_.set_pathway_grid(&pathway_grid_);
    provider_impl_.set_proximity_cache(&proximity_cache_);
    provider_impl_.set_network_graph(&network_graph_);
}

// =============================================================================
// ISimulatable interface
// =============================================================================

void TransportSystem::tick(float delta_time) {
    (void)delta_time;

    // Clear event buffers at tick start
    placed_events_.clear();
    removed_events_.clear();

    // Update internal tick counter
    ++current_tick_;
    ++decay_tick_counter_;

    // Update provider tick for grace period tracking
    provider_impl_.update_tick(current_tick_);

    // Execute tick phases
    phase1_rebuild_if_dirty();
    phase2_clear_flow();
    phase3_propagate_flow();
    phase4_calculate_congestion();
    phase5_apply_decay();
    phase6_emit_events();
}

// =============================================================================
// Pathway Management
// =============================================================================

uint32_t TransportSystem::place_pathway(int32_t x, int32_t y, PathwayType type, uint8_t owner) {
    // Validate bounds
    if (!pathway_grid_.in_bounds(x, y)) {
        return 0;
    }

    // Check if cell is already occupied
    if (pathway_grid_.has_pathway(x, y)) {
        return 0;
    }

    // Validate owner
    if (owner >= MAX_PLAYERS) {
        return 0;
    }

    // Assign entity ID
    uint32_t entity_id = next_entity_id_++;

    // Create RoadComponent with type-specific stats
    PathwayTypeStats stats = get_pathway_stats(type);
    RoadComponent road;
    road.type = type;
    road.direction = PathwayDirection::Bidirectional;
    road.base_capacity = stats.base_capacity;
    road.current_capacity = stats.base_capacity;
    road.health = 255;  // Pristine
    road.decay_rate = 1;
    road.connection_mask = 0;
    road.is_junction = false;
    road.network_id = 0;
    road.last_maintained_tick = current_tick_;

    // Create TrafficComponent
    TrafficComponent traffic_comp;

    // Store entity data
    roads_[entity_id] = road;
    traffic_[entity_id] = traffic_comp;
    road_owners_[entity_id] = owner;
    road_positions_[entity_id] = std::make_pair(x, y);

    // Place in grid (marks network dirty)
    pathway_grid_.set_pathway(x, y, entity_id);

    // Mark proximity cache dirty
    proximity_cache_.mark_dirty();

    // Emit placed event
    placed_events_.emplace_back(entity_id,
                                static_cast<uint32_t>(x),
                                static_cast<uint32_t>(y),
                                type, owner);

    return entity_id;
}

bool TransportSystem::remove_pathway(uint32_t entity_id, int32_t x, int32_t y, uint8_t owner) {
    // Validate entity exists
    auto it = roads_.find(entity_id);
    if (it == roads_.end()) {
        return false;
    }

    // Validate ownership
    auto owner_it = road_owners_.find(entity_id);
    if (owner_it == road_owners_.end() || owner_it->second != owner) {
        return false;
    }

    // Validate position matches
    auto pos_it = road_positions_.find(entity_id);
    if (pos_it == road_positions_.end() ||
        pos_it->second.first != x || pos_it->second.second != y) {
        return false;
    }

    // Clear from grid (marks network dirty)
    pathway_grid_.clear_pathway(x, y);

    // Mark proximity cache dirty
    proximity_cache_.mark_dirty();

    // Remove entity data
    roads_.erase(entity_id);
    traffic_.erase(entity_id);
    road_owners_.erase(entity_id);
    road_positions_.erase(entity_id);

    // Emit removed event
    removed_events_.emplace_back(entity_id,
                                  static_cast<uint32_t>(x),
                                  static_cast<uint32_t>(y),
                                  owner);

    return true;
}

// =============================================================================
// ITransportProvider Implementation (delegation)
// =============================================================================

bool TransportSystem::is_road_accessible_at(uint32_t x, uint32_t y, uint32_t max_distance) const {
    return provider_impl_.is_road_accessible_at(x, y, max_distance);
}

uint32_t TransportSystem::get_nearest_road_distance(uint32_t x, uint32_t y) const {
    return provider_impl_.get_nearest_road_distance(x, y);
}

bool TransportSystem::is_road_accessible(uint32_t entity_id) const {
    return provider_impl_.is_road_accessible(entity_id);
}

bool TransportSystem::is_connected_to_network(int32_t x, int32_t y) const {
    return provider_impl_.is_connected_to_network(x, y);
}

bool TransportSystem::are_connected(int32_t x1, int32_t y1, int32_t x2, int32_t y2) const {
    return provider_impl_.are_connected(x1, y1, x2, y2);
}

float TransportSystem::get_congestion_at(int32_t x, int32_t y) const {
    // Look up entity at position, return congestion from TrafficComponent
    uint32_t entity_id = pathway_grid_.get_pathway_at(x, y);
    if (entity_id == 0) {
        return 0.0f;
    }
    auto it = traffic_.find(entity_id);
    if (it == traffic_.end()) {
        return 0.0f;
    }
    return static_cast<float>(it->second.congestion_level) / 255.0f;
}

uint32_t TransportSystem::get_traffic_volume_at(int32_t x, int32_t y) const {
    uint32_t entity_id = pathway_grid_.get_pathway_at(x, y);
    if (entity_id == 0) {
        return 0;
    }
    auto it = traffic_.find(entity_id);
    if (it == traffic_.end()) {
        return 0;
    }
    return it->second.flow_current;
}

uint16_t TransportSystem::get_network_id_at(int32_t x, int32_t y) const {
    return provider_impl_.get_network_id_at(x, y);
}

// =============================================================================
// Queries
// =============================================================================

uint32_t TransportSystem::get_pathway_count() const {
    return static_cast<uint32_t>(roads_.size());
}

bool TransportSystem::has_pathway_at(int32_t x, int32_t y) const {
    return pathway_grid_.has_pathway(x, y);
}

const PathwayGrid& TransportSystem::get_pathway_grid() const {
    return pathway_grid_;
}

const ProximityCache& TransportSystem::get_proximity_cache() const {
    return proximity_cache_;
}

const NetworkGraph& TransportSystem::get_network_graph() const {
    return network_graph_;
}

// =============================================================================
// Events
// =============================================================================

const std::vector<PathwayPlacedEvent>& TransportSystem::get_placed_events() const {
    return placed_events_;
}

const std::vector<PathwayRemovedEvent>& TransportSystem::get_removed_events() const {
    return removed_events_;
}

// =============================================================================
// Grace Period
// =============================================================================

void TransportSystem::activate_grace_period(uint32_t current_tick) {
    provider_impl_.activate_grace_period(current_tick);
}

// =============================================================================
// Tick Phases
// =============================================================================

void TransportSystem::phase1_rebuild_if_dirty() {
    if (pathway_grid_.is_network_dirty()) {
        // Rebuild network graph from grid
        network_graph_.rebuild_from_grid(pathway_grid_);

        // Mark grid network clean
        pathway_grid_.mark_network_clean();

        // Update road components with network IDs
        for (auto& pair : roads_) {
            uint32_t entity_id = pair.first;
            auto pos_it = road_positions_.find(entity_id);
            if (pos_it != road_positions_.end()) {
                GridPosition pos;
                pos.x = pos_it->second.first;
                pos.y = pos_it->second.second;
                pair.second.network_id = network_graph_.get_network_id(pos);
            }
        }
    }

    // Rebuild proximity cache if dirty
    proximity_cache_.rebuild_if_dirty(pathway_grid_);
}

void TransportSystem::phase2_clear_flow() {
    // Shift current flow to previous, then clear current
    for (auto& pair : traffic_) {
        pair.second.flow_previous = pair.second.flow_current;
        pair.second.flow_current = 0;
    }
    // Clear flow accumulator
    flow_accumulator_.clear();
}

void TransportSystem::phase3_propagate_flow() {
    // Skip if no flow to propagate
    if (flow_accumulator_.empty()) {
        // Apply any remaining flow from previous tick propagation
        // Use flow_previous from traffic components as source
        for (const auto& pair : traffic_) {
            if (pair.second.flow_previous > 0) {
                auto pos_it = road_positions_.find(pair.first);
                if (pos_it != road_positions_.end()) {
                    uint64_t key = FlowDistribution::pack_position(
                        pos_it->second.first, pos_it->second.second);
                    flow_accumulator_[key] = pair.second.flow_previous;
                }
            }
        }
    }

    // Propagate flow via diffusion
    flow_propagation_.propagate(flow_accumulator_, pathway_grid_);

    // Write accumulated flow back to TrafficComponents
    for (const auto& flow_entry : flow_accumulator_) {
        int32_t fx = FlowDistribution::unpack_x(flow_entry.first);
        int32_t fy = FlowDistribution::unpack_y(flow_entry.first);
        uint32_t entity_id = pathway_grid_.get_pathway_at(fx, fy);
        if (entity_id != 0) {
            auto it = traffic_.find(entity_id);
            if (it != traffic_.end()) {
                it->second.flow_current = flow_entry.second;
            }
        }
    }
}

void TransportSystem::phase4_calculate_congestion() {
    for (auto& traffic_pair : traffic_) {
        uint32_t entity_id = traffic_pair.first;
        auto road_it = roads_.find(entity_id);
        if (road_it == roads_.end()) {
            continue;
        }

        // Update congestion level
        CongestionCalculator::update_congestion(traffic_pair.second, road_it->second);

        // Update blockage ticks
        CongestionCalculator::update_blockage_ticks(traffic_pair.second);
    }
}

void TransportSystem::phase5_apply_decay() {
    DecayConfig config;

    // Only apply decay every decay_cycle_ticks
    if (decay_tick_counter_ < config.decay_cycle_ticks) {
        return;
    }

    // Reset counter
    decay_tick_counter_ = 0;

    // Apply decay to all roads
    for (auto& road_pair : roads_) {
        uint32_t entity_id = road_pair.first;

        // Get optional traffic component
        const TrafficComponent* traffic_ptr = nullptr;
        auto traffic_it = traffic_.find(entity_id);
        if (traffic_it != traffic_.end()) {
            traffic_ptr = &traffic_it->second;
        }

        // Apply decay, check if threshold crossed
        bool crossed = PathwayDecay::apply_decay(road_pair.second, traffic_ptr, config);

        if (crossed) {
            auto pos_it = road_positions_.find(entity_id);
            if (pos_it != road_positions_.end()) {
                // Threshold was crossed - could emit PathwayDeterioratedEvent
                // (event emission handled in phase6 if needed)
                (void)pos_it;
            }
        }

        // Update capacity based on health
        // Capacity scales linearly with health: capacity = base * (health / 255)
        uint16_t base = road_pair.second.base_capacity;
        uint8_t health = road_pair.second.health;
        road_pair.second.current_capacity = static_cast<uint16_t>(
            (static_cast<uint32_t>(base) * health) / 255
        );
        // Minimum capacity of 1 to avoid division by zero
        if (road_pair.second.current_capacity == 0 && base > 0) {
            road_pair.second.current_capacity = 1;
        }
    }
}

void TransportSystem::phase6_emit_events() {
    // Events are already accumulated in placed_events_ and removed_events_
    // during place_pathway() and remove_pathway() calls.
    // They are cleared at the start of each tick.
    // No additional processing needed here.
}

} // namespace transport
} // namespace sims3000
