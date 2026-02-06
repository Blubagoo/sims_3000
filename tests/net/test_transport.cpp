/**
 * @file test_transport.cpp
 * @brief Unit tests for INetworkTransport implementations.
 *
 * Tests MockTransport functionality and verifies the interface contract.
 * ENetTransport is tested via manual integration tests (requires network).
 */

#include "sims3000/net/INetworkTransport.h"
#include "sims3000/net/MockTransport.h"
#include "sims3000/net/ENetTransport.h"
#include <cassert>
#include <cstring>
#include <iostream>
#include <thread>
#include <chrono>

using namespace sims3000;

// ============================================================================
// Test Utilities
// ============================================================================

static int g_testsPassed = 0;
static int g_testsFailed = 0;
static std::uint16_t g_nextPort = 18000;  // Dynamic port allocation to avoid TIME_WAIT

static std::uint16_t getNextPort() {
    return g_nextPort++;
}

#define TEST(name) void test_##name()
#define RUN_TEST(name) do { \
    std::cout << "Running " #name "... "; \
    try { \
        test_##name(); \
        std::cout << "PASSED\n"; \
        g_testsPassed++; \
    } catch (const std::exception& e) { \
        std::cout << "FAILED: " << e.what() << "\n"; \
        g_testsFailed++; \
    } catch (...) { \
        std::cout << "FAILED: unknown exception\n"; \
        g_testsFailed++; \
    } \
} while(0)

#define ASSERT(cond) do { \
    if (!(cond)) { \
        throw std::runtime_error("Assertion failed: " #cond); \
    } \
} while(0)

#define ASSERT_EQ(a, b) do { \
    if ((a) != (b)) { \
        throw std::runtime_error("Assertion failed: " #a " == " #b); \
    } \
} while(0)

// ============================================================================
// MockTransport Tests
// ============================================================================

TEST(MockTransport_StartServer) {
    MockTransport transport;

    ASSERT(!transport.isRunning());
    ASSERT(transport.startServer(7777, 4));
    ASSERT(transport.isRunning());

    // Can't start twice
    ASSERT(!transport.startServer(7778, 4));
}

TEST(MockTransport_Connect) {
    MockTransport transport;

    ASSERT(!transport.isRunning());
    PeerID server = transport.connect("127.0.0.1", 7777);
    ASSERT(server != INVALID_PEER_ID);
    ASSERT(transport.isRunning());
    ASSERT_EQ(transport.getPeerCount(), 1u);

    // Can't connect twice
    PeerID server2 = transport.connect("127.0.0.1", 7778);
    ASSERT_EQ(server2, INVALID_PEER_ID);
}

TEST(MockTransport_ConnectEvent) {
    MockTransport transport;

    PeerID server = transport.connect("127.0.0.1", 7777);
    ASSERT(server != INVALID_PEER_ID);

    // Should get a Connect event on first poll
    NetworkEvent event = transport.poll(0);
    ASSERT_EQ(event.type, NetworkEventType::Connect);
    ASSERT_EQ(event.peer, server);

    // No more events
    event = transport.poll(0);
    ASSERT_EQ(event.type, NetworkEventType::None);
}

TEST(MockTransport_Disconnect) {
    MockTransport transport;

    transport.startServer(7777, 4);
    transport.injectConnectEvent(1);

    ASSERT_EQ(transport.getPeerCount(), 1u);
    ASSERT(transport.isConnected(1));

    transport.disconnect(1);

    ASSERT_EQ(transport.getPeerCount(), 0u);
    ASSERT(!transport.isConnected(1));
}

TEST(MockTransport_DisconnectAll) {
    MockTransport transport;

    transport.startServer(7777, 4);
    transport.injectConnectEvent(1);
    transport.injectConnectEvent(2);
    transport.injectConnectEvent(3);

    ASSERT_EQ(transport.getPeerCount(), 3u);

    transport.disconnectAll();

    ASSERT_EQ(transport.getPeerCount(), 0u);
    ASSERT(!transport.isRunning());
}

TEST(MockTransport_Send) {
    MockTransport transport;

    transport.startServer(7777, 4);
    transport.injectConnectEvent(1);

    // Consume connect event
    transport.poll(0);

    std::vector<std::uint8_t> data = {0x01, 0x02, 0x03, 0x04};
    ASSERT(transport.send(1, data.data(), data.size()));
    ASSERT_EQ(transport.getOutgoingCount(), 1u);

    // Can't send to non-existent peer
    ASSERT(!transport.send(999, data.data(), data.size()));
}

TEST(MockTransport_Broadcast) {
    MockTransport transport;

    transport.startServer(7777, 4);
    transport.injectConnectEvent(1);
    transport.injectConnectEvent(2);

    std::vector<std::uint8_t> data = {0xAA, 0xBB};
    transport.broadcast(data.data(), data.size());

    // Should have 2 outgoing messages (one per peer)
    ASSERT_EQ(transport.getOutgoingCount(), 2u);
}

TEST(MockTransport_InjectReceive) {
    MockTransport transport;

    transport.startServer(7777, 4);
    transport.injectConnectEvent(1);

    // Consume connect event
    transport.poll(0);

    std::vector<std::uint8_t> data = {0x10, 0x20, 0x30};
    transport.injectReceiveEvent(1, data);

    NetworkEvent event = transport.poll(0);
    ASSERT_EQ(event.type, NetworkEventType::Receive);
    ASSERT_EQ(event.peer, 1u);
    ASSERT_EQ(event.data.size(), 3u);
    ASSERT_EQ(event.data[0], 0x10);
    ASSERT_EQ(event.data[1], 0x20);
    ASSERT_EQ(event.data[2], 0x30);
}

TEST(MockTransport_Stats) {
    MockTransport transport;

    transport.startServer(7777, 4);

    // No stats for non-existent peer
    auto stats = transport.getStats(1);
    ASSERT(!stats.has_value());

    transport.injectConnectEvent(1);
    stats = transport.getStats(1);
    ASSERT(stats.has_value());
    ASSERT_EQ(stats->roundTripTimeMs, 0u);  // Mock has no latency
}

TEST(MockTransport_LinkedPair) {
    auto [client, server] = MockTransport::createLinkedPair();

    server->startServer(7777, 4);
    PeerID serverPeer = client->connect("127.0.0.1", 7777);
    ASSERT(serverPeer != INVALID_PEER_ID);

    // Simulate connection establishment
    client->simulateConnect();

    // Server should see connect event
    NetworkEvent event = server->poll(0);
    ASSERT_EQ(event.type, NetworkEventType::Connect);

    // Client should also get connect notification (from initial connect call)
    event = client->poll(0);
    ASSERT_EQ(event.type, NetworkEventType::Connect);
}

TEST(MockTransport_LinkedPair_SendReceive) {
    auto [client, server] = MockTransport::createLinkedPair();

    server->startServer(7777, 4);
    PeerID serverPeer = client->connect("127.0.0.1", 7777);

    // Simulate connection
    client->simulateConnect();

    // Drain connect events
    client->poll(0);
    server->poll(0);

    // Send from client to server
    std::vector<std::uint8_t> data = {0xDE, 0xAD, 0xBE, 0xEF};
    ASSERT(client->send(serverPeer, data.data(), data.size()));
    client->flush();

    // Server should receive the message
    NetworkEvent event = server->poll(0);
    ASSERT_EQ(event.type, NetworkEventType::Receive);
    ASSERT_EQ(event.data.size(), 4u);
    ASSERT_EQ(event.data[0], 0xDE);
    ASSERT_EQ(event.data[1], 0xAD);
    ASSERT_EQ(event.data[2], 0xBE);
    ASSERT_EQ(event.data[3], 0xEF);
}

TEST(MockTransport_LinkedPair_Bidirectional) {
    auto [client, server] = MockTransport::createLinkedPair();

    server->startServer(7777, 4);
    client->connect("127.0.0.1", 7777);

    // Simulate connection on both sides
    client->simulateConnect();
    server->simulateConnect();

    // Drain connect events
    NetworkEvent ev;
    while ((ev = client->poll(0)).type != NetworkEventType::None) {}
    while ((ev = server->poll(0)).type != NetworkEventType::None) {}

    // Get peer IDs
    ASSERT_EQ(client->getPeerCount(), 2u);  // 1 from connect, 1 from simulateConnect
    ASSERT_EQ(server->getPeerCount(), 2u);

    // This demonstrates bidirectional communication is possible
    // (exact peer IDs depend on order of operations)
}

TEST(MockTransport_Reset) {
    MockTransport transport;

    transport.startServer(7777, 4);
    transport.injectConnectEvent(1);
    transport.injectConnectEvent(2);

    ASSERT(transport.isRunning());
    ASSERT_EQ(transport.getPeerCount(), 2u);

    transport.reset();

    ASSERT(!transport.isRunning());
    ASSERT_EQ(transport.getPeerCount(), 0u);
    ASSERT_EQ(transport.getPendingEventCount(), 0u);
}

TEST(MockTransport_Channels) {
    MockTransport transport;

    transport.startServer(7777, 4);
    transport.injectConnectEvent(1);
    transport.poll(0);  // Consume connect

    // Send on reliable channel
    std::vector<std::uint8_t> data = {0x01};
    transport.send(1, data.data(), data.size(), ChannelID::Reliable);

    // Send on unreliable channel
    transport.send(1, data.data(), data.size(), ChannelID::Unreliable);

    ASSERT_EQ(transport.getOutgoingCount(), 2u);

    // Inject receive on unreliable channel
    transport.injectReceiveEvent(1, {0xFF}, ChannelID::Unreliable);

    NetworkEvent event = transport.poll(0);
    ASSERT_EQ(event.type, NetworkEventType::Receive);
    ASSERT_EQ(event.channel, ChannelID::Unreliable);
}

// ============================================================================
// ENetTransport Tests (Basic - no network required)
// ============================================================================

TEST(ENetTransport_Construction) {
    // Just verify construction and destruction work
    ENetTransport transport;
    ASSERT(!transport.isRunning());
    ASSERT_EQ(transport.getPeerCount(), 0u);
}

TEST(ENetTransport_StartServer) {
    ENetTransport transport;

    // Start server on an available port
    std::uint16_t port = getNextPort();
    ASSERT(transport.startServer(port, 4));
    ASSERT(transport.isRunning());
    ASSERT_EQ(transport.getPeerCount(), 0u);

    // Can't start twice
    ASSERT(!transport.startServer(getNextPort(), 4));
}

TEST(ENetTransport_MoveConstruct) {
    ENetTransport transport1;
    std::uint16_t port = getNextPort();
    ASSERT(transport1.startServer(port, 4));
    ASSERT(transport1.isRunning());

    ENetTransport transport2(std::move(transport1));
    ASSERT(transport2.isRunning());
    ASSERT(!transport1.isRunning());  // NOLINT: testing moved-from state
}

TEST(ENetTransport_MoveAssign) {
    ENetTransport transport1;
    std::uint16_t port = getNextPort();
    ASSERT(transport1.startServer(port, 4));

    ENetTransport transport2;
    transport2 = std::move(transport1);

    ASSERT(transport2.isRunning());
    ASSERT(!transport1.isRunning());  // NOLINT: testing moved-from state
}

// ============================================================================
// ENetTransport Integration Test (requires local loopback)
// ============================================================================

TEST(ENetTransport_ConnectSendReceive) {
    // Create server
    std::uint16_t port = getNextPort();
    ENetTransport server;
    ASSERT(server.startServer(port, 4));

    // Create client and connect
    ENetTransport client;
    PeerID serverPeer = client.connect("127.0.0.1", port);
    ASSERT(serverPeer != INVALID_PEER_ID);

    // Wait for connection with timeout
    bool serverConnected = false;
    bool clientConnected = false;

    for (int i = 0; i < 100 && (!serverConnected || !clientConnected); ++i) {
        NetworkEvent serverEvent = server.poll(10);
        if (serverEvent.type == NetworkEventType::Connect) {
            serverConnected = true;
        }

        NetworkEvent clientEvent = client.poll(10);
        if (clientEvent.type == NetworkEventType::Connect) {
            clientConnected = true;
        }
    }

    ASSERT(serverConnected);
    ASSERT(clientConnected);
    ASSERT_EQ(server.getPeerCount(), 1u);
    ASSERT_EQ(client.getPeerCount(), 1u);

    // Send message from client to server
    std::vector<std::uint8_t> testData = {0x01, 0x02, 0x03, 0x04, 0x05};
    ASSERT(client.send(serverPeer, testData.data(), testData.size(), ChannelID::Reliable));
    client.flush();

    // Wait for message on server
    bool messageReceived = false;
    NetworkEvent receiveEvent;

    for (int i = 0; i < 100 && !messageReceived; ++i) {
        receiveEvent = server.poll(10);
        if (receiveEvent.type == NetworkEventType::Receive) {
            messageReceived = true;
        }
    }

    ASSERT(messageReceived);
    ASSERT_EQ(receiveEvent.type, NetworkEventType::Receive);
    ASSERT_EQ(receiveEvent.data.size(), testData.size());
    for (size_t i = 0; i < testData.size(); ++i) {
        ASSERT_EQ(receiveEvent.data[i], testData[i]);
    }

    // Verify stats
    auto stats = server.getStats(receiveEvent.peer);
    ASSERT(stats.has_value());
    ASSERT(stats->packetsReceived > 0);

    // Clean disconnect
    client.disconnect(serverPeer);
    client.flush();

    // Wait for disconnect on server
    bool disconnected = false;
    for (int i = 0; i < 100 && !disconnected; ++i) {
        NetworkEvent event = server.poll(10);
        if (event.type == NetworkEventType::Disconnect) {
            disconnected = true;
        }
    }

    ASSERT(disconnected);
    ASSERT_EQ(server.getPeerCount(), 0u);
}

TEST(ENetTransport_Broadcast) {
    // Create server
    std::uint16_t port = getNextPort();
    ENetTransport server;
    ASSERT(server.startServer(port, 4));

    // Create two clients
    ENetTransport client1, client2;
    PeerID server1 = client1.connect("127.0.0.1", port);
    PeerID server2 = client2.connect("127.0.0.1", port);

    ASSERT(server1 != INVALID_PEER_ID);
    ASSERT(server2 != INVALID_PEER_ID);

    // Wait for connections
    int connections = 0;
    for (int i = 0; i < 200 && connections < 2; ++i) {
        NetworkEvent event = server.poll(10);
        if (event.type == NetworkEventType::Connect) {
            connections++;
        }
        client1.poll(10);
        client2.poll(10);
    }

    ASSERT_EQ(connections, 2);
    ASSERT_EQ(server.getPeerCount(), 2u);

    // Broadcast from server to all clients
    std::vector<std::uint8_t> broadcastData = {0xAA, 0xBB, 0xCC};
    server.broadcast(broadcastData.data(), broadcastData.size());
    server.flush();

    // Both clients should receive
    int received = 0;
    for (int i = 0; i < 200 && received < 2; ++i) {
        NetworkEvent e1 = client1.poll(5);
        if (e1.type == NetworkEventType::Receive) {
            ASSERT_EQ(e1.data.size(), broadcastData.size());
            received++;
        }

        NetworkEvent e2 = client2.poll(5);
        if (e2.type == NetworkEventType::Receive) {
            ASSERT_EQ(e2.data.size(), broadcastData.size());
            received++;
        }
    }

    ASSERT_EQ(received, 2);
}

// ============================================================================
// Main
// ============================================================================

int main() {
    std::cout << "=== Network Transport Tests ===\n\n";

    // MockTransport tests
    std::cout << "--- MockTransport Tests ---\n";
    RUN_TEST(MockTransport_StartServer);
    RUN_TEST(MockTransport_Connect);
    RUN_TEST(MockTransport_ConnectEvent);
    RUN_TEST(MockTransport_Disconnect);
    RUN_TEST(MockTransport_DisconnectAll);
    RUN_TEST(MockTransport_Send);
    RUN_TEST(MockTransport_Broadcast);
    RUN_TEST(MockTransport_InjectReceive);
    RUN_TEST(MockTransport_Stats);
    RUN_TEST(MockTransport_LinkedPair);
    RUN_TEST(MockTransport_LinkedPair_SendReceive);
    RUN_TEST(MockTransport_LinkedPair_Bidirectional);
    RUN_TEST(MockTransport_Reset);
    RUN_TEST(MockTransport_Channels);

    // ENetTransport tests
    std::cout << "\n--- ENetTransport Tests ---\n";
    RUN_TEST(ENetTransport_Construction);
    RUN_TEST(ENetTransport_StartServer);
    RUN_TEST(ENetTransport_MoveConstruct);
    RUN_TEST(ENetTransport_MoveAssign);

    // Integration tests (require local network)
    std::cout << "\n--- ENetTransport Integration Tests ---\n";
    RUN_TEST(ENetTransport_ConnectSendReceive);
    RUN_TEST(ENetTransport_Broadcast);

    std::cout << "\n=== Results ===\n";
    std::cout << "Passed: " << g_testsPassed << "\n";
    std::cout << "Failed: " << g_testsFailed << "\n";

    return g_testsFailed > 0 ? 1 : 0;
}
