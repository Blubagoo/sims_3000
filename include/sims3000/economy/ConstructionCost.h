/**
 * @file ConstructionCost.h
 * @brief Construction cost constants and deduction logic (Ticket E11-020)
 *
 * Provides cost constants for all building/infrastructure types,
 * affordability check, and deduction from treasury.
 *
 * No system dependencies -- operates directly on TreasuryState.
 */

#ifndef SIMS3000_ECONOMY_CONSTRUCTIONCOST_H
#define SIMS3000_ECONOMY_CONSTRUCTIONCOST_H

#include <cstdint>
#include <sims3000/economy/TreasuryState.h>

namespace sims3000 {
namespace economy {

// ---------------------------------------------------------------------------
// Construction cost table (credits)
// ---------------------------------------------------------------------------
namespace construction_costs {

// Zone buildings (auto-constructed, so cost is for zoning)
constexpr int64_t ZONE_HABITATION_LOW   = 100;
constexpr int64_t ZONE_HABITATION_HIGH  = 500;
constexpr int64_t ZONE_EXCHANGE_LOW     = 150;
constexpr int64_t ZONE_EXCHANGE_HIGH    = 750;
constexpr int64_t ZONE_FABRICATION_LOW  = 200;
constexpr int64_t ZONE_FABRICATION_HIGH = 1000;

// Infrastructure (per tile)
constexpr int64_t PATHWAY        = 10;
constexpr int64_t ENERGY_CONDUIT = 5;
constexpr int64_t FLUID_CONDUIT  = 8;
constexpr int64_t RAIL_TRACK     = 25;

// Service buildings
constexpr int64_t SERVICE_POST    = 500;
constexpr int64_t SERVICE_STATION = 2000;
constexpr int64_t SERVICE_NEXUS   = 5000;

} // namespace construction_costs

// ---------------------------------------------------------------------------
// Events
// ---------------------------------------------------------------------------

/**
 * @struct InsufficientFundsEvent
 * @brief Event data for when a construction is rejected due to insufficient funds.
 */
struct InsufficientFundsEvent {
    uint8_t player_id;  ///< Player who attempted construction
    int64_t cost;       ///< Cost of the attempted construction
    int64_t balance;    ///< Player's balance at time of attempt
};

// ---------------------------------------------------------------------------
// Result types
// ---------------------------------------------------------------------------

/**
 * @struct ConstructionCostResult
 * @brief Result of checking whether a player can afford a construction cost.
 */
struct ConstructionCostResult {
    bool can_afford;        ///< Whether the player can afford the cost
    int64_t cost;           ///< The cost checked
    int64_t balance_after;  ///< Balance after deduction (only meaningful if can_afford)
};

// ---------------------------------------------------------------------------
// Free functions
// ---------------------------------------------------------------------------

/**
 * @brief Check if a player can afford a construction cost.
 *
 * @param treasury The player's treasury state (read-only).
 * @param cost The construction cost to check.
 * @return ConstructionCostResult with affordability and projected balance.
 */
ConstructionCostResult check_construction_cost(const TreasuryState& treasury, int64_t cost);

/**
 * @brief Deduct a construction cost from the treasury.
 *
 * If the player can afford it (balance >= cost), subtracts cost from balance.
 *
 * @param treasury The player's treasury state (modified in place).
 * @param cost The construction cost to deduct.
 * @return true if deduction succeeded, false if insufficient funds.
 */
bool deduct_construction_cost(TreasuryState& treasury, int64_t cost);

} // namespace economy
} // namespace sims3000

#endif // SIMS3000_ECONOMY_CONSTRUCTIONCOST_H
