/**
 * @file PortSerialization.h
 * @brief Port component serialization/deserialization
 *        (Epic 8, ticket E8-002)
 *
 * Provides network serialization for:
 * - PortComponent: 12-byte field-by-field LE serialization (1 version + 11 data)
 *
 * All multi-byte fields use little-endian encoding.
 * The 4-byte padding field in PortComponent is NOT serialized.
 *
 * @see PortComponent.h
 * @see PortTypes.h
 */

#pragma once

#include <sims3000/port/PortComponent.h>
#include <cstdint>
#include <vector>

namespace sims3000 {
namespace port {

// ============================================================================
// Serialization version
// ============================================================================

/// Current serialization version for port data
constexpr uint8_t PORT_SERIALIZATION_VERSION = 1;

// ============================================================================
// PortComponent serialization (Ticket E8-002)
// ============================================================================

/// Serialized size of PortComponent on the wire
/// (1 version + 1 port_type + 2 capacity + 2 max_capacity
///  + 1 utilization + 1 infrastructure_level + 1 is_operational
///  + 1 is_connected_to_edge + 1 demand_bonus_radius + 1 connection_flags = 12 bytes)
///
/// Note: The 4-byte padding field in PortComponent is NOT serialized.
constexpr size_t PORT_COMPONENT_SERIALIZED_SIZE = 12;

/**
 * @brief Serialize a PortComponent to a byte buffer.
 *
 * Uses field-by-field little-endian encoding for cross-platform safety.
 * Total serialized size: 12 bytes (1 version + 11 component fields).
 * Padding bytes are NOT serialized.
 *
 * @param comp The PortComponent to serialize.
 * @param buffer Output byte buffer (appended to).
 */
void serialize_port_component(const PortComponent& comp, std::vector<uint8_t>& buffer);

/**
 * @brief Deserialize a PortComponent from a byte buffer at offset.
 *
 * @param data Pointer to serialized data.
 * @param size Size of the data buffer in bytes.
 * @param comp Output PortComponent to populate.
 * @return Number of bytes consumed.
 * @throws std::runtime_error if buffer is too small or version mismatch.
 */
size_t deserialize_port_component(const uint8_t* data, size_t size, PortComponent& comp);

} // namespace port
} // namespace sims3000
