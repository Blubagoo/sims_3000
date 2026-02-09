/**
 * @file IEconomyQueryable.h
 * @brief Economy query interface for cross-system dependencies (Epic 10)
 *
 * Defines the IEconomyQueryable interface that allows other systems
 * (demand, services, etc.) to query tribute/tax rates without depending
 * directly on the economy system implementation.
 *
 * @see E10-113
 */

#ifndef SIMS3000_ECONOMY_IECONOMYQUERYABLE_H
#define SIMS3000_ECONOMY_IECONOMYQUERYABLE_H

#include <cstdint>

namespace sims3000 {
namespace economy {

/**
 * @class IEconomyQueryable
 * @brief Abstract interface for querying economy/tribute rate information.
 *
 * Used by DemandSystem and other systems to factor tribute rates into
 * their calculations without coupling to the full economy implementation.
 * Will be implemented by EconomySystem in a later epic.
 */
class IEconomyQueryable {
public:
    /// Virtual destructor for proper polymorphic cleanup
    virtual ~IEconomyQueryable() = default;

    /**
     * @brief Get the tribute (tax) rate for a specific zone type.
     * @param zone_type Zone type to query (0=habitation, 1=exchange, 2=fabrication).
     * @return Tribute rate as a percentage (e.g., 7.0 means 7%).
     */
    virtual float get_tribute_rate(uint8_t zone_type) const = 0;

    /**
     * @brief Get the average tribute rate across all zone types.
     * @return Average tribute rate as a percentage.
     */
    virtual float get_average_tribute_rate() const = 0;
};

} // namespace economy
} // namespace sims3000

#endif // SIMS3000_ECONOMY_IECONOMYQUERYABLE_H
