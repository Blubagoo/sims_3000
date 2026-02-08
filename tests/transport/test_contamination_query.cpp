/**
 * @file test_contamination_query.cpp
 * @brief Unit tests for ContaminationQuery (Epic 7, Ticket E7-029)
 *
 * Tests:
 * - get_contamination_rate_at with valid pathway and traffic
 * - get_contamination_rate_at with no pathway
 * - get_contamination_rate_at with no traffic data
 * - has_traffic_contamination with congested pathway
 * - has_traffic_contamination with uncongested pathway
 * - has_traffic_contamination with no pathway
 * - Out-of-bounds coordinates
 * - Multiple pathways at different positions
 */

#include <sims3000/transport/ContaminationQuery.h>
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
// get_contamination_rate_at: valid pathway with traffic
// ============================================================================

TEST(rate_valid_pathway_with_traffic) {
    PathwayGrid grid(16, 16);
    grid.set_pathway(5, 5, 42);

    std::unordered_map<uint32_t, TrafficComponent> traffic_map;
    TrafficComponent tc;
    tc.contamination_rate = 9;
    tc.congestion_level = 200;
    traffic_map[42] = tc;

    uint8_t rate = ContaminationQuery::get_contamination_rate_at(5, 5, grid, traffic_map);
    ASSERT_EQ(rate, 9);
}

// ============================================================================
// get_contamination_rate_at: no pathway at position
// ============================================================================

TEST(rate_no_pathway) {
    PathwayGrid grid(16, 16);

    std::unordered_map<uint32_t, TrafficComponent> traffic_map;

    uint8_t rate = ContaminationQuery::get_contamination_rate_at(5, 5, grid, traffic_map);
    ASSERT_EQ(rate, 0);
}

// ============================================================================
// get_contamination_rate_at: pathway exists but no traffic data
// ============================================================================

TEST(rate_no_traffic_data) {
    PathwayGrid grid(16, 16);
    grid.set_pathway(5, 5, 42);

    std::unordered_map<uint32_t, TrafficComponent> traffic_map;
    // No entry for entity 42

    uint8_t rate = ContaminationQuery::get_contamination_rate_at(5, 5, grid, traffic_map);
    ASSERT_EQ(rate, 0);
}

// ============================================================================
// get_contamination_rate_at: zero contamination (low congestion)
// ============================================================================

TEST(rate_zero_contamination) {
    PathwayGrid grid(16, 16);
    grid.set_pathway(5, 5, 42);

    std::unordered_map<uint32_t, TrafficComponent> traffic_map;
    TrafficComponent tc;
    tc.contamination_rate = 0;
    tc.congestion_level = 50;
    traffic_map[42] = tc;

    uint8_t rate = ContaminationQuery::get_contamination_rate_at(5, 5, grid, traffic_map);
    ASSERT_EQ(rate, 0);
}

// ============================================================================
// has_traffic_contamination: congested pathway
// ============================================================================

TEST(has_contamination_congested) {
    PathwayGrid grid(16, 16);
    grid.set_pathway(3, 3, 10);

    std::unordered_map<uint32_t, TrafficComponent> traffic_map;
    TrafficComponent tc;
    tc.congestion_level = 200;
    traffic_map[10] = tc;

    ASSERT(ContaminationQuery::has_traffic_contamination(3, 3, grid, traffic_map));
}

// ============================================================================
// has_traffic_contamination: uncongested pathway
// ============================================================================

TEST(has_contamination_uncongested) {
    PathwayGrid grid(16, 16);
    grid.set_pathway(3, 3, 10);

    std::unordered_map<uint32_t, TrafficComponent> traffic_map;
    TrafficComponent tc;
    tc.congestion_level = 100;
    traffic_map[10] = tc;

    ASSERT(!ContaminationQuery::has_traffic_contamination(3, 3, grid, traffic_map));
}

// ============================================================================
// has_traffic_contamination: exactly at threshold (128)
// ============================================================================

TEST(has_contamination_at_threshold) {
    PathwayGrid grid(16, 16);
    grid.set_pathway(3, 3, 10);

    std::unordered_map<uint32_t, TrafficComponent> traffic_map;
    TrafficComponent tc;
    tc.congestion_level = 128;
    traffic_map[10] = tc;

    // 128 is NOT > 128, so no contamination
    ASSERT(!ContaminationQuery::has_traffic_contamination(3, 3, grid, traffic_map));
}

// ============================================================================
// has_traffic_contamination: just above threshold (129)
// ============================================================================

TEST(has_contamination_above_threshold) {
    PathwayGrid grid(16, 16);
    grid.set_pathway(3, 3, 10);

    std::unordered_map<uint32_t, TrafficComponent> traffic_map;
    TrafficComponent tc;
    tc.congestion_level = 129;
    traffic_map[10] = tc;

    ASSERT(ContaminationQuery::has_traffic_contamination(3, 3, grid, traffic_map));
}

// ============================================================================
// has_traffic_contamination: no pathway
// ============================================================================

TEST(has_contamination_no_pathway) {
    PathwayGrid grid(16, 16);

    std::unordered_map<uint32_t, TrafficComponent> traffic_map;

    ASSERT(!ContaminationQuery::has_traffic_contamination(3, 3, grid, traffic_map));
}

// ============================================================================
// has_traffic_contamination: no traffic data
// ============================================================================

TEST(has_contamination_no_traffic) {
    PathwayGrid grid(16, 16);
    grid.set_pathway(3, 3, 10);

    std::unordered_map<uint32_t, TrafficComponent> traffic_map;

    ASSERT(!ContaminationQuery::has_traffic_contamination(3, 3, grid, traffic_map));
}

// ============================================================================
// Out-of-bounds coordinates
// ============================================================================

TEST(out_of_bounds) {
    PathwayGrid grid(16, 16);

    std::unordered_map<uint32_t, TrafficComponent> traffic_map;

    uint8_t rate = ContaminationQuery::get_contamination_rate_at(-1, -1, grid, traffic_map);
    ASSERT_EQ(rate, 0);

    ASSERT(!ContaminationQuery::has_traffic_contamination(100, 100, grid, traffic_map));
}

// ============================================================================
// Multiple pathways at different positions
// ============================================================================

TEST(multiple_pathways) {
    PathwayGrid grid(16, 16);
    grid.set_pathway(1, 1, 10);
    grid.set_pathway(2, 2, 20);

    std::unordered_map<uint32_t, TrafficComponent> traffic_map;

    TrafficComponent tc1;
    tc1.contamination_rate = 5;
    tc1.congestion_level = 180;
    traffic_map[10] = tc1;

    TrafficComponent tc2;
    tc2.contamination_rate = 0;
    tc2.congestion_level = 50;
    traffic_map[20] = tc2;

    ASSERT_EQ(ContaminationQuery::get_contamination_rate_at(1, 1, grid, traffic_map), 5);
    ASSERT_EQ(ContaminationQuery::get_contamination_rate_at(2, 2, grid, traffic_map), 0);

    ASSERT(ContaminationQuery::has_traffic_contamination(1, 1, grid, traffic_map));
    ASSERT(!ContaminationQuery::has_traffic_contamination(2, 2, grid, traffic_map));
}

// ============================================================================
// Main
// ============================================================================

int main() {
    printf("=== ContaminationQuery Unit Tests (Ticket E7-029) ===\n\n");

    RUN_TEST(rate_valid_pathway_with_traffic);
    RUN_TEST(rate_no_pathway);
    RUN_TEST(rate_no_traffic_data);
    RUN_TEST(rate_zero_contamination);
    RUN_TEST(has_contamination_congested);
    RUN_TEST(has_contamination_uncongested);
    RUN_TEST(has_contamination_at_threshold);
    RUN_TEST(has_contamination_above_threshold);
    RUN_TEST(has_contamination_no_pathway);
    RUN_TEST(has_contamination_no_traffic);
    RUN_TEST(out_of_bounds);
    RUN_TEST(multiple_pathways);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
