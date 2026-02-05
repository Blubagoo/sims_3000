#include "client.h"
#include "message_header.h"
#include "network_buffer.h"
#include <SDL3/SDL.h>
#include <cstdio>

Client::Client(int id, const std::string& host, uint16_t port,
               float connect_delay_s, uint32_t packet_loss_percent,
               std::atomic<bool>& running)
    : id_(id)
    , host_(host)
    , port_(port)
    , connect_delay_s_(connect_delay_s)
    , running_(running)
    , store_(ENTITY_COUNT)
    , applier_(store_)
    , loss_sim_(packet_loss_percent, 54321 + id)
{
}

Client::~Client() {
    if (peer_) {
        enet_peer_disconnect(peer_, 0);
    }
    if (enet_host_) {
        enet_host_flush(enet_host_);
        enet_host_destroy(enet_host_);
    }
}

void Client::run() {
    // Wait for connect delay
    if (connect_delay_s_ > 0.0f) {
        std::printf("[Client %d] Waiting %.1fs before connecting...\n", id_, connect_delay_s_);
        uint64_t delay_ms = static_cast<uint64_t>(connect_delay_s_ * 1000.0f);
        uint64_t waited = 0;
        while (running_.load() && waited < delay_ms) {
            SDL_Delay(100);
            waited += 100;
        }
        if (!running_.load()) return;
    }

    enet_host_ = enet_host_create(nullptr, 1, NUM_CHANNELS, 0, 0);
    if (!enet_host_) {
        std::fprintf(stderr, "[Client %d] Failed to create ENet host\n", id_);
        return;
    }

    ENetAddress address;
    enet_address_set_host(&address, host_.c_str());
    address.port = port_;

    peer_ = enet_host_connect(enet_host_, &address, NUM_CHANNELS, 0);
    if (!peer_) {
        std::fprintf(stderr, "[Client %d] Failed to initiate connection\n", id_);
        return;
    }

    connect_start_counter_ = SDL_GetPerformanceCounter();
    std::printf("[Client %d] Connecting to %s:%u (loss=%u%%)\n",
                id_, host_.c_str(), port_, loss_sim_.loss_percent());

    // Wait for connection
    ENetEvent event;
    if (enet_host_service(enet_host_, &event, 5000) > 0 &&
        event.type == ENET_EVENT_TYPE_CONNECT) {
        std::printf("[Client %d] Connected\n", id_);
        metrics_.connected = true;
    } else {
        std::fprintf(stderr, "[Client %d] Connection failed\n", id_);
        enet_peer_reset(peer_);
        peer_ = nullptr;
        return;
    }

    // Main receive loop
    while (running_.load()) {
        process_events();
        SDL_Delay(1);
    }
}

void Client::process_events() {
    ENetEvent event;
    while (enet_host_service(enet_host_, &event, 0) > 0) {
        switch (event.type) {
            case ENET_EVENT_TYPE_RECEIVE: {
                if (event.channelID == CHANNEL_FULL_SNAPSHOT) {
                    handle_full_snapshot(event.packet);
                } else if (event.channelID == CHANNEL_DELTA) {
                    handle_delta_snapshot(event.packet);
                }
                enet_packet_destroy(event.packet);
                break;
            }
            case ENET_EVENT_TYPE_DISCONNECT: {
                std::printf("[Client %d] Disconnected\n", id_);
                metrics_.connected = false;
                peer_ = nullptr;
                return;
            }
            default:
                break;
        }
    }
}

void Client::handle_full_snapshot(ENetPacket* packet) {
    metrics_.bytes_received += packet->dataLength;

    uint64_t start = SDL_GetPerformanceCounter();
    auto result = applier_.apply_full(packet->data, packet->dataLength);
    uint64_t end = SDL_GetPerformanceCounter();

    double elapsed_ms = static_cast<double>(end - start) * 1000.0 / SDL_GetPerformanceFrequency();

    if (result.success) {
        metrics_.full_snapshots_received++;
        metrics_.last_tick = result.tick;
        metrics_.snapshot_apply_time_ms += elapsed_ms;
        if (elapsed_ms > metrics_.max_apply_time_ms) {
            metrics_.max_apply_time_ms = elapsed_ms;
        }

        if (!metrics_.first_snapshot_received) {
            metrics_.first_snapshot_received = true;
            double connect_elapsed = static_cast<double>(end - connect_start_counter_)
                                     / SDL_GetPerformanceFrequency();
            metrics_.connect_time_s = connect_elapsed;
            std::printf("[Client %d] First full snapshot applied (tick=%u, %.1fms, late-join=%.3fs)\n",
                        id_, result.tick, elapsed_ms, connect_elapsed);
        }

        send_ack(result.tick);
    }
}

void Client::handle_delta_snapshot(ENetPacket* packet) {
    // Simulate packet loss on delta channel
    if (loss_sim_.should_drop()) {
        metrics_.delta_snapshots_dropped++;
        return;
    }

    if (!metrics_.first_snapshot_received) {
        // Can't apply delta before having a base state
        return;
    }

    metrics_.bytes_received += packet->dataLength;

    uint64_t start = SDL_GetPerformanceCounter();
    auto result = applier_.apply_delta(packet->data, packet->dataLength);
    uint64_t end = SDL_GetPerformanceCounter();

    double elapsed_ms = static_cast<double>(end - start) * 1000.0 / SDL_GetPerformanceFrequency();

    if (result.success) {
        metrics_.delta_snapshots_received++;
        metrics_.last_tick = result.tick;
        metrics_.snapshot_apply_time_ms += elapsed_ms;
        if (elapsed_ms > metrics_.max_apply_time_ms) {
            metrics_.max_apply_time_ms = elapsed_ms;
        }

        if (!result.checksum_match) {
            metrics_.desync_count++;
            std::fprintf(stderr, "[Client %d] DESYNC at tick %u - requesting resync\n", id_, result.tick);
            send_resync_request();
        } else {
            send_ack(result.tick);
        }
    }
}

void Client::send_ack(uint32_t tick) {
    if (!peer_) return;

    NetworkBuffer buf;
    MessageHeader header;
    header.type = MessageType::SnapshotAck;
    header.sequence = tick;
    header.payload_length = 0;
    header.serialize(buf);

    ENetPacket* packet = enet_packet_create(
        buf.data(), buf.size(), ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(peer_, CHANNEL_ACK, packet);
}

void Client::send_resync_request() {
    if (!peer_) return;

    NetworkBuffer buf;
    MessageHeader header;
    header.type = MessageType::ResyncRequest;
    header.sequence = 0;
    header.payload_length = 0;
    header.serialize(buf);

    ENetPacket* packet = enet_packet_create(
        buf.data(), buf.size(), ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(peer_, CHANNEL_ACK, packet);
}
