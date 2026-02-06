/**
 * @file NetworkThread.cpp
 * @brief Implementation of dedicated network I/O thread.
 */

#include "sims3000/net/NetworkThread.h"
#include <chrono>

namespace sims3000 {

NetworkThread::NetworkThread(
    std::unique_ptr<INetworkTransport> transport,
    std::size_t inboundCapacity,
    std::size_t outboundCapacity
)
    : m_transport(std::move(transport))
    , m_inboundQueue(inboundCapacity)
    , m_outboundQueue(outboundCapacity)
    , m_commandQueue(256)  // Commands are rare, small queue sufficient
{
    // Transport must be provided
    if (!m_transport) {
        throw std::invalid_argument("NetworkThread requires a valid transport");
    }
}

NetworkThread::~NetworkThread() {
    // Ensure clean shutdown
    stop();
    join();
}

bool NetworkThread::startServer(std::uint16_t port, std::uint32_t maxClients) {
    NetworkThreadCommandData cmd;
    cmd.command = NetworkThreadCommand::StartServer;
    cmd.port = port;
    cmd.maxClients = maxClients;
    return m_commandQueue.try_enqueue(std::move(cmd));
}

bool NetworkThread::connect(const std::string& address, std::uint16_t port) {
    NetworkThreadCommandData cmd;
    cmd.command = NetworkThreadCommand::Connect;
    cmd.address = address;
    cmd.port = port;
    return m_commandQueue.try_enqueue(std::move(cmd));
}

bool NetworkThread::disconnect(PeerID peer) {
    NetworkThreadCommandData cmd;
    cmd.command = NetworkThreadCommand::Disconnect;
    cmd.targetPeer = peer;
    return m_commandQueue.try_enqueue(std::move(cmd));
}

bool NetworkThread::disconnectAll() {
    NetworkThreadCommandData cmd;
    cmd.command = NetworkThreadCommand::DisconnectAll;
    return m_commandQueue.try_enqueue(std::move(cmd));
}

void NetworkThread::start() {
    // Prevent double-start
    bool expected = false;
    if (!m_running.compare_exchange_strong(expected, true)) {
        return;  // Already running
    }

    m_stopRequested.store(false);

    // Start the network thread
    m_thread = std::thread(&NetworkThread::threadLoop, this);
}

void NetworkThread::stop() {
    m_stopRequested.store(true);
}

void NetworkThread::join() {
    if (m_thread.joinable()) {
        m_thread.join();
    }
    m_running.store(false);
}

bool NetworkThread::isRunning() const {
    return m_running.load();
}

bool NetworkThread::enqueueOutbound(OutboundNetworkMessage&& message) {
    return m_outboundQueue.try_enqueue(std::move(message));
}

bool NetworkThread::pollInbound(InboundNetworkEvent& event) {
    return m_inboundQueue.try_dequeue(event);
}

std::size_t NetworkThread::getInboundCount() const {
    return m_inboundQueue.size_approx();
}

std::size_t NetworkThread::getOutboundCount() const {
    return m_outboundQueue.size_approx();
}

void NetworkThread::threadLoop() {
    while (!m_stopRequested.load()) {
        // Process commands first (start server, connect, disconnect)
        processCommands();

        // Send outbound messages
        processOutbound();

        // Poll transport for incoming events
        pollTransport();
    }

    // Final flush of outbound messages before exit
    processOutbound();

    // Disconnect all peers gracefully
    if (m_transport && m_transport->isRunning()) {
        m_transport->disconnectAll();
        m_transport->flush();
    }
}

void NetworkThread::processCommands() {
    NetworkThreadCommandData cmd;
    while (m_commandQueue.try_dequeue(cmd)) {
        switch (cmd.command) {
            case NetworkThreadCommand::StartServer: {
                bool success = m_transport->startServer(cmd.port, cmd.maxClients);
                // We could send a result event here, but for simplicity
                // the main thread can check isRunning() on the transport
                // or we report via a special event type (future enhancement)
                (void)success;
                break;
            }

            case NetworkThreadCommand::Connect: {
                PeerID peer = m_transport->connect(cmd.address, cmd.port);
                // Connection result will come via poll() events
                (void)peer;
                break;
            }

            case NetworkThreadCommand::Disconnect: {
                m_transport->disconnect(cmd.targetPeer);
                break;
            }

            case NetworkThreadCommand::DisconnectAll: {
                m_transport->disconnectAll();
                break;
            }

            case NetworkThreadCommand::None:
            default:
                break;
        }
    }
}

void NetworkThread::processOutbound() {
    OutboundNetworkMessage msg;
    while (m_outboundQueue.try_dequeue(msg)) {
        if (msg.broadcast) {
            m_transport->broadcast(msg.data.data(), msg.data.size(), msg.channel);
        } else {
            m_transport->send(msg.peer, msg.data.data(), msg.data.size(), msg.channel);
        }

        // Update statistics
        m_messagesSent.fetch_add(1, std::memory_order_relaxed);
        m_bytesSent.fetch_add(msg.data.size(), std::memory_order_relaxed);
    }

    // Flush to ensure packets are sent
    m_transport->flush();
}

void NetworkThread::pollTransport() {
    // Poll with 1ms timeout as per ticket requirement
    NetworkEvent event = m_transport->poll(POLL_TIMEOUT_MS);

    if (event.type == NetworkEventType::None) {
        return;  // No event
    }

    // Convert transport event to inbound event
    InboundNetworkEvent inbound;
    inbound.peer = event.peer;
    inbound.channel = event.channel;

    switch (event.type) {
        case NetworkEventType::Connect:
            inbound.type = NetworkThreadEventType::Connect;
            break;

        case NetworkEventType::Disconnect:
            inbound.type = NetworkThreadEventType::Disconnect;
            break;

        case NetworkEventType::Receive:
            inbound.type = NetworkThreadEventType::Message;
            inbound.data = std::move(event.data);
            m_messagesReceived.fetch_add(1, std::memory_order_relaxed);
            m_bytesReceived.fetch_add(inbound.data.size(), std::memory_order_relaxed);
            break;

        case NetworkEventType::Timeout:
            inbound.type = NetworkThreadEventType::Timeout;
            break;

        default:
            return;  // Unknown event type
    }

    // Enqueue for main thread (drop if queue is full - shouldn't happen
    // under normal load with properly sized queues)
    m_inboundQueue.try_enqueue(std::move(inbound));
}

} // namespace sims3000
