/**
 * @file test_disorder_comprehensive.cpp
 * @brief Comprehensive integration tests for disorder system (E10-123)
 *
 * Tests all disorder modules:
 * - DisorderSpread (4-neighbor spread, threshold, water blocking)
 * - DisorderGeneration (zone configs, occupancy, land value)
 * - LandValueDisorderEffect (land value amplification)
 * - EnforcerSuppression (service integration)
 * - DisorderGrid (double-buffering)
 * - DisorderStats (aggregate queries)
 * - Multi-tick simulation cycle
 */

#include <sims3000/disorder/DisorderSpread.h>
#include <sims3000/disorder/DisorderGeneration.h>
#include <sims3000/disorder/LandValueDisorderEffect.h>
#include <sims3000/disorder/EnforcerSuppression.h>
#include <sims3000/disorder/DisorderGrid.h>
#include <sims3000/disorder/DisorderStats.h>
#include <sims3000/landvalue/LandValueGrid.h>
#include <sims3000/building/ForwardDependencyStubs.h>
#include <cstdio>
#include <cstdlib>
#include <vector>

using namespace sims3000::disorder;
using namespace sims3000::landvalue;
using namespace sims3000::building;

// Test counter
int g_test_count = 0;
int g_passed = 0;

#define TEST_ASSERT(condition, message) \
    do { \
        g_test_count++; \
        if (!(condition)) { \
            printf("FAIL [Test %d]: %s\n", g_test_count, message); \
            exit(1); \
        } else { \
            g_passed++; \
            printf("PASS [Test %d]: %s\n", g_test_count, message); \
        } \
    } while (0)

// ============================================================================
// SPREAD THRESHOLD TESTS
// ============================================================================

void test_spread_threshold_no_spread() {
    DisorderGrid grid(10, 10);
    grid.set_level(5, 5, 50);  // Below threshold (64)

    apply_disorder_spread(grid, nullptr);

    TEST_ASSERT(grid.get_level(5, 4) == 0, "Below threshold: no spread to north");
    TEST_ASSERT(grid.get_level(5, 6) == 0, "Below threshold: no spread to south");
    TEST_ASSERT(grid.get_level(4, 5) == 0, "Below threshold: no spread to west");
    TEST_ASSERT(grid.get_level(6, 5) == 0, "Below threshold: no spread to east");
}

void test_spread_threshold_at_limit() {
    DisorderGrid grid(10, 10);
    grid.set_level(5, 5, 64);  // Exactly at threshold

    apply_disorder_spread(grid, nullptr);

    // Should not spread (> threshold required, not >=)
    TEST_ASSERT(grid.get_level(5, 4) == 0, "At threshold: no spread");
}

void test_spread_threshold_above() {
    DisorderGrid grid(10, 10);
    grid.set_level(5, 5, 80);  // Above threshold

    apply_disorder_spread(grid, nullptr);

    // spread = (80 - 64) / 8 = 2
    TEST_ASSERT(grid.get_level(5, 4) == 2, "Above threshold: spread to north");
    TEST_ASSERT(grid.get_level(5, 6) == 2, "Above threshold: spread to south");
    TEST_ASSERT(grid.get_level(4, 5) == 2, "Above threshold: spread to west");
    TEST_ASSERT(grid.get_level(6, 5) == 2, "Above threshold: spread to east");
}

// ============================================================================
// SPREAD PATTERN TESTS
// ============================================================================

void test_spread_4_neighbors() {
    DisorderGrid grid(10, 10);
    grid.set_level(5, 5, 120);  // (120 - 64) / 8 = 7

    apply_disorder_spread(grid, nullptr);

    TEST_ASSERT(grid.get_level(5, 4) == 7, "Spread north");
    TEST_ASSERT(grid.get_level(5, 6) == 7, "Spread south");
    TEST_ASSERT(grid.get_level(4, 5) == 7, "Spread west");
    TEST_ASSERT(grid.get_level(6, 5) == 7, "Spread east");

    // Diagonal neighbors should not receive spread
    TEST_ASSERT(grid.get_level(4, 4) == 0, "No spread to NW diagonal");
    TEST_ASSERT(grid.get_level(6, 4) == 0, "No spread to NE diagonal");
    TEST_ASSERT(grid.get_level(4, 6) == 0, "No spread to SW diagonal");
    TEST_ASSERT(grid.get_level(6, 6) == 0, "No spread to SE diagonal");
}

void test_spread_source_reduction() {
    DisorderGrid grid(10, 10);
    grid.set_level(5, 5, 120);  // spread = 7, has 4 neighbors

    apply_disorder_spread(grid, nullptr);

    // Source loses spread * 4 neighbors = 7 * 4 = 28
    // 120 - 28 = 92
    TEST_ASSERT(grid.get_level(5, 5) == 92, "Source reduced by spread amount");
}

void test_spread_exact_amounts() {
    DisorderGrid grid(10, 10);
    grid.set_level(5, 5, 192);  // (192 - 64) / 8 = 16

    apply_disorder_spread(grid, nullptr);

    TEST_ASSERT(grid.get_level(5, 4) == 16, "Exact spread calculation north");
    TEST_ASSERT(grid.get_level(5, 6) == 16, "Exact spread calculation south");
    TEST_ASSERT(grid.get_level(4, 5) == 16, "Exact spread calculation west");
    TEST_ASSERT(grid.get_level(6, 5) == 16, "Exact spread calculation east");

    // Source: 192 - (16 * 4) = 128
    TEST_ASSERT(grid.get_level(5, 5) == 128, "Exact source reduction");
}

// ============================================================================
// WATER BLOCKING TESTS
// ============================================================================

void test_water_blocks_spread() {
    DisorderGrid grid(10, 10);
    grid.set_level(5, 5, 120);  // spread = 7

    // Create water mask with water to the north and east
    std::vector<bool> water_mask(100, false);
    water_mask[4 * 10 + 5] = true;  // North (5, 4)
    water_mask[5 * 10 + 6] = true;  // East (6, 5)

    apply_disorder_spread(grid, &water_mask);

    // Water blocks spread
    TEST_ASSERT(grid.get_level(5, 4) == 0, "Water blocks spread to north");
    TEST_ASSERT(grid.get_level(6, 5) == 0, "Water blocks spread to east");

    // Non-water neighbors receive spread
    TEST_ASSERT(grid.get_level(5, 6) == 7, "Spread to south (no water)");
    TEST_ASSERT(grid.get_level(4, 5) == 7, "Spread to west (no water)");

    // Source loses spread * 2 valid neighbors only
    // 120 - (7 * 2) = 106
    TEST_ASSERT(grid.get_level(5, 5) == 106, "Source reduced only by valid neighbors");
}

void test_water_all_sides() {
    DisorderGrid grid(10, 10);
    grid.set_level(5, 5, 120);

    // Surround with water
    std::vector<bool> water_mask(100, false);
    water_mask[4 * 10 + 5] = true;  // North
    water_mask[6 * 10 + 5] = true;  // South
    water_mask[5 * 10 + 4] = true;  // West
    water_mask[5 * 10 + 6] = true;  // East

    apply_disorder_spread(grid, &water_mask);

    // No spread anywhere
    TEST_ASSERT(grid.get_level(5, 4) == 0, "No spread (water)");
    TEST_ASSERT(grid.get_level(5, 6) == 0, "No spread (water)");
    TEST_ASSERT(grid.get_level(4, 5) == 0, "No spread (water)");
    TEST_ASSERT(grid.get_level(6, 5) == 0, "No spread (water)");

    // Source unchanged (no valid neighbors)
    TEST_ASSERT(grid.get_level(5, 5) == 120, "Source unchanged (no valid neighbors)");
}

// ============================================================================
// DISORDER GENERATION TESTS
// ============================================================================

void test_generation_base_amount() {
    DisorderGrid grid(10, 10);

    // hab_low: base=2, pop_mult=0.5, landvalue_mod=0.3
    DisorderSource source;
    source.x = 5;
    source.y = 5;
    source.zone_type = 0;  // hab_low
    source.occupancy_ratio = 0.0f;  // No occupancy bonus
    source.land_value = 255;  // Max land value (no land value penalty)

    apply_disorder_generation(grid, source);

    // Should be base only = 2
    TEST_ASSERT(grid.get_level(5, 5) == 2, "Base generation without modifiers");
}

void test_generation_occupancy_modifier() {
    DisorderGrid grid(10, 10);

    DisorderSource source;
    source.x = 5;
    source.y = 5;
    source.zone_type = 1;  // hab_high: base=5, pop_mult=0.8
    source.occupancy_ratio = 1.0f;  // Full occupancy
    source.land_value = 255;  // No land value penalty

    apply_disorder_generation(grid, source);

    // base=5 + (5 * 0.8 * 1.0) = 5 + 4 = 9
    TEST_ASSERT(grid.get_level(5, 5) == 9, "Occupancy modifier applied");
}

void test_generation_land_value_modifier() {
    DisorderGrid grid(10, 10);

    DisorderSource source;
    source.x = 5;
    source.y = 5;
    source.zone_type = 2;  // exchange_low: base=3, landvalue_mod=0.2
    source.occupancy_ratio = 0.0f;
    source.land_value = 0;  // Minimum land value (max penalty)

    apply_disorder_generation(grid, source);

    // base=3, land_value_mod = 0.2 * (1.0 - 0/255) = 0.2
    // generation = 3 + (3 * 0.2) = 3 + 0.6 = 3.6 -> 3
    TEST_ASSERT(grid.get_level(5, 5) >= 3, "Land value modifier applied");
}

void test_generation_zone_configs() {
    DisorderGrid grid_hab_low(10, 10);
    DisorderGrid grid_hab_high(10, 10);
    DisorderGrid grid_exc_low(10, 10);
    DisorderGrid grid_exc_high(10, 10);
    DisorderGrid grid_fab(10, 10);

    DisorderSource source;
    source.x = 5;
    source.y = 5;
    source.occupancy_ratio = 0.0f;
    source.land_value = 255;

    // Test each zone type
    source.zone_type = 0;
    apply_disorder_generation(grid_hab_low, source);
    TEST_ASSERT(grid_hab_low.get_level(5, 5) == 2, "hab_low base generation");

    source.zone_type = 1;
    apply_disorder_generation(grid_hab_high, source);
    TEST_ASSERT(grid_hab_high.get_level(5, 5) == 5, "hab_high base generation");

    source.zone_type = 2;
    apply_disorder_generation(grid_exc_low, source);
    TEST_ASSERT(grid_exc_low.get_level(5, 5) == 3, "exchange_low base generation");

    source.zone_type = 3;
    apply_disorder_generation(grid_exc_high, source);
    TEST_ASSERT(grid_exc_high.get_level(5, 5) == 6, "exchange_high base generation");

    source.zone_type = 4;
    apply_disorder_generation(grid_fab, source);
    TEST_ASSERT(grid_fab.get_level(5, 5) == 1, "fabrication base generation");
}

// ============================================================================
// LAND VALUE EFFECT TESTS
// ============================================================================

void test_land_value_low_amplifies() {
    DisorderGrid dis_grid(10, 10);
    LandValueGrid lv_grid(10, 10);

    dis_grid.set_level(5, 5, 100);
    lv_grid.set_value(5, 5, 0);  // Minimum land value

    apply_land_value_effect(dis_grid, lv_grid);

    // extra = 100 * (1.0 - 0/255) = 100
    // new = 100 + 100 = 200
    TEST_ASSERT(dis_grid.get_level(5, 5) == 200, "Low land value doubles disorder");
}

void test_land_value_high_no_change() {
    DisorderGrid dis_grid(10, 10);
    LandValueGrid lv_grid(10, 10);

    dis_grid.set_level(5, 5, 100);
    lv_grid.set_value(5, 5, 255);  // Maximum land value

    apply_land_value_effect(dis_grid, lv_grid);

    // extra = 100 * (1.0 - 255/255) = 0
    // new = 100 + 0 = 100
    TEST_ASSERT(dis_grid.get_level(5, 5) == 100, "High land value no additional disorder");
}

void test_land_value_mid_range() {
    DisorderGrid dis_grid(10, 10);
    LandValueGrid lv_grid(10, 10);

    dis_grid.set_level(5, 5, 100);
    lv_grid.set_value(5, 5, 127);  // Mid land value (~50%)

    apply_land_value_effect(dis_grid, lv_grid);

    // extra = 100 * (1.0 - 127/255) ~= 50
    // new ~= 150
    uint8_t level = dis_grid.get_level(5, 5);
    TEST_ASSERT(level >= 145 && level <= 155, "Mid land value ~50% increase");
}

void test_land_value_saturation() {
    DisorderGrid dis_grid(10, 10);
    LandValueGrid lv_grid(10, 10);

    dis_grid.set_level(5, 5, 200);
    lv_grid.set_value(5, 5, 0);  // Would double to 400, but saturates at 255

    apply_land_value_effect(dis_grid, lv_grid);

    TEST_ASSERT(dis_grid.get_level(5, 5) == 255, "Disorder saturates at 255");
}

// ============================================================================
// ENFORCER SUPPRESSION TESTS
// ============================================================================

void test_enforcer_no_coverage() {
    DisorderGrid grid(10, 10);
    grid.set_level(5, 5, 100);

    StubServiceQueryable service_stub;
    // Default stub returns 0 coverage

    apply_enforcer_suppression(grid, service_stub, 0);

    // No coverage = no suppression
    TEST_ASSERT(grid.get_level(5, 5) == 100, "No coverage = no suppression");
}

void test_enforcer_with_coverage() {
    DisorderGrid grid(10, 10);
    grid.set_level(5, 5, 100);

    // Create a custom stub that returns coverage
    class TestServiceQueryable : public IServiceQueryable {
    public:
        float get_coverage(uint8_t, uint8_t) const override { return 0.0f; }
        float get_coverage_at(uint8_t, int32_t x, int32_t y, uint8_t) const override {
            if (x == 5 && y == 5) return 0.5f;  // 50% coverage
            return 0.0f;
        }
        float get_effectiveness(uint8_t, uint8_t) const override { return 1.0f; }
    };

    TestServiceQueryable service_query;
    apply_enforcer_suppression(grid, service_query, 0);

    // suppression = 100 * 0.5 * 1.0 * 0.7 = 35
    // new = 100 - 35 = 65
    TEST_ASSERT(grid.get_level(5, 5) == 65, "Enforcer suppression applied");
}

void test_enforcer_effectiveness_modifier() {
    DisorderGrid grid(10, 10);
    grid.set_level(5, 5, 100);

    class TestServiceQueryable : public IServiceQueryable {
    public:
        float get_coverage(uint8_t, uint8_t) const override { return 0.0f; }
        float get_coverage_at(uint8_t, int32_t x, int32_t y, uint8_t) const override {
            if (x == 5 && y == 5) return 1.0f;  // Full coverage
            return 0.0f;
        }
        float get_effectiveness(uint8_t, uint8_t) const override { return 0.5f; }  // 50% effective
    };

    TestServiceQueryable service_query;
    apply_enforcer_suppression(grid, service_query, 0);

    // suppression = 100 * 1.0 * 0.5 * 0.7 = 35
    // new = 100 - 35 = 65
    TEST_ASSERT(grid.get_level(5, 5) == 65, "Effectiveness modifier applied");
}

// ============================================================================
// DOUBLE-BUFFER TESTS
// ============================================================================

void test_double_buffer_read_write() {
    DisorderGrid grid(10, 10);

    // Write to current buffer
    grid.set_level(5, 5, 100);
    TEST_ASSERT(grid.get_level(5, 5) == 100, "Write to current buffer");
    TEST_ASSERT(grid.get_level_previous_tick(5, 5) == 0, "Previous buffer empty initially");

    // Swap buffers
    grid.swap_buffers();

    TEST_ASSERT(grid.get_level(5, 5) == 0, "Current buffer reset after swap");
    TEST_ASSERT(grid.get_level_previous_tick(5, 5) == 100, "Previous buffer has old data");
}

void test_double_buffer_isolation() {
    DisorderGrid grid(10, 10);

    grid.set_level(5, 5, 50);
    grid.swap_buffers();

    // Write to current shouldn't affect previous
    grid.set_level(5, 5, 100);

    TEST_ASSERT(grid.get_level(5, 5) == 100, "Current buffer updated");
    TEST_ASSERT(grid.get_level_previous_tick(5, 5) == 50, "Previous buffer unchanged");
}

// ============================================================================
// AGGREGATE STATS TESTS
// ============================================================================

void test_stats_total_disorder() {
    DisorderGrid grid(10, 10);

    grid.set_level(0, 0, 10);
    grid.set_level(1, 1, 20);
    grid.set_level(2, 2, 30);
    grid.update_stats();

    uint32_t total = grid.get_total_disorder();
    TEST_ASSERT(total == 60, "Total disorder aggregated correctly");
}

void test_stats_high_disorder_tiles() {
    DisorderGrid grid(10, 10);

    grid.set_level(0, 0, 100);
    grid.set_level(1, 1, 150);
    grid.set_level(2, 2, 200);
    grid.set_level(3, 3, 50);   // Below threshold
    grid.update_stats();

    uint32_t high_tiles = grid.get_high_disorder_tiles(128);
    TEST_ASSERT(high_tiles == 2, "High disorder tiles counted correctly (>= 128)");
}

void test_stats_query_functions() {
    DisorderGrid grid(10, 10);

    grid.set_level(5, 5, 100);
    grid.update_stats();

    float total = get_disorder_stat(grid, STAT_TOTAL_DISORDER);
    TEST_ASSERT(total == 100.0f, "Stat query: total disorder");

    uint8_t at_pos = get_disorder_at(grid, 5, 5);
    TEST_ASSERT(at_pos == 100, "Stat query: disorder at position");

    const char* name = get_disorder_stat_name(STAT_TOTAL_DISORDER);
    TEST_ASSERT(name != nullptr, "Stat query: stat name valid");
}

// ============================================================================
// MULTI-TICK SIMULATION TEST
// ============================================================================

void test_multi_tick_simulation() {
    DisorderGrid grid(20, 20);
    LandValueGrid lv_grid(20, 20);

    // Setup: Low land value area with a disorder source
    for (int y = 8; y < 12; y++) {
        for (int x = 8; x < 12; x++) {
            lv_grid.set_value(x, y, 50);  // Low land value
        }
    }

    // Tick 1: Generate disorder
    DisorderSource source;
    source.x = 10;
    source.y = 10;
    source.zone_type = 1;  // hab_high
    source.occupancy_ratio = 1.0f;
    source.land_value = 50;

    apply_disorder_generation(grid, source);
    uint8_t initial = grid.get_level(10, 10);
    TEST_ASSERT(initial > 0, "Multi-tick: initial generation");

    // Tick 2: Apply land value effect
    apply_land_value_effect(grid, lv_grid);
    uint8_t after_lv = grid.get_level(10, 10);
    TEST_ASSERT(after_lv > initial, "Multi-tick: land value amplifies");

    // Tick 3: Spread (if above threshold)
    grid.swap_buffers();  // Prepare for next tick
    grid.set_level(10, 10, 150);  // Set high disorder
    apply_disorder_spread(grid, nullptr);

    uint8_t neighbor = grid.get_level(10, 9);
    TEST_ASSERT(neighbor > 0, "Multi-tick: spread occurs");

    // Tick 4: Suppress with enforcer
    class TestServiceQueryable : public IServiceQueryable {
    public:
        float get_coverage(uint8_t, uint8_t) const override { return 0.0f; }
        float get_coverage_at(uint8_t, int32_t, int32_t, uint8_t) const override {
            return 0.8f;  // High coverage
        }
        float get_effectiveness(uint8_t, uint8_t) const override { return 1.0f; }
    };

    TestServiceQueryable service_query;
    uint8_t before_suppress = grid.get_level(10, 10);
    apply_enforcer_suppression(grid, service_query, 0);
    uint8_t after_suppress = grid.get_level(10, 10);
    TEST_ASSERT(after_suppress < before_suppress, "Multi-tick: suppression reduces disorder");

    // Tick 5: Update and check stats
    grid.update_stats();
    uint32_t total = grid.get_total_disorder();
    TEST_ASSERT(total > 0, "Multi-tick: aggregate stats valid");
}

// ============================================================================
// MAIN
// ============================================================================

int main() {
    printf("=== Comprehensive Disorder System Tests (E10-123) ===\n\n");

    printf("--- Spread Threshold Tests ---\n");
    test_spread_threshold_no_spread();
    test_spread_threshold_at_limit();
    test_spread_threshold_above();

    printf("\n--- Spread Pattern Tests ---\n");
    test_spread_4_neighbors();
    test_spread_source_reduction();
    test_spread_exact_amounts();

    printf("\n--- Water Blocking Tests ---\n");
    test_water_blocks_spread();
    test_water_all_sides();

    printf("\n--- Disorder Generation Tests ---\n");
    test_generation_base_amount();
    test_generation_occupancy_modifier();
    test_generation_land_value_modifier();
    test_generation_zone_configs();

    printf("\n--- Land Value Effect Tests ---\n");
    test_land_value_low_amplifies();
    test_land_value_high_no_change();
    test_land_value_mid_range();
    test_land_value_saturation();

    printf("\n--- Enforcer Suppression Tests ---\n");
    test_enforcer_no_coverage();
    test_enforcer_with_coverage();
    test_enforcer_effectiveness_modifier();

    printf("\n--- Double-Buffer Tests ---\n");
    test_double_buffer_read_write();
    test_double_buffer_isolation();

    printf("\n--- Aggregate Stats Tests ---\n");
    test_stats_total_disorder();
    test_stats_high_disorder_tiles();
    test_stats_query_functions();

    printf("\n--- Multi-Tick Simulation Test ---\n");
    test_multi_tick_simulation();

    printf("\n=== All %d tests passed ===\n", g_test_count);
    return 0;
}
