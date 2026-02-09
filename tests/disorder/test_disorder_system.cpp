/**
 * @file test_disorder_system.cpp
 * @brief Unit tests for DisorderSystem skeleton (Ticket E10-072)
 *
 * Tests cover:
 * - Construction with grid dimensions
 * - ISimulatable interface (priority, name)
 * - Grid access (dimensions match construction args)
 * - tick() swaps buffers (data moves to previous)
 * - tick() runs without crash
 * - Stats return 0 on empty grid
 * - Stats after manual grid manipulation
 */

#include <sims3000/disorder/DisorderSystem.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

using namespace sims3000;
using namespace sims3000::disorder;

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
// Minimal ISimulationTime stub for testing tick()
// =============================================================================

class StubSimulationTime : public ISimulationTime {
public:
    SimulationTick tick_value = 0;

    SimulationTick getCurrentTick() const override { return tick_value; }
    float getTickDelta() const override { return 0.05f; }
    float getInterpolation() const override { return 0.0f; }
    double getTotalTime() const override { return static_cast<double>(tick_value) * 0.05; }
};

// =============================================================================
// Construction Tests
// =============================================================================

TEST(construction_dimensions) {
    DisorderSystem system(256, 256);
    ASSERT_EQ(system.get_grid().get_width(), static_cast<uint16_t>(256));
    ASSERT_EQ(system.get_grid().get_height(), static_cast<uint16_t>(256));
}

TEST(construction_non_square) {
    DisorderSystem system(128, 64);
    ASSERT_EQ(system.get_grid().get_width(), static_cast<uint16_t>(128));
    ASSERT_EQ(system.get_grid().get_height(), static_cast<uint16_t>(64));
}

TEST(construction_small_grid) {
    DisorderSystem system(16, 16);
    ASSERT_EQ(system.get_grid().get_width(), static_cast<uint16_t>(16));
    ASSERT_EQ(system.get_grid().get_height(), static_cast<uint16_t>(16));
}

// =============================================================================
// ISimulatable Interface Tests
// =============================================================================

TEST(get_priority) {
    DisorderSystem system(64, 64);
    ASSERT_EQ(system.getPriority(), 70);
}

TEST(get_name) {
    DisorderSystem system(64, 64);
    ASSERT_EQ(strcmp(system.getName(), "DisorderSystem"), 0);
}

// =============================================================================
// Grid Access Tests
// =============================================================================

TEST(get_grid_const) {
    DisorderSystem system(128, 128);
    const DisorderSystem& const_sys = system;
    const DisorderGrid& grid = const_sys.get_grid();
    ASSERT_EQ(grid.get_width(), static_cast<uint16_t>(128));
    ASSERT_EQ(grid.get_height(), static_cast<uint16_t>(128));
}

TEST(get_grid_mut) {
    DisorderSystem system(64, 64);
    DisorderGrid& grid = system.get_grid_mut();
    // Should be able to write through mutable reference
    grid.set_level(10, 10, 42);
    ASSERT_EQ(system.get_grid().get_level(10, 10), 42);
}

TEST(grid_initially_all_zero) {
    DisorderSystem system(32, 32);
    const DisorderGrid& grid = system.get_grid();
    ASSERT_EQ(grid.get_level(0, 0), 0);
    ASSERT_EQ(grid.get_level(15, 15), 0);
    ASSERT_EQ(grid.get_level(31, 31), 0);
}

// =============================================================================
// tick() Tests — swap_buffers behavior
// =============================================================================

TEST(tick_swaps_buffers) {
    DisorderSystem system(32, 32);

    // Set data in the current buffer
    system.get_grid_mut().set_level(5, 5, 100);
    ASSERT_EQ(system.get_grid().get_level(5, 5), 100);
    ASSERT_EQ(system.get_grid().get_level_previous_tick(5, 5), 0);

    // tick() should call swap_buffers() first
    StubSimulationTime time;
    time.tick_value = 1;
    system.tick(time);

    // After swap: the old current (100) becomes previous
    ASSERT_EQ(system.get_grid().get_level_previous_tick(5, 5), 100);
    // Current (was previous, which was 0) should be 0
    ASSERT_EQ(system.get_grid().get_level(5, 5), 0);
}

TEST(tick_no_crash_empty_grid) {
    DisorderSystem system(64, 64);
    StubSimulationTime time;
    time.tick_value = 0;
    system.tick(time);
    // Should not crash
}

TEST(tick_no_crash_multiple_ticks) {
    DisorderSystem system(128, 128);
    StubSimulationTime time;
    for (uint64_t t = 0; t < 100; ++t) {
        time.tick_value = t;
        system.tick(time);
    }
    // Should not crash after 100 ticks
}

TEST(tick_no_crash_with_data) {
    DisorderSystem system(64, 64);

    // Populate some disorder
    system.get_grid_mut().set_level(10, 10, 200);
    system.get_grid_mut().set_level(20, 20, 150);
    system.get_grid_mut().set_level(30, 30, 100);

    StubSimulationTime time;
    for (uint64_t t = 0; t < 10; ++t) {
        time.tick_value = t;
        system.tick(time);
    }
    // Should not crash
}

// =============================================================================
// Stats Tests
// =============================================================================

TEST(stats_zero_on_empty_grid) {
    DisorderSystem system(64, 64);
    // Stats should be 0 before any tick
    ASSERT_EQ(system.get_total_disorder(), 0u);
    ASSERT_EQ(system.get_high_disorder_tiles(), 0u);
}

TEST(stats_zero_after_tick_on_empty_grid) {
    DisorderSystem system(64, 64);
    StubSimulationTime time;
    time.tick_value = 0;
    system.tick(time);

    // After tick, stats should be updated and still 0
    ASSERT_EQ(system.get_total_disorder(), 0u);
    ASSERT_EQ(system.get_high_disorder_tiles(), 0u);
}

TEST(stats_after_manual_set_and_tick) {
    DisorderSystem system(16, 16);

    // Set some disorder in the current buffer
    system.get_grid_mut().set_level(0, 0, 50);
    system.get_grid_mut().set_level(1, 0, 200);

    // tick() swaps buffers, so the data we just set goes to previous.
    // Current buffer (the old previous, all zeros) becomes current.
    // Since all stubs are empty, no new disorder is generated.
    // update_stats() operates on current buffer (all zeros after swap).
    StubSimulationTime time;
    time.tick_value = 0;
    system.tick(time);

    // After tick, current buffer should be all zeros (stubs don't write anything)
    ASSERT_EQ(system.get_total_disorder(), 0u);
    ASSERT_EQ(system.get_high_disorder_tiles(), 0u);
}

TEST(stats_with_direct_grid_manipulation) {
    DisorderSystem system(8, 8);

    // Manually set levels and update stats (bypassing tick)
    system.get_grid_mut().set_level(0, 0, 50);
    system.get_grid_mut().set_level(1, 0, 200);
    system.get_grid_mut().set_level(2, 0, 130);
    system.get_grid_mut().update_stats();

    ASSERT_EQ(system.get_total_disorder(), 380u);
    // Tiles >= 128: 200 and 130 = 2 tiles
    ASSERT_EQ(system.get_high_disorder_tiles(), 2u);
}

TEST(stats_high_disorder_custom_threshold) {
    DisorderSystem system(8, 8);

    system.get_grid_mut().set_level(0, 0, 50);
    system.get_grid_mut().set_level(1, 0, 100);
    system.get_grid_mut().set_level(2, 0, 150);
    system.get_grid_mut().set_level(3, 0, 200);

    // Custom threshold
    ASSERT_EQ(system.get_high_disorder_tiles(100), 3u);
    ASSERT_EQ(system.get_high_disorder_tiles(200), 1u);
    ASSERT_EQ(system.get_high_disorder_tiles(1), 4u);
}

// =============================================================================
// Main Entry Point
// =============================================================================

int main() {
    printf("=== DisorderSystem Unit Tests (Ticket E10-072) ===\n\n");

    // Construction
    RUN_TEST(construction_dimensions);
    RUN_TEST(construction_non_square);
    RUN_TEST(construction_small_grid);

    // ISimulatable interface
    RUN_TEST(get_priority);
    RUN_TEST(get_name);

    // Grid access
    RUN_TEST(get_grid_const);
    RUN_TEST(get_grid_mut);
    RUN_TEST(grid_initially_all_zero);

    // tick() behavior
    RUN_TEST(tick_swaps_buffers);
    RUN_TEST(tick_no_crash_empty_grid);
    RUN_TEST(tick_no_crash_multiple_ticks);
    RUN_TEST(tick_no_crash_with_data);

    // Stats
    RUN_TEST(stats_zero_on_empty_grid);
    RUN_TEST(stats_zero_after_tick_on_empty_grid);
    RUN_TEST(stats_after_manual_set_and_tick);
    RUN_TEST(stats_with_direct_grid_manipulation);
    RUN_TEST(stats_high_disorder_custom_threshold);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
