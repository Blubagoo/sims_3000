#include "benchmark.h"
#include <cstdio>
#include <algorithm>

Benchmark::Benchmark(const BenchmarkConfig& config)
    : config_(config)
{
}

BenchmarkResult Benchmark::evaluate(const std::vector<const ClientMetrics*>& client_metrics,
                                     const std::vector<int>& client_ids,
                                     const std::vector<bool>& is_late_join,
                                     const std::vector<bool>& has_packet_loss,
                                     double elapsed_s, uint32_t server_ticks) {
    BenchmarkResult result;
    result.elapsed_s = elapsed_s;
    result.server_ticks = server_ticks;
    result.all_passed = true;

    for (size_t i = 0; i < client_metrics.size(); ++i) {
        const auto& m = *client_metrics[i];

        BenchmarkResult::ClientResult cr;
        cr.client_id = client_ids[i];
        cr.is_late_join = is_late_join[i];
        cr.has_packet_loss = has_packet_loss[i];

        // Bandwidth: total bytes received / elapsed time
        double active_time = elapsed_s;
        if (cr.is_late_join) {
            active_time = elapsed_s - config_.late_join_delay_s;
            if (active_time < 1.0) active_time = 1.0;
        }
        cr.bandwidth_kbs = (static_cast<double>(m.bytes_received) / 1024.0) / active_time;

        // Snapshot apply times
        uint32_t total_snapshots = m.full_snapshots_received + m.delta_snapshots_received;
        cr.avg_apply_time_ms = total_snapshots > 0
            ? m.snapshot_apply_time_ms / total_snapshots
            : 0.0;
        cr.max_apply_time_ms = m.max_apply_time_ms;

        cr.late_join_time_s = m.connect_time_s;
        cr.desync_count = m.desync_count;
        cr.full_snapshots = m.full_snapshots_received;
        cr.delta_snapshots = m.delta_snapshots_received;
        cr.delta_dropped = m.delta_snapshots_dropped;

        result.clients.push_back(cr);
    }

    return result;
}

void Benchmark::print_report(const BenchmarkResult& result) const {
    std::printf("\n");
    std::printf("====================================================================\n");
    std::printf("  POC-2: ENet Multiplayer Snapshot Sync - Benchmark Report\n");
    std::printf("====================================================================\n");
    std::printf("  Duration: %.1fs  |  Server ticks: %u\n", result.elapsed_s, result.server_ticks);
    std::printf("====================================================================\n\n");

    bool bandwidth_pass = true;
    bool snapshot_pass = true;
    bool latejoin_pass = true;
    bool desync_pass = true;

    for (const auto& cr : result.clients) {
        std::printf("--- Client %d %s%s ---\n",
                    cr.client_id,
                    cr.is_late_join ? "[LATE-JOIN] " : "",
                    cr.has_packet_loss ? "[5%% LOSS] " : "");
        std::printf("  Bandwidth:        %7.1f KB/s", cr.bandwidth_kbs);
        bool bw_ok = cr.bandwidth_kbs <= config_.fail_bandwidth_kbs;
        bool bw_target = cr.bandwidth_kbs <= config_.max_bandwidth_kbs;
        std::printf("  %s\n", bw_target ? "[PASS]" : (bw_ok ? "[WARN]" : "[FAIL]"));
        if (!bw_ok) bandwidth_pass = false;

        std::printf("  Avg apply time:   %7.2f ms", cr.avg_apply_time_ms);
        std::printf("  Max: %.2f ms", cr.max_apply_time_ms);
        bool snap_ok = cr.max_apply_time_ms <= config_.fail_snapshot_time_ms;
        bool snap_target = cr.max_apply_time_ms <= config_.max_snapshot_time_ms;
        std::printf("  %s\n", snap_target ? "[PASS]" : (snap_ok ? "[WARN]" : "[FAIL]"));
        if (!snap_ok) snapshot_pass = false;

        if (cr.is_late_join) {
            std::printf("  Late-join time:   %7.3f s", cr.late_join_time_s);
            bool lj_ok = cr.late_join_time_s <= config_.fail_latejoin_time_s;
            bool lj_target = cr.late_join_time_s <= config_.max_latejoin_time_s;
            std::printf("  %s\n", lj_target ? "[PASS]" : (lj_ok ? "[WARN]" : "[FAIL]"));
            if (!lj_ok) latejoin_pass = false;
        }

        std::printf("  Desync count:     %7u", cr.desync_count);
        bool ds_ok = cr.desync_count <= config_.max_desync_count;
        std::printf("  %s\n", ds_ok ? "[PASS]" : "[FAIL]");
        if (!ds_ok) desync_pass = false;

        std::printf("  Full snapshots:   %7u\n", cr.full_snapshots);
        std::printf("  Delta snapshots:  %7u  (dropped: %u)\n",
                    cr.delta_snapshots, cr.delta_dropped);
        std::printf("\n");
    }

    std::printf("====================================================================\n");
    std::printf("  SUMMARY\n");
    std::printf("====================================================================\n");
    std::printf("  Bandwidth (<=%.0f KB/s target, <=%.0f fail):  %s\n",
                config_.max_bandwidth_kbs, config_.fail_bandwidth_kbs,
                bandwidth_pass ? "PASS" : "FAIL");
    std::printf("  Snapshot time (<=%.0fms target, <=%.0fms fail): %s\n",
                config_.max_snapshot_time_ms, config_.fail_snapshot_time_ms,
                snapshot_pass ? "PASS" : "FAIL");
    std::printf("  Late-join (<=%.1fs target, <=%.1fs fail):      %s\n",
                config_.max_latejoin_time_s, config_.fail_latejoin_time_s,
                latejoin_pass ? "PASS" : "FAIL");
    std::printf("  Desync (0 allowed):                          %s\n",
                desync_pass ? "PASS" : "FAIL");
    std::printf("====================================================================\n");

    bool all_pass = bandwidth_pass && snapshot_pass && latejoin_pass && desync_pass;
    std::printf("  OVERALL: %s\n", all_pass ? "PASS" : "FAIL");
    std::printf("====================================================================\n\n");
}
