/**
 * @file RailNetworkMessages.h
 * @brief Network message definitions for rail and terminal operations (Ticket E7-039)
 *
 * Defines serializable network messages for rail and terminal placement/demolition:
 * - RailPlaceRequest / RailPlaceResponse
 * - RailDemolishRequest / RailDemolishResponse
 * - TerminalPlaceRequest / TerminalPlaceResponse
 * - TerminalDemolishRequest / TerminalDemolishResponse
 *
 * All multi-byte fields use little-endian encoding.
 * Server validates ownership and energy requirements before responding.
 */

#pragma once

#include <cstdint>
#include <vector>
#include <sims3000/transport/RailComponent.h>
#include <sims3000/transport/TerminalComponent.h>

namespace sims3000 {
namespace transport {

/**
 * @enum RailMessageType
 * @brief Message types for rail and terminal operations.
 */
enum class RailMessageType : uint8_t {
    RailPlaceRequest         = 0,
    RailPlaceResponse        = 1,
    RailDemolishRequest      = 2,
    RailDemolishResponse     = 3,
    TerminalPlaceRequest     = 4,
    TerminalPlaceResponse    = 5,
    TerminalDemolishRequest  = 6,
    TerminalDemolishResponse = 7
};

// ============================================================================
// Rail messages
// ============================================================================

/**
 * @struct RailPlaceRequest
 * @brief Client request to place a rail segment at a grid position.
 *
 * Server validates ownership and energy requirements before placing.
 */
struct RailPlaceRequest {
    int32_t x = 0;               ///< Grid X coordinate
    int32_t y = 0;               ///< Grid Y coordinate
    RailType type = RailType::SurfaceRail;  ///< Rail type to place
    uint8_t owner = 0;           ///< Player ID requesting placement

    /// Serialized size: 4(x) + 4(y) + 1(type) + 1(owner) = 10 bytes
    static constexpr size_t SERIALIZED_SIZE = 10;

    /// Serialize to byte buffer (little-endian)
    void serialize(std::vector<uint8_t>& buffer) const;

    /// Deserialize from raw bytes
    static RailPlaceRequest deserialize(const uint8_t* data, size_t len);
};

/**
 * @struct RailPlaceResponse
 * @brief Server response to a rail placement request.
 *
 * Error codes: 0=ok, 1=occupied, 2=out_of_bounds, 3=invalid_terrain, 4=no_energy
 */
struct RailPlaceResponse {
    bool success = false;         ///< Whether placement succeeded
    uint32_t entity_id = 0;      ///< Assigned entity ID (valid if success)
    uint8_t error_code = 0;      ///< 0=ok, 1=occupied, 2=out_of_bounds, 3=invalid_terrain, 4=no_energy

    /// Serialized size: 1(success) + 4(entity_id) + 1(error_code) = 6 bytes
    static constexpr size_t SERIALIZED_SIZE = 6;

    /// Serialize to byte buffer (little-endian)
    void serialize(std::vector<uint8_t>& buffer) const;

    /// Deserialize from raw bytes
    static RailPlaceResponse deserialize(const uint8_t* data, size_t len);
};

/**
 * @struct RailDemolishRequest
 * @brief Client request to demolish a rail segment.
 *
 * Server validates ownership before demolishing.
 */
struct RailDemolishRequest {
    uint32_t entity_id = 0;      ///< Entity ID of the rail segment to demolish
    uint8_t owner = 0;           ///< Player ID requesting demolition

    /// Serialized size: 4(entity_id) + 1(owner) = 5 bytes
    static constexpr size_t SERIALIZED_SIZE = 5;

    /// Serialize to byte buffer (little-endian)
    void serialize(std::vector<uint8_t>& buffer) const;

    /// Deserialize from raw bytes
    static RailDemolishRequest deserialize(const uint8_t* data, size_t len);
};

/**
 * @struct RailDemolishResponse
 * @brief Server response to a rail demolition request.
 *
 * Error codes: 0=ok, 1=not_found, 2=not_owner
 */
struct RailDemolishResponse {
    bool success = false;         ///< Whether demolition succeeded
    uint32_t entity_id = 0;      ///< Entity ID that was demolished
    uint8_t error_code = 0;      ///< 0=ok, 1=not_found, 2=not_owner

    /// Serialized size: 1(success) + 4(entity_id) + 1(error_code) = 6 bytes
    static constexpr size_t SERIALIZED_SIZE = 6;

    /// Serialize to byte buffer (little-endian)
    void serialize(std::vector<uint8_t>& buffer) const;

    /// Deserialize from raw bytes
    static RailDemolishResponse deserialize(const uint8_t* data, size_t len);
};

// ============================================================================
// Terminal messages
// ============================================================================

/**
 * @struct TerminalPlaceRequest
 * @brief Client request to place a terminal at a grid position.
 *
 * Server validates ownership and energy requirements before placing.
 */
struct TerminalPlaceRequest {
    int32_t x = 0;               ///< Grid X coordinate
    int32_t y = 0;               ///< Grid Y coordinate
    TerminalType type = TerminalType::SurfaceStation;  ///< Terminal type to place
    uint8_t owner = 0;           ///< Player ID requesting placement

    /// Serialized size: 4(x) + 4(y) + 1(type) + 1(owner) = 10 bytes
    static constexpr size_t SERIALIZED_SIZE = 10;

    /// Serialize to byte buffer (little-endian)
    void serialize(std::vector<uint8_t>& buffer) const;

    /// Deserialize from raw bytes
    static TerminalPlaceRequest deserialize(const uint8_t* data, size_t len);
};

/**
 * @struct TerminalPlaceResponse
 * @brief Server response to a terminal placement request.
 *
 * Error codes: 0=ok, 1=occupied, 2=out_of_bounds, 3=invalid_terrain, 4=no_energy
 */
struct TerminalPlaceResponse {
    bool success = false;         ///< Whether placement succeeded
    uint32_t entity_id = 0;      ///< Assigned entity ID (valid if success)
    uint8_t error_code = 0;      ///< 0=ok, 1=occupied, 2=out_of_bounds, 3=invalid_terrain, 4=no_energy

    /// Serialized size: 1(success) + 4(entity_id) + 1(error_code) = 6 bytes
    static constexpr size_t SERIALIZED_SIZE = 6;

    /// Serialize to byte buffer (little-endian)
    void serialize(std::vector<uint8_t>& buffer) const;

    /// Deserialize from raw bytes
    static TerminalPlaceResponse deserialize(const uint8_t* data, size_t len);
};

/**
 * @struct TerminalDemolishRequest
 * @brief Client request to demolish a terminal.
 *
 * Server validates ownership before demolishing.
 */
struct TerminalDemolishRequest {
    uint32_t entity_id = 0;      ///< Entity ID of the terminal to demolish
    uint8_t owner = 0;           ///< Player ID requesting demolition

    /// Serialized size: 4(entity_id) + 1(owner) = 5 bytes
    static constexpr size_t SERIALIZED_SIZE = 5;

    /// Serialize to byte buffer (little-endian)
    void serialize(std::vector<uint8_t>& buffer) const;

    /// Deserialize from raw bytes
    static TerminalDemolishRequest deserialize(const uint8_t* data, size_t len);
};

/**
 * @struct TerminalDemolishResponse
 * @brief Server response to a terminal demolition request.
 *
 * Error codes: 0=ok, 1=not_found, 2=not_owner
 */
struct TerminalDemolishResponse {
    bool success = false;         ///< Whether demolition succeeded
    uint32_t entity_id = 0;      ///< Entity ID that was demolished
    uint8_t error_code = 0;      ///< 0=ok, 1=not_found, 2=not_owner

    /// Serialized size: 1(success) + 4(entity_id) + 1(error_code) = 6 bytes
    static constexpr size_t SERIALIZED_SIZE = 6;

    /// Serialize to byte buffer (little-endian)
    void serialize(std::vector<uint8_t>& buffer) const;

    /// Deserialize from raw bytes
    static TerminalDemolishResponse deserialize(const uint8_t* data, size_t len);
};

} // namespace transport
} // namespace sims3000
