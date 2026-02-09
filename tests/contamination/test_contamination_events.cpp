/**
 * @file test_contamination_events.cpp
 * @brief Unit tests for ContaminationEvents (Ticket E10-091)
 *
 * Tests cover:
 * - ToxicWarning event detection
 * - ContaminationSpike event detection
 * - ContaminationCleared event detection
 * - CityWideToxic event detection
 * - Threshold boundary conditions
 * - Multiple simultaneous events
 */

#include <sims3000/contamination/ContaminationEvents.h>
#include <sims3000/contamination/ContaminationGrid.h>
#include <sims3000/contamination/ContaminationType.h>
#include <cstdio>
#include <cstdlib>

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
        printf("\n  FAILED: %s == %s (line %d)\n", #a, #b, __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

// =============================================================================
// ToxicWarning Event Tests
// =============================================================================

TEST(toxic_warning_no_events_empty_grid) {
    ContaminationGrid grid(64, 64);
    auto events = detect_contamination_events(grid, 0);
    ASSERT_EQ(events.size(), 0u);
}

TEST(toxic_warning_detected_crossing_threshold) {
    ContaminationGrid grid(64, 64);
    grid.add_contamination(10, 10, 191, 0);  // Previous tick: 191
    grid.swap_buffers();
    grid.add_contamination(10, 10, 192, 0);  // Current tick: 192 (at threshold)

    auto events = detect_contamination_events(grid, 1);
    ASSERT(events.size() > 0);

    bool found_toxic_warning = false;
    for (const auto& event : events) {
        if (event.type == ContaminationEventType::ToxicWarning &&
            event.x == 10 && event.y == 10) {
            found_toxic_warning = true;
            ASSERT_EQ(event.severity, 192);
            ASSERT_EQ(event.tick, 1u);
        }
    }
    ASSERT(found_toxic_warning);
}

TEST(toxic_warning_not_detected_below_threshold) {
    ContaminationGrid grid(64, 64);
    grid.add_contamination(10, 10, 100, 0);
    grid.swap_buffers();
    grid.add_contamination(10, 10, 50, 0);  // 150 total, below 192

    auto events = detect_contamination_events(grid, 1);

    for (const auto& event : events) {
        ASSERT(event.type != ContaminationEventType::ToxicWarning ||
               event.x != 10 || event.y != 10);
    }
}

TEST(toxic_warning_not_detected_already_above_threshold) {
    ContaminationGrid grid(64, 64);
    grid.add_contamination(10, 10, 200, 0);
    grid.swap_buffers();
    grid.add_contamination(10, 10, 50, 0);  // 250 total, but was already above

    auto events = detect_contamination_events(grid, 1);

    for (const auto& event : events) {
        ASSERT(event.type != ContaminationEventType::ToxicWarning ||
               event.x != 10 || event.y != 10);
    }
}

TEST(toxic_warning_exact_threshold_boundary) {
    ContaminationGrid grid(64, 64);
    grid.add_contamination(10, 10, 191, 0);  // Previous: 191
    grid.swap_buffers();
    grid.add_contamination(10, 10, 192, 0);  // Current: 192 (exactly at threshold)

    auto events = detect_contamination_events(grid, 1);

    bool found = false;
    for (const auto& event : events) {
        if (event.type == ContaminationEventType::ToxicWarning) {
            found = true;
            break;
        }
    }
    ASSERT(found);
}

// =============================================================================
// ContaminationSpike Event Tests
// =============================================================================

TEST(spike_detected_exactly_at_threshold) {
    ContaminationGrid grid(64, 64);
    grid.add_contamination(10, 10, 100, 0);  // Previous: 100
    grid.swap_buffers();
    grid.add_contamination(10, 10, 164, 0);  // Current: 164 (increase of 64)

    auto events = detect_contamination_events(grid, 1);

    bool found_spike = false;
    for (const auto& event : events) {
        if (event.type == ContaminationEventType::ContaminationSpike &&
            event.x == 10 && event.y == 10) {
            found_spike = true;
            ASSERT_EQ(event.severity, 64);
            ASSERT_EQ(event.tick, 1u);
        }
    }
    ASSERT(found_spike);
}

TEST(spike_detected_above_threshold) {
    ContaminationGrid grid(64, 64);
    grid.add_contamination(10, 10, 100, 0);  // Previous: 100
    grid.swap_buffers();
    grid.add_contamination(10, 10, 200, 0);  // Current: 200 (increase of 100)

    auto events = detect_contamination_events(grid, 1);

    bool found_spike = false;
    for (const auto& event : events) {
        if (event.type == ContaminationEventType::ContaminationSpike &&
            event.x == 10 && event.y == 10) {
            found_spike = true;
            ASSERT_EQ(event.severity, 100);
        }
    }
    ASSERT(found_spike);
}

TEST(spike_not_detected_below_threshold) {
    ContaminationGrid grid(64, 64);
    grid.add_contamination(10, 10, 100, 0);  // Previous: 100
    grid.swap_buffers();
    grid.add_contamination(10, 10, 163, 0);  // Current: 163 (increase of 63, below 64 threshold)

    auto events = detect_contamination_events(grid, 1);

    for (const auto& event : events) {
        ASSERT(event.type != ContaminationEventType::ContaminationSpike ||
               event.x != 10 || event.y != 10);
    }
}

TEST(spike_not_detected_on_decrease) {
    ContaminationGrid grid(64, 64);
    grid.add_contamination(10, 10, 200, 0);  // Previous: 200
    grid.swap_buffers();
    grid.add_contamination(10, 10, 150, 0);  // Current: 150 (decrease, not spike)

    auto events = detect_contamination_events(grid, 1);

    for (const auto& event : events) {
        ASSERT(event.type != ContaminationEventType::ContaminationSpike ||
               event.x != 10 || event.y != 10);
    }
}

TEST(spike_captures_dominant_type) {
    ContaminationGrid grid(64, 64);
    grid.add_contamination(10, 10, 50, static_cast<uint8_t>(ContaminationType::Traffic));  // Previous: 50
    grid.swap_buffers();
    grid.add_contamination(10, 10, 150, static_cast<uint8_t>(ContaminationType::Traffic));  // Current: 150 (increase of 100)

    auto events = detect_contamination_events(grid, 1);

    bool found_spike = false;
    for (const auto& event : events) {
        if (event.type == ContaminationEventType::ContaminationSpike &&
            event.x == 10 && event.y == 10) {
            found_spike = true;
            ASSERT_EQ(event.contam_type, static_cast<uint8_t>(ContaminationType::Traffic));
        }
    }
    ASSERT(found_spike);
}

// =============================================================================
// ContaminationCleared Event Tests
// =============================================================================

TEST(cleared_detected_dropping_below_threshold) {
    ContaminationGrid grid(64, 64);
    grid.add_contamination(10, 10, 200, 0);  // Previous: 200
    grid.swap_buffers();
    grid.add_contamination(10, 10, 150, 0);  // Current: 150 (dropped below 192)

    auto events = detect_contamination_events(grid, 1);

    bool found_cleared = false;
    for (const auto& event : events) {
        if (event.type == ContaminationEventType::ContaminationCleared &&
            event.x == 10 && event.y == 10) {
            found_cleared = true;
            ASSERT_EQ(event.severity, 50);  // 200 - 150 = 50
            ASSERT_EQ(event.tick, 1u);
        }
    }
    ASSERT(found_cleared);
}

TEST(cleared_not_detected_staying_below_threshold) {
    ContaminationGrid grid(64, 64);
    grid.add_contamination(10, 10, 100, 0);  // Previous: 100 (below threshold)
    grid.swap_buffers();
    grid.add_contamination(10, 10, 50, 0);   // Current: 50 (still below threshold)

    auto events = detect_contamination_events(grid, 1);

    for (const auto& event : events) {
        ASSERT(event.type != ContaminationEventType::ContaminationCleared ||
               event.x != 10 || event.y != 10);
    }
}

TEST(cleared_not_detected_staying_above_threshold) {
    ContaminationGrid grid(64, 64);
    grid.add_contamination(10, 10, 255, 0);  // Previous: 255
    grid.swap_buffers();
    grid.add_contamination(10, 10, 205, 0);  // Current: 205 (still above 192 threshold)

    auto events = detect_contamination_events(grid, 1);

    for (const auto& event : events) {
        ASSERT(event.type != ContaminationEventType::ContaminationCleared ||
               event.x != 10 || event.y != 10);
    }
}

TEST(cleared_exact_threshold_boundary) {
    ContaminationGrid grid(64, 64);
    grid.add_contamination(10, 10, 192, 0);  // Previous: 192 (at threshold)
    grid.swap_buffers();
    grid.add_contamination(10, 10, 191, 0);  // Current: 191 (exactly below threshold)

    auto events = detect_contamination_events(grid, 1);

    bool found = false;
    for (const auto& event : events) {
        if (event.type == ContaminationEventType::ContaminationCleared) {
            found = true;
            break;
        }
    }
    ASSERT(found);
}

TEST(cleared_captures_previous_dominant_type) {
    ContaminationGrid grid(64, 64);
    grid.add_contamination(10, 10, 200, static_cast<uint8_t>(ContaminationType::Industrial));  // Previous: 200, type Industrial
    grid.swap_buffers();
    grid.add_contamination(10, 10, 100, static_cast<uint8_t>(ContaminationType::Industrial));  // Current: 100 (below threshold)

    auto events = detect_contamination_events(grid, 1);

    bool found_cleared = false;
    for (const auto& event : events) {
        if (event.type == ContaminationEventType::ContaminationCleared &&
            event.x == 10 && event.y == 10) {
            found_cleared = true;
            ASSERT_EQ(event.contam_type, static_cast<uint8_t>(ContaminationType::Industrial));
        }
    }
    ASSERT(found_cleared);
}

// =============================================================================
// CityWideToxic Event Tests
// =============================================================================

TEST(city_wide_toxic_detected_above_threshold) {
    ContaminationGrid grid(4, 4);  // 16 cells
    // Set average to exactly 80: 16 * 80 = 1280
    for (int32_t y = 0; y < 4; ++y) {
        for (int32_t x = 0; x < 4; ++x) {
            grid.add_contamination(x, y, 80, 0);
        }
    }

    auto events = detect_contamination_events(grid, 1);

    bool found_city_wide = false;
    for (const auto& event : events) {
        if (event.type == ContaminationEventType::CityWideToxic) {
            found_city_wide = true;
            ASSERT_EQ(event.x, -1);
            ASSERT_EQ(event.y, -1);
            ASSERT_EQ(event.severity, 80);
            ASSERT_EQ(event.tick, 1u);
        }
    }
    ASSERT(found_city_wide);
}

TEST(city_wide_toxic_not_detected_below_threshold) {
    ContaminationGrid grid(4, 4);
    for (int32_t y = 0; y < 4; ++y) {
        for (int32_t x = 0; x < 4; ++x) {
            grid.add_contamination(x, y, 79, 0);  // Average 79, below 80
        }
    }

    auto events = detect_contamination_events(grid, 1);

    for (const auto& event : events) {
        ASSERT(event.type != ContaminationEventType::CityWideToxic);
    }
}

TEST(city_wide_toxic_with_mixed_levels) {
    ContaminationGrid grid(4, 4);  // 16 cells
    // Total 1280 / 16 = 80 average
    grid.add_contamination(0, 0, 255, 0);
    grid.add_contamination(1, 0, 200, 0);
    grid.add_contamination(2, 0, 150, 0);
    grid.add_contamination(3, 0, 100, 0);
    // Remaining 12 cells: (1280 - 705) / 12 = 575 / 12 ≈ 47.9
    for (int32_t y = 1; y < 4; ++y) {
        for (int32_t x = 0; x < 4; ++x) {
            grid.add_contamination(x, y, 48, 0);
        }
    }
    // Total: 705 + (48 * 12) = 705 + 576 = 1281 / 16 = 80.0625

    auto events = detect_contamination_events(grid, 1);

    bool found_city_wide = false;
    for (const auto& event : events) {
        if (event.type == ContaminationEventType::CityWideToxic) {
            found_city_wide = true;
        }
    }
    ASSERT(found_city_wide);
}

TEST(city_wide_toxic_empty_grid) {
    ContaminationGrid grid(64, 64);
    auto events = detect_contamination_events(grid, 1);

    for (const auto& event : events) {
        ASSERT(event.type != ContaminationEventType::CityWideToxic);
    }
}

// =============================================================================
// Multiple Events Tests
// =============================================================================

TEST(multiple_events_same_tick) {
    ContaminationGrid grid(64, 64);

    // Setup: previous tick has some contamination
    grid.add_contamination(10, 10, 191, 0);  // Will cross toxic threshold
    grid.add_contamination(20, 20, 100, 1);  // Will spike
    grid.add_contamination(30, 30, 200, 2);  // Will clear

    grid.swap_buffers();

    // Current tick changes
    grid.add_contamination(10, 10, 192, 0);  // Current: 192 - toxic warning
    grid.add_contamination(20, 20, 180, 1);  // Current: 180 - spike of 80
    grid.add_contamination(30, 30, 150, 2);  // Current: 150 - cleared

    auto events = detect_contamination_events(grid, 5);

    // Should have at least 3 events (toxic, spike, cleared)
    ASSERT(events.size() >= 3);

    bool has_toxic = false;
    bool has_spike = false;
    bool has_cleared = false;

    for (const auto& event : events) {
        ASSERT_EQ(event.tick, 5u);

        if (event.type == ContaminationEventType::ToxicWarning && event.x == 10) {
            has_toxic = true;
        }
        if (event.type == ContaminationEventType::ContaminationSpike && event.x == 20) {
            has_spike = true;
        }
        if (event.type == ContaminationEventType::ContaminationCleared && event.x == 30) {
            has_cleared = true;
        }
    }

    ASSERT(has_toxic);
    ASSERT(has_spike);
    ASSERT(has_cleared);
}

TEST(multiple_cells_same_event_type) {
    ContaminationGrid grid(64, 64);

    grid.add_contamination(10, 10, 191, 0);  // Previous: 191
    grid.add_contamination(20, 20, 191, 1);  // Previous: 191
    grid.add_contamination(30, 30, 191, 2);  // Previous: 191

    grid.swap_buffers();

    grid.add_contamination(10, 10, 192, 0);  // Current: 192 (cross threshold)
    grid.add_contamination(20, 20, 192, 1);  // Current: 192 (cross threshold)
    grid.add_contamination(30, 30, 192, 2);  // Current: 192 (cross threshold)

    auto events = detect_contamination_events(grid, 1);

    int toxic_count = 0;
    for (const auto& event : events) {
        if (event.type == ContaminationEventType::ToxicWarning) {
            ++toxic_count;
        }
    }

    ASSERT_EQ(toxic_count, 3);
}

// =============================================================================
// Main Entry Point
// =============================================================================

int main() {
    printf("=== ContaminationEvents Unit Tests (Ticket E10-091) ===\n\n");

    // ToxicWarning tests
    RUN_TEST(toxic_warning_no_events_empty_grid);
    RUN_TEST(toxic_warning_detected_crossing_threshold);
    RUN_TEST(toxic_warning_not_detected_below_threshold);
    RUN_TEST(toxic_warning_not_detected_already_above_threshold);
    RUN_TEST(toxic_warning_exact_threshold_boundary);

    // ContaminationSpike tests
    RUN_TEST(spike_detected_exactly_at_threshold);
    RUN_TEST(spike_detected_above_threshold);
    RUN_TEST(spike_not_detected_below_threshold);
    RUN_TEST(spike_not_detected_on_decrease);
    RUN_TEST(spike_captures_dominant_type);

    // ContaminationCleared tests
    RUN_TEST(cleared_detected_dropping_below_threshold);
    RUN_TEST(cleared_not_detected_staying_below_threshold);
    RUN_TEST(cleared_not_detected_staying_above_threshold);
    RUN_TEST(cleared_exact_threshold_boundary);
    RUN_TEST(cleared_captures_previous_dominant_type);

    // CityWideToxic tests
    RUN_TEST(city_wide_toxic_detected_above_threshold);
    RUN_TEST(city_wide_toxic_not_detected_below_threshold);
    RUN_TEST(city_wide_toxic_with_mixed_levels);
    RUN_TEST(city_wide_toxic_empty_grid);

    // Multiple events tests
    RUN_TEST(multiple_events_same_tick);
    RUN_TEST(multiple_cells_same_event_type);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
