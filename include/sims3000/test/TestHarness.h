/**
 * @file TestHarness.h
 * @brief Orchestrator for multi-client test scenarios.
 *
 * Provides:
 * - Multi-client test coordination
 * - wait_for_sync(): Block until all clients synced
 * - assert_state_match(): Compare server and client states
 * - StateDiffer integration
 *
 * Usage:
 *   TestHarness harness;
 *   harness.setMapSize(MapSizeTier::Small);
 *   harness.createServer();
 *   harness.createClients(4);
 *
 *   harness.connectAllClients();
 *   harness.waitForSync(1000);
 *   auto result = harness.assertStateMatch();
 *
 * Ownership: TestHarness owns server and clients.
 * Cleanup: Destructor stops server and disconnects all clients.
 *
 * Thread safety: Not thread-safe. Single-threaded test use only.
 */

#ifndef SIMS3000_TEST_TESTHARNESS_H
#define SIMS3000_TEST_TESTHARNESS_H

#include "sims3000/test/TestServer.h"
#include "sims3000/test/TestClient.h"
#include "sims3000/test/StateDiffer.h"
#include <vector>
#include <memory>
#include <functional>
#include <string>

namespace sims3000 {

/**
 * @struct HarnessConfig
 * @brief Configuration for TestHarness.
 */
struct HarnessConfig {
    /// Map size for the test server
    MapSizeTier mapSize = MapSizeTier::Small;

    /// Network conditions for all connections
    NetworkConditions networkConditions = ConnectionQualityProfiles::PERFECT;

    /// Maximum number of clients
    std::uint8_t maxClients = 4;

    /// Server port (0 for automatic)
    std::uint16_t serverPort = 0;

    /// Random seed for deterministic behavior (0 for random)
    std::uint64_t seed = 0;

    /// Enable headless mode for all components
    bool headless = true;

    /// State diff options
    DiffOptions diffOptions = DiffOptions::defaults();

    /// Default timeout for wait operations (milliseconds)
    std::uint32_t defaultTimeoutMs = 5000;
};

/**
 * @struct SyncResult
 * @brief Result of a sync operation.
 */
struct SyncResult {
    bool success = false;
    std::uint32_t ticksElapsed = 0;
    std::uint32_t timeElapsedMs = 0;
    std::string message;

    explicit operator bool() const { return success; }
};

/**
 * @struct StateMatchResult
 * @brief Result of state comparison.
 */
struct StateMatchResult {
    bool allMatch = false;
    std::vector<std::vector<StateDifference>> clientDifferences;
    std::string summary;

    explicit operator bool() const { return allMatch; }
};

/**
 * @class TestHarness
 * @brief Orchestrator for multi-client test scenarios.
 *
 * Example usage:
 * @code
 *   TestHarness harness;
 *   harness.createServer();
 *   harness.createClients(2);
 *   harness.connectAllClients();
 *
 *   // Run scenario
 *   harness.getClient(0)->placeBuilding({10, 10}, 1);
 *   harness.advanceTicks(5);
 *
 *   // Verify state
 *   auto result = harness.assertStateMatch();
 *   ASSERT_TRUE(result.allMatch) << result.summary;
 * @endcode
 */
class TestHarness {
public:
    /**
     * @brief Construct a TestHarness with default configuration.
     */
    TestHarness();

    /**
     * @brief Construct a TestHarness with specified configuration.
     * @param config Harness configuration.
     */
    explicit TestHarness(const HarnessConfig& config);

    /**
     * @brief Destructor. Stops server and disconnects all clients.
     */
    ~TestHarness();

    // Non-copyable
    TestHarness(const TestHarness&) = delete;
    TestHarness& operator=(const TestHarness&) = delete;

    // =========================================================================
    // Setup
    // =========================================================================

    /**
     * @brief Set the map size for the server.
     * @param tier Map size tier.
     *
     * Must be called before createServer().
     */
    void setMapSize(MapSizeTier tier);

    /**
     * @brief Set network conditions for all connections.
     * @param conditions Network condition preset or custom conditions.
     */
    void setNetworkConditions(const NetworkConditions& conditions);

    /**
     * @brief Create the test server.
     * @return true if server created and started successfully.
     */
    bool createServer();

    /**
     * @brief Create test clients.
     * @param count Number of clients to create.
     * @return true if all clients created successfully.
     */
    bool createClients(std::size_t count);

    /**
     * @brief Connect all clients to the server.
     * @param timeoutMs Connection timeout per client.
     * @return true if all clients connected successfully.
     */
    bool connectAllClients(std::uint32_t timeoutMs = 0);

    /**
     * @brief Disconnect all clients from the server.
     */
    void disconnectAllClients();

    // =========================================================================
    // Time Control
    // =========================================================================

    /**
     * @brief Update all components by one frame.
     * @param deltaTime Frame delta time in seconds.
     */
    void update(float deltaTime);

    /**
     * @brief Advance all components by specified number of ticks.
     * @param ticks Number of ticks to advance.
     */
    void advanceTicks(std::uint32_t ticks);

    /**
     * @brief Advance all components by specified time.
     * @param ms Time to advance in milliseconds.
     */
    void advanceTime(std::uint32_t ms);

    // =========================================================================
    // Synchronization
    // =========================================================================

    /**
     * @brief Wait until all clients are synced with the server.
     * @param timeoutMs Maximum time to wait.
     * @return Sync result.
     *
     * Clients are considered synced when:
     * - All pending state updates have been processed
     * - Entity counts match between server and all clients
     */
    SyncResult waitForSync(std::uint32_t timeoutMs = 0);

    /**
     * @brief Flush all network buffers to ensure messages are delivered.
     */
    void flushAll();

    // =========================================================================
    // State Verification
    // =========================================================================

    /**
     * @brief Compare server state with all client states.
     * @return State match result with differences.
     */
    StateMatchResult assertStateMatch();

    /**
     * @brief Compare server state with a specific client.
     * @param clientIndex Client index to compare.
     * @return Differences found.
     */
    std::vector<StateDifference> compareWithClient(std::size_t clientIndex);

    /**
     * @brief Get the state differ for custom comparisons.
     */
    StateDiffer& getStateDiffer() { return m_differ; }

    // =========================================================================
    // Accessors
    // =========================================================================

    /**
     * @brief Get the test server.
     */
    TestServer* getServer() { return m_server.get(); }
    const TestServer* getServer() const { return m_server.get(); }

    /**
     * @brief Get a specific client by index.
     * @param index Client index (0-based).
     */
    TestClient* getClient(std::size_t index);
    const TestClient* getClient(std::size_t index) const;

    /**
     * @brief Get all clients.
     */
    std::vector<TestClient*> getAllClients();

    /**
     * @brief Get client count.
     */
    std::size_t getClientCount() const { return m_clients.size(); }

    /**
     * @brief Get connected client count.
     */
    std::size_t getConnectedClientCount() const;

    /**
     * @brief Get harness configuration.
     */
    const HarnessConfig& getConfig() const { return m_config; }

    // =========================================================================
    // Test Helpers
    // =========================================================================

    /**
     * @brief Shortcut to have a client perform an action.
     * @param clientIndex Client to use.
     * @param action Action callback receiving the client.
     */
    void withClient(std::size_t clientIndex,
                    std::function<void(TestClient&)> action);

    /**
     * @brief Shortcut to have all clients perform an action.
     * @param action Action callback receiving each client.
     */
    void withAllClients(std::function<void(TestClient&, std::size_t index)> action);

    /**
     * @brief Check if all clients are connected.
     */
    bool allClientsConnected() const;

    /**
     * @brief Get current server tick.
     */
    SimulationTick getCurrentTick() const;

private:
    /// Get default timeout from config
    std::uint32_t getDefaultTimeout() const;

    HarnessConfig m_config;
    std::unique_ptr<TestServer> m_server;
    std::vector<std::unique_ptr<TestClient>> m_clients;
    StateDiffer m_differ;
};

} // namespace sims3000

#endif // SIMS3000_TEST_TESTHARNESS_H
