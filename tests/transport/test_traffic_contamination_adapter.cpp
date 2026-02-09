/**
 * @file test_traffic_contamination_adapter.cpp
 * @brief Tests for TrafficContaminationAdapter (E10-115)
 */

#include <sims3000/transport/TrafficContaminationAdapter.h>
#include <cstdio>
#include <cassert>
#include <cmath>

using namespace sims3000;
using namespace sims3000::transport;
using namespace sims3000::contamination;

void test_empty_adapter() {
    printf("[test_empty_adapter]\n");
    TrafficContaminationAdapter adapter;

    std::vector<ContaminationSourceEntry> entries;
    adapter.get_contamination_sources(entries);

    assert(entries.empty());
    printf("  PASS: empty adapter produces no entries\n");
}

void test_single_tile_max_congestion() {
    printf("[test_single_tile_max_congestion]\n");
    TrafficContaminationAdapter adapter;

    TrafficTileInfo tile;
    tile.x = 10;
    tile.y = 20;
    tile.congestion = 1.0f; // Max congestion
    tile.is_active = true;

    adapter.set_traffic_tiles({tile});

    std::vector<ContaminationSourceEntry> entries;
    adapter.get_contamination_sources(entries);

    assert(entries.size() == 1);
    assert(entries[0].x == 10);
    assert(entries[0].y == 20);
    assert(entries[0].output == TRAFFIC_CONTAM_MAX);
    assert(entries[0].type == ContaminationType::Traffic);

    printf("  PASS: max congestion (1.0) produces output=%u\n", TRAFFIC_CONTAM_MAX);
}

void test_single_tile_min_congestion_threshold() {
    printf("[test_single_tile_min_congestion_threshold]\n");
    TrafficContaminationAdapter adapter;

    TrafficTileInfo tile;
    tile.x = 5;
    tile.y = 15;
    tile.congestion = MIN_CONGESTION_THRESHOLD; // At threshold
    tile.is_active = true;

    adapter.set_traffic_tiles({tile});

    std::vector<ContaminationSourceEntry> entries;
    adapter.get_contamination_sources(entries);

    assert(entries.size() == 1);
    assert(entries[0].x == 5);
    assert(entries[0].y == 15);
    // lerp(5, 50, 0.1) = 5 + (50 - 5) * 0.1 = 5 + 4.5 = 9.5 -> 9
    assert(entries[0].output >= 9 && entries[0].output <= 10);
    assert(entries[0].type == ContaminationType::Traffic);

    printf("  PASS: congestion at threshold (%.2f) produces output=%u\n",
           MIN_CONGESTION_THRESHOLD, entries[0].output);
}

void test_below_threshold_produces_no_contamination() {
    printf("[test_below_threshold_produces_no_contamination]\n");
    TrafficContaminationAdapter adapter;

    std::vector<TrafficTileInfo> tiles;
    for (int i = 0; i < 10; ++i) {
        TrafficTileInfo tile;
        tile.x = i * 10;
        tile.y = i * 10;
        tile.congestion = MIN_CONGESTION_THRESHOLD - 0.01f; // Below threshold
        tile.is_active = true;
        tiles.push_back(tile);
    }

    adapter.set_traffic_tiles(tiles);

    std::vector<ContaminationSourceEntry> entries;
    adapter.get_contamination_sources(entries);

    assert(entries.empty());
    printf("  PASS: congestion below threshold produces no contamination\n");
}

void test_inactive_tiles_produce_no_contamination() {
    printf("[test_inactive_tiles_produce_no_contamination]\n");
    TrafficContaminationAdapter adapter;

    std::vector<TrafficTileInfo> tiles;
    for (int i = 0; i < 10; ++i) {
        TrafficTileInfo tile;
        tile.x = i * 10;
        tile.y = i * 10;
        tile.congestion = 0.5f; // High congestion
        tile.is_active = false; // But inactive
        tiles.push_back(tile);
    }

    adapter.set_traffic_tiles(tiles);

    std::vector<ContaminationSourceEntry> entries;
    adapter.get_contamination_sources(entries);

    assert(entries.empty());
    printf("  PASS: inactive tiles produce no contamination\n");
}

void test_lerp_interpolation() {
    printf("[test_lerp_interpolation]\n");
    TrafficContaminationAdapter adapter;

    // Test various congestion levels
    struct TestCase {
        float congestion;
        uint32_t expected_min;
        uint32_t expected_max;
    };

    TestCase cases[] = {
        {0.0f, 5, 5},     // lerp(5, 50, 0.0) = 5
        {0.25f, 16, 17},  // lerp(5, 50, 0.25) = 5 + 45*0.25 = 16.25
        {0.5f, 27, 28},   // lerp(5, 50, 0.5) = 5 + 45*0.5 = 27.5
        {0.75f, 38, 39},  // lerp(5, 50, 0.75) = 5 + 45*0.75 = 38.75
        {1.0f, 50, 50}    // lerp(5, 50, 1.0) = 50
    };

    for (const auto& test_case : cases) {
        if (test_case.congestion < MIN_CONGESTION_THRESHOLD) {
            continue; // Skip cases below threshold
        }

        TrafficTileInfo tile;
        tile.x = 0;
        tile.y = 0;
        tile.congestion = test_case.congestion;
        tile.is_active = true;

        adapter.set_traffic_tiles({tile});

        std::vector<ContaminationSourceEntry> entries;
        adapter.get_contamination_sources(entries);

        assert(entries.size() == 1);
        assert(entries[0].output >= test_case.expected_min);
        assert(entries[0].output <= test_case.expected_max);

        printf("  PASS: congestion=%.2f produces output=%u (expected %u-%u)\n",
               test_case.congestion, entries[0].output, test_case.expected_min, test_case.expected_max);
    }
}

void test_mixed_tiles() {
    printf("[test_mixed_tiles]\n");
    TrafficContaminationAdapter adapter;

    std::vector<TrafficTileInfo> tiles;

    // Active, above threshold
    tiles.push_back({10, 10, 0.5f, true});
    // Inactive, above threshold
    tiles.push_back({20, 20, 0.5f, false});
    // Active, below threshold
    tiles.push_back({30, 30, 0.05f, true});
    // Active, max congestion
    tiles.push_back({40, 40, 1.0f, true});
    // Active, at threshold
    tiles.push_back({50, 50, MIN_CONGESTION_THRESHOLD, true});

    adapter.set_traffic_tiles(tiles);

    std::vector<ContaminationSourceEntry> entries;
    adapter.get_contamination_sources(entries);

    // Only 3 active tiles with congestion >= threshold (indices 0, 3, 4)
    assert(entries.size() == 3);

    // Verify positions (order preserved)
    assert(entries[0].x == 10 && entries[0].y == 10); // congestion 0.5
    assert(entries[1].x == 40 && entries[1].y == 40); // congestion 1.0
    assert(entries[2].x == 50 && entries[2].y == 50); // congestion at threshold

    printf("  PASS: mixed tiles correctly filtered (3 active above threshold)\n");
}

void test_congestion_clamping() {
    printf("[test_congestion_clamping]\n");
    TrafficContaminationAdapter adapter;

    std::vector<TrafficTileInfo> tiles;

    // Congestion > 1.0 (should be clamped to 1.0)
    tiles.push_back({10, 10, 1.5f, true});
    // Congestion < 0.0 (should be clamped to 0.0, but filtered by threshold)
    tiles.push_back({20, 20, -0.5f, true});

    adapter.set_traffic_tiles(tiles);

    std::vector<ContaminationSourceEntry> entries;
    adapter.get_contamination_sources(entries);

    // Only first tile passes (second is below threshold after clamping)
    assert(entries.size() == 1);
    assert(entries[0].x == 10);
    assert(entries[0].output == TRAFFIC_CONTAM_MAX); // Clamped to 1.0

    printf("  PASS: congestion clamped correctly (>1.0 -> max, <0.0 filtered)\n");
}

void test_clear() {
    printf("[test_clear]\n");
    TrafficContaminationAdapter adapter;

    TrafficTileInfo tile;
    tile.x = 10;
    tile.y = 20;
    tile.congestion = 0.5f;
    tile.is_active = true;

    adapter.set_traffic_tiles({tile});

    std::vector<ContaminationSourceEntry> entries;
    adapter.get_contamination_sources(entries);
    assert(entries.size() == 1);

    // Clear and verify
    adapter.clear();
    entries.clear();
    adapter.get_contamination_sources(entries);
    assert(entries.empty());

    printf("  PASS: clear removes all tiles\n");
}

int main() {
    printf("=== TrafficContaminationAdapter Tests ===\n\n");

    test_empty_adapter();
    test_single_tile_max_congestion();
    test_single_tile_min_congestion_threshold();
    test_below_threshold_produces_no_contamination();
    test_inactive_tiles_produce_no_contamination();
    test_lerp_interpolation();
    test_mixed_tiles();
    test_congestion_clamping();
    test_clear();

    printf("\n=== All TrafficContaminationAdapter tests passed! ===\n");
    return 0;
}
