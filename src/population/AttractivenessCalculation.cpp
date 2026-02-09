/**
 * @file AttractivenessCalculation.cpp
 * @brief City attractiveness calculation implementation (Ticket E10-024)
 *
 * Computes weighted sum of positive and negative migration factors
 * to produce a net city attractiveness score.
 */

#include "sims3000/population/AttractivenessCalculation.h"

#include <algorithm>
#include <cmath>

namespace sims3000 {
namespace population {

AttractivenessResult calculate_attractiveness(const MigrationFactors& factors) {
    AttractivenessResult result{};

    // Weighted positive factors (all 0-100 scale)
    result.weighted_positive =
        static_cast<float>(factors.job_availability)    * constants::JOB_WEIGHT +
        static_cast<float>(factors.housing_availability) * constants::HOUSING_WEIGHT +
        static_cast<float>(factors.sector_value_avg)    * constants::SECTOR_VALUE_WEIGHT +
        static_cast<float>(factors.service_coverage)    * constants::SERVICE_WEIGHT +
        static_cast<float>(factors.harmony_level)       * constants::HARMONY_WEIGHT;

    // Weighted negative factors (use absolute weight values since constants are negative)
    result.weighted_negative =
        static_cast<float>(factors.disorder_level)      * (-constants::DISORDER_WEIGHT) +
        static_cast<float>(factors.contamination_level) * (-constants::CONTAMINATION_WEIGHT) +
        static_cast<float>(factors.tribute_burden)      * (-constants::TRIBUTE_WEIGHT) +
        static_cast<float>(factors.congestion_level)    * (-constants::CONGESTION_WEIGHT);

    // Net attraction: clamp to [-100, +100]
    float net = result.weighted_positive - result.weighted_negative;
    net = std::max(-100.0f, std::min(100.0f, net));
    result.net_attraction = static_cast<int8_t>(std::round(net));

    return result;
}

} // namespace population
} // namespace sims3000
