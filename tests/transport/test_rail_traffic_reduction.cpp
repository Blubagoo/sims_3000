/**
 * @file test_rail_traffic_reduction.cpp
 * @brief Unit tests for rail traffic reduction calculation (Epic 7, Ticket E7-045)
 *
 * Tests cover:
 * - calculate_traffic_reduction: 50% at terminal, linear falloff to 0% at edge
 * - Only active terminals contribute to reduction
 * - Multiple terminals: maximum reduction applies
 * - Invalid owner returns 0
 * - Beyond coverage radius returns 0
 * - Matches get_traffic_reduction_at behavior
 */

#include <sims3000/transport/RailSystem.h>
#include <sims3000/building/ForwardDependencyInterfaces.h>
#include <cstdio>
#include <cstdlib>

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
// Helper: set up rail system with active terminal
// =============================================================================

static uint32_t setup_active_terminal(RailSystem& system, int32_t tx, int32_t ty,
                                       int32_t rx, int32_t ry, uint8_t owner) {
    system.place_rail(rx, ry, RailType::SurfaceRail, owner);
    uint32_t term_id = system.place_terminal(tx, ty, TerminalType::SurfaceStation, owner);
    system.tick(0.0f);
    return term_id;
}

// =============================================================================
// Basic reduction at terminal
// =============================================================================

TEST(reduction_50_at_terminal) {
    RailSystem system(64, 64);
    setup_active_terminal(system, 10, 10, 9, 10, 0);

    uint8_t reduction = system.calculate_traffic_reduction(10, 10, 0);
    ASSERT_EQ(reduction, 50);
}

// =============================================================================
// Distance-based falloff
// =============================================================================

TEST(reduction_linear_falloff_distance_1) {
    RailSystem system(64, 64);
    setup_active_terminal(system, 10, 10, 9, 10, 0);

    // Default radius = 8
    // distance 1: 50 * (8-1)/8 = 50 * 7/8 = 43
    uint8_t reduction = system.calculate_traffic_reduction(11, 10, 0);
    ASSERT_EQ(reduction, 43);
}

TEST(reduction_linear_falloff_distance_2) {
    RailSystem system(64, 64);
    setup_active_terminal(system, 10, 10, 9, 10, 0);

    // distance 2: 50 * (8-2)/8 = 50 * 6/8 = 37
    uint8_t reduction = system.calculate_traffic_reduction(12, 10, 0);
    ASSERT_EQ(reduction, 37);
}

TEST(reduction_linear_falloff_distance_4) {
    RailSystem system(64, 64);
    setup_active_terminal(system, 10, 10, 9, 10, 0);

    // distance 4: 50 * (8-4)/8 = 50 * 4/8 = 25
    uint8_t reduction = system.calculate_traffic_reduction(14, 10, 0);
    ASSERT_EQ(reduction, 25);
}

TEST(reduction_linear_falloff_distance_6) {
    RailSystem system(64, 64);
    setup_active_terminal(system, 10, 10, 9, 10, 0);

    // distance 6: 50 * (8-6)/8 = 50 * 2/8 = 12
    uint8_t reduction = system.calculate_traffic_reduction(16, 10, 0);
    ASSERT_EQ(reduction, 12);
}

TEST(reduction_linear_falloff_distance_7) {
    RailSystem system(64, 64);
    setup_active_terminal(system, 10, 10, 9, 10, 0);

    // distance 7: 50 * (8-7)/8 = 50 * 1/8 = 6
    uint8_t reduction = system.calculate_traffic_reduction(17, 10, 0);
    ASSERT_EQ(reduction, 6);
}

TEST(reduction_zero_at_radius_edge) {
    RailSystem system(64, 64);
    setup_active_terminal(system, 10, 10, 9, 10, 0);

    // distance 8 (radius edge): 50 * (8-8)/8 = 0
    uint8_t reduction = system.calculate_traffic_reduction(18, 10, 0);
    ASSERT_EQ(reduction, 0);
}

TEST(reduction_zero_beyond_radius) {
    RailSystem system(64, 64);
    setup_active_terminal(system, 10, 10, 9, 10, 0);

    // distance 9: beyond radius
    ASSERT_EQ(system.calculate_traffic_reduction(19, 10, 0), 0);
    // Far away
    ASSERT_EQ(system.calculate_traffic_reduction(30, 30, 0), 0);
}

// =============================================================================
// Manhattan distance in both axes
// =============================================================================

TEST(reduction_diagonal_manhattan) {
    RailSystem system(64, 64);
    setup_active_terminal(system, 10, 10, 9, 10, 0);

    // (14, 14): Manhattan distance = |14-10| + |14-10| = 8
    // reduction = 50 * (8-8)/8 = 0
    ASSERT_EQ(system.calculate_traffic_reduction(14, 14, 0), 0);

    // (12, 12): Manhattan distance = 4
    // reduction = 50 * (8-4)/8 = 25
    ASSERT_EQ(system.calculate_traffic_reduction(12, 12, 0), 25);

    // (11, 11): Manhattan distance = 2
    // reduction = 50 * (8-2)/8 = 37
    ASSERT_EQ(system.calculate_traffic_reduction(11, 11, 0), 37);
}

// =============================================================================
// Only active terminals contribute
// =============================================================================

TEST(inactive_terminal_no_reduction) {
    RailSystem system(64, 64);
    MockEnergyProvider provider;
    provider.default_powered = false;
    system.set_energy_provider(&provider);

    system.place_rail(9, 10, RailType::SurfaceRail, 0);
    system.place_terminal(10, 10, TerminalType::SurfaceStation, 0);
    system.tick(0.0f);  // Terminal won't activate (no power)

    ASSERT_EQ(system.calculate_traffic_reduction(10, 10, 0), 0);
}

TEST(active_terminal_provides_reduction) {
    RailSystem system(64, 64);
    MockEnergyProvider provider;
    provider.default_powered = true;
    system.set_energy_provider(&provider);

    system.place_rail(9, 10, RailType::SurfaceRail, 0);
    uint32_t term_id = system.place_terminal(10, 10, TerminalType::SurfaceStation, 0);
    system.tick(0.0f);

    ASSERT(system.is_terminal_active(term_id));
    ASSERT_EQ(system.calculate_traffic_reduction(10, 10, 0), 50);
}

// =============================================================================
// Multiple terminals: max reduction applies
// =============================================================================

TEST(multiple_terminals_max_reduction) {
    RailSystem system(64, 64);

    // Terminal A at (10, 10)
    system.place_rail(9, 10, RailType::SurfaceRail, 0);
    system.place_terminal(10, 10, TerminalType::SurfaceStation, 0);

    // Terminal B at (14, 10)
    system.place_rail(15, 10, RailType::SurfaceRail, 0);
    system.place_terminal(14, 10, TerminalType::SurfaceStation, 0);

    system.tick(0.0f);

    // Point (12, 10): distance 2 from both
    // Both give: 50 * (8-2)/8 = 37
    ASSERT_EQ(system.calculate_traffic_reduction(12, 10, 0), 37);

    // Point (11, 10): distance 1 from A, distance 3 from B
    // A: 50 * 7/8 = 43, B: 50 * 5/8 = 31
    // Max = 43
    ASSERT_EQ(system.calculate_traffic_reduction(11, 10, 0), 43);
}

TEST(multiple_terminals_closer_wins) {
    RailSystem system(64, 64);

    // Terminal A at (10, 10)
    system.place_rail(9, 10, RailType::SurfaceRail, 0);
    system.place_terminal(10, 10, TerminalType::SurfaceStation, 0);

    // Terminal B at (20, 10)
    system.place_rail(19, 10, RailType::SurfaceRail, 0);
    system.place_terminal(20, 10, TerminalType::SurfaceStation, 0);

    system.tick(0.0f);

    // Point (10, 10): distance 0 from A, distance 10 from B (beyond B radius of 8)
    // Max = 50 (from A)
    ASSERT_EQ(system.calculate_traffic_reduction(10, 10, 0), 50);

    // Point (15, 10): distance 5 from both
    // Both: 50 * (8-5)/8 = 50 * 3/8 = 18
    ASSERT_EQ(system.calculate_traffic_reduction(15, 10, 0), 18);
}

// =============================================================================
// Invalid owner
// =============================================================================

TEST(invalid_owner_returns_zero) {
    RailSystem system(64, 64);
    setup_active_terminal(system, 10, 10, 9, 10, 0);

    ASSERT_EQ(system.calculate_traffic_reduction(10, 10, 5), 0);
    ASSERT_EQ(system.calculate_traffic_reduction(10, 10, 255), 0);
}

TEST(wrong_owner_returns_zero) {
    RailSystem system(64, 64);
    setup_active_terminal(system, 10, 10, 9, 10, 0);

    // Player 1 has no terminals
    ASSERT_EQ(system.calculate_traffic_reduction(10, 10, 1), 0);
}

// =============================================================================
// Matches get_traffic_reduction_at
// =============================================================================

TEST(matches_get_traffic_reduction_at) {
    RailSystem system(64, 64);
    setup_active_terminal(system, 10, 10, 9, 10, 0);

    // Both methods should return the same values
    for (int32_t x = 0; x < 20; ++x) {
        for (int32_t y = 0; y < 20; ++y) {
            uint8_t calc = system.calculate_traffic_reduction(x, y, 0);
            uint8_t get = system.get_traffic_reduction_at(x, y, 0);
            if (calc != get) {
                printf("\n  FAILED: calculate vs get at (%d,%d): %d vs %d\n",
                       x, y, (int)calc, (int)get);
                tests_failed++;
                return;
            }
        }
    }
}

// =============================================================================
// No terminals = no reduction
// =============================================================================

TEST(no_terminals_no_reduction) {
    RailSystem system(64, 64);
    ASSERT_EQ(system.calculate_traffic_reduction(10, 10, 0), 0);
    ASSERT_EQ(system.calculate_traffic_reduction(0, 0, 0), 0);
}

// =============================================================================
// Main
// =============================================================================

int main() {
    printf("=== Rail Traffic Reduction Tests (Ticket E7-045) ===\n\n");

    // Basic reduction
    RUN_TEST(reduction_50_at_terminal);

    // Distance-based falloff
    RUN_TEST(reduction_linear_falloff_distance_1);
    RUN_TEST(reduction_linear_falloff_distance_2);
    RUN_TEST(reduction_linear_falloff_distance_4);
    RUN_TEST(reduction_linear_falloff_distance_6);
    RUN_TEST(reduction_linear_falloff_distance_7);
    RUN_TEST(reduction_zero_at_radius_edge);
    RUN_TEST(reduction_zero_beyond_radius);

    // Manhattan distance
    RUN_TEST(reduction_diagonal_manhattan);

    // Active terminals only
    RUN_TEST(inactive_terminal_no_reduction);
    RUN_TEST(active_terminal_provides_reduction);

    // Multiple terminals
    RUN_TEST(multiple_terminals_max_reduction);
    RUN_TEST(multiple_terminals_closer_wins);

    // Invalid owner
    RUN_TEST(invalid_owner_returns_zero);
    RUN_TEST(wrong_owner_returns_zero);

    // Matches existing API
    RUN_TEST(matches_get_traffic_reduction_at);

    // Edge case
    RUN_TEST(no_terminals_no_reduction);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
