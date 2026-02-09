/**
 * @file AttractivenessCalculation.h
 * @brief City attractiveness calculation for migration (Ticket E10-024)
 *
 * Computes net attractiveness of a city based on weighted positive
 * and negative migration factors. Used to determine migration pressure.
 */

#ifndef SIMS3000_POPULATION_ATTRACTIVENESSCALCULATION_H
#define SIMS3000_POPULATION_ATTRACTIVENESSCALCULATION_H

#include <cstdint>
#include "sims3000/population/MigrationFactors.h"

namespace sims3000 {
namespace population {

namespace constants {
    constexpr float JOB_WEIGHT = 0.20f;              ///< Weight for job availability
    constexpr float HOUSING_WEIGHT = 0.15f;          ///< Weight for housing availability
    constexpr float SECTOR_VALUE_WEIGHT = 0.10f;     ///< Weight for sector/land value
    constexpr float SERVICE_WEIGHT = 0.15f;          ///< Weight for service coverage
    constexpr float HARMONY_WEIGHT = 0.15f;          ///< Weight for social harmony
    constexpr float DISORDER_WEIGHT = -0.10f;        ///< Weight for disorder (negative)
    constexpr float CONTAMINATION_WEIGHT = -0.10f;   ///< Weight for contamination (negative)
    constexpr float TRIBUTE_WEIGHT = -0.03f;         ///< Weight for tribute/tax burden (negative)
    constexpr float CONGESTION_WEIGHT = -0.02f;      ///< Weight for congestion (negative)
} // namespace constants

/**
 * @struct AttractivenessResult
 * @brief Result of city attractiveness calculation.
 *
 * Contains net attraction score and the weighted positive/negative components.
 */
struct AttractivenessResult {
    int8_t net_attraction;      ///< Net city attractiveness (-100 to +100)
    float weighted_positive;    ///< Sum of weighted positive factors
    float weighted_negative;    ///< Sum of weighted negative factors
};

/**
 * @brief Calculate city attractiveness from migration factors.
 *
 * Computes a weighted sum of positive and negative factors:
 * - positive = job * JOB_W + housing * HOUSING_W + sector_value * SECTOR_W
 *              + service * SERVICE_W + harmony * HARMONY_W
 * - negative = disorder * |DISORDER_W| + contamination * |CONTAM_W|
 *              + tribute * |TRIBUTE_W| + congestion * |CONGESTION_W|
 * - net = clamp(positive - negative, -100, 100)
 *
 * All factor values are on a 0-100 scale.
 *
 * @param factors Migration factors for the city
 * @return AttractivenessResult with net attraction and component sums
 */
AttractivenessResult calculate_attractiveness(const MigrationFactors& factors);

} // namespace population
} // namespace sims3000

#endif // SIMS3000_POPULATION_ATTRACTIVENESSCALCULATION_H
