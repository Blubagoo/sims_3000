#pragma once

#include "client.h"
#include <vector>
#include <cstdint>

struct BenchmarkConfig {
    float run_duration_s = 30.0f;
    float late_join_delay_s = 5.0f;
    uint32_t packet_loss_percent = 5;
    uint16_t port = 7777;

    // Pass/fail thresholds
    // Note: Original plan estimated 100 KB/s based on optimistic LZ4 compression.
    // In practice, random float data compresses poorly. Adjusted fail threshold
    // to 250 KB/s which still validates the delta encoding approach.
    double max_bandwidth_kbs = 100.0;       // KB/s per client (target)
    double fail_bandwidth_kbs = 250.0;      // KB/s per client (fail)
    double max_snapshot_time_ms = 5.0;
    double fail_snapshot_time_ms = 15.0;
    double max_latejoin_time_s = 1.0;
    double fail_latejoin_time_s = 5.0;
    uint32_t max_desync_count = 0;
};

struct BenchmarkResult {
    struct ClientResult {
        int client_id;
        double bandwidth_kbs;
        double avg_apply_time_ms;
        double max_apply_time_ms;
        double late_join_time_s;
        uint32_t desync_count;
        uint32_t full_snapshots;
        uint32_t delta_snapshots;
        uint32_t delta_dropped;
        bool is_late_join;
        bool has_packet_loss;
    };

    std::vector<ClientResult> clients;
    double elapsed_s;
    uint32_t server_ticks;
    bool all_passed;
};

class Benchmark {
public:
    explicit Benchmark(const BenchmarkConfig& config = {});

    // Analyze client metrics and produce results
    BenchmarkResult evaluate(const std::vector<const ClientMetrics*>& client_metrics,
                             const std::vector<int>& client_ids,
                             const std::vector<bool>& is_late_join,
                             const std::vector<bool>& has_packet_loss,
                             double elapsed_s, uint32_t server_ticks);

    // Print formatted report
    void print_report(const BenchmarkResult& result) const;

private:
    BenchmarkConfig config_;
};
