/**
 * @file TestServer.cpp
 * @brief Implementation of TestServer for testing.
 */

#include "sims3000/test/TestServer.h"
#include "sims3000/ecs/Components.h"

namespace sims3000 {

TestServer::TestServer()
    : m_config{}
{
}

TestServer::TestServer(const TestServerConfig& config)
    : m_config(config)
{
}

TestServer::~TestServer() {
    stop();
}

bool TestServer::start() {
    return start(m_config.port, m_config.maxPlayers);
}

bool TestServer::start(std::uint16_t port, std::uint8_t maxPlayers) {
    if (m_running) return false;

    // Create mock socket with configured network conditions
    if (m_config.seed != 0) {
        m_socket = std::make_unique<MockSocket>(m_config.networkConditions, m_config.seed);
    } else {
        m_socket = std::make_unique<MockSocket>(m_config.networkConditions);
    }

    // Create server config
    ServerConfig serverConfig;
    serverConfig.port = port;
    serverConfig.maxPlayers = maxPlayers;
    serverConfig.mapSize = m_config.mapSize;
    serverConfig.serverName = m_config.serverName;

    // Note: We need to create a transport that the server will own
    // The server takes ownership of the transport, so we create a new one
    auto serverSocket = std::make_unique<MockSocket>(m_config.networkConditions);
    if (m_config.seed != 0) {
        serverSocket->setSeed(m_config.seed);
    }

    // Keep a reference to the socket for direct manipulation
    m_socket = std::move(serverSocket);

    // For testing purposes, we create the server but don't use NetworkThread
    // Instead, we manually manage the mock socket
    // This is a simplified test server that doesn't use the full NetworkServer

    // Start the mock socket as server
    if (!m_socket->startServer(port, maxPlayers)) {
        return false;
    }

    m_config.port = m_socket->getAssignedPort();
    m_running = true;
    m_currentTick = 0;

    return true;
}

void TestServer::stop() {
    if (!m_running) return;

    if (m_socket) {
        m_socket->disconnectAll();
    }

    m_running = false;
    m_socket.reset();
    m_server.reset();
}

bool TestServer::isRunning() const {
    return m_running;
}

std::uint16_t TestServer::getPort() const {
    return m_config.port;
}

void TestServer::update(float deltaTime) {
    if (!m_running) return;

    // Process any pending network events
    processNetworkEvents();

    // Advance simulated time on the socket
    auto deltaMs = static_cast<std::uint64_t>(deltaTime * 1000.0f);
    m_socket->advanceTime(deltaMs);
}

void TestServer::advanceTicks(std::uint32_t ticks) {
    // Assume 20 ticks per second (50ms per tick)
    const float tickDelta = 0.05f;

    for (std::uint32_t i = 0; i < ticks; ++i) {
        update(tickDelta);
        m_currentTick++;
    }
}

SimulationTick TestServer::getCurrentTick() const {
    return m_currentTick;
}

void TestServer::setCurrentTick(SimulationTick tick) {
    m_currentTick = tick;
}

Registry& TestServer::getRegistry() {
    return m_registry;
}

const Registry& TestServer::getRegistry() const {
    return m_registry;
}

std::uint32_t TestServer::getClientCount() const {
    if (m_socket) {
        return m_socket->getPeerCount();
    }
    return 0;
}

std::vector<PeerID> TestServer::getClientPeers() const {
    // For mock socket, we don't have direct access to peer list
    // This would need to be tracked separately in production
    std::vector<PeerID> peers;
    return peers;
}

std::vector<PlayerID> TestServer::getPlayerIds() const {
    std::vector<PlayerID> players;
    // Would be populated from actual server state
    return players;
}

bool TestServer::isPlayerConnected(PlayerID playerId) const {
    // Simplified check - in production would use actual server state
    (void)playerId;
    return false;
}

MockSocket* TestServer::getMockSocket() {
    return m_socket.get();
}

NetworkServer* TestServer::getNetworkServer() {
    return m_server.get();
}

EntityID TestServer::createTestEntity(GridPosition pos, PlayerID owner) {
    EntityID entity = m_registry.create();

    PositionComponent posComp;
    posComp.pos = pos;
    posComp.elevation = 0;
    m_registry.emplace<PositionComponent>(entity, posComp);

    OwnershipComponent ownComp;
    ownComp.owner = owner;
    ownComp.state = owner == 0 ? OwnershipState::Neutral : OwnershipState::Owned;
    m_registry.emplace<OwnershipComponent>(entity, ownComp);

    return entity;
}

EntityID TestServer::createBuilding(GridPosition pos, std::uint32_t buildingType,
                                     PlayerID owner) {
    EntityID entity = createTestEntity(pos, owner);

    BuildingComponent bldComp;
    bldComp.buildingType = buildingType;
    bldComp.level = 1;
    bldComp.health = 100;
    m_registry.emplace<BuildingComponent>(entity, bldComp);

    return entity;
}

std::size_t TestServer::getEntityCount() const {
    return m_registry.size();
}

PlayerID TestServer::simulateClientConnect(const std::string& playerName) {
    if (!m_socket) return 0;

    // Generate a peer ID
    static PeerID nextPeer = 1;
    PeerID peer = nextPeer++;

    // Inject connect event
    m_socket->injectConnectEvent(peer);

    // In a real implementation, this would go through the join handshake
    // For testing, we return a simple player ID
    return static_cast<PlayerID>(peer);
}

void TestServer::simulateClientDisconnect(PlayerID playerId) {
    if (!m_socket) return;

    // Inject disconnect event
    m_socket->injectDisconnectEvent(static_cast<PeerID>(playerId));
}

void TestServer::processNetworkEvents() {
    if (!m_socket) return;

    // Process all pending events
    NetworkEvent event;
    while ((event = m_socket->poll()).type != NetworkEventType::None) {
        // Handle events as needed for testing
        switch (event.type) {
            case NetworkEventType::Connect:
                // Client connected
                break;
            case NetworkEventType::Disconnect:
                // Client disconnected
                break;
            case NetworkEventType::Receive:
                // Data received - would be processed by message handlers
                break;
            default:
                break;
        }
    }
}

void TestServer::flushNetwork() {
    if (m_socket) {
        m_socket->flush();
    }
}

} // namespace sims3000
