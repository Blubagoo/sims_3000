/**
 * @file PortSystem.h
 * @brief Main port system orchestrator for Epic 8 (Ticket E8-006)
 *
 * PortSystem manages all port facilities, external connections, and trade
 * agreements. It provides the IPortProvider interface for downstream systems
 * to query port state, demand bonuses, and trade income.
 *
 * Implements ISimulatable (duck-typed) at priority 48.
 * Implements IPortProvider for downstream system queries.
 *
 * Tick phases:
 * 1. Update port operational states
 * 2. Update external connection states
 * 3. Calculate trade income from agreements
 * 4. Cache demand bonuses for zone queries
 *
 * Runs after RailSystem (47), before PopulationSystem (50).
 *
 * Uses canonical alien terminology per /docs/canon/terminology.yaml
 * (aero_port, aqua_port - not airport/seaport)
 */

#pragma once

#include <sims3000/building/ForwardDependencyInterfaces.h>
#include <sims3000/port/DemandBonus.h>
#include <sims3000/port/TradeIncome.h>
#include <sims3000/port/TradeAgreementComponent.h>
#include <sims3000/port/PortIncomeUI.h>
#include <sims3000/port/PortRenderData.h>
#include <sims3000/port/PortZoneComponent.h>
#include <array>
#include <cstdint>
#include <vector>

namespace sims3000 {
namespace port {

/**
 * @class PortSystem
 * @brief Main orchestrator for port facilities, connections, and trade.
 *
 * Implements IPortProvider for downstream system queries.
 * Implements ISimulatable (duck-typed) at priority 48.
 */
class PortSystem : public building::IPortProvider {
public:
    static constexpr int TICK_PRIORITY = 48;
    static constexpr std::uint8_t MAX_PLAYERS = 4;

    /**
     * @brief Construct PortSystem with map dimensions.
     *
     * @param map_width Map width in tiles.
     * @param map_height Map height in tiles.
     */
    PortSystem(std::int32_t map_width, std::int32_t map_height);

    // =========================================================================
    // ISimulatable interface (duck-typed)
    // =========================================================================

    /**
     * @brief Called every simulation tick.
     *
     * Executes all tick phases in order:
     * 1. Update port states
     * 2. Update external connections
     * 3. Calculate trade income
     * 4. Cache demand bonuses
     *
     * @param delta_time Time since last tick in seconds.
     */
    void tick(float delta_time);

    /**
     * @brief Get execution priority (lower = earlier).
     * @return 48 - runs after RailSystem (47), before PopulationSystem (50).
     */
    int get_priority() const { return TICK_PRIORITY; }

    // =========================================================================
    // IPortProvider overrides (stub implementations initially)
    // =========================================================================

    std::uint32_t get_port_capacity(std::uint8_t port_type, std::uint8_t owner) const override;
    float get_port_utilization(std::uint8_t port_type, std::uint8_t owner) const override;
    bool has_operational_port(std::uint8_t port_type, std::uint8_t owner) const override;
    std::uint32_t get_port_count(std::uint8_t port_type, std::uint8_t owner) const override;

    float get_global_demand_bonus(std::uint8_t zone_type, std::uint8_t owner) const override;
    float get_local_demand_bonus(std::uint8_t zone_type, std::int32_t x, std::int32_t y, std::uint8_t owner) const override;

    std::uint32_t get_external_connection_count(std::uint8_t owner) const override;
    bool is_connected_to_edge(std::uint8_t edge, std::uint8_t owner) const override;

    std::int64_t get_trade_income(std::uint8_t owner) const override;

    // =========================================================================
    // Trade income breakdown (E8-020: EconomySystem integration)
    // =========================================================================

    /**
     * @brief Get detailed trade income breakdown for a player.
     *
     * Returns the cached TradeIncomeBreakdown computed during the last tick.
     * EconomySystem (Epic 11) queries this each budget cycle.
     *
     * @param owner Player ID.
     * @return TradeIncomeBreakdown with per-source and total income.
     */
    TradeIncomeBreakdown get_trade_income_breakdown(std::uint8_t owner) const;

    // =========================================================================
    // Trade agreement management (E8-020)
    // =========================================================================

    /**
     * @brief Add a trade agreement to the system's tracked collection.
     *
     * @param agreement The trade agreement to add.
     */
    void add_trade_agreement(const TradeAgreementComponent& agreement);

    /**
     * @brief Clear all tracked trade agreements.
     */
    void clear_trade_agreements();

    /**
     * @brief Get read-only access to all tracked trade agreements.
     * @return Const reference to the agreements vector.
     */
    const std::vector<TradeAgreementComponent>& get_trade_agreements() const { return m_agreements; }

    // =========================================================================
    // Port zone data management (E8-030)
    // =========================================================================

    /**
     * @brief Associate port zone component data with a port at a position.
     *
     * @param owner Owner player ID.
     * @param x X coordinate of the port zone.
     * @param y Y coordinate of the port zone.
     * @param zone The port zone component data.
     */
    void set_port_zone(std::uint8_t owner, std::int32_t x, std::int32_t y,
                       const PortZoneComponent& zone);

    /**
     * @brief Get port zone component data for a port at a position.
     *
     * @param owner Owner player ID.
     * @param x X coordinate of the port zone.
     * @param y Y coordinate of the port zone.
     * @param out_zone Output parameter for the zone data.
     * @return true if zone data was found, false otherwise.
     */
    bool get_port_zone(std::uint8_t owner, std::int32_t x, std::int32_t y,
                       PortZoneComponent& out_zone) const;

    // =========================================================================
    // Port data management
    // =========================================================================

    /**
     * @brief Add a port to the system's tracked collection.
     *
     * The port data is used for demand bonus calculations and trade income.
     *
     * @param port The port data to add.
     */
    void add_port(const PortData& port);

    /**
     * @brief Remove all ports matching the given owner and position.
     *
     * @param owner Owner player ID.
     * @param x X coordinate of the port.
     * @param y Y coordinate of the port.
     */
    void remove_port(std::uint8_t owner, std::int32_t x, std::int32_t y);

    /**
     * @brief Clear all tracked ports.
     */
    void clear_ports();

    /**
     * @brief Get read-only access to all tracked ports.
     * @return Const reference to the ports vector.
     */
    const std::vector<PortData>& get_ports() const { return m_ports; }

    /**
     * @brief Get mutable access to all tracked ports.
     * @return Mutable reference to the ports vector.
     */
    std::vector<PortData>& get_ports_mutable() { return m_ports; }

    // =========================================================================
    // Trade income storage
    // =========================================================================

    /**
     * @brief Set the cached trade income for a player.
     *
     * @param owner Player ID.
     * @param income Trade income value.
     */
    void set_cached_trade_income(std::uint8_t owner, std::int64_t income);

    // =========================================================================
    // Trade income UI data (E8-021)
    // =========================================================================

    /**
     * @brief Get detailed trade income data for UI display.
     *
     * Returns per-port income details, aggregate breakdown, and
     * historical income tracking for the last 12 phases.
     *
     * @param owner Player ID.
     * @return TradeIncomeUIData for UI rendering.
     */
    TradeIncomeUIData get_trade_income_ui_data(std::uint8_t owner) const;

    // =========================================================================
    // Port render data (E8-030)
    // =========================================================================

    /**
     * @brief Get port visual state data for rendering.
     *
     * Returns position, type, development level, operational status,
     * and type-specific infrastructure data for all ports owned by
     * the given player.
     *
     * @param owner Player ID.
     * @return Vector of PortRenderData for all matching ports.
     */
    std::vector<struct PortRenderData> get_port_render_data(std::uint8_t owner) const;

    // =========================================================================
    // Queries
    // =========================================================================

    /**
     * @brief Get map width in tiles.
     * @return Map width.
     */
    std::int32_t get_map_width() const { return m_map_width; }

    /**
     * @brief Get map height in tiles.
     * @return Map height.
     */
    std::int32_t get_map_height() const { return m_map_height; }

private:
    // =========================================================================
    // Tick phases
    // =========================================================================

    /**
     * @brief Phase 1: Update operational state of all port facilities.
     */
    void update_port_states();

    /**
     * @brief Phase 2: Update external connection active/inactive states.
     */
    void update_external_connections();

    /**
     * @brief Phase 3: Calculate trade income from active agreements.
     */
    void calculate_trade_income();

    /**
     * @brief Phase 4: Cache demand bonuses from ports for zone queries.
     */
    void cache_demand_bonuses();

    // =========================================================================
    // Member data
    // =========================================================================

    std::int32_t m_map_width;
    std::int32_t m_map_height;

    /// Collection of port data for demand bonus and trade calculations
    std::vector<PortData> m_ports;

    /// Cached trade income per player (indexed by owner ID, 0..MAX_PLAYERS)
    std::int64_t m_cached_trade_income[MAX_PLAYERS + 1] = {};

    /// Cached trade income breakdowns per player (E8-020)
    TradeIncomeBreakdown m_cached_breakdowns[MAX_PLAYERS + 1] = {};

    /// Collection of trade agreements for income calculation (E8-020)
    std::vector<TradeAgreementComponent> m_agreements;

    /// Port zone data keyed by (owner, x, y) packed into uint64_t (E8-030)
    struct PortZoneEntry {
        std::uint8_t owner;
        std::int32_t x;
        std::int32_t y;
        PortZoneComponent zone;
    };
    std::vector<PortZoneEntry> m_port_zones;

    /// Historical income per player for last 12 phases (E8-021)
    std::array<int64_t, INCOME_HISTORY_SIZE> m_income_history[MAX_PLAYERS + 1] = {};

    /// Current history write index per player (circular buffer)
    std::size_t m_history_index[MAX_PLAYERS + 1] = {};

    /// Whether history has been initialized (first tick detection)
    bool m_history_initialized[MAX_PLAYERS + 1] = {};
};

} // namespace port
} // namespace sims3000
