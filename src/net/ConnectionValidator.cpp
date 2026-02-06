/**
 * @file ConnectionValidator.cpp
 * @brief Implementation of message validation and error handling.
 */

#include "sims3000/net/ConnectionValidator.h"
#include <SDL3/SDL_log.h>

namespace sims3000 {

// =============================================================================
// Validation Result Names
// =============================================================================

const char* getValidationResultName(ValidationResult result) {
    switch (result) {
        case ValidationResult::Valid:               return "Valid";
        case ValidationResult::EmptyData:           return "EmptyData";
        case ValidationResult::MessageTooLarge:     return "MessageTooLarge";
        case ValidationResult::InvalidEnvelope:     return "InvalidEnvelope";
        case ValidationResult::IncompatibleVersion: return "IncompatibleVersion";
        case ValidationResult::UnknownMessageType:  return "UnknownMessageType";
        case ValidationResult::PayloadTooLarge:     return "PayloadTooLarge";
        case ValidationResult::DeserializationFailed: return "DeserializationFailed";
        case ValidationResult::InvalidPlayerId:     return "InvalidPlayerId";
        case ValidationResult::BufferOverflow:      return "BufferOverflow";
        case ValidationResult::SecurityViolation:   return "SecurityViolation";
        default:                                    return "Unknown";
    }
}

// =============================================================================
// ConnectionValidator Implementation
// =============================================================================

bool ConnectionValidator::validateRawMessage(const std::vector<std::uint8_t>& data,
                                              const ValidationContext& ctx,
                                              ValidationOutput& output) {
    m_stats.totalValidated++;
    output.result = ValidationResult::Valid;
    output.errorMessage.clear();

    // Check for empty data
    if (data.empty()) {
        output.result = ValidationResult::EmptyData;
        output.errorMessage = "Empty message data";
        logValidationFailure(output.result, ctx, output.errorMessage);
        updateStats(output.result);
        return false;
    }

    // Check maximum message size BEFORE any parsing
    if (data.size() > MAX_MESSAGE_SIZE) {
        output.result = ValidationResult::MessageTooLarge;
        output.errorMessage = "Message size " + std::to_string(data.size()) +
                              " exceeds maximum " + std::to_string(MAX_MESSAGE_SIZE);
        logValidationFailure(output.result, ctx, output.errorMessage);
        updateStats(output.result);
        return false;
    }

    // Check minimum size for envelope header
    if (data.size() < MESSAGE_HEADER_SIZE) {
        output.result = ValidationResult::InvalidEnvelope;
        output.errorMessage = "Message too short for envelope header (size=" +
                              std::to_string(data.size()) + ", need=" +
                              std::to_string(MESSAGE_HEADER_SIZE) + ")";
        logValidationFailure(output.result, ctx, output.errorMessage);
        updateStats(output.result);
        return false;
    }

    // Parse envelope header
    NetworkBuffer buffer(data.data(), data.size());
    try {
        output.header = NetworkMessage::parseEnvelope(buffer);
    } catch (const BufferOverflowError& e) {
        output.result = ValidationResult::BufferOverflow;
        output.errorMessage = std::string("Buffer overflow parsing envelope: ") + e.what();
        logValidationFailure(output.result, ctx, output.errorMessage);
        updateStats(output.result);
        return false;
    } catch (const std::exception& e) {
        output.result = ValidationResult::InvalidEnvelope;
        output.errorMessage = std::string("Exception parsing envelope: ") + e.what();
        logValidationFailure(output.result, ctx, output.errorMessage);
        updateStats(output.result);
        return false;
    }

    // Check protocol version compatibility
    if (!output.header.isVersionCompatible()) {
        output.result = ValidationResult::IncompatibleVersion;
        output.errorMessage = "Protocol version " + std::to_string(output.header.protocolVersion) +
                              " not compatible (min=" + std::to_string(MIN_PROTOCOL_VERSION) +
                              ", max=" + std::to_string(PROTOCOL_VERSION) + ")";
        logValidationFailure(output.result, ctx, output.errorMessage);
        updateStats(output.result);
        return false;
    }

    // Check for Invalid message type
    if (output.header.type == MessageType::Invalid) {
        output.result = ValidationResult::UnknownMessageType;
        output.errorMessage = "Invalid message type (0)";
        logValidationFailure(output.result, ctx, output.errorMessage);
        updateStats(output.result);
        return false;
    }

    // Check payload size is reasonable
    if (output.header.payloadLength > MAX_PAYLOAD_SIZE) {
        output.result = ValidationResult::PayloadTooLarge;
        output.errorMessage = "Payload size " + std::to_string(output.header.payloadLength) +
                              " exceeds maximum " + std::to_string(MAX_PAYLOAD_SIZE);
        logValidationFailure(output.result, ctx, output.errorMessage);
        updateStats(output.result);
        return false;
    }

    // Check that declared payload size matches actual remaining data
    std::size_t remainingBytes = data.size() - MESSAGE_HEADER_SIZE;
    if (output.header.payloadLength > remainingBytes) {
        output.result = ValidationResult::PayloadTooLarge;
        output.errorMessage = "Declared payload size " + std::to_string(output.header.payloadLength) +
                              " exceeds actual remaining data " + std::to_string(remainingBytes);
        logValidationFailure(output.result, ctx, output.errorMessage);
        updateStats(output.result);
        return false;
    }

    // Check if message type is registered (known)
    if (!MessageFactory::isRegistered(output.header.type)) {
        output.result = ValidationResult::UnknownMessageType;
        output.errorMessage = "Unknown message type " +
                              std::to_string(static_cast<std::uint16_t>(output.header.type));
        logValidationFailure(output.result, ctx, output.errorMessage);
        updateStats(output.result);
        // Don't crash - just log and drop
        return false;
    }

    // Validation passed
    m_stats.validMessages++;
    return true;
}

bool ConnectionValidator::validatePlayerId(PlayerID messagePlayerId,
                                            const ValidationContext& ctx,
                                            ValidationOutput& output) {
    output.result = ValidationResult::Valid;
    output.errorMessage.clear();

    // PlayerID 0 is invalid (GameMaster/no player)
    if (messagePlayerId == 0) {
        output.result = ValidationResult::InvalidPlayerId;
        output.errorMessage = "Message contains invalid PlayerID 0";
        logValidationFailure(output.result, ctx, output.errorMessage);
        updateStats(output.result);
        return false;
    }

    // If we have an expected PlayerID, verify it matches
    if (ctx.expectedPlayerId != 0 && messagePlayerId != ctx.expectedPlayerId) {
        output.result = ValidationResult::SecurityViolation;
        output.errorMessage = "PlayerID mismatch: message has " +
                              std::to_string(messagePlayerId) +
                              " but connection is for player " +
                              std::to_string(ctx.expectedPlayerId);

        // This is a security warning - log it prominently
        if (m_securityLoggingEnabled) {
            SDL_Log("SECURITY WARNING: Peer %u sent message with wrong PlayerID "
                    "(claimed %u, should be %u)",
                    ctx.peer, messagePlayerId, ctx.expectedPlayerId);
        }

        updateStats(output.result);
        return false;
    }

    return true;
}

bool ConnectionValidator::safeDeserializePayload(NetworkBuffer& buffer,
                                                  NetworkMessage& message,
                                                  const ValidationContext& ctx,
                                                  ValidationOutput& output) {
    output.result = ValidationResult::Valid;
    output.errorMessage.clear();

    try {
        if (!message.deserializePayload(buffer)) {
            output.result = ValidationResult::DeserializationFailed;
            output.errorMessage = "Message deserializePayload() returned false";
            logValidationFailure(output.result, ctx, output.errorMessage);
            updateStats(output.result);
            return false;
        }
    } catch (const BufferOverflowError& e) {
        output.result = ValidationResult::BufferOverflow;
        output.errorMessage = std::string("Buffer overflow during deserialization: ") + e.what();
        logValidationFailure(output.result, ctx, output.errorMessage);
        updateStats(output.result);
        return false;
    } catch (const std::exception& e) {
        output.result = ValidationResult::DeserializationFailed;
        output.errorMessage = std::string("Exception during deserialization: ") + e.what();
        logValidationFailure(output.result, ctx, output.errorMessage);
        updateStats(output.result);
        return false;
    }

    return true;
}

void ConnectionValidator::resetStats() {
    m_stats = ValidationStats{};
}

void ConnectionValidator::logValidationFailure(ValidationResult result,
                                                const ValidationContext& ctx,
                                                const std::string& details) {
    m_stats.droppedMessages++;

    // Determine log level based on severity
    bool isSecurityIssue = (result == ValidationResult::SecurityViolation ||
                            result == ValidationResult::InvalidPlayerId);

    if (isSecurityIssue && m_securityLoggingEnabled) {
        SDL_Log("SECURITY: Message validation failed for peer %u: %s - %s",
                ctx.peer, getValidationResultName(result), details.c_str());
    } else {
        SDL_Log("NetworkValidator: Dropped message from peer %u: %s - %s",
                ctx.peer, getValidationResultName(result), details.c_str());
    }
}

void ConnectionValidator::updateStats(ValidationResult result) {
    switch (result) {
        case ValidationResult::EmptyData:
            m_stats.emptyDataCount++;
            break;
        case ValidationResult::MessageTooLarge:
            m_stats.tooLargeCount++;
            break;
        case ValidationResult::InvalidEnvelope:
            m_stats.invalidEnvelopeCount++;
            break;
        case ValidationResult::IncompatibleVersion:
            m_stats.versionMismatchCount++;
            break;
        case ValidationResult::UnknownMessageType:
            m_stats.unknownTypeCount++;
            break;
        case ValidationResult::PayloadTooLarge:
            m_stats.payloadTooLargeCount++;
            break;
        case ValidationResult::DeserializationFailed:
            m_stats.deserializeFailCount++;
            break;
        case ValidationResult::InvalidPlayerId:
            m_stats.invalidPlayerIdCount++;
            break;
        case ValidationResult::BufferOverflow:
            m_stats.bufferOverflowCount++;
            break;
        case ValidationResult::SecurityViolation:
            m_stats.securityViolationCount++;
            break;
        case ValidationResult::Valid:
            // No stat to update
            break;
    }
}

} // namespace sims3000
