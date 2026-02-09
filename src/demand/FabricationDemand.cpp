/**
 * @file FabricationDemand.cpp
 * @brief Fabrication (industrial) zone demand formula implementation (E10-045)
 *
 * Pure calculation: no side effects, no external state.
 */

#include "sims3000/demand/FabricationDemand.h"
#include <algorithm>

namespace sims3000 {
namespace demand {

FabricationDemandResult calculate_fabrication_demand(const FabricationInputs& inputs) {
    FabricationDemandResult result;
    result.factors = DemandFactors{};

    // --- Population factor ---
    // target_fab_jobs = total_beings / 5 (1 fab job per 5 beings)
    const uint32_t target_fab_jobs = inputs.total_beings / 5;

    // fab_ratio = fabrication_jobs / max(1, target_fab_jobs)
    const float fab_ratio = static_cast<float>(inputs.fabrication_jobs)
        / static_cast<float>(std::max(1u, target_fab_jobs));

    // 20 * (1.0 - fab_ratio), clamped to -15..+20
    const float pop_f = 20.0f * (1.0f - fab_ratio);
    result.factors.population_factor = static_cast<int8_t>(std::clamp(
        static_cast<int>(pop_f), -15, 20));

    // --- Employment factor ---
    // labor surplus = labor_force - employed_laborers
    // +15 * (surplus / max(1, labor_force)), clamped to -10..+15
    const int32_t surplus = static_cast<int32_t>(inputs.labor_force)
        - static_cast<int32_t>(inputs.employed_laborers);
    const float surplus_ratio = static_cast<float>(surplus)
        / static_cast<float>(std::max(1u, inputs.labor_force));
    const float emp_f = 15.0f * surplus_ratio;
    result.factors.employment_factor = static_cast<int8_t>(std::clamp(
        static_cast<int>(emp_f), -10, 15));

    // --- Transport factor ---
    // has_external_connectivity ? +5 : -10; minus congestion/10, clamped to -15..+10
    const float connectivity_base = inputs.has_external_connectivity ? 5.0f : -10.0f;
    const float trans_f = connectivity_base - inputs.congestion_level / 10.0f;
    result.factors.transport_factor = static_cast<int8_t>(std::clamp(
        static_cast<int>(trans_f), -15, 10));

    // --- Contamination factor ---
    // 0 (fabrication is contamination-tolerant)
    result.factors.contamination_factor = 0;

    // --- Final demand ---
    const int sum = static_cast<int>(result.factors.population_factor)
        + static_cast<int>(result.factors.employment_factor)
        + static_cast<int>(result.factors.transport_factor)
        + static_cast<int>(result.factors.contamination_factor);

    result.demand = static_cast<int8_t>(std::clamp(sum, -100, 100));

    return result;
}

} // namespace demand
} // namespace sims3000
