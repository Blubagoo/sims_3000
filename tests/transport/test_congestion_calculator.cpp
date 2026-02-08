/**
 * @file test_congestion_calculator.cpp
 * @brief Unit tests for CongestionCalculator (Epic 7, Ticket E7-015)
 *
 * Tests:
 * - Congestion level calculation (0-255)
 * - Zero capacity handling
 * - Overflow capping at 255
 * - update_congestion integration with TrafficComponent/RoadComponent
 * - Blockage tick increment and reset
 * - Blockage tick cap at 255
 * - Contamination rate formula
 * - Contamination rate boundary at 128
 * - Sector value penalty percentages
 * - Custom TrafficBalanceConfig penalties
 */

#include <sims3000/transport/CongestionCalculator.h>
#include <cassert>
#include <cstdio>
#include <cstdlib>

using namespace sims3000::transport;

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
        printf("\n  FAILED: %s == %s (line %d, got %lld vs %lld)\n", \
               #a, #b, __LINE__, (long long)(a), (long long)(b)); \
        tests_failed++; \
        return; \
    } \
} while(0)

// ============================================================================
// Congestion level: zero flow
// ============================================================================

TEST(congestion_zero_flow) {
    uint8_t level = CongestionCalculator::calculate_congestion(0, 100);
    ASSERT_EQ(level, 0);
}

// ============================================================================
// Congestion level: half capacity
// ============================================================================

TEST(congestion_half_capacity) {
    // (50 * 255) / 100 = 127
    uint8_t level = CongestionCalculator::calculate_congestion(50, 100);
    ASSERT_EQ(level, 127);
}

// ============================================================================
// Congestion level: at capacity
// ============================================================================

TEST(congestion_at_capacity) {
    // (100 * 255) / 100 = 255
    uint8_t level = CongestionCalculator::calculate_congestion(100, 100);
    ASSERT_EQ(level, 255);
}

// ============================================================================
// Congestion level: over capacity capped at 255
// ============================================================================

TEST(congestion_over_capacity_capped) {
    // (200 * 255) / 100 = 510, capped at 255
    uint8_t level = CongestionCalculator::calculate_congestion(200, 100);
    ASSERT_EQ(level, 255);
}

// ============================================================================
// Congestion level: zero capacity defaults to 1
// ============================================================================

TEST(congestion_zero_capacity) {
    // (10 * 255) / max(0, 1) = 2550, capped at 255
    uint8_t level = CongestionCalculator::calculate_congestion(10, 0);
    ASSERT_EQ(level, 255);
}

// ============================================================================
// Congestion level: zero flow zero capacity
// ============================================================================

TEST(congestion_zero_flow_zero_capacity) {
    uint8_t level = CongestionCalculator::calculate_congestion(0, 0);
    ASSERT_EQ(level, 0);
}

// ============================================================================
// Update congestion: sets congestion and contamination fields
// ============================================================================

TEST(update_congestion_sets_fields) {
    TrafficComponent traffic;
    traffic.flow_current = 80;

    RoadComponent road;
    road.current_capacity = 100;

    CongestionCalculator::update_congestion(traffic, road);

    // (80 * 255) / 100 = 204
    ASSERT_EQ(traffic.congestion_level, 204);
    // contamination: (204 - 128) / 8 = 9
    ASSERT_EQ(traffic.contamination_rate, 9);
}

// ============================================================================
// Update congestion: low flow produces no contamination
// ============================================================================

TEST(update_congestion_low_flow_no_contamination) {
    TrafficComponent traffic;
    traffic.flow_current = 20;

    RoadComponent road;
    road.current_capacity = 100;

    CongestionCalculator::update_congestion(traffic, road);

    // (20 * 255) / 100 = 51
    ASSERT_EQ(traffic.congestion_level, 51);
    // 51 <= 128 -> contamination = 0
    ASSERT_EQ(traffic.contamination_rate, 0);
}

// ============================================================================
// Blockage ticks: increment when above threshold
// ============================================================================

TEST(blockage_ticks_increment) {
    TrafficComponent traffic;
    traffic.congestion_level = 210;
    traffic.flow_blockage_ticks = 0;

    CongestionCalculator::update_blockage_ticks(traffic, 200);
    ASSERT_EQ(traffic.flow_blockage_ticks, 1);

    CongestionCalculator::update_blockage_ticks(traffic, 200);
    ASSERT_EQ(traffic.flow_blockage_ticks, 2);
}

// ============================================================================
// Blockage ticks: reset when below threshold
// ============================================================================

TEST(blockage_ticks_reset) {
    TrafficComponent traffic;
    traffic.congestion_level = 210;
    traffic.flow_blockage_ticks = 5;

    // Drop below threshold
    traffic.congestion_level = 150;
    CongestionCalculator::update_blockage_ticks(traffic, 200);
    ASSERT_EQ(traffic.flow_blockage_ticks, 0);
}

// ============================================================================
// Blockage ticks: capped at 255
// ============================================================================

TEST(blockage_ticks_capped) {
    TrafficComponent traffic;
    traffic.congestion_level = 255;
    traffic.flow_blockage_ticks = 254;

    CongestionCalculator::update_blockage_ticks(traffic, 200);
    ASSERT_EQ(traffic.flow_blockage_ticks, 255);

    // Should not exceed 255
    CongestionCalculator::update_blockage_ticks(traffic, 200);
    ASSERT_EQ(traffic.flow_blockage_ticks, 255);
}

// ============================================================================
// Blockage ticks: exactly at threshold (not above)
// ============================================================================

TEST(blockage_ticks_at_threshold) {
    TrafficComponent traffic;
    traffic.congestion_level = 200;
    traffic.flow_blockage_ticks = 3;

    // At threshold (not above) -> reset
    CongestionCalculator::update_blockage_ticks(traffic, 200);
    ASSERT_EQ(traffic.flow_blockage_ticks, 0);
}

// ============================================================================
// Contamination rate: below threshold
// ============================================================================

TEST(contamination_below_threshold) {
    ASSERT_EQ(CongestionCalculator::calculate_contamination_rate(0), 0);
    ASSERT_EQ(CongestionCalculator::calculate_contamination_rate(64), 0);
    ASSERT_EQ(CongestionCalculator::calculate_contamination_rate(128), 0);
}

// ============================================================================
// Contamination rate: above threshold
// ============================================================================

TEST(contamination_above_threshold) {
    // (129 - 128) / 8 = 0 (integer division)
    ASSERT_EQ(CongestionCalculator::calculate_contamination_rate(129), 0);
    // (136 - 128) / 8 = 1
    ASSERT_EQ(CongestionCalculator::calculate_contamination_rate(136), 1);
    // (200 - 128) / 8 = 9
    ASSERT_EQ(CongestionCalculator::calculate_contamination_rate(200), 9);
    // (255 - 128) / 8 = 15
    ASSERT_EQ(CongestionCalculator::calculate_contamination_rate(255), 15);
}

// ============================================================================
// Penalty: free flow (no penalty)
// ============================================================================

TEST(penalty_free_flow) {
    ASSERT_EQ(CongestionCalculator::get_penalty_percent(0), 0);
    ASSERT_EQ(CongestionCalculator::get_penalty_percent(50), 0);
}

// ============================================================================
// Penalty: light congestion
// ============================================================================

TEST(penalty_light) {
    ASSERT_EQ(CongestionCalculator::get_penalty_percent(51), 5);
    ASSERT_EQ(CongestionCalculator::get_penalty_percent(100), 5);
}

// ============================================================================
// Penalty: moderate congestion
// ============================================================================

TEST(penalty_moderate) {
    ASSERT_EQ(CongestionCalculator::get_penalty_percent(101), 10);
    ASSERT_EQ(CongestionCalculator::get_penalty_percent(150), 10);
}

// ============================================================================
// Penalty: heavy congestion
// ============================================================================

TEST(penalty_heavy) {
    ASSERT_EQ(CongestionCalculator::get_penalty_percent(151), 15);
    ASSERT_EQ(CongestionCalculator::get_penalty_percent(200), 15);
}

// ============================================================================
// Penalty: blockage level (above heavy)
// ============================================================================

TEST(penalty_blockage) {
    ASSERT_EQ(CongestionCalculator::get_penalty_percent(201), 15);
    ASSERT_EQ(CongestionCalculator::get_penalty_percent(255), 15);
}

// ============================================================================
// Penalty: custom config
// ============================================================================

TEST(penalty_custom_config) {
    TrafficBalanceConfig config;
    config.free_flow_max = 30;
    config.light_max = 80;
    config.moderate_max = 160;
    config.heavy_max = 220;
    config.light_penalty_pct = 3;
    config.moderate_penalty_pct = 8;
    config.heavy_penalty_pct = 20;

    ASSERT_EQ(CongestionCalculator::get_penalty_percent(30, config), 0);
    ASSERT_EQ(CongestionCalculator::get_penalty_percent(31, config), 3);
    ASSERT_EQ(CongestionCalculator::get_penalty_percent(81, config), 8);
    ASSERT_EQ(CongestionCalculator::get_penalty_percent(161, config), 20);
    ASSERT_EQ(CongestionCalculator::get_penalty_percent(221, config), 20);
}

// ============================================================================
// Main
// ============================================================================

int main() {
    printf("=== CongestionCalculator Unit Tests (Ticket E7-015) ===\n\n");

    RUN_TEST(congestion_zero_flow);
    RUN_TEST(congestion_half_capacity);
    RUN_TEST(congestion_at_capacity);
    RUN_TEST(congestion_over_capacity_capped);
    RUN_TEST(congestion_zero_capacity);
    RUN_TEST(congestion_zero_flow_zero_capacity);
    RUN_TEST(update_congestion_sets_fields);
    RUN_TEST(update_congestion_low_flow_no_contamination);
    RUN_TEST(blockage_ticks_increment);
    RUN_TEST(blockage_ticks_reset);
    RUN_TEST(blockage_ticks_capped);
    RUN_TEST(blockage_ticks_at_threshold);
    RUN_TEST(contamination_below_threshold);
    RUN_TEST(contamination_above_threshold);
    RUN_TEST(penalty_free_flow);
    RUN_TEST(penalty_light);
    RUN_TEST(penalty_moderate);
    RUN_TEST(penalty_heavy);
    RUN_TEST(penalty_blockage);
    RUN_TEST(penalty_custom_config);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
