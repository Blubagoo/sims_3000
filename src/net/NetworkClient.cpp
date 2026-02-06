/**
 * @file NetworkClient.cpp
 * @brief Implementation of client-side network loop.
 */

#include "sims3000/net/NetworkClient.h"
#include "sims3000/net/ENetTransport.h"
#include "sims3000/core/Logger.h"
#include <algorithm>
#include <cstring>

namespace sims3000 {

// =============================================================================
// ConnectionState Helpers
// =============================================================================

const char* getConnectionStateName(ConnectionState state) {
    switch (state) {
        case ConnectionState::Disconnected:  return "Disconnected";
        case ConnectionState::Connecting:    return "Connecting";
        case ConnectionState::Connected:     return "Connected";
        case ConnectionState::Playing:       return "Playing";
        case ConnectionState::Reconnecting:  return "Reconnecting";
        default:                             return "Unknown";
    }
}

// =============================================================================
// Constructor / Destructor
// =============================================================================

NetworkClient::NetworkClient(
    std::unique_ptr<INetworkTransport> transport,
    const ConnectionConfig& config
)
    : m_config(config)
    , m_currentReconnectDelayMs(config.initialReconnectDelayMs)
{
    // Create network thread with the transport
    m_networkThread = std::make_unique<NetworkThread>(std::move(transport));

    // Initialize timing
    m_lastMessageTime = Clock::now();
    m_lastHeartbeatTime = Clock::now();
    m_lastReconnectAttempt = Clock::now();
    m_connectionStartTime = Clock::now();

    LOG_DEBUG("NetworkClient created");
}

NetworkClient::~NetworkClient() {
    disconnect();
    LOG_DEBUG("NetworkClient destroyed");
}

// =============================================================================
// Connection Control
// =============================================================================

bool NetworkClient::connect(const std::string& address, std::uint16_t port, const std::string& playerName) {
    if (m_state != ConnectionState::Disconnected) {
        LOG_WARN("Cannot connect: already in state %s", getConnectionStateName(m_state));
        return false;
    }

    if (address.empty()) {
        LOG_ERROR("Cannot connect: empty address");
        return false;
    }

    if (playerName.empty()) {
        LOG_ERROR("Cannot connect: empty player name");
        return false;
    }

    // Store connection parameters
    m_serverAddress = address;
    m_serverPort = port;
    m_playerName = playerName;

    // Start network thread
    m_networkThread->start();

    // Initiate connection
    if (!m_networkThread->connect(address, port)) {
        LOG_ERROR("Failed to queue connection to %s:%u", address.c_str(), port);
        m_networkThread->stop();
        return false;
    }

    // Transition to connecting state
    transitionTo(ConnectionState::Connecting);
    m_connectionStartTime = Clock::now();

    LOG_INFO("Establishing neural link to %s:%u...", address.c_str(), port);

    return true;
}

void NetworkClient::disconnect() {
    if (m_state == ConnectionState::Disconnected) {
        return;
    }

    LOG_INFO("Disconnecting from server...");

    // Stop any reconnection attempts
    m_stats.reconnectAttempts = 0;
    resetReconnectBackoff();

    // Disconnect from server
    if (m_serverPeer != INVALID_PEER_ID) {
        m_networkThread->disconnect(m_serverPeer);
    }

    // Stop network thread
    m_networkThread->stop();
    m_networkThread->join();

    // Reset state
    m_serverPeer = INVALID_PEER_ID;
    m_playerId = 0;

    // Clear queues
    while (!m_inputQueue.empty()) m_inputQueue.pop();
    while (!m_stateUpdateQueue.empty()) m_stateUpdateQueue.pop();

    transitionTo(ConnectionState::Disconnected);
}

// =============================================================================
// Update Loop
// =============================================================================

void NetworkClient::update(float deltaTime) {
    (void)deltaTime; // Currently using wall-clock timing

    // Process inbound network events
    processInboundEvents();

    // State-specific update logic
    switch (m_state) {
        case ConnectionState::Disconnected:
            // Nothing to do
            break;

        case ConnectionState::Connecting: {
            // Check for connection timeout
            auto now = Clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - m_connectionStartTime
            ).count();

            if (elapsed > m_config.connectTimeoutMs) {
                LOG_WARN("Connection timeout after %lld ms", static_cast<long long>(elapsed));
                transitionTo(ConnectionState::Reconnecting);
                m_lastReconnectAttempt = now;
            }
            break;
        }

        case ConnectionState::Connected:
            // Waiting for server to be ready (server in Loading state)
            // or waiting for JoinAccept
            updateTimeoutLevel();
            break;

        case ConnectionState::Playing:
            // Send queued inputs
            sendQueuedInputs();

            // Send heartbeat if needed
            {
                auto now = Clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                    now - m_lastHeartbeatTime
                ).count();

                if (elapsed >= m_config.heartbeatIntervalMs) {
                    sendHeartbeat();
                    m_lastHeartbeatTime = now;
                }
            }

            // Update timeout detection
            updateTimeoutLevel();
            break;

        case ConnectionState::Reconnecting:
            attemptReconnect();
            break;
    }
}

// =============================================================================
// Input Handling
// =============================================================================

void NetworkClient::queueInput(const InputMessage& input) {
    if (m_state != ConnectionState::Playing) {
        // Silently ignore inputs when not playing
        return;
    }

    // Create a copy with sequence number
    InputMessage sequencedInput = input;
    sequencedInput.sequenceNum = ++m_inputSequence;
    sequencedInput.playerId = m_playerId;

    m_inputQueue.push(sequencedInput);
}

// =============================================================================
// State Updates
// =============================================================================

bool NetworkClient::pollStateUpdate(StateUpdateMessage& outUpdate) {
    if (m_stateUpdateQueue.empty()) {
        return false;
    }

    outUpdate = std::move(m_stateUpdateQueue.front());
    m_stateUpdateQueue.pop();
    return true;
}

// =============================================================================
// Callbacks
// =============================================================================

void NetworkClient::setStateChangeCallback(StateChangeCallback callback) {
    m_stateChangeCallback = std::move(callback);
}

void NetworkClient::setServerStatusCallback(ServerStatusCallback callback) {
    m_serverStatusCallback = std::move(callback);
}

void NetworkClient::setRejectionCallback(RejectionCallback callback) {
    m_rejectionCallback = std::move(callback);
}

bool NetworkClient::pollRejection(RejectionMessage& outRejection) {
    if (m_rejectionQueue.empty()) {
        return false;
    }

    outRejection = std::move(m_rejectionQueue.front());
    m_rejectionQueue.pop();
    return true;
}

// =============================================================================
// State Transitions
// =============================================================================

void NetworkClient::transitionTo(ConnectionState newState) {
    if (m_state == newState) {
        return;
    }

    ConnectionState oldState = m_state;
    m_state = newState;

    LOG_INFO("Connection state: %s -> %s",
             getConnectionStateName(oldState),
             getConnectionStateName(newState));

    // Reset timeout level on state change
    m_stats.timeoutLevel = ConnectionTimeoutLevel::None;

    // Notify callback
    if (m_stateChangeCallback) {
        m_stateChangeCallback(oldState, newState);
    }
}

// =============================================================================
// Network Event Processing
// =============================================================================

void NetworkClient::processInboundEvents() {
    InboundNetworkEvent event;

    while (m_networkThread->pollInbound(event)) {
        switch (event.type) {
            case NetworkThreadEventType::Connect:
                handleConnectEvent(event.peer);
                break;

            case NetworkThreadEventType::Disconnect:
                handleDisconnectEvent(event.peer);
                break;

            case NetworkThreadEventType::Timeout:
                handleTimeoutEvent(event.peer);
                break;

            case NetworkThreadEventType::Message:
                if (event.peer == m_serverPeer) {
                    handleMessage(event.data);
                    m_lastMessageTime = Clock::now();
                    m_stats.messagesReceived++;
                }
                break;

            case NetworkThreadEventType::Error:
                LOG_ERROR("Network error occurred");
                if (m_state == ConnectionState::Connecting ||
                    m_state == ConnectionState::Connected ||
                    m_state == ConnectionState::Playing) {
                    transitionTo(ConnectionState::Reconnecting);
                    m_lastReconnectAttempt = Clock::now();
                }
                break;

            case NetworkThreadEventType::None:
                break;
        }
    }
}

void NetworkClient::handleConnectEvent(PeerID peer) {
    LOG_INFO("Connected to server (peer %u)", peer);

    m_serverPeer = peer;
    m_lastMessageTime = Clock::now();

    // Reset reconnection tracking on successful connect
    resetReconnectBackoff();
    m_stats.reconnectAttempts = 0;

    // Transition to connected state
    transitionTo(ConnectionState::Connected);

    // Send join message
    sendJoinMessage();
}

void NetworkClient::handleDisconnectEvent(PeerID peer) {
    if (peer != m_serverPeer) {
        return;
    }

    LOG_WARN("Disconnected from server");

    m_serverPeer = INVALID_PEER_ID;

    // If we were playing, attempt reconnection
    if (m_state == ConnectionState::Playing ||
        m_state == ConnectionState::Connected) {
        transitionTo(ConnectionState::Reconnecting);
        m_lastReconnectAttempt = Clock::now();
    } else if (m_state == ConnectionState::Connecting) {
        transitionTo(ConnectionState::Reconnecting);
        m_lastReconnectAttempt = Clock::now();
    }
}

void NetworkClient::handleTimeoutEvent(PeerID peer) {
    if (peer != m_serverPeer) {
        return;
    }

    LOG_WARN("Connection to server timed out");

    m_serverPeer = INVALID_PEER_ID;

    // Attempt reconnection
    if (m_state != ConnectionState::Disconnected) {
        transitionTo(ConnectionState::Reconnecting);
        m_lastReconnectAttempt = Clock::now();
    }
}

void NetworkClient::handleMessage(const std::vector<std::uint8_t>& data) {
    if (data.empty()) {
        return;
    }

    // Parse message envelope
    NetworkBuffer buffer(data.data(), data.size());
    EnvelopeHeader header = NetworkMessage::parseEnvelope(buffer);

    if (!header.isValid()) {
        LOG_WARN("Received invalid message envelope");
        return;
    }

    // Handle message by type
    switch (header.type) {
        case MessageType::JoinAccept:
            handleJoinAccept(buffer);
            break;

        case MessageType::JoinReject:
            handleJoinReject(buffer);
            break;

        case MessageType::HeartbeatResponse:
            handleHeartbeatResponse(buffer);
            break;

        case MessageType::ServerStatus:
            handleServerStatus(buffer);
            break;

        case MessageType::StateUpdate:
            handleStateUpdate(buffer);
            break;

        case MessageType::Kick:
            handleKick(buffer);
            break;

        case MessageType::PlayerList:
            handlePlayerList(buffer);
            break;

        case MessageType::Rejection:
            handleRejection(buffer);
            break;

        default:
            // Skip unknown message types
            NetworkMessage::skipPayload(buffer, header.payloadLength);
            LOG_DEBUG("Skipped unknown message type %u", static_cast<unsigned>(header.type));
            break;
    }
}

// =============================================================================
// Message Handlers
// =============================================================================

void NetworkClient::handleJoinAccept(NetworkBuffer& buffer) {
    // JoinAccept payload structure:
    // [1 byte] playerId
    // [16 bytes] sessionToken
    // [8 bytes] serverTick
    // Additional data may follow

    if (buffer.remaining() < 25) {
        LOG_WARN("JoinAccept message too short");
        return;
    }

    m_playerId = buffer.read_u8();

    // Skip session token (16 bytes) - used for reconnection
    for (int i = 0; i < 16; i++) {
        buffer.read_u8();
    }

    // Read u64 as two u32s (little-endian)
    std::uint32_t tickLow = buffer.read_u32();
    std::uint32_t tickHigh = buffer.read_u32();
    SimulationTick serverTick = (static_cast<std::uint64_t>(tickHigh) << 32) | tickLow;

    LOG_INFO("Join accepted! Player ID: %u, Server tick: %llu",
             m_playerId, static_cast<unsigned long long>(serverTick));

    // Transition to playing state
    transitionTo(ConnectionState::Playing);
}

void NetworkClient::handleJoinReject(NetworkBuffer& buffer) {
    // JoinReject payload:
    // [4 bytes] reason string length
    // [N bytes] reason string

    std::string reason = "Unknown reason";
    if (buffer.remaining() >= 4) {
        std::uint32_t reasonLen = buffer.read_u32();
        if (reasonLen > 0 && reasonLen <= 256 && buffer.remaining() >= reasonLen) {
            reason.resize(reasonLen);
            for (std::uint32_t i = 0; i < reasonLen; i++) {
                reason[i] = static_cast<char>(buffer.read_u8());
            }
        }
    }

    LOG_ERROR("Join rejected: %s", reason.c_str());

    // Disconnect completely (don't attempt reconnection)
    disconnect();
}

void NetworkClient::handleHeartbeatResponse(NetworkBuffer& buffer) {
    HeartbeatResponseMessage response;
    if (!response.deserializePayload(buffer)) {
        LOG_WARN("Failed to deserialize HeartbeatResponse");
        return;
    }

    // Calculate RTT from client timestamp
    updateRTT(response.clientTimestamp);
}

void NetworkClient::handleServerStatus(NetworkBuffer& buffer) {
    ServerStatusMessage status;
    if (!status.deserializePayload(buffer)) {
        LOG_WARN("Failed to deserialize ServerStatus");
        return;
    }

    m_serverStatus = status;

    LOG_DEBUG("Server status: state=%u, map=%ux%u, players=%u/%u",
              static_cast<unsigned>(status.state),
              status.mapWidth, status.mapHeight,
              status.currentPlayers, status.maxPlayers);

    // Notify callback
    if (m_serverStatusCallback) {
        m_serverStatusCallback(status);
    }

    // If server is loading and we're connected (not playing), show status
    if (status.state == ServerState::Loading && m_state == ConnectionState::Connected) {
        LOG_INFO("Server is loading... please wait");
    }
}

void NetworkClient::handleStateUpdate(NetworkBuffer& buffer) {
    StateUpdateMessage update;
    if (!update.deserializePayload(buffer)) {
        LOG_WARN("Failed to deserialize StateUpdate");
        return;
    }

    // Queue for SyncSystem to process
    m_stateUpdateQueue.push(std::move(update));
}

void NetworkClient::handleKick(NetworkBuffer& buffer) {
    // Kick payload:
    // [4 bytes] reason string length
    // [N bytes] reason string

    std::string reason = "Unknown reason";
    if (buffer.remaining() >= 4) {
        std::uint32_t reasonLen = buffer.read_u32();
        if (reasonLen > 0 && reasonLen <= 256 && buffer.remaining() >= reasonLen) {
            reason.resize(reasonLen);
            for (std::uint32_t i = 0; i < reasonLen; i++) {
                reason[i] = static_cast<char>(buffer.read_u8());
            }
        }
    }

    LOG_WARN("Kicked from server: %s", reason.c_str());

    // Disconnect completely (don't attempt reconnection)
    disconnect();
}

void NetworkClient::handlePlayerList(NetworkBuffer& buffer) {
    PlayerListMessage playerList;
    if (!playerList.deserializePayload(buffer)) {
        LOG_WARN("Failed to deserialize PlayerList");
        return;
    }

    LOG_DEBUG("Received player list with %zu players", playerList.players.size());

    // Player list is informational - could notify UI callback here
}

void NetworkClient::handleRejection(NetworkBuffer& buffer) {
    RejectionMessage rejection;
    if (!rejection.deserializePayload(buffer)) {
        LOG_WARN("Failed to deserialize RejectionMessage");
        return;
    }

    LOG_INFO("Action rejected - seq %u, reason: %s",
             rejection.inputSequenceNum, rejection.message.c_str());

    // Queue for polling
    m_rejectionQueue.push(rejection);

    // Notify callback if set
    if (m_rejectionCallback) {
        m_rejectionCallback(rejection);
    }
}

// =============================================================================
// Sending
// =============================================================================

void NetworkClient::sendQueuedInputs() {
    while (!m_inputQueue.empty()) {
        const InputMessage& input = m_inputQueue.front();

        // Wrap in NetInputMessage
        NetInputMessage netMsg;
        netMsg.input = input;

        sendMessage(netMsg, ChannelID::Reliable);

        m_inputQueue.pop();
    }
}

void NetworkClient::sendHeartbeat() {
    HeartbeatMessage heartbeat;

    // Use high-resolution clock for RTT measurement
    auto now = Clock::now();
    heartbeat.clientTimestamp = static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count()
    );
    heartbeat.clientSequence = ++m_heartbeatSequence;

    sendMessage(heartbeat, ChannelID::Reliable);
}

void NetworkClient::sendJoinMessage() {
    JoinMessage joinMsg;
    joinMsg.playerName = m_playerName;
    joinMsg.hasSessionToken = false; // New connection, no session token

    sendMessage(joinMsg, ChannelID::Reliable);

    LOG_DEBUG("Sent join message for player '%s'", m_playerName.c_str());
}

void NetworkClient::sendMessage(const NetworkMessage& message, ChannelID channel) {
    if (m_serverPeer == INVALID_PEER_ID) {
        return;
    }

    // Serialize message with envelope
    NetworkBuffer buffer;
    message.serializeWithEnvelope(buffer);

    // Queue for network thread
    OutboundNetworkMessage outMsg;
    outMsg.peer = m_serverPeer;
    outMsg.data.assign(buffer.data(), buffer.data() + buffer.size());
    outMsg.channel = channel;
    outMsg.broadcast = false;

    if (m_networkThread->enqueueOutbound(std::move(outMsg))) {
        m_stats.messagesSent++;
    } else {
        LOG_WARN("Failed to enqueue outbound message (queue full?)");
    }
}

// =============================================================================
// Reconnection Logic
// =============================================================================

void NetworkClient::attemptReconnect() {
    auto now = Clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - m_lastReconnectAttempt
    ).count();

    if (elapsed < m_currentReconnectDelayMs) {
        // Not time to retry yet
        return;
    }

    m_stats.reconnectAttempts++;
    m_lastReconnectAttempt = now;

    LOG_INFO("Attempting reconnection (attempt %u, delay %u ms)",
             m_stats.reconnectAttempts, m_currentReconnectDelayMs);

    // Calculate next delay with exponential backoff
    m_currentReconnectDelayMs = calculateNextReconnectDelay();

    // Attempt connection
    if (!m_networkThread->connect(m_serverAddress, m_serverPort)) {
        LOG_WARN("Failed to queue reconnection attempt");
        return;
    }

    m_connectionStartTime = now;
    transitionTo(ConnectionState::Connecting);
}

void NetworkClient::resetReconnectBackoff() {
    m_currentReconnectDelayMs = m_config.initialReconnectDelayMs;
}

std::uint32_t NetworkClient::calculateNextReconnectDelay() {
    // Exponential backoff: double the delay each time, up to max
    std::uint32_t nextDelay = m_currentReconnectDelayMs * 2;
    return std::min(nextDelay, m_config.maxReconnectDelayMs);
}

// =============================================================================
// Timeout Detection
// =============================================================================

void NetworkClient::updateTimeoutLevel() {
    auto now = Clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - m_lastMessageTime
    ).count();

    m_stats.timeSinceLastMessageMs = static_cast<std::uint64_t>(elapsed);

    // Determine timeout level
    ConnectionTimeoutLevel newLevel = ConnectionTimeoutLevel::None;

    if (elapsed >= m_config.timeoutFullUIMs) {
        newLevel = ConnectionTimeoutLevel::FullUI;
    } else if (elapsed >= m_config.timeoutBannerMs) {
        newLevel = ConnectionTimeoutLevel::Banner;
    } else if (elapsed >= m_config.timeoutIndicatorMs) {
        newLevel = ConnectionTimeoutLevel::Indicator;
    }

    // Log level changes
    if (newLevel != m_stats.timeoutLevel) {
        switch (newLevel) {
            case ConnectionTimeoutLevel::Indicator:
                LOG_DEBUG("Connection timeout: showing indicator");
                break;
            case ConnectionTimeoutLevel::Banner:
                LOG_WARN("Connection timeout: showing banner");
                break;
            case ConnectionTimeoutLevel::FullUI:
                LOG_WARN("Connection timeout: showing full UI");
                break;
            case ConnectionTimeoutLevel::None:
                if (m_stats.timeoutLevel != ConnectionTimeoutLevel::None) {
                    LOG_DEBUG("Connection timeout: recovered");
                }
                break;
        }
    }

    m_stats.timeoutLevel = newLevel;
}

// =============================================================================
// RTT Calculation
// =============================================================================

void NetworkClient::updateRTT(std::uint64_t clientTimestamp) {
    auto now = Clock::now();
    auto currentMs = static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count()
    );

    // Calculate round-trip time
    if (currentMs > clientTimestamp) {
        std::uint32_t rtt = static_cast<std::uint32_t>(currentMs - clientTimestamp);
        m_stats.rttMs = rtt;

        // Exponential moving average for smoothed RTT (alpha = 0.2)
        if (m_stats.smoothedRttMs == 0) {
            m_stats.smoothedRttMs = rtt;
        } else {
            m_stats.smoothedRttMs = static_cast<std::uint32_t>(
                0.8f * m_stats.smoothedRttMs + 0.2f * rtt
            );
        }
    }
}

} // namespace sims3000
