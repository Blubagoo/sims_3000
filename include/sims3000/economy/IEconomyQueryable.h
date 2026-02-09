/**
 * @file IEconomyQueryable.h
 * @brief Economy query interface for cross-system dependencies (Epic 10, Epic 11)
 *
 * Defines the IEconomyQueryable interface that allows other systems
 * (demand, services, etc.) to query tribute/tax rates, treasury state,
 * funding levels, and bond information without depending directly on
 * the economy system implementation.
 *
 * @see E10-113, E11-005
 */

#ifndef SIMS3000_ECONOMY_IECONOMYQUERYABLE_H
#define SIMS3000_ECONOMY_IECONOMYQUERYABLE_H

#include <cstdint>

namespace sims3000 {
namespace economy {

/**
 * @class IEconomyQueryable
 * @brief Abstract interface for querying economy information.
 *
 * Used by DemandSystem, ServiceSystem, and other systems to query
 * tribute rates, treasury state, funding levels, and bond data
 * without coupling to the full economy implementation.
 *
 * Implemented by EconomySystem (real) and StubEconomyQueryable (test).
 */
class IEconomyQueryable {
public:
    /// Virtual destructor for proper polymorphic cleanup
    virtual ~IEconomyQueryable() = default;

    // -----------------------------------------------------------------------
    // Tribute rate queries (original Epic 10 interface)
    // -----------------------------------------------------------------------

    /**
     * @brief Get the tribute (tax) rate for a specific zone type (player 0 default).
     * @param zone_type Zone type to query (0=habitation, 1=exchange, 2=fabrication).
     * @return Tribute rate as a percentage (e.g., 7.0 means 7%).
     */
    virtual float get_tribute_rate(uint8_t zone_type) const = 0;

    /**
     * @brief Get the tribute (tax) rate for a specific zone type and player.
     * @param zone_type Zone type to query (0=habitation, 1=exchange, 2=fabrication).
     * @param player_id Player ID (0-3).
     * @return Tribute rate as a percentage (e.g., 7.0 means 7%).
     */
    virtual float get_tribute_rate(uint8_t zone_type, uint8_t player_id) const = 0;

    /**
     * @brief Get the average tribute rate across all zone types.
     * @return Average tribute rate as a percentage.
     */
    virtual float get_average_tribute_rate() const = 0;

    // -----------------------------------------------------------------------
    // Treasury queries (Epic 11)
    // -----------------------------------------------------------------------

    /**
     * @brief Get the current treasury balance for a player.
     * @param player_id Player ID (0-3).
     * @return Current credit balance (can be negative if in debt).
     */
    virtual int64_t get_treasury_balance(uint8_t player_id) const = 0;

    /**
     * @brief Check if a player can afford a given amount.
     * @param amount Amount to check.
     * @param player_id Player ID (0-3).
     * @return true if player's balance >= amount.
     */
    virtual bool can_afford(int64_t amount, uint8_t player_id) const = 0;

    // -----------------------------------------------------------------------
    // Funding queries (Epic 11)
    // -----------------------------------------------------------------------

    /**
     * @brief Get the funding level for a service type.
     * @param service_type ServiceType value (0=Enforcer, 1=HazardResponse, 2=Medical, 3=Education).
     * @param player_id Player ID (0-3).
     * @return Funding level as a percentage (0-150).
     */
    virtual uint8_t get_funding_level(uint8_t service_type, uint8_t player_id) const = 0;

    // -----------------------------------------------------------------------
    // Statistics queries (Epic 11)
    // -----------------------------------------------------------------------

    /**
     * @brief Get total income from the last budget cycle.
     * @param player_id Player ID (0-3).
     * @return Last cycle's total income.
     */
    virtual int64_t get_last_income(uint8_t player_id) const = 0;

    /**
     * @brief Get total expense from the last budget cycle.
     * @param player_id Player ID (0-3).
     * @return Last cycle's total expense.
     */
    virtual int64_t get_last_expense(uint8_t player_id) const = 0;

    // -----------------------------------------------------------------------
    // Bond queries (Epic 11)
    // -----------------------------------------------------------------------

    /**
     * @brief Get total outstanding debt (sum of remaining principal on all bonds).
     * @param player_id Player ID (0-3).
     * @return Total debt amount.
     */
    virtual int64_t get_total_debt(uint8_t player_id) const = 0;

    /**
     * @brief Get the number of active bonds.
     * @param player_id Player ID (0-3).
     * @return Number of active bonds.
     */
    virtual int get_bond_count(uint8_t player_id) const = 0;

    /**
     * @brief Check if a player can issue another bond.
     * @param player_id Player ID (0-3).
     * @return true if bond count < MAX_BONDS_PER_PLAYER.
     */
    virtual bool can_issue_bond(uint8_t player_id) const = 0;
};

} // namespace economy
} // namespace sims3000

#endif // SIMS3000_ECONOMY_IECONOMYQUERYABLE_H
