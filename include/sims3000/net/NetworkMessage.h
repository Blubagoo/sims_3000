/**
 * @file NetworkMessage.h
 * @brief Network message protocol framework for client-server communication.
 *
 * Defines the message envelope format, message types, and base class for
 * all network messages. Each message has a header followed by payload:
 *
 * Envelope format:
 *   [1 byte protocol version]
 *   [2 bytes message type]
 *   [2 bytes payload length]
 *   [N bytes payload]
 *
 * Message types are partitioned:
 *   - 0-99: System messages (connection, heartbeat, etc.)
 *   - 100-199: Gameplay messages (input, state updates, etc.)
 *   - 200+: Reserved for future use
 *
 * Usage:
 * @code
 * // Serialize a message
 * NetworkBuffer buffer;
 * myMessage.serializeWithEnvelope(buffer);
 *
 * // Deserialize a message
 * auto result = NetworkMessage::parseEnvelope(buffer);
 * if (result.isValid()) {
 *     auto msg = MessageFactory::create(result.type);
 *     msg->deserializePayload(buffer);
 * }
 * @endcode
 */

#ifndef SIMS3000_NET_NETWORKMESSAGE_H
#define SIMS3000_NET_NETWORKMESSAGE_H

#include "sims3000/net/NetworkBuffer.h"
#include <cstdint>
#include <memory>
#include <functional>
#include <unordered_map>

namespace sims3000 {

/// Current protocol version. Increment when making breaking changes.
constexpr std::uint8_t PROTOCOL_VERSION = 1;

/// Minimum protocol version we accept (for backward compatibility).
constexpr std::uint8_t MIN_PROTOCOL_VERSION = 1;

/// Maximum payload size in bytes (64KB - header size)
constexpr std::uint16_t MAX_PAYLOAD_SIZE = 65000;

/// Message envelope header size in bytes (1 + 2 + 2 = 5)
constexpr std::size_t MESSAGE_HEADER_SIZE = 5;

/**
 * @enum MessageType
 * @brief Network message type identifiers.
 *
 * Message types are partitioned into ranges:
 *   - 0-99: System messages
 *   - 100-199: Gameplay messages
 *   - 200+: Reserved
 */
enum class MessageType : std::uint16_t {
    // =========================================================================
    // Invalid/Unknown (0)
    // =========================================================================
    Invalid = 0,

    // =========================================================================
    // System Messages (1-99)
    // =========================================================================

    /// Client heartbeat to server (keepalive)
    Heartbeat = 1,

    /// Server response to heartbeat (RTT measurement)
    HeartbeatResponse = 2,

    /// Client requesting to join the game
    Join = 3,

    /// Server accepting join request
    JoinAccept = 4,

    /// Server rejecting join request
    JoinReject = 5,

    /// Client requesting to reconnect with session token
    Reconnect = 6,

    /// Client gracefully disconnecting
    Disconnect = 7,

    /// Server kicking a player
    Kick = 8,

    /// Server status information (loading, ready, running)
    ServerStatus = 9,

    /// Player list update (join/leave/status change)
    PlayerList = 10,

    /// Chat message between players
    Chat = 11,

    /// Snapshot transfer start marker
    SnapshotStart = 20,

    /// Snapshot data chunk
    SnapshotChunk = 21,

    /// Snapshot transfer complete marker
    SnapshotEnd = 22,

    // System message range end marker (not a real message type)
    _SystemEnd = 99,

    // =========================================================================
    // Gameplay Messages (100-199)
    // =========================================================================

    /// Player input action (build, zone, demolish, etc.)
    Input = 100,

    /// Server acknowledgment of input
    InputAck = 101,

    /// Delta state update (changed entities/components)
    StateUpdate = 102,

    /// Action rejected by server with reason
    Rejection = 103,

    /// Game event notification (disaster, milestone, etc.)
    Event = 104,

    /// Resource trade offer
    TradeOffer = 110,

    /// Resource trade accept
    TradeAccept = 111,

    /// Resource trade reject
    TradeReject = 112,

    /// Trade completion notification
    TradeComplete = 113,

    // Gameplay message range end marker (not a real message type)
    _GameplayEnd = 199,

    // =========================================================================
    // Reserved (200+)
    // =========================================================================

    // Reserved for future expansion
    _ReservedStart = 200
};

/**
 * @brief Check if a message type is a system message (0-99).
 */
inline bool isSystemMessage(MessageType type) {
    auto val = static_cast<std::uint16_t>(type);
    return val > 0 && val <= static_cast<std::uint16_t>(MessageType::_SystemEnd);
}

/**
 * @brief Check if a message type is a gameplay message (100-199).
 */
inline bool isGameplayMessage(MessageType type) {
    auto val = static_cast<std::uint16_t>(type);
    return val > static_cast<std::uint16_t>(MessageType::_SystemEnd) &&
           val <= static_cast<std::uint16_t>(MessageType::_GameplayEnd);
}

/**
 * @brief Get human-readable name for a message type.
 */
const char* getMessageTypeName(MessageType type);

/**
 * @struct EnvelopeHeader
 * @brief Parsed message envelope header.
 */
struct EnvelopeHeader {
    std::uint8_t protocolVersion = 0;
    MessageType type = MessageType::Invalid;
    std::uint16_t payloadLength = 0;

    /// Check if header is valid (version compatible, type known).
    bool isValid() const {
        return protocolVersion >= MIN_PROTOCOL_VERSION &&
               protocolVersion <= PROTOCOL_VERSION &&
               type != MessageType::Invalid;
    }

    /// Check if protocol version is compatible.
    bool isVersionCompatible() const {
        return protocolVersion >= MIN_PROTOCOL_VERSION &&
               protocolVersion <= PROTOCOL_VERSION;
    }
};

/**
 * @class NetworkMessage
 * @brief Base class for all network messages.
 *
 * Provides envelope serialization and parsing. Derived classes implement
 * serializePayload() and deserializePayload() for their specific data.
 */
class NetworkMessage {
public:
    virtual ~NetworkMessage() = default;

    /**
     * @brief Get the message type identifier.
     */
    virtual MessageType getType() const = 0;

    /**
     * @brief Serialize the message payload (not including envelope header).
     * @param buffer The buffer to write payload data to.
     */
    virtual void serializePayload(NetworkBuffer& buffer) const = 0;

    /**
     * @brief Deserialize the message payload (after header has been parsed).
     * @param buffer The buffer containing payload data.
     * @return true if deserialization succeeded, false if data is malformed.
     */
    virtual bool deserializePayload(NetworkBuffer& buffer) = 0;

    /**
     * @brief Get the expected payload size in bytes.
     *
     * Returns 0 for variable-length messages. Used for pre-allocation.
     */
    virtual std::size_t getPayloadSize() const { return 0; }

    /**
     * @brief Serialize the complete message with envelope header.
     * @param buffer The buffer to write the complete message to.
     */
    void serializeWithEnvelope(NetworkBuffer& buffer) const;

    /**
     * @brief Parse an envelope header from the buffer.
     * @param buffer The buffer to read from (read position will advance).
     * @return The parsed envelope header.
     *
     * If parsing fails (insufficient data), returns header with Invalid type.
     * Check header.isValid() before proceeding with payload.
     */
    static EnvelopeHeader parseEnvelope(NetworkBuffer& buffer);

    /**
     * @brief Skip over payload bytes in the buffer.
     *
     * Used when receiving an unknown message type - skip the payload
     * without crashing so subsequent messages can be processed.
     *
     * @param buffer The buffer to skip in.
     * @param payloadLength Number of bytes to skip.
     * @return true if skip succeeded, false if insufficient bytes.
     */
    static bool skipPayload(NetworkBuffer& buffer, std::uint16_t payloadLength);

    // =========================================================================
    // Sequence number support (optional, for reliable channel ordering)
    // =========================================================================

    /**
     * @brief Get the sequence number (0 = not set).
     */
    std::uint32_t getSequenceNumber() const { return m_sequenceNumber; }

    /**
     * @brief Set the sequence number for ordering.
     */
    void setSequenceNumber(std::uint32_t seq) { m_sequenceNumber = seq; }

protected:
    std::uint32_t m_sequenceNumber = 0;
};

/**
 * @class MessageFactory
 * @brief Factory for creating message objects from type IDs.
 *
 * Derived message classes register themselves with the factory to enable
 * dynamic message creation during deserialization.
 *
 * Usage:
 * @code
 * // Registration (typically in message class cpp file)
 * static bool registered = MessageFactory::registerType<HeartbeatMessage>(MessageType::Heartbeat);
 *
 * // Creation
 * auto msg = MessageFactory::create(MessageType::Heartbeat);
 * if (msg) {
 *     msg->deserializePayload(buffer);
 * }
 * @endcode
 */
class MessageFactory {
public:
    using Creator = std::function<std::unique_ptr<NetworkMessage>()>;

    /**
     * @brief Register a message type with its creator function.
     * @param type The message type identifier.
     * @param creator Function that creates a new instance of the message.
     * @return true (for use in static initialization).
     */
    static bool registerCreator(MessageType type, Creator creator);

    /**
     * @brief Register a message type using default construction.
     * @tparam T The message class type (must derive from NetworkMessage).
     * @param type The message type identifier.
     * @return true (for use in static initialization).
     */
    template<typename T>
    static bool registerType(MessageType type) {
        return registerCreator(type, []() -> std::unique_ptr<NetworkMessage> {
            return std::make_unique<T>();
        });
    }

    /**
     * @brief Create a message instance by type ID.
     * @param type The message type to create.
     * @return Pointer to new message, or nullptr if type is unknown.
     */
    static std::unique_ptr<NetworkMessage> create(MessageType type);

    /**
     * @brief Check if a message type is registered.
     */
    static bool isRegistered(MessageType type);

    /**
     * @brief Get the number of registered message types.
     */
    static std::size_t registeredCount();

private:
    static std::unordered_map<MessageType, Creator>& getRegistry();
};

/**
 * @class SequenceTracker
 * @brief Tracks sequence numbers for message ordering.
 *
 * Provides a monotonically increasing sequence counter for outgoing messages
 * and tracks the last received sequence for ordering on the receiving side.
 */
class SequenceTracker {
public:
    /**
     * @brief Get the next outgoing sequence number.
     *
     * Atomically increments and returns the next sequence number.
     * Sequence numbers start at 1 (0 means "no sequence").
     */
    std::uint32_t nextSequence() { return ++m_nextOutgoing; }

    /**
     * @brief Get the current outgoing sequence (without incrementing).
     */
    std::uint32_t currentSequence() const { return m_nextOutgoing; }

    /**
     * @brief Record a received sequence number.
     * @param seq The received sequence number.
     * @return true if this is the expected next sequence, false if out of order.
     */
    bool recordReceived(std::uint32_t seq);

    /**
     * @brief Get the last received sequence number.
     */
    std::uint32_t lastReceived() const { return m_lastReceived; }

    /**
     * @brief Check if a sequence number is newer than the last received.
     * @param seq The sequence number to check.
     * @return true if seq > lastReceived (handling wraparound).
     */
    bool isNewer(std::uint32_t seq) const;

    /**
     * @brief Reset the tracker to initial state.
     */
    void reset() {
        m_nextOutgoing = 0;
        m_lastReceived = 0;
    }

private:
    std::uint32_t m_nextOutgoing = 0;
    std::uint32_t m_lastReceived = 0;
};

// =============================================================================
// Static assertions for message envelope format
// =============================================================================

static_assert(sizeof(std::uint8_t) == 1, "uint8_t must be 1 byte");
static_assert(sizeof(std::uint16_t) == 2, "uint16_t must be 2 bytes");
static_assert(MESSAGE_HEADER_SIZE == 5, "Message header must be 5 bytes");

} // namespace sims3000

#endif // SIMS3000_NET_NETWORKMESSAGE_H
