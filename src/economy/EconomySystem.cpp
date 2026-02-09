/**
 * @file EconomySystem.cpp
 * @brief Economy simulation system implementation (Tickets E11-004, E11-005)
 *
 * Skeleton implementation with frequency-gated budget cycles.
 * Implements ISimulatable, IEconomyQueryable, and ICreditProvider.
 */

#include "sims3000/economy/EconomySystem.h"
#include "sims3000/economy/CreditAdvance.h"

namespace sims3000 {
namespace economy {

// Static empty treasury for invalid const queries
const TreasuryState EconomySystem::s_empty_treasury = TreasuryState{};

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
// ICreditProvider
// ---------------------------------------------------------------------------

bool EconomySystem::deduct_credits(std::uint32_t player_id, std::int64_t amount) {
    if (player_id >= MAX_PLAYERS) {
        return false;
    }
    // Allow deficit: always deduct and return true
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
// Budget cycle (stub for later tickets)
// ---------------------------------------------------------------------------

void EconomySystem::process_budget_cycle(uint8_t /*player_id*/) {
    // Stub: budget cycle processing (filled in by later tickets)
}

} // namespace economy
} // namespace sims3000
