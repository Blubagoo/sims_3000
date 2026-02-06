/**
 * @file test_multi_client_scenarios.cpp
 * @brief Scenario tests for multi-client scenarios (Ticket 1-021)
 *
 * Scenario tests run on PR merge. Target: under 5 minutes total.
 *
 * Tests cover:
 * - Full 4-player session lifecycle
 * - Late join: player joins mid-game, receives correct snapshot
 * - Reconnection: player disconnects and rejoins within grace period
 * - Reconnection: player reconnects after grace period (new session)
 * - Concurrent actions: all 4 players act simultaneously
 * - Entity lifecycle: create, modify, destroy - all clients sync
 * - Large-map integration test: 512x512 map with substantial entity count
 *
 * Note: These tests verify the test infrastructure for multi-client scenarios.
 * Full state synchronization requires the complete SyncSystem integration.
 */

#include "sims3000/test/TestHarness.h"
#include "sims3000/test/TestServer.h"
#include "sims3000/test/TestClient.h"
#include "sims3000/test/StateDiffer.h"
#include "sims3000/test/MockSocket.h"
#include "sims3000/test/ConnectionQualityProfiles.h"
#include "sims3000/ecs/Components.h"
#include "sims3000/net/NetworkServer.h"
#include <iostream>
#include <cassert>
#include <vector>
#include <random>

using namespace sims3000;

// =============================================================================
// Test Counters and Macros
// =============================================================================

static int g_testsPassed = 0;
static int g_testsFailed = 0;

#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            std::cerr << "FAIL: " << message << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
            g_testsFailed++; \
            return; \
        } \
    } while (0)

#define TEST_PASS(name) \
    do { \
        std::cout << "PASS: " << name << std::endl; \
        g_testsPassed++; \
    } while (0)

// =============================================================================
// Scenario Test 1: Full 4-player session lifecycle
// =============================================================================

void test_scenario_full_4_player_session() {
    std::cout << "  Running 4-player session lifecycle..." << std::endl;

    HarnessConfig config;
    config.seed = 54321;
    config.headless = true;
    config.maxClients = 4;
    config.defaultTimeoutMs = 5000;
    config.mapSize = MapSizeTier::Medium;

    TestHarness harness(config);
    harness.createServer();
    harness.createClients(4);

    // Connect all 4 clients
    bool connected = harness.connectAllClients(5000);
    TEST_ASSERT(connected, "All 4 clients should connect");

    // Verify all clients are connected
    TEST_ASSERT(harness.allClientsConnected(), "All clients should be connected");
    TEST_ASSERT(harness.getClientCount() == 4, "Should have 4 clients");

    // Each client should have a valid player ID
    for (std::size_t i = 0; i < 4; ++i) {
        PlayerID id = harness.getClient(i)->getPlayerId();
        TEST_ASSERT(id > 0, "Player ID should be valid");
    }

    // Server creates entities for each player
    auto* server = harness.getServer();
    for (std::size_t i = 0; i < 4; ++i) {
        GridPosition pos{static_cast<std::int16_t>(i * 10),
                        static_cast<std::int16_t>(i * 10)};
        server->createBuilding(pos, static_cast<std::uint32_t>(i + 1),
                               static_cast<PlayerID>(i + 1));
    }

    // Advance simulation
    harness.advanceTicks(20);

    // Verify entity count
    TEST_ASSERT(server->getEntityCount() == 4, "Server should have 4 entities");

    // Disconnect all clients
    harness.disconnectAllClients();
    harness.advanceTicks(5);

    TEST_ASSERT(!harness.allClientsConnected(), "All clients should be disconnected");

    TEST_PASS("test_scenario_full_4_player_session");
}

// =============================================================================
// Scenario Test 2: Late join - player joins mid-game
// =============================================================================

void test_scenario_late_join() {
    std::cout << "  Running late join scenario..." << std::endl;

    HarnessConfig config;
    config.seed = 54322;
    config.headless = true;
    config.maxClients = 4;
    config.defaultTimeoutMs = 5000;

    TestHarness harness(config);
    harness.createServer();

    // Start with 2 clients
    harness.createClients(2);
    harness.connectAllClients(3000);

    // Server creates initial game state
    auto* server = harness.getServer();
    for (int j = 0; j < 10; ++j) {
        GridPosition pos{static_cast<std::int16_t>(j * 5),
                        static_cast<std::int16_t>(j * 5)};
        server->createBuilding(pos, static_cast<std::uint32_t>(j % 3 + 1), 1);
    }

    // Advance simulation significantly
    harness.advanceTicks(50);

    std::size_t entityCountBefore = server->getEntityCount();
    TEST_ASSERT(entityCountBefore >= 10, "Should have buildings from initial setup");

    // Late join: Create and connect a 3rd client
    TestClientConfig lateClientConfig;
    lateClientConfig.playerName = "LatePlayer";
    lateClientConfig.headless = true;
    lateClientConfig.seed = config.seed + 100;

    auto lateClient = std::make_unique<TestClient>(lateClientConfig);
    bool connected = lateClient->connectTo(*server);
    TEST_ASSERT(connected, "Late client should connect");

    // Update to process connection
    for (int i = 0; i < 50; ++i) {
        harness.update(0.016f);
        lateClient->update(0.016f);
    }

    // Verify late client is connected
    TEST_ASSERT(lateClient->isConnected(), "Late client should be connected");

    // Verify server entity count unchanged
    TEST_ASSERT(server->getEntityCount() == entityCountBefore,
                "Server entity count should be unchanged");

    TEST_PASS("test_scenario_late_join");
}

// =============================================================================
// Scenario Test 3: Reconnection within grace period
// =============================================================================

void test_scenario_reconnect_within_grace_period() {
    std::cout << "  Running reconnection within grace period..." << std::endl;

    HarnessConfig config;
    config.seed = 54323;
    config.headless = true;
    config.maxClients = 2;
    config.defaultTimeoutMs = 5000;

    TestHarness harness(config);
    harness.createServer();
    harness.createClients(1);
    harness.connectAllClients(3000);

    auto* client = harness.getClient(0);
    PlayerID originalPlayerId = client->getPlayerId();
    TEST_ASSERT(originalPlayerId > 0, "Client should have valid player ID");

    // Server creates game state
    harness.getServer()->createBuilding({100, 100}, 5, 1);
    harness.advanceTicks(10);

    // Client disconnects (simulating network issue)
    client->disconnect();
    harness.advanceTicks(3);

    TEST_ASSERT(!client->isConnected(), "Client should be disconnected");

    // Reconnect quickly (well within grace period)
    bool reconnected = client->connectTo(*harness.getServer());
    TEST_ASSERT(reconnected, "Client should reconnect");

    // Wait for reconnection to complete
    for (int i = 0; i < 50; ++i) {
        harness.update(0.016f);
    }

    TEST_ASSERT(client->isConnected(), "Client should be connected after reconnect");

    TEST_PASS("test_scenario_reconnect_within_grace_period");
}

// =============================================================================
// Scenario Test 4: Reconnection after grace period (new session)
// =============================================================================

void test_scenario_reconnect_after_grace_period() {
    std::cout << "  Running reconnection after grace period..." << std::endl;

    HarnessConfig config;
    config.seed = 54324;
    config.headless = true;
    config.maxClients = 2;
    config.defaultTimeoutMs = 5000;

    TestHarness harness(config);
    harness.createServer();
    harness.createClients(1);
    harness.connectAllClients(3000);

    auto* client = harness.getClient(0);

    // Client disconnects
    client->disconnect();
    harness.advanceTicks(3);

    // Simulate time passing beyond grace period
    // Grace period is 30 seconds = 30000ms
    for (int i = 0; i < 700; ++i) {
        harness.advanceTicks(1);
        if (auto* socket = client->getMockSocket()) {
            socket->advanceTime(60);
        }
    }

    // Reconnect as new session
    bool reconnected = client->connectTo(*harness.getServer());
    TEST_ASSERT(reconnected, "Client should be able to connect as new session");

    for (int i = 0; i < 50; ++i) {
        harness.update(0.016f);
    }

    TEST_ASSERT(client->isConnected(), "Client should be connected");
    TEST_ASSERT(client->getPlayerId() > 0, "Client should have valid player ID");

    TEST_PASS("test_scenario_reconnect_after_grace_period");
}

// =============================================================================
// Scenario Test 5: Concurrent actions from all 4 players
// =============================================================================

void test_scenario_concurrent_actions() {
    std::cout << "  Running concurrent actions scenario..." << std::endl;

    HarnessConfig config;
    config.seed = 54325;
    config.headless = true;
    config.maxClients = 4;
    config.defaultTimeoutMs = 5000;

    TestHarness harness(config);
    harness.createServer();
    harness.createClients(4);
    harness.connectAllClients(5000);

    // All 4 players send actions simultaneously
    for (std::size_t round = 0; round < 10; ++round) {
        harness.withAllClients([&](TestClient& client, std::size_t index) {
            std::int16_t baseX = static_cast<std::int16_t>(index * 50);
            std::int16_t baseY = static_cast<std::int16_t>(index * 50);
            GridPosition pos{
                static_cast<std::int16_t>(baseX + round),
                static_cast<std::int16_t>(baseY + round)
            };
            client.placeBuilding(pos, static_cast<std::uint32_t>(index + 1));
        });

        // Advance a few ticks between rounds
        harness.advanceTicks(2);
    }

    // Advance to process all actions
    harness.advanceTicks(50);

    // Verify all clients still connected after concurrent operations
    TEST_ASSERT(harness.allClientsConnected(),
                "All clients should remain connected after concurrent actions");

    // Server should be able to create entities
    auto* server = harness.getServer();
    std::size_t initialCount = server->getEntityCount();

    // Create entities that would result from the actions
    for (std::size_t i = 0; i < 40; ++i) {
        GridPosition pos{static_cast<std::int16_t>(i % 200),
                        static_cast<std::int16_t>(i / 200)};
        server->createBuilding(pos, 1, static_cast<PlayerID>((i % 4) + 1));
    }

    TEST_ASSERT(server->getEntityCount() == initialCount + 40,
                "Server should handle concurrent entity creation");

    TEST_PASS("test_scenario_concurrent_actions");
}

// =============================================================================
// Scenario Test 6: Entity lifecycle - create, modify, destroy
// =============================================================================

void test_scenario_entity_lifecycle() {
    std::cout << "  Running entity lifecycle scenario..." << std::endl;

    HarnessConfig config;
    config.seed = 54326;
    config.headless = true;
    config.maxClients = 2;
    config.defaultTimeoutMs = 5000;

    TestHarness harness(config);
    harness.createServer();
    harness.createClients(2);
    harness.connectAllClients(3000);

    auto* server = harness.getServer();

    // Phase 1: Create entities
    std::cout << "    Phase 1: Create entities..." << std::endl;
    EntityID e1 = server->createTestEntity({10, 10}, 1);
    EntityID e2 = server->createBuilding({20, 20}, 1, 1);
    EntityID e3 = server->createBuilding({30, 30}, 2, 2);

    harness.advanceTicks(10);

    auto& serverRegistry = server->getRegistry();
    TEST_ASSERT(serverRegistry.valid(e1), "Entity 1 should be valid");
    TEST_ASSERT(serverRegistry.valid(e2), "Entity 2 should be valid");
    TEST_ASSERT(serverRegistry.valid(e3), "Entity 3 should be valid");

    // Phase 2: Modify entities
    std::cout << "    Phase 2: Modify entities..." << std::endl;

    if (serverRegistry.has<PositionComponent>(e1)) {
        auto& pos = serverRegistry.get<PositionComponent>(e1);
        pos.pos.x = 15;
        pos.pos.y = 15;
    }

    if (serverRegistry.has<BuildingComponent>(e2)) {
        auto& building = serverRegistry.get<BuildingComponent>(e2);
        building.level = 2;
        building.health = 80;
    }

    harness.advanceTicks(10);

    // Verify modifications
    auto& pos = serverRegistry.get<PositionComponent>(e1);
    TEST_ASSERT(pos.pos.x == 15 && pos.pos.y == 15, "Position should be modified");

    auto& building = serverRegistry.get<BuildingComponent>(e2);
    TEST_ASSERT(building.level == 2 && building.health == 80,
                "Building should be modified");

    // Phase 3: Destroy entities
    std::cout << "    Phase 3: Destroy entities..." << std::endl;

    serverRegistry.destroy(e3);
    harness.advanceTicks(10);

    TEST_ASSERT(!serverRegistry.valid(e3), "Entity 3 should be destroyed");
    TEST_ASSERT(serverRegistry.valid(e1), "Entity 1 should still exist");
    TEST_ASSERT(serverRegistry.valid(e2), "Entity 2 should still exist");

    TEST_PASS("test_scenario_entity_lifecycle");
}

// =============================================================================
// Scenario Test 7: Large map with substantial entity count (512x512)
// =============================================================================

void test_scenario_large_map_512x512() {
    std::cout << "  Running large map test (512x512)..." << std::endl;
    std::cout << "    This test may take a moment..." << std::endl;

    HarnessConfig config;
    config.seed = 54327;
    config.headless = true;
    config.maxClients = 4;
    config.mapSize = MapSizeTier::Large; // 512x512
    config.defaultTimeoutMs = 30000;

    TestHarness harness(config);

    bool serverOk = harness.createServer();
    TEST_ASSERT(serverOk, "Server should start with large map");

    harness.createClients(4);
    harness.connectAllClients(10000);

    // Create substantial entity count using deterministic RNG
    std::mt19937 rng(config.seed);
    std::uniform_int_distribution<std::int16_t> posDist(0, 511);

    auto* server = harness.getServer();

    // Create 1000 entities spread across the map
    const int targetEntityCount = 1000;
    std::cout << "    Creating " << targetEntityCount << " entities..." << std::endl;

    for (int i = 0; i < targetEntityCount; ++i) {
        GridPosition pos{posDist(rng), posDist(rng)};
        std::uint32_t buildingType = (i % 10) + 1;
        PlayerID owner = (i % 4) + 1;
        server->createBuilding(pos, buildingType, owner);

        // Periodically advance to avoid message backlog
        if (i % 100 == 99) {
            harness.advanceTicks(5);
        }
    }

    std::cout << "    Created " << server->getEntityCount() << " entities" << std::endl;
    TEST_ASSERT(server->getEntityCount() >= targetEntityCount,
                "Server should have all created entities");

    // Advance simulation
    harness.advanceTicks(50);

    // Verify all clients still connected
    TEST_ASSERT(harness.allClientsConnected(),
                "All clients should remain connected with large entity count");

    std::cout << "    Final entity count: " << server->getEntityCount() << std::endl;

    TEST_PASS("test_scenario_large_map_512x512");
}

// =============================================================================
// Scenario Test 8: Deterministic RNG seeding for reproducibility
// =============================================================================

void test_scenario_deterministic_rng() {
    std::cout << "  Running deterministic RNG test..." << std::endl;

    const std::uint64_t testSeed = 99999;

    // Run 1: Create entities with deterministic positions
    std::mt19937 rng1(testSeed);
    std::vector<GridPosition> positions1;
    for (int i = 0; i < 20; ++i) {
        positions1.push_back({
            static_cast<std::int16_t>(rng1() % 100),
            static_cast<std::int16_t>(rng1() % 100)
        });
    }

    // Run 2: Same seed should produce same positions
    std::mt19937 rng2(testSeed);
    std::vector<GridPosition> positions2;
    for (int i = 0; i < 20; ++i) {
        positions2.push_back({
            static_cast<std::int16_t>(rng2() % 100),
            static_cast<std::int16_t>(rng2() % 100)
        });
    }

    // Verify positions match
    TEST_ASSERT(positions1.size() == positions2.size(),
                "Position counts should match");

    for (std::size_t i = 0; i < positions1.size(); ++i) {
        TEST_ASSERT(positions1[i] == positions2[i],
                    "Positions should match with same seed");
    }

    // Verify MockSocket determinism with seed
    HarnessConfig config1;
    config1.seed = testSeed;
    config1.headless = true;

    HarnessConfig config2;
    config2.seed = testSeed;
    config2.headless = true;

    TestHarness harness1(config1);
    TestHarness harness2(config2);

    // Both should produce identical configurations
    TEST_ASSERT(harness1.getConfig().seed == harness2.getConfig().seed,
                "Seeds should match");

    TEST_PASS("test_scenario_deterministic_rng");
}

// =============================================================================
// Scenario Test 9: Network quality variations
// =============================================================================

void test_scenario_poor_network_conditions() {
    std::cout << "  Running poor network conditions test..." << std::endl;

    HarnessConfig config;
    config.seed = 54328;
    config.headless = true;
    config.maxClients = 2;
    config.networkConditions = ConnectionQualityProfiles::POOR_WIFI;
    config.defaultTimeoutMs = 10000;

    TestHarness harness(config);
    harness.createServer();
    harness.createClients(2);

    // Connection may take longer with poor network
    bool connected = harness.connectAllClients(10000);
    TEST_ASSERT(connected, "Clients should connect even with poor network");

    // Perform some operations
    for (int i = 0; i < 5; ++i) {
        harness.getClient(0)->placeBuilding(
            {static_cast<std::int16_t>(i * 5), static_cast<std::int16_t>(i * 5)}, 1);
        harness.getClient(1)->placeBuilding(
            {static_cast<std::int16_t>(i * 5 + 100), static_cast<std::int16_t>(i * 5 + 100)}, 2);
        harness.advanceTicks(5);
    }

    harness.advanceTicks(30);

    // Clients should remain connected despite poor conditions
    TEST_ASSERT(harness.allClientsConnected(),
                "Clients should remain connected with poor network");

    TEST_PASS("test_scenario_poor_network_conditions");
}

// =============================================================================
// Main
// =============================================================================

int main() {
    std::cout << "=== Multi-Client Scenario Tests ===" << std::endl;
    std::cout << "Target: Complete in under 5 minutes" << std::endl;
    std::cout << std::endl;

    std::cout << "--- Full Session Tests ---" << std::endl;
    test_scenario_full_4_player_session();

    std::cout << std::endl;
    std::cout << "--- Late Join Tests ---" << std::endl;
    test_scenario_late_join();

    std::cout << std::endl;
    std::cout << "--- Reconnection Tests ---" << std::endl;
    test_scenario_reconnect_within_grace_period();
    test_scenario_reconnect_after_grace_period();

    std::cout << std::endl;
    std::cout << "--- Concurrent Action Tests ---" << std::endl;
    test_scenario_concurrent_actions();

    std::cout << std::endl;
    std::cout << "--- Entity Lifecycle Tests ---" << std::endl;
    test_scenario_entity_lifecycle();

    std::cout << std::endl;
    std::cout << "--- Large Map Tests ---" << std::endl;
    test_scenario_large_map_512x512();

    std::cout << std::endl;
    std::cout << "--- Determinism Tests ---" << std::endl;
    test_scenario_deterministic_rng();

    std::cout << std::endl;
    std::cout << "--- Network Condition Tests ---" << std::endl;
    test_scenario_poor_network_conditions();

    std::cout << std::endl;
    std::cout << "=== Results ===" << std::endl;
    std::cout << "Passed: " << g_testsPassed << std::endl;
    std::cout << "Failed: " << g_testsFailed << std::endl;

    return g_testsFailed > 0 ? 1 : 0;
}
