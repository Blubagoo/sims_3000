#pragma once

#include "entity_store.h"
#include "snapshot_applier.h"
#include "packet_loss_sim.h"
#include "message_header.h"
#include <enet/enet.h>
#include <atomic>
#include <cstdint>
#include <string>

struct ClientMetrics {
    uint64_t bytes_received = 0;
    uint32_t full_snapshots_received = 0;
    uint32_t delta_snapshots_received = 0;
    uint32_t delta_snapshots_dropped = 0;
    uint32_t desync_count = 0;
    uint32_t last_tick = 0;
    double snapshot_apply_time_ms = 0.0;
    double max_apply_time_ms = 0.0;
    double connect_time_s = 0.0;      // time from connect call to first full snapshot applied
    bool connected = false;
    bool first_snapshot_received = false;
};

class Client {
public:
    Client(int id, const std::string& host, uint16_t port,
           float connect_delay_s, uint32_t packet_loss_percent,
           std::atomic<bool>& running);
    ~Client();

    // Run the client loop (blocking, runs until running == false)
    void run();

    const ClientMetrics& metrics() const { return metrics_; }
    int id() const { return id_; }

private:
    void process_events();
    void send_ack(uint32_t tick);
    void send_resync_request();
    void handle_full_snapshot(ENetPacket* packet);
    void handle_delta_snapshot(ENetPacket* packet);

    int id_;
    std::string host_;
    uint16_t port_;
    float connect_delay_s_;
    std::atomic<bool>& running_;

    ENetHost* enet_host_ = nullptr;
    ENetPeer* peer_ = nullptr;
    EntityStore store_;
    SnapshotApplier applier_;
    PacketLossSim loss_sim_;
    ClientMetrics metrics_;

    uint64_t connect_start_counter_ = 0;
};
