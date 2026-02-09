/**
 * @file ContaminationEvents.cpp
 * @brief Implementation of contamination event detection (Ticket E10-091)
 *
 * @see ContaminationEvents.h
 */

#include <sims3000/contamination/ContaminationEvents.h>
#include <sims3000/contamination/ContaminationGrid.h>

namespace sims3000 {
namespace contamination {

std::vector<ContaminationEvent> detect_contamination_events(
    const ContaminationGrid& grid,
    uint32_t current_tick)
{
    std::vector<ContaminationEvent> events;

    uint32_t total_contamination = 0;
    uint32_t cell_count = static_cast<uint32_t>(grid.get_width()) * grid.get_height();

    // Scan grid for per-cell events
    for (int32_t y = 0; y < grid.get_height(); ++y) {
        for (int32_t x = 0; x < grid.get_width(); ++x) {
            uint8_t current_level = grid.get_level(x, y);
            uint8_t previous_level = grid.get_level_previous_tick(x, y);
            uint8_t dominant_type = grid.get_dominant_type(x, y);

            total_contamination += current_level;

            // ToxicWarning: current >= threshold, previous < threshold
            if (current_level >= TOXIC_THRESHOLD && previous_level < TOXIC_THRESHOLD) {
                ContaminationEvent event;
                event.type = ContaminationEventType::ToxicWarning;
                event.x = x;
                event.y = y;
                event.severity = current_level;
                event.contam_type = dominant_type;
                event.tick = current_tick;
                events.push_back(event);
            }

            // ContaminationSpike: increase >= SPIKE_THRESHOLD
            if (current_level > previous_level) {
                uint8_t increase = current_level - previous_level;
                if (increase >= SPIKE_THRESHOLD) {
                    ContaminationEvent event;
                    event.type = ContaminationEventType::ContaminationSpike;
                    event.x = x;
                    event.y = y;
                    event.severity = increase;
                    event.contam_type = dominant_type;
                    event.tick = current_tick;
                    events.push_back(event);
                }
            }

            // ContaminationCleared: previous >= threshold, current < threshold
            if (previous_level >= TOXIC_THRESHOLD && current_level < TOXIC_THRESHOLD) {
                ContaminationEvent event;
                event.type = ContaminationEventType::ContaminationCleared;
                event.x = x;
                event.y = y;
                event.severity = previous_level - current_level;
                event.contam_type = grid.get_dominant_type_previous_tick(x, y);
                event.tick = current_tick;
                events.push_back(event);
            }
        }
    }

    // CityWideToxic: average contamination above threshold
    if (cell_count > 0) {
        float average = static_cast<float>(total_contamination) / static_cast<float>(cell_count);
        if (average >= CITY_WIDE_TOXIC_THRESHOLD) {
            ContaminationEvent event;
            event.type = ContaminationEventType::CityWideToxic;
            event.x = -1;  // City-wide event (no specific location)
            event.y = -1;
            event.severity = static_cast<uint8_t>(average);
            event.contam_type = 0;
            event.tick = current_tick;
            events.push_back(event);
        }
    }

    return events;
}

} // namespace contamination
} // namespace sims3000
