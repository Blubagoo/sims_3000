/**
 * @file ExchangeDemand.h
 * @brief Exchange (commercial) zone demand formula (Ticket E10-044)
 *
 * Pure calculation function that computes commercial demand based on
 * exchange job coverage, unemployment rate, congestion level, and
 * tribute rate. Returns a demand value in [-100, +100] with a
 * breakdown of contributing factors.
 *
 * @see E10-044
 */

#ifndef SIMS3000_DEMAND_EXCHANGEDEMAND_H
#define SIMS3000_DEMAND_EXCHANGEDEMAND_H

#include <cstdint>
#include "sims3000/demand/DemandData.h"

namespace sims3000 {
namespace demand {

/**
 * @struct ExchangeInputs
 * @brief Input parameters for exchange demand calculation.
 */
struct ExchangeInputs {
    uint32_t total_beings = 0;          ///< Current population count
    uint32_t exchange_jobs = 0;         ///< Current exchange (commercial) job count
    uint8_t unemployment_rate = 0;      ///< Unemployment percentage (0-100)
    float congestion_level = 0.0f;      ///< Transport congestion (0-100)
    float tribute_rate = 7.0f;          ///< Tax/tribute rate percentage
};

/**
 * @struct ExchangeDemandResult
 * @brief Output of exchange demand calculation.
 */
struct ExchangeDemandResult {
    int8_t demand;          ///< Net demand value clamped to [-100, +100]
    DemandFactors factors;  ///< Breakdown of individual contributing factors
};

/**
 * @brief Calculate exchange (commercial) zone demand.
 *
 * Computes demand based on:
 * - Population factor: exchange job coverage ratio (under-served = positive)
 * - Employment factor: unemployment rate impact
 * - Transport factor: congestion penalty
 * - Tribute factor: tax rate impact (lower tribute = more demand)
 *
 * @param inputs Exchange demand input parameters.
 * @return ExchangeDemandResult with clamped demand and factor breakdown.
 */
ExchangeDemandResult calculate_exchange_demand(const ExchangeInputs& inputs);

} // namespace demand
} // namespace sims3000

#endif // SIMS3000_DEMAND_EXCHANGEDEMAND_H
