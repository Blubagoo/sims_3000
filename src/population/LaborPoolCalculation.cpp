/**
 * @file LaborPoolCalculation.cpp
 * @brief Labor pool calculation implementation (Ticket E10-019)
 *
 * Computes labor force size from population demographics and
 * employment data. Uses harmony and education bonuses for
 * participation rate.
 */

#include "sims3000/population/LaborPoolCalculation.h"

#include <algorithm>
#include <cmath>

namespace sims3000 {
namespace population {

LaborPoolResult calculate_labor_pool(const PopulationData& pop, const EmploymentData& /*emp*/) {
    LaborPoolResult result{};

    // Zero population means zero labor force
    if (pop.total_beings == 0) {
        result.working_age_beings = 0;
        result.labor_participation_rate = 0.0f;
        result.labor_force = 0;
        return result;
    }

    // Working-age beings = total * adult_percent / 100
    result.working_age_beings = static_cast<uint32_t>(
        std::round(static_cast<float>(pop.total_beings) * pop.adult_percent / 100.0f)
    );

    // Participation rate = base + harmony bonus + education bonus
    float participation = constants::BASE_LABOR_PARTICIPATION
        + (pop.harmony_index / 100.0f) * constants::HARMONY_PARTICIPATION_BONUS
        + (pop.education_index / 100.0f) * constants::EDUCATION_PARTICIPATION_BONUS;

    // Clamp to [0, 1]
    result.labor_participation_rate = std::min(1.0f, std::max(0.0f, participation));

    // Labor force = round(working_age_beings * participation)
    result.labor_force = static_cast<uint32_t>(
        std::round(static_cast<float>(result.working_age_beings) * result.labor_participation_rate)
    );

    return result;
}

} // namespace population
} // namespace sims3000
