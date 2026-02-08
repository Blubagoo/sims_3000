/**
 * @file PathwayNetworkMessages.h
 * @brief Network message definitions for pathway operations (Ticket E7-038)
 *
 * Defines serializable network messages for pathway placement and demolition:
 * - PathwayPlaceRequest: Client requests pathway placement
 * - PathwayPlaceResponse: Server confirms/rejects placement
 * - PathwayDemolishRequest: Client requests pathway demolition
 * - PathwayDemolishResponse: Server confirms/rejects demolition
 *
 * All multi-byte fields use little-endian encoding.
 * Server validates ownership and terrain before responding.
 */

#pragma once

#include <cstdint>
#include <vector>
#include <sims3000/transport/TransportEnums.h>

namespace sims3000 {
namespace transport {

/**
 * @enum PathwayMessageType
 * @brief Message types for pathway operations.
 */
enum class PathwayMessageType : uint8_t {
    PlaceRequest     = 0,
    PlaceResponse    = 1,
    DemolishRequest  = 2,
    DemolishResponse = 3
};

/**
 * @struct PathwayPlaceRequest
 * @brief Client request to place a pathway segment at a grid position.
 *
 * Server validates ownership and terrain before placing.
 */
struct PathwayPlaceRequest {
    int32_t x = 0;               ///< Grid X coordinate
    int32_t y = 0;               ///< Grid Y coordinate
    PathwayType type = PathwayType::BasicPathway;  ///< Pathway type to place
    uint8_t owner = 0;           ///< Player ID requesting placement

    /// Serialized size on the wire: 4(x) + 4(y) + 1(type) + 1(owner) = 10 bytes
    static constexpr size_t SERIALIZED_SIZE = 10;

    /// Serialize to byte buffer (little-endian)
    void serialize(std::vector<uint8_t>& buffer) const;

    /// Deserialize from raw bytes
    static PathwayPlaceRequest deserialize(const uint8_t* data, size_t len);
};

/**
 * @struct PathwayPlaceResponse
 * @brief Server response to a pathway placement request.
 *
 * Contains success status, assigned entity ID, and error code.
 * Error codes: 0=ok, 1=occupied, 2=out_of_bounds, 3=invalid_terrain
 */
struct PathwayPlaceResponse {
    bool success = false;         ///< Whether placement succeeded
    uint32_t entity_id = 0;      ///< Assigned entity ID (valid if success)
    int32_t x = 0;               ///< Echoed grid X coordinate
    int32_t y = 0;               ///< Echoed grid Y coordinate
    uint8_t error_code = 0;      ///< 0=ok, 1=occupied, 2=out_of_bounds, 3=invalid_terrain

    /// Serialized size: 1(success) + 4(entity_id) + 4(x) + 4(y) + 1(error_code) = 14 bytes
    static constexpr size_t SERIALIZED_SIZE = 14;

    /// Serialize to byte buffer (little-endian)
    void serialize(std::vector<uint8_t>& buffer) const;

    /// Deserialize from raw bytes
    static PathwayPlaceResponse deserialize(const uint8_t* data, size_t len);
};

/**
 * @struct PathwayDemolishRequest
 * @brief Client request to demolish a pathway segment.
 *
 * Server validates ownership before demolishing.
 */
struct PathwayDemolishRequest {
    uint32_t entity_id = 0;      ///< Entity ID of the pathway to demolish
    int32_t x = 0;               ///< Grid X coordinate
    int32_t y = 0;               ///< Grid Y coordinate
    uint8_t owner = 0;           ///< Player ID requesting demolition

    /// Serialized size: 4(entity_id) + 4(x) + 4(y) + 1(owner) = 13 bytes
    static constexpr size_t SERIALIZED_SIZE = 13;

    /// Serialize to byte buffer (little-endian)
    void serialize(std::vector<uint8_t>& buffer) const;

    /// Deserialize from raw bytes
    static PathwayDemolishRequest deserialize(const uint8_t* data, size_t len);
};

/**
 * @struct PathwayDemolishResponse
 * @brief Server response to a pathway demolition request.
 *
 * Error codes: 0=ok, 1=not_found, 2=not_owner
 */
struct PathwayDemolishResponse {
    bool success = false;         ///< Whether demolition succeeded
    uint32_t entity_id = 0;      ///< Entity ID that was demolished
    uint8_t error_code = 0;      ///< 0=ok, 1=not_found, 2=not_owner

    /// Serialized size: 1(success) + 4(entity_id) + 1(error_code) = 6 bytes
    static constexpr size_t SERIALIZED_SIZE = 6;

    /// Serialize to byte buffer (little-endian)
    void serialize(std::vector<uint8_t>& buffer) const;

    /// Deserialize from raw bytes
    static PathwayDemolishResponse deserialize(const uint8_t* data, size_t len);
};

} // namespace transport
} // namespace sims3000
