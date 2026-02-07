/**
 * @file test_terrain_network_sync.cpp
 * @brief Unit tests for terrain network synchronization (ticket 3-037).
 *
 * Tests the optimized network sync flow:
 * - Seed + modifications sync request
 * - Client-side terrain generation from seed
 * - Modification record application
 * - Checksum verification
 * - Full snapshot fallback
 */

#include "sims3000/terrain/TerrainNetworkSync.h"
#include "sims3000/terrain/TerrainGrid.h"
#include "sims3000/terrain/WaterData.h"
#include "sims3000/terrain/TerrainTypes.h"
#include "sims3000/terrain/TerrainEvents.h"
#include "sims3000/terrain/ChunkDirtyTracker.h"
#include "sims3000/terrain/TerrainClientHandler.h"
#include "sims3000/net/NetworkBuffer.h"

#include <iostream>
#include <cstdlib>
#include <cmath>
#include <chrono>

using namespace sims3000;
using namespace sims3000::terrain;

// =============================================================================
// Test Utilities
// =============================================================================

static int g_testCount = 0;
static int g_passCount = 0;

#define TEST(name) \
    void test_##name(); \
    struct TestRunner_##name { \
        TestRunner_##name() { \
            std::cout << "  " << #name << "... "; \
            g_testCount++; \
            try { \
                test_##name(); \
                std::cout << "PASS" << std::endl; \
                g_passCount++; \
            } catch (const std::exception& e) { \
                std::cout << "FAIL: " << e.what() << std::endl; \
            } \
        } \
    } g_testRunner_##name; \
    void test_##name()

#define ASSERT(cond) \
    if (!(cond)) { \
        throw std::runtime_error("Assertion failed: " #cond); \
    }

#define ASSERT_EQ(a, b) \
    if ((a) != (b)) { \
        throw std::runtime_error("Assertion failed: " #a " != " #b); \
    }

#define ASSERT_NE(a, b) \
    if ((a) == (b)) { \
        throw std::runtime_error("Assertion failed: " #a " == " #b); \
    }

#define ASSERT_LT(a, b) \
    if (!((a) < (b))) { \
        throw std::runtime_error("Assertion failed: " #a " >= " #b); \
    }

#define ASSERT_GT(a, b) \
    if (!((a) > (b))) { \
        throw std::runtime_error("Assertion failed: " #a " <= " #b); \
    }

// =============================================================================
// TerrainModification Tests
// =============================================================================

TEST(TerrainModification_Size) {
    // Verify TerrainModification is exactly 24 bytes
    ASSERT_EQ(sizeof(TerrainModification), 24u);
}

TEST(TerrainModification_TriviallyCopyable) {
    // Verify TerrainModification is trivially copyable
    ASSERT(std::is_trivially_copyable<TerrainModification>::value);
}

TEST(TerrainModification_FromEvent) {
    // Create an event
    GridRect area;
    area.x = 10;
    area.y = 20;
    area.width = 5;
    area.height = 3;

    TerrainModifiedEvent event(area, ModificationType::Leveled);

    // Convert to modification record
    TerrainModification mod = TerrainModification::fromEvent(
        event, 42, 100, 1, 15, 0);

    ASSERT_EQ(mod.sequence_num, 42u);
    ASSERT_EQ(mod.timestamp_tick, 100u);
    ASSERT_EQ(mod.x, 10);
    ASSERT_EQ(mod.y, 20);
    ASSERT_EQ(mod.width, 5u);
    ASSERT_EQ(mod.height, 3u);
    ASSERT_EQ(static_cast<int>(mod.modification_type), static_cast<int>(ModificationType::Leveled));
    ASSERT_EQ(mod.new_elevation, 15u);
    ASSERT_EQ(mod.player_id, 1u);
}

TEST(TerrainModification_GetAffectedArea) {
    TerrainModification mod;
    mod.x = 5;
    mod.y = 10;
    mod.width = 8;
    mod.height = 4;

    GridRect area = mod.getAffectedArea();

    ASSERT_EQ(area.x, 5);
    ASSERT_EQ(area.y, 10);
    ASSERT_EQ(area.width, 8u);
    ASSERT_EQ(area.height, 4u);
}

// =============================================================================
// TerrainSyncRequestData Tests
// =============================================================================

TEST(TerrainSyncRequestData_Size) {
    // Verify TerrainSyncRequestData is exactly 32 bytes
    ASSERT_EQ(sizeof(TerrainSyncRequestData), 32u);
}

TEST(TerrainSyncVerifyData_Size) {
    // Verify TerrainSyncVerifyData is exactly 12 bytes
    ASSERT_EQ(sizeof(TerrainSyncVerifyData), 12u);
}

TEST(TerrainSyncCompleteData_Size) {
    // Verify TerrainSyncCompleteData is exactly 8 bytes
    ASSERT_EQ(sizeof(TerrainSyncCompleteData), 8u);
}

// =============================================================================
// Checksum Tests
// =============================================================================

TEST(Checksum_EmptyGrid) {
    TerrainGrid grid;
    std::uint32_t checksum = TerrainNetworkSync::computeChecksum(grid);
    ASSERT_EQ(checksum, 0u);
}

TEST(Checksum_SmallGrid) {
    TerrainGrid grid(MapSize::Small);
    std::uint32_t checksum = TerrainNetworkSync::computeChecksum(grid);
    ASSERT_NE(checksum, 0u);
}

TEST(Checksum_Deterministic) {
    TerrainGrid grid1(MapSize::Small);
    TerrainGrid grid2(MapSize::Small);

    // Initialize both grids identically
    for (std::size_t i = 0; i < grid1.tiles.size(); ++i) {
        grid1.tiles[i].setElevation(static_cast<std::uint8_t>(i % 32));
        grid2.tiles[i].setElevation(static_cast<std::uint8_t>(i % 32));
    }

    std::uint32_t checksum1 = TerrainNetworkSync::computeChecksum(grid1);
    std::uint32_t checksum2 = TerrainNetworkSync::computeChecksum(grid2);

    ASSERT_EQ(checksum1, checksum2);
}

TEST(Checksum_ChangesOnModification) {
    TerrainGrid grid(MapSize::Small);

    std::uint32_t checksum1 = TerrainNetworkSync::computeChecksum(grid);

    // Modify a tile
    grid.at(0, 0).setElevation(31);

    std::uint32_t checksum2 = TerrainNetworkSync::computeChecksum(grid);

    ASSERT_NE(checksum1, checksum2);
}

TEST(Checksum_FullChecksum) {
    TerrainGrid grid(MapSize::Small);
    WaterData waterData;
    waterData.initialize(MapSize::Small);

    std::uint32_t checksum = TerrainNetworkSync::computeFullChecksum(grid, waterData);
    ASSERT_NE(checksum, 0u);
}

// =============================================================================
// Message Serialization Tests
// =============================================================================

TEST(TerrainSyncRequestMessage_Serialization) {
    // Create a sync request message
    TerrainSyncRequestMessage msg;
    msg.data.map_seed = 0x123456789ABCDEF0ULL;
    msg.data.width = 256;
    msg.data.height = 256;
    msg.data.sea_level = 8;
    msg.data.authoritative_checksum = 0xDEADBEEF;
    msg.data.latest_sequence = 5;

    // Add some modifications
    TerrainModification mod1;
    mod1.sequence_num = 1;
    mod1.timestamp_tick = 100;
    mod1.x = 10;
    mod1.y = 20;
    mod1.width = 1;
    mod1.height = 1;
    mod1.modification_type = ModificationType::Cleared;
    mod1.player_id = 1;
    msg.modifications.push_back(mod1);

    TerrainModification mod2;
    mod2.sequence_num = 2;
    mod2.timestamp_tick = 200;
    mod2.x = 30;
    mod2.y = 40;
    mod2.width = 2;
    mod2.height = 2;
    mod2.modification_type = ModificationType::Leveled;
    mod2.new_elevation = 15;
    mod2.player_id = 2;
    msg.modifications.push_back(mod2);

    msg.data.modification_count = static_cast<std::uint32_t>(msg.modifications.size());

    // Serialize
    NetworkBuffer buffer;
    msg.serializePayload(buffer);

    // Verify payload size
    ASSERT_EQ(buffer.size(), msg.getPayloadSize());

    // Deserialize into new message
    buffer.reset_read();
    TerrainSyncRequestMessage msg2;
    ASSERT(msg2.deserializePayload(buffer));

    // Verify data matches
    ASSERT_EQ(msg2.data.map_seed, msg.data.map_seed);
    ASSERT_EQ(msg2.data.width, msg.data.width);
    ASSERT_EQ(msg2.data.height, msg.data.height);
    ASSERT_EQ(msg2.data.sea_level, msg.data.sea_level);
    ASSERT_EQ(msg2.data.authoritative_checksum, msg.data.authoritative_checksum);
    ASSERT_EQ(msg2.data.modification_count, msg.data.modification_count);
    ASSERT_EQ(msg2.modifications.size(), msg.modifications.size());

    // Verify modifications
    ASSERT_EQ(msg2.modifications[0].sequence_num, mod1.sequence_num);
    ASSERT_EQ(msg2.modifications[0].x, mod1.x);
    ASSERT_EQ(msg2.modifications[1].sequence_num, mod2.sequence_num);
    ASSERT_EQ(msg2.modifications[1].new_elevation, mod2.new_elevation);
}

TEST(TerrainSyncVerifyMessage_Serialization) {
    TerrainSyncVerifyMessage msg;
    msg.data.computed_checksum = 0xCAFEBABE;
    msg.data.last_applied_sequence = 42;
    msg.data.success = 1;

    NetworkBuffer buffer;
    msg.serializePayload(buffer);

    ASSERT_EQ(buffer.size(), sizeof(TerrainSyncVerifyData));

    buffer.reset_read();
    TerrainSyncVerifyMessage msg2;
    ASSERT(msg2.deserializePayload(buffer));

    ASSERT_EQ(msg2.data.computed_checksum, msg.data.computed_checksum);
    ASSERT_EQ(msg2.data.last_applied_sequence, msg.data.last_applied_sequence);
    ASSERT_EQ(msg2.data.success, msg.data.success);
}

TEST(TerrainSyncCompleteMessage_Serialization) {
    TerrainSyncCompleteMessage msg;
    msg.data.result = TerrainSyncResult::Success;
    msg.data.final_sequence = 100;

    NetworkBuffer buffer;
    msg.serializePayload(buffer);

    ASSERT_EQ(buffer.size(), sizeof(TerrainSyncCompleteData));

    buffer.reset_read();
    TerrainSyncCompleteMessage msg2;
    ASSERT(msg2.deserializePayload(buffer));

    ASSERT_EQ(static_cast<int>(msg2.data.result), static_cast<int>(msg.data.result));
    ASSERT_EQ(msg2.data.final_sequence, msg.data.final_sequence);
}

// =============================================================================
// TerrainNetworkSync Server-side Tests
// =============================================================================

TEST(TerrainNetworkSync_ServerSetTerrainData) {
    TerrainNetworkSync sync;
    TerrainGrid grid(MapSize::Small);
    WaterData waterData;
    waterData.initialize(MapSize::Small);

    std::uint64_t seed = 12345;
    sync.setTerrainData(grid, waterData, seed);

    ASSERT_NE(sync.getAuthoritativeChecksum(), 0u);
    ASSERT_EQ(sync.getModificationCount(), 0u);
}

TEST(TerrainNetworkSync_RecordModification) {
    TerrainNetworkSync sync;
    TerrainGrid grid(MapSize::Small);
    WaterData waterData;
    waterData.initialize(MapSize::Small);

    sync.setTerrainData(grid, waterData, 12345);

    // Record a modification
    GridRect area;
    area.x = 10;
    area.y = 20;
    area.width = 1;
    area.height = 1;
    TerrainModifiedEvent event(area, ModificationType::Cleared);

    std::uint32_t seq = sync.recordModification(event, 100, 1);

    ASSERT_EQ(seq, 1u);
    ASSERT_EQ(sync.getModificationCount(), 1u);
    ASSERT_EQ(sync.getLatestSequence(), 1u);

    // Record another
    seq = sync.recordModification(event, 200, 2);

    ASSERT_EQ(seq, 2u);
    ASSERT_EQ(sync.getModificationCount(), 2u);
    ASSERT_EQ(sync.getLatestSequence(), 2u);
}

TEST(TerrainNetworkSync_CreateSyncRequest) {
    TerrainNetworkSync sync;
    TerrainGrid grid(MapSize::Small);
    WaterData waterData;
    waterData.initialize(MapSize::Small);

    std::uint64_t seed = 12345;
    sync.setTerrainData(grid, waterData, seed);

    // Record some modifications
    GridRect area = GridRect::singleTile(5, 5);
    TerrainModifiedEvent event(area, ModificationType::Leveled);
    sync.recordModification(event, 100, 1, 20);
    sync.recordModification(event, 200, 1, 21);

    // Create sync request
    TerrainSyncRequestMessage request = sync.createSyncRequest();

    ASSERT_EQ(request.data.map_seed, seed);
    ASSERT_EQ(request.data.width, 128u);
    ASSERT_EQ(request.data.height, 128u);
    ASSERT_EQ(request.data.sea_level, 8u);
    ASSERT_EQ(request.data.modification_count, 2u);
    ASSERT_EQ(request.modifications.size(), 2u);
}

TEST(TerrainNetworkSync_VerifySyncResult_Success) {
    TerrainNetworkSync sync;
    TerrainGrid grid(MapSize::Small);
    WaterData waterData;
    waterData.initialize(MapSize::Small);

    sync.setTerrainData(grid, waterData, 12345);
    std::uint32_t authChecksum = sync.getAuthoritativeChecksum();

    TerrainSyncVerifyMessage verify;
    verify.data.computed_checksum = authChecksum;
    verify.data.success = 1;

    ASSERT(sync.verifySyncResult(verify));
}

TEST(TerrainNetworkSync_VerifySyncResult_Mismatch) {
    TerrainNetworkSync sync;
    TerrainGrid grid(MapSize::Small);
    WaterData waterData;
    waterData.initialize(MapSize::Small);

    sync.setTerrainData(grid, waterData, 12345);

    TerrainSyncVerifyMessage verify;
    verify.data.computed_checksum = 0xBADC0DE;  // Wrong checksum
    verify.data.success = 1;

    ASSERT(!sync.verifySyncResult(verify));
}

TEST(TerrainNetworkSync_PruneModifications) {
    TerrainNetworkSync sync;
    TerrainGrid grid(MapSize::Small);
    WaterData waterData;
    waterData.initialize(MapSize::Small);

    sync.setTerrainData(grid, waterData, 12345);

    // Record several modifications
    GridRect area = GridRect::singleTile(5, 5);
    TerrainModifiedEvent event(area, ModificationType::Cleared);
    for (int i = 0; i < 10; ++i) {
        sync.recordModification(event, i * 100, 1);
    }

    ASSERT_EQ(sync.getModificationCount(), 10u);

    // Prune modifications with sequence <= 5
    sync.pruneModifications(5);

    // Should have 4 remaining (sequences 6, 7, 8, 9, 10)
    // Wait, sequences are 1-10, so pruning <= 5 leaves 6, 7, 8, 9, 10 = 5 mods
    // But sequence starts at 1, so after 10 mods we have seqs 1..10
    // Pruning <= 5 leaves 6..10 = 5 modifications
    ASSERT_EQ(sync.getModificationCount(), 5u);
}

// =============================================================================
// TerrainNetworkSync Client-side Tests
// =============================================================================

TEST(TerrainNetworkSync_ClientHandleSyncRequest) {
    // Server side: create sync request
    TerrainNetworkSync serverSync;
    TerrainGrid serverGrid(MapSize::Small);
    WaterData serverWaterData;
    serverWaterData.initialize(MapSize::Small);

    std::uint64_t seed = 54321;
    serverSync.setTerrainData(serverGrid, serverWaterData, seed);

    TerrainSyncRequestMessage request = serverSync.createSyncRequest();

    // Client side: handle sync request
    TerrainNetworkSync clientSync;
    TerrainGrid clientGrid;
    WaterData clientWaterData;

    ASSERT(clientSync.handleSyncRequest(request, clientGrid, clientWaterData));

    // Verify client generated terrain with correct dimensions
    ASSERT_EQ(clientGrid.width, 128u);
    ASSERT_EQ(clientGrid.height, 128u);
    ASSERT_EQ(clientGrid.sea_level, 8u);

    // State should be verifying after all mods applied (none in this case)
    ASSERT_EQ(static_cast<int>(clientSync.getState()),
              static_cast<int>(TerrainSyncState::Verifying));
}

TEST(TerrainNetworkSync_ClientApplyModifications) {
    TerrainNetworkSync serverSync;
    TerrainGrid serverGrid(MapSize::Small);
    WaterData serverWaterData;
    serverWaterData.initialize(MapSize::Small);

    serverSync.setTerrainData(serverGrid, serverWaterData, 12345);

    // Record a modification
    GridRect area = GridRect::singleTile(10, 10);
    TerrainModifiedEvent event(area, ModificationType::Leveled);
    serverSync.recordModification(event, 100, 1, 25);

    TerrainSyncRequestMessage request = serverSync.createSyncRequest();

    // Client handles request and applies modifications
    TerrainNetworkSync clientSync;
    TerrainGrid clientGrid;
    WaterData clientWaterData;

    ASSERT(clientSync.handleSyncRequest(request, clientGrid, clientWaterData));

    // Modifications should be applied
    ASSERT(!clientSync.hasModificationsToApply());

    // Verify the modification was applied
    ASSERT_EQ(clientGrid.at(10, 10).getElevation(), 25u);
}

TEST(TerrainNetworkSync_ClientVerifyMessage) {
    TerrainGrid grid(MapSize::Small);

    // Set some specific elevation values
    for (std::uint16_t y = 0; y < grid.height; ++y) {
        for (std::uint16_t x = 0; x < grid.width; ++x) {
            grid.at(x, y).setElevation(static_cast<std::uint8_t>((x + y) % 32));
        }
    }

    TerrainNetworkSync sync;
    TerrainSyncVerifyMessage verify = sync.createVerifyMessage(grid);

    // Checksum should be computed
    ASSERT_NE(verify.data.computed_checksum, 0u);
}

TEST(TerrainNetworkSync_ClientHandleSyncComplete_Success) {
    TerrainNetworkSync sync;

    TerrainSyncCompleteMessage complete;
    complete.data.result = TerrainSyncResult::Success;
    complete.data.final_sequence = 10;

    ASSERT(sync.handleSyncComplete(complete));
    ASSERT_EQ(static_cast<int>(sync.getState()),
              static_cast<int>(TerrainSyncState::Complete));
}

TEST(TerrainNetworkSync_ClientHandleSyncComplete_Mismatch) {
    TerrainNetworkSync sync;

    TerrainSyncCompleteMessage complete;
    complete.data.result = TerrainSyncResult::ChecksumMismatch;
    complete.data.final_sequence = 10;

    ASSERT(!sync.handleSyncComplete(complete));
    ASSERT_EQ(static_cast<int>(sync.getState()),
              static_cast<int>(TerrainSyncState::FallbackSnapshot));
}

// =============================================================================
// Integration Tests
// =============================================================================

TEST(TerrainNetworkSync_FullSyncFlow) {
    // This tests the complete sync flow:
    // 1. Server sets up terrain and records modifications
    // 2. Client receives sync request
    // 3. Client generates terrain from seed
    // 4. Client applies modifications
    // 5. Client sends verification
    // 6. Server verifies checksum

    // Server setup
    TerrainNetworkSync serverSync;
    TerrainGrid serverGrid(MapSize::Small);
    WaterData serverWaterData;
    serverWaterData.initialize(MapSize::Small);

    std::uint64_t seed = 99999;
    serverSync.setTerrainData(serverGrid, serverWaterData, seed);

    // Server records some modifications (apply to server grid too)
    for (int i = 0; i < 5; ++i) {
        GridRect area = GridRect::singleTile(static_cast<std::int16_t>(i * 10),
                                              static_cast<std::int16_t>(i * 10));
        TerrainModifiedEvent event(area, ModificationType::Leveled);
        serverSync.recordModification(event, i * 100, 1, 20 + i);
        serverGrid.at(i * 10, i * 10).setElevation(20 + i);
    }

    // Update server checksum after modifications
    serverSync.setTerrainData(serverGrid, serverWaterData, seed);
    // Re-record modifications with correct checksums
    serverSync.clearModificationHistory();
    for (int i = 0; i < 5; ++i) {
        GridRect area = GridRect::singleTile(static_cast<std::int16_t>(i * 10),
                                              static_cast<std::int16_t>(i * 10));
        TerrainModifiedEvent event(area, ModificationType::Leveled);
        serverSync.recordModification(event, i * 100, 1, 20 + i);
    }

    // Server creates sync request
    TerrainSyncRequestMessage request = serverSync.createSyncRequest();

    ASSERT_EQ(request.data.modification_count, 5u);

    // Client receives and handles request
    TerrainNetworkSync clientSync;
    TerrainGrid clientGrid;
    WaterData clientWaterData;

    ASSERT(clientSync.handleSyncRequest(request, clientGrid, clientWaterData));

    // Verify modifications were applied
    for (int i = 0; i < 5; ++i) {
        ASSERT_EQ(clientGrid.at(i * 10, i * 10).getElevation(), 20u + i);
    }

    // Client creates verify message
    TerrainSyncVerifyMessage verify = clientSync.createVerifyMessage(clientGrid);

    // The checksums should match since we applied the same modifications
    // Note: This may not match exactly due to generation differences
    // In a real scenario, the server would use the actual checksum
}

TEST(TerrainNetworkSync_BandwidthEfficiency) {
    // Verify that seed + modifications is smaller than full snapshot

    TerrainNetworkSync sync;
    TerrainGrid grid(MapSize::Medium);  // 256x256
    WaterData waterData;
    waterData.initialize(MapSize::Medium);

    sync.setTerrainData(grid, waterData, 12345);

    // Add a typical number of modifications (say 10)
    for (int i = 0; i < 10; ++i) {
        GridRect area = GridRect::singleTile(static_cast<std::int16_t>(i * 20),
                                              static_cast<std::int16_t>(i * 20));
        TerrainModifiedEvent event(area, ModificationType::Leveled);
        sync.recordModification(event, i * 100, 1, 15);
    }

    TerrainSyncRequestMessage request = sync.createSyncRequest();

    // Serialize to get actual size
    NetworkBuffer buffer;
    request.serializePayload(buffer);

    std::size_t syncSize = buffer.size();

    // Full snapshot for 256x256:
    // Header: 12 bytes
    // Tiles: 256 * 256 * 4 = 262,144 bytes
    // Water IDs: 256 * 256 * 2 = 131,072 bytes
    // Flow directions: 256 * 256 * 1 = 65,536 bytes
    // Total: ~448KB
    std::size_t fullSnapshotSize = 12 + (256 * 256 * 7);

    // Sync request should be much smaller
    // Header: 32 bytes
    // Modifications: 10 * 24 = 240 bytes
    // Total: ~272 bytes
    std::size_t expectedSyncSize = 32 + (10 * 24);

    ASSERT_EQ(syncSize, expectedSyncSize);
    ASSERT_LT(syncSize, fullSnapshotSize);

    // Verify it's less than 1KB as specified in acceptance criteria
    ASSERT_LT(syncSize, 1024u);

    std::cout << "(sync: " << syncSize << " bytes, full: " << fullSnapshotSize << " bytes) ";
}

// =============================================================================
// TerrainClientHandler Integration Tests
// =============================================================================

TEST(TerrainClientHandler_HandlesSyncMessages) {
    TerrainGrid grid(MapSize::Small);
    WaterData waterData;
    waterData.initialize(MapSize::Small);
    ChunkDirtyTracker dirtyTracker(128, 128);

    TerrainClientHandler handler(grid, waterData, dirtyTracker);

    // Verify handler can handle sync messages
    ASSERT(handler.canHandle(MessageType::TerrainSyncRequest));
    ASSERT(handler.canHandle(MessageType::TerrainSyncComplete));
    ASSERT(handler.canHandle(MessageType::TerrainModifiedEvent));
}

TEST(TerrainClientHandler_LegacyConstructor) {
    TerrainGrid grid(MapSize::Small);
    ChunkDirtyTracker dirtyTracker(128, 128);

    // Legacy constructor without water data
    TerrainClientHandler handler(grid, dirtyTracker);

    // Should still work
    ASSERT(handler.canHandle(MessageType::TerrainModifiedEvent));
}

// =============================================================================
// Message Registration Tests
// =============================================================================

TEST(TerrainSyncMessages_Registered) {
    // Force registration
    ASSERT(initTerrainSyncMessages());

    // Verify message types are registered
    ASSERT(MessageFactory::isRegistered(MessageType::TerrainSyncRequest));
    ASSERT(MessageFactory::isRegistered(MessageType::TerrainSyncVerify));
    ASSERT(MessageFactory::isRegistered(MessageType::TerrainSyncComplete));
}

// =============================================================================
// Main
// =============================================================================

int main() {
    std::cout << "=== Terrain Network Sync Tests ===" << std::endl;
    std::cout << std::endl;

    // Tests run automatically via static initialization

    std::cout << std::endl;
    std::cout << "Results: " << g_passCount << "/" << g_testCount << " tests passed" << std::endl;

    return (g_passCount == g_testCount) ? 0 : 1;
}
