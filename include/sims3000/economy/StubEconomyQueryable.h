/**
 * @file StubEconomyQueryable.h
 * @brief Stub implementation of IEconomyQueryable for testing (Epic 10, Epic 11)
 *
 * Provides a permissive default implementation that returns fixed values
 * for all queries. Used for testing systems that depend on economy data
 * before the full economy system is wired in.
 *
 * @see E10-113, E11-005
 */

#ifndef SIMS3000_ECONOMY_STUBECONOMYQUERYABLE_H
#define SIMS3000_ECONOMY_STUBECONOMYQUERYABLE_H

#include <sims3000/economy/IEconomyQueryable.h>

namespace sims3000 {
namespace economy {

/**
 * @class StubEconomyQueryable
 * @brief Stub IEconomyQueryable that returns fixed default values.
 *
 * Permissive default implementation for use in tests and early development
 * before the real economy system is available.
 *
 * Default values:
 * - Tribute rates: 7% for all zone types
 * - Treasury balance: 20000 credits
 * - Funding levels: 100%
 * - Income/expense: 0
 * - Debt: 0, no bonds
 */
class StubEconomyQueryable : public IEconomyQueryable {
public:
    // -----------------------------------------------------------------------
    // Tribute rate queries
    // -----------------------------------------------------------------------

    /**
     * @brief Get the tribute rate for a zone type (always returns 7.0%).
     * @param zone_type Zone type (ignored in stub).
     * @return 7.0f (default 7% tribute rate).
     */
    float get_tribute_rate(uint8_t zone_type) const override {
        (void)zone_type;
        return 7.0f; // default 7%
    }

    /**
     * @brief Get the tribute rate for a zone type and player (always returns 7.0%).
     * @param zone_type Zone type (ignored in stub).
     * @param player_id Player ID (ignored in stub).
     * @return 7.0f (default 7% tribute rate).
     */
    float get_tribute_rate(uint8_t zone_type, uint8_t player_id) const override {
        (void)zone_type;
        (void)player_id;
        return 7.0f;
    }

    /**
     * @brief Get the average tribute rate (always returns 7.0%).
     * @return 7.0f (default 7% average tribute rate).
     */
    float get_average_tribute_rate() const override {
        return 7.0f;
    }

    // -----------------------------------------------------------------------
    // Treasury queries
    // -----------------------------------------------------------------------

    int64_t get_treasury_balance(uint8_t player_id) const override {
        (void)player_id;
        return 20000; // default starting balance
    }

    bool can_afford(int64_t amount, uint8_t player_id) const override {
        (void)player_id;
        return amount <= 20000; // based on default balance
    }

    // -----------------------------------------------------------------------
    // Funding queries
    // -----------------------------------------------------------------------

    uint8_t get_funding_level(uint8_t service_type, uint8_t player_id) const override {
        (void)service_type;
        (void)player_id;
        return 100; // default 100% funding
    }

    // -----------------------------------------------------------------------
    // Statistics queries
    // -----------------------------------------------------------------------

    int64_t get_last_income(uint8_t player_id) const override {
        (void)player_id;
        return 0;
    }

    int64_t get_last_expense(uint8_t player_id) const override {
        (void)player_id;
        return 0;
    }

    // -----------------------------------------------------------------------
    // Bond queries
    // -----------------------------------------------------------------------

    int64_t get_total_debt(uint8_t player_id) const override {
        (void)player_id;
        return 0;
    }

    int get_bond_count(uint8_t player_id) const override {
        (void)player_id;
        return 0;
    }

    bool can_issue_bond(uint8_t player_id) const override {
        (void)player_id;
        return true; // stub always allows bond issuance
    }
};

} // namespace economy
} // namespace sims3000

#endif // SIMS3000_ECONOMY_STUBECONOMYQUERYABLE_H
