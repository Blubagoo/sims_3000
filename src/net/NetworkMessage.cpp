/**
 * @file NetworkMessage.cpp
 * @brief Implementation of network message protocol framework.
 */

#include "sims3000/net/NetworkMessage.h"
#include "sims3000/core/Logger.h"

namespace sims3000 {

// =============================================================================
// Message Type Names
// =============================================================================

const char* getMessageTypeName(MessageType type) {
    switch (type) {
        case MessageType::Invalid:           return "Invalid";
        case MessageType::Heartbeat:         return "Heartbeat";
        case MessageType::HeartbeatResponse: return "HeartbeatResponse";
        case MessageType::Join:              return "Join";
        case MessageType::JoinAccept:        return "JoinAccept";
        case MessageType::JoinReject:        return "JoinReject";
        case MessageType::Reconnect:         return "Reconnect";
        case MessageType::Disconnect:        return "Disconnect";
        case MessageType::Kick:              return "Kick";
        case MessageType::ServerStatus:      return "ServerStatus";
        case MessageType::PlayerList:        return "PlayerList";
        case MessageType::Chat:              return "Chat";
        case MessageType::SnapshotStart:     return "SnapshotStart";
        case MessageType::SnapshotChunk:     return "SnapshotChunk";
        case MessageType::SnapshotEnd:       return "SnapshotEnd";
        case MessageType::Input:             return "Input";
        case MessageType::InputAck:          return "InputAck";
        case MessageType::StateUpdate:       return "StateUpdate";
        case MessageType::Rejection:         return "Rejection";
        case MessageType::Event:             return "Event";
        case MessageType::TradeOffer:        return "TradeOffer";
        case MessageType::TradeAccept:       return "TradeAccept";
        case MessageType::TradeReject:       return "TradeReject";
        case MessageType::TradeComplete:     return "TradeComplete";
        default:                             return "Unknown";
    }
}

// =============================================================================
// NetworkMessage Implementation
// =============================================================================

void NetworkMessage::serializeWithEnvelope(NetworkBuffer& buffer) const {
    // Create a temporary buffer for the payload
    NetworkBuffer payloadBuffer;
    serializePayload(payloadBuffer);

    // Check payload size
    if (payloadBuffer.size() > MAX_PAYLOAD_SIZE) {
        LOG_ERROR("Message payload too large: %zu bytes (max %u)",
                  payloadBuffer.size(), MAX_PAYLOAD_SIZE);
        return;
    }

    // Write envelope header
    buffer.write_u8(PROTOCOL_VERSION);
    buffer.write_u16(static_cast<std::uint16_t>(getType()));
    buffer.write_u16(static_cast<std::uint16_t>(payloadBuffer.size()));

    // Write payload
    buffer.write_bytes(payloadBuffer.data(), payloadBuffer.size());
}

EnvelopeHeader NetworkMessage::parseEnvelope(NetworkBuffer& buffer) {
    EnvelopeHeader header;

    // Check if we have enough data for the header
    if (buffer.remaining() < MESSAGE_HEADER_SIZE) {
        LOG_WARN("Insufficient data for message header: %zu bytes (need %zu)",
                 buffer.remaining(), MESSAGE_HEADER_SIZE);
        return header; // Returns Invalid type
    }

    // Parse header fields
    header.protocolVersion = buffer.read_u8();
    header.type = static_cast<MessageType>(buffer.read_u16());
    header.payloadLength = buffer.read_u16();

    // Validate protocol version
    if (!header.isVersionCompatible()) {
        LOG_WARN("Incompatible protocol version: %u (expected %u-%u)",
                 header.protocolVersion, MIN_PROTOCOL_VERSION, PROTOCOL_VERSION);
        header.type = MessageType::Invalid;
        return header;
    }

    // Validate payload length against remaining data
    if (buffer.remaining() < header.payloadLength) {
        LOG_WARN("Truncated message payload: need %u bytes, have %zu",
                 header.payloadLength, buffer.remaining());
        header.type = MessageType::Invalid;
        return header;
    }

    return header;
}

bool NetworkMessage::skipPayload(NetworkBuffer& buffer, std::uint16_t payloadLength) {
    if (buffer.remaining() < payloadLength) {
        LOG_WARN("Cannot skip %u bytes, only %zu remaining",
                 payloadLength, buffer.remaining());
        return false;
    }

    // Skip by reading into a discard buffer
    std::vector<std::uint8_t> discard(payloadLength);
    try {
        buffer.read_bytes(discard.data(), payloadLength);
        return true;
    } catch (const BufferOverflowError& e) {
        LOG_ERROR("Buffer overflow while skipping payload: %s", e.what());
        return false;
    }
}

// =============================================================================
// MessageFactory Implementation
// =============================================================================

std::unordered_map<MessageType, MessageFactory::Creator>& MessageFactory::getRegistry() {
    static std::unordered_map<MessageType, Creator> registry;
    return registry;
}

bool MessageFactory::registerCreator(MessageType type, Creator creator) {
    auto& registry = getRegistry();

    if (registry.find(type) != registry.end()) {
        LOG_WARN("Message type %u (%s) already registered, overwriting",
                 static_cast<std::uint16_t>(type), getMessageTypeName(type));
    }

    registry[type] = std::move(creator);
    return true;
}

std::unique_ptr<NetworkMessage> MessageFactory::create(MessageType type) {
    auto& registry = getRegistry();
    auto it = registry.find(type);

    if (it == registry.end()) {
        LOG_WARN("Unknown message type %u (%s) - not registered in factory",
                 static_cast<std::uint16_t>(type), getMessageTypeName(type));
        return nullptr;
    }

    return it->second();
}

bool MessageFactory::isRegistered(MessageType type) {
    auto& registry = getRegistry();
    return registry.find(type) != registry.end();
}

std::size_t MessageFactory::registeredCount() {
    return getRegistry().size();
}

// =============================================================================
// SequenceTracker Implementation
// =============================================================================

bool SequenceTracker::recordReceived(std::uint32_t seq) {
    // Handle sequence 0 as "no sequence"
    if (seq == 0) {
        return true;
    }

    // Check if this is the expected next sequence
    bool inOrder = (m_lastReceived == 0) || (seq == m_lastReceived + 1);

    // Update if newer (handles wraparound)
    if (isNewer(seq)) {
        m_lastReceived = seq;
    }

    return inOrder;
}

bool SequenceTracker::isNewer(std::uint32_t seq) const {
    // Handle wraparound: if diff > 2^31, assume wraparound
    // e.g., lastReceived=0xFFFFFFFE, seq=1 -> diff=3 < 2^31, so seq is newer
    // e.g., lastReceived=3, seq=0xFFFFFFFE -> diff=0xFFFFFFFB > 2^31, not newer
    if (m_lastReceived == 0) {
        return seq != 0;
    }

    std::uint32_t diff = seq - m_lastReceived;
    return diff != 0 && diff < 0x80000000;
}

} // namespace sims3000
