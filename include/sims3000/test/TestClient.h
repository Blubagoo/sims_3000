/**
 * @file TestClient.h
 * @brief Scriptable test client with assertion helpers.
 *
 * Wraps NetworkClient to provide:
 * - Simplified connection API
 * - Assertion helpers for testing
 * - State inspection
 * - Scriptable actions
 *
 * Usage:
 *   TestClient client;
 *   client.connectTo("127.0.0.1", 7777, "TestPlayer");
 *   client.waitForState(ConnectionState::Playing, 5000);
 *   client.assertConnected();
 *
 * Ownership: Test code owns TestClient.
 * Cleanup: Destructor disconnects and cleans up resources.
 *
 * Thread safety: Not thread-safe. Single-threaded test use only.
 */

#ifndef SIMS3000_TEST_TESTCLIENT_H
#define SIMS3000_TEST_TESTCLIENT_H

#include "sims3000/net/NetworkClient.h"
#include "sims3000/test/MockSocket.h"
#include "sims3000/ecs/Registry.h"
#include "sims3000/net/InputMessage.h"
#include <memory>
#include <functional>
#include <string>
#include <vector>

namespace sims3000 {

/**
 * @struct TestClientConfig
 * @brief Configuration for TestClient.
 */
struct TestClientConfig {
    /// Player name
    std::string playerName = "TestPlayer";

    /// Network conditions for the mock socket
    NetworkConditions networkConditions = ConnectionQualityProfiles::PERFECT;

    /// Random seed for deterministic behavior (0 for random)
    std::uint64_t seed = 0;

    /// Connection timeout in milliseconds
    std::uint32_t connectTimeoutMs = 5000;

    /// Enable headless mode (no SDL initialization)
    bool headless = true;
};

/**
 * @struct AssertionResult
 * @brief Result of a test assertion.
 */
struct AssertionResult {
    bool passed = false;
    std::string message;

    explicit operator bool() const { return passed; }
};

/**
 * @class TestClient
 * @brief Scriptable test client with assertion helpers.
 */
class TestClient {
public:
    /**
     * @brief Construct a TestClient with default configuration.
     */
    TestClient();

    /**
     * @brief Construct a TestClient with specified configuration.
     * @param config Client configuration.
     */
    explicit TestClient(const TestClientConfig& config);

    /**
     * @brief Destructor. Disconnects if connected.
     */
    ~TestClient();

    // Non-copyable
    TestClient(const TestClient&) = delete;
    TestClient& operator=(const TestClient&) = delete;

    // =========================================================================
    // Connection
    // =========================================================================

    /**
     * @brief Connect to a server.
     * @param address Server address.
     * @param port Server port.
     * @param playerName Player name (uses config default if empty).
     * @return true if connection attempt started.
     */
    bool connectTo(const std::string& address, std::uint16_t port,
                   const std::string& playerName = "");

    /**
     * @brief Connect to a TestServer directly via linked sockets.
     * @param server TestServer to connect to.
     * @return true if connection established.
     */
    bool connectTo(class TestServer& server);

    /**
     * @brief Disconnect from the server.
     */
    void disconnect();

    /**
     * @brief Check if connected and playing.
     */
    bool isConnected() const;

    /**
     * @brief Check if currently connecting.
     */
    bool isConnecting() const;

    /**
     * @brief Get current connection state.
     */
    ConnectionState getState() const;

    /**
     * @brief Get assigned player ID (valid after connection).
     */
    PlayerID getPlayerId() const;

    // =========================================================================
    // Update Control
    // =========================================================================

    /**
     * @brief Update the client by one frame.
     * @param deltaTime Frame delta time in seconds.
     */
    void update(float deltaTime);

    /**
     * @brief Advance client by specified time in milliseconds.
     * @param ms Time to advance.
     * @param frameSize Frame size in ms (default 16ms ~60fps).
     */
    void advanceTime(std::uint32_t ms, std::uint32_t frameSize = 16);

    /**
     * @brief Wait for a specific connection state.
     * @param state Target state.
     * @param timeoutMs Maximum time to wait.
     * @return true if state reached within timeout.
     *
     * Advances simulated time while waiting.
     */
    bool waitForState(ConnectionState state, std::uint32_t timeoutMs);

    // =========================================================================
    // State Inspection
    // =========================================================================

    /**
     * @brief Get the ECS registry for state inspection.
     */
    Registry& getRegistry();
    const Registry& getRegistry() const;

    /**
     * @brief Get pending state update count.
     */
    std::size_t getPendingStateUpdates() const;

    /**
     * @brief Get the mock socket for direct manipulation.
     */
    MockSocket* getMockSocket();

    /**
     * @brief Get the underlying NetworkClient.
     */
    NetworkClient* getNetworkClient();

    /**
     * @brief Get connection statistics.
     */
    const ConnectionStats& getStats() const;

    /**
     * @brief Get configuration.
     */
    const TestClientConfig& getConfig() const { return m_config; }

    // =========================================================================
    // Input Simulation
    // =========================================================================

    /**
     * @brief Send an input message to the server.
     * @param input Input message to send.
     */
    void sendInput(const InputMessage& input);

    /**
     * @brief Send a place building input.
     * @param pos Grid position.
     * @param buildingType Building type ID.
     */
    void placeBuilding(GridPosition pos, std::uint32_t buildingType);

    /**
     * @brief Send a demolish input.
     * @param pos Grid position.
     */
    void demolish(GridPosition pos);

    /**
     * @brief Get pending input message count.
     */
    std::size_t getPendingInputCount() const;

    // =========================================================================
    // Assertion Helpers
    // =========================================================================

    /**
     * @brief Assert that client is connected.
     * @return Assertion result.
     */
    AssertionResult assertConnected() const;

    /**
     * @brief Assert that client is disconnected.
     * @return Assertion result.
     */
    AssertionResult assertDisconnected() const;

    /**
     * @brief Assert that client is in a specific state.
     * @param state Expected state.
     * @return Assertion result.
     */
    AssertionResult assertState(ConnectionState state) const;

    /**
     * @brief Assert that player ID matches expected.
     * @param expected Expected player ID.
     * @return Assertion result.
     */
    AssertionResult assertPlayerId(PlayerID expected) const;

    /**
     * @brief Assert that entity exists in client registry.
     * @param entityId Entity to check.
     * @return Assertion result.
     */
    AssertionResult assertEntityExists(EntityID entityId) const;

    /**
     * @brief Assert that RTT is below threshold.
     * @param maxRttMs Maximum acceptable RTT in milliseconds.
     * @return Assertion result.
     */
    AssertionResult assertRttBelow(std::uint32_t maxRttMs) const;

    // =========================================================================
    // Link Support
    // =========================================================================

    /**
     * @brief Link this client's socket to a server's socket.
     * @param serverSocket Server's mock socket.
     */
    void linkToServer(MockSocket* serverSocket);

private:
    TestClientConfig m_config;
    std::unique_ptr<MockSocket> m_socket;
    std::unique_ptr<NetworkClient> m_client;
    Registry m_registry;
    bool m_linkedToServer = false;
};

} // namespace sims3000

#endif // SIMS3000_TEST_TESTCLIENT_H
