/**
 * @file PopulationStats.h
 * @brief IStatQueryable interface for population statistics (Ticket E10-030)
 *
 * Provides stat query interface for population system metrics.
 * Follows the IStatQueryable pattern used in other systems.
 */

#ifndef SIMS3000_POPULATION_POPULATIONSTATS_H
#define SIMS3000_POPULATION_POPULATIONSTATS_H

#include <cstdint>

namespace sims3000 {
namespace population {

// Forward declarations
struct PopulationData;
struct EmploymentData;

// --------------------------------------------------------------------------
// Stat IDs for population system
// --------------------------------------------------------------------------
constexpr uint16_t STAT_TOTAL_BEINGS = 200;      ///< Total population count
constexpr uint16_t STAT_BIRTH_RATE = 201;        ///< Birth rate per 1000
constexpr uint16_t STAT_DEATH_RATE = 202;        ///< Death rate per 1000
constexpr uint16_t STAT_GROWTH_RATE = 203;       ///< Net growth rate (percent)
constexpr uint16_t STAT_HARMONY = 204;           ///< Harmony index (0-100)
constexpr uint16_t STAT_HEALTH = 205;            ///< Health index (0-100)
constexpr uint16_t STAT_EDUCATION = 206;         ///< Education index (0-100)
constexpr uint16_t STAT_UNEMPLOYMENT = 207;      ///< Unemployment rate (percent)
constexpr uint16_t STAT_LIFE_EXPECTANCY = 208;   ///< Life expectancy (cycles)

// Stat ID range for population system
constexpr uint16_t STAT_POPULATION_MIN = 200;
constexpr uint16_t STAT_POPULATION_MAX = 299;

/**
 * @brief Get a stat value from PopulationData and EmploymentData
 *
 * @param pop Population data
 * @param emp Employment data
 * @param stat_id Stat identifier (STAT_TOTAL_BEINGS, etc.)
 * @return Stat value, or 0.0f if stat_id is invalid
 */
float get_population_stat(const PopulationData& pop, const EmploymentData& emp, uint16_t stat_id);

/**
 * @brief Get human-readable name for a stat
 *
 * @param stat_id Stat identifier
 * @return Stat name string, or nullptr if invalid
 */
const char* get_population_stat_name(uint16_t stat_id);

/**
 * @brief Check if a stat ID is valid for population system
 *
 * @param stat_id Stat identifier
 * @return True if stat_id is in valid range (200-299)
 */
bool is_valid_population_stat(uint16_t stat_id);

} // namespace population
} // namespace sims3000

#endif // SIMS3000_POPULATION_POPULATIONSTATS_H
