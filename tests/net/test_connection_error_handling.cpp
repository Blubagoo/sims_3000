/**
 * @file test_connection_error_handling.cpp
 * @brief Unit tests for connection error handling (Ticket 1-018).
 *
 * Tests cover:
 * - Malformed message handling
 * - Message size limits
 * - Unknown message type handling
 * - Rate limiting with token bucket algorithm
 * - Egregious abuse detection
 * - Invalid PlayerID validation
 * - Buffer overflow protection
 */

#include "sims3000/net/RateLimiter.h"
#include "sims3000/net/ConnectionValidator.h"
#include "sims3000/net/NetworkMessage.h"
#include "sims3000/net/NetworkBuffer.h"
#include "sims3000/net/ClientMessages.h"
#include "sims3000/net/InputMessage.h"
#include <cassert>
#include <cstdio>
#include <cmath>
#include <vector>
#include <chrono>
#include <thread>

using namespace sims3000;

// ============================================================================
// Test helpers
// ============================================================================

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            printf("  FAIL: %s\n", msg); \
            return false; \
        } \
    } while(0)

#define TEST_ASSERT_EQ(a, b, msg) \
    do { \
        if ((a) != (b)) { \
            printf("  FAIL: %s (expected %d, got %d)\n", msg, (int)(b), (int)(a)); \
            return false; \
        } \
    } while(0)

// ============================================================================
// RateLimiter Tests
// ============================================================================

bool test_rate_limiter_allows_normal_actions() {
    printf("  test_rate_limiter_allows_normal_actions...\n");

    RateLimiter limiter;
    std::uint64_t timeMs = 1000000;

    limiter.registerPlayer(1, timeMs);

    // Normal rate should be allowed (10/sec for building)
    for (int i = 0; i < 10; ++i) {
        auto result = limiter.checkAction(1, InputType::PlaceBuilding, timeMs);
        TEST_ASSERT(result.allowed, "Action should be allowed");
        TEST_ASSERT(!result.isAbuse, "Should not be abuse");
    }

    printf("  PASS\n");
    return true;
}

bool test_rate_limiter_blocks_excess_actions() {
    printf("  test_rate_limiter_blocks_excess_actions...\n");

    RateLimiter limiter;
    std::uint64_t timeMs = 1000000;

    limiter.registerPlayer(1, timeMs);

    // Exhaust tokens (burst size is 15 for building)
    for (int i = 0; i < 15; ++i) {
        limiter.checkAction(1, InputType::PlaceBuilding, timeMs);
    }

    // Next action should be rate limited
    auto result = limiter.checkAction(1, InputType::PlaceBuilding, timeMs);
    TEST_ASSERT(!result.allowed, "Action should be blocked after exhausting tokens");
    TEST_ASSERT(result.totalDropped == 1, "Should have 1 dropped action");

    printf("  PASS\n");
    return true;
}

bool test_rate_limiter_refills_tokens() {
    printf("  test_rate_limiter_refills_tokens...\n");

    RateLimiter limiter;
    std::uint64_t timeMs = 1000000;

    limiter.registerPlayer(1, timeMs);

    // Exhaust all tokens
    for (int i = 0; i < 20; ++i) {
        limiter.checkAction(1, InputType::PlaceBuilding, timeMs);
    }

    // Should be blocked
    auto result1 = limiter.checkAction(1, InputType::PlaceBuilding, timeMs);
    TEST_ASSERT(!result1.allowed, "Should be blocked after exhausting tokens");

    // Wait 1 second - should refill 10 tokens
    timeMs += 1000;

    // Should now be allowed
    auto result2 = limiter.checkAction(1, InputType::PlaceBuilding, timeMs);
    TEST_ASSERT(result2.allowed, "Should be allowed after token refill");

    printf("  PASS\n");
    return true;
}

bool test_rate_limiter_different_categories() {
    printf("  test_rate_limiter_different_categories...\n");

    RateLimiter limiter;
    std::uint64_t timeMs = 1000000;

    limiter.registerPlayer(1, timeMs);

    // Exhaust building tokens
    for (int i = 0; i < 20; ++i) {
        limiter.checkAction(1, InputType::PlaceBuilding, timeMs);
    }

    // Building should be blocked
    auto resultBuilding = limiter.checkAction(1, InputType::PlaceBuilding, timeMs);
    TEST_ASSERT(!resultBuilding.allowed, "Building should be blocked");

    // But zoning should still work (separate bucket)
    auto resultZoning = limiter.checkAction(1, InputType::SetZone, timeMs);
    TEST_ASSERT(resultZoning.allowed, "Zoning should still be allowed");

    printf("  PASS\n");
    return true;
}

bool test_rate_limiter_abuse_detection() {
    printf("  test_rate_limiter_abuse_detection...\n");

    RateLimiter limiter;
    std::uint64_t timeMs = 1000000;

    limiter.registerPlayer(1, timeMs);

    // Send 100 actions in the same second (abuse threshold)
    bool foundAbuse = false;
    for (int i = 0; i < 105; ++i) {
        auto result = limiter.checkAction(1, InputType::SetZone, timeMs);  // Zoning has higher limit
        if (result.isAbuse) {
            foundAbuse = true;
        }
    }

    TEST_ASSERT(foundAbuse, "Should detect abuse at 100+ actions/sec");
    TEST_ASSERT(limiter.getTotalAbuseEvents() >= 1, "Should have recorded abuse event");

    printf("  PASS\n");
    return true;
}

bool test_rate_limiter_player_registration() {
    printf("  test_rate_limiter_player_registration...\n");

    RateLimiter limiter;
    std::uint64_t timeMs = 1000000;

    // Player not registered - should auto-register
    auto result1 = limiter.checkAction(1, InputType::PlaceBuilding, timeMs);
    TEST_ASSERT(result1.allowed, "Should auto-register and allow");

    const PlayerRateState* state = limiter.getPlayerState(1);
    TEST_ASSERT(state != nullptr, "Player state should exist after action");

    // Unregister player
    limiter.unregisterPlayer(1);
    state = limiter.getPlayerState(1);
    TEST_ASSERT(state == nullptr, "Player state should be null after unregister");

    printf("  PASS\n");
    return true;
}

bool test_rate_limiter_reset_player() {
    printf("  test_rate_limiter_reset_player...\n");

    RateLimiter limiter;
    std::uint64_t timeMs = 1000000;

    limiter.registerPlayer(1, timeMs);

    // Exhaust tokens and accumulate stats
    for (int i = 0; i < 25; ++i) {
        limiter.checkAction(1, InputType::PlaceBuilding, timeMs);
    }

    const PlayerRateState* state = limiter.getPlayerState(1);
    TEST_ASSERT(state->totalDropped > 0, "Should have dropped actions");

    // Reset player
    limiter.resetPlayer(1, timeMs + 1000);

    state = limiter.getPlayerState(1);
    TEST_ASSERT(state->totalDropped == 0, "Dropped count should be reset");
    TEST_ASSERT(state->abuseCount == 0, "Abuse count should be reset");

    printf("  PASS\n");
    return true;
}

bool test_rate_limiter_skips_camera_inputs() {
    printf("  test_rate_limiter_skips_camera_inputs...\n");

    RateLimiter limiter;
    std::uint64_t timeMs = 1000000;

    // Camera inputs should always be allowed (client-only)
    for (int i = 0; i < 200; ++i) {
        auto result = limiter.checkAction(1, InputType::CameraMove, timeMs);
        TEST_ASSERT(result.allowed, "Camera input should always be allowed");
    }

    TEST_ASSERT(limiter.getTotalDropped() == 0, "No actions should be dropped");

    printf("  PASS\n");
    return true;
}

// ============================================================================
// ConnectionValidator Tests
// ============================================================================

bool test_validator_rejects_empty_data() {
    printf("  test_validator_rejects_empty_data...\n");

    ConnectionValidator validator;
    ValidationContext ctx;
    ctx.peer = 1;
    ValidationOutput output;

    std::vector<std::uint8_t> emptyData;
    bool valid = validator.validateRawMessage(emptyData, ctx, output);

    TEST_ASSERT(!valid, "Should reject empty data");
    TEST_ASSERT(output.result == ValidationResult::EmptyData, "Should be EmptyData result");
    TEST_ASSERT(validator.getStats().emptyDataCount == 1, "Should count empty data");

    printf("  PASS\n");
    return true;
}

bool test_validator_rejects_too_large_message() {
    printf("  test_validator_rejects_too_large_message...\n");

    ConnectionValidator validator;
    ValidationContext ctx;
    ctx.peer = 1;
    ValidationOutput output;

    // Create a message larger than MAX_MESSAGE_SIZE
    std::vector<std::uint8_t> largeData(70000, 0xFF);
    bool valid = validator.validateRawMessage(largeData, ctx, output);

    TEST_ASSERT(!valid, "Should reject too-large message");
    TEST_ASSERT(output.result == ValidationResult::MessageTooLarge, "Should be MessageTooLarge result");

    printf("  PASS\n");
    return true;
}

bool test_validator_rejects_invalid_envelope() {
    printf("  test_validator_rejects_invalid_envelope...\n");

    ConnectionValidator validator;
    ValidationContext ctx;
    ctx.peer = 1;
    ValidationOutput output;

    // Only 3 bytes - not enough for header (needs 5)
    std::vector<std::uint8_t> shortData = {0x01, 0x02, 0x03};
    bool valid = validator.validateRawMessage(shortData, ctx, output);

    TEST_ASSERT(!valid, "Should reject short data");
    TEST_ASSERT(output.result == ValidationResult::InvalidEnvelope, "Should be InvalidEnvelope result");

    printf("  PASS\n");
    return true;
}

bool test_validator_rejects_incompatible_version() {
    printf("  test_validator_rejects_incompatible_version...\n");

    ConnectionValidator validator;
    ValidationContext ctx;
    ctx.peer = 1;
    ValidationOutput output;

    // Create a message with incompatible protocol version
    NetworkBuffer buffer;
    buffer.write_u8(99);  // Bad protocol version
    buffer.write_u16(static_cast<std::uint16_t>(MessageType::Heartbeat));
    buffer.write_u16(0);  // Payload length 0

    std::vector<std::uint8_t> data = buffer.raw();
    bool valid = validator.validateRawMessage(data, ctx, output);

    TEST_ASSERT(!valid, "Should reject incompatible version");
    TEST_ASSERT(output.result == ValidationResult::IncompatibleVersion, "Should be IncompatibleVersion");

    printf("  PASS\n");
    return true;
}

bool test_validator_rejects_invalid_message_type() {
    printf("  test_validator_rejects_invalid_message_type...\n");

    ConnectionValidator validator;
    ValidationContext ctx;
    ctx.peer = 1;
    ValidationOutput output;

    // Create a message with type 0 (Invalid)
    NetworkBuffer buffer;
    buffer.write_u8(PROTOCOL_VERSION);
    buffer.write_u16(0);  // MessageType::Invalid
    buffer.write_u16(0);  // Payload length 0

    std::vector<std::uint8_t> data = buffer.raw();
    bool valid = validator.validateRawMessage(data, ctx, output);

    TEST_ASSERT(!valid, "Should reject invalid message type");
    TEST_ASSERT(output.result == ValidationResult::UnknownMessageType, "Should be UnknownMessageType");

    printf("  PASS\n");
    return true;
}

bool test_validator_rejects_unknown_message_type() {
    printf("  test_validator_rejects_unknown_message_type...\n");

    ConnectionValidator validator;
    ValidationContext ctx;
    ctx.peer = 1;
    ValidationOutput output;

    // Create a message with an unregistered type
    NetworkBuffer buffer;
    buffer.write_u8(PROTOCOL_VERSION);
    buffer.write_u16(9999);  // Unknown type
    buffer.write_u16(0);  // Payload length 0

    std::vector<std::uint8_t> data = buffer.raw();
    bool valid = validator.validateRawMessage(data, ctx, output);

    TEST_ASSERT(!valid, "Should reject unknown message type");
    TEST_ASSERT(output.result == ValidationResult::UnknownMessageType, "Should be UnknownMessageType");

    printf("  PASS\n");
    return true;
}

bool test_validator_rejects_payload_size_mismatch() {
    printf("  test_validator_rejects_payload_size_mismatch...\n");

    ConnectionValidator validator;
    ValidationContext ctx;
    ctx.peer = 1;
    ValidationOutput output;

    // Create a message claiming more payload than present
    NetworkBuffer buffer;
    buffer.write_u8(PROTOCOL_VERSION);
    buffer.write_u16(static_cast<std::uint16_t>(MessageType::Heartbeat));
    buffer.write_u16(100);  // Claims 100 bytes payload but only header present

    std::vector<std::uint8_t> data = buffer.raw();
    bool valid = validator.validateRawMessage(data, ctx, output);

    TEST_ASSERT(!valid, "Should reject payload size mismatch");
    // Note: parseEnvelope detects truncated payload and sets type to Invalid,
    // which triggers UnknownMessageType rather than PayloadTooLarge
    TEST_ASSERT(output.result == ValidationResult::UnknownMessageType, "Should be UnknownMessageType (truncated payload signals via Invalid type)");

    printf("  PASS\n");
    return true;
}

bool test_validator_accepts_valid_heartbeat() {
    printf("  test_validator_accepts_valid_heartbeat...\n");

    ConnectionValidator validator;
    ValidationContext ctx;
    ctx.peer = 1;
    ValidationOutput output;

    // Create a valid heartbeat message
    HeartbeatMessage heartbeat;
    heartbeat.clientTimestamp = 12345;
    heartbeat.clientSequence = 1;

    NetworkBuffer buffer;
    heartbeat.serializeWithEnvelope(buffer);

    std::vector<std::uint8_t> data = buffer.raw();
    bool valid = validator.validateRawMessage(data, ctx, output);

    TEST_ASSERT(valid, "Should accept valid heartbeat");
    TEST_ASSERT(output.result == ValidationResult::Valid, "Should be Valid");
    TEST_ASSERT(output.header.type == MessageType::Heartbeat, "Should parse type correctly");

    printf("  PASS\n");
    return true;
}

bool test_validator_rejects_invalid_player_id_zero() {
    printf("  test_validator_rejects_invalid_player_id_zero...\n");

    ConnectionValidator validator;
    ValidationContext ctx;
    ctx.peer = 1;
    ctx.expectedPlayerId = 0;  // No expected (accept any)
    ValidationOutput output;

    // PlayerID 0 is always invalid
    bool valid = validator.validatePlayerId(0, ctx, output);

    TEST_ASSERT(!valid, "Should reject PlayerID 0");
    TEST_ASSERT(output.result == ValidationResult::InvalidPlayerId, "Should be InvalidPlayerId");

    printf("  PASS\n");
    return true;
}

bool test_validator_rejects_player_id_mismatch() {
    printf("  test_validator_rejects_player_id_mismatch...\n");

    ConnectionValidator validator;
    ValidationContext ctx;
    ctx.peer = 1;
    ctx.expectedPlayerId = 2;  // Expect player 2
    ValidationOutput output;

    // Message claims to be from player 3
    bool valid = validator.validatePlayerId(3, ctx, output);

    TEST_ASSERT(!valid, "Should reject mismatched PlayerID");
    TEST_ASSERT(output.result == ValidationResult::SecurityViolation, "Should be SecurityViolation");
    TEST_ASSERT(validator.getStats().securityViolationCount == 1, "Should count security violation");

    printf("  PASS\n");
    return true;
}

bool test_validator_accepts_matching_player_id() {
    printf("  test_validator_accepts_matching_player_id...\n");

    ConnectionValidator validator;
    ValidationContext ctx;
    ctx.peer = 1;
    ctx.expectedPlayerId = 2;
    ValidationOutput output;

    bool valid = validator.validatePlayerId(2, ctx, output);

    TEST_ASSERT(valid, "Should accept matching PlayerID");
    TEST_ASSERT(output.result == ValidationResult::Valid, "Should be Valid");

    printf("  PASS\n");
    return true;
}

bool test_validator_safe_deserialize_catches_overflow() {
    printf("  test_validator_safe_deserialize_catches_overflow...\n");

    ConnectionValidator validator;
    ValidationContext ctx;
    ctx.peer = 1;
    ValidationOutput output;

    // Create a buffer with not enough data for a JoinMessage
    NetworkBuffer buffer;
    buffer.write_u32(100);  // Claims string length of 100
    buffer.write_u8('x');   // But only 1 byte

    JoinMessage msg;
    bool valid = validator.safeDeserializePayload(buffer, msg, ctx, output);

    TEST_ASSERT(!valid, "Should catch buffer overflow");
    // Note: JoinMessage::deserializePayload catches BufferOverflowError internally
    // and returns false, so validator sees DeserializationFailed not BufferOverflow
    TEST_ASSERT(output.result == ValidationResult::DeserializationFailed, "Should be DeserializationFailed (message class handles exception internally)");

    printf("  PASS\n");
    return true;
}

bool test_validator_stats_tracking() {
    printf("  test_validator_stats_tracking...\n");

    ConnectionValidator validator;
    ValidationContext ctx;
    ctx.peer = 1;
    ValidationOutput output;

    // Generate various validation failures
    std::vector<std::uint8_t> emptyData;
    validator.validateRawMessage(emptyData, ctx, output);

    std::vector<std::uint8_t> shortData = {0x01, 0x02};
    validator.validateRawMessage(shortData, ctx, output);

    // Create a valid message
    HeartbeatMessage heartbeat;
    NetworkBuffer buffer;
    heartbeat.serializeWithEnvelope(buffer);
    validator.validateRawMessage(buffer.raw(), ctx, output);

    const ValidationStats& stats = validator.getStats();
    TEST_ASSERT(stats.totalValidated == 3, "Should have validated 3 messages");
    TEST_ASSERT(stats.validMessages == 1, "Should have 1 valid message");
    TEST_ASSERT(stats.droppedMessages == 2, "Should have 2 dropped messages");

    // Reset stats
    validator.resetStats();
    TEST_ASSERT(validator.getStats().totalValidated == 0, "Should reset stats");

    printf("  PASS\n");
    return true;
}

// ============================================================================
// Token Bucket Algorithm Tests
// ============================================================================

bool test_token_bucket_refill() {
    printf("  test_token_bucket_refill...\n");

    TokenBucket bucket;
    bucket.maxTokens = 10.0f;
    bucket.refillRate = 5.0f;  // 5 per second
    bucket.tokens = 0.0f;  // Start empty

    std::uint64_t timeMs = 1000000;
    bucket.lastRefillMs = timeMs;

    // After 1 second, should have 5 tokens
    timeMs += 1000;
    bucket.refill(timeMs);
    TEST_ASSERT(std::fabs(bucket.tokens - 5.0f) < 0.01f, "Should have 5 tokens after 1 sec");

    // After another second, should have 10 (capped at max)
    timeMs += 1000;
    bucket.refill(timeMs);
    TEST_ASSERT(std::fabs(bucket.tokens - 10.0f) < 0.01f, "Should cap at max tokens");

    printf("  PASS\n");
    return true;
}

bool test_token_bucket_consume() {
    printf("  test_token_bucket_consume...\n");

    TokenBucket bucket;
    bucket.maxTokens = 10.0f;
    bucket.refillRate = 10.0f;
    bucket.reset(1000000);

    std::uint64_t timeMs = 1000000;

    // Should be able to consume tokens
    TEST_ASSERT(bucket.tryConsume(timeMs), "Should consume first token");
    TEST_ASSERT(std::fabs(bucket.tokens - 9.0f) < 0.01f, "Should have 9 tokens left");

    // Consume all tokens
    for (int i = 0; i < 9; ++i) {
        bucket.tryConsume(timeMs);
    }

    // Should now fail
    TEST_ASSERT(!bucket.tryConsume(timeMs), "Should fail when empty");

    printf("  PASS\n");
    return true;
}

// ============================================================================
// Action Category Mapping Tests
// ============================================================================

bool test_action_category_mapping() {
    printf("  test_action_category_mapping...\n");

    TEST_ASSERT(getActionCategory(InputType::PlaceBuilding) == ActionCategory::Building,
                "PlaceBuilding -> Building");
    TEST_ASSERT(getActionCategory(InputType::DemolishBuilding) == ActionCategory::Building,
                "DemolishBuilding -> Building");
    TEST_ASSERT(getActionCategory(InputType::SetZone) == ActionCategory::Zoning,
                "SetZone -> Zoning");
    TEST_ASSERT(getActionCategory(InputType::ClearZone) == ActionCategory::Zoning,
                "ClearZone -> Zoning");
    TEST_ASSERT(getActionCategory(InputType::PlaceRoad) == ActionCategory::Infrastructure,
                "PlaceRoad -> Infrastructure");
    TEST_ASSERT(getActionCategory(InputType::SetTaxRate) == ActionCategory::Economy,
                "SetTaxRate -> Economy");
    TEST_ASSERT(getActionCategory(InputType::PauseGame) == ActionCategory::GameControl,
                "PauseGame -> GameControl");

    printf("  PASS\n");
    return true;
}

// ============================================================================
// Main
// ============================================================================

int main() {
    printf("=== Connection Error Handling Tests (Ticket 1-018) ===\n\n");

    int passed = 0;
    int failed = 0;

    #define RUN_TEST(test) \
        do { \
            printf("Running %s\n", #test); \
            if (test()) { passed++; } else { failed++; } \
        } while(0)

    printf("--- Rate Limiter Tests ---\n");
    RUN_TEST(test_rate_limiter_allows_normal_actions);
    RUN_TEST(test_rate_limiter_blocks_excess_actions);
    RUN_TEST(test_rate_limiter_refills_tokens);
    RUN_TEST(test_rate_limiter_different_categories);
    RUN_TEST(test_rate_limiter_abuse_detection);
    RUN_TEST(test_rate_limiter_player_registration);
    RUN_TEST(test_rate_limiter_reset_player);
    RUN_TEST(test_rate_limiter_skips_camera_inputs);

    printf("\n--- Connection Validator Tests ---\n");
    RUN_TEST(test_validator_rejects_empty_data);
    RUN_TEST(test_validator_rejects_too_large_message);
    RUN_TEST(test_validator_rejects_invalid_envelope);
    RUN_TEST(test_validator_rejects_incompatible_version);
    RUN_TEST(test_validator_rejects_invalid_message_type);
    RUN_TEST(test_validator_rejects_unknown_message_type);
    RUN_TEST(test_validator_rejects_payload_size_mismatch);
    RUN_TEST(test_validator_accepts_valid_heartbeat);
    RUN_TEST(test_validator_rejects_invalid_player_id_zero);
    RUN_TEST(test_validator_rejects_player_id_mismatch);
    RUN_TEST(test_validator_accepts_matching_player_id);
    RUN_TEST(test_validator_safe_deserialize_catches_overflow);
    RUN_TEST(test_validator_stats_tracking);

    printf("\n--- Token Bucket Tests ---\n");
    RUN_TEST(test_token_bucket_refill);
    RUN_TEST(test_token_bucket_consume);

    printf("\n--- Action Category Tests ---\n");
    RUN_TEST(test_action_category_mapping);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", passed);
    printf("Failed: %d\n", failed);

    if (failed > 0) {
        printf("\n=== TESTS FAILED ===\n");
        return 1;
    }

    printf("\n=== All tests passed! ===\n");
    return 0;
}
