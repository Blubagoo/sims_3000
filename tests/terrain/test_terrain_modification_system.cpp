/**
 * @file test_terrain_modification_system.cpp
 * @brief Unit tests for TerrainModificationSystem (Ticket 3-019)
 *
 * Tests cover:
 * - clear_terrain validation for each clearable type
 * - clear_terrain rejection for non-clearable types
 * - IS_CLEARED flag behavior
 * - Cost returns (positive, negative for PrismaFields, -1 for non-clearable)
 * - TerrainModifiedEvent firing
 * - ChunkDirtyTracker integration
 * - Server-authoritative validation
 */

#include <sims3000/terrain/TerrainModificationSystem.h>
#include <sims3000/terrain/TerrainGrid.h>
#include <sims3000/terrain/TerrainEvents.h>
#include <sims3000/terrain/ChunkDirtyTracker.h>
#include <sims3000/terrain/TerrainTypes.h>
#include <sims3000/terrain/TerrainTypeInfo.h>
#include <cstdio>
#include <cstdlib>
#include <vector>

using namespace sims3000::terrain;

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
// Test Helpers
// =============================================================================

/**
 * @brief Helper to track fired events during tests.
 */
struct EventTracker {
    std::vector<TerrainModifiedEvent> events;

    void clear() { events.clear(); }

    TerrainEventCallback getCallback() {
        return [this](const TerrainModifiedEvent& e) {
            events.push_back(e);
        };
    }
};

/**
 * @brief Helper to set up a tile with specific terrain type.
 */
void setupTile(TerrainGrid& grid, std::int32_t x, std::int32_t y,
               TerrainType type, std::uint8_t elevation = 10) {
    if (grid.in_bounds(x, y)) {
        auto& tile = grid.at(x, y);
        tile.setTerrainType(type);
        tile.setElevation(elevation);
        tile.flags = 0; // Reset flags
    }
}

// =============================================================================
// Clear Terrain - Success Cases
// =============================================================================

TEST(clear_terrain_succeeds_for_biolume_grove) {
    TerrainGrid grid(MapSize::Small);
    ChunkDirtyTracker tracker(128, 128);
    TerrainModificationSystem system(grid, tracker);

    setupTile(grid, 10, 10, TerrainType::BiolumeGrove);
    ASSERT(!grid.at(10, 10).isCleared());

    bool result = system.clear_terrain(10, 10, 1);
    ASSERT(result == true);
    ASSERT(grid.at(10, 10).isCleared());
}

TEST(clear_terrain_succeeds_for_prisma_fields) {
    TerrainGrid grid(MapSize::Small);
    ChunkDirtyTracker tracker(128, 128);
    TerrainModificationSystem system(grid, tracker);

    setupTile(grid, 15, 15, TerrainType::PrismaFields);
    ASSERT(!grid.at(15, 15).isCleared());

    bool result = system.clear_terrain(15, 15, 1);
    ASSERT(result == true);
    ASSERT(grid.at(15, 15).isCleared());
}

TEST(clear_terrain_succeeds_for_spore_flats) {
    TerrainGrid grid(MapSize::Small);
    ChunkDirtyTracker tracker(128, 128);
    TerrainModificationSystem system(grid, tracker);

    setupTile(grid, 20, 20, TerrainType::SporeFlats);
    ASSERT(!grid.at(20, 20).isCleared());

    bool result = system.clear_terrain(20, 20, 1);
    ASSERT(result == true);
    ASSERT(grid.at(20, 20).isCleared());
}

TEST(clear_terrain_is_instant) {
    // Verifies operation completes in single call (instant, one tick)
    TerrainGrid grid(MapSize::Small);
    ChunkDirtyTracker tracker(128, 128);
    TerrainModificationSystem system(grid, tracker);

    setupTile(grid, 10, 10, TerrainType::BiolumeGrove);

    // Single call should complete the operation
    bool result = system.clear_terrain(10, 10, 1);
    ASSERT(result == true);

    // Tile should be fully cleared after one call
    ASSERT(grid.at(10, 10).isCleared());
}

// =============================================================================
// Clear Terrain - Rejection Cases
// =============================================================================

TEST(clear_terrain_fails_for_substrate) {
    TerrainGrid grid(MapSize::Small);
    ChunkDirtyTracker tracker(128, 128);
    TerrainModificationSystem system(grid, tracker);

    setupTile(grid, 10, 10, TerrainType::Substrate);

    bool result = system.clear_terrain(10, 10, 1);
    ASSERT(result == false);
    ASSERT(!grid.at(10, 10).isCleared());
}

TEST(clear_terrain_fails_for_ridge) {
    TerrainGrid grid(MapSize::Small);
    ChunkDirtyTracker tracker(128, 128);
    TerrainModificationSystem system(grid, tracker);

    setupTile(grid, 10, 10, TerrainType::Ridge);

    bool result = system.clear_terrain(10, 10, 1);
    ASSERT(result == false);
}

TEST(clear_terrain_fails_for_deep_void) {
    TerrainGrid grid(MapSize::Small);
    ChunkDirtyTracker tracker(128, 128);
    TerrainModificationSystem system(grid, tracker);

    setupTile(grid, 10, 10, TerrainType::DeepVoid);

    bool result = system.clear_terrain(10, 10, 1);
    ASSERT(result == false);
}

TEST(clear_terrain_fails_for_flow_channel) {
    TerrainGrid grid(MapSize::Small);
    ChunkDirtyTracker tracker(128, 128);
    TerrainModificationSystem system(grid, tracker);

    setupTile(grid, 10, 10, TerrainType::FlowChannel);

    bool result = system.clear_terrain(10, 10, 1);
    ASSERT(result == false);
}

TEST(clear_terrain_fails_for_still_basin) {
    TerrainGrid grid(MapSize::Small);
    ChunkDirtyTracker tracker(128, 128);
    TerrainModificationSystem system(grid, tracker);

    setupTile(grid, 10, 10, TerrainType::StillBasin);

    bool result = system.clear_terrain(10, 10, 1);
    ASSERT(result == false);
}

TEST(clear_terrain_fails_for_blight_mires) {
    TerrainGrid grid(MapSize::Small);
    ChunkDirtyTracker tracker(128, 128);
    TerrainModificationSystem system(grid, tracker);

    setupTile(grid, 10, 10, TerrainType::BlightMires);

    bool result = system.clear_terrain(10, 10, 1);
    ASSERT(result == false);
}

TEST(clear_terrain_fails_for_ember_crust) {
    TerrainGrid grid(MapSize::Small);
    ChunkDirtyTracker tracker(128, 128);
    TerrainModificationSystem system(grid, tracker);

    setupTile(grid, 10, 10, TerrainType::EmberCrust);

    bool result = system.clear_terrain(10, 10, 1);
    ASSERT(result == false);
}

TEST(clear_terrain_fails_for_already_cleared) {
    TerrainGrid grid(MapSize::Small);
    ChunkDirtyTracker tracker(128, 128);
    TerrainModificationSystem system(grid, tracker);

    setupTile(grid, 10, 10, TerrainType::BiolumeGrove);

    // First clear succeeds
    bool result1 = system.clear_terrain(10, 10, 1);
    ASSERT(result1 == true);

    // Second clear fails
    bool result2 = system.clear_terrain(10, 10, 1);
    ASSERT(result2 == false);
}

TEST(clear_terrain_fails_for_out_of_bounds) {
    TerrainGrid grid(MapSize::Small);
    ChunkDirtyTracker tracker(128, 128);
    TerrainModificationSystem system(grid, tracker);

    // Negative coordinates
    bool result1 = system.clear_terrain(-1, 10, 1);
    ASSERT(result1 == false);

    bool result2 = system.clear_terrain(10, -1, 1);
    ASSERT(result2 == false);

    // Beyond grid bounds
    bool result3 = system.clear_terrain(128, 10, 1);
    ASSERT(result3 == false);

    bool result4 = system.clear_terrain(10, 500, 1);
    ASSERT(result4 == false);
}

// =============================================================================
// IS_CLEARED Flag Behavior
// =============================================================================

TEST(is_cleared_flag_set_on_success) {
    TerrainGrid grid(MapSize::Small);
    ChunkDirtyTracker tracker(128, 128);
    TerrainModificationSystem system(grid, tracker);

    setupTile(grid, 10, 10, TerrainType::BiolumeGrove);
    ASSERT(!grid.at(10, 10).isCleared());
    ASSERT(!grid.at(10, 10).testFlag(TerrainFlags::IS_CLEARED));

    system.clear_terrain(10, 10, 1);

    ASSERT(grid.at(10, 10).isCleared());
    ASSERT(grid.at(10, 10).testFlag(TerrainFlags::IS_CLEARED));
}

TEST(is_cleared_flag_not_set_on_failure) {
    TerrainGrid grid(MapSize::Small);
    ChunkDirtyTracker tracker(128, 128);
    TerrainModificationSystem system(grid, tracker);

    // Non-clearable terrain type
    setupTile(grid, 10, 10, TerrainType::Substrate);
    ASSERT(!grid.at(10, 10).isCleared());

    system.clear_terrain(10, 10, 1);

    // Flag should remain unset
    ASSERT(!grid.at(10, 10).isCleared());
}

TEST(cleared_tile_becomes_buildable_substrate) {
    // After clearing, tile should visually become buildable substrate
    // This is verified by checking the cleared flag is set while terrain type preserved
    TerrainGrid grid(MapSize::Small);
    ChunkDirtyTracker tracker(128, 128);
    TerrainModificationSystem system(grid, tracker);

    setupTile(grid, 10, 10, TerrainType::BiolumeGrove);

    system.clear_terrain(10, 10, 1);

    // Terrain type is preserved (for rendering purposes)
    ASSERT_EQ(grid.at(10, 10).getTerrainType(), TerrainType::BiolumeGrove);
    // But the cleared flag indicates it's now buildable
    ASSERT(grid.at(10, 10).isCleared());
}

// =============================================================================
// Cost Returns
// =============================================================================

TEST(get_clear_cost_returns_positive_for_biolume_grove) {
    TerrainGrid grid(MapSize::Small);
    ChunkDirtyTracker tracker(128, 128);
    TerrainModificationSystem system(grid, tracker);

    setupTile(grid, 10, 10, TerrainType::BiolumeGrove);

    std::int64_t cost = system.get_clear_cost(10, 10);
    ASSERT(cost > 0);
    ASSERT_EQ(cost, 100); // From TerrainTypeInfo
}

TEST(get_clear_cost_returns_negative_for_prisma_fields) {
    // PrismaFields clearing yields one-time credit revenue
    TerrainGrid grid(MapSize::Small);
    ChunkDirtyTracker tracker(128, 128);
    TerrainModificationSystem system(grid, tracker);

    setupTile(grid, 10, 10, TerrainType::PrismaFields);

    std::int64_t cost = system.get_clear_cost(10, 10);
    ASSERT(cost < 0);
    ASSERT_EQ(cost, -500); // Negative = revenue
}

TEST(get_clear_cost_returns_positive_for_spore_flats) {
    TerrainGrid grid(MapSize::Small);
    ChunkDirtyTracker tracker(128, 128);
    TerrainModificationSystem system(grid, tracker);

    setupTile(grid, 10, 10, TerrainType::SporeFlats);

    std::int64_t cost = system.get_clear_cost(10, 10);
    ASSERT(cost > 0);
    ASSERT_EQ(cost, 50); // From TerrainTypeInfo
}

TEST(get_clear_cost_returns_zero_for_already_cleared) {
    TerrainGrid grid(MapSize::Small);
    ChunkDirtyTracker tracker(128, 128);
    TerrainModificationSystem system(grid, tracker);

    setupTile(grid, 10, 10, TerrainType::BiolumeGrove);
    system.clear_terrain(10, 10, 1);

    std::int64_t cost = system.get_clear_cost(10, 10);
    ASSERT_EQ(cost, 0);
}

TEST(get_clear_cost_returns_negative_one_for_non_clearable) {
    TerrainGrid grid(MapSize::Small);
    ChunkDirtyTracker tracker(128, 128);
    TerrainModificationSystem system(grid, tracker);

    // Substrate is not clearable
    setupTile(grid, 10, 10, TerrainType::Substrate);
    ASSERT_EQ(system.get_clear_cost(10, 10), -1);

    // Water is not clearable
    setupTile(grid, 11, 11, TerrainType::DeepVoid);
    ASSERT_EQ(system.get_clear_cost(11, 11), -1);

    // BlightMires is not clearable
    setupTile(grid, 12, 12, TerrainType::BlightMires);
    ASSERT_EQ(system.get_clear_cost(12, 12), -1);
}

TEST(get_clear_cost_returns_negative_one_for_out_of_bounds) {
    TerrainGrid grid(MapSize::Small);
    ChunkDirtyTracker tracker(128, 128);
    TerrainModificationSystem system(grid, tracker);

    ASSERT_EQ(system.get_clear_cost(-1, 10), -1);
    ASSERT_EQ(system.get_clear_cost(10, -1), -1);
    ASSERT_EQ(system.get_clear_cost(128, 10), -1);
    ASSERT_EQ(system.get_clear_cost(10, 500), -1);
}

// =============================================================================
// TerrainModifiedEvent Firing
// =============================================================================

TEST(fires_terrain_modified_event_on_clear) {
    TerrainGrid grid(MapSize::Small);
    ChunkDirtyTracker tracker(128, 128);
    TerrainModificationSystem system(grid, tracker);

    EventTracker events;
    system.setEventCallback(events.getCallback());

    setupTile(grid, 25, 30, TerrainType::BiolumeGrove);

    system.clear_terrain(25, 30, 1);

    ASSERT_EQ(events.events.size(), static_cast<size_t>(1));
    ASSERT_EQ(events.events[0].modification_type, ModificationType::Cleared);
    ASSERT_EQ(events.events[0].affected_area.x, 25);
    ASSERT_EQ(events.events[0].affected_area.y, 30);
    ASSERT_EQ(events.events[0].affected_area.width, 1);
    ASSERT_EQ(events.events[0].affected_area.height, 1);
}

TEST(no_event_fired_on_clear_failure) {
    TerrainGrid grid(MapSize::Small);
    ChunkDirtyTracker tracker(128, 128);
    TerrainModificationSystem system(grid, tracker);

    EventTracker events;
    system.setEventCallback(events.getCallback());

    // Non-clearable terrain
    setupTile(grid, 10, 10, TerrainType::Substrate);

    system.clear_terrain(10, 10, 1);

    ASSERT_EQ(events.events.size(), static_cast<size_t>(0));
}

TEST(event_fired_with_correct_modification_type) {
    TerrainGrid grid(MapSize::Small);
    ChunkDirtyTracker tracker(128, 128);
    TerrainModificationSystem system(grid, tracker);

    EventTracker events;
    system.setEventCallback(events.getCallback());

    setupTile(grid, 10, 10, TerrainType::PrismaFields);
    system.clear_terrain(10, 10, 1);

    ASSERT_EQ(events.events.size(), static_cast<size_t>(1));
    ASSERT_EQ(events.events[0].modification_type, ModificationType::Cleared);
}

// =============================================================================
// ChunkDirtyTracker Integration
// =============================================================================

TEST(marks_chunk_dirty_on_clear) {
    TerrainGrid grid(MapSize::Small);
    ChunkDirtyTracker tracker(128, 128);
    TerrainModificationSystem system(grid, tracker);

    // Tile (10, 10) is in chunk (0, 0) since chunk size is 32
    setupTile(grid, 10, 10, TerrainType::BiolumeGrove);

    ASSERT(!tracker.isChunkDirty(0, 0));

    system.clear_terrain(10, 10, 1);

    ASSERT(tracker.isChunkDirty(0, 0));
}

TEST(marks_correct_chunk_dirty) {
    TerrainGrid grid(MapSize::Small);
    ChunkDirtyTracker tracker(128, 128);
    TerrainModificationSystem system(grid, tracker);

    // Tile (50, 70) is in chunk (1, 2) since chunk size is 32
    setupTile(grid, 50, 70, TerrainType::BiolumeGrove);

    ASSERT(!tracker.isChunkDirty(1, 2));
    ASSERT(!tracker.isChunkDirty(0, 0));

    system.clear_terrain(50, 70, 1);

    ASSERT(tracker.isChunkDirty(1, 2));
    ASSERT(!tracker.isChunkDirty(0, 0)); // Other chunks unaffected
}

TEST(no_chunk_marked_dirty_on_failure) {
    TerrainGrid grid(MapSize::Small);
    ChunkDirtyTracker tracker(128, 128);
    TerrainModificationSystem system(grid, tracker);

    setupTile(grid, 10, 10, TerrainType::Substrate); // Non-clearable

    system.clear_terrain(10, 10, 1);

    // No chunks should be marked dirty
    ASSERT(!tracker.hasAnyDirty());
}

// =============================================================================
// Server-Authoritative Validation
// =============================================================================

TEST(validates_player_authority) {
    TerrainGrid grid(MapSize::Small);
    ChunkDirtyTracker tracker(128, 128);
    TerrainModificationSystem system(grid, tracker);

    setupTile(grid, 10, 10, TerrainType::BiolumeGrove);

    // GAME_MASTER (0) should work
    ASSERT(system.clear_terrain(10, 10, 0) == true);

    // Reset
    setupTile(grid, 11, 11, TerrainType::BiolumeGrove);

    // Player 1-4 should work
    for (PlayerID p = 1; p <= 4; ++p) {
        setupTile(grid, static_cast<std::int32_t>(p) + 10, 11, TerrainType::BiolumeGrove);
        ASSERT(system.clear_terrain(static_cast<std::int32_t>(p) + 10, 11, p) == true);
    }
}

TEST(validates_bounds) {
    TerrainGrid grid(MapSize::Small);
    ChunkDirtyTracker tracker(128, 128);
    TerrainModificationSystem system(grid, tracker);

    // All out-of-bounds should fail
    ASSERT(system.clear_terrain(-1, 0, 1) == false);
    ASSERT(system.clear_terrain(0, -1, 1) == false);
    ASSERT(system.clear_terrain(128, 0, 1) == false);
    ASSERT(system.clear_terrain(0, 128, 1) == false);
}

TEST(validates_clearable_type) {
    TerrainGrid grid(MapSize::Small);
    ChunkDirtyTracker tracker(128, 128);
    TerrainModificationSystem system(grid, tracker);

    // Test all terrain types
    struct TestCase {
        TerrainType type;
        bool should_succeed;
    };

    TestCase cases[] = {
        {TerrainType::Substrate, false},
        {TerrainType::Ridge, false},
        {TerrainType::DeepVoid, false},
        {TerrainType::FlowChannel, false},
        {TerrainType::StillBasin, false},
        {TerrainType::BiolumeGrove, true},
        {TerrainType::PrismaFields, true},
        {TerrainType::SporeFlats, true},
        {TerrainType::BlightMires, false},
        {TerrainType::EmberCrust, false}
    };

    for (int i = 0; i < 10; ++i) {
        setupTile(grid, i, 0, cases[i].type);
        bool result = system.clear_terrain(i, 0, 1);
        ASSERT(result == cases[i].should_succeed);
    }
}

TEST(validates_not_already_cleared) {
    TerrainGrid grid(MapSize::Small);
    ChunkDirtyTracker tracker(128, 128);
    TerrainModificationSystem system(grid, tracker);

    setupTile(grid, 10, 10, TerrainType::BiolumeGrove);

    // Pre-set cleared flag
    grid.at(10, 10).setCleared(true);

    // Should fail because already cleared
    bool result = system.clear_terrain(10, 10, 1);
    ASSERT(result == false);
}

// =============================================================================
// Level Terrain Cost Query (for completeness)
// =============================================================================

TEST(get_level_cost_returns_cost_based_on_elevation_diff) {
    TerrainGrid grid(MapSize::Small);
    ChunkDirtyTracker tracker(128, 128);
    TerrainModificationSystem system(grid, tracker);

    setupTile(grid, 10, 10, TerrainType::Substrate, 10);

    // 5 levels up = 50 cost
    ASSERT_EQ(system.get_level_cost(10, 10, 15), 50);

    // 5 levels down = 50 cost
    ASSERT_EQ(system.get_level_cost(10, 10, 5), 50);

    // Same level = 0 cost
    ASSERT_EQ(system.get_level_cost(10, 10, 10), 0);
}

TEST(get_level_cost_returns_negative_one_for_water) {
    TerrainGrid grid(MapSize::Small);
    ChunkDirtyTracker tracker(128, 128);
    TerrainModificationSystem system(grid, tracker);

    setupTile(grid, 10, 10, TerrainType::DeepVoid);
    ASSERT_EQ(system.get_level_cost(10, 10, 15), -1);

    setupTile(grid, 11, 11, TerrainType::FlowChannel);
    ASSERT_EQ(system.get_level_cost(11, 11, 15), -1);

    setupTile(grid, 12, 12, TerrainType::StillBasin);
    ASSERT_EQ(system.get_level_cost(12, 12, 15), -1);

    setupTile(grid, 13, 13, TerrainType::BlightMires);
    ASSERT_EQ(system.get_level_cost(13, 13, 15), -1);
}

// =============================================================================
// Edge Cases
// =============================================================================

TEST(multiple_clears_on_different_tiles) {
    TerrainGrid grid(MapSize::Small);
    ChunkDirtyTracker tracker(128, 128);
    TerrainModificationSystem system(grid, tracker);

    EventTracker events;
    system.setEventCallback(events.getCallback());

    // Setup multiple clearable tiles
    setupTile(grid, 10, 10, TerrainType::BiolumeGrove);
    setupTile(grid, 20, 20, TerrainType::PrismaFields);
    setupTile(grid, 30, 30, TerrainType::SporeFlats);

    // Clear all three
    ASSERT(system.clear_terrain(10, 10, 1) == true);
    ASSERT(system.clear_terrain(20, 20, 1) == true);
    ASSERT(system.clear_terrain(30, 30, 1) == true);

    // All should be cleared
    ASSERT(grid.at(10, 10).isCleared());
    ASSERT(grid.at(20, 20).isCleared());
    ASSERT(grid.at(30, 30).isCleared());

    // Three events should have fired
    ASSERT_EQ(events.events.size(), static_cast<size_t>(3));
}

TEST(clear_at_grid_boundaries) {
    TerrainGrid grid(MapSize::Small);
    ChunkDirtyTracker tracker(128, 128);
    TerrainModificationSystem system(grid, tracker);

    // Test corners and edges
    setupTile(grid, 0, 0, TerrainType::BiolumeGrove);
    setupTile(grid, 127, 0, TerrainType::BiolumeGrove);
    setupTile(grid, 0, 127, TerrainType::BiolumeGrove);
    setupTile(grid, 127, 127, TerrainType::BiolumeGrove);

    ASSERT(system.clear_terrain(0, 0, 1) == true);
    ASSERT(system.clear_terrain(127, 0, 1) == true);
    ASSERT(system.clear_terrain(0, 127, 1) == true);
    ASSERT(system.clear_terrain(127, 127, 1) == true);

    ASSERT(grid.at(0, 0).isCleared());
    ASSERT(grid.at(127, 0).isCleared());
    ASSERT(grid.at(0, 127).isCleared());
    ASSERT(grid.at(127, 127).isCleared());
}

TEST(callback_can_be_replaced) {
    TerrainGrid grid(MapSize::Small);
    ChunkDirtyTracker tracker(128, 128);
    TerrainModificationSystem system(grid, tracker);

    EventTracker events1, events2;

    // Set first callback
    system.setEventCallback(events1.getCallback());
    setupTile(grid, 10, 10, TerrainType::BiolumeGrove);
    system.clear_terrain(10, 10, 1);
    ASSERT_EQ(events1.events.size(), static_cast<size_t>(1));
    ASSERT_EQ(events2.events.size(), static_cast<size_t>(0));

    // Replace with second callback
    system.setEventCallback(events2.getCallback());
    setupTile(grid, 11, 11, TerrainType::BiolumeGrove);
    system.clear_terrain(11, 11, 1);
    ASSERT_EQ(events1.events.size(), static_cast<size_t>(1)); // Unchanged
    ASSERT_EQ(events2.events.size(), static_cast<size_t>(1)); // Now receiving
}

// =============================================================================
// Main Entry Point
// =============================================================================

int main() {
    printf("=== TerrainModificationSystem Unit Tests (Ticket 3-019) ===\n\n");

    // Clear terrain success cases
    RUN_TEST(clear_terrain_succeeds_for_biolume_grove);
    RUN_TEST(clear_terrain_succeeds_for_prisma_fields);
    RUN_TEST(clear_terrain_succeeds_for_spore_flats);
    RUN_TEST(clear_terrain_is_instant);

    // Clear terrain rejection cases
    RUN_TEST(clear_terrain_fails_for_substrate);
    RUN_TEST(clear_terrain_fails_for_ridge);
    RUN_TEST(clear_terrain_fails_for_deep_void);
    RUN_TEST(clear_terrain_fails_for_flow_channel);
    RUN_TEST(clear_terrain_fails_for_still_basin);
    RUN_TEST(clear_terrain_fails_for_blight_mires);
    RUN_TEST(clear_terrain_fails_for_ember_crust);
    RUN_TEST(clear_terrain_fails_for_already_cleared);
    RUN_TEST(clear_terrain_fails_for_out_of_bounds);

    // IS_CLEARED flag behavior
    RUN_TEST(is_cleared_flag_set_on_success);
    RUN_TEST(is_cleared_flag_not_set_on_failure);
    RUN_TEST(cleared_tile_becomes_buildable_substrate);

    // Cost returns
    RUN_TEST(get_clear_cost_returns_positive_for_biolume_grove);
    RUN_TEST(get_clear_cost_returns_negative_for_prisma_fields);
    RUN_TEST(get_clear_cost_returns_positive_for_spore_flats);
    RUN_TEST(get_clear_cost_returns_zero_for_already_cleared);
    RUN_TEST(get_clear_cost_returns_negative_one_for_non_clearable);
    RUN_TEST(get_clear_cost_returns_negative_one_for_out_of_bounds);

    // Event firing
    RUN_TEST(fires_terrain_modified_event_on_clear);
    RUN_TEST(no_event_fired_on_clear_failure);
    RUN_TEST(event_fired_with_correct_modification_type);

    // Chunk dirty tracking
    RUN_TEST(marks_chunk_dirty_on_clear);
    RUN_TEST(marks_correct_chunk_dirty);
    RUN_TEST(no_chunk_marked_dirty_on_failure);

    // Server-authoritative validation
    RUN_TEST(validates_player_authority);
    RUN_TEST(validates_bounds);
    RUN_TEST(validates_clearable_type);
    RUN_TEST(validates_not_already_cleared);

    // Level terrain cost query
    RUN_TEST(get_level_cost_returns_cost_based_on_elevation_diff);
    RUN_TEST(get_level_cost_returns_negative_one_for_water);

    // Edge cases
    RUN_TEST(multiple_clears_on_different_tiles);
    RUN_TEST(clear_at_grid_boundaries);
    RUN_TEST(callback_can_be_replaced);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
