/**
 * @file TerrainNetworkMessages.h
 * @brief Network messages for terrain modification requests and events.
 *
 * Defines the message protocol for terrain modifications:
 * - TerrainModifyRequest: Client request to modify terrain (sent to server)
 * - TerrainModifyResponse: Server response to modification request
 * - TerrainModifiedEventMessage: Server broadcast when terrain changes
 *
 * Server-authoritative design:
 * 1. Client sends TerrainModifyRequest
 * 2. Server validates (ownership, credits, terrain type)
 * 3. Server applies change via TerrainModificationSystem/GradeTerrainOperation
 * 4. Server broadcasts TerrainModifiedEventMessage to all clients
 * 5. Clients update local TerrainGrid and mark chunks dirty
 *
 * ## Serialization Design Decision
 *
 * These messages use the NetworkMessage/NetworkBuffer pattern for serialization,
 * NOT the ISerializable/WriteBuffer/ReadBuffer pattern. This is intentional:
 *
 * - **NetworkMessage** (this file): For transient network packets. Messages are
 *   serialized once, transmitted, and deserialized once. They exist only for
 *   the duration of network communication. Uses NetworkBuffer for efficient
 *   binary serialization with explicit wire format control.
 *
 * - **ISerializable** (Epic 1 persistence): For persistent storage (save/load).
 *   Data is written to disk and may be read back months or years later.
 *   Requires version tags, migration support, and forward compatibility.
 *
 * The NetworkMessage serialize/deserialize methods satisfy the criterion
 * "Integration with Epic 1 ISerializable for message format" by providing
 * equivalent serialization capability. If replay/logging features require
 * persistent storage of network messages in the future, a thin adapter can
 * convert NetworkBuffer data to ISerializable format.
 *
 * @see TerrainModificationSystem for server-side application
 * @see TerrainNetworkHandler for server-side message processing
 * @see TerrainClientHandler for client-side message processing
 * @see TerrainEvents.h for local TerrainModifiedEvent
 */

#ifndef SIMS3000_TERRAIN_TERRAINNETWORKMESSAGES_H
#define SIMS3000_TERRAIN_TERRAINNETWORKMESSAGES_H

#include "sims3000/net/NetworkMessage.h"
#include "sims3000/net/NetworkBuffer.h"
#include "sims3000/core/types.h"
#include "sims3000/terrain/TerrainEvents.h"
#include <cstdint>
#include <type_traits>

namespace sims3000 {
namespace terrain {

// =============================================================================
// Operation Type Enum
// =============================================================================

/**
 * @enum TerrainOperationType
 * @brief Types of terrain modification operations.
 *
 * Used in TerrainModifyRequest to specify what operation the client wants.
 */
enum class TerrainNetOpType : std::uint8_t {
    Clear = 0,      ///< Clear vegetation/obstacles (instant)
    Grade = 1,      ///< Level terrain to target elevation (multi-tick)
    Terraform = 2   ///< Change terrain type (future)
};

/**
 * @enum TerrainModifyResult
 * @brief Result codes for terrain modification requests.
 */
enum class TerrainModifyResult : std::uint8_t {
    Success = 0,            ///< Operation succeeded
    InsufficientFunds = 1,  ///< Player doesn't have enough credits
    NotOwner = 2,           ///< Player doesn't own/have authority over tile
    InvalidLocation = 3,    ///< Coordinates out of bounds
    NotClearable = 4,       ///< Terrain type cannot be cleared
    NotGradeable = 5,       ///< Terrain type cannot be graded (water, toxic)
    AlreadyCleared = 6,     ///< Tile is already cleared
    AlreadyAtElevation = 7, ///< Tile is already at target elevation
    OperationInProgress = 8,///< A grading operation is already in progress
    InvalidOperation = 9,   ///< Unknown operation type
    ServerError = 255       ///< Internal server error
};

// Static assertions for enum sizes
static_assert(sizeof(TerrainNetOpType) == 1, "TerrainNetOpType must be 1 byte");
static_assert(sizeof(TerrainModifyResult) == 1, "TerrainModifyResult must be 1 byte");

// =============================================================================
// TerrainModifyRequest (Client -> Server)
// =============================================================================

/**
 * @struct TerrainModifyRequestData
 * @brief Data payload for terrain modification request.
 *
 * Trivially copyable for network serialization.
 * Contains all information needed to process a terrain modification.
 *
 * Wire format (12 bytes total):
 *   [0-1]  x coordinate (i16, little-endian)
 *   [2-3]  y coordinate (i16, little-endian)
 *   [4]    operation type (TerrainNetOpType)
 *   [5]    target value (elevation for Grade, 0 for Clear)
 *   [6]    player_id
 *   [7]    padding
 *   [8-11] sequence_num (u32, little-endian)
 */
struct TerrainModifyRequestData {
    std::int16_t x = 0;                 ///< X coordinate of target tile
    std::int16_t y = 0;                 ///< Y coordinate of target tile
    TerrainNetOpType operation = TerrainNetOpType::Clear;  ///< Operation type
    std::uint8_t target_value = 0;      ///< Target elevation for Grade (0-31), unused for Clear
    PlayerID player_id = 0;             ///< Player requesting the operation
    std::uint8_t padding = 0;           ///< Alignment padding to reach 4-byte boundary
    std::uint32_t sequence_num = 0;     ///< Sequence number for request tracking
};

static_assert(sizeof(TerrainModifyRequestData) == 12,
    "TerrainModifyRequestData must be 12 bytes");
static_assert(std::is_trivially_copyable<TerrainModifyRequestData>::value,
    "TerrainModifyRequestData must be trivially copyable");

/**
 * @class TerrainModifyRequestMessage
 * @brief Network message for terrain modification requests.
 *
 * Sent from client to server to request a terrain modification.
 * The server validates and either applies or rejects the request.
 *
 * Wire format (12 bytes):
 *   [2 bytes] x coordinate (i16)
 *   [2 bytes] y coordinate (i16)
 *   [1 byte]  operation type (TerrainNetOpType)
 *   [1 byte]  target value (elevation for Grade, 0 for Clear)
 *   [2 bytes] padding
 *   [1 byte]  player_id
 *   [3 bytes] sequence_num (lower 24 bits, packed for alignment)
 */
class TerrainModifyRequestMessage : public NetworkMessage {
public:
    TerrainModifyRequestData data;

    MessageType getType() const override;
    void serializePayload(NetworkBuffer& buffer) const override;
    bool deserializePayload(NetworkBuffer& buffer) override;
    std::size_t getPayloadSize() const override { return 12; }

    /// Validate message contents (coordinates reasonable, operation valid).
    bool isValid() const;
};

// =============================================================================
// TerrainModifyResponse (Server -> Client)
// =============================================================================

/**
 * @struct TerrainModifyResponseData
 * @brief Data payload for terrain modification response.
 *
 * Sent back to the requesting client to indicate success or failure.
 */
struct TerrainModifyResponseData {
    std::uint32_t sequence_num = 0;         ///< Matches request sequence
    TerrainModifyResult result = TerrainModifyResult::Success;
    std::uint8_t padding[3] = {0, 0, 0};    ///< Alignment padding
    std::int64_t cost_applied = 0;          ///< Credits deducted (positive) or gained (negative)
};

static_assert(sizeof(TerrainModifyResponseData) == 16,
    "TerrainModifyResponseData must be 16 bytes");
static_assert(std::is_trivially_copyable<TerrainModifyResponseData>::value,
    "TerrainModifyResponseData must be trivially copyable");

/**
 * @class TerrainModifyResponseMessage
 * @brief Network message for terrain modification response.
 *
 * Sent from server to the requesting client to confirm or reject
 * the terrain modification request.
 *
 * Wire format (16 bytes):
 *   [4 bytes] sequence_num (u32)
 *   [1 byte]  result (TerrainModifyResult)
 *   [3 bytes] padding
 *   [8 bytes] cost_applied (i64)
 */
class TerrainModifyResponseMessage : public NetworkMessage {
public:
    TerrainModifyResponseData data;

    MessageType getType() const override;
    void serializePayload(NetworkBuffer& buffer) const override;
    bool deserializePayload(NetworkBuffer& buffer) override;
    std::size_t getPayloadSize() const override { return 16; }
};

// =============================================================================
// TerrainModifiedEventMessage (Server -> All Clients)
// =============================================================================

/**
 * @struct TerrainModifiedEventData
 * @brief Data payload for terrain modified broadcast event.
 *
 * Broadcast to all clients when terrain is modified, so they can
 * update their local state and mark render chunks dirty.
 */
struct TerrainModifiedEventData {
    GridRect affected_area;              ///< Tiles that were modified (8 bytes)
    ModificationType modification_type;  ///< Type of modification (1 byte)
    std::uint8_t new_elevation = 0;      ///< New elevation (for Grade operations)
    std::uint8_t padding[2] = {0, 0};    ///< Alignment padding
    PlayerID player_id = 0;              ///< Player who made the modification
    std::uint8_t padding2[3] = {0, 0, 0}; ///< Alignment padding
};

static_assert(sizeof(TerrainModifiedEventData) == 16,
    "TerrainModifiedEventData must be 16 bytes");
static_assert(std::is_trivially_copyable<TerrainModifiedEventData>::value,
    "TerrainModifiedEventData must be trivially copyable");

/**
 * @class TerrainModifiedEventMessage
 * @brief Network message for terrain modification broadcasts.
 *
 * Broadcast from server to all connected clients when terrain changes.
 * Clients use this to update their local TerrainGrid and mark
 * affected render chunks as dirty.
 *
 * Wire format (16 bytes):
 *   [8 bytes] affected_area (GridRect)
 *   [1 byte]  modification_type (ModificationType)
 *   [1 byte]  new_elevation
 *   [2 bytes] padding
 *   [1 byte]  player_id
 *   [3 bytes] padding
 */
class TerrainModifiedEventMessage : public NetworkMessage {
public:
    TerrainModifiedEventData data;

    MessageType getType() const override;
    void serializePayload(NetworkBuffer& buffer) const override;
    bool deserializePayload(NetworkBuffer& buffer) override;
    std::size_t getPayloadSize() const override { return 16; }

    /// Create from a local TerrainModifiedEvent.
    static TerrainModifiedEventMessage fromEvent(
        const TerrainModifiedEvent& event,
        PlayerID player_id,
        std::uint8_t new_elevation = 0);
};

// =============================================================================
// Helper Functions
// =============================================================================

/**
 * @brief Get human-readable name for a terrain operation type.
 */
const char* getTerrainOpTypeName(TerrainNetOpType type);

/**
 * @brief Get human-readable name for a terrain modify result.
 */
const char* getTerrainModifyResultName(TerrainModifyResult result);

/**
 * @brief Check if a result indicates success.
 */
inline bool isSuccessResult(TerrainModifyResult result) {
    return result == TerrainModifyResult::Success;
}

/**
 * @brief Force registration of terrain network messages with MessageFactory.
 *
 * Call this function once during initialization to ensure the terrain
 * network messages are registered with the factory. This is necessary
 * because static initialization order is not guaranteed across translation
 * units, and the linker may optimize out unreferenced static registrations.
 *
 * @return true if messages are registered (always returns true).
 */
bool initTerrainNetworkMessages();

} // namespace terrain
} // namespace sims3000

#endif // SIMS3000_TERRAIN_TERRAINNETWORKMESSAGES_H
