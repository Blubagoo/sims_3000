/**
 * @file test_server_messages.cpp
 * @brief Unit tests for server-to-client network messages (Ticket 1-006).
 *
 * Tests:
 * - StateUpdateMessage serialization and deserialization
 * - SnapshotStart/Chunk/End message serialization
 * - PlayerListMessage serialization and lookup
 * - RejectionMessage with reason codes
 * - EventMessage with game events
 * - HeartbeatResponseMessage for RTT measurement
 * - ServerStatusMessage with map size tiers
 * - LZ4 compression and decompression
 * - Snapshot chunking at 64KB boundaries
 */

#include "sims3000/net/ServerMessages.h"
#include "sims3000/net/NetworkBuffer.h"
#include <cassert>
#include <iostream>
#include <cstring>
#include <random>

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

// =============================================================================
// StateUpdateMessage Tests
// =============================================================================

void test_StateUpdate_EmptyDeltas() {
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

    TEST_PASS("StateUpdate_EmptyDeltas");
}

void test_StateUpdate_CreateDelta() {
    StateUpdateMessage msg;
    msg.tick = 100;

    std::vector<std::uint8_t> componentData = {0x01, 0x02, 0x03, 0x04};
    msg.addCreate(42, componentData);

    TEST_ASSERT(msg.hasDeltas(), "Has deltas");
    TEST_ASSERT(msg.deltas.size() == 1, "One delta");
    TEST_ASSERT(msg.deltas[0].entityId == 42, "Entity ID correct");
    TEST_ASSERT(msg.deltas[0].type == EntityDeltaType::Create, "Delta type is Create");

    NetworkBuffer buffer;
    msg.serializePayload(buffer);

    buffer.reset_read();
    StateUpdateMessage msg2;
    bool ok = msg2.deserializePayload(buffer);

    TEST_ASSERT(ok, "Deserialization succeeded");
    TEST_ASSERT(msg2.deltas.size() == 1, "One delta");
    TEST_ASSERT(msg2.deltas[0].entityId == 42, "Entity ID matches");
    TEST_ASSERT(msg2.deltas[0].type == EntityDeltaType::Create, "Type matches");
    TEST_ASSERT(msg2.deltas[0].componentData == componentData, "Component data matches");

    TEST_PASS("StateUpdate_CreateDelta");
}

void test_StateUpdate_UpdateDelta() {
    StateUpdateMessage msg;
    msg.tick = 200;

    std::vector<std::uint8_t> componentData = {0xAA, 0xBB, 0xCC};
    msg.addUpdate(99, componentData);

    NetworkBuffer buffer;
    msg.serializePayload(buffer);

    buffer.reset_read();
    StateUpdateMessage msg2;
    bool ok = msg2.deserializePayload(buffer);

    TEST_ASSERT(ok, "Deserialization succeeded");
    TEST_ASSERT(msg2.deltas[0].type == EntityDeltaType::Update, "Type is Update");

    TEST_PASS("StateUpdate_UpdateDelta");
}

void test_StateUpdate_DestroyDelta() {
    StateUpdateMessage msg;
    msg.tick = 300;
    msg.addDestroy(55);

    TEST_ASSERT(msg.deltas[0].componentData.empty(), "Destroy has no component data");

    NetworkBuffer buffer;
    msg.serializePayload(buffer);

    buffer.reset_read();
    StateUpdateMessage msg2;
    bool ok = msg2.deserializePayload(buffer);

    TEST_ASSERT(ok, "Deserialization succeeded");
    TEST_ASSERT(msg2.deltas[0].type == EntityDeltaType::Destroy, "Type is Destroy");
    TEST_ASSERT(msg2.deltas[0].entityId == 55, "Entity ID matches");
    TEST_ASSERT(msg2.deltas[0].componentData.empty(), "No component data");

    TEST_PASS("StateUpdate_DestroyDelta");
}

void test_StateUpdate_MultipleDeltas() {
    StateUpdateMessage msg;
    msg.tick = 400;

    msg.addCreate(1, {0x01});
    msg.addUpdate(2, {0x02, 0x03});
    msg.addDestroy(3);
    msg.addCreate(4, {0x04, 0x05, 0x06});

    TEST_ASSERT(msg.deltas.size() == 4, "Four deltas");

    NetworkBuffer buffer;
    msg.serializePayload(buffer);

    buffer.reset_read();
    StateUpdateMessage msg2;
    bool ok = msg2.deserializePayload(buffer);

    TEST_ASSERT(ok, "Deserialization succeeded");
    TEST_ASSERT(msg2.deltas.size() == 4, "Four deltas");
    TEST_ASSERT(msg2.deltas[0].type == EntityDeltaType::Create, "First is Create");
    TEST_ASSERT(msg2.deltas[1].type == EntityDeltaType::Update, "Second is Update");
    TEST_ASSERT(msg2.deltas[2].type == EntityDeltaType::Destroy, "Third is Destroy");
    TEST_ASSERT(msg2.deltas[3].type == EntityDeltaType::Create, "Fourth is Create");

    TEST_PASS("StateUpdate_MultipleDeltas");
}

void test_StateUpdate_LargeTick() {
    StateUpdateMessage msg;
    msg.tick = 0xFFFFFFFFFFFFFFFFULL; // Max uint64

    NetworkBuffer buffer;
    msg.serializePayload(buffer);

    buffer.reset_read();
    StateUpdateMessage msg2;
    bool ok = msg2.deserializePayload(buffer);

    TEST_ASSERT(ok, "Deserialization succeeded");
    TEST_ASSERT(msg2.tick == 0xFFFFFFFFFFFFFFFFULL, "Max tick value preserved");

    TEST_PASS("StateUpdate_LargeTick");
}

void test_StateUpdate_Clear() {
    StateUpdateMessage msg;
    msg.tick = 500;
    msg.addCreate(1, {0x01});
    msg.compressed = true;

    msg.clear();

    TEST_ASSERT(msg.tick == 0, "Tick reset");
    TEST_ASSERT(!msg.hasDeltas(), "No deltas");
    TEST_ASSERT(!msg.compressed, "Compressed reset");

    TEST_PASS("StateUpdate_Clear");
}

// =============================================================================
// Snapshot Message Tests
// =============================================================================

void test_SnapshotStart_Roundtrip() {
    SnapshotStartMessage msg;
    msg.tick = 9876543210ULL;
    msg.totalChunks = 15;
    msg.totalBytes = 1000000;
    msg.compressedBytes = 500000;
    msg.entityCount = 5000;

    NetworkBuffer buffer;
    msg.serializePayload(buffer);

    buffer.reset_read();
    SnapshotStartMessage msg2;
    bool ok = msg2.deserializePayload(buffer);

    TEST_ASSERT(ok, "Deserialization succeeded");
    TEST_ASSERT(msg2.tick == 9876543210ULL, "Tick matches");
    TEST_ASSERT(msg2.totalChunks == 15, "Total chunks matches");
    TEST_ASSERT(msg2.totalBytes == 1000000, "Total bytes matches");
    TEST_ASSERT(msg2.compressedBytes == 500000, "Compressed bytes matches");
    TEST_ASSERT(msg2.entityCount == 5000, "Entity count matches");

    TEST_PASS("SnapshotStart_Roundtrip");
}

void test_SnapshotChunk_Roundtrip() {
    SnapshotChunkMessage msg;
    msg.chunkIndex = 7;
    msg.data = {0x01, 0x02, 0x03, 0x04, 0x05};

    NetworkBuffer buffer;
    msg.serializePayload(buffer);

    buffer.reset_read();
    SnapshotChunkMessage msg2;
    bool ok = msg2.deserializePayload(buffer);

    TEST_ASSERT(ok, "Deserialization succeeded");
    TEST_ASSERT(msg2.chunkIndex == 7, "Chunk index matches");
    TEST_ASSERT(msg2.data.size() == 5, "Data size matches");
    TEST_ASSERT(msg2.data == msg.data, "Data content matches");

    TEST_PASS("SnapshotChunk_Roundtrip");
}

void test_SnapshotChunk_LargeData() {
    SnapshotChunkMessage msg;
    msg.chunkIndex = 0;
    msg.data.resize(SNAPSHOT_CHUNK_SIZE);

    // Fill with pattern
    for (std::size_t i = 0; i < msg.data.size(); ++i) {
        msg.data[i] = static_cast<std::uint8_t>(i & 0xFF);
    }

    NetworkBuffer buffer;
    msg.serializePayload(buffer);

    buffer.reset_read();
    SnapshotChunkMessage msg2;
    bool ok = msg2.deserializePayload(buffer);

    TEST_ASSERT(ok, "Deserialization succeeded");
    TEST_ASSERT(msg2.data.size() == SNAPSHOT_CHUNK_SIZE, "Full chunk size");
    TEST_ASSERT(msg2.data == msg.data, "Data content matches");

    TEST_PASS("SnapshotChunk_LargeData");
}

void test_SnapshotEnd_Roundtrip() {
    SnapshotEndMessage msg;
    msg.checksum = 0xDEADBEEF;

    NetworkBuffer buffer;
    msg.serializePayload(buffer);

    buffer.reset_read();
    SnapshotEndMessage msg2;
    bool ok = msg2.deserializePayload(buffer);

    TEST_ASSERT(ok, "Deserialization succeeded");
    TEST_ASSERT(msg2.checksum == 0xDEADBEEF, "Checksum matches");

    TEST_PASS("SnapshotEnd_Roundtrip");
}

// =============================================================================
// PlayerListMessage Tests
// =============================================================================

void test_PlayerList_EmptyList() {
    PlayerListMessage msg;

    NetworkBuffer buffer;
    msg.serializePayload(buffer);

    buffer.reset_read();
    PlayerListMessage msg2;
    bool ok = msg2.deserializePayload(buffer);

    TEST_ASSERT(ok, "Deserialization succeeded");
    TEST_ASSERT(msg2.players.empty(), "Empty list");

    TEST_PASS("PlayerList_EmptyList");
}

void test_PlayerList_SinglePlayer() {
    PlayerListMessage msg;
    msg.addPlayer(1, "Alice", PlayerStatus::Connected, 50);

    TEST_ASSERT(msg.players.size() == 1, "One player");

    NetworkBuffer buffer;
    msg.serializePayload(buffer);

    buffer.reset_read();
    PlayerListMessage msg2;
    bool ok = msg2.deserializePayload(buffer);

    TEST_ASSERT(ok, "Deserialization succeeded");
    TEST_ASSERT(msg2.players.size() == 1, "One player");
    TEST_ASSERT(msg2.players[0].playerId == 1, "Player ID matches");
    TEST_ASSERT(msg2.players[0].name == "Alice", "Name matches");
    TEST_ASSERT(msg2.players[0].status == PlayerStatus::Connected, "Status matches");
    TEST_ASSERT(msg2.players[0].latencyMs == 50, "Latency matches");

    TEST_PASS("PlayerList_SinglePlayer");
}

void test_PlayerList_MultiplePlayers() {
    PlayerListMessage msg;
    msg.addPlayer(1, "Alice", PlayerStatus::Connected, 30);
    msg.addPlayer(2, "Bob", PlayerStatus::Connected, 45);
    msg.addPlayer(3, "Charlie", PlayerStatus::Connecting, 0);
    msg.addPlayer(4, "Diana", PlayerStatus::Disconnected, 0);

    NetworkBuffer buffer;
    msg.serializePayload(buffer);

    buffer.reset_read();
    PlayerListMessage msg2;
    bool ok = msg2.deserializePayload(buffer);

    TEST_ASSERT(ok, "Deserialization succeeded");
    TEST_ASSERT(msg2.players.size() == 4, "Four players");
    TEST_ASSERT(msg2.players[1].name == "Bob", "Bob's name matches");
    TEST_ASSERT(msg2.players[2].status == PlayerStatus::Connecting, "Charlie's status matches");

    TEST_PASS("PlayerList_MultiplePlayers");
}

void test_PlayerList_FindPlayer() {
    PlayerListMessage msg;
    msg.addPlayer(1, "Alice", PlayerStatus::Connected, 30);
    msg.addPlayer(2, "Bob", PlayerStatus::Connected, 45);

    const PlayerInfo* alice = msg.findPlayer(1);
    TEST_ASSERT(alice != nullptr, "Found Alice");
    TEST_ASSERT(alice->name == "Alice", "Alice's name correct");

    const PlayerInfo* bob = msg.findPlayer(2);
    TEST_ASSERT(bob != nullptr, "Found Bob");
    TEST_ASSERT(bob->name == "Bob", "Bob's name correct");

    const PlayerInfo* nobody = msg.findPlayer(99);
    TEST_ASSERT(nobody == nullptr, "Unknown player returns nullptr");

    TEST_PASS("PlayerList_FindPlayer");
}

void test_PlayerList_AllStatuses() {
    PlayerListMessage msg;
    msg.addPlayer(1, "P1", PlayerStatus::Connecting, 0);
    msg.addPlayer(2, "P2", PlayerStatus::Connected, 10);
    msg.addPlayer(3, "P3", PlayerStatus::Disconnected, 0);
    msg.addPlayer(4, "P4", PlayerStatus::TimedOut, 0);

    // Add a fifth player that exceeds typical limit (test serialization handles it)
    PlayerInfo kicked;
    kicked.playerId = 5;
    kicked.name = "P5";
    kicked.status = PlayerStatus::Kicked;
    msg.players.push_back(kicked);

    NetworkBuffer buffer;
    msg.serializePayload(buffer);

    buffer.reset_read();
    PlayerListMessage msg2;
    bool ok = msg2.deserializePayload(buffer);

    TEST_ASSERT(ok, "Deserialization succeeded");
    TEST_ASSERT(msg2.players.size() == 5, "Five players");
    TEST_ASSERT(msg2.players[0].status == PlayerStatus::Connecting, "Connecting status");
    TEST_ASSERT(msg2.players[1].status == PlayerStatus::Connected, "Connected status");
    TEST_ASSERT(msg2.players[2].status == PlayerStatus::Disconnected, "Disconnected status");
    TEST_ASSERT(msg2.players[3].status == PlayerStatus::TimedOut, "TimedOut status");
    TEST_ASSERT(msg2.players[4].status == PlayerStatus::Kicked, "Kicked status");

    TEST_PASS("PlayerList_AllStatuses");
}

// =============================================================================
// RejectionMessage Tests
// =============================================================================

void test_Rejection_BasicRoundtrip() {
    RejectionMessage msg;
    msg.inputSequenceNum = 12345;
    msg.reason = RejectionReason::InsufficientFunds;
    msg.message = "Not enough credits!";

    NetworkBuffer buffer;
    msg.serializePayload(buffer);

    buffer.reset_read();
    RejectionMessage msg2;
    bool ok = msg2.deserializePayload(buffer);

    TEST_ASSERT(ok, "Deserialization succeeded");
    TEST_ASSERT(msg2.inputSequenceNum == 12345, "Sequence number matches");
    TEST_ASSERT(msg2.reason == RejectionReason::InsufficientFunds, "Reason matches");
    TEST_ASSERT(msg2.message == "Not enough credits!", "Message matches");

    TEST_PASS("Rejection_BasicRoundtrip");
}

void test_Rejection_AllReasonCodes() {
    // Test each reason code has a default message
    TEST_ASSERT(std::strlen(RejectionMessage::getDefaultMessage(RejectionReason::None)) > 0,
                "None has message");
    TEST_ASSERT(std::strlen(RejectionMessage::getDefaultMessage(RejectionReason::InsufficientFunds)) > 0,
                "InsufficientFunds has message");
    TEST_ASSERT(std::strlen(RejectionMessage::getDefaultMessage(RejectionReason::InvalidLocation)) > 0,
                "InvalidLocation has message");
    TEST_ASSERT(std::strlen(RejectionMessage::getDefaultMessage(RejectionReason::AreaOccupied)) > 0,
                "AreaOccupied has message");
    TEST_ASSERT(std::strlen(RejectionMessage::getDefaultMessage(RejectionReason::NotOwner)) > 0,
                "NotOwner has message");
    TEST_ASSERT(std::strlen(RejectionMessage::getDefaultMessage(RejectionReason::RateLimited)) > 0,
                "RateLimited has message");
    TEST_ASSERT(std::strlen(RejectionMessage::getDefaultMessage(RejectionReason::Unknown)) > 0,
                "Unknown has message");

    TEST_PASS("Rejection_AllReasonCodes");
}

void test_Rejection_EmptyMessage() {
    RejectionMessage msg;
    msg.inputSequenceNum = 1;
    msg.reason = RejectionReason::ActionNotAllowed;
    msg.message = "";

    NetworkBuffer buffer;
    msg.serializePayload(buffer);

    buffer.reset_read();
    RejectionMessage msg2;
    bool ok = msg2.deserializePayload(buffer);

    TEST_ASSERT(ok, "Deserialization succeeded");
    TEST_ASSERT(msg2.message.empty(), "Empty message preserved");

    TEST_PASS("Rejection_EmptyMessage");
}

// =============================================================================
// EventMessage Tests
// =============================================================================

void test_Event_BasicRoundtrip() {
    EventMessage msg;
    msg.tick = 5000;
    msg.eventType = GameEventType::MilestoneReached;
    msg.relatedEntity = 42;
    msg.location = {100, 200};
    msg.param1 = 10000; // e.g., population count
    msg.param2 = 1;     // e.g., milestone level
    msg.description = "Population reached 10,000!";

    NetworkBuffer buffer;
    msg.serializePayload(buffer);

    buffer.reset_read();
    EventMessage msg2;
    bool ok = msg2.deserializePayload(buffer);

    TEST_ASSERT(ok, "Deserialization succeeded");
    TEST_ASSERT(msg2.tick == 5000, "Tick matches");
    TEST_ASSERT(msg2.eventType == GameEventType::MilestoneReached, "Event type matches");
    TEST_ASSERT(msg2.relatedEntity == 42, "Entity matches");
    TEST_ASSERT(msg2.location.x == 100, "Location X matches");
    TEST_ASSERT(msg2.location.y == 200, "Location Y matches");
    TEST_ASSERT(msg2.param1 == 10000, "Param1 matches");
    TEST_ASSERT(msg2.param2 == 1, "Param2 matches");
    TEST_ASSERT(msg2.description == "Population reached 10,000!", "Description matches");

    TEST_PASS("Event_BasicRoundtrip");
}

void test_Event_DisasterType() {
    EventMessage msg;
    msg.tick = 6000;
    msg.eventType = GameEventType::DisasterStarted;
    msg.location = {50, 75};
    msg.param1 = 3; // Disaster type
    msg.description = "Meteor strike incoming!";

    NetworkBuffer buffer;
    msg.serializePayload(buffer);

    buffer.reset_read();
    EventMessage msg2;
    bool ok = msg2.deserializePayload(buffer);

    TEST_ASSERT(ok, "Deserialization succeeded");
    TEST_ASSERT(msg2.eventType == GameEventType::DisasterStarted, "Disaster type correct");

    TEST_PASS("Event_DisasterType");
}

void test_Event_AllEventTypes() {
    // Test that all event types can be serialized and deserialized
    GameEventType types[] = {
        GameEventType::None,
        GameEventType::MilestoneReached,
        GameEventType::DisasterStarted,
        GameEventType::DisasterEnded,
        GameEventType::BuildingCompleted,
        GameEventType::BudgetAlert,
        GameEventType::PopulationChange,
        GameEventType::TradeCompleted,
        GameEventType::PlayerAction
    };

    for (auto eventType : types) {
        EventMessage msg;
        msg.eventType = eventType;

        NetworkBuffer buffer;
        msg.serializePayload(buffer);

        buffer.reset_read();
        EventMessage msg2;
        bool ok = msg2.deserializePayload(buffer);

        TEST_ASSERT(ok, "Deserialization succeeded for event type");
        TEST_ASSERT(msg2.eventType == eventType, "Event type preserved");
    }

    TEST_PASS("Event_AllEventTypes");
}

// =============================================================================
// HeartbeatResponseMessage Tests
// =============================================================================

void test_HeartbeatResponse_Roundtrip() {
    HeartbeatResponseMessage msg;
    msg.clientTimestamp = 1234567890123ULL;
    msg.serverTimestamp = 1234567890200ULL;
    msg.serverTick = 50000;

    NetworkBuffer buffer;
    msg.serializePayload(buffer);

    buffer.reset_read();
    HeartbeatResponseMessage msg2;
    bool ok = msg2.deserializePayload(buffer);

    TEST_ASSERT(ok, "Deserialization succeeded");
    TEST_ASSERT(msg2.clientTimestamp == 1234567890123ULL, "Client timestamp matches");
    TEST_ASSERT(msg2.serverTimestamp == 1234567890200ULL, "Server timestamp matches");
    TEST_ASSERT(msg2.serverTick == 50000, "Server tick matches");

    TEST_PASS("HeartbeatResponse_Roundtrip");
}

void test_HeartbeatResponse_MaxValues() {
    HeartbeatResponseMessage msg;
    msg.clientTimestamp = 0xFFFFFFFFFFFFFFFFULL;
    msg.serverTimestamp = 0xFFFFFFFFFFFFFFFFULL;
    msg.serverTick = 0xFFFFFFFFFFFFFFFFULL;

    NetworkBuffer buffer;
    msg.serializePayload(buffer);

    buffer.reset_read();
    HeartbeatResponseMessage msg2;
    bool ok = msg2.deserializePayload(buffer);

    TEST_ASSERT(ok, "Deserialization succeeded");
    TEST_ASSERT(msg2.clientTimestamp == 0xFFFFFFFFFFFFFFFFULL, "Max client timestamp");
    TEST_ASSERT(msg2.serverTimestamp == 0xFFFFFFFFFFFFFFFFULL, "Max server timestamp");
    TEST_ASSERT(msg2.serverTick == 0xFFFFFFFFFFFFFFFFULL, "Max server tick");

    TEST_PASS("HeartbeatResponse_MaxValues");
}

// =============================================================================
// ServerStatusMessage Tests
// =============================================================================

void test_ServerStatus_BasicRoundtrip() {
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
    bool ok = msg2.deserializePayload(buffer);

    TEST_ASSERT(ok, "Deserialization succeeded");
    TEST_ASSERT(msg2.state == ServerState::Running, "State matches");
    TEST_ASSERT(msg2.mapSizeTier == MapSizeTier::Medium, "Map size tier matches");
    TEST_ASSERT(msg2.mapWidth == 256, "Map width matches");
    TEST_ASSERT(msg2.mapHeight == 256, "Map height matches");
    TEST_ASSERT(msg2.maxPlayers == 4, "Max players matches");
    TEST_ASSERT(msg2.currentPlayers == 2, "Current players matches");
    TEST_ASSERT(msg2.currentTick == 10000, "Current tick matches");
    TEST_ASSERT(msg2.serverName == "Test Server", "Server name matches");

    TEST_PASS("ServerStatus_BasicRoundtrip");
}

void test_ServerStatus_AllStates() {
    ServerState states[] = {
        ServerState::Loading,
        ServerState::Ready,
        ServerState::Running,
        ServerState::Paused,
        ServerState::Stopping
    };

    for (auto state : states) {
        ServerStatusMessage msg;
        msg.state = state;

        NetworkBuffer buffer;
        msg.serializePayload(buffer);

        buffer.reset_read();
        ServerStatusMessage msg2;
        bool ok = msg2.deserializePayload(buffer);

        TEST_ASSERT(ok, "Deserialization succeeded for state");
        TEST_ASSERT(msg2.state == state, "State preserved");
    }

    TEST_PASS("ServerStatus_AllStates");
}

void test_ServerStatus_MapSizeTiers() {
    // Test dimensions for each tier
    std::uint16_t width, height;

    ServerStatusMessage::getDimensionsForTier(MapSizeTier::Small, width, height);
    TEST_ASSERT(width == 128 && height == 128, "Small is 128x128");

    ServerStatusMessage::getDimensionsForTier(MapSizeTier::Medium, width, height);
    TEST_ASSERT(width == 256 && height == 256, "Medium is 256x256");

    ServerStatusMessage::getDimensionsForTier(MapSizeTier::Large, width, height);
    TEST_ASSERT(width == 512 && height == 512, "Large is 512x512");

    TEST_PASS("ServerStatus_MapSizeTiers");
}

void test_ServerStatus_LargeMap() {
    ServerStatusMessage msg;
    msg.state = ServerState::Running;
    msg.mapSizeTier = MapSizeTier::Large;
    msg.mapWidth = 512;
    msg.mapHeight = 512;
    msg.maxPlayers = 4;
    msg.currentPlayers = 4;
    msg.currentTick = 999999;
    msg.serverName = "Large Map Server";

    NetworkBuffer buffer;
    msg.serializePayload(buffer);

    buffer.reset_read();
    ServerStatusMessage msg2;
    bool ok = msg2.deserializePayload(buffer);

    TEST_ASSERT(ok, "Deserialization succeeded");
    TEST_ASSERT(msg2.mapSizeTier == MapSizeTier::Large, "Large tier");
    TEST_ASSERT(msg2.mapWidth == 512, "Width 512");
    TEST_ASSERT(msg2.mapHeight == 512, "Height 512");

    TEST_PASS("ServerStatus_LargeMap");
}

// =============================================================================
// LZ4 Compression Tests
// =============================================================================

void test_LZ4_EmptyData() {
    std::vector<std::uint8_t> input;
    std::vector<std::uint8_t> compressed;
    std::vector<std::uint8_t> decompressed;

    bool ok = compressLZ4(input, compressed);
    TEST_ASSERT(ok, "Compression of empty data succeeded");
    TEST_ASSERT(compressed.empty(), "Compressed empty data is empty");

    TEST_PASS("LZ4_EmptyData");
}

void test_LZ4_SmallData() {
    std::vector<std::uint8_t> input = {0x01, 0x02, 0x03, 0x04, 0x05};
    std::vector<std::uint8_t> compressed;
    std::vector<std::uint8_t> decompressed;

    bool ok = compressLZ4(input, compressed);
    TEST_ASSERT(ok, "Compression succeeded");
    TEST_ASSERT(!compressed.empty(), "Compressed data not empty");

    ok = decompressLZ4(compressed, decompressed);
    TEST_ASSERT(ok, "Decompression succeeded");
    TEST_ASSERT(decompressed == input, "Decompressed matches original");

    TEST_PASS("LZ4_SmallData");
}

void test_LZ4_LargeData() {
    // Create a larger buffer with repeating pattern (compresses well)
    std::vector<std::uint8_t> input(100000);
    for (std::size_t i = 0; i < input.size(); ++i) {
        input[i] = static_cast<std::uint8_t>(i % 256);
    }

    std::vector<std::uint8_t> compressed;
    std::vector<std::uint8_t> decompressed;

    bool ok = compressLZ4(input, compressed);
    TEST_ASSERT(ok, "Compression succeeded");

    // LZ4 should compress repeating patterns well
    TEST_ASSERT(compressed.size() < input.size(), "Compression reduced size");

    ok = decompressLZ4(compressed, decompressed);
    TEST_ASSERT(ok, "Decompression succeeded");
    TEST_ASSERT(decompressed == input, "Decompressed matches original");

    TEST_PASS("LZ4_LargeData");
}

void test_LZ4_RandomData() {
    // Random data (doesn't compress well)
    std::vector<std::uint8_t> input(10000);
    std::mt19937 rng(42);
    for (auto& byte : input) {
        byte = static_cast<std::uint8_t>(rng() & 0xFF);
    }

    std::vector<std::uint8_t> compressed;
    std::vector<std::uint8_t> decompressed;

    bool ok = compressLZ4(input, compressed);
    TEST_ASSERT(ok, "Compression of random data succeeded");

    ok = decompressLZ4(compressed, decompressed);
    TEST_ASSERT(ok, "Decompression succeeded");
    TEST_ASSERT(decompressed == input, "Decompressed matches original");

    TEST_PASS("LZ4_RandomData");
}

// =============================================================================
// Chunking Tests
// =============================================================================

void test_Chunking_EmptyData() {
    std::vector<std::uint8_t> data;
    auto chunks = splitIntoChunks(data);

    TEST_ASSERT(chunks.empty(), "Empty data produces no chunks");

    auto reassembled = reassembleChunks(chunks);
    TEST_ASSERT(reassembled.empty(), "Reassembled empty data is empty");

    TEST_PASS("Chunking_EmptyData");
}

void test_Chunking_SmallData() {
    std::vector<std::uint8_t> data = {0x01, 0x02, 0x03, 0x04, 0x05};
    auto chunks = splitIntoChunks(data);

    TEST_ASSERT(chunks.size() == 1, "Small data is one chunk");
    TEST_ASSERT(chunks[0] == data, "Chunk contains all data");

    auto reassembled = reassembleChunks(chunks);
    TEST_ASSERT(reassembled == data, "Reassembled matches original");

    TEST_PASS("Chunking_SmallData");
}

void test_Chunking_ExactBoundary() {
    // Create data exactly 64KB
    std::vector<std::uint8_t> data(SNAPSHOT_CHUNK_SIZE);
    for (std::size_t i = 0; i < data.size(); ++i) {
        data[i] = static_cast<std::uint8_t>(i & 0xFF);
    }

    auto chunks = splitIntoChunks(data);

    TEST_ASSERT(chunks.size() == 1, "Exactly 64KB is one chunk");
    TEST_ASSERT(chunks[0].size() == SNAPSHOT_CHUNK_SIZE, "Chunk is exactly 64KB");

    auto reassembled = reassembleChunks(chunks);
    TEST_ASSERT(reassembled == data, "Reassembled matches original");

    TEST_PASS("Chunking_ExactBoundary");
}

void test_Chunking_MultipleChunks() {
    // Create data requiring 3 chunks
    std::size_t totalSize = SNAPSHOT_CHUNK_SIZE * 2 + 1000;
    std::vector<std::uint8_t> data(totalSize);
    for (std::size_t i = 0; i < data.size(); ++i) {
        data[i] = static_cast<std::uint8_t>(i & 0xFF);
    }

    auto chunks = splitIntoChunks(data);

    TEST_ASSERT(chunks.size() == 3, "Three chunks for 2.something * 64KB");
    TEST_ASSERT(chunks[0].size() == SNAPSHOT_CHUNK_SIZE, "First chunk is 64KB");
    TEST_ASSERT(chunks[1].size() == SNAPSHOT_CHUNK_SIZE, "Second chunk is 64KB");
    TEST_ASSERT(chunks[2].size() == 1000, "Third chunk is remainder");

    auto reassembled = reassembleChunks(chunks);
    TEST_ASSERT(reassembled == data, "Reassembled matches original");

    TEST_PASS("Chunking_MultipleChunks");
}

void test_Chunking_LargeSnapshot() {
    // Simulate a realistic snapshot (1MB)
    std::size_t totalSize = 1024 * 1024;
    std::vector<std::uint8_t> data(totalSize);
    std::mt19937 rng(12345);
    for (auto& byte : data) {
        byte = static_cast<std::uint8_t>(rng() & 0xFF);
    }

    auto chunks = splitIntoChunks(data);

    std::size_t expectedChunks = (totalSize + SNAPSHOT_CHUNK_SIZE - 1) / SNAPSHOT_CHUNK_SIZE;
    TEST_ASSERT(chunks.size() == expectedChunks, "Correct number of chunks");

    auto reassembled = reassembleChunks(chunks);
    TEST_ASSERT(reassembled == data, "Reassembled matches original");

    TEST_PASS("Chunking_LargeSnapshot");
}

// =============================================================================
// Factory Registration Tests
// =============================================================================

void test_Factory_AllTypesRegistered() {
    TEST_ASSERT(MessageFactory::isRegistered(MessageType::StateUpdate), "StateUpdate registered");
    TEST_ASSERT(MessageFactory::isRegistered(MessageType::SnapshotStart), "SnapshotStart registered");
    TEST_ASSERT(MessageFactory::isRegistered(MessageType::SnapshotChunk), "SnapshotChunk registered");
    TEST_ASSERT(MessageFactory::isRegistered(MessageType::SnapshotEnd), "SnapshotEnd registered");
    TEST_ASSERT(MessageFactory::isRegistered(MessageType::PlayerList), "PlayerList registered");
    TEST_ASSERT(MessageFactory::isRegistered(MessageType::Rejection), "Rejection registered");
    TEST_ASSERT(MessageFactory::isRegistered(MessageType::Event), "Event registered");
    TEST_ASSERT(MessageFactory::isRegistered(MessageType::HeartbeatResponse), "HeartbeatResponse registered");
    TEST_ASSERT(MessageFactory::isRegistered(MessageType::ServerStatus), "ServerStatus registered");

    TEST_PASS("Factory_AllTypesRegistered");
}

void test_Factory_CreateInstances() {
    auto stateUpdate = MessageFactory::create(MessageType::StateUpdate);
    TEST_ASSERT(stateUpdate != nullptr, "Created StateUpdate");
    TEST_ASSERT(stateUpdate->getType() == MessageType::StateUpdate, "StateUpdate type correct");

    auto playerList = MessageFactory::create(MessageType::PlayerList);
    TEST_ASSERT(playerList != nullptr, "Created PlayerList");
    TEST_ASSERT(playerList->getType() == MessageType::PlayerList, "PlayerList type correct");

    auto rejection = MessageFactory::create(MessageType::Rejection);
    TEST_ASSERT(rejection != nullptr, "Created Rejection");
    TEST_ASSERT(rejection->getType() == MessageType::Rejection, "Rejection type correct");

    auto serverStatus = MessageFactory::create(MessageType::ServerStatus);
    TEST_ASSERT(serverStatus != nullptr, "Created ServerStatus");
    TEST_ASSERT(serverStatus->getType() == MessageType::ServerStatus, "ServerStatus type correct");

    TEST_PASS("Factory_CreateInstances");
}

// =============================================================================
// Envelope Roundtrip Tests
// =============================================================================

void test_Envelope_StateUpdateRoundtrip() {
    StateUpdateMessage msg;
    msg.tick = 777;
    msg.addCreate(1, {0x01, 0x02});
    msg.addDestroy(2);

    NetworkBuffer buffer;
    msg.serializeWithEnvelope(buffer);

    buffer.reset_read();
    EnvelopeHeader header = NetworkMessage::parseEnvelope(buffer);
    TEST_ASSERT(header.isValid(), "Header valid");
    TEST_ASSERT(header.type == MessageType::StateUpdate, "Type is StateUpdate");

    auto msg2 = MessageFactory::create(header.type);
    TEST_ASSERT(msg2 != nullptr, "Created message from factory");

    bool ok = msg2->deserializePayload(buffer);
    TEST_ASSERT(ok, "Deserialization succeeded");

    auto* update = dynamic_cast<StateUpdateMessage*>(msg2.get());
    TEST_ASSERT(update != nullptr, "Cast to StateUpdateMessage");
    TEST_ASSERT(update->tick == 777, "Tick matches");
    TEST_ASSERT(update->deltas.size() == 2, "Two deltas");

    TEST_PASS("Envelope_StateUpdateRoundtrip");
}

void test_Envelope_ServerStatusRoundtrip() {
    ServerStatusMessage msg;
    msg.state = ServerState::Ready;
    msg.mapSizeTier = MapSizeTier::Small;
    msg.mapWidth = 128;
    msg.mapHeight = 128;
    msg.maxPlayers = 4;
    msg.currentPlayers = 0;
    msg.currentTick = 0;
    msg.serverName = "Ready Server";

    NetworkBuffer buffer;
    msg.serializeWithEnvelope(buffer);

    buffer.reset_read();
    EnvelopeHeader header = NetworkMessage::parseEnvelope(buffer);
    TEST_ASSERT(header.isValid(), "Header valid");
    TEST_ASSERT(header.type == MessageType::ServerStatus, "Type is ServerStatus");

    auto msg2 = MessageFactory::create(header.type);
    bool ok = msg2->deserializePayload(buffer);
    TEST_ASSERT(ok, "Deserialization succeeded");

    auto* status = dynamic_cast<ServerStatusMessage*>(msg2.get());
    TEST_ASSERT(status != nullptr, "Cast to ServerStatusMessage");
    TEST_ASSERT(status->state == ServerState::Ready, "State matches");
    TEST_ASSERT(status->mapSizeTier == MapSizeTier::Small, "Map tier matches");
    TEST_ASSERT(status->serverName == "Ready Server", "Name matches");

    TEST_PASS("Envelope_ServerStatusRoundtrip");
}

// =============================================================================
// Main
// =============================================================================

int main() {
    std::cout << "=== Server Messages Tests (Ticket 1-006) ===" << std::endl << std::endl;

    // StateUpdateMessage tests
    test_StateUpdate_EmptyDeltas();
    test_StateUpdate_CreateDelta();
    test_StateUpdate_UpdateDelta();
    test_StateUpdate_DestroyDelta();
    test_StateUpdate_MultipleDeltas();
    test_StateUpdate_LargeTick();
    test_StateUpdate_Clear();

    // Snapshot message tests
    test_SnapshotStart_Roundtrip();
    test_SnapshotChunk_Roundtrip();
    test_SnapshotChunk_LargeData();
    test_SnapshotEnd_Roundtrip();

    // PlayerListMessage tests
    test_PlayerList_EmptyList();
    test_PlayerList_SinglePlayer();
    test_PlayerList_MultiplePlayers();
    test_PlayerList_FindPlayer();
    test_PlayerList_AllStatuses();

    // RejectionMessage tests
    test_Rejection_BasicRoundtrip();
    test_Rejection_AllReasonCodes();
    test_Rejection_EmptyMessage();

    // EventMessage tests
    test_Event_BasicRoundtrip();
    test_Event_DisasterType();
    test_Event_AllEventTypes();

    // HeartbeatResponseMessage tests
    test_HeartbeatResponse_Roundtrip();
    test_HeartbeatResponse_MaxValues();

    // ServerStatusMessage tests
    test_ServerStatus_BasicRoundtrip();
    test_ServerStatus_AllStates();
    test_ServerStatus_MapSizeTiers();
    test_ServerStatus_LargeMap();

    // LZ4 compression tests
    test_LZ4_EmptyData();
    test_LZ4_SmallData();
    test_LZ4_LargeData();
    test_LZ4_RandomData();

    // Chunking tests
    test_Chunking_EmptyData();
    test_Chunking_SmallData();
    test_Chunking_ExactBoundary();
    test_Chunking_MultipleChunks();
    test_Chunking_LargeSnapshot();

    // Factory tests
    test_Factory_AllTypesRegistered();
    test_Factory_CreateInstances();

    // Envelope roundtrip tests
    test_Envelope_StateUpdateRoundtrip();
    test_Envelope_ServerStatusRoundtrip();

    std::cout << std::endl;
    std::cout << "=== Results ===" << std::endl;
    std::cout << "Passed: " << testsPassed << std::endl;
    std::cout << "Failed: " << testsFailed << std::endl;

    return testsFailed == 0 ? 0 : 1;
}
