/**
 * @file ZoneNetworkMessages.h
 * @brief Network message definitions for zone operations (Ticket 4-038)
 *
 * Defines serializable network messages for zone operations:
 * - ZonePlacementRequestMsg: Client requests zone placement
 * - DezoneRequestMsg: Client requests zone removal
 * - RedesignateRequestMsg: Client requests zone type/density change
 * - ZoneDemandSyncMsg: Server syncs demand values to clients
 *
 * All messages use little-endian encoding for multi-byte fields
 * and include a version byte for forward compatibility.
 */

#ifndef SIMS3000_ZONE_ZONENETWORKMESSAGES_H
#define SIMS3000_ZONE_ZONENETWORKMESSAGES_H

#include <cstdint>
#include <vector>

namespace sims3000 {
namespace zone {

/**
 * @struct ZonePlacementRequestMsg
 * @brief Network message for requesting zone placement in a rectangular area.
 */
struct ZonePlacementRequestMsg {
    std::int32_t x = 0;
    std::int32_t y = 0;
    std::int32_t width = 0;
    std::int32_t height = 0;
    std::uint8_t zone_type = 0;   ///< ZoneType value
    std::uint8_t density = 0;     ///< ZoneDensity value
    std::uint8_t version = 1;

    /// Serialize to byte vector (little-endian)
    std::vector<std::uint8_t> serialize() const;

    /// Deserialize from byte vector, returns true on success
    static bool deserialize(const std::vector<std::uint8_t>& data, ZonePlacementRequestMsg& out);
};

/**
 * @struct DezoneRequestMsg
 * @brief Network message for requesting zone removal in a rectangular area.
 */
struct DezoneRequestMsg {
    std::int32_t x = 0;
    std::int32_t y = 0;
    std::int32_t width = 0;
    std::int32_t height = 0;
    std::uint8_t version = 1;

    /// Serialize to byte vector (little-endian)
    std::vector<std::uint8_t> serialize() const;

    /// Deserialize from byte vector, returns true on success
    static bool deserialize(const std::vector<std::uint8_t>& data, DezoneRequestMsg& out);
};

/**
 * @struct RedesignateRequestMsg
 * @brief Network message for requesting zone type/density change at a position.
 */
struct RedesignateRequestMsg {
    std::int32_t x = 0;
    std::int32_t y = 0;
    std::uint8_t new_zone_type = 0;
    std::uint8_t new_density = 0;
    std::uint8_t version = 1;

    /// Serialize to byte vector (little-endian)
    std::vector<std::uint8_t> serialize() const;

    /// Deserialize from byte vector, returns true on success
    static bool deserialize(const std::vector<std::uint8_t>& data, RedesignateRequestMsg& out);
};

/**
 * @struct ZoneDemandSyncMsg
 * @brief Network message for syncing zone demand values from server to clients.
 */
struct ZoneDemandSyncMsg {
    std::uint8_t player_id = 0;
    std::int8_t habitation_demand = 0;
    std::int8_t exchange_demand = 0;
    std::int8_t fabrication_demand = 0;
    std::uint8_t version = 1;

    /// Serialize to byte vector (little-endian)
    std::vector<std::uint8_t> serialize() const;

    /// Deserialize from byte vector, returns true on success
    static bool deserialize(const std::vector<std::uint8_t>& data, ZoneDemandSyncMsg& out);
};

} // namespace zone
} // namespace sims3000

#endif // SIMS3000_ZONE_ZONENETWORKMESSAGES_H
