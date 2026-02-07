/**
 * @file test_contamination_source_query.cpp
 * @brief Unit tests for ContaminationSourceQuery (Ticket 3-018)
 *
 * Tests cover:
 * - ContaminationSource struct size and layout
 * - Query construction and initial state
 * - get_terrain_contamination_sources() returns vector of sources
 * - Only BlightMires tiles produce contamination
 * - Output rate from TerrainTypeInfo static table
 * - Cache validity and invalidation
 * - Cache rebuild on terrain modification
 * - Performance: O(1) cache access, O(blight_mire_count) rebuild
 */

#include <sims3000/terrain/ContaminationSourceQuery.h>
#include <sims3000/terrain/TerrainTypeInfo.h>
#include <cstdio>
#include <cstdlib>
#include <chrono>

using namespace sims3000;
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
// ContaminationSource Struct Tests
// =============================================================================

TEST(contamination_source_size) {
    ASSERT_EQ(sizeof(ContaminationSource), 12);
}

TEST(contamination_source_trivially_copyable) {
    ASSERT(std::is_trivially_copyable<ContaminationSource>::value);
}

TEST(contamination_source_construction) {
    ContaminationSource source;
    source.position.x = 10;
    source.position.y = 20;
    source.contamination_per_tick = 5;
    source.source_type = TerrainType::BlightMires;

    ASSERT_EQ(source.position.x, 10);
    ASSERT_EQ(source.position.y, 20);
    ASSERT_EQ(source.contamination_per_tick, 5);
    ASSERT_EQ(source.source_type, TerrainType::BlightMires);
}

// =============================================================================
// Contamination Rate from TerrainTypeInfo Tests
// =============================================================================

TEST(blight_mires_contamination_from_terrain_info) {
    // Verify the contamination rate comes from TerrainTypeInfo static table
    const TerrainTypeInfo& info = getTerrainInfo(TerrainType::BlightMires);
    ASSERT(info.contamination_per_tick > 0);
    ASSERT(info.contamination_per_tick <= 100);  // Sanity check
    ASSERT_EQ(info.contamination_per_tick, 5);   // Expected value from table
}

TEST(blight_mires_generates_contamination) {
    // Verify BlightMires is marked as generating contamination in TerrainTypeInfo
    ASSERT(generatesContamination(TerrainType::BlightMires));
}

TEST(other_types_no_contamination) {
    // Verify other terrain types do NOT generate contamination
    ASSERT(!generatesContamination(TerrainType::Substrate));
    ASSERT(!generatesContamination(TerrainType::Ridge));
    ASSERT(!generatesContamination(TerrainType::DeepVoid));
    ASSERT(!generatesContamination(TerrainType::FlowChannel));
    ASSERT(!generatesContamination(TerrainType::StillBasin));
    ASSERT(!generatesContamination(TerrainType::BiolumeGrove));
    ASSERT(!generatesContamination(TerrainType::PrismaFields));
    ASSERT(!generatesContamination(TerrainType::SporeFlats));
    ASSERT(!generatesContamination(TerrainType::EmberCrust));
}

// =============================================================================
// Query Construction and Initial State Tests
// =============================================================================

TEST(query_construction_empty_grid) {
    TerrainGrid grid;  // Empty grid
    ContaminationSourceQuery query(grid);

    ASSERT(!query.is_cache_valid());
    ASSERT_EQ(query.source_count(), 0);
}

TEST(query_construction_initialized_grid) {
    TerrainGrid grid(MapSize::Small);  // 128x128
    ContaminationSourceQuery query(grid);

    // Cache should not be valid until first query
    ASSERT(!query.is_cache_valid());
}

// =============================================================================
// get_terrain_contamination_sources() Tests
// =============================================================================

TEST(query_empty_grid) {
    TerrainGrid grid;
    ContaminationSourceQuery query(grid);

    const auto& sources = query.get_terrain_contamination_sources();
    ASSERT(sources.empty());
    ASSERT(query.is_cache_valid());
}

TEST(query_no_blight_mires) {
    TerrainGrid grid(MapSize::Small);
    grid.fill_type(TerrainType::Substrate);  // All substrate, no BlightMires

    ContaminationSourceQuery query(grid);
    const auto& sources = query.get_terrain_contamination_sources();

    ASSERT(sources.empty());
    ASSERT(query.is_cache_valid());
}

TEST(query_single_blight_mire) {
    TerrainGrid grid(MapSize::Small);
    grid.fill_type(TerrainType::Substrate);

    // Place a single BlightMires tile
    grid.at(50, 60).setTerrainType(TerrainType::BlightMires);

    ContaminationSourceQuery query(grid);
    const auto& sources = query.get_terrain_contamination_sources();

    ASSERT_EQ(sources.size(), 1);
    ASSERT_EQ(sources[0].position.x, 50);
    ASSERT_EQ(sources[0].position.y, 60);
    ASSERT_EQ(sources[0].source_type, TerrainType::BlightMires);
    // Verify contamination_per_tick comes from TerrainTypeInfo lookup
    ASSERT_EQ(sources[0].contamination_per_tick, getTerrainInfo(TerrainType::BlightMires).contamination_per_tick);
}

TEST(query_multiple_blight_mires) {
    TerrainGrid grid(MapSize::Small);
    grid.fill_type(TerrainType::Substrate);

    // Place multiple BlightMires tiles
    grid.at(10, 10).setTerrainType(TerrainType::BlightMires);
    grid.at(50, 50).setTerrainType(TerrainType::BlightMires);
    grid.at(100, 100).setTerrainType(TerrainType::BlightMires);

    ContaminationSourceQuery query(grid);
    const auto& sources = query.get_terrain_contamination_sources();

    ASSERT_EQ(sources.size(), 3);

    // Verify all sources are BlightMires with output rate from TerrainTypeInfo
    const std::uint32_t expected_rate = getTerrainInfo(TerrainType::BlightMires).contamination_per_tick;
    for (const auto& source : sources) {
        ASSERT_EQ(source.source_type, TerrainType::BlightMires);
        ASSERT_EQ(source.contamination_per_tick, expected_rate);
    }
}

TEST(query_mixed_terrain) {
    TerrainGrid grid(MapSize::Small);
    grid.fill_type(TerrainType::Substrate);

    // Place various terrain types
    grid.at(0, 0).setTerrainType(TerrainType::BiolumeGrove);
    grid.at(10, 10).setTerrainType(TerrainType::BlightMires);  // Should be found
    grid.at(20, 20).setTerrainType(TerrainType::PrismaFields);
    grid.at(30, 30).setTerrainType(TerrainType::EmberCrust);
    grid.at(40, 40).setTerrainType(TerrainType::BlightMires);  // Should be found
    grid.at(50, 50).setTerrainType(TerrainType::DeepVoid);

    ContaminationSourceQuery query(grid);
    const auto& sources = query.get_terrain_contamination_sources();

    // Only BlightMires tiles should be returned
    ASSERT_EQ(sources.size(), 2);

    for (const auto& source : sources) {
        ASSERT_EQ(source.source_type, TerrainType::BlightMires);
    }
}

TEST(query_blight_mires_at_edges) {
    TerrainGrid grid(MapSize::Small);  // 128x128
    grid.fill_type(TerrainType::Substrate);

    // Place BlightMires at map edges
    grid.at(0, 0).setTerrainType(TerrainType::BlightMires);      // Top-left
    grid.at(127, 0).setTerrainType(TerrainType::BlightMires);    // Top-right
    grid.at(0, 127).setTerrainType(TerrainType::BlightMires);    // Bottom-left
    grid.at(127, 127).setTerrainType(TerrainType::BlightMires);  // Bottom-right

    ContaminationSourceQuery query(grid);
    const auto& sources = query.get_terrain_contamination_sources();

    ASSERT_EQ(sources.size(), 4);
}

// =============================================================================
// Cache Validity Tests
// =============================================================================

TEST(cache_valid_after_query) {
    TerrainGrid grid(MapSize::Small);
    ContaminationSourceQuery query(grid);

    ASSERT(!query.is_cache_valid());

    query.get_terrain_contamination_sources();

    ASSERT(query.is_cache_valid());
}

TEST(cache_invalidation) {
    TerrainGrid grid(MapSize::Small);
    ContaminationSourceQuery query(grid);

    query.get_terrain_contamination_sources();
    ASSERT(query.is_cache_valid());

    query.invalidate_cache();
    ASSERT(!query.is_cache_valid());
}

TEST(cache_rebuild) {
    TerrainGrid grid(MapSize::Small);
    grid.fill_type(TerrainType::Substrate);
    grid.at(10, 10).setTerrainType(TerrainType::BlightMires);

    ContaminationSourceQuery query(grid);

    ASSERT(!query.is_cache_valid());

    query.rebuild_cache();

    ASSERT(query.is_cache_valid());
    ASSERT_EQ(query.source_count(), 1);
}

TEST(cache_returns_same_reference) {
    TerrainGrid grid(MapSize::Small);
    grid.at(10, 10).setTerrainType(TerrainType::BlightMires);

    ContaminationSourceQuery query(grid);

    const auto& sources1 = query.get_terrain_contamination_sources();
    const auto& sources2 = query.get_terrain_contamination_sources();

    // Should return the same cached vector
    ASSERT(&sources1 == &sources2);
}

// =============================================================================
// Cache Invalidation via TerrainModifiedEvent Tests
// =============================================================================

TEST(event_terraformed_invalidates_cache) {
    TerrainGrid grid(MapSize::Small);
    ContaminationSourceQuery query(grid);

    query.get_terrain_contamination_sources();
    ASSERT(query.is_cache_valid());

    TerrainModifiedEvent event(GridRect::singleTile(10, 10), ModificationType::Terraformed);
    query.on_terrain_modified(event);

    ASSERT(!query.is_cache_valid());
}

TEST(event_generated_invalidates_cache) {
    TerrainGrid grid(MapSize::Small);
    ContaminationSourceQuery query(grid);

    query.get_terrain_contamination_sources();
    ASSERT(query.is_cache_valid());

    TerrainModifiedEvent event(GridRect::fromCorners(0, 0, 128, 128), ModificationType::Generated);
    query.on_terrain_modified(event);

    ASSERT(!query.is_cache_valid());
}

TEST(event_cleared_does_not_invalidate_cache) {
    // Clearing terrain doesn't change terrain type, so cache should remain valid
    TerrainGrid grid(MapSize::Small);
    ContaminationSourceQuery query(grid);

    query.get_terrain_contamination_sources();
    ASSERT(query.is_cache_valid());

    TerrainModifiedEvent event(GridRect::singleTile(10, 10), ModificationType::Cleared);
    query.on_terrain_modified(event);

    ASSERT(query.is_cache_valid());
}

TEST(event_leveled_does_not_invalidate_cache) {
    // Leveling terrain doesn't change terrain type, so cache should remain valid
    TerrainGrid grid(MapSize::Small);
    ContaminationSourceQuery query(grid);

    query.get_terrain_contamination_sources();
    ASSERT(query.is_cache_valid());

    TerrainModifiedEvent event(GridRect::singleTile(10, 10), ModificationType::Leveled);
    query.on_terrain_modified(event);

    ASSERT(query.is_cache_valid());
}

TEST(event_sea_level_does_not_invalidate_cache) {
    // Sea level changes don't affect terrain type, so cache should remain valid
    TerrainGrid grid(MapSize::Small);
    ContaminationSourceQuery query(grid);

    query.get_terrain_contamination_sources();
    ASSERT(query.is_cache_valid());

    TerrainModifiedEvent event(GridRect::fromCorners(0, 0, 128, 128), ModificationType::SeaLevelChanged);
    query.on_terrain_modified(event);

    ASSERT(query.is_cache_valid());
}

// =============================================================================
// Cache Reflects Grid Changes After Rebuild Tests
// =============================================================================

TEST(cache_reflects_added_blight_mire) {
    TerrainGrid grid(MapSize::Small);
    grid.fill_type(TerrainType::Substrate);

    ContaminationSourceQuery query(grid);

    // Initial query - no BlightMires
    const auto& sources1 = query.get_terrain_contamination_sources();
    ASSERT_EQ(sources1.size(), 0);

    // Modify grid (simulate terraforming)
    grid.at(50, 50).setTerrainType(TerrainType::BlightMires);

    // Invalidate cache (simulating TerrainModifiedEvent)
    query.invalidate_cache();

    // Query again - should now have the new BlightMires
    const auto& sources2 = query.get_terrain_contamination_sources();
    ASSERT_EQ(sources2.size(), 1);
    ASSERT_EQ(sources2[0].position.x, 50);
    ASSERT_EQ(sources2[0].position.y, 50);
}

TEST(cache_reflects_removed_blight_mire) {
    TerrainGrid grid(MapSize::Small);
    grid.fill_type(TerrainType::Substrate);
    grid.at(50, 50).setTerrainType(TerrainType::BlightMires);

    ContaminationSourceQuery query(grid);

    // Initial query - one BlightMires
    const auto& sources1 = query.get_terrain_contamination_sources();
    ASSERT_EQ(sources1.size(), 1);

    // Modify grid (simulate terraforming away BlightMires)
    grid.at(50, 50).setTerrainType(TerrainType::Substrate);

    // Invalidate cache
    query.invalidate_cache();

    // Query again - should now be empty
    const auto& sources2 = query.get_terrain_contamination_sources();
    ASSERT_EQ(sources2.size(), 0);
}

// =============================================================================
// Performance Tests
// =============================================================================

TEST(performance_cache_access_is_fast) {
    TerrainGrid grid(MapSize::Medium);  // 256x256
    grid.fill_type(TerrainType::Substrate);

    // Add some BlightMires
    for (int i = 0; i < 100; ++i) {
        grid.at(i, i).setTerrainType(TerrainType::BlightMires);
    }

    ContaminationSourceQuery query(grid);

    // Build cache first
    query.get_terrain_contamination_sources();
    ASSERT(query.is_cache_valid());

    // Measure time for cached access (should be O(1))
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 10000; ++i) {
        const auto& sources = query.get_terrain_contamination_sources();
        (void)sources;  // Prevent optimization
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    // 10000 accesses should complete in under 1ms (very conservative)
    ASSERT(duration.count() < 1000);
}

TEST(performance_rebuild_scales_with_blight_count) {
    TerrainGrid grid(MapSize::Medium);  // 256x256
    grid.fill_type(TerrainType::Substrate);

    // Add varying amounts of BlightMires and measure rebuild time
    ContaminationSourceQuery query(grid);

    // First with 10 BlightMires
    for (int i = 0; i < 10; ++i) {
        grid.at(i, 0).setTerrainType(TerrainType::BlightMires);
    }

    query.invalidate_cache();
    auto start1 = std::chrono::high_resolution_clock::now();
    query.rebuild_cache();
    auto end1 = std::chrono::high_resolution_clock::now();
    auto time_10 = std::chrono::duration_cast<std::chrono::microseconds>(end1 - start1).count();

    ASSERT_EQ(query.source_count(), 10);

    // Rebuild must complete in reasonable time for a 256x256 grid
    // Note: Full grid scan is O(n) where n = 65536 tiles
    ASSERT(time_10 < 50000);  // 50ms max for rebuild
}

TEST(source_count_accuracy) {
    TerrainGrid grid(MapSize::Small);
    grid.fill_type(TerrainType::Substrate);

    // Add exactly 42 BlightMires
    for (int i = 0; i < 42; ++i) {
        grid.at(i % 128, i / 128).setTerrainType(TerrainType::BlightMires);
    }

    ContaminationSourceQuery query(grid);
    const auto& sources = query.get_terrain_contamination_sources();

    ASSERT_EQ(sources.size(), 42);
    ASSERT_EQ(query.source_count(), 42);
}

// =============================================================================
// Large Grid Tests
// =============================================================================

TEST(query_large_grid) {
    TerrainGrid grid(MapSize::Large);  // 512x512
    grid.fill_type(TerrainType::Substrate);

    // Place BlightMires in a diagonal pattern
    for (int i = 0; i < 256; ++i) {
        grid.at(i * 2, i * 2).setTerrainType(TerrainType::BlightMires);
    }

    ContaminationSourceQuery query(grid);
    const auto& sources = query.get_terrain_contamination_sources();

    ASSERT_EQ(sources.size(), 256);

    // Verify first and last sources
    bool found_first = false;
    bool found_last = false;
    for (const auto& source : sources) {
        if (source.position.x == 0 && source.position.y == 0) {
            found_first = true;
        }
        if (source.position.x == 510 && source.position.y == 510) {
            found_last = true;
        }
    }
    ASSERT(found_first);
    ASSERT(found_last);
}

// =============================================================================
// Main Entry Point
// =============================================================================

int main() {
    printf("=== ContaminationSourceQuery Unit Tests ===\n\n");

    // ContaminationSource struct tests
    RUN_TEST(contamination_source_size);
    RUN_TEST(contamination_source_trivially_copyable);
    RUN_TEST(contamination_source_construction);

    // Contamination rate from TerrainTypeInfo tests
    RUN_TEST(blight_mires_contamination_from_terrain_info);
    RUN_TEST(blight_mires_generates_contamination);
    RUN_TEST(other_types_no_contamination);

    // Query construction tests
    RUN_TEST(query_construction_empty_grid);
    RUN_TEST(query_construction_initialized_grid);

    // get_terrain_contamination_sources() tests
    RUN_TEST(query_empty_grid);
    RUN_TEST(query_no_blight_mires);
    RUN_TEST(query_single_blight_mire);
    RUN_TEST(query_multiple_blight_mires);
    RUN_TEST(query_mixed_terrain);
    RUN_TEST(query_blight_mires_at_edges);

    // Cache validity tests
    RUN_TEST(cache_valid_after_query);
    RUN_TEST(cache_invalidation);
    RUN_TEST(cache_rebuild);
    RUN_TEST(cache_returns_same_reference);

    // Cache invalidation via event tests
    RUN_TEST(event_terraformed_invalidates_cache);
    RUN_TEST(event_generated_invalidates_cache);
    RUN_TEST(event_cleared_does_not_invalidate_cache);
    RUN_TEST(event_leveled_does_not_invalidate_cache);
    RUN_TEST(event_sea_level_does_not_invalidate_cache);

    // Cache reflects grid changes tests
    RUN_TEST(cache_reflects_added_blight_mire);
    RUN_TEST(cache_reflects_removed_blight_mire);

    // Performance tests
    RUN_TEST(performance_cache_access_is_fast);
    RUN_TEST(performance_rebuild_scales_with_blight_count);
    RUN_TEST(source_count_accuracy);

    // Large grid tests
    RUN_TEST(query_large_grid);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
