/**
 * @file test_terrain_events.cpp
 * @brief Unit tests for TerrainEvents.h and ChunkDirtyTracker (Ticket 3-004)
 *
 * Tests cover:
 * - GridRect construction and operations
 * - ModificationType enum values
 * - TerrainModifiedEvent structure
 * - ChunkDirtyTracker initialization and chunk grid sizing
 * - mark_chunk_dirty, is_chunk_dirty, clear_chunk_dirty operations
 * - Tile coordinate to chunk coordinate conversion
 * - Dirty flag propagation from tile coordinates to chunk coordinates
 * - markTilesDirty for rectangular regions
 * - Event processing via processEvent()
 */

#include <sims3000/terrain/TerrainEvents.h>
#include <sims3000/terrain/ChunkDirtyTracker.h>
#include <cstdio>
#include <cstdlib>

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
// GridRect Tests
// =============================================================================

TEST(grid_rect_default_construction) {
    GridRect rect;
    ASSERT_EQ(rect.x, 0);
    ASSERT_EQ(rect.y, 0);
    ASSERT_EQ(rect.width, 0);
    ASSERT_EQ(rect.height, 0);
    ASSERT(rect.isEmpty());
}

TEST(grid_rect_single_tile) {
    GridRect rect = GridRect::singleTile(10, 20);
    ASSERT_EQ(rect.x, 10);
    ASSERT_EQ(rect.y, 20);
    ASSERT_EQ(rect.width, 1);
    ASSERT_EQ(rect.height, 1);
    ASSERT(!rect.isEmpty());
}

TEST(grid_rect_from_corners) {
    GridRect rect = GridRect::fromCorners(5, 10, 15, 25);
    ASSERT_EQ(rect.x, 5);
    ASSERT_EQ(rect.y, 10);
    ASSERT_EQ(rect.width, 10);
    ASSERT_EQ(rect.height, 15);
    ASSERT(!rect.isEmpty());
}

TEST(grid_rect_from_corners_invalid) {
    // Right < left should produce empty rect
    GridRect rect = GridRect::fromCorners(15, 25, 5, 10);
    ASSERT_EQ(rect.width, 0);
    ASSERT_EQ(rect.height, 0);
    ASSERT(rect.isEmpty());
}

TEST(grid_rect_contains) {
    GridRect rect;
    rect.x = 10;
    rect.y = 10;
    rect.width = 5;
    rect.height = 5;

    // Inside
    ASSERT(rect.contains(10, 10));  // Top-left corner
    ASSERT(rect.contains(14, 14));  // Bottom-right (last valid)
    ASSERT(rect.contains(12, 12));  // Center

    // Outside
    ASSERT(!rect.contains(9, 10));   // Left of
    ASSERT(!rect.contains(10, 9));   // Above
    ASSERT(!rect.contains(15, 10));  // Right of (exclusive)
    ASSERT(!rect.contains(10, 15));  // Below (exclusive)
}

TEST(grid_rect_right_bottom) {
    GridRect rect;
    rect.x = 10;
    rect.y = 20;
    rect.width = 30;
    rect.height = 40;

    ASSERT_EQ(rect.right(), 40);
    ASSERT_EQ(rect.bottom(), 60);
}

TEST(grid_rect_equality) {
    GridRect a = GridRect::fromCorners(0, 0, 10, 10);
    GridRect b = GridRect::fromCorners(0, 0, 10, 10);
    GridRect c = GridRect::fromCorners(0, 0, 10, 11);

    ASSERT(a == b);
    ASSERT(!(a != b));
    ASSERT(a != c);
    ASSERT(!(a == c));
}

TEST(grid_rect_size) {
    ASSERT_EQ(sizeof(GridRect), 8);
}

TEST(grid_rect_trivially_copyable) {
    ASSERT(std::is_trivially_copyable<GridRect>::value);
}

// =============================================================================
// ModificationType Tests
// =============================================================================

TEST(modification_type_values) {
    ASSERT_EQ(static_cast<std::uint8_t>(ModificationType::Cleared), 0);
    ASSERT_EQ(static_cast<std::uint8_t>(ModificationType::Leveled), 1);
    ASSERT_EQ(static_cast<std::uint8_t>(ModificationType::Terraformed), 2);
    ASSERT_EQ(static_cast<std::uint8_t>(ModificationType::Generated), 3);
    ASSERT_EQ(static_cast<std::uint8_t>(ModificationType::SeaLevelChanged), 4);
}

TEST(modification_type_count) {
    ASSERT_EQ(MODIFICATION_TYPE_COUNT, 5);
}

TEST(modification_type_size) {
    ASSERT_EQ(sizeof(ModificationType), 1);
}

TEST(modification_type_validation) {
    for (std::uint8_t i = 0; i < 5; ++i) {
        ASSERT(isValidModificationType(i));
    }
    ASSERT(!isValidModificationType(5));
    ASSERT(!isValidModificationType(255));
}

// =============================================================================
// TerrainModifiedEvent Tests
// =============================================================================

TEST(terrain_modified_event_default) {
    TerrainModifiedEvent event;
    ASSERT(event.affected_area.isEmpty());
}

TEST(terrain_modified_event_with_area) {
    GridRect area = GridRect::fromCorners(10, 20, 30, 40);
    TerrainModifiedEvent event(area, ModificationType::Terraformed);

    ASSERT_EQ(event.affected_area.x, 10);
    ASSERT_EQ(event.affected_area.y, 20);
    ASSERT_EQ(event.affected_area.width, 20);
    ASSERT_EQ(event.affected_area.height, 20);
    ASSERT_EQ(event.modification_type, ModificationType::Terraformed);
}

TEST(terrain_modified_event_single_tile) {
    TerrainModifiedEvent event(100, 200, ModificationType::Cleared);

    ASSERT_EQ(event.affected_area.x, 100);
    ASSERT_EQ(event.affected_area.y, 200);
    ASSERT_EQ(event.affected_area.width, 1);
    ASSERT_EQ(event.affected_area.height, 1);
    ASSERT_EQ(event.modification_type, ModificationType::Cleared);
}

TEST(terrain_modified_event_size) {
    ASSERT_EQ(sizeof(TerrainModifiedEvent), 12);
}

TEST(terrain_modified_event_trivially_copyable) {
    ASSERT(std::is_trivially_copyable<TerrainModifiedEvent>::value);
}

// =============================================================================
// ChunkDirtyTracker Initialization Tests
// =============================================================================

TEST(chunk_dirty_tracker_default_construction) {
    ChunkDirtyTracker tracker;
    ASSERT(!tracker.isInitialized());
    ASSERT_EQ(tracker.getChunksX(), 0);
    ASSERT_EQ(tracker.getChunksY(), 0);
}

TEST(chunk_dirty_tracker_initialized_construction) {
    ChunkDirtyTracker tracker(512, 512);
    ASSERT(tracker.isInitialized());
    ASSERT_EQ(tracker.getMapWidth(), 512);
    ASSERT_EQ(tracker.getMapHeight(), 512);
    // 512 / 32 = 16 chunks in each direction
    ASSERT_EQ(tracker.getChunksX(), 16);
    ASSERT_EQ(tracker.getChunksY(), 16);
    ASSERT_EQ(tracker.getTotalChunks(), 256);
}

TEST(chunk_grid_size_small_map) {
    ChunkDirtyTracker tracker(128, 128);
    // 128 / 32 = 4 chunks in each direction
    ASSERT_EQ(tracker.getChunksX(), 4);
    ASSERT_EQ(tracker.getChunksY(), 4);
    ASSERT_EQ(tracker.getTotalChunks(), 16);
}

TEST(chunk_grid_size_medium_map) {
    ChunkDirtyTracker tracker(256, 256);
    // 256 / 32 = 8 chunks in each direction
    ASSERT_EQ(tracker.getChunksX(), 8);
    ASSERT_EQ(tracker.getChunksY(), 8);
    ASSERT_EQ(tracker.getTotalChunks(), 64);
}

TEST(chunk_grid_size_non_multiple) {
    // 100 / 32 = 3.125 -> 4 chunks (ceiling)
    ChunkDirtyTracker tracker(100, 100);
    ASSERT_EQ(tracker.getChunksX(), 4);
    ASSERT_EQ(tracker.getChunksY(), 4);
}

TEST(chunk_grid_size_asymmetric) {
    ChunkDirtyTracker tracker(512, 256);
    ASSERT_EQ(tracker.getChunksX(), 16);
    ASSERT_EQ(tracker.getChunksY(), 8);
    ASSERT_EQ(tracker.getTotalChunks(), 128);
}

TEST(chunk_dirty_tracker_reinitialize) {
    ChunkDirtyTracker tracker(128, 128);
    tracker.markAllDirty();
    ASSERT(tracker.hasAnyDirty());

    // Reinitialize with different size - should clear all dirty
    tracker.initialize(256, 256);
    ASSERT(!tracker.hasAnyDirty());
    ASSERT_EQ(tracker.getChunksX(), 8);
    ASSERT_EQ(tracker.getChunksY(), 8);
}

// =============================================================================
// Chunk Dirty Flag Operations Tests
// =============================================================================

TEST(mark_chunk_dirty_basic) {
    ChunkDirtyTracker tracker(512, 512);

    ASSERT(!tracker.isChunkDirty(0, 0));
    ASSERT(tracker.markChunkDirty(0, 0));
    ASSERT(tracker.isChunkDirty(0, 0));
}

TEST(mark_chunk_dirty_multiple) {
    ChunkDirtyTracker tracker(512, 512);

    ASSERT(tracker.markChunkDirty(0, 0));
    ASSERT(tracker.markChunkDirty(5, 5));
    ASSERT(tracker.markChunkDirty(15, 15));

    ASSERT(tracker.isChunkDirty(0, 0));
    ASSERT(tracker.isChunkDirty(5, 5));
    ASSERT(tracker.isChunkDirty(15, 15));
    ASSERT(!tracker.isChunkDirty(1, 1));  // Not marked
}

TEST(mark_chunk_dirty_idempotent) {
    ChunkDirtyTracker tracker(512, 512);

    ASSERT(tracker.markChunkDirty(0, 0));
    ASSERT_EQ(tracker.countDirty(), 1);

    // Marking again should not increase count
    ASSERT(tracker.markChunkDirty(0, 0));
    ASSERT_EQ(tracker.countDirty(), 1);
}

TEST(mark_chunk_dirty_out_of_bounds) {
    ChunkDirtyTracker tracker(512, 512);  // 16x16 chunks

    ASSERT(!tracker.markChunkDirty(16, 0));   // x out of bounds
    ASSERT(!tracker.markChunkDirty(0, 16));   // y out of bounds
    ASSERT(!tracker.markChunkDirty(100, 100)); // both out of bounds
}

TEST(is_chunk_dirty_out_of_bounds) {
    ChunkDirtyTracker tracker(512, 512);

    ASSERT(!tracker.isChunkDirty(16, 0));
    ASSERT(!tracker.isChunkDirty(0, 16));
}

TEST(clear_chunk_dirty_basic) {
    ChunkDirtyTracker tracker(512, 512);

    tracker.markChunkDirty(0, 0);
    ASSERT(tracker.isChunkDirty(0, 0));

    ASSERT(tracker.clearChunkDirty(0, 0));
    ASSERT(!tracker.isChunkDirty(0, 0));
}

TEST(clear_chunk_dirty_idempotent) {
    ChunkDirtyTracker tracker(512, 512);

    tracker.markChunkDirty(0, 0);
    ASSERT_EQ(tracker.countDirty(), 1);

    tracker.clearChunkDirty(0, 0);
    ASSERT_EQ(tracker.countDirty(), 0);

    // Clearing again should not go negative
    tracker.clearChunkDirty(0, 0);
    ASSERT_EQ(tracker.countDirty(), 0);
}

TEST(clear_chunk_dirty_out_of_bounds) {
    ChunkDirtyTracker tracker(512, 512);

    ASSERT(!tracker.clearChunkDirty(16, 0));
    ASSERT(!tracker.clearChunkDirty(0, 16));
}

// =============================================================================
// Tile to Chunk Conversion Tests
// =============================================================================

TEST(tile_to_chunk_origin) {
    std::uint16_t chunk_x, chunk_y;
    ChunkDirtyTracker::tileToChunk(0, 0, chunk_x, chunk_y);
    ASSERT_EQ(chunk_x, 0);
    ASSERT_EQ(chunk_y, 0);
}

TEST(tile_to_chunk_first_chunk_edge) {
    std::uint16_t chunk_x, chunk_y;
    ChunkDirtyTracker::tileToChunk(31, 31, chunk_x, chunk_y);
    ASSERT_EQ(chunk_x, 0);
    ASSERT_EQ(chunk_y, 0);
}

TEST(tile_to_chunk_second_chunk) {
    std::uint16_t chunk_x, chunk_y;
    ChunkDirtyTracker::tileToChunk(32, 32, chunk_x, chunk_y);
    ASSERT_EQ(chunk_x, 1);
    ASSERT_EQ(chunk_y, 1);
}

TEST(tile_to_chunk_various) {
    std::uint16_t chunk_x, chunk_y;

    // Tile (64, 64) should be in chunk (2, 2)
    ChunkDirtyTracker::tileToChunk(64, 64, chunk_x, chunk_y);
    ASSERT_EQ(chunk_x, 2);
    ASSERT_EQ(chunk_y, 2);

    // Tile (511, 511) should be in chunk (15, 15)
    ChunkDirtyTracker::tileToChunk(511, 511, chunk_x, chunk_y);
    ASSERT_EQ(chunk_x, 15);
    ASSERT_EQ(chunk_y, 15);

    // Tile (33, 65) should be in chunk (1, 2)
    ChunkDirtyTracker::tileToChunk(33, 65, chunk_x, chunk_y);
    ASSERT_EQ(chunk_x, 1);
    ASSERT_EQ(chunk_y, 2);
}

// =============================================================================
// Mark Tile Dirty Tests (Dirty Flag Propagation)
// =============================================================================

TEST(mark_tile_dirty_origin) {
    ChunkDirtyTracker tracker(512, 512);

    ASSERT(tracker.markTileDirty(0, 0));
    ASSERT(tracker.isChunkDirty(0, 0));
}

TEST(mark_tile_dirty_chunk_boundary) {
    ChunkDirtyTracker tracker(512, 512);

    // Tile 31 should still be in chunk 0
    ASSERT(tracker.markTileDirty(31, 31));
    ASSERT(tracker.isChunkDirty(0, 0));

    // Tile 32 should be in chunk 1
    ASSERT(tracker.markTileDirty(32, 32));
    ASSERT(tracker.isChunkDirty(1, 1));
}

TEST(mark_tile_dirty_out_of_bounds) {
    ChunkDirtyTracker tracker(512, 512);

    // Negative coordinates
    ASSERT(!tracker.markTileDirty(-1, 0));
    ASSERT(!tracker.markTileDirty(0, -1));

    // Beyond map bounds
    ASSERT(!tracker.markTileDirty(512, 0));
    ASSERT(!tracker.markTileDirty(0, 512));
}

TEST(mark_tile_dirty_edge_tile) {
    ChunkDirtyTracker tracker(512, 512);

    // Last valid tile (511, 511) should mark chunk (15, 15)
    ASSERT(tracker.markTileDirty(511, 511));
    ASSERT(tracker.isChunkDirty(15, 15));
}

// =============================================================================
// Mark Tiles Dirty (Rectangular Region) Tests
// =============================================================================

TEST(mark_tiles_dirty_single_tile) {
    ChunkDirtyTracker tracker(512, 512);

    GridRect rect = GridRect::singleTile(64, 64);
    std::uint32_t marked = tracker.markTilesDirty(rect);

    ASSERT_EQ(marked, 1);
    ASSERT(tracker.isChunkDirty(2, 2));
}

TEST(mark_tiles_dirty_within_chunk) {
    ChunkDirtyTracker tracker(512, 512);

    // Rectangle entirely within chunk (0, 0)
    GridRect rect = GridRect::fromCorners(5, 5, 20, 20);
    std::uint32_t marked = tracker.markTilesDirty(rect);

    ASSERT_EQ(marked, 1);
    ASSERT(tracker.isChunkDirty(0, 0));
}

TEST(mark_tiles_dirty_span_chunks) {
    ChunkDirtyTracker tracker(512, 512);

    // Rectangle spanning 2x2 chunks at the boundary
    GridRect rect = GridRect::fromCorners(30, 30, 34, 34);
    std::uint32_t marked = tracker.markTilesDirty(rect);

    ASSERT_EQ(marked, 4);  // 4 chunks
    ASSERT(tracker.isChunkDirty(0, 0));  // tiles 30-31
    ASSERT(tracker.isChunkDirty(1, 0));  // tiles 32-33
    ASSERT(tracker.isChunkDirty(0, 1));
    ASSERT(tracker.isChunkDirty(1, 1));
}

TEST(mark_tiles_dirty_large_region) {
    ChunkDirtyTracker tracker(512, 512);

    // Full map - should mark all 16x16 = 256 chunks
    GridRect rect = GridRect::fromCorners(0, 0, 512, 512);
    std::uint32_t marked = tracker.markTilesDirty(rect);

    ASSERT_EQ(marked, 256);
    ASSERT_EQ(tracker.countDirty(), 256);
}

TEST(mark_tiles_dirty_empty_rect) {
    ChunkDirtyTracker tracker(512, 512);

    GridRect rect;  // Empty by default
    std::uint32_t marked = tracker.markTilesDirty(rect);

    ASSERT_EQ(marked, 0);
    ASSERT(!tracker.hasAnyDirty());
}

TEST(mark_tiles_dirty_out_of_bounds) {
    ChunkDirtyTracker tracker(512, 512);

    // Rectangle entirely outside map
    GridRect rect = GridRect::fromCorners(600, 600, 700, 700);
    std::uint32_t marked = tracker.markTilesDirty(rect);

    ASSERT_EQ(marked, 0);
}

TEST(mark_tiles_dirty_partial_bounds) {
    ChunkDirtyTracker tracker(512, 512);

    // Rectangle partially inside map - should only mark valid chunks
    GridRect rect = GridRect::fromCorners(500, 500, 600, 600);
    std::uint32_t marked = tracker.markTilesDirty(rect);

    // Only tiles 500-511 are valid, which is in chunk 15
    ASSERT(marked >= 1);
    ASSERT(tracker.isChunkDirty(15, 15));
}

TEST(mark_tiles_dirty_negative_coords) {
    ChunkDirtyTracker tracker(512, 512);

    // Rectangle with negative start - should clamp to 0
    GridRect rect = GridRect::fromCorners(-10, -10, 40, 40);
    std::uint32_t marked = tracker.markTilesDirty(rect);

    // Should mark chunks covering 0-39 in both directions
    // Chunk 0: tiles 0-31
    // Chunk 1: tiles 32-39
    ASSERT(marked >= 2);  // At least 2x2 = 4 chunks
    ASSERT(tracker.isChunkDirty(0, 0));
    ASSERT(tracker.isChunkDirty(1, 1));
}

// =============================================================================
// Process Event Tests
// =============================================================================

TEST(process_event_basic) {
    ChunkDirtyTracker tracker(512, 512);

    TerrainModifiedEvent event(GridRect::fromCorners(10, 10, 20, 20),
                                ModificationType::Cleared);
    std::uint32_t marked = tracker.processEvent(event);

    ASSERT_EQ(marked, 1);
    ASSERT(tracker.isChunkDirty(0, 0));
}

TEST(process_event_generated) {
    ChunkDirtyTracker tracker(512, 512);

    // Full map generation
    TerrainModifiedEvent event(GridRect::fromCorners(0, 0, 512, 512),
                                ModificationType::Generated);
    std::uint32_t marked = tracker.processEvent(event);

    ASSERT_EQ(marked, 256);
    ASSERT_EQ(tracker.countDirty(), 256);
}

TEST(process_event_single_tile) {
    ChunkDirtyTracker tracker(512, 512);

    TerrainModifiedEvent event(100, 200, ModificationType::Leveled);
    std::uint32_t marked = tracker.processEvent(event);

    ASSERT_EQ(marked, 1);
    ASSERT(tracker.isChunkDirty(3, 6));  // 100/32=3, 200/32=6
}

// =============================================================================
// Bulk Operations Tests
// =============================================================================

TEST(mark_all_dirty) {
    ChunkDirtyTracker tracker(512, 512);

    tracker.markAllDirty();
    ASSERT_EQ(tracker.countDirty(), 256);
    ASSERT(tracker.hasAnyDirty());

    // Verify all chunks are dirty
    for (std::uint16_t y = 0; y < 16; ++y) {
        for (std::uint16_t x = 0; x < 16; ++x) {
            ASSERT(tracker.isChunkDirty(x, y));
        }
    }
}

TEST(clear_all_dirty) {
    ChunkDirtyTracker tracker(512, 512);

    tracker.markAllDirty();
    tracker.clearAllDirty();

    ASSERT_EQ(tracker.countDirty(), 0);
    ASSERT(!tracker.hasAnyDirty());

    // Verify all chunks are clean
    for (std::uint16_t y = 0; y < 16; ++y) {
        for (std::uint16_t x = 0; x < 16; ++x) {
            ASSERT(!tracker.isChunkDirty(x, y));
        }
    }
}

// =============================================================================
// Get Next Dirty Tests
// =============================================================================

TEST(get_next_dirty_none) {
    ChunkDirtyTracker tracker(512, 512);

    std::uint16_t chunk_x, chunk_y;
    ASSERT(!tracker.getNextDirty(chunk_x, chunk_y));
}

TEST(get_next_dirty_first) {
    ChunkDirtyTracker tracker(512, 512);

    tracker.markChunkDirty(5, 5);

    std::uint16_t chunk_x, chunk_y;
    ASSERT(tracker.getNextDirty(chunk_x, chunk_y));
    ASSERT_EQ(chunk_x, 5);
    ASSERT_EQ(chunk_y, 5);
}

TEST(get_next_dirty_order) {
    ChunkDirtyTracker tracker(512, 512);

    // Mark chunks in reverse order
    tracker.markChunkDirty(5, 5);
    tracker.markChunkDirty(2, 2);
    tracker.markChunkDirty(0, 0);

    // Should return first one in row-major order
    std::uint16_t chunk_x, chunk_y;
    ASSERT(tracker.getNextDirty(chunk_x, chunk_y));
    ASSERT_EQ(chunk_x, 0);
    ASSERT_EQ(chunk_y, 0);

    // Clear and get next
    tracker.clearChunkDirty(0, 0);
    ASSERT(tracker.getNextDirty(chunk_x, chunk_y));
    ASSERT_EQ(chunk_x, 2);
    ASSERT_EQ(chunk_y, 2);
}

// =============================================================================
// Count and Has Any Tests
// =============================================================================

TEST(count_dirty_empty) {
    ChunkDirtyTracker tracker(512, 512);
    ASSERT_EQ(tracker.countDirty(), 0);
    ASSERT(!tracker.hasAnyDirty());
}

TEST(count_dirty_incremental) {
    ChunkDirtyTracker tracker(512, 512);

    tracker.markChunkDirty(0, 0);
    ASSERT_EQ(tracker.countDirty(), 1);
    ASSERT(tracker.hasAnyDirty());

    tracker.markChunkDirty(1, 1);
    ASSERT_EQ(tracker.countDirty(), 2);

    tracker.clearChunkDirty(0, 0);
    ASSERT_EQ(tracker.countDirty(), 1);
    ASSERT(tracker.hasAnyDirty());

    tracker.clearChunkDirty(1, 1);
    ASSERT_EQ(tracker.countDirty(), 0);
    ASSERT(!tracker.hasAnyDirty());
}

// =============================================================================
// Chunk Size Constant Test
// =============================================================================

TEST(chunk_size_constant) {
    ASSERT_EQ(CHUNK_SIZE, 32);
}

// =============================================================================
// Main Entry Point
// =============================================================================

int main() {
    printf("=== TerrainEvents and ChunkDirtyTracker Unit Tests ===\n\n");

    // GridRect tests
    RUN_TEST(grid_rect_default_construction);
    RUN_TEST(grid_rect_single_tile);
    RUN_TEST(grid_rect_from_corners);
    RUN_TEST(grid_rect_from_corners_invalid);
    RUN_TEST(grid_rect_contains);
    RUN_TEST(grid_rect_right_bottom);
    RUN_TEST(grid_rect_equality);
    RUN_TEST(grid_rect_size);
    RUN_TEST(grid_rect_trivially_copyable);

    // ModificationType tests
    RUN_TEST(modification_type_values);
    RUN_TEST(modification_type_count);
    RUN_TEST(modification_type_size);
    RUN_TEST(modification_type_validation);

    // TerrainModifiedEvent tests
    RUN_TEST(terrain_modified_event_default);
    RUN_TEST(terrain_modified_event_with_area);
    RUN_TEST(terrain_modified_event_single_tile);
    RUN_TEST(terrain_modified_event_size);
    RUN_TEST(terrain_modified_event_trivially_copyable);

    // ChunkDirtyTracker initialization tests
    RUN_TEST(chunk_dirty_tracker_default_construction);
    RUN_TEST(chunk_dirty_tracker_initialized_construction);
    RUN_TEST(chunk_grid_size_small_map);
    RUN_TEST(chunk_grid_size_medium_map);
    RUN_TEST(chunk_grid_size_non_multiple);
    RUN_TEST(chunk_grid_size_asymmetric);
    RUN_TEST(chunk_dirty_tracker_reinitialize);

    // Chunk dirty flag operations tests
    RUN_TEST(mark_chunk_dirty_basic);
    RUN_TEST(mark_chunk_dirty_multiple);
    RUN_TEST(mark_chunk_dirty_idempotent);
    RUN_TEST(mark_chunk_dirty_out_of_bounds);
    RUN_TEST(is_chunk_dirty_out_of_bounds);
    RUN_TEST(clear_chunk_dirty_basic);
    RUN_TEST(clear_chunk_dirty_idempotent);
    RUN_TEST(clear_chunk_dirty_out_of_bounds);

    // Tile to chunk conversion tests
    RUN_TEST(tile_to_chunk_origin);
    RUN_TEST(tile_to_chunk_first_chunk_edge);
    RUN_TEST(tile_to_chunk_second_chunk);
    RUN_TEST(tile_to_chunk_various);

    // Mark tile dirty tests (dirty flag propagation)
    RUN_TEST(mark_tile_dirty_origin);
    RUN_TEST(mark_tile_dirty_chunk_boundary);
    RUN_TEST(mark_tile_dirty_out_of_bounds);
    RUN_TEST(mark_tile_dirty_edge_tile);

    // Mark tiles dirty (rectangular region) tests
    RUN_TEST(mark_tiles_dirty_single_tile);
    RUN_TEST(mark_tiles_dirty_within_chunk);
    RUN_TEST(mark_tiles_dirty_span_chunks);
    RUN_TEST(mark_tiles_dirty_large_region);
    RUN_TEST(mark_tiles_dirty_empty_rect);
    RUN_TEST(mark_tiles_dirty_out_of_bounds);
    RUN_TEST(mark_tiles_dirty_partial_bounds);
    RUN_TEST(mark_tiles_dirty_negative_coords);

    // Process event tests
    RUN_TEST(process_event_basic);
    RUN_TEST(process_event_generated);
    RUN_TEST(process_event_single_tile);

    // Bulk operations tests
    RUN_TEST(mark_all_dirty);
    RUN_TEST(clear_all_dirty);

    // Get next dirty tests
    RUN_TEST(get_next_dirty_none);
    RUN_TEST(get_next_dirty_first);
    RUN_TEST(get_next_dirty_order);

    // Count and has any tests
    RUN_TEST(count_dirty_empty);
    RUN_TEST(count_dirty_incremental);

    // Chunk size constant test
    RUN_TEST(chunk_size_constant);

    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
