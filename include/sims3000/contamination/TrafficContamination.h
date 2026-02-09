/**
 * @file TrafficContamination.h
 * @brief Traffic contamination generation from road congestion (Ticket E10-085).
 *
 * Road tiles with traffic produce contamination proportional to their
 * congestion level. Output is linearly interpolated between MIN and MAX.
 *
 * @see ContaminationGrid
 * @see ContaminationType
 * @see E10-085
 */

#ifndef SIMS3000_CONTAMINATION_TRAFFICCONTAMINATION_H
#define SIMS3000_CONTAMINATION_TRAFFICCONTAMINATION_H

#include <cstdint>
#include <vector>

#include "sims3000/contamination/ContaminationGrid.h"

namespace sims3000 {
namespace contamination {

/**
 * @struct TrafficSource
 * @brief Represents a road tile that generates traffic contamination.
 */
struct TrafficSource {
    int32_t x, y;      ///< Grid position
    float congestion;   ///< 0-1 congestion level
};

/// Minimum traffic contamination output (at zero congestion)
constexpr uint8_t TRAFFIC_CONTAMINATION_MIN = 5;

/// Maximum traffic contamination output (at full congestion)
constexpr uint8_t TRAFFIC_CONTAMINATION_MAX = 50;

/**
 * @brief Apply traffic contamination from road congestion to the grid.
 *
 * Per source: output = lerp(MIN, MAX, congestion).
 * Calls add_contamination(x, y, output, ContaminationType::Traffic).
 *
 * @param grid The contamination grid to write to.
 * @param sources Vector of traffic contamination sources.
 */
void apply_traffic_contamination(ContaminationGrid& grid,
                                  const std::vector<TrafficSource>& sources);

} // namespace contamination
} // namespace sims3000

#endif // SIMS3000_CONTAMINATION_TRAFFICCONTAMINATION_H
