/**
 * @file test_population_milestones.cpp
 * @brief Tests for population milestone detection (Ticket E10-031)
 *
 * Validates:
 * - Single milestone crossing (upgrade)
 * - Single milestone crossing (downgrade)
 * - Multiple milestone crossings in one jump
 * - No crossing when staying in same range
 * - Crossing at exact threshold
 * - get_milestone_level correctness
 * - Milestone name strings
 */

#include <cassert>
#include <cstdio>
#include <cstring>

#include "sims3000/population/PopulationMilestones.h"

using namespace sims3000::population;

// --------------------------------------------------------------------------
// Test: Single milestone upgrade
// --------------------------------------------------------------------------
static void test_single_upgrade() {
    // Grow from 50 to 150 (cross Village threshold)
    auto events = check_milestones(50, 150);

    assert(events.size() == 1 && "Should detect 1 milestone crossing");
    assert(events[0].type == MilestoneType::Village && "Should be Village milestone");
    assert(events[0].population == 150 && "Population should be 150");
    assert(events[0].is_upgrade == true && "Should be an upgrade");

    std::printf("  PASS: Single milestone upgrade\n");
}

// --------------------------------------------------------------------------
// Test: Single milestone downgrade
// --------------------------------------------------------------------------
static void test_single_downgrade() {
    // Shrink from 600 to 400 (fall below Town threshold)
    auto events = check_milestones(600, 400);

    assert(events.size() == 1 && "Should detect 1 milestone crossing");
    assert(events[0].type == MilestoneType::Town && "Should be Town milestone");
    assert(events[0].population == 400 && "Population should be 400");
    assert(events[0].is_upgrade == false && "Should be a downgrade");

    std::printf("  PASS: Single milestone downgrade\n");
}

// --------------------------------------------------------------------------
// Test: Multiple milestone upgrades in one jump
// --------------------------------------------------------------------------
static void test_multiple_upgrades() {
    // Jump from 50 to 3000 (cross Village, Town, and City)
    auto events = check_milestones(50, 3000);

    assert(events.size() == 3 && "Should detect 3 milestone crossings");

    // Should be in order: Village, Town, City
    assert(events[0].type == MilestoneType::Village && "First should be Village");
    assert(events[1].type == MilestoneType::Town && "Second should be Town");
    assert(events[2].type == MilestoneType::City && "Third should be City");

    // All should be upgrades
    assert(events[0].is_upgrade && "Village should be upgrade");
    assert(events[1].is_upgrade && "Town should be upgrade");
    assert(events[2].is_upgrade && "City should be upgrade");

    std::printf("  PASS: Multiple milestone upgrades\n");
}

// --------------------------------------------------------------------------
// Test: Multiple milestone downgrades in one jump
// --------------------------------------------------------------------------
static void test_multiple_downgrades() {
    // Crash from 15000 to 1000 (fall below Metropolis, City, Town)
    auto events = check_milestones(15000, 1000);

    assert(events.size() == 3 && "Should detect 3 milestone crossings");

    // Should be in order: Village, Town, City (iteration order)
    assert(events[0].type == MilestoneType::Village && "First should be Village");
    assert(events[1].type == MilestoneType::Town && "Second should be Town");
    assert(events[2].type == MilestoneType::City && "Third should be City");

    // All should be downgrades
    assert(!events[0].is_upgrade && "Village should be downgrade");
    assert(!events[1].is_upgrade && "Town should be downgrade");
    assert(!events[2].is_upgrade && "City should be downgrade");

    std::printf("  PASS: Multiple milestone downgrades\n");
}

// --------------------------------------------------------------------------
// Test: No crossing when staying in same range
// --------------------------------------------------------------------------
static void test_no_crossing() {
    // Stay between Village and Town (100-499)
    auto events = check_milestones(200, 300);
    assert(events.size() == 0 && "Should detect no crossings");

    // Stay above Megalopolis
    events = check_milestones(60000, 70000);
    assert(events.size() == 0 && "Should detect no crossings in high range");

    // No change at all
    events = check_milestones(1000, 1000);
    assert(events.size() == 0 && "Should detect no crossings when unchanged");

    std::printf("  PASS: No crossing when staying in same range\n");
}

// --------------------------------------------------------------------------
// Test: Crossing at exact threshold
// --------------------------------------------------------------------------
static void test_exact_threshold() {
    // Cross exactly at Village threshold (99 -> 100)
    auto events = check_milestones(99, 100);
    assert(events.size() == 1 && "Should detect crossing at exact threshold");
    assert(events[0].type == MilestoneType::Village && "Should be Village");
    assert(events[0].is_upgrade && "Should be upgrade");

    // Fall exactly at Town threshold (500 -> 499)
    events = check_milestones(500, 499);
    assert(events.size() == 1 && "Should detect falling at exact threshold");
    assert(events[0].type == MilestoneType::Town && "Should be Town");
    assert(!events[0].is_upgrade && "Should be downgrade");

    std::printf("  PASS: Crossing at exact threshold\n");
}

// --------------------------------------------------------------------------
// Test: get_milestone_level correctness
// --------------------------------------------------------------------------
static void test_get_milestone_level() {
    // Below all thresholds (should still return Village)
    assert(get_milestone_level(0) == MilestoneType::Village && "0 should be Village level");
    assert(get_milestone_level(50) == MilestoneType::Village && "50 should be Village level");

    // At and above Village
    assert(get_milestone_level(100) == MilestoneType::Village && "100 should be Village level");
    assert(get_milestone_level(200) == MilestoneType::Village && "200 should be Village level");

    // At and above Town
    assert(get_milestone_level(500) == MilestoneType::Town && "500 should be Town level");
    assert(get_milestone_level(1000) == MilestoneType::Town && "1000 should be Town level");

    // At and above City
    assert(get_milestone_level(2000) == MilestoneType::City && "2000 should be City level");
    assert(get_milestone_level(5000) == MilestoneType::City && "5000 should be City level");

    // At and above Metropolis
    assert(get_milestone_level(10000) == MilestoneType::Metropolis && "10000 should be Metropolis level");
    assert(get_milestone_level(25000) == MilestoneType::Metropolis && "25000 should be Metropolis level");

    // At and above Megalopolis
    assert(get_milestone_level(50000) == MilestoneType::Megalopolis && "50000 should be Megalopolis level");
    assert(get_milestone_level(100000) == MilestoneType::Megalopolis && "100000 should be Megalopolis level");

    std::printf("  PASS: get_milestone_level correctness\n");
}

// --------------------------------------------------------------------------
// Test: Milestone names
// --------------------------------------------------------------------------
static void test_milestone_names() {
    assert(std::strcmp(get_milestone_name(MilestoneType::Village), "Village") == 0
        && "Village name should be correct");
    assert(std::strcmp(get_milestone_name(MilestoneType::Town), "Town") == 0
        && "Town name should be correct");
    assert(std::strcmp(get_milestone_name(MilestoneType::City), "City") == 0
        && "City name should be correct");
    assert(std::strcmp(get_milestone_name(MilestoneType::Metropolis), "Metropolis") == 0
        && "Metropolis name should be correct");
    assert(std::strcmp(get_milestone_name(MilestoneType::Megalopolis), "Megalopolis") == 0
        && "Megalopolis name should be correct");

    std::printf("  PASS: Milestone names\n");
}

// --------------------------------------------------------------------------
// Test: Milestone thresholds
// --------------------------------------------------------------------------
static void test_milestone_thresholds() {
    assert(get_milestone_threshold(MilestoneType::Village) == 100 && "Village threshold should be 100");
    assert(get_milestone_threshold(MilestoneType::Town) == 500 && "Town threshold should be 500");
    assert(get_milestone_threshold(MilestoneType::City) == 2000 && "City threshold should be 2000");
    assert(get_milestone_threshold(MilestoneType::Metropolis) == 10000 && "Metropolis threshold should be 10000");
    assert(get_milestone_threshold(MilestoneType::Megalopolis) == 50000 && "Megalopolis threshold should be 50000");

    std::printf("  PASS: Milestone thresholds\n");
}

// --------------------------------------------------------------------------
// Test: Edge case - jump to exactly next milestone
// --------------------------------------------------------------------------
static void test_jump_to_next_milestone() {
    // From just below Village to exactly Town
    auto events = check_milestones(99, 500);
    assert(events.size() == 2 && "Should cross Village and Town");
    assert(events[0].type == MilestoneType::Village && "First should be Village");
    assert(events[1].type == MilestoneType::Town && "Second should be Town");

    std::printf("  PASS: Jump to next milestone\n");
}

// --------------------------------------------------------------------------
// Test: Edge case - massive population jump
// --------------------------------------------------------------------------
static void test_massive_jump() {
    // From 0 to max milestone
    auto events = check_milestones(0, 100000);
    assert(events.size() == 5 && "Should cross all 5 milestones");

    // Verify all are upgrades and in order
    for (size_t i = 0; i < events.size(); ++i) {
        assert(events[i].type == static_cast<MilestoneType>(i) && "Should be in order");
        assert(events[i].is_upgrade && "All should be upgrades");
    }

    std::printf("  PASS: Massive population jump\n");
}

// --------------------------------------------------------------------------
// Test: Edge case - complete population collapse
// --------------------------------------------------------------------------
static void test_complete_collapse() {
    // From max to 0
    auto events = check_milestones(100000, 0);
    assert(events.size() == 5 && "Should cross all 5 milestones downward");

    // All should be downgrades
    for (const auto& event : events) {
        assert(!event.is_upgrade && "All should be downgrades");
    }

    std::printf("  PASS: Complete population collapse\n");
}

// --------------------------------------------------------------------------
// Main
// --------------------------------------------------------------------------
int main() {
    std::printf("=== Population Milestone Tests (E10-031) ===\n");

    test_single_upgrade();
    test_single_downgrade();
    test_multiple_upgrades();
    test_multiple_downgrades();
    test_no_crossing();
    test_exact_threshold();
    test_get_milestone_level();
    test_milestone_names();
    test_milestone_thresholds();
    test_jump_to_next_milestone();
    test_massive_jump();
    test_complete_collapse();

    std::printf("All population milestone tests passed.\n");
    return 0;
}
