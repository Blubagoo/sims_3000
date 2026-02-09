/**
 * @file DisorderEvents.cpp
 * @brief Implementation of disorder event detection (Ticket E10-079)
 *
 * @see DisorderEvents.h for public interface documentation.
 */

#include <sims3000/disorder/DisorderEvents.h>
#include <sims3000/disorder/DisorderGrid.h>

namespace sims3000 {
namespace disorder {

std::vector<DisorderEvent> detect_disorder_events(
    const DisorderGrid& grid,
    uint32_t current_tick)
{
    std::vector<DisorderEvent> events;

    uint16_t width = grid.get_width();
    uint16_t height = grid.get_height();

    // Track city-wide average disorder
    uint32_t total_disorder = grid.get_total_disorder();
    uint32_t tile_count = static_cast<uint32_t>(width) * height;
    float average_disorder = tile_count > 0 ? static_cast<float>(total_disorder) / static_cast<float>(tile_count) : 0.0f;

    // Check for city-wide disorder event
    if (average_disorder >= CITY_WIDE_THRESHOLD) {
        DisorderEvent event;
        event.type = DisorderEventType::CityWideDisorder;
        event.x = width / 2;  // Center of grid
        event.y = height / 2;
        event.severity = static_cast<uint8_t>(average_disorder > 255.0f ? 255 : average_disorder);
        event.tick = current_tick;
        events.push_back(event);
    }

    // Scan each tile for tile-specific events
    for (int32_t y = 0; y < height; ++y) {
        for (int32_t x = 0; x < width; ++x) {
            uint8_t current_level = grid.get_level(x, y);
            uint8_t previous_level = grid.get_level_previous_tick(x, y);

            // HighDisorderWarning: crossed above threshold
            if (current_level >= HIGH_DISORDER_THRESHOLD && previous_level < HIGH_DISORDER_THRESHOLD) {
                DisorderEvent event;
                event.type = DisorderEventType::HighDisorderWarning;
                event.x = x;
                event.y = y;
                event.severity = current_level;
                event.tick = current_tick;
                events.push_back(event);
            }

            // DisorderResolved: dropped below threshold
            if (current_level < HIGH_DISORDER_THRESHOLD && previous_level >= HIGH_DISORDER_THRESHOLD) {
                DisorderEvent event;
                event.type = DisorderEventType::DisorderResolved;
                event.x = x;
                event.y = y;
                event.severity = previous_level; // Severity is the previous high level
                event.tick = current_tick;
                events.push_back(event);
            }

            // DisorderSpike: large sudden increase
            if (current_level > previous_level) {
                uint16_t increase = static_cast<uint16_t>(current_level) - static_cast<uint16_t>(previous_level);
                if (increase > SPIKE_THRESHOLD) {
                    DisorderEvent event;
                    event.type = DisorderEventType::DisorderSpike;
                    event.x = x;
                    event.y = y;
                    event.severity = static_cast<uint8_t>(increase > 255 ? 255 : increase);
                    event.tick = current_tick;
                    events.push_back(event);
                }
            }
        }
    }

    return events;
}

} // namespace disorder
} // namespace sims3000
