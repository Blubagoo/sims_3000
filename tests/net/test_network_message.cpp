/**
 * @file test_network_message.cpp
 * @brief Unit tests for NetworkMessage protocol framework.
 *
 * Tests:
 * - Message envelope format (version, type, length)
 * - MessageType enum and helpers
 * - EnvelopeHeader parsing and validation
 * - MessageFactory registration and creation
 * - SequenceTracker ordering
 * - Unknown message type handling
 * - Malformed message handling
 */

#include "sims3000/net/NetworkMessage.h"
#include "sims3000/net/NetworkBuffer.h"
#include <cassert>
#include <iostream>
#include <cstring>

using namespace sims3000;

// =============================================================================
// Test Message Implementation
// =============================================================================

/**
 * @brief Test message for unit testing the message framework.
 */
class TestMessage : public NetworkMessage {
public:
    std::uint32_t testValue = 0;
    std::string testString;

    MessageType getType() const override { return MessageType::Heartbeat; }

    void serializePayload(NetworkBuffer& buffer) const override {
        buffer.write_u32(testValue);
        buffer.write_string(testString);
    }

    bool deserializePayload(NetworkBuffer& buffer) override {
        try {
            testValue = buffer.read_u32();
            testString = buffer.read_string();
            return true;
        } catch (const BufferOverflowError&) {
            return false;
        }
    }

    std::size_t getPayloadSize() const override {
        // 4 bytes for u32 + 4 bytes for string length + string content
        return 4 + 4 + testString.size();
    }
};

// Register test message with factory
static bool testMessageRegistered = MessageFactory::registerType<TestMessage>(MessageType::Heartbeat);

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

// =============================================================================
// Message Type Tests
// =============================================================================

void test_MessageType_SystemRange() {
    // System messages are 1-99
    TEST_ASSERT(isSystemMessage(MessageType::Heartbeat), "Heartbeat is system message");
    TEST_ASSERT(isSystemMessage(MessageType::Join), "Join is system message");
    TEST_ASSERT(isSystemMessage(MessageType::SnapshotEnd), "SnapshotEnd is system message");
    TEST_ASSERT(!isSystemMessage(MessageType::Invalid), "Invalid is not system message");
    TEST_ASSERT(!isSystemMessage(MessageType::Input), "Input is not system message");
    TEST_PASS("MessageType_SystemRange");
}

void test_MessageType_GameplayRange() {
    // Gameplay messages are 100-199
    TEST_ASSERT(isGameplayMessage(MessageType::Input), "Input is gameplay message");
    TEST_ASSERT(isGameplayMessage(MessageType::StateUpdate), "StateUpdate is gameplay message");
    TEST_ASSERT(isGameplayMessage(MessageType::TradeOffer), "TradeOffer is gameplay message");
    TEST_ASSERT(!isGameplayMessage(MessageType::Heartbeat), "Heartbeat is not gameplay message");
    TEST_ASSERT(!isGameplayMessage(MessageType::Invalid), "Invalid is not gameplay message");
    TEST_PASS("MessageType_GameplayRange");
}

void test_MessageType_Names() {
    TEST_ASSERT(std::strcmp(getMessageTypeName(MessageType::Heartbeat), "Heartbeat") == 0,
                "Heartbeat name");
    TEST_ASSERT(std::strcmp(getMessageTypeName(MessageType::Input), "Input") == 0,
                "Input name");
    TEST_ASSERT(std::strcmp(getMessageTypeName(MessageType::Invalid), "Invalid") == 0,
                "Invalid name");
    TEST_ASSERT(std::strcmp(getMessageTypeName(static_cast<MessageType>(9999)), "Unknown") == 0,
                "Unknown type returns Unknown");
    TEST_PASS("MessageType_Names");
}

// =============================================================================
// Envelope Format Tests
// =============================================================================

void test_Envelope_Format() {
    TestMessage msg;
    msg.testValue = 42;
    msg.testString = "hello";

    NetworkBuffer buffer;
    msg.serializeWithEnvelope(buffer);

    // Verify envelope format: [version:1][type:2][length:2][payload:N]
    TEST_ASSERT(buffer.size() >= MESSAGE_HEADER_SIZE, "Buffer has header");

    buffer.reset_read();

    // Read raw header bytes
    std::uint8_t version = buffer.read_u8();
    std::uint16_t type = buffer.read_u16();
    std::uint16_t length = buffer.read_u16();

    TEST_ASSERT(version == PROTOCOL_VERSION, "Protocol version correct");
    TEST_ASSERT(type == static_cast<std::uint16_t>(MessageType::Heartbeat), "Message type correct");
    TEST_ASSERT(length == msg.getPayloadSize(), "Payload length correct");
    TEST_ASSERT(buffer.remaining() == length, "Remaining bytes match payload length");

    TEST_PASS("Envelope_Format");
}

void test_Envelope_Parse() {
    // Create a message and serialize it
    TestMessage msg;
    msg.testValue = 12345;
    msg.testString = "test message";

    NetworkBuffer buffer;
    msg.serializeWithEnvelope(buffer);

    // Parse the envelope
    buffer.reset_read();
    EnvelopeHeader header = NetworkMessage::parseEnvelope(buffer);

    TEST_ASSERT(header.isValid(), "Header is valid");
    TEST_ASSERT(header.protocolVersion == PROTOCOL_VERSION, "Version matches");
    TEST_ASSERT(header.type == MessageType::Heartbeat, "Type matches");
    TEST_ASSERT(header.payloadLength == msg.getPayloadSize(), "Length matches");

    TEST_PASS("Envelope_Parse");
}

void test_Envelope_VersionValidation() {
    NetworkBuffer buffer;

    // Write invalid version
    buffer.write_u8(0); // Version 0 is invalid
    buffer.write_u16(static_cast<std::uint16_t>(MessageType::Heartbeat));
    buffer.write_u16(0); // No payload

    buffer.reset_read();
    EnvelopeHeader header = NetworkMessage::parseEnvelope(buffer);

    TEST_ASSERT(!header.isValid(), "Version 0 is invalid");
    TEST_ASSERT(!header.isVersionCompatible(), "Version 0 is not compatible");

    // Write future version (above current)
    buffer.clear();
    buffer.write_u8(PROTOCOL_VERSION + 1);
    buffer.write_u16(static_cast<std::uint16_t>(MessageType::Heartbeat));
    buffer.write_u16(0);

    buffer.reset_read();
    header = NetworkMessage::parseEnvelope(buffer);

    TEST_ASSERT(!header.isValid(), "Future version is invalid");

    TEST_PASS("Envelope_VersionValidation");
}

void test_Envelope_InsufficientData() {
    NetworkBuffer buffer;

    // Write partial header (only 3 bytes of 5)
    buffer.write_u8(PROTOCOL_VERSION);
    buffer.write_u16(static_cast<std::uint16_t>(MessageType::Heartbeat));
    // Missing length field

    buffer.reset_read();
    EnvelopeHeader header = NetworkMessage::parseEnvelope(buffer);

    TEST_ASSERT(!header.isValid(), "Partial header is invalid");
    TEST_ASSERT(header.type == MessageType::Invalid, "Type is Invalid on parse failure");

    TEST_PASS("Envelope_InsufficientData");
}

void test_Envelope_TruncatedPayload() {
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

    TEST_PASS("Envelope_TruncatedPayload");
}

// =============================================================================
// Message Factory Tests
// =============================================================================

void test_Factory_Registration() {
    TEST_ASSERT(MessageFactory::isRegistered(MessageType::Heartbeat), "TestMessage is registered");
    TEST_ASSERT(MessageFactory::registeredCount() >= 1, "At least one type registered");
    TEST_PASS("Factory_Registration");
}

void test_Factory_Creation() {
    auto msg = MessageFactory::create(MessageType::Heartbeat);
    TEST_ASSERT(msg != nullptr, "Created message is not null");
    TEST_ASSERT(msg->getType() == MessageType::Heartbeat, "Created message has correct type");
    TEST_PASS("Factory_Creation");
}

void test_Factory_UnknownType() {
    auto msg = MessageFactory::create(static_cast<MessageType>(9999));
    TEST_ASSERT(msg == nullptr, "Unknown type returns nullptr");
    TEST_ASSERT(!MessageFactory::isRegistered(static_cast<MessageType>(9999)), "Unknown type is not registered");
    TEST_PASS("Factory_UnknownType");
}

// =============================================================================
// Message Roundtrip Tests
// =============================================================================

void test_Message_Roundtrip() {
    // Create and populate source message
    TestMessage srcMsg;
    srcMsg.testValue = 0xDEADBEEF;
    srcMsg.testString = "Hello, Network!";
    srcMsg.setSequenceNumber(42);

    // Serialize
    NetworkBuffer buffer;
    srcMsg.serializeWithEnvelope(buffer);

    // Parse envelope
    buffer.reset_read();
    EnvelopeHeader header = NetworkMessage::parseEnvelope(buffer);
    TEST_ASSERT(header.isValid(), "Header is valid");

    // Create via factory
    auto msg = MessageFactory::create(header.type);
    TEST_ASSERT(msg != nullptr, "Created message from factory");

    // Deserialize
    bool ok = msg->deserializePayload(buffer);
    TEST_ASSERT(ok, "Deserialization succeeded");

    // Verify values
    TestMessage* dstMsg = dynamic_cast<TestMessage*>(msg.get());
    TEST_ASSERT(dstMsg != nullptr, "Cast to TestMessage succeeded");
    TEST_ASSERT(dstMsg->testValue == srcMsg.testValue, "testValue matches");
    TEST_ASSERT(dstMsg->testString == srcMsg.testString, "testString matches");

    TEST_PASS("Message_Roundtrip");
}

void test_Message_EmptyPayload() {
    // Test message with empty string
    TestMessage msg;
    msg.testValue = 0;
    msg.testString = "";

    NetworkBuffer buffer;
    msg.serializeWithEnvelope(buffer);

    buffer.reset_read();
    EnvelopeHeader header = NetworkMessage::parseEnvelope(buffer);
    TEST_ASSERT(header.isValid(), "Empty payload header is valid");
    TEST_ASSERT(header.payloadLength == 8, "Empty payload is 8 bytes (u32 + string length u32)");

    TEST_PASS("Message_EmptyPayload");
}

// =============================================================================
// Unknown Message Type Handling
// =============================================================================

void test_UnknownType_SkipPayload() {
    // Manually construct a message with unknown type
    NetworkBuffer buffer;
    buffer.write_u8(PROTOCOL_VERSION);
    buffer.write_u16(999); // Unknown type
    buffer.write_u16(8);   // 8 bytes payload

    // Write 8 bytes of payload
    buffer.write_u32(0x12345678);
    buffer.write_u32(0xABCDEF00);

    // Write a second (valid) message after it
    TestMessage secondMsg;
    secondMsg.testValue = 42;
    secondMsg.testString = "second";
    secondMsg.serializeWithEnvelope(buffer);

    // Now parse - should handle unknown type gracefully
    buffer.reset_read();

    // Parse first message header
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

    auto msg2 = MessageFactory::create(header2.type);
    TEST_ASSERT(msg2 != nullptr, "Second message created");
    TEST_ASSERT(msg2->deserializePayload(buffer), "Second message deserialized");

    TestMessage* tm = dynamic_cast<TestMessage*>(msg2.get());
    TEST_ASSERT(tm->testValue == 42, "Second message value correct");
    TEST_ASSERT(tm->testString == "second", "Second message string correct");

    TEST_PASS("UnknownType_SkipPayload");
}

// =============================================================================
// Sequence Tracker Tests
// =============================================================================

void test_SequenceTracker_NextSequence() {
    SequenceTracker tracker;

    TEST_ASSERT(tracker.currentSequence() == 0, "Initial sequence is 0");
    TEST_ASSERT(tracker.nextSequence() == 1, "First sequence is 1");
    TEST_ASSERT(tracker.nextSequence() == 2, "Second sequence is 2");
    TEST_ASSERT(tracker.currentSequence() == 2, "Current is 2");

    TEST_PASS("SequenceTracker_NextSequence");
}

void test_SequenceTracker_RecordReceived() {
    SequenceTracker tracker;

    // First message
    bool inOrder = tracker.recordReceived(1);
    TEST_ASSERT(inOrder, "First message is in order");
    TEST_ASSERT(tracker.lastReceived() == 1, "Last received is 1");

    // Second message (in order)
    inOrder = tracker.recordReceived(2);
    TEST_ASSERT(inOrder, "Second message is in order");
    TEST_ASSERT(tracker.lastReceived() == 2, "Last received is 2");

    // Out of order (skipped 3)
    inOrder = tracker.recordReceived(4);
    TEST_ASSERT(!inOrder, "Fourth message is out of order (skipped 3)");
    TEST_ASSERT(tracker.lastReceived() == 4, "Last received updated to 4");

    // Duplicate/old message
    inOrder = tracker.recordReceived(2);
    TEST_ASSERT(!inOrder, "Old message is out of order");
    TEST_ASSERT(tracker.lastReceived() == 4, "Last received unchanged");

    TEST_PASS("SequenceTracker_RecordReceived");
}

void test_SequenceTracker_Reset() {
    SequenceTracker tracker;

    tracker.nextSequence();
    tracker.nextSequence();
    tracker.recordReceived(5);

    tracker.reset();

    TEST_ASSERT(tracker.currentSequence() == 0, "Sequence reset");
    TEST_ASSERT(tracker.lastReceived() == 0, "Last received reset");

    TEST_PASS("SequenceTracker_Reset");
}

void test_SequenceTracker_IsNewer() {
    SequenceTracker tracker;

    // Nothing received yet
    TEST_ASSERT(tracker.isNewer(1), "1 is newer than nothing");
    TEST_ASSERT(tracker.isNewer(100), "100 is newer than nothing");

    tracker.recordReceived(10);

    TEST_ASSERT(tracker.isNewer(11), "11 is newer than 10");
    TEST_ASSERT(tracker.isNewer(100), "100 is newer than 10");
    TEST_ASSERT(!tracker.isNewer(10), "10 is not newer than 10");
    TEST_ASSERT(!tracker.isNewer(5), "5 is not newer than 10");

    TEST_PASS("SequenceTracker_IsNewer");
}

void test_SequenceTracker_ZeroSequence() {
    SequenceTracker tracker;

    // Sequence 0 means "no sequence" and should be accepted
    bool inOrder = tracker.recordReceived(0);
    TEST_ASSERT(inOrder, "Sequence 0 is always in order");
    TEST_ASSERT(tracker.lastReceived() == 0, "Last received unchanged for 0");

    tracker.recordReceived(5);
    inOrder = tracker.recordReceived(0);
    TEST_ASSERT(inOrder, "Sequence 0 still in order after receiving 5");
    TEST_ASSERT(tracker.lastReceived() == 5, "Last received unchanged for 0");

    TEST_PASS("SequenceTracker_ZeroSequence");
}

// =============================================================================
// Edge Case Tests
// =============================================================================

void test_EdgeCase_MaxPayloadSize() {
    // Create a message with max payload
    TestMessage msg;
    msg.testValue = 42;
    msg.testString = std::string(MAX_PAYLOAD_SIZE - 8, 'X'); // Near max size

    NetworkBuffer buffer;
    msg.serializeWithEnvelope(buffer);

    buffer.reset_read();
    EnvelopeHeader header = NetworkMessage::parseEnvelope(buffer);
    TEST_ASSERT(header.isValid(), "Max payload header is valid");
    TEST_ASSERT(header.payloadLength <= MAX_PAYLOAD_SIZE, "Payload within limit");

    TEST_PASS("EdgeCase_MaxPayloadSize");
}

void test_EdgeCase_HeaderConstants() {
    // Verify constants are correct
    TEST_ASSERT(MESSAGE_HEADER_SIZE == 5, "Header size is 5 bytes");
    TEST_ASSERT(PROTOCOL_VERSION >= 1, "Protocol version is at least 1");
    TEST_ASSERT(MIN_PROTOCOL_VERSION <= PROTOCOL_VERSION, "Min version <= current version");
    TEST_ASSERT(MAX_PAYLOAD_SIZE > 0, "Max payload size is positive");
    TEST_ASSERT(MAX_PAYLOAD_SIZE <= 65535, "Max payload size fits in u16");

    TEST_PASS("EdgeCase_HeaderConstants");
}

void test_EdgeCase_MalformedDeserialize() {
    // Try to deserialize from an empty buffer
    NetworkBuffer emptyBuffer;
    TestMessage msg;
    bool ok = msg.deserializePayload(emptyBuffer);
    TEST_ASSERT(!ok, "Deserialization fails on empty buffer");

    // Try to deserialize from truncated data
    NetworkBuffer truncBuffer;
    truncBuffer.write_u32(42);
    // Missing string data

    truncBuffer.reset_read();
    TestMessage msg2;
    ok = msg2.deserializePayload(truncBuffer);
    // This may succeed with empty string or fail depending on implementation
    // The key is that it doesn't crash
    (void)ok;

    TEST_PASS("EdgeCase_MalformedDeserialize");
}

// =============================================================================
// Main
// =============================================================================

int main() {
    std::cout << "=== NetworkMessage Protocol Tests ===" << std::endl << std::endl;

    // Message type tests
    test_MessageType_SystemRange();
    test_MessageType_GameplayRange();
    test_MessageType_Names();

    // Envelope format tests
    test_Envelope_Format();
    test_Envelope_Parse();
    test_Envelope_VersionValidation();
    test_Envelope_InsufficientData();
    test_Envelope_TruncatedPayload();

    // Factory tests
    test_Factory_Registration();
    test_Factory_Creation();
    test_Factory_UnknownType();

    // Roundtrip tests
    test_Message_Roundtrip();
    test_Message_EmptyPayload();

    // Unknown type handling
    test_UnknownType_SkipPayload();

    // Sequence tracker tests
    test_SequenceTracker_NextSequence();
    test_SequenceTracker_RecordReceived();
    test_SequenceTracker_Reset();
    test_SequenceTracker_IsNewer();
    test_SequenceTracker_ZeroSequence();

    // Edge cases
    test_EdgeCase_MaxPayloadSize();
    test_EdgeCase_HeaderConstants();
    test_EdgeCase_MalformedDeserialize();

    std::cout << std::endl;
    std::cout << "=== Results ===" << std::endl;
    std::cout << "Passed: " << testsPassed << std::endl;
    std::cout << "Failed: " << testsFailed << std::endl;

    return testsFailed == 0 ? 0 : 1;
}
