/**
 * @file test_testing_infrastructure.cpp
 * @brief Unit tests for Testing Infrastructure (Ticket 1-019)
 *
 * Tests cover:
 * - MockSocket: in-memory message passing, network conditions
 * - Connection quality profiles
 * - TestServer: state inspection, entity management
 * - TestClient: assertions, input simulation
 * - TestHarness: multi-client coordination
 * - StateDiffer: ECS state comparison
 */

#include "sims3000/test/MockSocket.h"
#include "sims3000/test/ConnectionQualityProfiles.h"
#include "sims3000/test/TestServer.h"
#include "sims3000/test/TestClient.h"
#include "sims3000/test/TestHarness.h"
#include "sims3000/test/StateDiffer.h"
#include "sims3000/ecs/Components.h"
#include <cassert>
#include <iostream>
#include <cmath>

using namespace sims3000;

// =============================================================================
// Test Counters
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
// MockSocket Tests
// =============================================================================

void test_mock_socket_start_server() {
    MockSocket socket;
    TEST_ASSERT(!socket.isRunning(), "Socket should not be running initially");

    bool result = socket.startServer(0, 4);
    TEST_ASSERT(result, "startServer should succeed");
    TEST_ASSERT(socket.isRunning(), "Socket should be running after start");
    TEST_ASSERT(socket.getAssignedPort() > 0, "Should have assigned port");

    TEST_PASS("test_mock_socket_start_server");
}

void test_mock_socket_auto_port() {
    MockSocket socket1;
    MockSocket socket2;

    socket1.startServer(0, 4);
    socket2.startServer(0, 4);

    TEST_ASSERT(socket1.getAssignedPort() != socket2.getAssignedPort(),
                "Auto-assigned ports should be different");

    TEST_PASS("test_mock_socket_auto_port");
}

void test_mock_socket_connect() {
    MockSocket socket;
    PeerID peer = socket.connect("127.0.0.1", 7777);

    TEST_ASSERT(peer != INVALID_PEER_ID, "connect should return valid peer ID");
    TEST_ASSERT(socket.isRunning(), "Socket should be running after connect");

    // Poll should return connect event
    NetworkEvent event = socket.poll();
    TEST_ASSERT(event.type == NetworkEventType::Connect, "Should get connect event");
    TEST_ASSERT(event.peer == peer, "Event peer should match");

    TEST_PASS("test_mock_socket_connect");
}

void test_mock_socket_linked_pair() {
    auto [client, server] = MockSocket::createLinkedPair();

    server->startServer(7777, 4);
    client->simulateConnect();
    server->simulateConnect();

    // Send from client to server
    std::vector<std::uint8_t> data = {0x01, 0x02, 0x03};
    client->send(1, data.data(), data.size());
    client->flush();

    // Receive on server
    NetworkEvent event = server->poll();
    TEST_ASSERT(event.type == NetworkEventType::Receive, "Should receive data");
    TEST_ASSERT(event.data.size() == 3, "Data size should match");
    TEST_ASSERT(event.data[0] == 0x01, "Data content should match");

    TEST_PASS("test_mock_socket_linked_pair");
}

void test_mock_socket_latency_injection() {
    NetworkConditions conditions;
    conditions.latencyMs = 100;
    conditions.jitterMs = 0;

    auto [client, server] = MockSocket::createLinkedPair(conditions);

    server->startServer(7777, 4);
    client->simulateConnect();
    server->simulateConnect();

    // Send message
    std::vector<std::uint8_t> data = {0x01};
    client->send(1, data.data(), data.size());
    client->flush();

    // Message should be pending (latency not elapsed)
    TEST_ASSERT(server->getPendingDeliveryCount() == 1, "Should have pending delivery");

    NetworkEvent event = server->poll();
    TEST_ASSERT(event.type == NetworkEventType::None, "No event yet (latency)");

    // Advance time past latency
    server->advanceTime(150);
    event = server->poll();
    TEST_ASSERT(event.type == NetworkEventType::Receive, "Should receive after latency");

    TEST_PASS("test_mock_socket_latency_injection");
}

void test_mock_socket_packet_loss() {
    NetworkConditions conditions;
    conditions.packetLossPercent = 100.0f; // Drop everything

    MockSocket socket(conditions, 42); // Deterministic seed
    socket.startServer(7777, 4);
    socket.injectConnectEvent(1);

    std::vector<std::uint8_t> data = {0x01};
    socket.send(1, data.data(), data.size());

    TEST_ASSERT(socket.getDroppedPacketCount() == 1, "Should have dropped packet");

    TEST_PASS("test_mock_socket_packet_loss");
}

void test_mock_socket_message_interception() {
    MockSocket socket;
    socket.startServer(7777, 4);
    socket.injectConnectEvent(1);

    int interceptCount = 0;
    socket.setMessageInterceptor([&](const InterceptedMessage& msg) {
        interceptCount++;
    });

    std::vector<std::uint8_t> data = {0x01, 0x02};
    socket.send(1, data.data(), data.size());

    TEST_ASSERT(interceptCount == 1, "Interceptor should be called");
    TEST_ASSERT(socket.getInterceptedMessages().size() == 1, "Should have intercepted message");

    const auto& msg = socket.getInterceptedMessages()[0];
    TEST_ASSERT(msg.data.size() == 2, "Intercepted data size should match");

    TEST_PASS("test_mock_socket_message_interception");
}

void test_mock_socket_bandwidth_limit() {
    NetworkConditions conditions;
    conditions.bandwidthBytesPerSec = 10; // Very low limit

    MockSocket socket(conditions);
    socket.startServer(7777, 4);
    socket.injectConnectEvent(1);

    // Send more than bandwidth allows
    std::vector<std::uint8_t> bigData(20, 0x00);
    socket.send(1, bigData.data(), bigData.size());

    TEST_ASSERT(socket.getDroppedPacketCount() == 1, "Should drop over bandwidth limit");

    TEST_PASS("test_mock_socket_bandwidth_limit");
}

// =============================================================================
// Connection Quality Profile Tests
// =============================================================================

void test_quality_profiles_exist() {
    const auto& perfect = ConnectionQualityProfiles::PERFECT;
    const auto& lan = ConnectionQualityProfiles::LAN;
    const auto& goodWifi = ConnectionQualityProfiles::GOOD_WIFI;
    const auto& poorWifi = ConnectionQualityProfiles::POOR_WIFI;
    const auto& mobile3g = ConnectionQualityProfiles::MOBILE_3G;
    const auto& hostile = ConnectionQualityProfiles::HOSTILE;

    TEST_ASSERT(perfect.isPerfect(), "PERFECT should be perfect");
    TEST_ASSERT(lan.latencyMs == 1, "LAN should have 1ms latency");
    TEST_ASSERT(goodWifi.latencyMs == 20, "GOOD_WIFI should have 20ms latency");
    TEST_ASSERT(poorWifi.packetLossPercent > 0, "POOR_WIFI should have packet loss");
    TEST_ASSERT(mobile3g.latencyMs == 150, "MOBILE_3G should have 150ms latency");
    TEST_ASSERT(hostile.packetLossPercent >= 20.0f, "HOSTILE should have high packet loss");

    TEST_PASS("test_quality_profiles_exist");
}

void test_quality_profiles_get_by_name() {
    auto perfect = ConnectionQualityProfiles::getByName("perfect");
    auto lan = ConnectionQualityProfiles::getByName("LAN");
    auto unknown = ConnectionQualityProfiles::getByName("nonexistent");

    TEST_ASSERT(perfect.isPerfect(), "Should get PERFECT by name");
    TEST_ASSERT(lan.latencyMs == 1, "Should get LAN by name (case insensitive)");
    TEST_ASSERT(unknown.isPerfect(), "Unknown should return PERFECT");

    TEST_PASS("test_quality_profiles_get_by_name");
}

// =============================================================================
// TestServer Tests
// =============================================================================

void test_server_start_stop() {
    TestServer server;
    TEST_ASSERT(!server.isRunning(), "Server should not be running initially");

    bool result = server.start();
    TEST_ASSERT(result, "Server should start successfully");
    TEST_ASSERT(server.isRunning(), "Server should be running");
    TEST_ASSERT(server.getPort() > 0, "Should have assigned port");

    server.stop();
    TEST_ASSERT(!server.isRunning(), "Server should stop");

    TEST_PASS("test_server_start_stop");
}

void test_server_configurable_map_size() {
    TestServerConfig config;
    config.mapSize = MapSizeTier::Large;

    TestServer server(config);
    server.start();

    TEST_ASSERT(server.getConfig().mapSize == MapSizeTier::Large,
                "Map size should be configurable");

    TEST_PASS("test_server_configurable_map_size");
}

void test_server_entity_creation() {
    TestServer server;
    server.start();

    EntityID entity1 = server.createTestEntity({10, 20}, 1);
    EntityID entity2 = server.createBuilding({30, 40}, 5, 2);

    TEST_ASSERT(server.getEntityCount() == 2, "Should have 2 entities");

    auto& registry = server.getRegistry();
    TEST_ASSERT(registry.valid(entity1), "Entity 1 should be valid");
    TEST_ASSERT(registry.valid(entity2), "Entity 2 should be valid");
    TEST_ASSERT(registry.has<BuildingComponent>(entity2), "Entity 2 should have building");

    const auto& pos = registry.get<PositionComponent>(entity1);
    TEST_ASSERT(pos.pos.x == 10 && pos.pos.y == 20, "Position should match");

    TEST_PASS("test_server_entity_creation");
}

void test_server_tick_control() {
    TestServer server;
    server.start();

    TEST_ASSERT(server.getCurrentTick() == 0, "Initial tick should be 0");

    server.advanceTicks(10);
    TEST_ASSERT(server.getCurrentTick() == 10, "Tick should advance");

    server.setCurrentTick(100);
    TEST_ASSERT(server.getCurrentTick() == 100, "Tick should be settable");

    TEST_PASS("test_server_tick_control");
}

// =============================================================================
// TestClient Tests
// =============================================================================

void test_client_initial_state() {
    TestClient client;

    TEST_ASSERT(!client.isConnected(), "Client should not be connected initially");
    TEST_ASSERT(client.getState() == ConnectionState::Disconnected,
                "Initial state should be Disconnected");

    TEST_PASS("test_client_initial_state");
}

void test_client_assertions() {
    TestClient client;

    auto disconnected = client.assertDisconnected();
    TEST_ASSERT(disconnected.passed, "assertDisconnected should pass when disconnected");

    auto connected = client.assertConnected();
    TEST_ASSERT(!connected.passed, "assertConnected should fail when disconnected");
    TEST_ASSERT(!connected.message.empty(), "Failed assertion should have message");

    TEST_PASS("test_client_assertions");
}

void test_client_connect_to_server() {
    TestServer server;
    server.start();

    TestClient client;
    bool result = client.connectTo(server);

    TEST_ASSERT(result, "Connection should succeed");
    TEST_ASSERT(client.isConnected(), "Client should be connected");

    TEST_PASS("test_client_connect_to_server");
}

// =============================================================================
// TestHarness Tests
// =============================================================================

void test_harness_setup() {
    TestHarness harness;
    harness.setMapSize(MapSizeTier::Small);

    bool serverOk = harness.createServer();
    TEST_ASSERT(serverOk, "Server creation should succeed");

    bool clientsOk = harness.createClients(2);
    TEST_ASSERT(clientsOk, "Client creation should succeed");
    TEST_ASSERT(harness.getClientCount() == 2, "Should have 2 clients");

    TEST_PASS("test_harness_setup");
}

void test_harness_connect_all() {
    TestHarness harness;
    harness.createServer();
    harness.createClients(2);

    bool connected = harness.connectAllClients(1000);
    TEST_ASSERT(connected, "All clients should connect");
    TEST_ASSERT(harness.allClientsConnected(), "allClientsConnected should return true");

    TEST_PASS("test_harness_connect_all");
}

void test_harness_wait_for_sync() {
    TestHarness harness;
    harness.createServer();
    harness.createClients(2);
    harness.connectAllClients();

    SyncResult result = harness.waitForSync(1000);
    TEST_ASSERT(result.success, "Sync should succeed");

    TEST_PASS("test_harness_wait_for_sync");
}

void test_harness_state_match() {
    TestHarness harness;
    harness.createServer();
    harness.createClients(1);
    harness.connectAllClients();

    // States should match when empty
    auto result = harness.assertStateMatch();
    TEST_ASSERT(result.allMatch, "Empty states should match");

    TEST_PASS("test_harness_state_match");
}

void test_harness_with_client_helpers() {
    TestHarness harness;
    harness.createServer();
    harness.createClients(2);
    harness.connectAllClients();

    int actionCount = 0;
    harness.withClient(0, [&](TestClient& client) {
        actionCount++;
    });
    TEST_ASSERT(actionCount == 1, "withClient should call action once");

    actionCount = 0;
    harness.withAllClients([&](TestClient& client, std::size_t index) {
        actionCount++;
    });
    TEST_ASSERT(actionCount == 2, "withAllClients should call action for each client");

    TEST_PASS("test_harness_with_client_helpers");
}

// =============================================================================
// StateDiffer Tests
// =============================================================================

void test_differ_empty_registries() {
    Registry reg1;
    Registry reg2;
    StateDiffer differ;

    auto diffs = differ.compare(reg1, reg2);
    TEST_ASSERT(diffs.empty(), "Empty registries should have no differences");
    TEST_ASSERT(differ.statesMatch(reg1, reg2), "Empty registries should match");

    TEST_PASS("test_differ_empty_registries");
}

void test_differ_entity_missing() {
    Registry expected;
    Registry actual;
    StateDiffer differ;

    EntityID e = expected.create();
    expected.emplace<PositionComponent>(e, PositionComponent{{10, 20}, 0});

    auto diffs = differ.compare(expected, actual);
    TEST_ASSERT(diffs.size() == 1, "Should have one difference");
    TEST_ASSERT(diffs[0].type == DifferenceType::EntityMissing, "Should be EntityMissing");

    TEST_PASS("test_differ_entity_missing");
}

void test_differ_entity_extra() {
    Registry expected;
    Registry actual;
    StateDiffer differ;

    EntityID e = actual.create();
    actual.emplace<PositionComponent>(e, PositionComponent{{10, 20}, 0});

    auto diffs = differ.compare(expected, actual);
    TEST_ASSERT(diffs.size() == 1, "Should have one difference");
    TEST_ASSERT(diffs[0].type == DifferenceType::EntityExtra, "Should be EntityExtra");

    TEST_PASS("test_differ_entity_extra");
}

void test_differ_component_value_differs() {
    Registry expected;
    Registry actual;
    StateDiffer differ;

    // Create same entity in both
    EntityID e1 = expected.create();
    EntityID e2 = actual.create();

    expected.emplace<PositionComponent>(e1, PositionComponent{{10, 20}, 0});
    actual.emplace<PositionComponent>(e2, PositionComponent{{10, 30}, 0}); // Different Y

    // Need to ensure same entity ID for comparison
    if (e1 == e2) {
        auto diffs = differ.compare(expected, actual);
        bool found = false;
        for (const auto& diff : diffs) {
            if (diff.type == DifferenceType::ComponentValueDiffers &&
                diff.componentName == "PositionComponent") {
                found = true;
                break;
            }
        }
        TEST_ASSERT(found, "Should detect position difference");
    }

    TEST_PASS("test_differ_component_value_differs");
}

void test_differ_float_tolerance() {
    Registry expected;
    Registry actual;
    StateDiffer differ;

    EntityID e1 = expected.create();
    EntityID e2 = actual.create();

    TransformComponent t1;
    t1.position = {1.0f, 2.0f, 3.0f};
    expected.emplace<TransformComponent>(e1, t1);

    TransformComponent t2;
    t2.position = {1.0001f, 2.0001f, 3.0001f}; // Within tolerance
    actual.emplace<TransformComponent>(e2, t2);

    DiffOptions opts;
    opts.floatTolerance = 0.001f;
    opts.checkPosition = false; // Only check transform

    if (e1 == e2) {
        auto diffs = differ.compare(expected, actual, opts);
        // Should not report difference due to tolerance
        bool transformDiff = false;
        for (const auto& diff : diffs) {
            if (diff.componentName == "TransformComponent" &&
                diff.type == DifferenceType::ComponentValueDiffers) {
                transformDiff = true;
            }
        }
        TEST_ASSERT(!transformDiff, "Small float differences should be within tolerance");
    }

    TEST_PASS("test_differ_float_tolerance");
}

void test_differ_summarize() {
    std::vector<StateDifference> diffs;

    StateDifference d1;
    d1.type = DifferenceType::EntityMissing;
    d1.entityId = 1;
    diffs.push_back(d1);

    StateDifference d2;
    d2.type = DifferenceType::ComponentValueDiffers;
    d2.entityId = 2;
    d2.componentName = "PositionComponent";
    d2.expectedValue = "(10,20)";
    d2.actualValue = "(10,30)";
    diffs.push_back(d2);

    std::string summary = summarizeDifferences(diffs, 5);
    TEST_ASSERT(!summary.empty(), "Summary should not be empty");
    TEST_ASSERT(summary.find("2 difference") != std::string::npos,
                "Summary should mention difference count");

    TEST_PASS("test_differ_summarize");
}

// =============================================================================
// Headless Mode Test
// =============================================================================

void test_headless_mode() {
    HarnessConfig config;
    config.headless = true;

    TestHarness harness(config);
    harness.createServer();
    harness.createClients(1);
    harness.connectAllClients();

    // Should work without any SDL window
    harness.advanceTicks(10);

    TEST_ASSERT(harness.getServer()->isRunning(), "Server should run in headless mode");
    TEST_ASSERT(harness.allClientsConnected(), "Clients should connect in headless mode");

    TEST_PASS("test_headless_mode");
}

// =============================================================================
// Main
// =============================================================================

int main() {
    std::cout << "=== Testing Infrastructure Tests ===" << std::endl;
    std::cout << std::endl;

    std::cout << "--- MockSocket Tests ---" << std::endl;
    test_mock_socket_start_server();
    test_mock_socket_auto_port();
    test_mock_socket_connect();
    test_mock_socket_linked_pair();
    test_mock_socket_latency_injection();
    test_mock_socket_packet_loss();
    test_mock_socket_message_interception();
    test_mock_socket_bandwidth_limit();

    std::cout << std::endl;
    std::cout << "--- Connection Quality Profile Tests ---" << std::endl;
    test_quality_profiles_exist();
    test_quality_profiles_get_by_name();

    std::cout << std::endl;
    std::cout << "--- TestServer Tests ---" << std::endl;
    test_server_start_stop();
    test_server_configurable_map_size();
    test_server_entity_creation();
    test_server_tick_control();

    std::cout << std::endl;
    std::cout << "--- TestClient Tests ---" << std::endl;
    test_client_initial_state();
    test_client_assertions();
    test_client_connect_to_server();

    std::cout << std::endl;
    std::cout << "--- TestHarness Tests ---" << std::endl;
    test_harness_setup();
    test_harness_connect_all();
    test_harness_wait_for_sync();
    test_harness_state_match();
    test_harness_with_client_helpers();

    std::cout << std::endl;
    std::cout << "--- StateDiffer Tests ---" << std::endl;
    test_differ_empty_registries();
    test_differ_entity_missing();
    test_differ_entity_extra();
    test_differ_component_value_differs();
    test_differ_float_tolerance();
    test_differ_summarize();

    std::cout << std::endl;
    std::cout << "--- Headless Mode Tests ---" << std::endl;
    test_headless_mode();

    std::cout << std::endl;
    std::cout << "=== Results ===" << std::endl;
    std::cout << "Passed: " << g_testsPassed << std::endl;
    std::cout << "Failed: " << g_testsFailed << std::endl;

    return g_testsFailed > 0 ? 1 : 0;
}
