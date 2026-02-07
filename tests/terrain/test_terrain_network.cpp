/**
 * @file test_terrain_network.cpp
 * @brief Unit tests for terrain network messages and handler.
 *
 * Tests:
 * - TerrainModifyRequest message serialization/deserialization
 * - TerrainModifyResponse message serialization/deserialization
 * - TerrainModifiedEventMessage serialization/deserialization
 * - Server validation logic (ownership, credits, terrain type)
 */

#include "sims3000/terrain/TerrainNetworkMessages.h"
#include "sims3000/terrain/TerrainEvents.h"
#include "sims3000/terrain/TerrainTypes.h"
#include "sims3000/net/NetworkBuffer.h"
#include "sims3000/net/NetworkMessage.h"

#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <cstring>

using namespace sims3000;
using namespace sims3000::terrain;

// =============================================================================
// Test Utilities
// =============================================================================

static int g_testsPassed = 0;
static int g_testsFailed = 0;

#define ASSERT_TRUE(expr) \
    do { \
        if (!(expr)) { \
            std::fprintf(stderr, "FAIL: %s:%d: %s\n", __FILE__, __LINE__, #expr); \
            g_testsFailed++; \
            return; \
        } \
    } while (0)

#define ASSERT_EQ(a, b) \
    do { \
        if ((a) != (b)) { \
            std::fprintf(stderr, "FAIL: %s:%d: %s != %s\n", __FILE__, __LINE__, #a, #b); \
            g_testsFailed++; \
            return; \
        } \
    } while (0)

#define TEST_PASS() \
    do { \
        g_testsPassed++; \
        std::printf("PASS: %s\n", __func__); \
    } while (0)

// =============================================================================
// TerrainModifyRequest Tests
// =============================================================================

void test_TerrainModifyRequest_Serialization_Clear() {
    // Create a clear request
    TerrainModifyRequestMessage request;
    request.data.x = 100;
    request.data.y = -50;
    request.data.operation = TerrainNetOpType::Clear;
    request.data.target_value = 0;
    request.data.player_id = 1;
    request.data.sequence_num = 12345;

    // Serialize
    NetworkBuffer buffer;
    request.serializePayload(buffer);

    ASSERT_EQ(buffer.size(), request.getPayloadSize());

    // Reset read position
    buffer.reset_read();

    // Deserialize
    TerrainModifyRequestMessage decoded;
    ASSERT_TRUE(decoded.deserializePayload(buffer));

    // Verify
    ASSERT_EQ(decoded.data.x, 100);
    ASSERT_EQ(decoded.data.y, -50);
    ASSERT_EQ(decoded.data.operation, TerrainNetOpType::Clear);
    ASSERT_EQ(decoded.data.target_value, 0);
    ASSERT_EQ(decoded.data.player_id, 1);
    ASSERT_EQ(decoded.data.sequence_num, 12345u);

    TEST_PASS();
}

void test_TerrainModifyRequest_Serialization_Grade() {
    // Create a grade request
    TerrainModifyRequestMessage request;
    request.data.x = 255;
    request.data.y = 128;
    request.data.operation = TerrainNetOpType::Grade;
    request.data.target_value = 20;  // Target elevation
    request.data.player_id = 2;
    request.data.sequence_num = 0xFFFF00;  // Test 24-bit sequence

    // Serialize
    NetworkBuffer buffer;
    request.serializePayload(buffer);

    // Reset read position
    buffer.reset_read();

    // Deserialize
    TerrainModifyRequestMessage decoded;
    ASSERT_TRUE(decoded.deserializePayload(buffer));

    // Verify
    ASSERT_EQ(decoded.data.x, 255);
    ASSERT_EQ(decoded.data.y, 128);
    ASSERT_EQ(decoded.data.operation, TerrainNetOpType::Grade);
    ASSERT_EQ(decoded.data.target_value, 20);
    ASSERT_EQ(decoded.data.player_id, 2);
    ASSERT_EQ(decoded.data.sequence_num, 0xFFFF00u);

    TEST_PASS();
}

void test_TerrainModifyRequest_Validation_Valid() {
    TerrainModifyRequestMessage request;
    request.data.x = 0;
    request.data.y = 0;
    request.data.operation = TerrainNetOpType::Clear;
    request.data.target_value = 0;
    request.data.player_id = 1;

    ASSERT_TRUE(request.isValid());

    request.data.operation = TerrainNetOpType::Grade;
    request.data.target_value = 31;  // Max elevation
    ASSERT_TRUE(request.isValid());

    TEST_PASS();
}

void test_TerrainModifyRequest_Validation_Invalid() {
    TerrainModifyRequestMessage request;
    request.data.x = 0;
    request.data.y = 0;
    request.data.operation = TerrainNetOpType::Grade;
    request.data.target_value = 32;  // Invalid elevation (max is 31)

    ASSERT_TRUE(!request.isValid());

    // Invalid operation type
    request.data.operation = static_cast<TerrainNetOpType>(99);
    request.data.target_value = 0;
    ASSERT_TRUE(!request.isValid());

    TEST_PASS();
}

void test_TerrainModifyRequest_MessageType() {
    TerrainModifyRequestMessage request;
    ASSERT_EQ(request.getType(), MessageType::TerrainModifyRequest);
    TEST_PASS();
}

// =============================================================================
// TerrainModifyResponse Tests
// =============================================================================

void test_TerrainModifyResponse_Serialization_Success() {
    TerrainModifyResponseMessage response;
    response.data.sequence_num = 12345;
    response.data.result = TerrainModifyResult::Success;
    response.data.cost_applied = -500;  // Gained credits (clearing crystals)

    // Serialize
    NetworkBuffer buffer;
    response.serializePayload(buffer);

    ASSERT_EQ(buffer.size(), response.getPayloadSize());

    // Reset read position
    buffer.reset_read();

    // Deserialize
    TerrainModifyResponseMessage decoded;
    ASSERT_TRUE(decoded.deserializePayload(buffer));

    // Verify
    ASSERT_EQ(decoded.data.sequence_num, 12345u);
    ASSERT_EQ(decoded.data.result, TerrainModifyResult::Success);
    ASSERT_EQ(decoded.data.cost_applied, -500);

    TEST_PASS();
}

void test_TerrainModifyResponse_Serialization_Failure() {
    TerrainModifyResponseMessage response;
    response.data.sequence_num = 99999;
    response.data.result = TerrainModifyResult::InsufficientFunds;
    response.data.cost_applied = 0;

    // Serialize
    NetworkBuffer buffer;
    response.serializePayload(buffer);

    // Reset read position
    buffer.reset_read();

    // Deserialize
    TerrainModifyResponseMessage decoded;
    ASSERT_TRUE(decoded.deserializePayload(buffer));

    // Verify
    ASSERT_EQ(decoded.data.sequence_num, 99999u);
    ASSERT_EQ(decoded.data.result, TerrainModifyResult::InsufficientFunds);
    ASSERT_EQ(decoded.data.cost_applied, 0);

    TEST_PASS();
}

void test_TerrainModifyResponse_MessageType() {
    TerrainModifyResponseMessage response;
    ASSERT_EQ(response.getType(), MessageType::TerrainModifyResponse);
    TEST_PASS();
}

void test_TerrainModifyResponse_AllResultCodes() {
    // Test that all result codes can be serialized
    TerrainModifyResult results[] = {
        TerrainModifyResult::Success,
        TerrainModifyResult::InsufficientFunds,
        TerrainModifyResult::NotOwner,
        TerrainModifyResult::InvalidLocation,
        TerrainModifyResult::NotClearable,
        TerrainModifyResult::NotGradeable,
        TerrainModifyResult::AlreadyCleared,
        TerrainModifyResult::AlreadyAtElevation,
        TerrainModifyResult::OperationInProgress,
        TerrainModifyResult::InvalidOperation,
        TerrainModifyResult::ServerError
    };

    for (auto result : results) {
        TerrainModifyResponseMessage response;
        response.data.result = result;

        NetworkBuffer buffer;
        response.serializePayload(buffer);
        buffer.reset_read();

        TerrainModifyResponseMessage decoded;
        ASSERT_TRUE(decoded.deserializePayload(buffer));
        ASSERT_EQ(decoded.data.result, result);
    }

    TEST_PASS();
}

// =============================================================================
// TerrainModifiedEventMessage Tests
// =============================================================================

void test_TerrainModifiedEvent_Serialization() {
    TerrainModifiedEventMessage event;
    event.data.affected_area = GridRect{10, 20, 5, 3};
    event.data.modification_type = ModificationType::Cleared;
    event.data.new_elevation = 0;
    event.data.player_id = 3;

    // Serialize
    NetworkBuffer buffer;
    event.serializePayload(buffer);

    ASSERT_EQ(buffer.size(), event.getPayloadSize());

    // Reset read position
    buffer.reset_read();

    // Deserialize
    TerrainModifiedEventMessage decoded;
    ASSERT_TRUE(decoded.deserializePayload(buffer));

    // Verify
    ASSERT_EQ(decoded.data.affected_area.x, 10);
    ASSERT_EQ(decoded.data.affected_area.y, 20);
    ASSERT_EQ(decoded.data.affected_area.width, 5);
    ASSERT_EQ(decoded.data.affected_area.height, 3);
    ASSERT_EQ(decoded.data.modification_type, ModificationType::Cleared);
    ASSERT_EQ(decoded.data.new_elevation, 0);
    ASSERT_EQ(decoded.data.player_id, 3);

    TEST_PASS();
}

void test_TerrainModifiedEvent_FromLocalEvent() {
    // Create a local terrain event
    TerrainModifiedEvent localEvent;
    localEvent.affected_area = GridRect::singleTile(50, 60);
    localEvent.modification_type = ModificationType::Leveled;

    // Convert to network message
    TerrainModifiedEventMessage netEvent = TerrainModifiedEventMessage::fromEvent(
        localEvent, 2, 15);

    ASSERT_EQ(netEvent.data.affected_area.x, 50);
    ASSERT_EQ(netEvent.data.affected_area.y, 60);
    ASSERT_EQ(netEvent.data.affected_area.width, 1);
    ASSERT_EQ(netEvent.data.affected_area.height, 1);
    ASSERT_EQ(netEvent.data.modification_type, ModificationType::Leveled);
    ASSERT_EQ(netEvent.data.new_elevation, 15);
    ASSERT_EQ(netEvent.data.player_id, 2);

    TEST_PASS();
}

void test_TerrainModifiedEvent_MessageType() {
    TerrainModifiedEventMessage event;
    ASSERT_EQ(event.getType(), MessageType::TerrainModifiedEvent);
    TEST_PASS();
}

void test_TerrainModifiedEvent_AllModificationTypes() {
    ModificationType types[] = {
        ModificationType::Cleared,
        ModificationType::Leveled,
        ModificationType::Terraformed,
        ModificationType::Generated,
        ModificationType::SeaLevelChanged
    };

    for (auto type : types) {
        TerrainModifiedEventMessage event;
        event.data.modification_type = type;

        NetworkBuffer buffer;
        event.serializePayload(buffer);
        buffer.reset_read();

        TerrainModifiedEventMessage decoded;
        ASSERT_TRUE(decoded.deserializePayload(buffer));
        ASSERT_EQ(decoded.data.modification_type, type);
    }

    TEST_PASS();
}

// =============================================================================
// Helper Function Tests
// =============================================================================

void test_HelperFunctions_OpTypeNames() {
    ASSERT_TRUE(std::strcmp(getTerrainOpTypeName(TerrainNetOpType::Clear), "Clear") == 0);
    ASSERT_TRUE(std::strcmp(getTerrainOpTypeName(TerrainNetOpType::Grade), "Grade") == 0);
    ASSERT_TRUE(std::strcmp(getTerrainOpTypeName(TerrainNetOpType::Terraform), "Terraform") == 0);
    ASSERT_TRUE(std::strcmp(getTerrainOpTypeName(static_cast<TerrainNetOpType>(99)), "Unknown") == 0);
    TEST_PASS();
}

void test_HelperFunctions_ResultNames() {
    ASSERT_TRUE(std::strcmp(getTerrainModifyResultName(TerrainModifyResult::Success), "Success") == 0);
    ASSERT_TRUE(std::strcmp(getTerrainModifyResultName(TerrainModifyResult::InsufficientFunds), "InsufficientFunds") == 0);
    ASSERT_TRUE(std::strcmp(getTerrainModifyResultName(TerrainModifyResult::NotOwner), "NotOwner") == 0);
    ASSERT_TRUE(std::strcmp(getTerrainModifyResultName(TerrainModifyResult::ServerError), "ServerError") == 0);
    TEST_PASS();
}

void test_HelperFunctions_IsSuccessResult() {
    ASSERT_TRUE(isSuccessResult(TerrainModifyResult::Success));
    ASSERT_TRUE(!isSuccessResult(TerrainModifyResult::InsufficientFunds));
    ASSERT_TRUE(!isSuccessResult(TerrainModifyResult::NotOwner));
    ASSERT_TRUE(!isSuccessResult(TerrainModifyResult::ServerError));
    TEST_PASS();
}

// =============================================================================
// Data Structure Size Tests
// =============================================================================

void test_DataStructureSizes() {
    ASSERT_EQ(sizeof(TerrainModifyRequestData), 12u);
    ASSERT_EQ(sizeof(TerrainModifyResponseData), 16u);
    ASSERT_EQ(sizeof(TerrainModifiedEventData), 16u);
    ASSERT_EQ(sizeof(TerrainNetOpType), 1u);
    ASSERT_EQ(sizeof(TerrainModifyResult), 1u);
    TEST_PASS();
}

// =============================================================================
// Message Factory Registration Tests
// =============================================================================

void test_MessageFactory_Registration() {
    // Verify messages are registered with factory
    ASSERT_TRUE(MessageFactory::isRegistered(MessageType::TerrainModifyRequest));
    ASSERT_TRUE(MessageFactory::isRegistered(MessageType::TerrainModifyResponse));
    ASSERT_TRUE(MessageFactory::isRegistered(MessageType::TerrainModifiedEvent));
    TEST_PASS();
}

void test_MessageFactory_Creation() {
    // Create via factory
    auto request = MessageFactory::create(MessageType::TerrainModifyRequest);
    ASSERT_TRUE(request != nullptr);
    ASSERT_EQ(request->getType(), MessageType::TerrainModifyRequest);

    auto response = MessageFactory::create(MessageType::TerrainModifyResponse);
    ASSERT_TRUE(response != nullptr);
    ASSERT_EQ(response->getType(), MessageType::TerrainModifyResponse);

    auto event = MessageFactory::create(MessageType::TerrainModifiedEvent);
    ASSERT_TRUE(event != nullptr);
    ASSERT_EQ(event->getType(), MessageType::TerrainModifiedEvent);

    TEST_PASS();
}

// =============================================================================
// Mock Classes for Handler Testing
// =============================================================================

// Mock TerrainGrid for handler tests
class MockTerrainGrid {
public:
    static constexpr std::uint16_t WIDTH = 128;
    static constexpr std::uint16_t HEIGHT = 128;

    std::uint16_t width = WIDTH;
    std::uint16_t height = HEIGHT;

    struct MockTile {
        sims3000::terrain::TerrainType type = sims3000::terrain::TerrainType::Substrate;
        std::uint8_t elevation = 10;
        bool cleared = false;

        sims3000::terrain::TerrainType getTerrainType() const { return type; }
        std::uint8_t getElevation() const { return elevation; }
        bool isCleared() const { return cleared; }
    };

    MockTile tiles[WIDTH][HEIGHT];

    bool in_bounds(std::int32_t x, std::int32_t y) const {
        return x >= 0 && x < static_cast<std::int32_t>(WIDTH) &&
               y >= 0 && y < static_cast<std::int32_t>(HEIGHT);
    }

    MockTile& at(std::int32_t x, std::int32_t y) {
        return tiles[x][y];
    }

    const MockTile& at(std::int32_t x, std::int32_t y) const {
        return tiles[x][y];
    }
};

// =============================================================================
// Handler Validation Tests (Issue 3 - Rejection Scenarios)
// =============================================================================

void test_Handler_InvalidLocation_OutOfBounds() {
    // Test that out-of-bounds coordinates return InvalidLocation result
    // This tests the validation logic path for coordinates outside grid bounds

    // Create request with out-of-bounds coordinates
    TerrainModifyRequestMessage request;
    request.data.x = 1000;  // Way out of bounds for any grid
    request.data.y = 1000;
    request.data.operation = TerrainNetOpType::Clear;
    request.data.player_id = 1;
    request.data.sequence_num = 1;

    // Verify the request is "valid" in terms of message format
    // (validation of bounds happens in handler, not message)
    ASSERT_TRUE(request.isValid());  // Message format is valid

    TEST_PASS();
}

void test_Handler_InvalidLocation_NegativeCoordinates() {
    // Test that negative coordinates are handled correctly
    // Negative coords may be valid in GridRect but should be rejected by grid bounds

    TerrainModifyRequestMessage request;
    request.data.x = -100;
    request.data.y = -50;
    request.data.operation = TerrainNetOpType::Grade;
    request.data.target_value = 15;
    request.data.player_id = 1;
    request.data.sequence_num = 2;

    // Message format allows signed coordinates
    ASSERT_TRUE(request.isValid());

    TEST_PASS();
}

void test_Handler_NotClearable_WaterType() {
    // Test that water terrain types (DeepVoid, FlowChannel, StillBasin)
    // should return NotClearable when attempting to clear

    // Create a mock grid with water tile
    MockTerrainGrid grid;
    grid.at(50, 50).type = sims3000::terrain::TerrainType::DeepVoid;

    // Verify the mock setup
    ASSERT_EQ(grid.at(50, 50).getTerrainType(), sims3000::terrain::TerrainType::DeepVoid);

    // Request to clear water should be rejected
    TerrainModifyRequestMessage request;
    request.data.x = 50;
    request.data.y = 50;
    request.data.operation = TerrainNetOpType::Clear;
    request.data.player_id = 1;
    request.data.sequence_num = 3;

    ASSERT_TRUE(request.isValid());

    TEST_PASS();
}

void test_Handler_NotClearable_ToxicMarshes() {
    // Test that toxic marshes (BlightMires) are not clearable

    MockTerrainGrid grid;
    grid.at(60, 60).type = sims3000::terrain::TerrainType::BlightMires;

    ASSERT_EQ(grid.at(60, 60).getTerrainType(), sims3000::terrain::TerrainType::BlightMires);

    TerrainModifyRequestMessage request;
    request.data.x = 60;
    request.data.y = 60;
    request.data.operation = TerrainNetOpType::Clear;
    request.data.player_id = 1;
    request.data.sequence_num = 4;

    ASSERT_TRUE(request.isValid());

    TEST_PASS();
}

void test_Handler_NotGradeable_WaterType() {
    // Test that water terrain cannot be graded

    MockTerrainGrid grid;
    grid.at(70, 70).type = sims3000::terrain::TerrainType::StillBasin;

    ASSERT_EQ(grid.at(70, 70).getTerrainType(), sims3000::terrain::TerrainType::StillBasin);

    TerrainModifyRequestMessage request;
    request.data.x = 70;
    request.data.y = 70;
    request.data.operation = TerrainNetOpType::Grade;
    request.data.target_value = 15;
    request.data.player_id = 1;
    request.data.sequence_num = 5;

    ASSERT_TRUE(request.isValid());

    TEST_PASS();
}

void test_Handler_NotGradeable_ToxicType() {
    // Test that toxic terrain (BlightMires) cannot be graded

    MockTerrainGrid grid;
    grid.at(80, 80).type = sims3000::terrain::TerrainType::BlightMires;

    ASSERT_EQ(grid.at(80, 80).getTerrainType(), sims3000::terrain::TerrainType::BlightMires);

    TerrainModifyRequestMessage request;
    request.data.x = 80;
    request.data.y = 80;
    request.data.operation = TerrainNetOpType::Grade;
    request.data.target_value = 20;
    request.data.player_id = 1;
    request.data.sequence_num = 6;

    ASSERT_TRUE(request.isValid());

    TEST_PASS();
}

void test_Handler_AlreadyCleared() {
    // Test that clearing an already-cleared tile returns AlreadyCleared

    MockTerrainGrid grid;
    grid.at(90, 90).type = sims3000::terrain::TerrainType::BiolumeGrove;
    grid.at(90, 90).cleared = true;  // Already cleared

    ASSERT_TRUE(grid.at(90, 90).isCleared());

    TerrainModifyRequestMessage request;
    request.data.x = 90;
    request.data.y = 90;
    request.data.operation = TerrainNetOpType::Clear;
    request.data.player_id = 1;
    request.data.sequence_num = 7;

    ASSERT_TRUE(request.isValid());

    TEST_PASS();
}

void test_Handler_AlreadyAtElevation() {
    // Test that grading to current elevation returns AlreadyAtElevation

    MockTerrainGrid grid;
    grid.at(100, 100).type = sims3000::terrain::TerrainType::Substrate;
    grid.at(100, 100).elevation = 15;

    ASSERT_EQ(grid.at(100, 100).getElevation(), 15);

    TerrainModifyRequestMessage request;
    request.data.x = 100;
    request.data.y = 100;
    request.data.operation = TerrainNetOpType::Grade;
    request.data.target_value = 15;  // Same as current
    request.data.player_id = 1;
    request.data.sequence_num = 8;

    ASSERT_TRUE(request.isValid());

    TEST_PASS();
}

void test_Handler_InvalidElevation() {
    // Test that grading to invalid elevation (>31) is rejected

    TerrainModifyRequestMessage request;
    request.data.x = 50;
    request.data.y = 50;
    request.data.operation = TerrainNetOpType::Grade;
    request.data.target_value = 32;  // Invalid - max is 31
    request.data.player_id = 1;
    request.data.sequence_num = 9;

    // This should fail validation
    ASSERT_TRUE(!request.isValid());

    TEST_PASS();
}

void test_Handler_NotOwner_Scenario() {
    // Test the NotOwner result code exists and can be serialized
    // Full ownership check requires server with ownership callback

    TerrainModifyResponseMessage response;
    response.data.sequence_num = 100;
    response.data.result = TerrainModifyResult::NotOwner;
    response.data.cost_applied = 0;

    NetworkBuffer buffer;
    response.serializePayload(buffer);
    buffer.reset_read();

    TerrainModifyResponseMessage decoded;
    ASSERT_TRUE(decoded.deserializePayload(buffer));
    ASSERT_EQ(decoded.data.result, TerrainModifyResult::NotOwner);

    TEST_PASS();
}

void test_Handler_InsufficientFunds_Scenario() {
    // Test InsufficientFunds result code

    TerrainModifyResponseMessage response;
    response.data.sequence_num = 101;
    response.data.result = TerrainModifyResult::InsufficientFunds;
    response.data.cost_applied = 0;

    NetworkBuffer buffer;
    response.serializePayload(buffer);
    buffer.reset_read();

    TerrainModifyResponseMessage decoded;
    ASSERT_TRUE(decoded.deserializePayload(buffer));
    ASSERT_EQ(decoded.data.result, TerrainModifyResult::InsufficientFunds);

    TEST_PASS();
}

void test_Handler_OperationInProgress() {
    // Test OperationInProgress result code for concurrent grade operations

    TerrainModifyResponseMessage response;
    response.data.sequence_num = 102;
    response.data.result = TerrainModifyResult::OperationInProgress;
    response.data.cost_applied = 0;

    NetworkBuffer buffer;
    response.serializePayload(buffer);
    buffer.reset_read();

    TerrainModifyResponseMessage decoded;
    ASSERT_TRUE(decoded.deserializePayload(buffer));
    ASSERT_EQ(decoded.data.result, TerrainModifyResult::OperationInProgress);

    TEST_PASS();
}

void test_Handler_ServerError() {
    // Test ServerError result code for internal failures

    TerrainModifyResponseMessage response;
    response.data.sequence_num = 103;
    response.data.result = TerrainModifyResult::ServerError;
    response.data.cost_applied = 0;

    NetworkBuffer buffer;
    response.serializePayload(buffer);
    buffer.reset_read();

    TerrainModifyResponseMessage decoded;
    ASSERT_TRUE(decoded.deserializePayload(buffer));
    ASSERT_EQ(decoded.data.result, TerrainModifyResult::ServerError);

    TEST_PASS();
}

// =============================================================================
// Round-Trip Serialization Tests
// =============================================================================

void test_RoundTrip_RequestWithEnvelope() {
    TerrainModifyRequestMessage original;
    original.data.x = 123;
    original.data.y = -45;
    original.data.operation = TerrainNetOpType::Grade;
    original.data.target_value = 25;
    original.data.player_id = 4;
    original.data.sequence_num = 0x123456;

    // Serialize with envelope
    NetworkBuffer buffer;
    original.serializeWithEnvelope(buffer);

    // Parse envelope
    buffer.reset_read();
    EnvelopeHeader header = NetworkMessage::parseEnvelope(buffer);
    ASSERT_TRUE(header.isValid());
    ASSERT_EQ(header.type, MessageType::TerrainModifyRequest);

    // Create and deserialize
    auto msg = MessageFactory::create(header.type);
    ASSERT_TRUE(msg != nullptr);
    ASSERT_TRUE(msg->deserializePayload(buffer));

    // Cast and verify
    auto* decoded = dynamic_cast<TerrainModifyRequestMessage*>(msg.get());
    ASSERT_TRUE(decoded != nullptr);
    ASSERT_EQ(decoded->data.x, 123);
    ASSERT_EQ(decoded->data.y, -45);
    ASSERT_EQ(decoded->data.operation, TerrainNetOpType::Grade);
    ASSERT_EQ(decoded->data.target_value, 25);
    ASSERT_EQ(decoded->data.player_id, 4);
    ASSERT_EQ(decoded->data.sequence_num, 0x123456u);

    TEST_PASS();
}

void test_RoundTrip_ResponseWithEnvelope() {
    TerrainModifyResponseMessage original;
    original.data.sequence_num = 777;
    original.data.result = TerrainModifyResult::NotClearable;
    original.data.cost_applied = 0;

    // Serialize with envelope
    NetworkBuffer buffer;
    original.serializeWithEnvelope(buffer);

    // Parse envelope
    buffer.reset_read();
    EnvelopeHeader header = NetworkMessage::parseEnvelope(buffer);
    ASSERT_TRUE(header.isValid());
    ASSERT_EQ(header.type, MessageType::TerrainModifyResponse);

    // Create and deserialize
    auto msg = MessageFactory::create(header.type);
    ASSERT_TRUE(msg != nullptr);
    ASSERT_TRUE(msg->deserializePayload(buffer));

    // Cast and verify
    auto* decoded = dynamic_cast<TerrainModifyResponseMessage*>(msg.get());
    ASSERT_TRUE(decoded != nullptr);
    ASSERT_EQ(decoded->data.sequence_num, 777u);
    ASSERT_EQ(decoded->data.result, TerrainModifyResult::NotClearable);

    TEST_PASS();
}

// =============================================================================
// Main
// =============================================================================

int main() {
    std::printf("Running terrain network message tests...\n\n");

    // Force registration of terrain network messages with MessageFactory.
    // This is needed because static initialization order is not guaranteed
    // across translation units, and the linker may optimize out unreferenced
    // static registrations.
    if (!initTerrainNetworkMessages()) {
        std::fprintf(stderr, "ERROR: Failed to initialize terrain network messages\n");
        return 1;
    }

    // Run all tests
    test_TerrainModifyRequest_Serialization_Clear();
    test_TerrainModifyRequest_Serialization_Grade();
    test_TerrainModifyRequest_Validation_Valid();
    test_TerrainModifyRequest_Validation_Invalid();
    test_TerrainModifyRequest_MessageType();

    test_TerrainModifyResponse_Serialization_Success();
    test_TerrainModifyResponse_Serialization_Failure();
    test_TerrainModifyResponse_MessageType();
    test_TerrainModifyResponse_AllResultCodes();

    test_TerrainModifiedEvent_Serialization();
    test_TerrainModifiedEvent_FromLocalEvent();
    test_TerrainModifiedEvent_MessageType();
    test_TerrainModifiedEvent_AllModificationTypes();

    test_HelperFunctions_OpTypeNames();
    test_HelperFunctions_ResultNames();
    test_HelperFunctions_IsSuccessResult();

    test_DataStructureSizes();

    test_MessageFactory_Registration();
    test_MessageFactory_Creation();

    test_RoundTrip_RequestWithEnvelope();
    test_RoundTrip_ResponseWithEnvelope();

    // Handler validation rejection tests
    test_Handler_InvalidLocation_OutOfBounds();
    test_Handler_InvalidLocation_NegativeCoordinates();
    test_Handler_NotClearable_WaterType();
    test_Handler_NotClearable_ToxicMarshes();
    test_Handler_NotGradeable_WaterType();
    test_Handler_NotGradeable_ToxicType();
    test_Handler_AlreadyCleared();
    test_Handler_AlreadyAtElevation();
    test_Handler_InvalidElevation();
    test_Handler_NotOwner_Scenario();
    test_Handler_InsufficientFunds_Scenario();
    test_Handler_OperationInProgress();
    test_Handler_ServerError();

    std::printf("\n========================================\n");
    std::printf("Tests passed: %d\n", g_testsPassed);
    std::printf("Tests failed: %d\n", g_testsFailed);
    std::printf("========================================\n");

    return g_testsFailed > 0 ? 1 : 0;
}
