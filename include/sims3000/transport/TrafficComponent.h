/**
 * @file TrafficComponent.h
 * @brief Traffic flow component for Epic 7 (Ticket E7-003)
 *
 * Defines:
 * - TrafficComponent: Sparse attachment for pathways with active traffic flow
 *
 * Sparse attachment pattern: only pathways that currently carry flow
 * get this component. Pathways with zero flow do not have it attached.
 *
 * Uses canonical alien terminology per /docs/canon/terminology.yaml
 */

#pragma once

#include <cstdint>
#include <type_traits>

namespace sims3000 {
namespace transport {

/**
 * @struct TrafficComponent
 * @brief Traffic flow data attached to pathways with active flow (16 bytes).
 *
 * Tracks per-pathway traffic flow state, congestion, and blockage.
 * Only attached to pathway entities that currently carry traffic (sparse pattern).
 *
 * Layout (16 bytes):
 * - flow_current:       4 bytes (uint32_t) - current tick's flow count
 * - flow_previous:      4 bytes (uint32_t) - previous tick's flow count
 * - flow_sources:       2 bytes (uint16_t) - number of distinct flow sources
 * - congestion_level:   1 byte  (uint8_t)  - 0-255 congestion severity
 * - flow_blockage_ticks:1 byte  (uint8_t)  - consecutive ticks of blockage
 * - contamination_rate: 1 byte  (uint8_t)  - environmental contamination rate
 * - padding:            3 bytes (uint8_t[3])- alignment padding
 *
 * Total: 16 bytes
 */
struct TrafficComponent {
    uint32_t flow_current       = 0;    ///< Current tick's flow count
    uint32_t flow_previous      = 0;    ///< Previous tick's flow count
    uint16_t flow_sources       = 0;    ///< Number of distinct flow sources
    uint8_t  congestion_level   = 0;    ///< Congestion severity (0=free, 255=gridlock)
    uint8_t  flow_blockage_ticks = 0;   ///< Consecutive ticks of blockage
    uint8_t  contamination_rate = 0;    ///< Environmental contamination rate
    uint8_t  padding[3]         = {};   ///< Alignment padding
};

// Verify TrafficComponent size (16 bytes)
static_assert(sizeof(TrafficComponent) == 16,
    "TrafficComponent must be 16 bytes");

// Verify TrafficComponent is trivially copyable for serialization
static_assert(std::is_trivially_copyable<TrafficComponent>::value,
    "TrafficComponent must be trivially copyable");

} // namespace transport
} // namespace sims3000
