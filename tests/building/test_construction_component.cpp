/**
 * @file test_construction_component.cpp
 * @brief Unit tests for ConstructionComponent structure (Ticket 4-004)
 *
 * Tests cover:
 * - ConstructionComponent size verification (12 bytes)
 * - Trivially copyable for serialization
 * - Default initialization
 * - Progress percentage calculation
 * - Phase derivation from progress (0-25%, 25-50%, 50-75%, 75-100%)
 * - Phase progress within each phase (0-255)
 * - Pause behavior
 * - Tick advancement
 */

#include <sims3000/building/BuildingComponents.h>
#include <cstdio>
#include <cstdlib>

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
        printf("\n  FAILED: %s == %s (line %d)\n", #a, #b, __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

// =============================================================================
// ConstructionComponent Size Tests
// =============================================================================

TEST(construction_component_size) {
    // Critical: must be 12 bytes or less
    ASSERT(sizeof(ConstructionComponent) <= 12);
}

TEST(construction_component_trivially_copyable) {
    ASSERT(std::is_trivially_copyable<ConstructionComponent>::value);
}

// =============================================================================
// ConstructionComponent Initialization Tests
// =============================================================================

TEST(construction_component_default_init) {
    ConstructionComponent cc;
    ASSERT_EQ(cc.ticks_total, 100);
    ASSERT_EQ(cc.ticks_elapsed, 0);
    ASSERT_EQ(cc.phase, static_cast<std::uint8_t>(ConstructionPhase::Foundation));
    ASSERT_EQ(cc.phase_progress, 0);
    ASSERT_EQ(cc.construction_cost, 0);
    ASSERT_EQ(cc.is_paused, false);
}

TEST(construction_component_parameterized_init) {
    ConstructionComponent cc(200, 5000);
    ASSERT_EQ(cc.ticks_total, 200);
    ASSERT_EQ(cc.ticks_elapsed, 0);
    ASSERT_EQ(cc.construction_cost, 5000);
    ASSERT_EQ(cc.is_paused, false);
}

// =============================================================================
// Progress Percentage Tests
// =============================================================================

TEST(construction_component_progress_percent) {
    ConstructionComponent cc(100, 0);

    // 0% progress
    cc.ticks_elapsed = 0;
    ASSERT_EQ(cc.getProgressPercent(), 0);

    // 25% progress
    cc.ticks_elapsed = 25;
    ASSERT_EQ(cc.getProgressPercent(), 25);

    // 50% progress
    cc.ticks_elapsed = 50;
    ASSERT_EQ(cc.getProgressPercent(), 50);

    // 75% progress
    cc.ticks_elapsed = 75;
    ASSERT_EQ(cc.getProgressPercent(), 75);

    // 100% progress
    cc.ticks_elapsed = 100;
    ASSERT_EQ(cc.getProgressPercent(), 100);

    // Clamped above 100%
    cc.ticks_elapsed = 150;
    ASSERT_EQ(cc.getProgressPercent(), 100);
}

TEST(construction_component_progress_percent_zero_total) {
    ConstructionComponent cc(0, 0);
    // Edge case: zero total should return 100%
    ASSERT_EQ(cc.getProgressPercent(), 100);
}

// =============================================================================
// Phase Derivation Tests (CCR-011)
// =============================================================================

TEST(construction_component_phase_from_progress) {
    ConstructionComponent cc(100, 0);

    // Foundation: 0-24% (0-24 ticks)
    cc.ticks_elapsed = 0;
    cc.updatePhase();
    ASSERT_EQ(cc.getPhase(), ConstructionPhase::Foundation);

    cc.ticks_elapsed = 24;
    cc.updatePhase();
    ASSERT_EQ(cc.getPhase(), ConstructionPhase::Foundation);

    // Framework: 25-49% (25-49 ticks)
    cc.ticks_elapsed = 25;
    cc.updatePhase();
    ASSERT_EQ(cc.getPhase(), ConstructionPhase::Framework);

    cc.ticks_elapsed = 49;
    cc.updatePhase();
    ASSERT_EQ(cc.getPhase(), ConstructionPhase::Framework);

    // Exterior: 50-74% (50-74 ticks)
    cc.ticks_elapsed = 50;
    cc.updatePhase();
    ASSERT_EQ(cc.getPhase(), ConstructionPhase::Exterior);

    cc.ticks_elapsed = 74;
    cc.updatePhase();
    ASSERT_EQ(cc.getPhase(), ConstructionPhase::Exterior);

    // Finalization: 75-100% (75-100 ticks)
    cc.ticks_elapsed = 75;
    cc.updatePhase();
    ASSERT_EQ(cc.getPhase(), ConstructionPhase::Finalization);

    cc.ticks_elapsed = 100;
    cc.updatePhase();
    ASSERT_EQ(cc.getPhase(), ConstructionPhase::Finalization);
}

TEST(construction_component_phase_progress_within_phase) {
    ConstructionComponent cc(100, 0);

    // Foundation phase: 0% -> phase_progress = 0
    cc.ticks_elapsed = 0;
    cc.updatePhase();
    ASSERT_EQ(cc.phase_progress, 0);

    // Foundation phase: 12% (halfway through 0-25%) -> phase_progress = ~127
    cc.ticks_elapsed = 12;
    cc.updatePhase();
    // 12% within 0-25% phase = 12/25 * 255 = ~122
    ASSERT(cc.phase_progress >= 120 && cc.phase_progress <= 125);

    // Framework phase: 25% -> phase_progress = 0 (start of new phase)
    cc.ticks_elapsed = 25;
    cc.updatePhase();
    ASSERT_EQ(cc.phase_progress, 0);

    // Finalization phase: 100% -> phase_progress = 255 (end of phase)
    cc.ticks_elapsed = 100;
    cc.updatePhase();
    ASSERT_EQ(cc.phase_progress, 255);
}

// =============================================================================
// Tick Advancement Tests
// =============================================================================

TEST(construction_component_tick_advancement) {
    ConstructionComponent cc(10, 0);

    // Advance 5 ticks
    for (int i = 0; i < 5; ++i) {
        ASSERT(cc.tick());
    }
    ASSERT_EQ(cc.ticks_elapsed, 5);
    ASSERT_EQ(cc.getProgressPercent(), 50);

    // Advance to completion
    for (int i = 0; i < 5; ++i) {
        ASSERT(cc.tick());
    }
    ASSERT_EQ(cc.ticks_elapsed, 10);
    ASSERT(cc.isComplete());

    // Cannot advance past completion
    ASSERT(!cc.tick());
    ASSERT_EQ(cc.ticks_elapsed, 10);
}

TEST(construction_component_pause_behavior) {
    ConstructionComponent cc(10, 0);

    // Advance 3 ticks
    cc.tick();
    cc.tick();
    cc.tick();
    ASSERT_EQ(cc.ticks_elapsed, 3);

    // Pause
    cc.setPaused(true);
    ASSERT(cc.isPaused());

    // Ticks should not advance while paused
    ASSERT(!cc.tick());
    ASSERT_EQ(cc.ticks_elapsed, 3);

    ASSERT(!cc.tick());
    ASSERT_EQ(cc.ticks_elapsed, 3);

    // Unpause and continue
    cc.setPaused(false);
    ASSERT(!cc.isPaused());
    ASSERT(cc.tick());
    ASSERT_EQ(cc.ticks_elapsed, 4);
}

TEST(construction_component_completion_check) {
    ConstructionComponent cc(5, 0);

    ASSERT(!cc.isComplete());

    cc.ticks_elapsed = 4;
    ASSERT(!cc.isComplete());

    cc.ticks_elapsed = 5;
    ASSERT(cc.isComplete());

    cc.ticks_elapsed = 6;
    ASSERT(cc.isComplete());
}

// =============================================================================
// Main
// =============================================================================

int main() {
    printf("Running ConstructionComponent tests...\n\n");

    RUN_TEST(construction_component_size);
    RUN_TEST(construction_component_trivially_copyable);
    RUN_TEST(construction_component_default_init);
    RUN_TEST(construction_component_parameterized_init);
    RUN_TEST(construction_component_progress_percent);
    RUN_TEST(construction_component_progress_percent_zero_total);
    RUN_TEST(construction_component_phase_from_progress);
    RUN_TEST(construction_component_phase_progress_within_phase);
    RUN_TEST(construction_component_tick_advancement);
    RUN_TEST(construction_component_pause_behavior);
    RUN_TEST(construction_component_completion_check);

    printf("\n");
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_failed);

    return tests_failed > 0 ? 1 : 0;
}
