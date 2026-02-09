/**
 * @file ContaminationEvents.h
 * @brief Contamination event detection and types (Ticket E10-091)
 *
 * Provides event detection for contamination-related occurrences:
 * - Toxic warnings (contamination above threshold at location)
 * - Contamination spikes (sudden large increases)
 * - Contamination cleared (area contamination dropped below threshold)
 * - City-wide toxic (average contamination above critical level)
 *
 * Events are detected by analyzing the current and previous tick states
 * of a ContaminationGrid.
 */

#ifndef SIMS3000_CONTAMINATION_CONTAMINATIONEVENTS_H
#define SIMS3000_CONTAMINATION_CONTAMINATIONEVENTS_H

#include <cstdint>
#include <vector>

namespace sims3000 {
namespace contamination {

// Forward declaration
class ContaminationGrid;

/**
 * @enum ContaminationEventType
 * @brief Types of contamination events.
 */
enum class ContaminationEventType : uint8_t {
    ToxicWarning = 0,         ///< Contamination above threshold at location
    ContaminationSpike = 1,    ///< Sudden large increase
    ContaminationCleared = 2,  ///< Area contamination dropped below threshold
    CityWideToxic = 3          ///< Average contamination above critical level
};

/**
 * @struct ContaminationEvent
 * @brief Represents a single contamination event.
 */
struct ContaminationEvent {
    ContaminationEventType type;  ///< Type of event
    int32_t x;                    ///< X coordinate of event (or -1 for city-wide)
    int32_t y;                    ///< Y coordinate of event (or -1 for city-wide)
    uint8_t severity;             ///< Event severity (0-255)
    uint8_t contam_type;          ///< ContaminationType as uint8_t
    uint32_t tick;                ///< Simulation tick when event occurred
};

// Event detection thresholds
constexpr uint8_t TOXIC_THRESHOLD = 192;        ///< Threshold for toxic warning
constexpr uint8_t SPIKE_THRESHOLD = 64;         ///< Minimum increase for spike detection
constexpr float CITY_WIDE_TOXIC_THRESHOLD = 80.0f;  ///< Average for city-wide toxic

/**
 * @brief Detect contamination events by comparing current and previous states.
 *
 * Analyzes the grid's current and previous tick buffers to detect:
 * - ToxicWarning: Cells that crossed the TOXIC_THRESHOLD
 * - ContaminationSpike: Cells with increase >= SPIKE_THRESHOLD
 * - ContaminationCleared: Cells that dropped below TOXIC_THRESHOLD
 * - CityWideToxic: City-wide average above CITY_WIDE_TOXIC_THRESHOLD
 *
 * @param grid The contamination grid to analyze (must have previous tick data).
 * @param current_tick The current simulation tick number.
 * @return Vector of detected events.
 */
std::vector<ContaminationEvent> detect_contamination_events(
    const ContaminationGrid& grid,
    uint32_t current_tick);

} // namespace contamination
} // namespace sims3000

#endif // SIMS3000_CONTAMINATION_CONTAMINATIONEVENTS_H
