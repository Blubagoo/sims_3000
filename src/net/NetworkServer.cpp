/**
 * @file NetworkServer.cpp
 * @brief NetworkServer implementation.
 */

#include "sims3000/net/NetworkServer.h"
#include "sims3000/net/NetworkMessage.h"
#include "sims3000/net/NetworkBuffer.h"
#include "sims3000/net/ClientMessages.h"
#include "sims3000/net/ServerMessages.h"
#include "sims3000/net/InputMessage.h"
#include <SDL3/SDL_log.h>
#include <chrono>
#include <algorithm>
#include <random>
#include <cstring>

namespace sims3000 {

const char* getServerNetworkStateName(ServerNetworkState state) {
    switch (state) {
        case ServerNetworkState::Initializing: return "Initializing";
        case ServerNetworkState::Loading: return "Loading";
        case ServerNetworkState::Ready: return "Ready";
        case ServerNetworkState::Running: return "Running";
        default: return "Unknown";
    }
}

// =============================================================================
// PlayerSession Implementation
// =============================================================================

bool PlayerSession::tokenMatches(const std::array<std::uint8_t, SERVER_SESSION_TOKEN_SIZE>& other) const {
    return std::memcmp(token.data(), other.data(), SERVER_SESSION_TOKEN_SIZE) == 0;
}

bool PlayerSession::isWithinGracePeriod(std::uint64_t currentTimeMs, std::uint64_t gracePeriodMs) const {
    if (connected) {
        return true;  // Connected sessions are always valid
    }
    if (disconnectedAt == 0) {
        return false;  // Never disconnected but not connected - invalid state
    }
    return (currentTimeMs - disconnectedAt) <= gracePeriodMs;
}

NetworkServer::NetworkServer(std::unique_ptr<INetworkTransport> transport,
                             const ServerConfig& config)
    : m_config(config)
{
    // Enforce max player limit
    if (m_config.maxPlayers > MAX_PLAYERS) {
        m_config.maxPlayers = MAX_PLAYERS;
    }

    // Initialize player ID tracking (index 0 unused since PlayerID starts at 1)
    m_usedPlayerIds.resize(static_cast<std::size_t>(m_config.maxPlayers) + 1, false);
    m_usedPlayerIds[0] = true;  // Mark 0 as used (invalid/GameMaster)

    // Create network thread with the transport
    m_networkThread = std::make_unique<NetworkThread>(std::move(transport));
}

NetworkServer::~NetworkServer() {
    stop();
}

bool NetworkServer::start() {
    if (m_running) {
        SDL_Log("NetworkServer: Already running");
        return true;
    }

    SDL_Log("NetworkServer: Starting on port %d (max %d players, map: %s)",
            m_config.port, m_config.maxPlayers,
            m_config.mapSize == MapSizeTier::Small ? "small" :
            m_config.mapSize == MapSizeTier::Medium ? "medium" : "large");

    // Request server start via NetworkThread
    if (!m_networkThread->startServer(m_config.port, m_config.maxPlayers)) {
        SDL_Log("NetworkServer: Failed to queue server start command");
        return false;
    }

    // Start the network thread
    m_networkThread->start();

    // Transition through states
    m_state = ServerNetworkState::Loading;
    SDL_Log("NetworkServer: State -> Loading");

    // For now, immediately go to Ready (loading would happen here in future)
    m_state = ServerNetworkState::Ready;
    SDL_Log("NetworkServer: State -> Ready");

    m_running = true;
    m_uptime = 0.0f;
    m_timeSinceHeartbeat = 0.0f;

    // Initialize time tracking
    m_currentTimeMs = static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count()
    );

    SDL_Log("NetworkServer: Started successfully");
    return true;
}

void NetworkServer::stop() {
    if (!m_running) {
        return;
    }

    SDL_Log("NetworkServer: Stopping...");

    // Notify all clients of shutdown
    ServerStatusMessage statusMsg;
    statusMsg.state = ServerState::Stopping;
    statusMsg.mapSizeTier = m_config.mapSize;
    statusMsg.maxPlayers = m_config.maxPlayers;
    statusMsg.currentPlayers = static_cast<std::uint8_t>(m_clients.size());
    statusMsg.currentTick = m_currentTick;
    statusMsg.serverName = m_config.serverName;
    ServerStatusMessage::getDimensionsForTier(m_config.mapSize,
                                               statusMsg.mapWidth, statusMsg.mapHeight);
    broadcast(statusMsg);

    // Stop network thread
    m_networkThread->stop();
    m_networkThread->join();

    // Clear client state
    m_clients.clear();
    m_playerToPeer.clear();
    m_sessions.clear();
    std::fill(m_usedPlayerIds.begin(), m_usedPlayerIds.end(), false);
    m_usedPlayerIds[0] = true;

    m_running = false;
    m_state = ServerNetworkState::Initializing;

    SDL_Log("NetworkServer: Stopped");
}

void NetworkServer::setRunning() {
    if (m_state == ServerNetworkState::Ready) {
        m_state = ServerNetworkState::Running;
        SDL_Log("NetworkServer: State -> Running");

        // Broadcast updated status to all clients
        ServerStatusMessage statusMsg;
        statusMsg.state = ServerState::Running;
        statusMsg.mapSizeTier = m_config.mapSize;
        statusMsg.maxPlayers = m_config.maxPlayers;
        statusMsg.currentPlayers = static_cast<std::uint8_t>(m_clients.size());
        statusMsg.currentTick = m_currentTick;
        statusMsg.serverName = m_config.serverName;
        ServerStatusMessage::getDimensionsForTier(m_config.mapSize,
                                                   statusMsg.mapWidth, statusMsg.mapHeight);
        broadcast(statusMsg);
    }
}

void NetworkServer::update(float deltaTime) {
    if (!m_running) {
        return;
    }

    m_uptime += deltaTime;

    // Update current time
    m_currentTimeMs = static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count()
    );

    // Process incoming network events
    processInboundEvents();

    // Update heartbeat timing
    m_timeSinceHeartbeat += deltaTime;
    if (m_timeSinceHeartbeat >= HEARTBEAT_INTERVAL_SEC) {
        m_timeSinceHeartbeat -= HEARTBEAT_INTERVAL_SEC;
        sendHeartbeats();
        checkTimeouts();

        // Clean up expired sessions (check every heartbeat interval)
        cleanupExpiredSessions();
    }
}

void NetworkServer::processInboundEvents() {
    InboundNetworkEvent event;
    while (m_networkThread->pollInbound(event)) {
        switch (event.type) {
            case NetworkThreadEventType::Connect:
                handleConnect(event.peer);
                break;

            case NetworkThreadEventType::Disconnect:
                handleDisconnect(event.peer, false);
                break;

            case NetworkThreadEventType::Timeout:
                handleDisconnect(event.peer, true);
                break;

            case NetworkThreadEventType::Message:
                handleMessage(event.peer, event.data);
                break;

            case NetworkThreadEventType::Error:
                SDL_Log("NetworkServer: Network error for peer %u", event.peer);
                handleDisconnect(event.peer, false);
                break;

            default:
                break;
        }
    }
}

void NetworkServer::handleConnect(PeerID peer) {
    // Check if server is full
    if (m_clients.size() >= m_config.maxPlayers) {
        SDL_Log("NetworkServer: Rejecting connection from peer %u - server full", peer);

        // Send rejection message
        // Note: The client hasn't joined yet, so we can't use the full JoinReject
        // flow. We'll just disconnect them.
        m_networkThread->disconnect(peer);
        return;
    }

    SDL_Log("NetworkServer: New connection from peer %u", peer);

    // Create pending client entry
    ClientConnection client;
    client.peer = peer;
    client.playerId = 0;  // Assigned on Join
    client.status = PlayerStatus::Connecting;
    client.lastHeartbeatReceived = m_currentTimeMs;
    client.lastHeartbeatSent = m_currentTimeMs;  // Initialize so first heartbeat sends after 1s
    client.missedHeartbeats = 0;
    client.serverHeartbeatSequence = 0;

    m_clients[peer] = client;

    // Send server status to the new client
    sendServerStatus(peer);

    // Notify handlers
    for (INetworkHandler* handler : m_handlers) {
        handler->onClientConnected(peer);
    }
}

void NetworkServer::handleDisconnect(PeerID peer, bool timedOut) {
    auto it = m_clients.find(peer);
    if (it == m_clients.end()) {
        return;
    }

    ClientConnection& client = it->second;
    SDL_Log("NetworkServer: Client disconnected - peer %u, player %u (%s)",
            peer, client.playerId,
            timedOut ? "timeout" : "graceful");

    // Mark session as disconnected (keep for grace period)
    if (client.playerId != 0) {
        // Find the session and mark it as disconnected
        for (auto& session : m_sessions) {
            if (session.playerId == client.playerId && session.connected) {
                session.connected = false;
                session.disconnectedAt = m_currentTimeMs;
                SDL_Log("NetworkServer: Session for player %u entering grace period",
                        client.playerId);
                break;
            }
        }

        // Remove from active player-to-peer mapping but DON'T free the PlayerID yet
        // PlayerID stays reserved during grace period
        m_playerToPeer.erase(client.playerId);
    }

    // Notify handlers
    for (INetworkHandler* handler : m_handlers) {
        handler->onClientDisconnected(peer, timedOut);
    }

    // Remove client connection (session remains for reconnection)
    m_clients.erase(it);

    // Broadcast updated player list
    broadcastPlayerList();
}

void NetworkServer::handleMessage(PeerID peer, const std::vector<std::uint8_t>& data) {
    // Build validation context
    ValidationContext ctx;
    ctx.peer = peer;
    ctx.currentTimeMs = m_currentTimeMs;

    // Get expected PlayerID for this peer (if connected)
    auto it = m_clients.find(peer);
    if (it != m_clients.end() && it->second.playerId != 0) {
        ctx.expectedPlayerId = it->second.playerId;
    }

    // Validate the raw message (size, envelope, version, type)
    ValidationOutput output;
    if (!m_validator.validateRawMessage(data, ctx, output)) {
        // Message invalid - already logged, connection survives
        return;
    }

    // Create message object
    auto msg = MessageFactory::create(output.header.type);
    if (!msg) {
        // This shouldn't happen since validateRawMessage checks isRegistered
        SDL_Log("NetworkServer: Failed to create message object for type %u from peer %u",
                static_cast<unsigned>(output.header.type), peer);
        return;
    }

    // Safely deserialize payload with buffer overflow protection
    NetworkBuffer buffer(data.data(), data.size());
    buffer.reset_read();

    // Skip past the header we already parsed
    try {
        NetworkMessage::parseEnvelope(buffer);  // Re-parse to advance read position
    } catch (const BufferOverflowError&) {
        // Shouldn't happen, we already validated
        return;
    }

    // Deserialize the payload with protection
    if (!m_validator.safeDeserializePayload(buffer, *msg, ctx, output)) {
        // Deserialization failed - already logged, connection survives
        return;
    }

    // Route the message
    routeMessage(peer, *msg);
}

void NetworkServer::routeMessage(PeerID peer, const NetworkMessage& msg) {
    MessageType type = msg.getType();

    // Handle system messages internally
    if (isSystemMessage(type)) {
        handleSystemMessage(peer, msg);
    }

    // For Input messages, apply rate limiting before routing to handlers
    if (type == MessageType::Input) {
        // Get player ID for this peer
        auto clientIt = m_clients.find(peer);
        if (clientIt != m_clients.end() && clientIt->second.playerId != 0) {
            // Validate PlayerID in the message matches the connection
            const auto& netInputMsg = static_cast<const NetInputMessage&>(msg);

            ValidationContext ctx;
            ctx.peer = peer;
            ctx.expectedPlayerId = clientIt->second.playerId;
            ctx.currentTimeMs = m_currentTimeMs;

            ValidationOutput output;
            if (!m_validator.validatePlayerId(netInputMsg.input.playerId, ctx, output)) {
                // Invalid PlayerID - reject and log security warning
                // Message is dropped, connection survives
                return;
            }

            // Apply rate limiting
            auto rateResult = m_rateLimiter.checkAction(
                clientIt->second.playerId,
                netInputMsg.input.type,
                m_currentTimeMs
            );

            if (!rateResult.allowed) {
                // Rate limited - silently drop per Q039
                // Connection survives, no message sent to client
                return;
            }

            // Update player activity for ghost town timer
            updatePlayerActivity(clientIt->second.playerId);
        }
    }

    // Route to registered handlers
    for (INetworkHandler* handler : m_handlers) {
        if (handler->canHandle(type)) {
            handler->handleMessage(peer, msg);
        }
    }
}

void NetworkServer::handleSystemMessage(PeerID peer, const NetworkMessage& msg) {
    switch (msg.getType()) {
        case MessageType::Join: {
            const auto& joinMsg = static_cast<const JoinMessage&>(msg);
            handleJoinRequest(peer, joinMsg);
            break;
        }

        case MessageType::Reconnect: {
            const auto& reconnectMsg = static_cast<const ReconnectMessage&>(msg);
            handleReconnectRequest(peer, reconnectMsg);
            break;
        }

        case MessageType::Heartbeat: {
            const auto& heartbeatMsg = static_cast<const HeartbeatMessage&>(msg);
            handleHeartbeat(peer, heartbeatMsg);
            break;
        }

        case MessageType::Disconnect: {
            SDL_Log("NetworkServer: Client requested disconnect - peer %u", peer);
            m_networkThread->disconnect(peer);
            handleDisconnect(peer, false);
            break;
        }

        default:
            // Other system messages handled by registered handlers
            break;
    }
}

void NetworkServer::handleJoinRequest(PeerID peer, const JoinMessage& msg) {
    auto it = m_clients.find(peer);
    if (it == m_clients.end()) {
        SDL_Log("NetworkServer: Join request from unknown peer %u", peer);
        return;
    }

    ClientConnection& client = it->second;

    // Validate the join message
    if (!msg.isValid()) {
        SDL_Log("NetworkServer: Invalid join message from peer %u", peer);
        sendJoinReject(peer, JoinRejectReason::InvalidName, "Invalid player name");
        return;
    }

    // Check if this is a reconnection attempt with session token
    if (msg.hasSessionToken) {
        std::array<std::uint8_t, SERVER_SESSION_TOKEN_SIZE> token;
        std::copy(msg.sessionToken.begin(), msg.sessionToken.end(), token.begin());

        PlayerSession* session = findSessionByToken(token);
        if (session && session->isWithinGracePeriod(m_currentTimeMs, m_config.sessionGracePeriodMs)) {
            // Valid reconnection - check if another connection exists
            auto peerIt = m_playerToPeer.find(session->playerId);
            if (peerIt != m_playerToPeer.end() && peerIt->second != peer) {
                // Duplicate connection - kick the old one
                handleDuplicateConnection(peerIt->second, peer);
            }

            // Restore the session
            client.playerId = session->playerId;
            client.playerName = session->playerName;
            client.status = PlayerStatus::Connected;
            client.lastHeartbeatReceived = m_currentTimeMs;
            client.lastActivityMs = m_currentTimeMs;
            std::copy(token.begin(), token.end(), client.sessionToken.begin());
            client.sessionCreatedAt = session->createdAt;

            session->connected = true;
            session->disconnectedAt = 0;

            m_playerToPeer[session->playerId] = peer;

            // Reset rate limiter state for reconnecting player
            m_rateLimiter.resetPlayer(session->playerId, m_currentTimeMs);

            SDL_Log("NetworkServer: Player '%s' reconnected as player %u (peer %u)",
                    session->playerName.c_str(), session->playerId, peer);

            // Send JoinAccept with same token
            sendJoinAccept(peer, session->playerId, token);
            sendServerStatus(peer);
            broadcastPlayerList();
            return;
        } else if (session) {
            // Session expired
            SDL_Log("NetworkServer: Session expired for player '%s'", msg.playerName.c_str());
            sendJoinReject(peer, JoinRejectReason::SessionExpired,
                           "Session has expired, please rejoin as a new player");
            return;
        }
        // Token not found - treat as new connection
    }

    // Check if server is full (count active sessions within grace period)
    std::uint32_t activeSessions = getActiveSessionCount();
    if (activeSessions >= m_config.maxPlayers) {
        SDL_Log("NetworkServer: Rejecting join from %s - server full (%u active sessions)",
                msg.playerName.c_str(), activeSessions);
        sendJoinReject(peer, JoinRejectReason::ServerFull, "Server is full");
        return;
    }

    // Allocate player ID
    PlayerID playerId = allocatePlayerId();
    if (playerId == 0) {
        SDL_Log("NetworkServer: Failed to allocate player ID for %s",
                msg.playerName.c_str());
        sendJoinReject(peer, JoinRejectReason::ServerFull, "No player slots available");
        return;
    }

    // Generate session token
    std::array<std::uint8_t, SERVER_SESSION_TOKEN_SIZE> sessionToken;
    generateSessionToken(sessionToken);

    // Update client state
    client.playerId = playerId;
    client.playerName = msg.playerName;
    client.status = PlayerStatus::Connected;
    client.lastHeartbeatReceived = m_currentTimeMs;
    client.lastActivityMs = m_currentTimeMs;
    std::copy(sessionToken.begin(), sessionToken.end(), client.sessionToken.begin());
    client.sessionCreatedAt = m_currentTimeMs;

    m_playerToPeer[playerId] = peer;

    // Create session for reconnection support
    createSession(playerId, msg.playerName, sessionToken);

    // Register player with rate limiter
    m_rateLimiter.registerPlayer(playerId, m_currentTimeMs);

    SDL_Log("NetworkServer: Player '%s' joined as player %u (peer %u)",
            msg.playerName.c_str(), playerId, peer);

    // Send JoinAccept message with session token
    sendJoinAccept(peer, playerId, sessionToken);

    // Send current server status
    sendServerStatus(peer);

    // Broadcast updated player list to all clients
    broadcastPlayerList();
}

void NetworkServer::handleHeartbeat(PeerID peer, const HeartbeatMessage& msg) {
    auto it = m_clients.find(peer);
    if (it == m_clients.end()) {
        return;
    }

    ClientConnection& client = it->second;

    // Update heartbeat tracking
    client.lastHeartbeatReceived = m_currentTimeMs;
    client.missedHeartbeats = 0;
    client.heartbeatSequence = msg.clientSequence;

    // Calculate latency (round-trip time)
    if (msg.clientTimestamp > 0) {
        std::uint64_t rtt = m_currentTimeMs - msg.clientTimestamp;
        client.latencyMs = static_cast<std::uint32_t>(rtt);
    }

    // Send heartbeat response
    HeartbeatResponseMessage response;
    response.clientTimestamp = msg.clientTimestamp;
    response.serverTimestamp = m_currentTimeMs;
    response.serverTick = m_currentTick;

    sendTo(peer, response);
}

void NetworkServer::sendHeartbeats() {
    // Server-initiated heartbeats: send a HeartbeatResponse to each client
    // every 1 second to maintain connection liveness and provide server tick info.
    // This is in addition to responding to client-initiated heartbeats.

    for (auto& [peer, client] : m_clients) {
        // Check if 1 second has elapsed since last heartbeat sent to this client
        std::uint64_t elapsed = m_currentTimeMs - client.lastHeartbeatSent;
        if (elapsed >= 1000) {  // 1 second = 1000 ms
            // Send heartbeat to this client
            HeartbeatResponseMessage heartbeat;
            heartbeat.clientTimestamp = 0;  // No client timestamp to echo (server-initiated)
            heartbeat.serverTimestamp = m_currentTimeMs;
            heartbeat.serverTick = m_currentTick;

            // Increment server heartbeat sequence for this client
            client.serverHeartbeatSequence++;

            sendTo(peer, heartbeat);
            client.lastHeartbeatSent = m_currentTimeMs;
        }
    }
}

void NetworkServer::checkTimeouts() {
    std::vector<PeerID> timedOut;

    for (auto& [peer, client] : m_clients) {
        std::uint64_t elapsed = m_currentTimeMs - client.lastHeartbeatReceived;
        std::uint32_t missedCount = static_cast<std::uint32_t>(elapsed / 1000);  // 1 heartbeat per second

        if (missedCount > client.missedHeartbeats) {
            client.missedHeartbeats = missedCount;

            if (missedCount >= HEARTBEAT_WARNING_THRESHOLD &&
                missedCount < HEARTBEAT_DISCONNECT_THRESHOLD) {
                SDL_Log("NetworkServer: Client %u (%s) missed %u heartbeats",
                        peer, client.playerName.c_str(), missedCount);
            }
        }

        // Hard disconnect after 10 seconds of silence
        if (elapsed >= HEARTBEAT_DISCONNECT_THRESHOLD * 1000) {
            SDL_Log("NetworkServer: Client %u (%s) timed out after %llu ms",
                    peer, client.playerName.c_str(),
                    static_cast<unsigned long long>(elapsed));
            timedOut.push_back(peer);
        }
    }

    // Disconnect timed-out clients
    for (PeerID peer : timedOut) {
        m_networkThread->disconnect(peer);
        handleDisconnect(peer, true);
    }
}

PlayerID NetworkServer::allocatePlayerId() {
    // Find first available ID (starting at 1)
    for (std::size_t i = 1; i < m_usedPlayerIds.size(); ++i) {
        if (!m_usedPlayerIds[i]) {
            m_usedPlayerIds[i] = true;
            return static_cast<PlayerID>(i);
        }
    }
    return 0;  // No IDs available
}

void NetworkServer::freePlayerId(PlayerID id) {
    if (id > 0 && static_cast<std::size_t>(id) < m_usedPlayerIds.size()) {
        m_usedPlayerIds[id] = false;
    }
}

void NetworkServer::sendServerStatus(PeerID peer) {
    ServerStatusMessage statusMsg;

    // Map our internal state to the ServerState enum
    switch (m_state) {
        case ServerNetworkState::Initializing:
        case ServerNetworkState::Loading:
            statusMsg.state = ServerState::Loading;
            break;
        case ServerNetworkState::Ready:
            statusMsg.state = ServerState::Ready;
            break;
        case ServerNetworkState::Running:
            statusMsg.state = ServerState::Running;
            break;
    }

    statusMsg.mapSizeTier = m_config.mapSize;
    statusMsg.maxPlayers = m_config.maxPlayers;
    statusMsg.currentPlayers = static_cast<std::uint8_t>(getClientCount());
    statusMsg.currentTick = m_currentTick;
    statusMsg.serverName = m_config.serverName;

    // Get map dimensions from tier
    ServerStatusMessage::getDimensionsForTier(m_config.mapSize,
                                               statusMsg.mapWidth, statusMsg.mapHeight);

    sendTo(peer, statusMsg);
}

void NetworkServer::broadcastPlayerList() {
    PlayerListMessage playerList;

    for (const auto& [peer, client] : m_clients) {
        if (client.status == PlayerStatus::Connected) {
            playerList.addPlayer(client.playerId, client.playerName,
                                 client.status, client.latencyMs);
        }
    }

    broadcast(playerList);
}

bool NetworkServer::sendTo(PeerID peer, const NetworkMessage& msg, ChannelID channel) {
    if (!m_running) {
        return false;
    }

    auto it = m_clients.find(peer);
    if (it == m_clients.end()) {
        return false;
    }

    queueMessage(peer, msg, channel);
    return true;
}

bool NetworkServer::sendToPlayer(PlayerID playerId, const NetworkMessage& msg,
                                  ChannelID channel) {
    auto it = m_playerToPeer.find(playerId);
    if (it == m_playerToPeer.end()) {
        return false;
    }
    return sendTo(it->second, msg, channel);
}

void NetworkServer::broadcast(const NetworkMessage& msg, ChannelID channel) {
    if (!m_running) {
        return;
    }

    // Serialize once
    NetworkBuffer buffer;
    msg.serializeWithEnvelope(buffer);

    // Send to all connected clients
    OutboundNetworkMessage outMsg;
    outMsg.data = buffer.raw();
    outMsg.channel = channel;
    outMsg.broadcast = true;

    m_networkThread->enqueueOutbound(std::move(outMsg));
}

void NetworkServer::broadcastStateUpdate(const StateUpdateMessage& msg) {
    broadcast(msg, ChannelID::Reliable);
}

void NetworkServer::broadcastServerChat(const std::string& text) {
    ChatMessage chatMsg;
    chatMsg.senderId = 0;  // Server/GameMaster
    chatMsg.text = text;
    chatMsg.timestamp = m_currentTimeMs;

    broadcast(chatMsg);
}

void NetworkServer::queueMessage(PeerID peer, const NetworkMessage& msg,
                                  ChannelID channel) {
    NetworkBuffer buffer;
    msg.serializeWithEnvelope(buffer);

    OutboundNetworkMessage outMsg;
    outMsg.peer = peer;
    outMsg.data = buffer.raw();
    outMsg.channel = channel;
    outMsg.broadcast = false;

    m_networkThread->enqueueOutbound(std::move(outMsg));
}

void NetworkServer::registerHandler(INetworkHandler* handler) {
    if (handler && std::find(m_handlers.begin(), m_handlers.end(), handler) == m_handlers.end()) {
        m_handlers.push_back(handler);
    }
}

void NetworkServer::unregisterHandler(INetworkHandler* handler) {
    auto it = std::find(m_handlers.begin(), m_handlers.end(), handler);
    if (it != m_handlers.end()) {
        m_handlers.erase(it);
    }
}

std::uint32_t NetworkServer::getClientCount() const {
    std::uint32_t count = 0;
    for (const auto& [_, client] : m_clients) {
        if (client.status == PlayerStatus::Connected) {
            count++;
        }
    }
    return count;
}

std::vector<ClientConnection> NetworkServer::getClients() const {
    std::vector<ClientConnection> result;
    result.reserve(m_clients.size());
    for (const auto& [_, client] : m_clients) {
        result.push_back(client);
    }
    return result;
}

const ClientConnection* NetworkServer::getClient(PeerID peer) const {
    auto it = m_clients.find(peer);
    if (it != m_clients.end()) {
        return &it->second;
    }
    return nullptr;
}

const ClientConnection* NetworkServer::getClientByPlayerId(PlayerID playerId) const {
    auto it = m_playerToPeer.find(playerId);
    if (it != m_playerToPeer.end()) {
        return getClient(it->second);
    }
    return nullptr;
}

void NetworkServer::kickPlayer(PlayerID playerId, const std::string& reason) {
    auto it = m_playerToPeer.find(playerId);
    if (it != m_playerToPeer.end()) {
        kickPeer(it->second, reason);
    }
}

void NetworkServer::kickPeer(PeerID peer, const std::string& reason) {
    auto it = m_clients.find(peer);
    if (it == m_clients.end()) {
        return;
    }

    ClientConnection& client = it->second;

    SDL_Log("NetworkServer: Kicking player %u (%s): %s",
            client.playerId, client.playerName.c_str(), reason.c_str());

    // Update status
    client.status = PlayerStatus::Kicked;

    // Send Kick message with reason
    KickMessage kickMsg;
    kickMsg.reason = reason;
    sendTo(peer, kickMsg);

    // Disconnect
    m_networkThread->disconnect(peer);
    handleDisconnect(peer, false);
}

// =============================================================================
// Session Management Implementation
// =============================================================================

void NetworkServer::generateSessionToken(std::array<std::uint8_t, SERVER_SESSION_TOKEN_SIZE>& token) {
    // Use random_device for cryptographically secure random numbers
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<std::uint64_t> dis;

    // Generate two 64-bit random values to fill 128-bit token
    std::uint64_t part1 = dis(gen);
    std::uint64_t part2 = dis(gen);

    std::memcpy(token.data(), &part1, 8);
    std::memcpy(token.data() + 8, &part2, 8);
}

void NetworkServer::createSession(PlayerID playerId, const std::string& name,
                                   const std::array<std::uint8_t, SERVER_SESSION_TOKEN_SIZE>& token) {
    PlayerSession session;
    session.playerId = playerId;
    session.playerName = name;
    std::copy(token.begin(), token.end(), session.token.begin());
    session.createdAt = m_currentTimeMs;
    session.disconnectedAt = 0;
    session.connected = true;

    m_sessions.push_back(std::move(session));
}

PlayerSession* NetworkServer::findSessionByToken(
    const std::array<std::uint8_t, SERVER_SESSION_TOKEN_SIZE>& token) {
    for (auto& session : m_sessions) {
        if (session.tokenMatches(token)) {
            return &session;
        }
    }
    return nullptr;
}

const PlayerSession* NetworkServer::getSessionByToken(
    const std::array<std::uint8_t, SERVER_SESSION_TOKEN_SIZE>& token) const {
    for (const auto& session : m_sessions) {
        if (session.tokenMatches(token)) {
            return &session;
        }
    }
    return nullptr;
}

bool NetworkServer::isSessionValidForReconnect(
    const std::array<std::uint8_t, SERVER_SESSION_TOKEN_SIZE>& token) const {
    const PlayerSession* session = getSessionByToken(token);
    if (!session) {
        return false;
    }
    return session->isWithinGracePeriod(m_currentTimeMs, m_config.sessionGracePeriodMs);
}

std::uint32_t NetworkServer::getActiveSessionCount() const {
    std::uint32_t count = 0;
    for (const auto& session : m_sessions) {
        if (session.isWithinGracePeriod(m_currentTimeMs, m_config.sessionGracePeriodMs)) {
            count++;
        }
    }
    return count;
}

void NetworkServer::cleanupExpiredSessions() {
    // Remove sessions that are past the grace period
    auto it = m_sessions.begin();
    while (it != m_sessions.end()) {
        if (!it->connected &&
            !it->isWithinGracePeriod(m_currentTimeMs, m_config.sessionGracePeriodMs)) {
            SDL_Log("NetworkServer: Session for player %u (%s) expired, cleaning up",
                    it->playerId, it->playerName.c_str());

            // Free the player ID now that grace period is over
            freePlayerId(it->playerId);

            // Unregister from rate limiter
            m_rateLimiter.unregisterPlayer(it->playerId);

            it = m_sessions.erase(it);
        } else {
            ++it;
        }
    }
}

void NetworkServer::sendJoinAccept(PeerID peer, PlayerID playerId,
                                    const std::array<std::uint8_t, SERVER_SESSION_TOKEN_SIZE>& token) {
    JoinAcceptMessage acceptMsg;
    acceptMsg.playerId = playerId;
    std::copy(token.begin(), token.end(), acceptMsg.sessionToken.begin());
    acceptMsg.serverTick = m_currentTick;

    sendTo(peer, acceptMsg);
}

void NetworkServer::sendJoinReject(PeerID peer, JoinRejectReason reason, const std::string& message) {
    JoinRejectMessage rejectMsg;
    rejectMsg.reason = reason;
    rejectMsg.message = message;

    sendTo(peer, rejectMsg);

    // Disconnect the peer after rejection
    m_networkThread->disconnect(peer);
}

void NetworkServer::handleReconnectRequest(PeerID peer, const ReconnectMessage& msg) {
    auto it = m_clients.find(peer);
    if (it == m_clients.end()) {
        SDL_Log("NetworkServer: Reconnect request from unknown peer %u", peer);
        return;
    }

    ClientConnection& client = it->second;

    // Validate the reconnect message
    if (!msg.isValid()) {
        SDL_Log("NetworkServer: Invalid reconnect message from peer %u", peer);
        sendJoinReject(peer, JoinRejectReason::InvalidToken, "Invalid session token");
        return;
    }

    // Copy token to properly sized array
    std::array<std::uint8_t, SERVER_SESSION_TOKEN_SIZE> token;
    std::copy(msg.sessionToken.begin(), msg.sessionToken.end(), token.begin());

    // Find the session
    PlayerSession* session = findSessionByToken(token);
    if (!session) {
        SDL_Log("NetworkServer: Session token not found for reconnect");
        sendJoinReject(peer, JoinRejectReason::InvalidToken, "Session not found");
        return;
    }

    // Check if within grace period
    if (!session->isWithinGracePeriod(m_currentTimeMs, m_config.sessionGracePeriodMs)) {
        SDL_Log("NetworkServer: Session expired for reconnect");
        sendJoinReject(peer, JoinRejectReason::SessionExpired, "Session has expired");
        return;
    }

    // Check if there's an existing connection for this session
    auto existingPeerIt = m_playerToPeer.find(session->playerId);
    if (existingPeerIt != m_playerToPeer.end() && existingPeerIt->second != peer) {
        // Duplicate connection - kick the old one
        handleDuplicateConnection(existingPeerIt->second, peer);
    }

    // Restore the session
    client.playerId = session->playerId;
    client.playerName = session->playerName;
    client.status = PlayerStatus::Connected;
    client.lastHeartbeatReceived = m_currentTimeMs;
    client.lastActivityMs = m_currentTimeMs;
    std::copy(token.begin(), token.end(), client.sessionToken.begin());
    client.sessionCreatedAt = session->createdAt;

    session->connected = true;
    session->disconnectedAt = 0;

    m_playerToPeer[session->playerId] = peer;

    SDL_Log("NetworkServer: Player '%s' reconnected via ReconnectMessage as player %u (peer %u)",
            session->playerName.c_str(), session->playerId, peer);

    // Send JoinAccept with same token
    sendJoinAccept(peer, session->playerId, token);
    sendServerStatus(peer);
    broadcastPlayerList();
}

void NetworkServer::handleDuplicateConnection(PeerID existingPeer, PeerID newPeer) {
    (void)newPeer;  // Not used, but kept for clarity

    auto it = m_clients.find(existingPeer);
    if (it == m_clients.end()) {
        return;
    }

    SDL_Log("NetworkServer: Duplicate connection detected, kicking existing peer %u",
            existingPeer);

    // Send kick message to existing connection
    KickMessage kickMsg;
    kickMsg.reason = "Another connection with your session token connected";
    sendTo(existingPeer, kickMsg);

    // Disconnect the existing peer (don't call handleDisconnect - we want to preserve session)
    m_networkThread->disconnect(existingPeer);
    m_clients.erase(it);
}

void NetworkServer::updatePlayerActivity(PlayerID playerId) {
    auto peerIt = m_playerToPeer.find(playerId);
    if (peerIt == m_playerToPeer.end()) {
        return;
    }

    auto clientIt = m_clients.find(peerIt->second);
    if (clientIt != m_clients.end()) {
        clientIt->second.lastActivityMs = m_currentTimeMs;
    }
}

} // namespace sims3000
