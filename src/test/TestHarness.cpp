/**
 * @file TestHarness.cpp
 * @brief Implementation of TestHarness for multi-client testing.
 */

#include "sims3000/test/TestHarness.h"
#include <sstream>

namespace sims3000 {

TestHarness::TestHarness()
    : m_config{}
{
}

TestHarness::TestHarness(const HarnessConfig& config)
    : m_config(config)
{
}

TestHarness::~TestHarness() {
    disconnectAllClients();
    if (m_server) {
        m_server->stop();
    }
}

void TestHarness::setMapSize(MapSizeTier tier) {
    m_config.mapSize = tier;
}

void TestHarness::setNetworkConditions(const NetworkConditions& conditions) {
    m_config.networkConditions = conditions;
}

bool TestHarness::createServer() {
    TestServerConfig serverConfig;
    serverConfig.port = m_config.serverPort;
    serverConfig.maxPlayers = m_config.maxClients;
    serverConfig.mapSize = m_config.mapSize;
    serverConfig.networkConditions = m_config.networkConditions;
    serverConfig.seed = m_config.seed;
    serverConfig.headless = m_config.headless;

    m_server = std::make_unique<TestServer>(serverConfig);
    return m_server->start();
}

bool TestHarness::createClients(std::size_t count) {
    if (!m_server) return false;

    m_clients.clear();
    m_clients.reserve(count);

    for (std::size_t i = 0; i < count; ++i) {
        TestClientConfig clientConfig;
        clientConfig.playerName = "Player" + std::to_string(i + 1);
        clientConfig.networkConditions = m_config.networkConditions;
        clientConfig.seed = m_config.seed == 0 ? 0 : m_config.seed + i + 1;
        clientConfig.headless = m_config.headless;

        auto client = std::make_unique<TestClient>(clientConfig);
        m_clients.push_back(std::move(client));
    }

    return true;
}

bool TestHarness::connectAllClients(std::uint32_t timeoutMs) {
    if (!m_server) return false;

    std::uint32_t timeout = timeoutMs == 0 ? getDefaultTimeout() : timeoutMs;

    for (auto& client : m_clients) {
        if (!client->connectTo(*m_server)) {
            return false;
        }
    }

    // Wait for all clients to be connected
    std::uint32_t elapsed = 0;
    const std::uint32_t frameMs = 16;

    while (elapsed < timeout) {
        update(0.016f);

        if (allClientsConnected()) {
            return true;
        }

        elapsed += frameMs;
    }

    return allClientsConnected();
}

void TestHarness::disconnectAllClients() {
    for (auto& client : m_clients) {
        client->disconnect();
    }
}

void TestHarness::update(float deltaTime) {
    if (m_server) {
        m_server->update(deltaTime);
    }

    for (auto& client : m_clients) {
        client->update(deltaTime);
    }
}

void TestHarness::advanceTicks(std::uint32_t ticks) {
    // 20 ticks per second = 50ms per tick
    const float tickDelta = 0.05f;

    for (std::uint32_t i = 0; i < ticks; ++i) {
        if (m_server) {
            m_server->update(tickDelta);
        }

        for (auto& client : m_clients) {
            client->update(tickDelta);
        }

        // Flush network after each tick to propagate messages
        flushAll();
    }
}

void TestHarness::advanceTime(std::uint32_t ms) {
    const std::uint32_t frameMs = 16;
    const float frameDelta = 0.016f;

    std::uint32_t elapsed = 0;
    while (elapsed < ms) {
        update(frameDelta);
        elapsed += frameMs;
    }
}

SyncResult TestHarness::waitForSync(std::uint32_t timeoutMs) {
    SyncResult result;
    std::uint32_t timeout = timeoutMs == 0 ? getDefaultTimeout() : timeoutMs;

    const std::uint32_t frameMs = 16;
    std::uint32_t elapsed = 0;

    while (elapsed < timeout) {
        update(0.016f);
        flushAll();

        // Check if synced: no pending updates and entity counts match
        bool synced = true;

        if (m_server) {
            std::size_t serverEntityCount = m_server->getEntityCount();

            for (auto& client : m_clients) {
                if (client->getPendingStateUpdates() > 0) {
                    synced = false;
                    break;
                }

                // Check entity count match
                std::size_t clientEntityCount = client->getRegistry().size();
                if (clientEntityCount != serverEntityCount) {
                    synced = false;
                    break;
                }
            }
        }

        if (synced) {
            result.success = true;
            result.timeElapsedMs = elapsed;
            result.ticksElapsed = elapsed / 50; // Approximate ticks
            return result;
        }

        elapsed += frameMs;
    }

    result.success = false;
    result.timeElapsedMs = elapsed;
    result.ticksElapsed = elapsed / 50;
    result.message = "Sync timeout after " + std::to_string(elapsed) + "ms";

    return result;
}

void TestHarness::flushAll() {
    if (m_server) {
        m_server->flushNetwork();
    }

    for (auto& client : m_clients) {
        if (auto* socket = client->getMockSocket()) {
            socket->flush();
        }
    }
}

StateMatchResult TestHarness::assertStateMatch() {
    StateMatchResult result;
    result.allMatch = true;
    result.clientDifferences.resize(m_clients.size());

    if (!m_server) {
        result.allMatch = false;
        result.summary = "No server to compare against";
        return result;
    }

    std::ostringstream summaryStream;

    for (std::size_t i = 0; i < m_clients.size(); ++i) {
        auto diffs = compareWithClient(i);
        result.clientDifferences[i] = diffs;

        if (!diffs.empty()) {
            result.allMatch = false;
            summaryStream << "Client " << i << ": "
                         << diffs.size() << " difference(s)\n";

            // Show first few differences
            for (std::size_t j = 0; j < std::min(diffs.size(), std::size_t{3}); ++j) {
                summaryStream << "  - " << diffs[j].toString() << "\n";
            }

            if (diffs.size() > 3) {
                summaryStream << "  ... and " << (diffs.size() - 3) << " more\n";
            }
        }
    }

    if (result.allMatch) {
        result.summary = "All " + std::to_string(m_clients.size()) +
                        " client(s) match server state";
    } else {
        result.summary = summaryStream.str();
    }

    return result;
}

std::vector<StateDifference> TestHarness::compareWithClient(std::size_t clientIndex) {
    if (!m_server || clientIndex >= m_clients.size()) {
        return {};
    }

    return m_differ.compare(
        m_server->getRegistry(),
        m_clients[clientIndex]->getRegistry(),
        m_config.diffOptions
    );
}

TestClient* TestHarness::getClient(std::size_t index) {
    if (index >= m_clients.size()) return nullptr;
    return m_clients[index].get();
}

const TestClient* TestHarness::getClient(std::size_t index) const {
    if (index >= m_clients.size()) return nullptr;
    return m_clients[index].get();
}

std::vector<TestClient*> TestHarness::getAllClients() {
    std::vector<TestClient*> result;
    result.reserve(m_clients.size());
    for (auto& client : m_clients) {
        result.push_back(client.get());
    }
    return result;
}

std::size_t TestHarness::getConnectedClientCount() const {
    std::size_t count = 0;
    for (const auto& client : m_clients) {
        if (client->isConnected()) {
            count++;
        }
    }
    return count;
}

void TestHarness::withClient(std::size_t clientIndex,
                              std::function<void(TestClient&)> action) {
    if (clientIndex < m_clients.size()) {
        action(*m_clients[clientIndex]);
    }
}

void TestHarness::withAllClients(std::function<void(TestClient&, std::size_t)> action) {
    for (std::size_t i = 0; i < m_clients.size(); ++i) {
        action(*m_clients[i], i);
    }
}

bool TestHarness::allClientsConnected() const {
    for (const auto& client : m_clients) {
        if (!client->isConnected()) {
            return false;
        }
    }
    return !m_clients.empty();
}

SimulationTick TestHarness::getCurrentTick() const {
    if (m_server) {
        return m_server->getCurrentTick();
    }
    return 0;
}

std::uint32_t TestHarness::getDefaultTimeout() const {
    return m_config.defaultTimeoutMs;
}

} // namespace sims3000
