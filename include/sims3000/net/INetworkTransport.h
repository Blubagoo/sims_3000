/**
 * @file INetworkTransport.h
 * @brief Abstract network transport interface for testable networking.
 *
 * Provides a thin abstraction over network transport (ENet, mock, etc.)
 * to enable dependency injection and testing without real network connections.
 *
 * Ownership: NetworkManager owns INetworkTransport instances.
 * Cleanup: Derived classes must disconnect all peers and release resources
 *          in destructor.
 */

#ifndef SIMS3000_NET_INETWORKTRANSPORT_H
#define SIMS3000_NET_INETWORKTRANSPORT_H

#include <cstdint>
#include <vector>
#include <string>
#include <optional>
#include <functional>

namespace sims3000 {

/// Unique identifier for a connected peer
using PeerID = std::uint32_t;

/// Invalid peer ID constant
constexpr PeerID INVALID_PEER_ID = 0;

/**
 * @enum ChannelID
 * @brief Network channel identifiers.
 *
 * ENet supports multiple channels with different reliability guarantees.
 * Channel 0 is reliable/ordered for game actions.
 * Channel 1 is unreliable for optional data (e.g., cursor sync).
 */
enum class ChannelID : std::uint8_t {
    Reliable = 0,    ///< Reliable, ordered delivery for game actions
    Unreliable = 1,  ///< Unreliable for optional/frequent updates
    COUNT = 2
};

/**
 * @enum NetworkEventType
 * @brief Types of network events received from transport.
 */
enum class NetworkEventType : std::uint8_t {
    None = 0,        ///< No event (poll returned empty)
    Connect,         ///< New peer connected
    Disconnect,      ///< Peer disconnected
    Receive,         ///< Data received from peer
    Timeout          ///< Connection timed out
};

/**
 * @struct NetworkEvent
 * @brief Network event data from transport polling.
 */
struct NetworkEvent {
    NetworkEventType type = NetworkEventType::None;
    PeerID peer = INVALID_PEER_ID;
    std::vector<std::uint8_t> data;  ///< Received data (only for Receive events)
    ChannelID channel = ChannelID::Reliable;
};

/**
 * @struct NetworkStats
 * @brief Network statistics for a peer connection.
 */
struct NetworkStats {
    std::uint32_t packetsSent = 0;
    std::uint32_t packetsReceived = 0;
    std::uint32_t bytesSent = 0;
    std::uint32_t bytesReceived = 0;
    std::uint32_t roundTripTimeMs = 0;
    std::uint32_t packetLoss = 0;  ///< Estimated packet loss (0-100)
};

/**
 * @interface INetworkTransport
 * @brief Abstract interface for network transport operations.
 *
 * Implementations:
 * - ENetTransport: Real network transport using ENet library
 * - MockTransport: In-memory transport for testing
 *
 * Thread safety: Not thread-safe. All calls must be from the same thread.
 */
class INetworkTransport {
public:
    virtual ~INetworkTransport() = default;

    // ========================================================================
    // Host Operations
    // ========================================================================

    /**
     * @brief Start as a server, listening for incoming connections.
     * @param port Port to listen on
     * @param maxClients Maximum number of simultaneous clients
     * @return true if server started successfully
     */
    virtual bool startServer(std::uint16_t port, std::uint32_t maxClients) = 0;

    /**
     * @brief Connect to a server as a client.
     * @param address Server address (IP or hostname)
     * @param port Server port
     * @return PeerID of the server connection, or INVALID_PEER_ID on failure
     */
    virtual PeerID connect(const std::string& address, std::uint16_t port) = 0;

    /**
     * @brief Disconnect from a specific peer.
     * @param peer Peer to disconnect
     */
    virtual void disconnect(PeerID peer) = 0;

    /**
     * @brief Disconnect all peers and stop the host.
     */
    virtual void disconnectAll() = 0;

    /**
     * @brief Check if the transport is running (server started or client connected).
     * @return true if active
     */
    virtual bool isRunning() const = 0;

    // ========================================================================
    // Data Transfer
    // ========================================================================

    /**
     * @brief Send data to a specific peer.
     * @param peer Destination peer
     * @param data Data to send
     * @param size Size of data in bytes
     * @param channel Channel to send on (Reliable or Unreliable)
     * @return true if data was queued for sending
     */
    virtual bool send(PeerID peer, const void* data, std::size_t size,
                      ChannelID channel = ChannelID::Reliable) = 0;

    /**
     * @brief Broadcast data to all connected peers.
     * @param data Data to send
     * @param size Size of data in bytes
     * @param channel Channel to send on
     */
    virtual void broadcast(const void* data, std::size_t size,
                           ChannelID channel = ChannelID::Reliable) = 0;

    /**
     * @brief Poll for network events.
     *
     * Should be called regularly (e.g., every frame or tick).
     * Non-blocking: returns immediately if no events.
     *
     * @param timeoutMs Maximum time to wait for events (0 = non-blocking)
     * @return Network event, or event with type None if nothing happened
     */
    virtual NetworkEvent poll(std::uint32_t timeoutMs = 0) = 0;

    /**
     * @brief Flush all queued outgoing packets.
     *
     * Called after sending to ensure packets are transmitted.
     */
    virtual void flush() = 0;

    // ========================================================================
    // Status and Statistics
    // ========================================================================

    /**
     * @brief Get the number of connected peers.
     * @return Number of active connections
     */
    virtual std::uint32_t getPeerCount() const = 0;

    /**
     * @brief Get statistics for a specific peer.
     * @param peer Peer to query
     * @return Statistics, or nullopt if peer not found
     */
    virtual std::optional<NetworkStats> getStats(PeerID peer) const = 0;

    /**
     * @brief Check if a peer is currently connected.
     * @param peer Peer to check
     * @return true if peer is connected
     */
    virtual bool isConnected(PeerID peer) const = 0;
};

} // namespace sims3000

#endif // SIMS3000_NET_INETWORKTRANSPORT_H
