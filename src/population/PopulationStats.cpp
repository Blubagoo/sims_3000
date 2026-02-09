/**
 * @file PopulationStats.cpp
 * @brief Implementation of population stat query interface (Ticket E10-030)
 */

#include "sims3000/population/PopulationStats.h"
#include "sims3000/population/PopulationData.h"
#include "sims3000/population/EmploymentData.h"
#include "sims3000/population/LifeExpectancy.h"

namespace sims3000 {
namespace population {

float get_population_stat(const PopulationData& pop, const EmploymentData& emp, uint16_t stat_id) {
    switch (stat_id) {
        case STAT_TOTAL_BEINGS:
            return static_cast<float>(pop.total_beings);

        case STAT_BIRTH_RATE:
            return static_cast<float>(pop.birth_rate_per_1000);

        case STAT_DEATH_RATE:
            return static_cast<float>(pop.death_rate_per_1000);

        case STAT_GROWTH_RATE:
            // Return as percentage (-100 to +100)
            return pop.growth_rate * 100.0f;

        case STAT_HARMONY:
            return static_cast<float>(pop.harmony_index);

        case STAT_HEALTH:
            return static_cast<float>(pop.health_index);

        case STAT_EDUCATION:
            return static_cast<float>(pop.education_index);

        case STAT_UNEMPLOYMENT:
            // Return as percentage (0-100)
            return static_cast<float>(emp.unemployment_rate);

        case STAT_LIFE_EXPECTANCY: {
            // Calculate life expectancy on-demand
            // Note: We need contamination and disorder levels for full calculation
            // For now, use simplified calculation with available data
            LifeExpectancyInput input{};
            input.health_index = pop.health_index;
            input.contamination_level = 50;  // Default, should be provided by caller
            input.disorder_level = 50;       // Default, should be provided by caller
            input.education_index = pop.education_index;
            input.harmony_index = pop.harmony_index;

            LifeExpectancyResult result = calculate_life_expectancy(input);
            return result.life_expectancy;
        }

        default:
            // Invalid stat ID
            return 0.0f;
    }
}

const char* get_population_stat_name(uint16_t stat_id) {
    switch (stat_id) {
        case STAT_TOTAL_BEINGS:      return "Total Population";
        case STAT_BIRTH_RATE:        return "Birth Rate";
        case STAT_DEATH_RATE:        return "Death Rate";
        case STAT_GROWTH_RATE:       return "Growth Rate";
        case STAT_HARMONY:           return "Harmony";
        case STAT_HEALTH:            return "Health";
        case STAT_EDUCATION:         return "Education";
        case STAT_UNEMPLOYMENT:      return "Unemployment Rate";
        case STAT_LIFE_EXPECTANCY:   return "Life Expectancy";
        default:                     return nullptr;
    }
}

bool is_valid_population_stat(uint16_t stat_id) {
    return stat_id >= STAT_POPULATION_MIN && stat_id <= STAT_POPULATION_MAX;
}

} // namespace population
} // namespace sims3000
