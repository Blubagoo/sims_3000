/**
 * @file JobMarketAggregation.h
 * @brief Job market aggregation from building capacities (Ticket E10-020)
 *
 * Aggregates job counts from building sector capacities into a total
 * job market summary. BuildingSystem integration is deferred; for now
 * takes direct capacity values.
 */

#ifndef SIMS3000_POPULATION_JOBMARKETAGGREGATION_H
#define SIMS3000_POPULATION_JOBMARKETAGGREGATION_H

#include <cstdint>

namespace sims3000 {
namespace population {

/**
 * @struct JobMarketResult
 * @brief Aggregated job market totals by sector.
 *
 * Contains exchange (commercial) jobs, fabrication (industrial) jobs,
 * and total job count.
 */
struct JobMarketResult {
    uint32_t exchange_jobs;     ///< Commercial sector jobs
    uint32_t fabrication_jobs;  ///< Industrial sector jobs
    uint32_t total_jobs;        ///< Total available jobs (exchange + fabrication)
};

/**
 * @brief Aggregate jobs from building capacities.
 *
 * Simple aggregation: exchange_jobs = exchange_building_capacity,
 * fabrication_jobs = fabrication_building_capacity,
 * total_jobs = exchange_jobs + fabrication_jobs.
 *
 * BuildingSystem integration is deferred; this function currently
 * takes direct capacity values as input.
 *
 * @param exchange_building_capacity Total capacity of commercial buildings
 * @param fabrication_building_capacity Total capacity of industrial buildings
 * @return JobMarketResult with per-sector and total job counts
 */
JobMarketResult aggregate_job_market(uint32_t exchange_building_capacity,
                                      uint32_t fabrication_building_capacity);

} // namespace population
} // namespace sims3000

#endif // SIMS3000_POPULATION_JOBMARKETAGGREGATION_H
