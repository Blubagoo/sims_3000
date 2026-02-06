/**
 * @file NetworkThread.h
 * @brief Dedicated network I/O thread with lock-free message queues.
 *
 * NetworkThread provides a dedicated thread for ENet polling and message
 * handling, communicating with the main thread via lock-free SPSC queues.
 * This ensures the main thread never blocks on network operations.
 *
 * Architecture:
 * - Network thread polls ENet continuously (1ms timeout)
 * - Inbound queue: Network -> Main thread (received messages, events)
 * - Outbound queue: Main -> Network thread (messages to send)
 * - No shared mutable state beyond the queues
 *
 * Ownership: Application owns NetworkThread. NetworkThread owns INetworkTransport.
 * Cleanup: Destructor signals stop and joins thread. Transport cleaned up after.
 *
 * Thread safety:
 * - start(), stop(), join() called from main thread only
 * - enqueueOutbound() called from main thread only
 * - pollInbound() called from main thread only
 * - Network thread runs independently, never touches ECS registry
 */

#ifndef SIMS3000_NET_NETWORKTHREAD_H
#define SIMS3000_NET_NETWORKTHREAD_H

#include "sims3000/net/INetworkTransport.h"
#include "sims3000/core/types.h"
#include <readerwriterqueue.h>
#include <memory>
#include <thread>
#include <atomic>
#include <vector>
#include <string>
#include <optional>
#include <cstdint>

namespace sims3000 {

/**
 * @enum NetworkThreadEventType
 * @brief Types of events from network thread to main thread.
 */
enum class NetworkThreadEventType : std::uint8_t {
    None = 0,
    Connect,       ///< Peer connected
    Disconnect,    ///< Peer disconnected
    Message,       ///< Data received from peer
    Timeout,       ///< Connection timed out
    Error          ///< Network error occurred
};

/**
 * @struct InboundNetworkEvent
 * @brief Event from network thread to main thread.
 *
 * Represents a network event that the main thread should process.
 * Data is moved (not copied) for efficiency.
 */
struct InboundNetworkEvent {
    NetworkThreadEventType type = NetworkThreadEventType::None;
    PeerID peer = INVALID_PEER_ID;
    std::vector<std::uint8_t> data;  ///< Received data (only for Message events)
    ChannelID channel = ChannelID::Reliable;
};

/**
 * @struct OutboundNetworkMessage
 * @brief Message from main thread to network thread.
 *
 * Represents data that should be sent over the network.
 */
struct OutboundNetworkMessage {
    PeerID peer = INVALID_PEER_ID;               ///< Target peer (or 0 for broadcast)
    std::vector<std::uint8_t> data;              ///< Data to send
    ChannelID channel = ChannelID::Reliable;     ///< Channel to send on
    bool broadcast = false;                       ///< If true, send to all peers
};

/**
 * @enum NetworkThreadCommand
 * @brief Commands from main thread to network thread.
 */
enum class NetworkThreadCommand : std::uint8_t {
    None = 0,
    StartServer,   ///< Start as server
    Connect,       ///< Connect to server
    Disconnect,    ///< Disconnect specific peer
    DisconnectAll  ///< Disconnect all peers
};

/**
 * @struct NetworkThreadCommandData
 * @brief Command data from main thread to network thread.
 */
struct NetworkThreadCommandData {
    NetworkThreadCommand command = NetworkThreadCommand::None;
    std::string address;               ///< Server address (for Connect)
    std::uint16_t port = 0;            ///< Port number
    std::uint32_t maxClients = 0;      ///< Max clients (for StartServer)
    PeerID targetPeer = INVALID_PEER_ID;  ///< Target peer (for Disconnect)
};

/**
 * @class NetworkThread
 * @brief Dedicated network I/O thread with lock-free message queues.
 *
 * Example usage:
 * @code
 *   auto transport = std::make_unique<ENetTransport>();
 *   NetworkThread netThread(std::move(transport));
 *
 *   // Start as server
 *   netThread.startServer(7777, 4);
 *   netThread.start();
 *
 *   // Main game loop
 *   while (running) {
 *       // Process incoming events
 *       InboundNetworkEvent event;
 *       while (netThread.pollInbound(event)) {
 *           handleEvent(event);
 *       }
 *
 *       // Queue outgoing messages
 *       OutboundNetworkMessage msg;
 *       msg.peer = serverPeer;
 *       msg.data = serializeData(data);
 *       netThread.enqueueOutbound(std::move(msg));
 *   }
 *
 *   // Clean shutdown
 *   netThread.stop();
 * @endcode
 */
class NetworkThread {
public:
    /// Default queue capacity for SPSC queues
    static constexpr std::size_t DEFAULT_QUEUE_CAPACITY = 4096;

    /**
     * @brief Construct a NetworkThread with the given transport.
     * @param transport Network transport implementation (ENetTransport or MockTransport)
     * @param inboundCapacity Capacity of inbound event queue
     * @param outboundCapacity Capacity of outbound message queue
     */
    explicit NetworkThread(
        std::unique_ptr<INetworkTransport> transport,
        std::size_t inboundCapacity = DEFAULT_QUEUE_CAPACITY,
        std::size_t outboundCapacity = DEFAULT_QUEUE_CAPACITY
    );

    /**
     * @brief Destructor. Stops thread if running and joins.
     */
    ~NetworkThread();

    // Non-copyable, non-movable (thread cannot be moved)
    NetworkThread(const NetworkThread&) = delete;
    NetworkThread& operator=(const NetworkThread&) = delete;
    NetworkThread(NetworkThread&&) = delete;
    NetworkThread& operator=(NetworkThread&&) = delete;

    // =========================================================================
    // Control Methods (Main Thread Only)
    // =========================================================================

    /**
     * @brief Queue a command to start as a server.
     * @param port Port to listen on
     * @param maxClients Maximum number of simultaneous clients
     * @return true if command was queued
     *
     * The server will start on the network thread. Check for success/failure
     * via inbound events.
     */
    bool startServer(std::uint16_t port, std::uint32_t maxClients);

    /**
     * @brief Queue a command to connect to a server.
     * @param address Server address (IP or hostname)
     * @param port Server port
     * @return true if command was queued
     *
     * Connection status will be reported via inbound events.
     */
    bool connect(const std::string& address, std::uint16_t port);

    /**
     * @brief Queue a command to disconnect a specific peer.
     * @param peer Peer to disconnect
     * @return true if command was queued
     */
    bool disconnect(PeerID peer);

    /**
     * @brief Queue a command to disconnect all peers.
     * @return true if command was queued
     */
    bool disconnectAll();

    /**
     * @brief Start the network thread.
     *
     * Thread begins polling the transport and processing messages.
     * Must be called after construction and before queuing messages.
     */
    void start();

    /**
     * @brief Signal the network thread to stop.
     *
     * Thread will finish processing current batch and exit.
     * This is non-blocking; call join() to wait for thread exit.
     */
    void stop();

    /**
     * @brief Wait for the network thread to exit.
     *
     * Blocks until thread has stopped. Should be called after stop().
     */
    void join();

    /**
     * @brief Check if the network thread is running.
     * @return true if thread is active
     */
    bool isRunning() const;

    // =========================================================================
    // Message Methods (Main Thread Only)
    // =========================================================================

    /**
     * @brief Enqueue an outbound message for the network thread to send.
     * @param message Message to send (moved for efficiency)
     * @return true if enqueued successfully, false if queue is full
     *
     * This never blocks. If the queue is full, returns false.
     */
    bool enqueueOutbound(OutboundNetworkMessage&& message);

    /**
     * @brief Poll for an inbound event from the network thread.
     * @param event Output parameter for the received event
     * @return true if an event was available, false if queue is empty
     *
     * This never blocks. Call repeatedly to drain all pending events.
     */
    bool pollInbound(InboundNetworkEvent& event);

    /**
     * @brief Get approximate number of pending inbound events.
     * @return Approximate count (may be stale)
     */
    std::size_t getInboundCount() const;

    /**
     * @brief Get approximate number of pending outbound messages.
     * @return Approximate count (may be stale)
     */
    std::size_t getOutboundCount() const;

    // =========================================================================
    // Statistics (Thread-Safe via Atomics)
    // =========================================================================

    /**
     * @brief Get total messages sent since start.
     * @return Message count
     */
    std::uint64_t getMessagesSent() const { return m_messagesSent.load(std::memory_order_relaxed); }

    /**
     * @brief Get total messages received since start.
     * @return Message count
     */
    std::uint64_t getMessagesReceived() const { return m_messagesReceived.load(std::memory_order_relaxed); }

    /**
     * @brief Get total bytes sent since start.
     * @return Byte count
     */
    std::uint64_t getBytesSent() const { return m_bytesSent.load(std::memory_order_relaxed); }

    /**
     * @brief Get total bytes received since start.
     * @return Byte count
     */
    std::uint64_t getBytesReceived() const { return m_bytesReceived.load(std::memory_order_relaxed); }

private:
    /**
     * @brief Main network thread loop.
     *
     * Continuously polls the transport and processes queued messages
     * until stop is signaled.
     */
    void threadLoop();

    /**
     * @brief Process pending commands from main thread.
     */
    void processCommands();

    /**
     * @brief Process pending outbound messages.
     */
    void processOutbound();

    /**
     * @brief Poll transport and enqueue inbound events.
     */
    void pollTransport();

    // Transport (owned, accessed only by network thread after start)
    std::unique_ptr<INetworkTransport> m_transport;

    // Thread control
    std::thread m_thread;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_stopRequested{false};

    // Lock-free SPSC queues
    // Inbound: Network thread produces, main thread consumes
    moodycamel::ReaderWriterQueue<InboundNetworkEvent> m_inboundQueue;
    // Outbound: Main thread produces, network thread consumes
    moodycamel::ReaderWriterQueue<OutboundNetworkMessage> m_outboundQueue;
    // Commands: Main thread produces, network thread consumes
    moodycamel::ReaderWriterQueue<NetworkThreadCommandData> m_commandQueue;

    // Poll timeout in milliseconds (1ms as per ticket requirement)
    static constexpr std::uint32_t POLL_TIMEOUT_MS = 1;

    // Statistics (atomic for thread-safe reads)
    std::atomic<std::uint64_t> m_messagesSent{0};
    std::atomic<std::uint64_t> m_messagesReceived{0};
    std::atomic<std::uint64_t> m_bytesSent{0};
    std::atomic<std::uint64_t> m_bytesReceived{0};
};

} // namespace sims3000

#endif // SIMS3000_NET_NETWORKTHREAD_H
