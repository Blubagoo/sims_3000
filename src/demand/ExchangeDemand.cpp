/**
 * @file ExchangeDemand.cpp
 * @brief Exchange (commercial) zone demand formula implementation (E10-044)
 *
 * Pure calculation: no side effects, no external state.
 */

#include "sims3000/demand/ExchangeDemand.h"
#include <algorithm>

namespace sims3000 {
namespace demand {

ExchangeDemandResult calculate_exchange_demand(const ExchangeInputs& inputs) {
    ExchangeDemandResult result;
    result.factors = DemandFactors{};

    // --- Population factor ---
    // target_exchange_jobs = total_beings / 3 (1 exchange job per 3 beings)
    const uint32_t target_exchange_jobs = inputs.total_beings / 3;

    // exchange_ratio = exchange_jobs / max(1, target_exchange_jobs)
    const float exchange_ratio = static_cast<float>(inputs.exchange_jobs)
        / static_cast<float>(std::max(1u, target_exchange_jobs));

    // 30 * (1.0 - exchange_ratio), clamped to -20..+30 (under-served = positive)
    const float pop_f = 30.0f * (1.0f - exchange_ratio);
    result.factors.population_factor = static_cast<int8_t>(std::clamp(
        static_cast<int>(pop_f), -20, 30));

    // --- Employment factor ---
    // (50 - unemployment_rate) / 3, clamped to -15..+15
    const float emp_f = (50.0f - static_cast<float>(inputs.unemployment_rate)) / 3.0f;
    result.factors.employment_factor = static_cast<int8_t>(std::clamp(
        static_cast<int>(emp_f), -15, 15));

    // --- Transport factor ---
    // -congestion_level / 5, clamped to -20..0
    const float trans_f = -inputs.congestion_level / 5.0f;
    result.factors.transport_factor = static_cast<int8_t>(std::clamp(
        static_cast<int>(trans_f), -20, 0));

    // --- Tribute factor ---
    // (7.0 - tribute_rate) * 2, clamped to -10..+10
    const float trib_f = (7.0f - inputs.tribute_rate) * 2.0f;
    result.factors.tribute_factor = static_cast<int8_t>(std::clamp(
        static_cast<int>(trib_f), -10, 10));

    // --- Final demand ---
    const int sum = static_cast<int>(result.factors.population_factor)
        + static_cast<int>(result.factors.employment_factor)
        + static_cast<int>(result.factors.transport_factor)
        + static_cast<int>(result.factors.tribute_factor);

    result.demand = static_cast<int8_t>(std::clamp(sum, -100, 100));

    return result;
}

} // namespace demand
} // namespace sims3000
