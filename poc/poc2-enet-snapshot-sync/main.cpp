#include "server.h"
#include "client.h"
#include "benchmark.h"
#include <SDL3/SDL.h>
#include <enet/enet.h>
#include <thread>
#include <atomic>
#include <cstdio>
#include <vector>
#include <memory>

int main(int /*argc*/, char* /*argv*/[]) {
    // Initialize SDL (for timing)
    if (!SDL_Init(0)) {
        std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    // Initialize ENet
    if (enet_initialize() != 0) {
        std::fprintf(stderr, "enet_initialize failed\n");
        SDL_Quit();
        return 1;
    }

    std::printf("=== POC-2: ENet Multiplayer Snapshot Sync ===\n");
    std::printf("Entities: %u  |  Tick rate: %.0f Hz\n\n", ENTITY_COUNT, TICK_RATE);

    BenchmarkConfig config;
    config.run_duration_s = 30.0f;
    config.late_join_delay_s = 5.0f;
    config.packet_loss_percent = 5;
    config.port = 7777;

    std::atomic<bool> running{true};

    // Create server
    Server server(config.port, running);

    // Create clients:
    // Client 0: immediate connect, no loss
    // Client 1: immediate connect, no loss
    // Client 2: late-join (5s delay), no loss
    // Client 3: late-join (5s delay), 5% packet loss
    struct ClientInfo {
        int id;
        float delay;
        uint32_t loss;
        bool is_late_join;
        bool has_loss;
    };

    std::vector<ClientInfo> client_infos = {
        {0, 0.0f, 0, false, false},
        {1, 0.0f, 0, false, false},
        {2, config.late_join_delay_s, 0, true, false},
        {3, config.late_join_delay_s, config.packet_loss_percent, true, true},
    };

    std::vector<std::unique_ptr<Client>> clients;
    for (auto& ci : client_infos) {
        clients.push_back(std::make_unique<Client>(
            ci.id, "127.0.0.1", config.port,
            ci.delay, ci.loss, running));
    }

    // Launch threads
    uint64_t start_time = SDL_GetPerformanceCounter();

    std::thread server_thread([&server]() { server.run(); });

    // Small delay to let server start
    SDL_Delay(200);

    std::vector<std::thread> client_threads;
    for (auto& client : clients) {
        client_threads.emplace_back([&client]() { client->run(); });
    }

    // Run for configured duration
    uint64_t duration_ms = static_cast<uint64_t>(config.run_duration_s * 1000.0f);
    uint64_t waited = 0;
    while (running.load() && waited < duration_ms) {
        SDL_Delay(500);
        waited += 500;

        // Progress report every 5 seconds
        if (waited % 5000 == 0) {
            double elapsed = static_cast<double>(SDL_GetPerformanceCounter() - start_time)
                             / SDL_GetPerformanceFrequency();
            std::printf("[Main] %.0fs elapsed, server tick %u\n",
                        elapsed, server.current_tick());
        }
    }

    // Stop everything
    std::printf("\n[Main] Stopping...\n");
    running.store(false);

    for (auto& t : client_threads) {
        if (t.joinable()) t.join();
    }
    server_thread.join();

    double elapsed_s = static_cast<double>(SDL_GetPerformanceCounter() - start_time)
                       / SDL_GetPerformanceFrequency();

    // Collect metrics and evaluate
    Benchmark bench(config);
    std::vector<const ClientMetrics*> metrics;
    std::vector<int> ids;
    std::vector<bool> late_join;
    std::vector<bool> has_loss;

    for (size_t i = 0; i < clients.size(); ++i) {
        metrics.push_back(&clients[i]->metrics());
        ids.push_back(client_infos[i].id);
        late_join.push_back(client_infos[i].is_late_join);
        has_loss.push_back(client_infos[i].has_loss);
    }

    auto result = bench.evaluate(metrics, ids, late_join, has_loss,
                                  elapsed_s, server.current_tick());
    bench.print_report(result);

    // Check pass/fail
    bool all_pass = true;
    for (const auto& cr : result.clients) {
        if (cr.bandwidth_kbs > config.fail_bandwidth_kbs) all_pass = false;
        if (cr.max_apply_time_ms > config.fail_snapshot_time_ms) all_pass = false;
        if (cr.is_late_join && cr.late_join_time_s > config.fail_latejoin_time_s) all_pass = false;
        if (cr.desync_count > config.max_desync_count) all_pass = false;
    }

    enet_deinitialize();
    SDL_Quit();

    return all_pass ? 0 : 1;
}
