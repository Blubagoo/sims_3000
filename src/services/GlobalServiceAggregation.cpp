/**
 * @file GlobalServiceAggregation.cpp
 * @brief Implementation of capacity-based global service effectiveness (Ticket E9-023)
 *
 * @see GlobalServiceAggregation.h for interface documentation
 */

#include <sims3000/services/GlobalServiceAggregation.h>
#include <sims3000/services/ServiceConfigs.h>
#include <sims3000/services/FundingModifier.h>
#include <algorithm>

namespace sims3000 {
namespace services {

uint32_t get_beings_per_unit(ServiceType type) {
    switch (type) {
        case ServiceType::Medical:   return BEINGS_PER_MEDICAL_UNIT;
        case ServiceType::Education: return BEINGS_PER_EDUCATION_UNIT;
        default:                     return 0;  // Radius-based services
    }
}

GlobalServiceData calculate_global_service(
    ServiceType type,
    const std::vector<ServiceBuildingData>& buildings,
    uint32_t population,
    uint8_t funding_percent)
{
    GlobalServiceData result;

    // Sum capacity from all active buildings of the matching type
    for (const auto& building : buildings) {
        if (building.is_active && building.type == type) {
            result.total_capacity += building.capacity;
        }
    }

    // If population is zero, full effectiveness (nothing to serve)
    if (population == 0) {
        result.effectiveness = 1.0f;
        return result;
    }

    // If no capacity, zero effectiveness
    if (result.total_capacity == 0) {
        result.effectiveness = 0.0f;
        return result;
    }

    // Calculate raw effectiveness: total_capacity / population
    float raw_effectiveness = static_cast<float>(result.total_capacity)
                            / static_cast<float>(population);

    // Apply funding modifier
    float funding_mod = calculate_funding_modifier(funding_percent);
    raw_effectiveness *= funding_mod;

    // Clamp to [0.0, 1.0]
    result.effectiveness = std::min(1.0f, std::max(0.0f, raw_effectiveness));

    return result;
}

} // namespace services
} // namespace sims3000
