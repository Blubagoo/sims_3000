/**
 * @file ServerMessages.h
 * @brief Server-to-client network message types.
 *
 * Defines all messages sent from server to client:
 * - StateUpdateMessage: Delta state updates per tick
 * - FullStateMessage: Complete world snapshot for reconnection
 * - PlayerListMessage: Current players with connection status
 * - RejectionMessage: Action rejection with reason
 * - EventMessage: Game events (disasters, milestones)
 * - HeartbeatResponseMessage: Server heartbeat response
 * - ServerStatusMessage: Server state and configuration
 *
 * Large messages (>1KB) use LZ4 compression. The compression flag
 * is stored in the message header byte 0 (high bit of protocol version).
 */

#ifndef SIMS3000_NET_SERVERMESSAGES_H
#define SIMS3000_NET_SERVERMESSAGES_H

#include "sims3000/net/NetworkMessage.h"
#include "sims3000/net/NetworkBuffer.h"
#include "sims3000/core/types.h"
#include <array>
#include <cstdint>
#include <vector>
#include <string>

namespace sims3000 {

// =============================================================================
// Constants
// =============================================================================

/// Chunk size for snapshot transmission (64KB)
constexpr std::size_t SNAPSHOT_CHUNK_SIZE = 64 * 1024;

/// Compression threshold - messages larger than this get compressed (1KB)
constexpr std::size_t COMPRESSION_THRESHOLD = 1024;

/// Maximum entity deltas per StateUpdate (prevents oversized messages)
constexpr std::size_t MAX_ENTITY_DELTAS_PER_MESSAGE = 1000;

// =============================================================================
// Server Status Enums
// =============================================================================

/**
 * @enum ServerState
 * @brief Server operational state.
 */
enum class ServerState : std::uint8_t {
    Loading = 0,   // Server is loading resources/world
    Ready = 1,     // Server is ready but game not started
    Running = 2,   // Game is actively running
    Paused = 3,    // Game is paused
    Stopping = 4   // Server is shutting down
};

// NOTE: MapSizeTier is defined in sims3000/core/types.h (canonical location)
// This enum is used throughout the codebase, not just networking.

/**
 * @enum PlayerStatus
 * @brief Player connection status.
 */
enum class PlayerStatus : std::uint8_t {
    Connecting = 0,   // Connection in progress
    Connected = 1,    // Fully connected and playing
    Disconnected = 2, // Gracefully disconnected
    TimedOut = 3,     // Connection lost (timeout)
    Kicked = 4        // Kicked by server
};

/**
 * @enum EntityDeltaType
 * @brief Type of entity delta in state update.
 */
enum class EntityDeltaType : std::uint8_t {
    Create = 0,   // New entity with all components
    Update = 1,   // Existing entity with changed components
    Destroy = 2   // Entity removed
};

/**
 * @enum RejectionReason
 * @brief Reason codes for action rejection.
 */
enum class RejectionReason : std::uint8_t {
    None = 0,
    InsufficientFunds = 1,
    InvalidLocation = 2,
    AreaOccupied = 3,
    NotOwner = 4,
    BuildingLimitReached = 5,
    InvalidBuildingType = 6,
    ZoneConflict = 7,
    InfrastructureRequired = 8,
    ActionNotAllowed = 9,
    ServerBusy = 10,
    RateLimited = 11,
    InvalidInput = 12,
    Unknown = 255
};

/**
 * @enum GameEventType
 * @brief Types of game events.
 */
enum class GameEventType : std::uint8_t {
    None = 0,
    MilestoneReached = 1,      // Population/progress milestone
    DisasterStarted = 2,       // Disaster event began
    DisasterEnded = 3,         // Disaster event ended
    BuildingCompleted = 4,     // Major building finished
    BudgetAlert = 5,           // Low funds warning
    PopulationChange = 6,      // Significant population change
    TradeCompleted = 7,        // Trade between players completed
    PlayerAction = 8           // Notable player action
};

// =============================================================================
// Helper Structures
// =============================================================================

/**
 * @struct EntityDelta
 * @brief Represents a change to a single entity.
 *
 * Used in StateUpdateMessage to communicate entity changes.
 * For Create/Update: componentData contains serialized components.
 * For Destroy: componentData is empty.
 */
struct EntityDelta {
    EntityID entityId = 0;
    EntityDeltaType type = EntityDeltaType::Update;
    std::vector<std::uint8_t> componentData;  // Serialized components

    void serialize(NetworkBuffer& buffer) const;
    bool deserialize(NetworkBuffer& buffer);
};

/**
 * @struct PlayerInfo
 * @brief Player information for PlayerListMessage.
 */
struct PlayerInfo {
    PlayerID playerId = 0;
    std::string name;
    PlayerStatus status = PlayerStatus::Disconnected;
    std::uint32_t latencyMs = 0;  // RTT in milliseconds

    void serialize(NetworkBuffer& buffer) const;
    bool deserialize(NetworkBuffer& buffer);
};

// =============================================================================
// StateUpdateMessage (MessageType::StateUpdate = 102)
// =============================================================================

/**
 * @class StateUpdateMessage
 * @brief Delta state update sent each tick.
 *
 * Contains only entities that changed since the last update.
 * Tick number is used for ordering and duplicate detection.
 */
class StateUpdateMessage : public NetworkMessage {
public:
    SimulationTick tick = 0;            // Server tick number
    std::vector<EntityDelta> deltas;    // Changed entities
    bool compressed = false;            // True if payload was compressed

    MessageType getType() const override { return MessageType::StateUpdate; }
    void serializePayload(NetworkBuffer& buffer) const override;
    bool deserializePayload(NetworkBuffer& buffer) override;
    std::size_t getPayloadSize() const override;

    /// Add a create delta for a new entity.
    void addCreate(EntityID id, const std::vector<std::uint8_t>& components);

    /// Add an update delta for an existing entity.
    void addUpdate(EntityID id, const std::vector<std::uint8_t>& components);

    /// Add a destroy delta for a removed entity.
    void addDestroy(EntityID id);

    /// Clear all deltas.
    void clear();

    /// Check if there are any deltas.
    bool hasDeltas() const { return !deltas.empty(); }

private:
    /// Parse delta data from uncompressed buffer (used by deserializePayload).
    bool parseUncompressedDeltas(NetworkBuffer& buffer);
};

// =============================================================================
// FullStateMessage (Used with SnapshotStart/Chunk/End)
// =============================================================================

/**
 * @class SnapshotStartMessage
 * @brief Marks the beginning of a snapshot transfer.
 *
 * Sent before the first SnapshotChunk. Contains metadata about the snapshot.
 */
class SnapshotStartMessage : public NetworkMessage {
public:
    SimulationTick tick = 0;          // Tick when snapshot was taken
    std::uint32_t totalChunks = 0;    // Number of chunks to follow
    std::uint32_t totalBytes = 0;     // Total uncompressed size
    std::uint32_t compressedBytes = 0;// Total compressed size
    std::uint32_t entityCount = 0;    // Number of entities in snapshot

    MessageType getType() const override { return MessageType::SnapshotStart; }
    void serializePayload(NetworkBuffer& buffer) const override;
    bool deserializePayload(NetworkBuffer& buffer) override;
    std::size_t getPayloadSize() const override { return 8 + 4 + 4 + 4 + 4; }
};

/**
 * @class SnapshotChunkMessage
 * @brief A chunk of snapshot data.
 *
 * Snapshots are split into 64KB chunks for transmission.
 * Chunks are numbered sequentially starting from 0.
 */
class SnapshotChunkMessage : public NetworkMessage {
public:
    std::uint32_t chunkIndex = 0;        // 0-based chunk number
    std::vector<std::uint8_t> data;      // Chunk data (max 64KB)

    MessageType getType() const override { return MessageType::SnapshotChunk; }
    void serializePayload(NetworkBuffer& buffer) const override;
    bool deserializePayload(NetworkBuffer& buffer) override;
    std::size_t getPayloadSize() const override { return 4 + 4 + data.size(); }
};

/**
 * @class SnapshotEndMessage
 * @brief Marks completion of a snapshot transfer.
 *
 * Sent after the last SnapshotChunk. Client can now apply the snapshot.
 */
class SnapshotEndMessage : public NetworkMessage {
public:
    std::uint32_t checksum = 0;   // CRC32 of uncompressed data

    MessageType getType() const override { return MessageType::SnapshotEnd; }
    void serializePayload(NetworkBuffer& buffer) const override;
    bool deserializePayload(NetworkBuffer& buffer) override;
    std::size_t getPayloadSize() const override { return 4; }
};

// =============================================================================
// PlayerListMessage (MessageType::PlayerList = 10)
// =============================================================================

/**
 * @class PlayerListMessage
 * @brief Current player list with connection status.
 *
 * Broadcast when players join, leave, or status changes.
 */
class PlayerListMessage : public NetworkMessage {
public:
    std::vector<PlayerInfo> players;

    MessageType getType() const override { return MessageType::PlayerList; }
    void serializePayload(NetworkBuffer& buffer) const override;
    bool deserializePayload(NetworkBuffer& buffer) override;
    std::size_t getPayloadSize() const override;

    /// Add a player to the list.
    void addPlayer(PlayerID id, const std::string& name,
                   PlayerStatus status, std::uint32_t latencyMs = 0);

    /// Find a player by ID (nullptr if not found).
    const PlayerInfo* findPlayer(PlayerID id) const;

    /// Clear the player list.
    void clear() { players.clear(); }
};

// =============================================================================
// RejectionMessage (MessageType::Rejection = 103)
// =============================================================================

/**
 * @class RejectionMessage
 * @brief Server rejection of a player action.
 *
 * Sent when the server cannot process a client's input.
 * Contains both a machine-readable reason code and human-readable message.
 */
class RejectionMessage : public NetworkMessage {
public:
    std::uint32_t inputSequenceNum = 0;  // Matches InputMessage::sequenceNum
    RejectionReason reason = RejectionReason::None;
    std::string message;                  // Human-readable explanation

    MessageType getType() const override { return MessageType::Rejection; }
    void serializePayload(NetworkBuffer& buffer) const override;
    bool deserializePayload(NetworkBuffer& buffer) override;
    std::size_t getPayloadSize() const override { return 4 + 1 + 4 + message.size(); }

    /// Get a default message for a rejection reason.
    static const char* getDefaultMessage(RejectionReason reason);
};

// =============================================================================
// EventMessage (MessageType::Event = 104)
// =============================================================================

/**
 * @class EventMessage
 * @brief Game event notification.
 *
 * Used for disasters, milestones, and other game events.
 * This is a placeholder structure for future expansion.
 */
class EventMessage : public NetworkMessage {
public:
    SimulationTick tick = 0;           // When event occurred
    GameEventType eventType = GameEventType::None;
    EntityID relatedEntity = 0;        // Entity involved (0 = none)
    GridPosition location{0, 0};       // Location if applicable
    std::uint32_t param1 = 0;          // Event-specific parameter
    std::uint32_t param2 = 0;          // Event-specific parameter
    std::string description;           // Human-readable description

    MessageType getType() const override { return MessageType::Event; }
    void serializePayload(NetworkBuffer& buffer) const override;
    bool deserializePayload(NetworkBuffer& buffer) override;
    std::size_t getPayloadSize() const override;
};

// =============================================================================
// HeartbeatResponseMessage (MessageType::HeartbeatResponse = 2)
// =============================================================================

/**
 * @class HeartbeatResponseMessage
 * @brief Server response to client heartbeat.
 *
 * Contains the client's original timestamp for RTT calculation.
 */
class HeartbeatResponseMessage : public NetworkMessage {
public:
    std::uint64_t clientTimestamp = 0;  // Echo of client's timestamp
    std::uint64_t serverTimestamp = 0;  // Server's current timestamp
    SimulationTick serverTick = 0;      // Current server tick

    MessageType getType() const override { return MessageType::HeartbeatResponse; }
    void serializePayload(NetworkBuffer& buffer) const override;
    bool deserializePayload(NetworkBuffer& buffer) override;
    std::size_t getPayloadSize() const override { return 8 + 8 + 8; }
};

// =============================================================================
// ServerStatusMessage (MessageType::ServerStatus = 9)
// =============================================================================

/**
 * @class ServerStatusMessage
 * @brief Server state and configuration information.
 *
 * Sent on connection and when server state changes.
 * Includes map size tier and dimensions so clients know world size.
 */
class ServerStatusMessage : public NetworkMessage {
public:
    ServerState state = ServerState::Loading;
    MapSizeTier mapSizeTier = MapSizeTier::Medium;
    std::uint16_t mapWidth = 256;      // Grid width
    std::uint16_t mapHeight = 256;     // Grid height
    std::uint8_t maxPlayers = 4;       // Maximum players allowed
    std::uint8_t currentPlayers = 0;   // Current player count
    SimulationTick currentTick = 0;    // Current simulation tick
    std::string serverName;            // Server display name

    MessageType getType() const override { return MessageType::ServerStatus; }
    void serializePayload(NetworkBuffer& buffer) const override;
    bool deserializePayload(NetworkBuffer& buffer) override;
    std::size_t getPayloadSize() const override;

    /// Get map dimensions for a given tier.
    static void getDimensionsForTier(MapSizeTier tier,
                                     std::uint16_t& width, std::uint16_t& height);
};

// =============================================================================
// Compression Utilities
// =============================================================================

/**
 * @brief Compress data using LZ4.
 * @param input Input data to compress.
 * @param output Output buffer (will be resized).
 * @return true if compression succeeded, false on error.
 *
 * Output format: [4 bytes original size][compressed data]
 */
bool compressLZ4(const std::vector<std::uint8_t>& input,
                 std::vector<std::uint8_t>& output);

/**
 * @brief Decompress LZ4-compressed data.
 * @param input Compressed input data (with size prefix).
 * @param output Output buffer (will be resized to original size).
 * @return true if decompression succeeded, false on error.
 */
bool decompressLZ4(const std::vector<std::uint8_t>& input,
                   std::vector<std::uint8_t>& output);

/**
 * @brief Split data into chunks for snapshot transmission.
 * @param data Data to split.
 * @param chunkSize Maximum chunk size (default: SNAPSHOT_CHUNK_SIZE).
 * @return Vector of chunks.
 */
std::vector<std::vector<std::uint8_t>> splitIntoChunks(
    const std::vector<std::uint8_t>& data,
    std::size_t chunkSize = SNAPSHOT_CHUNK_SIZE);

/**
 * @brief Reassemble chunks into original data.
 * @param chunks Chunks to reassemble (must be in order).
 * @return Reassembled data.
 */
std::vector<std::uint8_t> reassembleChunks(
    const std::vector<std::vector<std::uint8_t>>& chunks);

// =============================================================================
// Static Assertions
// =============================================================================

static_assert(sizeof(ServerState) == 1, "ServerState must be 1 byte");
// MapSizeTier static_assert is in types.h (canonical location)
static_assert(sizeof(PlayerStatus) == 1, "PlayerStatus must be 1 byte");
static_assert(sizeof(EntityDeltaType) == 1, "EntityDeltaType must be 1 byte");
static_assert(sizeof(RejectionReason) == 1, "RejectionReason must be 1 byte");
static_assert(sizeof(GameEventType) == 1, "GameEventType must be 1 byte");
static_assert(SNAPSHOT_CHUNK_SIZE == 65536, "Chunk size must be 64KB");

// =============================================================================
// JoinAcceptMessage (MessageType::JoinAccept = 4)
// =============================================================================

/**
 * @class JoinAcceptMessage
 * @brief Server acceptance of player join request.
 *
 * Sent when the server accepts a player's JoinMessage. Contains the assigned
 * PlayerID (1-4), a 128-bit session token for reconnection, and current
 * server tick.
 *
 * Wire format (little-endian):
 *   [1 byte]  playerId - Assigned player ID (1-4)
 *   [16 bytes] sessionToken - 128-bit session token for reconnection
 *   [8 bytes] serverTick - Current simulation tick
 *
 * Payload size: 25 bytes (fixed)
 */
class JoinAcceptMessage : public NetworkMessage {
public:
    /// Assigned player ID (1-4, 0 is reserved for GAME_MASTER).
    PlayerID playerId = 0;

    /// Session token for reconnection (128-bit random).
    std::array<std::uint8_t, 16> sessionToken{};

    /// Current server tick when join was accepted.
    SimulationTick serverTick = 0;

    MessageType getType() const override { return MessageType::JoinAccept; }
    void serializePayload(NetworkBuffer& buffer) const override;
    bool deserializePayload(NetworkBuffer& buffer) override;
    std::size_t getPayloadSize() const override { return 25; }
};

// =============================================================================
// JoinRejectMessage (MessageType::JoinReject = 5)
// =============================================================================

/**
 * @enum JoinRejectReason
 * @brief Reason codes for join rejection.
 */
enum class JoinRejectReason : std::uint8_t {
    Unknown = 0,
    ServerFull = 1,
    InvalidName = 2,
    Banned = 3,
    InvalidToken = 4,
    SessionExpired = 5
};

/**
 * @class JoinRejectMessage
 * @brief Server rejection of player join request.
 *
 * Wire format (little-endian):
 *   [1 byte]  reason - JoinRejectReason code
 *   [4 bytes] message length
 *   [N bytes] message - Human-readable explanation
 *
 * Payload size: 5 + message.length()
 */
class JoinRejectMessage : public NetworkMessage {
public:
    JoinRejectReason reason = JoinRejectReason::Unknown;
    std::string message;

    MessageType getType() const override { return MessageType::JoinReject; }
    void serializePayload(NetworkBuffer& buffer) const override;
    bool deserializePayload(NetworkBuffer& buffer) override;
    std::size_t getPayloadSize() const override { return 5 + message.size(); }

    /// Get a default message for a rejection reason.
    static const char* getDefaultMessage(JoinRejectReason reason);
};

// =============================================================================
// KickMessage (MessageType::Kick = 8)
// =============================================================================

/**
 * @class KickMessage
 * @brief Server notification of player being kicked.
 *
 * Wire format (little-endian):
 *   [4 bytes] reason length
 *   [N bytes] reason string
 *
 * Payload size: 4 + reason.length()
 */
class KickMessage : public NetworkMessage {
public:
    std::string reason;

    MessageType getType() const override { return MessageType::Kick; }
    void serializePayload(NetworkBuffer& buffer) const override;
    bool deserializePayload(NetworkBuffer& buffer) override;
    std::size_t getPayloadSize() const override { return 4 + reason.size(); }
};

} // namespace sims3000

#endif // SIMS3000_NET_SERVERMESSAGES_H
