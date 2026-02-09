/**
 * @file DemandSystem.h
 * @brief Demand simulation system skeleton (Ticket E10-042)
 *
 * Manages per-player zone demand data for habitation, exchange, and fabrication.
 * Runs at tick_priority 52 with frequency-gated update cycles every
 * DEMAND_CYCLE_TICKS (5 ticks = 250ms).
 *
 * Implements both ISimulatable (for simulation loop participation) and
 * IDemandProvider (for cross-system demand queries from BuildingSystem/ZoneSystem).
 *
 * Sub-phase formula implementations are stubs in this skeleton; they will be
 * filled in by later tickets (E10-043 through E10-045).
 *
 * @see E10-042
 */

#ifndef SIMS3000_DEMAND_DEMANDSYSTEM_H
#define SIMS3000_DEMAND_DEMANDSYSTEM_H

#include <cstdint>

#include "sims3000/core/ISimulatable.h"
#include "sims3000/core/types.h"
#include "sims3000/building/ForwardDependencyInterfaces.h"
#include "sims3000/demand/DemandData.h"

namespace sims3000 {
namespace demand {

/**
 * @class DemandSystem
 * @brief Manages zone growth demand simulation for all players.
 *
 * Each active player has a DemandData instance tracking demand values,
 * capacity caps, and factor breakdowns for three zone types:
 * - Zone type 0: Habitation (residential)
 * - Zone type 1: Exchange (commercial)
 * - Zone type 2: Fabrication (industrial)
 *
 * The tick() method runs frequency-gated demand updates every
 * DEMAND_CYCLE_TICKS ticks. The update_demand() method is a stub
 * that will contain the actual demand formulas in later tickets.
 */
class DemandSystem : public ISimulatable, public building::IDemandProvider {
public:
    DemandSystem();

    // ISimulatable interface
    void tick(const ISimulationTime& time) override;
    int getPriority() const override { return 52; }
    const char* getName() const override { return "DemandSystem"; }

    // IDemandProvider interface (from building::IDemandProvider)
    float get_demand(uint8_t zone_type, uint32_t player_id) const override;
    uint32_t get_demand_cap(uint8_t zone_type, uint32_t player_id) const override;
    bool has_positive_demand(uint8_t zone_type, uint32_t player_id) const override;

    /**
     * @brief Get const reference to demand data for a player.
     * @param player_id Player ID (0-3).
     * @return Const reference to player's DemandData. Returns empty data if invalid.
     */
    const DemandData& get_demand_data(uint8_t player_id) const;

    /**
     * @brief Get mutable reference to demand data for a player.
     * @param player_id Player ID (0-3).
     * @return Mutable reference to player's DemandData. Returns slot 0 if invalid.
     */
    DemandData& get_demand_data_mut(uint8_t player_id);

    // Player management

    /**
     * @brief Activate a player slot and reset its demand data.
     * @param player_id Player ID (0-3). No-op if out of range.
     */
    void add_player(uint8_t player_id);

    /**
     * @brief Deactivate a player slot and reset its demand data.
     * @param player_id Player ID (0-3). No-op if out of range.
     */
    void remove_player(uint8_t player_id);

    /**
     * @brief Check if a player slot is active.
     * @param player_id Player ID (0-3).
     * @return true if the player is active, false otherwise.
     */
    bool has_player(uint8_t player_id) const;

    /// Demand update frequency: every 5 ticks (250ms at 20 Hz)
    static constexpr uint32_t DEMAND_CYCLE_TICKS = 5;

private:
    /**
     * @brief Update demand calculations for a single player.
     *
     * Stub implementation; actual formulas come in E10-043/044/045.
     *
     * @param player_id Player ID to update.
     * @param time Current simulation time.
     */
    void update_demand(uint8_t player_id, const ISimulationTime& time);

    /// Maximum number of concurrent players
    static constexpr uint8_t MAX_PLAYERS = 4;

    /// Per-player demand data
    DemandData m_demand[MAX_PLAYERS];

    /// Per-player active flags
    bool m_player_active[MAX_PLAYERS] = {};

    /// Static default data for invalid queries
    static const DemandData s_empty_demand;
};

} // namespace demand
} // namespace sims3000

#endif // SIMS3000_DEMAND_DEMANDSYSTEM_H
