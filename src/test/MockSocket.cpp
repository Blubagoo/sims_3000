/**
 * @file MockSocket.cpp
 * @brief Implementation of MockSocket for testing.
 */

#include "sims3000/test/MockSocket.h"
#include <algorithm>
#include <cstring>

namespace sims3000 {

// Start auto port assignment at 10000 (above well-known ports)
std::uint16_t MockSocket::s_nextAutoPort = 10000;

MockSocket::MockSocket()
    : m_conditions(ConnectionQualityProfiles::PERFECT)
    , m_rng(std::random_device{}())
{
}

MockSocket::MockSocket(const NetworkConditions& conditions)
    : m_conditions(conditions)
    , m_rng(std::random_device{}())
{
}

MockSocket::MockSocket(const NetworkConditions& conditions, std::uint64_t seed)
    : m_conditions(conditions)
    , m_rng(seed)
{
}

MockSocket::~MockSocket() {
    // Unlink from the linked socket to prevent dangling pointer access
    if (m_linkedSocket) {
        m_linkedSocket->m_linkedSocket = nullptr;
    }
}

bool MockSocket::startServer(std::uint16_t port, std::uint32_t maxClients) {
    if (m_running) return false;

    m_running = true;
    m_isServer = true;
    m_port = port;
    m_maxClients = maxClients;

    // Auto-assign port if 0
    if (port == 0) {
        m_assignedPort = s_nextAutoPort++;
    } else {
        m_assignedPort = port;
    }

    return true;
}

PeerID MockSocket::connect(const std::string& address, std::uint16_t port) {
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

void MockSocket::disconnect(PeerID peer) {
    if (m_connectedPeers.erase(peer) > 0) {
        // Notify linked socket if present
        if (m_linkedSocket) {
            NetworkEvent event;
            event.type = NetworkEventType::Disconnect;
            event.peer = m_linkedPeerId;
            m_linkedSocket->m_eventQueue.push(event);
        }
    }
}

void MockSocket::disconnectAll() {
    for (PeerID peer : m_connectedPeers) {
        if (m_linkedSocket) {
            NetworkEvent event;
            event.type = NetworkEventType::Disconnect;
            event.peer = m_linkedPeerId;
            m_linkedSocket->m_eventQueue.push(event);
        }
    }
    m_connectedPeers.clear();
    m_running = false;
}

bool MockSocket::isRunning() const {
    return m_running;
}

bool MockSocket::send(PeerID peer, const void* data, std::size_t size,
                      ChannelID channel) {
    if (!m_connectedPeers.count(peer)) {
        return false;
    }

    // Record for interception
    InterceptedMessage intercepted;
    intercepted.sourcePeer = 0; // Local
    intercepted.destPeer = peer;
    intercepted.data.resize(size);
    std::memcpy(intercepted.data.data(), data, size);
    intercepted.channel = channel;
    intercepted.timestampMs = m_currentTimeMs;
    intercepted.wasDropped = false;
    intercepted.wasDelayed = false;

    // Check packet loss
    if (shouldDropPacket()) {
        intercepted.wasDropped = true;
        m_droppedPackets++;
        m_interceptedMessages.push_back(intercepted);
        if (m_interceptor) {
            m_interceptor(intercepted);
        }
        return true; // From caller's perspective, send succeeded
    }

    // Check bandwidth
    if (!checkBandwidth(size)) {
        intercepted.wasDropped = true;
        m_droppedPackets++;
        m_interceptedMessages.push_back(intercepted);
        if (m_interceptor) {
            m_interceptor(intercepted);
        }
        return true; // Bandwidth exceeded, drop packet
    }

    m_totalBytesSent += size;

    // Store in outgoing queue for flush()
    PendingMessage msg;
    msg.peer = peer;
    msg.data.resize(size);
    std::memcpy(msg.data.data(), data, size);
    msg.channel = channel;
    m_outgoing.push(msg);

    // Check if latency applies
    if (m_conditions.latencyMs > 0 || m_conditions.jitterMs > 0) {
        intercepted.wasDelayed = true;
    }

    m_interceptedMessages.push_back(intercepted);
    if (m_interceptor) {
        m_interceptor(intercepted);
    }

    return true;
}

void MockSocket::broadcast(const void* data, std::size_t size,
                           ChannelID channel) {
    for (PeerID peer : m_connectedPeers) {
        send(peer, data, size, channel);
    }
}

NetworkEvent MockSocket::poll(std::uint32_t /*timeoutMs*/) {
    // Process any pending packets that should be delivered by now
    processPendingPackets();

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

void MockSocket::flush() {
    // Transfer outgoing messages to linked socket
    if (m_linkedSocket) {
        while (!m_outgoing.empty()) {
            PendingMessage& msg = m_outgoing.front();

            // Create the receive event
            NetworkEvent event;
            event.type = NetworkEventType::Receive;
            event.peer = m_linkedPeerId;
            event.data = std::move(msg.data);
            event.channel = msg.channel;

            // Apply latency if configured
            if (m_conditions.latencyMs > 0 || m_conditions.jitterMs > 0) {
                PendingPacket pending;
                pending.event = std::move(event);
                pending.deliveryTimeMs = calculateDeliveryTime();
                m_linkedSocket->m_pendingPackets.push_back(std::move(pending));
            } else {
                // Immediate delivery - capture size before move
                std::size_t dataSize = event.data.size();
                m_linkedSocket->m_eventQueue.push(std::move(event));
                m_linkedSocket->m_totalBytesReceived += dataSize;
            }

            m_outgoing.pop();
            m_packetsSent++;
        }
    } else {
        // No linked socket - just clear outgoing (for standalone use)
        while (!m_outgoing.empty()) {
            m_outgoing.pop();
        }
    }
}

std::uint32_t MockSocket::getPeerCount() const {
    return static_cast<std::uint32_t>(m_connectedPeers.size());
}

std::optional<NetworkStats> MockSocket::getStats(PeerID peer) const {
    if (!m_connectedPeers.count(peer)) {
        return std::nullopt;
    }

    NetworkStats stats;
    stats.packetsSent = m_packetsSent;
    stats.packetsReceived = m_packetsReceived;
    stats.bytesSent = static_cast<std::uint32_t>(m_totalBytesSent);
    stats.bytesReceived = static_cast<std::uint32_t>(m_totalBytesReceived);
    stats.roundTripTimeMs = m_conditions.latencyMs * 2;  // Approximate RTT
    stats.packetLoss = static_cast<std::uint32_t>(m_conditions.packetLossPercent);

    return stats;
}

bool MockSocket::isConnected(PeerID peer) const {
    return m_connectedPeers.count(peer) > 0;
}

void MockSocket::setNetworkConditions(const NetworkConditions& conditions) {
    m_conditions = conditions;
}

void MockSocket::setSeed(std::uint64_t seed) {
    m_rng.seed(seed);
}

void MockSocket::setMessageInterceptor(MessageInterceptor interceptor) {
    m_interceptor = std::move(interceptor);
}

const std::vector<InterceptedMessage>& MockSocket::getInterceptedMessages() const {
    return m_interceptedMessages;
}

void MockSocket::clearInterceptedMessages() {
    m_interceptedMessages.clear();
}

void MockSocket::advanceTime(std::uint64_t deltaMs) {
    m_currentTimeMs += deltaMs;
    processPendingPackets();
}

std::pair<std::unique_ptr<MockSocket>, std::unique_ptr<MockSocket>>
MockSocket::createLinkedPair(const NetworkConditions& conditions) {
    auto client = std::make_unique<MockSocket>(conditions);
    auto server = std::make_unique<MockSocket>(conditions);

    // Set up bidirectional link with a shared peer ID
    // Use peer ID 1 for the connection between client and server
    const PeerID linkPeerId = 1;

    client->m_linkedSocket = server.get();
    client->m_linkedPeerId = linkPeerId;
    client->m_connectedPeers.insert(linkPeerId);

    server->m_linkedSocket = client.get();
    server->m_linkedPeerId = linkPeerId;
    server->m_connectedPeers.insert(linkPeerId);

    return {std::move(client), std::move(server)};
}

void MockSocket::linkTo(MockSocket* other, PeerID peerId) {
    m_linkedSocket = other;
    m_linkedPeerId = peerId;
}

void MockSocket::simulateConnect() {
    if (m_linkedSocket) {
        // If already connected (e.g., from createLinkedPair), don't re-establish
        if (m_linkedPeerId != INVALID_PEER_ID && !m_connectedPeers.empty()) {
            return;
        }

        PeerID peerId = m_nextPeerId++;
        m_connectedPeers.insert(peerId);
        m_linkedPeerId = peerId;

        // Tell the other side about the connection
        NetworkEvent event;
        event.type = NetworkEventType::Connect;
        event.peer = peerId;
        m_linkedSocket->m_eventQueue.push(event);
        m_linkedSocket->m_connectedPeers.insert(peerId);
        m_linkedSocket->m_linkedPeerId = peerId;
    }
}

void MockSocket::injectConnectEvent(PeerID peer) {
    NetworkEvent event;
    event.type = NetworkEventType::Connect;
    event.peer = peer;
    m_eventQueue.push(event);
    m_connectedPeers.insert(peer);
}

void MockSocket::injectDisconnectEvent(PeerID peer) {
    NetworkEvent event;
    event.type = NetworkEventType::Disconnect;
    event.peer = peer;
    m_eventQueue.push(event);
    m_connectedPeers.erase(peer);
}

void MockSocket::injectReceiveEvent(PeerID peer, std::vector<std::uint8_t> data,
                                     ChannelID channel) {
    NetworkEvent event;
    event.type = NetworkEventType::Receive;
    event.peer = peer;
    event.data = std::move(data);
    event.channel = channel;
    m_eventQueue.push(event);
    m_packetsReceived++;
    m_totalBytesReceived += event.data.size();
}

std::size_t MockSocket::getPendingEventCount() const {
    return m_eventQueue.size();
}

std::size_t MockSocket::getOutgoingCount() const {
    return m_outgoing.size();
}

std::size_t MockSocket::getPendingDeliveryCount() const {
    return m_pendingPackets.size();
}

void MockSocket::reset() {
    while (!m_eventQueue.empty()) m_eventQueue.pop();
    while (!m_outgoing.empty()) m_outgoing.pop();
    m_pendingPackets.clear();
    m_connectedPeers.clear();
    m_interceptedMessages.clear();
    m_running = false;
    m_droppedPackets = 0;
    m_totalBytesSent = 0;
    m_totalBytesReceived = 0;
    m_packetsSent = 0;
    m_packetsReceived = 0;
    m_currentTimeMs = 0;
    m_bandwidthWindowStart = 0;
    m_bytesThisWindow = 0;
}

bool MockSocket::shouldDropPacket() {
    if (m_conditions.packetLossPercent <= 0.0f) {
        return false;
    }

    std::uniform_real_distribution<float> dist(0.0f, 100.0f);
    return dist(m_rng) < m_conditions.packetLossPercent;
}

std::uint64_t MockSocket::calculateDeliveryTime() {
    std::uint64_t latency = m_conditions.latencyMs;

    if (m_conditions.jitterMs > 0) {
        std::uniform_int_distribution<std::uint64_t> jitterDist(
            0, m_conditions.jitterMs * 2);
        std::int64_t jitter = static_cast<std::int64_t>(jitterDist(m_rng)) -
                              static_cast<std::int64_t>(m_conditions.jitterMs);
        latency = static_cast<std::uint64_t>(
            std::max(static_cast<std::int64_t>(0),
                     static_cast<std::int64_t>(latency) + jitter));
    }

    return m_currentTimeMs + latency;
}

void MockSocket::processPendingPackets() {
    // Sort by delivery time
    std::sort(m_pendingPackets.begin(), m_pendingPackets.end(),
              [](const PendingPacket& a, const PendingPacket& b) {
                  return a.deliveryTimeMs < b.deliveryTimeMs;
              });

    // Deliver packets that are ready
    auto it = m_pendingPackets.begin();
    while (it != m_pendingPackets.end()) {
        if (it->deliveryTimeMs <= m_currentTimeMs) {
            // Capture size before move
            std::size_t dataSize = it->event.data.size();
            m_eventQueue.push(std::move(it->event));
            m_totalBytesReceived += dataSize;
            m_packetsReceived++;
            it = m_pendingPackets.erase(it);
        } else {
            break; // Remaining packets are not ready yet
        }
    }
}

bool MockSocket::checkBandwidth(std::size_t bytes) {
    if (m_conditions.bandwidthBytesPerSec == 0) {
        return true; // No bandwidth limit
    }

    // Use 1-second windows for bandwidth tracking
    const std::uint64_t windowMs = 1000;

    if (m_currentTimeMs - m_bandwidthWindowStart >= windowMs) {
        // New window
        m_bandwidthWindowStart = m_currentTimeMs;
        m_bytesThisWindow = 0;
    }

    if (m_bytesThisWindow + bytes > m_conditions.bandwidthBytesPerSec) {
        return false; // Would exceed bandwidth limit
    }

    m_bytesThisWindow += bytes;
    return true;
}

} // namespace sims3000
