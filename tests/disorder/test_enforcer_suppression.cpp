/**
 * @file test_enforcer_suppression.cpp
 * @brief Unit tests for enforcer suppression integration (Ticket E10-076).
 *
 * Tests cover:
 * - StubServiceQueryable (all 0s) results in no suppression
 * - Fixed coverage values verify suppression math
 * - Suppression correctly reduces disorder levels
 * - Zero disorder + coverage = no change
 * - Partial coverage with effectiveness < 1.0
 */

#include <sims3000/disorder/EnforcerSuppression.h>
#include <sims3000/disorder/DisorderGrid.h>
#include <sims3000/services/ServiceTypes.h>
#include <cstdio>
#include <cstdlib>

using namespace sims3000::disorder;
using namespace sims3000::building;
using namespace sims3000::services;

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
// Stub Service Queryable - No coverage
// =============================================================================

class StubServiceQueryable : public IServiceQueryable {
public:
    float get_coverage(uint8_t service_type, uint8_t player_id) const override {
        (void)service_type;
        (void)player_id;
        return 0.0f; // No coverage
    }

    float get_coverage_at(uint8_t service_type, int32_t x, int32_t y, uint8_t player_id) const override {
        (void)service_type;
        (void)x;
        (void)y;
        (void)player_id;
        return 0.0f; // No coverage
    }

    float get_effectiveness(uint8_t service_type, uint8_t player_id) const override {
        (void)service_type;
        (void)player_id;
        return 1.0f; // Full effectiveness (doesn't matter if coverage = 0)
    }
};

// =============================================================================
// Fixed Coverage Service Queryable
// =============================================================================

class FixedCoverageQueryable : public IServiceQueryable {
public:
    FixedCoverageQueryable(float coverage, float effectiveness)
        : m_coverage(coverage), m_effectiveness(effectiveness) {}

    float get_coverage(uint8_t service_type, uint8_t player_id) const override {
        (void)service_type;
        (void)player_id;
        return m_coverage;
    }

    float get_coverage_at(uint8_t service_type, int32_t x, int32_t y, uint8_t player_id) const override {
        (void)service_type;
        (void)x;
        (void)y;
        (void)player_id;
        return m_coverage;
    }

    float get_effectiveness(uint8_t service_type, uint8_t player_id) const override {
        (void)service_type;
        (void)player_id;
        return m_effectiveness;
    }

private:
    float m_coverage;
    float m_effectiveness;
};

// =============================================================================
// Tests with StubServiceQueryable (no coverage)
// =============================================================================

TEST(stub_no_suppression) {
    DisorderGrid grid(16, 16);
    grid.set_level(8, 8, 100);

    StubServiceQueryable stub;
    apply_enforcer_suppression(grid, stub, 0);

    // No coverage -> no suppression
    ASSERT_EQ(grid.get_level(8, 8), 100);
}

TEST(stub_high_disorder_no_suppression) {
    DisorderGrid grid(16, 16);
    grid.set_level(5, 5, 255);
    grid.set_level(10, 10, 200);

    StubServiceQueryable stub;
    apply_enforcer_suppression(grid, stub, 0);

    // No coverage -> no changes
    ASSERT_EQ(grid.get_level(5, 5), 255);
    ASSERT_EQ(grid.get_level(10, 10), 200);
}

// =============================================================================
// Tests with fixed coverage (verify math)
// =============================================================================

TEST(full_coverage_full_effectiveness) {
    DisorderGrid grid(16, 16);
    // Set disorder to 100
    grid.set_level(8, 8, 100);

    // Full coverage (1.0), full effectiveness (1.0)
    FixedCoverageQueryable queryable(1.0f, 1.0f);
    apply_enforcer_suppression(grid, queryable, 0);

    // suppression = 100 * 1.0 * 1.0 * 0.7 = 70
    // 100 - 70 = 30
    ASSERT_EQ(grid.get_level(8, 8), 30);
}

TEST(partial_coverage_full_effectiveness) {
    DisorderGrid grid(16, 16);
    grid.set_level(8, 8, 100);

    // 50% coverage, full effectiveness
    FixedCoverageQueryable queryable(0.5f, 1.0f);
    apply_enforcer_suppression(grid, queryable, 0);

    // suppression = 100 * 0.5 * 1.0 * 0.7 = 35
    // 100 - 35 = 65
    ASSERT_EQ(grid.get_level(8, 8), 65);
}

TEST(full_coverage_partial_effectiveness) {
    DisorderGrid grid(16, 16);
    grid.set_level(8, 8, 100);

    // Full coverage, 50% effectiveness
    FixedCoverageQueryable queryable(1.0f, 0.5f);
    apply_enforcer_suppression(grid, queryable, 0);

    // suppression = 100 * 1.0 * 0.5 * 0.7 = 35
    // 100 - 35 = 65
    ASSERT_EQ(grid.get_level(8, 8), 65);
}

TEST(partial_coverage_partial_effectiveness) {
    DisorderGrid grid(16, 16);
    grid.set_level(8, 8, 100);

    // 50% coverage, 80% effectiveness
    FixedCoverageQueryable queryable(0.5f, 0.8f);
    apply_enforcer_suppression(grid, queryable, 0);

    // suppression = 100 * 0.5 * 0.8 * 0.7 = 28
    // 100 - 28 = 72
    ASSERT_EQ(grid.get_level(8, 8), 72);
}

TEST(low_disorder_with_coverage) {
    DisorderGrid grid(16, 16);
    grid.set_level(8, 8, 10);

    // Full coverage, full effectiveness
    FixedCoverageQueryable queryable(1.0f, 1.0f);
    apply_enforcer_suppression(grid, queryable, 0);

    // suppression = 10 * 1.0 * 1.0 * 0.7 = 7
    // 10 - 7 = 3
    ASSERT_EQ(grid.get_level(8, 8), 3);
}

TEST(very_high_disorder_with_coverage) {
    DisorderGrid grid(16, 16);
    grid.set_level(8, 8, 255);

    // Full coverage, full effectiveness
    FixedCoverageQueryable queryable(1.0f, 1.0f);
    apply_enforcer_suppression(grid, queryable, 0);

    // suppression = 255 * 1.0 * 1.0 * 0.7 = 178.5 -> 178
    // 255 - 178 = 77
    ASSERT_EQ(grid.get_level(8, 8), 77);
}

// =============================================================================
// Zero disorder edge cases
// =============================================================================

TEST(zero_disorder_with_coverage_no_change) {
    DisorderGrid grid(16, 16);
    grid.set_level(8, 8, 0); // Zero disorder

    // Full coverage, full effectiveness
    FixedCoverageQueryable queryable(1.0f, 1.0f);
    apply_enforcer_suppression(grid, queryable, 0);

    // No disorder -> no suppression -> no change
    ASSERT_EQ(grid.get_level(8, 8), 0);
}

TEST(mixed_disorder_levels) {
    DisorderGrid grid(16, 16);
    grid.set_level(0, 0, 100);
    grid.set_level(5, 5, 50);
    grid.set_level(10, 10, 200);
    grid.set_level(15, 15, 0);

    // 60% coverage, full effectiveness
    FixedCoverageQueryable queryable(0.6f, 1.0f);
    apply_enforcer_suppression(grid, queryable, 0);

    // suppression multiplier = 0.6 * 1.0 * 0.7 = 0.42
    // (0,0): 100 * 0.42 = 42 -> 100 - 42 = 58
    ASSERT_EQ(grid.get_level(0, 0), 58);

    // (5,5): 50 * 0.42 = 21 -> 50 - 21 = 29
    ASSERT_EQ(grid.get_level(5, 5), 29);

    // (10,10): 200 * 0.42 = 84 -> 200 - 84 = 116
    ASSERT_EQ(grid.get_level(10, 10), 116);

    // (15,15): 0 -> no change
    ASSERT_EQ(grid.get_level(15, 15), 0);
}

// =============================================================================
// Suppression multiplier constant check
// =============================================================================

TEST(enforcer_suppression_multiplier_is_0_7) {
    ASSERT(ENFORCER_SUPPRESSION_MULTIPLIER == 0.7f);
}

// =============================================================================
// Main
// =============================================================================

int main() {
    printf("=== Enforcer Suppression Tests (E10-076) ===\n\n");

    RUN_TEST(stub_no_suppression);
    RUN_TEST(stub_high_disorder_no_suppression);
    RUN_TEST(full_coverage_full_effectiveness);
    RUN_TEST(partial_coverage_full_effectiveness);
    RUN_TEST(full_coverage_partial_effectiveness);
    RUN_TEST(partial_coverage_partial_effectiveness);
    RUN_TEST(low_disorder_with_coverage);
    RUN_TEST(very_high_disorder_with_coverage);
    RUN_TEST(zero_disorder_with_coverage_no_change);
    RUN_TEST(mixed_disorder_levels);
    RUN_TEST(enforcer_suppression_multiplier_is_0_7);

    printf("\n=== Results: %d passed, %d failed ===\n",
           tests_passed, tests_failed);

    return tests_failed > 0 ? 1 : 0;
}
