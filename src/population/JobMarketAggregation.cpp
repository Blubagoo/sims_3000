/**
 * @file JobMarketAggregation.cpp
 * @brief Job market aggregation implementation (Ticket E10-020)
 *
 * Simple aggregation of job counts from building sector capacities.
 * BuildingSystem integration is deferred.
 */

#include "sims3000/population/JobMarketAggregation.h"

namespace sims3000 {
namespace population {

JobMarketResult aggregate_job_market(uint32_t exchange_building_capacity,
                                      uint32_t fabrication_building_capacity) {
    JobMarketResult result{};

    result.exchange_jobs = exchange_building_capacity;
    result.fabrication_jobs = fabrication_building_capacity;
    result.total_jobs = exchange_building_capacity + fabrication_building_capacity;

    return result;
}

} // namespace population
} // namespace sims3000
