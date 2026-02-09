/**
 * @file HabitationDemand.cpp
 * @brief Habitation (residential) zone demand formula implementation (E10-043)
 *
 * Pure calculation: no side effects, no external state.
 */

#include "sims3000/demand/HabitationDemand.h"
#include <algorithm>

namespace sims3000 {
namespace demand {

HabitationDemandResult calculate_habitation_demand(const HabitationInputs& inputs) {
    HabitationDemandResult result;
    result.factors = DemandFactors{};

    // --- Population factor ---
    // occupancy_ratio = total_beings / max(1, housing_capacity)
    const float occupancy_ratio = static_cast<float>(inputs.total_beings)
        / static_cast<float>(std::max(1u, inputs.housing_capacity));

    // +30 when occupancy > 0.9, -10 when < 0.5, linear between
    float pop_f;
    if (occupancy_ratio >= 0.9f) {
        pop_f = 30.0f;
    } else if (occupancy_ratio <= 0.5f) {
        pop_f = -10.0f;
    } else {
        // Linear interpolation from -10 at 0.5 to +30 at 0.9
        // slope = (30 - (-10)) / (0.9 - 0.5) = 40 / 0.4 = 100
        pop_f = -10.0f + (occupancy_ratio - 0.5f) * 100.0f;
    }
    result.factors.population_factor = static_cast<int8_t>(std::clamp(
        static_cast<int>(pop_f), -10, 30));

    // --- Employment factor ---
    // +20 when total_jobs > labor_force, -15 when labor_force > total_jobs * 2, linear between
    const float jobs = static_cast<float>(inputs.total_jobs);
    const float labor = static_cast<float>(inputs.labor_force);

    float emp_f;
    if (jobs >= labor) {
        emp_f = 20.0f;
    } else if (labor >= jobs * 2.0f) {
        emp_f = -15.0f;
    } else {
        // Linear interpolation from -15 at labor == 2*jobs to +20 at jobs == labor
        // Let ratio = jobs / max(1, labor). At ratio=0.5 -> -15, at ratio=1.0 -> +20
        const float ratio = jobs / std::max(1.0f, labor);
        // slope = (20 - (-15)) / (1.0 - 0.5) = 35 / 0.5 = 70
        emp_f = -15.0f + (ratio - 0.5f) * 70.0f;
    }
    result.factors.employment_factor = static_cast<int8_t>(std::clamp(
        static_cast<int>(emp_f), -15, 20));

    // --- Services factor ---
    // (service_coverage - 50) / 5, clamped to -10..+10
    const float svc_f = (inputs.service_coverage - 50.0f) / 5.0f;
    result.factors.services_factor = static_cast<int8_t>(std::clamp(
        static_cast<int>(svc_f), -10, 10));

    // --- Tribute factor ---
    // (7.0 - tribute_rate) * 3, clamped to -15..+15 (lower tribute = more demand)
    const float trib_f = (7.0f - inputs.tribute_rate) * 3.0f;
    result.factors.tribute_factor = static_cast<int8_t>(std::clamp(
        static_cast<int>(trib_f), -15, 15));

    // --- Contamination factor ---
    // -contamination_level / 5, clamped to -20..0
    const float cont_f = -inputs.contamination_level / 5.0f;
    result.factors.contamination_factor = static_cast<int8_t>(std::clamp(
        static_cast<int>(cont_f), -20, 0));

    // --- Final demand ---
    const int sum = static_cast<int>(result.factors.population_factor)
        + static_cast<int>(result.factors.employment_factor)
        + static_cast<int>(result.factors.services_factor)
        + static_cast<int>(result.factors.tribute_factor)
        + static_cast<int>(result.factors.contamination_factor);

    result.demand = static_cast<int8_t>(std::clamp(sum, -100, 100));

    return result;
}

} // namespace demand
} // namespace sims3000
