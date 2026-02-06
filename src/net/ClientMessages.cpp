/**
 * @file ClientMessages.cpp
 * @brief Implementation of client-to-server network messages.
 */

#include "sims3000/net/ClientMessages.h"
#include "sims3000/core/Logger.h"
#include <algorithm>
#include <cstring>

namespace sims3000 {

// =============================================================================
// Message Factory Registration
// =============================================================================

// Register all client message types with the factory for deserialization.
// These static booleans trigger registration at program startup.

static bool joinRegistered = MessageFactory::registerType<JoinMessage>(MessageType::Join);
static bool inputRegistered = MessageFactory::registerType<NetInputMessage>(MessageType::Input);
static bool chatRegistered = MessageFactory::registerType<ChatMessage>(MessageType::Chat);
static bool heartbeatRegistered = MessageFactory::registerType<HeartbeatMessage>(MessageType::Heartbeat);
static bool reconnectRegistered = MessageFactory::registerType<ReconnectMessage>(MessageType::Reconnect);

// Suppress unused variable warnings
namespace {
    [[maybe_unused]] auto _force_registration = joinRegistered && inputRegistered &&
                                                 chatRegistered && heartbeatRegistered &&
                                                 reconnectRegistered;
}

// =============================================================================
// JoinMessage Implementation
// =============================================================================

void JoinMessage::serializePayload(NetworkBuffer& buffer) const {
    // Truncate player name if too long
    std::string safeName = playerName;
    if (safeName.size() > MAX_PLAYER_NAME_LENGTH) {
        safeName.resize(MAX_PLAYER_NAME_LENGTH);
        LOG_WARN("JoinMessage: player name truncated to %zu bytes", MAX_PLAYER_NAME_LENGTH);
    }

    buffer.write_string(safeName);
    buffer.write_u8(hasSessionToken ? 1 : 0);

    if (hasSessionToken) {
        buffer.write_bytes(sessionToken.data(), SESSION_TOKEN_SIZE);
    }
}

bool JoinMessage::deserializePayload(NetworkBuffer& buffer) {
    try {
        playerName = buffer.read_string();

        // Validate name length
        if (playerName.size() > MAX_PLAYER_NAME_LENGTH) {
            LOG_WARN("JoinMessage: received oversized player name (%zu bytes), truncating",
                     playerName.size());
            playerName.resize(MAX_PLAYER_NAME_LENGTH);
        }

        hasSessionToken = buffer.read_u8() != 0;

        if (hasSessionToken) {
            buffer.read_bytes(sessionToken.data(), SESSION_TOKEN_SIZE);
        } else {
            std::fill(sessionToken.begin(), sessionToken.end(), 0);
        }

        return true;
    } catch (const BufferOverflowError& e) {
        LOG_ERROR("JoinMessage deserialization failed: %s", e.what());
        return false;
    }
}

std::size_t JoinMessage::getPayloadSize() const {
    std::size_t nameLen = std::min(playerName.size(), MAX_PLAYER_NAME_LENGTH);
    std::size_t size = 4 + nameLen + 1; // string length prefix + name + hasToken flag
    if (hasSessionToken) {
        size += SESSION_TOKEN_SIZE;
    }
    return size;
}

bool JoinMessage::isValid() const {
    // Name must not be empty
    if (playerName.empty()) {
        return false;
    }

    // Name must be within length limit
    if (playerName.size() > MAX_PLAYER_NAME_LENGTH) {
        return false;
    }

    return true;
}

// =============================================================================
// NetInputMessage Implementation
// =============================================================================

void NetInputMessage::serializePayload(NetworkBuffer& buffer) const {
    // Serialize using NetworkBuffer methods (matching InputMessage::serialize pattern)
    buffer.write_u32(static_cast<std::uint32_t>(input.tick & 0xFFFFFFFF));
    buffer.write_u32(static_cast<std::uint32_t>(input.tick >> 32));
    buffer.write_u8(input.playerId);
    buffer.write_u8(static_cast<std::uint8_t>(input.type));
    buffer.write_u32(input.sequenceNum);
    buffer.write_u16(static_cast<std::uint16_t>(input.targetPos.x));
    buffer.write_u16(static_cast<std::uint16_t>(input.targetPos.y));
    buffer.write_u32(input.param1);
    buffer.write_u32(input.param2);
    buffer.write_u32(static_cast<std::uint32_t>(input.value));
}

bool NetInputMessage::deserializePayload(NetworkBuffer& buffer) {
    try {
        // Read tick as two u32s (little-endian)
        std::uint32_t tickLow = buffer.read_u32();
        std::uint32_t tickHigh = buffer.read_u32();
        input.tick = static_cast<SimulationTick>(tickLow) |
                     (static_cast<SimulationTick>(tickHigh) << 32);

        input.playerId = buffer.read_u8();
        input.type = static_cast<InputType>(buffer.read_u8());
        input.sequenceNum = buffer.read_u32();
        input.targetPos.x = static_cast<std::int16_t>(buffer.read_u16());
        input.targetPos.y = static_cast<std::int16_t>(buffer.read_u16());
        input.param1 = buffer.read_u32();
        input.param2 = buffer.read_u32();
        input.value = static_cast<std::int32_t>(buffer.read_u32());

        return true;
    } catch (const BufferOverflowError& e) {
        LOG_ERROR("NetInputMessage deserialization failed: %s", e.what());
        return false;
    }
}

bool NetInputMessage::isValid() const {
    // Player ID must be assigned (1-4 for normal players, 0 is reserved)
    if (input.playerId == 0) {
        return false;
    }

    // Input type must be valid (not None)
    if (input.type == InputType::None) {
        return false;
    }

    // Input type must be within defined range
    if (static_cast<std::uint8_t>(input.type) >= static_cast<std::uint8_t>(InputType::COUNT)) {
        return false;
    }

    return true;
}

// =============================================================================
// ChatMessage Implementation
// =============================================================================

void ChatMessage::serializePayload(NetworkBuffer& buffer) const {
    buffer.write_u8(senderId);

    // Truncate text if too long
    std::string safeText = text;
    if (safeText.size() > MAX_CHAT_MESSAGE_LENGTH) {
        safeText.resize(MAX_CHAT_MESSAGE_LENGTH);
        LOG_WARN("ChatMessage: text truncated to %zu bytes", MAX_CHAT_MESSAGE_LENGTH);
    }

    buffer.write_string(safeText);
    buffer.write_u32(static_cast<std::uint32_t>(timestamp & 0xFFFFFFFF));
    buffer.write_u32(static_cast<std::uint32_t>(timestamp >> 32));
}

bool ChatMessage::deserializePayload(NetworkBuffer& buffer) {
    try {
        senderId = buffer.read_u8();
        text = buffer.read_string();

        // Validate text length
        if (text.size() > MAX_CHAT_MESSAGE_LENGTH) {
            LOG_WARN("ChatMessage: received oversized text (%zu bytes), truncating", text.size());
            text.resize(MAX_CHAT_MESSAGE_LENGTH);
        }

        std::uint32_t timestampLow = buffer.read_u32();
        std::uint32_t timestampHigh = buffer.read_u32();
        timestamp = static_cast<std::uint64_t>(timestampLow) |
                    (static_cast<std::uint64_t>(timestampHigh) << 32);

        return true;
    } catch (const BufferOverflowError& e) {
        LOG_ERROR("ChatMessage deserialization failed: %s", e.what());
        return false;
    }
}

std::size_t ChatMessage::getPayloadSize() const {
    std::size_t textLen = std::min(text.size(), MAX_CHAT_MESSAGE_LENGTH);
    return 1 + 4 + textLen + 8; // senderId + string prefix + text + timestamp
}

bool ChatMessage::isValid() const {
    // Text must not be empty
    if (text.empty()) {
        return false;
    }

    // Text must be within length limit
    if (text.size() > MAX_CHAT_MESSAGE_LENGTH) {
        return false;
    }

    return true;
}

// =============================================================================
// HeartbeatMessage Implementation
// =============================================================================

void HeartbeatMessage::serializePayload(NetworkBuffer& buffer) const {
    buffer.write_u32(static_cast<std::uint32_t>(clientTimestamp & 0xFFFFFFFF));
    buffer.write_u32(static_cast<std::uint32_t>(clientTimestamp >> 32));
    buffer.write_u32(clientSequence);
}

bool HeartbeatMessage::deserializePayload(NetworkBuffer& buffer) {
    try {
        std::uint32_t timestampLow = buffer.read_u32();
        std::uint32_t timestampHigh = buffer.read_u32();
        clientTimestamp = static_cast<std::uint64_t>(timestampLow) |
                          (static_cast<std::uint64_t>(timestampHigh) << 32);
        clientSequence = buffer.read_u32();
        return true;
    } catch (const BufferOverflowError& e) {
        LOG_ERROR("HeartbeatMessage deserialization failed: %s", e.what());
        return false;
    }
}

// =============================================================================
// ReconnectMessage Implementation
// =============================================================================

void ReconnectMessage::serializePayload(NetworkBuffer& buffer) const {
    buffer.write_bytes(sessionToken.data(), SESSION_TOKEN_SIZE);

    // Truncate player name if too long
    std::string safeName = playerName;
    if (safeName.size() > MAX_PLAYER_NAME_LENGTH) {
        safeName.resize(MAX_PLAYER_NAME_LENGTH);
        LOG_WARN("ReconnectMessage: player name truncated to %zu bytes", MAX_PLAYER_NAME_LENGTH);
    }

    buffer.write_string(safeName);
}

bool ReconnectMessage::deserializePayload(NetworkBuffer& buffer) {
    try {
        buffer.read_bytes(sessionToken.data(), SESSION_TOKEN_SIZE);
        playerName = buffer.read_string();

        // Validate name length
        if (playerName.size() > MAX_PLAYER_NAME_LENGTH) {
            LOG_WARN("ReconnectMessage: received oversized player name (%zu bytes), truncating",
                     playerName.size());
            playerName.resize(MAX_PLAYER_NAME_LENGTH);
        }

        return true;
    } catch (const BufferOverflowError& e) {
        LOG_ERROR("ReconnectMessage deserialization failed: %s", e.what());
        return false;
    }
}

std::size_t ReconnectMessage::getPayloadSize() const {
    std::size_t nameLen = std::min(playerName.size(), MAX_PLAYER_NAME_LENGTH);
    return SESSION_TOKEN_SIZE + 4 + nameLen; // token + string prefix + name
}

bool ReconnectMessage::isValid() const {
    // Token must not be all zeros (indicating no token)
    bool hasToken = false;
    for (std::size_t i = 0; i < SESSION_TOKEN_SIZE; ++i) {
        if (sessionToken[i] != 0) {
            hasToken = true;
            break;
        }
    }

    if (!hasToken) {
        return false;
    }

    // Name must be within length limit (can be empty for reconnect)
    if (playerName.size() > MAX_PLAYER_NAME_LENGTH) {
        return false;
    }

    return true;
}

} // namespace sims3000
