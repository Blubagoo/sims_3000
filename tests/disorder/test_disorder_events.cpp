/**
 * @file test_disorder_events.cpp
 * @brief Unit tests for DisorderEvents (Ticket E10-079)
 *
 * Tests cover:
 * - HighDisorderWarning: triggered when crossing above threshold
 * - DisorderSpike: triggered on large increase
 * - DisorderResolved: triggered when dropping below threshold
 * - CityWideDisorder: triggered when average exceeds threshold
 * - Event metadata correctness (position, severity, tick)
 */

#include <sims3000/disorder/DisorderEvents.h>
#include <sims3000/disorder/DisorderGrid.h>
#include <cstdio>
#include <cstdlib>

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
// No Events Tests
// =============================================================================

TEST(no_events_on_empty_grid) {
    DisorderGrid grid(64, 64);
    grid.update_stats();
    auto events = detect_disorder_events(grid, 100);
    ASSERT(events.empty());
}

TEST(no_events_below_thresholds) {
    DisorderGrid grid(64, 64);
    grid.set_level(10, 10, 50); // Below high threshold
    grid.update_stats();
    grid.swap_buffers();
    grid.set_level(10, 10, 60); // Small increase
    grid.update_stats();
    auto events = detect_disorder_events(grid, 100);
    ASSERT(events.empty());
}

// =============================================================================
// HighDisorderWarning Tests
// =============================================================================

TEST(high_disorder_warning_triggered) {
    DisorderGrid grid(64, 64);
    // Set previous tick below threshold
    grid.set_level(10, 10, 191);
    grid.swap_buffers();
    // Set current tick above threshold
    grid.set_level(10, 10, 192);
    grid.update_stats();

    auto events = detect_disorder_events(grid, 100);
    ASSERT_EQ(events.size(), 1u);
    ASSERT(events[0].type == DisorderEventType::HighDisorderWarning);
    ASSERT_EQ(events[0].x, 10);
    ASSERT_EQ(events[0].y, 10);
    ASSERT_EQ(events[0].severity, 192);
    ASSERT_EQ(events[0].tick, 100u);
}

TEST(high_disorder_warning_exact_threshold) {
    DisorderGrid grid(64, 64);
    grid.set_level(10, 10, 191);
    grid.swap_buffers();
    grid.set_level(10, 10, 192); // Exactly at threshold
    grid.update_stats();

    auto events = detect_disorder_events(grid, 200);
    ASSERT_EQ(events.size(), 1u);
    ASSERT(events[0].type == DisorderEventType::HighDisorderWarning);
}

TEST(high_disorder_warning_not_triggered_if_already_high) {
    DisorderGrid grid(64, 64);
    grid.set_level(10, 10, 200); // Already high
    grid.swap_buffers();
    grid.set_level(10, 10, 210); // Still high
    grid.update_stats();

    auto events = detect_disorder_events(grid, 100);
    // No HighDisorderWarning, but might have a spike
    for (const auto& event : events) {
        ASSERT(event.type != DisorderEventType::HighDisorderWarning);
    }
}

// =============================================================================
// DisorderSpike Tests
// =============================================================================

TEST(disorder_spike_triggered) {
    DisorderGrid grid(64, 64);
    grid.set_level(10, 10, 50);
    grid.swap_buffers();
    grid.set_level(10, 10, 115); // Increase of 65 > 64
    grid.update_stats();

    auto events = detect_disorder_events(grid, 100);
    bool found_spike = false;
    for (const auto& event : events) {
        if (event.type == DisorderEventType::DisorderSpike) {
            found_spike = true;
            ASSERT_EQ(event.x, 10);
            ASSERT_EQ(event.y, 10);
            ASSERT_EQ(event.severity, 65);
            ASSERT_EQ(event.tick, 100u);
        }
    }
    ASSERT(found_spike);
}

TEST(disorder_spike_exact_threshold) {
    DisorderGrid grid(64, 64);
    grid.set_level(10, 10, 50);
    grid.swap_buffers();
    grid.set_level(10, 10, 115); // Increase of exactly 65
    grid.update_stats();

    auto events = detect_disorder_events(grid, 100);
    bool found_spike = false;
    for (const auto& event : events) {
        if (event.type == DisorderEventType::DisorderSpike) {
            found_spike = true;
            ASSERT_EQ(event.severity, 65);
        }
    }
    ASSERT(found_spike);
}

TEST(disorder_spike_not_triggered_small_increase) {
    DisorderGrid grid(64, 64);
    grid.set_level(10, 10, 50);
    grid.swap_buffers();
    grid.set_level(10, 10, 114); // Increase of 64, not > 64
    grid.update_stats();

    auto events = detect_disorder_events(grid, 100);
    for (const auto& event : events) {
        ASSERT(event.type != DisorderEventType::DisorderSpike);
    }
}

TEST(disorder_spike_from_zero) {
    DisorderGrid grid(64, 64);
    // Previous is 0
    grid.swap_buffers();
    grid.set_level(10, 10, 65); // Increase of 65
    grid.update_stats();

    auto events = detect_disorder_events(grid, 100);
    bool found_spike = false;
    for (const auto& event : events) {
        if (event.type == DisorderEventType::DisorderSpike) {
            found_spike = true;
        }
    }
    ASSERT(found_spike);
}

// =============================================================================
// DisorderResolved Tests
// =============================================================================

TEST(disorder_resolved_triggered) {
    DisorderGrid grid(64, 64);
    grid.set_level(10, 10, 200); // High
    grid.swap_buffers();
    grid.set_level(10, 10, 191); // Dropped below threshold
    grid.update_stats();

    auto events = detect_disorder_events(grid, 100);
    bool found_resolved = false;
    for (const auto& event : events) {
        if (event.type == DisorderEventType::DisorderResolved) {
            found_resolved = true;
            ASSERT_EQ(event.x, 10);
            ASSERT_EQ(event.y, 10);
            ASSERT_EQ(event.severity, 200); // Previous high level
            ASSERT_EQ(event.tick, 100u);
        }
    }
    ASSERT(found_resolved);
}

TEST(disorder_resolved_exact_threshold) {
    DisorderGrid grid(64, 64);
    grid.set_level(10, 10, 192); // At threshold
    grid.swap_buffers();
    grid.set_level(10, 10, 191); // Just below
    grid.update_stats();

    auto events = detect_disorder_events(grid, 100);
    bool found_resolved = false;
    for (const auto& event : events) {
        if (event.type == DisorderEventType::DisorderResolved) {
            found_resolved = true;
        }
    }
    ASSERT(found_resolved);
}

TEST(disorder_resolved_not_triggered_if_still_low) {
    DisorderGrid grid(64, 64);
    grid.set_level(10, 10, 100); // Low
    grid.swap_buffers();
    grid.set_level(10, 10, 50); // Still low
    grid.update_stats();

    auto events = detect_disorder_events(grid, 100);
    for (const auto& event : events) {
        ASSERT(event.type != DisorderEventType::DisorderResolved);
    }
}

// =============================================================================
// CityWideDisorder Tests
// =============================================================================

TEST(city_wide_disorder_triggered) {
    DisorderGrid grid(4, 4); // 16 tiles
    // Set average to 100: 16 * 100 = 1600
    for (int32_t y = 0; y < 4; ++y) {
        for (int32_t x = 0; x < 4; ++x) {
            grid.set_level(x, y, 100);
        }
    }
    grid.update_stats();

    auto events = detect_disorder_events(grid, 100);
    bool found_citywide = false;
    for (const auto& event : events) {
        if (event.type == DisorderEventType::CityWideDisorder) {
            found_citywide = true;
            ASSERT_EQ(event.x, 2); // Center of 4x4 grid
            ASSERT_EQ(event.y, 2);
            ASSERT_EQ(event.severity, 100);
            ASSERT_EQ(event.tick, 100u);
        }
    }
    ASSERT(found_citywide);
}

TEST(city_wide_disorder_exact_threshold) {
    DisorderGrid grid(4, 4);
    for (int32_t y = 0; y < 4; ++y) {
        for (int32_t x = 0; x < 4; ++x) {
            grid.set_level(x, y, 100); // Exactly at threshold
        }
    }
    grid.update_stats();

    auto events = detect_disorder_events(grid, 100);
    bool found_citywide = false;
    for (const auto& event : events) {
        if (event.type == DisorderEventType::CityWideDisorder) {
            found_citywide = true;
        }
    }
    ASSERT(found_citywide);
}

TEST(city_wide_disorder_not_triggered_below_threshold) {
    DisorderGrid grid(4, 4);
    for (int32_t y = 0; y < 4; ++y) {
        for (int32_t x = 0; x < 4; ++x) {
            grid.set_level(x, y, 99); // Below threshold
        }
    }
    grid.update_stats();

    auto events = detect_disorder_events(grid, 100);
    for (const auto& event : events) {
        ASSERT(event.type != DisorderEventType::CityWideDisorder);
    }
}

// =============================================================================
// Multiple Events Tests
// =============================================================================

TEST(multiple_events_in_one_tick) {
    DisorderGrid grid(64, 64);
    // Setup previous state
    grid.set_level(10, 10, 191); // Will trigger HighDisorderWarning
    grid.set_level(20, 20, 50);  // Will trigger DisorderSpike
    grid.swap_buffers();

    // Current state
    grid.set_level(10, 10, 192); // HighDisorderWarning
    grid.set_level(20, 20, 116); // DisorderSpike (increase of 66)
    grid.update_stats();

    auto events = detect_disorder_events(grid, 100);
    ASSERT(events.size() >= 2); // At least the two we expect

    bool found_warning = false;
    bool found_spike = false;
    for (const auto& event : events) {
        if (event.type == DisorderEventType::HighDisorderWarning && event.x == 10 && event.y == 10) {
            found_warning = true;
        }
        if (event.type == DisorderEventType::DisorderSpike && event.x == 20 && event.y == 20) {
            found_spike = true;
        }
    }
    ASSERT(found_warning);
    ASSERT(found_spike);
}

TEST(spike_and_warning_same_tile) {
    DisorderGrid grid(64, 64);
    // Tile jumps from 100 to 200: both spike and warning
    grid.set_level(10, 10, 100);
    grid.swap_buffers();
    grid.set_level(10, 10, 200);
    grid.update_stats();

    auto events = detect_disorder_events(grid, 100);
    bool found_warning = false;
    bool found_spike = false;
    for (const auto& event : events) {
        if (event.type == DisorderEventType::HighDisorderWarning && event.x == 10 && event.y == 10) {
            found_warning = true;
        }
        if (event.type == DisorderEventType::DisorderSpike && event.x == 10 && event.y == 10) {
            found_spike = true;
        }
    }
    ASSERT(found_warning);
    ASSERT(found_spike);
}

// =============================================================================
// Main Entry Point
// =============================================================================

int main() {
    printf("=== DisorderEvents Unit Tests (Ticket E10-079) ===\n\n");

    // No events tests
    RUN_TEST(no_events_on_empty_grid);
    RUN_TEST(no_events_below_thresholds);

    // HighDisorderWarning tests
    RUN_TEST(high_disorder_warning_triggered);
    RUN_TEST(high_disorder_warning_exact_threshold);
    RUN_TEST(high_disorder_warning_not_triggered_if_already_high);

    // DisorderSpike tests
    RUN_TEST(disorder_spike_triggered);
    RUN_TEST(disorder_spike_exact_threshold);
    RUN_TEST(disorder_spike_not_triggered_small_increase);
    RUN_TEST(disorder_spike_from_zero);

    // DisorderResolved tests
    RUN_TEST(disorder_resolved_triggered);
    RUN_TEST(disorder_resolved_exact_threshold);
    RUN_TEST(disorder_resolved_not_triggered_if_still_low);

    // CityWideDisorder tests
    RUN_TEST(city_wide_disorder_triggered);
    RUN_TEST(city_wide_disorder_exact_threshold);
    RUN_TEST(city_wide_disorder_not_triggered_below_threshold);

    // Multiple events tests
    RUN_TEST(multiple_events_in_one_tick);
    RUN_TEST(spike_and_warning_same_tile);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
