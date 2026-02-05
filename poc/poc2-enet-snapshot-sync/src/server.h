#pragma once

#include "entity_store.h"
#include "simulation.h"
#include "snapshot_generator.h"
#include "message_header.h"
#include <enet/enet.h>
#include <atomic>
#include <cstdint>
#include <vector>
#include <mutex>

// Per-client state tracked by server
struct ClientState {
    ENetPeer* peer = nullptr;
    uint32_t last_acked_tick = 0;
    bool needs_full_snapshot = true;
    uint64_t bytes_sent = 0;
    // Accumulated dirty masks since last_acked_tick for packet loss recovery.
    // OR'd with each tick's dirty state. Cleared when client acks.
    std::vector<uint8_t> pending_dirty;
};

class Server {
public:
    Server(uint16_t port, std::atomic<bool>& running);
    ~Server();

    // Run the server loop (blocking, runs until running == false)
    void run();

    // Get total bytes sent across all clients
    uint64_t total_bytes_sent() const;

    // Get per-client bytes sent
    std::vector<uint64_t> per_client_bytes_sent() const;

    uint32_t current_tick() const { return sim_.current_tick(); }

private:
    void process_events();
    void tick_and_send();
    void send_full_snapshot(ClientState& cs);
    void send_delta_snapshot(ClientState& cs);
    void handle_ack(ENetPeer* peer, ENetPacket* packet);

    std::atomic<bool>& running_;
    ENetHost* host_ = nullptr;
    EntityStore store_;
    Simulation sim_;
    SnapshotGenerator gen_;
    std::vector<ClientState> clients_;
    mutable std::mutex clients_mutex_;
    uint16_t port_;
};
