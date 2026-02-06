/**
 * @file ENetTransport.h
 * @brief ENet-based implementation of INetworkTransport.
 *
 * Wraps the ENet reliable UDP library to provide network transport.
 * ENet provides reliable, ordered delivery on channel 0 and unreliable
 * delivery on channel 1.
 *
 * Ownership: ENetTransport owns the ENet host and manages peer connections.
 * Cleanup: Destructor disconnects all peers and destroys the ENet host.
 *          Global ENet initialization/deinitialization is ref-counted.
 *
 * Thread safety: Not thread-safe. All calls must be from the same thread.
 */

#ifndef SIMS3000_NET_ENETTRANSPORT_H
#define SIMS3000_NET_ENETTRANSPORT_H

#include "sims3000/net/INetworkTransport.h"
#include <unordered_map>
#include <memory>

// Forward declare ENet types to avoid including enet.h in header
struct _ENetHost;
struct _ENetPeer;

namespace sims3000 {

/**
 * @class ENetTransport
 * @brief ENet-based network transport implementation.
 *
 * Uses ENet library for reliable UDP networking:
 * - Channel 0: Reliable, ordered (for game actions)
 * - Channel 1: Unreliable (for optional data like cursor position)
 *
 * Example usage (server):
 * @code
 *   ENetTransport server;
 *   if (server.startServer(7777, 4)) {
 *       while (running) {
 *           NetworkEvent event = server.poll(0);
 *           switch (event.type) {
 *               case NetworkEventType::Connect:
 *                   // New client connected
 *                   break;
 *               case NetworkEventType::Receive:
 *                   // Handle data from event.peer
 *                   break;
 *           }
 *           server.flush();
 *       }
 *   }
 * @endcode
 *
 * Example usage (client):
 * @code
 *   ENetTransport client;
 *   PeerID server = client.connect("127.0.0.1", 7777);
 *   if (server != INVALID_PEER_ID) {
 *       // Wait for Connect event via poll()
 *       // Then send/receive data
 *   }
 * @endcode
 */
class ENetTransport : public INetworkTransport {
public:
    /**
     * @brief Construct an ENetTransport.
     *
     * Initializes ENet library on first instance (ref-counted).
     */
    ENetTransport();

    /**
     * @brief Destructor.
     *
     * Disconnects all peers, destroys host, and deinitializes ENet
     * if this is the last instance.
     */
    ~ENetTransport() override;

    // Non-copyable
    ENetTransport(const ENetTransport&) = delete;
    ENetTransport& operator=(const ENetTransport&) = delete;

    // Movable
    ENetTransport(ENetTransport&& other) noexcept;
    ENetTransport& operator=(ENetTransport&& other) noexcept;

    // ========================================================================
    // INetworkTransport Implementation
    // ========================================================================

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

private:
    /// Assign a unique peer ID and track the peer
    PeerID registerPeer(_ENetPeer* peer);

    /// Remove a peer from tracking
    void unregisterPeer(PeerID id);

    /// Get ENet peer from our ID
    _ENetPeer* getPeer(PeerID id) const;

    /// Get our ID from an ENet peer pointer
    PeerID getPeerID(_ENetPeer* peer) const;

    _ENetHost* m_host = nullptr;
    std::unordered_map<PeerID, _ENetPeer*> m_peers;
    std::unordered_map<_ENetPeer*, PeerID> m_peerIds;
    PeerID m_nextPeerId = 1;  // 0 is invalid
    bool m_ownsInitRef = true;  // Tracks if this instance should decrement s_initCount

    /// Reference count for global ENet initialization
    static int s_initCount;
};

} // namespace sims3000

#endif // SIMS3000_NET_ENETTRANSPORT_H
