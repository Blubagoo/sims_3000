#include "server.h"
#include "message_header.h"
#include "network_buffer.h"
#include <SDL3/SDL.h>
#include <cstdio>
#include <algorithm>
#include <cstring>

Server::Server(uint16_t port, std::atomic<bool>& running)
    : running_(running)
    , store_(ENTITY_COUNT)
    , sim_(store_, 42)
    , gen_(store_)
    , port_(port)
{
    store_.initialize_deterministic(42);
}

Server::~Server() {
    if (host_) {
        enet_host_destroy(host_);
    }
}

void Server::run() {
    ENetAddress address;
    address.host = ENET_HOST_ANY;
    address.port = port_;

    host_ = enet_host_create(&address, 8, NUM_CHANNELS, 0, 0);
    if (!host_) {
        std::fprintf(stderr, "[Server] Failed to create ENet host on port %u\n", port_);
        return;
    }

    std::printf("[Server] Listening on port %u\n", port_);

    uint64_t freq = SDL_GetPerformanceFrequency();
    uint64_t tick_interval_ticks = static_cast<uint64_t>(freq / TICK_RATE);
    uint64_t next_tick = SDL_GetPerformanceCounter() + tick_interval_ticks;

    while (running_.load()) {
        process_events();

        uint64_t now = SDL_GetPerformanceCounter();
        if (now >= next_tick) {
            tick_and_send();
            next_tick += tick_interval_ticks;
            // Prevent spiral of death
            if (now > next_tick + tick_interval_ticks * 3) {
                next_tick = now + tick_interval_ticks;
            }
        }

        // Sleep briefly to avoid busy-waiting
        SDL_Delay(1);
    }

    // Disconnect all clients
    std::lock_guard<std::mutex> lock(clients_mutex_);
    for (auto& cs : clients_) {
        if (cs.peer) {
            enet_peer_disconnect(cs.peer, 0);
        }
    }
    enet_host_flush(host_);
}

void Server::process_events() {
    ENetEvent event;
    while (enet_host_service(host_, &event, 0) > 0) {
        switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT: {
                std::printf("[Server] Client connected from %x:%u\n",
                            event.peer->address.host, event.peer->address.port);
                std::lock_guard<std::mutex> lock(clients_mutex_);
                ClientState cs;
                cs.peer = event.peer;
                cs.needs_full_snapshot = true;
                cs.last_acked_tick = 0;
                cs.pending_dirty.resize(ENTITY_COUNT, 0);
                event.peer->data = reinterpret_cast<void*>(static_cast<uintptr_t>(clients_.size()));
                clients_.push_back(std::move(cs));
                break;
            }
            case ENET_EVENT_TYPE_RECEIVE: {
                if (event.channelID == CHANNEL_ACK) {
                    handle_ack(event.peer, event.packet);
                }
                enet_packet_destroy(event.packet);
                break;
            }
            case ENET_EVENT_TYPE_DISCONNECT: {
                std::printf("[Server] Client disconnected\n");
                std::lock_guard<std::mutex> lock(clients_mutex_);
                auto idx = reinterpret_cast<uintptr_t>(event.peer->data);
                if (idx < clients_.size()) {
                    clients_[idx].peer = nullptr;
                }
                break;
            }
            default:
                break;
        }
    }
}

void Server::tick_and_send() {
    sim_.tick();
    uint32_t tick = sim_.current_tick();

    std::lock_guard<std::mutex> lock(clients_mutex_);

    // Accumulate this tick's dirty state into each connected client's pending_dirty
    for (auto& cs : clients_) {
        if (!cs.peer || cs.needs_full_snapshot) continue;
        for (uint32_t i = 0; i < store_.count(); ++i) {
            cs.pending_dirty[i] |= store_.dirty(i);
        }
    }

    for (auto& cs : clients_) {
        if (!cs.peer) continue;

        if (cs.needs_full_snapshot) {
            send_full_snapshot(cs);
            cs.needs_full_snapshot = false;
            cs.last_acked_tick = tick;
            std::memset(cs.pending_dirty.data(), 0, cs.pending_dirty.size());
        } else {
            send_delta_snapshot(cs);
        }
    }
}

void Server::send_full_snapshot(ClientState& cs) {
    auto data = gen_.generate_full(sim_.current_tick());
    if (data.empty()) return;

    ENetPacket* packet = enet_packet_create(
        data.data(), data.size(), ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(cs.peer, CHANNEL_FULL_SNAPSHOT, packet);
    cs.bytes_sent += data.size();
}

void Server::send_delta_snapshot(ClientState& cs) {
    uint64_t checksum = store_.compute_checksum();
    // Use per-client accumulated dirty mask for packet loss recovery
    auto data = gen_.generate_delta_from_mask(
        sim_.current_tick(), checksum,
        cs.pending_dirty.data(), store_.count());
    if (data.empty()) return;

    ENetPacket* packet = enet_packet_create(
        data.data(), data.size(), ENET_PACKET_FLAG_UNSEQUENCED);
    enet_peer_send(cs.peer, CHANNEL_DELTA, packet);
    cs.bytes_sent += data.size();

}

void Server::handle_ack(ENetPeer* peer, ENetPacket* packet) {
    if (packet->dataLength < MessageHeader::HEADER_SIZE) return;

    NetworkBuffer buf(packet->data, packet->dataLength);
    MessageHeader header;
    if (!header.deserialize(buf)) return;

    auto idx = reinterpret_cast<uintptr_t>(peer->data);
    std::lock_guard<std::mutex> lock(clients_mutex_);
    if (idx >= clients_.size()) return;

    if (header.type == MessageType::ResyncRequest) {
        // Client detected desync — schedule a full resync
        std::printf("[Server] Resync requested by client %zu\n", idx);
        clients_[idx].needs_full_snapshot = true;
    } else if (header.type == MessageType::SnapshotAck) {
        if (header.sequence > clients_[idx].last_acked_tick) {
            clients_[idx].last_acked_tick = header.sequence;
            // Client confirmed receipt — clear accumulated dirty state
            std::memset(clients_[idx].pending_dirty.data(), 0,
                        clients_[idx].pending_dirty.size());
        }
    }
}

uint64_t Server::total_bytes_sent() const {
    std::lock_guard<std::mutex> lock(clients_mutex_);
    uint64_t total = 0;
    for (const auto& cs : clients_) {
        total += cs.bytes_sent;
    }
    return total;
}

std::vector<uint64_t> Server::per_client_bytes_sent() const {
    std::lock_guard<std::mutex> lock(clients_mutex_);
    std::vector<uint64_t> result;
    for (const auto& cs : clients_) {
        result.push_back(cs.bytes_sent);
    }
    return result;
}
