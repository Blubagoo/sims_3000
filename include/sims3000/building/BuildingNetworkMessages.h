/**
 * @file BuildingNetworkMessages.h
 * @brief Network message definitions for building operations (Ticket 4-040)
 *
 * Defines serializable network messages for building operations:
 * - DemolishRequestMessage: Client requests building demolition
 * - ClearDebrisMessage: Client requests debris clearing
 * - BuildingSpawnedMessage: Server notifies building spawn
 * - BuildingStateChangedMessage: Server notifies state change
 * - BuildingUpgradedMessage: Server notifies building upgrade
 * - ConstructionProgressMessage: Server syncs construction progress
 * - BuildingDemolishedMessage: Server notifies building demolished
 * - DebrisClearedMessage: Server notifies debris cleared
 *
 * All messages use little-endian encoding for multi-byte fields
 * and include a version byte for forward compatibility.
 */

#ifndef SIMS3000_BUILDING_BUILDINGNETWORKMESSAGES_H
#define SIMS3000_BUILDING_BUILDINGNETWORKMESSAGES_H

#include <cstdint>
#include <vector>

namespace sims3000 {
namespace building {

/**
 * @struct DemolishRequestMessage
 * @brief Client -> Server: Request to demolish a building.
 */
struct DemolishRequestMessage {
    std::uint32_t entity_id = 0;
    std::uint8_t version = 1;

    /// Serialize to byte vector (little-endian)
    std::vector<std::uint8_t> serialize() const;

    /// Deserialize from byte vector, returns true on success
    static bool deserialize(const std::vector<std::uint8_t>& data, DemolishRequestMessage& out);
};

/**
 * @struct ClearDebrisMessage
 * @brief Client -> Server: Request to clear debris at a building location.
 */
struct ClearDebrisMessage {
    std::uint32_t entity_id = 0;
    std::uint8_t version = 1;

    /// Serialize to byte vector (little-endian)
    std::vector<std::uint8_t> serialize() const;

    /// Deserialize from byte vector, returns true on success
    static bool deserialize(const std::vector<std::uint8_t>& data, ClearDebrisMessage& out);
};

/**
 * @struct BuildingSpawnedMessage
 * @brief Server -> Client: Notify that a building has spawned.
 */
struct BuildingSpawnedMessage {
    std::uint32_t entity_id = 0;
    std::int32_t grid_x = 0;
    std::int32_t grid_y = 0;
    std::uint32_t template_id = 0;
    std::uint8_t owner_id = 0;
    std::uint8_t rotation = 0;
    std::uint8_t color_accent_index = 0;
    std::uint8_t version = 1;

    /// Serialize to byte vector (little-endian)
    std::vector<std::uint8_t> serialize() const;

    /// Deserialize from byte vector, returns true on success
    static bool deserialize(const std::vector<std::uint8_t>& data, BuildingSpawnedMessage& out);
};

/**
 * @struct BuildingStateChangedMessage
 * @brief Server -> Client: Notify that a building's state has changed.
 */
struct BuildingStateChangedMessage {
    std::uint32_t entity_id = 0;
    std::uint8_t new_state = 0;  ///< BuildingState value
    std::uint8_t version = 1;

    /// Serialize to byte vector (little-endian)
    std::vector<std::uint8_t> serialize() const;

    /// Deserialize from byte vector, returns true on success
    static bool deserialize(const std::vector<std::uint8_t>& data, BuildingStateChangedMessage& out);
};

/**
 * @struct BuildingUpgradedMessage
 * @brief Server -> Client: Notify that a building has been upgraded.
 */
struct BuildingUpgradedMessage {
    std::uint32_t entity_id = 0;
    std::uint8_t new_level = 0;
    std::uint32_t new_template_id = 0;  ///< May change template on upgrade
    std::uint8_t version = 1;

    /// Serialize to byte vector (little-endian)
    std::vector<std::uint8_t> serialize() const;

    /// Deserialize from byte vector, returns true on success
    static bool deserialize(const std::vector<std::uint8_t>& data, BuildingUpgradedMessage& out);
};

/**
 * @struct ConstructionProgressMessage
 * @brief Server -> Client: Sync construction progress (every 10 ticks).
 */
struct ConstructionProgressMessage {
    std::uint32_t entity_id = 0;
    std::uint16_t ticks_elapsed = 0;
    std::uint16_t ticks_total = 0;
    std::uint8_t version = 1;

    /// Serialize to byte vector (little-endian)
    std::vector<std::uint8_t> serialize() const;

    /// Deserialize from byte vector, returns true on success
    static bool deserialize(const std::vector<std::uint8_t>& data, ConstructionProgressMessage& out);
};

/**
 * @struct BuildingDemolishedMessage
 * @brief Server -> Client: Notify that a building has been demolished.
 */
struct BuildingDemolishedMessage {
    std::uint32_t entity_id = 0;
    std::uint8_t version = 1;

    /// Serialize to byte vector (little-endian)
    std::vector<std::uint8_t> serialize() const;

    /// Deserialize from byte vector, returns true on success
    static bool deserialize(const std::vector<std::uint8_t>& data, BuildingDemolishedMessage& out);
};

/**
 * @struct DebrisClearedMessage
 * @brief Server -> Client: Notify that debris has been cleared.
 */
struct DebrisClearedMessage {
    std::uint32_t entity_id = 0;
    std::int32_t grid_x = 0;
    std::int32_t grid_y = 0;
    std::uint8_t version = 1;

    /// Serialize to byte vector (little-endian)
    std::vector<std::uint8_t> serialize() const;

    /// Deserialize from byte vector, returns true on success
    static bool deserialize(const std::vector<std::uint8_t>& data, DebrisClearedMessage& out);
};

} // namespace building
} // namespace sims3000

#endif // SIMS3000_BUILDING_BUILDINGNETWORKMESSAGES_H
