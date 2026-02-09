/**
 * @file ServiceMaintenance.cpp
 * @brief Implementation of service maintenance calculations (Ticket E11-010)
 */

#include "sims3000/economy/ServiceMaintenance.h"
#include <cmath>

namespace sims3000 {
namespace economy {

int32_t get_service_base_cost(uint8_t service_type) {
    switch (service_type) {
        case 0: return SERVICE_COST_ENFORCER;        // ServiceType::Enforcer
        case 1: return SERVICE_COST_HAZARD_RESPONSE; // ServiceType::HazardResponse
        case 2: return SERVICE_COST_MEDICAL;          // ServiceType::Medical
        case 3: return SERVICE_COST_EDUCATION;        // ServiceType::Education
        default: return 0;
    }
}

ServiceMaintenanceResult calculate_service_maintenance(const ServiceMaintenanceInput& input) {
    ServiceMaintenanceResult result;
    result.funding_factor = static_cast<float>(input.funding_level) / 100.0f;
    double scaled = static_cast<double>(input.base_cost) * static_cast<double>(result.funding_factor);
    result.actual_cost = static_cast<int64_t>(std::round(scaled));
    return result;
}

ServiceMaintenanceSummary aggregate_service_maintenance(
    const std::vector<std::pair<uint8_t, int64_t>>& costs) {

    ServiceMaintenanceSummary summary = {0, 0, 0, 0, 0};

    for (const auto& entry : costs) {
        switch (entry.first) {
            case 0: // Enforcer
                summary.enforcer_cost += entry.second;
                break;
            case 1: // HazardResponse
                summary.hazard_response_cost += entry.second;
                break;
            case 2: // Medical
                summary.medical_cost += entry.second;
                break;
            case 3: // Education
                summary.education_cost += entry.second;
                break;
            default:
                break;
        }
    }

    summary.total = summary.enforcer_cost
                  + summary.hazard_response_cost
                  + summary.medical_cost
                  + summary.education_cost;

    return summary;
}

} // namespace economy
} // namespace sims3000
