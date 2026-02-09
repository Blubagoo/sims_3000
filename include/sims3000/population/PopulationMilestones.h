/**
 * @file PopulationMilestones.h
 * @brief Population milestone detection (Ticket E10-031)
 *
 * Detects when population crosses threshold milestones:
 * - Village: 100 beings
 * - Town: 500 beings
 * - City: 2000 beings
 * - Metropolis: 10000 beings
 * - Megalopolis: 50000 beings
 */

#ifndef SIMS3000_POPULATION_POPULATIONMILESTONES_H
#define SIMS3000_POPULATION_POPULATIONMILESTONES_H

#include <cstdint>
#include <vector>

namespace sims3000 {
namespace population {

/**
 * @enum MilestoneType
 * @brief Population milestone levels
 */
enum class MilestoneType : uint8_t {
    Village = 0,      ///< 100 beings
    Town = 1,         ///< 500 beings
    City = 2,         ///< 2000 beings
    Metropolis = 3,   ///< 10000 beings
    Megalopolis = 4   ///< 50000 beings
};

/// Milestone thresholds in order
constexpr uint32_t MILESTONE_THRESHOLDS[] = { 100, 500, 2000, 10000, 50000 };

/// Number of milestones
constexpr uint8_t MILESTONE_COUNT = 5;

/**
 * @struct MilestoneEvent
 * @brief Event data for a milestone crossing
 */
struct MilestoneEvent {
    MilestoneType type;     ///< Which milestone was crossed
    uint32_t population;    ///< Population at time of crossing
    bool is_upgrade;        ///< true = reached, false = fell below
};

/**
 * @brief Check if population crossed any milestone thresholds
 *
 * Detects milestone crossings between previous and current population.
 * Can detect multiple milestones if there's a large jump.
 *
 * @param previous_pop Population before this tick
 * @param current_pop Population after this tick
 * @return Vector of milestone events (empty if none crossed)
 */
std::vector<MilestoneEvent> check_milestones(uint32_t previous_pop, uint32_t current_pop);

/**
 * @brief Get current milestone level for a population
 *
 * Returns the highest milestone reached by the population.
 * Returns Village (0) if below 100 beings.
 *
 * @param population Current population
 * @return Milestone level
 */
MilestoneType get_milestone_level(uint32_t population);

/**
 * @brief Get milestone name string
 *
 * @param type Milestone type
 * @return Human-readable milestone name
 */
const char* get_milestone_name(MilestoneType type);

/**
 * @brief Get threshold for a milestone type
 *
 * @param type Milestone type
 * @return Population threshold for that milestone
 */
uint32_t get_milestone_threshold(MilestoneType type);

} // namespace population
} // namespace sims3000

#endif // SIMS3000_POPULATION_POPULATIONMILESTONES_H
