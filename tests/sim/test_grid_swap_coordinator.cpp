/**
 * @file test_grid_swap_coordinator.cpp
 * @brief Tests for GridSwapCoordinator (E10-063).
 */

#include <sims3000/sim/GridSwapCoordinator.h>
#include <sims3000/disorder/DisorderGrid.h>
#include <sims3000/contamination/ContaminationGrid.h>

#include <cassert>
#include <cstdio>

static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) \
    static void test_##name(); \
    struct test_##name##_registrar { \
        test_##name##_registrar() { \
            ++tests_run; \
            std::printf("  %-60s ", #name); \
            test_##name(); \
            ++tests_passed; \
            std::printf("PASS\n"); \
        } \
    } test_##name##_instance; \
    static void test_##name()

// ---------------------------------------------------------------------------
// Test: Default construction has no grids registered
// ---------------------------------------------------------------------------
TEST(default_construction_no_grids) {
    sims3000::sim::GridSwapCoordinator coordinator;
    assert(!coordinator.has_disorder_grid());
    assert(!coordinator.has_contamination_grid());
}

// ---------------------------------------------------------------------------
// Test: Register disorder grid, verify has_disorder_grid()
// ---------------------------------------------------------------------------
TEST(register_disorder_grid) {
    sims3000::sim::GridSwapCoordinator coordinator;
    sims3000::disorder::DisorderGrid grid(4, 4);

    coordinator.register_disorder_grid(&grid);
    assert(coordinator.has_disorder_grid());
    assert(!coordinator.has_contamination_grid());
}

// ---------------------------------------------------------------------------
// Test: Register contamination grid, verify has_contamination_grid()
// ---------------------------------------------------------------------------
TEST(register_contamination_grid) {
    sims3000::sim::GridSwapCoordinator coordinator;
    sims3000::contamination::ContaminationGrid grid(4, 4);

    coordinator.register_contamination_grid(&grid);
    assert(!coordinator.has_disorder_grid());
    assert(coordinator.has_contamination_grid());
}

// ---------------------------------------------------------------------------
// Test: Unregister grids by passing nullptr
// ---------------------------------------------------------------------------
TEST(unregister_grids_with_nullptr) {
    sims3000::sim::GridSwapCoordinator coordinator;
    sims3000::disorder::DisorderGrid d_grid(4, 4);
    sims3000::contamination::ContaminationGrid c_grid(4, 4);

    coordinator.register_disorder_grid(&d_grid);
    coordinator.register_contamination_grid(&c_grid);
    assert(coordinator.has_disorder_grid());
    assert(coordinator.has_contamination_grid());

    coordinator.register_disorder_grid(nullptr);
    assert(!coordinator.has_disorder_grid());
    assert(coordinator.has_contamination_grid());

    coordinator.register_contamination_grid(nullptr);
    assert(!coordinator.has_disorder_grid());
    assert(!coordinator.has_contamination_grid());
}

// ---------------------------------------------------------------------------
// Test: swap_all() with both grids - data moves from current to previous
// ---------------------------------------------------------------------------
TEST(swap_all_both_grids) {
    sims3000::sim::GridSwapCoordinator coordinator;
    sims3000::disorder::DisorderGrid d_grid(4, 4);
    sims3000::contamination::ContaminationGrid c_grid(4, 4);

    coordinator.register_disorder_grid(&d_grid);
    coordinator.register_contamination_grid(&c_grid);

    // Write values to current buffers
    d_grid.set_level(1, 1, 100);
    c_grid.set_level(2, 2, 200);

    // Before swap: current has data, previous is empty
    assert(d_grid.get_level(1, 1) == 100);
    assert(d_grid.get_level_previous_tick(1, 1) == 0);
    assert(c_grid.get_level(2, 2) == 200);
    assert(c_grid.get_level_previous_tick(2, 2) == 0);

    // Perform coordinated swap
    coordinator.swap_all();

    // After swap: previous now has the data, current is the old previous (empty)
    assert(d_grid.get_level_previous_tick(1, 1) == 100);
    assert(d_grid.get_level(1, 1) == 0);
    assert(c_grid.get_level_previous_tick(2, 2) == 200);
    assert(c_grid.get_level(2, 2) == 0);
}

// ---------------------------------------------------------------------------
// Test: swap_all() with only disorder grid registered (partial swap)
// ---------------------------------------------------------------------------
TEST(swap_all_only_disorder_grid) {
    sims3000::sim::GridSwapCoordinator coordinator;
    sims3000::disorder::DisorderGrid d_grid(4, 4);
    sims3000::contamination::ContaminationGrid c_grid(4, 4);

    // Only register disorder grid
    coordinator.register_disorder_grid(&d_grid);

    // Write values to both grids
    d_grid.set_level(0, 0, 50);
    c_grid.set_level(0, 0, 75);

    // Swap via coordinator - should only swap disorder
    coordinator.swap_all();

    // Disorder should have swapped
    assert(d_grid.get_level_previous_tick(0, 0) == 50);
    assert(d_grid.get_level(0, 0) == 0);

    // Contamination should NOT have swapped (not registered)
    assert(c_grid.get_level(0, 0) == 75);
    assert(c_grid.get_level_previous_tick(0, 0) == 0);
}

// ---------------------------------------------------------------------------
// Test: swap_all() with only contamination grid registered (partial swap)
// ---------------------------------------------------------------------------
TEST(swap_all_only_contamination_grid) {
    sims3000::sim::GridSwapCoordinator coordinator;
    sims3000::disorder::DisorderGrid d_grid(4, 4);
    sims3000::contamination::ContaminationGrid c_grid(4, 4);

    // Only register contamination grid
    coordinator.register_contamination_grid(&c_grid);

    // Write values to both grids
    d_grid.set_level(0, 0, 50);
    c_grid.set_level(0, 0, 75);

    // Swap via coordinator - should only swap contamination
    coordinator.swap_all();

    // Disorder should NOT have swapped (not registered)
    assert(d_grid.get_level(0, 0) == 50);
    assert(d_grid.get_level_previous_tick(0, 0) == 0);

    // Contamination should have swapped
    assert(c_grid.get_level_previous_tick(0, 0) == 75);
    assert(c_grid.get_level(0, 0) == 0);
}

// ---------------------------------------------------------------------------
// Test: swap_all() with no grids registered - should not crash
// ---------------------------------------------------------------------------
TEST(swap_all_no_grids_no_crash) {
    sims3000::sim::GridSwapCoordinator coordinator;
    // Should be a safe no-op
    coordinator.swap_all();
    coordinator.swap_all();
    coordinator.swap_all();
    // If we get here without crashing, the test passes
}

// ---------------------------------------------------------------------------
// Test: Multiple swap cycles maintain correct double-buffer semantics
// ---------------------------------------------------------------------------
TEST(multiple_swap_cycles) {
    sims3000::sim::GridSwapCoordinator coordinator;
    sims3000::disorder::DisorderGrid d_grid(4, 4);

    coordinator.register_disorder_grid(&d_grid);

    // Tick 1: write value
    d_grid.set_level(0, 0, 10);
    assert(d_grid.get_level(0, 0) == 10);

    // Tick 2: swap, then write new value
    coordinator.swap_all();
    assert(d_grid.get_level_previous_tick(0, 0) == 10);
    assert(d_grid.get_level(0, 0) == 0);
    d_grid.set_level(0, 0, 20);
    assert(d_grid.get_level(0, 0) == 20);

    // Tick 3: swap again
    coordinator.swap_all();
    assert(d_grid.get_level_previous_tick(0, 0) == 20);
    assert(d_grid.get_level(0, 0) == 10); // old previous becomes current
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------
int main() {
    std::printf("GridSwapCoordinator tests (E10-063)\n");
    std::printf("====================================\n");

    // Tests are auto-registered and run via static initialization above.

    std::printf("====================================\n");
    std::printf("Results: %d/%d passed\n", tests_passed, tests_run);

    return (tests_passed == tests_run) ? 0 : 1;
}
