/**
 * @file MockTransport.h
 * @brief Mock implementation of INetworkTransport for testing.
 *
 * Provides an in-memory transport that stores messages in queues,
 * allowing unit tests to verify network behavior without actual
 * network connections.
 *
 * Can be configured in two modes:
 * 1. Standalone: Single instance with internal message queue
 * 2. Linked: Two instances connected together for client-server tests
 *
 * Ownership: Test code owns MockTransport instances.
 * Cleanup: Destructor clears all queues. No external resources.
 *
 * Thread safety: Not thread-safe. Single-threaded test use only.
 */

#ifndef SIMS3000_NET_MOCKTRANSPORT_H
#define SIMS3000_NET_MOCKTRANSPORT_H

#include "sims3000/net/INetworkTransport.h"
#include <queue>
#include <memory>
#include <set>
#include <cstring>

namespace sims3000 {

/**
 * @class MockTransport
 * @brief Mock network transport for testing without real network.
 *
 * Messages are stored in queues and can be retrieved via poll().
 * Two MockTransport instances can be linked to simulate client-server
 * communication.
 *
 * Example usage (standalone):
 * @code
 *   MockTransport transport;
 *   transport.startServer(7777, 4);
 *
 *   // Simulate incoming message
 *   transport.injectConnectEvent(1);
 *   transport.injectReceiveEvent(1, {0x01, 0x02, 0x03});
 *
 *   // Test code that uses INetworkTransport
 *   NetworkEvent event = transport.poll(0);
 *   assert(event.type == NetworkEventType::Connect);
 * @endcode
 *
 * Example usage (linked pair):
 * @code
 *   auto [client, server] = MockTransport::createLinkedPair();
 *
 *   server->startServer(7777, 4);
 *   PeerID serverPeer = client->connect("127.0.0.1", 7777);
 *
 *   // Simulate handshake
 *   client->simulateConnect();
 *   server->simulateConnect();
 *
 *   // Send from client to server
 *   std::vector<uint8_t> data = {0x01, 0x02};
 *   client->send(serverPeer, data.data(), data.size());
 *   client->flush();
 *
 *   // Receive on server
 *   NetworkEvent event = server->poll(0);
 *   assert(event.type == NetworkEventType::Receive);
 * @endcode
 */
class MockTransport : public INetworkTransport {
public:
    MockTransport() = default;
    ~MockTransport() override = default;

    // Non-copyable but movable
    MockTransport(const MockTransport&) = delete;
    MockTransport& operator=(const MockTransport&) = delete;
    MockTransport(MockTransport&&) = default;
    MockTransport& operator=(MockTransport&&) = default;

    // ========================================================================
    // INetworkTransport Implementation
    // ========================================================================

    bool startServer(std::uint16_t port, std::uint32_t maxClients) override {
        if (m_running) return false;
        m_running = true;
        m_isServer = true;
        m_port = port;
        m_maxClients = maxClients;
        return true;
    }

    PeerID connect(const std::string& address, std::uint16_t port) override {
        if (m_running) return INVALID_PEER_ID;
        m_running = true;
        m_isServer = false;
        m_serverAddress = address;
        m_port = port;

        // Create a peer ID for the server
        PeerID serverPeer = m_nextPeerId++;
        m_connectedPeers.insert(serverPeer);

        // Queue a pending connect that will be delivered on poll
        m_pendingServerPeer = serverPeer;

        return serverPeer;
    }

    void disconnect(PeerID peer) override {
        if (m_connectedPeers.erase(peer) > 0) {
            // Notify linked transport if present
            if (m_linkedTransport) {
                NetworkEvent event;
                event.type = NetworkEventType::Disconnect;
                event.peer = m_linkedPeerId;
                m_linkedTransport->m_eventQueue.push(event);
            }
        }
    }

    void disconnectAll() override {
        for (PeerID peer : m_connectedPeers) {
            if (m_linkedTransport) {
                NetworkEvent event;
                event.type = NetworkEventType::Disconnect;
                event.peer = m_linkedPeerId;
                m_linkedTransport->m_eventQueue.push(event);
            }
        }
        m_connectedPeers.clear();
        m_running = false;
    }

    bool isRunning() const override {
        return m_running;
    }

    bool send(PeerID peer, const void* data, std::size_t size,
              ChannelID channel = ChannelID::Reliable) override {
        if (!m_connectedPeers.count(peer)) {
            return false;
        }

        // Store in outgoing queue for flush()
        PendingMessage msg;
        msg.peer = peer;
        msg.data.resize(size);
        std::memcpy(msg.data.data(), data, size);
        msg.channel = channel;
        m_outgoing.push(msg);

        return true;
    }

    void broadcast(const void* data, std::size_t size,
                   ChannelID channel = ChannelID::Reliable) override {
        for (PeerID peer : m_connectedPeers) {
            send(peer, data, size, channel);
        }
    }

    NetworkEvent poll(std::uint32_t /*timeoutMs*/ = 0) override {
        // Check for pending connect event from connect() call
        if (m_pendingServerPeer != INVALID_PEER_ID) {
            NetworkEvent event;
            event.type = NetworkEventType::Connect;
            event.peer = m_pendingServerPeer;
            m_pendingServerPeer = INVALID_PEER_ID;
            return event;
        }

        if (m_eventQueue.empty()) {
            return NetworkEvent{};  // type = None
        }

        NetworkEvent event = m_eventQueue.front();
        m_eventQueue.pop();
        return event;
    }

    void flush() override {
        // Transfer outgoing messages to linked transport
        if (m_linkedTransport) {
            while (!m_outgoing.empty()) {
                PendingMessage& msg = m_outgoing.front();

                NetworkEvent event;
                event.type = NetworkEventType::Receive;
                event.peer = m_linkedPeerId;
                event.data = std::move(msg.data);
                event.channel = msg.channel;
                m_linkedTransport->m_eventQueue.push(event);

                m_outgoing.pop();
            }
        } else {
            // No linked transport - just clear outgoing (for standalone use)
            while (!m_outgoing.empty()) {
                m_outgoing.pop();
            }
        }
    }

    std::uint32_t getPeerCount() const override {
        return static_cast<std::uint32_t>(m_connectedPeers.size());
    }

    std::optional<NetworkStats> getStats(PeerID peer) const override {
        if (!m_connectedPeers.count(peer)) {
            return std::nullopt;
        }

        NetworkStats stats;
        stats.packetsSent = m_packetsSent;
        stats.packetsReceived = m_packetsReceived;
        stats.bytesSent = m_bytesSent;
        stats.bytesReceived = m_bytesReceived;
        stats.roundTripTimeMs = 0;  // Mock has no latency
        stats.packetLoss = 0;

        return stats;
    }

    bool isConnected(PeerID peer) const override {
        return m_connectedPeers.count(peer) > 0;
    }

    // ========================================================================
    // Mock-specific Methods for Testing
    // ========================================================================

    /**
     * @brief Create a linked pair of transports for client-server testing.
     * @return Pair of (client, server) transports
     */
    static std::pair<std::unique_ptr<MockTransport>, std::unique_ptr<MockTransport>>
    createLinkedPair() {
        auto client = std::make_unique<MockTransport>();
        auto server = std::make_unique<MockTransport>();

        client->m_linkedTransport = server.get();
        server->m_linkedTransport = client.get();

        return {std::move(client), std::move(server)};
    }

    /**
     * @brief Inject a connect event into the event queue.
     * @param peer Peer ID for the connecting peer
     */
    void injectConnectEvent(PeerID peer) {
        NetworkEvent event;
        event.type = NetworkEventType::Connect;
        event.peer = peer;
        m_eventQueue.push(event);
        m_connectedPeers.insert(peer);
    }

    /**
     * @brief Inject a disconnect event into the event queue.
     * @param peer Peer ID for the disconnecting peer
     */
    void injectDisconnectEvent(PeerID peer) {
        NetworkEvent event;
        event.type = NetworkEventType::Disconnect;
        event.peer = peer;
        m_eventQueue.push(event);
        m_connectedPeers.erase(peer);
    }

    /**
     * @brief Inject a receive event into the event queue.
     * @param peer Source peer
     * @param data Received data
     * @param channel Channel data was received on
     */
    void injectReceiveEvent(PeerID peer, std::vector<std::uint8_t> data,
                            ChannelID channel = ChannelID::Reliable) {
        NetworkEvent event;
        event.type = NetworkEventType::Receive;
        event.peer = peer;
        event.data = std::move(data);
        event.channel = channel;
        m_eventQueue.push(event);
        m_packetsReceived++;
        m_bytesReceived += static_cast<std::uint32_t>(event.data.size());
    }

    /**
     * @brief Simulate the connection handshake completing.
     *
     * For linked transports, call on both client and server to
     * establish the connection.
     */
    void simulateConnect() {
        if (m_linkedTransport) {
            PeerID peerId = m_nextPeerId++;
            m_connectedPeers.insert(peerId);
            m_linkedPeerId = peerId;

            // Tell the other side about the connection
            NetworkEvent event;
            event.type = NetworkEventType::Connect;
            event.peer = peerId;
            m_linkedTransport->m_eventQueue.push(event);
            m_linkedTransport->m_connectedPeers.insert(peerId);
            m_linkedTransport->m_linkedPeerId = peerId;
        }
    }

    /**
     * @brief Get number of pending events in queue.
     * @return Event count
     */
    std::size_t getPendingEventCount() const {
        return m_eventQueue.size();
    }

    /**
     * @brief Get number of outgoing messages waiting for flush.
     * @return Message count
     */
    std::size_t getOutgoingCount() const {
        return m_outgoing.size();
    }

    /**
     * @brief Clear all queues and reset state.
     */
    void reset() {
        while (!m_eventQueue.empty()) m_eventQueue.pop();
        while (!m_outgoing.empty()) m_outgoing.pop();
        m_connectedPeers.clear();
        m_running = false;
        m_packetsSent = 0;
        m_packetsReceived = 0;
        m_bytesSent = 0;
        m_bytesReceived = 0;
    }

private:
    struct PendingMessage {
        PeerID peer;
        std::vector<std::uint8_t> data;
        ChannelID channel;
    };

    bool m_running = false;
    bool m_isServer = false;
    std::uint16_t m_port = 0;
    std::uint32_t m_maxClients = 0;
    std::string m_serverAddress;

    std::set<PeerID> m_connectedPeers;
    PeerID m_nextPeerId = 1;
    PeerID m_pendingServerPeer = INVALID_PEER_ID;

    std::queue<NetworkEvent> m_eventQueue;
    std::queue<PendingMessage> m_outgoing;

    // For linked pair testing
    MockTransport* m_linkedTransport = nullptr;
    PeerID m_linkedPeerId = INVALID_PEER_ID;

    // Statistics
    std::uint32_t m_packetsSent = 0;
    std::uint32_t m_packetsReceived = 0;
    std::uint32_t m_bytesSent = 0;
    std::uint32_t m_bytesReceived = 0;
};

} // namespace sims3000

#endif // SIMS3000_NET_MOCKTRANSPORT_H
