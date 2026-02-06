/**
 * @file ConnectionValidator.h
 * @brief Message validation and error handling for network connections.
 *
 * Provides comprehensive validation for incoming network messages:
 * - Malformed message detection
 * - Message size limits
 * - Unknown message type handling
 * - Invalid PlayerID detection
 * - Buffer overflow protection
 *
 * All validation is designed to be resilient - invalid messages are logged
 * and dropped, but the connection survives. The server must never crash
 * from malformed client data.
 *
 * Ownership: NetworkServer owns ConnectionValidator.
 * Thread safety: Not thread-safe. Call from main thread only.
 */

#ifndef SIMS3000_NET_CONNECTIONVALIDATOR_H
#define SIMS3000_NET_CONNECTIONVALIDATOR_H

#include "sims3000/net/NetworkMessage.h"
#include "sims3000/net/NetworkBuffer.h"
#include "sims3000/net/INetworkTransport.h"
#include "sims3000/core/types.h"
#include <cstdint>
#include <string>
#include <functional>

namespace sims3000 {

/**
 * @enum ValidationResult
 * @brief Result codes for message validation.
 */
enum class ValidationResult : std::uint8_t {
    Valid = 0,                  ///< Message is valid
    EmptyData,                  ///< No data received
    MessageTooLarge,            ///< Message exceeds maximum size
    InvalidEnvelope,            ///< Could not parse message envelope
    IncompatibleVersion,        ///< Protocol version mismatch
    UnknownMessageType,         ///< Message type not recognized
    PayloadTooLarge,            ///< Payload exceeds declared size
    DeserializationFailed,      ///< Failed to deserialize payload
    InvalidPlayerId,            ///< PlayerID in message doesn't match connection
    BufferOverflow,             ///< Read exceeded buffer bounds
    SecurityViolation           ///< Security-related validation failure
};

/**
 * @brief Get human-readable name for a validation result.
 */
const char* getValidationResultName(ValidationResult result);

/**
 * @struct ValidationStats
 * @brief Statistics about message validation.
 */
struct ValidationStats {
    std::uint64_t totalValidated = 0;     ///< Total messages validated
    std::uint64_t validMessages = 0;      ///< Messages that passed validation
    std::uint64_t droppedMessages = 0;    ///< Messages that failed validation

    // Breakdown by failure reason
    std::uint64_t emptyDataCount = 0;
    std::uint64_t tooLargeCount = 0;
    std::uint64_t invalidEnvelopeCount = 0;
    std::uint64_t versionMismatchCount = 0;
    std::uint64_t unknownTypeCount = 0;
    std::uint64_t payloadTooLargeCount = 0;
    std::uint64_t deserializeFailCount = 0;
    std::uint64_t invalidPlayerIdCount = 0;
    std::uint64_t bufferOverflowCount = 0;
    std::uint64_t securityViolationCount = 0;
};

/**
 * @struct ValidationContext
 * @brief Context for message validation including connection info.
 */
struct ValidationContext {
    PeerID peer = INVALID_PEER_ID;        ///< Source peer
    PlayerID expectedPlayerId = 0;         ///< Expected PlayerID for this connection (0 = any)
    std::uint64_t currentTimeMs = 0;       ///< Current timestamp for logging
};

/**
 * @struct ValidationOutput
 * @brief Output from message validation.
 */
struct ValidationOutput {
    ValidationResult result = ValidationResult::Valid;
    EnvelopeHeader header;                ///< Parsed header (if valid)
    std::string errorMessage;             ///< Human-readable error for logging
};

/**
 * @class ConnectionValidator
 * @brief Validates incoming network messages for safety and correctness.
 *
 * Example usage:
 * @code
 * ConnectionValidator validator;
 *
 * void handleMessage(PeerID peer, const std::vector<uint8_t>& data) {
 *     ValidationContext ctx;
 *     ctx.peer = peer;
 *     ctx.expectedPlayerId = getPlayerIdForPeer(peer);
 *
 *     ValidationOutput output;
 *     if (!validator.validateRawMessage(data, ctx, output)) {
 *         // Message invalid - already logged, connection survives
 *         return;
 *     }
 *
 *     // Proceed with message processing
 * }
 * @endcode
 */
class ConnectionValidator {
public:
    /// Maximum allowed message size in bytes (header + payload)
    static constexpr std::size_t MAX_MESSAGE_SIZE = 65536;  // 64KB

    /// Maximum allowed payload size
    static constexpr std::size_t MAX_PAYLOAD_SIZE = MAX_MESSAGE_SIZE - MESSAGE_HEADER_SIZE;

    /**
     * @brief Construct a ConnectionValidator.
     */
    ConnectionValidator() = default;

    /**
     * @brief Validate raw message data before deserialization.
     * @param data Raw message bytes.
     * @param ctx Validation context (peer, expected player, etc.)
     * @param output Output containing result and parsed header.
     * @return true if message is valid, false otherwise.
     *
     * This performs:
     * - Size checks (not too large, not empty)
     * - Envelope parsing
     * - Protocol version check
     * - Message type validation
     *
     * If validation fails, logs a warning and returns false.
     * The connection is NOT terminated - it survives.
     */
    bool validateRawMessage(const std::vector<std::uint8_t>& data,
                            const ValidationContext& ctx,
                            ValidationOutput& output);

    /**
     * @brief Validate a deserialized message's PlayerID.
     * @param messagePlayerId PlayerID from the message.
     * @param ctx Validation context with expected PlayerID.
     * @param output Output for error information.
     * @return true if PlayerID is valid, false otherwise.
     *
     * Checks that:
     * - PlayerID is not 0 (invalid)
     * - PlayerID matches the connection's assigned PlayerID (if set)
     */
    bool validatePlayerId(PlayerID messagePlayerId,
                          const ValidationContext& ctx,
                          ValidationOutput& output);

    /**
     * @brief Safely deserialize a message payload with overflow protection.
     * @param buffer Buffer containing payload data.
     * @param message Message object to deserialize into.
     * @param ctx Validation context.
     * @param output Output for error information.
     * @return true if deserialization succeeded, false otherwise.
     *
     * Wraps the deserialization in a try/catch to handle BufferOverflowError.
     */
    bool safeDeserializePayload(NetworkBuffer& buffer,
                                NetworkMessage& message,
                                const ValidationContext& ctx,
                                ValidationOutput& output);

    /**
     * @brief Get validation statistics.
     */
    const ValidationStats& getStats() const { return m_stats; }

    /**
     * @brief Reset validation statistics.
     */
    void resetStats();

    /**
     * @brief Enable or disable security logging.
     * @param enabled Whether to log security-related warnings.
     */
    void setSecurityLoggingEnabled(bool enabled) { m_securityLoggingEnabled = enabled; }

    /**
     * @brief Check if security logging is enabled.
     */
    bool isSecurityLoggingEnabled() const { return m_securityLoggingEnabled; }

private:
    /**
     * @brief Log a validation failure.
     */
    void logValidationFailure(ValidationResult result,
                              const ValidationContext& ctx,
                              const std::string& details);

    /**
     * @brief Update statistics for a validation result.
     */
    void updateStats(ValidationResult result);

    ValidationStats m_stats;
    bool m_securityLoggingEnabled = true;
};

} // namespace sims3000

#endif // SIMS3000_NET_CONNECTIONVALIDATOR_H
