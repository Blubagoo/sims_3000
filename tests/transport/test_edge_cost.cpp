/**
 * @file test_edge_cost.cpp
 * @brief Unit tests for EdgeCost function (Epic 7, Ticket E7-024)
 *
 * Tests:
 * - Base cost per pathway type
 * - Congestion penalty scaling
 * - Decay penalty scaling
 * - Combined cost calculation
 * - Custom config values
 * - Edge cases (zero congestion, full health, zero health)
 */

#include <sims3000/transport/EdgeCost.h>
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
// Base cost: BasicPathway with no penalties
// ============================================================================

TEST(base_cost_basic_pathway) {
    uint32_t cost = calculate_edge_cost(PathwayType::BasicPathway, 0, 255);
    ASSERT_EQ(cost, 15u);
}

// ============================================================================
// Base cost: TransitCorridor
// ============================================================================

TEST(base_cost_transit_corridor) {
    uint32_t cost = calculate_edge_cost(PathwayType::TransitCorridor, 0, 255);
    ASSERT_EQ(cost, 5u);
}

// ============================================================================
// Base cost: Pedestrian
// ============================================================================

TEST(base_cost_pedestrian) {
    uint32_t cost = calculate_edge_cost(PathwayType::Pedestrian, 0, 255);
    ASSERT_EQ(cost, 20u);
}

// ============================================================================
// Base cost: Bridge
// ============================================================================

TEST(base_cost_bridge) {
    uint32_t cost = calculate_edge_cost(PathwayType::Bridge, 0, 255);
    ASSERT_EQ(cost, 10u);
}

// ============================================================================
// Base cost: Tunnel
// ============================================================================

TEST(base_cost_tunnel) {
    uint32_t cost = calculate_edge_cost(PathwayType::Tunnel, 0, 255);
    ASSERT_EQ(cost, 10u);
}

// ============================================================================
// Congestion penalty: max congestion
// ============================================================================

TEST(congestion_penalty_max) {
    // congestion=255, health=255 (no decay)
    // congestion_penalty = (255 * 10) / 255 = 10
    uint32_t cost = calculate_edge_cost(PathwayType::BasicPathway, 255, 255);
    ASSERT_EQ(cost, 15u + 10u);
}

// ============================================================================
// Congestion penalty: half congestion
// ============================================================================

TEST(congestion_penalty_half) {
    // congestion=127, health=255
    // congestion_penalty = (127 * 10) / 255 = 4 (integer division)
    uint32_t cost = calculate_edge_cost(PathwayType::BasicPathway, 127, 255);
    ASSERT_EQ(cost, 15u + 4u);
}

// ============================================================================
// Decay penalty: zero health (maximum decay)
// ============================================================================

TEST(decay_penalty_max) {
    // congestion=0, health=0
    // missing_health = 255
    // decay_penalty = (255 * 5) / 255 = 5
    uint32_t cost = calculate_edge_cost(PathwayType::BasicPathway, 0, 0);
    ASSERT_EQ(cost, 15u + 5u);
}

// ============================================================================
// Decay penalty: half health
// ============================================================================

TEST(decay_penalty_half) {
    // congestion=0, health=128
    // missing_health = 127
    // decay_penalty = (127 * 5) / 255 = 2 (integer division)
    uint32_t cost = calculate_edge_cost(PathwayType::BasicPathway, 0, 128);
    ASSERT_EQ(cost, 15u + 2u);
}

// ============================================================================
// Combined: congestion + decay
// ============================================================================

TEST(combined_penalties) {
    // congestion=255, health=0
    // congestion_penalty = (255 * 10) / 255 = 10
    // decay_penalty = (255 * 5) / 255 = 5
    uint32_t cost = calculate_edge_cost(PathwayType::BasicPathway, 255, 0);
    ASSERT_EQ(cost, 15u + 10u + 5u);
}

// ============================================================================
// TransitCorridor with max penalties
// ============================================================================

TEST(transit_max_penalties) {
    uint32_t cost = calculate_edge_cost(PathwayType::TransitCorridor, 255, 0);
    ASSERT_EQ(cost, 5u + 10u + 5u);
}

// ============================================================================
// Custom config: different base costs
// ============================================================================

TEST(custom_config_base_costs) {
    EdgeCostConfig config;
    config.basic_cost = 30;
    config.transit_cost = 10;

    uint32_t cost = calculate_edge_cost(PathwayType::BasicPathway, 0, 255, config);
    ASSERT_EQ(cost, 30u);

    cost = calculate_edge_cost(PathwayType::TransitCorridor, 0, 255, config);
    ASSERT_EQ(cost, 10u);
}

// ============================================================================
// Custom config: different penalty ranges
// ============================================================================

TEST(custom_config_penalties) {
    EdgeCostConfig config;
    config.basic_cost = 10;
    config.max_congestion_penalty = 20;
    config.max_decay_penalty = 10;

    // congestion=255, health=0
    // congestion_penalty = (255 * 20) / 255 = 20
    // decay_penalty = (255 * 10) / 255 = 10
    uint32_t cost = calculate_edge_cost(PathwayType::BasicPathway, 255, 0, config);
    ASSERT_EQ(cost, 10u + 20u + 10u);
}

// ============================================================================
// Zero penalties (pristine, no congestion)
// ============================================================================

TEST(zero_penalties) {
    // For all types: congestion=0, health=255 -> no penalties
    ASSERT_EQ(calculate_edge_cost(PathwayType::BasicPathway, 0, 255), 15u);
    ASSERT_EQ(calculate_edge_cost(PathwayType::TransitCorridor, 0, 255), 5u);
    ASSERT_EQ(calculate_edge_cost(PathwayType::Pedestrian, 0, 255), 20u);
    ASSERT_EQ(calculate_edge_cost(PathwayType::Bridge, 0, 255), 10u);
    ASSERT_EQ(calculate_edge_cost(PathwayType::Tunnel, 0, 255), 10u);
}

// ============================================================================
// Main
// ============================================================================

int main() {
    printf("=== EdgeCost Unit Tests (Ticket E7-024) ===\n\n");

    RUN_TEST(base_cost_basic_pathway);
    RUN_TEST(base_cost_transit_corridor);
    RUN_TEST(base_cost_pedestrian);
    RUN_TEST(base_cost_bridge);
    RUN_TEST(base_cost_tunnel);
    RUN_TEST(congestion_penalty_max);
    RUN_TEST(congestion_penalty_half);
    RUN_TEST(decay_penalty_max);
    RUN_TEST(decay_penalty_half);
    RUN_TEST(combined_penalties);
    RUN_TEST(transit_max_penalties);
    RUN_TEST(custom_config_base_costs);
    RUN_TEST(custom_config_penalties);
    RUN_TEST(zero_penalties);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
