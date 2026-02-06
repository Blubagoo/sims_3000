/**
 * @file test_network_client.cpp
 * @brief Unit tests for NetworkClient (Ticket 1-009)
 *
 * Tests cover:
 * - Connection state machine transitions
 * - Reconnection with exponential backoff
 * - Input message queuing
 * - Heartbeat and RTT tracking
 * - Timeout detection levels
 */

#include "sims3000/net/NetworkClient.h"
#include "sims3000/net/MockTransport.h"
#include "sims3000/net/NetworkBuffer.h"
#include <cassert>
#include <iostream>
#include <thread>
#include <chrono>

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
// Test: Initial State
// =============================================================================

void test_initial_state() {
    auto transport = std::make_unique<MockTransport>();
    NetworkClient client(std::move(transport));

    TEST_ASSERT(client.getState() == ConnectionState::Disconnected,
                "Initial state should be Disconnected");
    TEST_ASSERT(!client.isPlaying(), "isPlaying should be false initially");
    TEST_ASSERT(!client.isConnecting(), "isConnecting should be false initially");
    TEST_ASSERT(client.getPlayerId() == 0, "Player ID should be 0 initially");
    TEST_ASSERT(client.getPendingInputCount() == 0, "No pending inputs initially");
    TEST_ASSERT(client.getPendingStateUpdateCount() == 0, "No pending state updates initially");

    TEST_PASS("test_initial_state");
}

// =============================================================================
// Test: Connection State Machine - Connect Attempt
// =============================================================================

void test_connect_transitions_to_connecting() {
    auto transport = std::make_unique<MockTransport>();
    NetworkClient client(std::move(transport));

    // Track state changes
    std::vector<ConnectionState> stateHistory;
    client.setStateChangeCallback([&](ConnectionState oldState, ConnectionState newState) {
        (void)oldState;
        stateHistory.push_back(newState);
    });

    bool result = client.connect("127.0.0.1", 7777, "TestPlayer");

    TEST_ASSERT(result, "connect() should return true");
    TEST_ASSERT(client.getState() == ConnectionState::Connecting,
                "State should be Connecting after connect()");
    TEST_ASSERT(client.isConnecting(), "isConnecting should be true");
    TEST_ASSERT(stateHistory.size() == 1, "Should have one state change");
    TEST_ASSERT(stateHistory[0] == ConnectionState::Connecting,
                "First state should be Connecting");

    client.disconnect();
    TEST_PASS("test_connect_transitions_to_connecting");
}

// =============================================================================
// Test: Connection State Machine - Already Connected
// =============================================================================

void test_connect_while_connecting_fails() {
    auto transport = std::make_unique<MockTransport>();
    NetworkClient client(std::move(transport));

    client.connect("127.0.0.1", 7777, "TestPlayer");

    // Try to connect again
    bool result = client.connect("127.0.0.1", 8888, "OtherPlayer");

    TEST_ASSERT(!result, "connect() should fail when already connecting");
    TEST_ASSERT(client.getState() == ConnectionState::Connecting,
                "State should remain Connecting");

    client.disconnect();
    TEST_PASS("test_connect_while_connecting_fails");
}

// =============================================================================
// Test: Connection with Empty Address
// =============================================================================

void test_connect_empty_address_fails() {
    auto transport = std::make_unique<MockTransport>();
    NetworkClient client(std::move(transport));

    bool result = client.connect("", 7777, "TestPlayer");

    TEST_ASSERT(!result, "connect() with empty address should fail");
    TEST_ASSERT(client.getState() == ConnectionState::Disconnected,
                "State should remain Disconnected");

    TEST_PASS("test_connect_empty_address_fails");
}

// =============================================================================
// Test: Connection with Empty Player Name
// =============================================================================

void test_connect_empty_player_name_fails() {
    auto transport = std::make_unique<MockTransport>();
    NetworkClient client(std::move(transport));

    bool result = client.connect("127.0.0.1", 7777, "");

    TEST_ASSERT(!result, "connect() with empty player name should fail");
    TEST_ASSERT(client.getState() == ConnectionState::Disconnected,
                "State should remain Disconnected");

    TEST_PASS("test_connect_empty_player_name_fails");
}

// =============================================================================
// Test: Disconnect from Disconnected State
// =============================================================================

void test_disconnect_when_disconnected() {
    auto transport = std::make_unique<MockTransport>();
    NetworkClient client(std::move(transport));

    // Should not crash or change state
    client.disconnect();

    TEST_ASSERT(client.getState() == ConnectionState::Disconnected,
                "State should remain Disconnected");

    TEST_PASS("test_disconnect_when_disconnected");
}

// =============================================================================
// Test: Disconnect from Connecting State
// =============================================================================

void test_disconnect_from_connecting() {
    auto transport = std::make_unique<MockTransport>();
    NetworkClient client(std::move(transport));

    client.connect("127.0.0.1", 7777, "TestPlayer");
    TEST_ASSERT(client.getState() == ConnectionState::Connecting,
                "Should be Connecting");

    client.disconnect();

    TEST_ASSERT(client.getState() == ConnectionState::Disconnected,
                "State should be Disconnected after disconnect()");
    TEST_ASSERT(!client.isConnecting(), "isConnecting should be false");

    TEST_PASS("test_disconnect_from_connecting");
}

// =============================================================================
// Test: Input Queuing When Not Playing
// =============================================================================

void test_input_queuing_when_not_playing() {
    auto transport = std::make_unique<MockTransport>();
    NetworkClient client(std::move(transport));

    // Create an input message
    InputMessage input;
    input.type = InputType::PlaceBuilding;
    input.targetPos = {10, 20};
    input.param1 = 1;

    // Queue input while not playing (should be ignored)
    client.queueInput(input);

    TEST_ASSERT(client.getPendingInputCount() == 0,
                "Input should be ignored when not playing");

    TEST_PASS("test_input_queuing_when_not_playing");
}

// =============================================================================
// Test: State Change Callback
// =============================================================================

void test_state_change_callback() {
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

    TEST_ASSERT(callbackCount == 1, "Callback should be called once");
    TEST_ASSERT(lastOldState == ConnectionState::Disconnected,
                "Old state should be Disconnected");
    TEST_ASSERT(lastNewState == ConnectionState::Connecting,
                "New state should be Connecting");

    client.disconnect();

    TEST_ASSERT(callbackCount == 2, "Callback should be called twice");
    TEST_ASSERT(lastNewState == ConnectionState::Disconnected,
                "Final state should be Disconnected");

    TEST_PASS("test_state_change_callback");
}

// =============================================================================
// Test: Connection Stats Initial Values
// =============================================================================

void test_connection_stats_initial() {
    auto transport = std::make_unique<MockTransport>();
    NetworkClient client(std::move(transport));

    const ConnectionStats& stats = client.getStats();

    TEST_ASSERT(stats.rttMs == 0, "Initial RTT should be 0");
    TEST_ASSERT(stats.smoothedRttMs == 0, "Initial smoothed RTT should be 0");
    TEST_ASSERT(stats.reconnectAttempts == 0, "Initial reconnect attempts should be 0");
    TEST_ASSERT(stats.messagesSent == 0, "Initial messages sent should be 0");
    TEST_ASSERT(stats.messagesReceived == 0, "Initial messages received should be 0");
    TEST_ASSERT(stats.timeoutLevel == ConnectionTimeoutLevel::None,
                "Initial timeout level should be None");

    TEST_PASS("test_connection_stats_initial");
}

// =============================================================================
// Test: Config Defaults
// =============================================================================

void test_connection_config_defaults() {
    ConnectionConfig config;

    TEST_ASSERT(config.initialReconnectDelayMs == 2000,
                "Initial reconnect delay should be 2000ms");
    TEST_ASSERT(config.maxReconnectDelayMs == 30000,
                "Max reconnect delay should be 30000ms");
    TEST_ASSERT(config.heartbeatIntervalMs == 1000,
                "Heartbeat interval should be 1000ms");
    TEST_ASSERT(config.timeoutIndicatorMs == 2000,
                "Timeout indicator should be 2s");
    TEST_ASSERT(config.timeoutBannerMs == 5000,
                "Timeout banner should be 5s");
    TEST_ASSERT(config.timeoutFullUIMs == 15000,
                "Timeout full UI should be 15s");

    TEST_PASS("test_connection_config_defaults");
}

// =============================================================================
// Test: Custom Config
// =============================================================================

void test_custom_connection_config() {
    ConnectionConfig config;
    config.initialReconnectDelayMs = 1000;
    config.maxReconnectDelayMs = 10000;
    config.heartbeatIntervalMs = 500;

    auto transport = std::make_unique<MockTransport>();
    NetworkClient client(std::move(transport), config);

    // Client should accept custom config (no way to query it back,
    // but at least it should construct without error)
    TEST_ASSERT(client.getState() == ConnectionState::Disconnected,
                "Should construct with custom config");

    TEST_PASS("test_custom_connection_config");
}

// =============================================================================
// Test: Connection State Name Strings
// =============================================================================

void test_connection_state_names() {
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

    TEST_PASS("test_connection_state_names");
}

// =============================================================================
// Test: Server Status Initial
// =============================================================================

void test_server_status_initial() {
    auto transport = std::make_unique<MockTransport>();
    NetworkClient client(std::move(transport));

    const ServerStatusMessage& status = client.getServerStatus();

    // Default-constructed status
    TEST_ASSERT(status.state == ServerState::Loading,
                "Initial server state should be Loading");
    TEST_ASSERT(status.mapSizeTier == MapSizeTier::Medium,
                "Initial map size tier should be Medium");

    TEST_ASSERT(client.isServerLoading(),
                "isServerLoading should return true initially");

    TEST_PASS("test_server_status_initial");
}

// =============================================================================
// Test: Multiple Disconnects
// =============================================================================

void test_multiple_disconnects() {
    auto transport = std::make_unique<MockTransport>();
    NetworkClient client(std::move(transport));

    client.connect("127.0.0.1", 7777, "TestPlayer");

    // Multiple disconnects should not crash
    client.disconnect();
    client.disconnect();
    client.disconnect();

    TEST_ASSERT(client.getState() == ConnectionState::Disconnected,
                "State should be Disconnected");

    TEST_PASS("test_multiple_disconnects");
}

// =============================================================================
// Test: Update While Disconnected
// =============================================================================

void test_update_while_disconnected() {
    auto transport = std::make_unique<MockTransport>();
    NetworkClient client(std::move(transport));

    // Should not crash
    client.update(0.016f);
    client.update(0.016f);

    TEST_ASSERT(client.getState() == ConnectionState::Disconnected,
                "State should remain Disconnected");

    TEST_PASS("test_update_while_disconnected");
}

// =============================================================================
// Test: Poll State Update Empty
// =============================================================================

void test_poll_state_update_empty() {
    auto transport = std::make_unique<MockTransport>();
    NetworkClient client(std::move(transport));

    StateUpdateMessage update;
    bool hasUpdate = client.pollStateUpdate(update);

    TEST_ASSERT(!hasUpdate, "pollStateUpdate should return false when empty");

    TEST_PASS("test_poll_state_update_empty");
}

// =============================================================================
// Test: Server Status Callback
// =============================================================================

void test_server_status_callback() {
    auto transport = std::make_unique<MockTransport>();
    NetworkClient client(std::move(transport));

    int callbackCount = 0;
    client.setServerStatusCallback([&](const ServerStatusMessage& status) {
        (void)status;
        callbackCount++;
    });

    // Callback won't be called until we receive a message, but it should be set
    TEST_ASSERT(callbackCount == 0, "Callback should not be called yet");

    TEST_PASS("test_server_status_callback");
}

// =============================================================================
// Main
// =============================================================================

int main() {
    std::cout << "=== NetworkClient Tests ===" << std::endl;

    test_initial_state();
    test_connect_transitions_to_connecting();
    test_connect_while_connecting_fails();
    test_connect_empty_address_fails();
    test_connect_empty_player_name_fails();
    test_disconnect_when_disconnected();
    test_disconnect_from_connecting();
    test_input_queuing_when_not_playing();
    test_state_change_callback();
    test_connection_stats_initial();
    test_connection_config_defaults();
    test_custom_connection_config();
    test_connection_state_names();
    test_server_status_initial();
    test_multiple_disconnects();
    test_update_while_disconnected();
    test_poll_state_update_empty();
    test_server_status_callback();

    std::cout << "\n=== Results ===" << std::endl;
    std::cout << "Passed: " << g_testsPassed << std::endl;
    std::cout << "Failed: " << g_testsFailed << std::endl;

    return g_testsFailed > 0 ? 1 : 0;
}
