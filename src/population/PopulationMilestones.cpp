/**
 * @file PopulationMilestones.cpp
 * @brief Implementation of population milestone detection (Ticket E10-031)
 */

#include "sims3000/population/PopulationMilestones.h"

namespace sims3000 {
namespace population {

std::vector<MilestoneEvent> check_milestones(uint32_t previous_pop, uint32_t current_pop) {
    std::vector<MilestoneEvent> events;

    // No change, no events
    if (previous_pop == current_pop) {
        return events;
    }

    bool is_growing = current_pop > previous_pop;

    // Check each milestone threshold
    for (uint8_t i = 0; i < MILESTONE_COUNT; ++i) {
        uint32_t threshold = MILESTONE_THRESHOLDS[i];

        if (is_growing) {
            // Growing: check if we crossed upward
            if (previous_pop < threshold && current_pop >= threshold) {
                MilestoneEvent event{};
                event.type = static_cast<MilestoneType>(i);
                event.population = current_pop;
                event.is_upgrade = true;
                events.push_back(event);
            }
        } else {
            // Shrinking: check if we fell below
            if (previous_pop >= threshold && current_pop < threshold) {
                MilestoneEvent event{};
                event.type = static_cast<MilestoneType>(i);
                event.population = current_pop;
                event.is_upgrade = false;
                events.push_back(event);
            }
        }
    }

    return events;
}

MilestoneType get_milestone_level(uint32_t population) {
    // Find the highest milestone reached
    // Iterate in reverse to find the highest threshold met
    for (int i = MILESTONE_COUNT - 1; i >= 0; --i) {
        if (population >= MILESTONE_THRESHOLDS[i]) {
            return static_cast<MilestoneType>(i);
        }
    }

    // Below all thresholds, still return Village (lowest level)
    return MilestoneType::Village;
}

const char* get_milestone_name(MilestoneType type) {
    switch (type) {
        case MilestoneType::Village:      return "Village";
        case MilestoneType::Town:         return "Town";
        case MilestoneType::City:         return "City";
        case MilestoneType::Metropolis:   return "Metropolis";
        case MilestoneType::Megalopolis:  return "Megalopolis";
        default:                          return "Unknown";
    }
}

uint32_t get_milestone_threshold(MilestoneType type) {
    uint8_t index = static_cast<uint8_t>(type);
    if (index < MILESTONE_COUNT) {
        return MILESTONE_THRESHOLDS[index];
    }
    return 0;
}

} // namespace population
} // namespace sims3000
