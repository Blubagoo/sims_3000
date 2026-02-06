/**
 * @file NetworkServer.h
 * @brief Server-side network manager for multiplayer game hosting.
 *
 * NetworkServer provides:
 * - ENet host listening on configurable port (default 7777)
 * - Client connection management (max 4 per canon)
 * - Per-client state tracking (connection status, PlayerID, heartbeat)
 * - Message routing to handlers via INetworkHandler interface
 * - State update broadcasting to all connected clients
 * - Heartbeat sending and timeout detection
 *
 * Server state machine: INITIALIZING -> LOADING -> READY -> RUNNING
 *
 * Ownership: Application owns NetworkServer.
 *            NetworkServer owns NetworkThread and INetworkTransport.
 * Cleanup: Destructor stops network thread and disconnects all clients.
 *
 * Thread safety:
 * - All public methods must be called from the main thread only.
 * - Internal network I/O runs on a dedicated thread via NetworkThread.
 */

#ifndef SIMS3000_NET_NETWORKSERVER_H
#define SIMS3000_NET_NETWORKSERVER_H

#include "sims3000/net/INetworkTransport.h"
#include "sims3000/net/INetworkHandler.h"
#include "sims3000/net/NetworkThread.h"
#include "sims3000/net/ServerMessages.h"
#include "sims3000/net/ClientMessages.h"
#include "sims3000/net/RateLimiter.h"
#include "sims3000/net/ConnectionValidator.h"
#include "sims3000/core/types.h"
#include <array>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <chrono>

namespace sims3000 {

// Forward declarations
class NetworkMessage;

/**
 * @enum ServerNetworkState
 * @brief Server operational states.
 */
enum class ServerNetworkState : std::uint8_t {
    Initializing = 0,  // Server starting up
    Loading = 1,       // Loading world/resources
    Ready = 2,         // Ready to accept connections
    Running = 3        // Game actively running
};

/// Session token size in bytes (128-bit = 16 bytes).
constexpr std::size_t SERVER_SESSION_TOKEN_SIZE = 16;

/// Reconnection grace period in milliseconds (30 seconds).
constexpr std::uint64_t SESSION_GRACE_PERIOD_MS = 30000;

/**
 * @struct ServerConfig
 * @brief Configuration for NetworkServer.
 */
struct ServerConfig {
    std::uint16_t port = 7777;           // Listen port
    std::uint8_t maxPlayers = 4;         // Max simultaneous clients
    MapSizeTier mapSize = MapSizeTier::Medium;  // Map size tier
    std::uint8_t tickRate = 20;          // Simulation ticks per second
    std::string serverName = "ZergCity Server";  // Server display name
    std::uint64_t sessionGracePeriodMs = SESSION_GRACE_PERIOD_MS;  // Reconnect grace period
};

/**
 * @struct PlayerSession
 * @brief Session data for reconnection support.
 *
 * Stores the session token and timing information needed to validate
 * reconnection attempts and track session expiration.
 */
struct PlayerSession {
    /// 128-bit session token for reconnection.
    std::array<std::uint8_t, SERVER_SESSION_TOKEN_SIZE> token{};

    /// Player ID assigned to this session.
    PlayerID playerId = 0;

    /// Player name for verification.
    std::string playerName;

    /// Timestamp when session was created (real-world ms).
    std::uint64_t createdAt = 0;

    /// Timestamp when player disconnected (0 if connected).
    std::uint64_t disconnectedAt = 0;

    /// Whether the session is currently connected.
    bool connected = false;

    /// Check if session token matches.
    bool tokenMatches(const std::array<std::uint8_t, SERVER_SESSION_TOKEN_SIZE>& other) const;

    /// Check if session is within grace period for reconnection.
    bool isWithinGracePeriod(std::uint64_t currentTimeMs, std::uint64_t gracePeriodMs) const;
};

/**
 * @struct ClientConnection
 * @brief Per-client connection state.
 */
struct ClientConnection {
    PeerID peer = INVALID_PEER_ID;       // Network peer ID
    PlayerID playerId = 0;               // Assigned player ID (1-255)
    std::string playerName;              // Player display name
    PlayerStatus status = PlayerStatus::Connecting;  // Connection status

    // Session management
    std::array<std::uint8_t, SERVER_SESSION_TOKEN_SIZE> sessionToken{};  // 128-bit session token
    std::uint64_t sessionCreatedAt = 0;   // When session was created (real-world ms)

    // Heartbeat tracking
    std::uint64_t lastHeartbeatReceived = 0;  // Timestamp of last heartbeat from client
    std::uint64_t lastHeartbeatSent = 0;      // Timestamp of last heartbeat sent to client
    std::uint32_t missedHeartbeats = 0;       // Consecutive missed heartbeats
    std::uint32_t heartbeatSequence = 0;      // Last received heartbeat sequence
    std::uint32_t serverHeartbeatSequence = 0;// Server-initiated heartbeat sequence

    // Activity tracking (real-world time, not ticks - per Q012)
    std::uint64_t lastActivityMs = 0;         // Last activity timestamp for ghost town timer

    // Statistics
    std::uint32_t latencyMs = 0;              // Measured round-trip time
};

/**
 * @class NetworkServer
 * @brief Server-side network management for multiplayer hosting.
 *
 * Example usage:
 * @code
 *   ServerConfig config;
 *   config.port = 7777;
 *   config.maxPlayers = 4;
 *   config.mapSize = MapSizeTier::Medium;
 *
 *   NetworkServer server(std::make_unique<ENetTransport>(), config);
 *
 *   if (!server.start()) {
 *       SDL_Log("Failed to start server");
 *       return 1;
 *   }
 *
 *   // Register message handlers
 *   server.registerHandler(&inputHandler);
 *   server.registerHandler(&chatHandler);
 *
 *   // Main loop
 *   while (running) {
 *       server.update(deltaTime);
 *
 *       // Simulation tick
 *       if (tickReady) {
 *           StateUpdateMessage stateUpdate;
 *           // ... populate deltas ...
 *           server.broadcastStateUpdate(stateUpdate);
 *       }
 *   }
 *
 *   server.stop();
 * @endcode
 */
class NetworkServer {
public:
    /// Maximum players per server (canon)
    static constexpr std::uint8_t MAX_PLAYERS = 4;

    /// Heartbeat interval in seconds
    static constexpr float HEARTBEAT_INTERVAL_SEC = 1.0f;

    /// Heartbeats missed before warning (5 = 5 seconds)
    static constexpr std::uint32_t HEARTBEAT_WARNING_THRESHOLD = 5;

    /// Heartbeats missed before disconnect (10 = 10 seconds)
    static constexpr std::uint32_t HEARTBEAT_DISCONNECT_THRESHOLD = 10;

    /**
     * @brief Construct a NetworkServer with the given transport.
     * @param transport Network transport implementation (usually ENetTransport).
     * @param config Server configuration.
     *
     * The transport is moved into the NetworkThread for I/O handling.
     */
    NetworkServer(std::unique_ptr<INetworkTransport> transport,
                  const ServerConfig& config = {});

    /**
     * @brief Destructor. Stops server if running.
     */
    ~NetworkServer();

    // Non-copyable, non-movable
    NetworkServer(const NetworkServer&) = delete;
    NetworkServer& operator=(const NetworkServer&) = delete;
    NetworkServer(NetworkServer&&) = delete;
    NetworkServer& operator=(NetworkServer&&) = delete;

    // =========================================================================
    // Lifecycle Methods
    // =========================================================================

    /**
     * @brief Start the server, begin listening for connections.
     * @return true if server started successfully.
     *
     * Transitions state: INITIALIZING -> LOADING -> READY
     */
    bool start();

    /**
     * @brief Stop the server, disconnect all clients.
     *
     * Sends graceful disconnect to all clients before closing.
     */
    void stop();

    /**
     * @brief Check if server is running.
     */
    bool isRunning() const { return m_running; }

    /**
     * @brief Get current server state.
     */
    ServerNetworkState getState() const { return m_state; }

    /**
     * @brief Transition to running state (game started).
     */
    void setRunning();

    // =========================================================================
    // Update Methods (Call from Main Thread)
    // =========================================================================

    /**
     * @brief Process network events and update connections.
     * @param deltaTime Frame delta time in seconds.
     *
     * Must be called each frame from the main thread.
     * - Polls inbound network events
     * - Routes messages to handlers
     * - Sends periodic heartbeats
     * - Checks for connection timeouts
     */
    void update(float deltaTime);

    // =========================================================================
    // Message Sending
    // =========================================================================

    /**
     * @brief Send a message to a specific client.
     * @param peer Target peer ID.
     * @param msg Message to send.
     * @param channel Channel to send on (default: Reliable).
     * @return true if message was queued for sending.
     */
    bool sendTo(PeerID peer, const NetworkMessage& msg,
                ChannelID channel = ChannelID::Reliable);

    /**
     * @brief Send a message to a specific player.
     * @param playerId Target player ID.
     * @param msg Message to send.
     * @param channel Channel to send on.
     * @return true if message was queued for sending.
     */
    bool sendToPlayer(PlayerID playerId, const NetworkMessage& msg,
                      ChannelID channel = ChannelID::Reliable);

    /**
     * @brief Broadcast a message to all connected clients.
     * @param msg Message to send.
     * @param channel Channel to send on.
     */
    void broadcast(const NetworkMessage& msg,
                   ChannelID channel = ChannelID::Reliable);

    /**
     * @brief Broadcast a state update to all connected clients.
     * @param msg StateUpdateMessage containing entity deltas.
     *
     * Called each simulation tick with changes since last tick.
     */
    void broadcastStateUpdate(const StateUpdateMessage& msg);

    /**
     * @brief Broadcast a chat message to all clients (from server).
     * @param text Message text.
     */
    void broadcastServerChat(const std::string& text);

    // =========================================================================
    // Handler Registration
    // =========================================================================

    /**
     * @brief Register a message handler.
     * @param handler Handler to register (not owned by server).
     *
     * Handlers are called for messages matching canHandle().
     */
    void registerHandler(INetworkHandler* handler);

    /**
     * @brief Unregister a message handler.
     * @param handler Handler to unregister.
     */
    void unregisterHandler(INetworkHandler* handler);

    // =========================================================================
    // Client Management
    // =========================================================================

    /**
     * @brief Get number of connected clients.
     */
    std::uint32_t getClientCount() const;

    /**
     * @brief Get list of all connected clients.
     */
    std::vector<ClientConnection> getClients() const;

    /**
     * @brief Get client by peer ID.
     * @return Pointer to client connection, or nullptr if not found.
     */
    const ClientConnection* getClient(PeerID peer) const;

    /**
     * @brief Get client by player ID.
     * @return Pointer to client connection, or nullptr if not found.
     */
    const ClientConnection* getClientByPlayerId(PlayerID playerId) const;

    /**
     * @brief Kick a client from the server.
     * @param playerId Player ID to kick.
     * @param reason Reason message to send.
     */
    void kickPlayer(PlayerID playerId, const std::string& reason = "Kicked by server");

    /**
     * @brief Kick a client by peer ID.
     * @param peer Peer ID to kick.
     * @param reason Reason message to send.
     */
    void kickPeer(PeerID peer, const std::string& reason = "Kicked by server");

    // =========================================================================
    // Session Management
    // =========================================================================

    /**
     * @brief Get session by token.
     * @param token Session token to look up.
     * @return Pointer to session, or nullptr if not found.
     */
    const PlayerSession* getSessionByToken(
        const std::array<std::uint8_t, SERVER_SESSION_TOKEN_SIZE>& token) const;

    /**
     * @brief Check if a session token is valid for reconnection.
     * @param token Session token to validate.
     * @return true if token is valid and within grace period.
     */
    bool isSessionValidForReconnect(
        const std::array<std::uint8_t, SERVER_SESSION_TOKEN_SIZE>& token) const;

    /**
     * @brief Get the number of active sessions (connected or within grace period).
     */
    std::uint32_t getActiveSessionCount() const;

    /**
     * @brief Update player activity timestamp (for ghost town timer).
     * @param playerId Player to update.
     */
    void updatePlayerActivity(PlayerID playerId);

    // =========================================================================
    // Server Information
    // =========================================================================

    /**
     * @brief Get server configuration.
     */
    const ServerConfig& getConfig() const { return m_config; }

    /**
     * @brief Get server uptime in seconds.
     */
    float getUptime() const { return m_uptime; }

    /**
     * @brief Get current simulation tick.
     */
    SimulationTick getCurrentTick() const { return m_currentTick; }

    /**
     * @brief Set current simulation tick (called by SimulationCore).
     */
    void setCurrentTick(SimulationTick tick) { m_currentTick = tick; }

    // =========================================================================
    // Error Handling Statistics (Ticket 1-018)
    // =========================================================================

    /**
     * @brief Get the rate limiter for statistics or configuration.
     */
    const RateLimiter& getRateLimiter() const { return m_rateLimiter; }

    /**
     * @brief Get the connection validator for statistics.
     */
    const ConnectionValidator& getValidator() const { return m_validator; }

    /**
     * @brief Get total messages dropped due to rate limiting.
     */
    std::uint64_t getRateLimitDropCount() const { return m_rateLimiter.getTotalDropped(); }

    /**
     * @brief Get total abuse events detected.
     */
    std::uint32_t getAbuseEventCount() const { return m_rateLimiter.getTotalAbuseEvents(); }

    /**
     * @brief Get validation statistics.
     */
    const ValidationStats& getValidationStats() const { return m_validator.getStats(); }

private:
    // =========================================================================
    // Internal Methods
    // =========================================================================

    /// Process incoming network events from NetworkThread
    void processInboundEvents();

    /// Handle a network connect event
    void handleConnect(PeerID peer);

    /// Handle a network disconnect event
    void handleDisconnect(PeerID peer, bool timedOut);

    /// Handle incoming message data
    void handleMessage(PeerID peer, const std::vector<std::uint8_t>& data);

    /// Route a deserialized message to appropriate handlers
    void routeMessage(PeerID peer, const NetworkMessage& msg);

    /// Handle system messages (Join, Heartbeat, etc.)
    void handleSystemMessage(PeerID peer, const NetworkMessage& msg);

    /// Process a Join request
    void handleJoinRequest(PeerID peer, const class JoinMessage& msg);

    /// Process a Reconnect request
    void handleReconnectRequest(PeerID peer, const class ReconnectMessage& msg);

    /// Process a Heartbeat message
    void handleHeartbeat(PeerID peer, const class HeartbeatMessage& msg);

    /// Generate a 128-bit random session token
    void generateSessionToken(std::array<std::uint8_t, SERVER_SESSION_TOKEN_SIZE>& token);

    /// Create a new session for a player
    void createSession(PlayerID playerId, const std::string& name,
                       const std::array<std::uint8_t, SERVER_SESSION_TOKEN_SIZE>& token);

    /// Find session by token
    PlayerSession* findSessionByToken(
        const std::array<std::uint8_t, SERVER_SESSION_TOKEN_SIZE>& token);

    /// Clean up expired sessions (past grace period)
    void cleanupExpiredSessions();

    /// Send JoinAccept message to a client
    void sendJoinAccept(PeerID peer, PlayerID playerId,
                        const std::array<std::uint8_t, SERVER_SESSION_TOKEN_SIZE>& token);

    /// Send JoinReject message to a client
    void sendJoinReject(PeerID peer, JoinRejectReason reason, const std::string& message);

    /// Handle duplicate connection (same token connects while existing connection active)
    void handleDuplicateConnection(PeerID existingPeer, PeerID newPeer);

    /// Send heartbeats to all clients
    void sendHeartbeats();

    /// Check for timed-out connections
    void checkTimeouts();

    /// Allocate a new PlayerID
    PlayerID allocatePlayerId();

    /// Free a PlayerID for reuse
    void freePlayerId(PlayerID id);

    /// Send server status to a specific client
    void sendServerStatus(PeerID peer);

    /// Broadcast updated player list to all clients
    void broadcastPlayerList();

    /// Serialize and queue a message for sending
    void queueMessage(PeerID peer, const NetworkMessage& msg, ChannelID channel);

    // =========================================================================
    // State
    // =========================================================================

    ServerConfig m_config;
    std::unique_ptr<NetworkThread> m_networkThread;
    ServerNetworkState m_state = ServerNetworkState::Initializing;
    bool m_running = false;
    float m_uptime = 0.0f;
    SimulationTick m_currentTick = 0;

    // Client tracking
    std::unordered_map<PeerID, ClientConnection> m_clients;
    std::unordered_map<PlayerID, PeerID> m_playerToPeer;
    std::vector<bool> m_usedPlayerIds;  // Index 0 unused (PlayerID starts at 1)

    // Session management
    std::vector<PlayerSession> m_sessions;  // All sessions (active and grace period)

    // Heartbeat timing
    float m_timeSinceHeartbeat = 0.0f;
    std::uint64_t m_currentTimeMs = 0;  // Monotonic time for heartbeat tracking

    // Message handlers
    std::vector<INetworkHandler*> m_handlers;

    // Error handling (Ticket 1-018)
    RateLimiter m_rateLimiter;
    ConnectionValidator m_validator;
};

/**
 * @brief Get string name for server network state.
 */
const char* getServerNetworkStateName(ServerNetworkState state);

} // namespace sims3000

#endif // SIMS3000_NET_NETWORKSERVER_H
