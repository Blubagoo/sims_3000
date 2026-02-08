/**
 * @file FluidSerialization.h
 * @brief Fluid component and pool serialization/deserialization
 *        (Epic 6, tickets 6-036, 6-037)
 *
 * Provides network serialization for:
 * - FluidComponent: Full component (12 bytes, memcpy) and compact
 *   per-tick fluid-state bit packing (1 bit per entity)
 * - FluidPoolSyncMessage: 22-byte snapshot of per-player fluid pool state
 *
 * All multi-byte fields use little-endian encoding.
 *
 * @see FluidComponent.h
 * @see FluidEnums.h
 */

#pragma once

#include <sims3000/fluid/FluidComponent.h>
#include <sims3000/fluid/FluidConduitComponent.h>
#include <sims3000/fluid/FluidEnums.h>
#include <sims3000/fluid/FluidProducerComponent.h>
#include <cstdint>
#include <cstring>
#include <vector>

namespace sims3000 {
namespace fluid {

// ============================================================================
// Serialization version
// ============================================================================

/// Current serialization version for fluid data
constexpr uint8_t FLUID_SERIALIZATION_VERSION = 1;

// ============================================================================
// FluidComponent serialization (Ticket 6-036)
// ============================================================================

/**
 * @brief Serialize a FluidComponent to a byte buffer.
 *
 * Since FluidComponent is trivially copyable and 12 bytes, serialization
 * uses memcpy for the component data preceded by a version byte.
 * Total serialized size: 13 bytes (1 version + 12 component).
 *
 * @param comp The FluidComponent to serialize.
 * @param buffer Output byte buffer (appended to).
 */
void serialize_fluid_component(const FluidComponent& comp, std::vector<uint8_t>& buffer);

/**
 * @brief Deserialize a FluidComponent from a byte buffer at offset.
 *
 * @param data Pointer to serialized data.
 * @param size Size of the data buffer in bytes.
 * @param comp Output FluidComponent to populate.
 * @return Number of bytes consumed.
 * @throws std::runtime_error if buffer is too small.
 */
size_t deserialize_fluid_component(const uint8_t* data, size_t size, FluidComponent& comp);

// ============================================================================
// Compact fluid-state bit packing (Ticket 6-036)
// ============================================================================

/**
 * @brief Serialize an array of fluid states as bit-packed data.
 *
 * Packs 8 entity fluid states per byte for efficient bulk sync.
 * Format: count(4 bytes LE) + ceil(count/8) packed bytes.
 * Bit 0 of each byte corresponds to the lowest-indexed entity in that group.
 *
 * @param states Array of boolean fluid states (has_fluid).
 * @param count Number of entities.
 * @param buffer Output byte buffer (appended to).
 */
void pack_fluid_states(const bool* states, uint32_t count, std::vector<uint8_t>& buffer);

/**
 * @brief Deserialize bit-packed fluid states.
 *
 * @param data Pointer to serialized data.
 * @param size Size of the data buffer in bytes.
 * @param states Output array to populate with boolean fluid states.
 * @param max_count Maximum number of states the output array can hold.
 * @return Number of bytes consumed.
 * @throws std::runtime_error if buffer is too small or count exceeds max_count.
 */
size_t unpack_fluid_states(const uint8_t* data, size_t size, bool* states, uint32_t max_count);

// ============================================================================
// FluidPoolSyncMessage (Ticket 6-037)
// ============================================================================

/**
 * @struct FluidPoolSyncMessage
 * @brief Fixed-size network message for syncing fluid pool state (22 bytes).
 *
 * Sent when pool values change. All players receive all pool states
 * so rivals' fluid infrastructure health is visible.
 *
 * Layout (22 bytes, serialized field-by-field in LE):
 * - owner:              1 byte  (uint8_t / PlayerID)
 * - state:              1 byte  (FluidPoolState as uint8_t)
 * - total_generated:    4 bytes (uint32_t)
 * - total_consumed:     4 bytes (uint32_t)
 * - surplus:            4 bytes (int32_t)
 * - reservoir_stored:   4 bytes (uint32_t)
 * - reservoir_capacity: 4 bytes (uint32_t)
 */
struct FluidPoolSyncMessage {
    uint8_t owner = 0;
    uint8_t state = 0;              ///< FluidPoolState as uint8_t
    uint32_t total_generated = 0;
    uint32_t total_consumed = 0;
    int32_t surplus = 0;
    uint32_t reservoir_stored = 0;
    uint32_t reservoir_capacity = 0;
};

/// Serialized size of FluidPoolSyncMessage on the wire (22 bytes)
constexpr size_t FLUID_POOL_SYNC_MESSAGE_SIZE = 22;

/**
 * @brief Serialize a FluidPoolSyncMessage to a byte buffer.
 *
 * Uses little-endian encoding for multi-byte fields.
 * Total serialized size: 22 bytes.
 *
 * @param msg The message to serialize.
 * @param buffer Output byte buffer (appended to).
 */
void serialize_pool_sync(const FluidPoolSyncMessage& msg, std::vector<uint8_t>& buffer);

/**
 * @brief Deserialize a FluidPoolSyncMessage from a byte buffer.
 *
 * @param data Pointer to serialized data.
 * @param size Size of the data buffer in bytes.
 * @param msg Output message to populate.
 * @return Number of bytes consumed.
 * @throws std::runtime_error if buffer is too small.
 */
size_t deserialize_pool_sync(const uint8_t* data, size_t size, FluidPoolSyncMessage& msg);

// ============================================================================
// FluidProducerComponent serialization (Ticket F6-SR-01)
// ============================================================================

/// Serialized size of FluidProducerComponent on the wire
/// (1 version + 4 base_output + 4 current_output + 1 max_water_distance
///  + 1 current_water_distance + 1 is_operational + 1 producer_type = 13 bytes)
constexpr size_t FLUID_PRODUCER_SERIALIZED_SIZE = 13;

/**
 * @brief Serialize a FluidProducerComponent to a byte buffer.
 *
 * Uses field-by-field little-endian encoding for cross-platform safety.
 * Total serialized size: 13 bytes (1 version + 12 component fields).
 *
 * @param comp The FluidProducerComponent to serialize.
 * @param buffer Output byte buffer (appended to).
 */
void serialize_fluid_producer(const FluidProducerComponent& comp, std::vector<uint8_t>& buffer);

/**
 * @brief Deserialize a FluidProducerComponent from a byte buffer at offset.
 *
 * @param data Pointer to serialized data.
 * @param size Size of the data buffer in bytes.
 * @param comp Output FluidProducerComponent to populate.
 * @return Number of bytes consumed.
 * @throws std::runtime_error if buffer is too small or version mismatch.
 */
size_t deserialize_fluid_producer(const uint8_t* data, size_t size, FluidProducerComponent& comp);

// ============================================================================
// FluidConduitComponent serialization (Ticket F6-SR-01)
// ============================================================================

/// Serialized size of FluidConduitComponent on the wire
/// (1 version + 1 coverage_radius + 1 is_connected + 1 is_active + 1 conduit_level = 5 bytes)
constexpr size_t FLUID_CONDUIT_SERIALIZED_SIZE = 5;

/**
 * @brief Serialize a FluidConduitComponent to a byte buffer.
 *
 * Uses field-by-field encoding for cross-platform safety.
 * Total serialized size: 5 bytes (1 version + 4 component fields).
 *
 * @param comp The FluidConduitComponent to serialize.
 * @param buffer Output byte buffer (appended to).
 */
void serialize_fluid_conduit(const FluidConduitComponent& comp, std::vector<uint8_t>& buffer);

/**
 * @brief Deserialize a FluidConduitComponent from a byte buffer at offset.
 *
 * @param data Pointer to serialized data.
 * @param size Size of the data buffer in bytes.
 * @param comp Output FluidConduitComponent to populate.
 * @return Number of bytes consumed.
 * @throws std::runtime_error if buffer is too small or version mismatch.
 */
size_t deserialize_fluid_conduit(const uint8_t* data, size_t size, FluidConduitComponent& comp);

} // namespace fluid
} // namespace sims3000
