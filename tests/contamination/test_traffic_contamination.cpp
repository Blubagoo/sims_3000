/**
 * @file test_traffic_contamination.cpp
 * @brief Unit tests for traffic contamination generation (Ticket E10-085).
 *
 * Tests cover:
 * - Zero congestion: output = 5
 * - Full congestion: output = 50
 * - Mid congestion: proportional
 */

#include <sims3000/contamination/TrafficContamination.h>
#include <sims3000/contamination/ContaminationGrid.h>
#include <sims3000/contamination/ContaminationType.h>
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
        printf("\n  FAILED: %s == %s (%d != %d) (line %d)\n", #a, #b, (int)(a), (int)(b), __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

// =============================================================================
// Zero congestion: output = MIN (5)
// =============================================================================

TEST(zero_congestion_output_5) {
    ContaminationGrid grid(64, 64);
    std::vector<TrafficSource> sources = {
        { 10, 10, 0.0f }
    };

    apply_traffic_contamination(grid, sources);

    ASSERT_EQ(grid.get_level(10, 10), 5);
}

// =============================================================================
// Full congestion: output = MAX (50)
// =============================================================================

TEST(full_congestion_output_50) {
    ContaminationGrid grid(64, 64);
    std::vector<TrafficSource> sources = {
        { 10, 10, 1.0f }
    };

    apply_traffic_contamination(grid, sources);

    ASSERT_EQ(grid.get_level(10, 10), 50);
}

// =============================================================================
// Mid congestion: proportional
// =============================================================================

TEST(half_congestion_proportional) {
    ContaminationGrid grid(64, 64);
    std::vector<TrafficSource> sources = {
        { 10, 10, 0.5f }
    };

    apply_traffic_contamination(grid, sources);

    // lerp(5, 50, 0.5) = 5 + 45 * 0.5 = 5 + 22.5 = 27.5 -> 27
    uint8_t result = grid.get_level(10, 10);
    ASSERT(result >= 27 && result <= 28);
}

TEST(quarter_congestion_proportional) {
    ContaminationGrid grid(64, 64);
    std::vector<TrafficSource> sources = {
        { 10, 10, 0.25f }
    };

    apply_traffic_contamination(grid, sources);

    // lerp(5, 50, 0.25) = 5 + 45 * 0.25 = 5 + 11.25 = 16.25 -> 16
    uint8_t result = grid.get_level(10, 10);
    ASSERT(result >= 16 && result <= 17);
}

TEST(three_quarter_congestion_proportional) {
    ContaminationGrid grid(64, 64);
    std::vector<TrafficSource> sources = {
        { 10, 10, 0.75f }
    };

    apply_traffic_contamination(grid, sources);

    // lerp(5, 50, 0.75) = 5 + 45 * 0.75 = 5 + 33.75 = 38.75 -> 38
    uint8_t result = grid.get_level(10, 10);
    ASSERT(result >= 38 && result <= 39);
}

// =============================================================================
// Contamination type is Traffic
// =============================================================================

TEST(contamination_type_is_traffic) {
    ContaminationGrid grid(64, 64);
    std::vector<TrafficSource> sources = {
        { 10, 10, 0.5f }
    };

    apply_traffic_contamination(grid, sources);

    ASSERT_EQ(grid.get_dominant_type(10, 10),
              static_cast<uint8_t>(ContaminationType::Traffic));
}

// =============================================================================
// Multiple sources
// =============================================================================

TEST(multiple_sources_different_congestion) {
    ContaminationGrid grid(64, 64);
    std::vector<TrafficSource> sources = {
        { 10, 10, 0.0f },   // 5
        { 20, 20, 1.0f },   // 50
        { 30, 30, 0.5f }    // ~27
    };

    apply_traffic_contamination(grid, sources);

    ASSERT_EQ(grid.get_level(10, 10), 5);
    ASSERT_EQ(grid.get_level(20, 20), 50);
    uint8_t mid = grid.get_level(30, 30);
    ASSERT(mid >= 27 && mid <= 28);
}

// =============================================================================
// Edge cases
// =============================================================================

TEST(empty_sources_no_changes) {
    ContaminationGrid grid(64, 64);
    std::vector<TrafficSource> sources;

    apply_traffic_contamination(grid, sources);

    ASSERT_EQ(grid.get_level(0, 0), 0);
    ASSERT_EQ(grid.get_level(32, 32), 0);
}

TEST(constants_correct) {
    ASSERT_EQ(TRAFFIC_CONTAMINATION_MIN, 5);
    ASSERT_EQ(TRAFFIC_CONTAMINATION_MAX, 50);
}

TEST(accumulation_from_same_cell) {
    ContaminationGrid grid(64, 64);
    std::vector<TrafficSource> sources = {
        { 10, 10, 1.0f },   // 50
        { 10, 10, 1.0f }    // 50 more -> 100
    };

    apply_traffic_contamination(grid, sources);

    ASSERT_EQ(grid.get_level(10, 10), 100);
}

// =============================================================================
// Main
// =============================================================================

int main() {
    printf("=== Traffic Contamination Tests (E10-085) ===\n\n");

    RUN_TEST(zero_congestion_output_5);
    RUN_TEST(full_congestion_output_50);
    RUN_TEST(half_congestion_proportional);
    RUN_TEST(quarter_congestion_proportional);
    RUN_TEST(three_quarter_congestion_proportional);
    RUN_TEST(contamination_type_is_traffic);
    RUN_TEST(multiple_sources_different_congestion);
    RUN_TEST(empty_sources_no_changes);
    RUN_TEST(constants_correct);
    RUN_TEST(accumulation_from_same_cell);

    printf("\n=== Results: %d passed, %d failed ===\n",
           tests_passed, tests_failed);

    return tests_failed > 0 ? 1 : 0;
}
