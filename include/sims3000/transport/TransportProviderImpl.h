/**
 * @file TransportProviderImpl.h
 * @brief Real implementation of ITransportProvider (Epic 7, Tickets E7-017/E7-018)
 *
 * TransportProviderImpl bridges the building system's ITransportProvider interface
 * to the transport system's data structures (ProximityCache, PathwayGrid, NetworkGraph).
 *
 * Query performance:
 * - is_road_accessible_at: O(1) via ProximityCache
 * - get_nearest_road_distance: O(1) via ProximityCache
 * - is_connected_to_network: O(1) via PathwayGrid + NetworkGraph
 * - are_connected: O(1) via NetworkGraph network_id comparison
 * - get_network_id_at: O(1) via NetworkGraph
 * - get_congestion_at / get_traffic_volume_at: stub (returns 0, E7-015 will fill in)
 *
 * @see /docs/epics/epic-7/tickets.md (tickets E7-017, E7-018)
 * @see ForwardDependencyInterfaces.h (ITransportProvider)
 */

#pragma once

#include <sims3000/building/ForwardDependencyInterfaces.h>
#include <sims3000/transport/ProximityCache.h>
#include <sims3000/transport/PathwayGrid.h>
#include <sims3000/transport/NetworkGraph.h>
#include <sims3000/transport/ConnectivityQuery.h>
#include <cstdint>
#include <vector>

namespace sims3000 {
namespace transport {

/**
 * @struct TransportGraceConfig
 * @brief Grace period configuration for existing buildings when transport system activates.
 *
 * When the real TransportProviderImpl replaces the stub, existing buildings
 * that were spawned under the permissive stub need time to build road access.
 * During the grace period, is_road_accessible_at returns true unconditionally
 * (matching stub behavior).
 */
struct TransportGraceConfig {
    uint32_t grace_period_ticks = 500;  ///< Duration in ticks (~25 seconds at 20 ticks/sec)
    bool grace_active = false;          ///< Whether grace period is currently active
    uint32_t grace_start_tick = 0;      ///< Tick when grace period was activated
};

/**
 * @struct TransportAccessLostEvent
 * @brief Event emitted when a building loses transport access after grace period ends.
 *
 * Consumed by:
 * - UISystem: Show visual warning on affected buildings
 * - BuildingSystem: May downgrade or abandon building
 * - StatisticsSystem: Track accessibility metrics
 */
struct TransportAccessLostEvent {
    uint32_t x;            ///< Grid X coordinate of affected position
    uint32_t y;            ///< Grid Y coordinate of affected position
    uint32_t max_distance; ///< The max_distance threshold that was exceeded
    uint8_t actual_distance; ///< The actual distance to nearest road

    TransportAccessLostEvent()
        : x(0), y(0), max_distance(0), actual_distance(0) {}

    TransportAccessLostEvent(uint32_t gx, uint32_t gy,
                             uint32_t max_dist, uint8_t actual_dist)
        : x(gx), y(gy), max_distance(max_dist), actual_distance(actual_dist) {}
};

/**
 * @class TransportProviderImpl
 * @brief Real ITransportProvider implementation backed by transport system data.
 *
 * Must be configured with set_proximity_cache(), set_pathway_grid(), and
 * set_network_graph() before use. Null pointers are handled gracefully
 * (methods return safe defaults).
 */
class TransportProviderImpl : public building::ITransportProvider {
public:
    TransportProviderImpl() = default;

    // =========================================================================
    // Grace period management (E7-019)
    // =========================================================================

    /**
     * @brief Activate the grace period for existing buildings.
     *
     * During the grace period, is_road_accessible_at returns true for all
     * positions (matching stub behavior), giving existing buildings time
     * to establish road connectivity.
     *
     * @param current_tick The current simulation tick.
     */
    void activate_grace_period(uint32_t current_tick);

    /**
     * @brief Check if the grace period is currently active.
     *
     * @param current_tick The current simulation tick.
     * @return true if within grace period window.
     */
    bool is_in_grace_period(uint32_t current_tick) const;

    /**
     * @brief Update the internally tracked tick for grace period evaluation.
     *
     * Since is_road_accessible_at is a const query method that doesn't receive
     * the current tick, we track it separately. Call this each simulation tick.
     *
     * @param current_tick The current simulation tick.
     */
    void update_tick(uint32_t current_tick);

    /**
     * @brief Get the grace period configuration (read-only).
     * @return Const reference to the grace config.
     */
    const TransportGraceConfig& get_grace_config() const;

    /**
     * @brief Get pending transport access lost events and clear the queue.
     *
     * Events are accumulated during is_road_accessible_at queries when
     * access is denied after grace period ends. Call this to drain the queue.
     *
     * @return Vector of access lost events since last drain.
     */
    std::vector<TransportAccessLostEvent> drain_access_lost_events();

    // =========================================================================
    // Data source configuration
    // =========================================================================

    /**
     * @brief Set the ProximityCache for distance queries.
     * @param cache Pointer to ProximityCache (may be nullptr for safe defaults).
     */
    void set_proximity_cache(const ProximityCache* cache);

    /**
     * @brief Set the PathwayGrid for spatial pathway lookup.
     * @param grid Pointer to PathwayGrid (may be nullptr for safe defaults).
     */
    void set_pathway_grid(const PathwayGrid* grid);

    /**
     * @brief Set the NetworkGraph for connectivity queries.
     * @param graph Pointer to NetworkGraph (may be nullptr for safe defaults).
     */
    void set_network_graph(const NetworkGraph* graph);

    // =========================================================================
    // ITransportProvider implementation - Original methods (Epic 4)
    // =========================================================================

    /**
     * @brief Check if position is within max_distance of a pathway.
     *
     * O(1) query via ProximityCache.
     *
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @param max_distance Maximum distance in tiles (default 3 for building spawn rule).
     * @return true if within max_distance tiles of a pathway.
     */
    bool is_road_accessible_at(std::uint32_t x, std::uint32_t y,
                                std::uint32_t max_distance) const override;

    /**
     * @brief Get distance to nearest pathway.
     *
     * O(1) query via ProximityCache. Returns 255 if no pathway in range
     * or if cache is not configured.
     *
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @return Distance to nearest pathway (0 = on pathway, 255 = none in range).
     */
    std::uint32_t get_nearest_road_distance(std::uint32_t x, std::uint32_t y) const override;

    // =========================================================================
    // ITransportProvider implementation - Extended methods (Epic 7, E7-016)
    // =========================================================================

    /**
     * @brief Check if an entity (building) has road access.
     *
     * Stub: always returns true for now. Full implementation will look up
     * entity position and check ProximityCache.
     *
     * @param entity The entity ID to check.
     * @return true (stub - always accessible).
     */
    bool is_road_accessible(EntityID entity) const override;

    /**
     * @brief Check if a position is connected to any road network.
     *
     * O(1) query via PathwayGrid + NetworkGraph.
     *
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @return true if the position has a pathway with a non-zero network_id.
     */
    bool is_connected_to_network(std::int32_t x, std::int32_t y) const override;

    /**
     * @brief Check if two positions are connected via the road network.
     *
     * O(1) query via NetworkGraph network_id comparison.
     *
     * @param x1 X coordinate of first position.
     * @param y1 Y coordinate of first position.
     * @param x2 X coordinate of second position.
     * @param y2 Y coordinate of second position.
     * @return true if both positions are in the same connected road network component.
     */
    bool are_connected(std::int32_t x1, std::int32_t y1,
                       std::int32_t x2, std::int32_t y2) const override;

    /**
     * @brief Get congestion level at a position.
     *
     * Stub: returns 0.0f (no congestion). E7-015 will implement real congestion.
     *
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @return 0.0f (stub).
     */
    float get_congestion_at(std::int32_t x, std::int32_t y) const override;

    /**
     * @brief Get traffic volume at a position.
     *
     * Stub: returns 0 (no traffic). E7-015 will implement real traffic volumes.
     *
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @return 0 (stub).
     */
    std::uint32_t get_traffic_volume_at(std::int32_t x, std::int32_t y) const override;

    /**
     * @brief Get the network component ID at a position.
     *
     * O(1) query via NetworkGraph.
     *
     * @param x X coordinate (column).
     * @param y Y coordinate (row).
     * @return Network ID (0 = not part of any network).
     */
    std::uint16_t get_network_id_at(std::int32_t x, std::int32_t y) const override;

private:
    const ProximityCache* cache_ = nullptr;   ///< Distance-to-nearest-pathway cache
    const PathwayGrid* grid_ = nullptr;       ///< Spatial pathway grid
    const NetworkGraph* graph_ = nullptr;     ///< Network connectivity graph
    ConnectivityQuery connectivity_;          ///< Connectivity query helper

    // Grace period state (E7-019)
    TransportGraceConfig grace_config_;       ///< Grace period configuration
    uint32_t current_tick_ = 0;              ///< Last known simulation tick

    // Access lost events (E7-019) - mutable because emitted from const query
    mutable std::vector<TransportAccessLostEvent> pending_access_lost_events_;
};

} // namespace transport
} // namespace sims3000
