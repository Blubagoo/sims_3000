/**
 * @file EconomySystem.cpp
 * @brief Economy simulation system implementation (Tickets E11-004, E11-005)
 *
 * Implements the full budget cycle: income collection, expense deduction,
 * bond processing, deficit handling, and emergency bond auto-issuance.
 *
 * Follow-up fixes:
 * - E11-GD-001: Wired process_budget_cycle to call all calculation modules
 * - E11-GD-002: Fixed deduct_credits to reject insufficient funds
 */

#include "sims3000/economy/EconomySystem.h"
#include "sims3000/economy/BudgetCycle.h"
#include "sims3000/economy/DeficitHandling.h"
#include "sims3000/economy/EmergencyBond.h"

namespace sims3000 {
namespace economy {

// Static empty instances for invalid const queries
const TreasuryState EconomySystem::s_empty_treasury = TreasuryState{};
const OrdinanceState EconomySystem::s_empty_ordinances = OrdinanceState{};
const IncomeHistory EconomySystem::s_empty_income_history = IncomeHistory{};
const ExpenseHistory EconomySystem::s_empty_expense_history = ExpenseHistory{};

EconomySystem::EconomySystem() {
    for (uint8_t i = 0; i < MAX_PLAYERS; ++i) {
        m_treasuries[i] = TreasuryState{};
        m_player_active[i] = false;
    }
}

// ---------------------------------------------------------------------------
// ISimulatable
// ---------------------------------------------------------------------------

void EconomySystem::tick(const ISimulationTime& time) {
    const SimulationTick current_tick = time.getCurrentTick();

    // Budget cycle runs every BUDGET_CYCLE_TICKS (200 ticks = 10s)
    if (current_tick > 0 && current_tick % BUDGET_CYCLE_TICKS == 0) {
        for (uint8_t i = 0; i < MAX_PLAYERS; ++i) {
            if (m_player_active[i]) {
                process_budget_cycle(i);
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Treasury access
// ---------------------------------------------------------------------------

TreasuryState& EconomySystem::get_treasury(uint8_t player_id) {
    if (player_id >= MAX_PLAYERS) {
        return m_treasuries[0];
    }
    return m_treasuries[player_id];
}

const TreasuryState& EconomySystem::get_treasury(uint8_t player_id) const {
    if (player_id >= MAX_PLAYERS || !m_player_active[player_id]) {
        return s_empty_treasury;
    }
    return m_treasuries[player_id];
}

// ---------------------------------------------------------------------------
// Player activation
// ---------------------------------------------------------------------------

void EconomySystem::activate_player(uint8_t player_id) {
    if (player_id >= MAX_PLAYERS) {
        return;
    }
    m_player_active[player_id] = true;
    m_treasuries[player_id] = TreasuryState{}; // reset to defaults
    m_cached_income[player_id] = IncomeBreakdown{};
    m_cached_infra_cost[player_id] = 0;
    m_cached_service_cost[player_id] = 0;
    m_cached_energy_cost[player_id] = 0;
    m_ordinances[player_id] = OrdinanceState{};
    m_income_history[player_id] = IncomeHistory{};
    m_expense_history[player_id] = ExpenseHistory{};
}

bool EconomySystem::is_player_active(uint8_t player_id) const {
    if (player_id >= MAX_PLAYERS) {
        return false;
    }
    return m_player_active[player_id];
}

// ---------------------------------------------------------------------------
// IEconomyQueryable: Tribute rates
// ---------------------------------------------------------------------------

float EconomySystem::get_tribute_rate(uint8_t zone_type) const {
    // Default overload uses player 0
    return get_tribute_rate(zone_type, 0);
}

float EconomySystem::get_tribute_rate(uint8_t zone_type, uint8_t player_id) const {
    if (player_id >= MAX_PLAYERS) {
        return 7.0f; // default fallback
    }
    const TreasuryState& treasury = m_treasuries[player_id];
    switch (zone_type) {
        case 0: return static_cast<float>(treasury.tribute_rate_habitation);
        case 1: return static_cast<float>(treasury.tribute_rate_exchange);
        case 2: return static_cast<float>(treasury.tribute_rate_fabrication);
        default: return 7.0f; // unknown zone type, return default
    }
}

float EconomySystem::get_average_tribute_rate() const {
    // Average across all zone types for player 0
    const TreasuryState& treasury = m_treasuries[0];
    float sum = static_cast<float>(treasury.tribute_rate_habitation)
              + static_cast<float>(treasury.tribute_rate_exchange)
              + static_cast<float>(treasury.tribute_rate_fabrication);
    return sum / 3.0f;
}

// ---------------------------------------------------------------------------
// IEconomyQueryable: Treasury queries
// ---------------------------------------------------------------------------

int64_t EconomySystem::get_treasury_balance(uint8_t player_id) const {
    if (player_id >= MAX_PLAYERS) {
        return 0;
    }
    return m_treasuries[player_id].balance;
}

bool EconomySystem::can_afford(int64_t amount, uint8_t player_id) const {
    if (player_id >= MAX_PLAYERS) {
        return false;
    }
    return m_treasuries[player_id].balance >= amount;
}

// ---------------------------------------------------------------------------
// IEconomyQueryable: Funding queries
// ---------------------------------------------------------------------------

uint8_t EconomySystem::get_funding_level(uint8_t service_type, uint8_t player_id) const {
    if (player_id >= MAX_PLAYERS) {
        return 100; // default funding
    }
    const TreasuryState& treasury = m_treasuries[player_id];
    switch (service_type) {
        case 0: return treasury.funding_enforcer;        // Enforcer
        case 1: return treasury.funding_hazard_response;  // HazardResponse
        case 2: return treasury.funding_medical;          // Medical
        case 3: return treasury.funding_education;        // Education
        default: return 100; // unknown service, default
    }
}

// ---------------------------------------------------------------------------
// IEconomyQueryable: Statistics queries
// ---------------------------------------------------------------------------

int64_t EconomySystem::get_last_income(uint8_t player_id) const {
    if (player_id >= MAX_PLAYERS) {
        return 0;
    }
    return m_treasuries[player_id].last_income;
}

int64_t EconomySystem::get_last_expense(uint8_t player_id) const {
    if (player_id >= MAX_PLAYERS) {
        return 0;
    }
    return m_treasuries[player_id].last_expense;
}

// ---------------------------------------------------------------------------
// IEconomyQueryable: Bond queries
// ---------------------------------------------------------------------------

int64_t EconomySystem::get_total_debt(uint8_t player_id) const {
    if (player_id >= MAX_PLAYERS) {
        return 0;
    }
    int64_t total = 0;
    for (const auto& bond : m_treasuries[player_id].active_bonds) {
        total += bond.remaining_principal;
    }
    return total;
}

int EconomySystem::get_bond_count(uint8_t player_id) const {
    if (player_id >= MAX_PLAYERS) {
        return 0;
    }
    return static_cast<int>(m_treasuries[player_id].active_bonds.size());
}

bool EconomySystem::can_issue_bond(uint8_t player_id) const {
    if (player_id >= MAX_PLAYERS) {
        return false;
    }
    return static_cast<int>(m_treasuries[player_id].active_bonds.size()) < MAX_BONDS_PER_PLAYER;
}

// ---------------------------------------------------------------------------
// ICreditProvider (E11-GD-002: reject insufficient funds)
// ---------------------------------------------------------------------------

bool EconomySystem::deduct_credits(std::uint32_t player_id, std::int64_t amount) {
    if (player_id >= MAX_PLAYERS) {
        return false;
    }
    if (amount <= 0) {
        return false;
    }
    if (m_treasuries[player_id].balance < amount) {
        return false;
    }
    m_treasuries[player_id].balance -= amount;
    return true;
}

bool EconomySystem::has_credits(std::uint32_t player_id, std::int64_t amount) const {
    if (player_id >= MAX_PLAYERS) {
        return false;
    }
    return m_treasuries[player_id].balance >= amount;
}

// ---------------------------------------------------------------------------
// Budget cycle data input
// ---------------------------------------------------------------------------

void EconomySystem::set_phase_income(uint8_t player_id, const IncomeBreakdown& income) {
    if (player_id >= MAX_PLAYERS) return;
    m_cached_income[player_id] = income;
}

void EconomySystem::set_phase_costs(uint8_t player_id, int64_t infra_cost,
                                     int64_t service_cost, int64_t energy_cost) {
    if (player_id >= MAX_PLAYERS) return;
    m_cached_infra_cost[player_id] = infra_cost;
    m_cached_service_cost[player_id] = service_cost;
    m_cached_energy_cost[player_id] = energy_cost;
}

// ---------------------------------------------------------------------------
// Ordinance and history access
// ---------------------------------------------------------------------------

OrdinanceState& EconomySystem::get_ordinances(uint8_t player_id) {
    if (player_id >= MAX_PLAYERS) return m_ordinances[0];
    return m_ordinances[player_id];
}

const OrdinanceState& EconomySystem::get_ordinances(uint8_t player_id) const {
    if (player_id >= MAX_PLAYERS) return s_empty_ordinances;
    return m_ordinances[player_id];
}

const IncomeHistory& EconomySystem::get_income_history(uint8_t player_id) const {
    if (player_id >= MAX_PLAYERS) return s_empty_income_history;
    return m_income_history[player_id];
}

const ExpenseHistory& EconomySystem::get_expense_history(uint8_t player_id) const {
    if (player_id >= MAX_PLAYERS) return s_empty_expense_history;
    return m_expense_history[player_id];
}

// ---------------------------------------------------------------------------
// Budget cycle (E11-GD-001: fully wired)
// ---------------------------------------------------------------------------

void EconomySystem::process_budget_cycle(uint8_t player_id) {
    TreasuryState& treasury = m_treasuries[player_id];

    // 1. Process bond payments (mutates bonds, removes matured)
    BondPaymentResult bond_result = process_bond_payments(treasury.active_bonds);

    // 2. Get ordinance costs
    int64_t ordinance_cost = m_ordinances[player_id].get_total_cost();

    // 3. Build expense breakdown from cached costs + bond payments + ordinances
    InfrastructureMaintenanceResult infra_result{};
    infra_result.total = m_cached_infra_cost[player_id];

    ServiceMaintenanceSummary service_result{};
    service_result.total = m_cached_service_cost[player_id];

    ExpenseBreakdown expenses = build_expense_breakdown(
        infra_result, service_result,
        m_cached_energy_cost[player_id],
        bond_result.total_payment,
        ordinance_cost);

    // 4. Build budget cycle input
    BudgetCycleInput input;
    input.income = m_cached_income[player_id];
    input.expenses = expenses;

    // 5. Process the budget cycle (updates balance, breakdowns)
    /*BudgetCycleResult cycle_result =*/ economy::process_budget_cycle(treasury, input, player_id);

    // 6. Record history
    m_income_history[player_id].record(input.income.total);
    m_expense_history[player_id].record(expenses.total);

    // 7. Check deficit recovery (resets flags if balance >= 0)
    check_deficit_recovery(treasury);

    // 8. Check deficit status
    DeficitCheckResult deficit = check_deficit(treasury, player_id);
    if (deficit.should_warn || deficit.should_offer_bond) {
        apply_deficit_state(treasury, deficit);
    }

    // 9. Auto-issue emergency bond if needed
    if (deficit.should_offer_bond) {
        check_and_issue_emergency_bond(treasury, player_id);
    }
}

} // namespace economy
} // namespace sims3000
