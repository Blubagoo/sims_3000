/**
 * @file test_network_server.cpp
 * @brief Unit tests for NetworkServer (Ticket 1-008)
 */

#include "sims3000/net/NetworkServer.h"
#include "sims3000/net/MockTransport.h"
#include "sims3000/net/ClientMessages.h"
#include "sims3000/net/ServerMessages.h"
#include "sims3000/net/NetworkBuffer.h"
#include <iostream>
#include <cassert>
#include <thread>
#include <chrono>

using namespace sims3000;

// Helper to serialize a message to bytes
std::vector<std::uint8_t> serializeMessage(const NetworkMessage& msg) {
    NetworkBuffer buffer;
    msg.serializeWithEnvelope(buffer);
    return buffer.raw();
}

// Test handler that records received messages
class TestHandler : public INetworkHandler {
public:
    std::vector<std::pair<PeerID, MessageType>> receivedMessages;
    std::vector<PeerID> connectedPeers;
    std::vector<std::pair<PeerID, bool>> disconnectedPeers;

    bool canHandle(MessageType type) const override {
        return type == MessageType::Chat || type == MessageType::Input;
    }

    void handleMessage(PeerID peer, const NetworkMessage& msg) override {
        receivedMessages.push_back({peer, msg.getType()});
    }

    void onClientConnected(PeerID peer) override {
        connectedPeers.push_back(peer);
    }

    void onClientDisconnected(PeerID peer, bool timedOut) override {
        disconnectedPeers.push_back({peer, timedOut});
    }

    void clear() {
        receivedMessages.clear();
        connectedPeers.clear();
        disconnectedPeers.clear();
    }
};

// ============================================================================
// Test: Server Creation and Configuration
// ============================================================================
void test_server_creation() {
    std::cout << "  test_server_creation..." << std::flush;

    ServerConfig config;
    config.port = 7777;
    config.maxPlayers = 4;
    config.mapSize = MapSizeTier::Medium;
    config.serverName = "Test Server";

    auto transport = std::make_unique<MockTransport>();
    NetworkServer server(std::move(transport), config);

    assert(!server.isRunning());
    assert(server.getState() == ServerNetworkState::Initializing);
    assert(server.getConfig().port == 7777);
    assert(server.getConfig().maxPlayers == 4);
    assert(server.getConfig().mapSize == MapSizeTier::Medium);
    assert(server.getConfig().serverName == "Test Server");

    std::cout << " PASS" << std::endl;
}

// ============================================================================
// Test: Server Start and Stop
// ============================================================================
void test_server_start_stop() {
    std::cout << "  test_server_start_stop..." << std::flush;

    ServerConfig config;
    config.port = 7778;
    config.maxPlayers = 4;

    auto transport = std::make_unique<MockTransport>();
    NetworkServer server(std::move(transport), config);

    // Start server
    assert(server.start());
    assert(server.isRunning());
    assert(server.getState() == ServerNetworkState::Ready);

    // Stop server
    server.stop();
    assert(!server.isRunning());
    assert(server.getState() == ServerNetworkState::Initializing);

    std::cout << " PASS" << std::endl;
}

// ============================================================================
// Test: Map Size Configuration
// ============================================================================
void test_map_size_configuration() {
    std::cout << "  test_map_size_configuration..." << std::flush;

    // Test small map
    {
        ServerConfig config;
        config.mapSize = MapSizeTier::Small;
        auto transport = std::make_unique<MockTransport>();
        NetworkServer server(std::move(transport), config);
        assert(server.getConfig().mapSize == MapSizeTier::Small);
    }

    // Test medium map (default)
    {
        ServerConfig config;
        config.mapSize = MapSizeTier::Medium;
        auto transport = std::make_unique<MockTransport>();
        NetworkServer server(std::move(transport), config);
        assert(server.getConfig().mapSize == MapSizeTier::Medium);
    }

    // Test large map
    {
        ServerConfig config;
        config.mapSize = MapSizeTier::Large;
        auto transport = std::make_unique<MockTransport>();
        NetworkServer server(std::move(transport), config);
        assert(server.getConfig().mapSize == MapSizeTier::Large);
    }

    std::cout << " PASS" << std::endl;
}

// ============================================================================
// Test: Max Players Enforcement
// ============================================================================
void test_max_players_enforcement() {
    std::cout << "  test_max_players_enforcement..." << std::flush;

    // Test that maxPlayers is capped at 4
    ServerConfig config;
    config.maxPlayers = 10;  // Try to set more than allowed

    auto transport = std::make_unique<MockTransport>();
    NetworkServer server(std::move(transport), config);

    // Should be capped at MAX_PLAYERS (4)
    assert(server.getConfig().maxPlayers == NetworkServer::MAX_PLAYERS);

    std::cout << " PASS" << std::endl;
}

// ============================================================================
// Test: Handler Registration
// ============================================================================
void test_handler_registration() {
    std::cout << "  test_handler_registration..." << std::flush;

    ServerConfig config;
    auto transport = std::make_unique<MockTransport>();
    NetworkServer server(std::move(transport), config);

    TestHandler handler;

    // Register handler
    server.registerHandler(&handler);

    // Unregister handler
    server.unregisterHandler(&handler);

    // Should not crash on double unregister
    server.unregisterHandler(&handler);

    std::cout << " PASS" << std::endl;
}

// ============================================================================
// Test: Client Count Initially Zero
// ============================================================================
void test_client_count_initially_zero() {
    std::cout << "  test_client_count_initially_zero..." << std::flush;

    ServerConfig config;
    auto transport = std::make_unique<MockTransport>();
    NetworkServer server(std::move(transport), config);

    assert(server.start());
    assert(server.getClientCount() == 0);
    assert(server.getClients().empty());

    server.stop();

    std::cout << " PASS" << std::endl;
}

// ============================================================================
// Test: Server State Transitions
// ============================================================================
void test_server_state_transitions() {
    std::cout << "  test_server_state_transitions..." << std::flush;

    ServerConfig config;
    auto transport = std::make_unique<MockTransport>();
    NetworkServer server(std::move(transport), config);

    // Initial state
    assert(server.getState() == ServerNetworkState::Initializing);

    // After start
    assert(server.start());
    assert(server.getState() == ServerNetworkState::Ready);

    // Transition to running
    server.setRunning();
    assert(server.getState() == ServerNetworkState::Running);

    // After stop
    server.stop();
    assert(server.getState() == ServerNetworkState::Initializing);

    std::cout << " PASS" << std::endl;
}

// ============================================================================
// Test: Server Uptime Tracking
// ============================================================================
void test_uptime_tracking() {
    std::cout << "  test_uptime_tracking..." << std::flush;

    ServerConfig config;
    auto transport = std::make_unique<MockTransport>();
    NetworkServer server(std::move(transport), config);

    assert(server.start());

    // Initial uptime should be 0
    assert(server.getUptime() == 0.0f);

    // Update with delta time
    server.update(0.5f);
    assert(server.getUptime() >= 0.5f);

    server.update(0.5f);
    assert(server.getUptime() >= 1.0f);

    server.stop();

    std::cout << " PASS" << std::endl;
}

// ============================================================================
// Test: Server State Name Helper
// ============================================================================
void test_state_name_helper() {
    std::cout << "  test_state_name_helper..." << std::flush;

    assert(std::string(getServerNetworkStateName(ServerNetworkState::Initializing)) == "Initializing");
    assert(std::string(getServerNetworkStateName(ServerNetworkState::Loading)) == "Loading");
    assert(std::string(getServerNetworkStateName(ServerNetworkState::Ready)) == "Ready");
    assert(std::string(getServerNetworkStateName(ServerNetworkState::Running)) == "Running");

    std::cout << " PASS" << std::endl;
}

// ============================================================================
// Test: Simulation Tick Tracking
// ============================================================================
void test_tick_tracking() {
    std::cout << "  test_tick_tracking..." << std::flush;

    ServerConfig config;
    auto transport = std::make_unique<MockTransport>();
    NetworkServer server(std::move(transport), config);

    assert(server.start());

    // Initial tick is 0
    assert(server.getCurrentTick() == 0);

    // Set tick
    server.setCurrentTick(100);
    assert(server.getCurrentTick() == 100);

    server.setCurrentTick(12345);
    assert(server.getCurrentTick() == 12345);

    server.stop();

    std::cout << " PASS" << std::endl;
}

// ============================================================================
// Test: Server Heartbeat Constants
// ============================================================================
void test_heartbeat_constants() {
    std::cout << "  test_heartbeat_constants..." << std::flush;

    // Verify heartbeat timing constants per ticket requirements
    // Heartbeat interval: 1 second
    assert(NetworkServer::HEARTBEAT_INTERVAL_SEC == 1.0f);

    // 5 consecutive missed heartbeats = 5s warning
    assert(NetworkServer::HEARTBEAT_WARNING_THRESHOLD == 5);

    // 10 seconds of silence = hard disconnect
    assert(NetworkServer::HEARTBEAT_DISCONNECT_THRESHOLD == 10);

    std::cout << " PASS" << std::endl;
}

// ============================================================================
// Test: Server Config Default Values
// ============================================================================
void test_default_config() {
    std::cout << "  test_default_config..." << std::flush;

    ServerConfig config;

    // Verify defaults per ticket requirements
    assert(config.port == 7777);  // Default port
    assert(config.maxPlayers == 4);  // Max 4 per canon
    assert(config.mapSize == MapSizeTier::Medium);  // Default medium

    std::cout << " PASS" << std::endl;
}

// ============================================================================
// Test: Client Connection Lookup
// ============================================================================
void test_client_lookup() {
    std::cout << "  test_client_lookup..." << std::flush;

    ServerConfig config;
    auto transport = std::make_unique<MockTransport>();
    NetworkServer server(std::move(transport), config);

    assert(server.start());

    // No clients initially
    assert(server.getClient(1) == nullptr);
    assert(server.getClientByPlayerId(1) == nullptr);

    server.stop();

    std::cout << " PASS" << std::endl;
}

// ============================================================================
// Test: Update Without Crash
// ============================================================================
void test_update_without_crash() {
    std::cout << "  test_update_without_crash..." << std::flush;

    ServerConfig config;
    auto transport = std::make_unique<MockTransport>();
    NetworkServer server(std::move(transport), config);

    assert(server.start());

    // Multiple updates should not crash
    for (int i = 0; i < 100; ++i) {
        server.update(0.016f);  // ~60 fps
    }

    server.stop();

    std::cout << " PASS" << std::endl;
}

// ============================================================================
// Test: Update When Not Running
// ============================================================================
void test_update_when_not_running() {
    std::cout << "  test_update_when_not_running..." << std::flush;

    ServerConfig config;
    auto transport = std::make_unique<MockTransport>();
    NetworkServer server(std::move(transport), config);

    // Should not crash when updating before start
    server.update(0.016f);
    server.update(0.016f);

    // Uptime should remain 0
    assert(server.getUptime() == 0.0f);

    std::cout << " PASS" << std::endl;
}

// ============================================================================
// Test: Send To Non-Existent Peer
// ============================================================================
void test_send_to_nonexistent_peer() {
    std::cout << "  test_send_to_nonexistent_peer..." << std::flush;

    ServerConfig config;
    auto transport = std::make_unique<MockTransport>();
    NetworkServer server(std::move(transport), config);

    assert(server.start());

    // Create a dummy message
    ServerStatusMessage msg;
    msg.state = ServerState::Ready;

    // Send to non-existent peer should return false
    assert(!server.sendTo(999, msg));
    assert(!server.sendToPlayer(1, msg));

    server.stop();

    std::cout << " PASS" << std::endl;
}

// ============================================================================
// Test: Kick Non-Existent Player
// ============================================================================
void test_kick_nonexistent_player() {
    std::cout << "  test_kick_nonexistent_player..." << std::flush;

    ServerConfig config;
    auto transport = std::make_unique<MockTransport>();
    NetworkServer server(std::move(transport), config);

    assert(server.start());

    // Should not crash when kicking non-existent player
    server.kickPlayer(99, "Test reason");
    server.kickPeer(999, "Test reason");

    // Client count should remain 0
    assert(server.getClientCount() == 0);

    server.stop();

    std::cout << " PASS" << std::endl;
}

// ============================================================================
// Test: Broadcast When No Clients
// ============================================================================
void test_broadcast_no_clients() {
    std::cout << "  test_broadcast_no_clients..." << std::flush;

    ServerConfig config;
    auto transport = std::make_unique<MockTransport>();
    NetworkServer server(std::move(transport), config);

    assert(server.start());

    // Broadcast should not crash with no clients
    StateUpdateMessage stateMsg;
    stateMsg.tick = 1;
    server.broadcastStateUpdate(stateMsg);

    server.broadcastServerChat("Hello world!");

    ServerStatusMessage statusMsg;
    server.broadcast(statusMsg);

    server.stop();

    std::cout << " PASS" << std::endl;
}

// ============================================================================
// Test: Double Start
// ============================================================================
void test_double_start() {
    std::cout << "  test_double_start..." << std::flush;

    ServerConfig config;
    auto transport = std::make_unique<MockTransport>();
    NetworkServer server(std::move(transport), config);

    assert(server.start());
    assert(server.isRunning());

    // Second start should still succeed (returns true, already running)
    assert(server.start());
    assert(server.isRunning());

    server.stop();

    std::cout << " PASS" << std::endl;
}

// ============================================================================
// Test: Double Stop
// ============================================================================
void test_double_stop() {
    std::cout << "  test_double_stop..." << std::flush;

    ServerConfig config;
    auto transport = std::make_unique<MockTransport>();
    NetworkServer server(std::move(transport), config);

    assert(server.start());
    server.stop();
    assert(!server.isRunning());

    // Second stop should not crash
    server.stop();
    assert(!server.isRunning());

    std::cout << " PASS" << std::endl;
}

// ============================================================================
// Main
// ============================================================================
int main() {
    std::cout << "Running NetworkServer tests..." << std::endl;
    std::cout << std::endl;

    std::cout << "Server Configuration:" << std::endl;
    test_server_creation();
    test_map_size_configuration();
    test_max_players_enforcement();
    test_default_config();
    std::cout << std::endl;

    std::cout << "Server Lifecycle:" << std::endl;
    test_server_start_stop();
    test_server_state_transitions();
    test_double_start();
    test_double_stop();
    std::cout << std::endl;

    std::cout << "Handler Management:" << std::endl;
    test_handler_registration();
    std::cout << std::endl;

    std::cout << "Client Management:" << std::endl;
    test_client_count_initially_zero();
    test_client_lookup();
    test_kick_nonexistent_player();
    std::cout << std::endl;

    std::cout << "Messaging:" << std::endl;
    test_send_to_nonexistent_peer();
    test_broadcast_no_clients();
    std::cout << std::endl;

    std::cout << "Time & State Tracking:" << std::endl;
    test_uptime_tracking();
    test_tick_tracking();
    test_state_name_helper();
    test_heartbeat_constants();
    std::cout << std::endl;

    std::cout << "Update Loop:" << std::endl;
    test_update_without_crash();
    test_update_when_not_running();
    std::cout << std::endl;

    std::cout << "All NetworkServer tests passed!" << std::endl;
    return 0;
}
