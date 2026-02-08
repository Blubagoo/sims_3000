/**
 * @file TransportSerialization.h
 * @brief Transport component serialization/deserialization
 *        (Epic 7, tickets E7-036, E7-037)
 *
 * Provides network serialization for:
 * - RoadComponent: 17-byte field-by-field LE serialization (1 version + 16 data)
 * - TrafficComponent: 14-byte field-by-field LE serialization (1 version + 13 data)
 *
 * All multi-byte fields use little-endian encoding.
 *
 * TrafficComponent serialization note:
 *   Traffic state changes rapidly. Callers should sync at a configurable
 *   frequency (e.g., every N ticks) rather than every tick to reduce
 *   bandwidth. The serialization functions themselves are stateless;
 *   frequency control is the responsibility of the caller.
 *
 * @see RoadComponent.h
 * @see TrafficComponent.h
 * @see TransportEnums.h
 */

#pragma once

#include <sims3000/transport/RoadComponent.h>
#include <sims3000/transport/TrafficComponent.h>
#include <cstdint>
#include <vector>

namespace sims3000 {
namespace transport {

// ============================================================================
// Serialization version
// ============================================================================

/// Current serialization version for transport data
constexpr uint8_t TRANSPORT_SERIALIZATION_VERSION = 1;

// ============================================================================
// RoadComponent serialization (Ticket E7-036)
// ============================================================================

/// Serialized size of RoadComponent on the wire
/// (1 version + 1 type + 1 direction + 2 base_capacity + 2 current_capacity
///  + 1 health + 1 decay_rate + 1 connection_mask + 1 is_junction
///  + 2 network_id + 4 last_maintained_tick = 17 bytes)
constexpr size_t ROAD_COMPONENT_SERIALIZED_SIZE = 17;

/**
 * @brief Serialize a RoadComponent to a byte buffer.
 *
 * Uses field-by-field little-endian encoding for cross-platform safety.
 * Total serialized size: 17 bytes (1 version + 16 component fields).
 *
 * @param comp The RoadComponent to serialize.
 * @param buffer Output byte buffer (appended to).
 */
void serialize_road_component(const RoadComponent& comp, std::vector<uint8_t>& buffer);

/**
 * @brief Deserialize a RoadComponent from a byte buffer at offset.
 *
 * @param data Pointer to serialized data.
 * @param size Size of the data buffer in bytes.
 * @param comp Output RoadComponent to populate.
 * @return Number of bytes consumed.
 * @throws std::runtime_error if buffer is too small or version mismatch.
 */
size_t deserialize_road_component(const uint8_t* data, size_t size, RoadComponent& comp);

// ============================================================================
// TrafficComponent serialization (Ticket E7-037)
// ============================================================================

/// Serialized size of TrafficComponent on the wire
/// (1 version + 4 flow_current + 4 flow_previous + 2 flow_sources
///  + 1 congestion_level + 1 flow_blockage_ticks + 1 contamination_rate = 14 bytes)
///
/// Note: The 3-byte padding field in TrafficComponent is NOT serialized.
///
/// Sync frequency note: Traffic data changes every tick. To reduce bandwidth,
/// callers should serialize at a configurable interval (e.g., every 4-8 ticks)
/// rather than every tick. The functions below are stateless; the caller
/// manages sync timing.
constexpr size_t TRAFFIC_COMPONENT_SERIALIZED_SIZE = 14;

/**
 * @brief Serialize a TrafficComponent to a byte buffer.
 *
 * Uses field-by-field little-endian encoding for cross-platform safety.
 * Skips the 3-byte padding field.
 * Total serialized size: 14 bytes (1 version + 13 component fields).
 *
 * @param comp The TrafficComponent to serialize.
 * @param buffer Output byte buffer (appended to).
 */
void serialize_traffic_component(const TrafficComponent& comp, std::vector<uint8_t>& buffer);

/**
 * @brief Deserialize a TrafficComponent from a byte buffer at offset.
 *
 * @param data Pointer to serialized data.
 * @param size Size of the data buffer in bytes.
 * @param comp Output TrafficComponent to populate.
 * @return Number of bytes consumed.
 * @throws std::runtime_error if buffer is too small or version mismatch.
 */
size_t deserialize_traffic_component(const uint8_t* data, size_t size, TrafficComponent& comp);

} // namespace transport
} // namespace sims3000
