/**
 * @file test_contamination_decay.cpp
 * @brief Unit tests for ContaminationDecay (Ticket E10-088)
 *
 * Tests cover:
 * - Base decay rate (2/tick)
 * - Water proximity bonus (+3 for dist <= 2)
 * - Bioremediation bonus (+3 for forest/spore)
 * - Decay rate calculation
 * - Grid decay application
 * - Uniform decay (nullptr tile_info)
 */

#include <sims3000/contamination/ContaminationDecay.h>
#include <sims3000/contamination/ContaminationGrid.h>
#include <cstdio>
#include <cstdlib>
#include <vector>

using namespace sims3000::contamination;

// Test result tracking
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) void test_##name()
#define RUN_TEST(name) do { \
    printf("Running %s...", #name); \
    test_##name(); \
    printf(" PASSED\n"); \
    tests_passed++; \
} while(0)

#define ASSERT(condition) do { \
    if (!(condition)) { \
        printf("\n  FAILED: %s (line %d)\n", #condition, __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

#define ASSERT_EQ(a, b) do { \
    if ((a) != (b)) { \
        printf("\n  FAILED: %s == %s (%d != %d) (line %d)\n", \
               #a, #b, static_cast<int>(a), static_cast<int>(b), __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

// =============================================================================
// Decay Rate Calculation Tests
// =============================================================================

TEST(base_decay_rate_only) {
    DecayTileInfo info{255, false, false};  // No water, no bio
    uint8_t rate = calculate_decay_rate(info);
    ASSERT_EQ(rate, BASE_DECAY_RATE);  // 2
}

TEST(water_proximity_bonus_on_water) {
    DecayTileInfo info{0, false, false};  // On water
    uint8_t rate = calculate_decay_rate(info);
    ASSERT_EQ(rate, BASE_DECAY_RATE + WATER_DECAY_BONUS);  // 2 + 3 = 5
}

TEST(water_proximity_bonus_adjacent) {
    DecayTileInfo info{1, false, false};  // Adjacent to water
    uint8_t rate = calculate_decay_rate(info);
    ASSERT_EQ(rate, BASE_DECAY_RATE + WATER_DECAY_BONUS);  // 5
}

TEST(water_proximity_bonus_two_away) {
    DecayTileInfo info{2, false, false};  // Two tiles from water
    uint8_t rate = calculate_decay_rate(info);
    ASSERT_EQ(rate, BASE_DECAY_RATE + WATER_DECAY_BONUS);  // 5
}

TEST(water_proximity_no_bonus_far) {
    DecayTileInfo info{3, false, false};  // Three tiles from water
    uint8_t rate = calculate_decay_rate(info);
    ASSERT_EQ(rate, BASE_DECAY_RATE);  // 2
}

TEST(forest_bioremediation_bonus) {
    DecayTileInfo info{255, true, false};  // Forest, no water
    uint8_t rate = calculate_decay_rate(info);
    ASSERT_EQ(rate, BASE_DECAY_RATE + BIO_DECAY_BONUS);  // 2 + 3 = 5
}

TEST(spore_plains_bioremediation_bonus) {
    DecayTileInfo info{255, false, true};  // Spore plains, no water
    uint8_t rate = calculate_decay_rate(info);
    ASSERT_EQ(rate, BASE_DECAY_RATE + BIO_DECAY_BONUS);  // 5
}

TEST(forest_and_spore_same_bonus) {
    DecayTileInfo forest{255, true, false};
    DecayTileInfo spore{255, false, true};
    ASSERT_EQ(calculate_decay_rate(forest), calculate_decay_rate(spore));
}

TEST(water_plus_forest) {
    DecayTileInfo info{1, true, false};  // Adjacent water + forest
    uint8_t rate = calculate_decay_rate(info);
    ASSERT_EQ(rate, BASE_DECAY_RATE + WATER_DECAY_BONUS + BIO_DECAY_BONUS);  // 2 + 3 + 3 = 8
}

TEST(water_plus_spore) {
    DecayTileInfo info{2, false, true};  // Near water + spore plains
    uint8_t rate = calculate_decay_rate(info);
    ASSERT_EQ(rate, BASE_DECAY_RATE + WATER_DECAY_BONUS + BIO_DECAY_BONUS);  // 8
}

TEST(max_decay_rate) {
    DecayTileInfo info{0, true, false};  // On water + forest
    uint8_t rate = calculate_decay_rate(info);
    ASSERT_EQ(rate, static_cast<uint8_t>(8));  // Maximum decay
}

// =============================================================================
// Grid Decay Application Tests
// =============================================================================

TEST(apply_decay_uniform_base_rate) {
    ContaminationGrid grid(8, 8);
    grid.add_contamination(0, 0, 10, 1);
    grid.add_contamination(1, 1, 20, 1);
    grid.add_contamination(2, 2, 30, 1);

    // Apply uniform decay (nullptr = base rate only)
    apply_contamination_decay(grid, nullptr);

    // Each tile should decay by BASE_DECAY_RATE (2)
    ASSERT_EQ(grid.get_level(0, 0), static_cast<uint8_t>(8));   // 10 - 2
    ASSERT_EQ(grid.get_level(1, 1), static_cast<uint8_t>(18));  // 20 - 2
    ASSERT_EQ(grid.get_level(2, 2), static_cast<uint8_t>(28));  // 30 - 2
}

TEST(apply_decay_with_tile_info) {
    ContaminationGrid grid(4, 4);
    grid.add_contamination(0, 0, 20, 1);
    grid.add_contamination(1, 0, 20, 1);
    grid.add_contamination(2, 0, 20, 1);
    grid.add_contamination(3, 0, 20, 1);

    // Set up tile info array (row-major order)
    std::vector<DecayTileInfo> tile_info(16);
    tile_info[0] = {255, false, false};  // (0,0): base only (2)
    tile_info[1] = {1, false, false};    // (1,0): water bonus (5)
    tile_info[2] = {255, true, false};   // (2,0): forest bonus (5)
    tile_info[3] = {1, true, false};     // (3,0): water + forest (8)

    apply_contamination_decay(grid, tile_info.data());

    ASSERT_EQ(grid.get_level(0, 0), static_cast<uint8_t>(18));  // 20 - 2
    ASSERT_EQ(grid.get_level(1, 0), static_cast<uint8_t>(15));  // 20 - 5
    ASSERT_EQ(grid.get_level(2, 0), static_cast<uint8_t>(15));  // 20 - 5
    ASSERT_EQ(grid.get_level(3, 0), static_cast<uint8_t>(12));  // 20 - 8
}

TEST(apply_decay_saturates_at_zero) {
    ContaminationGrid grid(4, 4);
    grid.add_contamination(0, 0, 5, 1);
    grid.add_contamination(1, 0, 3, 1);

    std::vector<DecayTileInfo> tile_info(16);
    tile_info[0] = {255, false, false};  // base (2)
    tile_info[1] = {1, true, false};     // water + forest (8)

    apply_contamination_decay(grid, tile_info.data());

    ASSERT_EQ(grid.get_level(0, 0), static_cast<uint8_t>(3));  // 5 - 2
    ASSERT_EQ(grid.get_level(1, 0), static_cast<uint8_t>(0));  // 3 - 8 = 0 (saturated)
}

TEST(apply_decay_skips_empty_tiles) {
    ContaminationGrid grid(4, 4);
    grid.add_contamination(0, 0, 10, 1);
    // (1,0) is empty
    grid.add_contamination(2, 0, 10, 1);

    apply_contamination_decay(grid, nullptr);

    ASSERT_EQ(grid.get_level(0, 0), static_cast<uint8_t>(8));   // 10 - 2
    ASSERT_EQ(grid.get_level(1, 0), static_cast<uint8_t>(0));   // Was 0, stays 0
    ASSERT_EQ(grid.get_level(2, 0), static_cast<uint8_t>(8));   // 10 - 2
}

TEST(apply_decay_full_grid) {
    ContaminationGrid grid(8, 8);

    // Fill grid with contamination
    for (int32_t y = 0; y < 8; ++y) {
        for (int32_t x = 0; x < 8; ++x) {
            grid.add_contamination(x, y, 100, 1);
        }
    }

    // Set up tile info with various modifiers
    std::vector<DecayTileInfo> tile_info(64);
    for (size_t i = 0; i < 64; ++i) {
        // First row: water bonus
        if (i < 8) {
            tile_info[i] = {1, false, false};
        }
        // Second row: forest bonus
        else if (i >= 8 && i < 16) {
            tile_info[i] = {255, true, false};
        }
        // Third row: both bonuses
        else if (i >= 16 && i < 24) {
            tile_info[i] = {2, false, true};
        }
        // Rest: base only
        else {
            tile_info[i] = {255, false, false};
        }
    }

    apply_contamination_decay(grid, tile_info.data());

    // First row: 100 - 5 = 95
    ASSERT_EQ(grid.get_level(0, 0), static_cast<uint8_t>(95));
    ASSERT_EQ(grid.get_level(7, 0), static_cast<uint8_t>(95));

    // Second row: 100 - 5 = 95
    ASSERT_EQ(grid.get_level(0, 1), static_cast<uint8_t>(95));
    ASSERT_EQ(grid.get_level(7, 1), static_cast<uint8_t>(95));

    // Third row: 100 - 8 = 92
    ASSERT_EQ(grid.get_level(0, 2), static_cast<uint8_t>(92));
    ASSERT_EQ(grid.get_level(7, 2), static_cast<uint8_t>(92));

    // Rest: 100 - 2 = 98
    ASSERT_EQ(grid.get_level(0, 3), static_cast<uint8_t>(98));
    ASSERT_EQ(grid.get_level(7, 7), static_cast<uint8_t>(98));
}

TEST(apply_decay_resets_type_at_zero) {
    ContaminationGrid grid(4, 4);
    grid.add_contamination(0, 0, 5, 3);  // Type 3
    ASSERT_EQ(grid.get_dominant_type(0, 0), 3);

    std::vector<DecayTileInfo> tile_info(16);
    tile_info[0] = {0, true, false};  // Max decay (8)

    apply_contamination_decay(grid, tile_info.data());

    ASSERT_EQ(grid.get_level(0, 0), static_cast<uint8_t>(0));
    ASSERT_EQ(grid.get_dominant_type(0, 0), static_cast<uint8_t>(0));  // Type reset
}

TEST(apply_decay_preserves_type_above_zero) {
    ContaminationGrid grid(4, 4);
    grid.add_contamination(0, 0, 20, 3);  // Type 3
    ASSERT_EQ(grid.get_dominant_type(0, 0), 3);

    std::vector<DecayTileInfo> tile_info(16);
    tile_info[0] = {255, false, false};  // Base decay (2)

    apply_contamination_decay(grid, tile_info.data());

    ASSERT_EQ(grid.get_level(0, 0), static_cast<uint8_t>(18));
    ASSERT_EQ(grid.get_dominant_type(0, 0), static_cast<uint8_t>(3));  // Type preserved
}

TEST(apply_decay_empty_grid) {
    ContaminationGrid grid(8, 8);
    // Grid is empty (all zeros)

    apply_contamination_decay(grid, nullptr);

    // Should remain empty (no changes)
    ASSERT_EQ(grid.get_level(0, 0), static_cast<uint8_t>(0));
    ASSERT_EQ(grid.get_level(4, 4), static_cast<uint8_t>(0));
    ASSERT_EQ(grid.get_level(7, 7), static_cast<uint8_t>(0));
}

TEST(apply_decay_multiple_ticks) {
    ContaminationGrid grid(4, 4);
    grid.add_contamination(0, 0, 100, 1);

    // Apply decay multiple times (simulate multiple ticks)
    for (int i = 0; i < 10; ++i) {
        apply_contamination_decay(grid, nullptr);
    }

    // After 10 ticks with base decay (2/tick): 100 - 20 = 80
    ASSERT_EQ(grid.get_level(0, 0), static_cast<uint8_t>(80));
}

// =============================================================================
// Constant Verification Tests
// =============================================================================

TEST(constants_values) {
    ASSERT_EQ(BASE_DECAY_RATE, static_cast<uint8_t>(2));
    ASSERT_EQ(WATER_DECAY_BONUS, static_cast<uint8_t>(3));
    ASSERT_EQ(BIO_DECAY_BONUS, static_cast<uint8_t>(3));
}

// =============================================================================
// Main Entry Point
// =============================================================================

int main() {
    printf("=== ContaminationDecay Unit Tests (Ticket E10-088) ===\n\n");

    // Decay rate calculation tests
    RUN_TEST(base_decay_rate_only);
    RUN_TEST(water_proximity_bonus_on_water);
    RUN_TEST(water_proximity_bonus_adjacent);
    RUN_TEST(water_proximity_bonus_two_away);
    RUN_TEST(water_proximity_no_bonus_far);
    RUN_TEST(forest_bioremediation_bonus);
    RUN_TEST(spore_plains_bioremediation_bonus);
    RUN_TEST(forest_and_spore_same_bonus);
    RUN_TEST(water_plus_forest);
    RUN_TEST(water_plus_spore);
    RUN_TEST(max_decay_rate);

    // Grid decay application tests
    RUN_TEST(apply_decay_uniform_base_rate);
    RUN_TEST(apply_decay_with_tile_info);
    RUN_TEST(apply_decay_saturates_at_zero);
    RUN_TEST(apply_decay_skips_empty_tiles);
    RUN_TEST(apply_decay_full_grid);
    RUN_TEST(apply_decay_resets_type_at_zero);
    RUN_TEST(apply_decay_preserves_type_above_zero);
    RUN_TEST(apply_decay_empty_grid);
    RUN_TEST(apply_decay_multiple_ticks);

    // Constants tests
    RUN_TEST(constants_values);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
