/**
 * @file EconomySystem.h
 * @brief Economy simulation system skeleton (Ticket E11-004, E11-005)
 *
 * Manages per-player treasury state, budget cycles, and provides
 * economy query and credit provider interfaces.
 *
 * Runs at tick_priority 60 with frequency-gated budget cycles
 * every BUDGET_CYCLE_TICKS (200 ticks = 10s at 20Hz).
 */

#ifndef SIMS3000_ECONOMY_ECONOMYSYSTEM_H
#define SIMS3000_ECONOMY_ECONOMYSYSTEM_H

#include <array>
#include <cstdint>

#include "sims3000/core/ISimulatable.h"
#include "sims3000/core/types.h"
#include "sims3000/economy/IEconomyQueryable.h"
#include "sims3000/economy/TreasuryState.h"
#include "sims3000/economy/RevenueTracking.h"
#include "sims3000/economy/ExpenseTracking.h"
#include "sims3000/economy/Ordinance.h"
#include "sims3000/building/ForwardDependencyInterfaces.h"

namespace sims3000 {
namespace economy {

/**
 * @class EconomySystem
 * @brief Manages city finances, budget cycles, and economic queries.
 *
 * Each active player has a TreasuryState instance. The tick() method
 * runs frequency-gated budget cycles every BUDGET_CYCLE_TICKS.
 *
 * Implements:
 * - ISimulatable: participates in simulation ticks at priority 60
 * - IEconomyQueryable: provides economy data to other systems
 * - ICreditProvider: allows building system to deduct/check credits
 */
class EconomySystem : public ISimulatable,
                      public IEconomyQueryable,
                      public building::ICreditProvider {
public:
    static constexpr uint8_t MAX_PLAYERS = 4;

    /// Budget cycle frequency: every 200 ticks (10 seconds at 20 Hz)
    static constexpr uint32_t BUDGET_CYCLE_TICKS = 200;

    EconomySystem();

    // -----------------------------------------------------------------------
    // ISimulatable interface
    // -----------------------------------------------------------------------

    void tick(const ISimulationTime& time) override;
    int getPriority() const override { return 60; }
    const char* getName() const override { return "EconomySystem"; }

    // -----------------------------------------------------------------------
    // Treasury access (per player)
    // -----------------------------------------------------------------------

    /**
     * @brief Get mutable reference to a player's treasury.
     * @param player_id Player ID (0-3).
     * @return Reference to TreasuryState. Returns player 0's treasury if invalid.
     */
    TreasuryState& get_treasury(uint8_t player_id);

    /**
     * @brief Get const reference to a player's treasury.
     * @param player_id Player ID (0-3).
     * @return Const reference to TreasuryState. Returns empty treasury if invalid.
     */
    const TreasuryState& get_treasury(uint8_t player_id) const;

    // -----------------------------------------------------------------------
    // Player activation
    // -----------------------------------------------------------------------

    /**
     * @brief Activate a player slot (initializes treasury to defaults).
     * @param player_id Player ID to activate (0-3).
     */
    void activate_player(uint8_t player_id);

    /**
     * @brief Check if a player slot is active.
     * @param player_id Player ID to check (0-3).
     * @return true if player is active.
     */
    bool is_player_active(uint8_t player_id) const;

    // -----------------------------------------------------------------------
    // IEconomyQueryable interface
    // -----------------------------------------------------------------------

    float get_tribute_rate(uint8_t zone_type) const override;
    float get_tribute_rate(uint8_t zone_type, uint8_t player_id) const override;
    float get_average_tribute_rate() const override;
    int64_t get_treasury_balance(uint8_t player_id) const override;
    bool can_afford(int64_t amount, uint8_t player_id) const override;
    uint8_t get_funding_level(uint8_t service_type, uint8_t player_id) const override;
    int64_t get_last_income(uint8_t player_id) const override;
    int64_t get_last_expense(uint8_t player_id) const override;
    int64_t get_total_debt(uint8_t player_id) const override;
    int get_bond_count(uint8_t player_id) const override;
    bool can_issue_bond(uint8_t player_id) const override;

    // -----------------------------------------------------------------------
    // ICreditProvider interface
    // -----------------------------------------------------------------------

    bool deduct_credits(std::uint32_t player_id, std::int64_t amount) override;
    bool has_credits(std::uint32_t player_id, std::int64_t amount) const override;

    // -----------------------------------------------------------------------
    // Budget cycle data input (called by integration layer each budget phase)
    // -----------------------------------------------------------------------

    /**
     * @brief Set pre-computed income breakdown for next budget cycle.
     *
     * The integration layer gathers tribute data from ECS and calls this
     * before the budget cycle tick fires.
     */
    void set_phase_income(uint8_t player_id, const IncomeBreakdown& income);

    /**
     * @brief Set pre-computed cost data for next budget cycle.
     *
     * The integration layer aggregates infrastructure/service/energy costs
     * from ECS and calls this before the budget cycle tick fires.
     */
    void set_phase_costs(uint8_t player_id, int64_t infra_cost,
                         int64_t service_cost, int64_t energy_cost);

    // -----------------------------------------------------------------------
    // Ordinance and history access
    // -----------------------------------------------------------------------

    OrdinanceState& get_ordinances(uint8_t player_id);
    const OrdinanceState& get_ordinances(uint8_t player_id) const;
    const IncomeHistory& get_income_history(uint8_t player_id) const;
    const ExpenseHistory& get_expense_history(uint8_t player_id) const;

private:
    std::array<TreasuryState, MAX_PLAYERS> m_treasuries;
    std::array<bool, MAX_PLAYERS> m_player_active{};

    /// Per-player cached income/expense data from integration layer
    std::array<IncomeBreakdown, MAX_PLAYERS> m_cached_income{};
    std::array<int64_t, MAX_PLAYERS> m_cached_infra_cost{};
    std::array<int64_t, MAX_PLAYERS> m_cached_service_cost{};
    std::array<int64_t, MAX_PLAYERS> m_cached_energy_cost{};

    /// Per-player ordinances and history
    std::array<OrdinanceState, MAX_PLAYERS> m_ordinances{};
    std::array<IncomeHistory, MAX_PLAYERS> m_income_history{};
    std::array<ExpenseHistory, MAX_PLAYERS> m_expense_history{};

    /// Static empty treasury for invalid const queries
    static const TreasuryState s_empty_treasury;
    static const OrdinanceState s_empty_ordinances;
    static const IncomeHistory s_empty_income_history;
    static const ExpenseHistory s_empty_expense_history;

    /// Process a complete budget cycle for one player.
    void process_budget_cycle(uint8_t player_id);
};

} // namespace economy
} // namespace sims3000

#endif // SIMS3000_ECONOMY_ECONOMYSYSTEM_H
