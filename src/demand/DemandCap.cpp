/**
 * @file DemandCap.cpp
 * @brief Demand capacity cap calculation implementation (E10-046)
 *
 * Pure calculation: no side effects, no external state.
 */

#include "sims3000/demand/DemandCap.h"

namespace sims3000 {
namespace demand {

DemandCapResult calculate_demand_caps(const DemandCapInputs& inputs) {
    DemandCapResult result;

    // habitation_cap = housing_capacity * energy_factor * fluid_factor
    result.habitation_cap = static_cast<uint32_t>(
        static_cast<float>(inputs.housing_capacity)
        * inputs.energy_factor
        * inputs.fluid_factor);

    // exchange_cap = exchange_jobs * transport_factor
    result.exchange_cap = static_cast<uint32_t>(
        static_cast<float>(inputs.exchange_jobs)
        * inputs.transport_factor);

    // fabrication_cap = fabrication_jobs * transport_factor
    result.fabrication_cap = static_cast<uint32_t>(
        static_cast<float>(inputs.fabrication_jobs)
        * inputs.transport_factor);

    return result;
}

} // namespace demand
} // namespace sims3000
