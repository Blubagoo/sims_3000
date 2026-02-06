/**
 * @file ClientMessages.h
 * @brief Client-to-server network message types.
 *
 * This file defines all messages sent from clients to the server:
 * - JoinMessage: Request to join the game world
 * - NetInputMessage: Player action wrapper for network transmission
 * - ChatMessage: Text chat between players
 * - HeartbeatMessage: Client keepalive with RTT measurement
 * - ReconnectMessage: Session recovery after disconnect
 *
 * All messages implement serialize/deserialize using NetworkBuffer and
 * register with MessageFactory for dynamic creation during deserialization.
 */

#ifndef SIMS3000_NET_CLIENTMESSAGES_H
#define SIMS3000_NET_CLIENTMESSAGES_H

#include "sims3000/net/NetworkMessage.h"
#include "sims3000/net/NetworkBuffer.h"
#include "sims3000/net/InputMessage.h"
#include "sims3000/core/types.h"
#include <string>
#include <array>
#include <cstdint>

namespace sims3000 {

// =============================================================================
// Size Limits
// =============================================================================

/// Maximum player name length in bytes.
constexpr std::size_t MAX_PLAYER_NAME_LENGTH = 64;

/// Maximum chat message length in bytes.
constexpr std::size_t MAX_CHAT_MESSAGE_LENGTH = 500;

/// Session token size in bytes (128-bit = 16 bytes).
constexpr std::size_t SESSION_TOKEN_SIZE = 16;

// =============================================================================
// JoinMessage (MessageType::Join)
// =============================================================================

/**
 * @class JoinMessage
 * @brief Client request to join the game world.
 *
 * Sent when a client first connects. Contains player name and optional
 * session token for reconnection. Server responds with JoinAccept or JoinReject.
 *
 * Wire format (little-endian):
 *   [4 bytes] playerName length (u32)
 *   [N bytes] playerName (UTF-8)
 *   [1 byte]  hasSessionToken (0 or 1)
 *   [16 bytes] sessionToken (only if hasSessionToken == 1)
 *
 * Payload size: 5 + playerName.length() [+ 16 if token present]
 * Maximum size: 5 + 64 + 16 = 85 bytes
 */
class JoinMessage : public NetworkMessage {
public:
    /// Player's display name (max 64 bytes UTF-8).
    std::string playerName;

    /// Optional session token for reconnection (empty if new connection).
    std::array<std::uint8_t, SESSION_TOKEN_SIZE> sessionToken{};

    /// Whether sessionToken contains a valid reconnection token.
    bool hasSessionToken = false;

    MessageType getType() const override { return MessageType::Join; }

    void serializePayload(NetworkBuffer& buffer) const override;
    bool deserializePayload(NetworkBuffer& buffer) override;
    std::size_t getPayloadSize() const override;

    /// Validate message contents (name not empty, name within length limit).
    bool isValid() const;
};

// =============================================================================
// NetInputMessage (MessageType::Input)
// =============================================================================

/**
 * @class NetInputMessage
 * @brief Network wrapper for player input actions.
 *
 * Wraps the Epic 0 InputMessage struct for network transmission. Contains
 * all input data plus network-specific metadata for ordering and acknowledgment.
 *
 * Wire format (little-endian):
 *   [8 bytes] tick (u64) - Client tick when input was generated
 *   [1 byte]  playerId (u8)
 *   [1 byte]  inputType (u8)
 *   [4 bytes] sequenceNum (u32) - For acknowledgment/replay
 *   [2 bytes] targetPos.x (i16)
 *   [2 bytes] targetPos.y (i16)
 *   [4 bytes] param1 (u32)
 *   [4 bytes] param2 (u32)
 *   [4 bytes] value (i32)
 *
 * Payload size: 30 bytes (fixed)
 */
class NetInputMessage : public NetworkMessage {
public:
    /// The input data being transmitted.
    InputMessage input;

    MessageType getType() const override { return MessageType::Input; }

    void serializePayload(NetworkBuffer& buffer) const override;
    bool deserializePayload(NetworkBuffer& buffer) override;
    std::size_t getPayloadSize() const override { return InputMessage::SERIALIZED_SIZE; }

    /// Validate message contents (valid input type, playerId > 0).
    bool isValid() const;
};

// =============================================================================
// ChatMessage (MessageType::Chat)
// =============================================================================

/**
 * @class ChatMessage
 * @brief Text chat message between players.
 *
 * Sent from client to server, then broadcast to all connected clients.
 * Server attaches sender ID before broadcasting (client senderId is ignored).
 *
 * Wire format (little-endian):
 *   [1 byte]  senderId (u8) - Filled by server on receipt
 *   [4 bytes] text length (u32)
 *   [N bytes] text (UTF-8, max 500 bytes)
 *   [8 bytes] timestamp (u64) - Client-side timestamp for display ordering
 *
 * Payload size: 13 + text.length()
 * Maximum size: 13 + 500 = 513 bytes
 */
class ChatMessage : public NetworkMessage {
public:
    /// Sender's player ID (set by server, ignored from client).
    PlayerID senderId = 0;

    /// Chat text content (max 500 bytes UTF-8).
    std::string text;

    /// Client timestamp for message ordering (milliseconds since epoch or game start).
    std::uint64_t timestamp = 0;

    MessageType getType() const override { return MessageType::Chat; }

    void serializePayload(NetworkBuffer& buffer) const override;
    bool deserializePayload(NetworkBuffer& buffer) override;
    std::size_t getPayloadSize() const override;

    /// Validate message contents (text not empty, within length limit).
    bool isValid() const;
};

// =============================================================================
// HeartbeatMessage (MessageType::Heartbeat)
// =============================================================================

/**
 * @class HeartbeatMessage
 * @brief Client keepalive for connection monitoring and RTT measurement.
 *
 * Sent periodically (every 1 second) to indicate client is still active.
 * Server responds with HeartbeatResponse containing the same timestamp
 * for round-trip time calculation.
 *
 * Wire format (little-endian):
 *   [8 bytes] clientTimestamp (u64) - Client's timestamp when heartbeat was sent
 *   [4 bytes] clientSequence (u32) - Heartbeat sequence number for tracking
 *
 * Payload size: 12 bytes (fixed)
 */
class HeartbeatMessage : public NetworkMessage {
public:
    /// Client's timestamp when heartbeat was sent (high-resolution clock).
    std::uint64_t clientTimestamp = 0;

    /// Monotonically increasing heartbeat sequence for loss detection.
    std::uint32_t clientSequence = 0;

    MessageType getType() const override { return MessageType::Heartbeat; }

    void serializePayload(NetworkBuffer& buffer) const override;
    bool deserializePayload(NetworkBuffer& buffer) override;
    std::size_t getPayloadSize() const override { return 12; }
};

// =============================================================================
// ReconnectMessage (MessageType::Reconnect)
// =============================================================================

/**
 * @class ReconnectMessage
 * @brief Request to recover an existing session after disconnect.
 *
 * Sent when a client reconnects using a previously-issued session token.
 * If the token is valid and within the grace period (30 seconds), the
 * server restores the player's session with their original PlayerID.
 *
 * Wire format (little-endian):
 *   [16 bytes] sessionToken - 128-bit session identifier
 *   [4 bytes]  playerName length (u32)
 *   [N bytes]  playerName (UTF-8) - For verification/display
 *
 * Payload size: 20 + playerName.length()
 * Maximum size: 20 + 64 = 84 bytes
 */
class ReconnectMessage : public NetworkMessage {
public:
    /// Session token received from previous JoinAccept.
    std::array<std::uint8_t, SESSION_TOKEN_SIZE> sessionToken{};

    /// Player name for verification (must match original).
    std::string playerName;

    MessageType getType() const override { return MessageType::Reconnect; }

    void serializePayload(NetworkBuffer& buffer) const override;
    bool deserializePayload(NetworkBuffer& buffer) override;
    std::size_t getPayloadSize() const override;

    /// Validate message contents (token not all zeros, name within limit).
    bool isValid() const;
};

// =============================================================================
// Message Size Validation
// =============================================================================

/**
 * @brief Check if a message payload exceeds the maximum allowed size.
 * @param size The payload size in bytes.
 * @return true if size is within limits, false if oversized.
 */
inline bool isPayloadSizeValid(std::size_t size) {
    return size <= MAX_PAYLOAD_SIZE;
}

/**
 * @brief Get the maximum possible payload size for each message type.
 */
constexpr std::size_t getMaxPayloadSize(MessageType type) {
    switch (type) {
        case MessageType::Join:
            return 5 + MAX_PLAYER_NAME_LENGTH + SESSION_TOKEN_SIZE; // 85
        case MessageType::Input:
            return InputMessage::SERIALIZED_SIZE; // 30
        case MessageType::Chat:
            return 13 + MAX_CHAT_MESSAGE_LENGTH; // 513
        case MessageType::Heartbeat:
            return 12;
        case MessageType::Reconnect:
            return 20 + MAX_PLAYER_NAME_LENGTH; // 84
        default:
            return MAX_PAYLOAD_SIZE;
    }
}

// =============================================================================
// Static Size Assertions
// =============================================================================

static_assert(SESSION_TOKEN_SIZE == 16, "Session token must be 16 bytes (128-bit)");
static_assert(MAX_PLAYER_NAME_LENGTH <= 256, "Player name must fit in u8 length prefix");
static_assert(MAX_CHAT_MESSAGE_LENGTH <= 65535, "Chat message must fit in u16 length");
static_assert(getMaxPayloadSize(MessageType::Join) <= MAX_PAYLOAD_SIZE, "JoinMessage within payload limit");
static_assert(getMaxPayloadSize(MessageType::Input) <= MAX_PAYLOAD_SIZE, "NetInputMessage within payload limit");
static_assert(getMaxPayloadSize(MessageType::Chat) <= MAX_PAYLOAD_SIZE, "ChatMessage within payload limit");
static_assert(getMaxPayloadSize(MessageType::Heartbeat) <= MAX_PAYLOAD_SIZE, "HeartbeatMessage within payload limit");
static_assert(getMaxPayloadSize(MessageType::Reconnect) <= MAX_PAYLOAD_SIZE, "ReconnectMessage within payload limit");

} // namespace sims3000

#endif // SIMS3000_NET_CLIENTMESSAGES_H
