/**
 * @file ENetTransport.cpp
 * @brief Implementation of ENet-based network transport.
 */

#include "sims3000/net/ENetTransport.h"
#include <enet/enet.h>
#include <cstring>

namespace sims3000 {

// Static member for ref-counted initialization
int ENetTransport::s_initCount = 0;

ENetTransport::ENetTransport() {
    // Initialize ENet on first instance
    if (s_initCount == 0) {
        if (enet_initialize() != 0) {
            // ENet initialization failed - host will remain null
            return;
        }
    }
    ++s_initCount;
}

ENetTransport::~ENetTransport() {
    disconnectAll();

    if (m_host) {
        enet_host_destroy(m_host);
        m_host = nullptr;
    }

    // Only decrement if we contributed to the count
    // A moved-from object has m_ownsInitRef set to false
    if (m_ownsInitRef) {
        --s_initCount;
        if (s_initCount == 0) {
            enet_deinitialize();
        }
    }
}

ENetTransport::ENetTransport(ENetTransport&& other) noexcept
    : m_host(other.m_host)
    , m_peers(std::move(other.m_peers))
    , m_peerIds(std::move(other.m_peerIds))
    , m_nextPeerId(other.m_nextPeerId)
    , m_ownsInitRef(other.m_ownsInitRef) {
    other.m_host = nullptr;
    other.m_nextPeerId = 1;
    other.m_ownsInitRef = false;  // Transfer ownership of init ref
}

ENetTransport& ENetTransport::operator=(ENetTransport&& other) noexcept {
    if (this != &other) {
        disconnectAll();
        if (m_host) {
            enet_host_destroy(m_host);
        }

        // If we owned an init ref and other doesn't, release ours
        if (m_ownsInitRef && !other.m_ownsInitRef) {
            --s_initCount;
            if (s_initCount == 0) {
                enet_deinitialize();
            }
        }
        // If other owned an init ref and we didn't, we now own it
        // (handled by transferring m_ownsInitRef below)

        m_host = other.m_host;
        m_peers = std::move(other.m_peers);
        m_peerIds = std::move(other.m_peerIds);
        m_nextPeerId = other.m_nextPeerId;
        m_ownsInitRef = other.m_ownsInitRef;

        other.m_host = nullptr;
        other.m_nextPeerId = 1;
        other.m_ownsInitRef = false;
    }
    return *this;
}

bool ENetTransport::startServer(std::uint16_t port, std::uint32_t maxClients) {
    if (m_host) {
        return false;  // Already running
    }

    ENetAddress address;
    address.host = ENET_HOST_ANY;
    address.port = port;

    // Create host with 2 channels (reliable and unreliable)
    m_host = enet_host_create(&address, maxClients,
                              static_cast<size_t>(ChannelID::COUNT),
                              0, 0);  // No bandwidth limits

    return m_host != nullptr;
}

PeerID ENetTransport::connect(const std::string& address, std::uint16_t port) {
    // Create client host if not already running
    if (!m_host) {
        m_host = enet_host_create(nullptr, 1,
                                  static_cast<size_t>(ChannelID::COUNT),
                                  0, 0);
        if (!m_host) {
            return INVALID_PEER_ID;
        }
    }

    ENetAddress enetAddr;
    enet_address_set_host(&enetAddr, address.c_str());
    enetAddr.port = port;

    // Initiate connection with 2 channels
    ENetPeer* peer = enet_host_connect(m_host, &enetAddr,
                                        static_cast<size_t>(ChannelID::COUNT),
                                        0);  // No data
    if (!peer) {
        return INVALID_PEER_ID;
    }

    return registerPeer(peer);
}

void ENetTransport::disconnect(PeerID peer) {
    ENetPeer* enetPeer = getPeer(peer);
    if (!enetPeer) {
        return;
    }

    enet_peer_disconnect(enetPeer, 0);
    unregisterPeer(peer);
}

void ENetTransport::disconnectAll() {
    if (!m_host) {
        return;
    }

    // Copy peer IDs since we'll modify the map
    std::vector<PeerID> peerIds;
    peerIds.reserve(m_peers.size());
    for (const auto& pair : m_peers) {
        peerIds.push_back(pair.first);
    }

    for (PeerID id : peerIds) {
        disconnect(id);
    }

    // Flush pending disconnects
    enet_host_flush(m_host);
}

bool ENetTransport::isRunning() const {
    return m_host != nullptr;
}

bool ENetTransport::send(PeerID peer, const void* data, std::size_t size,
                         ChannelID channel) {
    ENetPeer* enetPeer = getPeer(peer);
    if (!enetPeer) {
        return false;
    }

    // Create packet with appropriate flags
    enet_uint32 flags = (channel == ChannelID::Reliable)
                            ? ENET_PACKET_FLAG_RELIABLE
                            : ENET_PACKET_FLAG_UNSEQUENCED;

    ENetPacket* packet = enet_packet_create(data, size, flags);
    if (!packet) {
        return false;
    }

    int result = enet_peer_send(enetPeer, static_cast<enet_uint8>(channel),
                                packet);
    return result == 0;
}

void ENetTransport::broadcast(const void* data, std::size_t size,
                              ChannelID channel) {
    if (!m_host) {
        return;
    }

    enet_uint32 flags = (channel == ChannelID::Reliable)
                            ? ENET_PACKET_FLAG_RELIABLE
                            : ENET_PACKET_FLAG_UNSEQUENCED;

    ENetPacket* packet = enet_packet_create(data, size, flags);
    if (!packet) {
        return;
    }

    enet_host_broadcast(m_host, static_cast<enet_uint8>(channel), packet);
}

NetworkEvent ENetTransport::poll(std::uint32_t timeoutMs) {
    NetworkEvent event;
    event.type = NetworkEventType::None;

    if (!m_host) {
        return event;
    }

    ENetEvent enetEvent;
    int result = enet_host_service(m_host, &enetEvent, timeoutMs);

    if (result <= 0) {
        return event;  // No event or error
    }

    switch (enetEvent.type) {
        case ENET_EVENT_TYPE_CONNECT: {
            event.type = NetworkEventType::Connect;
            // Register new peer if not already registered (server-side)
            PeerID id = getPeerID(enetEvent.peer);
            if (id == INVALID_PEER_ID) {
                id = registerPeer(enetEvent.peer);
            }
            event.peer = id;
            break;
        }

        case ENET_EVENT_TYPE_DISCONNECT: {
            event.type = NetworkEventType::Disconnect;
            event.peer = getPeerID(enetEvent.peer);
            unregisterPeer(event.peer);
            break;
        }

        case ENET_EVENT_TYPE_RECEIVE: {
            event.type = NetworkEventType::Receive;
            event.peer = getPeerID(enetEvent.peer);
            event.channel = static_cast<ChannelID>(enetEvent.channelID);
            // Copy packet data
            event.data.resize(enetEvent.packet->dataLength);
            std::memcpy(event.data.data(), enetEvent.packet->data,
                        enetEvent.packet->dataLength);
            // Clean up packet
            enet_packet_destroy(enetEvent.packet);
            break;
        }

        default:
            // Note: ENET_EVENT_TYPE_DISCONNECT handles both normal disconnect and timeout
            break;
    }

    return event;
}

void ENetTransport::flush() {
    if (m_host) {
        enet_host_flush(m_host);
    }
}

std::uint32_t ENetTransport::getPeerCount() const {
    return static_cast<std::uint32_t>(m_peers.size());
}

std::optional<NetworkStats> ENetTransport::getStats(PeerID peer) const {
    ENetPeer* enetPeer = getPeer(peer);
    if (!enetPeer) {
        return std::nullopt;
    }

    NetworkStats stats;
    stats.packetsSent = enetPeer->packetsSent;
    stats.packetsReceived = enetPeer->packetsSent;  // Estimate - ENet doesn't track received per-peer easily
    stats.bytesSent = static_cast<std::uint32_t>(enetPeer->packetsSent * 64);  // Estimate
    stats.bytesReceived = static_cast<std::uint32_t>(enetPeer->packetsSent * 64);  // Estimate
    stats.roundTripTimeMs = enetPeer->roundTripTime;
    stats.packetLoss = enetPeer->packetLoss / 65536 * 100;  // ENet uses fixed-point

    return stats;
}

bool ENetTransport::isConnected(PeerID peer) const {
    ENetPeer* enetPeer = getPeer(peer);
    if (!enetPeer) {
        return false;
    }
    return enetPeer->state == ENET_PEER_STATE_CONNECTED;
}

PeerID ENetTransport::registerPeer(_ENetPeer* peer) {
    PeerID id = m_nextPeerId++;
    m_peers[id] = peer;
    m_peerIds[peer] = id;
    return id;
}

void ENetTransport::unregisterPeer(PeerID id) {
    auto it = m_peers.find(id);
    if (it != m_peers.end()) {
        m_peerIds.erase(it->second);
        m_peers.erase(it);
    }
}

_ENetPeer* ENetTransport::getPeer(PeerID id) const {
    auto it = m_peers.find(id);
    return (it != m_peers.end()) ? it->second : nullptr;
}

PeerID ENetTransport::getPeerID(_ENetPeer* peer) const {
    auto it = m_peerIds.find(peer);
    return (it != m_peerIds.end()) ? it->second : INVALID_PEER_ID;
}

} // namespace sims3000
