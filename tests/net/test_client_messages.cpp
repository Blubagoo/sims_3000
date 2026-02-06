/**
 * @file test_client_messages.cpp
 * @brief Unit tests for client-to-server network messages (Ticket 1-005).
 *
 * Tests:
 * - JoinMessage: player name, session token, serialization roundtrip
 * - NetInputMessage: input data, serialization roundtrip
 * - ChatMessage: text content, sender ID, timestamp
 * - HeartbeatMessage: timestamp, sequence number
 * - ReconnectMessage: session token recovery
 * - Size validation for all message types
 * - Edge cases: empty strings, max-length strings, malformed data
 */

#include "sims3000/net/ClientMessages.h"
#include "sims3000/net/NetworkMessage.h"
#include "sims3000/net/NetworkBuffer.h"
#include <cassert>
#include <iostream>
#include <cstring>
#include <random>
#include <algorithm>

using namespace sims3000;

// =============================================================================
// Test Utilities
// =============================================================================

static int testsPassed = 0;
static int testsFailed = 0;

#define TEST_ASSERT(expr, msg) \
    do { \
        if (!(expr)) { \
            std::cerr << "FAIL: " << msg << " (" << #expr << ")" << std::endl; \
            testsFailed++; \
            return; \
        } \
    } while(0)

#define TEST_PASS(name) \
    do { \
        std::cout << "PASS: " << name << std::endl; \
        testsPassed++; \
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
// JoinMessage Tests
// =============================================================================

void test_JoinMessage_BasicRoundtrip() {
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

    TEST_PASS("JoinMessage_BasicRoundtrip");
}

void test_JoinMessage_WithSessionToken() {
    JoinMessage src;
    src.playerName = "ReconnectingPlayer";
    src.hasSessionToken = true;
    src.sessionToken = generateTestToken();

    NetworkBuffer buffer;
    src.serializeWithEnvelope(buffer);

    buffer.reset_read();
    EnvelopeHeader header = NetworkMessage::parseEnvelope(buffer);
    TEST_ASSERT(header.isValid(), "Header is valid");

    auto msg = MessageFactory::create(header.type);
    TEST_ASSERT(msg->deserializePayload(buffer), "Deserialized");

    JoinMessage* dst = dynamic_cast<JoinMessage*>(msg.get());
    TEST_ASSERT(dst->playerName == "ReconnectingPlayer", "Name matches");
    TEST_ASSERT(dst->hasSessionToken, "Has token");
    TEST_ASSERT(dst->sessionToken == src.sessionToken, "Token matches");

    TEST_PASS("JoinMessage_WithSessionToken");
}

void test_JoinMessage_MaxLengthName() {
    JoinMessage src;
    src.playerName = std::string(MAX_PLAYER_NAME_LENGTH, 'X');
    src.hasSessionToken = false;

    NetworkBuffer buffer;
    src.serializeWithEnvelope(buffer);

    buffer.reset_read();
    EnvelopeHeader header = NetworkMessage::parseEnvelope(buffer);
    TEST_ASSERT(header.isValid(), "Header valid for max-length name");

    auto msg = MessageFactory::create(header.type);
    TEST_ASSERT(msg->deserializePayload(buffer), "Deserialized max-length name");

    JoinMessage* dst = dynamic_cast<JoinMessage*>(msg.get());
    TEST_ASSERT(dst->playerName.size() == MAX_PLAYER_NAME_LENGTH, "Name has max length");

    TEST_PASS("JoinMessage_MaxLengthName");
}

void test_JoinMessage_Validation() {
    JoinMessage emptyName;
    emptyName.playerName = "";
    TEST_ASSERT(!emptyName.isValid(), "Empty name is invalid");

    JoinMessage tooLongName;
    tooLongName.playerName = std::string(MAX_PLAYER_NAME_LENGTH + 10, 'X');
    TEST_ASSERT(!tooLongName.isValid(), "Oversized name is invalid");

    JoinMessage validMsg;
    validMsg.playerName = "ValidPlayer";
    TEST_ASSERT(validMsg.isValid(), "Normal name is valid");

    TEST_PASS("JoinMessage_Validation");
}

void test_JoinMessage_PayloadSize() {
    JoinMessage msg;
    msg.playerName = "Player";
    msg.hasSessionToken = false;

    std::size_t expectedSize = 4 + 6 + 1; // u32 length + "Player" + hasToken
    TEST_ASSERT(msg.getPayloadSize() == expectedSize, "Payload size without token");

    msg.hasSessionToken = true;
    msg.sessionToken = generateTestToken();
    expectedSize += SESSION_TOKEN_SIZE;
    TEST_ASSERT(msg.getPayloadSize() == expectedSize, "Payload size with token");

    TEST_PASS("JoinMessage_PayloadSize");
}

// =============================================================================
// NetInputMessage Tests
// =============================================================================

void test_NetInputMessage_BasicRoundtrip() {
    NetInputMessage src;
    src.input.tick = 12345;
    src.input.playerId = 1;
    src.input.type = InputType::PlaceBuilding;
    src.input.sequenceNum = 42;
    src.input.targetPos = {100, 200};
    src.input.param1 = 5;  // Building type
    src.input.param2 = 0;
    src.input.value = 0;

    NetworkBuffer buffer;
    src.serializeWithEnvelope(buffer);

    buffer.reset_read();
    EnvelopeHeader header = NetworkMessage::parseEnvelope(buffer);
    TEST_ASSERT(header.isValid(), "Header is valid");
    TEST_ASSERT(header.type == MessageType::Input, "Type is Input");

    auto msg = MessageFactory::create(header.type);
    TEST_ASSERT(msg != nullptr, "Created message");
    TEST_ASSERT(msg->deserializePayload(buffer), "Deserialized");

    NetInputMessage* dst = dynamic_cast<NetInputMessage*>(msg.get());
    TEST_ASSERT(dst != nullptr, "Cast succeeded");
    TEST_ASSERT(dst->input.tick == 12345, "Tick matches");
    TEST_ASSERT(dst->input.playerId == 1, "PlayerId matches");
    TEST_ASSERT(dst->input.type == InputType::PlaceBuilding, "InputType matches");
    TEST_ASSERT(dst->input.sequenceNum == 42, "SequenceNum matches");
    TEST_ASSERT(dst->input.targetPos.x == 100, "TargetPos.x matches");
    TEST_ASSERT(dst->input.targetPos.y == 200, "TargetPos.y matches");
    TEST_ASSERT(dst->input.param1 == 5, "Param1 matches");

    TEST_PASS("NetInputMessage_BasicRoundtrip");
}

void test_NetInputMessage_AllInputTypes() {
    // Test a few different input types
    InputType types[] = {
        InputType::PlaceBuilding,
        InputType::SetZone,
        InputType::PlaceRoad,
        InputType::SetTaxRate,
        InputType::PauseGame
    };

    for (InputType type : types) {
        NetInputMessage src;
        src.input.tick = 1000;
        src.input.playerId = 2;
        src.input.type = type;
        src.input.sequenceNum = 1;

        NetworkBuffer buffer;
        src.serializeWithEnvelope(buffer);
        buffer.reset_read();

        EnvelopeHeader header = NetworkMessage::parseEnvelope(buffer);
        auto msg = MessageFactory::create(header.type);
        TEST_ASSERT(msg->deserializePayload(buffer), "Deserialized input type");

        NetInputMessage* dst = dynamic_cast<NetInputMessage*>(msg.get());
        TEST_ASSERT(dst->input.type == type, "Input type preserved");
    }

    TEST_PASS("NetInputMessage_AllInputTypes");
}

void test_NetInputMessage_NegativeValues() {
    NetInputMessage src;
    src.input.tick = 0xFFFFFFFFFFFFFFFF; // Max u64
    src.input.playerId = 4;
    src.input.type = InputType::SetTaxRate;
    src.input.sequenceNum = 0xFFFFFFFF;
    src.input.targetPos = {-100, -200}; // Negative positions
    src.input.param1 = 0xFFFFFFFF;
    src.input.param2 = 0xFFFFFFFF;
    src.input.value = -12345; // Negative value

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

    TEST_PASS("NetInputMessage_NegativeValues");
}

void test_NetInputMessage_Validation() {
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

    TEST_PASS("NetInputMessage_Validation");
}

void test_NetInputMessage_FixedSize() {
    NetInputMessage msg;
    TEST_ASSERT(msg.getPayloadSize() == InputMessage::SERIALIZED_SIZE, "Payload is fixed size");
    TEST_ASSERT(msg.getPayloadSize() == 30, "Payload is 30 bytes");

    TEST_PASS("NetInputMessage_FixedSize");
}

// =============================================================================
// ChatMessage Tests
// =============================================================================

void test_ChatMessage_BasicRoundtrip() {
    ChatMessage src;
    src.senderId = 2;
    src.text = "Hello, world!";
    src.timestamp = 1234567890;

    NetworkBuffer buffer;
    src.serializeWithEnvelope(buffer);

    buffer.reset_read();
    EnvelopeHeader header = NetworkMessage::parseEnvelope(buffer);
    TEST_ASSERT(header.isValid(), "Header is valid");
    TEST_ASSERT(header.type == MessageType::Chat, "Type is Chat");

    auto msg = MessageFactory::create(header.type);
    TEST_ASSERT(msg != nullptr, "Created message");
    TEST_ASSERT(msg->deserializePayload(buffer), "Deserialized");

    ChatMessage* dst = dynamic_cast<ChatMessage*>(msg.get());
    TEST_ASSERT(dst != nullptr, "Cast succeeded");
    TEST_ASSERT(dst->senderId == 2, "SenderId matches");
    TEST_ASSERT(dst->text == "Hello, world!", "Text matches");
    TEST_ASSERT(dst->timestamp == 1234567890, "Timestamp matches");

    TEST_PASS("ChatMessage_BasicRoundtrip");
}

void test_ChatMessage_MaxLengthText() {
    ChatMessage src;
    src.senderId = 1;
    src.text = std::string(MAX_CHAT_MESSAGE_LENGTH, 'A');
    src.timestamp = 0;

    NetworkBuffer buffer;
    src.serializeWithEnvelope(buffer);

    buffer.reset_read();
    EnvelopeHeader header = NetworkMessage::parseEnvelope(buffer);
    TEST_ASSERT(header.isValid(), "Header valid for max-length text");

    auto msg = MessageFactory::create(header.type);
    TEST_ASSERT(msg->deserializePayload(buffer), "Deserialized max-length text");

    ChatMessage* dst = dynamic_cast<ChatMessage*>(msg.get());
    TEST_ASSERT(dst->text.size() == MAX_CHAT_MESSAGE_LENGTH, "Text has max length");

    TEST_PASS("ChatMessage_MaxLengthText");
}

void test_ChatMessage_UnicodeText() {
    ChatMessage src;
    src.senderId = 3;
    src.text = "Hello \xE4\xB8\x96\xE7\x95\x8C"; // "Hello 世界" in UTF-8
    src.timestamp = 9999;

    NetworkBuffer buffer;
    src.serializeWithEnvelope(buffer);

    buffer.reset_read();
    EnvelopeHeader header = NetworkMessage::parseEnvelope(buffer);
    auto msg = MessageFactory::create(header.type);
    TEST_ASSERT(msg->deserializePayload(buffer), "Deserialized Unicode text");

    ChatMessage* dst = dynamic_cast<ChatMessage*>(msg.get());
    TEST_ASSERT(dst->text == src.text, "Unicode text preserved");

    TEST_PASS("ChatMessage_UnicodeText");
}

void test_ChatMessage_Validation() {
    ChatMessage emptyText;
    emptyText.text = "";
    TEST_ASSERT(!emptyText.isValid(), "Empty text is invalid");

    ChatMessage tooLongText;
    tooLongText.text = std::string(MAX_CHAT_MESSAGE_LENGTH + 10, 'X');
    TEST_ASSERT(!tooLongText.isValid(), "Oversized text is invalid");

    ChatMessage validMsg;
    validMsg.text = "Valid message";
    TEST_ASSERT(validMsg.isValid(), "Normal text is valid");

    TEST_PASS("ChatMessage_Validation");
}

void test_ChatMessage_PayloadSize() {
    ChatMessage msg;
    msg.senderId = 1;
    msg.text = "Test";
    msg.timestamp = 0;

    // 1 (senderId) + 4 (text length) + 4 (text) + 8 (timestamp) = 17
    TEST_ASSERT(msg.getPayloadSize() == 17, "Payload size calculated correctly");

    TEST_PASS("ChatMessage_PayloadSize");
}

// =============================================================================
// HeartbeatMessage Tests
// =============================================================================

void test_HeartbeatMessage_BasicRoundtrip() {
    HeartbeatMessage src;
    src.clientTimestamp = 0x123456789ABCDEF0;
    src.clientSequence = 42;

    NetworkBuffer buffer;
    src.serializeWithEnvelope(buffer);

    buffer.reset_read();
    EnvelopeHeader header = NetworkMessage::parseEnvelope(buffer);
    TEST_ASSERT(header.isValid(), "Header is valid");
    TEST_ASSERT(header.type == MessageType::Heartbeat, "Type is Heartbeat");

    auto msg = MessageFactory::create(header.type);
    TEST_ASSERT(msg != nullptr, "Created message");
    TEST_ASSERT(msg->deserializePayload(buffer), "Deserialized");

    HeartbeatMessage* dst = dynamic_cast<HeartbeatMessage*>(msg.get());
    TEST_ASSERT(dst != nullptr, "Cast succeeded");
    TEST_ASSERT(dst->clientTimestamp == 0x123456789ABCDEF0, "Timestamp matches");
    TEST_ASSERT(dst->clientSequence == 42, "Sequence matches");

    TEST_PASS("HeartbeatMessage_BasicRoundtrip");
}

void test_HeartbeatMessage_FixedSize() {
    HeartbeatMessage msg;
    TEST_ASSERT(msg.getPayloadSize() == 12, "Payload is 12 bytes");

    NetworkBuffer buffer;
    msg.serializeWithEnvelope(buffer);

    buffer.reset_read();
    EnvelopeHeader header = NetworkMessage::parseEnvelope(buffer);
    TEST_ASSERT(header.payloadLength == 12, "Serialized payload is 12 bytes");

    TEST_PASS("HeartbeatMessage_FixedSize");
}

void test_HeartbeatMessage_MaxValues() {
    HeartbeatMessage src;
    src.clientTimestamp = 0xFFFFFFFFFFFFFFFF;
    src.clientSequence = 0xFFFFFFFF;

    NetworkBuffer buffer;
    src.serializeWithEnvelope(buffer);
    buffer.reset_read();

    EnvelopeHeader header = NetworkMessage::parseEnvelope(buffer);
    auto msg = MessageFactory::create(header.type);
    TEST_ASSERT(msg->deserializePayload(buffer), "Deserialized max values");

    HeartbeatMessage* dst = dynamic_cast<HeartbeatMessage*>(msg.get());
    TEST_ASSERT(dst->clientTimestamp == 0xFFFFFFFFFFFFFFFF, "Max timestamp preserved");
    TEST_ASSERT(dst->clientSequence == 0xFFFFFFFF, "Max sequence preserved");

    TEST_PASS("HeartbeatMessage_MaxValues");
}

// =============================================================================
// ReconnectMessage Tests
// =============================================================================

void test_ReconnectMessage_BasicRoundtrip() {
    ReconnectMessage src;
    src.sessionToken = generateTestToken();
    src.playerName = "ReconnectingPlayer";

    NetworkBuffer buffer;
    src.serializeWithEnvelope(buffer);

    buffer.reset_read();
    EnvelopeHeader header = NetworkMessage::parseEnvelope(buffer);
    TEST_ASSERT(header.isValid(), "Header is valid");
    TEST_ASSERT(header.type == MessageType::Reconnect, "Type is Reconnect");

    auto msg = MessageFactory::create(header.type);
    TEST_ASSERT(msg != nullptr, "Created message");
    TEST_ASSERT(msg->deserializePayload(buffer), "Deserialized");

    ReconnectMessage* dst = dynamic_cast<ReconnectMessage*>(msg.get());
    TEST_ASSERT(dst != nullptr, "Cast succeeded");
    TEST_ASSERT(dst->sessionToken == src.sessionToken, "Token matches");
    TEST_ASSERT(dst->playerName == "ReconnectingPlayer", "Name matches");

    TEST_PASS("ReconnectMessage_BasicRoundtrip");
}

void test_ReconnectMessage_EmptyName() {
    ReconnectMessage src;
    src.sessionToken = generateTestToken();
    src.playerName = ""; // Empty name is allowed for reconnect

    NetworkBuffer buffer;
    src.serializeWithEnvelope(buffer);
    buffer.reset_read();

    EnvelopeHeader header = NetworkMessage::parseEnvelope(buffer);
    auto msg = MessageFactory::create(header.type);
    TEST_ASSERT(msg->deserializePayload(buffer), "Deserialized empty name");

    ReconnectMessage* dst = dynamic_cast<ReconnectMessage*>(msg.get());
    TEST_ASSERT(dst->playerName.empty(), "Empty name preserved");

    TEST_PASS("ReconnectMessage_EmptyName");
}

void test_ReconnectMessage_Validation() {
    ReconnectMessage zeroToken;
    zeroToken.sessionToken = {}; // All zeros
    zeroToken.playerName = "Player";
    TEST_ASSERT(!zeroToken.isValid(), "All-zero token is invalid");

    ReconnectMessage tooLongName;
    tooLongName.sessionToken = generateTestToken();
    tooLongName.playerName = std::string(MAX_PLAYER_NAME_LENGTH + 10, 'X');
    TEST_ASSERT(!tooLongName.isValid(), "Oversized name is invalid");

    ReconnectMessage validMsg;
    validMsg.sessionToken = generateTestToken();
    validMsg.playerName = "ValidPlayer";
    TEST_ASSERT(validMsg.isValid(), "Normal reconnect is valid");

    TEST_PASS("ReconnectMessage_Validation");
}

void test_ReconnectMessage_PayloadSize() {
    ReconnectMessage msg;
    msg.sessionToken = generateTestToken();
    msg.playerName = "TestPlayer";

    // 16 (token) + 4 (name length) + 10 (name) = 30
    TEST_ASSERT(msg.getPayloadSize() == 30, "Payload size calculated correctly");

    TEST_PASS("ReconnectMessage_PayloadSize");
}

// =============================================================================
// Size Validation Tests
// =============================================================================

void test_SizeValidation_AllTypesWithinLimit() {
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

    TEST_PASS("SizeValidation_AllTypesWithinLimit");
}

void test_SizeValidation_IsPayloadSizeValid() {
    TEST_ASSERT(isPayloadSizeValid(0), "Zero size is valid");
    TEST_ASSERT(isPayloadSizeValid(100), "Small size is valid");
    TEST_ASSERT(isPayloadSizeValid(MAX_PAYLOAD_SIZE), "Max size is valid");
    TEST_ASSERT(!isPayloadSizeValid(MAX_PAYLOAD_SIZE + 1), "Over max is invalid");

    TEST_PASS("SizeValidation_IsPayloadSizeValid");
}

// =============================================================================
// Factory Registration Tests
// =============================================================================

void test_Factory_AllTypesRegistered() {
    TEST_ASSERT(MessageFactory::isRegistered(MessageType::Join), "Join registered");
    TEST_ASSERT(MessageFactory::isRegistered(MessageType::Input), "Input registered");
    TEST_ASSERT(MessageFactory::isRegistered(MessageType::Chat), "Chat registered");
    TEST_ASSERT(MessageFactory::isRegistered(MessageType::Heartbeat), "Heartbeat registered");
    TEST_ASSERT(MessageFactory::isRegistered(MessageType::Reconnect), "Reconnect registered");

    TEST_PASS("Factory_AllTypesRegistered");
}

void test_Factory_CreateCorrectTypes() {
    auto join = MessageFactory::create(MessageType::Join);
    TEST_ASSERT(join != nullptr && join->getType() == MessageType::Join, "Join type correct");

    auto input = MessageFactory::create(MessageType::Input);
    TEST_ASSERT(input != nullptr && input->getType() == MessageType::Input, "Input type correct");

    auto chat = MessageFactory::create(MessageType::Chat);
    TEST_ASSERT(chat != nullptr && chat->getType() == MessageType::Chat, "Chat type correct");

    auto heartbeat = MessageFactory::create(MessageType::Heartbeat);
    TEST_ASSERT(heartbeat != nullptr && heartbeat->getType() == MessageType::Heartbeat, "Heartbeat type correct");

    auto reconnect = MessageFactory::create(MessageType::Reconnect);
    TEST_ASSERT(reconnect != nullptr && reconnect->getType() == MessageType::Reconnect, "Reconnect type correct");

    TEST_PASS("Factory_CreateCorrectTypes");
}

// =============================================================================
// Malformed Data Tests
// =============================================================================

void test_Malformed_EmptyBuffer() {
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

    TEST_PASS("Malformed_EmptyBuffer");
}

void test_Malformed_TruncatedData() {
    // Create a buffer with partial NetInputMessage data (should need 30 bytes)
    NetworkBuffer truncBuffer;
    truncBuffer.write_u32(12345); // Only 4 bytes instead of 30

    NetInputMessage input;
    TEST_ASSERT(!input.deserializePayload(truncBuffer), "Truncated input fails gracefully");

    TEST_PASS("Malformed_TruncatedData");
}

// =============================================================================
// Byte Layout Tests
// =============================================================================

void test_ByteLayout_HeartbeatMessage() {
    HeartbeatMessage msg;
    msg.clientTimestamp = 0x0102030405060708;
    msg.clientSequence = 0x11223344;

    NetworkBuffer buffer;
    msg.serializePayload(buffer);

    // Verify exact byte layout (little-endian)
    const std::uint8_t* data = buffer.data();

    // Timestamp low bytes (0x05060708) then high bytes (0x01020304)
    TEST_ASSERT(data[0] == 0x08, "Timestamp byte 0");
    TEST_ASSERT(data[1] == 0x07, "Timestamp byte 1");
    TEST_ASSERT(data[2] == 0x06, "Timestamp byte 2");
    TEST_ASSERT(data[3] == 0x05, "Timestamp byte 3");
    TEST_ASSERT(data[4] == 0x04, "Timestamp byte 4");
    TEST_ASSERT(data[5] == 0x03, "Timestamp byte 5");
    TEST_ASSERT(data[6] == 0x02, "Timestamp byte 6");
    TEST_ASSERT(data[7] == 0x01, "Timestamp byte 7");

    // Sequence (0x11223344)
    TEST_ASSERT(data[8] == 0x44, "Sequence byte 0");
    TEST_ASSERT(data[9] == 0x33, "Sequence byte 1");
    TEST_ASSERT(data[10] == 0x22, "Sequence byte 2");
    TEST_ASSERT(data[11] == 0x11, "Sequence byte 3");

    TEST_PASS("ByteLayout_HeartbeatMessage");
}

void test_ByteLayout_JoinMessage() {
    JoinMessage msg;
    msg.playerName = "AB";
    msg.hasSessionToken = false;

    NetworkBuffer buffer;
    msg.serializePayload(buffer);

    const std::uint8_t* data = buffer.data();

    // String length (2) as u32 little-endian
    TEST_ASSERT(data[0] == 0x02, "String length byte 0");
    TEST_ASSERT(data[1] == 0x00, "String length byte 1");
    TEST_ASSERT(data[2] == 0x00, "String length byte 2");
    TEST_ASSERT(data[3] == 0x00, "String length byte 3");

    // String content "AB"
    TEST_ASSERT(data[4] == 'A', "String byte 0");
    TEST_ASSERT(data[5] == 'B', "String byte 1");

    // hasSessionToken = false (0)
    TEST_ASSERT(data[6] == 0x00, "hasSessionToken is 0");

    TEST_PASS("ByteLayout_JoinMessage");
}

// =============================================================================
// Main
// =============================================================================

int main() {
    std::cout << "=== Client Messages Tests (Ticket 1-005) ===" << std::endl << std::endl;

    // JoinMessage tests
    test_JoinMessage_BasicRoundtrip();
    test_JoinMessage_WithSessionToken();
    test_JoinMessage_MaxLengthName();
    test_JoinMessage_Validation();
    test_JoinMessage_PayloadSize();

    // NetInputMessage tests
    test_NetInputMessage_BasicRoundtrip();
    test_NetInputMessage_AllInputTypes();
    test_NetInputMessage_NegativeValues();
    test_NetInputMessage_Validation();
    test_NetInputMessage_FixedSize();

    // ChatMessage tests
    test_ChatMessage_BasicRoundtrip();
    test_ChatMessage_MaxLengthText();
    test_ChatMessage_UnicodeText();
    test_ChatMessage_Validation();
    test_ChatMessage_PayloadSize();

    // HeartbeatMessage tests
    test_HeartbeatMessage_BasicRoundtrip();
    test_HeartbeatMessage_FixedSize();
    test_HeartbeatMessage_MaxValues();

    // ReconnectMessage tests
    test_ReconnectMessage_BasicRoundtrip();
    test_ReconnectMessage_EmptyName();
    test_ReconnectMessage_Validation();
    test_ReconnectMessage_PayloadSize();

    // Size validation tests
    test_SizeValidation_AllTypesWithinLimit();
    test_SizeValidation_IsPayloadSizeValid();

    // Factory tests
    test_Factory_AllTypesRegistered();
    test_Factory_CreateCorrectTypes();

    // Malformed data tests
    test_Malformed_EmptyBuffer();
    test_Malformed_TruncatedData();

    // Byte layout tests
    test_ByteLayout_HeartbeatMessage();
    test_ByteLayout_JoinMessage();

    std::cout << std::endl;
    std::cout << "=== Results ===" << std::endl;
    std::cout << "Passed: " << testsPassed << std::endl;
    std::cout << "Failed: " << testsFailed << std::endl;

    return testsFailed == 0 ? 0 : 1;
}
