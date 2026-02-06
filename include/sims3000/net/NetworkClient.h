/**
 * @file NetworkClient.h
 * @brief Client-side network loop for connecting to server and exchanging messages.
 *
 * NetworkClient manages the client-side network connection lifecycle:
 * - Connection state machine (Disconnected -> Connecting -> Connected -> Playing)
 * - Automatic reconnection with exponential backoff
 * - Heartbeat sending and RTT measurement
 * - Input message queuing and transmission
 * - State update reception and queuing for SyncSystem
 *
 * Architecture:
 * - Uses NetworkThread for non-blocking network I/O
 * - Main thread calls update() each frame to process events
 * - Input messages queued via queueInput() and sent during update()
 * - Received state updates available via pollStateUpdate()
 *
 * Ownership: Application owns NetworkClient.
 * Cleanup: Destructor disconnects gracefully and stops network thread.
 *
 * Thread safety:
 * - All public methods must be called from main thread only
 * - Network thread handles actual I/O (via NetworkThread)
 */

#ifndef SIMS3000_NET_NETWORKCLIENT_H
#define SIMS3000_NET_NETWORKCLIENT_H

#include "sims3000/net/NetworkThread.h"
#include "sims3000/net/NetworkMessage.h"
#include "sims3000/net/ClientMessages.h"
#include "sims3000/net/ServerMessages.h"
#include "sims3000/net/InputMessage.h"
#include "sims3000/core/types.h"
#include <memory>
#include <string>
#include <queue>
#include <chrono>
#include <functional>
#include <optional>
#include <cstdint>

namespace sims3000 {

// Forward declarations
class INetworkTransport;

/**
 * @enum ConnectionState
 * @brief Client connection state machine states.
 */
enum class ConnectionState : std::uint8_t {
    Disconnected = 0,   ///< Not connected, idle
    Connecting,         ///< Connection attempt in progress
    Connected,          ///< TCP connected, waiting for server ready
    Playing,            ///< Fully connected and playing
    Reconnecting        ///< Connection lost, attempting to reconnect
};

/**
 * @brief Get human-readable name for a connection state.
 */
const char* getConnectionStateName(ConnectionState state);

/**
 * @enum ConnectionTimeoutLevel
 * @brief Levels of connection timeout severity.
 *
 * Per ticket: 2s = indicator, 5s = banner, 15s = full UI
 */
enum class ConnectionTimeoutLevel : std::uint8_t {
    None = 0,           ///< Connection healthy
    Indicator,          ///< 2+ seconds since last server message (subtle indicator)
    Banner,             ///< 5+ seconds (warning banner)
    FullUI              ///< 15+ seconds (full reconnection UI)
};

/**
 * @struct ConnectionConfig
 * @brief Configuration parameters for NetworkClient.
 */
struct ConnectionConfig {
    /// Initial reconnection delay in milliseconds
    std::uint32_t initialReconnectDelayMs = 2000;

    /// Maximum reconnection delay in milliseconds
    std::uint32_t maxReconnectDelayMs = 30000;

    /// Heartbeat interval in milliseconds
    std::uint32_t heartbeatIntervalMs = 1000;

    /// Connection timeout for indicator (milliseconds)
    std::uint32_t timeoutIndicatorMs = 2000;

    /// Connection timeout for banner (milliseconds)
    std::uint32_t timeoutBannerMs = 5000;

    /// Connection timeout for full UI (milliseconds)
    std::uint32_t timeoutFullUIMs = 15000;

    /// Connection attempt timeout (milliseconds)
    std::uint32_t connectTimeoutMs = 10000;
};

/**
 * @struct ConnectionStats
 * @brief Connection statistics for display.
 */
struct ConnectionStats {
    /// Round-trip time in milliseconds (0 if not measured)
    std::uint32_t rttMs = 0;

    /// Smoothed RTT for display (exponential moving average)
    std::uint32_t smoothedRttMs = 0;

    /// Number of reconnection attempts since last successful connection
    std::uint32_t reconnectAttempts = 0;

    /// Time since last message received from server (milliseconds)
    std::uint64_t timeSinceLastMessageMs = 0;

    /// Total messages sent
    std::uint64_t messagesSent = 0;

    /// Total messages received
    std::uint64_t messagesReceived = 0;

    /// Current timeout level
    ConnectionTimeoutLevel timeoutLevel = ConnectionTimeoutLevel::None;
};

/**
 * @class NetworkClient
 * @brief Client-side network loop manager.
 *
 * Example usage:
 * @code
 *   auto transport = std::make_unique<ENetTransport>();
 *   NetworkClient client(std::move(transport));
 *
 *   client.connect("127.0.0.1", 7777, "PlayerName");
 *
 *   // Game loop
 *   while (running) {
 *       float dt = getDeltaTime();
 *       client.update(dt);
 *
 *       if (client.getState() == ConnectionState::Playing) {
 *           // Queue input from InputSystem
 *           if (hasInput) {
 *               client.queueInput(inputMessage);
 *           }
 *
 *           // Process received state updates
 *           StateUpdateMessage stateUpdate;
 *           while (client.pollStateUpdate(stateUpdate)) {
 *               syncSystem.applyUpdate(stateUpdate);
 *           }
 *       }
 *   }
 *
 *   client.disconnect();
 * @endcode
 */
class NetworkClient {
public:
    /// Callback type for connection state changes
    using StateChangeCallback = std::function<void(ConnectionState oldState, ConnectionState newState)>;

    /// Callback type for server status updates
    using ServerStatusCallback = std::function<void(const ServerStatusMessage& status)>;

    /**
     * @brief Construct a NetworkClient with the given transport.
     * @param transport Network transport implementation (ENetTransport or MockTransport)
     * @param config Connection configuration parameters
     */
    explicit NetworkClient(
        std::unique_ptr<INetworkTransport> transport,
        const ConnectionConfig& config = ConnectionConfig{}
    );

    /**
     * @brief Destructor. Disconnects and stops network thread.
     */
    ~NetworkClient();

    // Non-copyable, non-movable
    NetworkClient(const NetworkClient&) = delete;
    NetworkClient& operator=(const NetworkClient&) = delete;
    NetworkClient(NetworkClient&&) = delete;
    NetworkClient& operator=(NetworkClient&&) = delete;

    // =========================================================================
    // Connection Control
    // =========================================================================

    /**
     * @brief Initiate connection to a server.
     * @param address Server IP address or hostname
     * @param port Server port
     * @param playerName Player's display name
     * @return true if connection attempt was started
     *
     * Transitions to Connecting state. Connection result reported via
     * state change callback or getState() poll.
     */
    bool connect(const std::string& address, std::uint16_t port, const std::string& playerName);

    /**
     * @brief Disconnect from the server gracefully.
     *
     * Sends disconnect message, stops reconnection, and returns to Disconnected state.
     */
    void disconnect();

    /**
     * @brief Get current connection state.
     */
    ConnectionState getState() const { return m_state; }

    /**
     * @brief Check if currently connected and playing.
     */
    bool isPlaying() const { return m_state == ConnectionState::Playing; }

    /**
     * @brief Check if any connection or reconnection is in progress.
     */
    bool isConnecting() const {
        return m_state == ConnectionState::Connecting ||
               m_state == ConnectionState::Reconnecting;
    }

    /**
     * @brief Get connection statistics.
     */
    const ConnectionStats& getStats() const { return m_stats; }

    /**
     * @brief Get the assigned player ID (valid after JoinAccept).
     */
    PlayerID getPlayerId() const { return m_playerId; }

    /**
     * @brief Get the last received server status.
     */
    const ServerStatusMessage& getServerStatus() const { return m_serverStatus; }

    /**
     * @brief Check if the server is in loading state.
     */
    bool isServerLoading() const {
        return m_serverStatus.state == ServerState::Loading;
    }

    // =========================================================================
    // Update Loop
    // =========================================================================

    /**
     * @brief Update the network client.
     * @param deltaTime Time since last update in seconds
     *
     * Must be called every frame. Handles:
     * - Processing inbound network events
     * - Sending queued messages
     * - Heartbeat timing
     * - Reconnection logic
     * - Timeout detection
     */
    void update(float deltaTime);

    // =========================================================================
    // Input Handling
    // =========================================================================

    /**
     * @brief Queue an input message to be sent to the server.
     * @param input The input message to queue
     *
     * Messages are sent during the next update() call.
     * Ignored if not in Playing state.
     */
    void queueInput(const InputMessage& input);

    /**
     * @brief Get the number of pending input messages.
     */
    std::size_t getPendingInputCount() const { return m_inputQueue.size(); }

    // =========================================================================
    // State Updates
    // =========================================================================

    /**
     * @brief Poll for a received state update.
     * @param outUpdate Output parameter for the state update
     * @return true if an update was available, false if queue empty
     */
    bool pollStateUpdate(StateUpdateMessage& outUpdate);

    /**
     * @brief Get the number of pending state updates.
     */
    std::size_t getPendingStateUpdateCount() const { return m_stateUpdateQueue.size(); }

    // =========================================================================
    // Callbacks
    // =========================================================================

    /**
     * @brief Set callback for connection state changes.
     * @param callback Function to call when state changes
     */
    void setStateChangeCallback(StateChangeCallback callback);

    /**
     * @brief Set callback for server status updates.
     * @param callback Function to call when server status is received
     */
    void setServerStatusCallback(ServerStatusCallback callback);

    // =========================================================================
    // Rejection Handling
    // =========================================================================

    /// Callback type for rejection messages
    using RejectionCallback = std::function<void(const RejectionMessage&)>;

    /**
     * @brief Set callback for rejection messages.
     * @param callback Function to call when a rejection is received.
     */
    void setRejectionCallback(RejectionCallback callback);

    /**
     * @brief Poll for a received rejection message.
     * @param outRejection Output parameter for the rejection.
     * @return true if a rejection was available, false if queue empty.
     */
    bool pollRejection(RejectionMessage& outRejection);

    /**
     * @brief Get the number of pending rejections.
     */
    std::size_t getPendingRejectionCount() const { return m_rejectionQueue.size(); }

private:
    // State transitions
    void transitionTo(ConnectionState newState);

    // Network event processing
    void processInboundEvents();
    void handleConnectEvent(PeerID peer);
    void handleDisconnectEvent(PeerID peer);
    void handleTimeoutEvent(PeerID peer);
    void handleMessage(const std::vector<std::uint8_t>& data);

    // Message handlers by type
    void handleJoinAccept(NetworkBuffer& buffer);
    void handleJoinReject(NetworkBuffer& buffer);
    void handleHeartbeatResponse(NetworkBuffer& buffer);
    void handleServerStatus(NetworkBuffer& buffer);
    void handleStateUpdate(NetworkBuffer& buffer);
    void handleKick(NetworkBuffer& buffer);
    void handlePlayerList(NetworkBuffer& buffer);
    void handleRejection(NetworkBuffer& buffer);

    // Sending
    void sendQueuedInputs();
    void sendHeartbeat();
    void sendJoinMessage();
    void sendMessage(const NetworkMessage& message, ChannelID channel = ChannelID::Reliable);

    // Reconnection logic
    void attemptReconnect();
    void resetReconnectBackoff();
    std::uint32_t calculateNextReconnectDelay();

    // Timeout detection
    void updateTimeoutLevel();

    // RTT calculation
    void updateRTT(std::uint64_t clientTimestamp);

    // Network thread and transport
    std::unique_ptr<NetworkThread> m_networkThread;
    PeerID m_serverPeer = INVALID_PEER_ID;

    // Configuration
    ConnectionConfig m_config;

    // Connection state
    ConnectionState m_state = ConnectionState::Disconnected;
    std::string m_serverAddress;
    std::uint16_t m_serverPort = 0;
    std::string m_playerName;
    PlayerID m_playerId = 0;

    // Server status
    ServerStatusMessage m_serverStatus;

    // Message queues
    std::queue<InputMessage> m_inputQueue;
    std::queue<StateUpdateMessage> m_stateUpdateQueue;
    std::queue<RejectionMessage> m_rejectionQueue;

    // Timing
    using Clock = std::chrono::steady_clock;
    using TimePoint = std::chrono::time_point<Clock>;

    TimePoint m_lastMessageTime;           ///< Time of last message from server
    TimePoint m_lastHeartbeatTime;         ///< Time of last heartbeat sent
    TimePoint m_lastReconnectAttempt;      ///< Time of last reconnection attempt
    TimePoint m_connectionStartTime;       ///< Time when current connection attempt started

    // Reconnection backoff
    std::uint32_t m_currentReconnectDelayMs = 2000;

    // Statistics
    ConnectionStats m_stats;

    // Heartbeat tracking
    std::uint32_t m_heartbeatSequence = 0;

    // Input sequence tracking
    std::uint32_t m_inputSequence = 0;

    // Callbacks
    StateChangeCallback m_stateChangeCallback;
    ServerStatusCallback m_serverStatusCallback;
    RejectionCallback m_rejectionCallback;
};

} // namespace sims3000

#endif // SIMS3000_NET_NETWORKCLIENT_H
