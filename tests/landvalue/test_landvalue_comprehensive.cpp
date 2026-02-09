/**
 * @file test_landvalue_comprehensive.cpp
 * @brief Comprehensive unit tests for land value system (Ticket E10-125)
 *
 * Tests cover:
 * 1. Terrain bonuses: each terrain type, water proximity
 * 2. Road accessibility: all distance tiers
 * 3. Disorder penalty: scaling 0-40, previous tick read
 * 4. Contamination penalty: scaling 0-50, previous tick read
 * 5. Full value recalculation: base 128 + bonuses - penalties
 * 6. Value clamping to [0, 255]
 * 7. Previous-tick double-buffer data access
 * 8. Combined scenario: terrain + road + disorder + contamination
 */

#include <sims3000/landvalue/TerrainValueFactors.h>
#include <sims3000/landvalue/RoadAccessBonus.h>
#include <sims3000/landvalue/DisorderPenalty.h>
#include <sims3000/landvalue/ContaminationPenalty.h>
#include <sims3000/landvalue/LandValueGrid.h>
#include <sims3000/disorder/DisorderGrid.h>
#include <sims3000/contamination/ContaminationGrid.h>

#include <cstdio>
#include <cstdlib>
#include <vector>

using namespace sims3000::landvalue;
using namespace sims3000::disorder;
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
        printf("\n  FAILED: %s == %s (line %d)\n", #a, #b, __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

// =============================================================================
// Terrain Bonus Tests
// =============================================================================

TEST(terrain_bonus_crystal_fields) {
    // Crystal fields (PrismaFields): +25
    int8_t bonus = calculate_terrain_bonus(6, 255);  // terrain type 6 = crystal, no water
    ASSERT_EQ(bonus, 25);
}

TEST(terrain_bonus_spore_plains) {
    // Spore plains (SporeFlats): +15
    int8_t bonus = calculate_terrain_bonus(7, 255);  // terrain type 7 = spore
    ASSERT_EQ(bonus, 15);
}

TEST(terrain_bonus_forest) {
    // Forest (BiolumeGrove): +10
    int8_t bonus = calculate_terrain_bonus(5, 255);  // terrain type 5 = forest
    ASSERT_EQ(bonus, 10);
}

TEST(terrain_penalty_toxic_marshes) {
    // Toxic marshes (BlightMires): -30
    int8_t bonus = calculate_terrain_bonus(8, 255);  // terrain type 8 = toxic
    ASSERT_EQ(bonus, -30);
}

TEST(terrain_bonus_water_adjacent) {
    // Water adjacent (dist <= 1): +30
    int8_t bonus = calculate_terrain_bonus(0, 1);  // neutral terrain, water distance 1
    ASSERT_EQ(bonus, 30);
}

TEST(terrain_bonus_water_one_tile) {
    // One tile from water (dist == 2): +20
    int8_t bonus = calculate_terrain_bonus(0, 2);
    ASSERT_EQ(bonus, 20);
}

TEST(terrain_bonus_water_two_tiles) {
    // Two tiles from water (dist == 3): +10
    int8_t bonus = calculate_terrain_bonus(0, 3);
    ASSERT_EQ(bonus, 10);
}

TEST(terrain_bonus_combined_crystal_and_water) {
    // Crystal fields + water adjacent: +25 + +30 = +55
    int8_t bonus = calculate_terrain_bonus(6, 1);  // terrain type 6 = crystal
    ASSERT_EQ(bonus, 55);
}

TEST(terrain_bonus_applied_to_grid) {
    LandValueGrid grid(10, 10);

    std::vector<TerrainTileInfo> terrain_info;
    terrain_info.push_back({5, 5, 6, 255});  // Crystal fields (type 6), no water

    apply_terrain_bonuses(grid, terrain_info);

    // Base 128 + 25 = 153
    ASSERT_EQ(grid.get_value(5, 5), 153);
    ASSERT_EQ(grid.get_terrain_bonus(5, 5), 25);
}

TEST(terrain_penalty_applied_to_grid) {
    LandValueGrid grid(10, 10);

    std::vector<TerrainTileInfo> terrain_info;
    terrain_info.push_back({5, 5, 8, 255});  // Toxic marshes (type 8), no water

    apply_terrain_bonuses(grid, terrain_info);

    // Base 128 - 30 = 98
    ASSERT_EQ(grid.get_value(5, 5), 98);
}

// =============================================================================
// Road Access Bonus Tests
// =============================================================================

TEST(road_bonus_on_road) {
    // On road (dist 0): +20
    uint8_t bonus = calculate_road_bonus(0);
    ASSERT_EQ(bonus, 20);
}

TEST(road_bonus_distance_1) {
    // One tile from road (dist 1): +15
    uint8_t bonus = calculate_road_bonus(1);
    ASSERT_EQ(bonus, 15);
}

TEST(road_bonus_distance_2) {
    // Two tiles from road (dist 2): +10
    uint8_t bonus = calculate_road_bonus(2);
    ASSERT_EQ(bonus, 10);
}

TEST(road_bonus_distance_3) {
    // Three tiles from road (dist 3): +5
    uint8_t bonus = calculate_road_bonus(3);
    ASSERT_EQ(bonus, 5);
}

TEST(road_bonus_distance_4_no_bonus) {
    // Four+ tiles from road (dist >= 4): +0
    uint8_t bonus = calculate_road_bonus(4);
    ASSERT_EQ(bonus, 0);
}

TEST(road_bonus_applied_to_grid) {
    LandValueGrid grid(10, 10);

    std::vector<RoadDistanceInfo> road_info;
    road_info.push_back({5, 5, 0});  // On road

    apply_road_bonuses(grid, road_info);

    // Base 128 + 20 = 148
    ASSERT_EQ(grid.get_value(5, 5), 148);
}

TEST(road_bonus_no_road_nearby) {
    LandValueGrid grid(10, 10);

    std::vector<RoadDistanceInfo> road_info;
    road_info.push_back({5, 5, 255});  // No road

    apply_road_bonuses(grid, road_info);

    // Base 128 + 0 = 128 (unchanged)
    ASSERT_EQ(grid.get_value(5, 5), 128);
}

// =============================================================================
// Disorder Penalty Tests
// =============================================================================

TEST(disorder_penalty_zero) {
    // Zero disorder: 0 penalty
    uint8_t penalty = calculate_disorder_penalty(0);
    ASSERT_EQ(penalty, 0);
}

TEST(disorder_penalty_max) {
    // Max disorder (255): 40 penalty
    uint8_t penalty = calculate_disorder_penalty(255);
    ASSERT_EQ(penalty, 40);
}

TEST(disorder_penalty_half) {
    // Half disorder (128): ~20 penalty
    uint8_t penalty = calculate_disorder_penalty(128);
    ASSERT(penalty >= 20 && penalty <= 21);
}

TEST(disorder_penalty_reads_previous_tick) {
    LandValueGrid value_grid(10, 10);
    DisorderGrid disorder_grid(10, 10);

    // Set disorder in current buffer
    disorder_grid.set_level(5, 5, 100);

    // Swap buffers so it's in previous buffer
    disorder_grid.swap_buffers();

    // Apply penalty (should read from previous buffer)
    apply_disorder_penalties(value_grid, disorder_grid);

    // Penalty for 100: (100 * 40) / 255 = ~15
    uint8_t penalty = (100 * 40) / 255;
    ASSERT_EQ(value_grid.get_value(5, 5), 128 - penalty);
}

TEST(disorder_penalty_applied_to_grid) {
    LandValueGrid value_grid(10, 10);
    DisorderGrid disorder_grid(10, 10);

    // Set disorder
    disorder_grid.set_level(5, 5, 200);
    disorder_grid.swap_buffers();

    // Apply penalties
    apply_disorder_penalties(value_grid, disorder_grid);

    // Penalty for 200: (200 * 40) / 255 = ~31
    uint8_t penalty = (200 * 40) / 255;
    ASSERT_EQ(value_grid.get_value(5, 5), 128 - penalty);
}

// =============================================================================
// Contamination Penalty Tests
// =============================================================================

TEST(contamination_penalty_zero) {
    // Zero contamination: 0 penalty
    uint8_t penalty = calculate_contamination_penalty(0);
    ASSERT_EQ(penalty, 0);
}

TEST(contamination_penalty_max) {
    // Max contamination (255): 50 penalty
    uint8_t penalty = calculate_contamination_penalty(255);
    ASSERT_EQ(penalty, 50);
}

TEST(contamination_penalty_half) {
    // Half contamination (128): ~25 penalty
    uint8_t penalty = calculate_contamination_penalty(128);
    ASSERT(penalty >= 25 && penalty <= 26);
}

TEST(contamination_penalty_reads_previous_tick) {
    LandValueGrid value_grid(10, 10);
    ContaminationGrid contam_grid(10, 10);

    // Set contamination in current buffer
    contam_grid.set_level(5, 5, 100);

    // Swap buffers so it's in previous buffer
    contam_grid.swap_buffers();

    // Apply penalty (should read from previous buffer)
    apply_contamination_penalties(value_grid, contam_grid);

    // Penalty for 100: (100 * 50) / 255 = ~19
    uint8_t penalty = (100 * 50) / 255;
    ASSERT_EQ(value_grid.get_value(5, 5), 128 - penalty);
}

TEST(contamination_penalty_applied_to_grid) {
    LandValueGrid value_grid(10, 10);
    ContaminationGrid contam_grid(10, 10);

    // Set contamination
    contam_grid.set_level(5, 5, 200);
    contam_grid.swap_buffers();

    // Apply penalties
    apply_contamination_penalties(value_grid, contam_grid);

    // Penalty for 200: (200 * 50) / 255 = ~39
    uint8_t penalty = (200 * 50) / 255;
    ASSERT_EQ(value_grid.get_value(5, 5), 128 - penalty);
}

// =============================================================================
// Value Clamping Tests
// =============================================================================

TEST(value_clamping_overflow) {
    LandValueGrid grid(10, 10);

    std::vector<TerrainTileInfo> terrain_info;
    // Crystal + water adjacent: +25 + +30 = +55
    terrain_info.push_back({5, 5, 6, 1});  // terrain type 6 = crystal

    std::vector<RoadDistanceInfo> road_info;
    // On road: +20
    road_info.push_back({5, 5, 0});

    // Apply bonuses: 128 + 55 + 20 = 203
    apply_terrain_bonuses(grid, terrain_info);
    apply_road_bonuses(grid, road_info);

    // Try to add more (should clamp at 255)
    ASSERT_EQ(grid.get_value(5, 5), 203);
}

TEST(value_clamping_underflow) {
    LandValueGrid grid(10, 10);
    DisorderGrid disorder_grid(10, 10);
    ContaminationGrid contam_grid(10, 10);

    // Set max disorder and contamination
    disorder_grid.set_level(5, 5, 255);
    disorder_grid.swap_buffers();
    contam_grid.set_level(5, 5, 255);
    contam_grid.swap_buffers();

    // Apply penalties: 128 - 40 - 50 = 38
    apply_disorder_penalties(grid, disorder_grid);
    apply_contamination_penalties(grid, contam_grid);

    // Should clamp at 0
    ASSERT_EQ(grid.get_value(5, 5), 38);
}

TEST(value_clamping_negative_terrain_bonus) {
    LandValueGrid grid(10, 10);

    std::vector<TerrainTileInfo> terrain_info;
    // Toxic marshes: -30
    terrain_info.push_back({5, 5, 8, 255});  // terrain type 8 = toxic

    apply_terrain_bonuses(grid, terrain_info);

    // Base 128 - 30 = 98 (should not go below 0)
    ASSERT_EQ(grid.get_value(5, 5), 98);
}

// =============================================================================
// Full Recalculation Tests
// =============================================================================

TEST(full_recalculation_all_bonuses) {
    LandValueGrid grid(10, 10);

    std::vector<TerrainTileInfo> terrain_info;
    terrain_info.push_back({5, 5, 6, 1});  // Crystal (type 6) + water adjacent: +55

    std::vector<RoadDistanceInfo> road_info;
    road_info.push_back({5, 5, 0});  // On road: +20

    // Apply all bonuses
    apply_terrain_bonuses(grid, terrain_info);
    apply_road_bonuses(grid, road_info);

    // Base 128 + 55 + 20 = 203
    ASSERT_EQ(grid.get_value(5, 5), 203);
}

TEST(full_recalculation_all_penalties) {
    LandValueGrid grid(10, 10);
    DisorderGrid disorder_grid(10, 10);
    ContaminationGrid contam_grid(10, 10);

    // Set disorder and contamination
    disorder_grid.set_level(5, 5, 100);
    disorder_grid.swap_buffers();
    contam_grid.set_level(5, 5, 100);
    contam_grid.swap_buffers();

    // Apply all penalties
    apply_disorder_penalties(grid, disorder_grid);
    apply_contamination_penalties(grid, contam_grid);

    // Disorder penalty: (100 * 40) / 255 = ~15
    // Contamination penalty: (100 * 50) / 255 = ~19
    // Base 128 - 15 - 19 = 94
    uint8_t disorder_penalty = (100 * 40) / 255;
    uint8_t contam_penalty = (100 * 50) / 255;
    ASSERT_EQ(grid.get_value(5, 5), 128 - disorder_penalty - contam_penalty);
}

TEST(full_recalculation_bonuses_and_penalties) {
    LandValueGrid grid(10, 10);
    DisorderGrid disorder_grid(10, 10);
    ContaminationGrid contam_grid(10, 10);

    // Apply terrain bonus
    std::vector<TerrainTileInfo> terrain_info;
    terrain_info.push_back({5, 5, 5, 2});  // Forest (type 5) + water 1 tile: +10 + +20 = +30
    apply_terrain_bonuses(grid, terrain_info);

    // Apply road bonus
    std::vector<RoadDistanceInfo> road_info;
    road_info.push_back({5, 5, 1});  // One tile from road: +15
    apply_road_bonuses(grid, road_info);

    // Set disorder and contamination
    disorder_grid.set_level(5, 5, 50);
    disorder_grid.swap_buffers();
    contam_grid.set_level(5, 5, 50);
    contam_grid.swap_buffers();

    // Apply penalties
    apply_disorder_penalties(grid, disorder_grid);
    apply_contamination_penalties(grid, contam_grid);

    // Base: 128
    // Bonuses: +30 + +15 = +45
    // Disorder penalty: (50 * 40) / 255 = ~7
    // Contamination penalty: (50 * 50) / 255 = ~9
    // Total: 128 + 45 - 7 - 9 = 157
    uint8_t disorder_penalty = (50 * 40) / 255;
    uint8_t contam_penalty = (50 * 50) / 255;
    ASSERT_EQ(grid.get_value(5, 5), 128 + 30 + 15 - disorder_penalty - contam_penalty);
}

// =============================================================================
// Double-Buffer Previous Tick Tests
// =============================================================================

TEST(disorder_reads_previous_tick_not_current) {
    LandValueGrid value_grid(10, 10);
    DisorderGrid disorder_grid(10, 10);

    // Set disorder in current buffer
    disorder_grid.set_level(5, 5, 200);

    // Without swapping, previous buffer is still 0
    apply_disorder_penalties(value_grid, disorder_grid);

    // Should read 0 from previous buffer (no penalty)
    ASSERT_EQ(value_grid.get_value(5, 5), 128);
}

TEST(contamination_reads_previous_tick_not_current) {
    LandValueGrid value_grid(10, 10);
    ContaminationGrid contam_grid(10, 10);

    // Set contamination in current buffer
    contam_grid.set_level(5, 5, 200);

    // Without swapping, previous buffer is still 0
    apply_contamination_penalties(value_grid, contam_grid);

    // Should read 0 from previous buffer (no penalty)
    ASSERT_EQ(value_grid.get_value(5, 5), 128);
}

TEST(multi_tick_simulation) {
    LandValueGrid value_grid(10, 10);
    DisorderGrid disorder_grid(10, 10);

    // Tick 1: Set disorder
    disorder_grid.set_level(5, 5, 100);
    disorder_grid.swap_buffers();
    value_grid.reset_values();
    apply_disorder_penalties(value_grid, disorder_grid);
    uint8_t value_tick1 = value_grid.get_value(5, 5);

    // Tick 2: Increase disorder
    disorder_grid.set_level(5, 5, 150);
    disorder_grid.swap_buffers();
    value_grid.reset_values();
    apply_disorder_penalties(value_grid, disorder_grid);
    uint8_t value_tick2 = value_grid.get_value(5, 5);

    // Value should decrease as disorder increases
    ASSERT(value_tick2 < value_tick1);
}

// =============================================================================
// Combined Scenario Tests
// =============================================================================

TEST(combined_scenario_pristine_area) {
    LandValueGrid grid(10, 10);
    DisorderGrid disorder_grid(10, 10);
    ContaminationGrid contam_grid(10, 10);

    // Pristine area: crystal fields near water, on road, no disorder/contamination
    std::vector<TerrainTileInfo> terrain_info;
    terrain_info.push_back({5, 5, 6, 1});  // Crystal (type 6) + water: +55

    std::vector<RoadDistanceInfo> road_info;
    road_info.push_back({5, 5, 0});  // On road: +20

    apply_terrain_bonuses(grid, terrain_info);
    apply_road_bonuses(grid, road_info);
    apply_disorder_penalties(grid, disorder_grid);
    apply_contamination_penalties(grid, contam_grid);

    // Base 128 + 55 + 20 = 203
    ASSERT_EQ(grid.get_value(5, 5), 203);
}

TEST(combined_scenario_degraded_area) {
    LandValueGrid grid(10, 10);
    DisorderGrid disorder_grid(10, 10);
    ContaminationGrid contam_grid(10, 10);

    // Degraded area: toxic marshes, no roads, high disorder/contamination
    std::vector<TerrainTileInfo> terrain_info;
    terrain_info.push_back({5, 5, 8, 255});  // Toxic (type 8): -30

    std::vector<RoadDistanceInfo> road_info;
    road_info.push_back({5, 5, 255});  // No road: +0

    disorder_grid.set_level(5, 5, 200);
    disorder_grid.swap_buffers();
    contam_grid.set_level(5, 5, 200);
    contam_grid.swap_buffers();

    apply_terrain_bonuses(grid, terrain_info);
    apply_road_bonuses(grid, road_info);
    apply_disorder_penalties(grid, disorder_grid);
    apply_contamination_penalties(grid, contam_grid);

    // Base 128 - 30 - disorder - contamination
    uint8_t disorder_penalty = (200 * 40) / 255;
    uint8_t contam_penalty = (200 * 50) / 255;
    ASSERT_EQ(grid.get_value(5, 5), 128 - 30 - disorder_penalty - contam_penalty);
}

TEST(combined_scenario_urban_core) {
    LandValueGrid grid(10, 10);
    DisorderGrid disorder_grid(10, 10);
    ContaminationGrid contam_grid(10, 10);

    // Urban core: neutral terrain, on road, moderate disorder, low contamination
    std::vector<TerrainTileInfo> terrain_info;
    terrain_info.push_back({5, 5, 0, 255});  // Neutral terrain: +0

    std::vector<RoadDistanceInfo> road_info;
    road_info.push_back({5, 5, 0});  // On road: +20

    disorder_grid.set_level(5, 5, 80);
    disorder_grid.swap_buffers();
    contam_grid.set_level(5, 5, 30);
    contam_grid.swap_buffers();

    apply_terrain_bonuses(grid, terrain_info);
    apply_road_bonuses(grid, road_info);
    apply_disorder_penalties(grid, disorder_grid);
    apply_contamination_penalties(grid, contam_grid);

    // Base 128 + 20 - disorder - contamination
    uint8_t disorder_penalty = (80 * 40) / 255;
    uint8_t contam_penalty = (30 * 50) / 255;
    ASSERT_EQ(grid.get_value(5, 5), 128 + 20 - disorder_penalty - contam_penalty);
}

// =============================================================================
// Main
// =============================================================================

int main() {
    printf("=== Land Value Comprehensive Tests ===\n\n");

    RUN_TEST(terrain_bonus_crystal_fields);
    RUN_TEST(terrain_bonus_spore_plains);
    RUN_TEST(terrain_bonus_forest);
    RUN_TEST(terrain_penalty_toxic_marshes);
    RUN_TEST(terrain_bonus_water_adjacent);
    RUN_TEST(terrain_bonus_water_one_tile);
    RUN_TEST(terrain_bonus_water_two_tiles);
    RUN_TEST(terrain_bonus_combined_crystal_and_water);
    RUN_TEST(terrain_bonus_applied_to_grid);
    RUN_TEST(terrain_penalty_applied_to_grid);
    RUN_TEST(road_bonus_on_road);
    RUN_TEST(road_bonus_distance_1);
    RUN_TEST(road_bonus_distance_2);
    RUN_TEST(road_bonus_distance_3);
    RUN_TEST(road_bonus_distance_4_no_bonus);
    RUN_TEST(road_bonus_applied_to_grid);
    RUN_TEST(road_bonus_no_road_nearby);
    RUN_TEST(disorder_penalty_zero);
    RUN_TEST(disorder_penalty_max);
    RUN_TEST(disorder_penalty_half);
    RUN_TEST(disorder_penalty_reads_previous_tick);
    RUN_TEST(disorder_penalty_applied_to_grid);
    RUN_TEST(contamination_penalty_zero);
    RUN_TEST(contamination_penalty_max);
    RUN_TEST(contamination_penalty_half);
    RUN_TEST(contamination_penalty_reads_previous_tick);
    RUN_TEST(contamination_penalty_applied_to_grid);
    RUN_TEST(value_clamping_overflow);
    RUN_TEST(value_clamping_underflow);
    RUN_TEST(value_clamping_negative_terrain_bonus);
    RUN_TEST(full_recalculation_all_bonuses);
    RUN_TEST(full_recalculation_all_penalties);
    RUN_TEST(full_recalculation_bonuses_and_penalties);
    RUN_TEST(disorder_reads_previous_tick_not_current);
    RUN_TEST(contamination_reads_previous_tick_not_current);
    RUN_TEST(multi_tick_simulation);
    RUN_TEST(combined_scenario_pristine_area);
    RUN_TEST(combined_scenario_degraded_area);
    RUN_TEST(combined_scenario_urban_core);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? 1 : 0;
}
