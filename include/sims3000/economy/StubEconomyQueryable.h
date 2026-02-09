/**
 * @file StubEconomyQueryable.h
 * @brief Stub implementation of IEconomyQueryable for testing (Epic 10)
 *
 * Provides a permissive default implementation that returns a fixed 7%
 * tribute rate for all queries. Used for testing systems that depend on
 * economy data before the full economy system is implemented.
 *
 * @see E10-113
 */

#ifndef SIMS3000_ECONOMY_STUBECONOMYQUERYABLE_H
#define SIMS3000_ECONOMY_STUBECONOMYQUERYABLE_H

#include <sims3000/economy/IEconomyQueryable.h>

namespace sims3000 {
namespace economy {

/**
 * @class StubEconomyQueryable
 * @brief Stub IEconomyQueryable that returns fixed 7% tribute rates.
 *
 * Permissive default implementation for use in tests and early development
 * before the real economy system is available.
 */
class StubEconomyQueryable : public IEconomyQueryable {
public:
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
     * @brief Get the average tribute rate (always returns 7.0%).
     * @return 7.0f (default 7% average tribute rate).
     */
    float get_average_tribute_rate() const override {
        return 7.0f;
    }
};

} // namespace economy
} // namespace sims3000

#endif // SIMS3000_ECONOMY_STUBECONOMYQUERYABLE_H
