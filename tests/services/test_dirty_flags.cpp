/**
 * @file test_dirty_flags.cpp
 * @brief Unit tests for dirty flag tracking and recalculation triggers (Epic 9, Ticket E9-011)
 *
 * Tests cover:
 * - mark_dirty / is_dirty per type per player
 * - mark_all_dirty marks all service types for a player
 * - isCoverageDirty() aggregate check
 * - recalculate_if_dirty() clears dirty flags
 * - Event handlers (constructed/deconstructed/power_changed) set dirty flags
 * - tick() triggers recalculation
 * - Lazy grid allocation on first recalculation
 * - Bounds checking on invalid player/type
 */

#include <sims3000/services/ServicesSystem.h>
#include <sims3000/services/ServiceCoverageGrid.h>
#include <sims3000/services/ServiceTypes.h>
#include <sims3000/core/ISimulationTime.h>
#include <cassert>
#include <cstdio>

using namespace sims3000::services;

// =============================================================================
// Mock ISimulationTime for tick() tests
// =============================================================================

class MockSimulationTime : public sims3000::ISimulationTime {
public:
    sims3000::SimulationTick getCurrentTick() const override { return m_tick; }
    float getTickDelta() const override { return 0.05f; }
    float getInterpolation() const override { return 0.0f; }
    double getTotalTime() const override { return static_cast<double>(m_tick) * 0.05; }

    sims3000::SimulationTick m_tick = 0;
};

// =============================================================================
// mark_dirty / is_dirty tests
// =============================================================================

void test_initial_not_dirty() {
    printf("Testing initial state: no dirty flags...\n");

    ServicesSystem sys;
    sys.init(64, 64);

    assert(!sys.is_dirty(ServiceType::Enforcer, 0));
    assert(!sys.is_dirty(ServiceType::HazardResponse, 0));
    assert(!sys.is_dirty(ServiceType::Medical, 0));
    assert(!sys.is_dirty(ServiceType::Education, 0));
    assert(!sys.isCoverageDirty());

    sys.cleanup();
    printf("  PASS: Initial state has no dirty flags\n");
}

void test_mark_dirty_single_type() {
    printf("Testing mark_dirty for a single type...\n");

    ServicesSystem sys;
    sys.init(64, 64);

    sys.mark_dirty(ServiceType::Enforcer, 0);

    assert(sys.is_dirty(ServiceType::Enforcer, 0));
    assert(!sys.is_dirty(ServiceType::HazardResponse, 0));
    assert(!sys.is_dirty(ServiceType::Medical, 0));
    assert(!sys.is_dirty(ServiceType::Education, 0));
    assert(sys.isCoverageDirty());

    sys.cleanup();
    printf("  PASS: mark_dirty marks only the specified type\n");
}

void test_mark_dirty_different_players() {
    printf("Testing mark_dirty for different players...\n");

    ServicesSystem sys;
    sys.init(64, 64);

    sys.mark_dirty(ServiceType::Enforcer, 0);
    sys.mark_dirty(ServiceType::Medical, 2);

    assert(sys.is_dirty(ServiceType::Enforcer, 0));
    assert(!sys.is_dirty(ServiceType::Enforcer, 1));
    assert(!sys.is_dirty(ServiceType::Enforcer, 2));

    assert(!sys.is_dirty(ServiceType::Medical, 0));
    assert(sys.is_dirty(ServiceType::Medical, 2));

    sys.cleanup();
    printf("  PASS: Dirty flags are per-player\n");
}

void test_mark_all_dirty() {
    printf("Testing mark_all_dirty marks all types for one player...\n");

    ServicesSystem sys;
    sys.init(64, 64);

    sys.mark_all_dirty(1);

    // Player 1 should have all types dirty
    assert(sys.is_dirty(ServiceType::Enforcer, 1));
    assert(sys.is_dirty(ServiceType::HazardResponse, 1));
    assert(sys.is_dirty(ServiceType::Medical, 1));
    assert(sys.is_dirty(ServiceType::Education, 1));

    // Player 0 should not be affected
    assert(!sys.is_dirty(ServiceType::Enforcer, 0));
    assert(!sys.is_dirty(ServiceType::HazardResponse, 0));

    sys.cleanup();
    printf("  PASS: mark_all_dirty only affects specified player\n");
}

// =============================================================================
// Invalid inputs
// =============================================================================

void test_mark_dirty_invalid_player() {
    printf("Testing mark_dirty with invalid player ID...\n");

    ServicesSystem sys;
    sys.init(64, 64);

    // Should not crash
    sys.mark_dirty(ServiceType::Enforcer, 255);
    sys.mark_all_dirty(255);

    assert(!sys.isCoverageDirty());

    sys.cleanup();
    printf("  PASS: Invalid player ID is safely ignored\n");
}

void test_is_dirty_invalid_player() {
    printf("Testing is_dirty with invalid player ID...\n");

    ServicesSystem sys;
    sys.init(64, 64);

    assert(!sys.is_dirty(ServiceType::Enforcer, 255));

    sys.cleanup();
    printf("  PASS: is_dirty returns false for invalid player\n");
}

// =============================================================================
// Recalculation clears dirty flags
// =============================================================================

void test_recalculate_clears_dirty() {
    printf("Testing recalculate_if_dirty clears dirty flags...\n");

    ServicesSystem sys;
    sys.init(64, 64);

    sys.mark_dirty(ServiceType::Enforcer, 0);
    sys.mark_dirty(ServiceType::Medical, 1);
    assert(sys.isCoverageDirty());

    sys.recalculate_if_dirty();

    assert(!sys.is_dirty(ServiceType::Enforcer, 0));
    assert(!sys.is_dirty(ServiceType::Medical, 1));
    assert(!sys.isCoverageDirty());

    sys.cleanup();
    printf("  PASS: recalculate_if_dirty clears dirty flags\n");
}

void test_recalculate_only_dirty() {
    printf("Testing recalculate only processes dirty grids...\n");

    ServicesSystem sys;
    sys.init(64, 64);

    // Mark only Enforcer for player 0 as dirty
    sys.mark_dirty(ServiceType::Enforcer, 0);

    sys.recalculate_if_dirty();

    // Enforcer grid should now exist (lazy allocated)
    assert(sys.get_coverage_grid(ServiceType::Enforcer, 0) != nullptr);

    // Medical grid should NOT exist (was never dirty)
    assert(sys.get_coverage_grid(ServiceType::Medical, 0) == nullptr);

    sys.cleanup();
    printf("  PASS: Only dirty grids are allocated/recalculated\n");
}

// =============================================================================
// Lazy grid allocation
// =============================================================================

void test_lazy_grid_allocation() {
    printf("Testing lazy grid allocation on first recalculation...\n");

    ServicesSystem sys;
    sys.init(128, 128);

    // Before any dirty marking, no grids should exist
    assert(sys.get_coverage_grid(ServiceType::Enforcer, 0) == nullptr);

    // Mark dirty and recalculate
    sys.mark_dirty(ServiceType::Enforcer, 0);
    sys.recalculate_if_dirty();

    // Grid should now exist with correct dimensions
    ServiceCoverageGrid* grid = sys.get_coverage_grid(ServiceType::Enforcer, 0);
    assert(grid != nullptr);
    assert(grid->get_width() == 128);
    assert(grid->get_height() == 128);

    sys.cleanup();
    printf("  PASS: Grid is lazily allocated on first recalculation\n");
}

void test_grid_persists_across_recalculations() {
    printf("Testing grid persists across multiple recalculations...\n");

    ServicesSystem sys;
    sys.init(64, 64);

    sys.mark_dirty(ServiceType::Enforcer, 0);
    sys.recalculate_if_dirty();
    ServiceCoverageGrid* grid1 = sys.get_coverage_grid(ServiceType::Enforcer, 0);
    assert(grid1 != nullptr);

    // Mark dirty again and recalculate
    sys.mark_dirty(ServiceType::Enforcer, 0);
    sys.recalculate_if_dirty();
    ServiceCoverageGrid* grid2 = sys.get_coverage_grid(ServiceType::Enforcer, 0);
    assert(grid2 != nullptr);

    // Should be the same grid object (not reallocated)
    assert(grid1 == grid2);

    sys.cleanup();
    printf("  PASS: Grid persists across recalculations\n");
}

// =============================================================================
// Event handlers set dirty flags
// =============================================================================

void test_building_constructed_sets_dirty() {
    printf("Testing on_building_constructed marks dirty...\n");

    ServicesSystem sys;
    sys.init(64, 64);

    assert(!sys.isCoverageDirty());

    sys.on_building_constructed(1, 0);

    assert(sys.isCoverageDirty());
    // Should mark all types dirty for player 0
    assert(sys.is_dirty(ServiceType::Enforcer, 0));
    assert(sys.is_dirty(ServiceType::HazardResponse, 0));
    assert(sys.is_dirty(ServiceType::Medical, 0));
    assert(sys.is_dirty(ServiceType::Education, 0));

    // Player 1 should not be affected
    assert(!sys.is_dirty(ServiceType::Enforcer, 1));

    sys.cleanup();
    printf("  PASS: on_building_constructed marks all types dirty for player\n");
}

void test_building_deconstructed_sets_dirty() {
    printf("Testing on_building_deconstructed marks dirty...\n");

    ServicesSystem sys;
    sys.init(64, 64);

    // Add and then remove a building
    sys.on_building_constructed(1, 0);

    // Clear dirty flags via recalculation
    sys.recalculate_if_dirty();
    assert(!sys.isCoverageDirty());

    // Deconstruct the building
    sys.on_building_deconstructed(1, 0);

    assert(sys.isCoverageDirty());
    assert(sys.is_dirty(ServiceType::Enforcer, 0));

    sys.cleanup();
    printf("  PASS: on_building_deconstructed marks dirty\n");
}

void test_power_changed_sets_dirty() {
    printf("Testing on_building_power_changed marks dirty...\n");

    ServicesSystem sys;
    sys.init(64, 64);

    assert(!sys.isCoverageDirty());

    sys.on_building_power_changed(1, 2);

    assert(sys.isCoverageDirty());
    assert(sys.is_dirty(ServiceType::Enforcer, 2));
    assert(sys.is_dirty(ServiceType::HazardResponse, 2));

    sys.cleanup();
    printf("  PASS: on_building_power_changed marks dirty\n");
}

// =============================================================================
// tick() triggers recalculation
// =============================================================================

void test_tick_recalculates_dirty() {
    printf("Testing tick() triggers recalculation of dirty grids...\n");

    ServicesSystem sys;
    sys.init(64, 64);

    sys.mark_dirty(ServiceType::Enforcer, 0);
    assert(sys.isCoverageDirty());

    MockSimulationTime time;
    time.m_tick = 1;
    sys.tick(time);

    // After tick, dirty flags should be cleared
    assert(!sys.isCoverageDirty());
    assert(!sys.is_dirty(ServiceType::Enforcer, 0));

    // Grid should have been allocated
    assert(sys.get_coverage_grid(ServiceType::Enforcer, 0) != nullptr);

    sys.cleanup();
    printf("  PASS: tick() recalculates dirty grids\n");
}

void test_tick_no_recalculation_when_clean() {
    printf("Testing tick() does nothing when not dirty...\n");

    ServicesSystem sys;
    sys.init(64, 64);

    assert(!sys.isCoverageDirty());

    MockSimulationTime time;
    time.m_tick = 1;
    sys.tick(time);

    // No grids should be allocated (nothing was dirty)
    assert(sys.get_coverage_grid(ServiceType::Enforcer, 0) == nullptr);
    assert(!sys.isCoverageDirty());

    sys.cleanup();
    printf("  PASS: tick() does nothing when clean\n");
}

// =============================================================================
// Cleanup resets everything
// =============================================================================

void test_cleanup_resets_dirty_flags() {
    printf("Testing cleanup resets dirty flags and grids...\n");

    ServicesSystem sys;
    sys.init(64, 64);

    sys.mark_all_dirty(0);
    sys.recalculate_if_dirty();
    assert(sys.get_coverage_grid(ServiceType::Enforcer, 0) != nullptr);

    sys.cleanup();

    // After cleanup, everything should be reset
    assert(!sys.isCoverageDirty());
    assert(sys.get_coverage_grid(ServiceType::Enforcer, 0) == nullptr);

    printf("  PASS: cleanup resets all dirty flags and grids\n");
}

void test_reinit_after_cleanup() {
    printf("Testing re-init after cleanup...\n");

    ServicesSystem sys;
    sys.init(64, 64);
    sys.mark_dirty(ServiceType::Enforcer, 0);
    sys.recalculate_if_dirty();
    sys.cleanup();

    // Re-init with different dimensions
    sys.init(128, 128);
    assert(!sys.isCoverageDirty());

    sys.mark_dirty(ServiceType::Medical, 1);
    sys.recalculate_if_dirty();

    ServiceCoverageGrid* grid = sys.get_coverage_grid(ServiceType::Medical, 1);
    assert(grid != nullptr);
    assert(grid->get_width() == 128);
    assert(grid->get_height() == 128);

    sys.cleanup();
    printf("  PASS: Re-init after cleanup works correctly\n");
}

// =============================================================================
// Main
// =============================================================================

int main() {
    printf("=== Dirty Flags Unit Tests (Epic 9, Ticket E9-011) ===\n\n");

    // mark_dirty / is_dirty
    test_initial_not_dirty();
    test_mark_dirty_single_type();
    test_mark_dirty_different_players();
    test_mark_all_dirty();

    // Invalid inputs
    test_mark_dirty_invalid_player();
    test_is_dirty_invalid_player();

    // Recalculation
    test_recalculate_clears_dirty();
    test_recalculate_only_dirty();

    // Lazy allocation
    test_lazy_grid_allocation();
    test_grid_persists_across_recalculations();

    // Event handlers
    test_building_constructed_sets_dirty();
    test_building_deconstructed_sets_dirty();
    test_power_changed_sets_dirty();

    // tick()
    test_tick_recalculates_dirty();
    test_tick_no_recalculation_when_clean();

    // Cleanup
    test_cleanup_resets_dirty_flags();
    test_reinit_after_cleanup();

    printf("\n=== All Dirty Flags Tests Passed ===\n");
    return 0;
}
