/**
 * @file TestServer.h
 * @brief Lightweight server wrapper with state inspection for testing.
 *
 * Wraps NetworkServer to provide:
 * - State inspection methods
 * - Deterministic tick control
 * - Client connection tracking
 * - Configurable map size parameter
 *
 * Usage:
 *   TestServer server;
 *   server.start(0, 4);  // Port 0 for automatic assignment
 *
 *   // Inspect state
 *   auto clientCount = server.getClientCount();
 *   auto& registry = server.getRegistry();
 *
 * Ownership: Test code owns TestServer.
 * Cleanup: Destructor stops server and cleans up resources.
 *
 * Thread safety: Not thread-safe. Single-threaded test use only.
 */

#ifndef SIMS3000_TEST_TESTSERVER_H
#define SIMS3000_TEST_TESTSERVER_H

#include "sims3000/net/NetworkServer.h"
#include "sims3000/test/MockSocket.h"
#include "sims3000/ecs/Registry.h"
#include "sims3000/net/ServerMessages.h"
#include <memory>
#include <functional>
#include <vector>
#include <string>

namespace sims3000 {

/**
 * @struct TestServerConfig
 * @brief Configuration for TestServer.
 */
struct TestServerConfig {
    /// Port to listen on (0 for automatic assignment)
    std::uint16_t port = 0;

    /// Maximum number of clients
    std::uint8_t maxPlayers = 4;

    /// Map size tier
    MapSizeTier mapSize = MapSizeTier::Small;

    /// Network conditions for the mock socket
    NetworkConditions networkConditions = ConnectionQualityProfiles::PERFECT;

    /// Random seed for deterministic behavior (0 for random)
    std::uint64_t seed = 0;

    /// Server name
    std::string serverName = "TestServer";

    /// Enable headless mode (no SDL initialization)
    bool headless = true;
};

/**
 * @class TestServer
 * @brief Lightweight server wrapper for testing.
 *
 * Provides a simplified interface for testing server functionality
 * without full game initialization.
 */
class TestServer {
public:
    /**
     * @brief Construct a TestServer with default configuration.
     */
    TestServer();

    /**
     * @brief Construct a TestServer with specified configuration.
     * @param config Server configuration.
     */
    explicit TestServer(const TestServerConfig& config);

    /**
     * @brief Destructor. Stops server if running.
     */
    ~TestServer();

    // Non-copyable
    TestServer(const TestServer&) = delete;
    TestServer& operator=(const TestServer&) = delete;

    // =========================================================================
    // Lifecycle
    // =========================================================================

    /**
     * @brief Start the server.
     * @return true if server started successfully.
     */
    bool start();

    /**
     * @brief Start the server with custom port and max players.
     * @param port Port to listen on (0 for automatic).
     * @param maxPlayers Maximum number of clients.
     * @return true if server started successfully.
     */
    bool start(std::uint16_t port, std::uint8_t maxPlayers);

    /**
     * @brief Stop the server.
     */
    void stop();

    /**
     * @brief Check if server is running.
     */
    bool isRunning() const;

    /**
     * @brief Get the assigned port (after start with port 0).
     */
    std::uint16_t getPort() const;

    // =========================================================================
    // Update Control
    // =========================================================================

    /**
     * @brief Update the server by one frame.
     * @param deltaTime Frame delta time in seconds.
     */
    void update(float deltaTime);

    /**
     * @brief Advance server by specified number of ticks.
     * @param ticks Number of ticks to advance.
     *
     * Calls update() with appropriate delta time for each tick.
     */
    void advanceTicks(std::uint32_t ticks);

    /**
     * @brief Get current simulation tick.
     */
    SimulationTick getCurrentTick() const;

    /**
     * @brief Set current simulation tick.
     * @param tick New tick value.
     */
    void setCurrentTick(SimulationTick tick);

    // =========================================================================
    // State Inspection
    // =========================================================================

    /**
     * @brief Get the ECS registry for state inspection/manipulation.
     */
    Registry& getRegistry();
    const Registry& getRegistry() const;

    /**
     * @brief Get connected client count.
     */
    std::uint32_t getClientCount() const;

    /**
     * @brief Get list of connected client peer IDs.
     */
    std::vector<PeerID> getClientPeers() const;

    /**
     * @brief Get list of connected player IDs.
     */
    std::vector<PlayerID> getPlayerIds() const;

    /**
     * @brief Check if a player ID is connected.
     * @param playerId Player ID to check.
     */
    bool isPlayerConnected(PlayerID playerId) const;

    /**
     * @brief Get server configuration.
     */
    const TestServerConfig& getConfig() const { return m_config; }

    /**
     * @brief Get the mock socket for direct manipulation.
     */
    MockSocket* getMockSocket();

    /**
     * @brief Get the underlying NetworkServer.
     */
    NetworkServer* getNetworkServer();

    // =========================================================================
    // Entity Management
    // =========================================================================

    /**
     * @brief Create an entity with standard components for testing.
     * @param pos Grid position.
     * @param owner Owner player ID.
     * @return Created entity ID.
     */
    EntityID createTestEntity(GridPosition pos, PlayerID owner = 0);

    /**
     * @brief Create a building entity.
     * @param pos Grid position.
     * @param buildingType Building type ID.
     * @param owner Owner player ID.
     * @return Created entity ID.
     */
    EntityID createBuilding(GridPosition pos, std::uint32_t buildingType,
                            PlayerID owner = 0);

    /**
     * @brief Get total entity count in registry.
     */
    std::size_t getEntityCount() const;

    // =========================================================================
    // Test Helpers
    // =========================================================================

    /**
     * @brief Simulate a client connecting.
     * @param playerName Player name.
     * @return Assigned player ID, or 0 if failed.
     */
    PlayerID simulateClientConnect(const std::string& playerName);

    /**
     * @brief Simulate a client disconnecting.
     * @param playerId Player to disconnect.
     */
    void simulateClientDisconnect(PlayerID playerId);

    /**
     * @brief Process all pending network events.
     */
    void processNetworkEvents();

    /**
     * @brief Flush all outgoing network messages.
     */
    void flushNetwork();

private:
    TestServerConfig m_config;
    std::unique_ptr<MockSocket> m_socket;
    std::unique_ptr<NetworkServer> m_server;
    Registry m_registry;
    SimulationTick m_currentTick = 0;
    bool m_running = false;
};

} // namespace sims3000

#endif // SIMS3000_TEST_TESTSERVER_H
