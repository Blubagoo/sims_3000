/**
 * @file TestClient.cpp
 * @brief Implementation of TestClient for testing.
 */

#include "sims3000/test/TestClient.h"
#include "sims3000/test/TestServer.h"
#include <sstream>

namespace sims3000 {

TestClient::TestClient()
    : m_config{}
{
}

TestClient::TestClient(const TestClientConfig& config)
    : m_config(config)
{
}

TestClient::~TestClient() {
    disconnect();
}

bool TestClient::connectTo(const std::string& address, std::uint16_t port,
                            const std::string& playerName) {
    // Create mock socket if not already linked
    if (!m_socket) {
        if (m_config.seed != 0) {
            m_socket = std::make_unique<MockSocket>(m_config.networkConditions, m_config.seed);
        } else {
            m_socket = std::make_unique<MockSocket>(m_config.networkConditions);
        }
    }

    // Create network client
    ConnectionConfig connConfig;
    connConfig.connectTimeoutMs = m_config.connectTimeoutMs;

    // Note: NetworkClient takes ownership of transport, but we want to keep
    // our mock socket for testing. For this simplified test client, we
    // just track state locally rather than using full NetworkClient.

    std::string name = playerName.empty() ? m_config.playerName : playerName;

    // Start connection via mock socket
    PeerID serverPeer = m_socket->connect(address, port);
    if (serverPeer == INVALID_PEER_ID) {
        return false;
    }

    return true;
}

bool TestClient::connectTo(TestServer& server) {
    // Get server's mock socket
    MockSocket* serverSocket = server.getMockSocket();
    if (!serverSocket) {
        return false;
    }

    // Create linked socket pair
    if (m_config.seed != 0) {
        m_socket = std::make_unique<MockSocket>(m_config.networkConditions, m_config.seed);
    } else {
        m_socket = std::make_unique<MockSocket>(m_config.networkConditions);
    }

    // Link sockets bidirectionally
    m_socket->linkTo(serverSocket, 1);
    serverSocket->linkTo(m_socket.get(), 1);

    // Simulate connection
    m_socket->simulateConnect();
    serverSocket->simulateConnect();

    m_linkedToServer = true;
    return true;
}

void TestClient::disconnect() {
    if (m_socket) {
        m_socket->disconnectAll();
    }
    m_linkedToServer = false;
}

bool TestClient::isConnected() const {
    if (m_socket) {
        return m_socket->getPeerCount() > 0;
    }
    return false;
}

bool TestClient::isConnecting() const {
    // Simplified - would check actual client state
    return false;
}

ConnectionState TestClient::getState() const {
    if (isConnected()) {
        return ConnectionState::Playing;
    }
    return ConnectionState::Disconnected;
}

PlayerID TestClient::getPlayerId() const {
    // Would come from join accept message
    return 1;
}

void TestClient::update(float deltaTime) {
    if (!m_socket) return;

    // Advance simulated time
    auto deltaMs = static_cast<std::uint64_t>(deltaTime * 1000.0f);
    m_socket->advanceTime(deltaMs);

    // Process network events
    NetworkEvent event;
    while ((event = m_socket->poll()).type != NetworkEventType::None) {
        switch (event.type) {
            case NetworkEventType::Connect:
                // Connection established
                break;
            case NetworkEventType::Disconnect:
                // Disconnected
                break;
            case NetworkEventType::Receive:
                // Data received - would be processed by message handlers
                break;
            default:
                break;
        }
    }
}

void TestClient::advanceTime(std::uint32_t ms, std::uint32_t frameSize) {
    std::uint32_t elapsed = 0;
    float frameDelta = static_cast<float>(frameSize) / 1000.0f;

    while (elapsed < ms) {
        update(frameDelta);
        elapsed += frameSize;
    }
}

bool TestClient::waitForState(ConnectionState state, std::uint32_t timeoutMs) {
    std::uint32_t elapsed = 0;
    const std::uint32_t frameMs = 16;

    while (elapsed < timeoutMs) {
        if (getState() == state) {
            return true;
        }
        advanceTime(frameMs);
        elapsed += frameMs;
    }

    return getState() == state;
}

Registry& TestClient::getRegistry() {
    return m_registry;
}

const Registry& TestClient::getRegistry() const {
    return m_registry;
}

std::size_t TestClient::getPendingStateUpdates() const {
    // Would come from actual NetworkClient
    return 0;
}

MockSocket* TestClient::getMockSocket() {
    return m_socket.get();
}

NetworkClient* TestClient::getNetworkClient() {
    return m_client.get();
}

const ConnectionStats& TestClient::getStats() const {
    static ConnectionStats defaultStats;
    if (m_client) {
        return m_client->getStats();
    }
    return defaultStats;
}

void TestClient::sendInput(const InputMessage& input) {
    if (!m_socket || !isConnected()) return;

    // Serialize input message
    // In a real implementation, this would use the proper message format
    std::vector<std::uint8_t> data;
    // Simplified serialization for testing
    data.push_back(static_cast<std::uint8_t>(input.type));
    data.push_back(static_cast<std::uint8_t>(input.targetPos.x >> 8));
    data.push_back(static_cast<std::uint8_t>(input.targetPos.x));
    data.push_back(static_cast<std::uint8_t>(input.targetPos.y >> 8));
    data.push_back(static_cast<std::uint8_t>(input.targetPos.y));
    data.push_back(static_cast<std::uint8_t>(input.param1 >> 24));
    data.push_back(static_cast<std::uint8_t>(input.param1 >> 16));
    data.push_back(static_cast<std::uint8_t>(input.param1 >> 8));
    data.push_back(static_cast<std::uint8_t>(input.param1));

    m_socket->send(1, data.data(), data.size(), ChannelID::Reliable);
}

void TestClient::placeBuilding(GridPosition pos, std::uint32_t buildingType) {
    InputMessage input;
    input.type = InputType::PlaceBuilding;
    input.targetPos = pos;
    input.param1 = buildingType;
    sendInput(input);
}

void TestClient::demolish(GridPosition pos) {
    InputMessage input;
    input.type = InputType::DemolishBuilding;
    input.targetPos = pos;
    sendInput(input);
}

std::size_t TestClient::getPendingInputCount() const {
    if (m_socket) {
        return m_socket->getOutgoingCount();
    }
    return 0;
}

AssertionResult TestClient::assertConnected() const {
    AssertionResult result;
    result.passed = isConnected();
    if (!result.passed) {
        result.message = "Expected connected, but state is: ";
        result.message += getConnectionStateName(getState());
    }
    return result;
}

AssertionResult TestClient::assertDisconnected() const {
    AssertionResult result;
    result.passed = !isConnected() && getState() == ConnectionState::Disconnected;
    if (!result.passed) {
        result.message = "Expected disconnected, but state is: ";
        result.message += getConnectionStateName(getState());
    }
    return result;
}

AssertionResult TestClient::assertState(ConnectionState state) const {
    AssertionResult result;
    result.passed = getState() == state;
    if (!result.passed) {
        std::ostringstream oss;
        oss << "Expected state " << getConnectionStateName(state)
            << ", but got " << getConnectionStateName(getState());
        result.message = oss.str();
    }
    return result;
}

AssertionResult TestClient::assertPlayerId(PlayerID expected) const {
    AssertionResult result;
    PlayerID actual = getPlayerId();
    result.passed = actual == expected;
    if (!result.passed) {
        std::ostringstream oss;
        oss << "Expected player ID " << static_cast<int>(expected)
            << ", but got " << static_cast<int>(actual);
        result.message = oss.str();
    }
    return result;
}

AssertionResult TestClient::assertEntityExists(EntityID entityId) const {
    AssertionResult result;
    result.passed = m_registry.valid(entityId);
    if (!result.passed) {
        std::ostringstream oss;
        oss << "Entity " << entityId << " does not exist in registry";
        result.message = oss.str();
    }
    return result;
}

AssertionResult TestClient::assertRttBelow(std::uint32_t maxRttMs) const {
    AssertionResult result;
    const auto& stats = getStats();
    result.passed = stats.rttMs <= maxRttMs;
    if (!result.passed) {
        std::ostringstream oss;
        oss << "RTT " << stats.rttMs << "ms exceeds threshold " << maxRttMs << "ms";
        result.message = oss.str();
    }
    return result;
}

void TestClient::linkToServer(MockSocket* serverSocket) {
    if (!m_socket) {
        if (m_config.seed != 0) {
            m_socket = std::make_unique<MockSocket>(m_config.networkConditions, m_config.seed);
        } else {
            m_socket = std::make_unique<MockSocket>(m_config.networkConditions);
        }
    }

    m_socket->linkTo(serverSocket, 1);
    serverSocket->linkTo(m_socket.get(), 1);
    m_linkedToServer = true;
}

} // namespace sims3000
