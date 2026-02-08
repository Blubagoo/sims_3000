/**
 * @file test_rail_coverage.cpp
 * @brief Unit tests for rail terminal coverage model (Epic 7, Ticket E7-035)
 *
 * Tests:
 * - is_in_terminal_coverage: basic coverage check
 * - get_traffic_reduction_at: linear falloff from 50% at terminal to 0% at edge
 * - Multiple terminals coverage overlap (max reduction)
 * - Inactive terminal has no coverage
 * - Invalid owner returns false/0
 * - Edge of coverage boundary
 */

#include <sims3000/transport/RailSystem.h>
#include <sims3000/building/ForwardDependencyInterfaces.h>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cmath>

using namespace sims3000::transport;
using namespace sims3000::building;

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
        printf("\n  FAILED: %s == %s (line %d, got %d vs %d)\n", \
               #a, #b, __LINE__, (int)(a), (int)(b)); \
        tests_failed++; \
        return; \
    } \
} while(0)

// =============================================================================
// Mock energy provider
// =============================================================================

class MockEnergyProvider : public IEnergyProvider {
public:
    bool default_powered = true;

    bool is_powered(uint32_t /*entity_id*/) const override {
        return default_powered;
    }

    bool is_powered_at(uint32_t /*x*/, uint32_t /*y*/, uint32_t /*player_id*/) const override {
        return default_powered;
    }
};

// =============================================================================
// Helper to set up a rail system with an active terminal
// =============================================================================

// Place rail + terminal + tick to activate
static uint32_t setup_active_terminal(RailSystem& system, int32_t tx, int32_t ty,
                                       int32_t rx, int32_t ry, uint8_t owner) {
    system.place_rail(rx, ry, RailType::SurfaceRail, owner);
    uint32_t term_id = system.place_terminal(tx, ty, TerminalType::SurfaceStation, owner);
    system.tick(0.0f);  // Activate (no energy provider = all powered)
    return term_id;
}

// =============================================================================
// is_in_terminal_coverage tests
// =============================================================================

TEST(coverage_at_terminal_position) {
    RailSystem system(64, 64);
    uint32_t term_id = setup_active_terminal(system, 10, 10, 9, 10, 0);
    ASSERT(term_id != 0);
    ASSERT(system.is_terminal_active(term_id));

    // Position at terminal itself
    ASSERT(system.is_in_terminal_coverage(10, 10, 0) == true);
}

TEST(coverage_adjacent_to_terminal) {
    RailSystem system(64, 64);
    setup_active_terminal(system, 10, 10, 9, 10, 0);

    // Adjacent positions (Manhattan distance 1)
    ASSERT(system.is_in_terminal_coverage(11, 10, 0) == true);
    ASSERT(system.is_in_terminal_coverage(9, 10, 0) == true);
    ASSERT(system.is_in_terminal_coverage(10, 11, 0) == true);
    ASSERT(system.is_in_terminal_coverage(10, 9, 0) == true);
}

TEST(coverage_at_radius_edge) {
    RailSystem system(64, 64);
    setup_active_terminal(system, 10, 10, 9, 10, 0);

    // Default coverage_radius is 8 (from TerminalComponent)
    // Manhattan distance 8 from (10,10) = (18,10)
    ASSERT(system.is_in_terminal_coverage(18, 10, 0) == true);
    // Manhattan distance 8 from (10,10) = (10,18)
    ASSERT(system.is_in_terminal_coverage(10, 18, 0) == true);
    // Distance 4+4=8
    ASSERT(system.is_in_terminal_coverage(14, 14, 0) == true);
}

TEST(coverage_beyond_radius) {
    RailSystem system(64, 64);
    setup_active_terminal(system, 10, 10, 9, 10, 0);

    // Default coverage_radius is 8
    // Manhattan distance 9 from (10,10) = (19,10)
    ASSERT(system.is_in_terminal_coverage(19, 10, 0) == false);
    // Manhattan distance 9 = (10,19)
    ASSERT(system.is_in_terminal_coverage(10, 19, 0) == false);
}

TEST(coverage_inactive_terminal) {
    RailSystem system(64, 64);
    MockEnergyProvider provider;
    provider.default_powered = false;
    system.set_energy_provider(&provider);

    system.place_rail(9, 10, RailType::SurfaceRail, 0);
    uint32_t term_id = system.place_terminal(10, 10, TerminalType::SurfaceStation, 0);
    ASSERT(term_id != 0);

    system.tick(0.0f);
    ASSERT(system.is_terminal_active(term_id) == false);

    // Inactive terminal should not provide coverage
    ASSERT(system.is_in_terminal_coverage(10, 10, 0) == false);
}

TEST(coverage_invalid_owner) {
    RailSystem system(64, 64);
    setup_active_terminal(system, 10, 10, 9, 10, 0);

    // Invalid owner
    ASSERT(system.is_in_terminal_coverage(10, 10, 5) == false);
    ASSERT(system.is_in_terminal_coverage(10, 10, 255) == false);
}

TEST(coverage_wrong_owner) {
    RailSystem system(64, 64);
    setup_active_terminal(system, 10, 10, 9, 10, 0);

    // Player 1 has no terminals
    ASSERT(system.is_in_terminal_coverage(10, 10, 1) == false);
}

// =============================================================================
// get_traffic_reduction_at tests
// =============================================================================

TEST(reduction_at_terminal_is_50) {
    RailSystem system(64, 64);
    setup_active_terminal(system, 10, 10, 9, 10, 0);

    // At the terminal position, reduction should be 50%
    uint8_t reduction = system.get_traffic_reduction_at(10, 10, 0);
    ASSERT_EQ(reduction, 50);
}

TEST(reduction_at_radius_edge_is_zero) {
    RailSystem system(64, 64);
    setup_active_terminal(system, 10, 10, 9, 10, 0);

    // Default radius = 8
    // At distance 8 (edge): reduction = 50 * (8-8)/8 = 0
    uint8_t reduction = system.get_traffic_reduction_at(18, 10, 0);
    ASSERT_EQ(reduction, 0);
}

TEST(reduction_linear_falloff) {
    RailSystem system(64, 64);
    setup_active_terminal(system, 10, 10, 9, 10, 0);

    // Default radius = 8
    // At distance 0: 50 * (8-0)/8 = 50
    ASSERT_EQ(system.get_traffic_reduction_at(10, 10, 0), 50);

    // At distance 1: 50 * (8-1)/8 = 50 * 7/8 = 43
    ASSERT_EQ(system.get_traffic_reduction_at(11, 10, 0), 43);

    // At distance 2: 50 * (8-2)/8 = 50 * 6/8 = 37
    ASSERT_EQ(system.get_traffic_reduction_at(12, 10, 0), 37);

    // At distance 4: 50 * (8-4)/8 = 50 * 4/8 = 25
    ASSERT_EQ(system.get_traffic_reduction_at(14, 10, 0), 25);

    // At distance 6: 50 * (8-6)/8 = 50 * 2/8 = 12
    ASSERT_EQ(system.get_traffic_reduction_at(16, 10, 0), 12);

    // At distance 7: 50 * (8-7)/8 = 50 * 1/8 = 6
    ASSERT_EQ(system.get_traffic_reduction_at(17, 10, 0), 6);
}

TEST(reduction_beyond_radius_is_zero) {
    RailSystem system(64, 64);
    setup_active_terminal(system, 10, 10, 9, 10, 0);

    // Beyond radius
    ASSERT_EQ(system.get_traffic_reduction_at(19, 10, 0), 0);
    ASSERT_EQ(system.get_traffic_reduction_at(30, 30, 0), 0);
}

TEST(reduction_inactive_terminal_zero) {
    RailSystem system(64, 64);
    MockEnergyProvider provider;
    provider.default_powered = false;
    system.set_energy_provider(&provider);

    system.place_rail(9, 10, RailType::SurfaceRail, 0);
    system.place_terminal(10, 10, TerminalType::SurfaceStation, 0);
    system.tick(0.0f);

    ASSERT_EQ(system.get_traffic_reduction_at(10, 10, 0), 0);
}

TEST(reduction_invalid_owner_zero) {
    RailSystem system(64, 64);
    setup_active_terminal(system, 10, 10, 9, 10, 0);

    ASSERT_EQ(system.get_traffic_reduction_at(10, 10, 5), 0);
}

TEST(reduction_multiple_terminals_max_applies) {
    RailSystem system(64, 64);

    // Terminal A at (10, 10) with rail at (9, 10)
    system.place_rail(9, 10, RailType::SurfaceRail, 0);
    system.place_terminal(10, 10, TerminalType::SurfaceStation, 0);

    // Terminal B at (14, 10) with rail at (15, 10)
    system.place_rail(15, 10, RailType::SurfaceRail, 0);
    system.place_terminal(14, 10, TerminalType::SurfaceStation, 0);

    system.tick(0.0f);

    // Point (12, 10) is distance 2 from A and distance 2 from B
    // Both give: 50 * (8-2)/8 = 37
    uint8_t reduction = system.get_traffic_reduction_at(12, 10, 0);
    ASSERT_EQ(reduction, 37);

    // Point (11, 10) is distance 1 from A and distance 3 from B
    // A gives: 50 * 7/8 = 43, B gives: 50 * 5/8 = 31
    // Max = 43
    reduction = system.get_traffic_reduction_at(11, 10, 0);
    ASSERT_EQ(reduction, 43);
}

// =============================================================================
// Main
// =============================================================================

int main() {
    printf("=== Rail Coverage Tests (Ticket E7-035) ===\n\n");

    // Coverage checks
    RUN_TEST(coverage_at_terminal_position);
    RUN_TEST(coverage_adjacent_to_terminal);
    RUN_TEST(coverage_at_radius_edge);
    RUN_TEST(coverage_beyond_radius);
    RUN_TEST(coverage_inactive_terminal);
    RUN_TEST(coverage_invalid_owner);
    RUN_TEST(coverage_wrong_owner);

    // Traffic reduction
    RUN_TEST(reduction_at_terminal_is_50);
    RUN_TEST(reduction_at_radius_edge_is_zero);
    RUN_TEST(reduction_linear_falloff);
    RUN_TEST(reduction_beyond_radius_is_zero);
    RUN_TEST(reduction_inactive_terminal_zero);
    RUN_TEST(reduction_invalid_owner_zero);
    RUN_TEST(reduction_multiple_terminals_max_applies);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
