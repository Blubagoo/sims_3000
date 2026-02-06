/**
 * @file MockSocket.h
 * @brief Enhanced mock transport with network condition simulation.
 *
 * Extends MockTransport with:
 * - Configurable latency injection
 * - Configurable packet loss percentage
 * - Configurable bandwidth limits
 * - Message interception for verification
 * - Deterministic control via seed
 *
 * Usage:
 *   MockSocket socket(ConnectionQualityProfiles::POOR_WIFI);
 *   socket.startServer(0, 4);  // Port 0 for automatic assignment
 *
 *   // Intercept messages for verification
 *   socket.setMessageInterceptor([](const InterceptedMessage& msg) {
 *       if (msg.type == MessageType::Input) {
 *           // Verify input message
 *       }
 *   });
 *
 * Ownership: Test code owns MockSocket instances.
 * Cleanup: Destructor clears all queues and timers. No external resources.
 *
 * Thread safety: Not thread-safe. Single-threaded test use only.
 */

#ifndef SIMS3000_TEST_MOCKSOCKET_H
#define SIMS3000_TEST_MOCKSOCKET_H

#include "sims3000/net/INetworkTransport.h"
#include "sims3000/test/ConnectionQualityProfiles.h"
#include <queue>
#include <vector>
#include <memory>
#include <set>
#include <functional>
#include <cstdint>
#include <random>

namespace sims3000 {

/**
 * @struct InterceptedMessage
 * @brief Captured message data for test verification.
 */
struct InterceptedMessage {
    PeerID sourcePeer = INVALID_PEER_ID;
    PeerID destPeer = INVALID_PEER_ID;
    std::vector<std::uint8_t> data;
    ChannelID channel = ChannelID::Reliable;
    std::uint64_t timestampMs = 0;  // When message was sent
    bool wasDropped = false;         // True if packet loss dropped this
    bool wasDelayed = false;         // True if latency applied
};

/**
 * @struct PendingPacket
 * @brief A packet waiting to be delivered after latency delay.
 */
struct PendingPacket {
    NetworkEvent event;
    std::uint64_t deliveryTimeMs = 0;  // When to deliver
};

/**
 * @class MockSocket
 * @brief Mock network transport with network condition simulation.
 *
 * Provides in-memory message passing with configurable network degradation.
 * Suitable for unit tests that need to verify behavior under various
 * network conditions without real network connections.
 */
class MockSocket : public INetworkTransport {
public:
    /// Message interceptor callback type
    using MessageInterceptor = std::function<void(const InterceptedMessage&)>;

    /**
     * @brief Construct a MockSocket with perfect network conditions.
     */
    MockSocket();

    /**
     * @brief Construct a MockSocket with specified network conditions.
     * @param conditions Network condition preset or custom conditions.
     */
    explicit MockSocket(const NetworkConditions& conditions);

    /**
     * @brief Construct a MockSocket with deterministic random seed.
     * @param conditions Network conditions.
     * @param seed Random seed for deterministic packet loss/jitter.
     */
    MockSocket(const NetworkConditions& conditions, std::uint64_t seed);

    ~MockSocket() override;

    // Non-copyable but movable
    MockSocket(const MockSocket&) = delete;
    MockSocket& operator=(const MockSocket&) = delete;
    MockSocket(MockSocket&&) = default;
    MockSocket& operator=(MockSocket&&) = default;

    // =========================================================================
    // INetworkTransport Implementation
    // =========================================================================

    bool startServer(std::uint16_t port, std::uint32_t maxClients) override;
    PeerID connect(const std::string& address, std::uint16_t port) override;
    void disconnect(PeerID peer) override;
    void disconnectAll() override;
    bool isRunning() const override;
    bool send(PeerID peer, const void* data, std::size_t size,
              ChannelID channel = ChannelID::Reliable) override;
    void broadcast(const void* data, std::size_t size,
                   ChannelID channel = ChannelID::Reliable) override;
    NetworkEvent poll(std::uint32_t timeoutMs = 0) override;
    void flush() override;
    std::uint32_t getPeerCount() const override;
    std::optional<NetworkStats> getStats(PeerID peer) const override;
    bool isConnected(PeerID peer) const override;

    // =========================================================================
    // Network Condition Configuration
    // =========================================================================

    /**
     * @brief Set network conditions.
     * @param conditions Network condition parameters.
     *
     * Can be changed at any time to simulate network quality changes.
     */
    void setNetworkConditions(const NetworkConditions& conditions);

    /**
     * @brief Get current network conditions.
     */
    const NetworkConditions& getNetworkConditions() const { return m_conditions; }

    /**
     * @brief Set the random seed for deterministic behavior.
     * @param seed Random seed.
     */
    void setSeed(std::uint64_t seed);

    // =========================================================================
    // Message Interception
    // =========================================================================

    /**
     * @brief Set a callback to intercept all messages.
     * @param interceptor Callback function, or nullptr to disable.
     *
     * The interceptor is called for every message sent through this socket,
     * including dropped and delayed messages.
     */
    void setMessageInterceptor(MessageInterceptor interceptor);

    /**
     * @brief Get all intercepted messages.
     * @return Vector of all messages sent through this socket.
     *
     * Messages are accumulated until clearInterceptedMessages() is called.
     */
    const std::vector<InterceptedMessage>& getInterceptedMessages() const;

    /**
     * @brief Clear the intercepted message history.
     */
    void clearInterceptedMessages();

    // =========================================================================
    // Time Control
    // =========================================================================

    /**
     * @brief Advance simulated time.
     * @param deltaMs Time to advance in milliseconds.
     *
     * Processes pending packets that should be delivered by the new time.
     * Call this to simulate time passing for latency delivery.
     */
    void advanceTime(std::uint64_t deltaMs);

    /**
     * @brief Get current simulated time.
     * @return Current time in milliseconds.
     */
    std::uint64_t getCurrentTime() const { return m_currentTimeMs; }

    /**
     * @brief Set current simulated time directly.
     * @param timeMs New time in milliseconds.
     */
    void setCurrentTime(std::uint64_t timeMs) { m_currentTimeMs = timeMs; }

    // =========================================================================
    // Linked Socket Support
    // =========================================================================

    /**
     * @brief Create a linked pair of sockets for client-server testing.
     * @return Pair of (client, server) sockets.
     *
     * Messages sent on one socket are delivered to the linked socket.
     */
    static std::pair<std::unique_ptr<MockSocket>, std::unique_ptr<MockSocket>>
    createLinkedPair(const NetworkConditions& conditions = ConnectionQualityProfiles::PERFECT);

    /**
     * @brief Link this socket to another socket.
     * @param other Socket to link to.
     * @param peerId Peer ID to use for the link.
     *
     * After linking, messages sent to peerId will be delivered to the other socket.
     */
    void linkTo(MockSocket* other, PeerID peerId);

    /**
     * @brief Simulate connection establishment for linked pair.
     *
     * For linked transports, call on both client and server to
     * establish the connection and trigger connect events.
     */
    void simulateConnect();

    // =========================================================================
    // Event Injection
    // =========================================================================

    /**
     * @brief Inject a connect event into the event queue.
     * @param peer Peer ID for the connecting peer.
     */
    void injectConnectEvent(PeerID peer);

    /**
     * @brief Inject a disconnect event into the event queue.
     * @param peer Peer ID for the disconnecting peer.
     */
    void injectDisconnectEvent(PeerID peer);

    /**
     * @brief Inject a receive event into the event queue.
     * @param peer Source peer.
     * @param data Received data.
     * @param channel Channel data was received on.
     */
    void injectReceiveEvent(PeerID peer, std::vector<std::uint8_t> data,
                            ChannelID channel = ChannelID::Reliable);

    // =========================================================================
    // Test Utilities
    // =========================================================================

    /**
     * @brief Get number of pending events in queue.
     */
    std::size_t getPendingEventCount() const;

    /**
     * @brief Get number of outgoing messages waiting for flush.
     */
    std::size_t getOutgoingCount() const;

    /**
     * @brief Get number of packets waiting for delivery (delayed by latency).
     */
    std::size_t getPendingDeliveryCount() const;

    /**
     * @brief Get total packets dropped due to packet loss simulation.
     */
    std::uint64_t getDroppedPacketCount() const { return m_droppedPackets; }

    /**
     * @brief Get total bytes sent (including dropped).
     */
    std::uint64_t getTotalBytesSent() const { return m_totalBytesSent; }

    /**
     * @brief Get total bytes received (excluding dropped).
     */
    std::uint64_t getTotalBytesReceived() const { return m_totalBytesReceived; }

    /**
     * @brief Reset all statistics and clear queues.
     */
    void reset();

    /**
     * @brief Get the automatically assigned port (after startServer with port 0).
     */
    std::uint16_t getAssignedPort() const { return m_assignedPort; }

private:
    /// Decide if a packet should be dropped based on loss percentage
    bool shouldDropPacket();

    /// Calculate delivery time with latency and jitter
    std::uint64_t calculateDeliveryTime();

    /// Process pending packets and move to event queue
    void processPendingPackets();

    /// Check bandwidth limit
    bool checkBandwidth(std::size_t bytes);

    struct PendingMessage {
        PeerID peer;
        std::vector<std::uint8_t> data;
        ChannelID channel;
    };

    // Network conditions
    NetworkConditions m_conditions;
    std::mt19937_64 m_rng;

    // State
    bool m_running = false;
    bool m_isServer = false;
    std::uint16_t m_port = 0;
    std::uint16_t m_assignedPort = 0;
    std::uint32_t m_maxClients = 0;
    std::string m_serverAddress;

    // Peer management
    std::set<PeerID> m_connectedPeers;
    PeerID m_nextPeerId = 1;
    PeerID m_pendingServerPeer = INVALID_PEER_ID;

    // Queues
    std::queue<NetworkEvent> m_eventQueue;
    std::queue<PendingMessage> m_outgoing;
    std::vector<PendingPacket> m_pendingPackets;  // Delayed by latency

    // Linked socket support
    MockSocket* m_linkedSocket = nullptr;
    PeerID m_linkedPeerId = INVALID_PEER_ID;

    // Message interception
    MessageInterceptor m_interceptor;
    std::vector<InterceptedMessage> m_interceptedMessages;

    // Time simulation
    std::uint64_t m_currentTimeMs = 0;

    // Bandwidth tracking
    std::uint64_t m_bandwidthWindowStart = 0;
    std::uint64_t m_bytesThisWindow = 0;

    // Statistics
    std::uint64_t m_droppedPackets = 0;
    std::uint64_t m_totalBytesSent = 0;
    std::uint64_t m_totalBytesReceived = 0;
    std::uint32_t m_packetsSent = 0;
    std::uint32_t m_packetsReceived = 0;

    // Port allocation for automatic assignment
    static std::uint16_t s_nextAutoPort;
};

} // namespace sims3000

#endif // SIMS3000_TEST_MOCKSOCKET_H
