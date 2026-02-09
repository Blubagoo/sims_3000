/**
 * @file DisorderEvents.h
 * @brief Disorder event detection and types (Ticket E10-079)
 *
 * Defines disorder-related events:
 * - HighDisorderWarning: Disorder above threshold in a specific area
 * - DisorderSpike: Sudden large increase in disorder
 * - DisorderResolved: Area disorder dropped below threshold
 * - CityWideDisorder: Average disorder above critical level
 *
 * Events are detected by comparing current and previous tick disorder state.
 */

#ifndef SIMS3000_DISORDER_DISORDEREVENTS_H
#define SIMS3000_DISORDER_DISORDEREVENTS_H

#include <cstdint>
#include <vector>

namespace sims3000 {
namespace disorder {

// Forward declaration
class DisorderGrid;

// ============================================================================
// Event Types
// ============================================================================

/**
 * @enum DisorderEventType
 * @brief Types of disorder events that can occur.
 */
enum class DisorderEventType : uint8_t {
    HighDisorderWarning = 0,  ///< Disorder above threshold in area
    DisorderSpike = 1,        ///< Sudden large increase
    DisorderResolved = 2,     ///< Area disorder dropped below threshold
    CityWideDisorder = 3      ///< Average disorder above critical level
};

/**
 * @struct DisorderEvent
 * @brief A single disorder event with location and metadata.
 */
struct DisorderEvent {
    DisorderEventType type; ///< Type of event
    int32_t x;              ///< X coordinate of event location
    int32_t y;              ///< Y coordinate of event location
    uint8_t severity;       ///< Event severity (0-255)
    uint32_t tick;          ///< Simulation tick when event occurred
};

// ============================================================================
// Event Thresholds
// ============================================================================

constexpr uint8_t HIGH_DISORDER_THRESHOLD = 192;   ///< Threshold for high disorder warning
constexpr uint8_t SPIKE_THRESHOLD = 64;            ///< Increase > 64 in one tick is a spike
constexpr float CITY_WIDE_THRESHOLD = 100.0f;      ///< Average disorder threshold for city-wide events

// ============================================================================
// Event Detection
// ============================================================================

/**
 * @brief Detect disorder events by comparing current and previous state.
 *
 * Scans the disorder grid for events:
 * - HighDisorderWarning: Tiles that crossed above HIGH_DISORDER_THRESHOLD
 * - DisorderSpike: Tiles that increased by more than SPIKE_THRESHOLD
 * - DisorderResolved: Tiles that dropped below HIGH_DISORDER_THRESHOLD
 * - CityWideDisorder: Average disorder exceeds CITY_WIDE_THRESHOLD
 *
 * @param grid The disorder grid to analyze.
 * @param current_tick The current simulation tick.
 * @return Vector of detected events (may be empty).
 */
std::vector<DisorderEvent> detect_disorder_events(
    const DisorderGrid& grid,
    uint32_t current_tick);

} // namespace disorder
} // namespace sims3000

#endif // SIMS3000_DISORDER_DISORDEREVENTS_H
