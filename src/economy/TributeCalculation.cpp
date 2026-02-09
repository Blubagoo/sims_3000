/**
 * @file TributeCalculation.cpp
 * @brief Implementation of per-building tribute calculation (Ticket E11-007)
 */

#include "sims3000/economy/TributeCalculation.h"

namespace sims3000 {
namespace economy {

// ---------------------------------------------------------------------------
// calculate_building_tribute
// ---------------------------------------------------------------------------
TributeResult calculate_building_tribute(const TributeInput& input) {
    TributeResult result;

    // occupancy_factor: 0 if capacity is 0, else current / capacity (0.0-1.0+)
    if (input.capacity == 0) {
        result.occupancy_factor = 0.0f;
    } else {
        result.occupancy_factor = static_cast<float>(input.current_occupancy)
                                / static_cast<float>(input.capacity);
    }

    // value_factor: 0.5 + (sector_value / 255.0) * 1.5  => range [0.5, 2.0]
    result.value_factor = 0.5f + (static_cast<float>(input.sector_value) / 255.0f) * 1.5f;

    // rate_factor: tribute_rate / 100.0  => range [0.0, 0.2] for valid rates
    result.rate_factor = static_cast<float>(input.tribute_rate) / 100.0f;

    // tribute_amount = base_value * occupancy_factor * value_factor
    //                  * rate_factor * tribute_modifier
    const double amount = static_cast<double>(input.base_value)
                        * static_cast<double>(result.occupancy_factor)
                        * static_cast<double>(result.value_factor)
                        * static_cast<double>(result.rate_factor)
                        * static_cast<double>(input.tribute_modifier);

    result.tribute_amount = static_cast<int64_t>(amount);

    return result;
}

// ---------------------------------------------------------------------------
// get_base_tribute_value
// ---------------------------------------------------------------------------
uint32_t get_base_tribute_value(ZoneBuildingType zone_type, uint8_t density_level) {
    switch (zone_type) {
        case ZoneBuildingType::Habitation:
            return (density_level >= 1) ? constants::BASE_TRIBUTE_HABITATION_HIGH
                                        : constants::BASE_TRIBUTE_HABITATION_LOW;
        case ZoneBuildingType::Exchange:
            return (density_level >= 1) ? constants::BASE_TRIBUTE_EXCHANGE_HIGH
                                        : constants::BASE_TRIBUTE_EXCHANGE_LOW;
        case ZoneBuildingType::Fabrication:
            return (density_level >= 1) ? constants::BASE_TRIBUTE_FABRICATION_HIGH
                                        : constants::BASE_TRIBUTE_FABRICATION_LOW;
    }
    // Safety fallback for invalid enum values
    return 0;
}

// ---------------------------------------------------------------------------
// aggregate_tribute
// ---------------------------------------------------------------------------
AggregateTributeResult aggregate_tribute(
    const std::vector<std::pair<ZoneBuildingType, int64_t>>& results) {
    AggregateTributeResult agg;

    for (const auto& entry : results) {
        switch (entry.first) {
            case ZoneBuildingType::Habitation:
                agg.habitation_total += entry.second;
                break;
            case ZoneBuildingType::Exchange:
                agg.exchange_total += entry.second;
                break;
            case ZoneBuildingType::Fabrication:
                agg.fabrication_total += entry.second;
                break;
        }
        agg.buildings_counted++;
    }

    agg.grand_total = agg.habitation_total
                    + agg.exchange_total
                    + agg.fabrication_total;

    return agg;
}

} // namespace economy
} // namespace sims3000
