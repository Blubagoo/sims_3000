/**
 * @file InfrastructureMaintenance.cpp
 * @brief Implementation of infrastructure maintenance calculations (Ticket E11-009)
 */

#include "sims3000/economy/InfrastructureMaintenance.h"
#include <cmath>

namespace sims3000 {
namespace economy {

int64_t calculate_infrastructure_cost(const InfrastructureMaintenanceInput& input) {
    // O(1) per entity: simple multiply + round
    double result = static_cast<double>(input.base_cost) * static_cast<double>(input.cost_multiplier);
    return static_cast<int64_t>(std::round(result));
}

int32_t get_infrastructure_maintenance_rate(InfrastructureType type) {
    switch (type) {
        case InfrastructureType::Pathway:       return MAINTENANCE_PATHWAY;
        case InfrastructureType::EnergyConduit: return MAINTENANCE_ENERGY_CONDUIT;
        case InfrastructureType::FluidConduit:  return MAINTENANCE_FLUID_CONDUIT;
        case InfrastructureType::RailTrack:     return MAINTENANCE_RAIL_TRACK;
        default:                                return 0;
    }
}

InfrastructureMaintenanceResult aggregate_infrastructure_maintenance(
    const std::vector<std::pair<InfrastructureType, int64_t>>& costs) {

    InfrastructureMaintenanceResult result = {0, 0, 0, 0, 0};

    for (const auto& entry : costs) {
        switch (entry.first) {
            case InfrastructureType::Pathway:
                result.pathway_cost += entry.second;
                break;
            case InfrastructureType::EnergyConduit:
                result.energy_conduit_cost += entry.second;
                break;
            case InfrastructureType::FluidConduit:
                result.fluid_conduit_cost += entry.second;
                break;
            case InfrastructureType::RailTrack:
                result.rail_track_cost += entry.second;
                break;
            default:
                break;
        }
    }

    result.total = result.pathway_cost
                 + result.energy_conduit_cost
                 + result.fluid_conduit_cost
                 + result.rail_track_cost;

    return result;
}

} // namespace economy
} // namespace sims3000
