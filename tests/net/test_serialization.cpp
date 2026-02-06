/**
 * @file test_serialization.cpp
 * @brief Comprehensive unit tests for serialization utilities and network state machines.
 *        Ticket 1-020 - Unit Tests for Serialization and State Machine
 *
 * Tests cover:
 * - NetworkBuffer: all primitive types, overflow handling, string encoding
 * - NetworkMessage: envelope parsing, serialization roundtrip
 * - Client messages: all client-to-server message types
 * - Server messages: all server-to-client message types
 * - NetworkClient state machine: all state transitions
 * - NetworkServer state machine: client connection lifecycle
 * - Edge cases: empty payloads, maximum sizes, corrupted data
 */

#include "sims3000/net/NetworkBuffer.h"
#include "sims3000/net/NetworkMessage.h"
#include "sims3000/net/ClientMessages.h"
#include "sims3000/net/ServerMessages.h"
#include "sims3000/net/NetworkClient.h"
#include "sims3000/net/NetworkServer.h"
#include "sims3000/net/MockTransport.h"
#include <cassert>
#include <iostream>
#include <cstring>
#include <cmath>
#include <limits>
#include <random>

using namespace sims3000;

// =============================================================================
// Test Utilities
// =============================================================================

static int g_testsPassed = 0;
static int g_testsFailed = 0;

#define TEST_ASSERT(expr, msg) \
    do { \
        if (!(expr)) { \
            std::cerr << "  FAIL: " << msg << " (" << #expr << ")" << std::endl; \
            g_testsFailed++; \
            return; \
        } \
    } while(0)

#define TEST_PASS(name) \
    do { \
        std::cout << "  PASS: " << name << std::endl; \
        g_testsPassed++; \
    } while(0)

#define TEST_ASSERT_EQ(a, b, msg) \
    do { \
        if ((a) != (b)) { \
            std::cerr << "  FAIL: " << msg << " (expected " << (b) << ", got " << (a) << ")" << std::endl; \
            g_testsFailed++; \
            return; \
        } \
    } while(0)

#define TEST_ASSERT_FLOAT_EQ(a, b, msg) \
    do { \
        float diff = std::fabs((a) - (b)); \
        if (diff > 0.0001f) { \
            std::cerr << "  FAIL: " << msg << " (expected " << (b) << ", got " << (a) << ")" << std::endl; \
            g_testsFailed++; \
            return; \
        } \
    } while(0)

#define TEST_ASSERT_THROWS(expr, exception_type, msg) \
    do { \
        bool caught = false; \
        try { expr; } catch (const exception_type&) { caught = true; } \
        if (!caught) { \
            std::cerr << "  FAIL: " << msg << " (expected exception)" << std::endl; \
            g_testsFailed++; \
            return; \
        } \
    } while(0)

// Generate a random session token for testing
std::array<std::uint8_t, SESSION_TOKEN_SIZE> generateTestToken() {
    std::array<std::uint8_t, SESSION_TOKEN_SIZE> token;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    for (auto& byte : token) {
        byte = static_cast<std::uint8_t>(dis(gen));
    }
    return token;
}

// =============================================================================
// Section 1: NetworkBuffer Tests - All Primitive Types
// =============================================================================

void test_NetworkBuffer_u8_write_read() {
    NetworkBuffer buf;
    buf.write_u8(0);
    buf.write_u8(127);
    buf.write_u8(255);

    TEST_ASSERT_EQ(buf.size(), 3, "buffer size after 3 u8 writes");

    buf.reset_read();
    TEST_ASSERT_EQ(buf.read_u8(), 0, "read u8 value 0");
    TEST_ASSERT_EQ(buf.read_u8(), 127, "read u8 value 127");
    TEST_ASSERT_EQ(buf.read_u8(), 255, "read u8 value 255");
    TEST_PASS("NetworkBuffer_u8_write_read");
}

void test_NetworkBuffer_u16_write_read() {
    NetworkBuffer buf;
    buf.write_u16(0);
    buf.write_u16(32767);
    buf.write_u16(65535);

    TEST_ASSERT_EQ(buf.size(), 6, "buffer size after 3 u16 writes");

    buf.reset_read();
    TEST_ASSERT_EQ(buf.read_u16(), 0, "read u16 value 0");
    TEST_ASSERT_EQ(buf.read_u16(), 32767, "read u16 value 32767");
    TEST_ASSERT_EQ(buf.read_u16(), 65535, "read u16 value 65535");
    TEST_PASS("NetworkBuffer_u16_write_read");
}

void test_NetworkBuffer_u16_little_endian() {
    NetworkBuffer buf;
    // 0x1234 should be stored as [0x34, 0x12] in little-endian
    buf.write_u16(0x1234);
    TEST_ASSERT_EQ(buf.size(), 2, "u16 uses 2 bytes");
    TEST_ASSERT_EQ(buf.data()[0], 0x34, "u16 low byte");
    TEST_ASSERT_EQ(buf.data()[1], 0x12, "u16 high byte");
    TEST_PASS("NetworkBuffer_u16_little_endian");
}

void test_NetworkBuffer_u32_write_read() {
    NetworkBuffer buf;
    buf.write_u32(0);
    buf.write_u32(2147483647);  // INT32_MAX
    buf.write_u32(4294967295u); // UINT32_MAX

    TEST_ASSERT_EQ(buf.size(), 12, "buffer size after 3 u32 writes");

    buf.reset_read();
    TEST_ASSERT_EQ(buf.read_u32(), 0u, "read u32 value 0");
    TEST_ASSERT_EQ(buf.read_u32(), 2147483647u, "read u32 value INT32_MAX");
    TEST_ASSERT_EQ(buf.read_u32(), 4294967295u, "read u32 value UINT32_MAX");
    TEST_PASS("NetworkBuffer_u32_write_read");
}

void test_NetworkBuffer_u32_little_endian() {
    NetworkBuffer buf;
    // 0x12345678 should be stored as [0x78, 0x56, 0x34, 0x12] in little-endian
    buf.write_u32(0x12345678);
    TEST_ASSERT_EQ(buf.size(), 4, "u32 uses 4 bytes");
    TEST_ASSERT_EQ(buf.data()[0], 0x78, "u32 byte 0");
    TEST_ASSERT_EQ(buf.data()[1], 0x56, "u32 byte 1");
    TEST_ASSERT_EQ(buf.data()[2], 0x34, "u32 byte 2");
    TEST_ASSERT_EQ(buf.data()[3], 0x12, "u32 byte 3");
    TEST_PASS("NetworkBuffer_u32_little_endian");
}

void test_NetworkBuffer_i32_positive() {
    NetworkBuffer buf;
    buf.write_i32(0);
    buf.write_i32(100);
    buf.write_i32(std::numeric_limits<std::int32_t>::max());

    buf.reset_read();
    TEST_ASSERT_EQ(buf.read_i32(), 0, "read i32 value 0");
    TEST_ASSERT_EQ(buf.read_i32(), 100, "read i32 value 100");
    TEST_ASSERT_EQ(buf.read_i32(), std::numeric_limits<std::int32_t>::max(), "read i32 INT32_MAX");
    TEST_PASS("NetworkBuffer_i32_positive");
}

void test_NetworkBuffer_i32_negative() {
    NetworkBuffer buf;
    buf.write_i32(-1);
    buf.write_i32(-100);
    buf.write_i32(std::numeric_limits<std::int32_t>::min());

    buf.reset_read();
    TEST_ASSERT_EQ(buf.read_i32(), -1, "read i32 value -1");
    TEST_ASSERT_EQ(buf.read_i32(), -100, "read i32 value -100");
    TEST_ASSERT_EQ(buf.read_i32(), std::numeric_limits<std::int32_t>::min(), "read i32 INT32_MIN");
    TEST_PASS("NetworkBuffer_i32_negative");
}

void test_NetworkBuffer_i32_byte_layout() {
    NetworkBuffer buf;
    // -1 in two's complement is 0xFFFFFFFF
    buf.write_i32(-1);
    TEST_ASSERT_EQ(buf.data()[0], 0xFF, "i32 -1 byte 0");
    TEST_ASSERT_EQ(buf.data()[1], 0xFF, "i32 -1 byte 1");
    TEST_ASSERT_EQ(buf.data()[2], 0xFF, "i32 -1 byte 2");
    TEST_ASSERT_EQ(buf.data()[3], 0xFF, "i32 -1 byte 3");
    TEST_PASS("NetworkBuffer_i32_byte_layout");
}

void test_NetworkBuffer_f32_basic() {
    NetworkBuffer buf;
    buf.write_f32(0.0f);
    buf.write_f32(1.0f);
    buf.write_f32(-1.0f);
    buf.write_f32(3.14159f);

    TEST_ASSERT_EQ(buf.size(), 16, "buffer size after 4 f32 writes");

    buf.reset_read();
    TEST_ASSERT_FLOAT_EQ(buf.read_f32(), 0.0f, "read f32 value 0.0");
    TEST_ASSERT_FLOAT_EQ(buf.read_f32(), 1.0f, "read f32 value 1.0");
    TEST_ASSERT_FLOAT_EQ(buf.read_f32(), -1.0f, "read f32 value -1.0");
    TEST_ASSERT_FLOAT_EQ(buf.read_f32(), 3.14159f, "read f32 value pi");
    TEST_PASS("NetworkBuffer_f32_basic");
}

void test_NetworkBuffer_f32_edge_cases() {
    NetworkBuffer buf;
    buf.write_f32(std::numeric_limits<float>::max());
    buf.write_f32(std::numeric_limits<float>::min());
    buf.write_f32(std::numeric_limits<float>::lowest());
    buf.write_f32(std::numeric_limits<float>::epsilon());

    buf.reset_read();
    TEST_ASSERT_FLOAT_EQ(buf.read_f32(), std::numeric_limits<float>::max(), "read f32 FLT_MAX");
    TEST_ASSERT_FLOAT_EQ(buf.read_f32(), std::numeric_limits<float>::min(), "read f32 FLT_MIN");
    TEST_ASSERT_FLOAT_EQ(buf.read_f32(), std::numeric_limits<float>::lowest(), "read f32 FLT_LOWEST");
    TEST_ASSERT_FLOAT_EQ(buf.read_f32(), std::numeric_limits<float>::epsilon(), "read f32 FLT_EPSILON");
    TEST_PASS("NetworkBuffer_f32_edge_cases");
}

// =============================================================================
// Section 2: NetworkBuffer - String Encoding
// =============================================================================

void test_NetworkBuffer_string_basic() {
    NetworkBuffer buf;
    buf.write_string("hello");
    // 4 bytes length prefix + 5 bytes content
    TEST_ASSERT_EQ(buf.size(), 9, "string 'hello' uses 9 bytes");

    buf.reset_read();
    std::string result = buf.read_string();
    TEST_ASSERT(result == "hello", "read string matches");
    TEST_PASS("NetworkBuffer_string_basic");
}

void test_NetworkBuffer_string_empty() {
    NetworkBuffer buf;
    buf.write_string("");
    // 4 bytes length prefix + 0 bytes content
    TEST_ASSERT_EQ(buf.size(), 4, "empty string uses 4 bytes");

    buf.reset_read();
    std::string result = buf.read_string();
    TEST_ASSERT(result.empty(), "empty string reads as empty");
    TEST_PASS("NetworkBuffer_string_empty");
}

void test_NetworkBuffer_string_long() {
    NetworkBuffer buf;
    // Create a string longer than 256 bytes to test u32 length
    std::string long_str(1000, 'x');
    buf.write_string(long_str);

    TEST_ASSERT_EQ(buf.size(), 1004, "long string uses 1004 bytes");

    buf.reset_read();
    std::string result = buf.read_string();
    TEST_ASSERT_EQ(result.size(), 1000, "long string length preserved");
    TEST_ASSERT(result == long_str, "long string content matches");
    TEST_PASS("NetworkBuffer_string_long");
}

void test_NetworkBuffer_string_with_null_byte() {
    NetworkBuffer buf;
    std::string test_str("hello\0world", 11);
    buf.write_string(test_str);

    buf.reset_read();
    std::string result = buf.read_string();
    TEST_ASSERT_EQ(result.size(), 11, "string with null preserves length");
    TEST_ASSERT(result == test_str, "string with null byte preserved");
    TEST_PASS("NetworkBuffer_string_with_null_byte");
}

void test_NetworkBuffer_string_byte_layout() {
    NetworkBuffer buf;
    buf.write_string("AB");
    // Length = 2 stored as little-endian u32: [0x02, 0x00, 0x00, 0x00]
    // Content: ['A', 'B']
    TEST_ASSERT_EQ(buf.size(), 6, "string 'AB' uses 6 bytes");
    TEST_ASSERT_EQ(buf.data()[0], 0x02, "length byte 0");
    TEST_ASSERT_EQ(buf.data()[1], 0x00, "length byte 1");
    TEST_ASSERT_EQ(buf.data()[2], 0x00, "length byte 2");
    TEST_ASSERT_EQ(buf.data()[3], 0x00, "length byte 3");
    TEST_ASSERT_EQ(buf.data()[4], 'A', "content byte 0");
    TEST_ASSERT_EQ(buf.data()[5], 'B', "content byte 1");
    TEST_PASS("NetworkBuffer_string_byte_layout");
}

// =============================================================================
// Section 3: NetworkBuffer - Overflow Handling
// =============================================================================

void test_NetworkBuffer_overflow_u8() {
    NetworkBuffer buf;
    TEST_ASSERT_THROWS(buf.read_u8(), BufferOverflowError, "read_u8 on empty buffer");
    TEST_PASS("NetworkBuffer_overflow_u8");
}

void test_NetworkBuffer_overflow_u16() {
    NetworkBuffer buf;
    buf.write_u8(0xFF);  // Only 1 byte
    buf.reset_read();
    TEST_ASSERT_THROWS(buf.read_u16(), BufferOverflowError, "read_u16 with insufficient data");
    TEST_PASS("NetworkBuffer_overflow_u16");
}

void test_NetworkBuffer_overflow_u32() {
    NetworkBuffer buf;
    buf.write_u16(0xFFFF);  // Only 2 bytes
    buf.reset_read();
    TEST_ASSERT_THROWS(buf.read_u32(), BufferOverflowError, "read_u32 with insufficient data");
    TEST_PASS("NetworkBuffer_overflow_u32");
}

void test_NetworkBuffer_overflow_i32() {
    NetworkBuffer buf;
    buf.write_u16(0xFFFF);  // Only 2 bytes
    buf.reset_read();
    TEST_ASSERT_THROWS(buf.read_i32(), BufferOverflowError, "read_i32 with insufficient data");
    TEST_PASS("NetworkBuffer_overflow_i32");
}

void test_NetworkBuffer_overflow_f32() {
    NetworkBuffer buf;
    buf.write_u16(0xFFFF);  // Only 2 bytes
    buf.reset_read();
    TEST_ASSERT_THROWS(buf.read_f32(), BufferOverflowError, "read_f32 with insufficient data");
    TEST_PASS("NetworkBuffer_overflow_f32");
}

void test_NetworkBuffer_overflow_string_length() {
    NetworkBuffer buf;
    buf.write_u16(0xFFFF);  // Only 2 bytes, need 4 for length
    buf.reset_read();
    TEST_ASSERT_THROWS(buf.read_string(), BufferOverflowError, "read_string length with insufficient data");
    TEST_PASS("NetworkBuffer_overflow_string_length");
}

void test_NetworkBuffer_overflow_string_content() {
    NetworkBuffer buf;
    buf.write_u32(100);  // Claims 100 bytes of content
    buf.write_u8('x');   // But only 1 byte of content
    buf.reset_read();
    TEST_ASSERT_THROWS(buf.read_string(), BufferOverflowError, "read_string content with insufficient data");
    TEST_PASS("NetworkBuffer_overflow_string_content");
}

void test_NetworkBuffer_overflow_read_bytes() {
    NetworkBuffer buf;
    buf.write_u32(0x12345678);
    buf.reset_read();
    char out[10];
    TEST_ASSERT_THROWS(buf.read_bytes(out, 10), BufferOverflowError, "read_bytes with insufficient data");
    TEST_PASS("NetworkBuffer_overflow_read_bytes");
}

void test_NetworkBuffer_roundtrip_mixed_types() {
    NetworkBuffer buf;

    // Write a mix of types
    buf.write_u8(42);
    buf.write_u16(1234);
    buf.write_u32(567890);
    buf.write_i32(-12345);
    buf.write_f32(3.14159f);
    buf.write_string("test message");
    buf.write_u8(255);

    // 1 + 2 + 4 + 4 + 4 + (4 + 12) + 1 = 32
    TEST_ASSERT_EQ(buf.size(), 32, "mixed types total size");

    buf.reset_read();
    TEST_ASSERT_EQ(buf.read_u8(), 42, "roundtrip u8");
    TEST_ASSERT_EQ(buf.read_u16(), 1234, "roundtrip u16");
    TEST_ASSERT_EQ(buf.read_u32(), 567890u, "roundtrip u32");
    TEST_ASSERT_EQ(buf.read_i32(), -12345, "roundtrip i32");
    TEST_ASSERT_FLOAT_EQ(buf.read_f32(), 3.14159f, "roundtrip f32");
    TEST_ASSERT(buf.read_string() == "test message", "roundtrip string");
    TEST_ASSERT_EQ(buf.read_u8(), 255, "roundtrip final u8");
    TEST_ASSERT(buf.at_end(), "buffer fully consumed");
    TEST_PASS("NetworkBuffer_roundtrip_mixed_types");
}

// =============================================================================
// Section 4: NetworkMessage - Envelope Parsing
// =============================================================================

void test_NetworkMessage_envelope_format() {
    // Create a test message that we can serialize
    HeartbeatMessage msg;
    msg.clientTimestamp = 12345;
    msg.clientSequence = 42;

    NetworkBuffer buffer;
    msg.serializeWithEnvelope(buffer);

    // Verify envelope format: [version:1][type:2][length:2][payload:N]
    TEST_ASSERT(buffer.size() >= MESSAGE_HEADER_SIZE, "Buffer has header");

    buffer.reset_read();

    // Read raw header bytes
    std::uint8_t version = buffer.read_u8();
    std::uint16_t type = buffer.read_u16();
    std::uint16_t length = buffer.read_u16();

    TEST_ASSERT_EQ(version, PROTOCOL_VERSION, "Protocol version correct");
    TEST_ASSERT_EQ(type, static_cast<std::uint16_t>(MessageType::Heartbeat), "Message type correct");
    TEST_ASSERT_EQ(length, msg.getPayloadSize(), "Payload length correct");
    TEST_PASS("NetworkMessage_envelope_format");
}

void test_NetworkMessage_envelope_parse() {
    HeartbeatMessage msg;
    msg.clientTimestamp = 12345;
    msg.clientSequence = 42;

    NetworkBuffer buffer;
    msg.serializeWithEnvelope(buffer);

    // Parse the envelope
    buffer.reset_read();
    EnvelopeHeader header = NetworkMessage::parseEnvelope(buffer);

    TEST_ASSERT(header.isValid(), "Header is valid");
    TEST_ASSERT_EQ(header.protocolVersion, PROTOCOL_VERSION, "Version matches");
    TEST_ASSERT(header.type == MessageType::Heartbeat, "Type matches");
    TEST_ASSERT_EQ(header.payloadLength, msg.getPayloadSize(), "Length matches");
    TEST_PASS("NetworkMessage_envelope_parse");
}

void test_NetworkMessage_envelope_invalid_version() {
    NetworkBuffer buffer;

    // Write invalid version
    buffer.write_u8(0); // Version 0 is invalid
    buffer.write_u16(static_cast<std::uint16_t>(MessageType::Heartbeat));
    buffer.write_u16(0); // No payload

    buffer.reset_read();
    EnvelopeHeader header = NetworkMessage::parseEnvelope(buffer);

    TEST_ASSERT(!header.isValid(), "Version 0 is invalid");
    TEST_ASSERT(!header.isVersionCompatible(), "Version 0 is not compatible");
    TEST_PASS("NetworkMessage_envelope_invalid_version");
}

void test_NetworkMessage_envelope_insufficient_data() {
    NetworkBuffer buffer;

    // Write partial header (only 3 bytes of 5)
    buffer.write_u8(PROTOCOL_VERSION);
    buffer.write_u16(static_cast<std::uint16_t>(MessageType::Heartbeat));
    // Missing length field

    buffer.reset_read();
    EnvelopeHeader header = NetworkMessage::parseEnvelope(buffer);

    TEST_ASSERT(!header.isValid(), "Partial header is invalid");
    TEST_ASSERT(header.type == MessageType::Invalid, "Type is Invalid on parse failure");
    TEST_PASS("NetworkMessage_envelope_insufficient_data");
}

void test_NetworkMessage_envelope_truncated_payload() {
    NetworkBuffer buffer;

    // Write header claiming 100 bytes of payload
    buffer.write_u8(PROTOCOL_VERSION);
    buffer.write_u16(static_cast<std::uint16_t>(MessageType::Heartbeat));
    buffer.write_u16(100); // Claim 100 bytes

    // Only write 10 bytes of "payload"
    for (int i = 0; i < 10; i++) {
        buffer.write_u8(static_cast<std::uint8_t>(i));
    }

    buffer.reset_read();
    EnvelopeHeader header = NetworkMessage::parseEnvelope(buffer);

    TEST_ASSERT(!header.isValid(), "Truncated payload is detected");
    TEST_PASS("NetworkMessage_envelope_truncated_payload");
}

void test_NetworkMessage_skip_unknown_type() {
    // Manually construct a message with unknown type
    NetworkBuffer buffer;
    buffer.write_u8(PROTOCOL_VERSION);
    buffer.write_u16(999); // Unknown type
    buffer.write_u16(8);   // 8 bytes payload

    // Write 8 bytes of payload
    buffer.write_u32(0x12345678);
    buffer.write_u32(0xABCDEF00);

    // Write a second (valid) message after it
    HeartbeatMessage secondMsg;
    secondMsg.clientTimestamp = 42;
    secondMsg.clientSequence = 1;
    secondMsg.serializeWithEnvelope(buffer);

    // Parse first message header
    buffer.reset_read();
    EnvelopeHeader header1 = NetworkMessage::parseEnvelope(buffer);
    TEST_ASSERT(header1.isVersionCompatible(), "Version is compatible");
    TEST_ASSERT(header1.type == static_cast<MessageType>(999), "Unknown type parsed");

    // Factory returns nullptr for unknown type
    auto msg1 = MessageFactory::create(header1.type);
    TEST_ASSERT(msg1 == nullptr, "Factory returns nullptr for unknown type");

    // Skip the payload
    bool skipped = NetworkMessage::skipPayload(buffer, header1.payloadLength);
    TEST_ASSERT(skipped, "Payload skipped successfully");

    // Parse second message - should work
    EnvelopeHeader header2 = NetworkMessage::parseEnvelope(buffer);
    TEST_ASSERT(header2.isValid(), "Second header is valid");
    TEST_ASSERT(header2.type == MessageType::Heartbeat, "Second message is Heartbeat");
    TEST_PASS("NetworkMessage_skip_unknown_type");
}

// =============================================================================
// Section 5: Client Messages - All Client-to-Server Types
// =============================================================================

void test_JoinMessage_roundtrip() {
    JoinMessage src;
    src.playerName = "TestPlayer";
    src.hasSessionToken = false;

    NetworkBuffer buffer;
    src.serializeWithEnvelope(buffer);

    buffer.reset_read();
    EnvelopeHeader header = NetworkMessage::parseEnvelope(buffer);
    TEST_ASSERT(header.isValid(), "Header is valid");
    TEST_ASSERT(header.type == MessageType::Join, "Type is Join");

    auto msg = MessageFactory::create(header.type);
    TEST_ASSERT(msg != nullptr, "Created message");
    TEST_ASSERT(msg->deserializePayload(buffer), "Deserialized");

    JoinMessage* dst = dynamic_cast<JoinMessage*>(msg.get());
    TEST_ASSERT(dst != nullptr, "Cast succeeded");
    TEST_ASSERT(dst->playerName == "TestPlayer", "Name matches");
    TEST_ASSERT(!dst->hasSessionToken, "No token");
    TEST_PASS("JoinMessage_roundtrip");
}

void test_JoinMessage_with_token() {
    JoinMessage src;
    src.playerName = "ReconnectingPlayer";
    src.hasSessionToken = true;
    src.sessionToken = generateTestToken();

    NetworkBuffer buffer;
    src.serializeWithEnvelope(buffer);

    buffer.reset_read();
    EnvelopeHeader header = NetworkMessage::parseEnvelope(buffer);
    auto msg = MessageFactory::create(header.type);
    TEST_ASSERT(msg->deserializePayload(buffer), "Deserialized");

    JoinMessage* dst = dynamic_cast<JoinMessage*>(msg.get());
    TEST_ASSERT(dst->playerName == "ReconnectingPlayer", "Name matches");
    TEST_ASSERT(dst->hasSessionToken, "Has token");
    TEST_ASSERT(dst->sessionToken == src.sessionToken, "Token matches");
    TEST_PASS("JoinMessage_with_token");
}

void test_JoinMessage_validation() {
    JoinMessage emptyName;
    emptyName.playerName = "";
    TEST_ASSERT(!emptyName.isValid(), "Empty name is invalid");

    JoinMessage tooLongName;
    tooLongName.playerName = std::string(MAX_PLAYER_NAME_LENGTH + 10, 'X');
    TEST_ASSERT(!tooLongName.isValid(), "Oversized name is invalid");

    JoinMessage validMsg;
    validMsg.playerName = "ValidPlayer";
    TEST_ASSERT(validMsg.isValid(), "Normal name is valid");
    TEST_PASS("JoinMessage_validation");
}

void test_NetInputMessage_roundtrip() {
    NetInputMessage src;
    src.input.tick = 12345;
    src.input.playerId = 1;
    src.input.type = InputType::PlaceBuilding;
    src.input.sequenceNum = 42;
    src.input.targetPos = {100, 200};
    src.input.param1 = 5;
    src.input.param2 = 0;
    src.input.value = 0;

    NetworkBuffer buffer;
    src.serializeWithEnvelope(buffer);

    buffer.reset_read();
    EnvelopeHeader header = NetworkMessage::parseEnvelope(buffer);
    TEST_ASSERT(header.isValid(), "Header is valid");
    TEST_ASSERT(header.type == MessageType::Input, "Type is Input");

    auto msg = MessageFactory::create(header.type);
    TEST_ASSERT(msg->deserializePayload(buffer), "Deserialized");

    NetInputMessage* dst = dynamic_cast<NetInputMessage*>(msg.get());
    TEST_ASSERT(dst->input.tick == 12345, "Tick matches");
    TEST_ASSERT(dst->input.playerId == 1, "PlayerId matches");
    TEST_ASSERT(dst->input.type == InputType::PlaceBuilding, "InputType matches");
    TEST_ASSERT(dst->input.sequenceNum == 42, "SequenceNum matches");
    TEST_ASSERT(dst->input.targetPos.x == 100, "TargetPos.x matches");
    TEST_ASSERT(dst->input.targetPos.y == 200, "TargetPos.y matches");
    TEST_PASS("NetInputMessage_roundtrip");
}

void test_NetInputMessage_negative_values() {
    NetInputMessage src;
    src.input.tick = 0xFFFFFFFFFFFFFFFF;
    src.input.playerId = 4;
    src.input.type = InputType::SetTaxRate;
    src.input.sequenceNum = 0xFFFFFFFF;
    src.input.targetPos = {-100, -200};
    src.input.param1 = 0xFFFFFFFF;
    src.input.param2 = 0xFFFFFFFF;
    src.input.value = -12345;

    NetworkBuffer buffer;
    src.serializeWithEnvelope(buffer);
    buffer.reset_read();

    EnvelopeHeader header = NetworkMessage::parseEnvelope(buffer);
    auto msg = MessageFactory::create(header.type);
    TEST_ASSERT(msg->deserializePayload(buffer), "Deserialized negative values");

    NetInputMessage* dst = dynamic_cast<NetInputMessage*>(msg.get());
    TEST_ASSERT(dst->input.tick == 0xFFFFFFFFFFFFFFFF, "Max tick preserved");
    TEST_ASSERT(dst->input.targetPos.x == -100, "Negative x preserved");
    TEST_ASSERT(dst->input.targetPos.y == -200, "Negative y preserved");
    TEST_ASSERT(dst->input.value == -12345, "Negative value preserved");
    TEST_PASS("NetInputMessage_negative_values");
}

void test_NetInputMessage_validation() {
    NetInputMessage invalidPlayerId;
    invalidPlayerId.input.playerId = 0;
    invalidPlayerId.input.type = InputType::PlaceBuilding;
    TEST_ASSERT(!invalidPlayerId.isValid(), "PlayerId 0 is invalid");

    NetInputMessage invalidType;
    invalidType.input.playerId = 1;
    invalidType.input.type = InputType::None;
    TEST_ASSERT(!invalidType.isValid(), "InputType::None is invalid");

    NetInputMessage validMsg;
    validMsg.input.playerId = 1;
    validMsg.input.type = InputType::PlaceRoad;
    TEST_ASSERT(validMsg.isValid(), "Normal input is valid");
    TEST_PASS("NetInputMessage_validation");
}

void test_ChatMessage_roundtrip() {
    ChatMessage src;
    src.senderId = 2;
    src.text = "Hello, world!";
    src.timestamp = 1234567890;

    NetworkBuffer buffer;
    src.serializeWithEnvelope(buffer);

    buffer.reset_read();
    EnvelopeHeader header = NetworkMessage::parseEnvelope(buffer);
    TEST_ASSERT(header.type == MessageType::Chat, "Type is Chat");

    auto msg = MessageFactory::create(header.type);
    TEST_ASSERT(msg->deserializePayload(buffer), "Deserialized");

    ChatMessage* dst = dynamic_cast<ChatMessage*>(msg.get());
    TEST_ASSERT(dst->senderId == 2, "SenderId matches");
    TEST_ASSERT(dst->text == "Hello, world!", "Text matches");
    TEST_ASSERT(dst->timestamp == 1234567890, "Timestamp matches");
    TEST_PASS("ChatMessage_roundtrip");
}

void test_ChatMessage_validation() {
    ChatMessage emptyText;
    emptyText.text = "";
    TEST_ASSERT(!emptyText.isValid(), "Empty text is invalid");

    ChatMessage tooLongText;
    tooLongText.text = std::string(MAX_CHAT_MESSAGE_LENGTH + 10, 'X');
    TEST_ASSERT(!tooLongText.isValid(), "Oversized text is invalid");

    ChatMessage validMsg;
    validMsg.text = "Valid message";
    TEST_ASSERT(validMsg.isValid(), "Normal text is valid");
    TEST_PASS("ChatMessage_validation");
}

void test_HeartbeatMessage_roundtrip() {
    HeartbeatMessage src;
    src.clientTimestamp = 0x123456789ABCDEF0;
    src.clientSequence = 42;

    NetworkBuffer buffer;
    src.serializeWithEnvelope(buffer);

    buffer.reset_read();
    EnvelopeHeader header = NetworkMessage::parseEnvelope(buffer);
    TEST_ASSERT(header.type == MessageType::Heartbeat, "Type is Heartbeat");

    auto msg = MessageFactory::create(header.type);
    TEST_ASSERT(msg->deserializePayload(buffer), "Deserialized");

    HeartbeatMessage* dst = dynamic_cast<HeartbeatMessage*>(msg.get());
    TEST_ASSERT(dst->clientTimestamp == 0x123456789ABCDEF0, "Timestamp matches");
    TEST_ASSERT(dst->clientSequence == 42, "Sequence matches");
    TEST_PASS("HeartbeatMessage_roundtrip");
}

void test_ReconnectMessage_roundtrip() {
    ReconnectMessage src;
    src.sessionToken = generateTestToken();
    src.playerName = "ReconnectingPlayer";

    NetworkBuffer buffer;
    src.serializeWithEnvelope(buffer);

    buffer.reset_read();
    EnvelopeHeader header = NetworkMessage::parseEnvelope(buffer);
    TEST_ASSERT(header.type == MessageType::Reconnect, "Type is Reconnect");

    auto msg = MessageFactory::create(header.type);
    TEST_ASSERT(msg->deserializePayload(buffer), "Deserialized");

    ReconnectMessage* dst = dynamic_cast<ReconnectMessage*>(msg.get());
    TEST_ASSERT(dst->sessionToken == src.sessionToken, "Token matches");
    TEST_ASSERT(dst->playerName == "ReconnectingPlayer", "Name matches");
    TEST_PASS("ReconnectMessage_roundtrip");
}

void test_ReconnectMessage_validation() {
    ReconnectMessage zeroToken;
    zeroToken.sessionToken = {}; // All zeros
    zeroToken.playerName = "Player";
    TEST_ASSERT(!zeroToken.isValid(), "All-zero token is invalid");

    ReconnectMessage validMsg;
    validMsg.sessionToken = generateTestToken();
    validMsg.playerName = "ValidPlayer";
    TEST_ASSERT(validMsg.isValid(), "Normal reconnect is valid");
    TEST_PASS("ReconnectMessage_validation");
}

// =============================================================================
// Section 6: Server Messages - All Server-to-Client Types
// =============================================================================

void test_StateUpdateMessage_empty_deltas() {
    StateUpdateMessage msg;
    msg.tick = 12345;
    msg.compressed = false;

    TEST_ASSERT(!msg.hasDeltas(), "No deltas initially");

    NetworkBuffer buffer;
    msg.serializePayload(buffer);

    buffer.reset_read();
    StateUpdateMessage msg2;
    bool ok = msg2.deserializePayload(buffer);

    TEST_ASSERT(ok, "Deserialization succeeded");
    TEST_ASSERT(msg2.tick == 12345, "Tick matches");
    TEST_ASSERT(!msg2.compressed, "Compressed flag matches");
    TEST_ASSERT(!msg2.hasDeltas(), "No deltas");
    TEST_PASS("StateUpdateMessage_empty_deltas");
}

void test_StateUpdateMessage_create_update_destroy() {
    StateUpdateMessage msg;
    msg.tick = 100;

    msg.addCreate(1, {0x01, 0x02});
    msg.addUpdate(2, {0x03, 0x04, 0x05});
    msg.addDestroy(3);

    TEST_ASSERT(msg.hasDeltas(), "Has deltas");
    TEST_ASSERT_EQ(msg.deltas.size(), 3, "Three deltas");

    NetworkBuffer buffer;
    msg.serializePayload(buffer);

    buffer.reset_read();
    StateUpdateMessage msg2;
    bool ok = msg2.deserializePayload(buffer);

    TEST_ASSERT(ok, "Deserialization succeeded");
    TEST_ASSERT_EQ(msg2.deltas.size(), 3, "Three deltas");
    TEST_ASSERT(msg2.deltas[0].type == EntityDeltaType::Create, "First is Create");
    TEST_ASSERT(msg2.deltas[1].type == EntityDeltaType::Update, "Second is Update");
    TEST_ASSERT(msg2.deltas[2].type == EntityDeltaType::Destroy, "Third is Destroy");
    TEST_PASS("StateUpdateMessage_create_update_destroy");
}

void test_SnapshotMessages_roundtrip() {
    // SnapshotStart
    SnapshotStartMessage startMsg;
    startMsg.tick = 9876543210ULL;
    startMsg.totalChunks = 15;
    startMsg.totalBytes = 1000000;
    startMsg.compressedBytes = 500000;
    startMsg.entityCount = 5000;

    NetworkBuffer buf1;
    startMsg.serializePayload(buf1);
    buf1.reset_read();
    SnapshotStartMessage startMsg2;
    TEST_ASSERT(startMsg2.deserializePayload(buf1), "SnapshotStart deserialization succeeded");
    TEST_ASSERT(startMsg2.tick == 9876543210ULL, "SnapshotStart tick matches");
    TEST_ASSERT_EQ(startMsg2.totalChunks, 15, "SnapshotStart totalChunks matches");

    // SnapshotChunk
    SnapshotChunkMessage chunkMsg;
    chunkMsg.chunkIndex = 7;
    chunkMsg.data = {0x01, 0x02, 0x03, 0x04, 0x05};

    NetworkBuffer buf2;
    chunkMsg.serializePayload(buf2);
    buf2.reset_read();
    SnapshotChunkMessage chunkMsg2;
    TEST_ASSERT(chunkMsg2.deserializePayload(buf2), "SnapshotChunk deserialization succeeded");
    TEST_ASSERT_EQ(chunkMsg2.chunkIndex, 7, "SnapshotChunk index matches");
    TEST_ASSERT(chunkMsg2.data == chunkMsg.data, "SnapshotChunk data matches");

    // SnapshotEnd
    SnapshotEndMessage endMsg;
    endMsg.checksum = 0xDEADBEEF;

    NetworkBuffer buf3;
    endMsg.serializePayload(buf3);
    buf3.reset_read();
    SnapshotEndMessage endMsg2;
    TEST_ASSERT(endMsg2.deserializePayload(buf3), "SnapshotEnd deserialization succeeded");
    TEST_ASSERT_EQ(endMsg2.checksum, 0xDEADBEEF, "SnapshotEnd checksum matches");
    TEST_PASS("SnapshotMessages_roundtrip");
}

void test_PlayerListMessage_roundtrip() {
    PlayerListMessage msg;
    msg.addPlayer(1, "Alice", PlayerStatus::Connected, 30);
    msg.addPlayer(2, "Bob", PlayerStatus::Connected, 45);
    msg.addPlayer(3, "Charlie", PlayerStatus::Connecting, 0);

    NetworkBuffer buffer;
    msg.serializePayload(buffer);

    buffer.reset_read();
    PlayerListMessage msg2;
    TEST_ASSERT(msg2.deserializePayload(buffer), "Deserialization succeeded");
    TEST_ASSERT_EQ(msg2.players.size(), 3, "Three players");
    TEST_ASSERT(msg2.players[0].name == "Alice", "Alice's name");
    TEST_ASSERT(msg2.players[1].name == "Bob", "Bob's name");
    TEST_ASSERT(msg2.players[2].status == PlayerStatus::Connecting, "Charlie's status");
    TEST_PASS("PlayerListMessage_roundtrip");
}

void test_RejectionMessage_roundtrip() {
    RejectionMessage msg;
    msg.inputSequenceNum = 12345;
    msg.reason = RejectionReason::InsufficientFunds;
    msg.message = "Not enough credits!";

    NetworkBuffer buffer;
    msg.serializePayload(buffer);

    buffer.reset_read();
    RejectionMessage msg2;
    TEST_ASSERT(msg2.deserializePayload(buffer), "Deserialization succeeded");
    TEST_ASSERT_EQ(msg2.inputSequenceNum, 12345, "Sequence number matches");
    TEST_ASSERT(msg2.reason == RejectionReason::InsufficientFunds, "Reason matches");
    TEST_ASSERT(msg2.message == "Not enough credits!", "Message matches");
    TEST_PASS("RejectionMessage_roundtrip");
}

void test_EventMessage_roundtrip() {
    EventMessage msg;
    msg.tick = 5000;
    msg.eventType = GameEventType::MilestoneReached;
    msg.relatedEntity = 42;
    msg.location = {100, 200};
    msg.param1 = 10000;
    msg.param2 = 1;
    msg.description = "Population reached 10,000!";

    NetworkBuffer buffer;
    msg.serializePayload(buffer);

    buffer.reset_read();
    EventMessage msg2;
    TEST_ASSERT(msg2.deserializePayload(buffer), "Deserialization succeeded");
    TEST_ASSERT(msg2.tick == 5000, "Tick matches");
    TEST_ASSERT(msg2.eventType == GameEventType::MilestoneReached, "Event type matches");
    TEST_ASSERT_EQ(msg2.relatedEntity, 42, "Entity matches");
    TEST_ASSERT(msg2.location.x == 100, "Location X matches");
    TEST_PASS("EventMessage_roundtrip");
}

void test_HeartbeatResponseMessage_roundtrip() {
    HeartbeatResponseMessage msg;
    msg.clientTimestamp = 1234567890123ULL;
    msg.serverTimestamp = 1234567890200ULL;
    msg.serverTick = 50000;

    NetworkBuffer buffer;
    msg.serializePayload(buffer);

    buffer.reset_read();
    HeartbeatResponseMessage msg2;
    TEST_ASSERT(msg2.deserializePayload(buffer), "Deserialization succeeded");
    TEST_ASSERT(msg2.clientTimestamp == 1234567890123ULL, "Client timestamp matches");
    TEST_ASSERT(msg2.serverTimestamp == 1234567890200ULL, "Server timestamp matches");
    TEST_ASSERT(msg2.serverTick == 50000, "Server tick matches");
    TEST_PASS("HeartbeatResponseMessage_roundtrip");
}

void test_ServerStatusMessage_roundtrip() {
    ServerStatusMessage msg;
    msg.state = ServerState::Running;
    msg.mapSizeTier = MapSizeTier::Medium;
    msg.mapWidth = 256;
    msg.mapHeight = 256;
    msg.maxPlayers = 4;
    msg.currentPlayers = 2;
    msg.currentTick = 10000;
    msg.serverName = "Test Server";

    NetworkBuffer buffer;
    msg.serializePayload(buffer);

    buffer.reset_read();
    ServerStatusMessage msg2;
    TEST_ASSERT(msg2.deserializePayload(buffer), "Deserialization succeeded");
    TEST_ASSERT(msg2.state == ServerState::Running, "State matches");
    TEST_ASSERT(msg2.mapSizeTier == MapSizeTier::Medium, "Map size tier matches");
    TEST_ASSERT_EQ(msg2.mapWidth, 256, "Map width matches");
    TEST_ASSERT_EQ(msg2.maxPlayers, 4, "Max players matches");
    TEST_ASSERT(msg2.serverName == "Test Server", "Server name matches");
    TEST_PASS("ServerStatusMessage_roundtrip");
}

void test_JoinAcceptMessage_roundtrip() {
    JoinAcceptMessage msg;
    msg.playerId = 2;
    for (std::size_t i = 0; i < msg.sessionToken.size(); ++i) {
        msg.sessionToken[i] = static_cast<std::uint8_t>(i + 1);
    }
    msg.serverTick = 12345;

    NetworkBuffer buffer;
    msg.serializePayload(buffer);

    buffer.reset_read();
    JoinAcceptMessage msg2;
    TEST_ASSERT(msg2.deserializePayload(buffer), "Deserialization succeeded");
    TEST_ASSERT_EQ(msg2.playerId, 2, "Player ID matches");
    TEST_ASSERT(msg2.sessionToken == msg.sessionToken, "Token matches");
    TEST_ASSERT(msg2.serverTick == 12345, "Server tick matches");
    TEST_PASS("JoinAcceptMessage_roundtrip");
}

void test_JoinRejectMessage_roundtrip() {
    JoinRejectMessage msg;
    msg.reason = JoinRejectReason::ServerFull;
    msg.message = "Server is full";

    NetworkBuffer buffer;
    msg.serializePayload(buffer);

    buffer.reset_read();
    JoinRejectMessage msg2;
    TEST_ASSERT(msg2.deserializePayload(buffer), "Deserialization succeeded");
    TEST_ASSERT(msg2.reason == JoinRejectReason::ServerFull, "Reason matches");
    TEST_ASSERT(msg2.message == "Server is full", "Message matches");
    TEST_PASS("JoinRejectMessage_roundtrip");
}

void test_KickMessage_roundtrip() {
    KickMessage msg;
    msg.reason = "Cheating detected";

    NetworkBuffer buffer;
    msg.serializePayload(buffer);

    buffer.reset_read();
    KickMessage msg2;
    TEST_ASSERT(msg2.deserializePayload(buffer), "Deserialization succeeded");
    TEST_ASSERT(msg2.reason == "Cheating detected", "Reason matches");
    TEST_PASS("KickMessage_roundtrip");
}

// =============================================================================
// Section 7: NetworkClient State Machine
// =============================================================================

void test_NetworkClient_initial_state() {
    auto transport = std::make_unique<MockTransport>();
    NetworkClient client(std::move(transport));

    TEST_ASSERT(client.getState() == ConnectionState::Disconnected, "Initial state is Disconnected");
    TEST_ASSERT(!client.isPlaying(), "isPlaying is false initially");
    TEST_ASSERT(!client.isConnecting(), "isConnecting is false initially");
    TEST_ASSERT_EQ(client.getPlayerId(), 0, "Player ID is 0 initially");
    TEST_PASS("NetworkClient_initial_state");
}

void test_NetworkClient_connect_transitions_to_connecting() {
    auto transport = std::make_unique<MockTransport>();
    NetworkClient client(std::move(transport));

    std::vector<ConnectionState> stateHistory;
    client.setStateChangeCallback([&](ConnectionState oldState, ConnectionState newState) {
        (void)oldState;
        stateHistory.push_back(newState);
    });

    bool result = client.connect("127.0.0.1", 7777, "TestPlayer");

    TEST_ASSERT(result, "connect() returns true");
    TEST_ASSERT(client.getState() == ConnectionState::Connecting, "State is Connecting");
    TEST_ASSERT(client.isConnecting(), "isConnecting is true");
    TEST_ASSERT_EQ(stateHistory.size(), 1, "One state change");
    TEST_ASSERT(stateHistory[0] == ConnectionState::Connecting, "Transitioned to Connecting");

    client.disconnect();
    TEST_PASS("NetworkClient_connect_transitions_to_connecting");
}

void test_NetworkClient_connect_while_connecting_fails() {
    auto transport = std::make_unique<MockTransport>();
    NetworkClient client(std::move(transport));

    client.connect("127.0.0.1", 7777, "TestPlayer");
    bool result = client.connect("127.0.0.1", 8888, "OtherPlayer");

    TEST_ASSERT(!result, "Second connect fails");
    TEST_ASSERT(client.getState() == ConnectionState::Connecting, "State remains Connecting");

    client.disconnect();
    TEST_PASS("NetworkClient_connect_while_connecting_fails");
}

void test_NetworkClient_connect_empty_address_fails() {
    auto transport = std::make_unique<MockTransport>();
    NetworkClient client(std::move(transport));

    bool result = client.connect("", 7777, "TestPlayer");

    TEST_ASSERT(!result, "connect with empty address fails");
    TEST_ASSERT(client.getState() == ConnectionState::Disconnected, "State remains Disconnected");
    TEST_PASS("NetworkClient_connect_empty_address_fails");
}

void test_NetworkClient_connect_empty_name_fails() {
    auto transport = std::make_unique<MockTransport>();
    NetworkClient client(std::move(transport));

    bool result = client.connect("127.0.0.1", 7777, "");

    TEST_ASSERT(!result, "connect with empty name fails");
    TEST_ASSERT(client.getState() == ConnectionState::Disconnected, "State remains Disconnected");
    TEST_PASS("NetworkClient_connect_empty_name_fails");
}

void test_NetworkClient_disconnect_from_disconnected() {
    auto transport = std::make_unique<MockTransport>();
    NetworkClient client(std::move(transport));

    // Should not crash
    client.disconnect();
    TEST_ASSERT(client.getState() == ConnectionState::Disconnected, "State remains Disconnected");
    TEST_PASS("NetworkClient_disconnect_from_disconnected");
}

void test_NetworkClient_disconnect_from_connecting() {
    auto transport = std::make_unique<MockTransport>();
    NetworkClient client(std::move(transport));

    client.connect("127.0.0.1", 7777, "TestPlayer");
    TEST_ASSERT(client.getState() == ConnectionState::Connecting, "State is Connecting");

    client.disconnect();
    TEST_ASSERT(client.getState() == ConnectionState::Disconnected, "State is Disconnected");
    TEST_ASSERT(!client.isConnecting(), "isConnecting is false");
    TEST_PASS("NetworkClient_disconnect_from_connecting");
}

void test_NetworkClient_input_queuing_when_not_playing() {
    auto transport = std::make_unique<MockTransport>();
    NetworkClient client(std::move(transport));

    InputMessage input;
    input.type = InputType::PlaceBuilding;
    input.targetPos = {10, 20};

    client.queueInput(input);

    TEST_ASSERT_EQ(client.getPendingInputCount(), 0, "Input ignored when not playing");
    TEST_PASS("NetworkClient_input_queuing_when_not_playing");
}

void test_NetworkClient_state_change_callback() {
    auto transport = std::make_unique<MockTransport>();
    NetworkClient client(std::move(transport));

    int callbackCount = 0;
    ConnectionState lastOldState = ConnectionState::Disconnected;
    ConnectionState lastNewState = ConnectionState::Disconnected;

    client.setStateChangeCallback([&](ConnectionState oldState, ConnectionState newState) {
        callbackCount++;
        lastOldState = oldState;
        lastNewState = newState;
    });

    client.connect("127.0.0.1", 7777, "TestPlayer");

    TEST_ASSERT_EQ(callbackCount, 1, "Callback called once");
    TEST_ASSERT(lastOldState == ConnectionState::Disconnected, "Old state is Disconnected");
    TEST_ASSERT(lastNewState == ConnectionState::Connecting, "New state is Connecting");

    client.disconnect();

    TEST_ASSERT_EQ(callbackCount, 2, "Callback called twice");
    TEST_ASSERT(lastNewState == ConnectionState::Disconnected, "Final state is Disconnected");
    TEST_PASS("NetworkClient_state_change_callback");
}

void test_NetworkClient_connection_stats_initial() {
    auto transport = std::make_unique<MockTransport>();
    NetworkClient client(std::move(transport));

    const ConnectionStats& stats = client.getStats();

    TEST_ASSERT_EQ(stats.rttMs, 0, "Initial RTT is 0");
    TEST_ASSERT_EQ(stats.smoothedRttMs, 0, "Initial smoothed RTT is 0");
    TEST_ASSERT_EQ(stats.reconnectAttempts, 0, "Initial reconnect attempts is 0");
    TEST_ASSERT_EQ(stats.messagesSent, 0, "Initial messages sent is 0");
    TEST_ASSERT_EQ(stats.messagesReceived, 0, "Initial messages received is 0");
    TEST_ASSERT(stats.timeoutLevel == ConnectionTimeoutLevel::None, "Initial timeout level is None");
    TEST_PASS("NetworkClient_connection_stats_initial");
}

void test_NetworkClient_connection_config_defaults() {
    ConnectionConfig config;

    TEST_ASSERT_EQ(config.initialReconnectDelayMs, 2000, "Initial reconnect delay is 2000ms");
    TEST_ASSERT_EQ(config.maxReconnectDelayMs, 30000, "Max reconnect delay is 30000ms");
    TEST_ASSERT_EQ(config.heartbeatIntervalMs, 1000, "Heartbeat interval is 1000ms");
    TEST_ASSERT_EQ(config.timeoutIndicatorMs, 2000, "Timeout indicator is 2s");
    TEST_ASSERT_EQ(config.timeoutBannerMs, 5000, "Timeout banner is 5s");
    TEST_ASSERT_EQ(config.timeoutFullUIMs, 15000, "Timeout full UI is 15s");
    TEST_PASS("NetworkClient_connection_config_defaults");
}

void test_NetworkClient_connection_state_names() {
    TEST_ASSERT(std::string(getConnectionStateName(ConnectionState::Disconnected)) == "Disconnected",
                "Disconnected name");
    TEST_ASSERT(std::string(getConnectionStateName(ConnectionState::Connecting)) == "Connecting",
                "Connecting name");
    TEST_ASSERT(std::string(getConnectionStateName(ConnectionState::Connected)) == "Connected",
                "Connected name");
    TEST_ASSERT(std::string(getConnectionStateName(ConnectionState::Playing)) == "Playing",
                "Playing name");
    TEST_ASSERT(std::string(getConnectionStateName(ConnectionState::Reconnecting)) == "Reconnecting",
                "Reconnecting name");
    TEST_PASS("NetworkClient_connection_state_names");
}

void test_NetworkClient_update_while_disconnected() {
    auto transport = std::make_unique<MockTransport>();
    NetworkClient client(std::move(transport));

    // Should not crash
    client.update(0.016f);
    client.update(0.016f);

    TEST_ASSERT(client.getState() == ConnectionState::Disconnected, "State remains Disconnected");
    TEST_PASS("NetworkClient_update_while_disconnected");
}

void test_NetworkClient_poll_state_update_empty() {
    auto transport = std::make_unique<MockTransport>();
    NetworkClient client(std::move(transport));

    StateUpdateMessage update;
    bool hasUpdate = client.pollStateUpdate(update);

    TEST_ASSERT(!hasUpdate, "pollStateUpdate returns false when empty");
    TEST_PASS("NetworkClient_poll_state_update_empty");
}

// =============================================================================
// Section 8: NetworkServer State Machine
// =============================================================================

void test_NetworkServer_creation() {
    ServerConfig config;
    config.port = 7777;
    config.maxPlayers = 4;
    config.mapSize = MapSizeTier::Medium;
    config.serverName = "Test Server";

    auto transport = std::make_unique<MockTransport>();
    NetworkServer server(std::move(transport), config);

    TEST_ASSERT(!server.isRunning(), "Server not running initially");
    TEST_ASSERT(server.getState() == ServerNetworkState::Initializing, "Initial state is Initializing");
    TEST_ASSERT_EQ(server.getConfig().port, 7777, "Port matches");
    TEST_ASSERT_EQ(server.getConfig().maxPlayers, 4, "Max players matches");
    TEST_PASS("NetworkServer_creation");
}

void test_NetworkServer_start_stop() {
    ServerConfig config;
    config.port = 7778;

    auto transport = std::make_unique<MockTransport>();
    NetworkServer server(std::move(transport), config);

    TEST_ASSERT(server.start(), "Server starts");
    TEST_ASSERT(server.isRunning(), "Server is running");
    TEST_ASSERT(server.getState() == ServerNetworkState::Ready, "State is Ready");

    server.stop();
    TEST_ASSERT(!server.isRunning(), "Server not running");
    TEST_ASSERT(server.getState() == ServerNetworkState::Initializing, "State is Initializing");
    TEST_PASS("NetworkServer_start_stop");
}

void test_NetworkServer_state_transitions() {
    ServerConfig config;
    auto transport = std::make_unique<MockTransport>();
    NetworkServer server(std::move(transport), config);

    // Initial state
    TEST_ASSERT(server.getState() == ServerNetworkState::Initializing, "Initial state");

    // After start
    TEST_ASSERT(server.start(), "Server starts");
    TEST_ASSERT(server.getState() == ServerNetworkState::Ready, "State is Ready after start");

    // Transition to running
    server.setRunning();
    TEST_ASSERT(server.getState() == ServerNetworkState::Running, "State is Running");

    // After stop
    server.stop();
    TEST_ASSERT(server.getState() == ServerNetworkState::Initializing, "State is Initializing after stop");
    TEST_PASS("NetworkServer_state_transitions");
}

void test_NetworkServer_max_players_enforcement() {
    ServerConfig config;
    config.maxPlayers = 10; // Try to set more than allowed

    auto transport = std::make_unique<MockTransport>();
    NetworkServer server(std::move(transport), config);

    // Should be capped at MAX_PLAYERS (4)
    TEST_ASSERT_EQ(server.getConfig().maxPlayers, NetworkServer::MAX_PLAYERS, "Max players capped at 4");
    TEST_PASS("NetworkServer_max_players_enforcement");
}

void test_NetworkServer_client_count_initially_zero() {
    ServerConfig config;
    auto transport = std::make_unique<MockTransport>();
    NetworkServer server(std::move(transport), config);

    TEST_ASSERT(server.start(), "Server starts");
    TEST_ASSERT_EQ(server.getClientCount(), 0, "Client count is 0");
    TEST_ASSERT(server.getClients().empty(), "Client list is empty");

    server.stop();
    TEST_PASS("NetworkServer_client_count_initially_zero");
}

void test_NetworkServer_uptime_tracking() {
    ServerConfig config;
    auto transport = std::make_unique<MockTransport>();
    NetworkServer server(std::move(transport), config);

    TEST_ASSERT(server.start(), "Server starts");
    TEST_ASSERT_FLOAT_EQ(server.getUptime(), 0.0f, "Initial uptime is 0");

    server.update(0.5f);
    TEST_ASSERT(server.getUptime() >= 0.5f, "Uptime after 0.5s update");

    server.update(0.5f);
    TEST_ASSERT(server.getUptime() >= 1.0f, "Uptime after 1s total");

    server.stop();
    TEST_PASS("NetworkServer_uptime_tracking");
}

void test_NetworkServer_tick_tracking() {
    ServerConfig config;
    auto transport = std::make_unique<MockTransport>();
    NetworkServer server(std::move(transport), config);

    TEST_ASSERT(server.start(), "Server starts");
    TEST_ASSERT_EQ(server.getCurrentTick(), 0, "Initial tick is 0");

    server.setCurrentTick(100);
    TEST_ASSERT_EQ(server.getCurrentTick(), 100, "Tick set to 100");

    server.setCurrentTick(12345);
    TEST_ASSERT_EQ(server.getCurrentTick(), 12345, "Tick set to 12345");

    server.stop();
    TEST_PASS("NetworkServer_tick_tracking");
}

void test_NetworkServer_state_name_helper() {
    TEST_ASSERT(std::string(getServerNetworkStateName(ServerNetworkState::Initializing)) == "Initializing",
                "Initializing name");
    TEST_ASSERT(std::string(getServerNetworkStateName(ServerNetworkState::Loading)) == "Loading",
                "Loading name");
    TEST_ASSERT(std::string(getServerNetworkStateName(ServerNetworkState::Ready)) == "Ready",
                "Ready name");
    TEST_ASSERT(std::string(getServerNetworkStateName(ServerNetworkState::Running)) == "Running",
                "Running name");
    TEST_PASS("NetworkServer_state_name_helper");
}

void test_NetworkServer_heartbeat_constants() {
    // Verify heartbeat timing constants
    TEST_ASSERT_FLOAT_EQ(NetworkServer::HEARTBEAT_INTERVAL_SEC, 1.0f, "Heartbeat interval is 1s");
    TEST_ASSERT_EQ(NetworkServer::HEARTBEAT_WARNING_THRESHOLD, 5, "Warning at 5 missed");
    TEST_ASSERT_EQ(NetworkServer::HEARTBEAT_DISCONNECT_THRESHOLD, 10, "Disconnect at 10 missed");
    TEST_PASS("NetworkServer_heartbeat_constants");
}

void test_NetworkServer_default_config() {
    ServerConfig config;

    TEST_ASSERT_EQ(config.port, 7777, "Default port is 7777");
    TEST_ASSERT_EQ(config.maxPlayers, 4, "Default max players is 4");
    TEST_ASSERT(config.mapSize == MapSizeTier::Medium, "Default map size is Medium");
    TEST_PASS("NetworkServer_default_config");
}

void test_NetworkServer_client_lookup() {
    ServerConfig config;
    auto transport = std::make_unique<MockTransport>();
    NetworkServer server(std::move(transport), config);

    TEST_ASSERT(server.start(), "Server starts");

    // No clients initially
    TEST_ASSERT(server.getClient(1) == nullptr, "getClient returns nullptr");
    TEST_ASSERT(server.getClientByPlayerId(1) == nullptr, "getClientByPlayerId returns nullptr");

    server.stop();
    TEST_PASS("NetworkServer_client_lookup");
}

void test_NetworkServer_send_to_nonexistent() {
    ServerConfig config;
    auto transport = std::make_unique<MockTransport>();
    NetworkServer server(std::move(transport), config);

    TEST_ASSERT(server.start(), "Server starts");

    ServerStatusMessage msg;
    msg.state = ServerState::Ready;

    TEST_ASSERT(!server.sendTo(999, msg), "sendTo nonexistent peer returns false");
    TEST_ASSERT(!server.sendToPlayer(1, msg), "sendToPlayer nonexistent returns false");

    server.stop();
    TEST_PASS("NetworkServer_send_to_nonexistent");
}

void test_NetworkServer_kick_nonexistent() {
    ServerConfig config;
    auto transport = std::make_unique<MockTransport>();
    NetworkServer server(std::move(transport), config);

    TEST_ASSERT(server.start(), "Server starts");

    // Should not crash
    server.kickPlayer(99, "Test reason");
    server.kickPeer(999, "Test reason");

    TEST_ASSERT_EQ(server.getClientCount(), 0, "Client count remains 0");

    server.stop();
    TEST_PASS("NetworkServer_kick_nonexistent");
}

void test_NetworkServer_broadcast_no_clients() {
    ServerConfig config;
    auto transport = std::make_unique<MockTransport>();
    NetworkServer server(std::move(transport), config);

    TEST_ASSERT(server.start(), "Server starts");

    // Should not crash
    StateUpdateMessage stateMsg;
    stateMsg.tick = 1;
    server.broadcastStateUpdate(stateMsg);

    server.broadcastServerChat("Hello world!");

    ServerStatusMessage statusMsg;
    server.broadcast(statusMsg);

    server.stop();
    TEST_PASS("NetworkServer_broadcast_no_clients");
}

void test_NetworkServer_double_start() {
    ServerConfig config;
    auto transport = std::make_unique<MockTransport>();
    NetworkServer server(std::move(transport), config);

    TEST_ASSERT(server.start(), "First start succeeds");
    TEST_ASSERT(server.isRunning(), "Server is running");

    // Second start should still succeed (returns true, already running)
    TEST_ASSERT(server.start(), "Second start succeeds");
    TEST_ASSERT(server.isRunning(), "Server still running");

    server.stop();
    TEST_PASS("NetworkServer_double_start");
}

void test_NetworkServer_double_stop() {
    ServerConfig config;
    auto transport = std::make_unique<MockTransport>();
    NetworkServer server(std::move(transport), config);

    TEST_ASSERT(server.start(), "Server starts");
    server.stop();
    TEST_ASSERT(!server.isRunning(), "Server not running");

    // Second stop should not crash
    server.stop();
    TEST_ASSERT(!server.isRunning(), "Server still not running");
    TEST_PASS("NetworkServer_double_stop");
}

void test_NetworkServer_update_when_not_running() {
    ServerConfig config;
    auto transport = std::make_unique<MockTransport>();
    NetworkServer server(std::move(transport), config);

    // Should not crash
    server.update(0.016f);
    server.update(0.016f);

    // Uptime should remain 0
    TEST_ASSERT_FLOAT_EQ(server.getUptime(), 0.0f, "Uptime is 0 when not running");
    TEST_PASS("NetworkServer_update_when_not_running");
}

// =============================================================================
// Section 9: Edge Cases - Empty Payloads, Maximum Sizes, Corrupted Data
// =============================================================================

void test_EdgeCase_empty_buffer_deserialization() {
    NetworkBuffer emptyBuffer;

    JoinMessage join;
    TEST_ASSERT(!join.deserializePayload(emptyBuffer), "JoinMessage fails on empty buffer");

    emptyBuffer.reset_read();
    NetInputMessage input;
    TEST_ASSERT(!input.deserializePayload(emptyBuffer), "NetInputMessage fails on empty buffer");

    emptyBuffer.reset_read();
    ChatMessage chat;
    TEST_ASSERT(!chat.deserializePayload(emptyBuffer), "ChatMessage fails on empty buffer");

    emptyBuffer.reset_read();
    HeartbeatMessage heartbeat;
    TEST_ASSERT(!heartbeat.deserializePayload(emptyBuffer), "HeartbeatMessage fails on empty buffer");

    emptyBuffer.reset_read();
    ReconnectMessage reconnect;
    TEST_ASSERT(!reconnect.deserializePayload(emptyBuffer), "ReconnectMessage fails on empty buffer");
    TEST_PASS("EdgeCase_empty_buffer_deserialization");
}

void test_EdgeCase_truncated_data() {
    // Create a buffer with partial NetInputMessage data (should need 30 bytes)
    NetworkBuffer truncBuffer;
    truncBuffer.write_u32(12345); // Only 4 bytes instead of 30

    NetInputMessage input;
    TEST_ASSERT(!input.deserializePayload(truncBuffer), "Truncated input fails gracefully");
    TEST_PASS("EdgeCase_truncated_data");
}

void test_EdgeCase_maximum_sizes() {
    // Test that max payload size is enforced
    TEST_ASSERT(getMaxPayloadSize(MessageType::Join) <= MAX_PAYLOAD_SIZE,
                "JoinMessage max size within limit");
    TEST_ASSERT(getMaxPayloadSize(MessageType::Input) <= MAX_PAYLOAD_SIZE,
                "NetInputMessage max size within limit");
    TEST_ASSERT(getMaxPayloadSize(MessageType::Chat) <= MAX_PAYLOAD_SIZE,
                "ChatMessage max size within limit");
    TEST_ASSERT(getMaxPayloadSize(MessageType::Heartbeat) <= MAX_PAYLOAD_SIZE,
                "HeartbeatMessage max size within limit");
    TEST_ASSERT(getMaxPayloadSize(MessageType::Reconnect) <= MAX_PAYLOAD_SIZE,
                "ReconnectMessage max size within limit");
    TEST_PASS("EdgeCase_maximum_sizes");
}

void test_EdgeCase_payload_size_validation() {
    TEST_ASSERT(isPayloadSizeValid(0), "Zero size is valid");
    TEST_ASSERT(isPayloadSizeValid(100), "Small size is valid");
    TEST_ASSERT(isPayloadSizeValid(MAX_PAYLOAD_SIZE), "Max size is valid");
    TEST_ASSERT(!isPayloadSizeValid(MAX_PAYLOAD_SIZE + 1), "Over max is invalid");
    TEST_PASS("EdgeCase_payload_size_validation");
}

void test_EdgeCase_max_length_player_name() {
    JoinMessage msg;
    msg.playerName = std::string(MAX_PLAYER_NAME_LENGTH, 'X');
    msg.hasSessionToken = false;

    NetworkBuffer buffer;
    msg.serializeWithEnvelope(buffer);

    buffer.reset_read();
    EnvelopeHeader header = NetworkMessage::parseEnvelope(buffer);
    TEST_ASSERT(header.isValid(), "Header valid for max-length name");

    auto msg2 = MessageFactory::create(header.type);
    TEST_ASSERT(msg2->deserializePayload(buffer), "Deserialized max-length name");

    JoinMessage* dst = dynamic_cast<JoinMessage*>(msg2.get());
    TEST_ASSERT_EQ(dst->playerName.size(), MAX_PLAYER_NAME_LENGTH, "Name has max length");
    TEST_PASS("EdgeCase_max_length_player_name");
}

void test_EdgeCase_max_length_chat_text() {
    ChatMessage msg;
    msg.senderId = 1;
    msg.text = std::string(MAX_CHAT_MESSAGE_LENGTH, 'A');
    msg.timestamp = 0;

    NetworkBuffer buffer;
    msg.serializeWithEnvelope(buffer);

    buffer.reset_read();
    EnvelopeHeader header = NetworkMessage::parseEnvelope(buffer);
    TEST_ASSERT(header.isValid(), "Header valid for max-length text");

    auto msg2 = MessageFactory::create(header.type);
    TEST_ASSERT(msg2->deserializePayload(buffer), "Deserialized max-length text");

    ChatMessage* dst = dynamic_cast<ChatMessage*>(msg2.get());
    TEST_ASSERT_EQ(dst->text.size(), MAX_CHAT_MESSAGE_LENGTH, "Text has max length");
    TEST_PASS("EdgeCase_max_length_chat_text");
}

void test_EdgeCase_corrupted_message_type() {
    NetworkBuffer buffer;

    // Write valid header but with an invalid/unregistered message type
    buffer.write_u8(PROTOCOL_VERSION);
    buffer.write_u16(12345); // Random unregistered type
    buffer.write_u16(0);     // Zero payload

    buffer.reset_read();
    EnvelopeHeader header = NetworkMessage::parseEnvelope(buffer);

    // Header parsing should succeed (valid format)
    TEST_ASSERT(header.isVersionCompatible(), "Version is compatible");

    // But factory should return nullptr
    auto msg = MessageFactory::create(header.type);
    TEST_ASSERT(msg == nullptr, "Factory returns nullptr for unknown type");
    TEST_PASS("EdgeCase_corrupted_message_type");
}

void test_EdgeCase_sequence_tracker() {
    SequenceTracker tracker;

    // Initial state
    TEST_ASSERT_EQ(tracker.currentSequence(), 0, "Initial sequence is 0");
    TEST_ASSERT_EQ(tracker.nextSequence(), 1, "First sequence is 1");
    TEST_ASSERT_EQ(tracker.nextSequence(), 2, "Second sequence is 2");

    // Record received
    bool inOrder = tracker.recordReceived(1);
    TEST_ASSERT(inOrder, "First message is in order");
    TEST_ASSERT_EQ(tracker.lastReceived(), 1, "Last received is 1");

    inOrder = tracker.recordReceived(2);
    TEST_ASSERT(inOrder, "Second message is in order");

    // Out of order (skipped 3)
    inOrder = tracker.recordReceived(4);
    TEST_ASSERT(!inOrder, "Fourth message is out of order");
    TEST_ASSERT_EQ(tracker.lastReceived(), 4, "Last received updated");

    // Reset
    tracker.reset();
    TEST_ASSERT_EQ(tracker.currentSequence(), 0, "Sequence reset");
    TEST_ASSERT_EQ(tracker.lastReceived(), 0, "Last received reset");
    TEST_PASS("EdgeCase_sequence_tracker");
}

void test_EdgeCase_buffer_state_operations() {
    NetworkBuffer buf;

    TEST_ASSERT(buf.empty(), "New buffer is empty");
    TEST_ASSERT_EQ(buf.size(), 0, "New buffer size is 0");
    TEST_ASSERT(buf.at_end(), "New buffer is at end");

    buf.write_u32(42);
    TEST_ASSERT(!buf.empty(), "Buffer not empty after write");
    TEST_ASSERT_EQ(buf.size(), 4, "Buffer size after write");
    TEST_ASSERT_EQ(buf.read_position(), 0, "Read position before read");
    TEST_ASSERT_EQ(buf.remaining(), 4, "Remaining before read");

    buf.read_u32();
    TEST_ASSERT_EQ(buf.read_position(), 4, "Read position after read");
    TEST_ASSERT_EQ(buf.remaining(), 0, "Remaining after read");
    TEST_ASSERT(buf.at_end(), "Buffer at end after read");

    buf.reset_read();
    TEST_ASSERT_EQ(buf.read_position(), 0, "Read position after reset");

    buf.clear();
    TEST_ASSERT(buf.empty(), "Buffer empty after clear");
    TEST_ASSERT_EQ(buf.size(), 0, "Buffer size 0 after clear");
    TEST_PASS("EdgeCase_buffer_state_operations");
}

// =============================================================================
// Main
// =============================================================================

int main() {
    std::cout << "=== Serialization and State Machine Unit Tests (Ticket 1-020) ===" << std::endl;

    std::cout << std::endl << "--- Section 1: NetworkBuffer Primitive Types ---" << std::endl;
    test_NetworkBuffer_u8_write_read();
    test_NetworkBuffer_u16_write_read();
    test_NetworkBuffer_u16_little_endian();
    test_NetworkBuffer_u32_write_read();
    test_NetworkBuffer_u32_little_endian();
    test_NetworkBuffer_i32_positive();
    test_NetworkBuffer_i32_negative();
    test_NetworkBuffer_i32_byte_layout();
    test_NetworkBuffer_f32_basic();
    test_NetworkBuffer_f32_edge_cases();

    std::cout << std::endl << "--- Section 2: NetworkBuffer String Encoding ---" << std::endl;
    test_NetworkBuffer_string_basic();
    test_NetworkBuffer_string_empty();
    test_NetworkBuffer_string_long();
    test_NetworkBuffer_string_with_null_byte();
    test_NetworkBuffer_string_byte_layout();

    std::cout << std::endl << "--- Section 3: NetworkBuffer Overflow Handling ---" << std::endl;
    test_NetworkBuffer_overflow_u8();
    test_NetworkBuffer_overflow_u16();
    test_NetworkBuffer_overflow_u32();
    test_NetworkBuffer_overflow_i32();
    test_NetworkBuffer_overflow_f32();
    test_NetworkBuffer_overflow_string_length();
    test_NetworkBuffer_overflow_string_content();
    test_NetworkBuffer_overflow_read_bytes();
    test_NetworkBuffer_roundtrip_mixed_types();

    std::cout << std::endl << "--- Section 4: NetworkMessage Envelope Parsing ---" << std::endl;
    test_NetworkMessage_envelope_format();
    test_NetworkMessage_envelope_parse();
    test_NetworkMessage_envelope_invalid_version();
    test_NetworkMessage_envelope_insufficient_data();
    test_NetworkMessage_envelope_truncated_payload();
    test_NetworkMessage_skip_unknown_type();

    std::cout << std::endl << "--- Section 5: Client Messages ---" << std::endl;
    test_JoinMessage_roundtrip();
    test_JoinMessage_with_token();
    test_JoinMessage_validation();
    test_NetInputMessage_roundtrip();
    test_NetInputMessage_negative_values();
    test_NetInputMessage_validation();
    test_ChatMessage_roundtrip();
    test_ChatMessage_validation();
    test_HeartbeatMessage_roundtrip();
    test_ReconnectMessage_roundtrip();
    test_ReconnectMessage_validation();

    std::cout << std::endl << "--- Section 6: Server Messages ---" << std::endl;
    test_StateUpdateMessage_empty_deltas();
    test_StateUpdateMessage_create_update_destroy();
    test_SnapshotMessages_roundtrip();
    test_PlayerListMessage_roundtrip();
    test_RejectionMessage_roundtrip();
    test_EventMessage_roundtrip();
    test_HeartbeatResponseMessage_roundtrip();
    test_ServerStatusMessage_roundtrip();
    test_JoinAcceptMessage_roundtrip();
    test_JoinRejectMessage_roundtrip();
    test_KickMessage_roundtrip();

    std::cout << std::endl << "--- Section 7: NetworkClient State Machine ---" << std::endl;
    test_NetworkClient_initial_state();
    test_NetworkClient_connect_transitions_to_connecting();
    test_NetworkClient_connect_while_connecting_fails();
    test_NetworkClient_connect_empty_address_fails();
    test_NetworkClient_connect_empty_name_fails();
    test_NetworkClient_disconnect_from_disconnected();
    test_NetworkClient_disconnect_from_connecting();
    test_NetworkClient_input_queuing_when_not_playing();
    test_NetworkClient_state_change_callback();
    test_NetworkClient_connection_stats_initial();
    test_NetworkClient_connection_config_defaults();
    test_NetworkClient_connection_state_names();
    test_NetworkClient_update_while_disconnected();
    test_NetworkClient_poll_state_update_empty();

    std::cout << std::endl << "--- Section 8: NetworkServer State Machine ---" << std::endl;
    test_NetworkServer_creation();
    test_NetworkServer_start_stop();
    test_NetworkServer_state_transitions();
    test_NetworkServer_max_players_enforcement();
    test_NetworkServer_client_count_initially_zero();
    test_NetworkServer_uptime_tracking();
    test_NetworkServer_tick_tracking();
    test_NetworkServer_state_name_helper();
    test_NetworkServer_heartbeat_constants();
    test_NetworkServer_default_config();
    test_NetworkServer_client_lookup();
    test_NetworkServer_send_to_nonexistent();
    test_NetworkServer_kick_nonexistent();
    test_NetworkServer_broadcast_no_clients();
    test_NetworkServer_double_start();
    test_NetworkServer_double_stop();
    test_NetworkServer_update_when_not_running();

    std::cout << std::endl << "--- Section 9: Edge Cases ---" << std::endl;
    test_EdgeCase_empty_buffer_deserialization();
    test_EdgeCase_truncated_data();
    test_EdgeCase_maximum_sizes();
    test_EdgeCase_payload_size_validation();
    test_EdgeCase_max_length_player_name();
    test_EdgeCase_max_length_chat_text();
    test_EdgeCase_corrupted_message_type();
    test_EdgeCase_sequence_tracker();
    test_EdgeCase_buffer_state_operations();

    std::cout << std::endl;
    std::cout << "=== Results ===" << std::endl;
    std::cout << "Passed: " << g_testsPassed << std::endl;
    std::cout << "Failed: " << g_testsFailed << std::endl;

    if (g_testsFailed > 0) {
        std::cout << std::endl << "=== TESTS FAILED ===" << std::endl;
        return 1;
    }

    std::cout << std::endl << "=== All tests passed! ===" << std::endl;
    return 0;
}
