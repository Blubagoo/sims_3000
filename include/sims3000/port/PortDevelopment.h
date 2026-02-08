/**
 * @file PortDevelopment.h
 * @brief Port development level calculation for Epic 8 (Ticket E8-012)
 *
 * Provides functions to calculate and update port zone development levels
 * based on capacity thresholds:
 *
 * | Level | Name          | Capacity Threshold |
 * |-------|---------------|--------------------|
 * | 0     | Undeveloped   | 0                  |
 * | 1     | Basic         | 100                |
 * | 2     | Standard      | 500                |
 * | 3     | Major         | 2000               |
 * | 4     | International | 5000+              |
 *
 * Level transitions emit PortUpgradedEvent for the RenderingSystem
 * and other consumers.
 *
 * Header-only implementation (pure logic, no external dependencies).
 *
 * Uses canonical alien terminology per /docs/canon/terminology.yaml
 */

#pragma once

#include <sims3000/port/PortZoneComponent.h>
#include <sims3000/port/PortEvents.h>
#include <cstdint>
#include <vector>

namespace sims3000 {
namespace port {

/// Number of development levels (0-4)
constexpr uint8_t DEVELOPMENT_LEVEL_COUNT = 5;

/// Maximum development level
constexpr uint8_t MAX_DEVELOPMENT_LEVEL = 4;

/// Capacity thresholds for each development level
constexpr uint16_t DEVELOPMENT_THRESHOLDS[DEVELOPMENT_LEVEL_COUNT] = {
    0,      // Level 0: Undeveloped
    100,    // Level 1: Basic
    500,    // Level 2: Standard
    2000,   // Level 3: Major
    5000    // Level 4: International
};

/**
 * @brief Get the name string for a development level.
 *
 * @param level Development level (0-4).
 * @return Null-terminated string name, or "Unknown" for invalid levels.
 */
inline const char* development_level_name(uint8_t level) {
    switch (level) {
        case 0: return "Undeveloped";
        case 1: return "Basic";
        case 2: return "Standard";
        case 3: return "Major";
        case 4: return "International";
        default: return "Unknown";
    }
}

/**
 * @brief Calculate the development level for a given capacity.
 *
 * Determines which development tier a port qualifies for based on
 * its current capacity value. The highest threshold that the capacity
 * meets or exceeds determines the level.
 *
 * @param capacity Current port capacity (0-65535).
 * @return Development level (0-4).
 */
inline uint8_t calculate_development_level(uint16_t capacity) {
    // Walk thresholds from highest to lowest
    for (uint8_t level = MAX_DEVELOPMENT_LEVEL; level > 0; --level) {
        if (capacity >= DEVELOPMENT_THRESHOLDS[level]) {
            return level;
        }
    }
    return 0;
}

/**
 * @brief Update a port zone's development level based on capacity.
 *
 * Calculates the new development level from the given capacity,
 * updates the zone_level field if it changed, and emits a
 * PortUpgradedEvent when a level transition occurs.
 *
 * @param zone The port zone component to update.
 * @param capacity Current port capacity.
 * @param events Output vector for PortUpgradedEvent emissions.
 * @param entity_id Entity ID of the port (for event reporting).
 * @return true if the level changed, false if unchanged.
 */
inline bool update_development_level(PortZoneComponent& zone,
                                      uint16_t capacity,
                                      std::vector<PortUpgradedEvent>& events,
                                      uint32_t entity_id) {
    uint8_t new_level = calculate_development_level(capacity);

    if (new_level == zone.zone_level) {
        return false;
    }

    uint8_t old_level = zone.zone_level;
    zone.zone_level = new_level;

    events.emplace_back(entity_id, old_level, new_level);

    return true;
}

} // namespace port
} // namespace sims3000
