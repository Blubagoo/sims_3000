/**
 * @file test_conduit_preview.cpp
 * @brief Unit tests for conduit placement preview - coverage delta (Ticket 5-031)
 *
 * Tests cover:
 * - Isolated conduit (no adjacent conduit/nexus) returns empty
 * - Connected conduit adjacent to nexus returns coverage delta
 * - Connected conduit adjacent to conduit returns coverage delta
 * - Coverage delta excludes already-covered tiles
 * - Out-of-bounds position returns empty
 * - Invalid owner returns empty
 * - Edge/corner clamping: conduit near map boundary
 * - Full overlap: all tiles already covered returns empty
 * - Multiple adjacencies still work (not double-counted)
 */

#include <sims3000/energy/EnergySystem.h>
#include <sims3000/energy/EnergyConduitComponent.h>
#include <sims3000/energy/EnergyProducerComponent.h>
#include <sims3000/energy/NexusTypeConfig.h>
#include <entt/entt.hpp>
#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <set>

using namespace sims3000::energy;

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
// Helper: check if a (x,y) pair exists in the delta vector
// =============================================================================

static bool delta_contains(const std::vector<std::pair<uint32_t, uint32_t>>& delta,
                           uint32_t x, uint32_t y) {
    for (const auto& p : delta) {
        if (p.first == x && p.second == y) {
            return true;
        }
    }
    return false;
}

// =============================================================================
// Isolated conduit: no adjacent conduit or nexus => empty
// =============================================================================

TEST(isolated_conduit_returns_empty) {
    EnergySystem sys(64, 64);
    entt::registry registry;
    sys.set_registry(&registry);

    // No nexus or conduit placed at all. Preview at (30, 30) for player 0.
    auto delta = sys.preview_conduit_coverage(30, 30, 0);
    ASSERT(delta.empty());
}

TEST(conduit_not_adjacent_to_network_returns_empty) {
    EnergySystem sys(64, 64);
    entt::registry registry;
    sys.set_registry(&registry);

    // Place a nexus at (10, 10), try to preview conduit at (30, 30) - not adjacent
    sys.place_nexus(NexusType::Carbon, 10, 10, 0);

    auto delta = sys.preview_conduit_coverage(30, 30, 0);
    ASSERT(delta.empty());
}

// =============================================================================
// Connected conduit adjacent to nexus => returns coverage delta
// =============================================================================

TEST(connected_to_nexus_returns_delta) {
    EnergySystem sys(64, 64);
    entt::registry registry;
    sys.set_registry(&registry);

    // Place nexus at (20, 20), preview conduit at (21, 20) - adjacent right
    sys.place_nexus(NexusType::Carbon, 20, 20, 0);

    auto delta = sys.preview_conduit_coverage(21, 20, 0);
    // Conduit at (21, 20) with radius=3 covers [18..24] x [17..23] = 7x7 = 49 tiles
    // Some of these may already be covered by the nexus.
    // The delta should be non-empty since the conduit extends coverage.
    ASSERT(!delta.empty());
}

TEST(connected_to_nexus_left) {
    EnergySystem sys(64, 64);
    entt::registry registry;
    sys.set_registry(&registry);

    // Place nexus at (20, 20), preview at (19, 20) - adjacent left
    sys.place_nexus(NexusType::Carbon, 20, 20, 0);

    auto delta = sys.preview_conduit_coverage(19, 20, 0);
    ASSERT(!delta.empty());
}

TEST(connected_to_nexus_above) {
    EnergySystem sys(64, 64);
    entt::registry registry;
    sys.set_registry(&registry);

    // Place nexus at (20, 20), preview at (20, 19) - adjacent above
    sys.place_nexus(NexusType::Carbon, 20, 20, 0);

    auto delta = sys.preview_conduit_coverage(20, 19, 0);
    ASSERT(!delta.empty());
}

TEST(connected_to_nexus_below) {
    EnergySystem sys(64, 64);
    entt::registry registry;
    sys.set_registry(&registry);

    // Place nexus at (20, 20), preview at (20, 21) - adjacent below
    sys.place_nexus(NexusType::Carbon, 20, 20, 0);

    auto delta = sys.preview_conduit_coverage(20, 21, 0);
    ASSERT(!delta.empty());
}

// =============================================================================
// Connected conduit adjacent to existing conduit => returns delta
// =============================================================================

TEST(connected_to_conduit_returns_delta) {
    EnergySystem sys(64, 64);
    entt::registry registry;
    sys.set_registry(&registry);

    // Place a conduit at (30, 30)
    sys.place_conduit(30, 30, 0);

    // Preview conduit at (31, 30) - adjacent to existing conduit
    auto delta = sys.preview_conduit_coverage(31, 30, 0);
    ASSERT(!delta.empty());
}

// =============================================================================
// Coverage delta excludes already-covered tiles
// =============================================================================

TEST(delta_excludes_already_covered_tiles) {
    EnergySystem sys(64, 64);
    entt::registry registry;
    sys.set_registry(&registry);

    // Place nexus at (20, 20) for player 0
    sys.place_nexus(NexusType::Carbon, 20, 20, 0);
    // Recalculate coverage so the nexus coverage is established
    sys.recalculate_coverage(0);

    // Preview conduit at (21, 20) - adjacent right
    auto delta = sys.preview_conduit_coverage(21, 20, 0);

    // All tiles in delta must NOT already be covered by owner 0
    uint8_t overseer_id = 0 + 1;
    for (const auto& p : delta) {
        ASSERT(sys.get_coverage_at(p.first, p.second) != overseer_id);
    }
}

TEST(delta_does_not_contain_duplicates) {
    EnergySystem sys(64, 64);
    entt::registry registry;
    sys.set_registry(&registry);

    // Place nexus at (20, 20) for player 0
    sys.place_nexus(NexusType::Carbon, 20, 20, 0);
    sys.recalculate_coverage(0);

    auto delta = sys.preview_conduit_coverage(21, 20, 0);

    // Check no duplicates
    std::set<std::pair<uint32_t, uint32_t>> unique_tiles(delta.begin(), delta.end());
    ASSERT_EQ(unique_tiles.size(), delta.size());
}

// =============================================================================
// Full overlap: all tiles in conduit radius already covered => empty delta
// =============================================================================

TEST(full_overlap_returns_empty_delta) {
    EnergySystem sys(64, 64);
    entt::registry registry;
    sys.set_registry(&registry);

    // Place nexus at (20, 20) - Carbon has coverage_radius = 8
    // Conduit radius = 3. If we place conduit at (21, 20),
    // its coverage [18..24]x[17..23] is entirely within nexus coverage [12..28]x[12..28].
    sys.place_nexus(NexusType::Carbon, 20, 20, 0);
    sys.recalculate_coverage(0);

    // Conduit at (21, 20) with radius=3: covers [18,24]x[17,23]
    // Nexus at (20, 20) with radius=8: covers [12,28]x[12,28]
    // All conduit tiles are already covered by nexus => empty delta
    auto delta = sys.preview_conduit_coverage(21, 20, 0);
    ASSERT(delta.empty());
}

// =============================================================================
// Out-of-bounds and invalid owner
// =============================================================================

TEST(out_of_bounds_x_returns_empty) {
    EnergySystem sys(64, 64);

    auto delta = sys.preview_conduit_coverage(64, 30, 0);
    ASSERT(delta.empty());
}

TEST(out_of_bounds_y_returns_empty) {
    EnergySystem sys(64, 64);

    auto delta = sys.preview_conduit_coverage(30, 64, 0);
    ASSERT(delta.empty());
}

TEST(out_of_bounds_both_returns_empty) {
    EnergySystem sys(64, 64);

    auto delta = sys.preview_conduit_coverage(100, 100, 0);
    ASSERT(delta.empty());
}

TEST(invalid_owner_returns_empty) {
    EnergySystem sys(64, 64);

    auto delta = sys.preview_conduit_coverage(30, 30, MAX_PLAYERS);
    ASSERT(delta.empty());
}

TEST(invalid_owner_255_returns_empty) {
    EnergySystem sys(64, 64);

    auto delta = sys.preview_conduit_coverage(30, 30, 255);
    ASSERT(delta.empty());
}

// =============================================================================
// Edge/corner clamping: conduit near map boundary
// =============================================================================

TEST(conduit_near_left_edge_clamps) {
    EnergySystem sys(64, 64);
    entt::registry registry;
    sys.set_registry(&registry);

    // Place conduit at (0, 30), then preview at (1, 30) - adjacent
    sys.place_conduit(0, 30, 0);

    auto delta = sys.preview_conduit_coverage(1, 30, 0);
    // All tiles should be within bounds
    for (const auto& p : delta) {
        ASSERT(p.first < 64);
        ASSERT(p.second < 64);
    }
    ASSERT(!delta.empty());
}

TEST(conduit_at_origin_adjacent_to_nexus) {
    EnergySystem sys(64, 64);
    entt::registry registry;
    sys.set_registry(&registry);

    // Place nexus at (1, 0), preview conduit at (0, 0)
    sys.place_nexus(NexusType::Carbon, 1, 0, 0);
    sys.recalculate_coverage(0);

    auto delta = sys.preview_conduit_coverage(0, 0, 0);
    // Conduit at (0,0) radius=3: covers [0,3]x[0,3] = 4x4 = 16 tiles (clamped)
    // Some may be covered by nexus already
    for (const auto& p : delta) {
        ASSERT(p.first < 64);
        ASSERT(p.second < 64);
    }
}

TEST(conduit_at_corner_top_right) {
    EnergySystem sys(64, 64);
    entt::registry registry;
    sys.set_registry(&registry);

    // Place conduit at (62, 0), preview at (63, 0)
    sys.place_conduit(62, 0, 0);

    auto delta = sys.preview_conduit_coverage(63, 0, 0);
    // All tiles should be within bounds
    for (const auto& p : delta) {
        ASSERT(p.first < 64);
        ASSERT(p.second < 64);
    }
    ASSERT(!delta.empty());
}

TEST(conduit_at_corner_bottom_right) {
    EnergySystem sys(64, 64);
    entt::registry registry;
    sys.set_registry(&registry);

    // Place conduit at (62, 63), preview at (63, 63)
    sys.place_conduit(62, 63, 0);

    auto delta = sys.preview_conduit_coverage(63, 63, 0);
    // Conduit at (63,63) radius=3: covers [60,63]x[60,63] = 4x4 = 16 tiles (clamped)
    for (const auto& p : delta) {
        ASSERT(p.first < 64);
        ASSERT(p.second < 64);
    }
    ASSERT(!delta.empty());
}

// =============================================================================
// Coverage delta size is correct (radius = 3, no existing coverage)
// =============================================================================

TEST(full_coverage_delta_7x7_no_existing) {
    EnergySystem sys(64, 64);
    entt::registry registry;
    sys.set_registry(&registry);

    // Place conduit at (30, 30), preview conduit at (31, 30)
    // No coverage exists, so all tiles in radius should be delta.
    sys.place_conduit(30, 30, 0);

    auto delta = sys.preview_conduit_coverage(31, 30, 0);
    // Conduit at (31,30) radius=3: covers [28..34]x[27..33] = 7x7 = 49 tiles
    ASSERT_EQ(delta.size(), static_cast<size_t>(49));
}

// =============================================================================
// Different player coverage doesn't affect delta
// =============================================================================

TEST(other_player_coverage_counts_as_uncovered) {
    EnergySystem sys(64, 64);
    entt::registry registry;
    sys.set_registry(&registry);

    // Player 1 has coverage at some tiles (via nexus)
    sys.place_nexus(NexusType::Carbon, 30, 30, 1);
    sys.recalculate_coverage(1);

    // Player 0 places conduit at (29, 30), preview conduit at (30, 30)
    sys.place_conduit(29, 30, 0);

    // Preview for player 0 at (30, 30) - adjacent to player 0's conduit
    auto delta = sys.preview_conduit_coverage(30, 30, 0);

    // Player 1's coverage should NOT reduce player 0's delta
    // All 49 tiles should appear in delta since player 0 has no coverage
    ASSERT_EQ(delta.size(), static_cast<size_t>(49));
}

// =============================================================================
// Multiple adjacent structures don't cause issues
// =============================================================================

TEST(multiple_adjacent_structures_still_connected) {
    EnergySystem sys(64, 64);
    entt::registry registry;
    sys.set_registry(&registry);

    // Place conduits surrounding (30, 30) on all 4 sides
    sys.place_conduit(29, 30, 0);
    sys.place_conduit(31, 30, 0);
    sys.place_conduit(30, 29, 0);
    sys.place_conduit(30, 31, 0);

    auto delta = sys.preview_conduit_coverage(30, 30, 0);
    // Should still work - connected via any one of the 4 neighbors
    ASSERT(!delta.empty());
    // 7x7 = 49 tiles, no existing coverage
    ASSERT_EQ(delta.size(), static_cast<size_t>(49));
}

// =============================================================================
// Preview is const and does not modify state
// =============================================================================

TEST(preview_does_not_modify_coverage_grid) {
    EnergySystem sys(64, 64);
    entt::registry registry;
    sys.set_registry(&registry);

    sys.place_conduit(30, 30, 0);

    // Snapshot coverage count before preview
    uint32_t count_before = sys.get_coverage_count(1); // overseer_id = owner + 1

    auto delta = sys.preview_conduit_coverage(31, 30, 0);
    ASSERT(!delta.empty());

    // Coverage grid should be unchanged after preview
    uint32_t count_after = sys.get_coverage_count(1);
    ASSERT_EQ(count_before, count_after);
}

TEST(preview_does_not_modify_dirty_flag) {
    EnergySystem sys(64, 64);
    entt::registry registry;
    sys.set_registry(&registry);

    sys.place_conduit(30, 30, 0);

    // Clear dirty flag
    sys.recalculate_coverage(0);
    ASSERT(!sys.is_coverage_dirty(0));

    auto delta = sys.preview_conduit_coverage(31, 30, 0);
    (void)delta;

    // Dirty flag should still be clean
    ASSERT(!sys.is_coverage_dirty(0));
}

// =============================================================================
// Main Entry Point
// =============================================================================

int main() {
    printf("=== Conduit Placement Preview Unit Tests (Ticket 5-031) ===\n\n");

    // Isolated conduit
    RUN_TEST(isolated_conduit_returns_empty);
    RUN_TEST(conduit_not_adjacent_to_network_returns_empty);

    // Connected to nexus
    RUN_TEST(connected_to_nexus_returns_delta);
    RUN_TEST(connected_to_nexus_left);
    RUN_TEST(connected_to_nexus_above);
    RUN_TEST(connected_to_nexus_below);

    // Connected to conduit
    RUN_TEST(connected_to_conduit_returns_delta);

    // Delta correctness
    RUN_TEST(delta_excludes_already_covered_tiles);
    RUN_TEST(delta_does_not_contain_duplicates);
    RUN_TEST(full_overlap_returns_empty_delta);

    // Out-of-bounds / invalid
    RUN_TEST(out_of_bounds_x_returns_empty);
    RUN_TEST(out_of_bounds_y_returns_empty);
    RUN_TEST(out_of_bounds_both_returns_empty);
    RUN_TEST(invalid_owner_returns_empty);
    RUN_TEST(invalid_owner_255_returns_empty);

    // Edge/corner clamping
    RUN_TEST(conduit_near_left_edge_clamps);
    RUN_TEST(conduit_at_origin_adjacent_to_nexus);
    RUN_TEST(conduit_at_corner_top_right);
    RUN_TEST(conduit_at_corner_bottom_right);

    // Coverage delta size
    RUN_TEST(full_coverage_delta_7x7_no_existing);

    // Cross-player coverage
    RUN_TEST(other_player_coverage_counts_as_uncovered);

    // Multiple adjacencies
    RUN_TEST(multiple_adjacent_structures_still_connected);

    // Const correctness (does not modify state)
    RUN_TEST(preview_does_not_modify_coverage_grid);
    RUN_TEST(preview_does_not_modify_dirty_flag);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
