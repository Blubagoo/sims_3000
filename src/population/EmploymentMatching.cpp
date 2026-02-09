/**
 * @file EmploymentMatching.cpp
 * @brief Employment matching algorithm implementation (Ticket E10-021)
 *
 * Proportionally distributes labor force across exchange and fabrication
 * sectors based on available job counts per sector.
 */

#include "sims3000/population/EmploymentMatching.h"

#include <algorithm>

namespace sims3000 {
namespace population {

EmploymentMatchingResult match_employment(uint32_t labor_force,
                                           uint32_t exchange_jobs,
                                           uint32_t fabrication_jobs) {
    EmploymentMatchingResult result{};

    uint32_t total_jobs = exchange_jobs + fabrication_jobs;

    // Edge case: no jobs available -> all unemployed
    if (total_jobs == 0) {
        result.employed_laborers = 0;
        result.unemployed = labor_force;
        result.exchange_employed = 0;
        result.fabrication_employed = 0;
        result.unemployment_rate = (labor_force > 0) ? 100 : 0;
        return result;
    }

    // Maximum employment is limited by both labor supply and job supply
    uint32_t max_employment = std::min(labor_force, total_jobs);

    // Proportional distribution based on sector job share
    // Use 64-bit to avoid overflow for large values
    result.exchange_employed = static_cast<uint32_t>(
        static_cast<uint64_t>(max_employment) * exchange_jobs / total_jobs);
    result.fabrication_employed = max_employment - result.exchange_employed;

    result.employed_laborers = result.exchange_employed + result.fabrication_employed;
    result.unemployed = labor_force - result.employed_laborers;

    // Unemployment rate as percentage (0-100)
    result.unemployment_rate = (labor_force > 0)
        ? static_cast<uint8_t>(result.unemployed * 100 / labor_force)
        : 0;

    return result;
}

} // namespace population
} // namespace sims3000
