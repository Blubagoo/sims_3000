/**
 * @file ContaminationQuery.h
 * @brief Traffic contamination query interface for Epic 7 (Ticket E7-029)
 *
 * Provides query utilities for the ContaminationSystem to retrieve
 * traffic-based contamination rates at specific grid positions.
 *
 * Only congested pathways (congestion > 128) contribute contamination.
 * The contamination rate is stored in TrafficComponent.contamination_rate
 * and computed by CongestionCalculator.
 *
 * Uses canonical alien terminology per /docs/canon/terminology.yaml
 */

#pragma once

#include <sims3000/transport/TrafficComponent.h>
#include <sims3000/transport/PathwayGrid.h>
#include <cstdint>
#include <unordered_map>

namespace sims3000 {
namespace transport {

/**
 * @class ContaminationQuery
 * @brief Static query interface for traffic-based contamination.
 *
 * Used by the ContaminationSystem to query contamination rates
 * at specific grid positions based on traffic data.
 */
class ContaminationQuery {
public:
    /**
     * @brief Get contamination rate at a grid position.
     *
     * Looks up the pathway entity at (x, y) in the grid, then retrieves
     * its TrafficComponent contamination_rate from the traffic map.
     *
     * @param x Grid X coordinate.
     * @param y Grid Y coordinate.
     * @param grid PathwayGrid for entity lookup.
     * @param traffic_map Map of entity_id to TrafficComponent.
     * @return Contamination rate (0-15). 0 if no pathway or no traffic.
     */
    static uint8_t get_contamination_rate_at(
        int32_t x, int32_t y,
        const PathwayGrid& grid,
        const std::unordered_map<uint32_t, TrafficComponent>& traffic_map
    );

    /**
     * @brief Check if a position has contaminating traffic.
     *
     * Returns true if the pathway at (x, y) has congestion > 128,
     * meaning it contributes environmental contamination.
     *
     * @param x Grid X coordinate.
     * @param y Grid Y coordinate.
     * @param grid PathwayGrid for entity lookup.
     * @param traffic_map Map of entity_id to TrafficComponent.
     * @return true if position has contaminating traffic (congestion > 128).
     */
    static bool has_traffic_contamination(
        int32_t x, int32_t y,
        const PathwayGrid& grid,
        const std::unordered_map<uint32_t, TrafficComponent>& traffic_map
    );
};

} // namespace transport
} // namespace sims3000
