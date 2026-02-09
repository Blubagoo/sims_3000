/**
 * @file TributeCalculation.h
 * @brief Per-building tribute calculation engine (Ticket E11-007)
 *
 * Pure calculation module -- no ECS registry queries. The system layer
 * gathers the input data and calls these functions.
 *
 * Formula per building:
 *   occupancy_factor = current_occupancy / capacity  (0 if capacity == 0)
 *   value_factor     = 0.5 + (sector_value / 255.0) * 1.5  (range 0.5-2.0)
 *   rate_factor      = tribute_rate / 100.0  (range 0.0-0.2)
 *   tribute_amount   = (int64_t)(base_value * occupancy_factor
 *                                * value_factor * rate_factor
 *                                * tribute_modifier)
 */

#ifndef SIMS3000_ECONOMY_TRIBUTECALCULATION_H
#define SIMS3000_ECONOMY_TRIBUTECALCULATION_H

#include <cstdint>
#include <utility>
#include <vector>
#include <sims3000/economy/EconomyComponents.h>

namespace sims3000 {
namespace economy {

// ---------------------------------------------------------------------------
// Base tribute value constants
// ---------------------------------------------------------------------------
namespace constants {

constexpr uint32_t BASE_TRIBUTE_HABITATION_LOW   = 50;
constexpr uint32_t BASE_TRIBUTE_HABITATION_HIGH  = 200;
constexpr uint32_t BASE_TRIBUTE_EXCHANGE_LOW     = 100;
constexpr uint32_t BASE_TRIBUTE_EXCHANGE_HIGH    = 400;
constexpr uint32_t BASE_TRIBUTE_FABRICATION_LOW  = 75;
constexpr uint32_t BASE_TRIBUTE_FABRICATION_HIGH = 300;

} // namespace constants

// ---------------------------------------------------------------------------
// Per-building calculation
// ---------------------------------------------------------------------------

/**
 * @struct TributeInput
 * @brief All inputs needed to compute tribute for a single building.
 *
 * Gathered from TributableComponent, BuildingOccupancyComponent,
 * the land-value system, and TreasuryState by the calling system.
 */
struct TributeInput {
    uint32_t base_value        = 0;    ///< From TributableComponent
    uint8_t  density_level     = 0;    ///< 0=low, 1=high
    float    tribute_modifier  = 1.0f; ///< From TributableComponent
    uint16_t current_occupancy = 0;    ///< From BuildingOccupancyComponent
    uint16_t capacity          = 0;    ///< From BuildingOccupancyComponent
    uint8_t  sector_value      = 0;    ///< From LandValueSystem (0-255)
    uint8_t  tribute_rate      = 0;    ///< From TreasuryState (0-20%)
};

/**
 * @struct TributeResult
 * @brief Output of the per-building tribute formula.
 */
struct TributeResult {
    int64_t tribute_amount  = 0;    ///< Final tribute collected
    float   occupancy_factor = 0.0f; ///< 0.0-1.0
    float   value_factor     = 0.0f; ///< 0.5-2.0
    float   rate_factor      = 0.0f; ///< 0.0-0.2
};

/**
 * @brief Compute tribute for a single building.
 *
 * Pure function -- O(1) per call.
 *
 * @param input All building / zone / treasury parameters.
 * @return TributeResult with the computed amount and intermediate factors.
 */
TributeResult calculate_building_tribute(const TributeInput& input);

// ---------------------------------------------------------------------------
// Base value lookup
// ---------------------------------------------------------------------------

/**
 * @brief Return the canonical base tribute value for a zone type
 *        and density level.
 *
 * @param zone_type    Habitation, Exchange, or Fabrication.
 * @param density_level 0 = low, 1 = high.
 * @return Base tribute value (uint32_t).
 */
uint32_t get_base_tribute_value(ZoneBuildingType zone_type, uint8_t density_level);

// ---------------------------------------------------------------------------
// Aggregate
// ---------------------------------------------------------------------------

/**
 * @struct AggregateTributeResult
 * @brief Summed tribute amounts broken down by zone type.
 */
struct AggregateTributeResult {
    int64_t  habitation_total  = 0;
    int64_t  exchange_total    = 0;
    int64_t  fabrication_total = 0;
    int64_t  grand_total       = 0;
    uint32_t buildings_counted = 0;
};

/**
 * @brief Aggregate a collection of per-building tribute results.
 *
 * @param results Vector of (zone_type, tribute_amount) pairs.
 * @return AggregateTributeResult with per-zone and grand totals.
 */
AggregateTributeResult aggregate_tribute(
    const std::vector<std::pair<ZoneBuildingType, int64_t>>& results);

} // namespace economy
} // namespace sims3000

#endif // SIMS3000_ECONOMY_TRIBUTECALCULATION_H
