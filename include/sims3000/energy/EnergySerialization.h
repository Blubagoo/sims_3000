/**
 * @file EnergySerialization.h
 * @brief Energy component and pool serialization/deserialization
 *        (Epic 5, tickets 5-034, 5-035)
 *
 * Provides network serialization for:
 * - EnergyComponent: Full component (12 bytes, memcpy) and compact
 *   per-tick power-state bit packing (1 bit per entity)
 * - EnergyPoolSyncMessage: 16-byte snapshot of PerPlayerEnergyPool
 *
 * All multi-byte fields use little-endian encoding.
 *
 * @see EnergyComponent.h
 * @see PerPlayerEnergyPool.h
 */

#pragma once

#include <sims3000/energy/EnergyComponent.h>
#include <sims3000/energy/PerPlayerEnergyPool.h>
#include <cstdint>
#include <cstring>
#include <vector>

namespace sims3000 {
namespace energy {

// ============================================================================
// Serialization version
// ============================================================================

/// Current serialization version for energy data
constexpr uint8_t ENERGY_SERIALIZATION_VERSION = 1;

// ============================================================================
// EnergyComponent serialization (Ticket 5-034)
// ============================================================================

/**
 * @brief Serialize an EnergyComponent to a byte buffer.
 *
 * Since EnergyComponent is trivially copyable and 12 bytes, serialization
 * uses memcpy for the component data preceded by a version byte.
 * Total serialized size: 13 bytes (1 version + 12 component).
 *
 * @param comp The EnergyComponent to serialize.
 * @param buffer Output byte buffer (appended to).
 */
void serialize_energy_component(const EnergyComponent& comp, std::vector<uint8_t>& buffer);

/**
 * @brief Deserialize an EnergyComponent from a byte buffer at offset.
 *
 * @param data Pointer to serialized data.
 * @param size Size of the data buffer in bytes.
 * @param comp Output EnergyComponent to populate.
 * @return Number of bytes consumed.
 * @throws std::runtime_error if buffer is too small.
 */
size_t deserialize_energy_component(const uint8_t* data, size_t size, EnergyComponent& comp);

// ============================================================================
// Compact power-state bit packing (Ticket 5-034)
// ============================================================================

/**
 * @brief Serialize an array of power states as bit-packed data.
 *
 * Packs 8 entity power states per byte for efficient bulk sync.
 * Format: count(4 bytes LE) + ceil(count/8) packed bytes.
 * Bit 0 of each byte corresponds to the lowest-indexed entity in that group.
 *
 * @param states Array of boolean power states.
 * @param count Number of entities.
 * @param buffer Output byte buffer (appended to).
 */
void serialize_power_states(const bool* states, uint32_t count, std::vector<uint8_t>& buffer);

/**
 * @brief Deserialize bit-packed power states.
 *
 * @param data Pointer to serialized data.
 * @param size Size of the data buffer in bytes.
 * @param states Output array to populate with boolean power states.
 * @param max_count Maximum number of states the output array can hold.
 * @return Number of bytes consumed.
 * @throws std::runtime_error if buffer is too small or count exceeds max_count.
 */
size_t deserialize_power_states(const uint8_t* data, size_t size, bool* states, uint32_t max_count);

// ============================================================================
// EnergyPoolSyncMessage (Ticket 5-035)
// ============================================================================

/**
 * @struct EnergyPoolSyncMessage
 * @brief Fixed-size network message for syncing energy pool state (16 bytes).
 *
 * Sent when pool values change. Contains the essential fields from
 * PerPlayerEnergyPool needed for client display.
 *
 * Layout (16 bytes):
 * - owner:           1 byte  (uint8_t / PlayerID)
 * - state:           1 byte  (EnergyPoolState)
 * - _padding:        2 bytes (alignment)
 * - total_generated: 4 bytes (uint32_t)
 * - total_consumed:  4 bytes (uint32_t)
 * - surplus:         4 bytes (int32_t)
 */
struct EnergyPoolSyncMessage {
    uint8_t owner = 0;
    uint8_t state = 0;          ///< EnergyPoolState as uint8_t
    uint8_t _padding[2] = {};
    uint32_t total_generated = 0;
    uint32_t total_consumed = 0;
    int32_t surplus = 0;
};

static_assert(sizeof(EnergyPoolSyncMessage) == 16, "EnergyPoolSyncMessage must be 16 bytes");

/**
 * @brief Serialize an EnergyPoolSyncMessage to a byte buffer.
 *
 * Uses little-endian encoding for multi-byte fields.
 * Total serialized size: 16 bytes.
 *
 * @param msg The message to serialize.
 * @param buffer Output byte buffer (appended to).
 */
void serialize_pool_sync(const EnergyPoolSyncMessage& msg, std::vector<uint8_t>& buffer);

/**
 * @brief Deserialize an EnergyPoolSyncMessage from a byte buffer.
 *
 * @param data Pointer to serialized data.
 * @param size Size of the data buffer in bytes.
 * @param msg Output message to populate.
 * @return Number of bytes consumed.
 * @throws std::runtime_error if buffer is too small.
 */
size_t deserialize_pool_sync(const uint8_t* data, size_t size, EnergyPoolSyncMessage& msg);

/**
 * @brief Create an EnergyPoolSyncMessage from a PerPlayerEnergyPool.
 *
 * Extracts the fields needed for network sync from the full pool structure.
 *
 * @param pool The energy pool to create a sync message from.
 * @return EnergyPoolSyncMessage populated with pool data.
 */
EnergyPoolSyncMessage create_pool_sync_message(const PerPlayerEnergyPool& pool);

} // namespace energy
} // namespace sims3000
