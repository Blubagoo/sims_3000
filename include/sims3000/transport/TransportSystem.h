/**
 * @file TransportSystem.h
 * @brief Main transport system orchestrator for Epic 7 (Ticket E7-022)
 *
 * TransportSystem ties all transport subsystems together:
 * - PathwayGrid: spatial pathway storage
 * - ProximityCache: distance-to-nearest-pathway cache
 * - NetworkGraph: connected component graph
 * - TransportProviderImpl: ITransportProvider implementation
 * - FlowPropagation: traffic flow diffusion
 * - CongestionCalculator: congestion from flow vs capacity
 * - PathwayDecay: pathway health degradation
 *
 * Implements ISimulatable (duck-typed) at priority 45.
 * Implements ITransportProvider via delegation to TransportProviderImpl.
 *
 * Tick phases:
 * 1. Rebuild network graph + proximity cache if dirty
 * 2. Clear previous tick flow
 * 3. Propagate flow (diffusion model)
 * 4. Calculate congestion from flow vs capacity
 * 5. Apply decay (every 100 ticks)
 * 6. Emit events (clear at tick start)
 *
 * Uses canonical alien terminology per /docs/canon/terminology.yaml
 */

#pragma once

#include <sims3000/transport/PathwayGrid.h>
#include <sims3000/transport/ProximityCache.h>
#include <sims3000/transport/NetworkGraph.h>
#include <sims3000/transport/TransportProviderImpl.h>
#include <sims3000/transport/FlowPropagation.h>
#include <sims3000/transport/RoadComponent.h>
#include <sims3000/transport/TrafficComponent.h>
#include <sims3000/transport/TransportEvents.h>
#include <sims3000/transport/PathwayDecay.h>
#include <sims3000/transport/CongestionCalculator.h>
#include <sims3000/transport/PathwayTypeConfig.h>
#include <sims3000/building/ForwardDependencyInterfaces.h>
#include <cstdint>
#include <unordered_map>
#include <vector>
#include <utility>

namespace sims3000 {
namespace transport {

/**
 * @class TransportSystem
 * @brief Main orchestrator that ties all transport subsystems together.
 *
 * Implements ITransportProvider for downstream system queries.
 * Implements ISimulatable (duck-typed) at priority 45.
 */
class TransportSystem : public building::ITransportProvider {
public:
    static constexpr int TICK_PRIORITY = 45;
    static constexpr uint8_t MAX_PLAYERS = 4;

    /**
     * @brief Construct TransportSystem with map dimensions.
     *
     * Initializes PathwayGrid, ProximityCache, and wires up the
     * TransportProviderImpl with references to internal data.
     *
     * @param map_width Map width in tiles.
     * @param map_height Map height in tiles.
     */
    TransportSystem(uint32_t map_width, uint32_t map_height);

    // =========================================================================
    // ISimulatable interface (duck-typed)
    // =========================================================================

    /**
     * @brief Called every simulation tick.
     *
     * Executes all tick phases in order:
     * 1. Rebuild if dirty
     * 2. Clear flow
     * 3. Propagate flow
     * 4. Calculate congestion
     * 5. Apply decay (every 100 ticks)
     * 6. Emit events
     *
     * @param delta_time Time since last tick in seconds.
     */
    void tick(float delta_time);

    /**
     * @brief Get execution priority (lower = earlier).
     * @return 45 per canonical interface spec.
     */
    int get_priority() const { return TICK_PRIORITY; }

    // =========================================================================
    // Pathway Management
    // =========================================================================

    /**
     * @brief Place a pathway on the grid.
     *
     * Creates a new pathway entity with RoadComponent and TrafficComponent,
     * sets it in the PathwayGrid, marks dirty flags, and emits a placed event.
     *
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @param type Type of pathway to place.
     * @param owner Owning player ID (0-3).
     * @return Entity ID of the placed pathway, or 0 if placement failed.
     */
    uint32_t place_pathway(int32_t x, int32_t y, PathwayType type, uint8_t owner);

    /**
     * @brief Remove a pathway from the grid.
     *
     * Clears the pathway from PathwayGrid, removes component data,
     * marks dirty flags, and emits a removed event.
     *
     * @param entity_id Entity ID of the pathway to remove.
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @param owner Owning player ID (for validation).
     * @return true if removal succeeded.
     */
    bool remove_pathway(uint32_t entity_id, int32_t x, int32_t y, uint8_t owner);

    // =========================================================================
    // ITransportProvider Implementation (delegated to TransportProviderImpl)
    // =========================================================================

    bool is_road_accessible_at(uint32_t x, uint32_t y, uint32_t max_distance) const override;
    uint32_t get_nearest_road_distance(uint32_t x, uint32_t y) const override;
    bool is_road_accessible(uint32_t entity_id) const override;
    bool is_connected_to_network(int32_t x, int32_t y) const override;
    bool are_connected(int32_t x1, int32_t y1, int32_t x2, int32_t y2) const override;
    float get_congestion_at(int32_t x, int32_t y) const override;
    uint32_t get_traffic_volume_at(int32_t x, int32_t y) const override;
    uint16_t get_network_id_at(int32_t x, int32_t y) const override;

    // =========================================================================
    // Queries
    // =========================================================================

    /**
     * @brief Get the total number of pathways on the grid.
     * @return Number of pathway entities.
     */
    uint32_t get_pathway_count() const;

    /**
     * @brief Check if a pathway exists at a position.
     * @param x X coordinate.
     * @param y Y coordinate.
     * @return true if a pathway exists at (x, y).
     */
    bool has_pathway_at(int32_t x, int32_t y) const;

    /**
     * @brief Get const reference to the pathway grid.
     * @return Const reference to PathwayGrid.
     */
    const PathwayGrid& get_pathway_grid() const;

    /**
     * @brief Get const reference to the proximity cache.
     * @return Const reference to ProximityCache.
     */
    const ProximityCache& get_proximity_cache() const;

    /**
     * @brief Get const reference to the network graph.
     * @return Const reference to NetworkGraph.
     */
    const NetworkGraph& get_network_graph() const;

    // =========================================================================
    // Events
    // =========================================================================

    /**
     * @brief Get placed events from the current tick.
     * @return Const reference to placed event buffer.
     */
    const std::vector<PathwayPlacedEvent>& get_placed_events() const;

    /**
     * @brief Get removed events from the current tick.
     * @return Const reference to removed event buffer.
     */
    const std::vector<PathwayRemovedEvent>& get_removed_events() const;

    // =========================================================================
    // Grace Period
    // =========================================================================

    /**
     * @brief Activate the grace period for existing buildings.
     * @param current_tick The current simulation tick.
     */
    void activate_grace_period(uint32_t current_tick);

private:
    uint32_t map_width_;
    uint32_t map_height_;
    uint32_t next_entity_id_ = 1;
    uint32_t current_tick_ = 0;
    uint32_t decay_tick_counter_ = 0;

    // Core subsystems
    PathwayGrid pathway_grid_;
    ProximityCache proximity_cache_;
    NetworkGraph network_graph_;
    TransportProviderImpl provider_impl_;
    FlowPropagation flow_propagation_;

    // Per-entity data
    std::unordered_map<uint32_t, RoadComponent> roads_;         ///< entity_id -> road
    std::unordered_map<uint32_t, TrafficComponent> traffic_;    ///< entity_id -> traffic
    std::unordered_map<uint32_t, uint8_t> road_owners_;         ///< entity_id -> owner
    std::unordered_map<uint32_t, std::pair<int32_t,int32_t>> road_positions_;  ///< entity_id -> (x,y)

    // Flow accumulator
    std::unordered_map<uint64_t, uint32_t> flow_accumulator_;   ///< packed_pos -> flow

    // Event buffers
    std::vector<PathwayPlacedEvent> placed_events_;
    std::vector<PathwayRemovedEvent> removed_events_;

    // =========================================================================
    // Tick phases
    // =========================================================================

    /**
     * @brief Phase 1: Rebuild network graph and proximity cache if dirty.
     */
    void phase1_rebuild_if_dirty();

    /**
     * @brief Phase 2: Clear previous tick flow values.
     */
    void phase2_clear_flow();

    /**
     * @brief Phase 3: Propagate flow via diffusion model.
     */
    void phase3_propagate_flow();

    /**
     * @brief Phase 4: Calculate congestion from flow vs capacity.
     */
    void phase4_calculate_congestion();

    /**
     * @brief Phase 5: Apply decay (every 100 ticks).
     */
    void phase5_apply_decay();

    /**
     * @brief Phase 6: Emit events (clear at tick start).
     */
    void phase6_emit_events();
};

} // namespace transport
} // namespace sims3000
