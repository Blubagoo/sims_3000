/**
 * @file test_fluid_reservoir_component.cpp
 * @brief Unit tests for FluidReservoirComponent (Epic 6, Ticket 6-004)
 *
 * Tests cover:
 * - Struct size (exactly 16 bytes)
 * - Trivially copyable for serialization
 * - Default initialization (CCR-005 values)
 * - Asymmetric fill/drain rates (drain > fill)
 * - Capacity and level tracking
 * - Active state management
 * - Copy semantics
 * - Reservoir type (reserved field)
 */

#include <sims3000/fluid/FluidReservoirComponent.h>
#include <cstdio>
#include <cstdlib>

using namespace sims3000::fluid;

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
// Struct Layout Tests (Ticket 6-004)
// =============================================================================

TEST(size_is_16_bytes) {
    ASSERT_EQ(sizeof(FluidReservoirComponent), 16u);
}

TEST(trivially_copyable) {
    ASSERT(std::is_trivially_copyable<FluidReservoirComponent>::value);
}

// =============================================================================
// Default Initialization Tests (CCR-005 values)
// =============================================================================

TEST(default_capacity) {
    FluidReservoirComponent reservoir{};
    ASSERT_EQ(reservoir.capacity, 1000u);
}

TEST(default_current_level) {
    FluidReservoirComponent reservoir{};
    ASSERT_EQ(reservoir.current_level, 0u);
}

TEST(default_fill_rate) {
    FluidReservoirComponent reservoir{};
    ASSERT_EQ(reservoir.fill_rate, 50u);
}

TEST(default_drain_rate) {
    FluidReservoirComponent reservoir{};
    ASSERT_EQ(reservoir.drain_rate, 100u);
}

TEST(default_is_active) {
    FluidReservoirComponent reservoir{};
    ASSERT_EQ(reservoir.is_active, false);
}

TEST(default_reservoir_type) {
    FluidReservoirComponent reservoir{};
    ASSERT_EQ(reservoir.reservoir_type, 0u);
}

TEST(default_padding_zeroed) {
    FluidReservoirComponent reservoir{};
    ASSERT_EQ(reservoir._padding[0], 0u);
    ASSERT_EQ(reservoir._padding[1], 0u);
}

// =============================================================================
// Asymmetric Rate Tests (Ticket 6-004)
// =============================================================================

TEST(drain_rate_exceeds_fill_rate) {
    // Per CCR-005: drain faster than fill (asymmetric rates)
    FluidReservoirComponent reservoir{};
    ASSERT(reservoir.drain_rate > reservoir.fill_rate);
}

TEST(drain_rate_is_double_fill_rate) {
    // Default drain is 2x fill (100 vs 50)
    FluidReservoirComponent reservoir{};
    ASSERT_EQ(reservoir.drain_rate, reservoir.fill_rate * 2);
}

// =============================================================================
// Capacity and Level Tests
// =============================================================================

TEST(level_within_capacity) {
    FluidReservoirComponent reservoir{};
    reservoir.current_level = 500;
    ASSERT(reservoir.current_level <= reservoir.capacity);
}

TEST(level_at_capacity) {
    FluidReservoirComponent reservoir{};
    reservoir.current_level = reservoir.capacity;
    ASSERT_EQ(reservoir.current_level, 1000u);
}

TEST(level_at_zero) {
    FluidReservoirComponent reservoir{};
    reservoir.current_level = 0;
    ASSERT_EQ(reservoir.current_level, 0u);
}

TEST(custom_capacity) {
    FluidReservoirComponent reservoir{};
    reservoir.capacity = 5000;
    reservoir.current_level = 3000;
    ASSERT_EQ(reservoir.capacity, 5000u);
    ASSERT_EQ(reservoir.current_level, 3000u);
    ASSERT(reservoir.current_level <= reservoir.capacity);
}

TEST(max_capacity) {
    // uint32_t max: supports very large reservoirs
    FluidReservoirComponent reservoir{};
    reservoir.capacity = UINT32_MAX;
    ASSERT_EQ(reservoir.capacity, UINT32_MAX);
}

// =============================================================================
// Fill/Drain Rate Tests
// =============================================================================

TEST(custom_fill_rate) {
    FluidReservoirComponent reservoir{};
    reservoir.fill_rate = 200;
    ASSERT_EQ(reservoir.fill_rate, 200u);
}

TEST(custom_drain_rate) {
    FluidReservoirComponent reservoir{};
    reservoir.drain_rate = 500;
    ASSERT_EQ(reservoir.drain_rate, 500u);
}

TEST(max_fill_rate) {
    // uint16_t max
    FluidReservoirComponent reservoir{};
    reservoir.fill_rate = UINT16_MAX;
    ASSERT_EQ(reservoir.fill_rate, UINT16_MAX);
}

TEST(max_drain_rate) {
    // uint16_t max
    FluidReservoirComponent reservoir{};
    reservoir.drain_rate = UINT16_MAX;
    ASSERT_EQ(reservoir.drain_rate, UINT16_MAX);
}

TEST(zero_rates) {
    // Inactive reservoir may have zero rates
    FluidReservoirComponent reservoir{};
    reservoir.fill_rate = 0;
    reservoir.drain_rate = 0;
    ASSERT_EQ(reservoir.fill_rate, 0u);
    ASSERT_EQ(reservoir.drain_rate, 0u);
}

// =============================================================================
// Active State Tests
// =============================================================================

TEST(activate_reservoir) {
    FluidReservoirComponent reservoir{};
    ASSERT_EQ(reservoir.is_active, false);
    reservoir.is_active = true;
    ASSERT_EQ(reservoir.is_active, true);
}

TEST(deactivate_reservoir) {
    FluidReservoirComponent reservoir{};
    reservoir.is_active = true;
    reservoir.is_active = false;
    ASSERT_EQ(reservoir.is_active, false);
}

// =============================================================================
// Reservoir Type Tests (reserved field)
// =============================================================================

TEST(reservoir_type_assignment) {
    FluidReservoirComponent reservoir{};
    reservoir.reservoir_type = 1;
    ASSERT_EQ(reservoir.reservoir_type, 1u);
    reservoir.reservoir_type = 255;
    ASSERT_EQ(reservoir.reservoir_type, 255u);
}

// =============================================================================
// Copy Semantics Tests
// =============================================================================

TEST(copy_preserves_all_fields) {
    FluidReservoirComponent original{};
    original.capacity = 2000;
    original.current_level = 750;
    original.fill_rate = 80;
    original.drain_rate = 160;
    original.is_active = true;
    original.reservoir_type = 3;

    FluidReservoirComponent copy = original;
    ASSERT_EQ(copy.capacity, 2000u);
    ASSERT_EQ(copy.current_level, 750u);
    ASSERT_EQ(copy.fill_rate, 80u);
    ASSERT_EQ(copy.drain_rate, 160u);
    ASSERT_EQ(copy.is_active, true);
    ASSERT_EQ(copy.reservoir_type, 3u);
}

TEST(assignment_preserves_all_fields) {
    FluidReservoirComponent original{};
    original.capacity = 3000;
    original.current_level = 1500;
    original.fill_rate = 100;
    original.drain_rate = 200;
    original.is_active = true;
    original.reservoir_type = 5;

    FluidReservoirComponent assigned{};
    assigned = original;
    ASSERT_EQ(assigned.capacity, 3000u);
    ASSERT_EQ(assigned.current_level, 1500u);
    ASSERT_EQ(assigned.fill_rate, 100u);
    ASSERT_EQ(assigned.drain_rate, 200u);
    ASSERT_EQ(assigned.is_active, true);
    ASSERT_EQ(assigned.reservoir_type, 5u);
}

// =============================================================================
// Main Entry Point
// =============================================================================

int main() {
    printf("=== FluidReservoirComponent Unit Tests (Epic 6, Ticket 6-004) ===\n\n");

    // Struct layout tests
    RUN_TEST(size_is_16_bytes);
    RUN_TEST(trivially_copyable);

    // Default initialization tests
    RUN_TEST(default_capacity);
    RUN_TEST(default_current_level);
    RUN_TEST(default_fill_rate);
    RUN_TEST(default_drain_rate);
    RUN_TEST(default_is_active);
    RUN_TEST(default_reservoir_type);
    RUN_TEST(default_padding_zeroed);

    // Asymmetric rate tests
    RUN_TEST(drain_rate_exceeds_fill_rate);
    RUN_TEST(drain_rate_is_double_fill_rate);

    // Capacity and level tests
    RUN_TEST(level_within_capacity);
    RUN_TEST(level_at_capacity);
    RUN_TEST(level_at_zero);
    RUN_TEST(custom_capacity);
    RUN_TEST(max_capacity);

    // Fill/drain rate tests
    RUN_TEST(custom_fill_rate);
    RUN_TEST(custom_drain_rate);
    RUN_TEST(max_fill_rate);
    RUN_TEST(max_drain_rate);
    RUN_TEST(zero_rates);

    // Active state tests
    RUN_TEST(activate_reservoir);
    RUN_TEST(deactivate_reservoir);

    // Reservoir type tests
    RUN_TEST(reservoir_type_assignment);

    // Copy semantics tests
    RUN_TEST(copy_preserves_all_fields);
    RUN_TEST(assignment_preserves_all_fields);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
