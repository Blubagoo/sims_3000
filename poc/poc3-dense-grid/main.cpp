#include "terrain_grid.h"

#include <lz4.h>

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

// ---------------------------------------------------------------------------
// Timing helpers
// ---------------------------------------------------------------------------

using Clock = std::chrono::high_resolution_clock;

struct BenchResult {
    double min_ms;
    double max_ms;
    double avg_ms;
};

template <typename Fn>
BenchResult benchmark(Fn&& fn, int iterations = 100) {
    double total = 0.0;
    double min_val = 1e9;
    double max_val = 0.0;

    // Warm-up run
    fn();

    for (int i = 0; i < iterations; ++i) {
        auto start = Clock::now();
        fn();
        auto end = Clock::now();
        double ms = std::chrono::duration<double, std::milli>(end - start).count();
        total += ms;
        if (ms < min_val) min_val = ms;
        if (ms > max_val) max_val = ms;
    }

    return { min_val, max_val, total / iterations };
}

// ---------------------------------------------------------------------------
// Populate grid with deterministic test data
// ---------------------------------------------------------------------------

void populate_grid(TerrainGrid& grid) {
    for (uint32_t y = 0; y < grid.height(); ++y) {
        for (uint32_t x = 0; x < grid.width(); ++x) {
            auto& tile = grid.at(x, y);
            tile.terrain_type = static_cast<uint8_t>((x + y) % 8);
            tile.elevation = static_cast<uint8_t>((x * 7 + y * 13) % 256);
            tile.moisture = static_cast<uint8_t>((x * 3 + y * 11) % 256);
            tile.flags = static_cast<uint8_t>((x ^ y) & 0x0F);
        }
    }
}

// ---------------------------------------------------------------------------
// Benchmark 1: Full grid iteration (read + write pass)
// ---------------------------------------------------------------------------

volatile uint32_t sink = 0; // prevent optimizing away reads

BenchResult bench_full_iteration(TerrainGrid& grid) {
    return benchmark([&]() {
        uint32_t acc = 0;
        for (auto& tile : grid) {
            acc += tile.elevation;
            tile.moisture = static_cast<uint8_t>(tile.moisture + 1);
        }
        sink = acc;
    });
}

// ---------------------------------------------------------------------------
// Benchmark 2: 3x3 neighbor aggregation across entire grid
// ---------------------------------------------------------------------------

BenchResult bench_neighbor_ops(TerrainGrid& grid) {
    return benchmark([&]() {
        uint32_t acc = 0;
        const int32_t w = static_cast<int32_t>(grid.width());
        const int32_t h = static_cast<int32_t>(grid.height());

        // Skip border tiles to avoid bounds checks in hot loop
        for (int32_t y = 1; y < h - 1; ++y) {
            for (int32_t x = 1; x < w - 1; ++x) {
                uint16_t sum = 0;
                sum += grid.at(x - 1, y - 1).elevation;
                sum += grid.at(x,     y - 1).elevation;
                sum += grid.at(x + 1, y - 1).elevation;
                sum += grid.at(x - 1, y    ).elevation;
                sum += grid.at(x,     y    ).elevation;
                sum += grid.at(x + 1, y    ).elevation;
                sum += grid.at(x - 1, y + 1).elevation;
                sum += grid.at(x,     y + 1).elevation;
                sum += grid.at(x + 1, y + 1).elevation;
                // Store averaged elevation back (simulating a smoothing pass)
                grid.at(x, y).moisture = static_cast<uint8_t>(sum / 9);
                acc += sum;
            }
        }
        sink = acc;
    });
}

// ---------------------------------------------------------------------------
// Benchmark 3: LZ4 serialization (compress + decompress)
// ---------------------------------------------------------------------------

struct SerializationResult {
    BenchResult compress;
    BenchResult decompress;
    int compressed_size;
    int original_size;
};

SerializationResult bench_serialization(TerrainGrid& grid) {
    const int src_size = static_cast<int>(grid.raw_size());
    const int max_dst_size = LZ4_compressBound(src_size);

    std::vector<char> compressed(max_dst_size);
    std::vector<uint8_t> decompressed(src_size);

    int compressed_size = 0;

    auto comp_result = benchmark([&]() {
        compressed_size = LZ4_compress_default(
            reinterpret_cast<const char*>(grid.raw_data()),
            compressed.data(),
            src_size,
            max_dst_size
        );
    }, 50);

    auto decomp_result = benchmark([&]() {
        LZ4_decompress_safe(
            compressed.data(),
            reinterpret_cast<char*>(decompressed.data()),
            compressed_size,
            src_size
        );
    }, 50);

    // Verify round-trip correctness
    LZ4_decompress_safe(
        compressed.data(),
        reinterpret_cast<char*>(decompressed.data()),
        compressed_size,
        src_size
    );
    if (memcmp(grid.raw_data(), decompressed.data(), src_size) != 0) {
        printf("ERROR: LZ4 round-trip verification FAILED!\n");
    }

    return { comp_result, decomp_result, compressed_size, src_size };
}

// ---------------------------------------------------------------------------
// Memory measurement
// ---------------------------------------------------------------------------

struct MemoryResult {
    size_t grid_bytes;
    size_t total_for_all_grids;
};

MemoryResult measure_memory(uint32_t size) {
    // Simulate the full set of grids the game would need at this map size:
    // terrain, elevation overlay, moisture overlay, zone overlay, building flags
    constexpr int NUM_GRIDS = 5;
    TerrainGrid grids[NUM_GRIDS];
    for (int i = 0; i < NUM_GRIDS; ++i) {
        grids[i].resize(size, size);
    }

    size_t single = grids[0].memory_bytes();
    size_t total = single * NUM_GRIDS;
    return { single, total };
}

// ---------------------------------------------------------------------------
// Result formatting
// ---------------------------------------------------------------------------

const char* pass_fail(double value, double target, double failure) {
    if (value <= target) return "PASS";
    if (value <= failure) return "WARN";
    return "FAIL";
}

const char* pass_fail_mem(size_t bytes, size_t target_mb, size_t failure_mb) {
    size_t mb = bytes / (1024 * 1024);
    if (mb <= target_mb) return "PASS";
    if (mb <= failure_mb) return "WARN";
    return "FAIL";
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

int main() {
    printf("=============================================================\n");
    printf("  POC-3: Dense Grid Performance at Scale\n");
    printf("=============================================================\n\n");

    const uint32_t sizes[] = { 128, 256, 512 };
    const int num_sizes = 3;

    for (int s = 0; s < num_sizes; ++s) {
        uint32_t size = sizes[s];
        printf("-------------------------------------------------------------\n");
        printf("  Grid Size: %u x %u  (%u tiles, %u bytes per tile)\n",
               size, size, size * size, (uint32_t)sizeof(TerrainComponent));
        printf("-------------------------------------------------------------\n\n");

        TerrainGrid grid(size, size);
        populate_grid(grid);

        // Benchmark 1: Full iteration
        auto iter_result = bench_full_iteration(grid);
        bool is_target_size = (size == 512);
        printf("  [1] Full Iteration (read+write pass)\n");
        printf("      Min: %.4f ms | Avg: %.4f ms | Max: %.4f ms",
               iter_result.min_ms, iter_result.avg_ms, iter_result.max_ms);
        if (is_target_size) {
            printf("  [%s]", pass_fail(iter_result.avg_ms, 0.5, 2.0));
        }
        printf("\n\n");

        // Benchmark 2: 3x3 neighbor ops
        auto neighbor_result = bench_neighbor_ops(grid);
        printf("  [2] 3x3 Neighbor Aggregation\n");
        printf("      Min: %.4f ms | Avg: %.4f ms | Max: %.4f ms",
               neighbor_result.min_ms, neighbor_result.avg_ms, neighbor_result.max_ms);
        if (is_target_size) {
            printf("  [%s]", pass_fail(neighbor_result.avg_ms, 2.0, 10.0));
        }
        printf("\n\n");

        // Benchmark 3: LZ4 serialization
        auto ser_result = bench_serialization(grid);
        double total_ser = ser_result.compress.avg_ms + ser_result.decompress.avg_ms;
        float ratio = 100.0f * ser_result.compressed_size / ser_result.original_size;
        printf("  [3] LZ4 Serialization\n");
        printf("      Compress:   Min: %.4f ms | Avg: %.4f ms | Max: %.4f ms\n",
               ser_result.compress.min_ms, ser_result.compress.avg_ms, ser_result.compress.max_ms);
        printf("      Decompress: Min: %.4f ms | Avg: %.4f ms | Max: %.4f ms\n",
               ser_result.decompress.min_ms, ser_result.decompress.avg_ms, ser_result.decompress.max_ms);
        printf("      Total (comp+decomp): %.4f ms", total_ser);
        if (is_target_size) {
            printf("  [%s]", pass_fail(total_ser, 10.0, 30.0));
        }
        printf("\n");
        printf("      Original: %d bytes | Compressed: %d bytes (%.1f%%)\n\n",
               ser_result.original_size, ser_result.compressed_size, ratio);

        // Memory measurement
        auto mem = measure_memory(size);
        printf("  [4] Memory Usage\n");
        printf("      Single grid:    %zu bytes (%.2f KB)\n",
               mem.grid_bytes, mem.grid_bytes / 1024.0);
        printf("      All 5 grids:    %zu bytes (%.2f MB)",
               mem.total_for_all_grids, mem.total_for_all_grids / (1024.0 * 1024.0));
        if (is_target_size) {
            printf("  [%s]", pass_fail_mem(mem.total_for_all_grids, 12, 20));
        }
        printf("\n\n");
    }

    // Final summary for 512x512 (the target size)
    printf("=============================================================\n");
    printf("  POC-3 Target Thresholds (512x512)\n");
    printf("=============================================================\n");
    printf("  Metric                    | Target   | Failure\n");
    printf("  --------------------------+----------+---------\n");
    printf("  Full iteration            | <= 0.5ms | > 2ms\n");
    printf("  3x3 neighbor ops          | <= 2ms   | > 10ms\n");
    printf("  LZ4 serialize (comp+dec)  | <= 10ms  | > 30ms\n");
    printf("  Memory (all grids)        | <= 12 MB | > 20 MB\n");
    printf("=============================================================\n");

    return 0;
}
